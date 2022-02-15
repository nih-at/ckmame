#ifndef HAD_PARSER_XML_H
#define HAD_PARSER_XML_H

/*
  ParserXml.h -- parser for MAME XML dat files
  Copyright (C) 1999-2020 Dieter Baron and Thomas Klausner

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

#include <utility>

#include "Parser.h"
#include "XmlProcessor.h"

class ParserXml : public Parser {
public:
    ParserXml(ParserSourcePtr source, const std::unordered_set<std::string> &exclude, const DatEntry *dat, OutputContext *output, Options options) : Parser(std::move(source), exclude, dat, output, std::move(options)) { }
    ~ParserXml() override = default;
     
    bool parse() override;


  private:
    static void line_number_callback(void *context, size_t line_number);

    class Arguments {
      public:
	explicit Arguments(filetype_t file_type, int hash_type = 0) : file_type(file_type), hash_type(hash_type) { }

	filetype_t file_type;
	int hash_type;
    };

    static const Arguments arguments_rom;
    static const Arguments arguments_rom_crc;
    static const Arguments arguments_rom_md5;
    static const Arguments arguments_rom_sha1;
    static const Arguments arguments_disk;
    static const Arguments arguments_disk_md5;
    static const Arguments arguments_disk_sha1;

    static const std::unordered_map<std::string, XmlProcessor::Entity> entities;
    static const std::unordered_map<std::string, XmlProcessor::Attribute> attributes_clrmamepro;
    static const std::unordered_map<std::string, XmlProcessor::Attribute> attributes_disk;
    static const std::unordered_map<std::string, XmlProcessor::Attribute> attributes_game;
    static const std::unordered_map<std::string, XmlProcessor::Attribute> attributes_mame;
    static const std::unordered_map<std::string, XmlProcessor::Attribute> attributes_mess;
    static const std::unordered_map<std::string, XmlProcessor::Attribute> attributes_rom;
    static const std::unordered_map<std::string, XmlProcessor::Attribute> attributes_softwarelist;

    static XmlProcessor::CallbackStatus parse_file_end(void *ctx, const void *args);
    static XmlProcessor::CallbackStatus parse_file_hash(void *ctx, const void *args, const std::string &value);
    static XmlProcessor::CallbackStatus parse_file_loadflag(void *ctx, const void *args, const std::string &value);
    static XmlProcessor::CallbackStatus parse_file_merge(void *ctx, const void *args, const std::string &value);
    static XmlProcessor::CallbackStatus parse_file_name(void *ctx, const void *args, const std::string &value);
    static XmlProcessor::CallbackStatus parse_file_start(void *ctx, const void *args);
    static XmlProcessor::CallbackStatus parse_file_status(void *ctx, const void *args, const std::string &value);
    static XmlProcessor::CallbackStatus parse_file_size(void *ctx, const void *args, const std::string &value);
    static XmlProcessor::CallbackStatus parse_game_cloneof(void *ctx, [[maybe_unused]] const void *args, const std::string &value);
    static XmlProcessor::CallbackStatus parse_game_description(void *ctx, [[maybe_unused]] const void *args, const std::string &value);
    static XmlProcessor::CallbackStatus parse_game_end(void *ctx, [[maybe_unused]] const void *args);
    static XmlProcessor::CallbackStatus parse_game_name(void *ctx, [[maybe_unused]] [[maybe_unused]] const void *args, const std::string &value);
    static XmlProcessor::CallbackStatus parse_game_start(void *ctx, [[maybe_unused]] const void *args);
    static XmlProcessor::CallbackStatus parse_header_end(void *ctx, [[maybe_unused]] [[maybe_unused]] const void *args);
    static XmlProcessor::CallbackStatus parse_mame_build(void *context, const void *arguments, const std::string &value);
    static XmlProcessor::CallbackStatus parse_prog_description(void *ctx, [[maybe_unused]] [[maybe_unused]] const void *args, const std::string &value);
    static XmlProcessor::CallbackStatus parse_prog_header(void *ctx, [[maybe_unused]] [[maybe_unused]] const void *args, const std::string &value);
    static XmlProcessor::CallbackStatus parse_prog_name(void *ctx, [[maybe_unused]] [[maybe_unused]] const void *args, const std::string &value);
    static XmlProcessor::CallbackStatus parse_prog_version(void *ctx, [[maybe_unused]] [[maybe_unused]] const void *args, const std::string &value);
    static XmlProcessor::CallbackStatus parse_softwarelist(void *ctx, [[maybe_unused]] [[maybe_unused]] const void *args, const std::string &value);

    static XmlProcessor::CallbackStatus status(bool ok) { return ok ? XmlProcessor::OK : XmlProcessor::ERROR; }
};

#endif // HAD_PARSER_XML_H
