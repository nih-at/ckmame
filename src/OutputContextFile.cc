#include "OutputContextFile.h"

#include <iostream>

#include "Exception.h"
#include "format.h"
#include "globals.h"

OutputContextFile::OutputContextFile(const std::optional<std::filesystem::path>& file_name, bool binary)
    : file_name(file_name), stream(is_stdout() ? std::cout : file_stream) {
    if (!is_stdout()) {
        auto mode = std::ios::out | std::ios::trunc;
        if (binary) {
            mode |= std::ios::binary;
        }
        file_stream.open(*file_name, mode);
        if (!file_stream.is_open()) {
            throw Exception("cannot create '{}': {}", *file_name, strerror(errno));
        }
    }
}

bool OutputContextFile::close() {
    if (is_stdout()) {
        return true;
    }
    if (file_stream.is_open()) {
        auto ok = file_stream.good();
        file_stream.close();
        return ok;
    }
    return true;
}

OutputContextFile::~OutputContextFile() { close(); }

void OutputContextFile::cond_print_string(const std::string& pre, const std::string& str, const std::string& post) {
    if (str.empty()) {
        return;
    }

    std::string out;
    if (str.find_first_of(" \t") == std::string::npos) {
        stream << pre << str << post;
    }
    else {
        stream << pre << "\"" << str << "\"" << post;
    }
}


void OutputContextFile::cond_print_hash(const std::string& pre, int t, const Hashes* h, const std::string& post) {
    cond_print_string(pre, h->to_string(t), post);
}
