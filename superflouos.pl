#!/usr/bin/perl

use DB_File;

tie %games, 'DB_File', 'mame.db', O_RDONLY, 0666, $DB_HASH;

$_ = `ls roms`;
@files = split /\n/;

foreach (@files) {
    $a = $_;
    s/\.zip$//;
    print "$a\n" unless exists $games{$_};
}
