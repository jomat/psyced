#!/usr/bin/perl
#
# new installer in the making
# see also http://about.psyc.eu/psyced

use strict;

# all of the full screen editing stuff could move into
# its appropriate own .pm file  ...? there even seems
# to be overlap with PSYCion.. nevermind..  :)
#
use Prompt;

use Term::Cap;
use Term::ANSIColor qw(colored :constants);

use Data::Dumper;
use Storable;
use Getopt::Std;

my $term;
my $prompt;
my $t; #misc stuff

my %v;
my %opts;

if (-f '.ser.data') {
    %v = %{retrieve('.ser.data')};
}

getopt('w', \%opts);

my %key2name;
my %name2key = (
'left'  =>      "\e[D",
'right' =>      "\e[C",
'down'  =>      "\e[B",
'up'    =>      "\e[A",
'a-left' =>     "\e\e[D",
'a-right' =>    "\e\e[C",
'a-down' =>     "\e\e[B",
'a-up'  =>      "\e\e[A",
'ret'   =>      "\x0d", 'C-m' => "\x0d",
'esc'   =>      "\x1b",
'c-c'   =>      "\x03",
'tab'   =>      "\x09",
'bs'    =>      "\x7f",
'pu'    =>      "\e[5~",
'pd'    =>      "\e[6~",
'home'  => 	["\e[H", "\eO", "\e[1~"],
'end'	=>	["\eOw","\eOe", "\e[4~"],
'ins' 	=>	"\e[2~",
'del'   =>      "\e[3~",
'f1'	=>	"\e[11~",
'f2'	=>	"\e[12~",
'f3'	=>	"\e[13~",
'f4'	=>	"\e[14~",
'f5'	=>	"\e[15~",
'f6'	=>	"\e[17~",
'f7'	=>	"\e[18~",
'f8'	=>	"\e[19~",
'f9'	=>	"\e[20~",
'f10'	=>	"\e[21~",
'f11'	=>	"\e[23~",
'f12'	=>	"\e[24~",
'c-a'   =>	"\x01",
'c-b'   =>	"\x02",
#'c-c'   =>	"\x03",
'c-d'   =>	"\x04",
'c-e'   =>	"\x05",
'c-f'   =>	"\x06",
'c-g'   =>	"\x07",
'c-h'   =>	"\x08",
'c-i'   =>	"\x09",
'c-j'	=>	"\x0a",
'c-k'   =>	"\x0b",
'c-l'   =>	"\x0c",
'c-m'   =>	"\x0d",
'c-n'   =>	"\x0e",
'c-o'   =>	"\x0f",
'c-p'   =>	"\x10",
'c-q'   =>	"\x11",
'c-r'   =>	"\x12",
'c-s'   =>	"\x13",
'c-t'   =>	"\x14",
'c-u'   =>	"\x15",
'c-v'   =>	"\x16",
'c-w'   =>	"\x17",
'c-x'   =>	"\x18",
'c-y'   =>	"\x19",
'c-z'   =>	"\x1a",
'a-1'	=>	"\e1",
'a-2'	=>	"\e2",
'a-3'	=>	"\e3",
'a-4'	=>	"\e4",
'a-5'	=>	"\e5",
'a-6'	=>	"\e6",
'a-7'	=>	"\e7",
'a-8'	=>	"\e8",
'a-9'	=>	"\e9",
'a-0'	=>	"\e0",
);

sub YESNO { +{ 'y' => 1, 'n' => 1 } }

sub NUMFT {
    my $from = shift;
    my $to = shift;

    die unless (defined $from && defined $to);
    my $href = {};

    foreach ($from..$to) {
	$href->{$_} = 1;
    }

    return $href;
}

sub bind {
    my $key = shift;
    my $cmd = shift;

    $key2name{$name2key{$key}} = $cmd;
#print "$key, $cmd, " . unpack("H*", $name2key{$key}) . "\r\n";
}

$name2key{'pos1'} = $name2key{'home'};
$name2key{'alt-left'} = $name2key{'a-left'};
$name2key{'alt-right'} = $name2key{'a-right'};

my %action = (
	       'exit' => sub { print "\r\n"; exit; }
	      );

sub del_line { "\r".$term->Tputs('dl', 1) }

