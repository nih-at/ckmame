package NiHTest;

use strict;
use warnings;

use File::Copy;
use File::Path qw(make_path);
use IPC::Open3;
use Symbol 'gensym';
use UNIVERSAL;

use Text::Diff;

#  NiHTest -- package to run regression tests
#  Copyright (C) 2002-2013 Dieter Baron and Thomas Klausner
#
#  This file is part of ckmame, a program to check rom sets for MAME.
#  The authors can be contacted at <ckmame@nih.at>
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in
#     the documentation and/or other materials provided with the
#     distribution.
#  3. The names of the authors may not be used to endorse or promote
#     products derived from this software without specific prior
#     written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
#  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
#  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
#  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
#  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
#  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
#  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
#  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# runtest TESTNAME
#
# files:
#   TESTNAME.test: test scenario
#
# test scenario:
#    Lines beginning with # are comments.
#
#    The following commands are recognized; return and args must
#    appear exactly once, the others are optional.
#
#	args ARGS
#	    run program with command line arguments ARGS
#
#	description TEXT
#	    description of what test is for
#
#	features FEATURE ...
#	    only run test if all FEATUREs are present, otherwise skip it.
#
#	file TEST IN OUT
#	    copy file IN as TEST, compare against OUT after program run.
#
#	file-del TEST IN
#	    copy file IN as TEST, check that it is removed by program.
#
#	file-new TEST OUT
#	    check that file TEST is created by program and compare
#	    against OUT.
#
#	mkdir MODE NAME
#	    create directory NAME with permissions MODE.
#
#	preload LIBRARY
#	    pre-load LIBRARY before running program.
#
#	program PRG
#	    run PRG instead of ckmame.
#
#	return RET
#	    RET is the expected exit code
#
#	setenv VAR VALUE
#	    set environment variable VAR to VALUE.
#
#	stderr TEXT
#	    program is expected to produce the error message TEXT.  If
#	    multiple stderr commands are used, the messages are
#	    expected in the order given.
#
#	stdout TEXT
#	    program is expected to print TEXT to stdout.  If multiple
#	    stdout commands are used, the messages are expected in
#	    the order given.
#
#	ulimit C VALUE
#	    set ulimit -C to VALUE while running the program.
#
# exit status
#	runtest uses the following exit codes:
#	    0: test passed
#	    1: test failed
#	    2: other error
#	   77: test was skipped
#
# environment variables:
#   RUN_GDB: if set, run gdb on program in test environment
#   NOCLEANUP: if set, don't delete test environment
#   SETUP_ONLY: if set, exit after creating test environment
#   VERBOSE: if set, be more verbose (e. g., output diffs)

my %EXIT_CODES = (
	PASS => 0,
	FAIL => 1,
	SKIP => 77,
	ERROR => 99
    );

sub new {
	my $class = UNIVERSAL::isa ($_[0], __PACKAGE__) ? shift : __PACKAGE__;
	my $self = bless {}, $class;
	
	my ($opts) = @_;

	$self->{default_program} = $opts->{default_program};
	$self->{zipcmp} = $opts->{zipcmp} // 'zipcmp';
	$self->{zipcmp_flags} = $opts->{zipcmp_flags};

	$self->{directives} = {
		args => { type => 'string...', once => 1, required => 1 },
		description => { type => 'string', once => 1 },
		features => { type => 'string...', once => 1 },
		file => { type => 'string string string' },
		'file-del' => { type => 'string string' },
		'file-new' => { type => 'string string' },
		mkdir => { type => 'string string' },
		preload => { type => 'string', once => 1 },
		program => { type => 'string', once => 1 },
		'return' => { type => 'int', once => 1, required => 1 },
		setenv => { type => 'string string' },
		stderr => { type => 'string' },
		stdout => { type => 'string' },
		ulimit => { type => 'char string' },
	};
	
	$self->{compare_by_type} = { 'zip/zip' => \&comparator_zip };
	$self->{hooks} = {};
	
	$self->{srcdir} = $opts->{srcdir} // $ENV{srcdir};
	
	if (!defined($self->{srcdir}) || $self->{srcdir} eq '') {
		$self->{srcdir} = `sed -n 's/^srcdir = \(.*\)/\1/p' Makefile`;
		chomp($self->{srcdir});
	}
	
	$self->{in_sandbox} = 0;
	
	$self->{verbose} = $ENV{VERBOSE};

	return $self;
}


sub add_comparator {
	my ($self, $ext, $sub) = @_;
	
	$self->{compare_by_type}->{$ext} = $sub;
	
	return 1;
}


sub add_directive {
	my ($self, $name, $def) = @_;
	
	if (exists($self->{directives}->{$name})) {
		$self->die("directive $name already defined");
	}
	
	# TODO: validate $def
	
	$self->{directives}->{$name} = $def;
	
	return 1;
}


