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
#include "XmlProcessor.h"


class DetectorParserContext {
public:
    DetectorParserContext() : detector(std::make_shared<Detector>()), rule(nullptr), test(nullptr) { }

    bool parse(ParserSource *source);
    
    DetectorPtr detector;

  private:
    Detector::Rule *rule;
    Detector::Test *test;
    
    class Arguments {
      public:
	explicit Arguments(Detector::TestType test) : test(test) { }
	Detector::TestType test;
    };
    
    static Arguments arguments_and;
    static Arguments arguments_data;
    static Arguments arguments_or;
    static Arguments arguments_xor;
    
    static std::optional<int> parse_enum(const std::string &value, const std::unordered_map<std::string, int> &enums);
    XmlProcessor::CallbackStatus parse_hex(std::vector<uint8_t> *result, const std::string &value);
    static XmlProcessor::CallbackStatus parse_number(int64_t *result, const std::string &value);
    static XmlProcessor::CallbackStatus parse_offset(int64_t *result, const std::string &value);
    static XmlProcessor::CallbackStatus parse_size(int64_t *result, const std::string &value);

    static XmlProcessor::CallbackStatus rule_close(void *ctx, [[maybe_unused]] const void *args);
    static XmlProcessor::CallbackStatus rule_end_offset(void *ctx, [[maybe_unused]] const void *args, const std::string &value);
    static XmlProcessor::CallbackStatus rule_open(void *ctx, [[maybe_unused]] const void *args);
    static XmlProcessor::CallbackStatus rule_operation(void *ctx, [[maybe_unused]] const void *args, const std::string &value);
    static XmlProcessor::CallbackStatus rule_start_offset(void *ctx, [[maybe_unused]] const void *args, const std::string &value);

    static XmlProcessor::CallbackStatus test_close(void *ctx, [[maybe_unused]] [[maybe_unused]] [[maybe_unused]] [[maybe_unused]] const void *args);
    static XmlProcessor::CallbackStatus test_mask(void *ctx, [[maybe_unused]] const void *args, const std::string &value);
    static XmlProcessor::CallbackStatus test_offset(void *ctx, [[maybe_unused]] const void *args, const std::string &value);
    static XmlProcessor::CallbackStatus test_open(void *ctx, [[maybe_unused]] const void *args);
    static XmlProcessor::CallbackStatus test_operator(void *ctx, [[maybe_unused]] const void *args, const std::string &value);
    static XmlProcessor::CallbackStatus test_result(void *ctx, [[maybe_unused]] const void *args, const std::string &value);
    static XmlProcessor::CallbackStatus test_size(void *ctx, [[maybe_unused]] const void *args, const std::string &value);
    static XmlProcessor::CallbackStatus test_value(void *ctx, [[maybe_unused]] const void *args, const std::string &value);
    static XmlProcessor::CallbackStatus text_author(void *ctx, [[maybe_unused]] const void *args, const std::string &value);
    static XmlProcessor::CallbackStatus text_name(void *ctx, [[maybe_unused]] const void *args, const std::string &value);
    static XmlProcessor::CallbackStatus text_version(void *ctx, [[maybe_unused]] const void *args, const std::string &value);
    
    static const std::unordered_map<std::string, XmlProcessor::Attribute> attributes_bit;
    static const std::unordered_map<std::string, XmlProcessor::Attribute> attributes_data;
    static const std::unordered_map<std::string, XmlProcessor::Attribute> attributes_file;
    static const std::unordered_map<std::string, XmlProcessor::Attribute> attributes_rule;
	
    static const std::unordered_map<std::string, XmlProcessor::Entity> entities;
};

DetectorParserContext::Arguments DetectorParserContext::arguments_and(Detector::TEST_AND);
DetectorParserContext::Arguments DetectorParserContext::arguments_data(Detector::TEST_DATA);
DetectorParserContext::Arguments DetectorParserContext::arguments_or(Detector::TEST_OR);
DetectorParserContext::Arguments DetectorParserContext::arguments_xor(Detector::TEST_XOR);

const std::unordered_map<std::string, XmlProcessor::Attribute> DetectorParserContext::attributes_bit = {
    { "offset", XmlProcessor::Attribute(test_offset, nullptr) },
    { "mask", XmlProcessor::Attribute(test_mask, nullptr) },
    { "value", XmlProcessor::Attribute(test_value, nullptr) },
    { "result", XmlProcessor::Attribute(test_result, nullptr) }
};

const std::unordered_map<std::string, XmlProcessor::Attribute> DetectorParserContext::attributes_data = {
    { "offset", XmlProcessor::Attribute(test_offset, nullptr) },
    { "value", XmlProcessor::Attribute(test_value, nullptr) },
    { "result", XmlProcessor::Attribute(test_result, nullptr) }
};

