/*   Module for interacting with a TI UIF or TI EZ430
     Copyright (C) 2008 Robert Spanton, Tom Bennellick

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
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/select.h>
#include <stdint.h>
#include <errno.h>
#include <termios.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

#include "xb-fd-source.h"
#include "fet-module.h"
#include "crc.h"

gboolean fet_module_io_error( FetModule* xb );

/*** Incoming Data Functions ***/

/* Process incoming data */
gboolean fet_module_proc_incoming( FetModule* xb );

/* Reads in available bytes from the input.
 * When a full frame is achieved, it returns 0.
 * When a full frame has not been acheived, it returns 1.
 * When an error occurs, it returns -1 */
static int fet_module_read_frame( FetModule* xb  );

/* Displays the contents of a frame */
static void debug_show_frame( uint8_t* buf, uint16_t len );

static void debug_show_data( uint8_t* buf, uint16_t len );

/*** Outgoing Queue Functions ***/

/* Process outgoing data */
static gboolean fet_module_proc_outgoing( FetModule* xb );

/* Escape the byte pointed to by d.  Put the result in d.
 * Returns TRUE if it's OK to move on to the next byte. */
static gboolean fet_module_outgoing_escape_byte( FetModule* xb, uint8_t *d );

/* Whether data's ready to transmit */
static gboolean fet_module_outgoing_queued( FetModule* xb );

/* Returns the next byte to transmit */
static uint8_t fet_module_outgoing_next( FetModule* xb );

/* Adds the frame directly to the queue (memory allocation must have
 * already been done) */
static void fet_module_out_queue_add_frame( FetModule* xb, fet_frame_t* frame );

/* Removes the last frame from the transmit queue */
static void fet_module_out_queue_del( FetModule* xb );

/*** "Internal" Client API Functions ***/

/* Configure the serial port */
gboolean fet_serial_init( FetModule* xb );

/* Set the serial port baud rate */
gboolean fet_serial_baud_set( int fd, uint32_t baud );

/* Initialise the module. */
gboolean fet_init( FetModule* xb );

void fet_instance_init( GTypeInstance *xb, gpointer g_class );

/* Free information related to a module. */
void fet_free( FetModule* xb );

static void fet_module_print_stats( FetModule* xb );

static gboolean fd_set_nonblocking( int fd );

gboolean fet_init( FetModule* fet )
{
	assert( fet != NULL );

/* 	if( !fet_serial_init( fet ) ) */
/* 		return FALSE; */

	return TRUE;
}

static uint8_t fet_module_outgoing_next( FetModule* fet )
{
	const uint8_t FRAME_BOUNDARY = 0x7E;
	fet_frame_t* frame;

	assert( fet != NULL );

	frame = (fet_frame_t*)g_queue_peek_tail( fet->out_frames );

	if( fet->tx_pos == 0 )
		return FRAME_BOUNDARY;
	else if( fet->tx_pos < frame->len + 1 )
		return frame->data[ fet->tx_pos - 1 ];
	else if( fet->tx_pos < frame->len + 3 )
	{
		if( !fet->checked ) {
			/* Calculate checksum */
			fet->o_chk = crc_block( frame->data, frame->len );
			fet->checked = TRUE;
		}

		if( fet->tx_pos == frame->len + 1 )
			/* Low byte */
			return (fet->o_chk) & 0xff;
		else
			/* High byte */
			return (fet->o_chk >> 8) & 0xff;
	} else if ( fet->tx_pos == frame->len + 3 ) {
		return FRAME_BOUNDARY;
	}
	else
		g_error( "Trying to transmit past end of frame" );
	
}

/* This function needs a bit of cleanup */
static gboolean fet_module_proc_outgoing( FetModule* fet )
{
	assert( fet != NULL );

	/* If there's an item in the list, transmit part of it */
	while( g_queue_get_length( fet->out_frames ) )
	{
		fet_frame_t* frame;
		gboolean inc;
		uint8_t d;
		ssize_t w;

		frame = (fet_frame_t*)g_queue_peek_tail( fet->out_frames );
		d = fet_module_outgoing_next( fet );

		inc = fet_module_outgoing_escape_byte( fet, &d );

		/* write data */
		w = TEMP_FAILURE_RETRY(write( fet->fd, &d, 1 ));
		if( w == -1 && errno == EAGAIN )
			break;
		if( w == -1 )
		{
			fprintf( stderr, "Error writing to file: %m\n" );
			return FALSE;
		}
		if( w == 0 ) continue;

/* 		printf( "Tx: 0x%2.2hhx (pos=%hu)\n", d, fet->tx_pos ); */

		if( inc )
			fet->tx_pos += w;

		fet->bytes_tx += w;

		/* check for end of frame */
		if( frame->len + 4 == fet->tx_pos )
		{
			fet_module_out_queue_del( fet );
			fet->frames_tx ++;
			fet->tx_pos = fet->o_chk = 0;
			fet->checked = FALSE;

			fet_module_print_stats( fet );
		}
	}

	return TRUE;
}

