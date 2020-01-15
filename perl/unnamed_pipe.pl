#!/usr/bin/perl

use strict;
use warnings qw(FATAL all);

my $filename = shift @ARGV;
die "Use ./unnamed_pipe.pl FILENAME" if !$filename;

open my $fd, '<', $filename or die "Cant open the $filename: $!\n";
pipe my $pipefd_rd, my $pipefd_wr or die "Cant create pipe: $!\n";

my $pid = fork();
if (!$pid) {
	# Child process
	close $pipefd_wr or die "Cant block parent pipe: $!\n";
	open STDIN, '<&', $pipefd_rd or die "Cant substitue STDIN: $!\n"; 

	$ENV{'PATH'} = '/usr/bin';
	exec "wc -c" or die "Cant use wc: $!\n";
} else {
	# Parent process
	close $pipefd_rd or die "Cant open child thread: $!\n";

	my $buf = "";
	while (read $fd, $buf, 2) {
		print $pipefd_wr substr($buf, 1, 1);
	}
}
