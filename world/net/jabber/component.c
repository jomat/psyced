// $Id: component.c,v 1.72 2008/10/01 10:59:24 lynx Exp $ // vim:syntax=lpc
//
// this implements a passive listener component 
#define NO_INHERIT
#include "jabber.h"
#undef NO_INHERIT
#include <uniform.h>

inherit NET_PATH "xml/common";

inherit NET_PATH "jabber/mixin_parse";
inherit NET_PATH "jabber/mixin_render";

// in theory - or future - scheme plan will be implemented by
// a gateway/imtransport which inherits this file
#define SCHEME_PLAN

volatile protected mapping config;
volatile string streamid;
volatile string componentname;
volatile int authenticated;


logon(arg) {
#ifdef _flag_log_sockets_XMPP
    log_file("RAW_XMPP", "\n%O logon\t%O", ME, ctime());
#endif
#ifdef INPUT_NO_TELNET
    input_to(#'feed, INPUT_IGNORE_BANG | INPUT_CHARMODE | INPUT_NO_TELNET);
#else
    enable_telnet(0, ME);
    input_to(#'feed, INPUT_IGNORE_BANG | INPUT_CHARMODE);
#endif
    if (this_interactive()) set_prompt("");
    set_combine_charset(COMBINE_CHARSET);
    sTextPath(0, "en", "jabber");
}

reboot(reason, restart, pass) {
    // close the stream according to XEP 0190
    if (interactive(ME)) {
	    flags |= TCP_PENDING_DISCONNECT;
	    emitraw("</stream:stream>");
    }
}

onHandshake() {
#ifdef SCHEME_PLAN
	    register_scheme(config["_scheme"]);
#else
	    register_target(XMPP + componentname);
#endif
}

#ifdef SCHEME_PLAN
jabberMsg(XMLNode node) {
    mixed *su = parse_uniform(XMPP + node["@from"]);
    string origin = config["_scheme"] + ":";
    unless(su) {
	P0(("%O could not find uniform source in %O\n", ME, node))
	return;
    }
    if (su[UUser]) 
	origin += su[UUser];
    return ::jabberMsg(node, origin, su);
}
#endif

#ifdef SCHEME_PLAN
determine_targetjid(target, vars) {
    mixed *tu;
    unless(vars["_INTERNAL_target_jabber_bare"]) {
	unless(target) 
	    vars["_INTERNAL_target_jabber_bare"] = componentname;
	else {
	    tu = parse_uniform(target);
	    if (stringp(tu[UBody]) && strlen(tu[UBody])) 
		vars["_INTERNAL_target_jabber_bare"] = tu[UBody] + "@" + componentname;
	    else 
		vars["_INTERNAL_target_jabber_bare"] = componentname;
	}
    }

    unless (vars["_INTERNAL_target_jabber"]) {
	vars["_INTERNAL_target_jabber"] = vars["_INTERNAL_target_jabber_bare"];
    }
}
#endif



waitfor_handshake(XMLNode node) {
    switch (node[Tag]) {
    case "handshake":
	if (node[Cdata] == sha1(streamid + config["_secret"])) {
	    PT(("%O component auth succeded as %O\n", ME, componentname))
	    nodeHandler = #'jabberMsg;
	    authenticated = 1;
	    emitraw("<handshake/>");
	    onHandshake();
	} else {
	    monitor_report("_error_invalid_password", 
               sprintf("%O tried to link to %O using a wrong password",
                       query_ip_name(), componentname));
	    STREAM_ERROR("not-authorized", "");    
	    remove_interactive(ME);
	}
	break;
    default:
	P0(("%O got %O while waiting for handshake\n", ME, node))
	break;
    }
}

int msg(string source, string mc, string data,
	    mapping vars, int showingLog, string target) {
#ifdef DEFLANG
    unless(vars["_language"]) vars["_language"] = DEFLANG;
#else
    unless(vars["_language"]) vars["_language"] = "en";
#endif
#if 0
    else if (abbrev("_status_person_absent", mc)) {
	PT(("Intercepted absent from %O to %O\n", mc, source, ME))
	return 1;
    }
#endif
    switch (mc){
    case "_message_echo_private":
	return 1;
    }
    // copied from active
    unless (vars["_INTERNAL_mood_jabber"])
	    vars["_INTERNAL_mood_jabber"] = "neutral";

    determine_sourcejid(source, vars);
    determine_targetjid(target, vars);

    if (vars["_place"]) vars["_place"] = mkjid(vars["_place"]);
    unless (vars["_INTERNAL_target_jabber"]) return 1;
    return ::msg(source, mc, data, vars, showingLog, target);
}

disconnected(remainder) {
    /* unregistering scheme / target is a bad idea, as the target hostname
     * does not resolve (in cases where we call it 'icq'), so we're going 
     * to enqueue until the component connects again
     */
    authenticated = 0;
    return 0;   // unexpected
}

open_stream(node) {
    string packet;
    if (node["@xmlns"] != "jabber:component:accept") {
        remove_interactive(ME); // disconnect
    }
    object o = find_object(CONFIG_PATH "config");

    if (o) config = o->qConfig(node["@to"]);
    P2(("\n%O loads config %O\n", ME, config))

    streamid = RANDHEXSTRING;
    packet = sprintf("<?xml version='1.0' encoding='UTF-8' ?>"
		     "<stream:stream "
		     "xmlns='%s' "
		     "xmlns:stream='http://etherx.jabber.org/streams' "
		     "xml:lang='en' id='%s' ", node["@xmlns"], streamid);
    if (node["@to"]) {
	packet += "from='" + node["@to"] + "' ";
    } else {
	packet += "from='" _host_XMPP "' ";
    }
    if (!config) {
	/* reply with a stream error */
	monitor_report("_error_unknown_host", 
		       sprintf("%O tried to link to %O but we dont have a config for this",
			       ME, node["@to"]));
	STREAM_ERROR("host-unknown", "")
	remove_interactive(ME);
    }
    packet += "version='1.0'>";
    emit(packet);

    nodeHandler = #'waitfor_handshake;
    componentname = node["@to"];

    return;
}
