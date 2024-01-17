/*
  Output.cc -- write information about the current operation
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

#include "Output.h"

#include "globals.h"
#include "compat.h"

Output::Output() :
    first_header(true),
    header_done(false),
    subheader_done(false),
    file_infos({FileInfo("", "")}),
    db(nullptr) {
}

void Output::set_header(std::string new_header) {
    header = std::move(new_header);
    header_done = false;
    set_subheader("");
}


void Output::set_subheader(std::string new_subheader) {
    subheader = std::move(new_subheader);
    subheader_done = false;
}


void Output::set_error_archive(std::string new_archive_name, std::string new_file_name) {
    file_infos.back().archive_name = std::move(new_archive_name);
    set_error_file(std::move(new_file_name));
}


void Output::set_error_file(std::string new_file_name) {
    file_infos.back().file_name = std::move(new_file_name);
}


void Output::set_error_database(DB* new_db) {
    db = new_db;
}


void Output::print_header() {
    if (!header_done && !header.empty()) {
	if (first_header) {
	    first_header = false;
	}
	else {
	    printf("\n");
	}
	printf("%s:\n", header.c_str());
    }
    if (!subheader_done && !subheader.empty()) {
	printf("%s:\n", subheader.c_str());
    }
    header_done = true;
    subheader_done = true;
}


void Output::message(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    print_message_v(fmt, va);
    va_end(va);
}


void Output::message_verbose(const char *fmt, ...) {
    if (!configuration.verbose) {
	return;
    }
    va_list va;
    va_start(va, fmt);
    print_message_v(fmt, va);
    va_end(va);
}

void Output::print_message_v(const char *fmt, va_list va) {
    print_header();
    vprintf(fmt, va);
    printf("\n");
}


void Output::error(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    print_error_v(fmt, va);
    va_end(va);
}


void Output::error_database(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    print_error_v(fmt, va, "", postfix_database());
    va_end(va);
}

void Output::error_system(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    print_error_v(fmt, va, "", postfix_system());
    va_end(va);
}


void Output::error_error_code(const std::error_code &ec, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    print_error_v(fmt, va, "", ec.message());
    va_end(va);
}


void Output::archive_error(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    print_error_v(fmt, va, file_infos.back().archive_name);
    va_end(va);
}


void Output::archive_error_database(const char *fmt, ...){
    va_list va;
    va_start(va, fmt);
    print_error_v(fmt, va, file_infos.back().archive_name, postfix_database());
    va_end(va);
}


void Output::archive_error_system(const char *fmt, ...){
    va_list va;
    va_start(va, fmt);
    print_error_v(fmt, va, file_infos.back().archive_name, postfix_system());
    va_end(va);
}


void Output::archive_error_error_code(const std::error_code &ec, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    print_error_v(fmt, va, file_infos.back().archive_name, ec.message());
    va_end(va);
}


void Output::archive_file_error(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    print_error_v(fmt, va, prefix_archive_file());
    va_end(va);
}


void Output::archive_file_error_database(const char *fmt, ...){
    va_list va;
    va_start(va, fmt);
    print_error_v(fmt, va, prefix_archive_file(), postfix_database());
    va_end(va);
}


void Output::archive_file_error_error_code(const std::error_code &ec, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    print_error_v(fmt, va, prefix_archive_file(), ec.message());
    va_end(va);
}


void Output::file_error(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    print_error_v(fmt, va, file_infos.back().file_name);
    va_end(va);
}


void Output::file_error_database(const char *fmt, ...){
    va_list va;
    va_start(va, fmt);
    print_error_v(fmt, va, file_infos.back().file_name, postfix_database());
    va_end(va);
}


void Output::file_error_system(const char *fmt, ...){
    va_list va;
    va_start(va, fmt);
    print_error_v(fmt, va, file_infos.back().file_name, postfix_system());
    va_end(va);
}


void Output::archive_file_error_system(const char *fmt, ...){
    va_list va;
    va_start(va, fmt);
    print_error_v(fmt, va, file_infos.back().file_name, postfix_system());
    va_end(va);
}


void Output::file_error_error_code(const std::error_code &ec, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    print_error_v(fmt, va, file_infos.back().file_name, ec.message());
    va_end(va);
}


void Output::line_error(size_t line_number, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    print_error_v(fmt, va, prefix_line(line_number));
    va_end(va);
}


void Output::line_error_database(size_t line_number, const char *fmt, ...){
    va_list va;
    va_start(va, fmt);
    print_error_v(fmt, va, prefix_line(line_number), postfix_database());
    va_end(va);
}


void Output::line_error_system(size_t line_number, const char *fmt, ...){
    va_list va;
    va_start(va, fmt);
    print_error_v(fmt, va, prefix_line(line_number), postfix_system());
    va_end(va);
}


void Output::line_error_error_code(size_t line_number, const std::error_code &ec, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    print_error_v(fmt, va, prefix_line(line_number), ec.message());
    va_end(va);
}



std::string Output::prefix_archive_file() {
    if (!file_infos.back().archive_name.empty() && !file_infos.back().file_name.empty()) {
	return file_infos.back().archive_name + "(" + file_infos.back().file_name + ")";
    }
    else {
	return "";
    }
}


std::string Output::postfix_database() {
    if (db == nullptr) {
        return "no database";
    }
    else {
        return db->error();
    }
}


std::string  Output::prefix_line(size_t line_number) {
    // TODO: also use archive_name
    return file_infos.back().file_name + ":" + std::to_string(line_number);
}

std::string Output::postfix_system() {
    if (errno == 0) {
	return "";
    }
    else {
        return strerror(errno);
    }
}

void Output::print_error_v(const char *fmt, va_list va, const std::string &prefix, const std::string &postfix) {
    // Don't print header to stdout for error messages printed to stderr.
    
    fprintf(stderr, "%s: ", getprogname());
    if (!prefix.empty()) {
	fprintf(stderr, "%s: ", prefix.c_str());
    }
    vfprintf(stderr, fmt, va);
    if (!postfix.empty()) {
	fprintf(stderr, ": %s", postfix.c_str());
    }
    fprintf(stderr, "\n");
}
void Output::push_error_archive(std::string archive_name, std::string file_name) {
    file_infos.emplace_back(FileInfo(std::move(archive_name), std::move(file_name)));
}


void Output::push_error_file(std::string file_name) {
    file_infos.emplace_back(FileInfo("", std::move(file_name)));
}


void Output::pop_error_file_info() {
    if (file_infos.size() > 1) {
        file_infos.pop_back();
    }
}
