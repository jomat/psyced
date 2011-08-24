//						    vim:noexpandtab:syntax=lpc
// $Id: usercmd.i,v 1.699 2008/12/27 00:42:04 lynx Exp $
//
// usercmd is normally just a part of the user object focused on implementing
// the user command set. it can also be used for implementing a subset of
// commands for remote irc and jabber users etc..
//
// how to use usercmd.i without being a user object (for gateways and bots):
//
// the command chars are handled by input(). you can call cmd() directly, but
// you still have to #define them to avoid the creation of unnecessary vars
//
//   #if EITHER_THIS1
//      #define cmdchar '/'
//      #define actchar ':'
//   #else
//      // if you want these to be variable, you must set them
//      cmdchar = '/';
//      actchar = ':';
//   #endif
//
// usercmd needs to know your name. pick a solution of your choice.
//
//   #if EITHER_THIS2
//	#undef MYNICK
//	#define MYNICK	<yournicknamevariable>
//   #else
// 	inherit NET_PATH "name";
//   #endif
//
// usercmd also needs to use your object or uniform. put it into
// a variable or #define called origin.
//
// now include the code and have fun!
//
//   #undef USER_PROGRAM
//   #include NET_PATH "usercmd.i"

#include <lang.h>
#include <closures.h>
#include <driver.h>
#include <misc.h>
#include <uniform.h>
#include <services.h>
#include <status.h>

#ifndef cmdchar
volatile mixed cmdchar;
#endif
#ifndef actchar
volatile mixed actchar;
#endif

#ifdef USER_PROGRAM
# define origin ME
volatile string more;
volatile mapping sayvars;
# define NICKPLACE	v("place")
#endif

#ifndef MAX_SUBS
# define MAX_SUBS 19 // allows 20 subscriptions
#endif

// prototype
msg(mixed source, mc, data, mapping vars, showingLog);
// dest fails when it is a local psyc: uniform, should prolly fix that.
// also, should _request_input be permitted to pass the object here instead,
// since it has already looked it up with psyc_object() ? TODO
input(a, dest) {
	P2(("»»> input(%O, %O) in %O\n", a, dest, ME))
#ifdef USER_PROGRAM
	vSet("aliveTime", time());
# if __EFUN_DEFINED__(convert_charset)
	if (v("charset") && v("charset") != SYSTEM_CHARSET
#  if SYSTEM_CHARSET == "UTF-8"
                // this still does not handle the special case when
                // a secondary psyc link is submitting a command
                // (but we don't have any such tools)
                // while the main client is irc or tn.. FIXME
            && v("scheme") != "psyc" && v("scheme") != "jabber"
#  endif
            ) {
#ifdef TRANSLIT
		iconv(a, v("charset"), SYSTEM_CHARSET);
		P4(("input from %O = %O\n", v("charset"), a))
#else
		mixed res;

		if (catch(res = convert_charset(a, v("charset"),
		    SYSTEM_CHARSET); nolog)) {
			P1(("catch! input iconv in %O\n", ME));
		}
                P4(("input from %O = %O\n", v("charset"), res))
                if (!res || res == -1) vDel("charset");
                else a = res;
#endif
	}
# endif
#endif
	// fix newlines in input
//	if (member(a, '\n') != -1) a = replace(a, "\n", " ");
	// irc-style commands
	unless(a) return 1;
	if (a[0] == cmdchar) {
		parsecmd(a[1..], dest);
		return 1;
	}
#ifndef _flag_disable_character_action
	// mud-style emote
	if (a[0] == actchar) {
	    if (strlen(a) > 3) {
		// new :s possessive emote syntax
		if (a[1] == 's' && a[2] == ' ') {
		    action(a[3..], dest, "_possessive");
		    return 1;
		}
		if ( (a[1] <= 'z' && a[1] >= 'a')
		    || (a[1] <= 'Z' && a[1] >= 'A')
		// very teutonic here.. but ldmud has no locale support
//		    || a[1] == 'ä' || a[1] == 'Ä'
//		    || a[1] == 'ü' || a[1] == 'Ü'
//		    || a[1] == 'ö' || a[1] == 'Ö'
		// doesn't work since we have utf8 now
		    ) {
			action(a[1..], dest);
			return 1;
		}
	    }
	}
#endif
#ifdef USER_PROGRAM
	// check for irc on every input? wär schöner wenn wir anders
	// sicherstellen dass die variable nie gesetzt wird.  -lynx
	if (qHasCurrentPlace() && v("query")) { // && v("scheme") != "irc") {
		query = tell(v("query"), a, query);
		return 1;
	}
	speak(a, dest);
	return 1;
#else
	return 0;
#endif
}

parsecmd(command, dest) {
	array(string) args;
	string a;

	P3(("»»> parsecmd(%O, %O) in %O\n", command, dest, ME))
	unless (command)
	    return 0;
#if __EFUN_DEFINED__(trim)
# include <sys/strings.h>
//# define TRIM_RIGHT 2
	command = trim(trim(command, TRIM_RIGHT), TRIM_LEFT, cmdchar);
	// did trimming leave us with nothing?
	unless (strlen(command)) return 0;
#else
	// strip leading slashes
	while (command[0] == cmdchar) command = command[1..];
	// strip trailing spaces
	while (command[<1] == ' ') command = command[0..<2];
#endif
	// "/ blah" notation for speaking within query
	if (command[0] == ' ') {
#ifdef USER_PROGRAM
		speak(command[1..], dest);
		return 1;
#else
		return 0;
#endif
	}
	// strip extra whitespace
	args = explode(command, " ");
	a = lower_case(args[0]);
	// catch mistakes like "/m  joe hi"
	if (sizeof(args)>1 && args[1] == "") args = args[1..];

//#ifdef USER_PROGRAM
//	if (function_exists("extracmd", ME)) {
//		if (extracmd(a, args, dest)) return 1;
//	}
//#else
//	PT(("%O usercmd: %O %O in %O\n", MYNICK, a, args, place))
//#endif
	return cmd(a, args, dest, command);
}

