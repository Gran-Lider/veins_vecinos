//
// Copyright (C) 2011 David Eckhoff <eckhoff@cs.fau.de>
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

#include "veins/modules/application/ieee80211p/DemoBaseApplLayer.h"
#include "veins/modules/application/utils/DirectionUtils.h"

//Agregado para la lectura de archivos
#include <fstream>
#include <sstream>
#include <map>
///

using namespace veins;

void DemoBaseApplLayer::initialize(int stage)
{
    BaseApplLayer::initialize(stage);

    if (stage == 0) {

        // initialize pointers to other modules
        if (FindModule<TraCIMobility*>::findSubModule(getParentModule())) {
            mobility = TraCIMobilityAccess().get(getParentModule());
            traci = mobility->getCommandInterface();
            traciVehicle = mobility->getVehicleCommandInterface();
        }
        else {
            traci = nullptr;
            mobility = nullptr;
            traciVehicle = nullptr;
        }

        annotations = AnnotationManagerAccess().getIfExists();
        ASSERT(annotations);

        mac = FindModule<DemoBaseApplLayerToMac1609_4Interface*>::findSubModule(getParentModule());
        ASSERT(mac);

        // read parameters
        headerLength = par("headerLength");
        sendBeacons = par("sendBeacons").boolValue();
        beaconLengthBits = par("beaconLengthBits");
        beaconUserPriority = par("beaconUserPriority");
        beaconInterval = par("beaconInterval");

        dataLengthBits = par("dataLengthBits");
        dataOnSch = par("dataOnSch").boolValue();
        dataUserPriority = par("dataUserPriority");

        wsaInterval = par("wsaInterval").doubleValue();
        currentOfferedServiceId = -1;

        isParked = false;

        findHost()->subscribe(BaseMobility::mobilityStateChangedSignal, this);
        findHost()->subscribe(TraCIMobility::parkingStateChangedSignal, this);

        sendBeaconEvt = new cMessage("beacon evt", SEND_BEACON_EVT);
        sendWSAEvt = new cMessage("wsa evt", SEND_WSA_EVT);

        generatedBSMs = 0;
        generatedWSAs = 0;
        generatedWSMs = 0;
        receivedBSMs = 0;
        receivedWSAs = 0;
        receivedWSMs = 0;


        //############################//beacon //#######################################//
        //#######################################################################################//
        nodeType = getParentModule()->getName();

    }
    else if (stage == 1) {

        // store MAC address for quick access
        myId = mac->getMACAddress();

        // simulate asynchronous channel access

        if (dataOnSch == true && !mac->isChannelSwitchingActive()) {
            dataOnSch = false;
            EV_ERROR << "App wants to send data on SCH but MAC doesn't use any SCH. Sending all data on CCH" << std::endl;
        }
        simtime_t firstBeacon = simTime();

        if (par("avoidBeaconSynchronization").boolValue() == true) {

            simtime_t randomOffset = dblrand() * beaconInterval;
            firstBeacon = simTime() + randomOffset;

            if (mac->isChannelSwitchingActive() == true) {
                if (beaconInterval.raw() % (mac->getSwitchingInterval().raw() * 2)) {
                    EV_ERROR << "The beacon interval (" << beaconInterval << ") is smaller than or not a multiple of  one synchronization interval (" << 2 * mac->getSwitchingInterval() << "). This means that beacons are generated during SCH intervals" << std::endl;
                }
                firstBeacon = computeAsynchronousSendingTime(beaconInterval, ChannelType::control);
            }

            if (sendBeacons) {
                scheduleAt(firstBeacon, sendBeaconEvt);
            }
        }
    }
}

simtime_t DemoBaseApplLayer::computeAsynchronousSendingTime(simtime_t interval, ChannelType chan)
{

    /*
     * avoid that periodic messages for one channel type are scheduled in the other channel interval
     * when alternate access is enabled in the MAC
     */

    simtime_t randomOffset = dblrand() * interval;
    simtime_t firstEvent;
    simtime_t switchingInterval = mac->getSwitchingInterval(); // usually 0.050s
    simtime_t nextCCH;

    /*
     * start event earliest in next CCH (or SCH) interval. For alignment, first find the next CCH interval
     * To find out next CCH, go back to start of current interval and add two or one intervals
     * depending on type of current interval
     */

    if (mac->isCurrentChannelCCH()) {
        nextCCH = simTime() - SimTime().setRaw(simTime().raw() % switchingInterval.raw()) + switchingInterval * 2;
    }
    else {
        nextCCH = simTime() - SimTime().setRaw(simTime().raw() % switchingInterval.raw()) + switchingInterval;
    }

    firstEvent = nextCCH + randomOffset;

    // check if firstEvent lies within the correct interval and, if not, move to previous interval

    if (firstEvent.raw() % (2 * switchingInterval.raw()) > switchingInterval.raw()) {
        // firstEvent is within a sch interval
        if (chan == ChannelType::control) firstEvent -= switchingInterval;
    }
    else {
        // firstEvent is within a cch interval, so adjust for SCH messages
        if (chan == ChannelType::service) firstEvent += switchingInterval;
    }

    return firstEvent;
}

