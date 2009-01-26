# $Id: INI.pm,v 1.34 2006/06/14 16:27:23 lynx Exp $
package INI;
require Tie::Hash;
@ISA = (Tie::ExtraHash);

use strict;

# lynx: you propably really want that _all_ tied INI hashes in a Perl ...
# "session" share the verbosity-setting, but would you mind if i did that some
# more OO, on a per hash basis?
my $verbose = 0;
# saga: i don't need this to be any better but if your heart aches, go ahead

sub HASH	{ 0 }
sub FNAME	{ 1 }
sub CHANGED	{ 2 }
sub MAX 	{ 3 }
sub JANITOR	{ 4 }

sub import {
    $_ = join ' ', @_;
    $verbose = /\bverbose\b/i;
}

sub iniSane {
    my $self = shift;
    my $key = shift;
    my $value = shift;

    if (defined $value && $key !~ /^.+?_.+/) {
	die "'$key' is not a valid key";
    }

    if ($key =~ /[\r\n\s]/) {
	die "invalid characters in key '$key'";
    }

    if (defined $value && $value =~ /[\r\n]/) {
	die "invalid characters in value for '$key'";
    }
}

# TODO:: make janitor a more swanky datastructure.
# like $janitor->{'_foo'}->[CHILDREN]->{'_bar'}->[COUNT]
# instead of $janitor->{'_foo_bar'}.
# on the other hand, we'd probably need entries for levels with a count of 0,
# if they got child levels. hm.
# it'd eat more memory (ok, we don't have to care for that), and.. i'm not even
# sure if it would be faster. probably not. same operations, splitting on "_",
# and we'd have to walk through the structure beginning from the leftmost level
# of a value to the right every time while we might find our levels earlier
# if coming from the right (.. trying _foo_bar_some before _foo_bar before
# _foo) in the average case. we'll at least in some cases, while we always need
# to walk through from the leftmost level if doing that tree specified above.
# hm. i'll have to think that over.
sub iniJanDel {
    my $self = shift;
    my $key = shift;
    my $janitor = $self->[JANITOR];
    my $max = $self->[MAX];

    while ($key =~ s/(.+)_.*/$1/) {
	if ($janitor->{$key}-- == $max) {
	    for (my $i = 1; $i < $max; $i++) {
		$self->iniJanAdd($key);
	    }
	    last;
	}
	last if $janitor->{$key} >= $max;
	delete $janitor->{$key} unless $janitor->{$key};
    }
}

sub iniJanAdd {
    my $self = shift;
    my $key = shift;
    my $janitor = $self->[JANITOR];
    my $max = $self->[MAX];

    while ($key =~ s/(.+)_.*/$1/) {
	if (++$janitor->{$key} == $max) {
	    for (my $i = 1; $i < $max; $i++) {
		$self->iniJanDel($key);
	    }
	    last;
	}
	last if $janitor->{$key} > $max;
    }
}

sub iniParse {
    my $self = shift;
    my $fname = $self->[FNAME];
    my $max = $self->[MAX];
    my $hash = $self->[HASH];
    my $janitor = $self->[JANITOR];
    my $section;

    open(FILE, '<', $fname) || die $!;
    while (<FILE>) {
	chomp;

	next if /^\s*(?:$|;)/;

	if (/^\[(.+?)\]\s*(?:;.*)?$/) {
	    $self->iniSane($1);
	    $section = $1;
	    print STDERR "$section " if $verbose;
	    next;
	}

	if ($section && /^(.+?)\s*=\s*((?>(?:(?<=(?<!\\)\\)(?:\\\\)*["\s;\\]|[^"\s\\;]|\s(?!\s*(?:;|$))|\\(?!$))*)|"(?:(?<=(?<!\\)\\)(?:\\\\)*["\s]|[^"])*?(?:(?<!\\)(?:\\\\)+|(?<!\\))")\s*(?:;.*)?$/) {
	    my $t = join('', $section, $1);
	    my $t2 = $2;
	    if (substr($t2, 0, 1) eq '"') {
		$t2 = substr($t2, 1, length($t2)-2);
	    }
	    $t2 =~ s/\\(.)/$1/g;
	    $self->iniSane($t, $t2);
	    $hash->{$t} = $t2;
#	    print STDERR "$t is $t2\n";
	    $self->iniJanAdd($t) if $max;
	    next;
	}

	die "failed in parsing from '$fname':\n$_\n(line $.)" if ($_);
    }
    close(FILE);
}

