/*
  db_init.c -- SQL statements to initialize mamedb
  Copyright (C) 2007-2014 Dieter Baron and Thomas Klausner

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


#include "DB.h"

const char *sql_db_init[] = {
    /* DBH_FMT_MAME	-- mame.db format */
    "\
create table dat (\n\
	dat_idx integer primary key,\n\
	name text,\n\
	description text,\n\
	author text,\n\
	version text\n\
);\n\
\n\
create table game (\n\
	game_id integer primary key autoincrement,\n\
	name text not null,\n\
        parent text,\n\
	description text,\n\
	dat_idx integer not null\n\
);\n\
create index game_name on game (name);\n\
\n\
create table file (\n\
	game_id integer,\n\
	file_type integer,\n\
	file_idx integer,\n\
	name text not null,\n\
	merge text,\n\
	status integer not null,\n\
	location integer not null,\n\
	size integer,\n\
	crc integer,\n\
	md5 binary,\n\
	sha1 binary,\n\
	primary key (game_id, file_type, file_idx)\n\
);\n\
create index file_game_type on file (game_id, file_type);\n\
\n\
create table rule (\n\
	rule_idx integer primary key,\n\
	start_offset integer,\n\
	end_offset integer,	\n\
	operation integer\n\
);\n\
\n\
create table test (\n\
	rule_idx integer,\n\
	test_idx integer,\n\
	type integer not null,\n\
	offset integer,\n\
	size integer,\n\
	mask binary,\n\
	value binary,\n\
	result integer not null,\n\
	primary key (rule_idx, test_idx)\n\
);\n\
",

    /* DBH_FMT_MEM -- in-memory db format */
    "\
create table file (\n\
	archive_id integer,\n\
	file_type integer,\n\
	file_idx integer,\n\
	detector_id integer,\n\
	location integer not null,\n\
	size integer,\n\
	crc integer,\n\
	md5 binary,\n\
	sha1 binary\n\
);\n\
create index file_id on file (archive_id, file_type, file_idx);\n\
create index file_location on file (location);\n\
create index file_size on file (size);\n\
create index file_crc on file (crc);\n\
create index file_md5 on file (md5);\n\
create index file_sha1 on file (sha1);\n\
",

    /* DBH_FMT_DIR -- unpacked dirs db format */
    "create table archive (\n\
	archive_id integer primary key autoincrement,\n\
	name text not null,\n\
        mtime integer not null,\n\
	size integer not null,\n\
        file_type integer not null\n\
);\n\
create index archive_name on archive (name);\n\
create table detector (\n\
    detector_id integer primary key autoincrement,\n\
    name text not null,\n\
    version text not null\n\
);\n\
create index detector_name_version on detector (name, version);\n\
create table file (\n\
	archive_id integer not null,\n\
	file_idx integer,\n\
	name text not null,\n\
        mtime integer not null,\n\
	status integer not null,\n\
	size integer not null,\n\
	crc integer,\n\
	md5 binary,\n\
	sha1 binary,\n\
        detector_id integer not null default 0\n\
);\n\
create index file_archive_id on file (archive_id);\n\
create index file_idx on file (file_idx);\n\
create index file_size on file (size);\n\
create index file_crc on file (crc);\n\
create index file_md5 on file (md5);\n\
create index file_sha1 on file (sha1);\n"};


const char *sql_db_init_2 = "\
create index file_name on file (name);\n\
create index file_size on file (size);\n\
create index file_crc on file (crc);\n\
create index file_md5 on file (md5);\n\
create index file_sha1 on file (sha1);\n\
";
