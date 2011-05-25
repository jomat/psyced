// $Id: gateway.c,v 1.461 2008/10/22 16:35:59 fippo Exp $ // vim:syntax=lpc
/*
 * jabber/gateway 
 * listens on jabber interserver port for incoming connections
 *
 */
#define NO_INHERIT
#include "jabber.h"	// inherits net/jabber/common
#undef NO_INHERIT
#include "uniform.h"
#include "services.h"

#define NO_INHERIT
#include "server.h"	// does not inherit net/server
#undef NO_INHERIT

//virtual inherit NET_PATH "output";
inherit NET_PATH "trust";

inherit NET_PATH "jabber/mixin_parse";
#ifdef XMPP_BIDI
inherit NET_PATH "jabber/mixin_render";
#endif

//inherit NET_PATH "storage";
volatile object active;	// currently unused, but you could send w() to it

#include "interserver.c" // interserver stuff

/* major pre-mammas-birthday-todo: 
 * replace usage of sendmsg() by usage of tell/placerequest
 * sendmsg should not be used directly!
 */

/* some notes about _error_unknown_name_user:
 * those should probably be _error_unknown_name if source of the error
 * does not have a node identifier (for example a bare server jid)
 */
volatile string host;	// about time to remember which host we are talking to

volatile string tag;
volatile string streamid;
volatile string streamfrom;
volatile string resource;
volatile mapping certinfo;


v(a, b) {
    PT(("%O v(%O, %O)\n", ME, a, b))
    return "";
}

load(ho, po) { host = ho; return ME; }

// this is not called by driver. this is called by call_out.
quit() { 
        flags |= TCP_PENDING_DISCONNECT;
        remove_interactive(ME);
}

disconnected(remainder) { 
        // TODO: handle remainder
        P2(( "gateway %O disconnected\n", ME ))
	// sometimes we get complete presence packets in the socket close
	// remainder. probably broken xmpp implementations, let's try and
	// do the best we can with it by forwarding stuff to feed().
	if (remainder && strlen(remainder)) feed(remainder);
        if (objectp(active)) active -> removeGateway(streamid);
#ifdef _flag_log_sockets_XMPP
        log_file("RAW_XMPP", "\n%O disc\t%O", ME, ctime());
#endif
        destruct(ME);
        // expected or unexpected disconnect? flags shall tell
        return flags & TCP_PENDING_DISCONNECT;
}

// this is called by active if it was created only for the 
// purpose of doing dialback and the attempt failed
remote_connection_failed() {
    STREAM_ERROR("remote-connection-failed", "")
    QUIT
}

qScheme() { return "jabberserver"; }

void create() {
    unless (clonep()) return;
    // not multilingual yet - TODO
    sTextPath(0, "en", "jabber");
    // we could switch them into variables anytime
    //	cmdchar = '/'; // to remain in style with /me
    //	actchar = ':'; // let's try some mudlike emote for jabber
    ::create();
}

#ifdef WANT_S2S_TLS
// similar code in other files
tls_logon(result) { 
    if (result == 0) {
	certinfo = tls_certificate(ME, 0);
	P3(("%O tls_logon fetching certificate: %O\n", ME, certinfo))
# ifdef ERR_TLS_NOT_DETECTED
    } else if (result == ERR_TLS_NOT_DETECTED) { 
	// just go on without encryption
# endif
    } else if (result < 0) { 
	P1(("%O TLS error %d: %O\n", ME, result, tls_error(result)))
	QUIT 
    } else {
	P0(("tls_logon with result > 0?!?!\n"))
	// should not happen
    }
}
#endif

