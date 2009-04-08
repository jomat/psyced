// example configuration for a server-level gateway to an ircnet
// called "ircgate" running on localhost.
//
// this is not a C file. this is a room definition for psyced.
// it belongs into psyced's "place" directory.
//
// to make this work you need to add the following to local.h:
//
//	#define RELAY		"ircgate"	    // name of ircnet (?)
//	#define RELAY_OBJECT	"place/ircgate"	    // this class
//
// do not use in existing psyced communities in the current set up
// as currently all incoming traffic is presumed to be meant for the
// ircnet. a future version of this ircgate will let psyc users and
// ircnet users coexist peacefully on a single psyced installation.
//
// configuring the ircgate into an ircd. add the following two lines:
//
//	C:127.0.0.1:pw:PSYC.EU::51
//	N:127.0.0.1:pw:PSYC.EU::51
//
// yes, the server name is "PSYC.EU" here, but you can define what it
// should be further below. syntax details may vary for your ircd
// implementation, but if somebody took the time to patch your ircd flavor,
// (s)he probably also published syntax details on how to configure it.
// of course pick a better password than 'pw'
//
// TESTING: start up psyced, wait until it has linked with the ircnet
// then from the ircnet issue
//
//	/m <uniform> hello
//
// the uniform can be anything psyced can handle, most notably psyc:
// and xmpp: urls

#include <net.h>

// pasword belongs into an unreadable config file like admins.h
#define IRCGATE_NAME	"PSYC.EU"
#define IRCGATE_LOCAL	"pw"

#define	NAME		"IRCgate"
#define ON_CONNECT	onConnect();
#define	CONNECT_IRC	"localhost", 7000
#define EMULATE_SERVER	// don't be a bot

#include <place.gen>

onConnect() {
	PT(("ircgate serving into "+query_ip_number()+"\n"))
	// login procedure may vary with different flavors of ircd,
	// that's why we keep it entirely here
	emit("PASS "+ IRCGATE_LOCAL +" TS\r\n"
	     "SERVER " IRCGATE_NAME " 1 " SERVER_VERSION
		     " :psyced.org http://about.psyc.eu/gateway\r\n"
	     "LUSERS\r\n");
// myself as pseudo subserver? needs extra hub config on ircd.
//	     "SERVER " SERVER_HOST " 2 " SERVER_VERSION
//		     " :psyced.org http://about.psyc.eu/gateway\r\n"
}

// testing:
#if 0

PASS pw TS
SERVER x.x 1 telnet/44.04 :telnet
PONG :base.psyc.eu
NICK JACK 1 1163580418 +i lynx 127.0.0.1 127.0.0.1 x.x :Get psyced.
:JACK PRIVMSG psyc://beta.ve.symlynx.com/~lynx :test

#endif
