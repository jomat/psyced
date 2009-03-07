// vim:foldmethod=marker:syntax=lpc:noexpandtab
// $Id: common.c,v 1.120 2008/12/16 11:58:52 lynx Exp $
//
// common functions for IRC servers and clients (gateways)
//
#define MYNICK qName()	// common for both server and user

#define NO_INHERIT
#include "irc.h"
//#include "hack.i"
#undef NO_INHERIT

#include "reply.h"
#include <strings.h>

#include <text.h>
virtual inherit NET_PATH "output";

volatile string prefix;

parse(a) {
	string t, from, cmd, args, text;

#ifdef _flag_log_sockets_IRC
	log_file("RAW_IRC", "%d %O\t» %s\n", time(), ME, a);
#endif
	IRCD( D("IRC< '"+a+"'\n"); )

	// some clients like ircII like to send trailing spaces
	// which may f**k up some parsing. also regular irc servers
	// ignore leading spaces, so should we
#if __EFUN_DEFINED__(trim)
	//if (a == "") return;	// but first make sure there's something at all
	//if (a[<1] == ' ' || a[0] == ' ')
	a = trim(a, TRIM_BOTH);
	// efun probably faster than doing all those checks anyway
#else //{{{
	// not as efficient as it could be, but we have trim() anyway
	// move this into a pike macro for trim() emulation?
	if (a == "") return;	// first make sure there's something at all
	while (a[0] == ' ') a = a[1..];
	while (a[<1] == ' ') a = a[0..<2];
#endif //}}}
	if (a == "") return;		// don't let " \n" execute "/s"
	unless (sscanf(a, ":%s %s", from, t)) t = a;
	sscanf(t, "%s :%s", t, text);
	unless (sscanf(t, "%s %s", cmd, args)) cmd = t;
	if (cmd) ircMsg(from, lower_case(cmd), args, text, a);
	return 1;
}

ircMsg(from, cmd, args, text, all) {
	mixed t,t1;
	switch(cmd) {
case "motd":
		motd();
		return 1;
#ifndef BETA
case "lusers":
		lusers();
		return 1;
#endif
case "time":
		t = time();
		t1 = ctime(t);
		write(SERVER_SOURCE "391 " + MYNICK + " " SERVER_HOST " " +
		     t + "0 :" + isotime(t1, 0) + " -- " + hhmmss(t1) + "\n");
		return 1;
case "ping":
		// sieht doof aus scheint aber amtlich zu sein
		// ne isses nich. man ponged, was der user gepinged hat.
//		write(SERVER_SOURCE "PONG " SERVER_HOST " :"+ MYNICK +"\n");
		// eine fantasievolle lösung:
                write(SERVER_SOURCE "PONG " SERVER_HOST " :"
		    +( text || args || MYNICK )+"\n");
                return 1;

case "quit":
		quit();
		return 1;

case "userhost":
                // kleine clientverarsche
		reply(RPL_USERHOST, ":"+args+"=+"+args+"@" SERVER_HOST);
		return 1;
default:
		// write("421 :Huh?\n");
//		D(S("Unknown IRC: %s\n", all));
//		log_file("IRCPROT", "[%s] %s\n", query_ip_number(), all);
//		return 1;
		return 0;
	}
}

motd() {
	string buffer = "";
// sollte alles in der textdb sein statt den reply hack zu verwenden
// ist aber leider ein bisschen tragisch, da sich ne motd abprubt ändern
// können soll... :( - die textdb geht ja eher von statischen inhalten aus.
// was halt ihr sinn ist.
//
// ok, also nicht die statische textdb - aber es gibt ja auch dynamische
// textdb sachen.. und ausserdem, MOTD sollte doch mehrsprachig sein können!?

	emit(sreply(RPL_MOTDSTART, ":- " SERVER_HOST " Message of the Day - "));

#ifdef MOTD_FILE
	if (file_size(MOTD_FILE) > 0) {
		string motd_file;

// wir machen demnäxt noch n anderes define, dann kommts aus der 
// datenbank. hat dann nur immernoch den beieffekt, dass in der datenbank
// direkt das MOTD-prefix stehen muss, und das werden ungefähr 3 leute
// wenn überhaupt verwenden, und alle davon aus dem kreis der muve/psycdevs.
// dummerweise fiel hier im Zuge des neuen emit das '- ' flach - fippo  
		P3(("MOTD_FILE (%O) found\n", MOTD_FILE))
		motd_file = read_file(MOTD_FILE);
		P4(("MOTD = %O\n", motd_file))
		emit(sreply(RPL_MOTD, ":" + motd_file));
	} else
#endif
	    emit(sreply(RPL_MOTD, ":- [PSYC] will probably make you happier. http://about.psyc.eu/\n"
#if 0
			    "- We're in public beta testing stage.\n"
			    "- Usage manuals and legacy transition help on http://help.pages.de/\n"
			    "- \n"
			    "- You're the administrator of this server? Save your customized\n"
#ifdef MOTD_FILE
			    "- MOTD to " MOTD_FILE " in psyced's world/ directory."
#else
			    "- MOTD to psyced's world/ directory and add\n"
			    "- #define MOTD_FILE \"<filename>\" to your local/local.h"
#endif
#endif
	));
	emit(sreply(RPL_ENDOFMOTD, ":End of MOTD command"));
	return 1;
}