sub iniSerialize {
    my $self = shift;
    my $hash = $self->[HASH];
    my $janitor = $self->[JANITOR];
    my $max = $self->[MAX];
    my %shash = ();
    my $nottop;
    my $res = ";this is an automatically generated file, you may change it as "
	      . "you like, but\n"
	      . ";keep in mind that your comments will be destroyed if a "
	      . "config tool makes some\n"
	      . ";changes.\n\n";

    foreach (keys %$hash) {
	my $sec = $_;
	my $key = "";
	my ($section, $buf);

	while ($max && $sec =~ s/(.+)(_.*)/$1/) {
	    $key = $2 . $key;
	    if ($janitor->{$sec} >= $max) {
		$section = $sec;
		last;
	    }
	}

	unless ($section) {
	    /(.+?)(_.*)/;
	    $section = $1;
	    $key = $2;
	}

	$shash{$section} = {} unless exists $shash{$section};

	$buf = $hash->{$_};
	$buf =~ s/\\/\\\\/g;
	$buf =~ s/"/\\"/g;
	if ($buf =~ /(?:^\s|\s$|;)/) {
	    $buf = join('', '"', $buf, '"');
	}

	$shash{$section}->{$key} = $buf;
    }

    foreach my $base (sort keys %shash) {
	if ($nottop) {
	    $res .= "\n";
	} else {
	    $nottop = 1;
	}

	$res .= join('', '[', $base, ']', "\n");

	foreach my $child (keys %{$shash{$base}}) {
	    $res .= join(' ', $child, '=', $shash{$base}->{$child});
	    $res .= "\n";
	}
    }

    return $res;
}

sub TIEHASH {
    my $pkg = shift;
    my $fname = shift || die 'NO FILENAME PROVIDED';
    my $max = shift;
    my $self = [{}, $fname, 0, $max, {}];

    bless $self, $pkg;

    if (-r $fname) {
	print STDERR "Loading from $fname:\n" if $verbose;
	$self->iniParse();
	print STDERR "\nConfiguration loaded.\n\n" if $verbose;
    } else {
	print STDERR "$fname does not exist.\n" if $verbose;
    }
    return $self;
}

sub STORE {
    my $self = shift;
    my $key = shift;
    my $value = shift;


#TODO: needs more restrictive checks, (key, value) are valid if they can be
#added to the hash.
#alternatively keep this somewhat abstract and overload it for psyced config
#sanity checks.
#oh, i like that.

    $self->iniSane($key, $value);

    $self->[CHANGED] = 1;

    if ($self->[MAX] && !exists $self->[HASH]->{$key}) {
	$self->iniJanAdd($key);
    }

    return $self->SUPER::STORE($key, $value);
}

sub DELETE {
    my $self = shift;
    my $key = shift;

    if ($self->[MAX] && exists $self->[HASH]->{$key}) {
	$self->iniJanDel($key);
    }

    return $self->SUPER::DELETE($key);
}

sub CLEAR {
    my $self = shift;

    $self->[CHANGED] = 1;
    $self->[JANITOR] = {};

    return $self->SUPER::CLEAR();
}

sub SCALAR {
    my $self = shift;
    
    return $self->iniSerialize();
}

sub UNTIE {
    my $self = shift;

    return unless $self->[CHANGED];

    open(FILE, '>', $self->[FNAME]) || die $!;
    print FILE $self->iniSerialize();
    close(FILE);
}

1;

=head1 NAME

INI - class definitions for tying .ini-like configuration files as hashes

=head1 SYNOPSIS

    require INI;

    tie(%hash, INI, 'conf.ini', 5);

    if ($hash{'_some_system_setting'} =~ /^y(?:es)$/) {
	# do something
    }

    $hash{'_some_system_setting2'} = 12;

    untie %hash;

=head1 FORMAT DESCRIPTION

    [section] ; comment
    key = value ; comment
    key = "value" ; comment
      ; comment
    (all whitespaces up to here are optional).

    key = \"
    (value of key will be a quote sign)

    key = \; 		; and
    key = ";"
    (value of key will will be a semicolon)

    Section- and keynames should start with underscores.

    The section names are just prepended to the key names, so

	[_foo]
	_bar_snafu = kibo
    and
	[_foo_bar]
	_snafu = kibo
    define the same (key, value)-tuple.

=head1 API

=over

=item tie(B<HASH>, B<FILE>, B<MAX>)

Tie B<FILE> to B<HASH>, B<FILE> will be parsed upon tie and fill the hash,
contents of B<HASH> will be serialized to B<FILE> on untie.

B<MAX> is optional and defines when a level counts as full, thus if B<MAX>
is set to 3, and you have 3 settings named _foo_bar_something1,
_foo_bar_something2 and _foo_bar_something3, they'll be serialized into a

section [_foo_bar] as _something<digit> = value.

=item untie(B<HASH>)

Untie B<HASH>, will be serialized into B<FILE> given in the tie statement if
changed.

Will use B<MAX> (from the tie statement) to find out which levels deserve their
own section.

=item scalar(B<HASH>)

Returns a string containing B<HASH> serialized into the INI format.
You'll most probably never need that for anything but debugging purposes, as
B<INI> does serializing into files for you on untie.

=back

=head1 SEE ALSO

L<http://about.psyc.eu/psyced>

=head1 AUTHOR

Tobias Josefowitz, <tobij@goodadvice.pages.de>

=head1 COPYRIGHT

Copyright (c) 2005 Tobias Josefowitz.
All rights reserved.

This program is free software under the GPL.
=cut
