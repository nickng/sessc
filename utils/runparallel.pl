#!/usr/bin/perl
#
# Perl script to start Session C programs in parallel.
#

use strict;
use warnings;

use Cwd;
use threads;

my $conf_file = q{connection.conf};
my $host_count = 0;
my $conn_count = 0;
my $sigint_count = 0;

#
# Local variables
#
my ($role, $host, $role_name);
my @threads;

my $pwd = getcwd;

#
# Signal handler
#
$SIG{'INT'} = \&sigint_handler;

sub sigint_handler {
  $SIG{'INT'} = \&sigint_handler;
  $sigint_count++;
  if ($sigint_count > 2) {
    foreach my $thread (@threads) {
        $thread->kill('KILL')->detach();
    }
    $sigint_count = 0;
  } else {
    print "^C ".(2 - $sigint_count). "more times to kill threads\n";
  }
}

sub ssh_thread {
    my $host = shift;
    my $pwd  = shift;
    my $role = shift;
    my $conf = shift;
    my $args = shift;
    print "{ host=$host, role=$role }";
    print " command=`ssh $host 'cd $pwd; ./$role -c $conf $args'\n";
    system("ssh $host 'cd $pwd; ./$role -c $conf $args'");
}

#
# Environment.
#
print "Current working directory is $pwd\n";

#
# Read file.
#
open my $conf, '<', $conf_file || die "Unable to open file: $!";
if (<$conf> =~ /(\d+)\s+(\d+)/) {
  ($host_count, $conn_count) = ($1, $2);
}
print "Number of hosts: $host_count\n";
print "Number of connections: $conn_count\n";


print "-----------------------------------------------------------------\n";
for my $host_idx (1 .. $host_count) {
  if (<$conf> =~ /([a-zA-Z0-9_-]+)\s+([a-zA-Z0-9\.-]+)/) {
    ($role, $host) = ($1, $2);
    $role_name = lc $role;

    push @threads, threads->create('ssh_thread', $host, $pwd, $role_name, $conf_file, "@ARGV");
  }
}

print "-----------------------------------------------------------------\n";

foreach my $thread (@threads) {
    $thread->join();
}

close $conf;

0;
