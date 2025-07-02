#include "veins/modules/application/traci/beacon.h"
//=======================================================================================
//=======    TABLA DE BEACONS CREADA POR C/ IZA MODIFICADA POR PABLO ====================
//=======================================================================================
using namespace std;


BeaconList::BeaconList(){
    head=NULL;
    curr=NULL;
    temp=NULL;
}

//=======================================================================================
//ADD beacon to NNT
//=======================================================================================

void BeaconList::AddBeacon(std::string b_typeNode, int b_idMsg, int b_idVehicle, double b_time, Coord b_SenderCoord, double b_Nv,double NH_Dst_to_Dest){
    beaconPtr n = new beacon; //new means dynamic memory reservation
    n->next = NULL;
    n->typeNode = b_typeNode;
    n->idMsg = b_idMsg;
    n->idVehicle = b_idVehicle;
    n->time = b_time;
    n->SenderCoord = b_SenderCoord;
    n->Nv = b_Nv;
    n->NH_Dst_to_Dest = NH_Dst_to_Dest;

    if(head != NULL){
        curr = head;
        while(curr->next != NULL){curr = curr->next;}
        curr->next=n;
    }
    else{head=n;}
}

//=======================================================================================
//DELETE beacon nodeID
//=======================================================================================

void BeaconList::DeleteBeacon(int delb_idVehicle){
    beaconPtr delPtr = NULL;
    temp = head;
    curr = head;

   while(curr !=NULL && curr->idVehicle != delb_idVehicle){
        temp = curr;
        curr = curr->next;

    }

    if(curr == NULL){delete delPtr;}
    else{
        delPtr = curr;
        curr = curr->next;
        temp->next = curr;
        if(delPtr==head){
            head= head->next;
            temp = NULL;
        }
        delete delPtr;
    }
}

//=======================================================================================
//PRINT NNT de cada nodo
//=======================================================================================

void BeaconList::PrintBeacons(int CurrNodeAddress, string IN_OUT){
    curr = head;
    EV<<"------------------------------------------------------------------------------------------------------------------------------------------------------------------------"<<endl;
    EV<<setw(35)<<IN_OUT<<"  -->  Vehicle Neighbor Table -> CurrNodeAddress ="<<CurrNodeAddress<<endl;
    EV<<"------------------------------------------------------------------------------------------------------------------------------------------------------------------------"<<endl;
    EV<<"NodeType"<<setw(15)<<"TreeMsg_ID"<<setw(15)<<"MAC_Address"<<setw(15)<<"NH_Adress"<<setw(16)<<"NH_Dst_to_Dest"<<setw(16)<<"Density"<<setw(15)<<"ArrivalTime"<<endl;
    while(curr != NULL)
    {
        EV<<setw(5)<<curr->typeNode<<setw(15)<<curr->idMsg<<setw(15)<<curr->idVehicle<<setw(15)<<curr->NH_Address<<setw(15)<<curr->NH_Dst_to_Dest<<setw(15)<<curr->Nv<<setw(15)<<curr->time<<endl;
        curr = curr->next;
    }
    EV<<"------------------------------------------------------------------------------------------------------------------------------------------------------------------------"<<endl;
}

//=======================================================================================
//RETURN mac address at the top of NNT para luego ser configurada como receiver address ***NODO SELECCIONADO PARA EL REENVIO ***
//=======================================================================================

int BeaconList::TopTable(){
    int nodeid=0;
    curr = head;

    if (curr != NULL){nodeid = curr->idVehicle;}

    return nodeid;
}

//=======================================================================================
//RETURN densidad del top table vehicle
//=======================================================================================

double BeaconList::TopTableNv(){
    double nodedensity=0;
    curr = head;

    if (curr != NULL){nodedensity=curr->Nv;}

    return nodedensity;
}

//=======================================================================================
//RETURN densidad del top table vehicle
//=======================================================================================

Coord BeaconList::TopTableNghCoord(){
    Coord NghCoord;
    curr = head;

    if (curr != NULL){NghCoord=curr->SenderCoord;}

    return NghCoord;
}

