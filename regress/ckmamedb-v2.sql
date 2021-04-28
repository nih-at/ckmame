create table archive (
        archive_id integer primary key autoincrement,
        name text not null,
        mtime integer not null,
        size integer not null
);
create index archive_name on archive (name);
create table file (
        archive_id integer not null,
        file_idx integer,
        name text not null,
        mtime integer not null,
        status integer not null,
        size integer not null,
        crc integer,
        md5 binary,
        sha1 binary
);
create index file_archive_id on file (archive_id);
create index file_idx on file (file_idx);
