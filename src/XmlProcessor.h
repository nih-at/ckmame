#ifndef HAD_XMLUTIL_H
#define HAD_XMLUTIL_H

/*
  XmlProcessor.h -- parse XML file via callbacks
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

#include <string>
#include <unordered_map>
#include <utility>

#include "ParserSource.h"

class XmlProcessor {
  public:
    enum CallbackStatus {
	OK,
	ERROR,
	END
    };

    typedef void (*LineNumberCallback)(void *context, size_t line_number);
    typedef CallbackStatus (*AttributeCallback)(void *context, const void *arguments, const std::string &value);
    typedef CallbackStatus (*TagCallback)(void *context, const void *arguments);
    typedef CallbackStatus (*TextCallback)(void *context, const void *arguments, const std::string &value);

    class Attribute {
      public:
	Attribute(AttributeCallback callback, const void *arguments);

	AttributeCallback cb_attr;
	const void *arguments;
    };

    class Entity {
      public:
	explicit Entity(std::unordered_map<std::string, Attribute> attributes_, TagCallback cb_open_ = nullptr, TagCallback cb_close_ = nullptr, const void *arguments_ = nullptr) : attr(std::move(attributes_)), cb_open(cb_open_), cb_close(cb_close_), cb_text(nullptr), arguments(arguments_) { }
	explicit Entity(TextCallback cb_text_ = nullptr, const void *arguments_ = nullptr) : cb_open(nullptr), cb_close(nullptr), cb_text(cb_text_), arguments(arguments_) { }
	std::unordered_map<std::string, Attribute> attr;
	TagCallback cb_open;
	TagCallback cb_close;
	TextCallback cb_text;
	const void *arguments;
    };

    XmlProcessor(LineNumberCallback line_number_callback, const std::unordered_map<std::string, Entity> &entities, void *context);

    int parse(ParserSource *parser_source); // TODO: return bool?

  private:
    LineNumberCallback line_number_callback;
    const std::unordered_map<std::string, Entity> &entities;
    void *context;

    bool ok;
    bool stop_parsing;

    [[nodiscard]] const Entity *find(const std::string &path) const;
    void handle_callback_status(CallbackStatus status);
};


#endif // HAD_XMLUTIL_H