sub add_hook {
	my ($self, $hook, $sub) = @_;
	
	$self->{hooks}->{$hook} = [] unless (defined($self->{hooks}->{$hook}));
	push @{$self->{hooks}->{$hook}}, $sub;
}


sub run {
	my ($self, @argv) = @_;
	
	if (scalar(@argv) != 1) {
		print STDERR "Usage: $0 testcase\n";
		exit(1);
	}
	
	my $testcase = shift @argv;

	$testcase .= '.test' unless ($testcase =~ m/\.test$/);

	my $testcase_file = $self->find_file($testcase);
	
	$self->die("cannot find test case $testcase") unless ($testcase_file);
	
	$testcase =~ s,^(?:.*/)?([^/]*)\.test$,$1,;
	$self->{testname} = $testcase;

	$self->die("error in test case definition") unless $self->parse_case($testcase_file);
	
	$self->check_features_requirement() if ($self->{test}->{features});
	
	$self->sandbox_create();
	$self->runtest();
}


#
# internal methods
#

sub add_file {
	my ($self, $file) = @_;
	
	if (defined($self->{files}->{$file->{destination}})) {
		$self->warn("duplicate specification for input file $file->{destination}");
		return undef;
	}
        
	$self->{files}->{$file->{destination}} = $file;
	
	return 1;
}


sub check_features_requirement() {
	my ($self) = @_;
	
	### TODO: implement
	
	return 1;
}


sub comparator_zip {
	my ($self, $got, $expected) = @_;

	my @args = ($self->{zipcmp}, $self->{verbose} ? '-v' : '-q');
	push @args, $self->{zipcmp_flags} if ($self->{zipcmp_flags});
	push @args, ($expected, $got);
        
	my $ret = system(@args);
	
	return $ret == 0;
}


sub compare_arrays() {
	my ($self, $a, $b, $tag) = @_;
	
	my $ok = 1;
	
	if (scalar(@$a) != scalar(@$b)) {
		$ok = 0;
	}
	else {
		for (my $i = 0; $i < scalar(@$a); $i++) {
			if ($a->[$i] ne $b->[$i]) {
				$ok = 0;
				last;
			}
		}
	}
	
	if (!$ok && $self->{verbose}) {
		print "Unexpected $tag:\n";
		my @a = map { $_ . "\n"; } @$a;
		my @b = map { $_ . "\n"; } @$b;
		print diff(\@a, \@b);
	}
	
	return $ok;
}


sub compare_file() {
	my ($self, $got, $expected) = @_;
	
	my $real_expected = $self->find_file($expected);
	unless ($real_expected) {
		$self->warn("cannot find expected result file $expected");
		return 0;
	}

	my $ext = ($self->get_extension($got)) . '/' . ($self->get_extension($expected));
	
	if ($self->{compare_by_type}->{$ext}) {
		return $self->{compare_by_type}->{$ext}($self, $got, $real_expected);
	}
	else {
		my $ret = system('diff', $self->{verbose} ? '-u' : '-q', $real_expected, $got);
		return $ret == 0;
	}
}


sub compare_files() {
	my ($self) = @_;
	
	my $ok = 1;
	
	my $ls;
	open $ls, "find . -type f -print |";
	unless ($ls) {
		# TODO: handle error
	}
	my @files_got = ();
	
	while (my $line = <$ls>) {
		chomp $line;
		$line =~ s,^\./,,;
		push @files_got, $line;
	}
	close($ls);
	
	@files_got = sort @files_got;
	my @files_should = ();
	
	for my $file (sort keys $self->{files}) {
		push @files_should, $file if ($self->{files}->{$file}->{result} || $self->{files}->{$file}->{ignore});
	}
	
	$ok = $self->compare_arrays(\@files_should, \@files_got, 'files');
	
	for my $file (@files_got) {
		my $file_def = $self->{files}->{$file};
		next unless ($file_def && $file_def->{result});
		
		$ok &= $self->compare_file($file, $file_def->{result});
	}
	
	return $ok;
}


sub copy_files {
	my ($self) = @_;
	
	my $ok = 1;
	
	for my $filename (sort keys %{$self->{files}}) {
		my $file = $self->{files}->{$filename};
		next unless ($file->{source});
		
		my $src = $self->find_file($file->{source});
		unless ($src) {
			$self->warn("cannot find input file $file->{source}");
			$ok = 0;
			next;
		}

		if ($file->{destination} =~ m,/,) {
			my $dir = $file->{destination};
			$dir =~ s,/[^/]*$,,;
			if (! -d $dir) {
				make_path($dir);
			}
		}
		
		unless (copy($src, $file->{destination})) {
			$self->warn("cannot copy $src to $file->{destination}: $!");
			$ok = 0;
		}
	}

	if (defined($self->{test}->{mkdir})) {
		for my $dir_spec (@{$self->{test}->{mkdir}}) {
			my ($mode, $dir) = @$dir_spec;
			if (! -d $dir) {
				unless (mkdir($dir, oct($mode))) {
					$self->warn("cannot create directory $dir: $!");
					$ok = 0;
				}
			}
		}
	}
	
	$self->die("failed to copy input files") unless ($ok);
}


