#ifndef _HAD_WARN_H
#define _HAD_WARN_H

/*
  warn.h -- emit warning
  Copyright (C) 1999-2014 Dieter Baron and Thomas Klausner

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


#include "disk.h"
#include "file.h"


#define WARN_UNKNOWN 0x0001
#define WARN_USED 0x0002
#define WARN_NOT_USED 0x0004
#define WARN_FILE_BROKEN 0x0008

#define WARN_SUPERFLUOUS (WARN_UNKNOWN | WARN_USED | WARN_NOT_USED)

#define WARN_WRONG_ZIP 0x0010
#define WARN_WRONG_NAME 0x0020
#define WARN_LONGOK 0x0040
#define WARN_ELSEWHERE 0x0080

#define WARN_FIXABLE (WARN_WRONG_ZIP | WARN_WRONG_NAME | WARN_LONGOK | WARN_ELSEWHERE)

#define WARN_WRONG_CRC 0x0100
#define WARN_LONG 0x0200
#define WARN_SHORT 0x0400
#define WARN_MISSING 0x0800
#define WARN_NO_GOOD_DUMP 0x1000

#define WARN_BROKEN (WARN_WRONG_CRC | WARN_LONG | WARN_SHORT | WARN_MISSING | WARN_NO_GOOD_DUMP)

#define WARN_ALL (WARN_SUPERFLUOUS | WARN_FIXABLE | WARN_BROKEN | WARN_FILE_BROKEN)

#define WARN_CORRECT 0x2000

/* keep in sync with tname in warn.c:warn_ensure_header() */
enum warn_type { WARN_TYPE_ARCHIVE, WARN_TYPE_GAME, WARN_TYPE_IMAGE };

typedef enum warn_type warn_type_t;


void warn_disk(const disk_t *, const char *, ...);
void warn_file(const file_t *, const char *, ...);
void warn_image(const char *, const char *, ...);
void warn_rom(const file_t *, const char *, ...);
void warn_set_info(warn_type_t, const char *);

#endif /* _HAD_WARN_H */
