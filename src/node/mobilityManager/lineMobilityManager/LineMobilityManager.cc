/****************************************************************************
 *  Copyright: National ICT Australia,  2007 - 2010                         *
 *  Developed at the ATP lab, Networked Systems research theme              *
 *  Author(s): Yuriy Tselishchev                                            *
 *  This file is distributed under the terms in the attached LICENSE file.  *
 *  If you do not find this file, copies can be found by writing to:        *
 *                                                                          *
 *      NICTA, Locked Bag 9013, Alexandria, NSW 1435, Australia             *
 *      Attention:  License Inquiry.                                        *
 *                                                                          *  
 ****************************************************************************/

#include "LineMobilityManager.h"

Define_Module(LineMobilityManager);

void LineMobilityManager::initialize()
{
	VirtualMobilityManager::initialize();

	updateInterval = par("updateInterval");
	updateInterval = updateInterval / 1000;

	loc1_x = nodeLocation.x;
	loc1_y = nodeLocation.y;
	loc1_z = nodeLocation.z;
	loc2_x = par("xCoorDestination");
	loc2_y = par("yCoorDestination");
	loc2_z = par("zCoorDestination");
	speed = par("speed");
	distance = sqrt(pow(loc1_x - loc2_x, 2) + pow(loc1_y - loc2_y, 2) +
		 pow(loc1_z - loc2_z, 2));
	direction = 1;
	calculateIncrements();
	scheduleAt(simTime() + updateInterval,
	        new MobilityManagerMessage("Periodic location update message", MOBILITY_PERIODIC));
}

void LineMobilityManager::calculateIncrements() {
    if (speed > 0 && distance > 0) {
            double tmp = (distance / speed) / updateInterval;
            incr_x = (loc2_x - loc1_x) / tmp;
            incr_y = (loc2_y - loc1_y) / tmp;
            incr_z = (loc2_z - loc1_z) / tmp;
            setLocation(loc1_x, loc1_y, loc1_z);
            trace()<< "incr_x=" << incr_x << " incr_y=" << incr_y << " incr_z=" << incr_z << "direction=" << direction;
        }
}
void LineMobilityManager::handleMessage(cMessage * msg)
{
	int msgKind = msg->getKind();
	switch (msgKind) {

		case MOBILITY_PERIODIC:{
			if (direction) {
				nodeLocation.x += incr_x;
				nodeLocation.y += incr_y;
				nodeLocation.z += incr_z;
				if (   (incr_x > 0 && nodeLocation.x > loc2_x)
				    || (incr_x < 0 && nodeLocation.x < loc2_x)
				    || (incr_y > 0 && nodeLocation.y > loc2_y)
				    || (incr_y < 0 && nodeLocation.y < loc2_y)
				    || (incr_z > 0 && nodeLocation.z > loc2_z)
				    || (incr_z < 0 && nodeLocation.z < loc2_z)) {
					direction = 0;
					nodeLocation.x -= (nodeLocation.x - loc2_x) * 2;
					nodeLocation.y -= (nodeLocation.y - loc2_y) * 2;
					nodeLocation.z -= (nodeLocation.z - loc2_z) * 2;
					//speed = originalSpeed * 2;
					//calculateIncrements();
				}
			} else {
				nodeLocation.x -= incr_x;
				nodeLocation.y -= incr_y;
				nodeLocation.z -= incr_z;
				if (   (incr_x > 0 && nodeLocation.x < loc1_x)
				    || (incr_x < 0 && nodeLocation.x > loc1_x)
				    || (incr_y > 0 && nodeLocation.y < loc1_y)
				    || (incr_y < 0 && nodeLocation.y > loc1_y)
				    || (incr_z > 0 && nodeLocation.z < loc1_z)
				    || (incr_z < 0 && nodeLocation.z > loc1_z)) {
					direction = 1;
					nodeLocation.x -= (nodeLocation.x - loc1_x) * 2;
					nodeLocation.y -= (nodeLocation.y - loc1_y) * 2;
					nodeLocation.z -= (nodeLocation.z - loc1_z) * 2;
					//speed = originalSpeed;
					//calculateIncrements();
				}
			}
			notifyWirelessChannel();
			scheduleAt(simTime() + updateInterval,
				new MobilityManagerMessage("Periodic location update message", MOBILITY_PERIODIC));

			trace() << "changed location(x:y:z) to " << nodeLocation.x << 
					":" << nodeLocation.y << ":" << nodeLocation.z;
			break;
		}

		default:{
			trace() << "WARNING: Unexpected message " << msgKind;
		}
	}

	delete msg;
	msg = NULL;
}
/**
 * Author: Shaza
 * Date: 18/08/2016 - readded in Castalia 3.3 on 9/10/2016
 * Description: Made so that in application layer I can understand which direction the object
 * is moving. Accorkding to initilaization, initial direction 1. Later it changes to 0 when
 * object is moving back to initial location. So in my case I assume that object goes to and from
 * sink started from a far location towardss sink. In this way I can calculate if object is going closer
 * to sink or moving far away. => 1 means going closer to sink and 0 means moving far away.
 */
int LineMobilityManager::getDirection() {
    return direction;
}

/**
 * Check that return speed is twice as forward speed!
 * edited Date 14/12/2016
 * Author: Shaza
 */
double LineMobilityManager::getSpeed() {
    return speed;
}

