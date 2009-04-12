#ifndef __GDB_CLIENT_H
#define __GDB_CLIENT_H
#include <glib.h>
#include <glib-object.h>
#include <stdint.h>
#include <gnet.h>

typedef struct gdb_client_ts GdbClient;

typedef struct
{
	GObjectClass parent;

	/* Nothing here */
} GdbClientClass;

GType gdb_client_get_type( void );

#define GDB_CLIENT_TYPE (gdb_client_get_type())
#define GDB_CLIENT(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GDB_CLIENT_TYPE, GdbClient))
#define GDB_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GDB_CLIENT, GdbClientClass))
#define GDB_IS_CLIENT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDB_CLIENT_TYPE))
#define GDB_IS_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GDB_CLIENT_TYPE))
#define GDB_CLIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GDB_CLIENT_TYPE, GdbClientClass))

#define GDB_CLIENT_INBUF_LEN 1024

struct gdb_client_ts
{
	GObject parent;

	/* Socket that we're using */
	GTcpSocket *sock;

	uint8_t inbuf[GDB_CLIENT_INBUF_LEN];
	uint16_t inpos;

	/* The receiver state */
	enum {
		/* Idle */
		GDB_REM_RECV_IDLE,

		/* Receiving the packet data */
		GDB_REM_RECV_DATA,

		/* Receiving the frame checksum */
		GDB_REM_RECV_CHECKSUM
	} recv_state;

	gboolean recv_escape_next;

	/* The received checksum */
	uint8_t chk_recv;
	uint8_t chk_recv_pos;
};

/* Create a new client. 
 * Arguments:
 *  - sock: The socket that the client is connected through.
 * Returns: The GdbClient object. */
GdbClient* gdb_client_new( GTcpSocket *sock );

#endif	/* __GDB_CLIENT_H */
