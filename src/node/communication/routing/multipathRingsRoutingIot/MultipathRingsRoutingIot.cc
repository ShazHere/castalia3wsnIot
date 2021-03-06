/*******************************************************************************
 *  Copyright: National ICT Australia,  2007 - 2011                            *
 *  Developed at the ATP lab, Networked Systems research theme                 *
 *  Author(s): Athanassios Boulis, Yuriy Tselishchev                           *
 *  This file is distributed under the terms in the attached LICENSE file.     *
 *  If you do not find this file, copies can be found by writing to:           *
 *                                                                             *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia                *
 *      Attention:  License Inquiry.                                           *
 *                                                                             *  
 *******************************************************************************/

#include "MultipathRingsRoutingIot.h"
#include "Radio.h"

Define_Module(MultipathRingsRoutingIot);

void MultipathRingsRoutingIot::startup()
{
    isMobile = par("isMobile");
    duplicateRebroadcastAtNodeEnable = par("duplicateRebroadcastAtNodeEnable");
    iotIdsStartFrom = (int)par("iotIdsStartFrom");
	netSetupTimeout = (double)par("netSetupTimeout") / 1000.0;
	mpathRingsSetupFrameOverhead = par("mpathRingsSetupFrameOverhead");

	// check that the Application module used has the boolean parameter "isSink"
	cModule *appModule = getParentModule()->getParentModule()->getSubmodule("Application");
	if (appModule->hasPar("isSink"))
		isSink = appModule->par("isSink");
	else
		opp_error("\nMultiPathRings routing has to be used with an application that defines the parameter isSink");

	currentLevel = tmpLevel = isSink ? 0 : NO_LEVEL;
	currentSinkID = tmpSinkID = isSink ? self : NO_SINK;

	isConnected = (isSink) ? true : false;
	isScheduledNetSetupTimeout = false;
	currentSequenceNumber = 0;
	if (isSink)
		sendTopologySetupPacket();
	dataPacketRecord.clear();
	justReturned = false;
	dropPacketTimeout = 0.5;
	snBroadcastTimeout = 2;
	setTimer(DROP_DATAPACKET_TIMEOUT, dropPacketTimeout);
//	trace()<< " isMobile is " << isMobile;
}

bool MultipathRingsRoutingIot::sourceIsIot(int src) {
    if (src >= iotIdsStartFrom)
        return true;
    else
        return false;
}
void MultipathRingsRoutingIot::sendTopologySetupPacket()
{

	MultipathRingsRoutingPacket *setupPkt =
	    new MultipathRingsRoutingPacket("Multipath rings routing setup packet", NETWORK_LAYER_PACKET);
	setupPkt->setMultipathRingsRoutingPacketKind(MPRINGS_TOPOLOGY_SETUP_PACKET);
	setupPkt->setSource(SELF_NETWORK_ADDRESS);
	setupPkt->setDestination(BROADCAST_NETWORK_ADDRESS);
	setupPkt->setSinkID(currentSinkID);
	setupPkt->setSenderLevel(currentLevel);
	toMacLayer(setupPkt, BROADCAST_MAC_ADDRESS);

}

void MultipathRingsRoutingIot::sendControlMessage(multipathRingsRoutingControlDef kind)
{

	MultipathRingsRoutingControlMessage *ctrlMsg =
	    new MultipathRingsRoutingControlMessage("Multipath routing control message",NETWORK_CONTROL_MESSAGE);
	ctrlMsg->setMultipathRingsRoutingControlMessageKind(kind);
	ctrlMsg->setLevel(currentLevel);
	ctrlMsg->setSinkID(currentSinkID);
	toApplicationLayer(ctrlMsg);

}

void MultipathRingsRoutingIot::rebroadcastSamePacket(MultipathRingsRoutingPacket* netPacket) {
    MultipathRingsRoutingPacket* dupPacket =  netPacket->dup();
    toMacLayer(dupPacket, BROADCAST_MAC_ADDRESS);
}

