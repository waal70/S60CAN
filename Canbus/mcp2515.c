/* Copyright (c) 2015 André de Waal 
 *
 * Portions copyright (c) 2007 Fabian Greif
 * All rights reserved.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
// ----------------------------------------------------------------------------
#include <avr/io.h>
#include <util/delay.h>

#if ARDUINO>=100
#include <Arduino.h> // Arduino 1.0
#else
#include <Wprogram.h> // Arduino 0022
#endif
#include <stdint.h>
#include <avr/pgmspace.h>

#include "global.h"
//#include <SPI.h>
#include "mcp2515.h"
#include "mcp2515_defs.h"

#include "defaults.h"


// -------------------------------------------------------------------------
// Schreibt/liest ein Byte ueber den Hardware SPI Bus

uint8_t spi_putc( uint8_t data )
{
	// put byte in send-buffer
	SPDR = data;
	
	// wait until byte was send
	while( !( SPSR & (1<<SPIF) ) );
	return SPDR;
}

/*********************************************************************************************************
** Function name:           mcp2515_setRegister
** Descriptions:            set register
*********************************************************************************************************/
void mcp2515_setRegister(uint8_t address, uint8_t value)
{
    MCP2515_SELECT();
    spi_putc(MCP_WRITE);
    spi_putc(address);
    spi_putc(value);
    MCP2515_UNSELECT();
}

// -------------------------------------------------------------------------
void mcp2515_write_register( uint8_t adress, uint8_t data )
{
	RESET(MCP2515_CS);
	
	spi_putc(SPI_WRITE);
	spi_putc(adress);
	spi_putc(data);
	
	SET(MCP2515_CS);
}

// -------------------------------------------------------------------------
uint8_t mcp2515_read_register(uint8_t adress)
{
	uint8_t data;
	
	RESET(MCP2515_CS);
	
	spi_putc(SPI_READ);
	spi_putc(adress);
	
	data = spi_putc(0xff);	
	
	SET(MCP2515_CS);
	
	return data;
}

// -------------------------------------------------------------------------
void mcp2515_bit_modify(uint8_t adress, uint8_t mask, uint8_t data)
{
	RESET(MCP2515_CS);
	
	spi_putc(SPI_BIT_MODIFY);
	spi_putc(adress);
	spi_putc(mask);
	spi_putc(data);
	
	SET(MCP2515_CS);
}

// ----------------------------------------------------------------------------
uint8_t mcp2515_read_status(uint8_t type)
{
	uint8_t data;
	
	RESET(MCP2515_CS);
	
	spi_putc(type);
	data = spi_putc(0xff);
	
	SET(MCP2515_CS);
	
	return data;
}
//---------------------------------------------------------------------------
uint8_t mcp2515_setCANCTRL_Mode (uint8_t newMode)
{
	uint8_t returnMode;

	mcp2515_bit_modify(CANCTRL, MODE_MASK, newMode);
	
	returnMode = mcp2515_read_register(CANCTRL);
	returnMode &= MODE_MASK;
	
	if (returnMode == newMode)
		return 0;
	else
		return 1;

}

