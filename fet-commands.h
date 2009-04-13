/*   Copyright (C) 2008 Robert Spanton, Tom Bennellick

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */
#ifndef __FET_COMMANDS
#define __FET_COMMANDS
#include <stdint.h>
#include "fet-module.h"

void fet_cmd_write_mem( FetModule* fet,
			uint16_t addr,
			const uint8_t *buf,
			uint16_t len );

void fet_cmd_read_mem( FetModule* fet, 
		       uint16_t addr,
		       uint16_t len );

void fet_cmd_poll( FetModule* fet );

void fet_cmd_open( FetModule* fet );

void fet_cmd_init( FetModule *fet );

/* sbw is TRUE for spy-bi-wire */
void fet_cmd_conf( FetModule *fet, gboolean sbw );

/* mv is millivolts */
void fet_cmd_set_vcc( FetModule *fet, uint16_t mv );

void fet_cmd_identify( FetModule *fet );

/* Write context.
   Args:
    -  fet: The FetModule to send the command on.
    - regs: 16 entry array of 16 register values. */
void fet_cmd_write_context( FetModule *fet, uint16_t *regs );

/* Read context.
   Args:
    -  fet: The FetModule to send the command on. */
void fet_cmd_read_context( gpointer _fet );

typedef enum {
	FET_ERASE_ALL,
	FET_ERASE_MAIN,
	FET_ERASE_ADDR,
	FET_ERASE_INFO
} fet_cmd_erase_t;

void fet_cmd_erase( FetModule *fet, 
		    fet_cmd_erase_t s,
		    uint16_t addr );		    

 enum {
	 FET_RESET_PUC = 1<<0,
	 FET_RESET_RST = 1<<1,
	 FET_RESET_VCC = 1<<2,
	 FET_RESET_ALL = 7 };

/* Reset the MSP430
 * Args:
 *  -   fet: The FetModule the MSP430 is connected to.
 *  - rtype: What types of reset to perform. OR together the following:
 *              o FET_RESET_PUC -- Do a PUC (probably)
 * 		o FET_RESET_RST -- Twiddle the RST pin (probably)
 * 		o FET_RESET_VCC -- Take VCC down and up (probably)
 * 		o FET_RESET_ALL -- All the above
 *  - dirty: The FET tool seems to accept two different arguments, depending
 * 	     on whether it's a reset to try and get out of an explosion/failure.
 *           Setting this to TRUE will send this failure reset command. */
void fet_cmd_reset( FetModule *fet, 
		    uint8_t rtype, 
		    gboolean dirty );
		    

void fet_cmd_close( FetModule *fet );

void fet_cmd_run( gpointer _fet );

#endif	/* __FET_COMMANDS */