//Ring level also changed according to current hop
void MultipathRingsRoutingIot::rebroadCastPacket(MultipathRingsRoutingPacket* netPacket) {
    MultipathRingsRoutingPacket* dupPacket = netPacket->dup();
    dupPacket->setSenderLevel(currentLevel);
    toMacLayer(dupPacket, BROADCAST_MAC_ADDRESS);
}

void MultipathRingsRoutingIot::timerFiredCallback(int index)
{
    if (!isMobile) //Means static SN
    {
        if (index == SN_BROADCAST_TIMEOUT ) {
            setTimer(SN_BROADCAST_TIMEOUT, snBroadcastTimeout);
            int size = (int)dataPacketRecord.size();
            for (int i = 0; i< size; i++) {
                //if (isNotDuplicatePacket(dataPacketRecord[i].dataPacket)) {
                    trace()<< "rebroadcasting the data packet from dataRecord src="
                            << dataPacketRecord[i].dataPacket->getSource()
                            << " seq=" << dataPacketRecord[i].dataPacket->getSequenceNumber();
                    rebroadCastPacket(dataPacketRecord[i].dataPacket);
                //}
            }
            dataPacketRecord.clear();
            return;
        }

        if (index != TOPOLOGY_SETUP_TIMEOUT)
            return;

        isScheduledNetSetupTimeout = false;
        if (tmpLevel == NO_LEVEL) {
            //trace()<< "still tmpLevel = NO_LEVEL";
            setTimer(TOPOLOGY_SETUP_TIMEOUT, netSetupTimeout);
            isScheduledNetSetupTimeout = true;
        } else if (currentLevel == NO_LEVEL) {
            //Broadcast to all nodes of currentLevel-1
            currentLevel = tmpLevel + 1;
            currentSinkID = tmpSinkID;
            if (!isConnected) {
                isConnected = true;
                sendControlMessage(MPRINGS_CONNECTED_TO_TREE);
                trace() << "Connected to " << currentSinkID << " at level " << currentLevel;
                if (!TXBuffer.empty())
                    processBufferedPacket();
            } else {
                trace()<< "trying to send reconnection message";
                sendControlMessage(MPRINGS_TREE_LEVEL_UPDATED);
                trace() << "Reconnected to " << currentSinkID << " at level " << currentLevel;
            }
            sendTopologySetupPacket();
        }

        tmpLevel = isSink ? 0 : NO_LEVEL;
        tmpSinkID = isSink ? self : NO_SINK;
    }
    else // means  mobile
    {
        switch (index) {
        case DROP_DATAPACKET_TIMEOUT: {
            //trace()<<"direction is " << getDirection() << getLocationText();
            if (directionCheckOk()){ //means the exact moment when IoT is changing direction
                int size = (int)dataPacketRecord.size();
                for (int i = 0; i< size; i++) {
                    trace()<< "drop now! loc=" << getLocationText() << "src="
                            << dataPacketRecord[i].dataPacket->getSource()
                            << " seq=" << dataPacketRecord[i].dataPacket->getSequenceNumber();;
                    rebroadcastSamePacket(dataPacketRecord[i].dataPacket);
                    //MultipathRingsRoutingPacket* dupPacket =  dataPacketRecord[i].dataPacket->dup();
                    //toMacLayer(dupPacket, 0);
                }
                printSome();
                dataPacketRecord.clear();
            }
            else ;
                //trace()<<"direction check failed";
            setTimer(DROP_DATAPACKET_TIMEOUT, dropPacketTimeout);
            break;
        }
        default:
            break;
        }//end switch(index)
    } // end else mobile
}

void MultipathRingsRoutingIot::processBufferedPacket()
{
	while (!TXBuffer.empty()) {
		toMacLayer(TXBuffer.front(), BROADCAST_MAC_ADDRESS);
		TXBuffer.pop();
	}
}

