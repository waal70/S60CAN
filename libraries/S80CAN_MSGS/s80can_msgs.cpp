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
** 00,00,07,E0 | 03,22,D9,DC,00,00,00,00
** In short: count the number of meaningful bytes and insert this BEFORE sending the actual message
*/
#include <Arduino.h>
#include <mcp2515.h>
#include "../../s60can/s60can.h"

#define KEEPALIVE_MSG 0
#define DPF_MSG 1 //DPF = KVL for S80
#define EGR_MSG 2 //EGR = TRX for S80
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
    keepalive_timeout = 40000;
}

void init_monitoring(int blnLBM) {
    LOOPBACKMODE = blnLBM;
    // initialize the dpf monitoring message
    last_dpf_msg=millis();
    last_dpf_frequency = 35; //every 3.5 seconds
    
    // initialize the egr monitoring message
    last_egr_msg=last_dpf_msg;
    last_egr_frequency = 10000; //every second

    // initialize the oil monitoring message
    last_oil_msg=last_dpf_msg;
    last_oil_frequency = 50000; //every 5 seconds

    // initialize the boost monitoring message
    last_boost_msg=last_dpf_msg;
    last_boost_frequency = 5000; //every half a second
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 1 = succes, 0 = fail
int isDPFMessage(tCAN * message) {

  // Ignore canid. Different cars may send different diagnostic id's
  // DPF-return message contains is COOLANT return message in S80
  // For S80: KVL-return message contains: 0x04,0x62,0xF4,0x05,0x61,0x00,0x00,0x00
  return ((message->data[1] == 0x62) && (message->data[2] == 0xF4) && (message->data[3] == 0x05));
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 1 = succes, 0 = fail
int isEGRMessage(tCAN * message) {

  // Ignore canid. Different cars may send different diagnostic id's
  // EGR-return message contains is TRANSMISSION message in S80
  // For S80: 0x05,0x62,0xD9,0x04,0x0D,0x33,0x00,0x00
  //loopback testing:
 return ((message->data[1] == 0x62) && (message->data[2] == 0xD9) && (message->data[3] == 0x04));

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 1 = succes, 0 = fail
int isOILMessage(tCAN * message) {

  // Ignore canid. Different cars may send different diagnostic id's
  //S80-OIL: Payload: 0x05,0x62,0xD9,0xDC,0x0D,0x33,0x00,0x00
 return ((message->data[1] == 0x62) && (message->data[2] == 0xD9) && (message->data[3] == 0xDC));

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 1 = succes, 0 = fail
int isBOOSTMessage(tCAN * message) {

  // Ignore canid. Different cars may send different diagnostic id's
  // S80-BOOST: Payload: 0x05,0x62,0xD9,0xE4,0x04,0x07,0x00,0x00
  return ((message->data[1] == 0x62) && (message->data[2] == 0xD9) && (message->data[3] == 0x04));

}
 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
tCAN construct_CAN_msg(int msgType) {
  tCAN message;

//Loopback-mode means sending the RETURN-message as the request, so
  // switch some fields around

//default things:
    message.id = 0x07e0;
    message.header.rtr = 0;
    message.length = 8;
    String payload ;
    payload = "default";

  switch (msgType) {
    case (KEEPALIVE_MSG):
	{
      //000FFFFE D800000000000000
      //                 =0=   =1=   =2=   =3=   =4=   =5=   =6=   =7=
      payload = "0xD8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00";
    break;
	}
    case (DPF_MSG):
		//DPF = KVL
      if (LOOPBACKMODE) {
        payload   = "0x04, 0x62, 0xF4, 0x05, 0x61, 0x00, 0x00, 0x00";
      }
      else {
        payload   = "0x03, 0x22, 0xF4, 0x05, 0x00, 0x00, 0x00, 0x00";
      }
    break;
    case (EGR_MSG):
      // EGR = TRX
      if (LOOPBACKMODE) {
        payload   = "0x05, 0x62, 0xD9, 0x04, 0x0D, 0x33, 0x00, 0x00";
      }
      else {
        payload   = "0x03, 0x22, 0xD9, 0x04, 0x00, 0x00, 0x00, 0x00";
      }
    break;
    case (OIL_MSG):
      if (LOOPBACKMODE) {
        payload   = "0x05, 0x62, 0xD9, 0xDC, 0x0D, 0x33, 0x00, 0x00";
      }
      else {
        payload   = "0x03, 0x22, 0xD9, 0xDC, 0x00, 0x00, 0x00, 0x00";
      }
    break;

    case (BOOST_MSG):

      if (LOOPBACKMODE) {
        payload   = "0x05, 0x62, 0xD9, 0xE4, 0x04, 0x07, 0x00, 0x00";
      }
      else {
        payload   = "0x03, 0x22, 0xD9, 0xE4, 0x00, 0x00, 0x00, 0x00";
      }      
    break;
    default:
    	payload   = "0x03, 0x22, 0xD9, 0xE4, 0x00, 0x00, 0x00, 0x00";
    break;

  }

    for (int i=0;i<8;i++)
    {
        message.data[i] = 0xAB;

    }
    return message;

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


