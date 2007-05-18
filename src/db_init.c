#include "dbh.h"

const char *sql_db_init = "\
create table dat (\n\
	dat_idx integer primary key,\n\
	name text,\n\
	description text,\n\
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
	mask binary,\n\
	value binary,\n\
	result integer not null,\n\
	primary key (rule_idx, test_idx)\n\
);\n\
\n\
insert into dat (dat_idx, name, version) values (-1, 'ckmame', '1');\n\
";

const char *sql_db_init_2 = "\
create index file_name on file (name);\n\
create index file_size on file (size);\n\
create index file_crc on file (crc);\n\
create index file_md5 on file (md5);\n\
create index file_sha1 on file (sha1);\n\
";
