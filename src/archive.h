#ifndef HAD_ARCHIVE_H
#define HAD_ARCHIVE_H

/*
  archive.h -- information about an archive
  Copyright (C) 1999-2013 Dieter Baron and Thomas Klausner

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


#include <sys/stat.h>
#include <sys/types.h>

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
    struct archive_ops *ops;
    void *ud;
};

typedef struct archive archive_t;

struct archive_ops {
    int (*check)(archive_t *);
    int (*close)(archive_t *);
    int (*commit)(archive_t *);
    int (*file_add_empty)(archive_t *, const char *name);
    void (*file_close)(void *);
    int (*file_copy)(archive_t *, int, archive_t *, int, const char *, off_t, off_t);
    int (*file_delete)(archive_t *, int);
    void *(*file_open)(archive_t *, int);
    int64_t (*file_read)(void *, void *, uint64_t);
    int (*file_rename)(archive_t *, int, const char *);
    const char *(*file_strerror)(void *);
    int (*read_infos)(archive_t *);
    int (*rollback)(archive_t *);  /* never called if commit never fails */
};

#define ARCHIVE_FL_CREATE		0x00100
#define ARCHIVE_FL_CHECK_INTEGRITY	0x00200
#define ARCHIVE_FL_QUIET		0x00400
#define ARCHIVE_FL_NOCACHE		0x00800
#define ARCHIVE_FL_RDONLY		0x01000
#define ARCHIVE_FL_TORRENTZIP		0x02000
#define ARCHIVE_FL_DELAY_READINFO	0x04000

#define ARCHIVE_FL_HASHTYPES_MASK	0x000ff
#define ARCHIVE_FL_MASK			0x0ff00

#define ARCHIVE_IFL_MODIFIED		0x10000



#define archive_file(a, i)	((file_t *)array_get(archive_files(a), (i)))
#define archive_files(a)	((a)->files)
#define archive_filetype(a)	((a)->filetype)
#define archive_flags(a)	((a)->flags)
#define archive_id(a)		((a)->id)
#define archive_name(a)		((a)->name)
#define archive_num_files(a)	(array_length(archive_files(a)))
#define archive_user_data(a)    ((a)->ud)
#define archive_where(a)	((a)->where)


archive_t *archive_by_id(int);
int archive_check(archive_t *);
int archive_close(archive_t *);
int archive_commit(archive_t *);
int archive_dir_add_file(archive_t *, const char *, struct stat *, file_sh_t *);
int archive_file_add_empty(archive_t *, const char *);
int archive_file_compare_hashes(archive_t *, int, const hashes_t *);
int archive_file_compute_hashes(archive_t *, int, int);
int archive_file_copy(archive_t *, int, archive_t *, const char *);
int archive_file_copy_or_move(archive_t *, int, archive_t *, const char *, int);
int archive_file_copy_part(archive_t *, int, archive_t *, const char *, off_t, off_t, const file_t *);
int archive_file_delete(archive_t *, int);
int archive_file_move(archive_t *, int, archive_t *, const char *);
int archive_file_rename(archive_t *, int, const char *);
int archive_file_rename_to_unique(archive_t *, int);
off_t archive_file_find_offset(archive_t *, int, int, const hashes_t *);
int archive_file_index_by_name(const archive_t *, const char *);
int archive_free(archive_t *);
void archive_global_flags(int, bool);
bool archive_is_empty(const archive_t *);
archive_t *archive_new(const char *, filetype_t, where_t, int);
int archive_read_infos(archive_t *);
void archive_real_free(archive_t *);
int archive_refresh(archive_t *);
int archive_rollback(archive_t *);

/* internal */
extern int _archive_global_flags;

#define ARCHIVE_IS_INDEXED(a)			\
	(((a)->flags & ARCHIVE_FL_NOCACHE) == 0	\
	 && IS_EXTERNAL(archive_where(a)))

#define archive_is_writable(a)	(((a)->flags & ARCHIVE_FL_RDONLY) == 0)

int archive_init_dir(archive_t *);
int archive_init_zip(archive_t *);

#endif /* archive.h */
