#include <net.h>
#define	NAME		"IRC"

// gatebot to ircnet
//
// just put this file into init.ls if you want it to
// activate at boot time. addressing irc:nickname
// should work also when this isn't loaded however.
//
//#define	CONNECT_IRC	"irc.freenet.de"	// an ircnet server
//#define	CONNECT_IRC	"us.ircnet.org"
#define		CONNECT_IRC	"irc1.us.ircnet.net"
//
// other example configuration
//
//#define	CONNECT_IRC	"test.example.org", 6777
//#define	CHAT_CHANNEL	"PSYC"			// enter #PSYC
//#define	PASS_IRC	"wooboowaha"		// server password
//#define	IRC_HIDE
//
// Some networks have unusual policies, thus sometimes hacks were
// necessary, like so:
//
//#define ON_CONNECT	emit("PRIVMSG NickServ :IDENTIFY " PASSWORD "\n");
//#define ON_CONNECT	call_out("emit", 9, "JOIN :#" CHAT_CHANNEL "\n");

#include <place.gen>

// See also http://about.psyc.eu/gateway
//
// To change the nickname of the gatebot you need to #define IRCGATE_NICK
// in your local.h, not here. It applies to all gatebots on this server.

