/* Module for interacting with a TI UIF or TI EZ430
   Copyright (C) 2008 Robert Spanton, Tom Bennellick

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

#ifndef __FET_MODULE_H
#define __FET_MODULE_H
#define _GNU_SOURCE		/* For TEMP_FAILURE_RETRY */
#include <stdio.h>
#include <sys/time.h>
#include <stdint.h>
#include <assert.h>
#include <glib.h>
#include <glib-object.h>
#include "xb-fd-source.h"
#include "serial.h"

#define FET_INBUF_LEN 512
#define FET_OUTBUF_LEN 512

/* Frames that head out of the FET.
 * Data is the frame payload.
 * i.e. it does _not_ contain the frame sentinels or checksum */
typedef struct
{
	uint16_t len;
	uint8_t *data;
} fet_frame_t;

struct fet_ts;

typedef struct fet_ts FetModule;	

typedef struct
{
	GObjectClass parent;

	/* Nothing here */
} FetModuleClass;

GType fet_module_get_type( void );

#define FET_MODULE_TYPE (fet_module_get_type())
#define FET_MODULE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), FET_MODULE_TYPE, FetModule))
#define FET_MODULE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), FET_MODULE, FetModuleClass))
#define FET_IS_MODULE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FET_MODULE_TYPE))
#define FET_IS_MODULE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FET_MODULE_TYPE))
#define FET_MODULE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), FET_MODULE_TYPE, FetModuleClass))

struct fet_ts
{
	GObject parent;

	/* private */
	SerialConn *serial;
	/* Whether we're currently following writability */
	gboolean mon_write;
	/* The channel for monitoring the serial */
	GIOChannel *ioc;

	/*** Transmission ***/
	/* Queue of outgoing frames - all of xb_frame_t */
	/* Note: the data in these frames has already been escaped */
	GQueue* out_frames; 

	/* Checksum value for the currently transmitting frame */
	uint16_t o_chk;
	gboolean checked;	/* Whether the checksum has been calculated */
	gboolean tx_escaped;	/* Whether the next byte has been escaped */
	/* The next byte to be transmitted within the current frame */
	uint16_t tx_pos;

	/*** Reception ***/
	/* Buffer of incoming data */
	/* Always contains the beginning part of a frame */
	uint8_t inbuf[FET_INBUF_LEN];
	/* The number of bytes in the input buffer */
	uint16_t in_len;

	/* Callback for receiving a frame.
	 * The data pointed to contains the beginning part of the frame */
	gpointer *userdata;
	
	/*** Stats ***/
	uint32_t bytes_discarded, frames_discarded; /* Frames with invalid checksums */
	uint32_t bytes_rx, bytes_tx;   
	uint32_t frames_rx, frames_tx;  /* Valid checksum frames received */

	/*** FET Properties ***/
};

/* Create a connection to an xbee.
 * Opens the serial port given in fname, and fills the 
 * structure *xb with stuff. */
FetModule* fet_module_open( char *fname, GMainContext *context );

/* Close an xbee connection */
void fet_module_close( FetModule *xb );

/* Add an xbee to a mainloop */
void fet_module_add_source( FetModule *xb, GMainContext *context );

/* Transmit a frame */
int fet_module_transmit( FetModule* fet, const void* buf, uint8_t len );

/* Register Callbacks */
/* void fet_module_register_callbacks ( FetModule *xb, fet_module_events_t *callbacks, gpointer *userdata); */

#endif	/* __FET_MODULE_H */
