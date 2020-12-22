/*
  output_mtree.c -- write games to mtree(8) files
  Copyright (C) 2013-2014 Dieter Baron and Thomas Klausner

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

#include "OutputContextMtree.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "output.h"
#include "util.h"
#include "xmalloc.h"


OutputContextMtree::OutputContextMtree(const std::string &fname_, int flags) : fname(fname_), extended(flags & OUTPUT_FL_EXTENDED) {
    if (fname.empty()) {
	f = stdout;
	fname = "*stdout*";
    }
    else {
        if ((f = fopen(fname.c_str(), "w")) == NULL) {
	    myerror(ERRDEF, "cannot create '%s': %s", fname.c_str(), strerror(errno));
            throw std::exception();
	}
    }
}

OutputContextMtree::~OutputContextMtree() {
    close();
}


bool OutputContextMtree::close() {
    auto ok = true;
    
    if (f != NULL && f != stdout) {
        ok = fclose(f) > 0;
        f = NULL;
    }

    return ok;
}


static std::string
strsvis_cstyle(const std::string &in) {
    /* maximal extension = 2/char */
    auto out = std::string(2 * in.length(), 0);

    size_t outpos = 0;
    for (size_t inpos = 0; inpos < in.length(); inpos++) {
	switch (in[inpos]) {
	case '\007':
	    out[outpos++] = '\\';
	    out[outpos++] = 'a';
	    break;
	case '\010':
	    out[outpos++] = '\\';
	    out[outpos++] = 'b';
	    break;
	case '\014':
	    out[outpos++] = '\\';
	    out[outpos++] = 'f';
	    break;
	case '\012':
	    out[outpos++] = '\\';
	    out[outpos++] = 'n';
	    break;
	case '\015':
	    out[outpos++] = '\\';
	    out[outpos++] = 'r';
	    break;
	case '\040':
	    out[outpos++] = '\\';
	    out[outpos++] = 's';
	    break;
	case '\011':
	    out[outpos++] = '\\';
	    out[outpos++] = 't';
	    break;
	case '\013':
	    out[outpos++] = '\\';
	    out[outpos++] = 'v';
	    break;
	case '#':
	    out[outpos++] = '\\';
	    out[outpos++] = '#';
	    break;
	default:
	    out[outpos++] = in[inpos];
	    break;
	}
    }
    out.resize(outpos);

    return out;
}


bool OutputContextMtree::game(GamePtr game) {
    auto dirname = strsvis_cstyle(game->name);

    fprintf(f, "./%s type=dir\n", dirname.c_str());
    for (size_t i = 0; i < game->roms.size(); i++) {
        auto &rom = game->roms[i];

        fprintf(f, "./%s/%s type=file size=%" PRIu64, dirname.c_str(), strsvis_cstyle(rom.name).c_str(), rom.size);
        cond_print_hash(f, " sha1=", Hashes::TYPE_SHA1, &rom.hashes, "");
        cond_print_hash(f, " md5=", Hashes::TYPE_MD5, &rom.hashes, "");
        cond_print_string(f, " status=", status_name(rom.status), "");
        if (extended) {
            /* crc is not in the standard set supported on NetBSD */
            cond_print_hash(f, " crc=", Hashes::TYPE_CRC, &rom.hashes, "");
            fprintf(f, " time=%llu", static_cast<unsigned long long>(rom.mtime));
	}
	fputs("\n", f);
    }
    for (size_t i = 0; i < game->disks.size(); i++) {
        auto d = &game->disks[i];

	fprintf(f, "./%s/%s type=file" PRIu64, dirname.c_str(), strsvis_cstyle(d->name).c_str());
        cond_print_hash(f, " sha1=", Hashes::TYPE_SHA1, disk_hashes(d), "");
	cond_print_hash(f, " md5=", Hashes::TYPE_MD5, disk_hashes(d), "");
        cond_print_string(f, " status=", status_name(disk_status(d)), "");
	fputs("\n", f);
    }

    return true;
}


bool OutputContextMtree::header(DatEntry *dat) {
    fprintf(f, ". type=dir\n");

    return true;
}
