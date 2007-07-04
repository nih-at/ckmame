#ifndef HAD_ROM_H
#define HAD_ROM_H

/*
  $NiH: rom.h,v 1.5 2006/09/29 16:01:34 dillo Exp $

  rom.h -- information about one rom
  Copyright (C) 1999-2006 Dieter Baron and Thomas Klausner

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



#include "hashes.h"
#include "types.h"
#include "parray.h"

struct rom {
    char *name;
    char *merge;
    hashes_t hashes;
    unsigned long size;
    status_t status;
    where_t where;
    parray_t *altnames;
};

typedef struct rom rom_t;



#define rom_altname(r, i)	((char *)parray_get(rom_altnames(r), (i)))
#define rom_altnames(r)		((r)->altnames)
#define rom_hashes(r)		(&(r)->hashes)
#define rom_merge(r)		((r)->merge)
#define rom_name(r)		((r)->name)
#define rom_num_altnames(r)	(rom_altnames(r) ? \
				 parray_length(rom_altnames(r)) : 0)
#define rom_size(r)		((r)->size)
#define rom_status(r)		((r)->status)
#define rom_where(r)		((r)->where)

#define rom_compare_m(r1, r2)	\
	(strcmp(rom_merge(r1) ? rom_merge(r1) : rom_name(r1), rom_name(r2)))
#define rom_compare_n(r1, r2)	(strcmp(rom_name(r1), rom_name(r2)))
#define rom_compare_msc(r1, r2)						\
	(rom_compare_m((r1), (r2)) || rom_compare_sc((r1), (r2)))
#define rom_compare_nsc(r1, r2)						\
	(rom_compare_n((r1), (r2)) || rom_compare_sc((r1), (r2)))
#define rom_compare_sc(rg, ra)						  \
	(!(!SIZE_IS_KNOWN(rom_size(rg))					  \
	   || (rom_size(rg) == rom_size(ra)				  \
	       && (rom_status(rg) == STATUS_NODUMP			  \
		   || ((hashes_types(rom_hashes(rg))			  \
			& hashes_types(rom_hashes(rg)) & HASHES_TYPE_CRC) \
		       && (hashes_crc(rom_hashes(rg))			  \
			   == hashes_crc(rom_hashes(ra))))))))

void rom_add_altname(rom_t *, const char *);
void rom_init(rom_t *);
void rom_finalize(rom_t *);

#endif /* rom.h */
