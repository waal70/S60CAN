char foo;  // for the sake of Arduino header parsing anti-automagic. Remove and prepare yourself for headache.
/* S60 CAN  - Arduino firmware - version 0.5 alpha
**
** Copyright (c) 2015 André de Waal
**
** Large portions copied from Olaf @ hackingvolvo.blogspot.com
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
** 10-09-2015: Start of CodeCleanup branch
** 25-04-2016: Special branch for Patrick, S80-can
* 
* Memory footprint (last updated: 25-04-2015):
* Global variables use 1,432 bytes (69%) of dynamic memory, leaving 707 bytes for local variables. Maximum is 2,048 bytes.
* 
* Memory footprint after S80-changes (conditional #includes):
* 
*/

//TODO:
// * Move all USBCAN-related functionality to USBCAN.CPP
// * Currently ifdefs are being misused. Clean up
//
//
// * Way-over-yonder: make the list of sensors to read dynamic

#include "compilecontrol.h"
#include "s60can.h"
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

#if TARGETS60==1
	#include <s60can_msgs.h>
  //#error S60CANMSGS
#else
	#include <s80can_msgs.h>
  //#error S80CANMSGS
#endif


#include <SD.h>
  const int chipSelect = 9; //The CARD_SC pin on the sparkfun board

#define MCP2551_STANDBY_PIN A1

// Filter does not pass messages by default, since VIDA seems to crash at start if all CAN messages are transmitted to it. If you are not using VIDA but
// for example CAN Hacker, you can uncomment this define or use Lawicell 'M' and 'm' commands to set acceptance register (set mask to 0x0 to pass all messages).
// #define PASS_ALL_MSGS
// initialize the library with the numbers of the interface pins
#ifdef LCDATTACHED
  //GND, VCC to GND and 5V
  //SDA to A4
  //SCL to A5
  LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
  int globalMessageCounter = 0;
#endif

// This enables using this as a passthru-device according to Lawicel standards
// In effect, dispatching received messages to the USB
//#define USBCAN    // we are using CANUSB (Lawicel) / CAN232 format by default now
#ifdef USBCAN
  #include "UsbCAN.h"
#endif
    
  char msgFromHost[32]; // message that is being read from host
  int msgLen=0;
  uint8_t status;
  unsigned int errorFlags;

// =====================STANDARD BLOCK TO ENABLE printf
static FILE uartout = {0} ;
static int uart_putchar (char c, FILE *stream)
  {
    // convert newline to carriage return
    if (c == '\n') 
        uart_putchar('\r', stream);
    Serial.write(c) ;
    return 0 ;
  }
