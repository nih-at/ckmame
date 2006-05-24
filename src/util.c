/*
  $NiH: util.c,v 1.6 2006/05/06 23:01:53 dillo Exp $

  util.c -- utility functions
  Copyright (C) 1999-2006 Dieter Baron and Thomas Klausner

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



#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "xmalloc.h"
#include "util.h"
#include "error.h"



const char *
mybasename(const char *fname)
{
    const char *p;

    if ((p=strrchr(fname, '/')) == NULL)
	return fname;
    return p+1;
}



char *
bin2hex(char *b, const unsigned char *s, unsigned int len)
{
    unsigned int i;

    for (i=0; i<len; i++)
	sprintf(b+2*i, "%02x", (unsigned char)s[i]);
    b[2*i] = '\0';

    return b;
}



#define HEX2BIN(c)	(((c)>='0' && (c)<='9') ? (c)-'0'	\
			 : ((c)>='A' && (c)<='F') ? (c)-'A'+10	\
			 : (c)-'a'+10)

int
hex2bin(unsigned char *t, const char *s, int unsigned tlen)
{
    unsigned int i;
    
    if (strspn(s, "0123456789AaBbCcDdEeFf") != tlen*2
	|| s[tlen*2] != '\0')
	return -1;

    for (i=0; i<tlen; i++)
	t[i] = HEX2BIN(s[i*2])<<4 | HEX2BIN(s[i*2+1]);
    
    return 0;
}



name_type_t
name_type(const char *name)
{
    int l;

    l = strlen(name);

    if (strchr(name, '.') == NULL)
	return NAME_NOEXT;

    if (l > 4) {
	if (strcmp(name+l-4, ".chd") == 0)
	    return NAME_CHD;
	if (strcmp(name+l-4, ".zip") == 0)
	    return NAME_ZIP;
    }

    return NAME_UNKNOWN;
}
