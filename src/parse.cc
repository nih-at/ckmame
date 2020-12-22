/*
  parse.c -- parser frontend
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

#include <algorithm>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "config.h"

#include "compat.h"
#include "dat.h"
#include "error.h"
#include "funcs.h"
#include "globals.h"
#include "parse.h"
#include "types.h"
#include "util.h"
#include "xmalloc.h"

#define CHECK_STATE(s)                                        \
    do {                                                           \
	if (state != (s)) {                                 \
	    myerror(ERRFILE, "%d: in wrong state", lineno); \
	    return false;                                             \
	}                                                          \
    } while (0)


bool ParserContext::parse(ParserSourcePtr source, const std::unordered_set<std::string> &exclude, const dat_entry_t *dat, output_context_t *out, int flags) {
    ParserContext ctx(source, exclude, dat, out, flags);

    bool ok;
    auto c = source->peek();

    switch (c) {
        case '<':
            ok = ctx.parse_xml();
            break;
        case '[':
            ok = ctx.parse_rc();
            break;
        default:
            ok = ctx.parse_cm();
    }

    if (ok) {
        if (!ctx.eof()) {
            ok = false;
	}
    }

    return ok;
}


bool ParserContext::eof() {
    if (state == PARSE_IN_HEADER) {
        return header_end();
    }

    return true;
}


bool ParserContext::file_continue(filetype_t ft) {
    CHECK_STATE(PARSE_IN_FILE);

    if (ft != TYPE_ROM) {
	myerror(ERRFILE, "%d: file continuation only supported for ROMs", lineno);
	return false;
    }

    if (flags & PARSE_FL_ROM_DELETED) {
        myerror(ERRFILE, "%d: internal error: trying to continue deleted file", lineno);
	return false;
    }

    flags |= PARSE_FL_ROM_CONTINUED;

    return true;
}


bool ParserContext::file_end(filetype_t ft) {
    CHECK_STATE(PARSE_IN_FILE);

    if (ft == TYPE_DISK) {
        disk_end();
    }
    else {
	rom_end(ft);
    }

    state = PARSE_IN_GAME;

    return 0;
}


bool ParserContext::file_status(filetype_t ft, const std::string &attr) {
    status_t status;

    CHECK_STATE(PARSE_IN_FILE);

    if (attr == "good") {
	status = STATUS_OK;
    }
    else if (attr == "verified") {
        status = STATUS_OK;
    }
    else if (attr == "baddump") {
        status = STATUS_BADDUMP;
    }
    else if (attr == "nodump") {
        status = STATUS_NODUMP;
    }
    else {
        myerror(ERRFILE, "%d: illegal status '%s'", lineno, attr.c_str());
	return false;
    }
 
    if (ft == TYPE_DISK) {
        d->status = status;
    }
    else {
        r->status = status;
    }

    return true;
}


bool ParserContext::file_hash(filetype_t ft, int ht, const std::string &attr) {
    Hashes *h;

    CHECK_STATE(PARSE_IN_FILE);

    if (attr == "-") {
	/* some dat files record crc - for 0-byte files, so skip it */
	return 0;
    }

    if (ft == TYPE_DISK) {
        h = &d->hashes;
    }
    else {
        h = &r->hashes;
    }

    if (h->set_from_string(attr) != ht) {
	myerror(ERRFILE, "%d: invalid argument for %s", lineno, Hashes::type_name(ht).c_str());
	return false;
    }

    return true;
}


bool ParserContext::file_ignore(filetype_t ft) {
    CHECK_STATE(PARSE_IN_FILE);

    if (ft != TYPE_ROM) {
        myerror(ERRFILE, "%d: file ignoring only supported for ROMs", lineno);
	return false;
    }

    flags |= PARSE_FL_ROM_IGNORE;

    return true;
}


bool ParserContext::file_merge(filetype_t ft, const std::string &attr) {
    CHECK_STATE(PARSE_IN_FILE);

    if (ft == TYPE_DISK) {
        d->merge = attr;
    }
    else {
        r->merge = attr;
    }

    return true;
}


bool ParserContext::file_mtime(filetype_t ft, time_t mtime) {
    CHECK_STATE(PARSE_IN_FILE);

    if (ft == TYPE_ROM) {
        r->mtime = mtime;
    }

    return true;
}


