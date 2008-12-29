#ifndef __CRC_H
#define __CRC_H
#include <stdint.h>

/* Returns the CRC */
uint16_t crc_block( const uint8_t* buf, /* data */
		    int32_t len ); /* Data length */

#endif	/* __CRC_H */
