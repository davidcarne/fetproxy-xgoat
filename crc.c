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
#include "crc.h"
#include <stdint.h>
#include <stdio.h>

uint32_t table[] = {
	0x00000000,      0x00001189,      0x00002312,      0x0000329b,
	0x00004624,      0x000057ad,      0x00006536,      0x000074bf,
	0x00008c48,      0x00009dc1,      0x0000af5a,      0x0000bed3,
	0x0000ca6c,      0x0000dbe5,      0x0000e97e,      0x0000f8f7,
	0x00001081,      0x00000108,      0x00003393,      0x0000221a,
	0x000056a5,      0x0000472c,      0x000075b7,      0x0000643e,
	0x00009cc9,      0x00008d40,      0x0000bfdb,      0x0000ae52,
	0x0000daed,      0x0000cb64,      0x0000f9ff,      0x0000e876,
	0x00002102,      0x0000308b,      0x00000210,      0x00001399,
	0x00006726,      0x000076af,      0x00004434,      0x000055bd,
	0x0000ad4a,      0x0000bcc3,      0x00008e58,      0x00009fd1,
	0x0000eb6e,      0x0000fae7,      0x0000c87c,      0x0000d9f5,
	0x00003183,      0x0000200a,      0x00001291,      0x00000318,
	0x000077a7,      0x0000662e,      0x000054b5,      0x0000453c,
	0x0000bdcb,      0x0000ac42,      0x00009ed9,      0x00008f50,
	0x0000fbef,      0x0000ea66,      0x0000d8fd,      0x0000c974,
	0x00004204,      0x0000538d,      0x00006116,      0x0000709f,
	0x00000420,      0x000015a9,      0x00002732,      0x000036bb,
	0x0000ce4c,      0x0000dfc5,      0x0000ed5e,      0x0000fcd7,
	0x00008868,      0x000099e1,      0x0000ab7a,      0x0000baf3,
	0x00005285,      0x0000430c,      0x00007197,      0x0000601e,
	0x000014a1,      0x00000528,      0x000037b3,      0x0000263a,
	0x0000decd,      0x0000cf44,      0x0000fddf,      0x0000ec56,
	0x000098e9,      0x00008960,      0x0000bbfb,      0x0000aa72,
	0x00006306,      0x0000728f,      0x00004014,      0x0000519d,
	0x00002522,      0x000034ab,      0x00000630,      0x000017b9,
	0x0000ef4e,      0x0000fec7,      0x0000cc5c,      0x0000ddd5,
	0x0000a96a,      0x0000b8e3,      0x00008a78,      0x00009bf1,
	0x00007387,      0x0000620e,      0x00005095,      0x0000411c,
	0x000035a3,      0x0000242a,      0x000016b1,      0x00000738,
	0x0000ffcf,      0x0000ee46,      0x0000dcdd,      0x0000cd54,
	0x0000b9eb,      0x0000a862,      0x00009af9,      0x00008b70,
	0x00008408,      0x00009581,      0x0000a71a,      0x0000b693,
	0x0000c22c,      0x0000d3a5,      0x0000e13e,      0x0000f0b7,
	0x00000840,      0x000019c9,      0x00002b52,      0x00003adb,
	0x00004e64,      0x00005fed,      0x00006d76,      0x00007cff,
	0x00009489,      0x00008500,      0x0000b79b,      0x0000a612,
	0x0000d2ad,      0x0000c324,      0x0000f1bf,      0x0000e036,
	0x000018c1,      0x00000948,      0x00003bd3,      0x00002a5a,
	0x00005ee5,      0x00004f6c,      0x00007df7,      0x00006c7e,
	0x0000a50a,      0x0000b483,      0x00008618,      0x00009791,
	0x0000e32e,      0x0000f2a7,      0x0000c03c,      0x0000d1b5,
	0x00002942,      0x000038cb,      0x00000a50,      0x00001bd9,
	0x00006f66,      0x00007eef,      0x00004c74,      0x00005dfd,
	0x0000b58b,      0x0000a402,      0x00009699,      0x00008710,
	0x0000f3af,      0x0000e226,      0x0000d0bd,      0x0000c134,
	0x000039c3,      0x0000284a,      0x00001ad1,      0x00000b58,
	0x00007fe7,      0x00006e6e,      0x00005cf5,      0x00004d7c,
	0x0000c60c,      0x0000d785,      0x0000e51e,      0x0000f497,
	0x00008028,      0x000091a1,      0x0000a33a,      0x0000b2b3,
	0x00004a44,      0x00005bcd,      0x00006956,      0x000078df,
	0x00000c60,      0x00001de9,      0x00002f72,      0x00003efb,
	0x0000d68d,      0x0000c704,      0x0000f59f,      0x0000e416,
	0x000090a9,      0x00008120,      0x0000b3bb,      0x0000a232,
	0x00005ac5,      0x00004b4c,      0x000079d7,      0x0000685e,
	0x00001ce1,      0x00000d68,      0x00003ff3,      0x00002e7a,
	0x0000e70e,      0x0000f687,      0x0000c41c,      0x0000d595,
	0x0000a12a,      0x0000b0a3,      0x00008238,      0x000093b1,
	0x00006b46,      0x00007acf,      0x00004854,      0x000059dd,
	0x00002d62,      0x00003ceb,      0x00000e70,      0x00001ff9,
	0x0000f78f,      0x0000e606,      0x0000d49d,      0x0000c514,
	0x0000b1ab,      0x0000a022,      0x000092b9,      0x00008330,
	0x00007bc7,      0x00006a4e,      0x000058d5,      0x0000495c,
	0x00003de3,      0x00002c6a,      0x00001ef1,      0x00000f78
};

uint16_t crc_block( const uint8_t* buf, /* data */
		    int32_t len ) /* Data length */
{
	uint32_t i;
	/* The UIF initial value is 0xffff */
	uint16_t crc = 0xffff;

	for( i=0; i<len; i++)
	{
		uint16_t tmp = crc >> 8;

		crc = (crc ^ (*buf)) & 0xff;
		buf++;

		tmp ^= table[ crc ];

		crc = tmp;
	}

	return ~crc;
}
