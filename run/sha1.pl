#!/usr/bin/perl
#
# these files demonstrate how to use external pipe processes
# (sort of like cgi) with LDMUD. but by now LDMUD knows how
# to do MD5 and SHA1 itself, so they are not being used.
#
# consult http://about.psyc.eu/spawn for instructions
#
use Digest::SHA1 qw(sha1 sha1_hex);

$| = 1;

$sha1 = Digest::SHA1->new;

while(defined ($line = <STDIN>)) {
	chomp $line;
	$sha1->add($line);
	print STDOUT $sha1->hexdigest;
}

1;