//---------------------------------------------------------------------------
void mcp2515_reset(void)
{
    MCP2515_SELECT();
    spi_putc(MCP_RESET);
    MCP2515_UNSELECT();
    delay(10);

}
// -------------------------------------------------------------------------
uint8_t mcp2515_init(uint8_t speed)
{
	
	uint8_t result;
	//char msg[32];
	//sprintf(msg,"speed: %02x |", (uint8_t)(speed) );
	//printf(msg);
	mcp2515_reset();
	//set config mode
	result = mcp2515_setCANCTRL_Mode(MODE_CONFIG);
	if (result >0)
		return result;
		
	uint8_t set, cfg1, cfg2, cfg3;
	set = 1;

	
	switch (speed)
	{
		case (7): //7=125 kbps
			cfg1 = MCP_16MHz_125kBPS_CFG1;
			cfg2 = MCP_16MHz_125kBPS_CFG2;
			cfg3 = MCP_16MHz_125kBPS_CFG3;
			break;
		case (3):
			cfg1 = MCP_16MHz_250kBPS_CFG1;
			cfg2 = MCP_16MHz_250kBPS_CFG2;
			cfg3 = MCP_16MHz_250kBPS_CFG3;
			break;
		case (1):
			cfg1 = MCP_16MHz_500kBPS_CFG1;
			cfg2 = MCP_16MHz_500kBPS_CFG2;
			cfg3 = MCP_16MHz_500kBPS_CFG3;
			break;
		default:
			set = 0;
			break;	
	}
	if (set) {
        //mcp2515_setRegister(CNF1, cfg1);  //Andre: to decide, write and setRegister have the same effect
        //mcp2515_setRegister(CNF2, cfg2);  // although setRegister seems a bit cleaner (no reset?)
        //mcp2515_setRegister(CNF3, cfg3);
		mcp2515_write_register(CNF1, cfg1);
        mcp2515_write_register(CNF2, cfg2);
        mcp2515_write_register(CNF3, cfg3);
		//return true;
		}
	
	//test Andre enable interrupts:
	// probably also could have been write_register
	mcp2515_setRegister(CANINTE, RX0IF | RX1IF);

	//test code Andre
	//set = mcp2515_read_register(CNF1);
	//char msg[32];
	//sprintf(msg,"CNF1: %02x |", (uint8_t)(set) );
	//printf(msg);
	
	if (mcp2515_read_register(CNF1) != cfg1) {
		SET(LED2_HIGH);

		return false;
	}
	
	// deaktivate the RXnBF Pins (High Impedance State)
	mcp2515_write_register(BFPCTRL, 0);
	
	// set TXnRTS as inputs
	mcp2515_write_register(TXRTSCTRL, 0);
	
	// turn off filters => receive any message
	mcp2515_write_register(RXB0CTRL, (1<<RXM1)|(1<<RXM0));
	mcp2515_write_register(RXB1CTRL, (1<<RXM1)|(1<<RXM0));
	
	// reset device to normal mode
	mcp2515_write_register(CANCTRL, 0);
//	SET(LED2_HIGH);
	return true;
}
// ----------------------------------------------------------------------------
uint8_t split_canbus_id(uint8_t requestedPart, uint32_t canbus_id) {

	switch (requestedPart)
	{
		case (EID0):
			return (uint8_t) (canbus_id & 0xFF);
			break;
		case (EID8):
			return (uint8_t) (canbus_id >> 8);
		break;
		case (SIDL):
			{
				uint8_t sidl;
				uint16_t id2 = (uint16_t)(canbus_id>>16);
				sidl = (uint8_t) (id2 & 0x03);
				sidl += (uint8_t) ((id2 & 0x1C) << 3);
				sidl |= 0x08;
				return sidl;
				break;
			}
		case (SIDH):
			{
				uint16_t id2 = (uint16_t)(canbus_id>>16);
				return (uint8_t) (id2 >> 5);
			}
		break;
	}
}
// ----------------------------------------------------------------------------
// Utility to set the mask and filter in the appropriate registers 
void mcp2515_setHWFilter(uint32_t masks[], int len_mask, uint32_t data_ids[], int len_data) {
  
  //Assume the mask and the id come in the following format:
  // 0x01234567
  //    if ((message->id & 0xffff0000) == 0x00800000)
  // Also, there can only  be mask0 and mask1 (2)
  //  There can only be filter0,1,2,3,4,5 (6)
  
  if (len_mask > 2)
	len_mask = 2;
  if (len_data > 6)
	len_data = 6;
  //if ((len_mask =< 0) | (len_data =< 0))
	
	//need to quit
  // First, set the mask(s):
  int i=0;
  for (i=0; i<len_mask; i++)
	{
		  int j = i*4;
		  mcp2515_write_register(RXM0SIDH + j, masks[i] & 0xFF);
		  mcp2515_write_register(RXM0SIDL + j, masks[i] >> 8);
		  mcp2515_write_register(RXM0EID8 + j, masks[i] >> 16);
		  mcp2515_write_register(RXM0EID0 + j, masks[i] >> 24);		
	}
	
  // Now the filters:
  int fc=0;
  for (i=0; i<len_data; i++)
	{
		if (i>=3)
		 fc = 4; //extra correcting for moving from filter 2 to filter 3
		int j = fc + (i*4);
		mcp2515_write_register(RXF0SIDH + j, (split_canbus_id(SIDH, data_ids[i])));
		mcp2515_write_register(RXF0SIDL + j, (split_canbus_id(SIDL, data_ids[i])));
		mcp2515_write_register(RXF0EID8 + j, (split_canbus_id(EID8, data_ids[i])));
		mcp2515_write_register(RXF0EID0 + j, (split_canbus_id(EID0, data_ids[i])));	
	}
  //A note on filters:
  //RXM<1:0>: Receive Buffer Operating mode bits 6-5
	//11 = Turn mask/filters off; receive any message
	//10 = Receive only valid messages with extended identifiers that meet filter criteria
	//01 = Receive only valid messages with standard identifiers that meet filter criteria. Extended ID filter
	//		registers RXFnEID8:RXFnEID0 are ignored for the messages with standard IDs.
	//00 = Receive all valid messages using either standard or extended identifiers that meet filter criteria.
	//		Extended ID filter registers RXFnEID8:RXFnEID0 are applied to first two bytes of data in the
	//		messages with standard IDs.
  
  //this enables filtering. Use 1<< to disable filters
  //Enable filter for buffer 0:
	mcp2515_write_register(RXB0CTRL, (0<<RXM1)|(0<<RXM0));
	if (len_mask == 2)
		mcp2515_write_register(RXB1CTRL, (0<<RXM1)|(0<<RXM0));
	else
		mcp2515_write_register(RXB1CTRL, (1<<RXM1)|(1<<RXM0)); //disable second filter
	
}

