// $Id: server.c,v 1.54 2008/05/05 14:31:10 lynx Exp $ // vim:syntax=lpc
//
// THE GENERIC SERVER
// this code is not applicable to a protocol scheme directly
// although it's connection-oriented, base for telnet/applet/irc-schemes

// local debug messages - turn them on by using psyclpc -DDserver=<level>
#ifdef Dserver
# undef DEBUG
# define DEBUG Dserver
#endif

#include <net.h>
#include <person.h>
#include <text.h>
#include <lang.h>

#define NO_INHERIT
#include <server.h>
#undef NO_INHERIT

volatile string nick;
volatile object user;
volatile int guesses;

// just a clean-up to avoid ugly warnings when net/server and
// net/jabber/* use differing flags in input_to calls.
volatile int input_to_settings = INPUT_IGNORE_BANG;

qScheme() { return 0; }
qLayout() { return 0; }
qLang() { return DEFLANG; }

sName(ni) { nick = ni; }

// prototypes
morph();
promptForPassword();
authChecked();


void create() {
	if (!clonep()) return;
	sTextPath(qLayout(), qLang(), qScheme());
}

// supercede this in inheriting server class
createUser(nick) {
	P3(("%O net/server:createUser(%O)\n", ME, nick))
	return named_clone(NET_PATH "user", nick);
}

// returns 1 if password prompt is necessary  - who needs that?
// see also morph.i
hello(ni, elm, try, method, salt) {
        string master_nick;
        object master_user;
#ifdef LOGIN_BY_EMAIL
	if (nick = legal_mailto(ni)) {
		elm = 0;
		ni = "mailto:"+ nick;
		P0(("Login by mailto for %O\n", ni))
	} else
#endif
	unless(nick = legal_name(ni)) {
		w("_error_illegal_name_person", 0, ([ "_nick": ni ]) );
		QUIT
	}

	// this assumes person is not switching schemes (protocols)!
	// if it does, we'll attach a port to the wrong protocol!
	// apparently it never happens.
	unless (user = find_person(nick)) {
		// catch returns "1" here, weird!!
		//if (catch (user = createUser(nick)) && !user ) {
		unless (user = createUser(nick)) {
		  //	pr("_failure_object_creation",
		  //	    "Cannot create user object.\n");
			QUIT
		}
	}
        if (nick)
          master_nick=explode(nick,"+")[0]
          ,master_user=find_person(master_nick); 
        else
          master_user=user;

        if (!master_user) {
          // by now we need an already existing unaliased object of the user
          // which isn't created on the fly yet
          // TODO: either: notify the client about the problem
          //       recommended or: spawn a non-interactive user-object which
          //                       relays the messages to the aliases
          QUIT
        }
	return master_user -> checkPassword(try, method, salt, 0, #'authChecked, 
				     ni, try, elm);
}