// =====================END STANDARD BLOCK TO ENABLE printf

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void display_operation_mode() {

  #ifdef LCDATTACHED
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F("mode: "));
    switch((Canbus.getMode()))
      {
        case MODE_NORMAL:
          lcd.print(F("NO"));
          break;
        case MODE_SLEEP:
          lcd.print(F("SL"));
          break;
        case MODE_CONFIG:
          lcd.print(F("CF"));
          break;
        case MODE_LISTENONLY:
          lcd.print(F("LI"));
          break;
        case MODE_LOOPBACK:
          lcd.print(F("LP"));
          break;
        default:
          lcd.print(F("ER"));
          break;
    }
    // now write the speed status line:
    lcd.setCursor (0,1);
    lcd.print(F("kbps: "));
      //for a positive ID on speed, read CNF1 and CNF2
      // but as we limit the speeds to 125, 250 and 500,
      // what suffices is:
      // CNF1: 0x00: 500kbps
      // CNF1: 0x41: 250kbps
      // CNF1: 0x03: 125kbps
    uint8_t kbps = mcp2515_read_register(CNF1);
    switch (kbps)
      {
        case MCP_16MHz_125kBPS_CFG1:
          lcd.print(F("125"));
          break;
        case MCP_16MHz_250kBPS_CFG1:
          lcd.print(F("250"));
          break;
        case MCP_16MHz_500kBPS_CFG1:
          lcd.print(F("500"));
          break;
        default:
          lcd.print(F("???"));
          break;
      }
    lcd.print(Canbus.getDisplayFilter());
  #endif
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int switch_mode(unsigned int mode) {
  //Assume the worst from this mode switch
  uint8_t ret_mode = MODE_ERROR;

  if (Canbus.setMode(mode))
    ret_mode = Canbus.getMode();

  display_operation_mode();  
  
  return (ret_mode == mode);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void write_DPF_msg_on_LCD (tCAN *message) {

  //pre-condition isDPFMessage is true (1)
 lcd.setCursor(0,0);
 lcd.print(F("                "));
 lcd.setCursor(0,0);
 char msg[16];
 prepDPFMessage(message, msg);
 lcd.print(msg);
 lcd.setCursor(16,3);
 lcd.print(globalMessageCounter);
 globalMessageCounter++;
  
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void write_EGR_msg_on_LCD (tCAN *message) {
 char msg[16];
  //pre-condition isEGRMessage is true (1)
 //lcd.clearLine(0);
 //lcd.backlight();
 lcd.setCursor(0,1);
 lcd.print(F("          ")); //we do this because initialization leaves characters on line 2
 lcd.setCursor(0,1);
  prepEGRMessage(message, msg);
  lcd.print(msg);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void write_OIL_msg_on_LCD (tCAN * message) {
 char msg[16];
  //pre-condition isOILMessage is true (1)
  lcd.setCursor(0,2);
  prepOILMessage(message,msg);
  lcd.print(msg);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void write_BOOST_msg_on_LCD (tCAN *message) {
  char msg[16];
  lcd.setCursor(0,3);
  prepBOOSTMessage(message, msg);
  lcd.print(msg);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef LCDATTACHED
void show_CAN_msg_on_LCD( tCAN * message, bool recv )
{
      char msg[32];
      char data[8];
      lcd.clear();
      lcd.setCursor(0,0);
      if (recv)
        lcd.print(F("rx "));
      else
        lcd.print(F("tx "));
      
      if (message->header.rtr)
          {
          lcd.print(F("r"));
      } else
          {
          lcd.print(F(" "));        
      }
      //(uint8_t)(((message->id) >> 21)&0x7ff) will give identifier A (module id?)   
      sprintf(msg,"%02x%02x%02x%02x %d", (uint8_t)(message->id>>24),(uint8_t)((message->id>>16)&0xff),(uint8_t)((message->id>>8)&0xff),(uint8_t)(message->id&0xff),(uint8_t)(((message->id) >> 21)&0x7ff) );      
      lcd.setCursor(5,0);
      lcd.print(msg);
      lcd.setCursor(0,1);

      int i;
      for (i=0;i<=7;i++)
        {
        sprintf(data,"%02x", message->data[i]);
        lcd.print(data);
        }
}
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int send_CAN_msg(tCAN * msg)  {
  int ret = 0;   
  #ifdef LCDATTACHED
    //disable this for monitoring mode
    #ifndef DPFMONITOR
      write_DPF_msg_on_LCD(msg, false);
    #endif
  #endif
  
#if TARGETS60
	  ret=mcp2515_send_message_J1939(msg);  // ret=0 (buffers full), 1 or 2 = used send buffer
#else
	  ret=mcp2515_send_message(msg);
#endif

  printf("Message SENT now \n");
  char hdr[16];
  sprintf(hdr,"HDR: %02x \n", msg->id);
  printf(hdr);
  int i;
  char data[8];
  for (i=0;i<=7;i++)
    {
	  sprintf(data,"%02x", msg->data[i]);
	  printf(data);
    }
  if (ret==0) {
    #ifdef LCDATTACHED
      lcd.setCursor(0,0);
      lcd.print(F("ovfl"));  // overflow
    #endif
  }
  return ret; // FIXME: we return the result of adding the msg to MCP2515 internal buffer, not actually the result of sending it! 
  }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void checkRam() {

  #ifdef DEBUG_FREE_MEM
	  unsigned int freeram = freeRam();
  if (freeram < 256)
    {
      printf(F("Warning! Lo mem: "));
      printf(freeRam());
    }
  else
    {
      printf(F("Free mem: "));
      printf(freeRam());
    }
  #endif
  //if (freeram < 128)
  // give warning somewhere
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// returns 1 if success, 0 if failed
int init_module( unsigned long baudrate )
{
  #ifdef LCDATTACHED
    lcd.setCursor(0,1);
    lcd.print(F("init: "));
    char txt[16];
    sprintf(txt,"%lu ",baudrate);
    lcd.print(txt);
  #endif

  if(!(Canbus.init(baudrate)))
    {
    // initialization failed!
    #ifdef LCDATTACHED
      lcd.print(F("fail!"));
    #endif
    #ifdef DEBUG_MAIN
      printf("mcp2515 init failed!");
    #endif
    return 0;
    }
  else
    return 0;

  
  #ifdef LCDATTACHED
    lcd.setCursor(11,1);
    lcd.print(F(" ok"));
  #endif
  return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setFilter()
{
  //setHWFilter (masks[], len_masks, ids[], len_ids)
  // examples: 
  // 1. filter only certain ids (e.g. keepalive-message2):
  //    masks[1] = {0xffffffff};
  //    filters[1] = {0x0381526c};
  // 2. filter on a common item in the id (e.g. all id's ending in 6c):
  //    masks[1] = {0x000000ff};
  //    filters[1] = {0x0381526c};
  // 3. no filter at all, same as not setting a filter (so why would you set it :))
  //    masks[1] = {0x00000000};
  //    filters[1] = {0x0381526c};
  // 4. filter on two specific ids:
  //    masks[1] = {0xffffffff};
  //    filters[2] = {0x0381526c, 0x000fff6c};
  // 5. filter diagnostic messages & replies
  //    requests: 000FFFFE responses: 01200021
  //    00801001: LS response?
  //    00800009: DIM response
//  01A0600A: power on state?
// 01800008: PHM
// 0131726C: SWM
// 012173BE: PAM
// 03C01428: ?
// 01E0162A: ?
  
 #if TARGETS60
  uint32_t masks[2] = {0xffffffff, 0xffffffff};
  uint32_t filters[6] = {0x000FFFFE, 0x01200021, 0x00000000, 0x00000000, 0x00000000, 0x00000000};
  delay(10);
    mcp2515_setHWFilter(masks,2, filters, 6);
  delay(10);
 #else
  uint16_t masks[2] = {0xffff, 0xffff};
  uint16_t filters[6] = {0x07e0, 0x07e8, 0x0000, 0x0000, 0x0000, 0x0000};
  delay(10);
    mcp2515_setHWFilterS80(masks,2, filters, 6);
  delay(10);
 #endif




}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() { 
 
  Serial.begin(115200);
  // Here we enable us to use printf to write to host instead of having to use Serial.print!
  // fill in the UART file descriptor with pointer to writer.
  fdev_setup_stream (&uartout, uart_putchar, NULL, _FDEV_SETUP_WRITE);
  // The uart is the standard output device STDOUT.
  stdout = &uartout ;

  #ifdef LCDATTACHED
    lcd.begin(20, 4);
    lcd.clear();
  #endif

  //Perform SPI initialization routine anyway...Precondtion: SD-card present!
    if (!SD.begin(chipSelect)) 
       Serial.println("Card NOK");

  // we have to initialize the CAN module anyway, otherwise SPI commands (read registers/status/get_operation_mode etc) hang during invocation
  if (!init_module(500000))
    printf("NOK");
  // set MCP2551 to normal mode
  // TODO: move to abstraction?
  pinMode(MCP2551_STANDBY_PIN,OUTPUT);
  digitalWrite(MCP2551_STANDBY_PIN,LOW);
  
  #ifdef USBCAN
    UsbCAN::init_protocol();
  #endif

  delay(500); 
  #ifdef KEEPALIVE
    init_keepalive(LOOPBACKMODE);
  #endif //KEEPALIVE

  #ifdef DPFMONITOR
    init_monitoring(LOOPBACKMODE);
  #endif

  //set the filters for the messages
  switch_mode(MODE_CONFIG);
    setFilter();

  //This sets DEFAULT mode:
  if (LOOPBACKMODE)
    switch_mode(MODE_LOOPBACK);
  else
    switch_mode(MODE_NORMAL);
    
  #ifdef DEBUG_FREE_MEM
    unsigned int freemem = freeRam();
    printf("free mem: ");
    printf(freemem);
  #endif

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////


int is_in_normal_mode()
{
  return (Canbus.getMode() == MODE_NORMAL);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void handle_CAN_rx() {
  
  while (mcp2515_check_message())
    {
      tCAN message;
      uint8_t status = mcp2515_get_message(  &message );
      if (status)  
        {
    	  printf("Message received \n");
          #ifdef LCDATTACHED
            #ifndef DPFMONITOR
              show_CAN_msg_on_LCD(&message,true);
            #endif
          #endif
          if (isDPFMessage(&message))
            write_DPF_msg_on_LCD(&message);
          if (isEGRMessage(&message))
            write_EGR_msg_on_LCD(&message);
          if (isOILMessage(&message))
            write_OIL_msg_on_LCD(&message);
          if (isBOOSTMessage(&message))
            write_BOOST_msg_on_LCD(&message);
          
        } // if (status)  {
    } // while (mcp2515_check_message())
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef USBCAN
int handle_cmd(char * cmd)
{
  return UsbCAN::handle_host_message(cmd);
}
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef USBCAN
void handle_host_messages()
{
 int receivedByte;
 while (Serial.available() > 0) {
	receivedByte = Serial.read();
          if  (receivedByte =='\r')
            {
              #ifdef LCDATTACHED
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print(msgFromHost);
                //use this if you want to see what the host sends
                //delay(2000);
              #endif
              if (handle_cmd(msgFromHost)==0)
                {
                  #ifdef LCDATTACHED
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print(F("err"));
                    lcd.setCursor(4, 0);
                    lcd.print(msgFromHost);
                  #endif
                }            
                msgLen=0;
              }
            else if (receivedByte =='\b')  // handle backspaces as well (since we might be testing functionality on terminal)
              {
                if (msgLen>0)
                  msgLen--; 
                msgFromHost[msgLen]=0;
              }
            else
              {
                if (receivedByte !='\n') // ignore linefeeds
                  {
                    msgFromHost[msgLen++] = receivedByte;
                    msgFromHost[msgLen]=0;
	                }
              }
        } // while serial available
}
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop() {

#ifdef USBCAN
  handle_host_messages();
#endif
  handle_CAN_rx();

  checksend_CAN_msgs();
  
  checkRam();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