#ifndef BETA
lusers() {
# ifndef _flag_disable_query_server
	reply(RPL_LUSERCLIENT, ":There are " + amount_people()
				+ " users on this server");
# endif
}
#endif

qCharset() {}

render(string mc, string data, mapping vars, mixed source) {
        string template, output;
	mixed t;

	P3(("common:render %O %O\n", ME, data));
#if 1 // def IRCEXPERIMENTAL
	template = T(mc, 0); // enable textdb inheritance
#else
	template = T(mc, ""); 
#endif
#ifndef _flag_disable_stamp_time_IRC
	t = vars["_time_place"] || vars["_time_log"];
	// this goes thru ->v()
	if (t && v("timestamp") != "off" // && abbrev("_message", mc)
	    && stringp(data)) {
		if (v("timestamp") == "on") {
			string msa = " ";
			msa[0] = 0x01; // msa's CTCP character
			// should use psyctime instead of unixtime
			data = msa +"TS "+ (t - PSYC_EPOCH) +msa+ data;
		} else
		    data = "["+ time_or_date(t) +"] "+ data; // use T() ?
		P3(("%O data is %O\n", ME, data))
	}
#endif
	P3(("c:r pre ptext: %O %O %O %O\n", template, vars, data, source ));
	output = psyctext( template, vars, data, source);
	P3(("c:r 1st ptext: %O\n", output));
	if (!output || output=="") return D2(D("irc/user: empty output\n"));

#ifdef NEW_LINE
	output += "\n";
#else
	if (template == "") output += "\n";
#endif
	if (output[0] == '#') output = SERVER_SOURCE + output[1 ..];
	else if (output[0] != ':') {
		string t2;

		if (prefix) { output = prefix+output; prefix=0; }
		// ich wage es mal hier den raumnamen als ziel auszugeben.
		// wenn das nicht für alle fälle gut ist, dann mit dem
		// if auf _notice_action einschränken. else liegt auch
		// schon bereit. alles klar --lynX
//	if (abbrev("_notice_action", mc)) {
//		output = psyc2irc(mc) +" "+(vars["_nick_place"] ?
//			place2channel(vars["_nick_place"]) : MYNICK)
//			    +" :" + output;
//	else
//		output = psyc2irc(mc) +" "+MYNICK+" :" + output;

		// TODO: in theory, going thru the textdb twice should
		// 	not be necessary, but it is the most straightforward
		// 	solution
		t2 = T(mc, data); 
		if (template != t2) {
			output = psyctext( t2, vars, data, source) + "\n";
			P3(("c:r 2nd ptext: %O\n", output));
#if DEBUG > 0
		} else {
			P3(("c:r %O==%O so there is no 2nd ptext for %O.\n"
			    "see also render(%O,%O,%O,%O)\n",
			    template, t2, output,
			    mc, data, vars, source));
#endif
		}
		unless (trail("\n", output)) {
			PT(("(harmless) no IRC template for %O\n", mc))
			log_file("IRC_TEXTDB", "%O\n", mc);
			return;
		}
		// _silent: when casts from a conversation-filtered place
		// arrive, which isnt known to the client as a channel, we
		// revert to personal notices to not irritate it.
		if (
#if 1
		    // would be nicer to check if source is a place but
		    // isn't trivial right here.. looks like joining remote
		    // xmpp: mucs is affected by this change - you may not
		    // be able to see history. fippo is looking into this.
		    // <fippo> i wanted this for the _notice_place_* use case
		    // 	i would propose keeping the old way and adding a 
		    // 	condition with _nick_place && abbrev(_notice_place, mc)
		    (vars["_nick_place"] || vars["_context"]) &&
#else
		    vars["_context"] && vars["_nick_place"] &&
#endif
			    (vars["_INTERNAL_control"] != "_silent" ||
				v("entersilent") == "off")) {
			P2(("irc/user: psyc2irc channel notice (%O)\n", mc))
			// psyc2irc is called without source||context here so 
			// this will be server notices - it is yet to be 
			// determined if this is appropriate
			output = psyc2irc(mc, 0) +" "+ // MYNICK+" "+
			    place2channel(vars["_nick_place"]) +" :" + output;
		} else {
			//PT(("irc/user: psyc2irc personal notice (%O) with %O\n", mc, vars))
			output = psyc2irc(mc, source) +" "+MYNICK+" :" + output;
		}
	}
#ifdef NO_SMART_MSA
	else {
		// klammern und indent gemacht aus style-gruenden
		for (i=strstr(output, "%"); i>=0; i=strstr(output, "%", i)) {
			output[i] = 0x01; // support for msa's CTCP char
		}
	}
#endif
	// i would really love to do it using the textdb, but it is unflexible.
	// probably i'll change that somewhen.
	if (mc == "_status_place_members_amount") {
	    // since irc ALWAYS needs to have a namreply, we
	    // simply give it one that only contains ourselves
	    w(mc[..<8], 0, ([ "_nick_place" : vars["_nick_place"],
			      "_members" : vars["_INTERNAL_nick_me"] ]));
	    w(mc[..<8] + "_end", 0, vars);
	}
	P4(("calling emit(%O)\n", output));
	emit(output);
}

