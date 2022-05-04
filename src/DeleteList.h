#ifndef HAD_DELETE_LIST_H
#define HAD_DELETE_LIST_H

/*
  delete_list.h -- list of files to delete
  Copyright (C) 2005-2021 Dieter Baron and Thomas Klausner

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

#include "Archive.h"
#include "ArchiveLocation.h"
#include "FileLocation.h"

class DeleteList;
typedef std::shared_ptr<DeleteList> DeleteListPtr;

class DeleteList {
 public:
    class Mark {
    public:
        explicit Mark(const DeleteListPtr& list = DeleteListPtr());
        ~Mark();

        void commit() { rollback = false; }

    private:
        std::weak_ptr<DeleteList> list;
        size_t index;
        bool rollback;
    };
    
    std::vector<ArchiveLocation> archives;
    std::vector<FileLocation> entries;

    DeleteList() = default;;
    
    void add(const Archive *a) { archives.emplace_back(a); }
    void add_directory(const std::string &directory, bool omit_known);
    int execute();
    void remove_archive(Archive *archive);
    void sort_archives();
    void sort_entries();

private:
    static bool close_archive(Archive *archive);
    void list_non_chds(const std::string &directory);
};


#endif // HAD_DELETE_LIST_H
