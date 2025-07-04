//
// Copyright (C) 2006-2011 Christoph Sommer <christoph.sommer@uibk.ac.at>
//
// Documentation for these modules is at http://veins.car2x.org/
//
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include "veins/modules/application/traci/TraCIDemo11p.h"
#include "veins/modules/application/utils/DirectionUtils.h"
#include "veins/modules/application/traci/TraCIDemo11pMessage_m.h"

Coord destCoord;      // posición fija de destino (52)
bool  destInitialized = false;

#include <fstream>  // std::ifstream
#include <cstdio>   // popen
#include <iostream>
#include <sstream>
#include "beacon.h"


using namespace veins;

Define_Module(veins::TraCIDemo11p);
//recuerdes la última posición de cada emisor para calcular su dirección la próxima
std::map<int, Coord> ultimaPosVecino;

void TraCIDemo11p::initialize(int stage)
{
    DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {

        std::cout << "✅ Nodo " << getParentModule()->getIndex() << " usando TraCIDemo11p" << std::endl;


        sentMessage = false;
        lastDroveAt = simTime();
        currentSubscribedServiceId = -1;
        ////////////////////agregado////////////////////
        // Nombre único por nodo (index)
        int nodeIndex = getParentModule()->getIndex();
        std::string filename = "nnt_node_" + std::to_string(nodeIndex) + ".txt";

        // Abre archivo de log
        neighborLog.open(filename, std::ios::out);

        // Mensaje inicial
        neighborLog << "=== TABLA DE VECINOS DEL NODO " << nodeIndex << " ===" << std::endl;

    }


}

void TraCIDemo11p::onWSA(DemoServiceAdvertisment* wsa)
{
    if (currentSubscribedServiceId == -1) {
        mac->changeServiceChannel(static_cast<Channel>(wsa->getTargetChannel()));
        currentSubscribedServiceId = wsa->getPsid();
        if (currentOfferedServiceId != wsa->getPsid()) {
            stopService();
            startService(static_cast<Channel>(wsa->getTargetChannel()), wsa->getPsid(), "Mirrored Traffic Service");
        }
    }
}

void TraCIDemo11p::onBSM(DemoSafetyMessage* bsm) {

    //Datos del nodo actual
    Coord current_Pos = mobility->getPositionAt(simTime());
    int current_node = myId;
    int nodeIndex = getParentModule()->getIndex();

    //Datos del beacon
    Coord senderPos = bsm->getIni_position();

    //Calculo distancia
    double distancia = current_Pos.distance(senderPos);

    if (!forwardMsg && distancia >= 3000) {
        forwardMsg = true;

        bsm->setDistancia(distancia);
        //sendDown(bsm->dup());    // !!!!Aqui su lǿgica de reenvio del beacaon
    }

    //=======================================  Actualiza Tabla de vecinos ==================================================
    PreprocessingPreparaBeaconValues(bsm);      // actualiza variable locales con campos del beacon para decidir si procesa el beacon recibido

    bool Msg_Processed = ListBeacon.SearchBeaconTreeID(Table_Ngh_Msg_TreeID);        //VERIFICA SI YA HE PROCESADO ESTE BEACON TREEID. EVITA GUARDAR BEACONS FORWARDED

    //===================== ACTUALIZA NNT  -- NODE  ==========================================================
    if(!Msg_Processed){
       UpdateNNT(bsm);
    }


}


