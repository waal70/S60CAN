//========================S80 MESSAGES
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
** 25-04-2016: Start for S80 messages
**
*
*
** RANT MODE: So those sneaky sneakers at Volvo try to confuse the hell out of us
** All is well and fine when there is an S60 with Extended CAN id's set
** But for the S80, well, it uses Standard CAN frames AND
** The VIDA log-file OMITS the significant data byte setting.
** For instance, oil temperature is a request like this: Ecu '7E0', Message '22D9DC'
** Ecu translates into CAN-id (11-bit), which is all well and fine, but the message...
** VIDA constructs thusly:
** 00,00,07,E0,22,D9,DC
** Whilst this is what it should be:
** 00,00,07,E0,03,22,D9,DC,00,00,00,00
** In short: count the number of meaningful bytes and insert this BEFORE sending the actual message
*/
#if TARGETS80 == 1
#include <Arduino.h>
#include <mcp2515.h>
#include "s60can.h"

#define KEEPALIVE_MSG 0
#define DPF_MSG 1
#define EGR_MSG 2
#define OIL_MSG 3
#define BOOST_MSG 4

    int LOOPBACKMODE = 0;
    unsigned long last_keepalive_msg;
    unsigned long keepalive_timeout; // timeout in 1/10 seconds. 0=keepalive messaging disabled

    unsigned long last_dpf_msg;
    unsigned long last_dpf_frequency; //frequency in 1/10 seconds. 0=diagnostic messaging disabled

    unsigned long last_egr_msg;
    unsigned long last_egr_frequency; //frequency in 1/10 seconds. 0=diagnostic messaging disabled

    unsigned long last_oil_msg;
    unsigned long last_oil_frequency; //frequency in 1/10 seconds. 0=diagnostic messaging disabled

    unsigned long last_boost_msg;
    unsigned long last_boost_frequency; //frequency in 1/10 seconds. 0=diagnostic messaging disabled

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void init_keepalive(int blnLBM) {
    LOOPBACKMODE = blnLBM;
    last_keepalive_msg=millis();
    keepalive_timeout = 40;
}

