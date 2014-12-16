package CkmameDB;

use strict;
use warnings;

sub new {
	my $class = UNIVERSAL::isa ($_[0], __PACKAGE__) ? shift : __PACKAGE__;
	my $self = bless {}, $class;

	my ($dir, $skip) = @_;
	
	$self->{dir} = $dir;
	if ($skip) {
		$self->{skip} = { map { $_ => 1} @$skip };
	}
	else {
		$self->{skip} = {};
	}
	
	$self->{dump_got} = [];
	$self->{archive_id} = {};
	$self->{max_id} = 0;
	$self->{archives_got} = {};
	
	return $self;
}

sub read_db {
	my ($self) = @_;
	
	if (! -f "$self->{dir}/.ckmame.db") {
		$self->{dump_got} = [ '>>> table archive (archive_id, name, mtime, size)', '>>> table file (archive_id, file_idx, name, mtime, status, size, crc, md5, sha1)' ];
		return 1;
	}
	
	my $dump;
	unless (open $dump, "../dbdump $self->{dir}/.ckmame.db |") {
		print "dbdump in $self->{dir}/.ckmame.db failed: $!\n" if ($self->{verbose});
		return undef;
	}

	my @files = ();

	my $table = '';
	while (my $line = <$dump>) {
		chomp $line;
		
		if ($table eq 'file') {
			my ($id, $name) = split '\|', $line;
			push @files, [ $id, $name, $line ];
			next;
		}
		push @{$self->{dump_got}}, $line;
		if ($line =~ m/>>> table (\w+)/) {
			$table = $1;
			next;
		}
		if ($table eq 'archive') {
			my ($id, $name) = split '\|', $line;
			$self->{archive_id}->{$name} = $id; 
			if ($id > $self->{max_id}) {
				$self->{max_id} = $id;
			}
		}
	}
	close($dump);

	for my $file (sort { return $a->[1] cmp $b->[1] if ($a->[0] == $b->[0]) ; return $a->[0] <=> $b->[0]; } @files) {
		push @{$self->{dump_got}}, $file->[2];
	}
	
	return 1;
}


sub read_archives {
	my ($self) = @_;
	
	my $dat;
	unless (open $dat, "../../src/mkmamedb --no-directory-cache -F cm -u -o /dev/stdout $self->{dir} 2>/dev/null | ") {
		print "mkmamedb using $self->{dir} failed: $!\n" if ($self->{verbose});
		return undef;
	}

	my $archive;
	my $in_game = 0;
	while (my $line = <$dat>) {
		chomp $line;
		if ($line =~ m/^game \(/) {
			$in_game = 1;
			$archive = { files => {} };
		}
		elsif (!$in_game) {
			next;
		}
		if ($line =~ m/^\)/) {
			$in_game = 0;
			next;
		}
		
		if ($line =~ m/^\s*name (.*)/) {
			if ($self->{skip}->{$1}) {
				undef $archive;
				next;
			}
			
			$archive->{name} = $1;
			if ($self->{archive_id}->{$archive->{name}}) {
				$archive->{id} = $self->{archive_id}->{$archive->{name}};
			}
			else {
				$archive->{id} = ++$self->{max_id};
			}
			$self->{archives_got}->{$archive->{id}} = $archive;
		}
		elsif ($line =~ m/rom \( (.*) \)/) {
			next unless ($archive);

			my $rom = { split ' ', $1 };
			
			$rom->{mtime} = (stat("$self->{dir}/$archive->{name}/$rom->{name}"))[9];
			$rom->{crc} = hex($rom->{crc});
			
			$archive->{files}->{$rom->{name}} = $rom;
		}
	}

	close($dat);
	
	return 1;
}

sub make_dump {
	my ($self) = @_;
	
	my @dump = ();
	
	push @dump, '>>> table archive (archive_id, name, mtime, size)';
	
	for my $id (sort { $a <=> $b } keys %{$self->{archives_got}}) {
		push @dump, "$id|$self->{archives_got}->{$id}->{name}|0|0";
	}
	push @dump, '>>> table file (archive_id, file_idx, name, mtime, status, size, crc, md5, sha1)';
	my $idx = 0;
	for my $id (sort { $a <=> $b} keys %{$self->{archives_got}}) {
		my $archive = $self->{archives_got}->{$id};
		
		for my $fname (sort keys %{$archive->{files}}) {
			my $file = $archive->{files}->{$fname};
			push @dump, join '|', $id, $idx++, $fname, $file->{mtime}, 0, $file->{size}, $file->{crc}, "<$file->{md5}>", "<$file->{sha1}>";
		}
	}
	
	$self->{dump_expected} = \@dump;
	
	return 1;
}

1;
