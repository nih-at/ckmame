/*
  fbh_make_key.c -- make dbkey for file_location struct
  Copyright (C) 2005-2014 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
  3. The name of the author may not be used to endorse or promote
     products derived from this software without specific prior
     written permission.

  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdlib.h>
#include <string.h>

#include "file_location.h"
#include "xmalloc.h"

static int filetype_char(filetype_t);


int
file_location_default_hashtype(filetype_t ft) {
    if (ft == TYPE_DISK)
	return Hashes::TYPE_MD5;
    else
	return Hashes::TYPE_CRC;
}


const char *
file_location_make_key(filetype_t filetype, const Hashes *hash) {
    static char key[Hashes::MAX_SIZE * 2 + 4];

    key[0] = '/';
    key[1] = filetype_char(filetype);
    key[2] = '/';
    
    auto str = hash->to_string(file_location_default_hashtype(filetype));
    strcpy(key + 3, str.c_str());

    return key;
}


static int
filetype_char(enum filetype filetype) {
    /* TODO: I hate these fucking switch statements! */

    switch (filetype) {
    case TYPE_ROM:
	return 'r';

    case TYPE_DISK:
	return 'd';

    default:
	return '?';
    }
}
