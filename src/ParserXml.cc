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

#include "ParserXml.h"

#include "XmlProcessor.h"

const ParserXml::Arguments ParserXml::arguments_rom(TYPE_ROM);
const ParserXml::Arguments ParserXml::arguments_rom_crc(TYPE_ROM, Hashes::TYPE_CRC);
const ParserXml::Arguments ParserXml::arguments_rom_md5(TYPE_ROM, Hashes::TYPE_MD5);
const ParserXml::Arguments ParserXml::arguments_rom_sha1(TYPE_ROM, Hashes::TYPE_SHA1);
const ParserXml::Arguments ParserXml::arguments_disk(TYPE_DISK);
const ParserXml::Arguments ParserXml::arguments_disk_md5(TYPE_DISK, Hashes::TYPE_MD5);
const ParserXml::Arguments ParserXml::arguments_disk_sha1(TYPE_DISK, Hashes::TYPE_SHA1);

void ParserXml::line_number_callback(void *context, size_t line_number){
    static_cast<ParserXml *>(context)->lineno = line_number;
}


XmlProcessor::CallbackStatus ParserXml::parse_file_end(void *ctx, const void *args) {
    auto parser = static_cast<ParserXml *>(ctx);
    auto arguments = static_cast<const Arguments *>(args);

    return status(parser->file_end(arguments->file_type));
}


XmlProcessor::CallbackStatus ParserXml::parse_file_hash(void *ctx, const void *args, const std::string &value) {
    auto parser = static_cast<ParserXml *>(ctx);
    auto arguments = static_cast<const Arguments *>(args);

    return status(parser->file_hash(arguments->file_type, arguments->hash_type, value));
}


XmlProcessor::CallbackStatus ParserXml::parse_file_loadflag(void *ctx, const void *args, const std::string &value) {
    auto parser = static_cast<ParserXml *>(ctx);
    auto arguments = static_cast<const Arguments *>(args);

    if (value == "continue" || value == "ignore") {
        return status(parser->file_continue(arguments->file_type));
    }
    else if (value == "reload" || value == "reload_plain" || value == "fill") {
        return status(parser->file_ignore(arguments->file_type));
    }
    return status(true);
}


XmlProcessor::CallbackStatus ParserXml::parse_file_merge(void *ctx, const void *args, const std::string &value) {
    auto parser = static_cast<ParserXml *>(ctx);
    auto arguments = static_cast<const Arguments *>(args);

    return status(parser->file_merge(arguments->file_type, value));
}


XmlProcessor::CallbackStatus ParserXml::parse_file_name(void *ctx, const void *args, const std::string &value) {
    auto parser = static_cast<ParserXml *>(ctx);
    auto arguments = static_cast<const Arguments *>(args);

    return status(parser->file_name(arguments->file_type, value));
}


XmlProcessor::CallbackStatus ParserXml::parse_file_start(void *ctx, const void *args) {
    auto parser = static_cast<ParserXml *>(ctx);
    auto arguments = static_cast<const Arguments *>(args);

    return status(parser->file_start(arguments->file_type));
}


XmlProcessor::CallbackStatus ParserXml::parse_file_status(void *ctx, const void *args, const std::string &value) {
    auto parser = static_cast<ParserXml *>(ctx);
    auto arguments = static_cast<const Arguments *>(args);

    return status(parser->file_status(arguments->file_type, value));
}


XmlProcessor::CallbackStatus ParserXml::parse_file_size(void *ctx, const void *args, const std::string &value) {
    auto parser = static_cast<ParserXml *>(ctx);
    auto arguments = static_cast<const Arguments *>(args);

    if (value.empty()) {
        return XmlProcessor::OK;
    }
    else {
        return status(parser->file_size(arguments->file_type, value));
    }
}


XmlProcessor::CallbackStatus ParserXml::parse_game_cloneof(void *ctx, [[maybe_unused]] const void *args, const std::string &value) {
    auto parser = static_cast<ParserXml *>(ctx);

    return status(parser->game_cloneof(value));
}


XmlProcessor::CallbackStatus ParserXml::parse_game_description(void *ctx, [[maybe_unused]] const void *args, const std::string &value) {
    auto parser = static_cast<ParserXml *>(ctx);

    return status(parser->game_description(value));
}


XmlProcessor::CallbackStatus ParserXml::parse_game_end(void *ctx, [[maybe_unused]] const void *args) {
    auto parser = static_cast<ParserXml *>(ctx);

    return status(parser->game_end());
}


XmlProcessor::CallbackStatus ParserXml::parse_game_name(void *ctx, [[maybe_unused]] const void *arguments, const std::string &value) {
    auto parser = static_cast<ParserXml *>(ctx);

    return status(parser->game_name(value));
}

XmlProcessor::CallbackStatus ParserXml::parse_game_start(void *ctx, [[maybe_unused]] const void *arguments) {
    auto parser = static_cast<ParserXml *>(ctx);

    if (parser->header_only) {
	return XmlProcessor::END;
    }
    
    return status(parser->game_start());
}


XmlProcessor::CallbackStatus ParserXml::parse_header_end(void *ctx, [[maybe_unused]] const void *args) {
    auto parser = static_cast<ParserXml *>(ctx);

    auto ok = parser->header_end();
    if (ok && parser->header_only) {
	return XmlProcessor::END;
    }
    return status(ok);
}


XmlProcessor::CallbackStatus ParserXml::parse_mame_build(void *ctx, const void *args, const std::string &value) {
    auto parser = static_cast<ParserXml *>(ctx);
    auto arguments = static_cast<const Arguments *>(args);

    if (!parser->prog_name(arguments->file_type == TYPE_ROM ? "M.A.M.E." : "M.E.S.S.")) {
        return status(false);
    }

    return status(parser->prog_version(value.substr(0, value.find(' '))));
}


