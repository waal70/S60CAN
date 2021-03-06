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
#ifdef __cplusplus

extern "C"
{
	

#endif
// ----------------------------------------------------------------------------
typedef struct
{
	uint32_t id;
	struct {
		int8_t rtr : 1;
		uint8_t length : 4;
		int8_t eid : 1;
	} header;
	uint8_t data[8];
} tCAN;

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
uint8_t mcp2515_send_message(tCAN *message);
// ----------------------------------------------------------------------------
// sends a CAN message using the 29-bit J939 extended identifier
uint8_t mcp2515_send_message_J1939(tCAN *message);

#ifdef __cplusplus
}
#endif

#endif	// MCP2515_H