sub die() {
	my ($self, $msg) = @_;
	
	print STDERR "$0: $msg\n" if ($msg);
	
	$self->end_test('ERROR', $msg);
}


sub end_test {
	my ($self, $status, $reason) = @_;
	
	my $exit_code = $EXIT_CODES{$status} // $EXIT_CODES{ERROR};
	
	if ($self->{verbose}) {
		print "$self->{testname} -- $status";
		print ": $reason" if ($reason);
		print "\n";
	}
	
	$self->exit($exit_code);
}



sub exit() {
	my ($self, $status) = @_;
	### TODO: cleanup
	
	exit($status);
}


sub fail() {
	my ($self, $msg) = @_;
	
	$self->end_test(1, 'FAILED', $msg);
}


sub find_file() {
	my ($self, $fname) = @_;
	
	for my $dir (('', "$self->{srcdir}/")) {
		my $f = "$dir$fname";
		$f = "../$f" if ($self->{in_sandbox} && $dir !~ m,^/,);
		
		return $f if (-f $f);
	}
	
	return undef;
}


sub get_extension {
	my ($self, $fname) = @_;

	my $ext = $fname;
	if ($ext =~ m/\./) {
		$ext =~ s/.*\.//;
	}
	else {
		$ext = '';
	}

	return $ext;
}


sub parse_args {
	my ($self, $type, $str) = @_;
	
	if ($type =~ m/(\s|\.\.\.$)/) {
		my $ellipsis = 0;
		if ($type =~ m/(.*)\.\.\.$/) {
			$ellipsis = 1;
			$type = $1;
		}
		my @types = split /\s+/, $type;
		my @strs = split /\s+/, $str;
		
		if (!$ellipsis && scalar(@types) != scalar(@strs)) {
			$self->warn_file_line("expected " . (scalar(@types)) . " arguments, got " . (scalar(@strs)));
			return undef;
		}
		
		my $args = [];
		
		my $n = scalar(@types);
		for (my $i=0; $i<scalar(@strs); $i++) {
			my $val = $self->parse_args(($i >= $n ? $types[$n-1] : $types[$i]), $strs[$i]);
			return undef unless (defined($val));
			push @$args, $val;
		}
		
		return $args;
	}
	else {
		if ($type eq 'string') {
			return $str;
		}
		elsif ($type eq 'int') {
			if ($str !~ m/^\d+$/) {
				$self->warn_file_line("illegal int [$str]");
				return undef;
			}
			return $str+0;
		}
		elsif ($type eq 'char') {
			if ($str !~ m/^.$/) {
				$self->warn_file_line("illegal char [$str]");
				return undef;
			}
			return $str;
		}
		else {
			$self->warn_file_line("unknown type $type");
			return undef;
		}
	}
}


