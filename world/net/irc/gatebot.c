// $Id: gatebot.c,v 1.151 2008/09/12 15:37:39 lynx Exp $ // vim:syntax=lpc
//
// the PSYC-IRC gateway robot.
// based on the ircbot.c in Nemesis that i wrote in 1992.	-lynX
// documentation: http://www.psyc.eu/ircgate
// it can still use plenty of improvements and new features, just go ahead!

// gatebot hands out some room status to the channel:
// -|symlynX/#23c3- PSYC Identification of freeNode: psyc://psyced.org/@freenode
// -|symlynX/#23c3- You enter freeNode.
// "you" is obviously wrong here. also why |symlynX rather than PSYCgate?

// TODO? fippo doesn't like #ifdefs, so he wrote the next generation gateway.c
// that allows for entering channels on the ircnet in a way not compatible
// to this file. How can we merge this back? Can we handle things better?
//
// #ifdef SERVER is a bit ugly, yet having several files would result
// in almost the same code in several files, just slightly different. We can
// try to simplify some ifdefs via textdb. Then again, I don't think #ifdef
// is really that bad. The biggest problem of ifdef is not the preprocessing
// as such, it's just how ugly preprocessor statements look. Maybe a macro
// would help? As in GATEWAY( ...code... ) vs GATEBOT( ...code... ). Hm!
//
// Several files would look like this: Put the majority of gatebot.c in a
// gateshare.i or even gateshare.c, then have gatebot.c and gateway.c
// include or inherit it and adapt. The stupid thing is that functions like
// disc() or reboot() contain almost the same code for both variants and
// should better stay the way they are - together. And once gateway.c has
// learned how to enter channels, wouldn't you want gatebot.c to be able
// to do that as well? So we have to code it in a generic way, anyway.
// Sure, it's harder to do abstract coding, but the benefit can be double.
// Not only you solve two things at once, you also only have one place to
// deal with when something needs to be fixed. Having two implementations
// of the same thing wouldn't be what we want, anyway.

#include "gatebot.h"
#include <status.h>
#include <services.h>
#include <text.h>
#include <url.h>
#include "error.h"      // gets numeric codes
#include "reply.h"      // gets numeric codes

// ctcp support etc.. yes even dcc works across the gateway if the psyc
// user runs an irc client
inherit IRC_PATH "decode";

// message queueing and automatic reconnect mgmt
inherit NET_PATH "circuit";

#ifndef _uniform_node
# define _uniform_node query_server_unl()
#endif

#ifdef RELAY
# define IRCER_UNIFORM(NICK)	(_uniform_node +"~"+ NICK)
#else
		    // will upgrade to irc: syntax..
# define IRCER_UNIFORM(NICK)	(MYLOWERNICK +":"+ NICK)
#endif

// dieses objekt ist ein blueprint für einen raum -
// d.h. man kann ircgateways betreten und auf diese weise
// nicht zuordnungsfähige protokollreplies von irc abholen
// (irc bietet kein request/response-tagging, und andere
//  lösungsansätze sind gepfriemel)
#ifdef IRCGATE_HISTORY
inherit NET_PATH "place/storic";
#else
// we normally don't want to irritate our fellow ircers by
// keeping logs of their conversations. it's atypical to
// irc traditions. unfortunately we can take this decision
// only systemwide as of now.
inherit NET_PATH "place/public";
#endif

#ifndef SERVER
# undef IRCGATE_REGISTERED_ONLY
// the gateway command level also used by other gateways like ICQ
inherit NET_PATH "gateway/generic";
#else
// allow federated communications for registered nicks only.
// this could've been implemented in the ircd aswell...

# define psycer rcpt
# ifdef ADVERTISE_PSYCERS
volatile mapping gated;
# endif
# ifdef IRCGATE_REGISTERED_ONLY
volatile mapping regged;
# endif

volatile string joe, joe_nick;
volatile mixed joe_unl;
#endif

volatile int notice;
volatile string names, namesfrom, namesto;

qChatChannel () { }

