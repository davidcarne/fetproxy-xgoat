/*   Copyright (C) 2008 Robert Spanton

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */
#include "elf-access.h"
#include <libelf.h>
#include <fcntl.h>
#include <glib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

/* Allocate memory for a section and copy it into memory.
   Arguments:
    - section: The section to copy from.
    -  header: The section header for the section to copy from. */
static elf_section_t* elf_access_load_section( Elf_Scn *section,
					       Elf32_Shdr *header );

void elf_access_load_sections( char* fname,
			       elf_section_t **text,
			       elf_section_t **vectors )
{
	int efd;
	Elf *elf;
	size_t stable;
	Elf32_Ehdr *ehdr;
	Elf_Scn *section;
	g_assert( fname != NULL && text != NULL && vectors != NULL );

        if( elf_version(EV_CURRENT) == EV_NONE )
		g_error( "ELF library initialization failed: %s", elf_errmsg(-1) );

	efd = open( fname , O_RDONLY );
	if( efd < 0 )
		g_error( "Failed to open elf file 'motor': %m" );

	elf = elf_begin( efd, ELF_C_READ, NULL );
	if( elf == NULL )
		g_error( "elf_begin failed: %s", elf_errmsg(-1) );
	
	if( elf_kind( elf ) != ELF_K_ELF )
		g_error( "ELF file is of wrong type" );

	/* Find the string table section index */
	ehdr = elf32_getehdr( elf );
	if( ehdr == NULL )
		g_error( "Failed to get elf header: %s", elf_errmsg(-1) );
	stable = ehdr->e_shstrndx;

	/* Find the .text and .vectors sections */
	section = NULL;
	/* Loop through all the sections */
	while( (section = elf_nextscn( elf, section )) != NULL ) {
		Elf32_Shdr *hdr;
		char *name;

		/* Retrieve the section header */
		hdr = elf32_getshdr( section );
		if( hdr == NULL )
			g_error( "Couldn't retrieve section header: %s", elf_errmsg(-1) );
		
		name = elf_strptr( elf, stable, hdr->sh_name );
		if( name == NULL )
			g_error( "Couldn't get section name: %s", elf_errmsg(-1) );

		if( strcmp( name, ".text" ) == 0 ) {
			*text = elf_access_load_section( section, hdr );
			(*text)->name = ".text";
		}
		else if( strcmp( name, ".vectors" ) == 0 ) {
			*vectors = elf_access_load_section( section, hdr );
			(*vectors)->name = ".vectors";
		}
	}	

	if( text == NULL )
		g_error( ".text section not found in ELF file" );
	if( vectors == NULL )
		g_error( ".vectors section not found in ELF file" );

	elf_end( elf );
	close( efd );
}

static elf_section_t* elf_access_load_section( Elf_Scn *section, Elf32_Shdr *header )
{
	Elf_Data *d = NULL;
	elf_section_t *s = NULL;
	uint8_t *tpos;
	g_assert( section != NULL && header != NULL );

	s = g_malloc( sizeof(elf_section_t) );
	s->data = tpos = g_malloc( header->sh_size );
	s->len = header->sh_size;
	s->addr = header->sh_addr;

	/* The ELF library returns data in multiple buffers, which we iterate through */
	while ( (d = elf_rawdata( section, d )) != NULL )
	{
		g_memmove( tpos, d->d_buf, d->d_size );
		tpos += d->d_size;
	}

	return s;
}
