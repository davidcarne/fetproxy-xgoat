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
/* Routines for accessing data from elf files */
#ifndef __ELF_ACCESS
#define __ELF_ACCESS
#include <stdint.h>

typedef struct {
	uint8_t *data;
	uint32_t len;
	uint32_t addr;
	char* name;
} elf_section_t;

/* Load */
void elf_access_load_sections( char* fname,
			       elf_section_t **text,
			       elf_section_t **vectors );

#endif	/* __ELF_ACCESS */
