#define _GNU_SOURCE
#include "gdb-client.h"
#include <ctype.h>
#include <stdio.h>

/* Instance initialisation */
static void gdb_client_instance_init( GTypeInstance *gti, gpointer g_class );

/* Process incoming data */
static gboolean gdb_client_incoming( GIOChannel *source,
				     GIOCondition cond,
				     gpointer _cli );

/* HUP handler  */
static gboolean gdb_client_hup( GIOChannel *source,
				GIOCondition cond,
				gpointer _cli );

/* Process an incoming byte */
static void gdb_client_proc_byte( GdbClient *cli, uint8_t b );

static uint8_t gdb_client_checksum( uint8_t* data, uint16_t len );

/* Process a received frame */
static void gdb_client_proc_frame( GdbClient *cli );

/* Return in the lower nibble.
 * 0xff if the character isn't found. */
static uint8_t hex_dig_to_nibble( gchar h )
{
	const gchar lut[] = "0123456789ABCDEF";
	const gchar *p;
	
	h = toupper(h);

	for( p=lut; *p!=0; p++ )
		if( *p == h )
			return p-lut;

	return 0xff;	
}

static void debug_frame_out( uint8_t *d, uint16_t len )
{
	uint32_t i;

	printf( "FRAME: " );
	for( i=0; i<len; i++ )
		printf( "%c", d[i] );
	printf( "\n" );
}

GType gdb_client_get_type( void )
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (GdbClientClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			NULL,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (GdbClient),
			0,      /* n_preallocs */
			gdb_client_instance_init    /* instance_init */
		};
		type = g_type_register_static (G_TYPE_OBJECT,
					       "GdbClientType",
					       &info, 0);
	}
	return type;
}

static void gdb_client_instance_init( GTypeInstance *gti, gpointer g_class )
{
	GdbClient *rem = (GdbClient*)gti;

	rem->sock = NULL;
	rem->inpos = 0;
	rem->recv_state = GDB_REM_RECV_IDLE;

	rem->chk_recv = 0;
	rem->chk_recv_pos = 0;
}

GdbClient* gdb_client_new( GTcpSocket *sock )
{
	GdbClient *cli = g_object_new( GDB_CLIENT_TYPE, NULL );
	GIOChannel *c = gnet_tcp_socket_get_io_channel( sock );
	GError *err = NULL;

	cli->sock = sock;

	g_debug( "New client." );

	/* Make it non-blocking */
	if( g_io_channel_set_flags( c, G_IO_FLAG_NONBLOCK, &err ) != G_IO_STATUS_NORMAL )
		g_error( "Failed to set socket to non-blocking: %s",
			 err->message );

	g_io_add_watch( c, G_IO_IN, gdb_client_incoming, cli );
	g_io_add_watch( c, G_IO_HUP, gdb_client_hup, cli );

	return cli;
}

static gboolean gdb_client_incoming( GIOChannel *source,
				     GIOCondition cond,
				     gpointer _cli )
{
	gchar b;
	gsize r;
	GError *err = NULL;
	GIOStatus stat;
	GdbClient *cli = (GdbClient*)_cli;
	g_debug( "incoming" );

	do
	{
		stat = g_io_channel_read_chars( source, &b, 1, &r, &err );

		if( stat == G_IO_STATUS_EOF ) {
			cli->sock = NULL;
			g_debug( "TODO: Client disconnection not yet supported!" );
			return FALSE;
		}

		if( stat == G_IO_STATUS_NORMAL )
			gdb_client_proc_byte( cli, b );
	}
	while( stat == G_IO_STATUS_NORMAL );	

	return TRUE;
}

static gboolean gdb_client_hup( GIOChannel *source,
				GIOCondition cond,
				gpointer _cli )
{
	GdbClient *cli = (GdbClient*)_cli;
	g_debug( "TODO: handle hup!" );

	gnet_tcp_socket_delete( cli->sock );
	cli->sock = NULL;
	
	return FALSE;
}

static void gdb_client_proc_byte( GdbClient *cli, uint8_t b )
{
	switch( cli->recv_state ) {
	case GDB_REM_RECV_IDLE:
		cli->inpos = 0;
		cli->chk_recv = 0;
		cli->chk_recv_pos = 0;

		if( b == '$' )
			cli->recv_state = GDB_REM_RECV_DATA;
		break;

	case GDB_REM_RECV_DATA:
		if( b == '#' )
			cli->recv_state = GDB_REM_RECV_CHECKSUM;
		else if( cli->inpos > GDB_CLIENT_INBUF_LEN ) {
			g_warning( "Incoming frame buffer to long to store incoming frame -- discarding." );
			cli->recv_state = GDB_REM_RECV_IDLE;
		} else {
			cli->inbuf[ cli->inpos ] = b;
			cli->inpos++;
		}
		break;

	case GDB_REM_RECV_CHECKSUM:
	{
		/* Character from checksum */
		uint8_t c = hex_dig_to_nibble(b);

		if( c == 0xff ) {
			g_warning( "Invalid character received in checksum field" );
			cli->recv_state = GDB_REM_RECV_IDLE;
		} else {
			cli->chk_recv |= c << (4 * (1-cli->chk_recv_pos));
			cli->chk_recv_pos++;

			if( cli->chk_recv_pos == 2 ) {
				debug_frame_out( cli->inbuf, cli->inpos );
				if( gdb_client_checksum( cli->inbuf, cli->inpos ) == cli->chk_recv )
					gdb_client_proc_frame( cli );

				cli->recv_state = GDB_REM_RECV_IDLE;
			}
		}
		break;		
	}

	default:
		g_debug( "Ignoring incoming character '%c'", b );
	}
}

static uint8_t gdb_client_checksum( uint8_t* data, uint16_t len )
{
	uint32_t i;
	uint8_t c = 0;

	for( i=0; i<len; i++ )
		c += data[i];

	return c;
}

static void gdb_client_proc_frame( GdbClient *cli )
{
	g_debug( "Yay, received frame" );
}
