# INSTALLATION
#
# - Add the statusbar item:
#    /statusbar window add typing_notice
#    You won't see anything until someone is typing.
#
# SETTINGS
#
# [typing_notice]
# send_typing = ON
#   -> send typing notifications to supported users
# 
# CHANGES
# 2008-01-11
# * threw out most bitlbee stuff
# * all XEP 0085 typing levels
# * working interop with 0085
# 2008-01-05
# * ctcp capab protocol - irssis clientinfo is buggy
# * renamed typing levels - inspired by XEP 0085
# * started to remove bitlbee stuff
# -----------------------------------------------
# fippoism starts
# this script is based on version 1.9.1 of 
# http://the-timing.nl/Projects/Irssi-BitlBee/typing_notice.pl
# the plan is to get compat to xmpp xep 0085 at least
# and of course psyc legacy ctcp support :-)
# -----------------------------------------------
#
use strict;
use Irssi::TextUI;
use Data::Dumper;

use vars qw($VERSION %IRSSI);

$VERSION = '0.2';
%IRSSI = (
    authors     => 'Philipp "fippo" Hancke',
    contact     => 'fippo@goodadvice.pages.de',
    name        => 'typing_notice_psyc',
    description => 'based on Tijmen\' typing notification script for bitlbee
		    1. Receiving typing notices: Adds an item to the status bar which says [typing] when someone is typing a message.
                    2. Sending typing notices: Sends CTCP TYPING messages to PSYC, XMPP and IRC users (If they support it)',
    license     => 'GPLv2',
    url         => 'http://www.psyced.org/',
    changed     => '2008-01-11',
);

my $debug = 0;

my %TIMEOUTS = (
    "PAUSED"	=> 5 * 1000,
    "INACTIVE"	=> 30 * 1000,
    "GONE"	=> 180 * 1000,
);


my %typers; # for storage

my $line;
my $lastkey;
my $keylog_active = 1;
my $command_char = Irssi::settings_get_str('cmdchars');
my $to_char = Irssi::settings_get_str("completion_char");

## IRC only ##############
# this is used to append a non-printable control sequence to all messages
# quite a smart hack indeed - but i would prefer ctcp for it
my $o = "\cO"; 
my $oo = $o.$o;
##########################

sub get_current {
	my $server = Irssi::active_server();
        my $window = Irssi::active_win();
	if ($server && $window) {
		return ($server->{tag}, $window->get_active_name());
	}
	return undef;
}

sub event_ctcp_msg { # called for ctcp msg, not ctcp replies
	my ($server, $msg, $from, $address) = @_;
	$server = $server->{tag};

	if ($msg =~ /TYPING (INACTIVE|PAUSED|COMPOSING|ACTIVE|GONE)/ ) {
		if ( not $debug ) {
		    Irssi::signal_stop();
		}
		# if someone sends this, they usually support that stuff
		$typers{$server}{$from}{capability} = 1;

		$typers{$server}{$from}{typing_in} = $1;
		Irssi::statusbar_items_redraw('typing_notice');
		Irssi::signal_stop();
	} elsif ( my($type) = $msg =~ /CAPAB TYPING/ ) {
		if ($debug) {
			print "capab typing query from $from.";
		}
		my $serverobj = Irssi::server_find_tag($server);
		$serverobj->ctcp_send_reply("NOTICE $from :\001CAPAB TYPING\001");
		Irssi::signal_stop();
	}
}

sub event_ctcp_reply { # called for ctcp replies
	my ($server, $msg, $from, $address) = @_;
	if ( $msg =~ /CAPAB TYPING/ && exists( $typers{$server->{tag}}{$from} )) {
	    if ($debug) {
		print "capab typing reply from $from.";
	    }
	    $typers{$server->{tag}}{$from}{capability} = 1;
	    Irssi::signal_stop();
	}
}

sub unset_typing_in {
	my ($a) = @_;
	my ($server, $nick) = @{$a};
	if ($debug) {
		print "unset: $server, $nick";
	}
    	$typers{$server}{$nick}{typing_in} = undef;
    	Irssi::timeout_remove($typers{$server}{$nick}{timer_tag_in});
    	Irssi::statusbar_items_redraw('typing_notice');
}

