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

#include <format>
#include <string>
#include <system_error>

#include "DB.h"

class Output {
  public:
    class FileInfo {
      public:
        FileInfo(std::string archive_name, std::string file_name)
            : archive_name(std::move(archive_name)), file_name(std::move(file_name)) {}
        FileInfo(std::string file_name) : archive_name(), file_name(std::move(file_name)) {}
        FileInfo() = default;

        std::string archive_name;
        std::string file_name;

        std::string full_name() const {
            if (archive_name.empty()) {
                return file_name;
            }
            else {
                return archive_name + "/" + file_name;
            }
        }
    };

    Output();

    /**
     * Set the current header.
     *
     * @param header The new header.
     */
    void set_header(std::string header);

    /**
     * Set the current subheader.
     *
     * @param subheader The new subheader.
     */
    void set_subheader(std::string subheader);

    /**
     * Print a message to the output.
     *
     * @param string The message to print.
     */
    void message(std::string_view string) { print_message(string); }

    /**
     * Print a formatted message to the output.
     *
     * @tparam Args The types of the arguments.
     * @param format The format string.
     * @param args The arguments to format.
     */
    template <typename... Args> void message(std::format_string<Args...> format, Args&&... args) {
        print_message(std::format(format, std::forward<Args>(args)...));
    }

    /**
     * Print a message to the output if verbose mode is enabled.
     *
     * @param string The message to print.
     */
    void message_verbose(std::string_view string) { print_message_verbose(string); }

    /**
     * Print a formatted message to the output if verbose mode is enabled.
     *
     * @tparam Args The types of the arguments.
     * @param format The format string.
     * @param args The arguments to format.
     */
    template <typename... Args> void message_verbose(std::format_string<Args...> format, Args&&... args) {
        print_message_verbose(std::format(format, std::forward<Args>(args)...));
    }

    /**
     * Push a new archive onto the error information stack.
     *
     * @param archive_name The name of the archive.
     * @param file_name The name of the file (optional).
     */
    void push_error_archive(std::string archive_name, std::string file_name = "");

    /**
     * Push a new file onto the error information stack.
     *
     * @param file_name The name of the file.
     */
    void push_error_file(std::string file_name);

    /**
     * Push a new file info onto the error information stack.
     *
     * @param info The file info to push.
     */
    void push_error_info(FileInfo info);

    /**
     * Pop the top file info from the error information stack.
     */
    void pop_error_file_info();

    /**
     * Set the archive for error messages, replacing the topmost file info, if already set.
     *
     * @param archive_name The name of the archive.
     * @param file_name The name of the file (optional).
     */
    void set_error_archive(std::string archive_name, std::string file_name = "");

    /**
     * Set the database for error messages.
     *
     * @param db The database to set.
     */
    void set_error_database(DB* db);

    /**
     * Set the file for error messages, replacing the topmost file info, if already set.
     *
     * @param file_name The name of the file.
     */
    void set_error_file(std::string file_name);

    /**
     * Print an error message to the output.
     *
     * @param string The error message to print.
     */
    void error(std::string_view string) { print_error(string); }

    /**
     * Print a formatted error message.
     *
     * @tparam Args The types of the arguments.
     * @param format The format string.
     * @param args The arguments to format.
     */
    template <typename... Args> void error(std::format_string<Args...> format, Args&&... args) {
        print_error(std::format(format, std::forward<Args>(args)...));
    }

    /**
     * Print a formatted error message with database error information.
     *
     * @tparam Args The types of the arguments.
     * @param format The format string.
     * @param args The arguments to format.
     */
    template <typename... Args> void error_database(std::format_string<Args...> format, Args&&... args) {
        print_error(std::format(format, std::forward<Args>(args)...), {}, postfix_database());
    }

    /**
     * Print a formatted error message with system error information.
     *
     * @tparam Args The types of the arguments.
     * @param format The format string.
     * @param args The arguments to format.
     */
    template <typename... Args> void error_system(std::format_string<Args...> format, Args&&... args) {
        print_error(std::format(format, std::forward<Args>(args)...), {}, postfix_system());
    }

    /**
     * Print a formatted error message with a specific error code.
     *
     * @tparam Args The types of the arguments.
     * @param ec The error code.
     * @param format The format string.
     * @param args The arguments to format.
     */
    template <typename... Args>
    void error_error_code(const std::error_code& ec, std::format_string<Args...> format, Args&&... args) {
        print_error(std::format(format, std::forward<Args>(args)...), {}, ec.message());
    }


    /**
     * Print a formatted error message relating to an archive.
     *
     * @tparam Args The types of the arguments.
     * @param format The format string.
     * @param args The arguments to format.
     */
    template <typename... Args> void archive_error(std::format_string<Args...> format, Args&&... args) {
        print_error(std::format(format, std::forward<Args>(args)...), prefix_archive());
    }

    /**
     * Print a formatted error message relating to an archive with database error information.
     *
     * @tparam Args The types of the arguments.
     * @param format The format string.
     * @param args The arguments to format.
     */
    template <typename... Args> void archive_error_database(std::format_string<Args...> format, Args&&... args) {
        print_error(std::format(format, std::forward<Args>(args)...), prefix_archive(), postfix_database());
    }

