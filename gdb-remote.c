#include "gdb-remote.h"
#include <gnet.h>

static void gdb_remote_instance_init( GTypeInstance *gti, gpointer g_class );

/* Callback for when a client connects */
static void gdb_remote_accept( GTcpSocket *server,
			       GTcpSocket *client,
			       gpointer _rem )
{
	GdbRemote *rem = (GdbRemote*)_rem;

	rem->client = gdb_client_new( client );
}

GType gdb_remote_get_type( void )
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (GdbRemoteClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			NULL,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (GdbRemote),
			0,      /* n_preallocs */
			gdb_remote_instance_init    /* instance_init */
		};
		type = g_type_register_static (G_TYPE_OBJECT,
					       "GdbRemoteType",
					       &info, 0);
	}
	return type;
}

static void gdb_remote_instance_init( GTypeInstance *gti, gpointer g_class )
{
/* 	GdbRemote *rem = (GdbRemote*)gti; */
}

GdbRemote* gdb_remote_listen( uint16_t port )
{
	GdbRemote *rem = NULL;

	rem = g_object_new( GDB_REMOTE_TYPE, NULL  );

	rem->tcp = gnet_tcp_socket_server_new_with_port( port );
	if( rem->tcp == NULL )
		g_error( "Failed to listen on port %hhu", port );
	
	gnet_tcp_socket_server_accept_async( rem->tcp, gdb_remote_accept, rem );

	return rem;
}
