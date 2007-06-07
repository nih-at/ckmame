create table dat (
	dat_idx integer primary key,
	name text not null,
	description text,
	version text
);

create table game (
	game_id integer primary key autoincrement,
	name text not null,
	description text,
	dat_idx integer not null,
);
create index game_name on game (name);

create table parent (
	game_id integer,
	file_type integer,
	parent integer not null,
	primary key (game_id, file_type)
);
create index parnet_parent on parent (parent);

create table file (
	game_id integer,
	file_type integer,
	file_idx integer,
	name text not null,
	merge text,
	status integer not null,
	location integer not null,
	size integer,
	crc binary,
	md5 binray,
	sha1 binary,
	primary key (game_id, file_type, file_idx)
);
create index file_game_type on file (game_id, file_type);
create index file_name on file (name);
create index file_size on file (size);
create index file_crc on file (crc);
create index file_md5 on file (md5);
create index file_sha1 on file (sha1);

create table rule (
	rule_idx integer primary key,
	start_offset integer,
	end_offset integer,	
	operation integer
);

create table test (
	rule_idx integer,
	test_idx integer,
	type integer not null,
	offset integer,
	mask binary,
	value binary,
	result integer not null,
	primary key (rule_idx, test_idx)
);
