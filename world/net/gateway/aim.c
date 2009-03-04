// $Id: aim.c,v 1.35 2007/09/18 08:37:57 lynx Exp $ // vim:syntax=lpc
//
#if 0
//
// this gateway uses an external python script called AIMgate.py
// or something like that. should be in the pypsyc dist somewhere.
// we tried to use jabber transports instead, so look into config.c
// and aim2.c
//
// in fact both methods aren't functional right now, not in the
// gatebot style... but we keep it in case there is a chance or
// necessity to recover.
//
#include <net.h>
#include <uniform.h>
//#include <text.h>

inherit NET_PATH "gateway/generic";
inherit PSYC_PATH "active";

volatile string buf;

#ifdef AIM_GATEWAY
void create() {
	array(string) u = URL(AIM_GATEWAY);
// for a gateway that only supports tcp, append "c"
// #define AIM_GATEWAY             "psyc://aim.symlynX.com:49152c"
// for a gateway that only supports udp, append "d"
// #define AIM_GATEWAY             "psyc://aim.symlynX.com:49152d"
//
// we prefer using udp, but if the gateway cannot handle that
// we'll have to open up a tcp connection..
//
//	if (u && u[UTransport] == "c") {
		register_target(lower_case(AIM_GATEWAY));
//			ahost = u[UHost];
//			aport = u[UPort];
		connect(u[UHost], u[UPort]);
//	}
//	else ahost = 0;
	joe = "nobody";
	sTextPath("default", "en", "aim");
	register_scheme("aim");
	::create();
}

msg(source, mc, data, mapping vars, showingLog, target) {
	P1(("aim:msg %O » %O (%s, %O, %O)\n",
	     source, target, mc, data, vars))
	unless (mappingp(vars)) vars = ([]);
	if (target && stringp(target) && abbrev("aim:", target)) {
		vars["_source_relay"] = source;
		vars["_target_relay"] = target;

		P1(("aim:outgoing %s\n", mc))
		if (interactive(ME))
		    //delivermsg(target, mc, data, vars, ME);
		    ::msg(target, mc, data, vars, ME);
		else
		    sendmsg(AIM_GATEWAY, mc, data, vars);

		// REPLY mode by default
		unless (talk[target]) {
//		    talk[target] = source;
		    bot(target, 0, source);
		}
	} else if (source && stringp(source)) {
			// && abbrev(AIM_GATEWAY, source)) { needs a trustworthy check
		source = vars["_source_relay"];
		P1(("aim:incoming from %O\n", source))
		if (abbrev("_request_input", mc)) {
			bot(source, data, 0);
		} else {
#if 1
// es gibt ab sofort eine funktion in der library die dasselbe tut..
// bitte einbauen TODO ... oder sendmsg tuts? sendmsg sollte das tun
			string *u;
			target = vars["_target_relay"];
			unless (target) return;
			// bis sendmsg() lokale stringp's erkennen kann... TODO
			u = parse_uniform(target);
	//              if (u[UPort] && u[UPort] == query_imp_port()) {
			// vergleichen mit bisheriger logik in parse.i  TODO
			if (u[UResource][0] == '~') 
			    target = find_person(u[UNick]);
			else if (u[UUser]) target = find_person(u[UUser]);
			else target = find_object(u[UResource]);
	//		}
#else
			target = vars["_target_relay"];
#endif
			m_delete(vars, "_source_relay");
			m_delete(vars, "_target_relay");
			sendmsg(target, mc, data, vars, source);
		}
	}
}

bot(aimer, data, psycer) {
	// joe_nick = 
	joe = joe_unl = aimer;
	buf = 0;
	if (data) input(data);
	else talkto(psycer);
	if (buf) msg(ME, "_notice_reply",
		"<font size=2 color=\"#006600\">"+ buf + "</font>", 0,0, joe);
	//	"<html><body bgcolor=black text=green padding=9>"+
	//	buf + "</body></html>", 0,0, joe);
}

reply(text) {
//	if (buf) buf+="<br>\n"+text;	// bug in pypsyc, should be \n
	if (buf) buf+="<br>"+text;
	else buf=text;
}

#endif

#endif
