/*
  parse-xml.c -- parse listxml format files
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "parse.h"
#include "xmlutil.h"


static void parse_xml_lineno_cb(void *ctx, int lineno) {
    static_cast<ParserContext *>(ctx)->lineno = static_cast<size_t>(lineno);
}


static bool parse_xml_file_end(void *ctx, int file_type) {
    return static_cast<ParserContext *>(ctx)->file_end(static_cast<filetype_t>(file_type));
}

static bool parse_xml_file_hash(void *ctx, int file_type, int hash_type, const std::string &value) {
    return static_cast<ParserContext *>(ctx)->file_hash(static_cast<filetype_t>(file_type), hash_type, value);
}

static bool parse_xml_file_loadflag(void *ctx, int file_type, int hash_type, const std::string &value) {
    if (value == "continue" || value == "ignore") {
        return static_cast<ParserContext *>(ctx)->file_continue(static_cast<filetype_t>(file_type));
    }
    else if (value == "reload" || value == "reload_plain" || value == "fill") {
        return static_cast<ParserContext *>(ctx)->file_ignore(static_cast<filetype_t>(file_type));
    }
    return true;
}

static bool parse_xml_file_merge(void *ctx, int file_type, int hash_type, const std::string &value) {
    return static_cast<ParserContext *>(ctx)->file_merge(static_cast<filetype_t>(file_type), value);
}

static bool parse_xml_file_name(void *ctx, int file_type, int hash_type, const std::string &value) {
    return static_cast<ParserContext *>(ctx)->file_name(static_cast<filetype_t>(file_type), value);
}

static bool parse_xml_file_start(void *ctx, int file_type) {
    return static_cast<ParserContext *>(ctx)->file_start(static_cast<filetype_t>(file_type));
}

static bool parse_xml_file_status(void *ctx, int file_type, int hash_type, const std::string &value) {
    return static_cast<ParserContext *>(ctx)->file_status(static_cast<filetype_t>(file_type), value);
}

static bool parse_xml_file_size(void *ctx, int file_type, int hash_type, const std::string &value) {
    return static_cast<ParserContext *>(ctx)->file_size(static_cast<filetype_t>(file_type), value);
}


static bool parse_xml_game_cloneof(void *ctx, int file_type, int hash_type, const std::string &value) {
    return static_cast<ParserContext *>(ctx)->game_cloneof(static_cast<filetype_t>(file_type), value);
}

static bool parse_xml_game_description(void *ctx, const std::string &value) {
    return static_cast<ParserContext *>(ctx)->game_description(value);
}

static bool parse_xml_game_end(void *ctx, int file_type) {
    return static_cast<ParserContext *>(ctx)->game_end();
}

static bool parse_xml_game_name(void *ctx, int file_type, int hash_type, const std::string &value) {
    return static_cast<ParserContext *>(ctx)->game_name(value);
}

static bool parse_xml_game_start(void *ctx, int file_type) {
    return static_cast<ParserContext *>(ctx)->game_start();
}


static bool parse_xml_mame_build(void *ctx, int ft, int ht, const std::string &value) {
    auto parser_context = static_cast<ParserContext *>(ctx);

    if (!parser_context->prog_name(ft == 0 ? "M.A.M.E." : "M.E.S.S.")) {
        return false;
    }

    return parser_context->prog_version(value.substr(0, value.find(' ')));
}


static bool parse_xml_prog_description(void *ctx, const std::string &value) {
    return static_cast<ParserContext *>(ctx)->prog_description(value);
}

static bool parse_xml_prog_header(void *ctx, int ft, int ht, const std::string &value) {
    return static_cast<ParserContext *>(ctx)->prog_header(value);
}

static bool parse_xml_prog_name(void *ctx, const std::string &value) {
    return static_cast<ParserContext *>(ctx)->prog_name(value);
}

static bool parse_xml_prog_version(void *ctx, const std::string &value) {
    return static_cast<ParserContext *>(ctx)->prog_version(value);
}


static bool parse_xml_softwarelist(void *ctx, int ft, int ht, const std::string &value) {
    /* sadly, no version information */
    return static_cast<ParserContext *>(ctx)->prog_name(value);
}


