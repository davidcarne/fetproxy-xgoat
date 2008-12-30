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
/* 	uint8_t e[12] = { 0x0D, 0x02, 0x02, 0x00, */
/* 			  addr & 0xff, (addr>>8) & 0xff, 0x00, 0x00, */
/* 			  len & 0xff, (len>>8) & 0xff, 0x00, 0x00 }; */

	g_error( "FAIL" );

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
