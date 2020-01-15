#!/usr/bin/perl -T
use strict;
use warnings qw(FATAL all);
use IO::Socket::UNIX;
$ENV{'PATH'} = '/usr/bin';
$ENV{CDPATH}="";
$ENV{ENV}="";

my $path = '/tmp/socket';
my $pid = $$;
my $uid = $<; 
my @gr_list = split(/ /, $(); 
my $gid = $gr_list[0];
my $start_time = time();
my $work_time;
my @sys_load;


$work_time = time() - $start_time;
@sys_load = split(/load average: /, qx(uptime));
@sys_load = split(', ', $sys_load[1]);


sub update_info {
    $work_time = time() - $start_time;
    @sys_load = split(/load average: /, qx(uptime));
    @sys_load = split(', ', $sys_load[1]);
}

$SIG{HUP} = sub{ print "Server was destroyed by HUP\nwork_time = $work_time, 1min = $sys_load[0], 5min = $sys_load[1], 15min = $sys_load[2]\n"; };
$SIG{INT} = sub{ print "Server was destroyed by INT\nwork_time = $work_time, 1min = $sys_load[0], 5min = $sys_load[1], 15min = $sys_load[2]\n"; };
$SIG{TERM} = sub{ print "Server was destroyed by TERM\nwork_time = $work_time, 1min = $sys_load[0], 5min = $sys_load[1], 15min = $sys_load[2]\n"; };
$SIG{USR1} = sub{ print "Server was destroyed by USR1\nwork_time = $work_time, 1min = $sys_load[0], 5min = $sys_load[1], 15min = $sys_load[2]\n"; };
$SIG{USR2} = sub{ print "Server was destroyed by USR2\nwork_time = $work_time, 1min = $sys_load[0], 5min = $sys_load[1], 15min = $sys_load[2]\n"; };

print("Server works.\npid = $pid, uid = $uid, gid = $gid\n");
unlink $path;
my $server = IO::Socket::UNIX->new(
        Type => SOCK_STREAM(),
        Local => $path,
        Listen => 1,
    ) or die "Cant create a socket\n";
	
while (my $conn = $server->accept()) {
	  update_info;
      $conn->print("work_time = $work_time, 1min = $sys_load[0], 5min = $sys_load[1], 15min = $sys_load[2]\n");
	  $conn->close();
    }
