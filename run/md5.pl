#!/usr/bin/perl
#
# these files demonstrate how to use external pipe processes
# (sort of like cgi) with LDMUD. but by now LDMUD knows how
# to do MD5 and SHA1 itself, so they are not being used.
#
# consult http://about.psyc.eu/spawn for instructions
#
use Digest::MD5 qw(md5 md5_hex);

$| = 1;

$md5 = Digest::MD5->new;

while(defined ($line = <STDIN>)) {
	chomp $line;
	$md5->add($line);
	print STDOUT $md5->hexdigest;
}

1;