sub init {
    select(STDIN); $| = 1;
    select(STDOUT); $| = 1;
    system "stty raw -echo";
    $term = Tgetent Term::Cap { TERM => undef, OSPEED => 9600 };
    *Prompt::del_line = \&del_line;
    if ($opts{'w'} && int($opts{'w'}) == $opts{'w'}) {
	print "true!\r\n";
	$prompt = new Prompt({ 'width' => $opts{'w'} });
    } else {
	$prompt = new Prompt;
    }
    $Term::ANSIColor::AUTORESET = 1;

    main::bind('left', 'backward-char');
    main::bind('right', 'forward-char');
    main::bind('c-c', 'exit');
    main::bind('bs', 'backward-delete-char');
    main::bind('home', 'beginning-of-line');
    main::bind('c-a', 'beginning-of-line');
    main::bind('end', 'end-of-line');
    main::bind('c-e', 'end-of-line');
    main::bind('c-u', 'kill-whole-line');
    main::bind('c-k', 'kill-line');
}

sub type {
    my $stdin = shift;
    my $raw;

    sysread(*$stdin, $raw, 5);

    if ($raw =~ /^[\r\n]$/) {
	return 1;
    }

    if (exists $key2name{$raw}) {
	if (exists($action{$key2name{$raw}})) {
	    return $action{$key2name{$raw}}->();
	}

	print $prompt->type($key2name{$raw}, $raw);
	return;
    }
    $prompt->put($raw);

    print del_line() . $prompt->prompt();

    return;
}

sub get {
    my $p = shift;
    my $default = shift;
    my $enforce = shift;

    do {
	$prompt->Ret();
	$prompt->setPrompt($p);
	$prompt->data($default) if $default;

	$prompt->type('end-of-line');

	print del_line() . $prompt->prompt();
	while (!type(\*STDIN)) { }

	print "\r\n";
    } while ($enforce && (!$prompt->data()
			  || ref $enforce
			  && !$enforce->{$prompt->data()})
	     && print "Invalid input. Try again!\r\n");


    return $prompt->data();
}

sub find_driver {
    opendir(DIR, ".");
    my @driver = grep { /^(psyclpc|ldmud)-.+\.tar\.(gz|bz2)$/ } readdir(DIR);
    closedir(DIR);

    if (scalar(@driver) !=  1) {
	return \@driver;
    }

    return $driver[0];
}

sub investigate_protocol {
    my $setting = shift || die;
    my $what = shift || die;
    my $duse = shift || die;
    my $dport = shift || die;
    my $prange = shift;
    my $psome = shift;
    my $pport = shift;

    $v{'use_' . $setting} = get('Enable ' . $what . ': ', $v{'use_' . $setting} || $duse, YESNO);
    if ($v{'use_' . $setting} eq 'y') {
	my $trange;
	my $enf;

	if ($prange) {
	    $trange = " between $prange->[0] and $prange->[1]";
	    $enf = NUMFT(@$prange);
	} elsif ($psome) {
	    $trange = " (";
	    $enf = {};

	    for (my $i = 0; $i < scalar(@$psome); $i++) {
		$trange .= ' or ' if $i;
		$trange .= $psome->[$i];
		$enf->{$psome->[$i]} = 1;
	    }

	    $trange .= ')';
	} else {
	    $enf = 1;
	}

	$v{'port_' . $setting} = get('Port number' . $trange . ': ',
				     $dport, $enf);
	if ($pport) {
	    print '[official port for ' . uc($setting) . " is $pport]\r\n\n";
	}
    }
}

init();

#print Dumper(\%key2name);

print BOLD "psyced.ini configuration generator wizard\r\n\n";

## let's leave ldmud installation in install.sh for those who still want or
## need to do it manually. the majority should use distribution packages!
##
#unless (!ref($t = find_driver())) {
#    print BOLD "ATTENTION: ";
#    print "You have ";
#
#    if (scalar(@$t) == 0) {
#	print "no psyclpc-*.tar.gz in this directory\r\n";
#	print "Please obtain one from http://lpc.psyc.eu\r\n";
#    } else {
#	print "several driver tars in this directory\r\n";
#	print "Please delete some of them until you have only one left.\r\n";
#	print "If you continue now, this script won't compile a driver for you. \r\n\n";
#    }
#
#    unless (get("Continue? ", "n", YESNO) eq 'y') { exit; }
#    print "\r\n";
#} else {
#    print "I can see you have a driver tar here. That's good.\r\n\n";
#}

print BOLD "INSTALLATION SPECIFIC QUESTIONS\r\n\n";