#ifdef REGISTERED_USERS_ONLY	// TODO: rename into a _flag
ohYeah(whatever) {
	input_to(#'ohYeah, input_to_settings);
	// input ignore warning? inverting mc's is really a good idea!
	w("_warning_ignored_input", "Oh yeah? ");
}
#endif

authChecked(int result, ni, try, elm) {
	P3(("authChecked(%O,%O,%O,%O) in %O",
	    result, ni, try, elm, ME))
	unless (result) {
		int invalid;

		// überzeugt mich nicht so richtig..
		// der ip check da unten sollte vorher passieren  TODO
		if (try && elm) {
		    string *rc;

		    unless (user -> isNewbie()) {
			    w("_error_unavailable_name",
	"Sorry, this name is registered to somebody else.\n");
			    invalid = 1;
		    }
		    else if (rc = legal_password(try, nick)) {
			    pr(rc[0], rc[1]);
			    invalid = 1;
		    }
		    else unless (elm = legal_mailto(elm)) {
			    w("_error_invalid_mailto",
	"Sorry, that doesn't look like a valid email address to me.\n");
			    invalid = 1;
		    }
//		    if (user -> vQuery("email") != elm) {
//			    w("_error_invalid_mailto",
//	"Sorry, that doesn't look like a valid email address to me.\n");
//			    return;
//		    }
		}
#ifdef REGISTERED_USERS_ONLY
		else {
			if (user -> isNewbie()) {
#ifdef PSYC_SYNCHRONIZE
				synchro_report(	// _warning? doesn't get forwarded by @sync :(
				    "_notice_error_necessary_registration_sign_on",
				 "Login attempt by unregistered user [_nick].",
				      ([ "_nick": nick ]) );
#endif
				w("_error_necessary_registration",
		"Sorry, you cannot use this without prior registration.");
				// this funny hack intentionally breaks
				// the login procedure. the connection
				// stays up in the air until the time-out.
				input_to(#'ohYeah, input_to_settings);
				// would be better to count the login
				// attempts per host however.
			} else {
				unless (try) return promptForPassword(user);
				w("_error_invalid_password",
				    "Invalid password for [_nick].",
					    ([ "_nick": nick ]) );
			}
			invalid = 1;
		}
		if (invalid) {
			input_to(#'hello, input_to_settings);
			return;
		}
		user -> set("email", elm);		// TODO!?!?
		user -> set("password", try);
		if (ni != nick) user->vSet("longname", ni);
		return morph();
#else
		return promptForPassword(user);
#endif
	}
#ifndef REGISTERED_USERS_ONLY
	// added user->isNewbie() check for ircers
	if (user->online() && user->isNewbie()
#ifdef _flag_log_hosts
            && user->vQuery("ip") != query_ip_number()
#endif
            ) {
		w("_error_status_person_connected", 0, ([ "_nick": nick ]) );
		QUIT
	}
#endif
	if (ni != nick) user->vSet("longname", ni);
	return morph();
}

password(try, method, salt) {
	mixed authCb;
        string master_nick;
        object master_user;

        if (nick)
          master_nick=explode(nick,"+")[0]
          ,master_user=find_person(master_nick); 
        else
          master_user=user;

	unless (user) {
		w("_failure_object_destructed",	    // never happens
		    "Huh? Your (master)user object has disappeared!");
		QUIT
	}
	// nick, guesses needed?
	authCb = CLOSURE((int result), (nick, guesses), 
			 (string nick, int guesses), 
	    {
		unless(result) {
		    w("_error_invalid_password", 
		      "Invalid password for [_nick].",
		      ([ "_nick": nick ]) );
		    if (++guesses > 3) { QUIT }
		    return promptForPassword(user);
		}
		return morph();
	    });
	master_user -> checkPassword(try, method, salt, 0, authCb);
}

// supercede this in inheriting server class
promptForPassword() {
	// the no-echo option is a negotation within telnet protocol
	// therefore unsuited for most other cases
	input_to(#'password, input_to_settings);
	return 1;
}

morph() {
	P2(("morph %O to %O\n", ME, user))
	if (! keepUserObject(user)) {
		user -> quit(2);
		if (user) destruct(user);

		unless (user = createUser(nick)) {
			w("_failure_object_creation",
			    "Cannot create new user object.");
			QUIT
		}
	}
	if (interactive(user)) remove_interactive(user);
	if (exec(user, ME)) userLogon();
	else {
		pr("_failure_object_switch",
			"Eh! Could not switch to object %O.\n", user);
		QUIT
	}
	destruct(ME);
	return 1;
}

userLogon() {
	P3(("userLogon %O to %O\n", ME, user))
	return user -> logon();
}

// supercede this in inheriting server class
keepUserObject(user) {
	if (qScheme()) return user->vQuery("scheme") == qScheme();
}

quit() { QUIT }		// self-destruct on timeout or external request

// self-destruct when the TCP link gets lost
disconnected(remaining) {
	P2(( "%O got disconnected\n", ME ))
    	QUIT	// returns 'unexpected'
}

logon() {
	if (nick) {
		// authlocal support!
		// only auto-login the first instance of nick
		user = find_person(nick);
		unless (user) {
			user = createUser(nick);
			morph();
			return 0;
		}
		else unless(interactive(user)) {
			morph();
			return 0;
		}
		// otherwise prompt regularely
		nick = user = 0;
	}
#ifdef LIMIT_USERS
	// TODO: admins MUST be able to login!
	if (AMOUNT_SOCKETS >= LIMIT_USERS) {
		w("_failure_exceeded_limit_users", "Extremely sorry, but "
		   "the maximum possible amount of people has been reached.");
		QUIT
	}
#endif
	input_to(#'hello, input_to_settings);
	call_out(#'quit, TIME_LOGIN_IDLE);
	guesses = 0;
	return 1;
}