//=======================================================================================
//RETURN top table U_DIST
//=======================================================================================

double BeaconList::TopTable_U_dist(){
    double U_dist=0;
    curr = head;

    if (curr != NULL){U_dist=curr->wdist;}

    return U_dist;
}

//=======================================================================================
//RETURN DSD value (distodestination -> distancia desde el que envia el beacon a la posicion del RSU.) at the top of the list
//=======================================================================================

double BeaconList::TopTabledisToDestination(){ ///
    double dsd=0;
    curr = head;

    if (curr != NULL){dsd=curr->dsd;}

    return dsd;
}

//=======================================================================================
//RETURN DSR  del primer nodo de la NNT (Distancia del nodo que recibe el beacon al nodo que origina (no el nodo que reenvia) el beacon)
//=======================================================================================

double BeaconList::topTableDtoS(){
    double dsr=0;
    curr = head;

    if (curr != NULL){dsr=curr->dsr;}

    return dsr;
}

//=======================================================================================
//RETURN cuenta del numbero de vecinos -> se usa para la densidad
//=======================================================================================

int BeaconList::CounterBeacons(){
    curr = head;
    int counter =0;

    while(curr != NULL)
    {
        curr = curr->next;
        counter++;
    }
    return counter;
}

//=======================================================================================
//RETURN true if the nodeID es encontrado en la NNT
//=======================================================================================

bool BeaconList::SearchBeaconNodeID(int NodeID){
    bool foundBeacon=false;
    beaconPtr delPtr = NULL;
    temp = head;
    curr = head;

    while(curr !=NULL && curr->idVehicle != NodeID){
        temp = curr;
        curr = curr->next;
    }

    if(curr != NULL){foundBeacon = true;}
    delete delPtr;
    return foundBeacon;
}

//=======================================================================================
//RETURN true if the treeID es encontrado en la NNT
//=======================================================================================

bool BeaconList::SearchBeaconTreeID(int treeID){
    bool foundBeacon = false;
    temp = head;
    curr = head;

    while(curr !=NULL && curr->idMsg != treeID ){
        temp = curr;
        curr = curr->next;
    }

    if(curr != NULL){foundBeacon = true;}
    return foundBeacon;
}

//=======================================================================================
//RETURN coord del nodeID enviado como parametro
//=======================================================================================

Coord BeaconList::SearchBeaconCoord(int NodeID){
    Coord NodeCoord;
    temp = head;
    curr = head;

    while(curr !=NULL && curr->idVehicle != NodeID ){
        temp = curr;
        curr = curr->next;
    }

    if(curr != NULL){NodeCoord = curr->SenderCoord;}
    return NodeCoord;
}

//=======================================================================================
//RETURN nodeID del nodo mas cercano al destino del packet. Devueleve 0 si no hay nodos en mi tabla
//=======================================================================================

int BeaconList::SearchClstNODE(Coord DstNodeCoord){
    int ClosestnodeID = 0;
    double mindistance = 4000;
    double currdistance = 0;
    Coord currNeighborNodeCoord;

    curr = head;
    while(curr != NULL) {
        if (curr->typeNode != "rsu"){
            currNeighborNodeCoord = curr->SenderCoord;
            currdistance = currNeighborNodeCoord.distance(DstNodeCoord);
            if(currdistance < mindistance){mindistance = currdistance;ClosestnodeID = curr->idVehicle;}
            curr = curr->next;
        }else{curr = curr->next;}
    }
    return ClosestnodeID;
}

//=======================================================================================
//RETURN RSUid ENCONTRADO en la NNT
//=======================================================================================

int BeaconList::SearchBeaconRSU(){
    int idRSU=0;
    temp = head;
    curr = head;

    while(curr !=NULL && curr->typeNode != "rsu"){
        temp = curr;
        curr = curr->next;
    }

    if(curr == NULL){idRSU = 0;}
    else{idRSU = curr->idVehicle;}
    return idRSU;
}

//=======================================================================================
//RETURN true if RSU is Found
//=======================================================================================

