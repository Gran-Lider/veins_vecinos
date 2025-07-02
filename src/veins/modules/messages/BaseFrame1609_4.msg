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

import veins.base.utils.SimpleAddress;

namespace veins;

packet BaseFrame1609_4 {
    //Channel Number on which this packet was sent
    int channelNumber;
    //User priority with which this packet was sent (note the AC mapping rules in Mac1609_4::mapUserPriority)
    int userPriority = 7;
    //Unique number to identify the service
    int psid = 0;
    //Recipient of frame (-1 for any)
    LAddress::L2Type recipientAddress = -1;
    
    
    //====================================================================================== NEW FIELDS Beacon
    //My Address SimpleAddress Function
    LAddress::L2Type currentNodeAddress;
    //Sender Address -> node that originate the MSG
    LAddress::L2Type senderAddress = 0;
    // Destination Address -> Final destination of message
    LAddress::L2Type destinationAddress = 0;
    //NEXT HOP ADDRESSS TO INCLUDE IN BEACON 
    LAddress::L2Type NH_Address = -1;					 
    // NODE TYPE (VEHICLE OR RSU)
    string nodeType = "";
    double NodeDensity = 0;
    
}