// command, *args, querynick, deprecated: unparsedcmdstring
cmd(a, args, dest, command) {
	object ob;
	mixed t = 0, t2 = 0, t3 = 0; // duh. they'll be 0 anyways, so..?

	P3(("»»> cmd(%O, %O, %O) in %O\n", a, args, dest, ME))
	switch(a) {
#ifdef RELAY
	case "h":
	case "help":
# ifdef WEBMASTER_EMAIL
		w("_info_administration_gateway",
		  "This gateway is operated by " WEBMASTER_EMAIL);
			// " on "+ SERVER_UNIFORM);
# endif
	// ARGH MULTILINE!!
		w("_info_commands_gateway",
 "Commands: friend, unfriend, show, shout, whois, surf, avail, mood.\n"
 "Manual is provided on [_page_help_gateway]",
		    ([ "_command": a, "_page_help_gateway":
			     "http://about.psyc.eu/Gateway" ]) );
		return 1;
#else
	case "act":
	case "action":
                if (sizeof(args) != 2 || !place)
                    w("_failure_unsupported_command_action",
                        "The /act command has been removed. Simply type /<action> instead.");
                else act(args[1], place, origin);
                break;
	case "i":
	case "inv":
	case "invite":
		if (sizeof(args) != 2) return
			w("_warning_usage_invite", "Usage: /invite <person>");
		unless (place) return
			w("_error_status_place_none", "You aren't in a room");
		return invite(args[1]);
	case "me":
	case "emote":
		action(ARGS(1), dest);
		break;
	case "my":
	case "mein":
	case "meine":
		action(ARGS(1), dest, "_possessive");
		break;
	case "cls":
	case "clear":
	case "clearscreen":
		w("_warning_usage_clearscreen",
      "The way to clear the screen is to just enter an empty input line.");
		return 1;
# if DEBUG > 0
	case "error":
		raise_error("manually created test error\n");
		return 1;
# endif
#endif
#ifndef _flag_disable_query_server
	case "na":
	case "names":
	case "people":
	//se "ppl":
	case "p":
		people();
		break;
	case "lusers":
#ifndef GAMMA
		if (sizeof(args) > 1 && is_formal(args[1])) {
		    sendmsg(args[1], "_request_user_amount", 
			    "[_nick] would like to know how many users you have.",
			    ([ "_nick" : MYNICK ]), 0, origin, #'msg);
		} else {
		    // wozu selber erzeugen? der code oben würde/sollte das an
		    // rootMsg() schicken, welches auch ein _status_user_amount
		    // erzeugen sollte.. no? ausserdem sind die vars und
		    // das fmt unterschiedlich. und fast derselbe käse steht
		    // nochmal in net/irc/common... TODO
		    w("_status_user_amount", 0,
			 ([ "_amount_users_loaded" : amount_people() ]) );
		}
#else
		if (sizeof(args) > 1 && is_formal(args[1])) t = args[1];
		else t = "/";
		sendmsg(t, "_request_user_amount", 
		    "[_nick] would like to know how many users you have.",
		    ([ "_nick" : MYNICK ]), 0, origin, #'msg);
#endif
		break;
# ifdef __BOOT_TIME__
        case "up":
        case "uptime":  
        case "boottime":
		t = __BOOT_TIME__;
		w("_info_time_boot", "[_source_host] up since [_time_boot] "
				    "([_time_boot_duration]).\nLoad average: "
				  "[_system_load_average].", ([
				    "_source_host": SERVER_HOST,
				    "_time_boot": isotime(t, 1),
				    "_time_boot_duration": timedelta(time()-t),
				    "_system_load_average":
					 query_load_average() ]) );
		return 1;
# endif          
	case "w":
	case "who":
# ifdef USER_PROGRAM
		if (sizeof(args) > 1) examine(args[1]);
		else
# endif
		    who();
		return 1;
# ifdef USER_PROGRAM
	case "disco":
		if (sizeof(args) > 1 && is_formal(args[1])) {
		    sendmsg(args[1], "_request_list_feature",
			    "[_nick] would like to know what features you offer.",
			    ([ "_nick" : MYNICK ]), 0, origin, #'msg);
		}
		break;
	case "items":
		if (sizeof(args) > 1 && is_formal(args[1])) {
		    sendmsg(args[1], "_request_list_item",
			    "[_nick] would like to know what services are available.",
			    ([ "_nick" : MYNICK ]), 0, origin, #'msg);
		}
		break;
        case "registerservice":
                if (sizeof(args) > 1 && is_formal(args[1])) {
                    mapping rv = ([ "_nick" : MYNICK ]);
                    if (sizeof(args) >= 3)
                        rv["_username"] = args[2];
                    if (sizeof(args) >= 4)
                        rv["_password"] = args[3];
                    sendmsg(args[1], "_request_registration",
                        "[_nick] would like register at your service.",
                        rv, 0, origin, #'msg);
                } else {
		    w("_warning_usage_registerservice", "Usage: /registerservice <gateway> [<username> <password>]");
                }
                break;
# endif
#endif
	case "info":
		showStatus(VERBOSITY_COMMAND_STATUS_INFO);
		return 1;
	case "st":
	case "stat":
	case "status":
		showStatus(VERBOSITY_COMMAND_STATUS);
		return 1;
	case "":
	case "s":
		showStatus(VERBOSITY_COMMAND_STATUS_TERSE);
		return 1;
	case "friends":
		showStatus(VERBOSITY_COMMAND_FRIENDS);
		return 1;
	case "members":
		//showStatus(VERBOSITY_COMMAND_MEMBERS);
                showRoom(1);
		return 1;
	case "version":
	case "vers":
	case "ver":
	case "req":
	case "request":				// ircII compat
#ifndef _PAGE_SERVER_SOFTWARE
# define _PAGE_SERVER_SOFTWARE T("_URL_server_software", "http://www.psyced.org")
#endif
		unless (sizeof(args) >= 2) {
		    w("_info_server_version",
		       "[_version_server] ([_version] [_version_driver]) available from [_page_server_software]", ([
			    "_version": SERVER_VERSION,
			    "_version_server": SERVER_DESCRIPTION,
			    "_version_driver": DRIVER_VERSION,
			    "_page_server_software": _PAGE_SERVER_SOFTWARE,
			    // required for RFC 1459 compliance
			    "_degree_debug": DEBUG,
			    "_host_server": SERVER_HOST
		    ]) );
		} else {
                    mapping rv = ([ "_nick" : MYNICK ]);
		    t = lower_case(args[1]);
#ifdef ALIASES
		    t = aliases[t] || t;
#endif

		    unless (is_formal(t)) t = summon_person(t);
		    sendmsg(t, "_request_version",
			"[_nick] requests your version.", rv, origin, 0,
				    (: w($2, $3, $4, $1);
				     return; :)
		    );
		}
		return 1;
	case "activity":
	case "idle":
		unless (sizeof(args) >= 2)
		    w("_warning_usage_activity", "Usage: /activity <person>");
		else {
		    t = lower_case(args[1]);
#ifdef ALIASES
		    t = aliases[t] || t;
#endif
		    unless (is_formal(t)) t = summon_person(t);
//		    unless (t = find_person(args[1])) t = args[1];
		    sendmsg(t, "_request_description_time", 
			    "[_nick] wants to know when you were active.",
			    ([ "_nick" : MYNICK ]), origin, 0, #'msg);
		}
		return 1;
#ifdef USER_PROGRAM
	case "ping":
		if (sizeof(args) >= 2) ping(args[1], ARGS(2));
		else w("_warning_usage_ping",
		       "Usage: /ping <entity> [<timestamp>]");
		break;
# ifndef _flag_disable_module_trust
#  if HAS_PORT(HTTPS_PORT, HTTP_PATH) || HAS_PORT(HTTP_PORT, HTTP_PATH)
	case "login":
		makeToken();
	case "surf":
		unless (v("token")) makeToken();
		w("_request_open_page_login",
		  "Please open [_page_login] to login.", ([
		    "_page_login" : (
#ifdef __TLS__
				     (tls_available() && HTTPS_URL) ||
#endif
				     HTTP_URL)
		   + NET_PATH "http/login?user="+ MYNICK
		   +"&token="+ v("token") +
			    (v("language") ? "&lang="+ v("language") : "")
			    ]) );
		examine(sizeof(args) > 1 ? args[1] : dest, 0,
		       	sizeof(args) > 2 ? args[2] : 0,
		       	/* sizeof(args) > 3 ? args[3] : */ 0, "_surf");
		return 1;
	case "edit":
		t = makeToken();
		w("_request_open_page_edit",
		  "Please open [_page_edit] to change your settings.",
		  ([ "_token" : t,
		     "_page_edit" : (
#ifdef __TLS__
				     (tls_available() && HTTPS_URL) ||
#endif
				     HTTP_URL)
		    + NET_PATH "http/edit?user="+ MYNICK
		    +"&token="+ t +
			    (v("language") ? "&lang="+ v("language") : "")
			    ]) );
		return 1;
#  else
	case "login":
	case "edit":
	case "surf":
		w("_failure_disabled_function_HTTP",
          "Sorry, but web functions have been disabled on this server.");
		return 1;
#  endif
# endif
	case "whois":
# ifdef IRC_PATH
		// irc has this funny /whois where what syntax
		examine(sizeof(args) > 1 ? ( sizeof(args) > 2
					     ? args[2]
					     : args[1])
					 : dest);
		return 1;
# endif
	case "x":
	case "examine":
		//examine(sizeof(args) > 1 ? args[1] : dest);
		examine(sizeof(args) > 1 ? args[1] : dest, 0,
		       	sizeof(args) > 2 ? args[2] : 0, 0,
		       	sizeof(args) > 3 ? args[3] : 0);
		return 1;
#ifdef ALIASES
	case "al":
	case "alias":
	case "expand":
		if (sizeof(args) == 3
			&& (t2 = lower_case(t = args[1]))
			&& !is_formal(args[1])) {
		    // it is ok to lowercase the UNI at this point
		    string converted = lower_case(args[2]);
# ifdef IRC_PATH
		    // test this pls and apply elsewhere too
		    t3 = index(converted, '%');
		    if (t3 >= 0) converted[t3] = '@';
		    // this is actually a hack specific to the IRC access
		    // not sure if this should stay here..
# endif
		    if (member(raliases, converted)) {
			w("_error_duplicate_alias",
			  "[_alias] is already pointing to [_address], you "
			  "cannot add a second name for that target.",
			  ([ "_alias" : raliases[converted],
			     "_address" : converted,
			     "_new": t
			  ]));
			return 1;
		    }

		    // first delete former entry (if existing)
		    // but we can't just delete here, we need to tell the user
		    // he just deleted an alias, so that net/irc (and other
		    // clients and/or protocols who need to do things like
		    // that) can issue the right NICKchange-messages to the
		    // client.
		    if (t3 = aliases[t2]) {
			w("_echo_alias_removed", "Alias [_alias] "
			  "for [_address] now removed.",
			  ([ "_alias" : t2,
			     "_address" : t3 ]));
			m_delete(v("aliases"), raliases[t3]);
			m_delete(aliases, t2);
			m_delete(raliases, t3);
		    }

		    v("aliases")[t] = converted;
		    aliases[t2] = converted;
		    raliases[converted] = t;

		    // v("query") might now point to a different target
		    // so we must delete query if we want send the messages
		    // to the new target.
		    if (query) query = 0;

		    w("_echo_alias_added", "[_address] is now aliased to "
		      "[_alias].", ([ "_alias" : t,
					"_address" : converted ]));
		    save();
		} else {
		    if (sizeof(args) == 1 && sizeof(v("aliases"))) {
			string short, long;
			foreach (short, long : v("aliases")) {
			    w("_list_alias", "[_long] is aliased to "
			      "[_short].", ([ "_short" : short,
					    "_long" : long ]));
			}
		    } else if (sizeof(args) == 2
			       && (t = aliases[lower_case(args[1])])
			       && (t = v("aliases")[t2 = raliases[t]])) {
			w("_list_alias", "[_long] is aliased to "
			  "[_short].", ([ "_short" : t,
					 "_long" : t2 ]));
		    } else w("_warning_usage_alias",
				 "Usage: /alias <newnick> <uniform-of-person>");
		}
		return 1;
	case "unal":
	case "unalias":
	case "unexpand":
		if (sizeof(args) == 2 &&
		    (t = aliases[t2 = lower_case(args[1])])) {
			m_delete(v("aliases"), raliases[t]);
			m_delete(aliases, t2);
			t2 = raliases[t];
			m_delete(raliases, t);

			// v("query") might now point to a different target
			// so we must delete query if we want send the messages
			// to the new target.
			if (query) query = 0;

			w("_echo_alias_removed", "Alias [_alias] "
			  "for [_address] now removed.",
			  ([ "_alias" : t2,
			     "_address" : t ]));
			save();
		}
		else w("_warning_usage_unalias", "Usage: /unalias <nick>");
		return 1;
#endif
		// undocumented compatibility
	case "col":
	case "color":
		if (sizeof(args) > 1) set("color", args[1]);
		break;
		// undocumented compatibility 2
	case "assign":
	case "setvar":
	case "var":
	case "set":
	case "sset":	// new irc protocol command as agreed with equinox
			// unless we prefer implementing capab or cap
		if (sizeof(args) > 2) set(args[1], ARGS(2));
		else if (sizeof(args) == 2) set(args[1]);
		else w("_warning_usage_set",
		    "Usage: /set <variable> [<value>]");
		break;
#if 0
	case "n":
	case "north":
		walk("N");
		break;
	case "s":
	case "south":
		walk("S");
		break;
	case "w":
	case "west":
		walk("W");
		break;
	case "e":
	case "east":
		walk("E");
		break;
#endif
#ifndef _flag_disable_module_authentication
//	"manual" authentication.. probably doesn't work because the
//	recipient will have a hard time figuring out what it's about without
//	vars.. so the proper way to do this is to request a /token ahead.
	case "authenticate":
		if (sizeof(args) > 1)
		    returnAuthentication(6, ARGS(1), 0);
		break;
#endif
	// here we go:
        case "token":
        case "makenonce":
        case "tan":
		v("nonce") = RANDHEXSTRING;
		w("_notice_token",
  "The PSYC authentication token \"[_nonce]\" is now valid. Use it wisely.",
		  ([ "_nonce" : v("nonce") ]));
		break;
#ifdef TELEPHONY_SECRET
# include <sys/tls.h>
# ifndef TELEPHONY_EXPIRY
#  define TELEPHONY_EXPIRY (60*60)
# endif
	case "answer":
		if (sizeof(args) > 1) {
			t = NET_PATH "http/call"->answer(args[1], 1);
			if (stringp(t)) w("_status_established_call",
			    "Call with [_nick] established.",
			    ([ "_nick": t ]));
			else w("_error_invalid_call", "Invalid call.");
		} else w("_warning_usage_answer", "Usage: /answer <sid>");
		break;
	case "reject":
		if (sizeof(args) > 1) {
			t = NET_PATH "http/call"->answer(args[1], 0);
			if (stringp(t)) w("_status_rejected_call",
			    "Call with [_nick] rejected.",
			    ([ "_nick": t ]));
			else w("_error_invalid_call", "Invalid call.");
		} else w("_warning_usage_reject", "Usage: /reject <sid>");
		break;
	case "call":
		// this is how the current stuff works:
		//
		// we identify the incoming and outgoing rtmp streams as
		//	"U"+ expiryTime +":"+ callerNick	for the callee
		// and	"I"+ expiryTime +":"+ callerNick	for the caller
		//
		// these things are then hmac'd using the secret, so we
		// have something to check validity against. the colons
		// are just for decoration - there is no way a combination
		// of expiry and nickname could result in the same string.
		//
		t = ([]);
		t["_time_expire"] = t3 = time() + TELEPHONY_EXPIRY;
		t["_check_call"] = t2 = hmac(TLS_HASH_SHA256,
				       	TELEPHONY_SECRET, "U"+t3+":"+MYNICK);
		P1(("hmac/call %O for %O\n", t2, "U"+t3+":"+MYNICK))
		t2 = "?user="+ MYNICK +"&expiry="+ t3 +"&jack="+ t2;
		t3 = NET_PATH "http/call"->make_session(MYNICK, t3, t);
		t["_token_call"] = t3;
		t["_page_call"] = (
#ifdef __TLS__
				     (tls_available() && HTTPS_URL) ||
#endif
				   HTTP_URL) + NET_PATH +"http/call?sid=" + t3;
# ifdef TELEPHONY_SERVER
		t["_uniform_call"] = TELEPHONY_SERVER + t2;
# endif
		if (sizeof(args) > 2) t3 = 0;
		else if (sizeof(args) == 2) t3 = more = args[1];
		else t3 = more;
		if (t3) {
			if (tell(t3, 0, 0, 0, "_request_call_link", t))
			    w("_echo_call", 0, ([ "_nick_target": t3, ]));
#if 0 // do we want this?
			t["_nick_target"] = t3;
			w("_info_call_link",
		 "Your link to a call to [_nick_target] is [_page_call].", t);
#endif
		} else w("_warning_usage_call");
		break;
#endif
//#ifndef ADVERTISE_PSYCERS
// pointless to de-activate lastlog, as it is shown for
// new messages at login automatically.. and then the bug
// happens
// TODO.. fix the lastlog command for ircgate mode.
	case "log":
	case "backlog":
	case "lastlog":
	case "last":
	case "la":
		logView(sizeof(args)>1? args[1]: 0, 1);
		break;
	case "logclear":
		logClear();
		save();
		w("_echo_log_cleared", "The log of last messages has been deleted.");
		break;
//#endif
#ifndef RELAY
# if 0
        case "tlshopcheck":
                if (objectp(ME) && interactive(ME)
                         && (t = tls_query_connection_state(ME))) {
                    
                    PT(("tls stuff on hop: %O\n", t))
                    PT(("args is %O\n", args[1]))
                    sendmsg(args[1], "_request_hopcheck_tls", "hopcheck!",
                            /*
                                ([ "_list_hops" : ({ "c2s" }),		     // _tab
                                   "_list_hops_encryption" : ({ 1 }),	     // _tab
                                   "_list_hops_authentication" : ({ 1 }) ]), // _tab
                                   */
                                ([ ]),
                                origin, 0,
                                (: PT(("hopcheck result\n")) :)
                    );
                }
                break;
# endif
	case "u":
	case "users":
		// showUsers();
		showRoom(1);
		break;
	case "reg":
	case "register":
	case "signup":	// bitnet relay style
		set("password", sizeof(args) > 1 ? ARGS(1) : 0);
		break;
//	case "charset":
//		set("charset", sizeof(args) > 1 ? ARGS(1) : 0);
//		break;
	case "unregister":
	case "drop":
		// sure we could unregister here, but what if somebody abuses
		// this? so we would need to check for a password argument..
		// still someone who figures out somebody else's password
		// can ruin his whole chat existence.. bad idea!
		w("_warning_usage_unregister",
		  "Why unregister? Just give somebody else the password...");
		break;
	case "list":
# ifdef PUBLIC_PLACES
	case "rooms":
	case "ro":
		// still feels wrong to do this in psyced rather
		// than with links on a web page, but many people
		// aren't ready for the revolution, so let's provide
		// this as a fallback.
                //
                // <fippo> if irc needs it, why not hook it in irc?
                //      also, why one message per room?
                //      psyc can transport lists, I dont see why 
                //      you always fall back to stupid hacks made 
                //      for webchats
		w("_echo_start_places_public"); // IRC needs it
		// not very efficient:
		//r = (NET_PATH "d/config") -> vQuery("_places_public");
		array(mixed) r = advertised_places();
		// in same style as _status_description
		for (t=0; t < sizeof(r); t+=2) {
			// if this command gets popular, we should have
			// place objects ready to use in advertised_places()
			t2 = find_place(r[t]);
			w("_list_places_public",
	    "[_location_place] ([_amount_members]) \"[_description_place]\"",
			       ([ "_nick_place" : r[t],
			      "_location_place" : t2 ? UNIFORM(t2) : r[t],
			   "_description_place" : r[t+1],
			      "_amount_members" : objectp(t2) ? t2->size() : 0,
			]));
		}
		w("_echo_end_places_public"); // IRC needs it
		break;
# endif
	case "places":
	case "pl":
		w("_echo_start_places_entered"); // weird but IRC needs it
		foreach (t2,t : places) w("_list_places_entered",
	    "[_location_place] ([_amount_members]) \"[_description_place]\"",
		    ([ "_nick_place" : t,
		       "_location_place" : UNIFORM(t2),
// we dont *need* to use psyc for every little bla bla within the server
// thats the job of a *real* client. so here goes the fast topic fetcher:
		    "_description_place" : (objectp(t2) && t2->qTopic()) || "",
		       "_amount_members" : (objectp(t2) && t2->size()) || 0
		     ]));
		w("_echo_end_places_entered");
		break;
#ifdef _flag_enable_place_single
# define STAY v("multiplace")
#else
# define STAY 1
#endif
	case "change":
	//se "channel":
	case "ch":
	case "c":
		if (sizeof(args) < 2) {
			if (v("lastplace") && v("lastplace") != NICKPLACE)
                            teleport(v("lastplace"), "_other", 0, STAY);
			else
                            w("_error_unavailable_place_other", 
                            "You haven't entered any other room yet.");
			break;
		}
		teleport(args[1], 0, 0, STAY);
		break;
	case "go":
		if (sizeof(args) < 2) {
			w("_warning_usage_go", "Usage: /go <place>");
			break;
		}
		teleport(args[1]);
		break;
        case "f":
	case "follow":
		if (v("invitationplace")) {
			teleport(v("invitationplace"), "_follow");
			vDel("invitationplace");
			return;
		}
		if (v("otherplace")) {
			teleport(v("otherplace"), "_other", 0, STAY);
			vDel("otherplace");
			return;
		}
		else w("_error_unavailable_invitation",
			"You haven't been invited yet.");
		break;
	case "h":
	case "ho":
	case "home":
		teleport(v("home") || DEFPLACE, "_home", 0, STAY);
		break;
#ifndef _flag_disable_place_enter_automatic
	case "subscribe":
	case "sub":
		subscribe(sizeof(args) > 2 ? SUBSCRIBE_PERMANENT :
			  SUBSCRIBE_TEMPORARY, sizeof(args) < 2 ? 0 : args[1]);
		break;
	case "unsubscribe":
	case "unsub":
		subscribe(SUBSCRIBE_NOT, sizeof(args) < 2 ? 0 : args[1]);
		break;
	case "enrol":
	case "enroll":
//	case "embark":
//	case "inscribe":
		foreach (t : (sizeof(args) < 2 ? ({ place }) : args[1..])) {
			unless(objectp(t)) if (t2 = find_place(t)) t = t2;
			sendmsg(t, "_request_enroll",
			    "[_nick] would like to become a steady member.",
				([ "_nick": MYNICK ]), origin);
		}
		break;
#endif // _flag_disable_place_enter_automatic
#if 0
	case "look":
	case "l":
		foreach (t : (sizeof(args) < 2 ? ({ place }) : args[1..])) {
			unless(objectp(t)) if (t2 = find_place(t)) t = t2;
			sendmsg(t, "_request_description",
			    "[_nick] would like to see your description.",
				([ "_nick": MYNICK ]) );
		}
		break;
#endif
	case "enter":
	case "e":
		switch(sizeof(args)) {
//		case 3:
//		default:
//			placeRequest(args[1], "_request_enter", 0, 0,
//				    ([ "_nick_local" : ARGS(2) ]));
//			break;
		case 2:
			placeRequest(args[1],
#ifdef SPEC
				     "_request_context_enter"
#else
				     "_request_enter"
#endif
				     );
			break;
		case 1:
			if (NICKPLACE) {
			    teleport(NICKPLACE); // historic, sort of
			    break;
			}
			// else fall thru
		case 0:	// impossible
		default:
			w("_warning_usage_enter",
//			   "Usage: /enter [<group> [<localNick>]]");
			   "Usage: /enter [<group>]");
		}
		break;
	case "join":
	case "j":
		if (sizeof(args) < 2)
		    teleport(NICKPLACE, "_join");
		else
		    teleport(args[1], "_join", 0, 1);
		break;
#ifdef DEVELOPMENT
	case "forceenter":
	case "forcejoin":
	        if (sizeof(args) < 2) {
		    w("_warning_usage_forceenter", "Usage: /forceenter <context> [<nick_place>]");
		    break;
		}

	        string plc = args[1];
		string nick_place = sizeof(args) >= 3 ? args[2] : regreplace(plc, "^.*@", "", 1);

		placeRequest(plc,
#ifdef SPEC
			     "_request_context_enter"
#else
			     "_request_enter"
#endif
			     );

		places[plc] = nick_place;

		P3(("%O force joins mcast group for %O\n", ME, plc))
		register_context(ME, plc);

		w("_notice_forceenter", "You force entered [_place] ([_nick_place]).", ([ "_place": plc, "_nick_place": nick_place ]));

		break;
        // etwas hässlich so.. aber was will man sonst? beim zweiten versuch?
        // oder gar als flag von /leave?
#endif
        case "forceleave": // delete the membership from places mapping
                if (member(places, (t = sizeof(args) < 2 ? place : args[1]))) {
                    m_delete(places, t);
                }
	// fall thru
	case "unenter":         // and the protocol should follow.. unenter!
	case "leave":
	case "part":
	case "pa":
	case "le":
		placeRequest(sizeof(args) < 2 ? place : args[1],
#ifdef SPEC
			     "_request_context_leave"
#else
			     "_request_leave"
#endif
			     , 1);
		break;
	case "connect":
	case "con":
	//case "nick":
		if (v("scheme") == "irc" ||
// but let's presume psyc clients wanna learn this stuff ..  ;)
//		    v("scheme") == "psyc" ||
		    v("scheme") == "jabber") {
			w("_warning_inappropriate_access",
   "Caution! Your client may not be ready to handle a change of identity.");
		}
		if (sizeof(args) < 2) {
			w("_warning_usage_connect",
			   "Usage: /connect <identity> [<password>]");
			break;
		}
		connect(args[1], ARGS(2));
		// if everything goes well we don't get here
		break;
	case "g":
	case "gr":
	case "greet":
		if (sizeof(args) != 1)
		    w("_warning_usage_greet", "Usage: /greet");
		else if (v("greet")) {
		    talk(v("greet"), 2);
		} else w("_error_unavailable_person_greet",
		    "Oh, there is no one to greet.");
		break;
	case "mo":
	case "more":
		if (sizeof(args) != 1)
		    w("_warning_usage_more",
			"Usage: /more. See also /tell and /talk.");
		else if (more) talk(more, 1);
		else w("_error_unavailable_person_more",
		    "Oh, there is no conversation to continue.");
		// or maybe just simply:
		// talk(more);   ...?
		break;
	case "r":
	case "re":
	case "reply":
		if (sizeof(args) != 1)
		    w("_warning_usage_reply", "Usage: /reply");
		else {
#if 0
			string vr = v("reply");
			string vq = v("query");

			if (vr && (!vq ||
				   (vr = lower_case(vr)) != lower_case(vq))) {
#ifdef ALIASES
			    talk(raliases[vr] || (aliases[vr]
				       	? SERVER_UNIFORM
					  +"~"+ v("reply")
					: v("reply")) );
			    // let /r have its toggle-behaviour again, almost
			    //if (args[1] != v("reply")) vDel("reply");
			    // then again.. maybe this was broken...
#else
			    talk(v("reply"), 1);
#endif
			} else talk();
#else
			// let's try a simple approach and make sure it
			// works. we can make it smarter on an other day.
			if (v("query")) talk();
			else talk(v("reply"), 2);
#endif
		}
		break;
	case "notice":
		t2 = "_message_private_annotate";
	// fall thru
	case "msg":
	case "tell":
	case "m":
	case "t":
		if (sizeof(args) > 2) {
			tell(more = args[1], ARGS(2), 0, 0, t2);
			break;
		}
		if (strlen(a) > 1) {
			w("_warning_usage_tell",
			    "Usage: /tell <person> <message>");
			break;
		} else if (a == "m") {
			if (more) talk(more, 1);
			else w("_warning_usage_more",
			    "Usage: /more. See also /tell and /talk.");
			break;
		}
	// fall thru
	case "q":
	case "query":
	case "talk":
	case "dialog":
		if (sizeof(args) == 1 && v("query")) talk();
		else if (sizeof(args) == 2) talk(args[1], 1);
		else w("_warning_usage_query",
		    "Usage: /talk <person> or /talk to stop");
		break;
	case "quit":
	case "bye":
	case "signoff":
	case "exit":
		// bye(ARGS(1));
                quit();
		break;
        case "unlink":
                linkDel(ARGS(1));
                return 1;
        case "link":
		if (sizeof(args) == 3) {
                        linkSet(args[1], args[2]);
                        return 1;
                }
                break;
#endif
#ifndef _flag_disable_module_friendship
	case "shout":
	case "cast":
	case "friendcast":
		if (sizeof(args) > 1) friendcast(0, ARGS(1));
                else w("_warning_usage_shout",
		    "Usage: /shout <message-to-your-friends>");
		break;
# ifndef _flag_disable_module_presence
	// experimental new way to log out without logging out.
	// may very well not work as planned
	    // detach for psyc clients: _do_presence offline + _unlink
	case "det":
	case "detach":
                //availability = AVAILABILITY_OFFLINE;
                remove_interactive(ME);
                //break;
                // used to fall thru to declare myself offline as well..
                // now you have to declare yourself offline manually
                // no you don't. if availability isn't offline the
                // disconnected() handler will clean you out!
		// ok let's do it manually.. see if we get in trouble later.
		availability = AVAILABILITY_OFFLINE;
		// yes v("availability") is retained.. maybe useful later
		return 1;
	case "offline":
		announce(AVAILABILITY_OFFLINE, 1, 1, ARGS(1));
		return 1;
	case "vacation":
		announce(AVAILABILITY_VACATION, 1, 1, ARGS(1));
		return 1;
	case "ircgone":
		// irc's /away may be a client automization
		announce(AVAILABILITY_AWAY, 0, 0, ARGS(1));
                // main purpose is to ensure proper RPL_NOWAWAY
		w("_echo_set_description_away", 0, ([
			"_nick": MYNICK,
			"_description_presence": v("presencetext")
		]) );
		return 1;
	case "away":
	case "gone":
	case "bbl":
		// maybe verbose should be 0 for /away
		// or maybe i just forgot an if (verbose) in user:announce()
		// if that's it, remove this remark
		announce(AVAILABILITY_AWAY, 1, 1, ARGS(1));
		return 1;
	case "unavailable":
	case "dnd":
		announce(AVAILABILITY_DO_NOT_DISTURB, 1, 1, ARGS(1));
		return 1;
	case "nearby":
	case "afk":
	case "brb":
		announce(AVAILABILITY_NEARBY, 1, 1, ARGS(1));
		return 1;
	case "busy":
		announce(AVAILABILITY_BUSY, 1, 1, ARGS(1));
		return 1;
	case "irchere":
                // main purpose is to ensure proper RPL_UNAWAY
		w("_echo_set_description_away_none", 0, ([
			"_nick": MYNICK,
			"_description_presence": v("presencetext")
		]) );
		// irc's /away may be a client automization
		announce(AVAILABILITY_HERE, 0, 0, ARGS(1));
		return 1;
	case "here":
	case "online":
	case "attach":
		announce(AVAILABILITY_HERE, 1, 1, ARGS(1));
		return 1;
	case "available":
	case "talkative":
	case "chatty":
		announce(AVAILABILITY_TALKATIVE, 1, 1, ARGS(1));
		return 1;
	case "realtime":
		announce(AVAILABILITY_REALTIME, 1, 1, ARGS(1));
		return 1;
	case "av":
	case "avail":
	case "availability":
		// if args[1] is a word we could re-issue cmd().. then again no
			// this sscanf rejects any further digits.. funny,
			// this already supports my dotless float syntax idea
		if (sizeof(args) < 2 ||! sscanf(args[1], "%1d", t))
		    w("_warning_usage_availability",
			"Usage: /availability <digit> [<description>]");
		else
		    announce(t, 1, 1, ARGS(2));
		return 1;
	case "mood":
			// this sscanf rejects any further digits.. funny,
			// this already supports my dotless float syntax idea
		if (sizeof(args) < 2 ||! sscanf(args[1], "%1d", t))
		    w("_warning_usage_mood",
			"Usage: /mood <digit> [<description>]");
		else {
			vSet("mood", mood = t);
			if (sizeof(args) > 2)
			    announce(0, 1, 1, ARGS(2));
		}
		return 1;
	case "auto":
	case "motto":
		// this command is normally accessed as /mynick
		// as it behaves similarely to /me
		return motto(ARGS(1));
	case "presence":
                showMyPresence(1);
		return 1;
# endif /* _flag_disable_module_presence */
	case "cancel":
	case "can":
		if (sizeof(args) < 2) w("_warning_usage_cancel",
		    "Usage: /cancel <person>");
		else foreach (t : args[1..]) friend(2, 0, t);
		return 1;
	case "unfriend":
	case "unfr":
	case "enemy":
		if (sizeof(args) < 2) w("_warning_usage_friend",
		    "Usage: /friend <person> or /unfriend <person>");
		else foreach (t : args[1..]) friend(1, 0, t);
		showFriends();
		return 1;
	case "friend":
	case "fr":
		if (sizeof(args) < 2) w("_warning_usage_friend",
		    "Usage: /friend <person> or /unfriend <person>");
		    // "Usage: /friend <person> or /friend -<person>"
                    // intentional that it falls thru to showFriends() ?
		else foreach (t : args[1..]) friend(0, 0, t);
		showFriends();
		return 1;
	case "notify":
	case "nf":
		t = 0;
		switch(sizeof(args)) {
		// saga found an interesting bug in the lpc compiler
		// code between switch and the first case is not complained
		// about, but it makes the switch completely inactive.
		case 2:
#ifdef ALIASES
			t3 = aliases[t3 = lower_case(args[1])] || t3;
#else
			t3 = lower_case( args[1] );
#endif
			if (member(ppl, t3) && ppl[t3][PPL_NOTIFY]
				&& ppl[t3][PPL_NOTIFY] != PPL_NOTIFY_NONE) {
			    sPerson(t3, PPL_NOTIFY, PPL_NOTIFY_NONE);
			    w("_echo_notification_removed",
				"[_nick_target] will no longer be notified.", ([
				    "_nick_target": args[1]
			    ]) );
			    return 1;
			}
			t = PPL_NOTIFY_IMMEDIATE;
			t2 = "immediate";
			break;
		case 3:
#ifdef ALIASES
			t3 = aliases[t3 = lower_case(args[1])] || t3;
#else
			t3 = lower_case( args[1] );
#endif
			if (abbrev(args[2], "immediate")) {
				t = PPL_NOTIFY_IMMEDIATE;
				t2 = "immediate";
			} else if (abbrev(args[2], "delayed")) {
				t = PPL_NOTIFY_DELAYED;
				t2 = "delayed";
			} else if (abbrev(args[2], "DelayedMore")) {
				t = PPL_NOTIFY_DELAYED_MORE;
				t2 = "delayed_more";
			} else if (abbrev(args[2], "silent")) {
				t = PPL_NOTIFY_MUTE;
				t2 = "silent";
			}
		}
		if (t && sPerson(t3, PPL_NOTIFY, t)) {
			w("_echo_notification_" + t2,
			    "[_nick_target] will be notified.", ([
				"_nick_target": args[1]
			]) );
			break;
		}
		w("_warning_usage_notify",
"Usage: /notify <person> [ i(mmediate),d(elayed),D(elayedMore) ]");
		break;
	case "sh":
	case "show":
//		if (sizeof(args) != 2) return listAcq(PPL_ANY);
		if (sizeof(args) == 2) switch(args[1]) {
		//case "friends": // this should probably move to 'trust'
		// but first we have to rethink the /friend command
		case "no":
		case "nf":
		case "notifications":
		case "notified":
		case "notify":	return listAcq(PPL_NOTIFY);
		case "offered":
		case "in":	return listAcq(PPL_NOTIFY, PPL_NOTIFY_OFFERED);
		case "pending":
		case "out":	return listAcq(PPL_NOTIFY, PPL_NOTIFY_PENDING);
		case "tr":
		case "trust":	return listAcq(PPL_TRUST);
		case "ex":
		case "expose":	return listAcq(PPL_EXPOSE);
		case "ig":
		case "ignore":
		case "ignored":
		case "di":
		case "dis":
		case "display":	return listAcq(PPL_DISPLAY);
		case "all":	return listAcq(PPL_ANY);
		case "complete":
		case "*":	return listAcq(PPL_COMPLETE);
		case "json":
		case "data":	return listAcq(PPL_JSON);
		}
		w("_warning_usage_show",
	  "Usage: /show [notify|in|out|display|trust|expose|all|*|data]");
		break;
	case "ignore":
	case "ig":
		if (sizeof(args) != 2) return listAcq(PPL_DISPLAY);
#ifdef ALIASES
		t3 = aliases[t3 = lower_case(args[1])] || t3;
#else
		t3 = args[1];
#endif
		if (sPerson(t3, PPL_DISPLAY, PPL_DISPLAY_NONE)) {
			w("_echo_acquaintance_display_none",
			    "You won't be seeing a word from [_nick_target].",
			    ([ "_nick_target": t3 ]) );
		}
		break;
	case "unignore":
	case "unig":
	case "di":
	case "dis":
	case "display":
		if (sizeof(args) != 2) return
		    w("_warning_usage_display", "Usage: /display <person>");
#ifdef ALIASES
		t3 = aliases[t3 = lower_case(args[1])] || t3;
#else
		t3 = args[1];
#endif
		if (sPerson(t3, PPL_DISPLAY, PPL_DISPLAY_REGULAR)) {
			w("_echo_acquaintance_display_normal",
			    "Regular display restored for [_nick_target].", ([
				"_nick_target": t3
			    ]) );
		}
		break;
	case "unexpose":
		// add '-' as 2nd arg here?
	case "expose":
		if (sizeof(args) != 3 || strlen(args[2]) != 1) return
		    w("_warning_usage_expose",
		      "Usage: /expose <person> <degree>");
		t = args[2][0];
		if (t < '0' || t > '9') {
		    t = PPL_EXPOSE_DEFAULT;
		    args[2] = "normal";
		}
#ifdef ALIASES
		t3 = aliases[t3 = lower_case(args[1])] || t3;
#else
		t3 = args[1];
#endif
		if (sPerson(t3, PPL_EXPOSE, t)) {
		    w("_echo_acquaintance_exposed",
	"[_nick_target]'s friendship exposed to degree [_degree_expose].", ([
			    "_nick_target": t3,
			    "_degree_expose": args[2]
		    ]) );
		}
		break;
	case "untrust":
		// add '-' as 2nd arg here?
	case "trust":
		if (sizeof(args) != 3 || strlen(args[2]) != 1) return
		    w("_warning_usage_trust",
		      "Usage: /trust <person> <degree>");
		t = args[2][0];
		if (t < '0' || t > '9') {
		    t = PPL_TRUST_DEFAULT;
		    args[2] = "normal";
		}
#ifdef ALIASES
		t3 = aliases[t3 = lower_case(args[1])] || t3;
#else
		t3 = args[1];
#endif
		if (sPerson(t3, PPL_TRUST, t)) {
		    w("_echo_acquaintance_trusted",
	"Your degree of trust for [_nick_target] is [_degree_trust].", ([
			    "_nick_target": t3,
			    "_degree_trust": args[2]
		    ]) );
		}
		break;
#endif /* _flag_disable_module_friendship */
	// contributed by saga@symlynX.com
	case "wake":
		if (sizeof(args) != 2) return wake(0);
                else return wake(args[1]);
	case "save":
		t = save();
		w("_echo_save", "[_amount_lines] lines of log saved.",
			(["_amount_lines": t]));
		break;
#endif /* USER_PROGRAM */
	default:
#ifdef USER_PROGRAM
		if (boss(ME)) switch(a) {
		case "warn":
		case "kill":
			unless (sizeof(args) > 1) {
				w("_warning_usage_"+a,
				  "Usage: /"+a+" <nick> [<message>]");
				return 1;
			}
#if 0 //def ALIASES
			// makes no sense for this command to support aliases
			t3 = aliases[t3 = lower_case(args[1])] || t3;
#else
			t3 = args[1];
#endif
			// learn to accept local uniforms, too?
			if (is_formal(t3)) {
				w("_error_necessary_nick_local");
				return 1;
			}
		       	ob = find_person(t3);
			if (ob) {
				t = sizeof(args) > 2 ? ARGS(2) : 0;
				// log first, after kill ob will be 0
				log_file("BEHAVIOUR", "[%s] %O %ss %O: %O\n",
				  ctime(), ME, a, ob, t);
				w("_echo_"+a, "[_entity] "+a+"ed.", ([
					"_entity": ob ]));
				ob -> sanction(a, t);
			} else w("_error_unknown_name_user", 0, ([
				 "_nick_target": t3 ]));
			return 1;
		case "config":
			if (sizeof(args) < 3) return
			    w("_warning_usage_config",
			      "Usage: /config <entry> <setting> [<value>]");
			t = ARGS(3);
			if (t == "") t = 0;
			return w("_echo_config", "Config for [_setting] in "
				  "[_entry] is [_value]", ([
				"_entry": args[1],
				"_setting": args[2],
				"_value": config(args[1], args[2], t)
			]));
#ifndef PRO_PATH
		case "patch":
			if (sizeof(args) == 4 && args[2] == "password"
			     && (ob = summon_person(args[1]))
			     && ob->vSet(args[2], args[3])) {
				w("_echo_patch",
				    "Password has been reset in [_nick].",
				    ([ "_nick": ob->qName() ]));
			} else w("_warning_usage_patch",
			    "Usage: /patch <user> password <value>");
			return 1;
#endif
#ifdef DAEMON_PATH
		case "block":
			unless (sizeof(args) >= 2) {
				DAEMON_PATH "hosts" -> list();
				return 1;
			}
			switch(DAEMON_PATH "hosts" -> modify(t = args[1],
						     sizeof(args) > 2
						     ? implode(args[2..], " ")
						     : 0)) {
			case 1:
			    w("_echo_block_on", "[_address_mask] blocked.",
				    ([ "_address_mask": t ]) );
			    // this part can run into a too long evaluation..
			    foreach (ob : users()) {
				if (abbrev(t, query_ip_number(ob))) {
				    log_file("BEHAVIOUR",
					"[%s] %O blocks %O (%s)\n",
					  ctime(), ME, ob, t);
				    ob -> sanction("kill");
				}
			    }
			    break; 
			case -1:
			    w("_echo_block_off", "[_address_mask] unblocked.",
				    ([ "_address_mask": t ]) );
			    break; 
			case -2:
			    w("_error_illegal_address_block_reason",
				    "You need to specify a reason.");
			    break;
			default:
			    w("_error_illegal_address_block",
				    "You can't do that.");
			    break; 
			}
			return 1;
#endif
		case "yell":
			if ((t = ARGS(1)) && strlen(t)) {
				log_file("ANNOUNCE", "[%s] %s announces: %s\n",
				  ctime(), MYNICK, t);
				shout(ME, "_message_announcement", t, ([
					"_authenticated" : 1,
					"_nick" : MYNICK
				]) );
			}
			return 1;
		case "lu":	// funny old tradition, but should change
			list_sockets(sizeof(args) > 1 ? args[1] : 0,
                                  SOCKET_LIST_USER + SOCKET_LIST_GHOST);
			return 1;
		case "tcp":
			list_sockets(0, SOCKET_LIST_LINK);
			return 1;
		case "mem":
		case "memory":
			// command("status");	// from the mysts of LPMUD
                        if (t = debug_info(DINFO_STATUS, ARGS(1))) {
                                w("_status_system_"+a, "[_text]",
                                  ([ "_text": t ]) );
                                return 1;
                        }
                        break;
		//case "malloc":
			// command("malloc");
			// return 1;
		case "odump":
			debug_info(5, "objects", "/log/objects.dump");
			// command("dumpallobj");
			return 1;
		case "update":	    // apilosov feels it should be /update  ;)
		case "recompile":
		case "reload":
		case "load":
			return recompile(args[1..], 1);
		case "rm":
                        // warn about obsolete command..?
                        // what's the name of the new /rm then?
                        // /destruct is tooo long
		case "destruct":
			return recompile(args[1..], 0);
		default:
			if (boss_command(a, ARGS(1))) return 1;
		}
		if (a == MYLOWERNICK) return motto(ARGS(1));
                // how to implement something like /comfort <nick> ?
                // and should it be /_smile instead of /smile?
                if (sizeof(args) == 1 &&
                    act(a, dest || query || place, origin)) return 1;
		// we could implement ctcp-style requests to the query
		// person here.. but we don't have any really..
#endif /* USER_PROGRAM */
		if (objectp(place)) {
			 if (place->cmd(a, args, boss(origin), origin,
#ifdef USER_PROGRAM
				     sayvars ||
#endif
				     ([ "_nick": MYNICK ]) ))
				 return 1;
	//
	// <fippo> or should the room itself send a list of commands?
	// <lynX> i'm not sure if i like it, but the room could send a
	//	command listing every time it receives a cmd() or
	//	_request_execute that doesn't work. so to achieve
	//	that kind of behaviour no change is necessary here,
	//	just change your room code.
	//
		} else if (place && command) {
			// need to postpone this
			sendmsg(place, "_request_execute", command,
#ifdef USER_PROGRAM
				sayvars ||
#endif
			       	([ "_nick": MYNICK ]), origin);
			return 1;
		}
	    // should we allow for /help still? if (a == "help") ...
#if !defined(RELAY) && !defined(_flag_disable_info_commands)
# define MYTEMP	"[_page_help] provides Manual and Helpdesk."
		t = T("_URL_help", "http://help.pages.de");
	// consider _flag_enable_unauthenticated_message_private here?
# ifdef USER_PROGRAM
		if (IS_NEWBIE) w("_info_commands_newbie", "\
Commands: go, me, who, p(eople), bye.\n" MYTEMP,
			    ([ "_nick": MYLOWERNICK, "_command": a,
			       "_page_help" : t ]) );
		else w("_info_commands", "\
Commands: tell, talk, go, me, [_nick], who, p(eople), log, reply, greet, bye.\n"
		    MYTEMP, ([ "_nick": MYLOWERNICK, "_command": a,
			       "_page_help" : t ]) );
		break;
# else
		w("_info_commands_remote", "\
Commands: me, my, who, whois, p(eople), s(tatus), i(nvite), req(uest).\n"
		    MYTEMP, ([ "_command": a, "_page_help" : t ]) );
# endif
#endif
	}
}

#ifdef USER_PROGRAM
// psyc conformant command request parser for _request_do_somethings
request(source, mc, vars, data) {
//      int ackonly = 0;
	string k, tag, family;
	mixed t, s;
	int glyph;

#if DEBUG > 3
	// in case of _store from jabber client vars may contain photo data
	P4(("»»> request(%O) %O from %O in %O (%O)\n",
	    mc, data, source, ME, vars))
#else
	P2(("»»> request(%O) %O from %O in %O\n",
	    mc, data, source, ME))
#endif
	PSYC_TRY(mc) {
// quite dubious choice of method names.. sigh
case "_message_private":
case "_private":
case "_tell_question":  // _ask? _say? .. _group?
case "_tell":
                tell(vars["_person"] || vars["_focus"],
		     data, 0, vars["_action"], 0);
                return 1;
case "_message":
                if (vars["_person"]) {
			tell(vars["_person"], data, 0, vars["_action"], 0);
			return 1;
		}
	// else.. fall thru
case "_message_public":
case "_public":
case "_speak":
                if (data) speak(data, 0, vars["_group"] || vars["_focus"]);
		else if (vars["_action"]) action(vars["_action"],
					    vars["_group"] || vars["_focus"]);
                return 1;
case "_cast":
                friendcast(vars["_method"], data, vars, // redundant for now
                           vars["_amount_friendivity"]);
                break;
case "_wake":
                return wake(vars["_person"]);
case "_enter_join":
		t = "_join";
case "_enter":
                // vars["_flag"] is used internally by net/irc and
                // probably will never be necessary for psyc clients
                // we should check if _flag contains _quiet here really
                // but i don't even know if i want this to go into psyc
                // protocol for real.
		if (vars["_group"]) // always stay
		    teleport(vars["_group"], t, vars["_flag"], 1, vars);
		return 1;
case "_enter_change":		// non-standard, psyced only
		if (vars["_group"])
		    teleport(vars["_group"], 0, vars["_flag"], 0, vars);
		return 1;
case "_leave":
		if (t = vars["_group"] || vars["_focus"])
		    placeRequest(t,
#ifdef SPEC                  
                             "_request_context_leave"
#else                        
                             "_request_leave"
#endif
			     , 1, vars["_flag"]);
		return 1;
case "_invite":
		// _focus has been taken care of beforehand in person.c
		if (t = vars["_person"]) {
			unless (place) return w("_error_status_place_none",
			  "You aren't in a room");
                        m_delete(vars, "_person");
			return invite(t, vars);
		}
		return 0;
case "_subscribe_permanent":
case "_subscribe_temporary":
case "_subscribe":
		if (t = vars["_group"] || vars["_focus"]) {
			subscribe(family == "_subscribe_permanent" ?
			    SUBSCRIBE_PERMANENT : SUBSCRIBE_TEMPORARY, t);
			return 1;
		}
		return 0;
case "_unsubscribe":
		if (t = vars["_group"] || vars["_focus"]) {
			subscribe(SUBSCRIBE_NOT, t);
			return 1;
		}
		return 0;
case "_remove_register":
case "_register_remove": // to go
		// unregister a user? only if you are trustworthy!
		// this function should not be available to clients or
		// somebody could figure out somebody else's password
		// and ruin his chat life (see also /unregister) 
		if (vars["_INTERNAL_trust"] < 8) return 0;
		t = PERSON_DATA_FILE(MYNICK);
		if (file_size(t) < 0)
		    return w("_error_unknown_remove_register",
			"No such registration found.");
		if (rm(t)) {
			destruct_object(ME);	// avoid saving this!!
			return 1;
		} else
		    return w("_failure_unsuccessful_remove_register",
			"Something went wrong removing that registration.");
		return 0;
case "_register":
		// override old password if coming from trusted source
		if (vars["_INTERNAL_trust"] > 7) vDel("password");
		set("password", vars["_password"]);
		return 1;
case "_set":
		if (t = vars["_key_set"]
                          || convert_setting(vars["_key"],0,"set")) {
#ifdef _flag_enable_trust_client_settings
			// we skip safety checks in set() here!
			vSet(t, vars["_value"]);
#else
			set(t, vars["_value"]);
#endif
			return 1;
		}
		return 0;
case "_store":
		foreach (k, t : vars) {
			P3(("%O storing %O\n", mc, k))
			if (abbrev("_application", k)) vSet(k, t);
			else if (s = convert_setting(k, 0, "set")) {
				PT(("%s calling %O:set(%O)\n", mc, ME, s))
#ifdef _flag_enable_trust_client_settings
				// we skip safety checks in set() here!
				vSet(s, t);
#else
				set(s, t);
#endif
			}
			else {
				P1(("%s dropping %O from %O\n", mc, k, source))
			}
		}
#if 0
		if (v("locations")[0])
		    sendmsg(v("locations")[0], "_echo_request_store",
			    0, ([]));
		else
		    w("_echo_request_store");   // old style
#else
		w("_echo_request_store");
		// may be a client, but may also be a remote control
		if (source != ME) sendmsg(source, "_echo_request_store");
#endif
		save();
		return 1;
case "_retrieve":
		mapping vm = ([]);

		foreach (k, t : vMapping()) {
			if (k[0] == '_') vm[k] = t;
		//      else switch(t) {
		//      }
		}
#if 0
		// weird output logic
		if (v("locations")[0])
		    sendmsg(v("locations")[0], "_status_storage", 0, vm);
		else w("_status_storage", 0, vm);
#else
		// may be a client, but may also be a remote control
		if (source != ME) sendmsg(source, "_status_storage", 0, vm);
		else w("_status_storage", 0, vm);
#endif
		return 1;
case "_examine":
		examine(vars["_nick_person"], vars["_person"],
		       	vars["_trustee"], vars["_tag"], vars["_format"] || 1);
		return 1;
case "_clear_friend_acknowledge":
//		ackonly = 1; // and fall thru
case "_clear_friend":
case "_friend_cancel": // tmp
		friend(2, 0, vars["_nick_person"] || vars["_person"]);
		return 1;
case "_remove_friend_acknowledge":
//		ackonly = 1; // and fall thru
case "_remove_friend":
case "_friend_remove": // tmp
		friend(1, 0, vars["_nick_person"] || vars["_person"]);
		return 1;
case "_add_friend_acknowledge":
//		ackonly = 1; // and fall thru
case "_add_friend":
case "_friend": // tmp
		friend(0, 0, vars["_nick_person"] || vars["_person"],
			     vars["_trustee"]);
		return 1;
#ifndef _flag_disable_module_presence
case "_presence":
		// parser takes care of checking _degree type
		if (t = vars["_degree_mood"]) {
//			if (! sscanf(t, "%1d", t)) {
//				w("_warning_usage_mood");
//				return 1;
//			}
			vSet("mood", mood = t);
		}
		if (t = vars["_degree_availability"]) {
//			if (! sscanf(t, "%1d", t))
//			    w("_warning_usage_availability");
//			else
			    announce(t, !vars["_degree_automation"],
				 1, vars["_description_presence"]);
			return 1;
                }
                P1(("got invalid %O: %O, %O in %O\n", mc, vars, data, ME))
		w("_failure_necessary_variable");
		return 1;
#endif // _flag_disable_module_presence
case "_list_peers_JSON":
		listAcq(PPL_JSON);
		return 1;
case "_list_peers": // _show_peers ?
		listAcq(PPL_COMPLETE);
		return 1;
//case "_list_aliases":
//		alias();	// needs a rewrite of /alias
//		return 1;
case "_unlink":
                unlink(vars["_service"]);
                return 1;
case "_exit":
		// so this is some kind of ugly hack not to be used.. huh?
		announce(AVAILABILITY_OFFLINE);
	// fall thru
case "_quit":
                // bye(vars["_reason"]);
                quit();
		//
		// this shouldn't be necessary!!
		//call_out( (: destruct(ME) :) , 30);
		// i hope it isnt..
                return 1;
case "_show_log": // _list_log ?
case "_log":
                tag = vars["_tag"];
		if ((t = vars["_amount"] || vars["_match"]))
		    cmd("lastlog", ({ 0, t }) );
		else
		    cmd("lastlog");
                // indicate end of lastlog
                w("_echo_lastlog", 0, ([ "_tag_reply": tag ]));
		return 1;
case "_enter_home":
		// calling cmd() here is like calling request() in cmd()
		// where to put semantics.. keep it in a switch for performance
		// or have function table bloat? i suggest we put things into
		// functions as soon as there is the faintest need for it.
		// worst option of all is probably code duplication, unless
		// it were automized. hmmmm.
		cmd("home");
		return 1;
case "_list_places":
		cmd("places");
		return 1;
case "_list_places_public":
		cmd("rooms");
		return 1;
case "_list_users_public":
		cmd("people");
		return 1;
case "_clear_log":
		cmd("logclear");
		return 1;
case "_time_boot":
		cmd("boottime");
		return 1;
case "_follow":
case "_help":
case "_save":
case "_status":
		cmd(mc[1..]);
		return 1;
// case "_topic":
	    // reminder: commands that may be forwarded to a remote entity
	    // probably shouldn't get here but they do, so we need to provide
	    // a 'cmdline' or come up with some other fine way to properly
	    // forward the _request_do
// this one is actually fed by PSYC_TRY and SLICE. was "default:" + abbrev
case "_action":
		unless (mc != family && act(mc[sizeof("_action_")..],
					    vars["_focus"], ME))
		    w("_failure_unsupported_action",
    "Sorry, there is no such action. Check [_page_help_action]",
	([ "_page_help_action":
	   "http://about.psyc.eu/Action#Prefabricated_actions" ]));
		return 1;
	PSYC_SLICE_AND_REPEAT
	}
	return 0;
}

ping(to, timestamp) {
	to = lower_case(to);
# ifdef ALIASES
	to = aliases[to] || to;
# endif
	unless (is_formal(to)) to = summon_person(to);
	sendmsg(to, "_request_ping", "[_nick] pings you.",
	   ([ "_nick": MYNICK, "_time_ping": timestamp || time() ]),
	    origin, 0, #'msg);	// callback to make sendmsg give us a tag
}

motto(t) {
	if (t && strlen(t)) {
//		if (awaytext) t = T("_TEXT_action_away",
//				       "is away")+": "+t;
		vSet("me", t);
		save();
		w("_echo_set_description",
		    "Ok: '[_nick] [_action_description].'", ([
			"_nick": MYNICK, 
			"_action_description": v("me")
		]) );
//		if (awaypresence)
//		    announce(AVAILABILITY_AWAY, 1, 1);
	//	else if (stringp(availability))
	//	    announce(AVAILABILITY_HERE, 1, 1);
#if 0
		else if (place) sendmsg(place,
			       "_notice_description", 0,
			([ "_nick" : v("coolname") || MYNICK,
			 "_action" : v("me") ]) );
#endif
	}
	else if (v("me")) {
		w("_echo_set_description_none",
		    "No longer: '[_nick] [_action_description].'", ([
			"_nick": MYNICK,
			"_action_description": v("me")
		]) );
			
		vDel("me");
//		if (awaypresence) {
//		    if (stringp(availability))
//			announce(AVAILABILITY_HERE, 1, 1);
//		}
		save();
	}
	else w("_warning_usage_auto",		 /* motto instead? */
		"Usage: /[_nick] <description>", 
		([ "_nick": MYNICK ]) );
	return 1;
}

private talk(to, handleAliases) {
	// check if we are already in a query with this person.. here?
	if (to) {
		string tn;
		// looking for a bug.. intermediate hack here..
		ASSERT("talk()", stringp(to), to)
#ifdef ALIASES
		switch (handleAliases) {
		case 1:
			tn = aliases[lower_case(to)];
			query = find_person(tn || to);
			vSet("query", tn ? to : (query ? query->qName() : to));
			P3(("talk(%O,%O) w/ %O = %O via %O\n",
			    to, handleAliases, v("query"), query, tn))
			break;
		case 2:
			query = find_person(to);
			tn = raliases[to];
			vSet("query", tn ? tn : (query ? query->qName() : to));
			break;
		default:
#endif
			query = find_person(to);
			vSet("query", query ? query->qName() : to);
			P3(("talk(%O,%O) w/ %O = %O\n",
			    to, handleAliases, v("query"), query))
#ifdef ALIASES
		}
#endif
		w("_notice_query_on",
			"Dialogue with [_nick_target] started.", ([
			    "_nick_target": v("query")
		]) );
		return 1;
	}
	else if (v("query")) {
		w("_notice_query_off",
		    "Dialogue with [_nick_target] ended.", ([
			"_nick_target": v("query")
		]) );
		vDel("query");
		query = 0;
		return 0;
	}
}

tell(pal, what, palo, how, mc, tv) {
	string deaPal; // dealiased pal

#ifndef _flag_enable_unauthenticated_message_private
	if (IS_NEWBIE) {
#ifdef VOLATILE
		w("_error_unavailable_function_here",
		   "This function is not available here.");
#else
		w("_error_necessary_registration",
		   "Sorry, you cannot use this without prior registration.");
#endif
		vDel("query");
		return;
	}
#endif
#ifdef ALIASES
        // this also allows for /alias MEP MunichElectropunk
       deaPal = aliases[lower_case(pal)] || pal;
#else
       deaPal = pal;
#endif
	unless(palo) {
		// used to be lower_uniform, but then jabber resources
		// like xmpp:ve.symlynx.com/Echo don't work
		unless (palo = is_formal(deaPal)) {
		    if (deaPal = legal_name(deaPal)) {
			// there must be a better way... summon_person()
			// and find out if that user existed before..
			// i'll find it once. otherwise, i'll probably
			// code it.
			// and i'll find out whether two find_person are
			// better than find_person and a call_other()
			unless (palo = find_person(deaPal)) {
			    if (palo = summon_person(deaPal, load_name())) {
#ifndef RELAY
				if (palo->isNewbie()) {
				    destruct(palo);
				    w("_error_unknown_name_user",
				      "Cannot reach [_nick_target].",
				      ([ "_nick_target" : pal ]));
				    return;
				}
#endif
			    }
			}
		    }
		// needed? unless (pal) pal = palnick;
		}
	}
	/* dont allow tell ignored ppl */
	if (ppl[deaPal]&& ppl[deaPal][PPL_DISPLAY] == PPL_DISPLAY_NONE) {
		w("_error_status_person_display_none", 
	    "Before talking you must stop ignoring [_nick_target] first.",
		    ([ "_nick_target" : pal ]) );
		return 0;
	}
	
#ifdef LOCAL_ECHO
	if (how) {
		msg(0, "_message_echo_private", 0, ([
		    "_nick_target" : pal, "_action" : how ]) );
	} else {
		msg(0, "_message_echo_private", what, ([
		    "_nick_target" : pal ]) );
	}
#endif
        // _nick_target is not very interesting to the other
        // side, but it is copied back in the echo so you are
        // actually providing this for yourself..
	unless (tv) tv = ([]);
#ifdef _flag_enable_measurement_network_latency
	tv["_time_sent"] = time();
#endif
	tv["_nick"] = MYNICK;
	tv["_nick_target"] = pal;
	if (how) tv["_action"] = how;
	if (v("color")) tv["_color"] = v("color");

	unless (mc) mc = stringp(what) &&
#ifdef OLD_QUESTION_RECOGNITION_ANY_QUESTION_MARK_WILL_DO
		index(what, '?') != -1
#else
	       	what[<1] == '?'
#endif
	   ? "_message_private_question" : "_message_private";
#if 0 // def TAGGING
	// echo tagging doesnt work yet
	sendmsg(palo, mc, what, tv, ME, 0, 
		(: 
		 // wir nutzen hier what und die tv's weiterhin!
		 // dont show the source ($1) here
		 // das ist nicht so ideal...
		 // TODO: das hier funktioniert noch nicht ideal
		 // 	und ALLE callbacks brauchen error handling
		 // 	(nicht existenter user, etc)
//		    w("_message" + $2, what, tv, $1);
		    msg($1, "_message" + $2, what, tv);
		    return; 
		:));
#else
	unless (sendmsg(palo, mc, what, tv)) {
		// synchronous error handling to be faded out.. to be replaced by tagging
		//      w("_error_unknown_name_user",
		//              "Can't find \"[_nick_target]\".", ([ "_nick_target": palnick ]) );
		return;
	}
#endif
	return palo;
}
#endif

friendcast(mc, data, vars, friendivity) {
        unless (vars) vars = ([ ]);
        // recipient could decide to recast with reduced friendivity...
        // but doesn't make much sense as long as we cannot sort out dupes
        //if (friendivity) vars["_amount_friendivity"] = friendivity;
        if (castmsg(mc || "_message_friends", data, vars))
#ifdef GAMMA
	    // to put the echo into lastlog, we need to msg it to ourselves
            msg(0, mc? "_echo"+mc : "_message_echo_friends", data, vars);
#else
            w(mc? "_echo"+mc : "_message_echo_friends", data, vars);
#endif
}

// should we run all /cmds thru this, so it's simply /laugh ?
act(mc, target, source) {
        string t, t2;

        t = "_notice_action_"+ mc;
        unless (mc && (t2 = T(t,"")) && t2 != "") {
//		w("_warning_usage_action",
//		    "Usage: /action <prefabricated>");
                return 0;
        }
        // _notice_action_get_drink himself herself
        sendmsg(target, t, t2, ([ "_nick": MYNICK,
                                   "_possessive": "the" ]), source);
                                // gender support coming soon! TODO
        return 1;
}

action(m, dest, posse) {
        P2(("action(%O,%O,%O) in %O\n", m, dest, posse, ME))
	if (!m || m == "") {
		if (posse) w("_warning_usage_my",
		    "Usage: /my <possessive description> or :s <description>.");
		else w("_warning_usage_me",
		    "Usage: /me <action description> or :<action>.");
		return 1;
	}
#ifdef USER_PROGRAM
	if (dest) {
		tell(dest, 0, 0, m, posse);
		return 1;
	}
	if (v("query")) { // query
		tell(v("query"), 0, query, m, posse);
		return 1;
	}
#endif
#if 0 // ... actually.. elridion.. we don't understand you
	// this is absolutely extremely necessary to work with psyc-users
	if (place && v("place"))
#else
	if (place)
#endif
       	{
#ifdef USER_PROGRAM
		mapping tv;
		tv = ([ "_nick" : MYNICK, (posse ? ("_action"+posse)
						 : "_action") : m ]);
		if (v("coolname")) tv["_nick_local"] = v("coolname"); 
		if (v("color")) tv["_color"] = v("color");
		sendmsg(place, "_message_public", 0, tv );
		// msg(0, "_message_echo_public", 0, ([ "_action" : m ]) );
#else
		sendmsg(place, "_message_public", 0, ([ 
		    (posse ? ("_action"+posse) : "_action" ) : m ]), origin);
#endif
		return 1;
	} else {
		w("_error_status_place_none",
			"You aren't in a room.");
	}
}

#ifdef USER_PROGRAM
protected speak(a, dest, room) {
	if (dest) {
		tell(dest, a);
		return 1;
	}
#if 1 // the third argument is being used by _request_do_message only
	if (room) {
	    mixed t;

	    unless (objectp(room)) {
		unless (t = find_place(room)) {
			w("_error_illegal_name_place",
			    "Room name '[_nick_place]' is not permitted.",
				    ([ "_nick_place": room ]));
			return 1;
		}
		room = t;
	    }
	}
	else
#endif
	room = place;

	if (room) {
		unless(sayvars) {
			sayvars = ([ "_nick": MYNICK ]);
			if (v("speakaction"))
			    sayvars["_action"] = v("speakaction");
			if (v("color"))
			    sayvars["_color"] = v("color");
		//	if (v("longname"))
		//	    sayvars["_nick_long"] = v("longname");
#ifdef VOLATILE
			// what about peer scheme, host and -port
			sayvars["_location"] = (member(v("locations"), 0) && sizeof(v("locations")[0])) ?
			    m_indices(v("locations")[0])[0] : v("scheme")+"://"+v("host")+":-"+query_mud_port();
#endif
		}
#ifndef NO_PUBLIC_QUESTIONS
		if
# ifdef OLD_QUESTION_RECOGNITION_ANY_QUESTION_MARK_WILL_DO
		   (index(a, '?') != -1)
# else
		   (a[<1] == '?')
# endif
			sendmsg(room, "_message_public_question",
				a, copy(sayvars));
//				 v("color") ?
//				([ "_color": v("color"), "_nick": MYNICK ]) :
//						      ([ "_nick": MYNICK ]) );
		else
#endif
		{
		    // since room copies the vars, why should we? TODO
		    sendmsg(room, "_message_public", a, copy(sayvars));
		}
		return 1;
	} else {
                // also gets here when typing to the jabber server window
                // which is a bit irreführend...  TODO
		w("_error_status_place_none",
			"You aren't in a room.");
	}
}
#endif

protected invite(nick, vars) {
	object ob;

	unless (mappingp(vars)) vars = ([]);
#ifdef USER_PROGRAM
#ifdef ALIASES
	nick = aliases[lower_case(nick)] || nick;
#endif
#endif
#if defined(BRAIN) && defined(SMTP_PATH)
        // this BRAIN sonderfall is a potential cause for trouble
	unless (ob = lower_uniform(nick)) {
		ob = summon_person(nick, SMTP_PATH "user");
		nick = ob->qName();
	}
#else
	unless (ob = lower_uniform(nick)) {
		ob = find_person(nick);
		if (objectp(ob)) {
			unless (ob->online()) ob = 0;
			else nick = ob->qName();
		}
	}
	if (!ob) {
		w("_error_unknown_name_user",
		    "Cannot reach [_nick_target].", ([
			"_nick_target": nick
		]) );
		return 0;
	}
#endif
	vars["_nick"] = MYNICK;
	vars["_place"] = place;
	vars["_nick_place"] = NICKPLACE;
	vars["_nick_target"] = nick;
	// is it a _request or a _notice?
	if (sendmsg(ob, "_notice_invitation",
	    "[_nick] invites you into [_nick_place].",
	    // the nick in the vars is modified by the recipients aliases!
	    copy(vars), origin)) {
		vars["_invitation"] = ob;
		sendmsg(place, "_request_invitation",
			"[_nick] invites [_nick_target] into [_nick_place].",
			vars, origin);
		// room will decide to notify or not.. so no echo
	//	w("_echo_invitation",
	//		"[_nick_target] invited into [_nick_place].", vars);
	}
	return 1;
}

#ifdef USER_PROGRAM
# ifndef _flag_disable_module_friendship
// two ways to call this:
// the gui style is	friend(deleteflag, entity, optlNick))
// the cmdline style is	friend(deleteflag, 0, nickname)
friend(rm, entity, ni, trustee) {
	mixed t;

	P0(("friend(%O, %O, %O, %O) in %O\n", rm, entity, ni, trustee, ME))
	if (IS_NEWBIE) {
#ifdef VOLATILE
		w("_error_unavailable_function_here",
		   "This function is not available here.");
#else
		w("_error_necessary_registration",
		   "You need to register first.");
#endif
		return;
	}
	if (stringp(entity)) {
		PT(("string entity %O provided in friend() - ignoring %O\n",
		    entity, ni))
//		unless (stringp(ni) && strlen(ni)) {
			t = lower_case(ni = entity);
//		} else {
//			raise_error("entity provided with nickname\n");
//		}
	} else {
		unless (stringp(ni) && strlen(ni)) return;
#ifdef ALIASES
		t = aliases[t = lower_case(ni)] || t;
#else
		t = lower_case(ni);
#endif
		if (rm != 2) {
		    if (is_formal(t)) entity = t;
		    else unless (entity = summon_person(t, load_name())) {
			w("_error_unknown_name_user",
			    "Cannot reach [_nick_target].",
			     ([ "_nick_target": ni ]) );
			// in the rm case:
			// sollten wir hier die Person nicht
			// aus friends/ppl rausnehmen?
			// so sie denn da drin ist
			// <lynX> du meinst wir könnten karteileichen
			// in unserem roster haben, die sich nicht
			// summonen lassen aber trotzdem rausmüssen?
			// ist mir bisher nicht unterlaufen..
			return;
		    }
		}
	}
	// doesn't strictly belong into here. flag == 2 means to
	// delete all data without trying to make any sense of it.
	// used by /cancel command.
	if (rm == 2) {
		if (member(ppl, t)) {
			w("_echo_acquaintance_deleted",
			    "All contact data for [_nick_target] deleted.", 
			    ([ "_nick_target": ni ]) );
			m_delete(ppl, t);
			m_delete(friends, entity);
			return 1;
		}
		w("_error_unknown_acquaintance",
		    "No contact data for [_nick_target] defined.",
		     ([ "_nick_target": ni ]) );
		return;
	} else if (rm) {
		if (member(ppl, t)	// && ppl[t][PPL_NOTIFY]
		    && ppl[t][PPL_NOTIFY] != PPL_NOTIFY_NONE) {
			sPerson(t, PPL_NOTIFY, PPL_NOTIFY_NONE);
#ifndef FRIEND_ECHO
			w("_echo_friendship_removed",
			    "Friendship with [_nick_target] deleted.", 
			    ([ "_nick_target": ni ]) );
#endif
                        // leave psyc context slave.. does nothing when
                        // the entity is an xmpp or icq or whatever uniform
                        if (stringp(entity))
                            deregister_context(ME, entity);
			// this does not consider the case when we just
			// offered a friendship to a person. XMPP behaviour
			// is to let the other side know about our "mistake".
			// if noone has hard feelings about this, we should
			// choose to behave compatibly, which in this case
			// simply means to remove this if check.
			//if (friends[entity]) {
			sendmsg(entity, "_notice_friendship_removed",
		"[_nick] deletes [_possessive] friendship with you.",
						// TODO: gender support
			    ([ "_nick": MYNICK, "_possessive": "the" ]) );
			m_delete(friends, entity);
			PT(("entity %O (%O) removed from friends %O\n",
			     entity, t, friends))
			//}
			return 1;
		}
		w("_error_unknown_friendship",
		    "No friendship settings for [_nick_target] defined.",
		     ([ "_nick_target": ni ]) );
		return;
	}
	if (objectp(entity)) {
		if (entity->isNewbie()) {
			// checked again on reception, good.
			w("_failure_necessary_registration",
			    "[_nick_target] cannot handle friendships yet.", 
			    ([ "_nick_target": ni ]) );
			return;
		}
#if 0	// dangerous change.. lynX 2006-11-06
	// no -> magic, let's wait for proper _status message
		else if (entity->online()) {
		    friends[entity, FRIEND_NICK] = ni || 1;
		    friends[entity, FRIEND_AVAILABILITY] = AVAILABILITY_HERE;
		}
#endif
	}
//	if (member(ppl, t)) switch(ppl[t][PPL_NOTIFY]) {
	switch (member(ppl, t) ? ppl[t][PPL_NOTIFY] : PPL_NOTIFY_NONE) {
	case PPL_NOTIFY_OFFERED:
		sPerson(t, PPL_NOTIFY, PPL_NOTIFY_DEFAULT);
		friends[entity, FRIEND_NICK] = ni || 1;
		if (objectp(entity))
		    insert_member(entity);
		else
		    insert_member(entity, parse_uniform(entity, 1)[URoot]);

#ifdef _flag_enable_module_microblogging
		string uni = psyc_name(ME);
		sendmsg(entity, "_notice_place_enter_automatic_subscription_follow",
			"Following [_nick_place]", (["_nick": MYNICK, "_nick_place": uni + "#follow" ]));
		sendmsg(entity, "_notice_place_enter_automatic_subscription_follow",
			"Following [_nick_place]", (["_nick": MYNICK, "_nick_place": uni + "#friends" ]));
#endif
		// this used to imply a symmetric request for
		// friendship, but we prefer to make it an
		// informational message instead. the protocol
		// should not enforce symmetric friendships.
		sendmsg(entity, "_notice_friendship_established",
			    "[_nick] is your friend.",
			  ([ "_nick": MYNICK ]) );
#ifndef FRIEND_ECHO
		w("_echo_notice_friendship_established",
		    "Accepting friendship with [_nick].", 
		  ([ "_nick": ni ]) );
#endif
		// this should not be displayed at all
		// within psyc.. even jabber should
		// normally auto-acknowledge this request
		sendmsg(entity, "_request_friendship_implied",
			0, ([ "_nick": MYNICK, "_degree_availability": availability ]) );
		// don't know how this hack got here but it drives some
		// jabber servers nuts as you are not supposed to probe
		// people that you aren't subscribed to, yet
		//sendmsg(entity, "_request_status_person",
		//	0, ([ "_nick": MYNICK ]) );
		// we should instead ensure we are always sending our presence
		// status once a subscription is completed.. FIXME
		// or we just scrap it all and redo context subscription
		// strictly as suggested by the new specs.. sigh
#ifdef TRY_THIS
		// currently friend() only gets called from
		// online commands. so we skip the if
		//if (ONLINE)
		sendmsg(entity, "_status_presence_here",
		     0, ([ "_nick": MYNICK ]) );
		// should a _notice_friendship_established
		// carrying _presence be enough? well, we
		// don't have _presence yet
#endif
		return 1;
	case PPL_NOTIFY_NONE:
                sPerson(t, PPL_NOTIFY, PPL_NOTIFY_PENDING);
#ifndef FRIEND_ECHO
                w("_echo_request_friendship",
                    "Asking [_nick] for friendship.", 
                    ([ "_nick": ni ]) );
#endif
		break;
	case PPL_NOTIFY_PENDING:
#if 1
		w("_warning_pending_friendship",
		    "You already have initiated a friendship with [_nick], "
		    "but let's reinforce that...",
		  ([ "_nick": ni ]) );
                break;
                // but let's give it another try!
#else
		w("_error_pending_friendship",
"You already have initiated a friendship with [_nick].",
		  ([ "_nick": ni ]) );
		return;
#endif
	default:
#if 1
		w("_warning_duplicate_friendship",
"You already have a friendship with [_nick], but let's be sure about it...",
		  ([ "_nick": ni ]) );
                break;
                // but let's give it another try!
#else
		w("_error_duplicate_friendship",
"You already have a friendship with [_nick].",
		  ([ "_nick": ni ]) );
		return;
#endif
	}
	// ich haette das gerne ueberladbar
	// then check for that mc in jabber::w()
	// if you really want to msg(), which is a bad and
	// evil idea, then use a "correct" source and the _real_
	// nick, not the one the user typed in.
	t = ([ "_nick": MYNICK ]);
	if (trustee) t["_trustee"] = trustee;
	sendmsg(entity, "_request_friendship", 0, t);
	return 1;
}
# endif // _flag_disable_module_friendship
#endif

static wake(entity) {
        mixed ob;

        unless (stringp(entity) && strlen(entity)) {
                w("_warning_usage_wake", "Usage: /wake <person>");
                return 1;
        }
#ifdef ALIASES
        // some client coders think aliases should work in all situations
        // also for them.. some psyc purists may think this is not elegant.
        // i don't know, we can ifdef it
	entity = aliases[entity = lower_case(entity)] || entity;
#endif
#if 0
        string t2;

	if (is_formal(entity)) ob = entity;
	else if (ob = find_person(entity)) {
		if (!ob->online()) ob = 0;
		if (!ob) {
			w("_error_unknown_name_user",
			    "Cannot reach [_nick_target].", 
			    ([ "_nick_target": entity ]) );
			break;
		}
		// mannomann wieso so unpsycig!??
		t2 = ob->vQuery("scheme");
		// die anderen müssen es einfach nur lernen... TODO
		if (t2 != "tn" && t2 != "ht" && t2 != "irc") {
			w("_error_invalid_protocol_wake",
			  "[_nick_target] has incompatible "
			  "connection type for wake.", ([
				"_nick_target": entity
			]) );
			break;
		}
	}
#else
	if (is_formal(entity)) ob = entity;
	else ob = find_person(entity);
#endif
        // should better get the echo from the other side.. how to
        // go about that? special case or rather use a family like
        // _request or even _message?   TODO
	w("_echo_wake", "Trying to wake [_nick_target].", ([
		"_nick_target": entity
	]) );
	// receiving side should typically reject wake unless
        // friends.. TODO
        // would something like _notice_friend_attention
        // be more appropriate?
        // user should also be allowed to set a minimum idle
        // time before things are delivered as alerts...
        sendmsg(ob, "_request_attention_wake",
                "[_nick] seeks your attention. WAKE[_beep]UP!", ([
                        "_nick": MYNICK,
                        "_beep": " «beep» " // replace with your beep
                ]) );
        return 1;
}

static examine(pal, target, trustee, tag, format) {
        mapping vars;

	unless(pal) {
                // we don't really need it anyhow..
                if (target) pal = to_string(target);
#ifdef USER_PROGRAM
                else unless (pal = v("query")) {
                        target = ME; pal = MYNICK;
                }
#else
                else {
                        // produce an error?
                        // now, /examine shouldn't let us get here
                        D("Impossible. I can't be here in examine()\n");
                        return 1;
                }
#endif
	}
	unless(target) {
#ifdef ALIASES // && defined(USER_PROGRAM)?
		string t = aliases[lower_case(pal)] || pal;
#else
		string t = pal;
#endif
		// should it be '#' instead? or '*' rather?
		if (t[0] == '@') {
			// examine a local place
			target = find_place(t[1..]);
		} else if (is_formal(t)) {
			// if the object has already been created, we can
			// use it rightaway. otherwise we let sendmsg parse
			// the uniform and find out it is a local one.
			unless (target = psyc_object(t)) {
				target = t;
				w("_echo_request_description",
			   "Requesting description from [_location_target].",
				   ([ "_location_target" : target ]));
			}
		}
		//else target = find_person(pal);
		else if (target = summon_person(t, load_name())) {
#if 1
			if (target->online(1) == 0) target = 0;
#else
			if (target->isNewbie() && !target->online()) target = 0;
#endif
		}
	}

	if (stringp(trustee) && !is_formal(trustee)) {
		// we assume it is a nick
		object o = summon_person(trustee);
		if (o) trustee = psyc_name(o);
	}
	vars = ([
            // "_degree_twilight": boss(ME),
            "_nick": MYNICK,
            // yes, it is okay to encode custom metadata into the tag
            // it's not a dirty hack since only i need to understand this
            "_tag": format +" "+ (tag || RANDHEXSTRING),
        ]);
	if (trustee) vars["_trustee"] = trustee;

	unless(target && sendmsg(target, "_request_description",
	       "[_nick] looks up your description.", vars, origin, 0, #'msg)) {
		w("_error_unknown_name_user", 0, ([ "_nick_target": pal ]) );
		return;
	}
#ifdef USER_PROGRAM
	vSet("examine", target);
#endif
	return target;
}

#ifdef USER_PROGRAM
checkVar(key, value) {
	string a, b;
	array(string) t;

	// aliases for variable names
	switch(key) {
	case "befehlszeichen":
	case "cmdchar": key = "commandcharacter"; break;
	case "farbe":
	case "colour": key = "color"; break;
	case "geschlecht":
	case "gender": key = "sex"; break;
	case "sprache":
	case "lang": key = "language"; break;
	case "languages": key = "alternatelanguage"; break;
	case "actchar": key = "actioncharacter"; break;
	case "ignorecolours": key = "ignorecolors"; break;
	case "nick": key = "name"; break;
	case "realname": key = "publicname"; break;
	case "telephone": key = "phone"; break;
	case "identities": key = "otherid"; break;
	}
// we go thru the whole process to be able to tell which keys exist
//	if (!stringp(value) || value == "") {
//		// return 1 allows /set to display contents of variable
//		// we dont want that for password.
//		unless (key == "password") return 1;
//	}
	switch(key) {
	case "verbatimuniform":
#ifdef _flag_encode_uniforms_IRC
		w("_failure_set_uniform_verbatim",
		    "Ability to filter @ from IRC uniforms has been discontinued on this server.");
		return;
#endif
	case "entersilent":
	case "ignorecolors":
	case "clearscreen":
	case "exposetime":
	case "visibility":	// toggle settings with positive default
		if (value == "on" || value == "-") value = "-";
		else if (value) value = "off";
		break;
	case "fulljid":		// used by net/irc
#ifndef NO_CTCP_PRESENCE
	case "ctcppresence":	// just for irc users really..
#endif
#ifdef _flag_enable_place_single
	case "multiplace":
#endif
		// toggle settings with negative default
		if (value == "off" || value == "-") value = "-";
		else if (value) value = "on";
		break;
	case "postalcode":	// integer settings
	case "latitude":
	case "longitude":
		unless (value = to_int(value)) value = "-";
		break;
	case "password":
#if defined(VOLATILE) || defined(RELAY)
		w("_failure_set_password",
		    "You cannot set a password on this server.");
		return;
#else
		if (v("password")) {
			unless(value && sscanf(value, "%s%t%s", a,b)) {
			    w("_warning_usage_set_password",
				"Usage: /set password <oldpass> <newpass>");
			    return;
			}
			if (a != v("password")) {
			    w("_error_invalid_password",
				"That's not your password, [_nick].", 
				([ "_nick": MYNICK ]) );
			    return;
			}
			value = b;
		}
		if (!value || value == "") {
		    w("_warning_usage_set_password_register",
			"To register use /set password <newpass>.");
		    return;
		}
		// unset password explicitely permitted
		if (value == "-") return 1;
		// following code also appears in person.c:_set_password
		if (t = legal_password(value, MYNICK)) {
// this needs some further preparation TODO
//		    w(t[0], t[1]);
		    pr(t[0], t[1]);
		    return;
		}
# ifdef PASSWORDSET
		value = PASSWORDSET(value, MYLOWERNICK);
# endif
#endif
		break;
	case "stylesheet":
		key = "stylefile";	// fall thru
	case "stylefile":
	case "publicpage":
	case "privatepage":
	case "startpage":
	case "keyfile":
		if (value) unless (value = legal_url(value, "http")
		             || legal_url(value, "https")
		             || legal_url(value, "ftp")) value = "";
		break;
	case "speakaction":
	case "askaction":
		a = strlen(value);
		if (a < 3 || a > 15
#ifdef UTF8_TROUBLE_WITH_SPEAKACTION
		    || index(value, 160) >= 0
		    || index(value, '\t') >= 0
		    || index(value, '\n') >= 0
		    || index(value, '\r') >= 0
		    || index(value, '\f') >= 0
		    || index(value, ' ') >= 0
#else
                    || !(value = legal_name(value))
#endif
                   )
			value = "";		// clear it
		else value = lower_case(value);
		sayvars = 0;
		break;
	case "email":
		unless (value) break;
		unless (value = legal_mailto(value)) {
			w("_error_invalid_mailto",
	  "Sorry, that doesn't look like a valid email address to me.");
			value = "";
		}
		vDel("emailvalidity");
		break;
	case "longname":
		if (value != "-") {
		    a = legal_name(value);
		    if (!a || stricmp(a, v("name"))) {
			w("_error_invalid_setting_name_long",
			    "You may only change the style of your name.");
			return;
		    }
		}
		break;
	case "name":
		unless (value) break;
		if (stricmp(value, v("name"))) {
			w("_error_invalid_setting_name",
			    "You may only change the case of your name.");
			return;
		}
		vDel("longname");
		// this *does* belong here, but for safety of encapsulation i
		// would have done ::sName(value) instead. let's call this ok:
		_myNick = value;
		break;
	case "filter":
		unless (value) break;
		if (value == "on" || abbrev(value, FILTER_STRANGERS))
                    value = FILTER_STRANGERS;
		else if (value != FILTER_NONE) value = "-";
		break;
	case "presencefilter":
		unless (value == "on" || value == "off" ||
		       	value == "all" || value == "none")
		    value = "-";
		break;
	case "color":
		sayvars = 0;
		switch(strlen(value)) {
		case 7:
                        // hex2int actually fails on #000000  ... TODO?
			if (value[0] == '#' && hex2int(value[1..])) return 1;
			break;
		case 6:
			if (hex2int(value)) {
				value = "#"+value;
				return 1;
			}
			break;
		case 4:
			if (value[0] == '#') value = value[1..];
			else break;
		case 3:
			if (hex2int(value)) {
				value = sprintf("#%c%c%c%c%c%c",
					value[0],value[0],
					value[1],value[1],
					value[2],value[2]);
				return 1;
			}
			break;
		}
		value = "-";
		break;
	case "language":
		if (value && index( ({ LANGUAGES }), value) < 0) value = 0;
		if (value) sTextPath(v("layout"), value, v("scheme"));
		break;
	case "sex":
		switch(value) {
		case "male":
		case "mann":
			value = "m";
		case "m":
			break;
		case "female":
		case "frau":
			value = "f";
		case "f":
			break;
		case "neuter":
		case "neutrum":
		case "kind":
			value = "n";
		case "n":
			break;
		case "unspecified":
		case "egal":
		default:
			value = "-";
		case "-":
			break;
		}
		break;
	case "groupsexpose":
                // fall thru, use same values
	case "friendsexpose":
		// special case off, otherwise friendivity 1..9
		// where 0 is deleted because it is the default.
		// showing your friends to at least your own friends
		// is the fundament of any social network technology.
		// not being able to "surf" any further by default
		// is already a terrible limitation compared to the
		// commercial friends platforms. degree 1 should
		// probably be the default really. degree 0 is ok for now.
		unless (value == "off" || (strlen(value) == 1
		       && value = to_int(value))) value = "-";
		break;
	case "timestamp":
		// "plain" timestamps is the default
		// "on" means IRCers receive timestamp by "CTCP TS"
		unless (value == "off" || value == "on") value = "-";
		break;
	case "echo":
		unless (value == "off" || value == "on") value = "-";
                ASSIGN_SHOWECHO(value, v("scheme"))
		break;
	case "greeting":
		unless (value == "off" || value == "on") value = "-";
		break;
	case "visiblespeakaction":
#ifdef VISIBLESPEAKACTION_BY_DEFAULT
		// default is to do it "automatically"
		unless (value == "off" || value == "on") value = "-";
#else
		// default is not to show speakactions
		// because the cultural shock for ircers
		// is too hard and most irc clients still
		// have an ugly way to show actions
		unless (value == "on" || value == "mixed") value = "-";
#endif
		break;
	case "actioncharacter":
#ifndef actchar
		if (strlen(value) == 1 && value != "-") actchar = value[0];
		else if (value == "off") actchar = 0;
		else {
			actchar = T("_MISC_character_action", ":")[0];
			value = "-";
		}
		break;
#else
		w("_error_invalid_setting_character_action",
		    "You cannot change your action character here.");
		return;
#endif
	case "commandcharacter":
#ifndef cmdchar
		if (strlen(value) == 1 && value != "-") cmdchar = value[0];
		else {
			cmdchar = T("_MISC_character_command", "/")[0];
			value = "-";
		}
		break;
#else
		w("_error_invalid_setting_character_command",
		    "You cannot change your command character here.");
		return;
#endif
	case "organisation": // maybe one day we'll want two different fields
		key = "affiliation"; // fall thru
//	case "affiliation":
//	case "languages":
//	case "publicname":
//	case "likestext":
//	case "dislikestext":
//	case "publictext":
//	case "privatetext":
//	case "telephone":
//	case "identities":
//	case "home":
//	case "animalfave":
//	case "popstarfave":
//	case "musicfave":
		break;	// always valid
	case "encoding":
		key = "charset"; // fall thru
	case "charset":
		unless (value) {
		    value = "";
		    break;
		}
		switch(value = upper_case(value)) {
		case "UTF-8":
		case "UTF":
		case "UTF8":
		case "UNICODE":
			value = "UTF-8";
			break;
		case "CP1252":
		case "WINDOWS":
		case "WINDOWS-1252":
			value = "CP1252";
			break;
		case "ISO-8859-1":
		case "LATIN":
		case "LATIN1":
#ifdef GOODADVICE
			// consider switching to utf8.
			// as there are terminals and terminal emulations out
			// there not supporting that, you may keep on using -1.
			value = "ISO-8859-1";
			break;
#endif
		case "ISO-8859-15":
		case "LATIN9": // yes, 8859-15 equals LATIN9, not LATIN15.
			// deprecated charset. use utf8.
			// you're most probably using -15 because you want the
			// EURO sign, not for compatibility, which is bad in
			// communication with others
			value = "ISO-8859-15";
			break;
		default:
#ifndef ALLOW_ANY_CHARSET
			value = "-";
#endif
		}
#ifdef ALLOW_ANY_CHARSET
		if (value == SYSTEM_CHARSET) value = "-";
#endif
		break;
	default:
                // we use LF only here
                if (trail("text", key) && stringp(value))
                    value = replace(value, "\r", "");
		// allow all other settings from profiles.gen
		// to have any string value
		if (convert_setting(key, "set")) break;
		w("_error_unknown_setting",
		   "No such setting defined: [_key_set].", 
		   ([ "_key_set": key ]) );
		return;
	}
	return 1;
}
set(key, value) {
	mixed t = value;

	key = lower_case(key);
#ifdef DRIVER_HAS_CALL_BY_REFERENCE
	unless (checkVar(&key, &value)) return;
#endif
	unless (t) {
		if (v(key)) w("_info_set",
			    "Setting \"[_key_set]\" is \"[_value]\".", ([
				"_key_set": key,
				"_value": v(key)
			]) );
		else w("_info_set_none",
			"Setting \"[_key_set]\" is currently unset.", ([
			    "_key_set": key,
		    ]) );
		return 1;
	}
	else unless (value) {
		 w("_error_invalid_setting_value",
	       "Sorry, that is not a valid value for the [_key_set] setting.", 
		   ([ "_key_set": key ]) );
		return 1;
	}
	if (value == "" || value == "-") {
		vDel(key);
//		w("_echo_set_none",
//		    "Ok. Variable \"[_key_set]\" deleted.", 
		w("_echo_set_default", 
	  "Setting [_key_set] has been reset to its default state.",
		    ([ "_key_set" : key ]) );
#ifdef _flag_disable_registration
	} else if (key == "password" && IS_NEWBIE) {
# ifdef REGISTER_URL
		w("_failure_disabled_function_register_URI", 0, ([ "_page_register": REGISTER_URL ]));
# else
		w("_failure_disabled_function_register");
# endif
#endif
	} else {
		vSet(key, value);
		if (key == "password") w("_echo_set_password",
		    "Ok. You have a new password now.");
		else w("_echo_set",
		    "Ok. Setting [_key_set] is \"[_value]\" now.", ([
			"_key_set": key,
			"_value": v(key)
		]) );
	}
	// (un)registering by /set password allowed
	save();
#ifdef PSYC_SYNCHRONIZE
	string psyc = convert_setting(key, "set");
	mapping vars = ([
	    "_nick": MYNICK,
	    "_key_set": key,
	    "_key": psyc,
	    "_value": v(key)
	]);
	if (t = convert_setting(psyc, 0, "LDAP"))
	    vars["_key_LDAP"] = t;
	if (t = convert_setting(psyc, 0, "vCard"))
	    vars["_key_vCard"] = t;
	synchro_report("_notice_synchronize_set",
	    "[_nick] has set \"[_key_set]\" to \"[_value]\".", vars);
#endif
	return 1;
}

listAcq(ppltype, pplvalue) { // Acq(uaintance) to be renamed into Peer
			    // or.. Contact?
	string prof, person, *p, *all;
	mixed mc;
	int i, s;
#ifdef ALIASES
	string pdisp;
#else
# define pdisp	person
#endif
	mapping share = shared_memory();

	p = m_indices(ppl);
	if (ppltype == PPL_JSON) all = allocate(sizeof(p));
	for (i=sizeof(p)-1; i>=0; i--) {
		person = p[i];
		prof = ppl[person];
		if (prof == "    ") {
			// unlikely to ever happen!
			P1(("Interesting.. %O had empty profile for %O\n",
			    ME, person))
			m_delete(ppl, person);
			// no special action should be needed for JSON
		} else {
#ifdef ALIASES
	// well, i wouldn't need to ;, but it helps the vim syntax parser
		    DEALIAS(pdisp, person);
#endif
		    if (ppltype == PPL_COMPLETE)
			w("_list_acquaintance_each",
//			    "[_acquaintance] known as [_nick]: D[_level_display], N[_level_notification], T[_level_trust], E[_level_expose].", ([
//			       "_level_trust": PPLDEC(prof[PPL_TRUST]),
//			      "_level_expose": PPLDEC(prof[PPL_EXPOSE]),
//			     "_level_display": PPLDEC(prof[PPL_DISPLAY]),
//			"_level_notification": PPLDEC(prof[PPL_NOTIFY]),
			    "[_acquaintance] ([_nick]): D[_type_display], N[_type_notification], T[_degree_trust], E[_degree_expose].", ([
		    "_degree_trust" : PPLDEC(prof[PPL_TRUST]),
		    "_degree_expose": PPLDEC(prof[PPL_EXPOSE]),
		"_type_display"     : share["_display"][prof[PPL_DISPLAY]],
		"_type_notification": share["_notification"][prof[PPL_NOTIFY]],
		   // should we call _acquaintance _person instead?
		     "_acquaintance": PERSON2UNIFORM(person),
			     "_nick": pdisp ]) );
		    else if (ppltype == PPL_JSON) {
			// should we call _contact _person instead?
			mapping m = ([ "_contact": PERSON2UNIFORM(person) ]);
#ifdef ALIASES
			if (pdisp != person) m["_nick_alias"] = pdisp;
#endif
			if (prof[PPL_TRUST] != PPL_TRUST_DEFAULT)
			    m["_degree_trust"] = PPLDEC(prof[PPL_TRUST]);
			if (prof[PPL_EXPOSE] != PPL_EXPOSE_DEFAULT)
			    m["_degree_expose"] = PPLDEC(prof[PPL_EXPOSE]);
			if (prof[PPL_DISPLAY] != PPL_DISPLAY_DEFAULT)
			    m["_type_display"] =
				    share["_display"][prof[PPL_DISPLAY]];
			if (prof[PPL_NOTIFY] != PPL_NOTIFY_NONE)
			    m["_type_notification"] =
				    share["_notification"][prof[PPL_NOTIFY]];
			all[i] = m;
		    } else {
			if (ppltype == PPL_ANY || ppltype == PPL_DISPLAY) {
				// should be using share["_display"]
				switch(prof[PPL_DISPLAY]) {
				case PPL_DISPLAY_NONE:
					mc = "none";
					break;
				case PPL_DISPLAY_SMALL:
					mc = "reduced";
					break;
				case PPL_DISPLAY_BIG:
					mc = "highlighted";
					break;
				default:
					mc = 0;
				}
				if (mc) w("_list_acquaintance_display_" + mc,
				 "Display of messages from [_nick]: "+ mc +".", 
					([ "_nick": pdisp ]) );
			}
			if (ppltype == PPL_ANY || ppltype == PPL_NOTIFY) {
			    s = prof[PPL_NOTIFY];
			    if (pplvalue == s || (!pplvalue &&
			       s > PPL_NOTIFY_FRIEND)) {
				// should be using share["_notification"]
				switch(s = prof[PPL_NOTIFY]) {
				case PPL_NOTIFY_IMMEDIATE:
					mc = "immediate";
					break;
				case PPL_NOTIFY_DELAYED:
					mc = "delayed";
					break;
				case PPL_NOTIFY_DELAYED_MORE:
					mc = "delayed_more";
					break;
				case PPL_NOTIFY_MUTE:
					mc = "mute";
					break;
				case PPL_NOTIFY_PENDING:
					mc = "pending";
					break;
				case PPL_NOTIFY_OFFERED:
					mc = "offered";
					break;
				default:
					mc = 0;
				}
				if (mc) w("_list_acquaintance_notification_" + mc,
					"Notification of [_nick] is "+ mc +".", 
					([ "_nick": pdisp ]) );
			    }
			}
			if ((ppltype == PPL_ANY || ppltype == PPL_TRUST) &&
				prof[PPL_TRUST] != PPL_TRUST_DEFAULT)
			    w("_list_acquaintance_trust",
			"Trust degree for [_nick] is [_degree_trust].", 
				([ "_degree_trust": PPLDEC(prof[PPL_TRUST]),
				   "_nick": pdisp ]) );

			if ((ppltype == PPL_ANY || ppltype == PPL_EXPOSE) &&
				prof[PPL_EXPOSE] != PPL_EXPOSE_DEFAULT)
			    w("_list_acquaintance_expose",
			"Expose degree for [_nick] is [_degree_expose].", 
				([ "_degree_expose": PPLDEC(prof[PPL_EXPOSE]),
				   "_nick": pdisp ]) );
		    }
		}
	}			  // not really a _list method then, huh?
	if (ppltype == PPL_JSON) w("_list_acquaintance_JSON", make_json(all));
	else w("_list_acquaintance_end");   // end marker for psyc clients
#ifdef pdisp
# undef pdisp
#endif
}

#if 0
bye(a) {
#if 0
	// handled by user:quit() 
	if (place)
	    // sendmsg(place, "_status_place_person_leave_logout", a,
	    sendmsg(place, "_notice_place_leave_logout", a,
		    ([ "_nick" : MYNICK ]) );
	else
#endif
            w("_echo_logoff", "You leave the world.");
	return quit();
}
#endif

sanction(type, text) {   
	switch(type) {          
	case "warn":            
		// for once the correct way to handle this.
		msg(0, "_message_behaviour_warning", text, ([]));
//		if (text) w("_message_behaviour_warning", text);
//		else w("_message_behaviour_warning_default");
		return 1;
	case "kill":            
		msg(0, "_message_behaviour_punishment", text, ([]));
//		if (text) w("_message_behaviour_punishment", text);
//		else w("_message_behaviour_punishment_default");
		return quit();          
	}                               
}                       

static recompile(args, reload) {
	object o, o2;
	string i, t, s = "";
        mixed members, routes;
        int myplace = 0;

        PT(("%O\n", args))
	foreach(t : args) {
		o = find_object(t);
                myplace = o == place;
		unless(o) {
		    unless (reload) w("_error_unknown_object_destruct",
	    "No such object to destruct: [_object]", ([ "_object" : t ]) );
		} else {
		    mixed* ls = inherit_list(o);
                    members = o->members();
                    routes = o->routes();
		    if (o) catch(o -> quit());	// a chance to self-destruct
		    foreach(i : ls) {
			if (stringp(i)) o2 = find_object(i);
			else o2 = i;
			s += "» "+i+" ";
			if (i = destruct(o2))
			    s += "NOT! "+i+" #";
		    }
		    if (o && i = object_name(o)) {
			s += "» "+i+" ";
			if (i = destruct(o))
			    s += "NOT! "+i+" #";
		    }
		}
		if (reload) {
		    s += "« "+t+" ";
//		    if (i = catch(load_object(t)))
		    if (i = catch(o = call_other(t, "load")))
			s += "NOT! "+i+" #";
                    else if (o) {
                        if (members) o->members(members);
                        if (routes) o->routes(routes);
                    }
		}
                if (myplace) place = o;
	}
	if (strlen(s)) {
		if (reload)
		    w("_echo_recompile", "Recompiling [_classes].",
			([ "_classes": chop(s) ]) );
		else
		    w("_echo_destruct", "Destructing [_classes].",
			([ "_classes": chop(s) ]) );
	}
	return 1;
}

#if 0
walk(dir) {
	mixed room;
 
	unless(place) {
		w("_error_status_place_none",
			"You aren't in a room.");
		return;
	}
	room = place -> walk(dir);
	unless(room) {
		// _error_navigation ?
		w("_error_unknown_exit",
			"There's no exit in that direction.");
		return;
	}
	return teleport(room, "_navigate");
}
#endif

subscribe(how, arg, quiet) {
	mapping sups, size;

	if (IS_NEWBIE) {
#ifdef VOLATILE
		w("_error_unavailable_function_here",
		   "This function is not available here.");
#else
		w("_error_necessary_registration",
		   "You need to register first.");
#endif
		return;
	}
#ifdef SUBSCRIBE_PERMANENT
	sups = v("subscriptions");
	if (!sups) sups = ([ ]);
				// this puts zeros inside.. careful
	else if (widthof(sups) == 0) sups = m_reallocate(sups, 1);
#else
	sups = v("subscriptions") || m_allocate(1, 0);
#endif
	size = sizeof(sups);
	P3(("subscriptions (%d): %O\n", size, sups))
	if (arg && arg != "") {
		//unless (is_formal(arg)) arg = lower_case(arg);
		arg = lower_case(arg);
		if (how == SUBSCRIBE_NOT) {
			if (member(sups, arg)) {
			    m_delete(sups, arg);
			    placeRequest(arg,
#ifdef SPEC                  
                             "_request_context_leave"
#else                        
                             "_request_leave"
#endif
			     "_subscribe", 1, quiet);
			}
		} else {
			//string t;

			if (member(sups, arg)) {
			    unless (quiet) w("_error_duplicate_subscription",
			       "You already subscribed that.");
			    return;
			}
			if (size > MAX_SUBS && !boss(ME)) {
			    unless (quiet) w("_error_excessive_subscriptions",
			       "Too many subscriptions for you and me.");
			    return;
			}

			// the return value of placeRequest actually only
			// says that a request has been sent.. we allow
			// invalid place names here, because it's still
			// safer than doing the subscriptions at reception
			// of _notice_place_enter_subscribe.. that would
			// be like implementing HTTPs cookies..
	//		if (arg = placeRequest(arg, "_request_enter_subscribe"))
	//		    sups += ([ objectp(arg) ? arg->qName() : arg ]);
			if (find_place(arg)) {
			    // don't join to temporarily subscribed places when not online
			    if (ONLINE || how == SUBSCRIBE_PERMANENT)
				placeRequest(arg,
#ifdef SPEC
				     "_request_context_enter_subscribe",
#else
				     "_request_enter_subscribe",
#endif
				     0, quiet);
			    sups += ([ arg : how ]);
			} else return 1;
		}
		if (size != sizeof(sups)) {
			if (sizeof(sups))
				vSet("subscriptions", sups);
			else
				vDel("subscriptions");
			P2(("subscriptions now: %O\n", sups))
			save();
			return 1;
		}
	} else if (size && how) {
//		pr("_list_subscriptions",
//		    "Your subscriptions are %O\n", sups);
		foreach (arg in sups) w("_list_subscriptions_each",
		    "Subscribed: [_subscription]",
		    ([ "_subscription": arg ]) );
		return 1;
	}
	unless (quiet) w("_warning_usage_subscribe",
	   "Usage: /sub(scribe) or /unsub(scribe) <place>");
	return;
}
#endif

// do tagging with USER_PROGRAM only, or always?
static placeRequest(where, mc, leave, quiet, morevars) {
        mixed cb = 0;
	P2(("placeRequest(%O,%O,%O,%O)\n", where, mc, leave, quiet))
	if (where) {
		mixed t;
		mapping vars = ([ "_nick" : MYNICK ]);
                // can a client overwrite _nick like this?
		if (morevars) vars += morevars;

		unless (t = find_place(where)) {
			unless(quiet) w("_error_illegal_name_place",
		    "Room name '[_nick_place]' is not permitted.",
			    ([ "_nick_place": where ]));
			return 1;
		}
                P3(("find_place produced %O for %O\n", t, where))
		where = t;
#ifdef USER_PROGRAM
		if (!leave
// <lynX> sending tags to local places is overkill in most cases, but
// sometimes those local places *do* indeed redirect you to a remote room,
// like news junctions do. then again, commenting this check out doesn't solve
// the problem, so let's not change anything that doesn't solve problems.
//
// from here
		    && (!objectp(where)
# ifdef SANDBOX
			|| objectp(where) && (!stringp(geteuid(where))
					      || geteuid(where)[0] != '/')
# endif
		   )
// up to here
		    ) {
                        cb = (: 
                              // TODO: this is a temporary hack
                              tags[vars["_tag_reply"] || vars["_tag"]] = 1;
                              return msg($1, $2, $3, $4); :);
		}
		else if (leave) {
		    // call out a forced leave
                        cb = (: 
                              return msg($1, $2, $3, $4); :);
                        // we use our right to leave the context ourselves
                        // after informing the place..
# ifdef BETA
                        // <fippo> but that makes any _notice_place_leave run
			// into the filter so i comment it out
                        leavePlace(where);
			// <lynX> i put it into EXPERIMENTAL ... i think
			// muve will not operate correctly without this, but
			// fippo is probably right, that it doesn't operate
			// correctly when this is in place, either.
			// TODOOOOOOOOOOOOOOOOOOOOOOOOOOOOO!!!!!!
# endif
		}
#endif
		P3(("calling sendmsg(%O, %O..\n", where, mc))
		sendmsg(where, mc, 0, vars, origin, 0, cb);
	}
	return where;
}

#ifdef USER_PROGRAM
teleport(where, mcv, quiet, stay, morevars) {
	mixed t;

	P3(("teleport(%O,%O,%O,%O,%O)\n", where, mcv, quiet, stay, morevars))
	if (where) {
		unless (t = find_place(where)) {
			// unless(quiet)    -- too important to be filtered
                        w("_error_illegal_name_place",
		    "Room name '[_nick_place]' is not permitted.",
			    ([ "_nick_place": where ]));
			return;
		}
		where = t;
	}
	unless(mcv) mcv = "";
	if (place && member(places, place)) {
#ifndef EXPERIMENTAL // ALTE_SCHULE
		if (place == where) {
                        P3(("teleport: %O is already in %O\n", ME, place))
                        // why error.. this should be a _warning !!
			unless(quiet) w("_error_status_place_matches",
                            "You already are in [_nick_place].",
                             ([ "_nick_place": NICKPLACE ]));
                        // used to return here, but for psyc clients we may
                        // run into here after a client crash and relink
                        // and want to receive room memberlist etc. so
                        // here's a kludge to not return in that case:
			unless (morevars) return;
		}
#endif
                if (NICKPLACE) {
                    vSet("lastplace", NICKPLACE);
                    unless (stay) {
			mapping subs = v("subscriptions");
			unless(subs && member(subs, lower_case(NICKPLACE)))
			    sendmsg(place,
#ifdef SPEC                  
				 "_request_context_leave"
#else                        
				 "_request_leave"
#endif
				 +mcv, "[_nick] leaves.",
				    ([ "_nick" : MYNICK, ]) );
                        // should vDel & =0 move outta here?
                        // dont think so.. but..
			vDel("place");
			place = 0;
                    }
		}
	}
	unless (where) return place;
	P3(("where %O in places %O\n", where, places))
	if (places[where]) {
		place = where;
                vSet("place", objectp(where) ? where->qName() : where);
#ifndef EXPERIMENTAL // ALTE_SCHULE
		unless(quiet) showRoom();
		return place;
#endif
	}
	return placeRequest(where,
#ifdef SPEC
			     "_request_context_enter"
#else
			     "_request_enter"
#endif
			   +mcv, 0, quiet, morevars);
}
#endif

#include "members.i"

// showRoom() in http/user is a lot more complicated... so keep this.
static showRoom(verbose) {
#if 0 // why should we switch to placeRequest?
      // it does nothing that we need it to do!
     //	placeRequest(place, verbose ? "_request_status" : "_request_members");
#else
	// provide verbosity here?
        if (place)
            sendmsg(place,
# ifdef USER_PROGRAM
			    v("scheme") == "irc" ?
		// special treatment for irc users. the place is asked not
		// to return neither member lists nor topics, as those have
		// to be handled in an irc-specific way. such a requirement
		// is so weird, that i just call it _IRC here.
		(verbose ? "_request_status_IRC" :
			   "_request_status_terse_IRC") :
# endif
		(verbose ? "_request_status" : "_request_members"),
	       	0, ([]), origin);
#endif
	return 1;
}

showStatus(verbosity) {
	if (verbosity & VERBOSITY_PLACE) showRoom(1);
	else if (verbosity & VERBOSITY_MEMBERS) showRoom(0);
#ifdef USER_PROGRAM
	if ((verbosity & VERBOSITY_EVENTS) &&! IS_NEWBIE) w("_status_events",
	       "Last invitation: [_place_invitation].\n"
	       "Last conversation: [_person_reply].\n"
	       "Last arrival: [_person_greet].", ([
# ifdef ALIASES
		   "_person_greet" : raliases[v("greet")] || v("greet") || "-",
		   "_person_reply" : raliases[v("reply")] || v("reply") || "-",
# else
		   "_person_greet" : v("greet") || "-",
		   "_person_reply" : v("reply") || "-",
# endif
		   "_place_invitation": v("invitationplace") || "-" ]) );
	if (verbosity & VERBOSITY_FRIENDS_DETAILS) showFriends(1);
	else if (verbosity & VERBOSITY_FRIENDS) showFriends(0);
# ifndef _flag_disable_module_presence
	if (verbosity & VERBOSITY_PRESENCE_DETAILS) showMyPresence(1);
	else if (verbosity & VERBOSITY_PRESENCE) showMyPresence(0);
# endif
#endif
}

#ifndef _flag_disable_query_server
who() {
	mapping uv;
	mixed* u;
	int all;
	string desc;

	u = objects_people();
        PT(("objects_people %O\n", u))
	all = sizeof(u) < 23;
	// same code in gateway/generic and http/user
	u = sort_array(u->qPublicInfo(all), (:
		unless (mappingp($1)) return 0;
		unless (mappingp($2)) return 1;
		return	lower_case($1["name"] || "") >
			lower_case($2["name"] || "");
	:) );
	foreach (uv : u) if (mappingp(uv)) {
		desc = uv["me"];
		if (desc || all) {
			if (desc)
			    w("_list_user_description", "[_time_idle]\t[_nick] [_action_description].", ([
				"_time_idle": uv["idleTime"],
				"_nick": uv["nick"], 
				"_action_description": desc
			    ]) );
			else
			    w("_list_user_description_none",
				"[_time_idle]\t[_nick] is online.", ([
				    "_time_idle": uv["idleTime"],
				    "_nick": uv["nick"],
			    ]) );
		}
	}
}

// complex new people function, allows sorted output
people() {
	mapping m, tmp;
	array(string) u;
	string k, p, limbo, sep;
	int showguests, guests, invisibles;

	sep = T("_MISC_separator_list", ", ");	// space muss schon sein..
	u = objects_people();
#if 1 //def VOLATILE
	showguests = 1; // sizeof(u) < 50;		TODO
#else
	showguests = sizeof(u) < 50;
#endif
	invisibles = guests = 0;
	m = ([]);
	foreach (k : u) {
		tmp = k -> qPublicInfo(showguests, 1);
		if (mappingp(tmp) && tmp["nick"]) {
			if (member(m, tmp["place"]))
				    m[tmp["place"]] += sep + tmp["nick"];
			else m[tmp["place"]] = tmp["nick"];
		}
		else if (tmp == 1) invisibles++;
		else if (tmp == 2) guests++;
	}
	if (member(m, 0)) {
		limbo = m[0];
		m_delete(m, 0);
	}
	tmp = ([]);
	foreach(k : m) {
		// objectp-check superflous here
		if (p = objectp(k) ? k->qPublicName() : k) tmp[p] = m[k];
		else {
			if (limbo) limbo += sep + m[k];
			else limbo = m[k];
		}
	}
	foreach(k : sort_array(m_indices(tmp), #'>)) // '
	    w("_list_places_members", "[_nick_place]: [_members].", ([ 
		"_nick_place": k,
		"_members": tmp[k]
	    ]) );
	if (limbo) w("_list_places_none", "(private) [_users_private].", ([ "_users_private": limbo ]) ); 
	if (guests) w("_list_places_amount_guests",
		      "[_amount_guests] guests.",
		      ([ "_amount_guests": guests ]) );
	if (invisibles) w("_list_places_amount_invisibles",
		      "[_amount_invisibles] invisibles.",
		      ([ "_amount_invisibles": invisibles ]) );
	return 1;
}
#endif // _flag_disable_query_server

#ifdef USER_PROGRAM
morph(user, nick, pw) {
	save();
	// we should probably do _unlink and _link from the new source huh?
	w("_notice_switch_identity",
	  "Switching identity from [_nick] to [_nick_next]", ([
	    "_nick": MYNICK, "_nick_next": nick
	]));
	return ::morph(user, nick, pw);
}
createUser(nick) {
	P3(("%O creating clone of %O for %O\n", ME, load_name(), nick))
	if (load_name()) return named_clone(load_name(), nick);
}
keepUserObject(user) {
	P3(("%O of %O finds that %O has scheme %O\n", ME, v("scheme"),
	    user, user->vQuery("scheme")))
        return user->vQuery("scheme") == v("scheme");
}
promptForPassword(user) {
//	w("_failure_unimplemented_authentication_connect",
//	  "Sorry, that name is registered and cannot be /connect'ed yet.");
	w("_warning_require_authentication_connect",
	  "Sorry, I need a password to connect that identity.",
	      ([ "_nick": user->qName() || "Nobody",
		 "_action": user->vQuery("me") || ".." ]) );
}
#endif

