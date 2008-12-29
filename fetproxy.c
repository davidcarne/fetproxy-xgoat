#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include "fet-module.h"

void config_create( int argc, char **argv );

static gchar *sdev = "/dev/ttyUSB0";
static GMainLoop *ml = NULL;

static GOptionEntry entries[] = 
{
	{ "serial", 's', 0, G_OPTION_ARG_FILENAME, &sdev, "FET Serial Device" },
	{ NULL }
};

gboolean munge_stuff( gpointer data )
{
	FetModule *fet = (FetModule*)data;
	uint8_t d[] = { 0x0d, 0x02, 0x02, 0x00, /* read command */
			0xc0, 0xff, 0x00, 0x00, /* offset */
			64, 0, 0, 0 }; /* length */

	fet_module_transmit( fet, d, sizeof(d) );

	return FALSE;
}

int main( int argc, char** argv )
{
	GMainContext *context;
	FetModule *fet;

	g_type_init();

	config_create( argc, argv );

	ml = g_main_loop_new( NULL, FALSE );
	context = g_main_loop_get_context( ml );

	if( sdev != NULL )
	{
		fet = fet_module_open( sdev, context, 9600, 9600 );
		g_return_val_if_fail( fet != NULL, 1 );
	}

	g_timeout_add( 500, munge_stuff, (gpointer)fet );

	g_main_loop_run( ml );

	fet_module_close( fet );

	return 0;
}

void config_create( int argc, char **argv )
{
	GOptionContext *opt_context;
	GError *error = NULL;
	
	opt_context = g_option_context_new( "" );  
	g_option_context_set_summary( opt_context, "xbee server" );
	g_option_context_add_main_entries(opt_context, entries, NULL);

	if(!g_option_context_parse( opt_context, &argc, &argv, &error ))
	{
		g_print("Error: %s\n", error->message);
		exit(1);
	}

	if( sdev == NULL )
		g_print( "Warning: No serial port specified = no xbee!\n" );
}
