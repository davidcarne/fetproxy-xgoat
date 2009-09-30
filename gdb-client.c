#include "gdb-client.h"
#include <ctype.h>
#include <stdio.h>

/* Instance initialisation */
static void gdb_client_instance_init( GTypeInstance *gti, gpointer g_class );

/* Process incoming data */
static gboolean gdb_client_incoming( GIOChannel *source,
				     GIOCondition cond,
				     gpointer _cli );

/* Socket write callback */
static gboolean gdb_client_write_cb( GIOChannel *source,
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

/* Add a frame to the transmit queue.
 * Copies the data. */
static void gdb_client_tx_queue( GdbClient *cli,
				 gboolean wrap,
				 uint8_t *data,
				 uint16_t len );

static void gdb_client_proc_next_frame( GdbClient *cli );

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
	rem->recv_escape_next = FALSE;

	rem->chk_recv = 0;
	rem->chk_recv_pos = 0;

	rem->out_q = g_queue_new();
	rem->in_q = g_queue_new();
	rem->wait_state = GDB_CLIENT_IDLE;
}

GdbClient* gdb_client_new( GTcpSocket *sock, gdb_client_callbacks_t *cb )
{
	GdbClient *cli = g_object_new( GDB_CLIENT_TYPE, NULL );
	GIOChannel *c = gnet_tcp_socket_get_io_channel( sock );
	GError *err = NULL;

	cli->sock = sock;
	cli->target_cb = cb;
	cb->init( cli, cb->userdata );

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
		else if( b == '}' )
			cli->recv_escape_next = TRUE;
		else if( cli->inpos > GDB_CLIENT_INBUF_LEN ) {
			g_warning( "Incoming frame buffer to long to store incoming frame -- discarding." );
			cli->recv_state = GDB_REM_RECV_IDLE;
		} else {
			if( cli->recv_escape_next ) {
				b ^= 0x20;
				cli->recv_escape_next = FALSE;
			}

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
				g_debug( "(checksum = %x)", cli->chk_recv );
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
	gdb_client_frame_t *f;

	g_debug( "Frame added to incoming queue" );
	/* Stick it on the incoming queue */
	f = g_malloc( sizeof(gdb_client_frame_t) );
	f->data = NULL;
	f->len = cli->inpos;
	if( cli->inpos > 0 )
		f->data = g_memdup( cli->inbuf, cli->inpos );

	g_queue_push_tail( cli->in_q, f );
	gdb_client_proc_next_frame( cli );
}

static gboolean gdb_client_write_cb( GIOChannel *source,
				     GIOCondition cond,
				     gpointer _cli )
{
	GdbClient *cli = (GdbClient*)_cli;
	gdb_client_frame_t *frame = (gdb_client_frame_t*)g_queue_peek_tail( cli->out_q );
	uint8_t tx;
	GIOStatus s;
	gsize w;
	GError *err = NULL;
	gboolean lose_frame = FALSE;

	/* No more frames to transmit */
	if( frame == NULL )
		return FALSE;

	switch( cli->tx_state ) {
	case GDB_REM_TX_IDLE:
		cli->opos = 0;

		cli->tx_state = GDB_REM_TX_DATA;

		if( frame->wrap ) {
			tx = '$';
			
			if( frame->len == 0 )
				/* There's no data, so skip to the checksum */
				cli->tx_state = GDB_REM_TX_CHK_BOUNDARY;
		}
		else {
			g_assert( frame->len > 0 );
			tx = frame->data[0];
			cli->opos++;

			if( frame->len == 1 ) {
				/* We have transmitted all the frame */
				lose_frame = TRUE;
				cli->tx_state = GDB_REM_TX_IDLE;
			}
		}
		break;

	case GDB_REM_TX_DATA:
		tx = frame->data[ cli->opos ];
		
		cli->opos++;
		if( cli->opos == frame->len ) {
			if( !frame->wrap ) {
				lose_frame = TRUE;
				cli->tx_state = GDB_REM_TX_IDLE;
			} else
				cli->tx_state = GDB_REM_TX_CHK_BOUNDARY;
		}
		break;

	case GDB_REM_TX_CHK_BOUNDARY:
		tx = '#';
		cli->o_chk_pos = 0;
		cli->o_chk = gdb_client_checksum( frame->data, frame->len );
		cli->tx_state = GDB_REM_TX_CHK;
		break;

	case GDB_REM_TX_CHK: {
		const uint8_t lut[] = "0123456789ABCDEF";

		tx = lut[ ( cli->o_chk >> (4*(1-cli->o_chk_pos)) ) & 0x0f ];
		
		cli->o_chk_pos++;
		if( cli->o_chk_pos == 2 ) {
			lose_frame = TRUE;
			cli->tx_state = GDB_REM_TX_IDLE;
		}
		break;
	}

	default:
		g_error( "Erroneous state in tx state machine" );

	}

	if( lose_frame ) {
		g_queue_pop_tail( cli->out_q );

		if( frame->data != NULL )
			g_free( frame->data );

		g_free( frame );
	}

	s = g_io_channel_write_chars( source,
				      (gchar*)&tx, 1,
				      &w, &err );

	if( s != G_IO_STATUS_NORMAL )
		g_error( "Failed to write byte." );

	g_debug( "Wrote '%c'", tx );

	if( g_queue_is_empty( cli->out_q ) )
		return FALSE;

	return TRUE;
}

static void gdb_client_tx_queue( GdbClient *cli,
				 gboolean wrap,
				 uint8_t *data,
				 uint16_t len )
{
	gdb_client_frame_t *frame;
	g_debug( "GdbClient: Adding frame to output queue" );

	frame = g_malloc( sizeof(gdb_client_frame_t) );

	frame->data = NULL;
	if( len != 0 )
		frame->data = g_memdup( data, len );

	frame->len = len;
	frame->wrap = wrap;

	/* If the frame output queue isn't empty, then the write callback
	   is already configured */
	if( g_queue_is_empty( cli->out_q ) ) {
		GIOChannel *c = gnet_tcp_socket_get_io_channel( cli->sock );
		g_io_add_watch( c, G_IO_OUT, gdb_client_write_cb, cli );
	}

	/* Add to the queue */
	g_queue_push_head( cli->out_q, frame );
}

static void gdb_client_proc_next_frame( GdbClient *cli )
{
	gdb_client_frame_t *frame;

	if( cli->wait_state != GDB_CLIENT_IDLE )
		return;

	frame = g_queue_peek_head(cli->in_q);

	/* No frame there */
	if( frame == NULL )
		return;

	if( frame->len == 0 ) {
		g_warning( "Ignoring zero length frame." );
		goto free_frame;
	}

	/* ACK */
	gdb_client_tx_queue( cli, FALSE, (uint8_t*)"+", 1 );

	switch( cli->inbuf[0] ) {
	case '?':
		/* gdb's asking why we halted */
		/* Say that we haven't! */
		gdb_client_tx_queue( cli, TRUE, (uint8_t*)"T00", 3 );
		break;

	case 'g':
		/* gdb wants to know the contents of our registers */
		cli->wait_state = GDB_CLIENT_REG_READ;
		cli->target_cb->read_registers( cli->target_cb->userdata );
		break;

	case 'c':
		cli->wait_state = GDB_CLIENT_CONTINUE;
		cli->target_cb->cont( cli->target_cb->userdata );
		break;

	default:
		/* We don't support that command */
		gdb_client_tx_queue( cli, TRUE, (uint8_t*)"", 0 );
	}

free_frame:
	if( frame->data != NULL )
		g_free( frame->data );
	g_free( frame );
	g_queue_pop_head( cli->in_q );
}

void gdb_client_command_complete( gdb_client_info_t *state, gpointer _cli )
{
	GdbClient *cli = GDB_CLIENT(_cli);

	switch( cli->wait_state ) {
	case GDB_CLIENT_REG_READ:
	{
		const char lut[] = "0123456789abcdef";
		char buf[64];
		char *p;
		uint8_t i;

		for( i=0, p=buf; i<16; i++, p+=4 ) {
			p[3] = lut[ state->reg[i] & 0x0f ];
		        p[2] = lut[ (state->reg[i] >> 4) & 0x0f ];
		        p[1] = lut[ (state->reg[i] >> 8) & 0x0f ];
		        p[0] = lut[ (state->reg[i] >> 12) & 0x0f ];
		}

		gdb_client_tx_queue( cli, TRUE, buf, 64 );

		cli->wait_state = GDB_CLIENT_IDLE;
		break;
	}

	case GDB_CLIENT_CONTINUE:
		/* gdb doesn't need to know that the target is now running */
		cli->wait_state = GDB_CLIENT_IDLE;
		break;


	default:
		g_debug( "Ignoring command complete call from FetModule" );
	}

	gdb_client_proc_next_frame( cli );
}