#ifdef SERVER
ulogin(ni) {
#ifndef IRCGATE_NO_LOGIN
	object u = summon_person(ni, IRC_PATH "ghost");
	if (u) u -> logon();
#endif
#ifdef IRCGATE_REGISTERED_ONLY
	regged[lower_case(ni)]++;
#endif
}
ulogout(ni) {
#ifndef IRCGATE_NO_LOGIN
	mixed u = find_person(ni);
	if (u) return u->quit();
#endif
#ifdef IRCGATE_REGISTERED_ONLY
	ni = lower_case(ni);
	if (regged[ni]) {
		regged[ni] = 0;
	}
//	PT(("regged: %O\n", regged))
#endif
}

emit(all, source) {
#ifdef ADVERTISE_PSYCERS
	if (source) advertise(source);
#endif
#ifdef _flag_log_sockets_IRC
	log_file("RAW_IRC", "%d %O\t-> %s", time(), ME, all);
#endif
	return ::emit(all);
}

qName() { return joe_nick; }

# if 0
// circumventing library:psyc2irc with its special cases
psyc2irc(mc, source) {
	PT(("psyc2irc wird einfach nicht aufgerufen. arggh!!\n"))
	return ":" IRCGATE_NICK " NOTICE";
}
# endif

# ifdef ADVERTISE_PSYCERS
advertise(source) {
	unless (gated[source]) {
		mixed *u;
		string login;

		// in this case 'gated' doesn't strictly mean this source
		// is already advertised but rather this source has already
		// been considered for advertising and either been advertised
		// or not..
		gated[source] = 1;
		if (!stringp(source) || !strlen(source)) return;
	       	unless (u = parse_uniform(source)) return;
		if (login = u[UUser]) {
#if 1
			// for some mysterious reason sporadically we get
			// UNRs here. maybe we should raise_error to figure
			// out why that happens.
		// since gmail likes to change its UNRs quite often,
		// this results in 'gated' getting slowly polluted.
		// we could either fight this by storing timestamps
		// in gated[] and weeding out old entries occasionally
		// (how do we find out if an entry has actually been
		// advertised and thus needs to be QUITted? parse again?).
		// the other option is to have psyced periodically
		// reconnect the ircnet. sufficient intermediate solution
		// really, since the resync between psyced and a locally
		// running ircd is merely cpu exercise, not traffic.
			// this whole problem should no longer occur, since
			// fippo took a measure in person.c to avoid UNRs
			// arriving here. i wonder why person.c gets them
			// in the first place. shouldn't entity.c deal with
			// them? hmmmm
			if (u[UResource]) {
				monitor_report("_failure_invalid_identity",
				    sprintf("%O advertising UNR %O from %O",
					    ME, source, previous_object()));
				source = u[UScheme] +":"+ u[UUserAtHost];
	    // of course we need to check this variant, too. thx fippo.
				if (gated[source]) return 1;
				gated[source] = 1;
			}
#endif
		} else {
			// maybe this kind of logic should be in parse_uniform?
			// or is it too rarely needed thus inefficient?
			login = u[UResource];
			if (stringp(login) && strlen(login) > 1)
			     login = login[1 ..];
		}
		unless (login && strlen(login)) return;
		// introduce the federation user into the ircnet
		// database. not strictly necessary for plain
		// messaging but surely as soon as the person wants
		// to join an ircnet channel.
		P0(("ADVERTISING %O as %O of %O\n",
		    source, login, u[UHost]))
// hmm PTLINK syntax.. we may have to add some ifdefs here for other flavors
// or maybe we do just like with login() and throw this into the ircgate.c
/* m_nick
**      parv[0] = sender prefix
**      parv[1] = nickname
**      parv[2] = optional hopcount when new user; TS when nick change
**      parv[3] = optional TS
**      parv[4] = optional umode
**      parv[5] = optional username
**      parv[6] = optional hostname
**      parv[7] = optional spoofed hostname
**      parv[8] = optional server
**      parv[9] = optional info
*/
		emit("NICK "+ source +" 1 4404 +i " + login + " " SERVER_HOST
		     " " + u[UHost] + " " IRCGATE_NICK " :I'm psyced!\n");
	}
	// we could update a timestamp in gated[] so every 15m we
	// can clean out old names.
}

