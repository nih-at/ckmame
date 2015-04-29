/*
  parser_source.c -- reading parser input data from various sources
  Copyright (C) 2008-2014 Dieter Baron and Thomas Klausner

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


#include <errno.h>
#include <stdlib.h>

#include "error.h"
#include "myinttypes.h"
#include "parser_source.h"
#include "util.h"
#include "xmalloc.h"

#define PSBLKSIZE	1024

struct parser_source {
    parser_source_t *(*open)(void *, const char *);
    int (*read)(void *, void *, int); 
    int (*close)(void *);
    void *ud;

    char *data;
    int size;
    char *cur;
    int len;
};

struct pszip_ud {
    char *fname;
    struct zip *za;
    struct zip_file *zf;
};

typedef struct pszip_ud pszip_ud_t;

struct psfile_ud {
    char *fname;
    FILE *f;
};

typedef struct psfile_ud psfile_ud_t;

static int psfile_close(psfile_ud_t *);
static parser_source_t *psfile_open(psfile_ud_t *, const char *);
static int psfile_read(psfile_ud_t *, void *, int);
static int pszip_close(pszip_ud_t *);
static parser_source_t *pszip_open(pszip_ud_t *, const char *);
static int pszip_read(pszip_ud_t *, void *, int);

static void _buffer_consume(parser_source_t *, int);
static void _buffer_fill(parser_source_t *, int);
static void _buffer_grow(parser_source_t *, int);

static parser_source_t *_ps_file_open(const char *, const char *);
static parser_source_t *_ps_new_zip(const char *, struct zip *, const char *,
				    bool);


int
ps_close(parser_source_t *ps)
{
    int ret;

    if (ps->close)
	ret = ps->close(ps->ud);
    else
	ret = 0;

    free(ps->data);
    free(ps);

    return ret;
}


char *
ps_getline(parser_source_t *ps)
{
    char *line, *p;
    int len;

    for (;;) {
	if (ps->len > 0 && (p=memchr(ps->cur, '\n', ps->len)) != NULL) {
	    line = ps->cur;
	    len = p - ps->cur;
	    if (len > 0 && line[len-1] == '\r')
		line[len-1] = '\0';
	    else
		line[len] = '\0';
	    _buffer_consume(ps, len+1);
	    break;
	}

	len = ps->len;
	_buffer_fill(ps, (ps->len/PSBLKSIZE+1)*PSBLKSIZE);
	
	if (len == ps->len) {
	    if (ps->len == 0)
		return NULL;
	    
	    line = ps->cur;
	    ps->cur[ps->len] = '\0';
	    _buffer_consume(ps, ps->len);
	    break;
	}
    }

    return line;
    
}


parser_source_t *
ps_new(void *ud, parser_source_close fn_close, parser_source_open fn_open,
			parser_source_read fn_read)
{
    parser_source_t *ps;

    ps = xmalloc(sizeof(*ps));

    ps->ud = ud;
    ps->close = fn_close;
    ps->open = fn_open;
    ps->read = fn_read;

    ps->data = NULL;
    ps->size = 0;
    ps->cur = NULL;
    ps->len = 0;

    return ps;
}


parser_source_t *
ps_new_file(const char *fname)
{
    psfile_ud_t *ud;
    FILE *f;

    if (fname) {
	if ((f=fopen(fname, "r")) == NULL)
	    return NULL;
    }
    else
	f = stdin;

    ud = xmalloc(sizeof(*ud));

    if (fname)
	ud->fname = xstrdup(fname);
    else
	ud->fname = NULL;
    ud->f = f;

    seterrinfo(fname, NULL);
    return ps_new(ud, (parser_source_close)psfile_close,
		  (parser_source_open)psfile_open,
		  (parser_source_read)psfile_read);
}


parser_source_t *
ps_new_stdin(void)
{
    return ps_new_file(NULL);
}


parser_source_t *
ps_new_zip(const char *zaname, struct zip *za, const char *fname)
{
    return _ps_new_zip(zaname, za, fname, false);
}


parser_source_t *
ps_open(parser_source_t *ps, const char *fname)
{
    return ps->open(ps->ud, fname);
}


int
ps_peek(parser_source_t *ps)
{
    _buffer_fill(ps, 1);

    if (ps->len == 0)
	return EOF;

    return ps->cur[0];
}


int
ps_read(parser_source_t *ps, void *buf, int n)
{
    int done, ret;
    
    if (ps->len > 0) {
	done = (ps->len<n ? ps->len : n);
	memcpy(buf, ps->cur, done);
	_buffer_consume(ps, n);
	buf += done;
	n -= done;
    }
    else
	done = 0;

    ret = ps->read(ps->ud, buf, n);

    if (ret >= 0)
	done += ret;

    return done;
}


static int
psfile_close(psfile_ud_t *ud)
{
    int ret;

    if (ud->fname)
	ret = fclose(ud->f);
    else
	ret = 0;

    free(ud->fname);
    free(ud);

    return ret;
}


static parser_source_t *
psfile_open(psfile_ud_t *ud, const char *fname)
{
    return _ps_file_open(fname, ud->fname);
}


static int
psfile_read(psfile_ud_t *ud, void *b, int n)
{
    return fread(b, 1, n, ud->f);
}


static int
pszip_close(pszip_ud_t *ud)
{
    int ret;

    ret = zip_fclose(ud->zf);

    free(ud->fname);
    free(ud);

    return ret;
}


static parser_source_t *
pszip_open(pszip_ud_t *ud, const char *fname)
{
    parser_source_t *ps;

    ps = _ps_new_zip(ud->fname, ud->za, fname, true);

    if (ps == NULL && errno == ENOENT)
	ps = _ps_file_open(fname, ud->fname);

    return ps;
}


static int
pszip_read(pszip_ud_t *ud, void *b, int n)
{
    return zip_fread(ud->zf, b, n);
}


static void
_buffer_consume(parser_source_t *ps, int n)
{
    if (n > ps->len)
	n = ps->len;

    ps->len -= n;
    ps->cur += n;

    if (ps->len == 0)
	ps->cur = ps->data;
}


static void
_buffer_fill(parser_source_t *ps, int n)
{
    int done;
    
    if (ps->len >= n)
	return;

    _buffer_grow(ps, n);

    done = ps->read(ps->ud, ps->cur+ps->len, n-ps->len);

    if (done > 0)
	ps->len += done;
}


static void
_buffer_grow(parser_source_t *ps, int n)
{
    int new_size;

    if (ps->len && ps->cur > ps->data) {
	memmove(ps->data, ps->cur, ps->len);
	ps->cur = ps->data;
    }

    new_size = n+ps->len+1;

    if (ps->size < new_size) {
	int off = ps->cur - ps->data;
	ps->data = xrealloc(ps->data, new_size);
	ps->size = new_size;
	ps->cur = ps->data + off;
    }
}


static parser_source_t *
_ps_file_open(const char *fname, const char *parent)
{
    parser_source_t *ps;
    char *full_name = NULL;

    if (parent != NULL) {
	char *dir;

	full_name = NULL;

	dir = mydirname(parent);
	full_name = xmalloc(strlen(dir)+strlen(fname)+2);
	sprintf(full_name, "%s/%s", dir, fname);
	free(dir);
	fname = full_name;
    }
    
    ps = ps_new_file(fname);
    free(full_name);
    return ps;
}


static parser_source_t *
_ps_new_zip(const char *zaname, struct zip *za, const char *fname, bool relaxed)
{
    struct zip_file *zf;
    pszip_ud_t *ud;
    int flags;

    flags = (relaxed ? ZIP_FL_NOCASE|ZIP_FL_NODIR : 0); 

    if ((zf=zip_fopen(za, fname, flags)) == NULL) {
	int zer, ser;

	zip_error_get(za, &zer, &ser);

	switch (zer) {
	case ZIP_ER_NOENT:
	    errno = ENOENT;
	    break;
	case ZIP_ER_MEMORY:
	    errno = ENOMEM;
	    break;
	default:
	    errno = EIO;
	    break;
	}

	return NULL;
    }

    ud = xmalloc(sizeof(*ud));
    ud->fname = xstrdup(zaname);
    ud->za = za;
    ud->zf = zf;

    return ps_new(ud, (parser_source_close)pszip_close,
		  (parser_source_open)pszip_open,
		  (parser_source_read)pszip_read);
}
