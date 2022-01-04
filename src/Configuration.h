#ifndef HAD_CONFIGURATION_H
#define HAD_CONFIGURATION_H

/*
  Configuration.h -- configuration settings from file
  Copyright (C) 2021 Dieter Baron and Thomas Klausner

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
#include <vector>

class Configuration {
public:
    Configuration();

    /*
     fixdat none/auto/filename
     Complete-only yes/no
     Dbfile filename
     Search-dir directories
     Unknown ignore/move/delete
     Long move/delete
     Old-db filename
     Rom-db filename
     Rom-dir directory
     Stats yes/no
     Duplicate keep/delete (in old db)
     zipped yes/no
     Superfluous keep/delete
     Extra-used keep/delete
     Extra-unused keep/delete

     */
    
    std::string romdb_name;
    std::string olddb_name;
    
    std::string rom_directory;
    std::vector<std::string> extra_directories;
    
    std::string fixdat;
    
    bool roms_zipped;

    // not in config files, per invocation
    bool fix_romset; // actually fix, otherwise no archive is changed
    bool keep_old_duplicate;

    // output
    bool verbose; // print all actions taken to fix ROM set

    // options
    bool complete_games_only; // only add ROMs to games if they are complete afterwards.
    bool move_from_extra; // remove files taken from extra directories, otherwise copy them and don't change extra directory.

//    bool fix_move_long; - always on
//    bool fix_move_unknown; - always on
//    bool fix_superfluous; - always on
//    bool fix_ignore_unknown; - always off
//    bool fix_delete_duplicate; - always on
    
    /*
     in ROM set:
        everything, including no good dump
        everything, excluding no good dump
        (everything that can be fixed with existing files)
     
        ROM states:
            missing - per option, default on (warn_missing)
                no good dump exists - per option, default off (warn_no_good_dump)
            fixable - per option, default on
            correct - per option, default off (warn_correct)

        file states:
            broken - always
            fixable (name, long, used elsewhere, unused) - per option
            unknown - per option (warn_unknown)
            correct - never

    in extra dir:
        (unused file)
        (unknown file)
     
     */

    bool report_correct; /* report ROMs that are correct */
    bool report_detailed; /* one line for each ROM */
    bool report_fixable; /* report ROMs that are not correct but can be fixed */
    bool report_missing; /* report missing ROMs with good dumps, one line per game if no own ROM found */
    bool report_summary; /* print statistics about ROM set at end of run */

    /* file_correct */
    bool warn_file_known;   // files that are known but don't belong in this archive
    bool warn_file_unknown; // files that are not in ROM set
    
/*    bool warn_extra_used; */
};

#endif // HAD_CONFIGURATION_H

