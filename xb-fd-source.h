/* A glib source for monitoring a file descriptor. 
   Copyright (C) 2008 Robert Spanton, Philip Smith

   This file part of libxb.

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

#ifndef __COMMON_H
#define __COMMON_H
#include <glib.h>
#include <glib-object.h>

typedef gboolean (*xbee_fd_callback) ( gpointer );

typedef struct
{
	/* source is used internally in GSource code */
	GSource source;
	GPollFD pollfd;

	gpointer userdata;

	xbee_fd_callback read_callback, write_callback, error_callback;

	/* Whether data's ready */
	xbee_fd_callback data_ready;
} xbee_fd_source_t;

xbee_fd_source_t* xbee_fd_source_new( int fd,
				      GMainContext *context,
				      gpointer userdata,
				      xbee_fd_callback read_callback,
				      xbee_fd_callback write_callback,
				      xbee_fd_callback error_callback,
				      xbee_fd_callback data_ready );

void xbee_fd_source_data_ready( xbee_fd_source_t* source );

void xbee_fd_source_free( xbee_fd_source_t* source );

#endif	/* __COMMON_H */
