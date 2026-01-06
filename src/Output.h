/*
  Output.h -- write information about the current operation
  Copyright (C) 1999-2024 Dieter Baron and Thomas Klausner

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

#ifndef OUTPUT_H
#define OUTPUT_H

#include <string>
#include <system_error>

#include "DB.h"
#include "printf_like.h"

class Output {
  public:
    Output();

    void set_header(std::string header);
    void set_subheader(std::string subheader);
    void message(const std::string& string) { message("%s", string.c_str()); }
    void message(const char* fmt, ...) PRINTF_LIKE(2, 3);
    void message_verbose(const std::string& string) { message_verbose("%s", string.c_str()); }
    void message_verbose(const char* fmt, ...) PRINTF_LIKE(2, 3);

    void push_error_archive(std::string archive_name, std::string file_name = "");
    void push_error_file(std::string file_name);
    void pop_error_file_info();

    void set_error_archive(std::string archive_name, std::string file_name = "");
    void set_error_database(DB* db);
    void set_error_file(std::string file_name);

    void error(const char* fmt, ...) PRINTF_LIKE(2, 3);
    void error_database(const char* fmt, ...) PRINTF_LIKE(2, 3);
    void error_system(const char* fmt, ...) PRINTF_LIKE(2, 3);
    void error_error_code(const std::error_code& ec, const char* fmt, ...) PRINTF_LIKE(3, 4);

    void archive_error(const char* fmt, ...) PRINTF_LIKE(2, 3);
    void archive_error_database(const char* fmt, ...) PRINTF_LIKE(2, 3);
    void archive_error_system(const char* fmt, ...) PRINTF_LIKE(2, 3);
    void archive_error_error_code(const std::error_code& ec, const char* fmt, ...) PRINTF_LIKE(3, 4);

    void archive_file_error(const char* fmt, ...) PRINTF_LIKE(2, 3);
    void archive_file_error_database(const char* fmt, ...) PRINTF_LIKE(2, 3);
    void archive_file_error_system(const char* fmt, ...) PRINTF_LIKE(2, 3);
    void archive_file_error_error_code(const std::error_code& ec, const char* fmt, ...) PRINTF_LIKE(3, 4);

    void file_error(const char* fmt, ...) PRINTF_LIKE(2, 3);
    void file_error_database(const char* fmt, ...) PRINTF_LIKE(2, 3);
    void file_error_system(const char* fmt, ...) PRINTF_LIKE(2, 3);
    void file_error_error_code(const std::error_code& ec, const char* fmt, ...) PRINTF_LIKE(3, 4);

    void line_error(size_t line_number, const char* fmt, ...) PRINTF_LIKE(3, 4);
    void line_error_database(size_t line_number, const char* fmt, ...) PRINTF_LIKE(3, 4);
    void line_error_system(size_t line_number, const char* fmt, ...) PRINTF_LIKE(3, 4);
    void line_error_error_code(size_t line_number, const std::error_code& ec, const char* fmt, ...) PRINTF_LIKE(4, 5);

  private:
    class FileInfo {
      public:
        FileInfo(std::string archive_name, std::string file_name) : archive_name(std::move(archive_name)), file_name(std::move(file_name)) { }

        std::string archive_name;
        std::string file_name;
    };

    std::string header;
    std::string subheader;
    bool first_header;
    bool header_done;
    bool subheader_done;

    std::vector<FileInfo> file_infos;
    DB* db;

    void print_header();

    void print_message_v(const char* fmt, va_list va);
    void print_error_v(const char* fmt, va_list va, const std::string& prefix = "", const std::string& postfix ="");
    std::string prefix_line(size_t line_number);
    std::string prefix_archive_file();
    std::string postfix_database();
    static std::string postfix_system();
};


#endif // OUTPUT_H
