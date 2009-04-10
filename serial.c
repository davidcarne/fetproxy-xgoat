/* Module for interacting with a TI UIF or TI EZ430
   Copyright (C) 2008,2009 Robert Spanton, Tom Bennellick

   This file part of fetproxy.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>. */
#include "serial.h"
#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

static void serial_conn_instance_init( GTypeInstance *gti, gpointer g_class );

/* Set the baud rate to the given baud. */
static gboolean serial_baud_set( int fd, uint32_t baud );

/* Set a file descriptor to be nonblocking */
static gboolean fd_set_nonblocking( int fd );

static gboolean serial_conn_apply_settings( SerialConn *sc );

GType serial_conn_get_type( void )
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (SerialConnClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			NULL,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (SerialConn),
			0,      /* n_preallocs */
			serial_conn_instance_init    /* instance_init */
		};
		type = g_type_register_static (G_TYPE_OBJECT,
					       "SerialConnType",
					       &info, 0);
	}
	return type;
}

static void serial_conn_instance_init( GTypeInstance *gti, gpointer g_class )
{
	SerialConn *sc = (SerialConn*)gti;

	sc->fd = 0;
	sc->channel = NULL;

	sc->settings.baud = 9600;
	sc->settings.parity = PARITY_NONE;
	sc->settings.stop_bits = 1;
	sc->settings.flow_control = FLOW_NONE;

	sc->bytes_rx = sc->bytes_tx = 0;
}

SerialConn* serial_conn_open( char *fname, const serial_settings_t *settings )
{
	SerialConn *sc = NULL;
	g_assert( fname != NULL && settings != NULL );

	sc = g_object_new( SERIAL_CONN_TYPE, NULL  );

	sc->fd = open( fname, O_RDWR | O_NONBLOCK );
	if( sc->fd < 0 ) {
		fprintf( stderr, "Error: Failed to open serial port\n" );
		return NULL; 
	}

	if( !fd_set_nonblocking( sc->fd ) )
		return FALSE;

	sc->settings = *settings;

	g_assert( serial_conn_apply_settings( sc ) );

	/* Pass the file descriptor on to a GIOChannel   */
	sc->channel = g_io_channel_unix_new( sc->fd );
	g_io_channel_set_encoding( sc->channel, NULL, NULL );
	g_io_channel_set_buffered( sc->channel, FALSE );

	return sc;
}

SerialConn* serial_conn_close( SerialConn* sc )
{
	return NULL;
}

static gboolean serial_baud_set( int fd, uint32_t baud )
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

static gboolean serial_conn_apply_settings( SerialConn *sc )
{
	struct termios t;
	g_assert( sc != NULL );

	if( !isatty( sc->fd ) )	{
		fprintf( stderr, "File isn't a serial device\n" );
		return FALSE;
	}

	if( tcgetattr( sc->fd, &t ) < 0 ) {
		fprintf( stderr, "Failed to get terminal device information: %m\n" );
		return 0;
	}

	switch( sc->settings.parity )
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

	switch( sc->settings.flow_control )
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

	switch( sc->settings.stop_bits )
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


	if( tcsetattr( sc->fd, TCSANOW, &t ) < 0 )
	{
		fprintf( stderr, "Failed to configure terminal settings: %m\n" );
		return FALSE;
	}

	if( !serial_baud_set( sc->fd, sc->settings.baud ) )
		return FALSE;

	return TRUE;
}

GIOChannel* serial_conn_get_io_channel( SerialConn* sc )
{
	g_assert( sc != NULL );
	
	return sc->channel;
}