void DemoBaseApplLayer::populateWSM(BaseFrame1609_4* wsm, LAddress::L2Type rcvId, int serial)
{
    wsm->setRecipientAddress(rcvId);
    wsm->setBitLength(headerLength);


    // =====================  actualizo campos del beacon
    wsm->setCurrentNodeAddress(myId); // store MAC address for quick access
    wsm->setNodeType(nodeType.c_str()); //NODO O RSU
    //wsm->setNodeDensity(NodeDensity()); //!!! Aqui su funcion para definir la densidad por ejemplo

    if (DemoSafetyMessage* bsm = dynamic_cast<DemoSafetyMessage*>(wsm)) {
        bsm->setSenderPos(curPosition);
        bsm->setSenderSpeed(curSpeed);
        bsm->setPsid(-1);
        bsm->setChannelNumber(static_cast<int>(Channel::cch));
        bsm->addBitLength(beaconLengthBits);
        //bsm->setIni_position(ini_position);
        bsm->setIni_position(curPosition);  // ✅ Funciona y es seguro



        wsm->setUserPriority(beaconUserPriority);
    }
    else if (DemoServiceAdvertisment* wsa = dynamic_cast<DemoServiceAdvertisment*>(wsm)) {
        wsa->setChannelNumber(static_cast<int>(Channel::cch));
        wsa->setTargetChannel(static_cast<int>(currentServiceChannel));
        wsa->setPsid(currentOfferedServiceId);
        wsa->setServiceDescription(currentServiceDescription.c_str());
    }
    else {
        if (dataOnSch)
            wsm->setChannelNumber(static_cast<int>(Channel::sch1)); // will be rewritten at Mac1609_4 to actual Service Channel. This is just so no controlInfo is needed
        else
            wsm->setChannelNumber(static_cast<int>(Channel::cch));
        wsm->addBitLength(dataLengthBits);
        wsm->setUserPriority(dataUserPriority);
    }
}

void DemoBaseApplLayer::receiveSignal(cComponent* source, simsignal_t signalID, cObject* obj, cObject* details)
{
    Enter_Method_Silent();
    if (signalID == BaseMobility::mobilityStateChangedSignal) {
        handlePositionUpdate(obj);
    }
    else if (signalID == TraCIMobility::parkingStateChangedSignal) {
        handleParkingUpdate(obj);
    }
}

void DemoBaseApplLayer::handlePositionUpdate(cObject* obj)
{
    ChannelMobilityPtrType const mobility = check_and_cast<ChannelMobilityPtrType>(obj);
    curPosition = mobility->getPositionAt(simTime());
    curSpeed = mobility->getCurrentSpeed();

    //Actualiza campo position node 0 solo una vez
    if(pos_updated && myId == 10){
           pos_updated = false;
           ini_position = curPosition;

    }
}

void DemoBaseApplLayer::handleParkingUpdate(cObject* obj)
{
    isParked = mobility->getParkingState();
}

void DemoBaseApplLayer::handleLowerMsg(cMessage* msg)
{

    BaseFrame1609_4* wsm = dynamic_cast<BaseFrame1609_4*>(msg);
    ASSERT(wsm);

    if (DemoSafetyMessage* bsm = dynamic_cast<DemoSafetyMessage*>(wsm)) {
        receivedBSMs++;

        onBSM(bsm);
    }
    else if (DemoServiceAdvertisment* wsa = dynamic_cast<DemoServiceAdvertisment*>(wsm)) {
        receivedWSAs++;
        onWSA(wsa);
    }
    else {
        receivedWSMs++;
        onWSM(wsm);
    }

    delete (msg);
}

void DemoBaseApplLayer::handleSelfMsg(cMessage* msg)
{
    switch (msg->getKind()) {
    case SEND_BEACON_EVT: {
        DemoSafetyMessage* bsm = new DemoSafetyMessage();
        populateWSM(bsm);
        sendDown(bsm);
        scheduleAt(simTime() + beaconInterval, sendBeaconEvt);
        break;
    }
    case SEND_WSA_EVT: {
        DemoServiceAdvertisment* wsa = new DemoServiceAdvertisment();
        populateWSM(wsa);
        sendDown(wsa);
        scheduleAt(simTime() + wsaInterval, sendWSAEvt);
        break;
    }
    default: {
        if (msg) EV_WARN << "APP: Error: Got Self Message of unknown kind! Name: " << msg->getName() << endl;
        break;
    }
    }
}

