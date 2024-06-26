#ifndef HAD_OUTPUT_H
#define HAD_OUTPUT_H

/*
  OutputContext.h -- output game info
  Copyright (C) 2006-2020 Dieter Baron and Thomas Klausner

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
#include <string>

#include "DatEntry.h"
#include "Detector.h"
#include "Game.h"
#include "Hashes.h"
#include "SharedFile.h"

class OutputContext;

typedef std::shared_ptr<OutputContext> OutputContextPtr;

#define OUTPUT_FL_RUNTEST  1

class OutputContext {
public:
    enum Format {
        FORMAT_CM,
        FORMAT_DATAFILE_XML,
        FORMAT_DB,
        FORMAT_MTREE
    };
    
    virtual ~OutputContext() = default;

    static OutputContextPtr create(Format format, const std::string &fname, int flags);

    virtual bool close() = 0;
    virtual bool detector(Detector *detector) { return true; }
    virtual bool game(GamePtr game, const std::string &original_name = "") = 0;
    virtual bool header(DatEntry *dat) { return true; }
    virtual void error_occurred() { }

protected:
    void cond_print_string(FILEPtr f, const std::string &pre, const std::string &str, const std::string &post);
    void cond_print_hash(FILEPtr f, const std::string &pre, int t, const Hashes *h, const std::string &post);
};

#endif // HAD_OUTPUT_H