// ----------------------------------------------------------------------------
// check if there are any new messages waiting

uint8_t mcp2515_check_message(void) {
	return (!IS_SET(MCP2515_INT));
}

// ----------------------------------------------------------------------------
// check if there is a free buffer to send messages

uint8_t mcp2515_check_free_buffer(void)
{
	uint8_t status = mcp2515_read_status(SPI_READ_STATUS);
	
	if ((status & 0x54) == 0x54) {
		// all buffers used
		return false;
	}
	
	return true;
}

// ----------------------------------------------------------------------------
uint8_t mcp2515_get_message(tCAN *message)
{
	// read status
	uint8_t status = mcp2515_read_status(SPI_RX_STATUS);
	uint8_t addr;
	uint8_t t;
	if (bit_is_set(status,6)) {
		// message in buffer 0
		addr = SPI_READ_RX;
	}
	else if (bit_is_set(status,7)) {
		// message in buffer 1
		addr = SPI_READ_RX | 0x04;
	}
	else {
		// Error: no message available
		return 0;
	}

	RESET(MCP2515_CS);
	spi_putc(addr);
	
	if (bit_is_set(status,RXS_EXIDE) )
		{
		// Extended id: read id
		message->header.eid = 1;
		message->id = (uint32_t)(spi_putc(0xff)) << 21; // 8 MSB of Identifier A
		uint8_t RXBnSIDL = spi_putc(0xff);
		message->id |= ((uint32_t)(RXBnSIDL & 0b11100000)) << (18-5);  // 3 MSB of Identifier A
		message->id |= ((uint32_t)(RXBnSIDL & 0x3)) << 16; // 2 HSB of Identifier B
		message->id |= ((uint32_t)spi_putc(0xff)) << 8;
		message->id |= spi_putc(0xff);
		}
	else
		{
		// Standard id: read id
		message->header.eid = 0;
		message->id  = (uint16_t) spi_putc(0xff) << 3;
		message->id |=            spi_putc(0xff) >> 5;
	
		spi_putc(0xff);
		spi_putc(0xff);
		}
	
	// read DLC
	uint8_t length = spi_putc(0xff) & 0x0f;
	
	message->header.length = length;
	message->header.rtr = (bit_is_set(status, 3)) ? 1 : 0;
	
	// read data
	for (t=0;t<length;t++) {
		message->data[t] = spi_putc(0xff);
	}
	SET(MCP2515_CS);
	
	// clear interrupt flag
	if (bit_is_set(status, 6)) {
		mcp2515_bit_modify(CANINTF, (1<<RX0IF), 0);
	}
	else {
		mcp2515_bit_modify(CANINTF, (1<<RX1IF), 0);
	}
	
	return (status & 0x07) + 1;
}