static const std::unordered_map<std::string, XmluAttr> attr_mame = {
    { "build", { parse_xml_mame_build, 0, 0} }
};

static const std::unordered_map<std::string, XmluAttr> attr_mess = {
    { "build", { parse_xml_mame_build, 1, 0 } }
};

static const std::unordered_map<std::string, XmluAttr> attr_clrmamepro = {
    { "header", { parse_xml_prog_header , 0, 0 } }
};

static const std::unordered_map<std::string, XmluAttr> attr_disk = {
    { "md5", { parse_xml_file_hash, TYPE_DISK, Hashes::TYPE_MD5 } },
    { "merge", { parse_xml_file_merge, TYPE_DISK, 0 } },
    { "name", { parse_xml_file_name, TYPE_DISK, 0 } },
    { "sha1", { parse_xml_file_hash, TYPE_DISK, Hashes::TYPE_SHA1 } },
    { "status", { parse_xml_file_status, TYPE_DISK, 0 } }
};

static const std::unordered_map<std::string, XmluAttr> attr_game = {
    { "name", { parse_xml_game_name, 0, 0} },
    { "romof", { parse_xml_game_cloneof, TYPE_ROM, 0 } }
};

static const std::unordered_map<std::string, XmluAttr> attr_rom = {
    { "crc", { parse_xml_file_hash, TYPE_ROM, Hashes::TYPE_CRC } },
    { "loadflag", { parse_xml_file_loadflag, TYPE_ROM, 0} },
    { "md5", { parse_xml_file_hash, TYPE_ROM, Hashes::TYPE_MD5 } },
    { "merge", { parse_xml_file_merge, TYPE_ROM, 0 } },
    { "name", { parse_xml_file_name, TYPE_ROM, 0 } },
    { "sha1", { parse_xml_file_hash, TYPE_ROM, Hashes::TYPE_SHA1 } },
    { "size", { parse_xml_file_size, TYPE_ROM, 0} },
    { "status", { parse_xml_file_status, TYPE_ROM, 0} }
};

static const std::unordered_map<std::string, XmluAttr> attr_softwarelist = {
    { "description", { parse_xml_softwarelist , 0, 0} }
};

static const std::unordered_map<std::string, XmluEntity> entities = {
    { "clrmamepro", XmluEntity(attr_clrmamepro) },
    { "disk", XmluEntity(attr_disk, parse_xml_file_start, parse_xml_file_end, TYPE_DISK) },
    { "game", XmluEntity(attr_game, parse_xml_game_start, parse_xml_game_end) },
    { "game/description", XmluEntity(parse_xml_game_description) },
    { "header/description", XmluEntity(parse_xml_prog_description) },
    { "header/name", XmluEntity(parse_xml_prog_name) },
    { "header/version", XmluEntity(parse_xml_prog_version) },
    { "machine", XmluEntity(attr_game, parse_xml_game_start, parse_xml_game_end) },
    { "machine/description", XmluEntity(parse_xml_game_description) },
    { "mame", XmluEntity(attr_mame) },
    { "mess", XmluEntity(attr_mess) },
    { "rom", XmluEntity(attr_rom, parse_xml_file_start, parse_xml_file_end, TYPE_ROM) },
    { "software", XmluEntity(attr_game, parse_xml_game_start, parse_xml_game_end) },
    { "software/description", XmluEntity(parse_xml_game_description) },
    { "softwarelist", XmluEntity(attr_softwarelist) }
};


bool ParserContext::parse_xml() {
    return xmlu_parse(ps.get(), this, parse_xml_lineno_cb, entities);
}




