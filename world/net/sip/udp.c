// $Id: udp.c,v 1.36 2007/09/21 11:05:03 lynx Exp $ // vim:syntax=lpc
#if 0
/* 
 * this is currently broken and I dont have the time to fix it
 */
#include <net.h>
#include <uniform.h>
#include <person.h>
#include <dns.h>

#include "sip.h"
/*
 * what this is about:
 * build a psyc presence aware SIP-Proxy that will only let friends ring
 * and only if you're present
 *
 * HIGHLY experimental module! use at your own risk
 *
 * functionality: 
 * - registering 70% 
 * 	tested with linphonec, kphone, shtoom, sipsak
 * - proxying 30%
 * 	remote user to local user 70% (rings, kphone -> linphone works)
 * 	local user to remote user 0%
 * - messaging ? sipsak -r 4404 -M -v -s sip:fippo@adamantine -B "lunch time"
 *
 * TODO
 * - better integration with core framework, e.g. using sendmsg()
 *   instead of send_udp
 *
 */
#include <net.h>
#include <uniform.h>
#include <person.h>

#include "sip.h"

inherit NET_PATH "sip/common";

send_udp_srv2(mixed *hostlist, string domain, string buf) {
	if (hostlist == -1) {
		send_udp_nonblocking(domain, 5060, buf);
	} else {
		send_udp_nonblocking(hostlist[0][DNS_SRV_NAME],
				     hostlist[0][DNS_SRV_PORT], buf);
	}
}