const std::unordered_map<std::string, XmlProcessor::Attribute> DetectorParserContext::attributes_file = {
    { "size", XmlProcessor::Attribute(test_size, nullptr) },
    { "operator", XmlProcessor::Attribute(test_operator, nullptr) },
    { "result", XmlProcessor::Attribute(test_result, nullptr) }
};

const std::unordered_map<std::string, XmlProcessor::Attribute> DetectorParserContext::attributes_rule = {
    { "start_offset", XmlProcessor::Attribute(rule_start_offset, nullptr) },
    { "end_offset", XmlProcessor::Attribute(rule_end_offset, nullptr) },
    { "operation", XmlProcessor::Attribute(rule_operation,nullptr) }
};

const std::unordered_map<std::string, XmlProcessor::Entity> DetectorParserContext::entities = {
    { "and", XmlProcessor::Entity(attributes_bit, test_open, test_close, &arguments_and) },
    { "author", XmlProcessor::Entity(text_author) },
    { "data", XmlProcessor::Entity(attributes_data, test_open, test_close, &arguments_data) },
    { "file", XmlProcessor::Entity(attributes_file, test_open, test_close) },
    { "name", XmlProcessor::Entity(text_name) },
    { "or", XmlProcessor::Entity(attributes_bit, test_open, test_close, &arguments_or) },
    { "rule", XmlProcessor::Entity(attributes_rule, rule_open, rule_close) },
    { "version", XmlProcessor::Entity(text_version) },
    { "xor", XmlProcessor::Entity(attributes_bit, test_open, test_close, &arguments_xor) }
};

bool DetectorParserContext::parse(ParserSource *source) {
    XmlProcessor processor(nullptr, entities, this);

    return processor.parse(source);
}

DetectorPtr Detector::parse(ParserSource *parser_source) {
    DetectorParserContext parser_context;

    /* TODO: lineno callback */
    if (!parser_context.parse(parser_source)) {
	return nullptr;
    }

    auto detector = parser_context.detector;
    
    detector->id = get_id(DetectorDescriptor(detector.get()));
    return detector;
}


std::optional<int> DetectorParserContext::parse_enum(const std::string &value, const std::unordered_map<std::string, int> &enums) {
    auto it = enums.find(value);
    
    if (it == enums.end()) {
        errno = EINVAL;
        return {};
    }

    return it->second;
}


XmlProcessor::CallbackStatus DetectorParserContext::parse_hex(std::vector<uint8_t> *result, const std::string &value) {
    if (value.size() % 2 != 0) {
        errno = EINVAL;
	return XmlProcessor::ERROR;
    }
    auto length = value.size() / 2;

    if (test->length != 0 && test->length != length) {
	errno = EINVAL;
	return XmlProcessor::ERROR;
    }

    *result = hex2bin(value);

    test->length = length;
    return XmlProcessor::OK;
}


XmlProcessor::CallbackStatus DetectorParserContext::parse_number(int64_t *result, const std::string &value) {
    int64_t i;

    if (value.empty()) {
	errno = EINVAL;
	return XmlProcessor::ERROR;
    }

    try {
	size_t end;

	i = std::stoll(value, &end, 16);

	if (end != value.length()) {
	    errno = EINVAL;
	    return XmlProcessor::ERROR;
	}
    }
    catch (...) {
	errno = EINVAL;
	return XmlProcessor::ERROR;
    }

    *result = i;
    return XmlProcessor::OK;
}


XmlProcessor::CallbackStatus DetectorParserContext::parse_offset(int64_t *result, const std::string &value) {
    if (value == "EOF") {
	*result = DETECTOR_OFFSET_EOF;
	return XmlProcessor::ERROR;
    }

    return parse_number(result, value);
}


XmlProcessor::CallbackStatus DetectorParserContext::parse_size(int64_t *result, const std::string &value) {
    if (value == "PO2") {
	*result = DETECTOR_SIZE_POWER_OF_2;
	return XmlProcessor::OK;
    }

    return parse_number(result, value);
}


XmlProcessor::CallbackStatus DetectorParserContext::rule_close(void *ctx, [[maybe_unused]] const void *args) {
    auto context = static_cast<DetectorParserContext *>(ctx);

    context->rule = nullptr;

    return XmlProcessor::OK;
}


XmlProcessor::CallbackStatus DetectorParserContext::rule_end_offset(void *ctx, [[maybe_unused]] const void *args, const std::string &value) {
    auto context = static_cast<DetectorParserContext *>(ctx);

    return parse_offset(&context->rule->end_offset, value);
}


XmlProcessor::CallbackStatus DetectorParserContext::rule_open(void *ctx, [[maybe_unused]] const void *args) {
    auto context = static_cast<DetectorParserContext *>(ctx);
    
    context->detector->rules.emplace_back();
    context->rule = &context->detector->rules[context->detector->rules.size() - 1];

    return XmlProcessor::OK;
}