void MultipathRingsRoutingIot::fromApplicationLayer(cPacket * pkt, const char *destination)
{
    if (!isMobile)
    {
	string dst(destination);

	MultipathRingsRoutingPacket *netPacket =
	    new MultipathRingsRoutingPacket("Multipath rings routing data packet", NETWORK_LAYER_PACKET);
	netPacket->setMultipathRingsRoutingPacketKind(MPRINGS_DATA_PACKET);
	netPacket->setSource(SELF_NETWORK_ADDRESS);
	netPacket->setDestination(destination);
	netPacket->setSinkID(currentSinkID);
	netPacket->setSenderLevel(currentLevel);
	encapsulatePacket(netPacket, pkt);

	if (dst.compare(SINK_NETWORK_ADDRESS) == 0 || dst.compare(PARENT_NETWORK_ADDRESS) == 0) {
		netPacket->setSequenceNumber(currentSequenceNumber);
		currentSequenceNumber++;
		if (bufferPacket(netPacket)) {
			if (isConnected)
				processBufferedPacket();
			else
				sendControlMessage(MPRINGS_NOT_CONNECTED);
		} else {
			//Here we could send a control message to upper layer informing that our buffer is full
		}
	} else {		//++++ need to control flooding
		toMacLayer(netPacket, BROADCAST_MAC_ADDRESS);
	}
    }
    else // means  mobile
        {

        }
}