logon(a) {
    P4(("logon(%O) beim jabber:gateway\n", a))
    call_out(#'quit, TIME_LOGIN_IDLE);
    set_combine_charset(COMBINE_CHARSET);
#ifdef INPUT_NO_TELNET
    input_to(#'feed, INPUT_IGNORE_BANG | INPUT_CHARMODE | INPUT_NO_TELNET);
#else
    enable_telnet(0, ME);
    input_to(#'feed, INPUT_IGNORE_BANG | INPUT_CHARMODE);
#endif
#ifdef _flag_log_sockets_XMPP
    log_file("RAW_XMPP", "\n%O logon\t%O", ME, ctime());
#endif
    // return ::logon(a);
}

verify_connection(string to, string from, string type) {
    P2(("verify connection from %s to %s type %s\n", to, from, type))
    /* 10. Receiving server informs originating server of the result */
    emit(sprintf("<db:result from='%s' to='%s' type='%s'/>",
		 to, from, type));
    if (type != "valid") {
	emitraw("</stream:stream>");
	P2(("quitting invalid stream\n"))
	QUIT
    } else {
#ifdef LOG_XMPP_AUTH
	D0( log_file("XMPP_AUTH", "\n%O has authenticated %O via dialback", ME, from); )
#endif 
	sAuthenticated(from);
#ifdef XMPP_BIDI
	if (bidi) {
		P0(("doing register target for xmpp bidi!!!!\n"))
		register_target(XMPP + from);
	}
#endif
	while (remove_call_out(#'quit) != -1);
    }
}

#if 0
volatile mapping proper_addressing = ([
	"stream:error" : 1,
	"auth" : 1,
	"response" : 1,
#ifdef SWITCH2PSYC
	"switching" : 1,
#endif
	"starttls" : 1
]);
#endif

jabberMsg(XMLNode node) {
    mixed *su, *tu;
    string source, target;
    object o;
    mixed t, t2;

    target = node["@to"];
    source = node["@from"];
    origin = XMPP + source;

    /* TODO: we should reset origin when we're done here
     */

#ifdef XMPPERIMENTAL
    /* note for world domination:
     * 	if !target target is the cslave
     * 	we need to reflect this somehow in sendmsg...
     * 	or we can handle it as a special case everywhere
     */
    if (node[Tag] == "presence" && !target) {
	string mc;
	su = parse_uniform(origin);
	unless(su) return 0;
	o = find_context(su[UUserAtHost]);
	PT(("find_context(%O) -> %O\n", su[UUserAtHost], o))
	unless (o) return 1;
	if (node["@type"] == "probe") {
	    mc = "_request_status_person";
	    data = "";
	} else {
	    mc = "_notice_presence_here"; // does not handle away, etc
	    data = "[_nick] is available.";
	    //vars["_INTERNAL_mood_jabber"] = "neutral";
	}

	o -> castmsg(origin, mc, data, ([ "_nick" : su[UUser] ]));
	return 1;
    }
#endif
    /* error conditions as in XMPP CORE 4.7.3 first */
// todo.. use proper_addressing but this combination of && and || is not
// very nice.. you can't expect people to know which operator is stronger
    if (! (source && target 
		|| node[Tag] == "stream:error"
		|| node[Tag] == "auth"
#ifdef SWITCH2PSYC
		|| node[Tag] == "switching"
#endif
#ifdef XMPP_BIDI
		|| node[Tag] == "bidi"
#endif
		|| node[Tag] == "starttls") ) {
	// apparently we are the only jabber server to complain about it
	if (node[Tag] == "stream:features") {
	    P3(("(warn) %O got buggy stream:features: %O\n", ME, node))
	    return;
	}
	monitor_report("_error_syntax_missing_element_jabber",
	    S("%O got jabber %O type %O from %O to %O", ME, node[Tag],
	      node["@type"], source, target));
	PT(("node %O\n", node))
	STREAM_ERROR("improper-addressing", "")
	remove_interactive(ME);
	return;
    }
    P2(("%O jabberMsg(%O from %O to %O)\n", ME, node[Tag], source, target))
    /* handling for stream features, dialback, etc pp */
    switch (node[Tag]) {
#ifdef SWITCH2PSYC
    case "switching":
	// tested manually using:

	// <?xml version='1.0'?><stream:stream xmlns:stream='http://etherx.jabber.org/streams' xmlns='jabber:server' to='localhost' xmlns:db='jabber:server:dialback' version='1.0'><switching xmlns='http://switch.psyced.org'><scheme>psyc</scheme></switching>

	emitraw("<switched xmlns='http://switch.psyced.org'/>");
	PT(("received 'switching'. authhosts %O\n", authhosts))
	o = ("S:psyc:" + host) -> load();
	P1(("%O switching to %O for %O\n", ME, o, host))
	exec(o, ME);
	o -> logon();
	o -> sAuthHosts(authhosts);
	destruct(ME);
	return;
#endif
#ifdef XMPP_BIDI
    case "bidi":
	if (node["@xmlns"] == "urn:xmpp:bidi") {
		//emit("<success xmlns='urn:xmpp:bidi'/>");
		bidi = 1;
	}
	return;
#endif
    case "db:result":
	target = NAMEPREP(target);
	source = NAMEPREP(source);
	origin = XMPP + source;
	/* this is receiving step 4
	 * what we do now is 
	 * 8. receiving server sends authorative server
	 *    a request for verification of a key
	 */
	// if we dont know the host, complain
	// put NAMEPREP(_host_XMPP) into the localhost mapping pleeeease
	//if (target != NAMEPREP(_host_XMPP)) {	
	unless (is_localhost(lower_case(target))) {
	    monitor_report("_error_unknown_host", 
	       sprintf("%O sent us a dialback packet believing we would be %O",
		       source, target));
	    emit(sprintf("<db:result from='%s' to='%s' type='error'>"
			 "<error type='cancel'>"
			 "<item-not-found xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'/>"
			 "</error>"
			 "</db:result>",
			 target, source));
	    /* no more...
	    STREAM_ERROR("host-unknown", "")
	    remove_interactive(ME);
	    */
	    return;
	}
	// dialback without dial-back - if the certificate is valid and the sender 
	// is contained in the subject take the shortcut and consider the request
	// valid
	// paranoia note: as with XEP 0178 we might want to check dns anyway to
	// 	protect against stolen certificates
	if (mappingp(certinfo) && certinfo[0] == 0 
	    && node["@from"] && certificate_check_jabbername(node["@from"], certinfo)) {
		P0(("dialback without dialback %O\n", certinfo))
		verify_connection(node["@to"], node["@from"], "valid"); 
	} else {
		sendmsg(origin,
			"_dialback_request_verify", 0,
			([ "_INTERNAL_target_jabber" : source,
			   "_INTERNAL_source_jabber" : NAMEPREP(_host_XMPP),
			   "_dialback_key" : node[Cdata],
			   "_tag" : streamid
			   ])
			);
		unless (o = find_target_handler(NAMEPREP(origin))) {
		    // sendmsg should have created it!
		    P0(("%O could not find target handler for %O "
			"after sendmsg\n", ME, origin))
		    return;
		}
		active = o -> sGateway(ME, target, streamid);
	}
	return;
    case "db:verify":
	target = NAMEPREP(target);
	source = NAMEPREP(source);
	// pretty much everywhere else, where XMPP is prepended, either
	// the UString of su[] or tu[] should have been used..
	origin = XMPP + source;
	/* check to and from */
	/* verify dialback key */
	o = find_target_handler(origin);
	unless (o) {
	    mixed *u = parse_uniform(origin);
	    // probably this has been destructed due to gone wrong
	    P0(("ERROR: could not find_target_handler for %O in db:verify\n",
		origin))
	    // fatal error, we have not called that host
	    // probably thats what invalid-from is for
#ifdef QUEUE_WITH_SCHEME
	    o = ("C:"+origin)-> circuit(u[UHost], u[UPort]
		 || JABBER_S2S_SERVICE, 0, "xmpp-server", origin);
#else
	    o = ("C:"+origin)-> circuit(u[UHost], u[UPort]
		 || JABBER_S2S_SERVICE, 0, "xmpp-server", u[UHostPort]);
#endif
	    register_target(origin, o);
	}
	// TODO: das hier gehoert noch eins weiter unten rein
	// oder einfach ein stueckl tiefer
	active = o -> sGateway(ME, target, streamid);
	unless (node["@type"]){
	    int valid;
	    /* we were calling this server, this packet is step 8
	     * and we are doing step 9
	     */
	    /* if we dont recognize target (currently: == _host_XMPP)
	     * then croak with a host-unknown and commit suicide
	     */
	    // same as above...
	    unless (is_localhost(lower_case(target))) {
		emit(sprintf("<db:verify from='%s' to='%s' id='%s' type='error'>"
			     "<error type='cancel'>"
			     "<item-not-found xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'/>"
			     "</error>"
			     "</db:result>",
			     target, source, node["@id"]));
		/*
		STREAM_ERROR("host-unknown", "")
		QUIT
		*/
	    }
	    valid = node[Cdata] == DIALBACK_KEY(node["@id"], source, 
						target);
	    emit(sprintf("<db:verify from='%s' to='%s' type='%s' id='%s'/>",
			 target, source, 
			 valid ? "valid" : "invalid", 
			 node["@id"]));
	} else {
	    P0(("received db:verify without type on an incoming connection\n"))
	}
	return;
    case "starttls":
#ifdef WANT_S2S_TLS
	if (tls_available()) {
	    emitraw("<proceed xmlns='" NS_XMPP "xmpp-tls'/>");
# if __EFUN_DEFINED__(tls_want_peer_certificate)
	    tls_want_peer_certificate(ME);
# endif
	    tls_init_connection(ME, #'tls_logon);
	    return;
	} 
#endif
	emitraw("<failure xmlns='" NS_XMPP "xmpp-tls'/>");
	return;
    case "stream:error":
	if (node["/connection-timeout"]) {
	    /* ignore it */
	} else if (node["/system-shutdown"]) {
	    P1(("%O: counterpart is doing a system shutdown\n", ME))
	    /* ignore it */
	} else {
	    P0(("stream error in %O: %O\n", ME, node))
	}
	return;
#ifdef WANT_S2S_SASL
    case "auth": 
	// if the authorization id is present, use that, else use streamfrom
	// note that the standard says that streamfrom MUST be the same as the authorization id
	// so we could save the base64 stuff and use streamfrom in all cases if we ignore the 
	// standard
	t = node[Cdata] ? to_string(decode_base64(node[Cdata])) : streamfrom; 
	switch (node["@mechanism"]) {
	case "EXTERNAL":
	    if (tls_query_connection_state(ME) == 1
		    && mappingp(certinfo)
		    && certinfo[0] == 0) {
		/*
		 * order of checks should be first against
		 * id-on-xmppAddr field and only if that is
		 * unused match against common name
		 */
		int success = 0;

		success = certificate_check_jabbername(t, certinfo);
		if (success) {
		    emitraw("<success xmlns='" NS_XMPP "xmpp-sasl'/>");
		    P2(("successful sasl external authentication with "
			"%O\n", t))
		    sAuthenticated(t);
		    while (remove_call_out(#'quit) != -1);
#ifdef XMPP_BIDI
		    if (bidi) {
			    P0(("doing register target for xmpp bidi!!!!\n"))
			    register_target(XMPP + t);
		    }
#endif
# ifdef LOG_XMPP_AUTH
		    D0( log_file("XMPP_AUTH", "\n%O has authenticated %O via SASL external", ME, t); )
# endif 
		} else {
		    SASL_ERROR("not-authorized")
		    QUIT
		}
	    } else {
		// we did not offer it definetly
		SASL_ERROR("invalid-mechanism")
		QUIT
	    }
	    break;
	default:
	    SASL_ERROR("invalid-mechanism")
	    QUIT
	    break;
	}
	return;
#endif
    }
    su = parse_uniform(origin);
    unless (su) {
	// this is not jid-malformed, rather invalid-from
	P0(("%O: source jid malformed? %O\n", ME, source))
	STREAM_ERROR("invalid-from", "I could not parse that from");
	QUIT	// too hard?
    }
    /* security policy 
     * DROP unless authenticated
     * see XMPP-Core §4.3
     */
    unless (qAuthenticated(su[UHost])) {
	PT(("%O dropping %O: su[UHost] %O\n", 
	    ME, node[Tag], su[UHost]))
	STREAM_ERROR("invalid-from", "")
	QUIT	// too hard?
    }
    tu = parse_uniform(XMPP + target);
    unless (tu) {
	// we should croak with an jid-malformed error and commit suicide
	// jabberd responds with a invalid-from, but that doesnt seem 
	// to be the right semantic
	// but... well, for now
	P0(("gateway: target jid malformed? %O in %O\n", target, ME))
	STREAM_ERROR("invalid-from", "could not parse that to");
	QUIT	// too hard?
    }
    return ::jabberMsg(node, origin, su, tu);
}

open_stream(XMLNode node) {
    string packet;
    float version;

    // make a loooong random string and hash it not to expose our random numbers
    streamid = sha1(RANDHEXSTRING + RANDHEXSTRING + RANDHEXSTRING + RANDHEXSTRING);
    version = to_float(node["@version"]);
    packet = sprintf("<?xml version='1.0' encoding='UTF-8' ?>"
		     "<stream:stream "
		     "xmlns='%s' "
		     "xmlns:db='jabber:server:dialback' "
		     "xmlns:stream='http://etherx.jabber.org/streams' "
		     "xml:lang='en' id='%s' ", node["@xmlns"], streamid);
    if (node["@to"]) {
	packet += "from='" + node["@to"] + "' ";
    } else {
	packet += "from='" _host_XMPP "' ";
    }
    if (node["@from"]) {
	packet += "to='" + node["@from"] + "' ";
    }
    if (node["@to"] && !(is_localhost(lower_case(node["@to"])))) {
	emit(packet + ">");
	STREAM_ERROR("host-unknown", "")
	QUIT
	return;
    }
    if (node["@xmlns"] != "jabber:server") {
    /* If the namespace name is incorrect, then Receiving Server 
     * MUST generate an <invalid-namespace/> stream error condition 
     * and terminate both the XML stream and the underlying TCP connection.
     */
	emit(packet + ">");
	STREAM_ERROR("invalid-namespace", "")
	QUIT
	return;
    }
    streamfrom = node["@from"];

    /* if stream version is >= "1.0" reply with stream version 
     * attribute and add a stream:feature tag
     */
    if (version >= 1) {
	packet += "version='1.0'>";
	emit(packet);
	// this sends one stanza per tcp packet
	packet = "<stream:features>";
#ifdef WANT_S2S_TLS
	if (tls_available()) {
	    if (tls_query_connection_state(ME) == 0) {
		packet += "<starttls xmlns='" NS_XMPP "xmpp-tls'/>";
	    } else unless (mappingp(authhosts)) {
# ifdef WANT_S2S_SASL
		// if the other side did present a client certificate
		// and we have verified it as X509_V_OK (0)
		// we offer SASL external (authentication via name
		// presented in x509 certificate
		PT(("gateway::certinfo %O\n", certinfo))
#  ifndef DIALBACK_WITHOUT_DIAL_BACK
		if (mappingp(certinfo) && certinfo[0] == 0) {
		    // if from attribute is present we only offer
		    // sasl external if we know that it will succeed
		    // later on
		    if (node["@from"] &&
			    certificate_check_jabbername(node["@from"],
						     certinfo)) {
			packet += "<mechanisms xmlns='" NS_XMPP "xmpp-sasl'>";
			packet += "<mechanism>EXTERNAL</mechanism>";
			packet += "</mechanisms>";
		    }
		}
#  endif
# endif
	    }
	}
#endif
#ifdef SWITCH2PSYC
	packet +=   "<switch xmlns='http://switch.psyced.org'>"
			"<scheme>psyc</scheme>"
		    "</switch>";
#endif
#ifdef XMPP_BIDI
	packet += "<bidi xmlns='urn:xmpp:features:bidi'/>";
#endif
	packet += "<dialback xmlns='urn:xmpp:features:dialback'><errors/></dialback>"; 
	packet += "</stream:features>";
    } else {
	packet += ">";
    }
    emit(packet);
}

/* watch out, this is used by the user objects cmd() parser!
 * and that must not happen as we are using unidirectional sockets
 * this is not really a bug, we just dont handle those yet...
 * the strategy will be to do a sendmsg() to origin
 */
w(string mc, string data, mapping vars, mixed source) {
    P2(("%O using w() for %O, unimplemented... mc %O, source %O\n", 
	ME, origin, mc, source))
    unless (vars) vars = ([ ]);
    vars["_INTERNAL_source_jabber"] = objectp(source) ? mkjid(source) : _host_XMPP;
    sendmsg(origin, mc, data, vars); 
}

#ifdef XMPP_BIDI
int msg(string source, string mc, string data,
	    mapping vars, int showingLog, string target) {
	// copy+paste stuff from active.c
    vars = copy(vars);
#ifdef DEFLANG
    unless(vars["_language"]) vars["_language"] = DEFLANG;
#else
    unless(vars["_language"]) vars["_language"] = "en";
#endif
    /* another note:
     * instead of queuing the msg()-calls we could simply queue
     * the output from emit (use the new net/outputb ?)
     * this avoids bugs with destructed objects
     */
#if 0 // !EXPERIMENTAL
    /* currently, we want _status_person_absent
     * this may change...
     */
    else if (abbrev("_status_person_absent", mc)) return 1;
#endif
    switch (mc){
    case "_message_echo_private":
	return 1;
    }
    // desperate hack
    unless (vars["_INTERNAL_mood_jabber"])
	    vars["_INTERNAL_mood_jabber"] = "neutral";
    if (abbrev("_dialback", mc)) {
	P0(("gateway got dialback method %O, this should not happen...\n", mc))
	return 1;
    }

    determine_sourcejid(source, vars);
    determine_targetjid(target, vars);
    if (vars["_place"]) vars["_place"] = mkjid(vars["_place"]);
    // end of copy+paste from active
    return ::msg(source, mc, data, vars, showingLog, target);
}
#endif
