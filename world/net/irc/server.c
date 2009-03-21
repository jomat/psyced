// $Id: server.c,v 1.86 2008/10/16 13:07:13 lynx Exp $ // vim:syntax=lpc
//
// IRC protocol receptionist
//
#define MYNICK "*"

#include "irc.h"	// inherits net/irc/common
#include "server.h"	// inherits net/server
#include "error.h"	// gets numeric codes
#include "reply.h"	// gets numeric codes

//#define emit(bla) binary_message(replace(bla, "\n", "\r\n"))
#include "hack.i"

volatile string login;
volatile string host;
volatile string name;
volatile string pass;

// for now it's a bit complicated to re-issue all joins and i'm not
// even sure if it would be enough for the irc clients.. so better
// just quit the old object and start anew. a bit silly when user
// objects have just been incarnated and never actually logged in -
// for each ircer we thus incarnate twice. FIXME
keepUserObject(user) { return 0; }

qScheme() { return "irc"; }

qName() { return "*"; } // MYNICK?	no i think i tried that before
			// 		and it didn't work

void create() {
	unless (clonep()) return;
	sTextPath(0, 0, "irc");
}

createUser(nick) {
	return named_clone(IRC_PATH "user", nick);
}

parse(a) {
	::parse(a);
	if (ME) next_input_to(#'parse);
}

// allow for nickserv syntax somehow?
promptForPassword() {
	// string me;
	//
	// that's too complicated for dirty coded clients
	// if (me = user->vQuery("me")) p(nick+" "+me);
	unless (pass) w("_query_password", 0, ([ "_nick": MYNICK ]) );
	return 1;
}

#ifdef PRO_PATH
# define BOUNCE_D	NET_PATH "d/bounce"
#else // grummel
# define BOUNCE_D	DAEMON_PATH "bounce"
#endif
morph() {
// das zeug hier gehört irgendwann mal in die textdb, eilt nich..
    if (user && interactive(user) && BOUNCE_D->checkBounce(nick)) {
	write("ERROR :Closing Link: " + nick
	      + " (You've been bouncing in and out (2 or more clients try "
	      "to connect to this server using your identity). Please "
	      "stop all clients except the one you want to use for "
	      "chatting and (if needed) try to connect again in two "
	      "minutes. Thanks.)\n");
	quit();
	return;
    }

#ifdef IRCPLUS
	string lynx="/"; lynx[0] = 0x1c;    // ctrl-backslash

# define SUPPORTS ":" SERVER_HOST " 800 "+nick+" IDENTITY :"+ \
       	"password"+ lynx +"email optional\n"
#else
# define SUPPORTS
#endif
	// feeps says mIRC extracts its public ip from the 001 message
	write("\
:" SERVER_HOST " 001 "+nick+" :Connected to an IRC emulation daemon on the [PSYC] network, "+ nick +"@"+ query_ip_name() +"\n\
:" SERVER_HOST " 002 "+nick+" :Your host is " SERVER_HOST " running version " SERVER_VERSION "\n\
" SUPPORTS "\
:" SERVER_HOST " 004 "+nick+" " SERVER_HOST " " SERVER_VERSION " * *\n");
// :" SERVER_HOST " 003 "+nick+" :This server was created long before you were born.\n\
	// 005 is issued by user.c
	unless (::morph()) return;
}

ircMsg(from, cmd, args, text, all) {
	switch(cmd) {
case "server":
		// welcher client tut das senden?
		// eben kein client, sondern ein server mit C/N lines
		write("402 :No server connectivity implemented.\n");
		quit();
		return 1;
case "user":
D1(case "u":)
		if (sscanf(args, "%s %s %s", login, host, name) < 3) {
			//reply(ERR_NEEDMOREPARAMS,
			//	":Incorrect number of parameters");
		}
// TODO: why is it using THIS syntax here?
		login = "irc:"+login+"@"+host;
		// unfortunately we have to go by USER since
		// NICK may be reissued in a new connection and
		// a password query would interfere with the PASS
		// which is already coming our way before we know
		if (nick) {
			D2( D("irc:calling hello from USER\n"); )
			hello(nick, login, pass);
		}
		return 1;
case "oper":
case "pass":
D1(case "p":)
		pass = args || text;
		// if (nick) password(args);
		if (nick && pass) {
			D2( D("irc:calling hello from PASS\n"); )
			hello(nick, login, pass);
		}
		return 1;
case "nick":
D1(case "n":)
                // bug in some clients like ircg.. they send NICK :nick
		nick = args || text;
		if (nick && (pass || login)) {
			D2( D("irc:calling hello from NICK\n"); )
			hello(nick, login, pass);
		}
		return 1;
	}
	return ::ircMsg(from, cmd, args, text, all);
}

quit() {
	destruct(this_object());
	return 1;
}

w(mc, data, vars, a,b,c,d) {
	emit(psyc2irc(mc, 0) +" "+MYNICK+" :");
	return ::w(mc, data, vars, a,b,c,d);
}

tls_logon(a) {
	PT(("%O tls_logon\n", ME))
//	PT(("tls_certificate info:\n%O\n", tls_certificate(ME, 1)))
	next_input_to(#'parse);
	::logon(a);
	return 1;
}

logon(failure) {
	if (this_interactive()) set_prompt(""); // case of failure?
	next_input_to(#'parse);
#if __EFUN_DEFINED__(tls_query_connection_state)
	if (tls_query_connection_state(ME) == 0) {
	    // DONT ::logon if this is to be done by tls_logon
	    ::logon(failure);
	}
#else
	::logon(failure);
#endif
#ifdef _flag_log_sockets_IRC
	log_file("RAW_IRC", "\nnew connection %O from %O\n",
		 ME,
# ifdef _flag_log_hosts
		 query_ip_name()
# else
		 "?"
# endif
		 );
#endif
	return 1;
}