// ----------------------------------------------------------------------------
uint8_t mcp2515_send_message(tCAN *message)
{
	uint8_t status = mcp2515_read_status(SPI_READ_STATUS);
	
	/* Statusbyte:
	 *
	 * Bit	Function
	 *  2	TXB0CNTRL.TXREQ
	 *  4	TXB1CNTRL.TXREQ
	 *  6	TXB2CNTRL.TXREQ
	 */
	uint8_t address;
	uint8_t t;
	if (bit_is_clear(status, 2)) {
		address = 0x00;
	}
	else if (bit_is_clear(status, 4)) {
		address = 0x02;
	} 
	else if (bit_is_clear(status, 6)) {
		address = 0x04;
	}
	else {
		// all buffer used => could not send message
		return 0;
	}
	
	RESET(MCP2515_CS);
	spi_putc(SPI_WRITE_TX | address);
	
	spi_putc(message->id >> 3);
    spi_putc(message->id << 5);
	spi_putc(0);
	spi_putc(0);

	uint8_t length = message->header.length & 0x0f;
	
	if (message->header.rtr) {
		// a rtr-frame has a length, but contains no data
		spi_putc((1<<RTR) | length);
	}
	else {
		// set message length
		spi_putc(length);
		
		// data
		for (t=0;t<length;t++) {
			spi_putc(message->data[t]);
		}
	}
	SET(MCP2515_CS);
	
	_delay_us(1);
	
	// send message
	RESET(MCP2515_CS);
	address = (address == 0) ? 1 : address;
	spi_putc(SPI_RTS | address);
	SET(MCP2515_CS);
	
	return address;
}

uint8_t mcp2515_send_message_J1939(tCAN *message)
{
	uint8_t status = mcp2515_read_status(SPI_READ_STATUS);
	
	uint8_t address;
	uint8_t t;

	uint8_t TXBnSIDH=0; 
	uint8_t TXBnSIDL=0;
	uint8_t TXBnEID8=0;
	uint8_t TXBnEID0=0;

	if (bit_is_clear(status, 2)) {
		address = 0x00;
	}
	else if (bit_is_clear(status, 4)) {
		address = 0x02;
	} 
	else if (bit_is_clear(status, 6)) {
		address = 0x04;
	}
	else {
		// all buffers are used => could not send message
		return 0;
	}
	
	RESET(MCP2515_CS);
	spi_putc(SPI_WRITE_TX | address);
	
	
	TXBnSIDH = (message->id) >> 21; // 8 MSB of Identifier A

	TXBnSIDL = (message->id) >> 18; 
	TXBnSIDL = (TXBnSIDL << 5) | 0x08 ; // 3 LSB of Identifier A. Enable Extended identifier (bit 3)
	
	if (GETBIT((message->id),17)==1) SETBIT(TXBnSIDL,1);	// bit 17 of Identifier B
	if (GETBIT((message->id),16)==1) SETBIT(TXBnSIDL,0);	// bit 16 of Identifier B

	TXBnEID8 = (message->id)>>8; 	// bits 8-15 of Identifier B
	TXBnEID0 = (message->id); 	// bits 0-7 of Identifier B
	
	spi_putc(TXBnSIDH);
	spi_putc(TXBnSIDL);
	spi_putc(TXBnEID8);
	spi_putc(TXBnEID0);	

	uint8_t length = message->header.length & 0x0f;
	
	if (message->header.rtr) {
		// a rtr-frame has a length, but contains no data
		spi_putc((1<<RTR) | length);
	}
	else {
		// set message length
		spi_putc(length);
		
		// data
		for (t=0;t<length;t++) {
			spi_putc(message->data[t]);
		}
	}
	SET(MCP2515_CS);
	
	_delay_us(1);
	
	// send message
	RESET(MCP2515_CS);
	address = (address == 0) ? 1 : address;
	spi_putc(SPI_RTS | address);
	SET(MCP2515_CS);
	
	return address;
}

