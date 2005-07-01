#ifndef HAD_PARSE_H
#define HAD_PARSE_H

/*
  $NiH: parse.h,v 1.3 2005/06/12 19:30:12 dillo Exp $

  parse.h -- parser interface
  Copyright (C) 1999, 2001, 2002, 2003, 2004 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include <stdio.h>

#include "dbl.h"

/* parser functions */

int parse(DB *, const char *, const char *, const char *);
int parse_xml(FILE *f);
int parse_cm(FILE *f);

/* callbacks */

int parse_disk_end(void);
int parse_disk_md5(const char *);
int parse_disk_name(const char *);
int parse_disk_sha1(const char *);
int parse_disk_start(void);
int parse_game_cloneof(const char *);
int parse_game_description(const char *);
int parse_game_end(void);
int parse_game_name(const char *);
int parse_game_sampleof(const char *);
int parse_game_start(void);
int parse_prog_name(const char *);
int parse_prog_version(const char *);
int parse_rom_crc(const char *);
int parse_rom_end(void);
int parse_rom_flags(const char *);
int parse_rom_md5(const char *);
int parse_rom_merge(const char *);
int parse_rom_name(const char *);
int parse_rom_sha1(const char *);
int parse_rom_size(const char *);
int parse_rom_start(void);
int parse_sample_end(void);
int parse_sample_name(const char *);
int parse_sample_start(void);

extern char *parse_errstr;

#endif /* parse.h */