void MultipathRingsRoutingIot::fromMacLayer(cPacket * pkt, int macAddress, double rssi, double lqi)
{
	MultipathRingsRoutingPacket *netPacket = dynamic_cast <MultipathRingsRoutingPacket*>(pkt);
	if (!netPacket)
		return;
	if (!isMobile)
	{
	switch (netPacket->getMultipathRingsRoutingPacketKind()) {

		case MPRINGS_TOPOLOGY_SETUP_PACKET:{
			if (isSink)
				break;
			if (!isScheduledNetSetupTimeout) {
				isScheduledNetSetupTimeout = true;
				setTimer(TOPOLOGY_SETUP_TIMEOUT, netSetupTimeout);
				tmpLevel = NO_LEVEL;
				tmpSinkID = NO_SINK;
			}
			if (tmpLevel == NO_LEVEL || tmpLevel > netPacket->getSenderLevel()) {
				tmpLevel = netPacket->getSenderLevel();
				tmpSinkID = netPacket->getSinkID();
			}
			//trace()<< "tmpLevel = " << tmpLevel << " and tmpSinkID = " << tmpSinkID;
			break;
		}

		case MPRINGS_DATA_PACKET:{
			string dst(netPacket->getDestination());
			string src(netPacket->getSource());
			int senderLevel = netPacket->getSenderLevel();
			int sinkID = netPacket->getSinkID();
			if (dst.compare("0") == 0)
			    dst = SINK_NETWORK_ADDRESS;
			if (dst.compare(BROADCAST_NETWORK_ADDRESS) == 0 ||
					dst.compare(SELF_NETWORK_ADDRESS) == 0) {
			    trace()<<"WARN: Packet sent to node other than SINK";
			    assert(false);
				// We are not filtering packets that are sent to this node directly or to 
				// broadcast network address, making application layer responsible for them
				toApplicationLayer(pkt->decapsulate());

			} else if (dst.compare(SINK_NETWORK_ADDRESS) == 0) {
			    //trace()<<"received packet with dst.compare(SINK_NETWORK_ADDRESS) == 0";
				if (senderLevel == currentLevel + 1) {
					if (self == sinkID) {
						// Packet is for this node, if filter passes, forward it to application
						if (isNotDuplicatePacket(pkt)) {
						    trace()<<"reached SINK, src="<< src <<
						    " seqNO= "<< netPacket->getSequenceNumber()<<
						    " from hop" << macAddress;
						    toApplicationLayer(decapsulatePacket(pkt));
						}
						else
							trace() << "Discarding duplicate src="<< src <<
                            " seqNO= "<< netPacket->getSequenceNumber()<<
                            " from hop" << macAddress;
					} else if (sinkID == currentSinkID) {
						// We want to rebroadcast this packet since we are not its destination
						// For this, a copy of the packet is created and sender level field is 
						// updated before calling toMacLayer() function
					    if (false == duplicateRebroadcastAtNodeEnable) {
					        //rebroadCastPacket(netPacket); add packet in list to broadcast a bit latter
					        //if sender is IoT object, then ignore packet (since added in duplicate record)
					        //coz IoT will already deliver it towards sink
					        if (!sourceIsIot(macAddress)) {
					            trace()<< "Sender not IoT, so ..." ;
					            if (isNotDuplicatePacket(pkt)) {
					                trace()<< "adding packet, hop=" << macAddress << " src="<< src
	                                        << " packet#" <<  netPacket->getSequenceNumber();
					                addDataPacketRecord(netPacket);
					                setTimer(SN_BROADCAST_TIMEOUT, snBroadcastTimeout);
					            }
					            else
					                trace()<<" Found a duplicate Packet, not adding and ignoring";
					        }
					        else {
					            trace()<<"sender is IOT, hop-id="<<macAddress;
					            if (deleteDataPacketRecord (netPacket))
					                ;//trace()<< "packet deleted";
					        }
					    }
					    else
					        rebroadCastPacket(netPacket);
					}
				}

			} else if (dst.compare(PARENT_NETWORK_ADDRESS) == 0) {
			    trace()<<"WARN: received packet with dst.compare(PARENT_NETWORK_ADDRESS) == 0";
			    assert(false);
				if (senderLevel == currentLevel + 1 && sinkID == currentSinkID) {
					// Packet is for this node, if filter passes, forward it to application
					if (isNotDuplicatePacket(pkt))
						toApplicationLayer(decapsulatePacket(pkt));
					else
						trace() << "Discarding duplicate packet from " << src << " lasthop= " << macAddress;
				}
			}
			else {//for debugging purpose
			    trace()<<"WARN: NO if true dst is " << dst << " sink network address is " << SINK_NETWORK_ADDRESS;
			    assert(false);
			}
			break;
		}
	}
    } //end if (!isMobile)
    else  if (towardsSink())//means mobile node and moving towards sink
    {
        switch (netPacket->getMultipathRingsRoutingPacketKind()){
            case MPRINGS_TOPOLOGY_SETUP_PACKET:{
                break;
            }
            case MPRINGS_DATA_PACKET:{
                string dst(netPacket->getDestination());
                string src(netPacket->getSource());
                int senderLevel = netPacket->getSenderLevel();
                int sinkID = netPacket->getSinkID();
                trace()<<"mobile: received packet# " << netPacket->getSequenceNumber()
                        << " src="<< src<<" hop "<< macAddress  ;
                if(isNotDuplicatePacket(netPacket)) {
                    addDataPacketRecord(netPacket);
                    trace()<<"rebroadcast by IoT to avoid further propagation";
                    rebroadcastSamePacket(netPacket); // for the sake that neighbours of current node donot boradcast it again
                }
                else
                    trace()<<"discarding packet# " << netPacket->getSequenceNumber() << " from " << src<< " lasthop= " << macAddress;
            }
        } //end Switch
    }//end if (towardsSink())
    else  //mobile node and moving away from sink
          trace()<< "MOVING AWAY FROM SINK";
}
/**
 * return True if Iot object should drop messages considering its direction.
 */

void MultipathRingsRoutingIot::addDataPacketRecord(MultipathRingsRoutingPacket *dataPacket) {
    DataPacketRecord dpr;
    MultipathRingsRoutingPacket *mrrp =dataPacket->dup();
    dpr.dataPacket = mrrp;
    dataPacketRecord.push_back(dpr);
   // int size = (int)dataPacketRecord.size();
        //trace()<< "size of dataPacketRecord = " << size;
}

