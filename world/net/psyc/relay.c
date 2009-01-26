// $Id: relay.c,v 1.9 2007/09/18 08:37:58 lynx Exp $ // vim:syntax=lpc
//
// to become a generic gateway to all sorts of services
//
// dies ist noch auf AIM zugeschnitten.. "aim" und AIM_GATEWAY müssen
// einfach nur in konfigurierbare strings rein und schon hat man
// einen generischen relay für allerlei zwecke.

#include <net.h>
#include <url.h>
//#include <text.h>

//inherit NET_PATH "gateway/generic";
inherit PSYC_PATH "active";

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
		if (u && u[URL_TRANSPORT] == "c") {
			ahost = u[URL_HOST];
			aport = u[URL_PORT];
			register_target(lower_case(AIM_GATEWAY));
		}
		else ahost = 0;
//		joe = "nobody";
//		sTextPath("default", "en", "aim");
		register_scheme("aim");
}

msg(source, mc, data, mapping vars, showingLog, target) {
	if (target && stringp(target) && abbrev("aim:", target)) {
		vars["_source_relay"] = source;
		vars["_target_relay"] = target;

		if (interactive(ME))
		    return ::msg(target, mc, data, vars, ME);
		    //delivermsg(target, mc, data, vars, ME);
		else
		    sendmsg(AIM_GATEWAY, mc, data, vars);
	} else if (source && stringp(source) && abbrev(AIM_GATEWAY, source)) {
		target = vars["_target_relay"];
		source = vars["_source_relay"];
		m_delete(vars, "_target_relay");
		m_delete(vars, "_source_relay");
		sendmsg(target, mc, data, vars, source);
	}
}
#endif
