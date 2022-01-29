#ifndef HAD_DETECTOR_H
#define HAD_DETECTOR_H

/*
  detector.h -- clrmamepro XML header skip detector
  Copyright (C) 2007-2021 Dieter Baron and Thomas Klausner

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

#include <memory>
#include <vector>

#include "DetectorCollection.h"
#include "File.h"
#include "Hashes.h"
#include "ParserSource.h"
#include "zip_util.h"

#define DETECTOR_OFFSET_EOF 0
#define DETECTOR_SIZE_POWER_OF_2 (-1)

class Detector;

typedef std::shared_ptr<Detector> DetectorPtr;

typedef int64_t (*detector_read_cb)(void *, void *, uint64_t);


class Detector {
public:
    enum Operation {
        OP_NONE,
        OP_BITSWAP,
        OP_BYTESWAP,
        OP_WORDSWAP
    };
    
    enum TestType {
        TEST_DATA,
        TEST_OR,
        TEST_AND,
        TEST_XOR,
        TEST_FILE_EQ,
        TEST_FILE_LE,
        TEST_FILE_GR
    };
    
    class Test {
    public:
        Test() : type(TEST_DATA), offset(0), length(0), result(true) { }
        
        TestType type;
        int64_t offset;
        uint64_t length;
        std::vector<uint8_t> mask;
        std::vector<uint8_t> value;
        bool result;

        bool execute(const std::vector<uint8_t> &data) const;
        void print(FILE *fout) const;
        
    private:
        bool bit_cmp(const uint8_t *data) const;
    };

    class Rule {
    public:
        Rule() : start_offset(0), end_offset(DETECTOR_OFFSET_EOF), operation(OP_NONE) { }
        
        int64_t start_offset;
        int64_t end_offset;
        Operation operation;
        std::vector<Test> tests;
        
        Hashes execute(const std::vector<uint8_t> &data) const;
        void print(FILE *fout) const;
        
    private:
        Hashes compute_values(Operation operation, const std::vector<uint8_t> &data, uint64_t start, uint64_t length) const;
    };
    

    std::string name;
    std::string author;
    std::string version;
    size_t id;
    
    std::vector<Rule> rules;
    
    static const uint64_t MAX_DETECTOR_FILE_SIZE;

    static DetectorPtr parse(const std::string &filename);
    static DetectorPtr parse(ParserSource *parser_source);

    Hashes execute(const std::vector<uint8_t> &data) const;
    bool print(FILE *) const;

    static std::string file_test_type_name(TestType type);
    static std::string operation_name(Operation operation);
    static std::string test_type_name(TestType type);

    static size_t get_id(const DetectorDescriptor &descriptor) { return detector_ids.get_id(descriptor); }
    static const DetectorDescriptor *get_descriptor(size_t id) { return detector_ids.get_descriptor(id); }
    
    // Returns true if new hashes were computed.
    static bool compute_hashes(const std::vector<uint8_t> &data, File *file, const std::unordered_map<size_t, DetectorPtr> &detectors);

private:
    static uint64_t operation_unit_size(Operation operation);
    static DetectorCollection detector_ids;
};

#endif // HAD_DETECTOR_H
