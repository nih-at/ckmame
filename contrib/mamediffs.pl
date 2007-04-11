#!/usr/pkg/bin/perl

# TODO:
# . less hardwired paths
# . handle spaces in file names better
# . handle CHDs

$zipname="mamediffs";
$linkdir="/archive/roms/dillo";
$rompath="/archive/roms/mame/roms/";

sub do_game_copy {
    my $gamename = shift;
    if (@_) {
	system("unzip","-Cd",$gamename,"$rompath/$gamename.zip",@_);
	system("zip","-9rm",$zipname,$gamename);
    }
}

while (<>) {
    if (/^In game (.*):/) {
	if ($gamename) {
	    do_game_copy($gamename, @files);
	    @files=();
	}
	$gamename = $1;
    }
    elsif (/^rom\s+(.*)\s+size/ and not /no good dump: exists/
	   and not /best bad dump/) {
	push @files, $1;
    }
    elsif (/^game\s+([^\s]*)/) {
	if ( ! -f "$rompath/$gamename.zip") {
	    print("$gamename not found\n");
	    next;
	}
	if ( ! -d $linkdir ) {
	    mkdir($linkdir, 0755);
	}
	symlink("$rompath/$gamename.zip", "$linkdir/$gamename.zip");
    }
}
do_game_copy($gamename, @files);
