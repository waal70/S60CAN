
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
*/

//TODO:
// * Move all USBCAN-related functionality to USBCAN.CPP
// * Currently ifdefs are being misused. Clean up

//#define DEBUG_MAIN
//#define DEBUG_FREE_MEM
// This enables the logging of the messages to an attached SD-card
// Warning, although this is for SD-logging, without this define
// still Serial initialization takes place. TODO: fix
//#define DATALOGGER

// This enables the sending of periodic keep-alive messages
// Also pretty useful for loopback testing
#define KEEPALIVE
#define LOOPBACKMODE   1

// This enables using the apparatus as an indepent DPF temp monitor 
//  the original goal for making this :)
#define DPFMONITOR

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

#include <SD.h>
  #define TIME_TO_CLOSE   15
  int fileCloseCounter =0;
  File dataFile;
  const int chipSelect = 9; //The CARD_SC pin on the sparkfun board
  String timeStamp;

#include "usbcan.h"

char foo;  // for the sake of Arduino header parsing anti-automagic. Remove and prepare yourself for headache.

#define HW_VER        0x01    // hardware version
#define SW_VER        0x00    // software version
#define SW_VER_MAJOR  0x05    // software major version
#define SW_VER_MINOR  0x01    // software minor version
#define MCP2551_STANDBY_PIN A1

// ONE_SHOT_MODE tries to send message only once even if error occurs during transmit.  (see MCP2515 data sheet)
// If we are testing the Sardine CAN without any other CAN device on the network, then there will be no ACK signals acknowledging that
// transmit succeeded and thus sending fails. If this happens, MCP2515 will keep on sending the message forever and transmit buffers will eventually
// fill up. Also cheap ELM327 clones (with older firmware) do not support ACK-signaling, so a network consisting of MCP2515 + ELM327 does not work
// if ONE_SHOT_MODE is not enabled. You should however disable this when connecting Sardine CAN to a car
//#define ONE_SHOT_MODE

// Filter does not pass messages by default, since VIDA seems to crash at start if all CAN messages are transmitted to it. If you are not using VIDA but
// for example CAN Hacker, you can uncomment this define or use Lawicell 'M' and 'm' commands to set acceptance register (set mask to 0x0 to pass all messages).
// #define PASS_ALL_MSGS

// This enables filtering according to my own settings. It uses hardware filtering and masking, and therefore is speedy
#define PASS_S60CAN

// This enables the use of a LiquidCrystal_I2C LCD attached to the Arduino
#define LCD

// initialize the library with the numbers of the interface pins
#ifdef LCD
  //GND, VCC to GND and 5V
  //SDA to A4
  //SCL to A5
  LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
  int globalMessageCounter = 0;
#endif

// This enables using this as a passthru-device according to Lawicel standards
// In effect, dispatching received messages to the USB
#define USBCAN    // we are using CANUSB (Lawicel) / CAN232 format by default now

    
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

