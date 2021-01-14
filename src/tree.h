#ifndef HAD_TREE_H
#define HAD_TREE_H

/*
  tree.h -- traverse tree of games to check
  Copyright (C) 1999-2021 Dieter Baron and Thomas Klausner

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

#include <map>

#include "archive.h"
#include "hashes.h"
#include "images.h"
#include "types.h"

class Tree;

typedef std::shared_ptr<Tree> TreePtr;

class Tree {
public:
    Tree() : check(false), checked(false) { }
    Tree(const std::string &name_, bool check_) : name(name_), check(check_), checked(false) { }
    
    std::string name;
    bool check;
    bool checked;
    
    std::map<std::string, TreePtr> children;
    
    bool add(const std::string &game_name);
    bool recheck(const std::string &game_name);
    bool recheck_games_needing(uint64_t size, const Hashes *hashes);
    void traverse();
    
private:
    Tree *add_node(const std::string &game_name, bool check);
    void traverse_internal(ArchivePtr *ancestor_archives, ImagesPtr *ancestor_images);
    void process(ArchivePtr *archives, ImagesPtr *images);
};

#endif /* tree.h */
