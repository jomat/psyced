// $Id: icq.c,v 1.47 2007/08/27 16:54:12 lynx Exp $ // vim:syntax=lpc
//
// this code is no longer in use. component.c has changed.
// and the gatebot principle isn't politically well accepted
// with the centralistic providers. but there may be others,
// or politics may change, so this code may return into action
// one day. alright, maybe not exactly the code as it is in
// this file, but after some correction. it's better to start
// from something that needs to be updated than to spend days
// writing something from scratch. or maybe it isn't, but at
// least i had the option.
//
#if 0
//
// component logic for jabber aim-t transports (icq, aim)
//
// this registers a gateway bot implemented by an external jabber server.
// unfortunately this plan has not proven any better than doing an aim
// gateway in python (see aim.c)
//
#include <net.h>
#include <url.h>
//#include <text.h>
#include <xml.h>

inherit JABBER_PATH "component";
inherit GATEWAY_PATH "generic";

volatile string buf;

void create() {
    object o = find_object(CONFIG_PATH "config");

    if (o) config = o->qConfig();
    if (!config) return;
    P2(("\n%O loads config %O\n", ME, config))

    joe = "nobody";
    sTextPath(0, 0, "jabber");
    // init();
    connect();
}

msg(source, mc, data, mapping vars, showingLog, target) {
    P1(("%s:msg %O » %O (%s, %O, %O)\n",
	 config["scheme"], source, target, mc, data, vars))

    unless (mappingp(vars)) vars = ([]);
    if (abbrev("_message", mc)) data = xmlquote(data);
    if (target && stringp(target) && abbrev(config["scheme"] + ":", 
					    target)) {
	string uin;
	sscanf(target, config["scheme"] + ":%s", uin);
	vars["_source_relay"] = source;
	vars["_INTERNAL_source_jabber"] = "PSYCgate@" + SERVER_HOST; 
	vars["_INTERNAL_target_jabber"] = uin + "@" + config["name"];
	w(mc, data, vars, source);
	// REPLY mode by default
	unless (talk[target]) {
//	    		talk[target] = source;
	    bot(target, 0, source);
	}
    }
}

bot(icqer, data, psycer) {
    // joe_nick = 
    joe = joe_unl = icqer;
    buf = 0;
    if (data) input(data);
    else talkto(psycer);
    if (buf) msg(ME, "_notice_reply",
	    "<message from='[_source_jabber]' to='[_target_jabber]' type='chat'><body>"+ buf + "</body></message>", 0,0, joe);
}

reply(text) {
    text = xmlquote(text);
    if (buf) buf+="\n"+text;
    else buf=text;
}

onHandshake() {
    // do not use a standard method here, or the template will be replaced
    // by textdb into something incompatible
    w("_notice_presence_here_gateway", 
	 "<presence to='[_INTERNAL_target_jabber]'"
	 " from='[_INTERNAL_source_jabber]'/>",
      ([ "_INTERNAL_target_jabber" : config["name"] + "/registered",
	 "_INTERNAL_source_jabber" : "PSYCgate@" + SERVER_HOST,
	 "_INTERNAL_mood_jabber" : "neutral" ]));

}

jabberMsg(XMLNode node) {
    string uin;
	
    P2(("%s gateway jabberMsg \n", config["scheme"]))
    switch(node[Tag]) {
    case "xdb":
	xdb(node);
	break;
    case "presence":
	switch(node["@type"]) {
	case "error":
	    P2(("gateway/%s error %O\n", config["scheme"], 
		node["/error"][Cdata]))
	    break;
	case "unavailable":
	    P2(("gateway/%s unavailable %O\n", config["scheme"], 
		node["/status"][Cdata]))
	    break;
	case 0:
	    unless(node["/status"]) return -1;
	    if (node["/status"][Cdata] == "Connected" || 
		node["/status"][Cdata] == "Online") {
		P2(("registering scheme %O\n", config["scheme"]))
		register_scheme(config["scheme"]);
	    }
	    break;
	default:
	    P2(("gateway/%s presence default: %O\n", config["scheme"], 
		node["@type"]))
	    break;
	}
	break;
    case "message": 
	// message 
	// if(node["@from"] == "PSYCgate@" + SERVER_HOST) return;
	P2(("talk %O, joe %O\n", talk, node["@from"]))
	sscanf(node["@from"], "%s@", uin);
	P1(("config %O\nnode %O\n", config, node))
	bot(config["scheme"]+":" + uin, node["/body"][Cdata], 0);
	break;
    default:
	return ::jabberMsg(node);
    }
}

xdb(node) {
    switch(node["@type"]) {
    case "get":
	switch(node["@ns"]) {
	case "aimtrans:data":
	    emit(sprintf("<xdb type='result' to='%s' from='%s' id='%s' ns='%s'>"
			 "<aimtrans><login id='%s' pass='%s'/></aimtrans>"
			 "</xdb>",
			 node["@from"], node["@to"],
			 node["@id"], node["@ns"],
			 config["nickname"], config["password"]));
	    break;
	case "aimtrans:roster":
	    emit(sprintf("<xdb type='result' to='%s' from='%s' ns='%s' "
			 "id='%s'><aimtrans><buddies/></aimtrans></xdb>",
			 node["@from"], node["@to"], 
			 node["@ns"], node["@id"]));
	    break;
	default:
	    break;
	}
	break;
    case "set":
	break;
    default:
	break;
    }
}

quit() {
    emit(sprintf("<presence type='unavailable' from='%s' to='%s'/>",
		 "PSYCgate@" + SERVER_HOST, config["host"] + "/registered"));
    emit("</stream:stream>");
}

reboot(reason, restart, pass) {
    emit(sprintf("<presence type='unavailable' from='%s' to='%s'/>",
		 "PSYCgate@" + SERVER_HOST, config["host"] + "/registered"));
    emit("</stream:stream>");
    ::reboot(reason, restart, pass);
}

#endif
