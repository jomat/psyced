# oh how nice.. just like ircII 2.1.5's /set command_mode
# which unfortunately no longer works in modern ircIIs.
# i think it is mentioned in the historic documents
# on http://about.psyc.eu/IRC					-lynX

use strict;
use vars qw($VERSION %IRSSI);

use Irssi;
$VERSION = '200412171';
%IRSSI = (
    authors     => 'MB',
    contact     => 'mb',
    name        => 'command_mode',
    description => 'interpretes everything sent to a channel as a command',
    license => 'GPL',
);

sub send_text {

    #"send text", char *line, SERVER_REC, WI_ITEM_REC
    my ( $data, $server, $witem ) = @_;
    if ( $witem
        && ( $witem->{type} eq "CHANNEL"
	     || $witem->{type} eq "QUERY" ) )
    {
        $witem->command('/^say ' . $data);
        Irssi::signal_stop();
    } elsif ($witem && $witem->{type} eq 'STATUS') {
	$witem->command('/quote ' . $data);
    } elsif (!$witem) {
	$server->command('/quote ' . $data);
    }
}

Irssi::signal_add 'send text' => 'send_text';