    /**
     * Print a formatted error message relating to an archive with system error information.
     *
     * @tparam Args The types of the arguments.
     * @param format The format string.
     * @param args The arguments to format.
     */
    template <typename... Args> void archive_error_system(std::format_string<Args...> format, Args&&... args) {
        print_error(std::format(format, std::forward<Args>(args)...), prefix_archive(), postfix_system());
    }

    /**
     * Print a formatted error message relating to an archive with a specific error code.
     *
     * @tparam Args The types of the arguments.
     * @param ec The error code.
     * @param format The format string.
     * @param args The arguments to format.
     */
    template <typename... Args>
    void archive_error_error_code(const std::error_code& ec, std::format_string<Args...> format, Args&&... args) {
        print_error(std::format(format, std::forward<Args>(args)...), prefix_archive(), ec.message());
    }

    /**
     * Print a formatted error message relating to a file inside an archive.
     *
     * @tparam Args The types of the arguments.
     * @param format The format string.
     * @param args The arguments to format.
     */
    template <typename... Args> void archive_file_error(std::format_string<Args...> format, Args&&... args) {
        print_error(std::format(format, std::forward<Args>(args)...), prefix_archive_file());
    }

    /**
     * Print a formatted error message relating to a file inside an archive with database error information.
     *
     * @tparam Args The types of the arguments.
     * @param format The format string.
     * @param args The arguments to format.
     */
    template <typename... Args> void archive_file_error_database(std::format_string<Args...> format, Args&&... args) {
        print_error(std::format(format, std::forward<Args>(args)...), prefix_archive_file(), postfix_database());
    }

    /**
     * Print a formatted error message relating to a file inside an archive with system error information.
     *
     * @tparam Args The types of the arguments.
     * @param format The format string.
     * @param args The arguments to format.
     */
    template <typename... Args> void archive_file_error_system(std::format_string<Args...> format, Args&&... args) {
        print_error(std::format(format, std::forward<Args>(args)...), prefix_archive_file(), postfix_system());
    }

    /**
     * Print a formatted error message relating to a file inside an archive with a specific error code.
     *
     * @tparam Args The types of the arguments.
     * @param ec The error code.
     * @param format The format string.
     * @param args The arguments to format.
     */
    template <typename... Args>
    void archive_file_error_error_code(const std::error_code& ec, std::format_string<Args...> format, Args&&... args) {
        print_error(std::format(format, std::forward<Args>(args)...), prefix_archive_file(), ec.message());
    }

    /**
     * Print a formatted error message relating to a file.
     *
     * @tparam Args The types of the arguments.
     * @param format The format string.
     * @param args The arguments to format.
     */
    template <typename... Args> void file_error(std::format_string<Args...> format, Args&&... args) {
        print_error(std::format(format, std::forward<Args>(args)...), prefix_file());
    }

    /**
     * Print a formatted error message relating to a file with database error information.
     *
     * @tparam Args The types of the arguments.
     * @param format The format string.
     * @param args The arguments to format.
     */
    template <typename... Args> void file_error_database(std::format_string<Args...> format, Args&&... args) {
        print_error(std::format(format, std::forward<Args>(args)...), prefix_file(), postfix_database());
    }

    /**
     * Print a formatted error message relating to a file with system error information.
     *
     * @tparam Args The types of the arguments.
     * @param format The format string.
     * @param args The arguments to format.
     */
    template <typename... Args> void file_error_system(std::format_string<Args...> format, Args&&... args) {
        print_error(std::format(format, std::forward<Args>(args)...), prefix_file(), postfix_system());
    }

    /**
     * Print a formatted error message relating to a file with a specific error code.
     *
     * @tparam Args The types of the arguments.
     * @param ec The error code.
     * @param format The format string.
     * @param args The arguments to format.
     */
    template <typename... Args>
    void file_error_error_code(const std::error_code& ec, std::format_string<Args...> format, Args&&... args) {
        print_error(std::format(format, std::forward<Args>(args)...), prefix_file(), ec.message());
    }

    /**
     * Print a formatted error message relating to the specified file info.
     *
     * @tparam Args The types of the arguments.
     * @param file_info The file info.
     * @param format The format string.
     * @param args The arguments to format.
     */
    template <typename... Args>
    void file_info_error(const FileInfo& file_info, std::format_string<Args...> format, Args&&... args) {
        print_error(std::format(format, std::forward<Args>(args)...), file_info.full_name());
    }

    /**
     * Print a formatted error message relating to the specified file info with database error information.
     *
     * @tparam Args The types of the arguments.
     * @param file_info The file info.
     * @param format The format string.
     * @param args The arguments to format.
     */
    template <typename... Args>
    void file_info_error_database(const FileInfo& file_info, std::format_string<Args...> format, Args&&... args) {
        print_error(std::format(format, std::forward<Args>(args)...), file_info.full_name(), postfix_database());
    }

