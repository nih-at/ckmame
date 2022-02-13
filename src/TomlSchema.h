#ifndef TOML_SCHEMA_H
#define TOML_SCHEMA_H

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

#ifdef HAVE_TOMLPLUSPLUS
#include <toml++/toml.h>
#else
#include <utility>

#include "toml.hpp"
#endif


class TomlSchema {
  public:
    class Type {
      public:
	virtual bool validate(const toml::node& node, const TomlSchema& schema, const std::string& path, bool quiet) const = 0;
    };

    typedef std::shared_ptr<Type> TypePtr;

    class Boolean : public Type {
      public:
	bool validate(const toml::node& node, const TomlSchema& schema, const std::string& path, bool quiet) const override;
    };
    
    class Integer : public Type {
      public:
	bool validate(const toml::node& node, const TomlSchema& schema, const std::string& path, bool quiet) const override;
    };

    class Number : public Type {
      public:
	bool validate(const toml::node& node, const TomlSchema& schema, const std::string& path, bool quiet) const override;
    };

    class String : public Type {
      public:
	bool validate(const toml::node& node, const TomlSchema& schema, const std::string& path, bool quiet) const override;
    };
    
    class Array : public Type {
      public:
	explicit Array(TypePtr elements) : elements(std::move(elements)) { }
	bool validate(const toml::node& node, const TomlSchema& schema, const std::string& path, bool quiet) const override;
	
      private:
	TypePtr elements;
    };

    class Table : public Type {
      public:
	explicit Table(std::map<std::string, TypePtr> members, TypePtr other_members) : members(std::move(members)), other_members(std::move(other_members)) { }
	bool validate(const toml::node& node, const TomlSchema& schema, const std::string& path, bool quiet) const override;

      private:
	std::map<std::string, TypePtr> members;
	TypePtr other_members;
    };
    
    class Alternatives : public Type {
      public:
	Alternatives(std::vector<TypePtr> alternatives, std::string name) : alternatives(std::move(alternatives)), name(std::move(name)) { }
	bool validate(const toml::node& node, const TomlSchema& schema, const std::string& path, bool quiet) const override;

      private:
	std::vector<TypePtr> alternatives;
	std::string name;
    };

    explicit TomlSchema(TypePtr root_type) : root_type(std::move(root_type)) { }

    static TypePtr boolean() { return TypePtr(new Boolean()); }
    static TypePtr integer() { return TypePtr(new Integer()); }
    static TypePtr number() { return TypePtr(new Number()); }
    static TypePtr string() { return TypePtr(new String()); }
    static TypePtr array(TypePtr elements) { return TypePtr(new Array(std::move(elements))); }
    static TypePtr table(std::map<std::string, TypePtr> members, TypePtr other_members) { return TypePtr(new Table(std::move(members), std::move(other_members))); }
    static TypePtr alternatives(std::vector<TypePtr> alternatives, std::string name) { return TypePtr(new Alternatives(std::move(alternatives), std::move(name))); }

    bool validate(const toml::node& node, const std::string& file_name);
    
    void print(const std::string& path, const std::string& message, bool quiet) const;
    static std::string path_append(const std::string& path, const std::string& element);
    static std::string path_append(const std::string& path, int index) { return path_append(path, std::to_string(index)); };

  private:
    TypePtr root_type;
    std::string file_name;
};


#endif // TOML_SCHEMA_H