sub event_msg {
	my ($server, $data, $nick, $address, $target) = @_;
	$server = $server->{tag};

#	haeh??? ist das eine art <active/> angehaengt als leeres ctcp
	if ( $data =~ /$oo\z/ ) {
		if ( not exists( $typers{$server}{$nick} ) ) {
			$typers{$server}{$nick}{capability} = 1;
			if ($debug) {
				print "This user supports typing! $server, $nick";
			}
		}
	} elsif (0) { # ah... this ensures that it stays valid
		if ( exists( $typers{$server}{$nick} ) ) {
			if ($debug) {
				print "This user does not support typing anymore! $nick. splice: ";
			}
			delete $typers{$server}{$nick};
		}
	}
	
	if ( 0 and exists( $typers{$server}{$nick} ) ) {
		unset_typing_in( [$server, $nick] );
	}
}

sub typing_notice { ## redraw statusbar item
	my ($item, $get_size_only) = @_;
	my ($server, $channel) = get_current();

	return unless exists $typers{$server}{$channel};

	if ( $typers{$server}{$channel}{typing_in} ne undef ) {
		my $append = $typers{$server}{$channel}{typing_in};
		$item->default_handler($get_size_only, "{sb typing $append}", 0, 1);
		if ($debug >= 2) {
			print "typing: $server, $channel.";
		}
	} else {
		if ($debug) {
			print "clear: $server, $channel ";
		}
		$item->default_handler($get_size_only, "", 0, 1);
		if ($typers{$server}{$channel}{timer_tag_in} ne undef) {
			Irssi::timeout_remove($typers{$server}{$channel}{timer_tag_in});
			$typers{$server}{$channel}{timer_tag_in} = undef;
		}
    	}
}

sub window_change {
	Irssi::statusbar_items_redraw('typing_notice');
	my ($server, $channel) = get_current();
	
    	if ( exists( $typers{$server}{$channel} ) ) {
        	if ( not $keylog_active ) {
            		$keylog_active = 1;
            		Irssi::signal_add_last('gui key pressed', 'key_pressed');
        	}
    	} else {
        	if ($keylog_active) {
            		$keylog_active = 0;
            		Irssi::signal_remove('gui key pressed', 'key_pressed');
        	}
    	}
	return if not Irssi::settings_get_bool("send_typing");
#	send INACTIVE?
}

sub window_close {
	return if not Irssi::settings_get_bool("send_typing");
	my ($server, $channel) = get_current();
	if ( exists( $typers{$server}{$channel} ) and $typers{$server}{$channel}{state_out} ne "GONE" ) {
		my $serverobj = Irssi::server_find_tag($server);
		$serverobj->command("^CTCP $channel TYPING GONE");
		$typers{$server}{$channel}{state_out} = "GONE";
	}
}

sub window_open {
	return if not Irssi::settings_get_bool("send_typing");
	my ($item) = @_;
	# look if we should disco
}

sub key_pressed {
	return if not Irssi::settings_get_bool("send_typing");
    	my $key = shift;
    	if ($key == 9 && $key == 10 && $lastkey == 27 && $key == 27 && $lastkey == 91 && $key == 126 && $key == 127) { # ignore these keys
		$lastkey = $key;
		return 0;
	}
        
	my ($server, $channel) = get_current();

	if ( exists( $typers{$server}{$channel} ) ) {
	    
              	my $input = Irssi::parse_special("\$L");
		if ($input !~ /^$command_char.*/ && length($input) > 0){
		    send_typing( $server, $channel );
		}
    	}
    	$lastkey = $key; # some keys, like arrow-up, contain two events. 
}


sub send_typing_pause {
	return if not Irssi::settings_get_bool("send_typing");
	my ($a) = @_;
    	my( $server, $nick ) = @{$a};
	send_typing_update_state($server, $nick, "PAUSED");

	Irssi::timeout_remove($typers{$server}{$nick}{timer_tag_out});
	$typers{$server}{$nick}{timer_tag_out} = Irssi::timeout_add_once($TIMEOUTS{INACTIVE}, 'send_typing_inactive', [$server, $nick]);
    	
}

