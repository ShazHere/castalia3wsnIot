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

package node.communication.routing.multipathRingsRoutingIot;
//********************************************************************************
//*  Copyright: University                *
//*  Author(s): Shaza Hanif  *
//*                                                                              *
//*******************************************************************************/


simple MultipathRingsRoutingIot like node.communication.routing.iRouting {
 parameters:
	bool collectTraceInfo = default (false);
	int maxNetFrameSize = default (0);					// bytes, 0 means no limit
	int netBufferSize = default (32);					// number of messages
	int netDataFrameOverhead = default (14);			// bytes
	int mpathRingsSetupFrameOverhead = default (13);	// bytes
	int netSetupTimeout = default (50);					// msec
	
	bool isMobile = default (false); 					// For mobile IoT objects
	bool duplicateRebroadcastAtNodeEnable = default (true);	//by default (as in orginal protocol)
												// each node re-boradcast the packets it has seen 
														//if false it will not re-broadcast
//With IoT's it should be set false, so that when IoT informs that I am taking x packet, its not rebroadcasted
 gates:
	output toCommunicationModule;
	output toMacModule;
	input fromCommunicationModule;
	input fromMacModule;
	input fromCommModuleResourceMgr;
}