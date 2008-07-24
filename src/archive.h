#ifndef HAD_ARCHIVE_H
#define HAD_ARCHIVE_H

/*
  archive.h -- information about an archive
  Copyright (C) 1999-2007 Dieter Baron and Thomas Klausner

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



#include <zip.h>

#include "array.h"
#include "file.h"

struct archive {
    int id;
    int refcount;
    char *name;
    filetype_t filetype;
    where_t where;
    int flags;
    array_t *files;
    struct zip *za;
};

typedef struct archive archive_t;

#define ARCHIVE_FL_CREATE		0x01
#define ARCHIVE_FL_CHECK_INTEGRITY	0x02
#define ARCHIVE_FL_QUIET		0x04
#define ARCHIVE_FL_NOCACHE		0x08
#define ARCHIVE_FL_RDONLY		0x10
#define ARCHIVE_FL_TORRENTZIP		0x20

#define ARCHIVE_FL_MASK			0xff

#define ARCHIVE_IFL_MODIFIED		0x100



#define archive_file(a, i)	((file_t *)array_get(archive_files(a), (i)))
#define archive_files(a)	((a)->files)
#define archive_filetype(a)	((a)->filetype)
#define archive_id(a)		((a)->id)
#define archive_name(a)		((a)->name)
#define archive_num_files(a)	(array_length(archive_files(a)))
#define archive_where(a)	((a)->where)

/* internal */
#define archive_zip(a)		((a)->za)


#define archive_file_copy(sa, sidx, da, dname)	\
    (archive_file_copy_part((sa), (sidx), (da), (dname), 0, -1))

archive_t *archive_by_id(int);
int archive_commit(archive_t *);
int archive_file_add_empty(archive_t *, const char *);
int archive_file_compare_hashes(archive_t *, int, const hashes_t *);
int archive_file_compute_hashes(archive_t *, int, int);
int archive_file_copy_part(archive_t *, int, archive_t *, const char *,
			   off_t, off_t);
int archive_file_copy_or_move(archive_t *, int, archive_t *, const char *, int);
int archive_file_delete(archive_t *, int);
int archive_file_move(archive_t *, int, archive_t *, const char *);
int archive_file_rename(archive_t *, int, const char *);
off_t archive_file_find_offset(archive_t *, int, int, const hashes_t *);
int archive_file_index_by_name(const archive_t *, const char *);
int archive_free(archive_t *);
void archive_global_flags(int, bool);
archive_t *archive_new(const char *, filetype_t, where_t, int);
void archive_real_free(archive_t *);
int archive_rollback(archive_t *);
bool archive_is_empty(const archive_t *);

/* internal */
extern int _archive_global_flags;

#define ARCHIVE_IS_INDEXED(a)			\
	(((a)->flags & ARCHIVE_FL_NOCACHE) == 0	\
	 && IS_EXTERNAL(archive_where(a)))

/* XXX: a is not evaluated */
#define archive_is_rdwr(a)	\
    ((_archive_global_flags & ARCHIVE_FL_RDONLY) == 0)

bool archive_is_torrentzipped(archive_t *);

int archive_close_zip(archive_t *);
int archive_ensure_zip(archive_t *);
int archive_refresh(archive_t *);

#endif /* archive.h */
