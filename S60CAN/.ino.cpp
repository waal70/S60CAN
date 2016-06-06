#ifdef __IN_ECLIPSE__
//This is a automatic generated file
//Please do not modify this file
//If you touch this file your change will be overwritten during the next build
//This file has been generated on 2016-06-06 17:41:34

#include "Arduino.h"
#include <TimerOne.h>
#include <Wire.h>
#include <Canbus.h>
#include <SPI.h>
#include <SD.h>
#include <defaults.h>
#include <global.h>
#include <stdio.h>
#include <mcp2515.h>
#include <mcp2515_defs.h>
#include <LiquidCrystal_I2C.h>
#include "s60can_msgs.h"
#include "s80can_msgs.h"
#include <SD.h>
#include "UsbCAN.h"
static int uart_putchar (char c, FILE *stream)   ;
void display_operation_mode() ;
int switch_mode(unsigned int mode) ;
void write_DPF_msg_on_LCD (tCAN *message) ;
void write_EGR_msg_on_LCD (tCAN *message) ;
void write_OIL_msg_on_LCD (tCAN *message) ;
void write_BOOST_msg_on_LCD (tCAN *message) ;
void show_CAN_msg_on_LCD( tCAN * message, bool recv ) ;
int send_CAN_msg(tCAN * msg)  ;
int freeRam () ;
void checkRam() ;
int init_module( unsigned long baudrate ) ;
void setFilter() ;
void setup() ;
int is_in_normal_mode() ;
void handle_CAN_rx() ;
int handle_cmd(char * cmd) ;
void handle_host_messages() ;
void loop() ;

#include "S60CAN.ino"


#endif
