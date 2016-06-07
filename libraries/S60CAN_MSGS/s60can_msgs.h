#if !defined(__IN_ECLIPSE__)
/* S60 CAN  - Arduino firmware - version 0.5 alpha
**
** Copyright (c) 2015 André de Waal
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation, either version 3 of the License, or (at
** your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public
** License along with this program; if not, <http://www.gnu.org/licenses/>.
** Changelog:
** 24-08-2015: Start of dpfmonitor branch
** 26-08-2015: Start of EGRMonitor branch
** 08-09-2015: New display, 20x4
** 09-09-2015: Implemented oil and boost pressure readings
**
*/

#include "../UsbCAN/usbcan.h"
#include "../../s60can/s60can.h"

void init_keepalive(int blnLBM);
void init_monitoring(int blnLBM);
int isEGRMessage(tCAN * message);
int isDPFMessage(tCAN * message);
int isOILMessage(tCAN * message);
int isBOOSTMessage(tCAN * message);
void checksend_CAN_msgs();
void prepOILMessage(tCAN * message, char * msg);
void prepEGRMessage(tCAN * message, char * msg);
void prepBOOSTMessage(tCAN * message, char * msg);
void prepDPFMessage(tCAN * message, char * msg);
#endif
