/*
  detector_parse_ps.c -- parse clrmamepro header skip detector XML file
  Copyright (C) 2007-2014 Dieter Baron and Thomas Klausner

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

#include "Detector.h"

#include <cerrno>

#include "util.h"
#include "xmlutil.h"


class DetectorParserContext {
public:
    DetectorParserContext() : detector(std::make_shared<Detector>()), rule(NULL), test(NULL) { }
    
    DetectorPtr detector;
    Detector::Rule *rule;
    Detector::Test *test;
};


static std::optional<int> parse_enum(const std::string &value, const std::unordered_map<std::string, int> &enums);
static bool parse_hex(Detector::Test *test, std::vector<uint8_t> *result, const std::string &value);
static bool parse_number(int64_t *result, const std::string &value);
static bool parse_offset(int64_t *result, const std::string &value);
static bool parse_size(int64_t *result, const std::string &value);

static bool rule_close(void *, int);
static bool rule_end_offset(void *, int, int, const std::string &);
static bool rule_open(void *, int);
static bool rule_operation(void *, int, int, const std::string &);
static bool rule_start_offset(void *, int, int, const std::string &);

static bool test_close(void *, int);
static bool test_mask(void *, int, int, const std::string &);
static bool test_offset(void *, int, int, const std::string &);
static bool test_open(void *, int);
static bool test_operator(void *ctx, int, int, const std::string &);
static bool test_result(void *, int, int, const std::string &);
static bool test_size(void *, int, int, const std::string &);
static bool test_value(void *, int, int, const std::string &);

static bool text_author(void *, const std::string &);
static bool text_name(void *, const std::string &);
static bool text_version(void *, const std::string &);

static const std::unordered_map<std::string, XmluAttr> attr_bit = {
    { "offset", { test_offset, 0, 0 } },
    { "mask", { test_mask, 0, 0 } },
    { "value", { test_value, 0, 0 } },
    { "result", { test_result, 0, 0 } }
};

static const std::unordered_map<std::string, XmluAttr> attr_data = {
    { "offset", { test_offset, 0, 0 } },
    { "value", { test_value, 0, 0 } },
    { "result", { test_result, 0, 0 } }
};

static const std::unordered_map<std::string, XmluAttr> attr_file = {
    { "size", { test_size, 0, 0 } },
    { "operator", { test_operator, 0, 0 } },
    { "result", { test_result, 0, 0} }
};

static const std::unordered_map<std::string, XmluAttr> attr_rule = {
    { "start_offset", { rule_start_offset, 0, 0 } },
    { "end_offset", { rule_end_offset, 0, 0 } },
    { "operation", { rule_operation, 0, 0 } }
};

static const std::unordered_map<std::string, XmluEntity> entities = {
    { "and", XmluEntity(attr_bit, test_open, test_close, Detector::TEST_AND) },
    { "author", XmluEntity(text_author) },
    { "data", XmluEntity(attr_data, test_open, test_close, Detector::TEST_DATA) },
    { "file", XmluEntity(attr_file, test_open, test_close) },
    { "name", XmluEntity(text_name) },
    { "or", XmluEntity(attr_bit, test_open, test_close, Detector::TEST_OR) },
    { "rule", XmluEntity(attr_rule, rule_open, rule_close) },
    { "version", XmluEntity(text_version) },
    { "xor", XmluEntity(attr_bit, test_open, test_close, Detector::TEST_XOR) }
};


DetectorPtr
Detector::parse(ParserSource *ps) {
    DetectorParserContext ctx;

    /* TODO: lineno callback */
    if (!xmlu_parse(ps, &ctx, NULL, entities)) {
	return NULL;
    }

    auto detector = ctx.detector;
    
    detector->id = get_id(DetectorDescriptor(detector.get()));
    return detector;
}


static std::optional<int> parse_enum(const std::string &value, const std::unordered_map<std::string, int> &enums) {
    auto it = enums.find(value);
    
    if (it == enums.end()) {
        errno = EINVAL;
        return {};
    }

    return it->second;
}


static bool parse_hex(Detector::Test *test, std::vector<uint8_t> *result, const std::string &value) {
    if (value.size() % 2 != 0) {
        errno = EINVAL;
	return false;
    }
    auto length = value.size() / 2;

    if (test->length != 0 && test->length != length) {
	errno = EINVAL;
	return false;
    }

    *result = hex2bin(value);

    test->length = length;
    return true;
}


static bool parse_number(int64_t *offsetp, const std::string &value) {
    int64_t i;

    if (value.empty()) {
	errno = EINVAL;
	return false;
    }

    try {
	size_t end;

	i = std::stoll(value, &end, 16);

	if (end != value.length()) {
	    errno = EINVAL;
	    return false;
	}
    }
    catch (...) {
	errno = EINVAL;
	return false;
    }

    *offsetp = i;
    return true;
}