XmlProcessor::CallbackStatus DetectorParserContext::rule_operation(void *ctx, [[maybe_unused]] const void *args, const std::string &value) {
    static const std::unordered_map<std::string, int> op = {
        { "bitswap", Detector::OP_BITSWAP },
        { "byteswap", Detector::OP_BYTESWAP },
        { "none", Detector::OP_NONE },
        { "wordswap", Detector::OP_WORDSWAP }
    };

    auto context = static_cast<DetectorParserContext *>(ctx);
    auto i = parse_enum(value, op);
    
    if (!i.has_value()) {
	return XmlProcessor::ERROR;
    }

    context->rule->operation = static_cast<Detector::Operation>(i.value());
    return XmlProcessor::OK;
}


XmlProcessor::CallbackStatus DetectorParserContext::rule_start_offset(void *ctx, [[maybe_unused]] const void *args, const std::string &value) {
    auto context = static_cast<DetectorParserContext *>(ctx);

    return parse_offset(&context->rule->start_offset, value);
}


[[maybe_unused]] [[maybe_unused]] [[maybe_unused]] XmlProcessor::CallbackStatus DetectorParserContext::test_close(void *ctx, [[maybe_unused]] const void *args) {
    auto context = static_cast<DetectorParserContext *>(ctx);

    context->test = nullptr;

    return XmlProcessor::OK;
}


XmlProcessor::CallbackStatus DetectorParserContext::test_mask(void *ctx, [[maybe_unused]] const void *args, const std::string &value) {
    auto context = static_cast<DetectorParserContext *>(ctx);

    return context->parse_hex(&context->test->mask, value);
}


XmlProcessor::CallbackStatus DetectorParserContext::test_offset(void *ctx, [[maybe_unused]] const void *args, const std::string &value) {
    auto context = static_cast<DetectorParserContext *>(ctx);

    return parse_offset(&context->test->offset, value);
}


XmlProcessor::CallbackStatus DetectorParserContext::test_open(void *ctx, [[maybe_unused]] const void *args) {
    auto context = static_cast<DetectorParserContext *>(ctx);

    context->rule->tests.emplace_back();
    context->test = &context->rule->tests[context->rule->tests.size() - 1];

    return XmlProcessor::OK;
}


XmlProcessor::CallbackStatus DetectorParserContext::test_operator(void *ctx, [[maybe_unused]] const void *args, const std::string &value) {
    static std::unordered_map<std::string, int> enums = {
        { "equal", Detector::TEST_FILE_EQ },
	{ "greater", Detector::TEST_FILE_GR },
        { "less", Detector::TEST_FILE_LE }
    };

    auto context = static_cast<DetectorParserContext *>(ctx);
    auto i = parse_enum(value, enums);
    
    if (!i.has_value()) {
    return XmlProcessor::ERROR;
    }

    context->test->type = static_cast<Detector::TestType>(i.value());
    return XmlProcessor::OK;
}


XmlProcessor::CallbackStatus DetectorParserContext::test_result(void *ctx, [[maybe_unused]] const void *args, const std::string &value) {
    static std::unordered_map<std::string, int> enums = {
	{"false", false},
	{"true", true},
    };

    auto context = static_cast<DetectorParserContext *>(ctx);
    auto i = parse_enum(value, enums);
    
    if (!i.has_value()) {
        return XmlProcessor::ERROR;
    }

    context->test->result = i.value();
    return XmlProcessor::OK;
}


XmlProcessor::CallbackStatus DetectorParserContext::test_size(void *ctx, [[maybe_unused]] const void *args, const std::string &value) {
    auto context = static_cast<DetectorParserContext *>(ctx);

    return parse_size(&context->test->offset, value);
}


XmlProcessor::CallbackStatus DetectorParserContext::test_value(void *ctx, [[maybe_unused]] const void *args, const std::string &value) {
    auto context = static_cast<DetectorParserContext *>(ctx);

    return context->parse_hex(&context->test->value, value);
}


XmlProcessor::CallbackStatus DetectorParserContext::text_author(void *ctx, [[maybe_unused]] const void *args, const std::string &value) {
    auto context = static_cast<DetectorParserContext *>(ctx);

    context->detector->author = value;
    
    return XmlProcessor::OK;
}


XmlProcessor::CallbackStatus DetectorParserContext::text_name(void *ctx, [[maybe_unused]] const void *args, const std::string &value) {
    auto context = static_cast<DetectorParserContext *>(ctx);

    context->detector->name = value;

    return XmlProcessor::OK;
}


XmlProcessor::CallbackStatus DetectorParserContext::text_version(void *ctx, [[maybe_unused]] const void *args, const std::string &value) {
    auto context = static_cast<DetectorParserContext *>(ctx);

    context->detector->version = value;

    return XmlProcessor::OK;
}