static gboolean fet_module_outgoing_queued( FetModule* fet )
{
	assert( fet != NULL );

	if( g_queue_get_length(fet->out_frames) )
		return TRUE;
	else
		return FALSE;
}

static void fet_module_out_queue_add_frame( FetModule* fet, fet_frame_t* frame )
{
	assert( fet != NULL && frame != NULL );

	g_queue_push_head( fet->out_frames, frame );
	xbee_fd_source_data_ready( fet->source );
}


static void fet_module_out_queue_del( FetModule* xb )
{
	fet_frame_t *frame;
	assert( xb != NULL );

	frame = (fet_frame_t*)g_queue_peek_tail( xb->out_frames );

	/* Free the data */
	g_free( frame->data );
	frame->data = NULL;

	/* Free the element */
	g_free( frame );

	g_queue_pop_tail( xb->out_frames );
}

int fet_module_transmit( FetModule* xb, const void* buf, uint8_t len )
{
	fet_frame_t *frame;
	assert( xb != NULL && buf != NULL );

	printf("Out: ");
	debug_show_data( buf, len );
	printf("\n");

	frame = g_malloc( sizeof(fet_frame_t) );

	/* Allocate memory for the data and it's sentinel and checksum */
	frame->data = g_malloc( len );
	assert( frame->data != NULL );
	g_memmove( frame->data, buf, len );

	frame->len = len;

	fet_module_out_queue_add_frame( xb, frame );

	return 0;
}

static void fet_module_print_stats( FetModule* xb )
{
	assert( xb != NULL );
	return;

	printf( "\rFrames: %6lu IN, %6lu OUT. Bytes: %9lu IN, %9lu OUT",
		(long unsigned int)xb->frames_rx, 
		(long unsigned int)xb->frames_tx, 
		(long unsigned int)xb->bytes_rx, 
		(long unsigned int)xb->bytes_tx );
}


void fet_free( FetModule* xb )
{
	assert( xb != NULL );

	while( g_queue_get_length( xb->out_frames ) > 0 )
	{
		fet_frame_t *f = (fet_frame_t*)g_queue_peek_tail(xb->out_frames);
		g_free( f->data );
		g_free( f );
		g_queue_pop_tail( xb->out_frames );
	}

	g_queue_free( xb->out_frames );
	xb->out_frames = NULL;
}

gboolean fet_serial_init( FetModule* fet )
{
	struct termios t;
	assert( fet != NULL );

	if( !isatty( fet->fd ) )
	{
		fprintf( stderr, "File isn't a serial device\n" );
		return FALSE;
	}

	if( tcgetattr( fet->fd, &t ) < 0 )
	{
		fprintf( stderr, "Failed to get terminal device information: %m\n" );
		return 0;
	}

	switch( fet->parity )
	{
	case PARITY_NONE:
		t.c_iflag &= ~INPCK;
		t.c_cflag &= ~PARENB;
		break;

	case PARITY_ODD:
		t.c_cflag |= PARODD;

	case PARITY_EVEN:
		t.c_cflag &= ~PARODD;
		
		t.c_iflag |= INPCK;
		t.c_cflag |= PARENB;
	}
	
	/* Ignore bytes with parity errors */
	t.c_iflag |= IGNPAR;

	/* 8 bits */
	/* Ignore break conditions */
	/* Keep carriage returns & prevent carriage return translation */
	t.c_iflag &= ~(ISTRIP | IGNBRK | IGNCR | ICRNL);

	switch( fet->flow_control )
	{
	case FLOW_NONE:
		t.c_iflag &= ~( IXOFF | IXON );
 		t.c_cflag &= ~CRTSCTS;
		break;

	case FLOW_RTSCTS:
		t.c_cflag |= CRTSCTS;
		break;

	case FLOW_SOFTWARE:
 		t.c_cflag |= CRTSCTS;
		t.c_iflag |= IXOFF | IXON;
	}

/* 	t.c_cflag &= ~MDMBUF; */

	/* Disable character mangling */
	t.c_oflag &= ~OPOST;
	
	/* No modem disconnect excitement */
	t.c_cflag &= ~(HUPCL | CSIZE);

	/* Use input from the terminal */
	/* Don't use the carrier detect lines  */
	t.c_cflag |= CREAD | CS8 | CLOCAL;

	switch( fet->stop_bits )
	{
	case 0:
		/*  */
		t.c_cflag &= ~CSTOPB;
		break;

	case 1:
		/*  */
		t.c_cflag &= ~CSTOPB;
		break;

	case 2:
		/*  */
		t.c_cflag |= CSTOPB;
		break;
	}

	/*** c_lflag stuff ***/
	/* non-canonical (i.e. non-line-based) */
	/* no input character looping */
	/* no erase printing or usage */
	/* no special character processing */
	t.c_lflag &= ~(ICANON | ECHO | ECHO | ECHOPRT | ECHOK
			| ECHOKE | ISIG | IEXTEN | TOSTOP /* | NOKERNINFO */ );
	t.c_lflag |= ECHONL;


	if( tcsetattr( fet->fd, TCSANOW, &t ) < 0 )
	{
		fprintf( stderr, "Failed to configure terminal settings: %m\n" );
		return FALSE;
	}

	if( !fet_serial_baud_set( fet->fd, fet->init_baud ) )
		return FALSE;

	return TRUE;
}

