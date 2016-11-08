/*
 * OldApp.cpp
 *
 *  Created on: Nov 1, 2016
 *      Author: shaza
 */

#include "OldApp.h"

Define_Module(OldApp);

void OldApp::startup()
{
    packet_rate = par("packet_rate");
    recipientAddress = par("nextRecipient").stringValue();
    recipientId = atoi(recipientAddress.c_str());
    startupDelay = par("startupDelay");
    delayLimit = par("delayLimit");
    packet_spacing = packet_rate > 0 ? 1 / float (packet_rate) : -1;
    dataSN = 0;

    numNodes = getParentModule()->getParentModule()->par("numNodes");
    packetsSent.clear();
    packetsReceived.clear();
    bytesReceived.clear();

    if (packet_spacing > 0 && recipientAddress.compare(SELF_NETWORK_ADDRESS) != 0)
        setTimer(SEND_PACKET, packet_spacing + startupDelay);
    declareOutput("Packets received per node");
}

void OldApp::fromNetworkLayer(ApplicationPacket * rcvPacket,
        const char *source, double rssi, double lqi)
{
    int sequenceNumber = rcvPacket->getSequenceNumber();
    int sourceId = atoi(source);
    trace() << "RRRRReceived packet #" << sequenceNumber << " from node " << source;
    // This node is the final recipient for the packet
    if (recipientAddress.compare(SELF_NETWORK_ADDRESS) == 0) {
        if (delayLimit == 0 || (simTime() - rcvPacket->getCreationTime()) <= delayLimit) {
            trace() << "Received packet #" << sequenceNumber << " from node " << source;
            collectOutput("Packets received per node", sourceId);
            packetsReceived[sourceId]++;
            bytesReceived[sourceId] += rcvPacket->getByteLength();
        } else {
            trace() << "Packet #" << sequenceNumber << " from node " << source <<
                " exceeded delay limit of " << delayLimit << "s";
        }
    // Packet has to be forwarded to the next hop recipient
    } else {
        trace()<< "forwarded to next recipient";
        ApplicationPacket* fwdPacket = rcvPacket->dup();
        // Reset the size of the packet, otherwise the app overhead will keep adding on
        fwdPacket->setByteLength(0);
        toNetworkLayer(fwdPacket, recipientAddress.c_str());
    }
}

void OldApp::timerFiredCallback(int index)
{
    switch (index) {
        case SEND_PACKET:{
            trace() << "Sending packet #" << dataSN;
            toNetworkLayer(createGenericDataPacket(0, dataSN), recipientAddress.c_str());
            packetsSent[recipientId]++;
            dataSN++;
            setTimer(SEND_PACKET, packet_spacing);
            break;
        }
    }
}

// This method processes a received carrier sense interupt. Used only for demo purposes
// in some simulations. Feel free to comment out the trace command.
void OldApp::handleRadioControlMessage(RadioControlMessage *radioMsg)
{
    switch (radioMsg->getRadioControlMessageKind()) {
        case CARRIER_SENSE_INTERRUPT:
            //trace() << "CS Interrupt received! current RSSI value is: " << radioModule->readRSSI();
                        break;
    }
}

void OldApp::finishSpecific() {
    declareOutput("Packets reception rate");
    declareOutput("Packets loss rate");

    cTopology *topo;    // temp variable to access packets received by other nodes
    topo = new cTopology("topo");
    topo->extractByNedTypeName(cStringTokenizer("node.Node").asVector());

    long bytesDelivered = 0;
    for (int i = 0; i < numNodes; i++) {
        OldApp *appModule = dynamic_cast<OldApp*>
            (topo->getNode(i)->getModule()->getSubmodule("Application"));
        //trace() << "value of i is " << i << " and self is " << self;
        if (appModule) {
            //trace() << " appModule True" << "value of i is " << i << " and self is " << self;
            int packetsSent = appModule->getPacketsSent(self);
            if (packetsSent > 0) { // this node sent us some packets
                trace() << " packetsSent > 0 ";
                float rate = (float)packetsReceived[i]/packetsSent;
                collectOutput("Packets reception rate", i, "total", rate);
                collectOutput("Packets loss rate", i, "total", 1-rate);
            }

            bytesDelivered += appModule->getBytesReceived(self);
        }
    }
    delete(topo);

    if (packet_rate > 0 && bytesDelivered > 0) {
        double energy = (resMgrModule->getSpentEnergy() * 1000000000)/(bytesDelivered * 8); //in nanojoules/bit
        declareOutput("Energy nJ/bit");
        collectOutput("Energy nJ/bit","",energy);
    }
}