bool BeaconList::Search_RSU_NNT(){

    bool RSU_Found = false;
    temp = head;
    curr = head;

    while(curr !=NULL && curr->typeNode != "rsu"){
        temp = curr;
        curr = curr->next;
    }

    if(curr != NULL){RSU_Found = true;}
    return RSU_Found;
}

//=======================================================================================
//ACTUALIZA los campos del beacon que ya estaba en la tabla de vecinos
//=======================================================================================

void BeaconList::UpdateBeacon(std::string b_typeNode, int b_idMsg, int b_idVehicle, double b_time, Coord b_SenderCoord, double b_Nv,double NH_Dst_to_Dest){
    beaconPtr n = new beacon;
    n =head;
    while(n != NULL)
    {
        if(n->idVehicle == b_idVehicle)
        {
            n->typeNode = b_typeNode;
            n->idMsg = b_idMsg;
            n->idVehicle = b_idVehicle;
            n->time = b_time;
            n->SenderCoord = b_SenderCoord;
            n->Nv = b_Nv;
            n->NH_Dst_to_Dest = NH_Dst_to_Dest;
            return;
        }
        else
            n = n->next;
    }
}

//=======================================================================================
//Ordena las entradas de la NNT para luego seleccionar el primer vecino como candidato para el reenvio
//=======================================================================================

void BeaconList::SortBeacons(){//organize
   double GS_temp = 0;//means: global score temporal
   double GS_curr = 0;
   bool changeMade;
   do{
       changeMade = false;
       curr = head;
       while(curr != NULL && curr->next != NULL) {
           temp = curr;
           curr = curr->next;
           //MMMR DISTANCIA, BANDWIDTH, DENSIDAD
       //    GS_temp = ((temp->wdist) + (temp->Nv) + (temp->abe/6000000))*0.33;
       //    GS_curr = ((curr->wdist) + (curr->Nv) + (curr->abe/6000000))*0.33;
       //MMMR DISTACNIA DENSIDAD
           GS_temp = ((temp->wdist) + (temp->Nv))*0.5;
           GS_curr = ((curr->wdist) + (curr->Nv))*0.5;

           if(curr && GS_curr > GS_temp){//&&(curr->Nid!=0)){//---let id of node, 0=rsu, 1=node
                   changeMade = true;
                   swap(temp->typeNode, curr->typeNode);
                   swap(temp->idMsg, curr->idMsg );
                   swap(temp->idVehicle, curr->idVehicle);
                   swap(temp->time, curr->time );
                   swap(temp->SenderCoord, curr->SenderCoord);
                   swap(temp->dsr, curr->dsr);
                   swap(temp->dsd, curr->dsd);
                   swap(temp->drd, curr->drd);
                   swap(temp->Nv, curr->Nv);
                   swap(temp->wdist, curr->wdist);
                   swap(temp->abe, curr->abe);
                   swap(temp->NH_Address,curr->NH_Address);
                   swap(temp->NH_Dst_to_Dest,curr->NH_Dst_to_Dest);
                   swap(temp->NH_GS,curr->NH_GS);

           }
        }
   } while(changeMade);
}

//=======================================================================================
////BORRA ELEMENTOS OUTDATED DE LA NNT -> SI LA ENTRADA ES MENOR A 2 segundos por default
//=======================================================================================

void BeaconList::PurgeBeacons(double b_ttl){
    curr = head;
    double TimeExpited = simTime().dbl() - b_ttl ;
    // BORRA el primer elemento de la tabla si esta outdated
    if (curr != NULL){
        if (curr->time < TimeExpited){DeleteBeacon(curr->idVehicle);}
    }

    while(curr != NULL && curr->next != NULL){
           if(curr->next->time < TimeExpited){
              temp = curr->next;
              curr->next = temp->next;
              free(temp);
           }else{curr = curr->next;}
       }
}

//=======================================================================================
//RETURN top table GS
//=======================================================================================

double BeaconList::TopTableGS(){
    double T_GS=0;
    curr = head;

    if (curr != NULL){T_GS=curr->NH_GS;}

    return T_GS;
}