bool ParserContext::file_name(filetype_t ft, const std::string &attr) {
    CHECK_STATE(PARSE_IN_FILE);

    if (ft == TYPE_DISK) {
        d->name = attr;
    }
    else {
        auto name = std::string(attr);
        
        /* TODO: warn about broken dat file? */
        std::replace(name.begin(), name.end(), '\\', '/');
        
        r->name = name;
    }

    return true;
}


bool ParserContext::file_size(filetype_t ft, const std::string &attr) {
    /* TODO: check for strol errors */
    return file_size(ft, strtouq(attr.c_str(), NULL, 0));
}


bool ParserContext::file_size(filetype_t ft, uint64_t size) {
    CHECK_STATE(PARSE_IN_FILE);

    if (ft == TYPE_DISK) {
        myerror(ERRFILE, "%d: unknown attribute `size' for disk", lineno);
        return false;
    }

    r->size = size;
    return true;
}


bool ParserContext::file_start(filetype_t ft) {
    CHECK_STATE(PARSE_IN_GAME);

    if (ft == TYPE_DISK) {
        g->disks.push_back(Disk());
        d = &g->disks[g->disks.size() - 1];
    }
    else {
        g->roms.push_back(File());
        r = &g->roms[g->roms.size() - 1];
    }

    state = PARSE_IN_FILE;

    return true;
}


bool ParserContext::game_cloneof(filetype_t ft, const std::string &attr) {
    CHECK_STATE(PARSE_IN_GAME);

    g->cloneof[0] = attr;

    return true;
}


bool ParserContext::game_description(const std::string &attr) {
    CHECK_STATE(PARSE_IN_GAME);

    g->description = attr;

    return true;
}

bool ParserContext::game_end() {
    CHECK_STATE(PARSE_IN_GAME);
    
    auto ok = true;
    
    if (ignore_game(g->name)) {
	/* omit description if same as name (to save space) */
        if (g->name == g->description) {
            g->description = "";
	}

        if (!g->cloneof[0].empty()) {
	    if (g->cloneof[0] == g->name) {
                g->cloneof[0] = "";
	    }
	}

        ok = output_game(output, g) >= 0;
    }

    g = NULL;

    state = PARSE_OUTSIDE;

    return ok;
}


bool ParserContext::game_name(const std::string &attr) {
    CHECK_STATE(PARSE_IN_GAME);

    g->name = attr;

    if (!full_archive_name) {
	/* slashes are directory separators on some systems, and at least
	 * one redump dat contained a slash in a rom name */
        std::replace(g->name.begin(), g->name.end(), '/', '-');
    }

    return true;
}


bool ParserContext::game_start() {
    if (state == PARSE_IN_HEADER) {
        if (!header_end()) {
            return false;
	}
    }

    CHECK_STATE(PARSE_OUTSIDE);

    if (g) {
	myerror(ERRFILE, "%d: game inside game", lineno);
	return false;
    }

    g = std::make_shared<Game>();

    state = PARSE_IN_GAME;

    return true;
}


bool ParserContext::prog_description(const std::string &attr) {
    CHECK_STATE(PARSE_IN_HEADER);

    dat_entry_description(&de) = xstrdup(attr.c_str());

    return 0;
}


bool ParserContext::prog_header(const std::string attr) {
    CHECK_STATE(PARSE_IN_HEADER);

    if (detector) {
	myerror(ERRFILE, "%d: warning: detector already defined, header '%s' ignored", lineno, attr.c_str());
	return true;
    }

    auto ok = true;
    
    ParserSourcePtr dps = ps->open(attr);
    if (!dps) {
        myerror(ERRFILESTR, "%d: cannot open detector '%s'", lineno, attr.c_str());
	return false;
    }
#if defined(HAVE_LIBXML2)
    detector = Detector::parse(dps.get());
    if (!detector) {
        myerror(ERRFILESTR, "%d: cannot parse detector '%s'", lineno, attr.c_str());
        ok = false;
    }
    else
#endif
    {
        if (output_detector(output, detector.get()) < 0) {
            ok = false;
        }
    }

    return ok;
}


bool ParserContext::prog_name(const std::string &attr) {
    CHECK_STATE(PARSE_IN_HEADER);

    dat_entry_name(&de) = xstrdup(attr.c_str());

    return true;
}


bool ParserContext::prog_version(const std::string &attr) {
    CHECK_STATE(PARSE_IN_HEADER);

    dat_entry_version(&de) = xstrdup(attr.c_str());

    return true;
}


ParserContext::~ParserContext() {
    dat_entry_finalize(&de);
}