FetModule* fet_module_open( char* fname, GMainContext *context,
			      guint baud, guint init_baud )
{
	FetModule *fet = NULL;
	assert( fname != NULL );

	fet = g_object_new( FET_MODULE_TYPE, NULL  );

	fet->fd = open( fname, O_RDWR | O_NONBLOCK );
	if( fet->fd < 0 )
	{
		fprintf( stderr, "Error: Failed to open serial port\n" );
		return NULL; 
	}
	
	if( !fd_set_nonblocking( fet->fd ) )
		return FALSE;

	fet->baud = baud;
	fet->init_baud = init_baud;

	if( !fet_init( fet ) )
		return FALSE;

	fet->source = xbee_fd_source_new( fet->fd, context, (gpointer)fet,
					 (xbee_fd_callback)fet_module_proc_incoming,
					 (xbee_fd_callback)fet_module_proc_outgoing,
					 (xbee_fd_callback)fet_module_io_error,
					 (xbee_fd_callback)fet_module_outgoing_queued );
					 
	return fet;
}

void fet_module_close( FetModule* xb )
{
	assert( xb != NULL );

	close( xb->fd );

	fet_free( xb );

	g_object_unref( xb );
}

GType fet_module_get_type( void )
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (FetModuleClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			NULL,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (FetModule),
			0,      /* n_preallocs */
			fet_instance_init    /* instance_init */
		};
		type = g_type_register_static (G_TYPE_OBJECT,
					       "FetModuleType",
					       &info, 0);
	}
	return type;
}

void fet_instance_init( GTypeInstance *gti, gpointer g_class )
{
	FetModule *xb = (FetModule*)gti;

	/* The default serial mode is 9600bps 8n1  */
	xb->baud = 9600;
	xb->init_baud = 9600;
	xb->parity = PARITY_NONE;
	xb->stop_bits = 1;
	xb->flow_control = FLOW_NONE;

	xb->out_frames = g_queue_new();

	xb->in_len = 0;

	xb->bytes_discarded = 0;
	xb->frames_discarded = 0;
	xb->bytes_rx = xb->bytes_tx = 0;
	xb->frames_rx = xb->frames_tx = 0;

	xb->o_chk = xb->tx_pos = 0;
	xb->checked = FALSE;
	xb->tx_escaped = FALSE;
}

gboolean fet_module_proc_incoming( FetModule* fet )
{
	assert( fet != NULL );

	while( fet_module_read_frame( fet ) == 0 )
	{
		uint16_t flen = ((uint16_t)fet->inbuf[1]) << 8 | fet->inbuf[0];

		printf( "In: " );
		debug_show_data( fet->inbuf + 2, flen - 2 );
		printf("\n");
	}

	return TRUE;
}

/* Reads in available bytes from the input.
 * When a full frame is achieved, it returns 0.
 * When a full frame has not been acheived, it returns 1.
 * When an error occurs, it returns -1 */
