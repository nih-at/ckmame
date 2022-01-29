/*
  XmlProcessor.c -- parse XML file via callbacks
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


#include "config.h"

#include "error.h"
#include "XmlProcessor.h"

XmlProcessor::XmlProcessor(LineNumberCallback line_number_callback_, const std::unordered_map<std::string, Entity> &entities_, void *context_) :
    line_number_callback(line_number_callback_),
    entities(entities_),
    context(context_),
    ok(true),
    stop_parsing(false) { }


#ifndef HAVE_LIBXML2

int XmlProcessor::parse(ParserSource *parser_source) {
    myerror(ERRFILE, "support for XML parsing not compiled in.");
    return -1;
}

#else

#include <libxml/xmlreader.h>

#include <utility>

XmlProcessor::Attribute::Attribute(XmlProcessor::AttributeCallback callback_, const void *arguments_):
    cb_attr(callback_), arguments(arguments_) { }


static int xml_close([[maybe_unused]] void *);
static int xml_read(void *source, char *buffer, int length);


int XmlProcessor::parse(ParserSource *parser_source) {
    auto reader = xmlReaderForIO(xml_read, xml_close, parser_source, nullptr, nullptr, 0);
    if (reader == nullptr) {
	myerror(ERRFILE, "can't open\n");
	return -1;
    }

    ok = true;
    stop_parsing = false;

    const Entity *entity_text = nullptr;
    std::string path;

    int ret;
    while (!stop_parsing && (ret = xmlTextReaderRead(reader)) == 1) {
	if (line_number_callback) {
	    line_number_callback(context, static_cast<size_t>(xmlTextReaderGetParserLineNumber(reader)));
	}

	switch (xmlTextReaderNodeType(reader)) {
            case XML_READER_TYPE_ELEMENT: {
                auto name = std::string(reinterpret_cast<const char *>(xmlTextReaderConstName(reader)));
                path += '/' + name;
                
                auto entity = find(path);
                if (entity != nullptr) {
                    if (entity->cb_open) {
			try {
			    handle_callback_status(entity->cb_open(context, entity->arguments));
			}
			catch (std::exception &e) {
                            myerror(ERRFILE, "parse error: %s", e.what());
			    ok = false;
			}
                    }
                    
                    for (const auto &it : entity->attr) {
                        auto &attribute = it.second;
                        auto value = reinterpret_cast<char *>(xmlTextReaderGetAttribute(reader, reinterpret_cast<const xmlChar *>(it.first.c_str())));

                        if (value != nullptr) {
			    try {
				handle_callback_status(attribute.cb_attr(context, attribute.arguments, value));
			    }
			    catch (std::exception &e) {
                                myerror(ERRFILE, "parse error: %s", e.what());
				ok = false;
			    }
                            free(value);
                        }
                    }

		    if (entity->cb_text) {
			entity_text = entity;
		    }
                }
                
                if (!xmlTextReaderIsEmptyElement(reader)) {
                    break;
                }
            }
                /*
                 Fallthrough for empty elements, as we won't get an
                 extra close.
                 */

            case XML_READER_TYPE_END_ELEMENT: {
                auto entity = find(path);
                if (entity != nullptr) {
		    if (entity->cb_close) {
			try {
			    handle_callback_status(entity->cb_close(context, entity->arguments));
			}
                        catch (std::exception &e) {
                            myerror(ERRFILE, "parse error: %s", e.what());
			    ok = false;
			}
		    }
                }

                path.resize(path.find_last_of('/'));
                entity_text = nullptr;

                break;
            }
                
	case XML_READER_TYPE_TEXT:
                if (entity_text) {
		    try {
			handle_callback_status(entity_text->cb_text(context, entity_text->arguments, (const char *)xmlTextReaderConstValue(reader)));
		    }
                    catch (std::exception &e) {
                        myerror(ERRFILE, "parse error: %s", e.what());
			ok = false;
		    }
                }
                break;

            default:
                break;
        }
    }
    xmlFreeTextReader(reader);

    if (ret != 0) {
	myerror(ERRFILE, "XML parse error");
	return false;
    }

    return ok;
}


const XmlProcessor::Entity *XmlProcessor::find(const std::string &path) const {
    for (auto &pair : entities) {
	auto &name = pair.first;
	if (name == path) {
	    return &pair.second;
	}

	if (name.length() < path.length() && path[path.length() - name.length() - 1] == '/' && path.compare(path.length() - name.length(), name.length(), name) == 0) {
	    return &pair.second;
	}
    }

    return nullptr;
}


void XmlProcessor::handle_callback_status(CallbackStatus status) {
    switch (status) {
    case OK:
	break;

    case ERROR:
	ok = false;
	break;

    case END:
	stop_parsing = true;
	break;
    }
}


static int
xml_close([[maybe_unused]] void *ctx) {
    return 0;
}


static int
xml_read(void *source, char *b, int len) {
    return static_cast<int>(static_cast<ParserSource *>(source)->read(b, static_cast<size_t>(len)));
}

#endif /* HAVE_LIBXML2 */
