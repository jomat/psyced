// vim:foldmethod=marker:syntax=lpc:noexpandtab
// $Id: user.c,v 1.565 2008/10/01 10:59:47 lynx Exp $
//
// server-side handler for a logged in IRC client
//
#include "irc.h"

// ctcp support etc
inherit IRC_PATH "decode";
//volatile string msa, msare, cc, bc, uc;

#include "user.h"
#include "error.h"
#include "reply.h"
#include "person.h"
#include "psyc.h"
#include "uniform.h"
#include "hack.i"


volatile int isService;
#ifdef IRC_ANNOYANCE
volatile int isGenervt;
#endif

qCharset() { return v("charset"); }

qHasCurrentPlace() { return 0; }

parse(a) {
	next_input_to(#'parse);
	::parse(a);
}

msg(source, mc, data, mapping vars, showingLog) {
	string t;
	int special;
	mixed a, res;
	
	P4(("irc:msg (%O,%s,%O,%O)\n", source, mc, data, vars))
	P2(("irc:msg (%O,%s,%O..)\n", source, mc, data))

	if (abbrev("_notice_place_enter", mc) && source == ME) {
	    // it seems there is a bug here if the remote room is reloaded
	    // and someone else from local server joins 
	    // source == ME should be false then
	    P0(("%O ignoring myself joining via _notice\n", ME)) // monitor report?
	    return;
	}
//#if DEBUG > 0 // searching for recursions
	if (vars["_INTERNAL_recursion_counter_" + MYNICK]) {
	    // ok.. no one seems to care to fix this bug so i will
	    // stop it jamming up the debug output
	    P1(("recursion mc %O in %O from %O\n", mc, ME, source))
// seen 2007-09-24 very often:
//   recursion mc "_request_ping" in net/irc/user#nisa from net/irc/user#nisa
// probably an autopinging client..
	    // here it gets its own log, so we can slowly examine when it
	    // happens and if it is indeed a bug or problem
	    log_file("IRC_RECURSION", "[%s] %O (%O, %O, %O, %O)\n", ctime(),
		     ME, source, mc, data, vars);
	}
	vars["_INTERNAL_recursion_counter_" + MYNICK]++;
//#endif
	res = ::msg(&source, mc, data, &vars, showingLog);

	/* diese lösung ist eigentlich falsch.. wir müssen das annehmen
	 * fremder templates generell bürokratisieren: origin wird
	 * davorgepappt, so wie es bei telnet der fall ist. dann isses
	 * auch egal wenn da ein : oder # vorn ist. oder? denk..
	 * vielleicht ist es so gar nicht so schlecht:
	 */
	if (!objectp(source) && !objectp(vars["_context"])
	    && stringp(data) && strlen(data) &&
	   (data[0] == ':' || data[0] == '#') && (!abbrev("_message", mc)
		    || T(mc, "") == "")) {
		monitor_report("_warning_abuse_invalid_data_IRC",
		    S("%O received %O from %O\n", ME, data, source));
		data = " "+data;
	}
	// this is partly a hack for a more generic 512-byte-length problem
	// but we need a #366 end of names... 
	// maybe call ::msg and then return writing the 366
#ifdef ENTER_MEMBERS //{{{
	if (mc == "_status_place_members")
	    return _status_place_members(source, mc, data, vars);
	else
#endif //}}}
	if (mc == "_message_announcement") {
	    // what about not checking for _context and using ::msg() or even
	    // w()?
	    foreach (mixed room, string np : places) {
		w(mc, data, vars +
		    ([ "_context" : room,
		       "_nick_place" : np ]), source);
		// wait a minute? this is faking a multicast here? JEEZ!! TODO
	    }
	}
	// das müsste das longname und coolname problem lösen
	// und vars soll ja angeblich eine kopie sein..
//	m_delete(vars, "_nick_local");

	return res;
}

static int namreply(mapping vars) {
	mixed u;

	// TODO: control == silent ist eigentlich nicht richtig,
	// 	control == keine join/part waere richtiger
		// && vars["_control"] != "silent"
	if (pointerp(u = vars["_list_members_nicks"]) && sizeof(u) > 1) {
		// PARANOID check missing but no longer necessary
		//if (pointerp(vars["_list_members"]))
// UNI is supposed to do this.
//		    u = implode(renderMembers(vars["_list_members"],
//				  vars["_list_members_nicks"]), " ");
		u = implode(u, " ");
		if (!strlen(u)) u = MYNICK;	    // paranoid
//		    if (strlen(u)) u += " " + MYNICK;
//		    else u = MYNICK;
# ifdef _flag_encode_uniforms_IRC
		u = uniform2irc(u);
# endif
	} else {
		// empty, quiet or overcrowded place.
		// some IRC clients require ANY info, so we give them ANY
		u = MYNICK;
		// normal behaviour, when _amount is sent instead of _list
		// w/o is american for without  ;)
		P3(("Empty or anonymous channel: %O w/o _list_members\n",
		    vars["_nick_place"]))
	}
	// this code is all weirdly redundant with net/user.c
	// doing renderMembers itself in some cases, in some not -
	// it is working sort-of, but next time it breaks it needs
	// a reorg across all schemes
	render("_status_place_members", 0, ([
		  "_nick_place" : vars["_nick_place"],
		  "_members" : u,
		  "_INTERNAL_nick_me" : MYNICK ]) );
	render("_status_place_members_end", 0, vars);
	return 1;
}

static int _status_place_members(mixed source, string mc,
				 mixed data, mapping vars) {
	mixed u = "";

	P4(("irc:_status_place_members got %O\n", vars))
	if (vars["_tag_reply"] == "W") {
	    mapping uv, v2;
	    string n;

	    /*
	    u = renderMembers(vars["_list_members"],
		 vars["_list_members_nicks"],
		 objectp(source));
	    PT(("renderMembers %O vs %O\n", u, vars["_list_members"]))
	    */
	    // the room could be empty
	    if (pointerp(vars["_list_members"]))    // _tab
	      foreach (u : vars["_list_members"]) {
		if (objectp(u)) {
		    uv = u -> qPublicInfo(1, 0, 1);
		    unless (mappingp(uv)) continue;
#ifdef ALIASES
		    n = raliases[uv["lowerNick"]] || uv["nick"];
#else
		    n = uv["nick"];
#endif
		    v2 = vars + ([
		       "_nick" : n,
		       "_nick_login" : uv["nick"],
		       "_identification_host" : SERVER_HOST,
// "_IRC_away" : time() - uv["aliveTime"] > 30*60 + 60*60 ? "G" : "H",
   "_IRC_away" : uv["idleTime"] > 30*60 + 60*60 ? "G" : "H",
		       "_IRC_hops" : "0",
		       "_IRC_identified" :
			    // 123456 = 34h, 1234567 = 14 days
			    uv["age"] > 123456 ? "~" :
			    uv["registered"] ? "" : "!",
		       "_IRC_operator" : uv["operator"] ?
			    "*" : "",
		       "_name" : uv["name"] || "",
		       "_identification" : psyc_name(u),
//				       "_description_action" : uv["me"] || "",
		    ]);
		} else {
		    if (stringp(u)) u = parse_uniform(u);
		    unless (pointerp(u)) {
			P0(("%O waaargh.. broken parse_uniform in %O\n",
			    ME, vars["_list_members"])) // _tab
			continue;
		    }
#ifdef ALIASES
		    n = raliases[lower_case(u[UString])] || u[UString];
#else
		    n = u[UString];
#endif
		    v2 = vars + ([
# ifdef _flag_encode_uniforms_IRC
		       "_nick" : uniform2irc(n),
# else
		       "_nick" : n,
# endif
		       "_nick_login" : u[UNick],
		       "_identification_host" : u[UHost],
		       "_IRC_away" : "H",
		       "_IRC_hops" : "1",
		       "_IRC_identified" : "",
		       "_IRC_operator" : "",
		       "_name" : "",
		       "_identification" : u[UString],
//				       "_description_action" : "",
		    ]);
		}
		w(mc + "_each", 0, v2);
	    }
	    w(mc + "_end_verbose", 0, vars);
	    return 1;
	} else return namreply(vars);
}

w(string mc, string data, mapping vars, mixed source) {
	string nick, nick_place, nick2, family;
	mapping di; // = printStyle(mc); // display infos
	mixed t, u = "";
	int glyph;

	P3(("%O: irc:w(%O, %O, %O, %O) %O\n", ME, mc, data, 0, source, vars))

#ifndef GHOST
	// should it be..?
	//unless (ONLINE) return;
	unless (interactive(ME)) return;
#endif
#ifdef VARS_IS_SACRED //{{{
	// "VARS_IS_SACRED" bedeutet dass *kein* copy gemacht wurde und
	// man deshalb hier paranoid sein muss. der normalfall ist, dass
	// der raum uns ne kopie gibt.. ist auch gut so, denn der irc code
	// versaut total die history der räume! einmal ein ircer
	// drin gehabt und aus vars["_time_place"] sind _prefix'e geworden
	// und _INTERNAL_source_IRC's sind im raum gespeichert und mehr so unsinn.
	if (mappingp(vars)) vars = copy(vars);
	else vars = ([]);
	// hmm.. wie die _time_place's aus der room history rausfallen
	// konnten ist mir ein rätsel.. aber ich habs mit eigenen augen
	// gesehen.. ich kopiers sogar nach /ve/data/damaged-rendezvous.o
	// ah.. der neue foreach code im place ist schuld
#else //}}}
	unless (mappingp(vars)) vars = ([]);
#endif
	if (trail("_important", mc)) {
	    mc = mc[..<11];
#ifdef PRO_PATH
	} else if (trail("_magnify", mc)) {
	    mc = mc[..<9];
	} else if (trail("_reduce", mc)) {
	    mc = mc[..<8];
#endif
// render() still doesn't handle unexpected notices like _unicast from MUC..
// so we don't get to see the nice leave messages. the inheritance mechanism
// isn't working in the order that is necessary here. freaky issue.
#ifndef SAFE_IRC
	} else if (abbrev("_notice_place_enter", mc)) {
	    mc = "_notice_place_enter";
	} else if(abbrev("_notice_place_leave", mc)) {
	    mc = "_notice_place_leave";
#endif
	}

	// currently only this _control is in use and 'tempofficial'
	if (vars["_control"] == "_silent") {
#ifndef SAFE_IRC
	    if (mc == "_notice_place_enter")
#else
	    if (abbrev("_notice_place_enter", mc))
#endif
	    {
		if (v("entersilent") != "off") return 7;
	    }
	    // war das hier nicht vorher da, wo es den kram besser abfing?!
	    // lynX ist schuld!
	    else if (abbrev("_echo_place_enter", mc)) {
		// maybe the smarter way to do this would be to send entersilent
		// out from placeRequest and also make members and status depend
		// on it. as now we can make irc join newsrooms with
		// 	 /set entersilent off
		// but still not know who's there.	TODO
		if (v("entersilent") != "off") return 7;
		mc = "_echo_place_enter_join";
	    }
	}
	// do we need a special case for _echo_place_leave_logout ?
	else if (abbrev("_echo_place_leave", mc))
	    mc[0..4] = "_notice";	// impractical distinction
	if (v("visiblespeakaction") == "on"
	    && !vars["_action"]
	    && (mc == "_message_public"
	     || mc == "_message_echo_public"))
	    return ::wAction(mc, data, vars
		 + ([ "_action" : T("_TEXT_action_says", 0) ]),
			 source, "", vars["_nick"]);
#if 1 //def USE_THE_NICK
# ifdef OLD_LOCAL_NICK_PLAIN_TEXTDB_HACK //{{{
	else if (vars["_nick_local"]) {			// less work
		if (mc == "_message_echo_public_action"
			&& (t = vars["_INTERNAL_nick_plain"])) {
		    vars["_nick"] = t;
//		    template = T(mc, "");
		} else {
#ifdef PRO_PATH
		    sTextPath(0, v("language"), "ircgate");
#else
		    sTextPath(0, v("language"), "plain");
#endif
//		    template = T(mc, 0);
		    sTextPath(0, v("language"), v("scheme"));
		}
	}
# else //}}}
	else if (vars["_nick_local"] &&
		 vars["_nick_local"] == vars["_nick"])
	    vars["_nick"] = vars["_INTERNAL_nick_plain"] || vars["_nick_verbatim"];
# endif
#endif // USE_THE_NICK

//	if (vars["_nick_local"]) mc += "_masquerade";	// more work
	di = printStyle(mc); // display infos
	if ((mc == "_message_public"
		|| mc == "_message_echo_public")
		&& !vars["_action"]
		&& v("visiblespeakaction") == "on")
	    return ::wAction(mc, data, vars
			     + ([ "_action" :
				T("_TEXT_action_says", 0) ]),
			     source, "", vars["_nick"]);
	if (!prefix && di["_prefix"]) prefix = di["_prefix"];
	vars["_prefix"] = prefix || "";
 
	D2(
	   if (vars["_INTERNAL_nick_me"] && source)
		D(S("COLLISION: msg arrived in irc:w had _INTERNAL_nick_me with "
		    "value: %O\n", vars["_INTERNAL_nick_me"]));
	  )
#if !defined(PRO_PATH) && defined(ALIASES)
	// heldensaga thinks nickspaces are perfect only when you can
	// give away your own nickname. i think that is unnecessary geek pride.
	// so be it!
	vars["_INTERNAL_nick_me"] = aliases[MYLOWERNICK]
				? SERVER_UNIFORM +"~"+ MYNICK
				: MYNICK;
#else
	vars["_INTERNAL_nick_me"] = MYNICK;
#endif
	if (vars["_nick"] && places[source] && vars["_time_place"]) {
		P3(("%O glaubt dem _relay von %O jetz mal, gell?\n",
		    ME, source))
		if (stringp(source) && !vars["_source_relay"]
			&& abbrev("psyc://", source)) {
		    source = source[7..index(source[7..], '/')] + "~"
			    + (vars["_INTERNAL_nick_plain"] || vars["_nick"]);
		} else
		    source = vars["_source_relay"];
	}
	if (objectp(source) || source == 0 // TODO:: look out for mysterious
					   // source == 0 msgs.
					   // those might be simple w()s... hm.
#ifndef UNSAFE_LASTLOG
		|| abbrev(SERVER_UNIFORM +"~", source)
#endif
		) {
#ifdef GHOST //{{{
	    // in S2S mode we are not supposed to deliver nick!user@host
	    // thus we use plain nicks or plain uniforms
	    vars["_INTERNAL_source_IRC"] = vars["_INTERNAL_nick_plain"] || vars["_nick"];
#else //}}}
# if 0	// OLD // according to elmex "should never happen" happened...
	    if (vars["_nick"]) {
		vars["_INTERNAL_source_IRC"] = 
			(vars["_INTERNAL_nick_plain"] || vars["_nick"])
			+ "!"+ (vars["_nick_long"] || vars["_INTERNAL_nick_plain"]
				|| vars["_nick"])
			+"@" SERVER_HOST;
	    } else // should never happen
		vars["_INTERNAL_source_IRC"] = to_string(source);
# else // EXPERIMENTAL
	    nick2 = vars["_INTERNAL_nick_plain"] || vars["_nick"];
	    vars["_INTERNAL_source_IRC"] = nick2 ? nick2
		  +"!"+ (vars["_nick_long"] || vars["_INTERNAL_nick_plain"]
					    || vars["_nick"]) +"@" SERVER_HOST
		    : to_string(source); // should never happen
# endif
	} else if (abbrev("_echo_place_enter", mc)) {
	    vars["_INTERNAL_source_IRC"] = MYNICK + "!" + MYNICK + "@" SERVER_HOST;
#endif
	} else {
#ifdef GHOST //{{{
	    // in S2S mode we are not supposed to deliver nick!user@host
	    // thus we use plain nicks or plain uniforms
	    vars["_INTERNAL_source_IRC"] = source;
#else //}}}
	    u = parse_uniform(source);
	    unless (u) {
		// this happens when a user@host notation gets here..
		// we could handle that, but how did it happen?
		P0(("%O got a w() from %O which is neither object nor uniform!?\n%O %O\n", ME, source, data, vars))
# if DEBUG > 1
		raise_error("hey sammy, i got a jid on irc\n");
# endif
		return;
	    }
# ifdef ALIASES
	    if (raliases[source]) {
		nick2 = raliases[source];
		vars["_INTERNAL_source_IRC"] = nick2 +"!"+
		    u[UNick]? u[UNick] +"@"+ u[UHost]
			    : (vars["_nick_long"]
				 || vars["_INTERNAL_nick_plain"]
				 || vars["_nick"])
		      +"@alias.undefined";
	    }

	    unless (nick2) {
# endif
		// should all of this go into our own copy of uni2nick() ?
		// or should we keep protocol source hacks separate from
		// people nicks in message templates?
		switch (u[UScheme]) {
    case "psyc":
		    if (u[UUser] || (u[UResource] && strlen(u[UResource])
				     && u[UResource][0] == '~')) {
			string tmp = u[UNick];
			vars["_INTERNAL_source_IRC"] = u[UScheme] + "://"
			    + u[UHostPort] +"/~"+ tmp +"!"+ tmp +"@"
			    + u[UHostPort];
			P4(("w:psyc _INTERNAL_source_IRC %O\n", vars["_INTERNAL_source_IRC"]))
		    } else {
			vars["_INTERNAL_source_IRC"] = uniform2irc(source)
                                +"!*@"+ (u[UHostPort] || "host.undefined");
                            // '*' nicer than 'undefined' and never a nick
			// this happens a lot more often then we thought  :(
			// like for _request_version from server_unl
			D1( unless(u[UHostPort]) PP(("irc/user %O source %O results in %O for %O\n", ME, source, vars["_INTERNAL_source_IRC"], mc)); )
		    }
		    break;
    case "jabber":
    case "xmpp":
		    if (vars["_location"] && v("fulljid") == "on") {
			// TODO: the resource may contain characters indigestible 
			// to irc, e.g. whitespace

			/*
		    if (vars["_INTERNAL_identification"] 
			    && v("fulljid") != "on") {
			    */
			P3(("changing source from %O to %O\n", source, vars["_location"]))
			source = vars["_location"];
		    }
		    vars["_INTERNAL_source_IRC"] = uniform2irc(source)
                                            + "!" + u[UUserAtHost];
#ifdef JABBER_PATH // MUC support..
		    // unfortunately, net/entity has rendered _nick unusable
		    if (vars["_nick_place"] && vars["_INTERNAL_source_resource"] && member(vars["_INTERNAL_source_resource"], ' ') != -1){
			// xmpp: is here in cleartext.. TODO maybe?
			vars["_INTERNAL_source_IRC"] = "xmpp:"+
                                    uniform2irc(u[UUserAtHost]);
			t = replace(vars["_INTERNAL_source_resource"], " ", "%20");
			vars["_INTERNAL_source_IRC"] += "/" + t + "!" + u[UUserAtHost];
			PT(("irc/user: XMPP source hack %O for %O\n", vars["_INTERNAL_source_IRC"], ME))
		    }
#endif
		    break;
    default:
		    vars["_INTERNAL_source_IRC"] = source + "!*@*";
		}
# ifdef ALIASES
	    }
# endif
#endif
	}
# ifndef NO_CTCP_PRESENCE
	if ((t = vars["_degree_availability"]) && strlen(mc) > 16
					 && mc[7..15] == "_presence") {
	    if (v("ctcppresence")) {
		int l;
		string output;

		output =":"+ vars["_INTERNAL_source_IRC"] +" NOTICE "+ MYNICK +" :";
		l = strlen(output);
		output += "#PRESENCE "+ t +" "+ vars["_degree_mood"]
			       +" :"+ vars["_description_presence"] +"#\n";
		output[l] = 0x01;
		output[<2] = 0x01;
		emit(output);
		return 1;
#  ifdef IRC_FRIENDCHANNEL //{{{
	    } else {
#   ifdef IRC_FRIENDCHANNEL_HEREAWAY
		string old = vars["_degree_availability_old"];

		if (old <= AVAILABILITY_VACATION) {
		    if (t >= AVAILABILITY_NEARBY)
			emit(":"+ vars["_INTERNAL_source_IRC"] +" JOIN :&HERE\n");
		    else if (t >= AVAILABILITY_AWAY)
			emit(":"+ vars["_INTERNAL_source_IRC"] +" JOIN :&AWAY\n");
		} else if (old < AVAILABILITY_NEARBY) {
		    if (t >= AVAILABILITY_NEARBY) {
			emit(":"+ vars["_INTERNAL_source_IRC"] +" PART :&AWAY\n");
			emit(":"+ vars["_INTERNAL_source_IRC"] +" JOIN :&HERE\n");
		    } else if (t < AVAILABILITY_AWAY)
			emit(":"+ vars["_INTERNAL_source_IRC"] +" PART :&AWAY\n");
		} else {
		    if (t < AVAILABILITY_NEARBY)
			emit(":"+ vars["_INTERNAL_source_IRC"] +" PART :&HERE\n");
		    if (t >= AVAILABILITY_AWAY)
			emit(":"+ vars["_INTERNAL_source_IRC"] +" JOIN :&AWAY\n");
		}
#   else
// ... MODE #whateverTesting +v xni3 
		if (t >= AVAILABILITY_NEARBY)
		   emit(":"+ SERVER_HOST +" MODE & +o-v "+ vars["_nick"] +"\n");
		else if (t >= AVAILABILITY_VACATION)
		   emit(":"+ SERVER_HOST +" MODE & +v-o "+ vars["_nick"] +"\n");
		else
		   emit(":"+ SERVER_HOST +" MODE & -v-o "+ vars["_nick"] +"\n");
#   endif
#  endif //}}}
	    }
	} else
# endif
	P2(("irc/user:w(%O,%O,..,%O)\n", mc, data, source))
	t = 0;
	PSYC_TRY(mc) {
#ifdef IRC_FRIENDCHANNEL //{{{
	case "_list_friends_offline":   // _tab
		t = " "; // fall thru
	case "_list_friends_away":  // _tab
		unless (t) t = " +"; // fall thru
	case "_list_friends_present":   // _tab
		unless (t) t = " @"; // fall thru
		if (pointerp(vars["_list_friends_nicknames"])) {    // _tab
# ifdef IRC_FRIENDCHANNEL_HEREAWAY
			u = implode(vars["_list_friends_nicknames"], " ");
# else
			u = implode(vars["_list_friends_nicknames"], t);
# endif
	//		u = strlen(u) ? (MYNICK+" "+u) : MYNICK;
# ifdef _flag_encode_uniforms_IRC
			u = uniform2irc(u);
# endif
			// just a trick to do the #ifdef in textdb, should we
			// opt for IRC_FRIENDCHANNEL forever, we can remove
			// _channel again, from both textdb and here
			render(
# ifdef IRC_FRIENDCHANNEL_HEREAWAY
			       mc+"_channel"
# else
			       "_list_friends_channel"  // _tab
# endif
					, 0, ([ "_friends": u,
					 "_INTERNAL_nick_me" : MYNICK ]) );
		} else {
		    P1(("%O irc/user:w() got %O without friends list in %O\n",
			ME, mc, vars))
		}
# ifdef IRC_FRIENDCHANNEL_HEREAWAY
		reply(RPL_ENDOFNAMES, (mc == "_list_friends_away"
			? "&AWAY": "&HERE") +" :End of Buddylist.");
# else
		// bad!
		if (mc == "_list_friends_away")
		    reply(RPL_ENDOFNAMES, "& :End of Buddylist.");
# endif
		return 1;
#endif //}}}
	case "_status_place_topic":
		// traditional IRC topic message without author
		render(mc +"_only", 0, vars);
		// extra semi-official '333' code containing author and time
		render(mc +"_author", 0, vars);
		return 1;
	case "_status_place_members_automatic":
		mc = "_status_place_members"; // fall thru
	case "_status_place_members":
		unless (vars["_members"])
#ifndef ENTER_MEMBERS
		    return _status_place_members(source, mc, data, vars);
#else
		    return 1;
#endif
		// recursive call from _status_place_members()..
		// yeah yeah don't ask
		//PT(("irc:w(%O,%O,%O)\n", mc,data,vars))
		break;
	case "_request_attention":
	case "_request_attention_wake":
		vars["_beep"] = " ";
		break;
	case "_echo_place_enter":
	case "_echo_place":
	case "_echo":
	case "_list_user_description_end":
	case "_list_user_description":
	case "_list_user":
	case "_list":
	case "_message_public":
	case "_message":
	case "_notice_session_end":
	case "_notice_session":
	case "_notice_place_leave":
	case "_notice_place":
	case "_notice_logon_last":
	case "_notice_logon":
	case "_notice_login":
	case "_notice":
	case "_request_version":
	case "_request":
	case "_status_log_none":
	case "_status_log":
	case "_status_place_members_end_verbose":
	case "_status_place_members_end":
	case "_status_place_members_each":
	case "_status_place":
	case "_status_person_present_action":
	case "_status_person_present":
	case "_status_person":
	case "_status":
	case "_warning_place_duty_owner":
	case "_warning_place_duty":
	case "_warning_place":
	case "_warning_server_shutdown_temporary":
	case "_warning_server_shutdown":
	case "_warning_server":
	case "_warning_usage_set_charset":
	case "_warning_usage_set":
	case "_warning_usage":
	case "_warning":
//		PT(("%O wegoptimiert in %O\n", mc, ME))
		break; // optimization, avoid slicing for nothing
	PSYC_SLICE_AND_REPEAT
        }
	render(mc, data, vars, source);
	PSYC_TRY(mc) {
#ifndef GHOST
    // cannot print it in irc::quit() as the last thing outputted w/ SANE_QUIT
	case "_notice_session_end":
		emit("ERROR :Closing Link: "+MYNICK+" (Don't worry, this is the "
		     "normal erratic way of IRC to close a connection)\n");
		break;
#endif
#ifdef ALIASES
	case "_echo_alias_added":
		// man könnte das auch einfach so machen.. oder als
		// sprintf().. und nur einen emit absetzen statt zwei.
		// psyctext ist an dieser stelle in der tat ohne vorteil
		// aber auch kein performancefaktor.. also egal
		emit(":"+ vars["_alias"] +" NICK "+ 
		     SERVER_UNIFORM +"~"+ vars["_alias"] + "\n");
		emit(psyctext(":[_nick_old] NICK [_nick_new]", 
		  ([ "_nick_old" : aliases[lower_case(vars["_address"])]
				  ? SERVER_UNIFORM +"~"+
				    vars["_address"]
				  : uniform2irc(vars["_address"]),
		     "_nick_new" : vars["_alias"] ])) + "\n");
		break;
	case "_echo_alias_removed":
		emit(psyctext(":[_nick_old] NICK [_nick_new]", 
		  ([ "_nick_old" : vars["_alias"],
		     "_nick_new" : aliases[lower_case(vars["_address"])]
				  ? SERVER_UNIFORM +"~"+ vars["_address"]
				  : uniform2irc(vars["_address"])
                   ])) + "\n");
		emit(psyctext(":[_nick_old] NICK [_nick_new]", 
		  ([ "_nick_old" : SERVER_UNIFORM +"~"+ vars["_alias"],
		     "_nick_new" : vars["_alias"] ])) + "\n");
		break;
#endif
#ifdef ENTER_MEMBERS //{{{
// now obsolete since net/user does the rendering of _list_members
// and converts it to _status_members* w()
	case "_echo_place_enter":
		namreply(vars);
		break;
#endif //}}}
	case "_message_public":
	case "_message":
	case "_notice_place_leave":
	case "_notice_place_enter":
	case "_notice_place":
	case "_notice_logon_last":
	case "_notice_logon":
	case "_notice_login":
	case "_notice":
	case "_request_version":
	case "_request":
	case "_status_log_none":
	case "_status_log":
	case "_status_person_present_action":
	case "_status_person_present":
	case "_status_person":
	case "_status":
	case "_warning_server_shutdown_temporary":
	case "_warning_server_shutdown":
	case "_warning_server":
	case "_warning_usage_set_charset":
	case "_warning_usage_set":
	case "_warning_usage":
	case "_warning":
		break; // optimization, avoid slicing for nothing
	PSYC_SLICE_AND_REPEAT
        }
	return 1;
}

// somehow we lost visiblespeakaction == no handling... duh.
// but handling it here seems to be more adequate than doing it in msg()
wAction(mc, data, vars, source, variant, nick) {
    // maybe this does the same job after all
    // with that variant == "_ask" check, it does.
    if (data && data != "" &&
#ifdef VISIBLESPEAKACTION_BY_DEFAULT
       	v("visiblespeakaction") == "off"
#else
       	!v("visiblespeakaction")
#endif
    ) {
	if (variant == "_ask") {
	    return w(mc, data, vars, source);
	}
	return 0;
    }
    return ::wAction(mc, data, vars, source, variant, nick);
}

#ifndef _limit_amount_history_place_default
# define _limit_amount_history_place_default 5
#endif

// irc has it's own autojoin, which is a little different from others
autojoin() {
#if !defined(_flag_disable_place_enter_automatic) && !defined(GHOST)		// too tricky for now
    mixed t, t2;
    string s;

    if (isService) return -1;
    // subscriptions are stored in lowercase, warum auch immer
    if (sizeof(v("subscriptions"))) 
	foreach (s in v("subscriptions")) {
	    // call_out(#'placeRequest, delay++, s, "_request_enter", //_automatic_subscription
	    placeRequest(s,
# ifdef SPEC
                         "_request_context_enter"
# else
                         "_request_enter"
# endif
                         , // _automatic_subscription
		     0, 0, ([ "_amount_history" : _limit_amount_history_place_default ]));
    } else {
	unless (v("place"))
	  vSet("place", T("_MISC_defplace", DEFPLACE));
# ifndef _flag_disable_place_default
	// call_out(#'placeRequest, delay++, v("place"), ...
	placeRequest(v("place"),
#  ifdef SPEC
                     "_request_context_enter"
#  else
                     "_request_enter"
#  endif
                     "_login", 0, 0, 
		     ([ "_amount_history" : _limit_amount_history_place_default ]));
# endif
    }
# ifdef IRC_FRIENDCHANNEL //{{{
#  ifdef IRC_FRIENDCHANNEL_HEREAWAY
    emit(":"+ MYNICK +" JOIN :&HERE\n");
    emit(":"+ MYNICK +" JOIN :&AWAY\n");
#  else
    emit(":"+ MYNICK +" JOIN :&\n");
#  endif
# endif //}}}
#endif // GHOST || _flag_disable_place_enter_automatic
}

logon() {
	mixed rc;

	decodeInit();
	// the only robot/service we are currently supporting:
	// and we dont like those proxyscanners either
	isService = abbrev("tunix", MYNICK);

	vSet("scheme", "irc");
	vDel("layout");
	// vDel("agent"); -- either you start a ctcp to find it out
	//		     or we prefer to have the old info
	vDel("query");	// server-side query would drive most ircers crazy
#if 0
	// what's wrong with doing this.. here?
	// it's redundant, as it happens again in ::logon
	sTextPath(0, v("language"), v("scheme"));
	// it's necessary for _request_user_amount to work
	// let's see if we can simply postpone that to after ::logon
#endif
	//
	// this helps handle the /set visiblespeakaction setting if this
	// define has changed
#ifdef VISIBLESPEAKACTION_BY_DEFAULT
	if (v("visiblespeakaction") == "mixed") vDel("visiblespeakaction");
#else
	if (v("visiblespeakaction") == "off") vDel("visiblespeakaction");
#endif

#ifndef GHOST
	set_buffer_size(32768); // enlarge buffer size
	set_prompt("");
	next_input_to(#'parse);

# ifdef RPL_ISUPPORT
	// see also http://www.irc.org/tech_docs/005.html
#  ifndef MAX_UNIFORM_LEN
#   define MAX_UNIFORM_LEN 256	// anything goes. too dangerous to put nothing.
#  endif
	//
	// PSYC for the fact that we natively speak a certain PSYC protocol
	// version. UNIFORMS for our extension that nicknames may be
	// full-fledged uniforms starting with a scheme. in fact we could list
	// all schemes we have on this server but even then, schemes are
	// dynamically extendable. then we have a list of psyc commands which
	// may be seen as seperate modules and could in a fantastic world be
	// considered as custom irc extensions. finally the regular 005
	// settings as defined out there in the messed up world. to add all
	// these "custom" setting names is primarily a suggestion from Nei,
	// but i guess it is indeed appropriate to make it clear how very much
	// different we are from a regular irc server.
	//
	reply(RPL_ISUPPORT, "PSYC=.99 ALIAS AVAILABILITY FRIEND HISTORY MOOD SHOUT SSET STATUS SUBSCRIBE THREAD TRUST PREFIX= CHANTYPES=# CHANMODES= NICKLEN="+ (string)MAX_UNIFORM_LEN +" CHANNELLEN="+ (string)MAX_UNIFORM_LEN +" CASEMAPPING=ascii TOPICLEN=4404 KICKLEN=4404 AWAYLEN=4404 MAXTARGETS=1 CHARSET="+ (v("charset")||SYSTEM_CHARSET) +" NETWORK=PSYC CTCP=PRESENCE,TS UNIFORMS=psyc,xmpp :are supported by this server");
	//
	// MAXCHANNELS vs CHANLIMIT - we currently only have a limit on subs
	// STD? what the hell is STD?
	// RFC2812? should we say that?
	// FNC: forced nick changes. we may want this one day.
	// TOPICLEN=4404 KICKLEN=4404 AWAYLEN=4404 or just ignore
	// MAXNICKLEN?
	// neue befehle: IGNORE vs SILENCE? SHOW? MASQUERADE?
	//
	// PSYC as network name is not a #define, since any psyc server is
	// a gateway to the complete PSYC. introducing network names here is
	// misleading and not useful thing to do. that's why it is statically
	// PSYC, nothing more or less.
	//
	// nei suggests: ALIAS SUBSCRIBE FRIEND SET SSET SILENCE CHANTYPES=# PREFIX= CHANMODES= CMDCHAR=+ ACTIONCHAR=: EINOTIFY=notify LANGUAGE=en CHARSET_PAYLOAD=utf-8 CHARSET=utf-8 NETWORK=psyc UNIFORM_NICK UNIFORM_CHAN
	// SILENCE: ach und ich weiss nicht ob /quote silence bzw /silence den psyced befehl silence aufruft, aber imo sollte er das aus verwirrungs-vermeidungs-gruenden nicht tun. silence im irc ist serverseitiges ignore.

# endif
# ifndef BETA
	lusers();
# endif
	motd();
	rc = ::logon();
	// the following things happen after logon, because the textdb isn't
	// available earlier. if this order of things is not acceptable, then
	// we have to run sTextPath twice (see above)
# ifdef BETA
#  ifndef _flag_disable_query_server
	sendmsg("/", "_request_user_amount", 0, ([]));
	// reply.h says RPL_LUSERME is mandatory.. huh.. FIXME?
	// #255 [_INTERNAL_nick_me] :I have 4404 clients and 4404 servers
#  endif
# endif
# ifndef _flag_disable_request_version_IRC
	// since we cannot relay tagged version requests to the client
	// easily, we request the version number once at starting time.
	// any other protocol finds it completely normal to exchange
	// version strings, only IRC makes it terribly complicated and
	// even political. oh of course, that's because on irc the server
	// admin isn't necessarily a person of your trusting.
	w("_request_version", 0, 0, SERVER_UNIFORM);
# endif
#endif
	return rc;
}

ircMsg(from, c, args, text, all) {
	mixed n, t, t2;

//	if (isService)
//	    log_file("IRCSERV", "[%s %s] %s\n", query_ip_number(), MYNICK, all);
	switch(c) {
#if 0
		// wird von ircII falsch verarbeitet.. also lieber
		// help ausgeben.
case "nick":
		reply(ERR_NICKNAMEINUSE, args + " :Nickchanges are not supported on "
		      "this server. They never will be.");
		//emit(":"+SERVER_HOST+" 432 " +v("name")+" # :Nick changes are not permitted on this server yet.\r\n");
		return 1;
#endif
case "privmsg":
		privmsg(args, text, 1);
		return 1;
case "msg":		// very old-school irc syntax
		privmsg(0, args || text, 1);
		return 1;
case "notice":
case "n":
		privmsg(args, text, 0);
		return 1;
case "who":
		// denkbare argumentsyntax:
		// 	WHO #fasel
		// 	WHO nickname
		// 	WHO *.fi
		// manche irc-clients (GUI) senden who #raum,
		// oder sonstigen WHO kram automatisiert..
		// das filtern wir dann mal, weil wir noch kein
		// korrektes WHOREPLY zurückgeben.
		// sobald wir das haben kann man's freigeben.....
		if (args && strlen(args)>1) {
			if (channel2place(args)) {
				whores(args, 1);
				return 1;
			}
		}
		// in any case: end of who
		// it is okay to ignore perverted requests like *.fi
		// use google for that
		w("_list_user_description_end");
		return 1;
case "names":
		// denkbare argumentsyntax NAMES #bla,#fase
		if (args && strlen(args)>1) {
			map(explode(args, ","), #'whores);
			return 1;
		}
		people();
		w("_list_places_end");	// end of names
		return;
case "ison":
		// ha! ich hab ne neue idee:
		// so lange damit nerven bis sie's abschalten
		// aber nur wenn sie sich gereggt haben
		// <fippo>: schlechte idee, du nervst damit unsere huebschen
		// 		userinnen
#ifdef IRC_ANNOYANCE
		if (!IS_NEWBIE && !isGenervt++)
		    reply(ERR_UNKNOWNCOMMAND,
		      //c+" :PSYC does not support historic poll-based notify"
		      c+" :Please clean out your notify list for this server. It is of no use on PSYC.");
#else
		// faking everyone to be on.
		// some clients need that to be gui-usable at all
		reply(RPL_ISON, args);
		// would be better to check our state for presence
#endif
		return 1;
case "mode":
#ifndef GHOST
		// "don't mode me" is a software design rule - you should
		// not send people into cryptic non-obvious modes.
//		reply(ERR_UNKNOWNMODE, args+" :Don't mode me (No support for IRC legacy, use PSYC features instead)");
		// die clients moden soviel sie lust haben.. und machen stress
		// wenn man es ignoriert.. oder?
		unless (stringp(args)) t = "";
		else unless (sscanf(args, "%s %s", t, t2)) t = args;
		if (strlen(t) && channel2place(t)) {
		    unless (t2) {
			reply(RPL_CHANNELMODEIS, t+" +");
		    } else for (int i = 0; i < strlen(t2); i++) {
			switch (t2[i]) {
			case '+':
			case '-':
			    break;
			case 'b':
			    reply(RPL_ENDOFBANLIST,
				  t + " :No traditional banlist on " SERVER_VERSION);
				  // " :End of new KONTEXT-AWAY banlist"
			    break;
			case ' ':
			    return; // skip argument list
			default:
#ifdef _flag_strict_IRC_mode
			    reply(RPL_CHANNELMODEUNKNOWN, t2[i..i] + " :"
				  "is unknown mode char to me.");
#endif
			}
		    }
# ifdef IRC_FRIENDCHANNEL
		} else if (t[0] == '&') {
		    reply(RPL_CHANNELMODEIS, t+" +m");
# endif
		} else
		    //emit(MYNICK+" MODE "+MYNICK+" :+\n");
		    reply(RPL_UMODEIS, "+");
#endif
		return 1;
case "topic":
		unless (args && strlen(args)) return;
		P4(("IRC topic %O %O\n", args, text))
		unless (t = channel2place(args)) return;
		t = find_place(t) || t;
		// The topic for channel <channel> is returned if there is
		// no <topic> given. If the <topic> parameter is an empty
		// string, the topic for that channel will be removed.
		unless (text)	//wrong://  && strlen(text)
		    sendmsg(t, "_query_topic", 0, ([ "_nick" : MYNICK ]));
		else
		    sendmsg(t, "_request_set_topic", 0, ([ 
			"_topic" : text,
			 "_nick" : MYNICK
		    ]));
		return 1;
case "user":
		if (sscanf(args, "%s %s %s", n, t, t2)) {
			vSet("publiclogin", n);
			vSet("publichost", t);
		}            
		// publicname is sent as a 'nicer' full name to jabber
		// users. if it contains a typical IRCNAME they will freak
		// out, especially unaccostumed newbies on iChat, G-Talk etc.
		// iChat even shows 'publicname' for every line of chat!
		// so let's make a specific field for IRCNAME instead.
		// chat cultures just don't want to get merged sometimes....
		vSet("mottotext", text);
		return 1;
case "invite":
		if (sscanf(args, "%s #%s", n, t) && strlen(t)) {
			t2 = find_place(t);
			if (!t2 || !places[t2]) {
				PT(("irc: invite %s into %s = %O.\n", n, t, t2))
				w("_error_necessary_membership",
					0, ([ "_nick_place": t ]));
				return 1;
			}
			vSet("place", t);
			place = t2;
		} else {
#if 0
			// atypical: allow for invite using current place.
			n = args;
			// runs the risk of you inviting the person to the
			// wrong channel. too risky. let's go traditional:
#else
			reply(ERR_NEEDMOREPARAMS,
			      "INVITE :Not enough parameters");
			return 1;
#endif
		}
		if (invite(n)) reply(RPL_INVITING, args);
		// else: ERR_USERONCHANNEL or ERR_CHANOPRIVSNEEDED otherwise?
		return 1;
case "join":
case "part":
		// bug in some clients like.. uh.. ircg
		unless (args) args = text;

		// make sure there _is_ something. this ignores requests to
		// 'JOIN #' but also 'JOIN 0' which i don't see why it
		// should be a useful thing to implement. should anyone
		// ask we'll add a +panic command to usercmd.i and have it
		// called from here... ;)  we should in any case not
		// support the completely absurd /join #0,0 syntax
		unless (args && strlen(args)>1) return 1;

		// PART: the text var contains the parting reason message.
		// we happily ignore that as it is bad protocol design.   ;)

		// JOIN: we should only be using the first arg as the second arg
		// would be the key of a +k channel (since we don't have
		// them, we never see that). formally it can even be a
		// comma list of keys matching the comma list of chans.
		// that's seriously awful and a good reason not to support
		// MODE +k  ;)

		// request() calls teleport() which in the case of places we
		// have already joined quietly returns and does nothing.
		// this is the same behaviour that ircd(8) provides, even if
		// it is not clearly specified in RFC2812. anyway, this should
		// be the clean way to solve all our issues with autojoining
		// and reconnecting irc clients
		foreach(t : explode(args, ","))
		    request(ME, c == "part"? "_leave": "_enter_join",
		       ([ "_group": channel2place(t) || t,
			  "_flag": "_quiet",
		]));
		return 1;
case "away":
		// irc-semantik ist anders als in psyc
		unless (text) text = args; // just in case a user forgot the :
		if (!stringp(text) || !strlen(text)) text = 0;
		return parsecmd(text ? "ircgone "+text : "irchere");
case "identify":
// some script providing us with nickserv credentials
// we don't support such kinky stuff
		return 1;
case "cs":
case "chanserv":
//		instead of emulating "mode" one could implement chanserv
//		compatible commands in net/place/standard.c -- hmmm
case "statserv":
case "bs":
case "botserv":
case "hs":
case "helpserv":
case "os":
case "operserv":
case "ms":
case "memoserv":
case "nws":
case "newsserv":
//		these don't really work, but we could make them..
case "ns":
case "nickserv":
case "psyc":
		return parsecmd(text || args);
#if 1
case "dialog":
case "m":
case "more":
case "message":
case "r":
case "reply":
case "q":
case "query":
case "t":
case "talk":
case "tell":
case "g":
case "gr":
case "greet":
		// diese befehle sind für irc nicht sinnvoll, egal wie.
		// sollten aber hübscher abfangbar sein, one happy day
		reply(ERR_UNKNOWNCOMMAND, c+" :Command not useful here");
		// hmm.. per +r und +q kann man den mist immernoch
		// einschalten.. dies ist also genau genommen der falsche
		// ort das abzuhandeln.. TODO
		return 1;
#endif
default:
		if (::ircMsg(from, c, args, text, all)) return 1;
			// mud-compatible debug tool
			// just type 'hello to say hello
		D1(if (all[0] == '\'') return ::input(all[1..]);)
			// fall
	}

		// most commands can be handled by usercmd:cmd()
		// that's not ultimately efficient as of now,
		// but better than code duplication
	if (text && args) return parsecmd(c+" "+args+" "+text);
	if (text || args) return parsecmd(c+" "+(text || args));
	return parsecmd(c);
		// but the argument passing needs some tweaking..
}

static whores(chan, flag) {
	chan = channel2place(chan);
	if (chan) chan = find_place(chan);
	unless (chan) return;
	P4(("irc:whores(%O, %O)\n", chan, flag? "W": "N"))
	sendmsg(chan, "_request_members", 0, ([ "_nick" : MYNICK,
					  "_tag": flag? "W": "N" ]));
	return 1;
}

static privmsg(args, text, req) {
	string person,room;
	mixed t;

	unless (stringp(text) && strlen(text)) return;

#ifdef GAMMA
	// fippoism typing indicator.. but shouldn't it *do* something
	// after detecting this CTCP-like "typing" flag hack?
	if (strlen(text) > 1 && text[<1] == 0x0f && text[<2] == 0x0f) {
	    text = text[..<2];
	}
#endif
	if (index(args, ',') > 0) {
		w("_failure_unsupported_targets_multiple",
	"We do not allow sending to several recipients at once. Why did your client ignore our MAXTARGETS=1 directive?");
		return 0;
	}
	if (room = channel2place(args)) {
	    if (!place || !v("place") || stricmp(room, v("place"))) {
		if (t = find_place(room)) {
		    place = t;
		    // this makes positive use of the psyc notion
		    // of a "current" room even if irc doesn't support
		    // that protocolwise..
		    vSet("place", room);
		} else { // should the caller print this on privmsg() == 0?
		    w("_error_illegal_name_place", "Room name contains illegal "
		      "characters."); // uh. might be a lie
		    return 0;
		}
	    }
	} else {
	    if (lower_case(args) == "nickserv") return parsecmd(text);
	    person = args;
	}

	unless (text = decode(text, person, req)) return;

//	unless (args) speak(text);
//	else if (room = channel2place(args)) speak(text, 0, room);
//	else tell(args, text);
	if (!req && person)	// NOTICE <nick>
	     tell(person, text, 0, 0, "_message_private_annotate");
	// NOTICE <channel> treated as normal room chat
	else return input(text, person);
}

version(text, person, req) {
	mixed target;
#ifdef ALIASES
	mixed t = aliases[lower_case(person)] || person;
#else
# define t	person
#endif
	target = find_person(t) || t;
	P4(("user:version() in %O from %O\n", ME, target))
	if (text &&! req &&! v("agent")) vSet("agent", text);
	return ::version(text, target, req, MYNICK);
#ifdef t
# undef t
#endif
}

ctcp(code, text, person, req) {
	mixed target;
#ifdef ALIASES
	mixed t = aliases[lower_case(person)] || person;
#else
# define t	person
#endif

	target = find_person(t) || t;
	return ::ctcp(code, text, target, req, MYNICK);
#ifdef t
# undef t
#endif
}

