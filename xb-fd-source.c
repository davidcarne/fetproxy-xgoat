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

#include "xb-fd-source.h"
#include <assert.h>

static gboolean xbee_fd_source_prepare( GSource *source, gint *timeout_ );
static gboolean xbee_fd_source_check( GSource *source );
static gboolean xbee_fd_source_dispatch( GSource *source,
					     GSourceFunc callback, 
					     gpointer user_data );
static void xbee_fd_source_finalize( GSource *source );

static GSourceFuncs xbee_fd_sourcefuncs = 
{
	.prepare = xbee_fd_source_prepare,
	.check = xbee_fd_source_check,
	.dispatch = xbee_fd_source_dispatch,
	.finalize = xbee_fd_source_finalize,

	.closure_callback = NULL,
	.closure_marshal = NULL
};

static gboolean xbee_fd_source_event( xbee_fd_source_t* source );

xbee_fd_source_t *xbee_fd_source_new( int fd,
				      GMainContext *context,
				      gpointer userdata,
				      xbee_fd_callback read_callback,
				      xbee_fd_callback write_callback,
				      xbee_fd_callback error_callback,
				      xbee_fd_callback data_ready )
{
	xbee_fd_source_t *source;
	GPollFD *pfd;
	guint id;
	gushort events = G_IO_HUP | G_IO_NVAL | G_IO_ERR;

	if( read_callback != NULL )
		events |= G_IO_IN;

	if( write_callback != NULL )
		events |= G_IO_OUT;

	source = (xbee_fd_source_t*)g_source_new( &xbee_fd_sourcefuncs,
						  sizeof( xbee_fd_source_t ) );

	source->read_callback = read_callback;
	source->write_callback = write_callback;
	source->error_callback = error_callback;
	source->data_ready = data_ready;
	source->userdata = userdata;
	
	pfd = &source->pollfd;
	pfd->fd = fd;
	pfd->events = events;
	g_source_add_poll( (GSource*) source, pfd );

	id = g_source_attach( (GSource*)source, context );

	g_source_set_callback( (GSource*)source,
			       (GSourceFunc)xbee_fd_source_event,
			       (gpointer)source,
			       NULL );

	return source;
}

static gboolean xbee_fd_source_event( xbee_fd_source_t* source )
{
	assert( source != NULL );

	if( source->pollfd.revents & (G_IO_ERR | G_IO_HUP) )
	{
		if( source->error_callback != NULL )
			source->error_callback( source->userdata );

		return FALSE;
	}
	
	if( source->read_callback != NULL 
	    && (source->pollfd.revents & G_IO_IN) )
		if( !source->read_callback( source->userdata ) )
			return FALSE;

	if( source->write_callback != NULL
	    && (source->pollfd.revents & G_IO_OUT) )
		if( !source->write_callback( source->userdata ) )
			return FALSE;

	return TRUE;
}

static gboolean xbee_fd_source_prepare( GSource *source, gint *timeout_ )
{
	assert( timeout_ != NULL && source != NULL );

	*timeout_ = -1;

	/* For the moment, we're not ready */
	return FALSE;
}

static gboolean xbee_fd_source_check( GSource *src )
{
	assert( src != NULL );
	xbee_fd_source_t *source = (xbee_fd_source_t*)src; 
	gushort r = source->pollfd.revents;

	if( r & ( G_IO_IN | G_IO_OUT | G_IO_ERR | G_IO_HUP | G_IO_NVAL) )
		/* ready to dispatch */
		return TRUE;

	return FALSE; 
}

/* Run the callback */
static gboolean xbee_fd_source_dispatch( GSource *src,
					 GSourceFunc callback, 
					 gpointer user_data )
{
	xbee_fd_source_t *source = (xbee_fd_source_t*)src;
	gboolean rval = FALSE;
	assert( source != NULL );

	if( callback != NULL )
		rval = callback( source );

	/* Modulate the write requirement if necessary */
	source->pollfd.events &= ~G_IO_OUT;
	if( source->write_callback != NULL 
		&& ( source->data_ready == NULL 
		     || ( source->data_ready != NULL 
			  && source->data_ready( source->userdata ) ) ) )
		source->pollfd.events |= G_IO_OUT;

	return rval;
}

static void xbee_fd_source_finalize( GSource *source )
{
	/* Don't need to do anything here */
}

void xbee_fd_source_data_ready( xbee_fd_source_t* source )
{
	assert( source != NULL );

	source->pollfd.events |= G_IO_OUT;
}

void xbee_fd_source_free( xbee_fd_source_t* source )
{
	g_source_unref( (GSource*)source );
}