void DemoBaseApplLayer::finish()
{
    recordScalar("generatedWSMs", generatedWSMs);
    recordScalar("receivedWSMs", receivedWSMs);

    recordScalar("generatedBSMs", generatedBSMs);
    recordScalar("receivedBSMs", receivedBSMs);

    recordScalar("generatedWSAs", generatedWSAs);
    recordScalar("receivedWSAs", receivedWSAs);
}

DemoBaseApplLayer::~DemoBaseApplLayer()
{
    cancelAndDelete(sendBeaconEvt);
    cancelAndDelete(sendWSAEvt);
    findHost()->unsubscribe(BaseMobility::mobilityStateChangedSignal, this);
}

void DemoBaseApplLayer::startService(Channel channel, int serviceId, std::string serviceDescription)
{
    if (sendWSAEvt->isScheduled()) {
        throw cRuntimeError("Starting service although another service was already started");
    }

    mac->changeServiceChannel(channel);
    currentOfferedServiceId = serviceId;
    currentServiceChannel = channel;
    currentServiceDescription = serviceDescription;

    simtime_t wsaTime = computeAsynchronousSendingTime(wsaInterval, ChannelType::control);
    scheduleAt(wsaTime, sendWSAEvt);
}

void DemoBaseApplLayer::stopService()
{
    cancelEvent(sendWSAEvt);
    currentOfferedServiceId = -1;
}

void DemoBaseApplLayer::sendDown(cMessage* msg)
{
    checkAndTrackPacket(msg);
    BaseApplLayer::sendDown(msg);
}

void DemoBaseApplLayer::sendDelayedDown(cMessage* msg, simtime_t delay)
{
    checkAndTrackPacket(msg);
    BaseApplLayer::sendDelayedDown(msg, delay);
}

void DemoBaseApplLayer::checkAndTrackPacket(cMessage* msg)
{
    if (dynamic_cast<DemoSafetyMessage*>(msg)) {
        EV_TRACE << "sending down a BSM" << std::endl;
        generatedBSMs++;
    }
    else if (dynamic_cast<DemoServiceAdvertisment*>(msg)) {
        EV_TRACE << "sending down a WSA" << std::endl;
        generatedWSAs++;
    }
    else if (dynamic_cast<BaseFrame1609_4*>(msg)) {
        EV_TRACE << "sending down a wsm" << std::endl;
        generatedWSMs++;
    }
}

void DemoBaseApplLayer::PreparaBeaconValues(DemoSafetyMessage* bsm) {

    //Obtengo parametros del beacon ->
    Table_Ngh_NodeType = bsm->getNodeType();                                   //rsu or node
    Table_Ngh_Coord = bsm->getSenderPos();                                     //posicion del node que envia el beacon
    Table_Beacon_ArrivalTime =  bsm->getArrivalTime().dbl();                   //timepo de llegada del beacon

    if(nodeType != "rsu"){curPosition = mobility->getPositionAt(simTime());}   //actualiza posicion del nodo para calcular la distancia al RSU

}




