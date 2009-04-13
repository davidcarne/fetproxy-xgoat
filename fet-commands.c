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
#include "fet-commands.h"
#include <glib.h>
#include <stdint.h>
#include <string.h>

void fet_cmd_poll( FetModule* fet )
{
	uint8_t d[] = { 0x12, 0x02, 0x01, 0x00,
			0x00, 0x00, 0x00, 0x00 };

	fet_module_transmit( fet, d, sizeof(d) );
}

void fet_cmd_write_mem( FetModule* fet,
			uint16_t addr,
			const uint8_t *buf,
			uint16_t len )
{
	uint8_t e[ 12 + len ];

	e[0] = 0x0e;
	e[1] = 0x04;
	e[2] = 0x01;
	e[3] = 0x00;
	
	e[4] = addr & 0xff;
	e[5] = (addr>>8) & 0xff;

	e[6] = e[7] = 0;

	e[8] = len & 0xff;
	e[9] = (len>>8) & 0xff;

	e[10] = e[11] = 0;

	g_memmove( e + 12, buf, len );

	fet_module_transmit( fet, e, len + 12 );
	
}

void fet_cmd_read_mem( FetModule* fet, 
		       uint16_t addr,
		       uint16_t len )
{
	uint8_t e[12] = { 0x0D, 0x02, 0x02, 0x00,
			  addr & 0xff, (addr>>8) & 0xff, 0x00, 0x00,
			  len & 0xff, (len>>8) & 0xff, 0x00, 0x00 };

	fet_module_transmit( fet, e, sizeof(e) );
}

void fet_cmd_open( FetModule* fet )
{
	const uint8_t d[2] = {0x01, 0x01};

	fet_module_transmit( fet, d, sizeof(d) );
}

void fet_cmd_init( FetModule *fet )
{
	const uint8_t d[8] = {0x27, 0x02, 0x01, 0x00, 
			      0x04, 0x00, 0x00, 0x00};

	fet_module_transmit( fet, d, sizeof(d) );
}

void fet_cmd_conf( FetModule *fet, gboolean sbw )
{
	uint8_t d[12] = { 0x05, 0x02, 0x02, 0x00,
			 0x08, 0x00, 0x00, 0x00,
			 sbw?0x01:0x00, 0x00, 0x00, 0x00 };

	fet_module_transmit( fet, d, sizeof(d) );
}

void fet_cmd_set_vcc( FetModule *fet, uint16_t mv )
{
	uint8_t d[8] = { 0x06, 0x02, 0x01, 0x00,
			 mv & 0xff, (mv>>8) & 0xff, 0x00, 0x00 };

	fet_module_transmit( fet, d, sizeof(d) );
}

void fet_cmd_identify( FetModule *fet )
{
	uint8_t d[12] = { 0x03, 0x02, 0x02, 0x00,
			 0x50, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00 };

	fet_module_transmit( fet, d, sizeof(d) );
}

void fet_cmd_write_context( FetModule *fet, uint16_t *regs )
{
	const uint8_t cmd[] = { 0x09, 0x04, 0x01, 0x00,
				0xff, 0xff, 0x00, 0x00, 
				0x40, 0x00, 0x00, 0x00 };
	uint8_t d[12 + 64 + 1];
	uint8_t i;

	/* Copy the command in */
	g_memmove( d, cmd, 12 );

	for( i=0; i<16; i++ ) {
		uint8_t *p = d+12 + (i*4);

		p[0] = regs[i] & 0xff;
		p[1] = (regs[i] >> 8) & 0xff;
		p[2] = p[3] = 0;
	}
	
	d[12+64] = 0;

	fet_module_transmit( fet, d, sizeof(d) );
}

void fet_cmd_read_context( gpointer _fet )
{
	FetModule *fet = FET_MODULE(_fet);
	uint8_t d[4] = { 0x08, 0x01 };

	fet_module_transmit( fet, d, sizeof(d) );
}

void fet_cmd_erase( FetModule *fet, 
		    fet_cmd_erase_t s,
		    uint16_t addr )
{
	uint8_t d[16] = { 0x0C, 0x02, 0x03, 0x00,
			  0x00, 0x00, 0x00, 0x00,
			  0x00, 0x00, 0x00, 0x00,
			  0x00, 0x00, 0x00, 0x00 };
	uint8_t t;
	uint16_t m;

	switch(s) {
	case FET_ERASE_ALL:
		t = 0x02;
		addr = 0xffe0;
		m = 0x0002;
		break;

	case FET_ERASE_MAIN:
		t = 0x01;
		addr = 0xffe0;
		m = 0x0002;
		break;

	case FET_ERASE_ADDR:
		t = 0x00;
		m = 0x0002;
		break;

	case FET_ERASE_INFO:
		t = 0x00;
		m = 0x0100;
		addr = 0x1000;
		break;
	}

	d[4] = t;
	d[8] = addr & 0xff;
	d[9] = (addr>>8) & 0xff;
	d[12] = m & 0xff;
	d[13] = (m>>8) & 0xff;

	fet_module_transmit( fet, d, sizeof(d) );
}

void fet_cmd_reset( FetModule *fet, uint8_t rtype, gboolean dirty )
{
	uint8_t d[16] = { 0x07, 0x02, 0x03, 0x00,
			  rtype, 0x00, 0x00, 0x00,
			  dirty?0x00:0x01, 0x00, 0x00, 0x00,
			  dirty?0x00:0x01, 0x00, 0x00, 0x00 };

	fet_module_transmit( fet, d, sizeof(d) );
}

void fet_cmd_close( FetModule *fet )
{
	uint8_t d[4] = { 0x02, 0x02, 0x01, 0x00 };

	fet_module_transmit( fet, d, sizeof(d) );
}

void fet_cmd_run( gpointer _fet )
{
	FetModule *fet = FET_MODULE(_fet);

	uint8_t d[12] = { 0x11, 0x02, 0x02, 0x00,
			  0x03, 0x00, 0x00, 0x00,
			  0x00, 0x00, 0x00, 0x00 };

	fet_module_transmit( fet, d, sizeof(d) );
}