int fet_module_read_frame( FetModule* fet )
{
	int r;
	uint8_t d;
	gboolean whole_frame = FALSE;
	assert( fet != NULL && fet->in_len < FET_INBUF_LEN );

	while( !whole_frame )
	{
		r = TEMP_FAILURE_RETRY( read( fet->fd,  &d, 1 ) );

		if( r == -1 )
		{
			if ( errno == EAGAIN )
				break;

			fprintf( stderr, "Error: Failed to read input: %m\n" );
			return -1;
		}

		/* Serial devices can return 0 - but doesn't mean EOF */
		if( r == 0 ) break;

/* 		printf( "Read: 0x%2.2hhx (in_len = %hu)\n", d, fet->in_len ); */

		fet->bytes_rx ++;

		/* FIXME:  At the moment, this assumes that we don't miss any bytes from 
		   the FET tool.  This should be changed. */

		/* Make sure we don't overflow the buffer */
		if( fet->in_len >= FET_INBUF_LEN )
		{
			fprintf( stderr, "Warning: Incoming frame too long - discarding\n" );
			fet->bytes_discarded += fet->in_len;
			fet->in_len = 0;
		}
		
		fet->inbuf[ fet->in_len ] = d;
		fet->in_len ++;


		if( fet->in_len >= 2 )
		{
			uint16_t flen = ((uint16_t)fet->inbuf[1]) << 8 | fet->inbuf[0];

			if( fet->in_len >= (flen + 2) )
			{
				uint16_t chk = ((uint16_t)fet->inbuf[flen+1]) << 8 | fet->inbuf[flen];
				uint16_t calc = crc_block( fet->inbuf + 2, flen - 2 );

/* 				printf( "Checksumming: " ); */
/* 				debug_show_data( fet->inbuf + 2, flen - 2 ); */
/* 				printf ("\n"); */
				if( calc == chk )
					whole_frame = TRUE;
				else {
					g_error( "FAIL: Checksum incorrect (%4.4hx received, %4.4hx calculated)", chk, calc );
					memmove( fet->inbuf, 
						 fet->inbuf + flen + 2, 
						 fet->in_len - (flen + 2 ) );

					printf( "Checksum invalid\n" );

					fet->frames_discarded ++;

					/* Discard current data */
					fet->bytes_discarded += fet->in_len;
					fet->in_len -= flen + 2;
				}

				fet->in_len = 0;
			}
		}
	}

	if( !whole_frame )
		return 1;	/* Not a whole frame yet */

	fet->frames_rx++;
	return 0;	/* Whole frame */
}

void debug_show_frame( uint8_t* buf, uint16_t len )
{
	printf("IN: ");
	debug_show_data( buf, len );
	printf("\n");
}

static void debug_show_data( uint8_t* buf, uint16_t len )
{
	uint16_t i;

	for( i=0 ; i < len; i++ )
	{
		printf( "%2.2X ", (unsigned int)buf[i] );
	}
}

static gboolean fet_module_outgoing_escape_byte( FetModule* fet, uint8_t *d )
{
	fet_frame_t *frame;
	assert( fet != NULL );
	frame = (fet_frame_t*)g_queue_peek_tail( fet->out_frames );

	if( fet->tx_pos != 0 && fet->tx_pos != frame->len + 3
	    && ( *d == 0x7E || *d == 0x7D ) )
	{
		if( !fet->tx_escaped ) {
			*d = 0x7D;
			fet->tx_escaped = TRUE;
			return FALSE;
		} else {
			*d ^= 0x20;
			fet->tx_escaped = FALSE;
			return TRUE;
		}
	} 

	fet->tx_escaped = FALSE;
	return TRUE;
}

gboolean fet_module_io_error( FetModule* xb )
{
	fprintf( stderr, "Erk, error.  I should do something\n" );
	return FALSE;
}

gboolean fet_serial_baud_set( int fd, uint32_t baud )
{
	struct termios t;

	if( tcgetattr( fd, &t ) < 0 )
	{
		fprintf( stderr, "Failed to get terminal device information: %m\n" );
		return FALSE;
	}

	if( cfsetspeed( &t, baud ) < 0 )
	{
		fprintf( stderr, "Failed to set serial baud rate to %u: %m\n", baud );
		return FALSE;
	}

	g_debug( "Setting baud to %u", baud );

	if( tcsetattr( fd, TCSANOW, &t ) < 0 )
	{
		fprintf( stderr, "Failed to configure terminal settings: %m\n" );
		return FALSE;
	}

	return TRUE;
}

static gboolean fd_set_nonblocking( int fd )
{
	int flags;

	flags = fcntl( fd, F_GETFL );
	if( flags == -1 )
	{
		fprintf( stderr, "Failed to set non-blocking IO: %m\n" );
		return FALSE;
	}

	flags |= O_NONBLOCK;

	if( fcntl( fd, F_SETFL, flags ) == -1 )
	{
		fprintf( stderr, "Failed to set non-blocking IO: %m\n" );
		return FALSE;
	}

	return TRUE;
}
