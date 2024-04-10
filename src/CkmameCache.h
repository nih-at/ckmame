#ifndef CKMAME_CKMAME_CACHE_H
#define CKMAME_CKMAME_CACHE_H

/*
CkMameCache.h -- collection of CkmameDBs.
Copyright (C) 1999-2022 Dieter Baron and Thomas Klausner

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

#include <unordered_set>

#include "CkmameDB.h"
#include "DeleteList.h"
#include "Stats.h"

class CkmameCache {
  public:
    CkmameCache();
    ~CkmameCache() { close_all(); }

    void ensure_extra_maps();
    void ensure_needed_maps();

    CkmameDBPtr get_db_for_archive(const std::string &name);
    std::string get_directory_name_for_archive(const std::string &name);
    void register_directory(const std::string &directory, where_t where);

    std::vector<CkmameDB::FindResult> find_file(filetype_t filetype, size_t detector_id, const FileData& rom);
    bool compute_all_detector_hashes(bool needed_only, const std::unordered_map<size_t, DetectorPtr>& detectors);

    void used(Archive *a, size_t idx);

    DeleteListPtr extra_delete_list;
    DeleteListPtr needed_delete_list;
    DeleteListPtr superfluous_delete_list;

    std::unordered_set<std::string> complete_games;

    Stats stats;

  private:
    class CacheDirectory {
      public:
	std::string name;
        where_t where;
	std::shared_ptr<CkmameDB> db;
	bool initialized = false;

	explicit CacheDirectory(std::string name_, where_t where): name(std::move(name_)), where(where) { }

        void initialize();
    };

    bool close_all();

    std::vector<CacheDirectory> cache_directories;

    bool extra_map_done;
    bool needed_map_done;

    bool enter_dir_in_map_and_list(const DeleteListPtr &list, const std::string &directory_name, where_t where);
    static bool enter_dir_in_map_and_list_unzipped(const DeleteListPtr &list, const std::string &directory_name, where_t where);
    static bool enter_dir_in_map_and_list_zipped(const DeleteListPtr &list, const std::string &dir_name, where_t where);
    static bool enter_file_in_map_and_list(const DeleteListPtr &list, const std::string &name, where_t where);

    const CacheDirectory* get_directory_for_archive(const std::string &name);
};

typedef std::shared_ptr<CkmameCache> CkmameCachePtr;

extern CkmameCachePtr ckmame_cache;

#endif // CKMAME_CKMAME_CACHE_H
