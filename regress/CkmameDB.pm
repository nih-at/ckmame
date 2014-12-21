package CkmameDB;

use strict;
use warnings;

my %status = (baddump => 1, nodump => 2);

sub new {
	my $class = UNIVERSAL::isa ($_[0], __PACKAGE__) ? shift : __PACKAGE__;
	my $self = bless {}, $class;

	my ($dir, $skip, $unzipped, $no_hashes) = @_;
	
	$self->{dir} = $dir;
	$self->{unzipped} = $unzipped;
	$self->{no_hashes} = {};

	if (defined($no_hashes)) {
		for my $no_hash (@$no_hashes) {
			next unless ($no_hash->[0] eq $dir);

			if (scalar(@$no_hash) == 2) {
				$self->{no_hashes}->{$no_hash->[1]} = 1;
			}
			else {
				$self->{no_hashes}->{$no_hash->[1]}->{$no_hash->[2]} = $no_hash->[3] // 1;
			}
		}
	}

	if ($skip) {
		$self->{skip} = { map { $_ => 1} @$skip };
	}
	else {
		$self->{skip} = {};
	}
	
	$self->{dump_got} = [];
	$self->{dump_archives} = {};
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

	my %archive_names;
	my @files = ();

	my $table = '';
	while (my $line = <$dump>) {
		chomp $line;
		
		if ($table eq 'file') {
			my ($id, $idx, $name) = split '\|', $line;
			push @files, [ $id, $idx, $line ];
			if (exists($archive_names{$id})) {
				my $dump_archive = $self->{dump_archives}->{$archive_names{$id}};

				$dump_archive->{files}->{$name} = $idx;
				if ($idx > $dump_archive->{max_idx}) {
					$dump_archive->{max_idx} = $idx;
				}
			}
			next;
		}
		push @{$self->{dump_got}}, $line;
		if ($line =~ m/>>> table (\w+)/) {
			$table = $1;
			next;
		}
		if ($table eq 'archive') {
			my ($id, $name) = split '\|', $line;
			$archive_names{$id} = $name;
			$self->{dump_archives}->{$name} = { id => $id, files => {}, max_idx => 0 };
			if ($id > $self->{max_id}) {
				$self->{max_id} = $id;
			}
		}
	}
	close($dump);

	for my $file (sort { return $a->[1] <=> $b->[1] if ($a->[0] == $b->[0]) ; return $a->[0] <=> $b->[0]; } @files) {
		push @{$self->{dump_got}}, $file->[2];
	}
	
	return 1;
}


