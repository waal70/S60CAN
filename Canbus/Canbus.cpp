/**
 * 
 * Copyright (c) 2015 André de Waal - revamped for S60CAN
 * Copyright (c) 2008-2009  All rights reserved.
 */

#if ARDUINO>=100
#include <Arduino.h> // Arduino 1.0
#else
#include <Wprogram.h> // Arduino 0022
#endif
#include <stdint.h>

#include <stdio.h>
#include <inttypes.h>
#include "mcp2515.h"
#include "Canbus.h"


// ONE_SHOT_MODE tries to send message only once even if error occurs during transmit.  (see MCP2515 data sheet)
// If we are testing the Sardine CAN without any other CAN device on the network, then there will be no ACK signals acknowledging that
// transmit succeeded and thus sending fails. If this happens, MCP2515 will keep on sending the message forever and transmit buffers will eventually
// fill up. Also cheap ELM327 clones (with older firmware) do not support ACK-signaling, so a network consisting of MCP2515 + ELM327 does not work
// if ONE_SHOT_MODE is not enabled. You should however disable this when connecting Sardine CAN to a car
//#define ONE_SHOT_MODE


/* C++ wrapper */
CanbusClass::CanbusClass() {

 
}

char CanbusClass::init(unsigned long speed) {

  //check if init succesful
  char msg[16];
  sprintf(msg, "Speed: %ul", speed);
  printf(msg);
  if (isSupportedBaudrate(speed)) {
	if (mcp2515_init(speed))
		{
			// we need to be in config mode by default, before opening the channel
			setMode(MODE_CONFIG);
		
			// don't require interrupts from successful send
			mcp2515_bit_modify(CANINTE, (1<<TX0IE), 0);
			
			// enable one-shot mode, if needed. By default: off (connected to car)
			#ifdef ONE_SHOT_MODE
				mcp2515_bit_modify(CANCTRL, (1<<OSM), (1<<OSM));
			#else
				mcp2515_bit_modify(CANCTRL, (1<<OSM), 0);
			#endif
			// roll-over: receiving message will be moved to receive buffer 1 if buffer 0 is full
			mcp2515_bit_modify(RXB0CTRL, (1<<BUKT), (1<<BUKT));
			return 1;
		}
	else {
			printf("mcp2515.init");
			return 0;
		}
	}
  else
  {
	printf("not supported baudrate");
	return 0;
  }
 
}
///////////////////////////////////////////////////////////////////////////////////
// returns 1 on succes, 0 on failure
char CanbusClass::setMode(unsigned int mode) {
	mcp2515_setCANCTRL_Mode((uint8_t)mode);
}
///////////////////////////////////////////////////////////////////////////////////
char CanbusClass::getMode() {
	return mcp2515_read_register(CANCTRL);
}

///////////////////////////////////////////////////////////////////////////////////
char* CanbusClass::getDisplayFilter() {
	//3 because I am returning an asterisk and one null character
	char* result = (char*) malloc(2);
    //WARNING: this only works if we are setting filter mode to either
    // 11 or 00. This will incorrectly interpret intermediate settings!
    uint8_t fltr = mcp2515_read_register(RXB0CTRL);
    if (!((fltr & 0x60) == 0x60))
      result = "*";
	return result;
        //one of the two bits is zero. Because of the above
        // assumption, both bits are zero. Therefore, filter is set.
}
///////////////////////////////////////////////////////////////////////////////////
// returns 1 on succes, 0 on failure
char CanbusClass::isSupportedBaudrate(unsigned long speed) {

//Currently support 125000, 250000 and 500000

	switch (speed)
	{
		case 125000:
			return 1;
			break;
		case 250000:
			return 1;
			break;
		case 500000:
			return 1;
			break;
		default:
			return 0;
			break;
	}
	return 0;
}


CanbusClass Canbus;