unsigned int get_operation_mode() {

  uint8_t mode = mcp2515_read_register(CANCTRL);
  uint8_t kbps = mcp2515_read_register(CNF1);
  //for a positive ID on speed, read CNF1 and CNF2
  // but as we limit the speeds to 125, 250 and 500,
  // what suffices is:
  // CNF1: 0x00: 500kbps
  // CNF1: 0x41: 250kbps
  // CNF1: 0x03: 125kbps
  #ifdef LCD
    lcd.setCursor(14,0);
    switch(mode)
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
    //WARNING: this only works if we are setting filter mode to either
    // 11 or 00. This will incorrectly interpret intermediate settings!
    uint8_t fltr = mcp2515_read_register(RXB0CTRL);
    if (!((fltr & 0x60) == 0x60))
      lcd.print("*");
        //one of the two bits is zero. Because of the above
        // assumption, both bits are zero. Therefore, filter is set.
  #endif
  return mode;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int switch_mode(unsigned int mode) {

  #ifdef LCD
    lcd.clear();
  #endif 
  mcp2515_setCANCTRL_Mode((uint8_t)mode);
  uint8_t ret_mode = mcp2515_read_register(CANCTRL);
  
  get_operation_mode();  
  
  #ifdef LCD
    lcd.setCursor(0,0);
    lcd.print(F("mode: "));
  
    switch (ret_mode)
      {
        case MODE_NORMAL:
          lcd.print(F("normal"));
          break;
        case MODE_SLEEP:
          lcd.print(F("sleep"));
          break;
        case MODE_CONFIG:
          lcd.print(F("config"));
          break;
        case MODE_LISTENONLY:
          lcd.print(F("listen"));
          break;
        case MODE_LOOPBACK:
          lcd.print(F("loopback"));
          break;
        default:
          lcd.print(F("error!"));
          return 0;
        break;
      }
  #endif
  return (ret_mode == mode);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void write_DPF_msg_on_LCD (tCAN *message) {

  //pre-condition isDPFMessage is true (1)
 //lcd.clearLine(0);
 //lcd.backlight();
 lcd.setCursor(0,0);
 lcd.write("                ");
 lcd.setCursor(0,0);
  char temp[7]; 
  char msg[16];
  uint16_t value;
  // CD 11 E6 01 96 0B D4 00
  // This gets the 6th and 7th element from the DPF response message (tested through isDPFMessage())
  // And calculates the temperature as follows:
  // Decimal value is temperature in tenths of degrees Kelvin. Therefore:
  // decimal value /10 - 273.15 = degrees celsius:
  value = (uint16_t)(((message->data[5] << 8) | (message->data[6])) & 0xFFFF);

  // Check for a valid temperature, between 0 and 2000 degrees celsius
  // Character buffers need to be at least 1 character longer than the number of characters you are writing to them
  // As we are writing 0.1 to maximum 2000.0 this means a buffer of 6+1
  if (((double)value > 2732) && ((double)value < 22732) )
  {
    dtostrf((value-2731.5)/10,4,1,temp);
    //337 is the degree symbol
    sprintf(msg, "DPF: %s \337C", temp);
    lcd.print(msg);
  }
  else
    lcd.print(F("DPF: ERR \337C"));

  lcd.setCursor(16,3);
  lcd.print(globalMessageCounter);

  globalMessageCounter++;
  
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void write_EGR_msg_on_LCD (tCAN *message) {

  //pre-condition isDPFMessage is true (1)
 //lcd.clearLine(0);
 //lcd.backlight();
 lcd.setCursor(0,1);
 lcd.write("          ");
 lcd.setCursor(0,1);
  char temp[7]; 
  char msg[16];
  float factor = 0.01220703125;
  uint16_t value;
  // CD 11 E6 01 96 0B D4 00
  // This gets the 6th and 7th element from the DPF response message (tested through isDPFMessage())
  // And calculates the temperature as follows:
  // Decimal value is temperature in tenths of degrees Kelvin. Therefore:
  // decimal value /10 - 273.15 = degrees celsius:
  value = (uint16_t)(((message->data[5] << 8) | (message->data[6])) & 0xFFFF);

  // Check for a valid temperature, between 0 and 2000 degrees celsius
  // Character buffers need to be at least 1 character longer than the number of characters you are writing to them
  // As we are writing 0.1 to maximum 2000.0 this means a buffer of 6+1
  if (((double)value > 0) && ((double)value < 8193) )
  {
    dtostrf((double)value*factor,3,1,temp);
    //337 is the degree symbol
    sprintf(msg, "EGR: %s%%", temp);
    lcd.print(msg);
  }
  else
    lcd.print(F("EGR: ERR %"));  
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void write_OIL_msg_on_LCD (tCAN *message) {

  //pre-condition isOILMessage is true (1)
  lcd.setCursor(0,2);
  char temp[7]; 
  char msg[16];
  uint16_t value;
  // CD 11 E6 01 96 0B D4 00
  // This gets the 6th and 7th element from the OIL response message (tested through isOILMessage())
  // And calculates the temperature as follows:
  // Decimal value is temperature in tenths of degrees Kelvin. Therefore:
  // decimal value /10 - 273.15 = degrees celsius:
  value = (uint16_t)(((message->data[5] << 8) | (message->data[6])) & 0xFFFF);

  // Check for a valid temperature, between 0 and 2000 degrees celsius
  // Character buffers need to be at least 1 character longer than the number of characters you are writing to them
  // As we are writing 0.1 to maximum 999.9 this means a buffer of 5+1
  if (((double)value > 0) && ((double)value < 8193) )
  {
    dtostrf((double)value-2731.5,3,1,temp);
    //337 is the degree symbol
    sprintf(msg, "OIL: %s \337C", temp);
    lcd.print(msg);
  }
  else
    lcd.print(F("OIL: ERR \337C"));  
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void write_BOOST_msg_on_LCD (tCAN *message) {

  lcd.setCursor(0,3);
  char temp[5]; 
  char msg[16];
  float factor = 0.001;
  uint16_t value;
  // CD 11 E6 01 96 0B D4 00
  // This gets the 6th and 7th element from the BOOST response message (tested through isBOOSTMessage())
  // And calculates the boost pressure as follows:
  // Decimal value is boost pressure in hectoPascals (1hPa = 1/1000 bar)
  value = (uint16_t)(((message->data[5] << 8) | (message->data[6])) & 0xFFFF);

  // Check for a valid temperature, between 0 and 2000 degrees celsius
  // Character buffers need to be at least 1 character longer than the number of characters you are writing to them
  // As we are writing 1.00 to maximum 4.00 this means a buffer of 4+1
  if (((double)value > 0) && ((double)value < 8193) )
  {
    dtostrf((double)value*factor,1,2,temp);
    sprintf(msg, "TRB: %s bar", temp);
    lcd.print(msg);
  }
  else
    lcd.print(F("TRB: ERR bar"));  
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef LCD
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
      for (i=0;i<message->header.length;i++)
        {
        sprintf(data,"%02x", message->data[i]);
        lcd.print(data);
        }
}
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef DATALOGGER
void write_CAN_msg_to_file( tCAN * message, bool recv )
{
      char msg[32];
      char data[8];
      timeStamp = String(millis()); //get current timestamp
      // Strategy is as follows:
      // In principle, the file only closes every TIME_TO_CLOSE times
      // So it could be, on entry of this function, that there is already a file.
      // In that case, just log to it, otherwise, open the file.
      File dataFile = SD.open("dl.txt", FILE_WRITE);
      if (dataFile) {
        if (recv)
          dataFile.print(timeStamp + " rx ");
        else
          dataFile.print(timeStamp + " tx ");
        //I do not think Volvo sends requests to remote
        //if (message->header.rtr)
        //    dataFile.print(F("r"));
        //else
        //    dataFile.print(F(" "));        
        sprintf(msg,"%02x%02x%02x%02x|%02d|", (uint8_t)(message->id>>24),(uint8_t)((message->id>>16)&0xff),(uint8_t)((message->id>>8)&0xff),(uint8_t)(message->id&0xff),(uint8_t)(((message->id) >> 21)&0x7ff) );      
        dataFile.print(msg);
        for (int i=0;i<message->header.length;i++) {
            sprintf(data,"%02x", message->data[i]);
            dataFile.print(data);
          }
        dataFile.println(F("|"));
        dataFile.close();
      }
}
#endif //DATALOGGER

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int send_CAN_msg(tCAN * msg)  {   
  printf("send_CAN_msg");
  #ifdef LCD
    //disable this for monitoring mode
    #ifndef DPFMONITOR
      write_DPF_msg_on_LCD(msg, false);
    #endif
  #endif
  
  #ifdef DATALOGGER
    write_CAN_msg_to_file(msg, false);
  #endif
  
  int ret=mcp2515_send_message_J1939(msg);  // ret=0 (buffers full), 1 or 2 = used send buffer
  if (ret==0) {
    #ifdef LCD
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
  unsigned int freeram = freeRam();
  #ifdef DEBUG_FREE_MEM
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
  #ifdef LCD
    lcd.setCursor(0,1);
    lcd.print(F("init: "));
    char txt[16];
    sprintf(txt,"%lu ",baudrate);
    lcd.print(txt);
  #endif
  
  unsigned char mcp2515_speed;
  switch (baudrate)
    {
      case 125000:
        mcp2515_speed=CANSPEED_125;
        break;
      case 250000:
        mcp2515_speed=CANSPEED_250;
        break;
      case 500000:
        mcp2515_speed=CANSPEED_500;
        break;
      default:
        #ifdef LCD
          lcd.print(F("unsup"));
        #endif
        return 0;
    }

  if(!Canbus.init(mcp2515_speed))
    {
      // initialization failed!
      #ifdef LCD
        lcd.print(F("fail!"));
      #endif
      #ifdef DEBUG_MAIN
	      printf("mcp2515 init failed!");
      #endif
      return 0;
    }
  // we need to be in config mode by default, before opening the channel
  switch_mode(MODE_CONFIG);

  // don't require interrupts from successful send
  mcp2515_bit_modify(CANINTE, (1<<TX0IE), 0);

  // enable one-shot mode
  #ifdef ONE_SHOT_MODE
    mcp2515_bit_modify(CANCTRL, (1<<OSM), (1<<OSM));
  #else
    mcp2515_bit_modify(CANCTRL, (1<<OSM), 0);
  #endif
  
  // roll-over: receiving message will be moved to receive buffer 1 if buffer 0 is full
  mcp2515_bit_modify(RXB0CTRL, (1<<BUKT), (1<<BUKT));
  
  #ifdef LCD
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
  
  uint32_t masks[2] = {0xffffffff, 0xffffffff};
  uint32_t filters[6] = {0x000FFFFE, 0x01200021, 0x00000000, 0x00000000, 0x00000000, 0x00000000};
  
  mcp2515_setHWFilter(masks,2, filters, 6);


}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() { 
 
  Serial.begin(115200);
  // Here we enable us to use printf to write to host instead of having to use Serial.print!
  // fill in the UART file descriptor with pointer to writer.
  fdev_setup_stream (&uartout, uart_putchar, NULL, _FDEV_SETUP_WRITE);
  // The uart is the standard output device STDOUT.
  stdout = &uartout ;

  #ifdef LCD
    lcd.begin(20, 4);
    lcd.print(F("S60 logger CAN"));
    lcd.setCursor(0, 1);  
    char version[16];
    sprintf(version,"v%d.%d INIT",SW_VER_MAJOR,SW_VER_MINOR);
    lcd.print(version);
    //delay(1000); // a little time to read the status message
    lcd.clear();
  #endif

  //#ifdef DATALOGGER: perform initialization routine anyway...
    if (!SD.begin(chipSelect)) 
       Serial.println("Card NOK");
  //#endif

  // we have to initialize the CAN module anyway, otherwise SPI commands (read registers/status/get_operation_mode etc) hang during invocation
  if (!init_module(500000))
    printf("NOK");
  // set MCP2551 to normal mode
 
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
  delay(10);
  setFilter();
  delay(10);

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
  return (get_operation_mode() == MODE_NORMAL);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void handle_CAN_rx() {
  
  while (mcp2515_check_message())
    {
      tCAN message;
      uint8_t status = mcp2515_get_message(  &message );
      if (status)  
        {
          #ifdef LCD
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
          
          #ifdef DATALOGGER
            write_CAN_msg_to_file(&message, true);
            // check if RX buffer overflow has occured since last receive
            //uint8_t eflg = mcp2515_read_register(EFLG);
            //if (eflg & (1<<RX1OVR) ) -> write or log OVERFLOW
          #endif
          #ifdef USBCAN
            return UsbCAN::dispatch_CAN_message(&message);
          #endif
        } // if (status)  {
    } // while (mcp2515_check_message())
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int handle_cmd(char * cmd)
{
  return UsbCAN::handle_host_message(cmd);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void handle_host_messages()
{
 int receivedByte;
 while (Serial.available() > 0) {
	receivedByte = Serial.read();
          if  (receivedByte =='\r')
            {
              #ifdef LCD
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print(msgFromHost);
                //use this if you want to see what the host sends
                //delay(2000);
              #endif
              if (handle_cmd(msgFromHost)==0)
                {
                  #ifdef LCD
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop() {

  handle_host_messages();
  handle_CAN_rx();

  checksend_CAN_msgs();
  

      
  checkRam();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