ParserContext::ParserContext(ParserSourcePtr source, const std::unordered_set<std::string> &exclude, const dat_entry_t *dat, output_context_t *output_, int flags) : lineno(0), ignore(exclude), output(output_), ps(source), flags(0), state(PARSE_IN_HEADER), r(NULL), d(NULL) {
    dat_entry_merge(&dat_default, dat, NULL);
    full_archive_name = flags & PARSER_FL_FULL_ARCHIVE_NAME;
    dat_entry_init(&de);
}


bool ParserContext::header_end() {
    dat_entry_t de;

    CHECK_STATE(PARSE_IN_HEADER);

    dat_entry_merge(&de, &dat_default, &de);
    output_header(output, &de);
    dat_entry_finalize(&de);

    state = PARSE_OUTSIDE;

    return true;
}


void ParserContext::disk_end() {
    if (d->hashes.empty()) {
        d->status = STATUS_NODUMP;
    }

    if (d->name == d->merge) {
        d->merge = "";
    }
}


bool ParserContext::ignore_game(const std::string &name) {
    for (auto &pattern : ignore) {
        if (fnmatch(pattern.c_str(), name.c_str(), 0) == 0) {
	    return true;
        }
    }

    return false;
}


void ParserContext::rom_end(filetype_t) {
    size_t n = g->roms.size() - 1;

    if (r->size == 0) {
        unsigned char zeroes[Hashes::MAX_SIZE];

        memset(zeroes, 0, sizeof(zeroes));
        
        /* some dats don't record crc for 0-byte files, so set it here */
        if (!r->hashes.has_type(Hashes::TYPE_CRC)) {
            r->hashes.set(Hashes::TYPE_CRC, zeroes);
        }
        
        /* some dats record all-zeroes md5 and sha1 for 0 byte files, fix */
        if (r->hashes.has_type(Hashes::TYPE_MD5) && r->hashes.verify(Hashes::TYPE_MD5, zeroes)) {
            r->hashes.set(Hashes::TYPE_MD5, (const unsigned char *)"\xd4\x1d\x8c\xd9\x8f\x00\xb2\x04\xe9\x80\x09\x98\xec\xf8\x42\x7e");
        }
        if (r->hashes.has_type(Hashes::TYPE_SHA1) && r->hashes.verify(Hashes::TYPE_SHA1, zeroes)) {
            r->hashes.set(Hashes::TYPE_SHA1, (const unsigned char *)"\xda\x39\xa3\xee\x5e\x6b\x4b\x0d\x32\x55\xbf\xef\x95\x60\x18\x90\xaf\xd8\x07\x09");
        }
    }

    /* omit duplicates */
    auto deleted = false;

    if (flags & PARSE_FL_ROM_IGNORE) {
        deleted = true;
    }
    else if (flags & PARSE_FL_ROM_CONTINUED) {
        auto &rom2 = g->roms[n - 1];
        rom2.size += r->size;
	deleted = true;
    }
    else if (r->name.empty()) {
	myerror(ERRFILE, "%d: ROM without name", lineno);
	deleted = true;
    }
    for (size_t j = 0; j < n && !deleted; j++) {
        auto &rom2 = g->roms[j];
        if (r->compare_size_crc(rom2)) {
	    /* TODO: merge in additional hash types? */
            if (r->compare_name(rom2)) {
		myerror(ERRFILE, "%d: the same rom listed multiple times (%s)", lineno, r->name.c_str());
		deleted = true;
		break;
	    }
            else if (!rom2.merge.empty() && r->merge == rom2.merge) {
		/* file_add_altname(r2, file_name(r)); */
		myerror(ERRFILE, "%d: the same rom listed multiple times (%s, merge-name %s)", lineno, r->name.c_str(), r->merge.c_str());
		deleted = true;
		break;
	    }
	}
        else if (r->compare_name(rom2)) {
	    myerror(ERRFILE, "%d: two different roms with same name (%s)", lineno, r->name.c_str());
	    deleted = true;
	    break;
	}
    }
    if (!r->merge.empty() && g->cloneof[0].empty()) {
        myerror(ERRFILE, "%d: rom '%s' has merge information but game '%s' has no parent", lineno, r->name.c_str(), g->name.c_str());
    }
    if (deleted) {
	flags = (flags & PARSE_FL_ROM_CONTINUED) ? 0 : PARSE_FL_ROM_DELETED;
        g->roms.pop_back();
    }
    else {
	flags = 0;
        if (!r->merge.empty() && r->merge == r->name) {
            r->merge = "";
	}
    }
}
