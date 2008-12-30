#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include "fet-module.h"
#include "fet-commands.h"
#include "elf-access.h"

void config_create( int argc, char **argv );

void send_elf( FetModule *fet, char* fname );

void send_elf_section( FetModule *fet, elf_section_t* sec );

elf_section_t *cur_sec = NULL;
elf_section_t *text = NULL, *vectors = NULL;

gboolean send_elf_timeout( gpointer data );

void send_elf_section_chunk( FetModule *fet, elf_section_t* sec, uint32_t *pos );

static gchar *sdev = "/dev/ttyUSB0";
static gchar *elf_file = NULL;
static GMainLoop *ml = NULL;

static GOptionEntry entries[] = 
{
	{ "serial", 's', 0, G_OPTION_ARG_FILENAME, &sdev, "FET Serial Device" },
	{ "load-file", 'l', 0, G_OPTION_ARG_FILENAME, &elf_file, "ELF file to load into device" },
	{ NULL }
};

#define P1DIR 0x22
#define P1OUT 0x21
#define P1SEL 0x26

gboolean munge_stuff( gpointer data )
{
	uint8_t d;
	static uint8_t v = 1;
	FetModule *fet = (FetModule*)data;

	d = 1;
	fet_cmd_write_mem( fet, P1DIR, &d, 1 );

	d = 0;
	fet_cmd_write_mem( fet, P1SEL, &d, 1 );

	fet_cmd_write_mem( fet, P1OUT, &v, 1 );
	v ^= 1;

	return TRUE;
}

gboolean init_stuff( gpointer data )
{
	FetModule *fet = (FetModule*)data;

	fet_cmd_open(fet);
	fet_cmd_init(fet);
	fet_cmd_conf(fet, TRUE);
	fet_cmd_set_vcc( fet, 3000 );
	fet_cmd_identify( fet );

	if( elf_file != NULL ) {
		fet_cmd_erase( fet, FET_ERASE_ALL, 0 );
		send_elf( fet, elf_file );
	}
	else {
		g_timeout_add( 500, munge_stuff, (gpointer)fet );
		munge_stuff((gpointer)fet);
	}
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

	g_timeout_add( 0, init_stuff, (gpointer)fet );

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

void send_elf( FetModule *fet, char* fname )
{
	elf_access_load_sections( fname, &text, &vectors );

	g_timeout_add( 200, send_elf_timeout, (gpointer)fet );
}

void send_elf_section_chunk( FetModule *fet, elf_section_t* section, uint32_t *pos )
{
	uint32_t rem;
	const uint8_t CHUNK_LEN = 32;
	g_assert( (*pos) < section->len );

	rem = section->len - (*pos);

	if( rem < CHUNK_LEN )
		fet_cmd_write_mem( fet, 
				   section->addr + (*pos),
				   section->data + (*pos),
				   rem );
	else
		fet_cmd_write_mem( fet, 
				   section->addr + (*pos),
				   section->data + (*pos),
				   CHUNK_LEN );

	*pos += CHUNK_LEN;
}

gboolean send_elf_timeout( gpointer data )
{
	static uint32_t pos = 0;
	FetModule *fet = (FetModule*)data;

	if( cur_sec == NULL )
		cur_sec = text;

	send_elf_section_chunk( fet, cur_sec, &pos );
	
	if( pos >= cur_sec->len ) {
		if( cur_sec == text ) {
			cur_sec = vectors;
			pos = 0;
		} else {
			fet_cmd_run( fet );

			/* Stop calling me */
			return FALSE;
		}
	}

	return TRUE;
}
