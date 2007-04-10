/*
  $NiH: parse-xml.c,v 1.8 2006/10/04 17:36:44 dillo Exp $

  parse-xml.c -- parse listxml format files
  Copyright (C) 1999-2007 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

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
#include <string.h>

#include "error.h"
#include "parse.h"
#include "xmlutil.h"



#define XA(f)	((xmlu_attr_cb)f)
#define XC(f)	((xmlu_tag_cb)f)
#define XO(f)	((xmlu_tag_cb)f)
#define XT(f)	((xmlu_text_cb)f)

static const xmlu_attr_t attr_disk[] = {
    { "md5",      XA(parse_file_hash),    TYPE_DISK,   HASHES_TYPE_MD5  },
    { "merge",    XA(parse_file_merge),   TYPE_DISK,   0                },
    { "name",     XA(parse_file_name),    TYPE_DISK,   0                },
    { "sha1",     XA(parse_file_hash),    TYPE_DISK,   HASHES_TYPE_SHA1 },
    { "status",   XA(parse_file_status),  TYPE_DISK,   0                },
    { NULL }
};
static const xmlu_attr_t attr_game[] = {
    { "name",     XA(parse_game_name),    0,           0                },
    { "romof",    XA(parse_game_cloneof), TYPE_ROM,    0                },
    { "sampleof", XA(parse_game_cloneof), TYPE_SAMPLE, 0                },
    { NULL }
};
static const xmlu_attr_t attr_rom[] = {
    { "crc",      XA(parse_file_hash),    TYPE_ROM,    HASHES_TYPE_CRC  },
    { "md5",      XA(parse_file_hash),    TYPE_ROM,    HASHES_TYPE_MD5  },
    { "merge",    XA(parse_file_merge),   TYPE_ROM,    0                },
    { "name",     XA(parse_file_name),    TYPE_ROM,    0                },
    { "sha1",     XA(parse_file_hash),    TYPE_ROM,    HASHES_TYPE_SHA1 },
    { "size",     XA(parse_file_size),    TYPE_ROM,    0                },
    { "status",   XA(parse_file_status),  TYPE_ROM,    0                },
    { NULL }
};
static const xmlu_attr_t attr_sample[] = {
    { "name",     XA(parse_file_name),    TYPE_SAMPLE, 0                },
    { NULL }
};
static const xmlu_entity_t entities[] = {
    { "disk",   attr_disk,   1, XO(parse_file_start), XC(parse_file_end),
      NULL, TYPE_DISK },
    { "game",   attr_game,   0, XO(parse_game_start), XC(parse_game_end),
      XT(parse_game_description), 0 },
    { "machine", attr_game,  0, XO(parse_game_start), XC(parse_game_end),
      NULL, 0 },
    { "rom",    attr_rom,    1, XO(parse_file_start), XC(parse_file_end),
      NULL, TYPE_ROM },
    { "sample", attr_sample, 1, XO(parse_file_start), XC(parse_file_end),
      NULL, TYPE_SAMPLE }
};
static const int nentities = sizeof(entities)/sizeof(entities[0]);



int
parse_xml(FILE *fin, parser_context_t *ctx)
{
    return xmlu_parse(fin, ctx, entities, nentities);
}