void TraCIDemo11p::onWSM(BaseFrame1609_4* frame)
{

    if (auto* msg = dynamic_cast<TraCIDemo11pMessage*>(frame)) {
        std::cout << "📩 Nodo " << myId
                  << " recibió TraCIDemo11pMessage: "
                  << msg->getName()
                  << ", contenido: " << msg->getDemoData()
                  << std::endl;

        if (std::string(msg->getName()) == "beaconGPRS") {
            int destinoFinal = msg->par("destino").longValue();
            int prevHop      = msg->par("prevHop").longValue();      // ← extraemos prevHop
            double dx        = msg->par("destX").doubleValue();      // ← y el destX/destY
            double dy        = msg->par("destY").doubleValue();
            Coord destC(dx, dy, 0);

            std::cout << "📌 Destino objetivo: " << destinoFinal << std::endl;

            if (myId != destinoFinal) {
                std::cout << "🔁 Nodo " << myId
                          << " reenviando beacon (prevHop=" << prevHop
                          << ") hacia destino final desde posición: "
                          << mobility->getPositionAt(simTime()).str()
                          << std::endl;

                forwardToNextHop(
                    /*originId*/      myId,
                    /*destinationId*/ destinoFinal,
                    /*destCoord*/     destC,
                    /*prevHop*/       prevHop    // ← y aquí lo pasamos
                );
            }
            else {
                std::cout << "✅ Nodo " << myId
                          << " llegó a destino final." << std::endl;
            }
            return;
        }
    }
    else if (auto* bsm = dynamic_cast<DemoSafetyMessage*>(frame)) {
        onBSM(bsm);
    }

}



void TraCIDemo11p::handleSelfMsg(cMessage* msg)
{

 ///////////////////////////////////////////////////////////////////////
    // 1) En handleSelfMsg: disparar el bucle en t > 20s para nodo 16
    // Por ejemplo, para iniciar el forwarding tras 20s:

    // sólo una vez, en el nodo 16, guardo la posición fija del nodo 52

       if (simTime() > 20 && myId == 16 && !yaImprimiInfo) {
           yaImprimiInfo = true;

           destCoord = Coord(2622.26, 28.6916, 0);  // Asignación directa
           std::cout << "📌 Posición fija del nodo 52 = " << destCoord.str() << std::endl;


           forwardToNextHop(
               /*originId*/      16,
               /*destinationId*/ 52,
               /*destCoord*/     destCoord,
               /*prevHop=*/-1
           );
           return;
           std::cout << "✅ Nodo entro aqui N veces" << std::endl;
           return;
       }

       DemoBaseApplLayer::handleSelfMsg(msg);


}

void TraCIDemo11p::handlePositionUpdate(cObject* obj)
{
    DemoBaseApplLayer::handlePositionUpdate(obj);

    // stopped for for at least 10s?
    if (mobility->getSpeed() < 1) {
        if (simTime() - lastDroveAt >= 10 && sentMessage == false) {
            findHost()->getDisplayString().setTagArg("i", 1, "red");
            sentMessage = true;

            TraCIDemo11pMessage* wsm = new TraCIDemo11pMessage();
            populateWSM(wsm);
            wsm->setDemoData(mobility->getRoadId().c_str());

            // host is standing still due to crash
            if (dataOnSch) {
                startService(Channel::sch2, 42, "Traffic Information Service");
                // started service and server advertising, schedule message to self to send later
                scheduleAt(computeAsynchronousSendingTime(1, ChannelType::service), wsm);
            }
            else {
                // send right away on CCH, because channel switching is disabled
                sendDown(wsm);
            }
        }
    }
    else {
        lastDroveAt = simTime();
    }
}

void TraCIDemo11p::PreprocessingPreparaBeaconValues(DemoSafetyMessage* bsm) {
    //Obtengo del beacon ->
    Table_Ngh_NodeId = bsm->getCurrentNodeAddress();                   //MAC identificdor del beacon recibido
    Table_Ngh_Msg_TreeID = bsm->getTreeId();                           //busca el msg en la tabla de beacons


    // ✅ Nuevo: obtener coordenada del emisor
    Coord emisorCoord = bsm->getIni_position();


    ////////////////////////////////////////////////////////////////////////////////
    // En donde quieras imprimir (por ejemplo, en initialize o en PrintBeacons):
    int id = myId;  // O tu identificador real
    std::string nombreNodo = getParentModule()->getFullName();
    //std::cout << "Nodo con ID = " << id << " → " << nombreNodo << std::endl;
    ////////////////////////////////////////////////////////////////////////////////

}


