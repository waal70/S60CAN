/*
 * compilecontrol.h
 *
 *  Created on: Jun 7, 2016
 *      Author: awaal
 */

#ifndef COMPILECONTROL_H_
#define COMPILECONTROL_H_

//#define DEBUG_MAIN
//#define DEBUG_FREE_MEM

// This enables the sending of periodic keep-alive messages
// Also pretty useful for loopback testing
#define KEEPALIVE
#define LOOPBACKMODE   1

// This enables using the apparatus as an indepent DPF temp monitor
//  the original goal for making this :)
#define DPFMONITOR
#define TARGETS60 	0
// This enables filtering according to my own settings. It uses hardware filtering and masking, and therefore is speedy
#define PASS_S60CAN

  // This enables the use of a LiquidCrystal_I2C LCD attached to the Arduino
  #define LCDATTACHED

#endif /* COMPILECONTROL_H_ */