sub parse_case() {
	my ($self, $fname) = @_;
	
	my $ok = 1;
	
	open TST, "< $fname" or $self->die("cannot open test case $fname: $!");
	
	$self->{testcase_fname} = $fname;
	
	my %test = ();
	
	while (my $line = <TST>) {
		chomp $line;
		
		next if ($line =~ m/^\#/);
		
		unless ($line =~ m/(\S*)\s*(.*)/) {
			$self->warn_file_line("cannot parse line $line");
			$ok = 0;
			next;
		}
		my ($cmd, $argstring) = ($1, $2);
		
		my $def = $self->{directives}->{$cmd};
		
		unless ($def) {
			$self->warn_file_line("unknown directive $cmd in test file");
		}
		
		my $args = $self->parse_args($def->{type}, $argstring);
		
		next unless (defined($args));
		
		if ($def->{once}) {
			if (defined($test{$cmd})) {
				$self->warn_file_line("directive $cmd appeared twice in test file");
			}
			$test{$cmd} = $args;
		}
		else {
			$test{$cmd} = [] unless (defined($test{$cmd}));
			push @{$test{$cmd}}, $args;
		}
	}

	close TST;
	
	return undef unless ($ok);
	
	for my $cmd (sort keys %test) {
		if ($self->{directives}->{$cmd}->{required} && !defined($test{$cmd})) {
			$self->warn_file("required directive $cmd missing in test file");
			$ok = 0;
		}
	}
	
	return undef unless ($ok);
	
	if (!defined($test{program})) {
		$test{program} = $self->{default_program};
	}

	$self->{test} = \%test;

	$self->run_hook('mangle_program');
	
	if (!$self->parse_postprocess_files()) {
		return 0;
	}

	return $self->run_hook('post_parse');
}


sub parse_postprocess_files {
	my ($self) = @_;
	
	$self->{files} = {};
	
	my $ok = 1;
	
	for my $file (@{$self->{test}->{file}}) {
		$ok = 0 unless ($self->add_file({ source => $file->[1], destination => $file->[0], result => $file->[2] }));
	}
	
	for my $file (@{$self->{test}->{'file-del'}}) {
		$ok = 0 unless ($self->add_file({ source => $file->[1], destination => $file->[0], result => undef }));
	}
	
	for my $file (@{$self->{test}->{'file-new'}}) {
		$ok = 0 unless ($self->add_file({ source => undef, destination => $file->[0], result => $file->[1] }));
	}
	
	return $ok;
}


sub runtest {
	my ($self) = @_;
	
	$self->sandbox_enter();
	
	$self->copy_files();
	$self->run_hook('prepare_sandbox');
	$self->run_program();

	my @failed = ();
	
	if ($self->{exit_status} != $self->{test}->{return} // 0) {
		push @failed, 'exit status';
		if ($self->{verbose}) {
			print "Unexpected exit status:\n";
			print "-" . ($self->{test}->{return} // 0) . "\n+$self->{exit_status}\n";
		}
	}
	if (!$self->compare_arrays($self->{test}->{stdout} // [], $self->{stdout}, 'output')) {
		push @failed, 'output';
	}
	if (!$self->compare_arrays($self->{test}->{stderr} // [], $self->{stderr}, 'error output')) {
		push @failed, 'error output';
	}
	if (!$self->compare_files()) {
		push @failed, 'files';
	}
	
	$self->sandbox_leave();
	### TODO: remove sandbox

	$self->end_test(scalar(@failed) == 0 ? 'PASS' : 'FAIL', join ', ', @failed);
}


sub run_hook {
	my ($self, $hook) = @_;
	
	my $ok = 1;
	
	if (defined($self->{hooks}->{$hook})) {
		for my $sub (@{$self->{hooks}->{$hook}}) {
			$ok &= $sub->($self, $hook);
		}
	}
	
	return $ok;
}


sub run_program {
	my ($self) = @_;
	
	my ($stdin, $stdout, $stderr);
	$stderr = gensym;
	
	my $cmd = '../' . $self->{test}->{program} . " " . (join ' ', @{$self->{test}->{args}});
	
	### TODO: catch errors?
	
	my $pid = open3($stdin, $stdout, $stderr, $cmd);
	
	$self->{stdout} = [];
	$self->{stderr} = [];
	
	while (my $line = <$stdout>) {
		chomp $line;
		push @{$self->{stdout}}, $line;
	}
	while (my $line = <$stderr>) {
		chomp $line;
		$line =~ s/^[^:]*: //; # TODO: make overridable
		push @{$self->{stderr}}, $line;
	}
	
	waitpid ($pid, 0);
	
	$self->{exit_status} = $? >> 8;
}


sub sandbox_create {
	my ($self) = @_;
	
	$self->{sandbox_dir} = "sandbox-$self->{testname}.d$$";
	
	$self->die("sandbox $self->{sandbox_dir} already exists") if (-e $self->{sandbox_dir});
	
	mkdir($self->{sandbox_dir}) or $self->die("cannot create sandbox $self->{sandbox_dir}: $!");
	
	return 1;
}


sub sandbox_enter {
	my ($self) = @_;
	
	$self->die("internal error: cannot enter sandbox before creating it") unless (defined($self->{sandbox_dir}));

	return if ($self->{in_sandbox});

	chdir($self->{sandbox_dir}) or $self->die("cant cd into sandbox $self->{sandbox_dir}: $!");
	
	$self->{in_sandbox} = 1;
}


sub sandbox_leave {
	my ($self) = @_;
	
	return if (!$self->{in_sandbox});
	
	chdir('..') or $self->die("cannot leave sandbox: $!");
	
	$self->{in_sandbox} = 0;
}


sub skip() {
	my ($self, $msg) = @_;
	
	$self->end_test(77, 'skipped', $msg);
}


sub succeed() {
	my ($self, $msg) = @_;
	
	$self->end_test(0, 'passed', $msg);
}


sub warn {
	my ($self, $msg) = @_;
	
	print STDERR "$0: $msg\n";
}


sub warn_file {
	my ($self, $msg) = @_;
	
	$self->warn("$self->{testcase_fname}: $msg");
}


sub warn_file_line {
	my ($self, $msg) = @_;
	
	$self->warn("$self->{testcase_fname}:$.: $msg");
}

1;