send_udp_srv(domain, service, prot, buf) {
	dns_srv_resolve(domain, service, prot, #'send_udp_srv2, domain, buf);
}

reset() { }

object load() { 
	reset();
	register_scheme("sip");
	::load();
	return ME; 
} 

parseUDP(ip, port, msg) {
	string cmdline, headers, data;
	mapping v;
	string source, nick, target, cmd, prot;
	string reply;
	mixed *su, *tu;
	mixed authuser;
	int islocaltarget, authstate;

	P4(("sip:parseUDP(%O,%O, ...):\n", ip,port))

	sscanf(msg, "%s" CRLF "%s", cmdline, msg);
	sscanf(msg, "%s" CRLF CRLF "%.0s", headers, data);
	unless(cmdline && headers) {
		reply = "SIP/2.0 400 Bad request!" CRLF + msg;
		send_udp(ip, port, reply);
		return;
	}
	sscanf(cmdline, "%s%t%s%t%s", cmd, target, prot);
	/* header parsing 
	 */
	v = parseHeaders(headers);
	if (v == -1) {
		// parsing failed
	}
	// TODO: loop detection im via
	// do we have to check via headers vs source ip?
	
	/* response handling and generation 
	 */
	if(abbrev("SIP", cmd) && (target = to_int(target))) {
		handleResponse(cmd, target, cmdline, v, data);
#if 0
		// should not happen as we are proxy
		if (stringp(v["via"])) {
			PT(("local response, generating 200\n"));
			reply = makeResponse(prot, 200, v);
			send_udp(ip, port, reply);
		}
#endif
		return;
	}
	if (target) tu = parse_uniform(target);
	unless(tu) {
		reply = makeResponse(prot, 400, v);
		send_udp(ip, port, reply);
		return;
	}
	islocaltarget = is_localhost(tu[UHost]);
	// TODO this may be either local or remote users
	if (islocaltarget && tu[UUser]) target = find_person(tu[UUser]); 
	// TODO works only for online users
#if 0
	unless(target) {
		// ?
	}
#endif

	if (v["authorization"]) {
		// how to integrate with net/* PSYC 
		// infrastructure?
		// for now we want features and make a dirty
		// implementation
		mapping a;
		string method, auth;
		string vname, vvalue;
		sscanf(v["authorization"], "%s%t%s", method, auth);
		/* parse the string */
		a = ([ "_method" : "REGISTER" ]);
		while(sscanf(auth, "%s=%s,%t%s", 
			     vname, vvalue, auth) >= 2) { 
			sscanf(vvalue, "\"%s\"", vvalue);
			a["_" + vname] = vvalue; 
		}
		sscanf(auth, "%s=\"%s\"", vname, vvalue);
		sscanf(vvalue, "\"%s\"", vvalue);
		a["_" + vname] = vvalue;
		a["_password"] = a["_response"];
		/* end */

		authuser = find_person(a["_username"]);
		// TODO: dont call-other 0
		// TODO: this is broken as of async checkPassword
		// 	 and will need a rewrite as it is getting ugly
		authstate = authuser -> checkPassword(a["_response"], 
						  "http-digest",
						  a["_nonce"], a);
	}
	// TODO:
	if (v["from"]) {
		sscanf(v["from"], "%s%.0t<%s>%!s", nick, source); 
		// TODO what about the extra part?
		unless(source) source = v["from"]; 
		P2(("from source %O with nick %O\n", source, nick))
		su = parse_uniform(source);
		source = 0;
#if 0
		if (is_localhost(su[UHost])) {
			object o;
			o = find_person(su[UUser]);
			// TODO: check that source is coming from
			// an authorized location!
			PT(("location %O, %s:%d\n", o -> qLocation("sip"),
			    ip, port))
			source = o;
		}
#endif
		unless(source) {
			source = SIP + su[UUserAtHost];
#if 0
			if (su[UPort] && su[UPort] != 5060) 
				source += ":" + su[UPort];
#endif
		}
	}
	P2(("sip cmd %O from %O to %O\n", cmd, source, target)) 
	switch(upper_case(cmd)) {
	case "OPTIONS": 
		reply = makeResponse(prot, 400, v);
		send_udp(ip, port, reply);
		return;
	case "MESSAGE": 
		if (islocaltarget) {
			if (data && data != "") 
				sendmsg(target, "_message_private", data, 
					([ "_nick" : objectp(source) ? source -> qName() : ""]), source);
			// TODO: should be done via _message_private_echo
			// 	somehow...
			if (pointerp(v["via"])) v["via"][0] += ";received=" + __HOST_IP_NUMBER__ + ":" + query_udp_port();
			reply = makeResponse(prot, 200, v); 
			send_udp(ip, port, reply);
		}
		return;
	case "SUBSCRIBE":
		// subscribe sip:... SIP/2.0
		// ein _request_friendship quasi
		// oder ein
		if (islocaltarget) {
			sendmsg(target, "_notice_presence_here", data, ([
				    "_INTERNAL_mood_jabber" : "neutral"
			       	]), source);
			if (pointerp(v["via"])) v["via"][0] += ";received=" + __HOST_IP_NUMBER__ + ":" + query_udp_port();
			reply = makeResponse(prot, 200, v);
			send_udp(ip, port, reply);
		}
		return;
	case "CANCEL":
	case "BYE": 
		if (islocaltarget) {
			// TODO: secure that
			string branch = ";branch=z9hG4bK" + RANDHEXSTRING;
			string ni, tgt, ho, po;
			mixed o;
			PT(("%O should be directed from %O to %O\n", 
			    cmd, v["from"], v["to"]))
			sscanf(v["to"], "%s<%s>%s", ni, tgt, ni);
			tu = parse_uniform(tgt);
			PT(("going to %O\n", tu))
			if(stringp(v["via"])) 
				v["via"] = ({ SIP_UDP " " SERVER_HOST ":4404" + branch, v["via"] });
			else if (pointerp(v["via"])) 
				v["via"] = ({ SIP_UDP " " SERVER_HOST ":4404" + branch }) + v["via"];
			// TODO
			reply = cmdline + CRLF +
				serialize( ({ "via", "to", "from", "call-id", "cseq", 
					    "max-forwards", "contact" }), v, data);
			send_udp_nonblocking("adamantine", 5060, reply);
			return;
		} else {
			PT(("local to remote bye/cancel\n"))
		}
		break;
	case "INVITE":
	case "ACK":
		// hier muessen wir zwei cases unterscheiden
		// a) ein remote/lokal user will einem unserer user was schicken
		// b) ein user von uns will einem remote user etwas
		//    schicken

		// Fall a:
		if (islocaltarget) {
			string loc;
			if (target && (loc = target -> qLocation("sip"))) {
				string branch;
				
				tu = parse_uniform(loc);
				PT(("should proxy call from %O to %O\n", source, loc))
				if (stringp(v["max-forwards"])) v["max-forwards"] = to_int(v["max-forwards"]);
				v["max-forwards"] -= 1;

				branch = ";branch=z9hG4bK" + RANDHEXSTRING;
				if(stringp(v["via"])) 
					v["via"] = ({ SIP_UDP " " SERVER_HOST ":4404" + branch, v["via"] });
				else if (pointerp(v["via"])) 
					v["via"] = ({ SIP_UDP " " SERVER_HOST ":4404" + branch}) + v["via"];
				// PSYC integration
#if 0
				if (cmd == "INVITE") 
					sendmsg(target, "_notice_location_sip_ring", 
						"[_source] is ringing your SIP phone at [_location]",
						([ "_source" : source, "_location" : loc ]), 
						"sip:" + source);
#endif
				reply = cmd + " " + loc + " " + prot + CRLF +
					serialize( ({ "via", "to", "from", "call-id", "cseq", 
						    "max-forwards", "contact" }),
						   v, data);
				send_udp_nonblocking(tu[UHost], to_int(tu[UPort]) || 5060, reply);
			}
		} else {
			// der Part ist wohl doch zu kompliziert
			// magic branch cookie as of RFC 3261, 8.1.1.7
			string branch = ";branch=" SIP_MAGIC_COOKIE;
			// Section 16.6
			// unless (su) ??
			if (su[UHost] != SERVER_HOST) return -1; // remote to remote??
			source = find_person(su[UUser]);
			PT(("local user %O is calling to %O\n", source, target))
			PT(("contact %O\n", v["contact"]))
			PT(("authorized? %O == %O\n", source -> qLocation("sip"), "sip:" + ip + ":" + port))

			// step 4
			if (stringp(v["max-forwards"])) v["max-forwards"] = to_int(v["max-forwards"]);
			else v["max-forwards"] = 10;
			v["max-forwards"] -= 1;

			// step 8
			branch += RANDHEXSTRING;
			if(stringp(v["via"])) v["via"] = ({ SIP_UDP " " SERVER_HOST ":4404" + branch, v["via"] });
			else if (pointerp(v["via"])) v["via"] = ({ SIP_UDP " " SERVER_HOST ":4404" + branch }) + v["via"];
			reply = cmd + " " + target + " " SIP_UDP CRLF +
				serialize( ({ "via", "to", "from", "call-id", "cseq", 
					      "max-forwards", "contact" }), v, data);
			// step 10
			PT(("data %O\nreply %O\n", data, reply))
			send_udp_srv(tu[UHost], "sip", "udp", reply); 
			return;
		}
		break;
	case "REGISTER":
		if (authstate) {
			int expires;
			string loc;
			mixed *cu;
			
			expires = to_int(v["expires"]);
			P2(("contact(s) %O expires in %d seconds\n", v["contact"], expires))
			// TODO
			loc = v["contact"];
			if (!v["contact"]) {
				// query for all contacts
			}
			sscanf(v["contact"], "%!.0s<%s>%!.0s", loc); 
			cu = parse_uniform(loc);
			if (cu) {
				loc = SIP + cu[UUserAtHost];
			}
			if (stringp(v["via"]))
				v["via"] += ";received=" + __HOST_IP_NUMBER__ + ":" + query_udp_port();
			else if (pointerp(v["via"]))
				v["via"][0] += ";received=" + __HOST_IP_NUMBER__ + ":" + query_udp_port();
			reply = makeResponse(prot, 200, v, "");
			if (expires!= 0) {
				reply += serialize(({ "contact" }), v);
			}
			send_udp(ip, port, reply);
			/* IF authstate == 1
			 * 	expire != 0: 
			 * 		set users sip-location to v["contact"]
			 * 		for expire seconds
			 * 	else delete users sip-location
			 */
			if (expires != 0) {
				// TODO: this gets annoying, 
				// should only be shown if changed
				// TODO: how do we deal with expires?
				if (authuser -> sLocation("sip", loc))
					sendmsg(authuser, "_notice_location_sip", 
	"Your SIP Phone ([_version_agent]) has registered at "
	"[_location_sip] (valid for [_duration] seconds).",
					([ "_location_sip" : loc, 
					   "_version_agent" : v["user-agent"],
					   "_duration" : v["expires"] ]), authuser);
			} else {
				if (authuser -> sLocation("sip", 0))
					sendmsg(authuser, "_notice_location_sip_signoff",
	"Your SIP Phone has signed off from [_location_sip].",
					([ "_location_sip" : v["contact"] ]), authuser);
				}
		} else if (v["authorization"]) {
			// authorization failed
			reply = makeResponse(prot, 403, v, "");
//			reply += "Content-Length: 0" CRLF CRLF;
			send_udp(ip, port, reply);
		} else {
			//v["to"] += ";tag=1234567890abcde"; // ??? TODO
			/* need authorization
			 */
			v["www-authenticate"] = "Digest realm=\"" + SERVER_UNIFORM + "\", nonce=\"" + time() + "\"";
			reply = makeResponse(prot, 401, v, "");
			// TODO: nonce generation has to be secure
			send_udp(ip, port, reply);
		}
		return;
	default:
		reply = makeResponse(prot, 400, v, "");
		send_udp(ip, port, reply);
		return;
	}
	// RFC 3261, figure 1
	//
	// [...] This response contains the same To, From, Call-ID, CSeq and
	// 	branch parameter in the via as the invite
	reply = makeResponse(prot, 100, v); 
	reply += "Content-Length: 0" CRLF CRLF;
	send_udp(ip, port, reply);
	return;
}

handleResponse(prot, statuscode, cmdline, v, data) {
	/* status message, send to topmost via-header 
	 */
	string target, host, tag; // tag == branch???
	string reply;
	int port;

	PT(("via %O\n", v["via"]))
	if (stringp(v["via"])) {
		// TODO response is addressed to us
		PT(("local response code %O\n", statuscode))
		return;
	}
	sscanf(v["via"][1], "%s%t%s", prot, target);
	sscanf(target, "%s;%s", target, tag);
	sscanf(target, "%s:%d", host, port);
	v["via"] = v["via"][1..];
	unless(host) host = target;
	unless(port) port = 5060;
	PT(("sending status %O to %s:%d\n", statuscode, host, port))
	reply = cmdline + CRLF +
		serialize( ({ "via", "to", "from", "call-id", "cseq",
			    "contact" }), v, data);
	send_udp_nonblocking(host, port, reply);
	return;
}

msg(source, mc, data, mapping vars, showingLog, target) {
	mapping v;
	string packet;
	PT(("should send %O from %O to %O\nvars %O\n", mc, source, target, vars))
	// note: stpeters xmpp2sip work may be - though outdated - useful 
#if 0
	v = ([ "content-type" : vars["_type_data"] || "text/plain" ]);
	packet = "";
	switch(mc) {
	case "_message_private":
		v["via"] = SIP_UDP " " SERVER_HOST ";branch=z9hG4bK" 
			+ RANDHEXSTRING;
		v["cseq"] = random(__INT_MAX__) + " MESSAGE";
		v["to"] = target;
		v["from"] = source;
		v["call-id"] = RANDHEXSTRING + "@" SERVER_HOST;
		packet = "MESSAGE " + target + " SIP/2.0" CRLF +
			serialize(({ "to", "from", "via", "call-id", "cseq" }),
				  v, data);
		PT(("packet %O\n", packet))
		break;
	}
#endif
	return;
}
#endif
