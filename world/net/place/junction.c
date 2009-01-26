// $Id: junction.c,v 1.22 2008/03/28 20:05:44 lynx Exp $ // vim:syntax=lpc
//
// quick attempt at glueing up master and slave to obtain a junction
// node in a simple tree-form multicast structure. it is more advanced
// than irc, because each place has its own tree, but it's
// still not as smart as a routing multicast approach..		-lynX

#include <net.h>
#include <person.h>
#include <status.h>

inherit NET_PATH "place/master";
inherit NET_PATH "place/slave";

histClear(a, b, source, vars) { return "master"::histClear(a, b, source, vars); }

memberInfo(person) { return "slave"::memberInfo(person); }

mixed isValidRelay(mixed x) { 
    return "slave"::isValidRelay(x) || "master"::isValidRelay(x); }

#ifndef QUIET_REMOTE_MEMBERS

castmsg(source, mc, data, vars) {
    if (objectp(source)
	&& (abbrev("_request_place_enter", mc)
	    || abbrev("_request_enter", mc)
	    || abbrev("_request_context", mc)
	    || abbrev("_notice_context", mc)
	    || abbrev("_notice_place_enter", mc)
	    || abbrev("_notice_place_leave", mc)
	    || abbrev("_request_leave", mc))) {
	return "slave"::castmsg(source, mc, data, vars);
    }

    return "master"::castmsg(source, mc, data, vars);
}

castPresence(source, mc, data, vars) {
    if (source == master || vars["_source_relay"] == master) {
	return "master"::castmsg(ME, mc, data 
			    ? data 
			    : "[_nick] enters [_nick_place].", 
			vars + (stringp(source) && is_formal(source)
			    ? ([]) : ([ "_source_relay" : source ])));
    }
    return 0;
}
#endif

msg(source, mc, data, vars) {
	P1(("junction:msg(%O, %O, %O, %O)\n", source, mc, data, vars))
	unless (source == master
		|| vars["_source_relay"] == master
		|| vars["_context"] == master
		|| mc == "_request_link") {
	    return "slave"::msg(source, mc, data, vars);
	} else {
	    return "master"::msg(source, mc, data, vars);
	}
	// going thru both probably results in calling "storic"::msg twice
}

reboot(reason, restart, pass) {
	"master"::reboot(reason, restart, pass);
	return "slave"::reboot(reason, restart, pass);
}

qJunction() {
    return 1;
}