void DemoBaseApplLayer::UpdateNNT(DemoSafetyMessage* bsm) {   //SE ACTUALIZA LA NNT con el beacon received


    PreparaBeaconValues(bsm);

    ///================================================= DENSITY ================================
    double b_NodeDensity = bsm->getNodeDensity();
    //============================================================================================

//    if(ListBeacon.SearchBeaconNodeID(Table_Ngh_NodeId)){
//        ListBeacon.UpdateBeacon(Table_Ngh_NodeType, Table_Ngh_Msg_TreeID ,Table_Ngh_NodeId,Table_Beacon_ArrivalTime, Table_Ngh_Coord, b_NodeDensity, NH_Dst_to_Dest);
//    }
//
//    else{
//        ListBeacon.AddBeacon(Table_Ngh_NodeType, Table_Ngh_Msg_TreeID ,Table_Ngh_NodeId,Table_Beacon_ArrivalTime, Table_Ngh_Coord, b_NodeDensity, NH_Dst_to_Dest);
//    }
    // Paso 1: Obtener la coordenada anterior ANTES de actualizar
    Coord coordAnterior;
    if (ListBeacon.SearchBeaconNodeID(Table_Ngh_NodeId)) {
        coordAnterior = ListBeacon.SearchBeaconCoord(Table_Ngh_NodeId);
    } else {
        coordAnterior = Table_Ngh_Coord;  // primera vez, sin movimiento
    }

    // Paso 2: Actualizar la tabla con los nuevos datos del beacon
    if (ListBeacon.SearchBeaconNodeID(Table_Ngh_NodeId)) {
        ListBeacon.UpdateBeacon(Table_Ngh_NodeType, Table_Ngh_Msg_TreeID, Table_Ngh_NodeId,
                                Table_Beacon_ArrivalTime, Table_Ngh_Coord, b_NodeDensity, NH_Dst_to_Dest);
    } else {
        ListBeacon.AddBeacon(Table_Ngh_NodeType, Table_Ngh_Msg_TreeID, Table_Ngh_NodeId,
                             Table_Beacon_ArrivalTime, Table_Ngh_Coord, b_NodeDensity, NH_Dst_to_Dest);
    }

    // Paso 3: Calcular la dirección correctamente
       std::string direccion = calcularDireccion(coordAnterior, Table_Ngh_Coord);



    //============================================================================================AGREGADO

    //std::cout << "Escribiendo en: " << filename << std::endl;

//    Coord coordAnterior = ListBeacon.SearchBeaconCoord(Table_Ngh_NodeId);
//    std::string direccion = calcularDireccion(coordAnterior, Table_Ngh_Coord);

//SOLO LA PARTE DE ABAJO SE MODIFICO
    // Paso 4: Imprimir en archivo
    std::ofstream file;
    std::string filename = "/home/pepe/Descargas/Neighbor table Veins-20250619/veins/out/neithbors/neighbors_node_" + std::to_string(myId) + ".txt";
    file.open(filename, std::ios::app);  // Abrir modo agregar


    file << "=== Beacon recibido por nodo " << myId << " ===" << std::endl;
    file << "De nodo: " << Table_Ngh_NodeId << " a tiempo: " << simTime() << std::endl;
    file << "Distancia: " << Table_Ngh_Coord.distance(curPosition) << " m" << std::endl;
    file << "Posición del emisor: " << Table_Ngh_Coord << std::endl;
    file << "Dirección estimada: " << direccion << std::endl;
    file << "DEBUG - Coord anterior usada: " << coordAnterior << std::endl;
    file << "DEBUG - Coord actual usada:   " << Table_Ngh_Coord << std::endl;


    file << "==========================================" << std::endl;

    file.close();

       // LA SIGUIENTE FUNCION COMENTADA SIRVE UNICAMENTE PARA RECOPILAR LOS ULTIMOS ARCHIVOS OBTENIDOS DE CADA NODO ES DECIR NO SE AGREGAN SOLO SE SOBREESCRIBEN
//       std::string filename = "/home/pepe/Descargas/Neighbor table Veins-20250619/veins/out/neithbors/neighbors_node_" + std::to_string(myId) + ".txt";
//       std::ifstream infile(filename);
//       std::map<int, std::string> registros;  // nodo_id → bloque de texto
//
//       std::string line, buffer;
//       int nodo_actual = -1;
//
//       while (std::getline(infile, line)) {
//           if (line.find("=== Beacon recibido por nodo") != std::string::npos) {
//               if (nodo_actual != -1) {
//                   registros[nodo_actual] = buffer;
//                   buffer.clear();
//               }
//               buffer = line + "\n";
//           } else if (line.find("De nodo:") != std::string::npos) {
//               std::istringstream iss(line);
//               std::string aux;
//               iss >> aux >> aux >> nodo_actual;  // Salta palabras hasta obtener el ID del nodo
//               buffer += line + "\n";
//           } else {
//               buffer += line + "\n";
//           }
//       }
//       if (nodo_actual != -1) registros[nodo_actual] = buffer;  // guardar el último bloque
//       infile.close();
//
//       // Paso 2: Reemplazar o agregar el nuevo
//       std::ostringstream nuevoRegistro;
//       nuevoRegistro << "=== Beacon recibido por nodo " << myId << " ===" << std::endl;
//       nuevoRegistro << "De nodo: " << Table_Ngh_NodeId << " a tiempo: " << simTime() << std::endl;
//       nuevoRegistro << "Distancia: " << Table_Ngh_Coord.distance(curPosition) << " m" << std::endl;
//       nuevoRegistro << "Posición del emisor: " << Table_Ngh_Coord << std::endl;
//       nuevoRegistro << "Dirección estimada: " << direccion << std::endl;
//       nuevoRegistro << "DEBUG - Coord anterior usada: " << coordAnterior << std::endl;
//       nuevoRegistro << "DEBUG - Coord actual usada:   " << Table_Ngh_Coord << std::endl;
//       nuevoRegistro << "==========================================" << std::endl;
//
//       registros[Table_Ngh_NodeId] = nuevoRegistro.str();

       // Paso 3: Reescribir el archivo completo
//       std::ofstream outfile(filename, std::ios::trunc);
//       for (const auto& par : registros) {
//           outfile << par.second;
//       }
//       outfile.close();
    //============================================================================================

    ListBeacon.PurgeBeacons(3);
    //ListBeacon.SortBeacons();
    ListBeacon.PrintBeacons(myId, "IN");

}