XmlProcessor::CallbackStatus ParserXml::parse_prog_description(void *ctx, [[maybe_unused]] const void *args, const std::string &value) {
    auto parser = static_cast<ParserXml *>(ctx);

    return status(parser->prog_description(value));
}

XmlProcessor::CallbackStatus ParserXml::parse_prog_header(void *ctx, [[maybe_unused]] const void *args, const std::string &value) {
    auto parser = static_cast<ParserXml *>(ctx);

    return status(parser->prog_header(value));
}

XmlProcessor::CallbackStatus ParserXml::parse_prog_name(void *ctx, [[maybe_unused]] const void *args, const std::string &value) {
    auto parser = static_cast<ParserXml *>(ctx);

    return status(parser->prog_name(value));
}

XmlProcessor::CallbackStatus ParserXml::parse_prog_version(void *ctx, [[maybe_unused]] const void *args, const std::string &value) {
    auto parser = static_cast<ParserXml *>(ctx);

    return status(parser->prog_version(value));
}


XmlProcessor::CallbackStatus ParserXml::parse_softwarelist_name(void *ctx, [[maybe_unused]] const void *args, const std::string &value) {
    auto parser = static_cast<ParserXml *>(ctx);

    auto ok = parser->prog_name(value);

    /* sadly, no version information, so use mtime of dat file */
    auto mtime = parser->ps->get_mtime();
    if (mtime != 0) {
        ok = parser->prog_version(std::to_string(mtime));
    }

    return status(ok);
}


const std::unordered_map<std::string, XmlProcessor::Attribute> ParserXml::attributes_clrmamepro = {
    { "header", XmlProcessor::Attribute(parse_prog_header, nullptr) }
};

const std::unordered_map<std::string, XmlProcessor::Attribute> ParserXml::attributes_mame = {
    { "build", XmlProcessor::Attribute(parse_mame_build, &arguments_rom) }
};

const std::unordered_map<std::string, XmlProcessor::Attribute> ParserXml::attributes_mess = {
    { "build", XmlProcessor::Attribute(parse_mame_build, &arguments_disk) }
};

const std::unordered_map<std::string, XmlProcessor::Attribute> ParserXml::attributes_disk = {
    { "md5", XmlProcessor::Attribute(parse_file_hash, &arguments_disk_md5) },
    { "merge", XmlProcessor::Attribute(parse_file_merge, &arguments_disk) },
    { "name", XmlProcessor::Attribute(parse_file_name, &arguments_disk) },
    { "sha1", XmlProcessor::Attribute(parse_file_hash, &arguments_disk_sha1)},
    { "status", XmlProcessor::Attribute(parse_file_status, &arguments_disk) }
};

const std::unordered_map<std::string, XmlProcessor::Attribute> ParserXml::attributes_game = {
    { "name", XmlProcessor::Attribute(parse_game_name, nullptr) },
    { "romof", XmlProcessor::Attribute(parse_game_cloneof, nullptr) }
};

const std::unordered_map<std::string, XmlProcessor::Attribute> ParserXml::attributes_rom = {
    { "crc", XmlProcessor::Attribute(parse_file_hash, &arguments_rom_crc) },
    { "loadflag", XmlProcessor::Attribute(parse_file_loadflag, &arguments_rom) },
    { "md5", XmlProcessor::Attribute(parse_file_hash, &arguments_rom_md5) },
    { "merge", XmlProcessor::Attribute(parse_file_merge, &arguments_rom) },
    { "name", XmlProcessor::Attribute(parse_file_name, &arguments_rom) },
    { "sha1", XmlProcessor::Attribute(parse_file_hash, &arguments_rom_sha1) },
    { "size", XmlProcessor::Attribute(parse_file_size, &arguments_rom) },
    { "status", XmlProcessor::Attribute(parse_file_status, &arguments_rom) }
};

const std::unordered_map<std::string, XmlProcessor::Attribute> ParserXml::attributes_softwarelist = {
    { "description", XmlProcessor::Attribute(parse_prog_description, nullptr) },
    { "name", XmlProcessor::Attribute(parse_softwarelist_name, nullptr) }
};

const std::unordered_map<std::string, XmlProcessor::Entity> ParserXml::entities = {
    { "clrmamepro", XmlProcessor::Entity(attributes_clrmamepro) },
    { "disk", XmlProcessor::Entity(attributes_disk, parse_file_start, parse_file_end, &arguments_disk) },
    { "game", XmlProcessor::Entity(attributes_game, parse_game_start, parse_game_end) },
    { "game/description", XmlProcessor::Entity(parse_game_description) },
    { "header", XmlProcessor::Entity({}, nullptr, parse_header_end) },
    { "header/description", XmlProcessor::Entity(parse_prog_description) },
    { "header/name", XmlProcessor::Entity(parse_prog_name) },
    { "header/version", XmlProcessor::Entity(parse_prog_version) },
    { "machine", XmlProcessor::Entity(attributes_game, parse_game_start, parse_game_end) },
    { "machine/description", XmlProcessor::Entity(parse_game_description) },
    { "mame", XmlProcessor::Entity(attributes_mame) },
    { "mess", XmlProcessor::Entity(attributes_mess) },
    { "rom", XmlProcessor::Entity(attributes_rom, parse_file_start, parse_file_end, &arguments_rom) },
    { "software", XmlProcessor::Entity(attributes_game, parse_game_start, parse_game_end) },
    { "software/description", XmlProcessor::Entity(parse_game_description) },
    { "/softwarelist", XmlProcessor::Entity(attributes_softwarelist) }
};


bool ParserXml::parse() {
    auto processor = XmlProcessor(line_number_callback, entities, this);

    return processor.parse(ps.get());
}
