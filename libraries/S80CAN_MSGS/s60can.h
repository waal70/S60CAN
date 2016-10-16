/* Sardine CAN (Open Source J2534 device) - Arduino firmware - version 0.4 alpha
**
** Copyright (C) 2012 Olaf @ Hacking Volvo blog (hackingvolvo.blogspot.com)
** Author: Olaf <hackingvolvo@gmail.com>
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
**
*/

/*#define MODE_NORMAL 0
#define MODE_SLEEP 1
#define MODE_LOOPBACK 2
#define MODE_LISTEN 3
#define MODE_CONFIG 4*/

#include <mcp2515.h>

#define HW_VER        0x01		// hardware version
#define SW_VER        0x00		// software version
#define SW_VER_MAJOR  0x01    // software major version
#define SW_VER_MINOR  0x01    // software minor version

#define STATUS_INIT 0
#define STATUS_CONFIG 1
#define STATUS_READY 2
#define STATUS_UNRECOVERABLE_ERROR 3

#define ERRSTATUS_NONE				0
#define ERRSTATUS_OUT_OF_MEMORY		1
#define ERRSTATUS_CAN_INIT_ERROR  2
#define ERRSTATUS_CAN_TX_BUFFER_OVERFLOW  4
#define ERRSTATUS_CAN_RX_BUFFER_OVERFLOW  8



#define send_to_host(...) {\
  printf("{");  \
  printf(__VA_ARGS__);  \
  printf("}\r"); } 

#define _send_to_host(...) { printf(__VA_ARGS__); }


int send_CAN_msg(tCAN * msg);
void set_keepalive_timeout( unsigned long timeout );
int init_module( unsigned long baudrate );
int is_in_normal_mode();
void get_operation_mode();

int switch_mode( unsigned int mode);