    /**
     * Print a formatted error message relating to the specified file info with system error information.
     *
     * @tparam Args The types of the arguments.
     * @param file_info The file info.
     * @param format The format string.
     * @param args The arguments to format.
     */
    template <typename... Args>
    void file_info_error_system(const FileInfo& file_info, std::format_string<Args...> format, Args&&... args) {
        print_error(std::format(format, std::forward<Args>(args)...), file_info.full_name(), postfix_system());
    }

    /**
     * Print a formatted error message relating to the specified file info with a specific error code.
     *
     * @tparam Args The types of the arguments.
     * @param file_info The file info.
     * @param ec The error code.
     * @param format The format string.
     * @param args The arguments to format.
     */
    template <typename... Args>
    void file_info_error_error_code(const FileInfo& file_info, const std::error_code& ec,
                                    std::format_string<Args...> format, Args&&... args) {
        print_error(std::format(format, std::forward<Args>(args)...), file_info.full_name(), ec.message());
    }

    /**
     * Print a formatted error message relating to a specific line in the current file.
     *
     * @tparam Args The types of the arguments.
     * @param line_number The line number.
     * @param format The format string.
     * @param args The arguments to format.
     */
    template <typename... Args>
    void line_error(size_t line_number, std::format_string<Args...> format, Args&&... args) {
        print_error(std::format(format, std::forward<Args>(args)...), prefix_line(line_number));
    }

    /**
     * Print a formatted error message relating to a specific line in the current file with database error information.
     *
     * @tparam Args The types of the arguments.
     * @param line_number The line number.
     * @param format The format string.
     * @param args The arguments to format.
     */
    template <typename... Args>
    void line_error_database(size_t line_number, std::format_string<Args...> format, Args&&... args) {
        print_error(std::format(format, std::forward<Args>(args)...), prefix_line(line_number), postfix_database());
    }

    /**
     * Print a formatted error message relating to a specific line in the current file with system error information.
     *
     * @tparam Args The types of the arguments.
     * @param line_number The line number.
     * @param format The format string.
     * @param args The arguments to format.
     */
    template <typename... Args>
    void line_error_system(size_t line_number, std::format_string<Args...> format, Args&&... args) {
        print_error(std::format(format, std::forward<Args>(args)...), prefix_line(line_number), postfix_system());
    }

    /**
     * Print a formatted error message relating to a specific line in the current file with a specific error code.
     *
     * @tparam Args The types of the arguments.
     * @param line_number The line number.
     * @param ec The error code.
     * @param format The format string.
     * @param args The arguments to format.
     */
    template <typename... Args>
    void line_error_error_code(size_t line_number, const std::error_code& ec, std::format_string<Args...> format,
                               Args&&... args) {
        print_error(std::format(format, std::forward<Args>(args)...), prefix_line(line_number), ec.message());
    }

  private:
    /**
     * Print a message to the output.
     *
     * @param string The message to print.
     */
    void print_message(std::string_view string);

    /**
     * Print a message to the output if verbose output is enabled.
     *
     * @param string The message to print.
     */
    void print_message_verbose(std::string_view string);

    /**
     * Print an error message to the output.
     *
     * prefix, message, and postfix are separated by ": " if they are not empty.
     *
     * @param string The error message to print.
     * @param prefix An optional prefix to prepend to the error message.
     * @param postfix An optional postfix to append to the error message.
     */
    void print_error(std::string_view string, std::string_view prefix = "", std::string_view postfix = "");

    /// @brief An empty string to return when no prefix is available.
    static std::string empty_string;

    /// @brief The header to print before the first message or when the header changed.
    std::string header;

    /// @brief The subheader to print before the first message or when the subheader changed.
    std::string subheader;

    /// @brief Whether this is the first header to print.
    bool first_header;

    /// @brief Whether the current header has been printed.
    bool header_done;

    /// @brief Whether the current subheader has been printed.
    bool subheader_done;

    /// @brief The stack of file infos to use for error messages.
    std::vector<FileInfo> file_infos;

    /// @brief The database to use for error messages.
    DB* db;

    /// @brief Print the current header and subheader if they have not been printed yet.
    void print_header();

    /**
     * Get the prefix for line_error functions.
     *
     * @param line_number The line number to use in the prefix.
     * @return The prefix to use.
     */
    std::string prefix_line(size_t line_number);

    /**
     * Get the prefix for archive_error functions.
     *
     * @return The prefix to use.
     */
    const std::string& prefix_archive();

    /**
     * Get the prefix for archive_file_error functions.
     *
     * @return The prefix to use.
     */
    std::string prefix_archive_file();

    /**
     * Get the prefix for file_error functions.
     *
     * @return The prefix to use.
     */
    const std::string& prefix_file();

    /**
     * Get the postfix for database errors.
     *
     * @return The postfix to use.
     */
    std::string postfix_database();
    /**
     * Get the postfix for system errors.
     *
     * @return The postfix to use.
     */
    static std::string postfix_system();
};


#endif // OUTPUT_H
