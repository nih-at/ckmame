/*
  $NiH$

  db_init.c -- SQL statements to initialize mamedb
  Copyright (C) 2007 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <ckmame@nih.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#include "dbh.h"

const char *sql_db_init = "\
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
	description text,\n\
	dat_idx integer not null\n\
);\n\
create index game_name on game (name);\n\
\n\
create table parent (\n\
	game_id integer,\n\
	file_type integer,\n\
	parent integer not null,\n\
	primary key (game_id, file_type)\n\
);\n\
create index parnet_parent on parent (parent);\n\
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
	md5 binray,\n\
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
";

const char *sql_db_init_2 = "\
create index file_name on file (name);\n\
create index file_size on file (size);\n\
create index file_crc on file (crc);\n\
create index file_md5 on file (md5);\n\
create index file_sha1 on file (sha1);\n\
";
