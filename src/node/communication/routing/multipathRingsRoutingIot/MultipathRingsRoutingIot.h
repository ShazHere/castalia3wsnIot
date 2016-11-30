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

#ifndef _MULTIPATHRINGSROUTINGIOT_H_
#define _MULTIPATHRINGSROUTINGIOT_H_

#include <map>
#include "VirtualRouting.h"
#include "MultipathRingsRoutingPacket_m.h"
#include "MultipathRingsRoutingControl_m.h"
#include "LineMobilityManager.h"
#include "assert.h"
//#define NDEBUG //enable it to turn off debug mode and all assertions in current file


#define NO_LEVEL  -110
#define NO_SINK   -120

using namespace std;
struct DataPacketRecord {
  //  int receivedFrom;
    MultipathRingsRoutingPacket *dataPacket;
};
enum MultipathRingsRoutingTimers {
	TOPOLOGY_SETUP_TIMEOUT = 1,
	DROP_DATAPACKET_TIMEOUT = 2,
	SN_BROADCAST_TIMEOUT = 3,
};

class MultipathRingsRoutingIot: public VirtualRouting {
 private:
	int mpathRingsSetupFrameOverhead;	// in bytes
	double netSetupTimeout;
	double dropPacketTimeout;
	double snBroadcastTimeout;

	// multipathRingsRouting-related member variables
	int currentSequenceNumber;
	int currentSinkID;
	int currentLevel;
	int tmpSinkID;
	int tmpLevel;
	bool isSink;		//is a .ned file parameter of the Application module
	bool isConnected;	//attached under a parent node
	bool isScheduledNetSetupTimeout;

	bool justReturned; // for keeping track of mobility direction of Iot.
	//True means just took turn and should send datapackets
	// False means that either direction is towards sink or it has returned from quite some time.
	bool isMobile;
	bool duplicateRebroadcastAtNodeEnable; //see .ned file for description
	int iotIdsStartFrom;
	vector<DataPacketRecord> dataPacketRecord; //for storing packets by IoT and SN
	//Iot stores so that it sends down when direction changes, SN stores to create a delay for duplicate message re-broadcast
	bool towardsSink();
    void addDataPacketRecord(MultipathRingsRoutingPacket *);
    bool deleteDataPacketRecord (MultipathRingsRoutingPacket *dataPacket);
    bool getDirection();
    bool directionCheckOk ();
    LineMobilityManager* getMobilityModule ();
    string getLocationText();
    void rebroadCastPacket(MultipathRingsRoutingPacket* netPacket);
    void rebroadcastSamePacket(MultipathRingsRoutingPacket* netPacket);
    bool sourceIsIot(int src);

 protected:
	void startup();
	void fromApplicationLayer(cPacket *, const char *);
	void fromMacLayer(cPacket *, int, double, double);

	void sendTopologySetupPacket();
	void sendControlMessage(multipathRingsRoutingControlDef);

	void timerFiredCallback(int);
	void processBufferedPacket();
 public :
	bool getIsMobile();
};

#endif				//MULTIPATHRINGSROUTINGMODULE