void TraCIDemo11p::sendDirectedBeaconTo(int dstId) {
    DemoSafetyMessage* bsm = new DemoSafetyMessage();

    // Establecemos el destino del mensaje (MAC del nodo destino)
    populateWSM(bsm, dstId);  // Esto llenará el mensaje con info básica
    //bsm->setTreeId(1001);  // Puedes usar un ID único de mensaje o ruta
    bsm->setIni_position(curPosition); // Guarda la posición del nodo emisor

    std::cout << "[T=" << simTime() << "] Nodo " << myId
                  << " creó beacon dirigido a nodo " << dstId
                  << " con posición " << curPosition.str() << "\n";

    sendDown(bsm);  // Envía el mensaje al nivel inferior (MAC)
}

void TraCIDemo11p::handleLowerMsg(cMessage* msg) {
    BaseFrame1609_4* wsm = dynamic_cast<BaseFrame1609_4*>(msg);
    if (wsm) {
        onWSM(wsm);  // pasa a tu lógica
    } else {
        std::cout << "Mensaje desconocido: " << msg->getClassName() << std::endl;

        delete msg;
    }
}

void TraCIDemo11p::forwardToNextHop(int originId, int destinationId, const Coord& curPos,  int prevHop) {
    // 1) Llamada a Python con redirección de salida a archivo
   // std::cout << "=== Python recibió los vecinos ===" << std::endl;

    std::ostringstream cmd;
    cmd << "python3 -u calcular_siguiente_salto.py "
        << originId << " "
        << destinationId << " "
        << curPos.x << " " << curPos.y
        << " > nextHop.txt";
    FILE* pipe = popen(cmd.str().c_str(), "w");
    if (!pipe) {
        std::cout << "=== [ERROR] No pude abrir tubería a Python ===" << std::endl;
        return;
    }

    // 2) Escribir la tabla de vecinos
    BeaconList::beaconPtr curr = ListBeacon.getHead();
    while (curr) {
        std::string dir = "Desconocida";
        if (curr->lastCoord != curr->SenderCoord)
            dir = calcularDireccion(curr->lastCoord, curr->SenderCoord);
        EV << "[VECINO RECIBIDO] ID:" << curr->idVehicle
           << " Coord:" << curr->SenderCoord.str()
           << " Dir:" << dir << "\n";
        fprintf(pipe,
                "ID:%d Coord:(%.2f,%.2f,0) Dir:%s\n",
                curr->idVehicle,
                curr->SenderCoord.x,
                curr->SenderCoord.y,
                dir.c_str()
        );
        curr = curr->next;
    }
    pclose(pipe);

    // 3) Leer el nextHop desde el archivo
    std::ifstream in("nextHop.txt");
    if (!in.is_open()) {
        std::cout << "[ERROR] No pude abrir nextHop.txt" << std::endl;

        return;
    }
    int nextHop = -1;
    in >> nextHop;
    in.close();

    std::cout << "🚀 Nodo " << myId
              << " enviará beaconGPRS unicast al nextHop = "
              << nextHop << std::endl;


    // 4) Construir y enviar el mensaje
    if (nextHop != -1) {
        auto* wsm = new TraCIDemo11pMessage("beaconGPRS");
        wsm->setDemoData("hola2");
        wsm->addPar("destino") = destinationId;  // ✅ OK
        wsm->addPar("prevHop") = originId;       // ✅ AGREGAR ESTE
        wsm->addPar("destX") = curPos.x;         // ✅ Si usas esto en onWSM
        wsm->addPar("destY") = curPos.y;
        populateWSM(wsm, nextHop);
        sendDown(wsm);
    }
    else {
        std::cout << "[WARN] Python devolvió nextHop = -1" << std::endl;

    }
}










