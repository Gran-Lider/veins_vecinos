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


        // Solo nodo 0 envía el beacon GPRS a los 20s
        if (nodeIndex == 0) {
            cMessage* beaconGPRS = new cMessage("beaconGPRS");
            scheduleAt(simTime() + 20, beaconGPRS);
            std::cout << "[Nodo 0] Evento beaconGPRS programado para los 20s" << std::endl;
        }

        ////////////////////////////////////////////////////////////
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

    //std::cout << "[T=" << simTime() << "] Nodo " << myId << " recibió beacon del nodo "
      //        << bsm->getCurrentNodeAddress()
        //      << " (destino: " << bsm->getRecipientAddress()
          //    << ", TreeID: " << bsm->getTreeId() << ")\n";


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


//void TraCIDemo11p::onWSM(BaseFrame1609_4* frame)
//{
//    TraCIDemo11pMessage* wsm = check_and_cast<TraCIDemo11pMessage*>(frame);
//
//    findHost()->getDisplayString().setTagArg("i", 1, "green");
//
//    Coord cur_pos = mobility->getPositionAt(simTime()) ;
//
//
//    if (mobility->getRoadId()[0] != ':') traciVehicle->changeRoute(wsm->getDemoData(), 9999);
//
//    if (!sentMessage) {
//        sentMessage = true;
//
//        // repeat the received traffic update once in 2 seconds plus some random delay
//        wsm->setSenderAddress(myId);
//        wsm->setSerial(3);
//
//        scheduleAt(simTime() + 2 + uniform(0.01, 0.2), wsm->dup());
//    }
//}

void TraCIDemo11p::onWSM(BaseFrame1609_4* frame)
{
    TraCIDemo11pMessage* wsm = check_and_cast<TraCIDemo11pMessage*>(frame);

    int destinoFinal = 52;  // Nodo objetivo fijo
    Coord curPos = mobility->getPositionAt(simTime());

    std::string comando = "python3 -u calcular_siguiente_salto.py " + std::to_string(myId) + " " +
                          std::to_string(destinoFinal) + " " +
                          std::to_string(curPos.x) + " " + std::to_string(curPos.y);

    FILE* pipe = popen(comando.c_str(), "w");
    if (!pipe) {
        std::cerr << "Error al ejecutar el script Python" << std::endl;
        return;
    }


    // Enviar vecinos actuales
    BeaconList::beaconPtr curr = ListBeacon.getHead();
    while (curr != NULL) {
        std::string dir = "Desconocida";
        if (curr->lastCoord != curr->SenderCoord) {
            dir = calcularDireccion(curr->lastCoord, curr->SenderCoord);
        }

        fprintf(pipe, "ID:%d Coord:(%.2f,%.2f,0) Dir:%s\n",
                curr->idVehicle,
                curr->SenderCoord.x,
                curr->SenderCoord.y,
                dir.c_str());
        curr = curr->next;
    }

    // Leer respuesta
    char buffer[128];
    fgets(buffer, sizeof(buffer), pipe);
    int nextHop = atoi(buffer);
    pclose(pipe);

    if (nextHop != -1) {
        TraCIDemo11pMessage* nuevoBeacon = new TraCIDemo11pMessage("beaconGPRS");
        nuevoBeacon->addPar("destino") = destinoFinal;
        nuevoBeacon->setSenderAddress(myId);
        nuevoBeacon->setRecipientAddress(nextHop);
        sendDown(nuevoBeacon);
    } else {
        EV << "❌ Nodo " << myId << " no encontró un next hop válido." << endl;
    }
}



