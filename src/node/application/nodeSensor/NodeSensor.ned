//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package node.application.nodeSensor;

simple NodeSensor like node.application.iApplication {
 parameters:
    int totalIotNodes = default (1); 
     
 	string applicationID = default ("wsnIot");//("nodeSensor"); //make it empty string for accepting everyapp packet
	bool collectTraceInfo = default (false);
	int priority = default (1);
	int packetHeaderOverhead = default (5); //5 in throughput //8 default
	int constantDataPayload = default (100);  //100 in throughtput //8 default
	double delayLimit = default (0); 			// application delay limit (0 - no limit)
											// discard packets arriving after the limit

	string nextRecipient = default ("0");	// Destination for packets received in this node. 
											// These parameters can be used to create an
											// application-level static routing. This way we can
											// have a multi-hop throughput test   
	int generateDataInterval = default (50);//in seconds
//inspired by throughtput app
	double packet_rate = default (0);	// packets per second, by default we transmit no packets
	double startupDelay = default (0);	// delay in seconds before the app stars producing packets
    bool isSink = default (false);//to check if it should work in NodeSensor
	
	double latencyHistogramMax = default (200);
	int latencyHistogramBuckets = default (10);

 gates:
 	output toCommunicationModule;
	output toSensorDeviceManager;
	input fromCommunicationModule;
	input fromSensorDeviceManager;
	input fromResourceManager;
}