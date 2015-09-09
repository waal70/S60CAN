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
**
*/
#include <Arduino.h>
#include <mcp2515.h>
#include "s60can.h"

#define KEEPALIVE_MSG 0
#define DPF_MSG 0x0196
#define EGR_MSG 0x002C

    int LOOPBACKMODE = 0;
    unsigned long last_keepalive_msg;
    unsigned long keepalive_timeout; // timeout in 1/10 seconds. 0=keepalive messaging disabled
    
    unsigned long last_keepalive_msg2;
    unsigned long keepalive_timeout2; // timeout in 1/10 seconds. 0=keepalive messaging disabled

    unsigned long last_dpf_msg;
    unsigned long last_dpf_frequency; //frequency in 1/10 seconds. 0=diagnostic messaging disabled

    unsigned long last_egr_msg;
    unsigned long last_egr_frequency; //frequency in 1/10 seconds. 0=diagnostic messaging disabled

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void init_keepalive(int blnLBM) {
    LOOPBACKMODE = blnLBM;
    last_keepalive_msg=millis();
  // initialize the keep-alive message
    last_keepalive_msg2=millis();
    keepalive_timeout = 40;
     keepalive_timeout2 = 0;
}

void init_dpf(int blnLBM) {
    LOOPBACKMODE = blnLBM;
    // initialize the dpf monitoring message
    last_dpf_msg=millis();
    last_dpf_frequency = 35; //every 3.5 seconds

    last_egr_msg=last_dpf_msg;
    last_egr_frequency = 10; //every second
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 1 = succes, 0 = fail
int isDPFMessage(tCAN * message) {

  // Ignore canid. Different cars may send different diagnostic id's
  // DPF-return message contains: CE 11 E6 01 96 xx yy 00. 11 E6 01 96 are relevant
  //loopback testing:
  if (LOOPBACKMODE)
      return ((message->data[1] == 0x11) && (message->data[2] == 0xA6) && (message->data[3] == 0x01) && (message->data[4] == 0x96));
  else
      return ((message->data[1] == 0x11) && (message->data[2] == 0xE6) && (message->data[3] == 0x01) && (message->data[4] == 0x96));

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 1 = succes, 0 = fail
int isEGRMessage(tCAN * message) {

  // Ignore canid. Different cars may send different diagnostic id's
  // DPF-return message contains: CE 11 E6 01 96 xx yy 00. 11 E6 01 96 are relevant
  //loopback testing:
  if (LOOPBACKMODE)
      return ((message->data[1] == 0x11) && (message->data[2] == 0xA6) && (message->data[3] == 0x00) && (message->data[4] == 0x2C));
  else
      return ((message->data[1] == 0x11) && (message->data[2] == 0xE6) && (message->data[3] == 0x00) && (message->data[4] == 0x2C));

}
 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
tCAN construct_CAN_msg(int msgType) {
  tCAN message;
  
  switch (msgType) {
    case KEEPALIVE_MSG:
      //live keepalive message:
      //000FFFFE 8 D8 00 00 00 00 00 00 00
    
      // initialize the keep-alive message
      message.header.rtr = 0;
      message.header.eid = 1;
      message.header.length = 8;
      message.id = 0x000ffffe;
      message.data[0] = 0xd8;
      for (int i=1;i<8;i++)
        message.data[i] = 0x00;  
   /* keepalive_msg2.header.rtr = 0;
    keepalive_msg2.header.eid = 1;
    keepalive_msg2.header.length = 8;
    keepalive_msg2.id = 0x01017ffc;
    for (int i=0;i<8;i++)
      keepalive_msg2.data[i] = 0x00;  
    keepalive_msg2.data[4] = 0x1f;
    keepalive_msg2.data[5] = 0x40;*/
    return message;
    break;
    case (DPF_MSG):
      message.header.rtr = 0;
      message.header.eid = 1;
      message.header.length = 8;
      message.id = 0x000ffffe; //default diagnostic id
      message.data[0] = 0xCD;  
      message.data[1] = 0x11;
      message.data[2] = 0xA6;
      message.data[3] = 0x01;
      message.data[4] = 0x96;
      message.data[5] = 0x01;
      message.data[6] = 0x00;
      //for loopback testing:
      if (LOOPBACKMODE) {
        message.data[5] = 0x0F;
        message.data[6] = 0xD9; // OBD4 = temp of 29.6, 58CB = temp of 2000
        } 
      message.data[7] = 0x00;
      return message;
    break;
    case (EGR_MSG):
      message.header.rtr = 0;
      message.header.eid = 1;
      message.header.length = 8;
      message.id = 0x000ffffe; //default diagnostic id
      message.data[0] = 0xCD;  
      message.data[1] = 0x11;
      message.data[2] = 0xA6;
      message.data[3] = 0x00;
      message.data[4] = 0x2C;
      message.data[5] = 0x01;
      message.data[6] = 0x00;
      //for loopback testing:
      if (LOOPBACKMODE) {
        message.data[5] = 0x20;
        message.data[6] = 0x00; // 15DB = percentage of 67.%, 2000 = 100%
        } 
      message.data[7] = 0x00;
      return message;
    break;
    default:
    break;
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


}