sub read_archives {
	my ($self) = @_;
	
	my $dat;
	my $opt = ($self->{unzipped} ? '-u' : '');
	unless (open $dat, "../../src/mkmamedb --no-directory-cache -F mtree --extended $opt -o /dev/stdout $self->{dir} 2>/dev/null | ") {
		print "mkmamedb using $self->{dir} failed: $!\n" if ($self->{verbose});
		return undef;
	}

	my $archive;
	my $prefix;
	my $idx;
	while (my $line = <$dat>) {
		chomp $line;

		unless ($line =~ m/^(\S+) (.*)/) {
			print "can't parse mtree line '$line'\n" if ($self->{verbose});
			return undef;
		}

		my $name = $1;
		my @args = split ' ', $2;
		$name =~ s,^\./,,;
		$name = destrsvis($name);
		my %attributes = ();
		for my $attr (@args) {
			unless ($attr =~ m/^([^=]+)=(.*)/) {
				print "can't parse mtree line '$line'\n" if ($self->{verbose});
				return undef;
			}
			$attributes{$1} = $2;
		}

		next if ($name eq '.');

		if ($attributes{type} eq 'dir') {
			if ($self->{skip}->{$name}) {
				undef $archive;
				next;
			}

			$prefix = $name;
			$archive = { name => $name, files => [] };
			$idx = 0;

			if ($self->{unzipped}) {
				$archive->{mtime} = 0;
				$archive->{size} = 0;
			}
			else {
				$archive->{name} .= '.zip';
				my @stat = stat("$self->{dir}/$archive->{name}");
				$archive->{mtime} = $stat[9];
				$archive->{size} = $stat[7];
			}
			if ($self->{dump_archives}->{$archive->{name}}) {
				$archive->{id} = $self->{dump_archives}->{$archive->{name}}->{id};
			}
			else {
				$archive->{id} = ++$self->{max_id};
			}
			$self->{archives_got}->{$archive->{id}} = $archive;
		}
		elsif ($attributes{type} eq 'file') {
			next unless ($archive);

			$name =~ s,^$prefix/,,;

			my $rom = { name => $name, idx => $idx++, status => 0 };
			for my $attr (qw(size sha1 md5)) {
				$rom->{$attr} = $attributes{$attr} // 'null';
			}
			if (exists($attributes{status})) {
				if (!exists($status{$attributes{status}})) {
					print "unknown file status '$attributes{status}'\n" if ($self->{verbose});
					return undef;
				}
				$rom->{status} = $status{$attributes{status}};
			}
			$rom->{mtime} = $attributes{time};
			$rom->{crc} = hex($attributes{crc});

			$self->omit_hashes($archive->{name}, $rom);

			push @{$archive->{files}}, $rom;
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
		my $archive = $self->{archives_got}->{$id};
		push @dump, "$id|$archive->{name}|$archive->{mtime}|$archive->{size}";
	}
	push @dump, '>>> table file (archive_id, file_idx, name, mtime, status, size, crc, md5, sha1)';

	for my $id (sort { $a <=> $b} keys %{$self->{archives_got}}) {
		my $archive = $self->{archives_got}->{$id};

		if ($self->{unzipped}) {
			my $dump_archive;
			my $next_idx = 0;
			if (exists($self->{dump_archives}->{$archive->{name}})) {
				$dump_archive = $self->{dump_archives}->{$archive->{name}};
				$next_idx = $dump_archive->{max_idx} + 1;
			}
			for my $file (@{$archive->{files}}) {
				if ($dump_archive && exists($dump_archive->{files}->{$file->{name}})) {
					$file->{idx} = $dump_archive->{files}->{$file->{name}};
				}
				else {
					$file->{idx} = $next_idx++;
				}
			}

			$archive->{files} = [ sort { $a->{idx} <=> $b->{idx} } @{$archive->{files}} ];
		}
		for my $file (@{$archive->{files}}) {
			push @dump, join '|', $id, $file->{idx}, $file->{name}, $file->{mtime}, $file->{status}, $file->{size}, $file->{crc}, "<$file->{md5}>", "<$file->{sha1}>";
		}
	}
	
	$self->{dump_expected} = \@dump;
	
	return 1;
}


sub omit_hashes {
	my ($self, $archive, $file) = @_;

	my $omit;

	return if ($self->{unzipped});
	return unless (exists($self->{no_hashes}->{$archive}));
	if (ref($self->{no_hashes}->{$archive}) eq 'HASH') {
		$omit = $self->{no_hashes}->{$archive}->{$file->{name}};
	}
	else {
		$omit = 1;
	}

	if ($omit eq '1') {
		$omit = 'md5,sha1';
	}

	return unless defined($omit);

	$file->{status} = 0;
	for my $hash (split ',', $omit) {
		$file->{$hash} = 'null';
	}
}


sub destrsvis {
	my ($str) = @_;

	$str =~ s/\\a/\a/g;
	$str =~ s/\\b/\b/g;
	$str =~ s/\\f/\f/g;
	$str =~ s/\\n/\n/g;
	$str =~ s/\\r/\r/g;
	$str =~ s/\\s/ /g;
	$str =~ s/\\t/\t/g;
	$str =~ s/\\v/\cK/g;
	$str =~ s/\\(#|\\)/$1/g;

	return $str;
}
1;
