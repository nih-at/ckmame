//
// Created by Dieter Baron on 2022/03/10.
//

#include "Output.h"

#include "globals.h"

Output::Output() :
    file_infos({FileInfo("", "")}),
    first_header(true),
    header_done(false),
    subheader_done(false),
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
    print_header();

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
