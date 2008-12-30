#ifndef __FET_COMMANDS
#define __FET_COMMANDS
#include <stdint.h>
#include "fet-module.h"

void fet_cmd_erase( FetModule* fet );

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


#endif	/* __FET_COMMANDS */
