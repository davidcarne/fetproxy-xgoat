/* Copyright (C) 2008 Robert Spanton, Tom Bennellick

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
#include <glib.h>
#include <gnet.h>
#include <stdio.h>
#include <stdlib.h>
#include "fet-module.h"
#include "fet-commands.h"
#include "elf-access.h"
#include "gdb-remote.h"
#include "gdb-client.h"

void config_create( int argc, char **argv );

void send_elf( FetModule *fet, char* fname );

void send_elf_section( FetModule *fet, elf_section_t* sec );

elf_section_t *cur_sec = NULL;
elf_section_t *text = NULL, *vectors = NULL;

gboolean send_elf_timeout( gpointer data );

void send_elf_section_chunk( FetModule *fet, elf_section_t* sec, uint32_t *pos );

static gchar *sdev = "/dev/ttyUSB0";
static gchar *elf_file = NULL;
static gint port = 2000;
static GMainLoop *ml = NULL;

static GOptionEntry entries[] = 
{
	{ "serial", 's', 0, G_OPTION_ARG_FILENAME, &sdev, "FET Serial Device" },
	{ "load-file", 'l', 0, G_OPTION_ARG_FILENAME, &elf_file, "ELF file to load into device" },
	{ "port", 'p', 0, G_OPTION_ARG_INT, &port, "Port to listen for gdb on" },
	{ NULL }
};

#define P1DIR 0x22
#define P1OUT 0x21
#define P1SEL 0x26

gboolean init_stuff( gpointer data )
{
	FetModule *fet = (FetModule*)data;

	fet_cmd_open(fet);
	fet_cmd_init(fet);
	fet_cmd_conf(fet, TRUE);
	fet_cmd_set_vcc( fet, 3000 );
	fet_cmd_identify( fet );

	return FALSE;
}

int main( int argc, char** argv )
{
	GMainContext *context;
	FetModule *fet;
	GdbRemote *rem;
	gdb_client_callbacks_t fet_callbacks =
	{
		.init = fet_module_gdbclient_init,
		.read_registers = fet_cmd_read_context,
		.cont = fet_cmd_run
	};

	g_type_init();
	gnet_init();

	config_create( argc, argv );

	ml = g_main_loop_new( NULL, FALSE );
	context = g_main_loop_get_context( ml );

	if( sdev != NULL )
	{
		fet = fet_module_open( sdev, context );
		g_return_val_if_fail( fet != NULL, 1 );
	}

	/* Pass the FetModule* to all the FetModule callbacks */
	fet_callbacks.userdata = fet;

	rem = gdb_remote_listen( port, &fet_callbacks );

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
