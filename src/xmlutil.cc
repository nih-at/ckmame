/*
  xmlutil.c -- parse XML file via callbacks
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
#include "xmlutil.h"

#ifndef HAVE_LIBXML2

/*ARGSUSED1*/
int xmlu_parse(ParserSource *ps, void *ctx, xmlu_lineno_cb lineno_cb, const std::unordered_map<std::string, XmluEntity> &entities) {
    myerror(ERRFILE, "support for XML parsing not compiled in.");
    return -1;
}

#else

#include <libxml/xmlreader.h>

#define XMLU_MAX_PATH 8192

static int xml_close(void *);
static const XmluEntity *xml_find(const std::unordered_map<std::string, XmluEntity> &entities, const std::string &path);
static int xml_read(void *source, char *buffer, int length);


int xmlu_parse(ParserSource *ps, void *ctx, xmlu_lineno_cb lineno_cb, const std::unordered_map<std::string, XmluEntity> &entities) {
    auto reader = xmlReaderForIO(xml_read, xml_close, ps, NULL, NULL, 0);
    if (reader == NULL) {
	myerror(ERRFILE, "can't open\n");
	return -1;
    }

    auto ok = true;
    const XmluEntity *entity_text = NULL;
    std::string path;

    int ret;
    while ((ret = xmlTextReaderRead(reader)) == 1) {
	if (lineno_cb)
	    lineno_cb(ctx, xmlTextReaderGetParserLineNumber(reader));

	switch (xmlTextReaderNodeType(reader)) {
            case XML_READER_TYPE_ELEMENT: {
                auto name = std::string(reinterpret_cast<const char *>(xmlTextReaderConstName(reader)));
                path = path + '/' + name;
                
                auto entity = xml_find(entities, path);
                if (entity != NULL) {
                    if (entity->cb_open) {
			try {
			    if (!entity->cb_open(ctx, entity->arg1)) {
				ok = false;
			    }
			}
			catch (std::exception &e) {
                            myerror(ERRFILE, "parse error: %s", e.what());
			    ok = false;
			}
                    }
                    
                    for (auto it : entity->attr) {
                        auto &attribute = it.second;
                        auto value = reinterpret_cast<char *>(xmlTextReaderGetAttribute(reader, reinterpret_cast<const xmlChar *>(it.first.c_str())));

                        if (value != NULL) {
			    try {
				if (!attribute.cb_attr(ctx, attribute.arg1, attribute.arg2, value)) {
				    ok = false;
				}
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
                auto entity = xml_find(entities, path);
                if (entity != NULL) {
		    if (entity->cb_close) {
			try {
			    if (!entity->cb_close(ctx, entity->arg1)) {
				ok = false;
			    }
			}
                        catch (std::exception &e) {
                            myerror(ERRFILE, "parse error: %s", e.what());
			    ok = false;
			}
		    }
                }

                path.resize(path.find_last_of('/'));
                entity_text = NULL;

                break;
            }
                
	case XML_READER_TYPE_TEXT:
                if (entity_text) {
		    try {
			entity_text->cb_text(ctx, (const char *)xmlTextReaderConstValue(reader));
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
	/* TODO: parse error */
	printf("parse error\n");
	return false;
    }

    return ok;
}


static int
xml_close(void *ctx) {
    return 0;
}


static const XmluEntity *xml_find(const std::unordered_map<std::string, XmluEntity> &entities, const std::string &path) {
    for (auto &pair : entities) {
        auto &name = pair.first;
        if (name == path) {
            return &pair.second;
        }

        if (name.length() < path.length() && path[path.length() - name.length() - 1] == '/' && path.compare(path.length() - name.length(), name.length(), name) == 0) {
            return &pair.second;
       }
    }
    
    return NULL;
}


static int
xml_read(void *source, char *b, int len) {
    return static_cast<int>(static_cast<ParserSource *>(source)->read(b, static_cast<size_t>(len)));
}

#endif /* HAVE_LIBXML2 */