// server:w() doesnt call this anyway, so we dont need last_prefix
//volatile private string last_prefix;
emit(string output) {
	string* outlines;

	P3(("common:emit %O %O\n", ME, output));
	// misteries of virtual inheritance.. why doesnt this get called
	// from gatebot? we need to get this working for 512-split!!
#ifdef _flag_log_sockets_IRC
# define EMIT(OUT) (log_file("RAW_IRC", "%d %O\t« %s", time(), ME, OUT), ::emit(OUT))
# else
# define EMIT ::emit
#endif
#if __EFUN_DEFINED__(convert_charset)
	string cs = qCharset();
        if (cs && cs != SYSTEM_CHARSET) {
		iconv(output, SYSTEM_CHARSET, cs);
		P4(("output in %O = %O\n", cs, output))
        }
#endif
	if (output[<1] != '\n') {
		// this is used when prompting for password
		P4(("irc:emit optimized for prefix on %O\n", output))
		return EMIT(output);
	}
	outlines = explode(slice_from_end(output, 0, 2), "\n");
	if (sizeof(outlines) == 1 && strlen(output) < MAX_IRC_BYTES) {
		// optimized for single line
		P4(("irc:emit single %O\n", outlines[0]))
		return EMIT(outlines[0] + "\r\n");
	} else {
		string split_prefix, line;
		int cut, t;

		cut = strstr(output, " :");
		P4(("IRC:emit splitting large line at %O\n%O\n", cut, outlines))
		if (cut >= 0) {
			split_prefix = outlines[0][.. ++cut];
#if 0 // annoying rendering bug we had here.. but is <2 always wrong?
			outlines[0] = outlines[0][++cut .. <2];
#else
			outlines[0] = outlines[0][++cut .. <1];
#endif
			if (strlen(outlines[0])
			    && outlines[0][0] == outlines[0][<1]
			    && outlines[0][0] == 0x01) {
				outlines[0] = chop(outlines[0]);
			}
		} else {
			split_prefix = ""; //last_prefix;
			outlines[0] = chop(outlines[0]);
		}
	       	// because of additional \\\r\n we have to subtract 3
		cut = MAX_IRC_BYTES-3 - strlen(split_prefix);
		if (cut < 9) {
			// happens when 005 messages grows too big
			P1(("%O encountered data w/out a decently placed ':' "
			    "to be able to do 512 splitting\n%O\n", ME, line))
			return;
		}
		foreach(line : outlines) if (strlen(line)) {
			while (strlen(line) > cut) {
#if 1
	// we shall look for last whitespace instead
			    t = rindex(line, ' ', cut);
			    if (t > 9) {
				// msa's CTCP character
				if (line[0] == 0x01) { // can only be true in
						       // the very first cycle.
						       // any realistic ideas
						       // how to do do this
						       // just once?
					P4(("splitting an msa line %O\n", line[.. t]))
					EMIT(split_prefix + line[..t]
					       + line[0..0] + "\r\n");
					line = line[t+1 ..];
				} else {
					P4(("time for %O\n", line[.. t]))
					EMIT(split_prefix + line[.. t]+ "\r\n");
					line = line[t+1 ..];
				}
			    } else {
				P1(("%O encountered data w/out ' ' to "
				    "allow for a decent IRC 512 split:"
				    "\n%O\n", ME, line))
				// ignore this junk
				return;
				// we might aswell use the old
				// backslash splitting code below here
			    }
#else //{{{
				t = line[cut] == '\r' ? cut-1 : cut;

				// msa's CTCP character
				if (line[0] == 0x01) { // can only be true in
						       // the very first cycle.
						       // any realistic ideas
						       // how to do do this
						       // only once?
					// inserting stargazer-style backslashes
					EMIT(split_prefix + line[..t-1]
					       + "\\" + line[0..0] + "\r\n");
					line = line[cut..];
				} else {
					EMIT(split_prefix + line[..t]
					       + "\\\r\n");
					line = line[cut+1..];
				}
#endif //}}}
			}
			P4(("irc:emit each %O\n", line))
			EMIT(split_prefix + line +"\r\n");
		}
	}
	return 1;
}

internalError() {
#ifdef RELAY
    // yay. we could "fix" it here by
    next_input_to(#'parse);
    PT(("crazy experimental autofix in internalError()\n"))
#else
    emit("ERROR :Congratulations! you triggered a runtime error in psyced."
	 " This server will no longer accept input until you reconnect.\n");
#endif
}