if ($>) {
    print "Since you started this installation not as root, you will see non-root defaults.\r\n\n";
}

$v{'prefix'} = get("Install psyced to: ", $v{'prefix'} || $ENV{'HOME'} .
"/psyced", 1);

$v{'prefix_sbin'} = get("Install binaries to: ", $v{'prefix_sbin'} || $v{'prefix'}.'/sbin');

$v{'hostname'} = get("Server host name: ", $v{'hostname'} || $ENV{'HOSTNAME'});

print "\r\n";
print BOLD "HINT:";
print " Your domain name. If your server is funny.bunny.com, that'll be bunny.com\r\n";
$v{'domainname'} = get("Domain name: ", $v{'domainname'});

print "\r\nIf you have a static IP, please tell me. Otherwise leave this field blank.\r\n";
$v{'ip'} = get("Server IP address: ", $v{'ip'});

print "\r\n";

unless ($>) {
    $v{'user'} = get("Install and run psyced as user: ", $v{'user'} || $ENV{'USER'});
    v{'group'} = get("Install and run psyced as group: ", $v{'group'} || $ENV{'GROUP'});

    print "\r\n";
}

print "Where do you want psyced runtime output? For manually started development\r\n";
print "servers choose 'console', for background daemon service use 'files'.\r\n";

$v{'log'} = get("Send server runtime output to: ", $v{'log'} || 'console',
		{ 'files' => 1, 'console' => 1 });

$v{'debug'} = get("Debug level (0..4): ", '1',
		    NUMFT(0, 4));

unless ($>) {
    $v{'init'} = get("Install a System V style init script? ", "n", YESNO);
}

print BOLD "\r\nPSYC SPECIFIC OPTIONS\r\n\n";

print<<X;
Set the PSYC identification for your server. e.g. funny.bunny.com.\r
If you are using dial-up internet, you can try out a few things, but\r
if you want this software to serve a serious purpose you need to have\r
a dynamic DNS address for this machine installed and provide it here.\r
\r
X

$v{'psyc_hostname'} = get('PSYC hostname: ', $v{'psyc_hostname'} || $v{'hostname'} . ($v{'domainname'} ? '.' . $v{'domainname'} : ''), 1);

print<<X;
\r
Now comes the best part. You get to decide which of the many\r
protocols and services that psyced provides you want to\r
activate. Since psyclpc doesn't have the the ability to run safely\r
as root all protocols use non-privilege port numbers.\r
We also mention the official privileged port numbers in case\r
you want to set up a firewall based port mapping.\r
\r
X

$v{'use_psyc'} = get('Enable PSYC (better say yes here)? ', $v{'yesno'} || 'y', YESNO);

$v{'port_psyc'} = get("Port number between 4400 and 4409: ", $v{'psyc_port'} || '4404', NUMFT(4400, 4409)) if $v{'use_psyc'} eq 'y';

print BOLD "\r\npsyced REGULAR PROTOCOL SERVICES\r\n\n";

my @prots = ( [ 'jabber_interserver', 
	        'XMPP communication with other JABBER servers',
		'y',
		'5269'
	      ],
	      [ 'irc',
	        'access for IRC clients',
	        'y',
		'6667',
		[ 6600, 6699 ]
	      ],
	      [ 'jabber_clients',
	        'access for JABBER clients (experimental)',
		'n',
		'5222',
		undef,
		[ 5222, 55222 ]
	      ],
	      [ 'smtp',
	        'SMTP reception server (experimental)',
		'n',
		'2525',
		[ 2500, 2599 ],
		undef,
		25
	      ],
	      [ 'pop3',
	        'POP3 server (experimental)',
		'n',
		'1100'
	      ],
	      [ 'nntp',
		'access for NNTP readers (experimental)',
		'n',
		'1199',
		[ 1190, 1199 ],
		undef,
		'119'
	      ],
	      [ 'telnet',
	        'telnet access',
		'y',
		'2323',
		[ 2300, 2399 ],
		undef,
		23
	      ],
	      [ 'http',
		'builtin HTTP daemon',
		'y',
		'44444',
		undef,
		undef,
		80
	      ],
	      [ 'applet',
	        'applet access',
		'y',
		'2008',
		[ 2006, 2009 ]
	      ]
	    );

foreach (@prots) {
    investigate_protocol(@$_);
}

store \%v, '.ser.data';

# now generate the psyced.ini ... TODO

1;
