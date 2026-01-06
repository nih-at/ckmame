#ifndef HAD_DETECTOR_COLLECTION_H
#define HAD_DETECTOR_COLLECTION_H

/*
  DetectorCollection.h -- clrmamepro XML header skip detector
  Copyright (C) 2007-2024 Dieter Baron and Thomas Klausner

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

#include <string>
#include <unordered_map>

class Detector;

class DetectorDescriptor {
public:
    DetectorDescriptor() = default;
    DetectorDescriptor(const std::string &name_, const std::string &version_) : name(name_), version(version_) { }
    explicit DetectorDescriptor(const Detector *detector);
    
    std::string name;
    std::string version;
    
    bool operator==(const DetectorDescriptor &other) const { return name == other.name && version == other.version; }
};

namespace std {
template <>
struct hash<DetectorDescriptor> {
    std::size_t operator()(const DetectorDescriptor &k) const {
        return std::hash<std::string>()(k.name) ^ std::hash<std::string>()(k.version);
    }
};
}


class DetectorCollection {
public:
    DetectorCollection() : next_id(1) { }
    
    const DetectorDescriptor *get_descriptor(size_t id) const;
    size_t get_id(const DetectorDescriptor &descriptor);
    void add(const DetectorDescriptor &descriptor, size_t id);
    bool known(const DetectorDescriptor &descriptor) const;
    
private:
    size_t next_id;
    std::unordered_map<DetectorDescriptor, size_t> detectors;
    std::unordered_map<size_t, DetectorDescriptor> ids;
};

#endif // HAD_DETECTOR_COLLECTION_H