void init_monitoring(int blnLBM) {
    LOOPBACKMODE = blnLBM;
    // initialize the dpf monitoring message
    last_dpf_msg=millis();
    last_dpf_frequency = 35; //every 3.5 seconds
    
    // initialize the egr monitoring message
    last_egr_msg=last_dpf_msg;
    last_egr_frequency = 10; //every second

    // initialize the oil monitoring message
    last_oil_msg=last_dpf_msg;
    last_oil_frequency = 50; //every 5 seconds

    // initialize the boost monitoring message
    last_boost_msg=last_dpf_msg;
    last_boost_frequency = 5; //every half a second
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 1 = succes, 0 = fail
int isDPFMessage(tCAN * message) {

  // Ignore canid. Different cars may send different diagnostic id's
  // DPF-return message contains: CE 11 E6 01 96 xx yy 00. 11 E6 01 96 are relevant
  //loopback testing:
  return ((message->data[1] == 0x11) && (message->data[2] == 0xE6) && (message->data[3] == 0x01) && (message->data[4] == 0x96));

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 1 = succes, 0 = fail
int isEGRMessage(tCAN * message) {

  // Ignore canid. Different cars may send different diagnostic id's
  // DPF-return message contains: CE 11 E6 01 96 xx yy 00. 11 E6 01 96 are relevant
  //loopback testing:
 return ((message->data[1] == 0x11) && (message->data[2] == 0xE6) && (message->data[3] == 0x00) && (message->data[4] == 0x2C));

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 1 = succes, 0 = fail
int isOILMessage(tCAN * message) {

  // Ignore canid. Different cars may send different diagnostic id's
  // DPF-return message contains: CE 11 E6 00 ED xx yy 00. 11 E6 00 ED are relevant
  //loopback testing:
 return ((message->data[1] == 0x11) && (message->data[2] == 0xE6) && (message->data[3] == 0x00) && (message->data[4] == 0xED));

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 1 = succes, 0 = fail
int isBOOSTMessage(tCAN * message) {

  // Ignore canid. Different cars may send different diagnostic id's
  // DPF-return message contains: CE 11 E6 01 76 xx yy 00. 11 E6 01 76 are relevant
  //loopback testing:
  return ((message->data[1] == 0x11) && (message->data[2] == 0xE6) && (message->data[3] == 0x01) && (message->data[4] == 0x76));

}
 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
tCAN construct_CAN_msg(int msgType) {
  tCAN message;

//default things:
    message.id = 0x07e0;
    message.header.rtr = 0;
    message.header.length = 8;

  switch (msgType) {
    case KEEPALIVE_MSG:
      //000FFFFE D800000000000000
      //                 =0=   =1=   =2=   =3=   =4=   =5=   =6=   =7=
      char payload[8] = {0xD8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    break;
    case (DPF_MSG):
      // CD 11 E6 01 96 0F D9 00
      // CD 11 A6 01 96 01 00 00
      if (LOOPBACKMODE) {
        char payload[8]   = {0xCD, 0x11, 0xE6, 0x01, 0x96, 0x0F, 0xD9, 0x00};
      }
      else {
        char payload[8]   = {0xCD, 0x11, 0xA6, 0x01, 0x96, 0x01, 0x00, 0x00};
      }
    break;
    case (EGR_MSG):
      // CD 11 E6 00 2C 15 DB 00
      // CD 11 A6 00 2C 01 00 00
      if (LOOPBACKMODE) {
        char payload[8]   = {0xCD, 0x11, 0xE6, 0x00, 0x2C, 0x15, 0xDB, 0x00};
      }
      else {
        char payload[8]   = {0xCD, 0x11, 0xA6, 0x00, 0x2C, 0x01, 0x00, 0x00};
      }
    break;
    case (OIL_MSG):
      // CD 11 E6 00 ED 0E 3C 00
      // CD 11 A6 00 ED 01 00 00
      if (LOOPBACKMODE) {
        char payload[8]   = {0xCD, 0x11, 0xE6, 0x00, 0xED, 0x0E, 0x3C, 0x00};
      }
      else {
        char payload[8]   = {0xCD, 0x11, 0xA6, 0x00, 0xED, 0x01, 0x00, 0x00};
      }
    break;
    case (BOOST_MSG):
      // CD 11 E6 01 76 04 09 00
      // CD 11 A6 01 76 01 00 00
      if (LOOPBACKMODE) {
        char payload[8]   = {0xCD, 0x11, 0xE6, 0x01, 0x76, 0x04, 0x09, 0x00};
      }
      else {
        char payload[8]   = {0xCD, 0x11, 0xA6, 0x01, 0x76, 0x01, 0x00, 0x00};
      }      
    break;
    default:
    break;

    for (int i=0;i=7;i++)
        message.data[i] = payload[i];
   
    return message;
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void checksend_CAN_msgs() {

tCAN sendMessage;
    if (keepalive_timeout>0)
      if (millis()-last_keepalive_msg > keepalive_timeout *100)
        {
          sendMessage = construct_CAN_msg(KEEPALIVE_MSG);
          send_CAN_msg(&sendMessage);
          last_keepalive_msg = millis();
        }

    if (last_dpf_frequency>0)
      if (millis()-last_dpf_msg > last_dpf_frequency *100)
        {
          sendMessage = construct_CAN_msg(DPF_MSG);
          send_CAN_msg(&sendMessage);
          last_dpf_msg = millis();
        }
        
    if (last_egr_frequency>0)
      if (millis()-last_egr_msg > last_egr_frequency *100)
        {
          sendMessage = construct_CAN_msg(EGR_MSG);
          send_CAN_msg(&sendMessage);
          last_egr_msg = millis();
        }

    if (last_oil_frequency>0)
      if (millis()-last_oil_msg > last_oil_frequency *100)
        {
          sendMessage = construct_CAN_msg(OIL_MSG);
          send_CAN_msg(&sendMessage);
          last_oil_msg = millis();
        }
    
    if (last_boost_frequency>0)
      if (millis()-last_boost_msg > last_boost_frequency *100)
        {
          sendMessage = construct_CAN_msg(BOOST_MSG);
          send_CAN_msg(&sendMessage);
          last_boost_msg = millis();
        }



}
#endif