//static send(source, text) {
render(mc, data, vars, source) {
//# include <url.h>
//	mixed *u = parse_uniform(psyc_name(source));
//	unless (mappingp(u) && u[UHost]) {
//		P1(("failed\n"))
//		return;
//	}
//	string ircsrc = (u[UScheme] || "xmpp") +";"+
//			(UName(u) +"|"+
//			replace(u[UHost], ".", "_");
	if (source) {
		advertise(source);
	//	text = ":"+ source +" "+ text;
	}
//	PT(("gateOut: "+ text))
//	emit(text);
	return ::render(mc, data, vars, source);
}
# endif
#else
# define send(source, text)	emit(text)
#endif

showStatus(verbosity, al, person, mc, data, vars) {
	// this is for my local users only
	// TODO: this info doesn't get updated on joins and leaves
	// re-request NAMES every time? damn.
	if (names && namesfrom) sendmsg(person, "_status_place_members_IRC",
	     "irc://[_server_IRC]/[_nick_place_IRC] contains: [_members_IRC]",
		    ([  "_members_IRC": names,
		       //"_source_IRC": "irc://"+namesfrom+"/"+namesto
		         "_server_IRC": namesfrom,
		     "_nick_place_IRC": namesto ]));
	return ::showStatus(verbosity, al, person, mc, data, vars);
}

link(ho, po) {
	register_scheme(MYLOWERNICK);
	joe = "nobody";
	// we cannot use "irc" here, because we don't want the textdb
	// to do the irc protocol formatting for us. we need a plain-text
	// formatting of the messages that we can put into NOTICEs etc
	// "ircgate" simply gives us the chance to change a few message
	// formats specifically for the gateway, textdb inheritance takes
	// care of the rest. theoretically we could enable multiple
	// languages here, but uh.. do you care?
#ifdef SERVER
	sTextPath(0, 0, "irc");
#else
	sTextPath(0, 0, "ircgate");
#endif
	decodeInit();
	return ::circuit(ho, po || IRC_SERVICE);
}

