/* Client for speaking the gdb remote serial protocol.
   Copyright (C) 2009 Robert Spanton, Tom Bennellick

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
#ifndef __GDB_REMOTE_H
#define __GDB_REMOTE_H
#include <glib.h>
#include <glib-object.h>
#include <stdint.h>
#include <gnet.h>
#include "gdb-client.h"

typedef struct gdb_remote_ts GdbRemote;

typedef struct
{
	GObjectClass parent;

	/* Nothing here */
} GdbRemoteClass;

GType gdb_remote_get_type( void );

#define GDB_REMOTE_TYPE (gdb_remote_get_type())
#define GDB_REMOTE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GDB_REMOTE_TYPE, GdbRemote))
#define GDB_REMOTE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GDB_REMOTE, GdbRemoteClass))
#define GDB_IS_REMOTE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDB_REMOTE_TYPE))
#define GDB_IS_REMOTE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GDB_REMOTE_TYPE))
#define GDB_REMOTE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GDB_REMOTE_TYPE, GdbRemoteClass))

struct gdb_remote_ts
{
	GObject parent;

	GTcpSocket *tcp;

	/* We currently only support one client */
	GdbClient *client;
};

/* Start listening on the given port */
GdbRemote* gdb_remote_listen( uint16_t port );

#endif	/* __GDB_REMOTE_H */