bool MultipathRingsRoutingIot::deleteDataPacketRecord (MultipathRingsRoutingPacket *dataPacket) {
    int src = resolveNetworkAddress(dataPacket->getSource());
    unsigned int seq = dataPacket->getSequenceNumber();
    int size = dataPacketRecord.size();
    trace()<< "deleteable? src=" << src<< " seq=" << seq<< " sizeRecord=" << size;
    int removeIndex = -1;
    for( int i = 0; i< size; i++) {
        trace ()<< "src = " << src << " resolve = "
                << resolveNetworkAddress(dataPacketRecord[i].dataPacket->getSource())
                << " seq = " << seq << " datapacket[i].getSeq= "
                << dataPacketRecord[i].dataPacket->getSequenceNumber();
        if (resolveNetworkAddress(dataPacketRecord[i].dataPacket->getSource()) == src
                && dataPacketRecord[i].dataPacket->getSequenceNumber() == seq)
            removeIndex = i;
    }
    if (removeIndex >=0) {
        dataPacketRecord.erase(dataPacketRecord.begin() + removeIndex);
        size = dataPacketRecord.size();
        trace ()<<"removed packet src="<< src << " seq# " << seq << " sizeRecord=" << size ;
        return true;
    }
    else
        return false;
}

bool MultipathRingsRoutingIot::directionCheckOk ()
{
    bool direction = getDirection();
    if (1 == direction ) {//0 means going away from sink
        justReturned = false;
        return false;
    }
    else{ // direction == 0
        if (false == justReturned) {
            justReturned = true;
            return true;
        }
        else
            return false;
    }
}
bool MultipathRingsRoutingIot::towardsSink() {
    bool direction = getDirection();
    if (1 == direction ) {//0 means going away from sink
        return true;
    }
    else
        return false; //means going away from sink
}
bool MultipathRingsRoutingIot::getDirection(){
    bool direction = getMobilityModule()->getDirection();
    //trace()<< " direction is " << direction;
    return direction;
}
LineMobilityManager* MultipathRingsRoutingIot::getMobilityModule ()
{
    cModule *selfNode = getParentModule()->getParentModule();
    // get the name of module by printing module.getName at application layer
    LineMobilityManager *mobility = ((check_and_cast<LineMobilityManager*>)(selfNode->getSubmodule("MobilityManager")));
    return mobility;
}
string MultipathRingsRoutingIot::getLocationText() {
    std::string str;
   /* str =  " Location = (" + std::to_string((int)(floor(getMobilityModule()->getLocation().x))) + ","
           + std::to_string((int)(floor(getMobilityModule()->getLocation().y))) + ")";
    */return str;
}

bool MultipathRingsRoutingIot::getIsMobile() {
    return isMobile;
}

void MultipathRingsRoutingIot::printSome() {
    cModule *selfNode = getParentModule()->getParentModule(); //(check_and_cast<Radio*>)(
    Radio *radioModule = (check_and_cast<Radio*>)(selfNode->getSubmodule("Communication")->getSubmodule("Radio"));
    trace()<< "self radio state=" << radioModule->getStateText();

    int numNodes = getParentModule()->getParentModule()->getParentModule()->par("numNodes");
    cTopology *topo;    // temp variable to access packets received by other nodes
    topo = new cTopology("topo");
    topo->extractByNedTypeName(cStringTokenizer("node.Node").asVector());

    for (int i = 0; i < numNodes; i++) {
//            ThroughputTest *appModule = dynamic_cast<ThroughputTest*>
//                (topo->getNode(i)->getModule()->getSubmodule("Application"));
            Radio *radioModule = (check_and_cast<Radio*>)(topo->getNode(i)->getModule()->
                    getSubmodule("Communication")->getSubmodule("Radio"));
            trace()<< "radio " << i << " state= " << radioModule->getStateText();

        }
}
