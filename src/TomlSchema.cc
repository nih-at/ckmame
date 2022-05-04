/*
TomlSchema.h -- Schema for checking values in TOML files.
Copyright (C) 2022 Dieter Baron and Thomas Klausner

This file is part of ckmame, a program to superfluous rom sets for MAME.
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

#include "TomlSchema.h"


bool TomlSchema::validate(const toml::node& node, const std::string& file_name_) {
    file_name = file_name_;

    return root_type->validate(node, *this, "", false);
}


std::string TomlSchema::path_append(const std::string &path, const std::string &element) {
    if (path.empty()) {
	return element;
    }
    else {
	return path + "." + element;
    }
}


void TomlSchema::print(const std::string& path, const std::string& message, bool quiet) {
    if (!quiet) {
	fprintf(stderr, "%s:%s%s %s\n", file_name.c_str(), path.c_str(), path.empty() ? "" : ":", message.c_str());
	warned = true;
    }
}


bool TomlSchema::Boolean::validate(const toml::node& node, TomlSchema& schema, const std::string& path, bool quiet) const {
    if (!node.is_boolean()) {
	schema.print(path, "expected boolean", quiet);
	return false;
    }

    return true;
}


bool TomlSchema::Integer::validate(const toml::node& node, TomlSchema& schema, const std::string& path, bool quiet) const {
    if (!node.is_integer()) {
	schema.print(path, "expected integer", quiet);
	return false;
    }

    return true;
}


bool TomlSchema::Number::validate(const toml::node& node, TomlSchema& schema, const std::string& path, bool quiet) const {
    if (!node.is_number()) {
	schema.print(path, "expected number", quiet);
	return false;
    }

    return true;
}


bool TomlSchema::String::validate(const toml::node& node, TomlSchema& schema, const std::string& path, bool quiet) const {
    if (!node.is_string()) {
	schema.print(path, "expected string", quiet);
	return false;
    }

    return true;
}


bool TomlSchema::Array::validate(const toml::node& node, TomlSchema& schema, const std::string& path, bool quiet) const {
    if (!node.is_array()) {
	schema.print(path, "expected array", quiet);
	return false;
    }

    const auto& array = *(node.as_array());

    auto ok = true;

    auto i = 0;
    for (const auto& element : array) {
	ok = elements->validate(element, schema, path_append(path, i), false) && ok;
	i++;
    }

    return true;
}


bool TomlSchema::Table::validate(const toml::node& node, TomlSchema& schema, const std::string& path, bool quiet) const {
    if (!node.is_table()) {
	schema.print(path, "expected array", quiet);
	return false;
    }

    const auto& table = *(node.as_table());

    auto ok = true;

    for (auto pair : table) {
	auto it = members.find(std::string(pair.first));
	Type *type;

	if (it != members.end()) {
	    type = it->second.get();
	}
	else {
	    type = other_members.get();
	}

	if (type == nullptr) {
	    schema.print(path_append(path, std::string(pair.first)), "unknown variable", false);
	    ok = false;
	}
	else {
	    ok = type->validate(pair.second, schema, path_append(path, std::string(pair.first)), false) && ok;
	}
    }

    return ok;
}


bool TomlSchema::Alternatives::validate(const toml::node& node, TomlSchema& schema, const std::string& path, bool quiet) const {
    for (const auto& type : alternatives) {
	if (type->validate(node, schema, path, true)) {
	    return true;
	}
    }

    if (!schema.warned) {
	schema.print(path, std::string("expected ") + name, quiet);
    }
    return false;
}
