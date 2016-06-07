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
#include <Arduino.h>
#include <mcp2515.h>
#include "../../s60can/s60can.h"

#define KEEPALIVE_MSG 0
#define DPF_MSG 0x0196
#define EGR_MSG 0x002C
#define OIL_MSG 0x00ED
#define BOOST_MSG 0x0176

    int LOOPBACK = 0;
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
    LOOPBACK = blnLBM;
    last_keepalive_msg=millis();
    keepalive_timeout = 40;
}

void init_monitoring(int blnLBM) {
    LOOPBACK = blnLBM;
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
// 1 = succes, 0 = fail
int isOILMessage(tCAN * message) {

  // Ignore canid. Different cars may send different diagnostic id's
  // DPF-return message contains: CE 11 E6 00 ED xx yy 00. 11 E6 00 ED are relevant
  //loopback testing:
  if (LOOPBACKMODE)
      return ((message->data[1] == 0x11) && (message->data[2] == 0xA6) && (message->data[3] == 0x00) && (message->data[4] == 0xED));
  else
      return ((message->data[1] == 0x11) && (message->data[2] == 0xE6) && (message->data[3] == 0x00) && (message->data[4] == 0xED));

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 1 = succes, 0 = fail
int isBOOSTMessage(tCAN * message) {

  // Ignore canid. Different cars may send different diagnostic id's
  // DPF-return message contains: CE 11 E6 01 76 xx yy 00. 11 E6 01 76 are relevant
  //loopback testing:
  if (LOOPBACKMODE)
      return ((message->data[1] == 0x11) && (message->data[2] == 0xA6) && (message->data[3] == 0x01) && (message->data[4] == 0x76));
  else
      return ((message->data[1] == 0x11) && (message->data[2] == 0xE6) && (message->data[3] == 0x01) && (message->data[4] == 0x76));

}
 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
tCAN construct_CAN_msg(int msgType) {
  tCAN message;
  
  Serial.println("S60!");
  switch (msgType) {
    case KEEPALIVE_MSG:
      //live keepalive message:
      //000FFFFE 8 D8 00 00 00 00 00 00 00
      // initialize the keep-alive message
      message.header.rtr = 0;
      message.header.eid = 1;
      message.length = 8;
      message.id = 0x000ffffe;
      message.data[0] = 0xd8;
      for (int i=1;i<8;i++)
        message.data[i] = 0x00;  
    return message;
    break;
    case (DPF_MSG):
      message.header.rtr = 0;
      message.header.eid = 1;
      message.length = 8;
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
      message.length = 8;
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
    case (OIL_MSG):
      message.header.rtr = 0;
      message.header.eid = 1;
      message.length = 8;
      message.id = 0x000ffffe; //default diagnostic id
      message.data[0] = 0xCD;  
      message.data[1] = 0x11;
      message.data[2] = 0xA6;
      message.data[3] = 0x00;
      message.data[4] = 0xED;
      message.data[5] = 0x01;
      message.data[6] = 0x00;
      //for loopback testing:
      if (LOOPBACKMODE) {
        message.data[5] = 0x0E;
        message.data[6] = 0x3C; // 0E3C = 91.66 degrees C, 0E39 = 90.96
        } 
      message.data[7] = 0x00;
      return message;
    break;
    case (BOOST_MSG):
      message.header.rtr = 0;
      message.header.eid = 1;
      message.length = 8;
      message.id = 0x000ffffe; //default diagnostic id
      message.data[0] = 0xCD;  
      message.data[1] = 0x11;
      message.data[2] = 0xA6;
      message.data[3] = 0x01;
      message.data[4] = 0x76;
      message.data[5] = 0x01;
      message.data[6] = 0x00;
      //for loopback testing:
      if (LOOPBACKMODE) {
        message.data[5] = 0x04;
        message.data[6] = 0x09; // 0409 = 1033 hPa, 040D = 1037 hPa
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
void prepOILMessage(tCAN * message, char * msg) {
	  //pre-condition isOILMessage is true (1)
	  char temp[6];
	  uint16_t value;
	  //S80: 0x05, 0x62, 0xD9, 0xDC, 0x0D, 0x33, 0x00, 0x00
	  value = (uint16_t)(((message->data[5] << 8) | (message->data[6])) & 0xFFFF);

	  // Check for a valid temperature, between 0 and 999.9 degrees celsius
	  // Character buffers need to be at least 1 character longer than the number of characters you are writing to them
	  // As we are writing 0.1 to maximum 999.9 this means a buffer of 5+1
	  if (((double)value > 0) && ((double)value < 3732) )
	  {
	    dtostrf((value-2731.5)/10,3,1,temp);
	    //337 is the degree symbol
	    sprintf(msg, "OIL: %s \337C", temp);
	  }
	  else
	    sprintf(msg, "OIL: ERR \337C");

	}
void prepEGRMessage(tCAN * message, char * msg) {
	  char temp[6];
	  float factor = 0.01220703125;
	  uint16_t value;
	  // CD 11 E6 01 96 0B D4 00
	  // This gets the 6th and 7th element from the EGR response message (tested through isEGRMessage())
	  // And calculates the percentage as follows:
	  // Decimal value is 8192 (hex: 2000)-based, meaning 8192 corresponds with 100%
	  // Lower value is XXX, so the factor becomes 0.0122 (more or less)
	  //S80: 0x05, 0x62, 0xD9, 0x04, 0x0D, 0x33, 0x00, 0x00
	  value = (uint16_t)(((message->data[5] << 8) | (message->data[6])) & 0xFFFF);
	  // Check for a valid value, between 0 and 8193 decimal
	  // Character buffers need to be at least 1 character longer than the number of characters you are writing to them
	  // As we are writing 0.1 to maximum 100.0 this means a buffer of 5+1
	  if (((double)value > 0) && ((double)value < 8193) )
	  {
	    dtostrf((double)value*factor,3,1,temp);
	    //337 is the degree symbol
	    sprintf(msg, "EGR: %s%%", temp);
	  }
	  else
	    sprintf(msg,"EGR: ERR %");
	}
void prepBOOSTMessage(tCAN * message, char * msg) {
	  char temp[5];
	  float factor = 0.001;
	  uint16_t value;
	  // CD 11 E6 01 76 04 0D 00
	  // 040D (1037) = 1037 hPa, 0409 (1033) = 1033 hPa
	  // This gets the 6th and 7th element from the BOOST response message (tested through isBOOSTMessage())
	  // And calculates the boost pressure as follows:
	  // Decimal value is boost pressure in hectoPascals (1hPa = 1/1000 bar)
	  value = (uint16_t)(((message->data[5] << 8) | (message->data[6])) & 0xFFFF);
	  // Check for a valid pressure, between 0 and 4500 hPA
	  // Character buffers need to be at least 1 character longer than the number of characters you are writing to them
	  // As we are writing 1.00 to maximum 4.00 this means a buffer of 4+1
	  if (((double)value > 0) && ((double)value < 8193) )
	  {
	    dtostrf((double)value*factor,1,2,temp);
	    sprintf(msg, "TRB: %s bar", temp);
	  }
	  else
	    sprintf(msg,"TRB: ERR bar");

	}
void prepDPFMessage(tCAN * message, char * msg) {
	  char temp[7];
	  uint16_t value;
	  // CD 11 E6 01 96 0B D4 00
	  // This gets the 6th and 7th element from the DPF response message (tested through isDPFMessage())
	  // And calculates the temperature as follows:
	  // Decimal value is temperature in tenths of degrees Kelvin. Therefore:
	  // decimal value /10 - 273.15 = degrees celsius:
	  // For S80 return message is 0x04, 0x62, 0xF4, 0x05, 0x61, 0x00, 0x00, 0x00

	  value = (uint16_t)(((message->data[5] << 8) | (message->data[6])) & 0xFFFF);
	  // Check for a valid temperature, between 0 and 2000 degrees celsius
	  // Character buffers need to be at least 1 character longer than the number of characters you are writing to them
	  // As we are writing 0.1 to maximum 2000.0 this means a buffer of 6+1

	  if (((double)value > 2732) && ((double)value < 22732) )
	  {
	    dtostrf((value-2731.5)/10,4,1,temp);
	    //337 is the degree symbol
	    sprintf(msg, "DPF: %s \337C", temp);
	  }
	  else
	    sprintf(msg,"DPF: ERR \337C");
}

#endif
