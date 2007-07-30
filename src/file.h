#ifndef HAD_FILE_H
#define HAD_FILE_H

/*
  file.h -- information about one file
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

struct file {
    char *name;
    char *merge;
    hashes_t hashes;
    unsigned long size;
    status_t status;
    where_t where;
    parray_t *altnames;
};

typedef struct file file_t;



#define file_altname(r, i)	((char *)parray_get(file_altnames(r), (i)))
#define file_altnames(r)		((r)->altnames)
#define file_hashes(r)		(&(r)->hashes)
#define file_merge(r)		((r)->merge)
#define file_name(r)		((r)->name)
#define file_num_altnames(r)	(file_altnames(r) ? \
				 parray_length(file_altnames(r)) : 0)
#define file_size(r)		((r)->size)
#define file_status(r)		((r)->status)
#define file_where(r)		((r)->where)

#define file_compare_m(r1, r2)	\
	(strcmp(file_merge(r1) ? file_merge(r1) : file_name(r1), file_name(r2)))
#define file_compare_n(r1, r2)	(strcmp(file_name(r1), file_name(r2)))
#define file_compare_msc(r1, r2)						\
	(file_compare_m((r1), (r2)) || file_compare_sc((r1), (r2)))
#define file_compare_nsc(r1, r2)						\
	(file_compare_n((r1), (r2)) || file_compare_sc((r1), (r2)))
#define file_compare_sc(rg, ra)						  \
	(!(!SIZE_IS_KNOWN(file_size(rg))					  \
	   || (file_size(rg) == file_size(ra)				  \
	       && (file_status(rg) == STATUS_NODUMP			  \
		   || ((hashes_types(file_hashes(rg))			  \
			& hashes_types(file_hashes(rg)) & HASHES_TYPE_CRC) \
		       && (hashes_crc(file_hashes(rg))			  \
			   == hashes_crc(file_hashes(ra))))))))

void file_add_altname(file_t *, const char *);
void file_init(file_t *);
void file_finalize(file_t *);

#endif /* file.h */