void TraCIDemo11p::handleSelfMsg(cMessage* msg)
{

/////////////////////ESTE CODIGO VALE PARA ELEGIR MI VECINO /////////////////////
//    if (simTime() > 20 && myId == 16 && !yaImprimiInfo) {
//        yaImprimiInfo = true;
//
//        FILE* pipe = popen("python3 -u calcular_siguiente_salto.py 22 52 2622.26 28.6916", "w");
//        if (!pipe) {
//            std::cerr << "Error al ejecutar el script Python" << std::endl;
//            return;
//        }
//
//        BeaconList::beaconPtr curr = ListBeacon.getHead();
//        while (curr != NULL) {
//            std::string dir = "Desconocida";
//            if (curr->lastCoord != curr->SenderCoord) {
//                dir = calcularDireccion(curr->lastCoord, curr->SenderCoord);
//            }
//
//            fprintf(pipe, "ID:%d Coord:(%.2f,%.2f,0) Dir:%s\n",
//                    curr->idVehicle,
//                    curr->SenderCoord.x,
//                    curr->SenderCoord.y,
//                    dir.c_str());
//
//            curr = curr->next;
//        }
//
//        pclose(pipe);  // Cerramos escritura (por ahora no esperamos respuesta)
//    }
    ////////////////////////////////////////////////////////////////////////////////////


    if (simTime() > 20 && myId == 16 && !yaImprimiInfo) {
        yaImprimiInfo = true;

        // 1. Ejecutar script para decidir next hop
        FILE* pipe = popen("python3 -u calcular_siguiente_salto.py 16 52 2622.26 28.6916", "w");
        if (!pipe) {
            std::cerr << "Error al ejecutar el script Python" << std::endl;
            return;
        }

        BeaconList::beaconPtr curr = ListBeacon.getHead();
        while (curr != NULL) {
            std::string dir = "Desconocida";
            if (curr->lastCoord != curr->SenderCoord) {
                dir = calcularDireccion(curr->lastCoord, curr->SenderCoord);
            }

            fprintf(pipe, "ID:%d Coord:(%.2f,%.2f,0) Dir:%s\n",
                    curr->idVehicle,
                    curr->SenderCoord.x,
                    curr->SenderCoord.y,
                    dir.c_str());
            curr = curr->next;
        }

        // Leer next hop
        char buffer[128];
        fgets(buffer, sizeof(buffer), pipe);
        int nextHop = atoi(buffer);  // ← convertir a int
        pclose(pipe);

        // 2. Crear el beacon y enviarlo
        if (nextHop != -1) {
            TraCIDemo11pMessage* bsm = new TraCIDemo11pMessage("beaconGPRS");
            bsm->addPar("destino") = 52;  // ← lo puedes cambiar dinámicamente
            bsm->setSenderAddress(myId);
            bsm->setRecipientAddress(nextHop);  // usar next hop
            sendDown(bsm);
        } else {
            std::cerr << "[ERROR] No se encontró next hop desde nodo " << myId << std::endl;
        }
    }



    if (TraCIDemo11pMessage* wsm = dynamic_cast<TraCIDemo11pMessage*>(msg)) {
        // send this message on the service channel until the counter is 3 or higher.
        // this code only runs when channel switching is enabled
        sendDown(wsm->dup());
        wsm->setSerial(wsm->getSerial() + 1);

        if (wsm->getSerial() >= 3) {
            // stop service advertisements
            stopService();
            delete (wsm);
        }
        else {
            scheduleAt(simTime() + 1, wsm);

        }
    }
    else {

        DemoBaseApplLayer::handleSelfMsg(msg);
    }



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

        // Puedes imprimirlo para verificar
    //imprime al mandar el mensaje
    //std::cout << "=== Coordenada del emisor ===" << std::endl;
    //std::cout << "Nodo: " << Table_Ngh_NodeId << " Posición: " << emisorCoord.str() << std::endl;
    ////////////////////////////////////////////////////////////////////////////////
    // En donde quieras imprimir (por ejemplo, en initialize o en PrintBeacons):
    int id = myId;  // O tu identificador real
    std::string nombreNodo = getParentModule()->getFullName();
    //std::cout << "Nodo con ID = " << id << " → " << nombreNodo << std::endl;
    ////////////////////////////////////////////////////////////////////////////////

    //Msg_RecipienAddress = bsm->getRecipientAddress();                  //cual es el destino del MSG????
    //Msg_DestinationAddress = bsm->getDestinationAddress();             // verifica la posible direccion de destino
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