sub send_typing_inactive {
	return if not Irssi::settings_get_bool("send_typing");
	my ($a) = @_;
    	my( $server, $nick ) = @{$a};
	send_typing_update_state($server, $nick, "INACTIVE");

	Irssi::timeout_remove($typers{$server}{$nick}{timer_tag_out});
	$typers{$server}{$nick}{timer_tag_out} = Irssi::timeout_add_once($TIMEOUTS{GONE}, 'send_typing_gone', [$server, $nick]);
    	
}

sub send_typing_gone {
	return if not Irssi::settings_get_bool("send_typing");
	my ($a) = @_;
    	my( $server, $nick ) = @{$a};
	send_typing_update_state($server, $nick, "GONE");
	Irssi::timeout_remove($typers{$server}{$nick}{timer_tag_out});
}

sub send_typing {
	my ( $server, $nick ) = @_;

	send_typing_update_state($server, $nick, "COMPOSING");
	        
	Irssi::timeout_remove($typers{$server}{$nick}{timer_tag_out});
	$typers{$server}{$nick}{timer_tag_out} = Irssi::timeout_add_once($TIMEOUTS{PAUSED}, 'send_typing_pause', [$server, $nick]);
}

sub send_typing_update_state {
	return if not Irssi::settings_get_bool("send_typing");
	my ( $server, $nick, $state ) = @_;
	if (not exists($typers{$server}{$nick}) or $typers{$server}{$nick}{capability} != 1) {
	    print Dumper(%typers);
	    print "(dont) send_typing_update_state to non-typer $server->$nick"; #if ($debug);
	    return 0;
	 }

	if ($state eq $typers{$server}{$nick}{state_out}) {
	    print "(dont) send_typing_update_state: $state already known" if $debug;
	    return;
	}
	# FIXME: allowed state transitions could be checked here
	if ($debug) {
	    print "$server: ctcp $nick typing $state";
	}

	my $serverobj = Irssi::server_find_tag($server);
	if (!$serverobj) {
	    print "send typing update state: server not found";
	    return;
	}
	$serverobj->command("^CTCP $nick TYPING $state");

	$typers{$server}{$nick}{state_out} = $state;
}

sub db_typing { 
	print "------ Typers -----\n".Dumper(%typers);	
}

sub event_send_msg { # outgoing messages
	my ($msg, $server, $window) = @_;
	return unless $window and $window->{type} eq "QUERY";
	my $nick = $window->{name};

	if ($debug) {
		print "send msg: $server->{tag}, $nick";
	}
	if ( exists($typers{$server->{tag}}{$nick}) and 
		     $typers{$server->{tag}}{$nick}{capability} == 1) {
		$typers{$server->{tag}}{$nick}{state_out} = "ACTIVE";
		Irssi::timeout_remove($typers{$server->{tag}}{$nick}{timer_tag_out});	
	}

	if (!exists( $typers{$server->{tag}}{$nick} ) ) {
		if ($debug) {
		    print "send capa query to $nick.";
		}
		$typers{$server->{tag}}{$nick}{capability} = -1;
		if (my $serverobj = Irssi::server_find_tag($server->{tag})) {
        	    $serverobj->command("^CTCP $nick CAPAB TYPING");
		}
	}

	if ( 0 and length($msg) > 0) {
#		ist das eine art <active/>
		$msg .= $oo;
	}
	
	Irssi::signal_stop();
	Irssi::signal_remove('send text', 'event_send_msg');
	Irssi::signal_emit('send text', $msg, $server, $window);
	Irssi::signal_add_first('send text', 'event_send_msg');
}

# Command
Irssi::command_bind('db_typing','db_typing');

# Settings
Irssi::settings_add_bool("typing_notice","send_typing",1);

# IRC events
Irssi::signal_add_first("send text", "event_send_msg"); # Outgoing messages
Irssi::signal_add("ctcp msg", "event_ctcp_msg");
Irssi::signal_add("ctcp reply", "event_ctcp_reply");
Irssi::signal_add("message private", "event_msg");
Irssi::signal_add("message public", "event_msg");

# GUI events
Irssi::signal_add_last('window changed', 'window_change');
Irssi::signal_add_last('window destroyed', 'window_close');
Irssi::signal_add_last('window created', 'window_open');
Irssi::signal_add_last('gui key pressed', 'key_pressed');

# Statusbar
Irssi::statusbar_item_register('typing_notice', undef, 'typing_notice');
Irssi::statusbars_recreate_items();
