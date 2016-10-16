#ifndef	MCP2515_H
#define	MCP2515_H

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

#include <inttypes.h>

#include "mcp2515_defs.h"
#include "global.h"
#include "compilecontrol.h"
#ifdef __cplusplus

extern "C"
{
	

#endif
// ----------------------------------------------------------------------------
typedef struct
{
	#if TARGETS60 == 1
		uint32_t id;				//!< ID der Nachricht (11 oder 29 Bit)
		struct {
			int rtr : 1;			//!< Remote-Transmit-Request-Frame?
			int eid : 1;		//!< extended ID?
		} header;
	#else
		uint16_t id;				//!< ID der Nachricht (11 Bit)
		struct {
			int rtr : 1;			//!< Remote-Transmit-Request-Frame?
		} header;
	#endif
	
	uint8_t length;				//!< Anzahl der Datenbytes
	uint8_t data[8];			//!< Die Daten der CAN Nachricht
	
	#if SUPPORT_TIMESTAMPS
		uint16_t timestamp;
	#endif
} tCAN;

typedef struct
{
#if	TARGETS60
		uint32_t id;				//!< ID der Nachricht (11 oder 29 Bit)
		uint32_t mask;				//!< Maske
		struct {
			uint8_t rtr : 2;		//!< Remote Request Frame
			uint8_t eid : 2;	//!< extended ID
		} header;
#else
		uint16_t id;				//!< ID der Nachricht 11 Bits
		uint16_t mask;				//!< Maske
			struct {
			uint8_t rtr : 2;		//!< Remote Request Frame
		} header;
	#endif
} tCANFILTER;

// ----------------------------------------------------------------------------
uint8_t spi_putc( uint8_t data );
// ----------------------------------------------------------------------------
void mcp2515_setRegister(uint8_t address, uint8_t value);
// ----------------------------------------------------------------------------
void mcp2515_write_register( uint8_t adress, uint8_t data );
// ----------------------------------------------------------------------------
uint8_t split_canbus_id(uint8_t requestedPart, uint32_t canbus_id);
// ----------------------------------------------------------------------------
void mcp2515_setHWFilter(uint32_t masks[], int len_mask, uint32_t data_ids[], int len_data);
// ----------------------------------------------------------------------------
void mcp2515_setHWFilterS80(uint16_t masks[], int len_mask, uint16_t data_ids[], int len_data);
// ----------------------------------------------------------------------------

uint8_t mcp2515_read_register(uint8_t adress);
// ----------------------------------------------------------------------------
void mcp2515_bit_modify(uint8_t adress, uint8_t mask, uint8_t data);
// ----------------------------------------------------------------------------
uint8_t mcp2515_read_status(uint8_t type);
// ----------------------------------------------------------------------------
uint8_t mcp2515_setCANCTRL_Mode (uint8_t newMode);
// ----------------------------------------------------------------------------
void mcp2515_reset(void);
// ----------------------------------------------------------------------------
uint8_t mcp2515_init(uint8_t speed);
// ----------------------------------------------------------------------------
// check if there are any new messages waiting
uint8_t mcp2515_check_message(void);
// ----------------------------------------------------------------------------
// check if there is a free buffer to send messages
uint8_t mcp2515_check_free_buffer(void);
// ----------------------------------------------------------------------------
uint8_t mcp2515_get_message(tCAN *message);
// ----------------------------------------------------------------------------
uint8_t mcp2515_send_message(const tCAN *message);
// ----------------------------------------------------------------------------
#if TARGETS60
void mcp2515_write_id(const uint32_t *id, uint8_t extended);
#else
void mcp2515_write_id(const uint16_t *id);
#endif
// sends a CAN message using the 29-bit J939 extended identifier
uint8_t mcp2515_send_message_J1939(tCAN *message);

#ifdef __cplusplus
}
#endif

#endif	// MCP2515_H