static bool parse_offset(int64_t *offsetp, const std::string &value) {
    if (value == "EOF") {
	*offsetp = DETECTOR_OFFSET_EOF;
	return true;
    }

    return parse_number(offsetp, value);
}


static bool parse_size(int64_t *offsetp, const std::string &value) {
    if (value == "PO2") {
	*offsetp = DETECTOR_SIZE_POWER_OF_2;
	return true;
    }

    return parse_number(offsetp, value);
}


static bool rule_close(void *ctx, int arg1) {
    auto context = static_cast<DetectorParserContext *>(ctx);

    context->rule = NULL;

    return true;
}


static bool rule_end_offset(void *ctx, int arg1, int arg2, const std::string &value) {
    auto context = static_cast<DetectorParserContext *>(ctx);

    return parse_offset(&context->rule->end_offset, value);
}


static bool rule_open(void *ctx, int arg1) {
    auto context = static_cast<DetectorParserContext *>(ctx);
    
    context->detector->rules.push_back(Detector::Rule());
    context->rule = &context->detector->rules[context->detector->rules.size() - 1];

    return true;
}


static bool rule_operation(void *ctx, int arg1, int arg2, const std::string &value) {
    static const std::unordered_map<std::string, int> op = {
        { "bitswap", Detector::OP_BITSWAP },
        { "byteswap", Detector::OP_BYTESWAP },
        { "none", Detector::OP_NONE },
        { "wordswap", Detector::OP_WORDSWAP }
    };

    auto context = static_cast<DetectorParserContext *>(ctx);
    auto i = parse_enum(value, op);
    
    if (!i.has_value()) {
	return false;
    }

    context->rule->operation = static_cast<Detector::Operation>(i.value());
    return true;
}


static bool rule_start_offset(void *ctx, int arg1, int arg2, const std::string &value) {
    auto context = static_cast<DetectorParserContext *>(ctx);

    return parse_offset(&context->rule->start_offset, value);
}


static bool test_close(void *ctx, int arg1) {
    auto context = static_cast<DetectorParserContext *>(ctx);

    context->test = NULL;

    return true;
}


static bool test_mask(void *ctx, int arg1, int arg2, const std::string &value) {
    auto context = static_cast<DetectorParserContext *>(ctx);

    return parse_hex(context->test, &context->test->mask, value);
}


static bool test_offset(void *ctx, int arg1, int arg2, const std::string &value) {
    auto context = static_cast<DetectorParserContext *>(ctx);

    return parse_offset(&context->test->offset, value);
}


static bool test_open(void *ctx, int type) {
    auto context = static_cast<DetectorParserContext *>(ctx);

    context->rule->tests.push_back(Detector::Test());
    context->test = &context->rule->tests[context->rule->tests.size() - 1];

    return true;
}


static bool test_operator(void *ctx, int arg1, int arg2, const std::string &value) {
    static std::unordered_map<std::string, int> enums = {
        { "equal", Detector::TEST_FILE_EQ },
	{ "greater", Detector::TEST_FILE_GR },
        { "less", Detector::TEST_FILE_LE }
    };

    auto context = static_cast<DetectorParserContext *>(ctx);
    auto i = parse_enum(value, enums);
    
    if (!i.has_value()) {
    return false;
    }

    context->test->type = static_cast<Detector::TestType>(i.value());
    return true;
}


static bool test_result(void *ctx, int arg1, int arg2, const std::string &value) {
    static std::unordered_map<std::string, int> enums = {
	{"false", false},
	{"true", true},
    };

    auto context = static_cast<DetectorParserContext *>(ctx);
    auto i = parse_enum(value, enums);
    
    if (!i.has_value()) {
        return false;
    }

    context->test->result = i.value();
    return true;
}


static bool test_size(void *ctx, int arg1, int arg2, const std::string &value) {
    auto context = static_cast<DetectorParserContext *>(ctx);

    return parse_size(&context->test->offset, value);
}


static bool test_value(void *ctx, int arg1, int arg2, const std::string &value) {
    auto context = static_cast<DetectorParserContext *>(ctx);

    return parse_hex(context->test, &context->test->value, value);
}


static bool text_author(void *ctx, const std::string &txt) {
    auto context = static_cast<DetectorParserContext *>(ctx);

    context->detector->author = txt;
    
    return true;
}


static bool text_name(void *ctx, const std::string &txt) {
    auto context = static_cast<DetectorParserContext *>(ctx);

    context->detector->name = txt;

    return true;
}


static bool text_version(void *ctx, const std::string &txt) {
    auto context = static_cast<DetectorParserContext *>(ctx);

    context->detector->version = txt;

    return true;
}