ircMsg(from, cmd, args, text, all) {
	string *a, t, rcpt, deb;

	//PT(("ircMsg(%O,%O,%O,%O)\n", from,cmd,args,text))
	notice = 0;
	next_input_to(#'parse);
	a = explode(cmd+" "+args, " ");		// TODO! redefine a properly
	rcpt = sizeof(a)>1 && a[1];
				    // here we presume PSYC.EU is uppercase!
	if (strlen(rcpt) == sizeof(IRCGATE_NICK)
	    && upper_case(rcpt) == IRCGATE_NICK) rcpt = 0;
#define DEB if (!deb) deb = MYNICK+" "+(from||"")+" "+\
		(t||"")+" '"+(text||"")+"'\n"
//	DT( if (cmd != "ping") { DEB; D(deb); } )
	switch(cmd) {
case "ping":    
                //emit(SERVER_SOURCE "PONG " SERVER_HOST " :"+ MYNICK +"\n");
//		emit("PONG " SERVER_HOST " :"+ MYLOWERNICK +"\n");
//		emit("PONG " SERVER_HOST " :"+ rcpt +"\n");
		emit("PONG " IRCGATE_NICK " :"+ text +"\n");
		return 1;
#ifndef RELAY
case "notice":
#ifndef RELAY_SMART_IRCSERVS
		// would be better if servs weren't talking to us!!
		if (strlen(from) > 7 && from[4 .. 7] == "Serv") {
			// check for 'Nick' and 'Chan' ?
			P2(("... ignoring crap from "+ from +"\n"))
			return 1;
		}
#endif
		// strange behaviour of some ircds.. sending notices
		// to a virtual user called 'auth' before login
		// correct behaviour is to send "NOTICE *"
		if (rcpt == "*"
#ifdef SERVER
		    || rcpt == IRCGATE_NICK
#endif
		    || rcpt == "AUTH") return 1;
		// notice from my or any other server
		if (from && index(from, '.') > 0) {
			joe = from;
			from = 0;
		}
		notice = 1; // fall thru
case "privmsg":
		if (joe_nick = from) {
		    joe = joe_nick;
		    if (sscanf(joe, "%s!%s@%s", joe_nick)) {
			// if (stupidNetwork())
			joe = joe_nick;
		    }
		    joe_unl = IRCER_UNIFORM(joe);
		    joe = lower_case(joe);
		}
#ifdef SERVER
# ifdef IRCGATE_REGISTERED_ONLY
		unless (regged[joe]) {
//			emit(SERVER_SOURCE + ERR_SUMMONDISABLED +" "+
//			     joe_nick +" :You're not registered yet.\n";
			render("_error_necessary_registration",
		  "Sorry, you cannot use this without prior registration.", ([
				"_source_hack": IRCGATE_NICK,
				"_nick_me" : joe_nick,
			   ])); 
			return 1;
		}
# endif
# ifndef IRCGATE_FULL_UNIFORM_SUPPORT
		// also accept uniform@PSYC syntax.. necessary when the
		// ircd has not been patched to always forward uniforms
		// to the psyced. newer patches however do that, so we
		// should not need this.
		if (upper_case(rcpt[<sizeof(IRCGATE_NICK) ..]) == IRCGATE_NICK)
		    rcpt = rcpt[0 .. <sizeof(IRCGATE_NICK)+2];
# endif
		PT(("the rcpt is %O for %O from %O\n", rcpt, text, joe_unl))
		if (!rcpt || channel2place(rcpt)) pubmsg(text);
		else {
			unless (text = decode(text, rcpt, !notice)) break;
			sendmsg(rcpt, notice? "_message_private_annotate":
					 "_message_private", text,
			    ([ "_nick" : joe_nick ]), joe_unl);
		}
#else
		unless (rcpt && (rcpt = channel2place(rcpt))) rcpt = 0;
		unless (text = decode(text, rcpt, !notice)) break;
		if (rcpt) {
		    if (from) pubmsg(text);
					    // weird hack, i know
		    else castmsg(ME, "_notice_IRC", "[_server]: [_text]",
			    ([ "_text": text, "_server": joe ]) );
		}
		else unless (notice) input(text);
#endif
		break;
#endif
case ERR_NOSUCHNICK:
#ifdef SERVER
	        if (rcpt && sizeof(a)>2)
		     sendmsg(rcpt, "_error_unknown_name_user",
				   "[_nick_target] isn't available.",
				   ([ "_nick_target": IRCER_UNIFORM(a[2]) ]) );
#else
		if (psycer) sendmsg(psycer, "_error_unknown_name_user_IRC",
				from+": "+text+"\n");
#endif
		break;
case RPL_AWAY:
		if (psycer) sendmsg(psycer, "_status_person_absent_IRC",
				from+" is away: "+text+"\n");
		break;
case RPL_WHOREPLY:
		// parsewho(a, text);
		break;
case RPL_LUSERCLIENT:
		monitor_report("_notice_circuit_established_IRC",
		     from+": "+text);
		break;
case RPL_NAMREPLY:
		names = text; namesfrom = from; namesto = a[3][1 ..];
#ifdef CHAT_CHANNEL
		castmsg(ME, "_notice_place_members_IRC",
		     "On [_nick_place_IRC]: [_members_IRC]",
		    ([  "_members_IRC": names,
		         "_source_IRC": namesfrom,
		     "_nick_place_IRC": namesto ]));
#endif
#ifndef SERVER
		monitor_report("_notice_place_members_IRC",
		     "irc://"+namesfrom+"/"+namesto+" contains: "+names);
		break;
case "join":
		unless (rcpt) rcpt = text; // historic syntax bug in JOIN
		if (rcpt && (rcpt = channel2place(rcpt))) {
			if (stricmp(rcpt, qChatChannel())) {
				P0(("%O got autojoined into %O\n", ME, rcpt))
				emit("PART #"+rcpt+
				     " :looks like I was autojoined here\n");
				break;
			}
		} else rcpt = 0;
		// fall thru
case "quit":	// IRC QUIT is broken really
case "part":
		sscanf(from, "%s!%s@%s", t);
		castmsg(IRCER_UNIFORM(from), "_notice_place_IRC_"+cmd,
		    "[_nick] "+ cmd +"s [_channel].",
		    ([ "_nick" : t, "_channel" : (rcpt||text) ]));
		break;
case "001":
#ifndef SERVER
		emit("JOIN #"+qChatChannel()+" "+CHAT_CHANNEL_MODE+"\n");
#endif
#ifdef EXTRA_LOGON
		emit(EXTRA_LOGON+"\n");
#endif
		// calling showStatus on myself for my fellow ircers
		::showStatus(VERBOSITY_IRCGATE_LOGON, 0, ME);
		break;
case "002":
case "003":
case "004":
case "005":
case RPL_LUSEROP:
case RPL_LUSERUNKNOWN:
case RPL_LUSERCHANNELS:
case RPL_LUSERME:
case "250":
  // Highest connection count: 775 (774 clients) (19656 connections received)
case "265":
case "266":
case RPL_MOTDSTART:
case RPL_MOTD:
case RPL_ENDOFMOTD:
//case ERR_NOSUCHCHANNEL: // UMODE error messages from old servers
case RPL_ENDOFNAMES:
case RPL_ENDOFWHO:
		break; // ignore'm all
default:
#ifdef RELAY
		if (strlen(from) && index(from, '.') == -1) {
			mixed u = find_person(from);
			PT(("cmd:relay %O to %O!\n", cmd, u))
			if (u) return u->ircMsg(from, cmd, args, text, all);
		}
#else
		DEB;
		castmsg(ME, "_notice_unexpected_message_IRC", chomp(deb), ([]));
		// D1(D(deb);)
#endif
		// fall thru
case ERR_NOSUCHSERVER:
case "mode":
		DEB;
		log_file("IRCEVENT", deb);
#else
case "part":	// careful about PART in particular:
		// normal irc/user behaviour is to echo this
		// which in S2S makes no sense!
		break;
case "sjoin":	
# if 0 // taken from gateway.c, works quite well, but the place is too noisy
	{
		// sjoin timestamp channel modelist :list-of-nicks
		rcpt = find_place(channel2place(a[2]));
		string *members;
		members = map(explode(trim(text), " "), 
			      // watch out, this should to be adapted to the
			      // channel privileges (like chanadmin (.),
			      // chanpop (@), halfop (%) and voice(+)).
			      // ifdef as needed
			      (: 
			       return (
				       $1[0] == '@' || 
				       $1[0] == '+' ||
				       $1[0] == '.' || 
				       $1[0] == '%') 
			       ? $1[1..] : $1; 
				:));
		foreach (string member : members) {
		    sendmsg(rcpt, "_request_enter", 0,
			    ([ "_nick" :  member,
			     "_INTERNAL_origin" : ME ]), summon_person(member));
		}
	}
# endif
		break;
case "topic":	// let's postpone these complications a bit
# if 0 // likewise from gateway.c
	{
		// source TOPIC channel topic-user timestamp :text
		// TODO: we need to catch the "answer" to that
		rcpt = find_place(channel2place(a[1]));
		sendmsg(rcpt, "_request_set_topic", 0, 
			([ 
			 "_topic" : text,
			 "_nick" : a[2] ]),
			summon_person(a[2]));
	}
# endif
#endif
		break;
#ifdef SERVER
case "gline":
		// block'ing may be appropriate... 
		// relaying to a remote place is not,
		// ignore it for now
		break;
case "nnick":	// ptlink6 style
case "nick":
		PT(("NICK START\n"))
		if (trail("Serv", rcpt)) return 1;
#ifndef IRCGATE_REGISTERED_ONLY
		ulogin(rcpt);
#endif
		// combined nick/user announcement
		if (sizeof(a)>4) {
#ifdef IRCGATE_REGISTERED_ONLY
			if (index(a[4], 'r') >= 0)
			    ulogin(rcpt);
#endif
			return 1;
		}
		// otherwise nickchange, fall thru
case "quit":
		if (from) ulogout(from);
		PT(("QUIT NICK\n"))
		return 1;
#ifdef IRCGATE_REGISTERED_ONLY
case "mode":
		if (strlen(text) && index(text, 'r') >= 0) {
//			regged[lower_case(rcpt)] = text[0] == '-' ? 0 : 1;
			if (text[0] == '-') ulogout(rcpt);
			else ulogin(rcpt);
		}
#endif
		return 1;
#endif
	}
	return 1;
}

action(text, rcpt) {
#ifdef SERVER
	if (rcpt) sendmsg(rcpt, "_message_private", 0,
		([ "_nick" : joe_nick, "_action": text ]), joe_unl);
#else
	if (!rcpt && talk[joe])
	    sendmsg(talk[joe], "_message_private", 0,
		([ "_nick" : joe_nick, "_action": text ]), joe_unl);
#endif
	else castmsg(joe_unl, "_message_public", 0,
		    ([ "_nick" : joe_nick, "_action": text ]));
}

// incomplete TODO
version(text, rcpt, req) {
#ifdef SERVER
	if (rcpt)
	    ::version(text, rcpt, req, joe_nick, joe_unl);
#else
	if (talk[joe])
	    ::version(text, talk[joe], req, joe_nick, joe_unl);
#endif
	//else
	    // send *my* version info
}

ctcp(code, text, rcpt, req) {
#ifdef SERVER
	if (rcpt)
	    ::ctcp(code, text, rcpt, req, joe_nick, joe_unl);
#else
	if (talk[joe])
	    ::ctcp(code, text, talk[joe], req, joe_nick, joe_unl);
#endif
}

static pubmsg(text, ext) {
	castmsg(joe_unl, ext ? "_message_public_external" : "_message_public",
		    text, ([ "_nick" : joe_nick ]));
}

unknownmsg(text) {
	PT(("unknown: "+text+"\n"))
	if (abbrev("*** ", text)) {
		text = text[4..];
		notice = 1;
	}
	pubmsg(text, 1);
	if (notice) return;
//	P1(("%s tells %s: %s [%O]\n", joe, MYNICK, text, talk))
//	reply("Your message has been relayed. Try HELP for help.");
#ifndef SERVER
	return ::unknownmsg(text);
#endif
}

#ifndef SERVER
static reply(text) {
	if (text && text != "") emit("NOTICE "+joe+" :"+text+"\n");
}

static stat() {
	// calling showStatus on myself for my friend joe
	::showStatus(VERBOSITY_IRCGATE_USER, 0, ME);
	return ::stat();
}
#endif

reboot(reason, restart, pass) {
#ifdef SERVER
	emit("SQUIT :Service "+ (restart ? "restart" : "shutdown")
	     +": "+ reason +"\n");
#else
	emit("QUIT :Service "+ (restart ? "restart" : "shutdown")
	     +": "+ reason +"\n");
#endif
	return ::reboot(reason, restart, pass);
}

static disc() {
	log_file("IRC_BOSS", "%O disconnects %O\n", previous_object(), ME);
#ifdef SERVER
	emit("SQUIT :Service temporarily disabled\n");
#else
	emit("QUIT :Service temporarily disabled\n");
#endif
	return 1;
}

logon(failure) {
	int rc = ::logon(failure);
	unless (rc) return 0;
#ifdef SERVER
# ifdef ADVERTISE_PSYCERS
	gated = ([]);
# endif
# ifdef IRCGATE_REGISTERED_ONLY
	regged = ([]);
# endif
	// leave login procedure to place.gen
#else
	emit("NICK "+ IRCGATE_NICK +"\r\nUSER "+ IRCGATE_USERID
	    +" . . :"+ IRCGATE_NAME +"\r\n"
#ifdef IRCGATE_HIDE
	    +"MODE "+ IRCGATE_NICK +" +i\r\n"
#endif
	);
#endif
	next_input_to(#'parse);
	call_out(#'runQ, 3);	// deliver the queue of messages in circuit.c
	return rc;
}

//psycName() { return "@"+MYLOWERNICK; }

#if 0
static parsewho(a, text) {
//	string channel, uid, host, server, nik, stat, hops, ircname;
//	sscanf(s, "%s %s %s %s %s %s :%s %s", channel, uid, host, server, nik, stat, hops, ircname);

	sscanf(text, "%s %s", hops, ircname);
	host = a[2];
	nik = a[4];
	if (!nik || host == "Host") return;
	if (wflag == "l" && ircname)
		sendmsg(wuser, "_who", nik+" ("+uid+"@"+host+") is \""+ircname+"\"\n");
	else
		sendmsg(wuser, "_who", ladjust(nik,10)+"("+uid+"@"+host+")\n");
}

// old stuff from the MUD gateway

ircwho(mask) {
	if (!interactive(ME)) return;
	log_file("IRCWHO", mask ? mask : "(null)");
	wflag = 0;
	if (mask && !sscanf(mask, "-%s %s", wflag, mask)
	 && sscanf(mask, "-%s", wflag))
		mask = 0;
	if (!mask || strlen(mask) < 3) mask = DEF_WHOMASK;
	emit("WHO "+mask+"\n");
	wuser = this_player();
	return 1;
}

// Quote function for gods, use with marker
quote(text) {
	if (!userp(this_player())
	 || this_player()->query_level() < LEVEL_GOD) return;
	sender = this_player();
	emit(text+"\n");
	return 1;
}

static plist() {
	array(string) list=explode(read_file(TOP_PLAYERS_FILE),"\n");
	for (int i=0; i<sizeof(list); i++) {
		reply(list[i]);
	}
	return;
}

#endif

disconnect(remainder) {
	// we typically just want to give it one more try
	reconnect();
	// the reconnect in circuit.c will do certain amount of retries only
        return 0;   // unexpected
}

#ifdef CHAT_CHANNEL
// place specific methods..

tellChannel(source, mc, vars, data) {
#ifndef SERVER
	data = irctext( T(mc, ""), vars, data, source );
	PT(("tellChannel(%O) %O\n", mc, data))
	send(source, "NOTICE #"+qChatChannel()+" :"+ data +"\n");
#endif
}

msg(source, mc, data, mapping vars, showingLog, target) {
	string t, ni, mca;

	if (showingLog) return 1;
	PT(("GATEMSG(%O,%O,%O,%O,%O,%O)\n", source,mc,data,vars,
					    showingLog,target))
	unless(interactive()) {
		connect();
		// special requirement for enqueue
		unless (vars["_source"])
		    vars["_source"] = UNIFORM(source);
		return enqueue(source, mc, data, vars, showingLog, target);
	}
	if (abbrev("_status_place",mc) || abbrev("_message_announcement",mc)) {
		if (source && source != ME) {
			monitor_report("_error_rejected_message_IRC",
			    S("Caught a %O from %O (%O)", mc, source, data));
			return;
		}
		if (joe_unl && target == joe_unl)
		     reply(irctext( T(mc, ""), vars, data, source ));
		else tellChannel(source, mc, vars, data);
		//
		// i don't know why it doesn't show member listings to
		// psyced-ircd's - must be something in net/irc/user.c -
		// but we're not using this on psyceds anyway..
		return 1;
	}
	if (!data && stringp(vars["_action"]) && abbrev("_message", mc))
	    mca = mc +"_action";
	else
	    mca = mc;

	// und DCC und freundschaft und /req ... benötigt alles
	// eine generischere oder eben eigene irc-textdb

	// hmm... echo geht immernoch nicht.. -|klotz- You tell 0: TODO
	if (abbrev("_message_echo", mc)) return;
	P4(("gatebot %O,%O,%O target: %O\n", source,mc,data, target))
#ifndef RELAY
	if (target && stringp(target) && abbrev(MYLOWERNICK, target))
       	{
#if 0
	    //unless (stupidNetwork())
	      unless (sscanf(target[1+strlen(MYLOWERNICK)..], "%s!%~s@%~s", ni) == 3)
		    ni = target[1+strlen(MYLOWERNICK)..];
#else
	    //if (stupidNetwork())
		ni = target[1+strlen(MYLOWERNICK)..];
#endif
#else
		ni = target;
#endif
# ifdef IRCGATE_REGISTERED_ONLY
	    unless (regged[lower_case(ni)]) {
		    sendmsg(source, "_error_unknown_name_user",
			"No [_nick_target] is available for talking.",
			    ([ "_nick_target" : ni ]));
		    return 1;
	    }
# endif
#ifndef SERVER
	    psycer = source;
	    if (stringp(source)) vars["_nick"] = source; // remote psycer
	    // t = irctext( T(mca, 0), vars, data, source );
#else
	    t = irctext( T(mca, data), vars, data, source );
#endif
	    if (t && strlen(t)>5)
	    {
		int pm = 0;
		if (abbrev("_message", mc)) {
		    unless (trail("_annotate", mc)) pm = 1;
		    // irc doesn't echo, so we "fake" it here
		    sendmsg(source, "_message_echo"+ mc[8..], data, vars);
		} else if (abbrev("_request", mc))
		    pm = 1;
#ifndef SERVER
		send(source, (pm ? "PRIVMSG ": "NOTICE ") + ni
				 +" :"+ t +"\n");
		if (pm && !talk[ni]) {
		    joe_unl = target;
		    joe_nick = ni;
		    joe = lower_case(joe_nick);
		    talkto(source);
		}
#else
		// TODO: remove 'pm' logic?
		vars["_source_hack"] = source;
		vars["_nick_me"] = ni;
		//send(source, psyctext(t, vars, data, source));
		render(mca, data, vars, source);
#endif
	    }
#ifndef RELAY
	    else sendmsg(source, "_failure_unsupported_function_IRC",
		"Sorry, [_method] is not supported by the IRC gateway.",
		([ "_method": mc ]) );
	    return 1;
	}
	// if (::msg(source, mc, data, vars) && abbrev("_message", mc)) {
	// sollte 1 zurückgeben wenn die msg ok ist
#ifdef IRCGATE_HISTORY
	"place/storic"::msg(source, mc, data, vars);
#else
	"place/public"::msg(source, mc, data, vars);
#endif

	// ein generischer renderer muss her.. das hier ist
	// zu blöd so, erkennt keine varianten mit _action etc
	if (abbrev("_message_public", mc)) {
#ifdef SERVER
//	    t = data || irctext( T(mca, 0), vars, data, source );
//	    send(source, "PRIVMSG #"+qChatChannel()+" :"+ t);
#else
	    if (stringp(source)) vars["_nick"] = source;
	    send(source, "PRIVMSG #"+qChatChannel()+" :"+
		 irctext( T(mca, 0), vars, data, source ));
#endif
	}
	else {
		PT(("abgefangen: %O\n", mc))
	}
#endif
	return 1;
}

static chanop(arg) {
#ifndef SERVER
	log_file("IRC_BOSS", "%O chops %O on %O\n", previous_object(), arg, ME);
	emit("MODE #"+ qChatChannel()+ " +o "+arg+"\n");
#endif
	return 1;
}

cmd(a, args, b, source, vars) {
	string t;

	// extra room commands for operators of the gateway
	if (b > 0) switch(a) {
	case "chop":
		if (sizeof(args) == 2) chanop(args[1]);
		return 1;
	case "disc":
		return disc();
#if !defined(SERVER) || DEBUG > 0
	case "quote":
		t = ARGS(1);
		log_file("IRC_BOSS", "[%s] %O quotes %O\n",
			 ctime(), previous_object(), t);
		emit(t+ "\n");
		return 1;
#endif
	}
	return ::cmd(a, args, b, source, vars);
}

onEnter(source, mc, data, vars) {
	if (interactive(ME)) tellChannel(source, mc, vars, data);
	return ::onEnter(source, mc, data, vars);
}
leave(source, mc, data, vars) {
	if (interactive(ME)) tellChannel(source, mc, vars, data);
	return ::leave(source, mc, data, vars);
}

#endif
