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
#ifndef __SERIAL_H
#define __SERIAL_H
#include <glib.h>
#include <glib-object.h>
#include <stdint.h>

struct serial_ts;
typedef struct serial_ts SerialConn;

typedef struct
{
	GObjectClass parent;

	/* Nothing here */
} SerialConnClass;

GType serial_conn_get_type( void );

#define SERIAL_CONN_TYPE (serial_conn_get_type())
#define SERIAL_CONN(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), SERIAL_CONN_TYPE, SerialConn))
#define SERIAL_CONN_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SERIAL_CONN, SerialConnClass))
#define SERIAL_IS_CONN(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SERIAL_CONN_TYPE))
#define SERIAL_IS_CONN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SERIAL_CONN_TYPE))
#define SERIAL_CONN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SERIAL_CONN_TYPE, SerialConnClass))

typedef struct
{
	/*** serial configuration ***/
	uint32_t baud;
	enum { PARITY_NONE, PARITY_ODD, PARITY_EVEN } parity;
	uint8_t stop_bits;
	enum { FLOW_NONE, FLOW_RTSCTS, FLOW_SOFTWARE } flow_control;
} serial_settings_t;

struct serial_ts
{
	GObject parent;

	/* private */
	int fd;
	GIOChannel *channel;

	serial_settings_t settings;

	/*** Stats ***/
	uint32_t bytes_rx, bytes_tx;   
};

/* Open a serial port.
 * Arguments:
 *  -    fname: The path of the serial device.
 *  - settings: The serial port options to apply.
 * Returns handle to the connection. */
SerialConn* serial_conn_open( char *fname, const serial_settings_t *settings );

SerialConn* serial_conn_close( SerialConn* sc );

/* Get the IO channel  */
GIOChannel* serial_conn_get_io_channel( SerialConn* sc );

#endif	/* __SERIAL_H */
