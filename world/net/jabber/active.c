// $Id: active.c,v 1.404 2008/10/26 17:24:57 lynx Exp $ // vim:syntax=lpc:ts=8

// a jabber thing which actively connects something
#define NO_INHERIT
#include "jabber.h"
#undef NO_INHERIT
#include <url.h>

#ifdef ERQ_WITHOUT_SRV
# define hostname host	// hostname contains the name before SRV resolution
#endif

inherit NET_PATH "jabber/mixin_render";

// a derivate of circuit which knows JABBER
inherit NET_PATH "circuit";
inherit NET_PATH "name";

#if 0	// apparently unused
// virtual inherit of textc.c thus output.c isnt really working
#define NO_INHERIT
#include <text.h>
#undef NO_INHERIT
#endif

#include "interserver.c" // interserver stuff

volatile mixed gateways;
volatile mixed *dialback_queue;

volatile string streamid;
volatile float streamversion;
volatile int authenticated;
volatile int ready; // finished with sasl, starttls and such
volatile int dialback_outgoing;

tls_logon(); // prototype

// not strictly necessary, but more spec conformant
quit() {
	emitraw("</stream:stream>");
#ifdef _flag_log_sockets_XMPP
	D0( log_file("RAW_XMPP", "\n%O: shutting down, quit called\n", ME); )
#endif
	remove_interactive(ME);	// not XEP-0190 conformant, but shouldn't
				// matter on an outgoing-only socket
	//destruct(ME);
}

sGateway(gw, ho, id) {
    // TODO: ho is obsolete
    P2(("%O: setting %O as gateway for %O, id %O\n", ME, gw, ho, id))
    unless (gateways) gateways = ([ ]);
    gateways[id] = gw; 
    return ME; // set myself as active for this gateway
}

removeGateway(gw, id) {
    if (gateways[id] == gw) {
	m_delete(gateways, id);
    }
}

start_dialback() {
    string source_host, key;

    source_host = NAMEPREP(_host_XMPP);
    key = DIALBACK_KEY(streamid, hostname, source_host);

    P3(("%O: starting dialback from %O to %O\n", ME, source_host, hostname))
    
    dialback_outgoing = 1;
    emit(sprintf("<db:result to='%s' from='%s'>%s</db:result>",
		 hostname, source_host, key));
}

process_dialback_queue() {
    mixed *t;
    ready = 1;
    if (pointerp(dialback_queue)) 
	foreach (t : dialback_queue) 
	    render(t[1], t[2], t[3], t[0]);
    dialback_queue = ({ });
}

handle_starttls(XMLNode node) {
    if (node["@xmlns"] != NS_XMPP "xmpp-tls") {
	P0(("%O: expecting xmpp-tls proceed or failure, got %O:%O\n",
	    ME, node[Tag], node["@xmlns"]))
#ifdef _flag_log_sockets_XMPP
	D0( log_file("RAW_XMPP", "\n%O: expecting xmpp-tls proceed or failure, got %O:%O\t%O", ME, node[Tag], node["@xmlns"], ctime()); )
#endif
	nodeHandler = #'jabberMsg;
	return jabberMsg(node);
    }
    nodeHandler = #'jabberMsg;
    if (node[Tag] == "proceed") {
	tls_init_connection(ME, #'tls_logon);
    } else { // treat everything else as a failure
	P0(("%O got a tls failure\n", ME))
    }
}

handle_stream_features(XMLNode node) {
    unless (node[Tag] == "stream:features") {
	// this should not happen!
	P0(("%O: expecting stream:features, got %O\n", ME, node[Tag]))
	nodeHandler = #'jabberMsg;
	// TODO
	// be careful, usually there are not many nodes which an active 
	// may get, mostly errors, so it might not be a good idea
	// to do dialback if this happens

	start_dialback(); 
	process_dialback_queue();
#ifdef _flag_log_sockets_XMPP
	D0( log_file("RAW_XMPP", "\nprocessing dialback queue, expecting stream features but not found" ); ) 
#endif
	return jabberMsg(node);
    }
    unless (streamversion >= 1.0) {
	P0(("%O unexpected stream:features with stream:version %O\n",
	    ME, streamversion))
	return;
    }
    nodeHandler = #'jabberMsg;
#ifdef WANT_S2S_TLS
    // should only happen if stream version is >= "1.0"
    if (node["/starttls"] && tls_available() 
	    && !tls_query_connection_state(ME)
	    // dont starttls to hosts that are known to have 
	    // invalid tls certificates
//		&& !config(XMPP + hostname, "_tls_invalid")
       ){
	// may use tls unless we already do so
	emitraw("<starttls xmlns='urn:ietf:params:xml:ns:xmpp-tls'/>");
	nodeHandler = #'handle_starttls;
	return;
    }
#else
    if (node["/starttls"] 
	    && node["/starttls"]["/required"]) {
	P0(("%O requires <starttls/> but we can't provide that\n", ME))
	connect_failure("_encrypt_necessary", "Encryption required but unable to provide that");
	return;
    }
#endif
#ifdef WANT_S2S_SASL
    // possibly authenticated via sasl?
    if (authenticated && !node["/switch"]) {
	P2(("%O running Q after getting stream:features and having "
	    "authenticated via sasl\n", ME))
	process_dialback_queue();
	return runQ();
    }
    if (node["/mechanisms"] 
	    && node["/mechanisms"]["/mechanism"]){
	XMLNode tx;
	mixed mechs = ([ ]);

	// get the mechanismsm
	tx = node["/mechanisms"]["/mechanism"];

	if (nodelistp(tx)) 
	    foreach (XMLNode tx2 : tx) 
		mechs[tx2[Cdata]] = 1;
	else 
	    mechs[tx[Cdata]] = 1;

	// choose a mechanism
	P2(("available SASL mechanisms: %O\n", mechs))

#ifndef _flag_disable_authentication_external_XMPP
	if (mechs["EXTERNAL"]) {
	    // TODO we should check that the name in our 
	    // certificate is equal to _host_XMPP
	    // but so should the other side!
	    emit("<auth mechanism='EXTERNAL' "
		 "xmlns='" NS_XMPP "xmpp-sasl'>" +
		 encode_base64(_host_XMPP)
		 + "</auth>");
	    return;
	} else
#endif
	if (mechs["DIGEST-MD5"] 
		   && config(XMPP + hostname, "_secret_shared")) { 
	    PT(("jabber/active requesting to do digest md5\n"))
	    emit("<auth mechanism='DIGEST-MD5' "
		 "xmlns='" NS_XMPP "xmpp-sasl>" +
		 encode_base64(_host_XMPP) +
		 "</auth>");
	    return;

	}
    }
#endif
#ifdef SWITCH2PSYC
    else if (node["/switch"]) {	// should check scheme
	PT(("upgrading %O from XMPP to PSYC.\n", ME))
	emitraw("<switching xmlns='http://switch.psyced.org'>"
		"<scheme>psyc</scheme>"
	     "</switching>");
	return;
    }
#endif
    // send dialback request (step 4) here if nothing else
    // is done
    if (dialback_outgoing == 0 && qSize(me)) {
	start_dialback();
    }
    process_dialback_queue();
}

disconnected(remainder) {
    P2(("active %O disconnected\n", ME))
#ifdef _flag_log_sockets_XMPP
    D0( log_file("RAW_XMPP", "\n%O disc\t%O", ME, ctime()); )
#endif
    authenticated = 0;
    ready = 0;
    if (dialback_outgoing != 0) {
	// just saw that happen twice with jabber.org
	// should we trigger a reconnect then?
	P0(("%O got disconnected with dialback outgoing != 0\n", ME))
	// TODO: reconnect koennte sinnvoll sein
#ifdef _flag_log_sockets_XMPP
	D0( log_file("RAW_XMPP", "\n%O disconnected with dialback outgoing != 0\t%O", ME, ctime()); )
#endif
	dialback_outgoing = 0;
    }
    // nothing else happening here? no reconnect?
    // TODO: what about the dialback Q if any?
    ::disconnected(remainder);
    // let's call this a special case of good will:
    // hopefully a sending side socket close operation
    if (remainder == "</stream:stream>") return 1;
    // we could forward remainder to feed(), but we haven't seen any other
    // cases of content than the one above.
    return flags & TCP_PENDING_DISCONNECT;
}

// we love multiple inheritance.. rock'n'roll!
static int logon(int failure) {
    // TODO: it seems that it is bad to call this from tls_logon
    // 	 for a second time
// 1. Originating Server establishes TCP connection to Receiving Server.
    if (NET_PATH "circuit"::logon(failure)) {
	set_combine_charset(COMBINE_CHARSET);
#ifdef INPUT_NO_TELNET
	input_to(#'feed, INPUT_IGNORE_BANG | INPUT_CHARMODE | INPUT_NO_TELNET);
#else
	enable_telnet(0, ME);
	input_to(#'feed, INPUT_IGNORE_BANG | INPUT_CHARMODE);
#endif
	sTextPath(0, "en", "jabber");
#ifdef _flag_log_sockets_XMPP
	D0( log_file("RAW_XMPP", "\n%O logon\t%O", ME, ctime()); )
#endif
/* 2. Originating Server sends a stream header to Receiving Server */
	emit("<stream:stream xmlns:stream='http://etherx.jabber.org/streams' "
	     "xmlns='jabber:server' xmlns:db='jabber:server:dialback' "
	     "to='" + hostname + "' "
	     "from='" + NAMEPREP(_host_XMPP) + "' "
	     "xml:lang='en' "
	     "version='1.0'>");
#if 1 // not strictly necessary, but more spec conformant
    } else if (!qSize(me)) { // no retry for dialback-only
	//if (sizeof(dialback_queue) > 1) {
	    P2(("%O failed to connect. dialback queue is %O\n",
	       	ME, dialback_queue))
	//}
	if (sizeof(gateways)) {
	    foreach(string id, mixed gw : gateways) {
		if (objectp(gw)) {
		    P2(("%O notifies %O of failure (%O).\n", ME, gw, id))
		    gw->remote_connection_failed();
		}
	    }
	}
	dialback_queue = 0;
	destruct(ME);
#endif
    }
    unless (gateways) {
	gateways = ([ ]);
    }
    return 1;
}

#ifdef WANT_S2S_TLS
tls_logon(result) {
    if (result < 0) {
	P1(("%O tls_logon %d: %O\n", ME, result, tls_error(result) ))
	// would be nice to insert the tls_error() message here.. TODO
	connect_failure("_encrypt", "Problems setting up an encrypted circuit");
    } else if (result == 0) {
	// we need to check the certificate
	// selfsigned certs are ok, but we need dialback then
	//
	// if the cert is ok, we can set authenticated to 1
	// to skip dialback
	mixed cert = tls_certificate(ME, 0);
	P3(("active::certinfo %O\n", cert))
	if (mappingp(cert)) {
	    unless (certificate_check_jabbername(hostname, cert)) {
#ifdef _flag_report_bogus_certificates
		monitor_report("_error_invalid_certificate_identity",
			       sprintf("%O presented a certificate that "
				       "contains %O/%O",
				       hostname, cert["2.5.4.3"],
				       cert["2.5.29.17:1.3.6.1.5.5.7.8.5"]));
#endif
#ifdef _flag_log_bogus_certificates
		log_file("CERTS", S("%O %O %O id?\n", ME, hostname, cert));
#else
		P1(("TLS: %s presented a certificate with unexpected identity.\n", hostname))
		P2(("%O\n", cert))
#endif
#if 0 //def _flag_reject_bogus_certificates
		QUIT
		return 1;
#endif
	    } 
	    else if (cert[0] != 0) {
#ifdef _flag_report_bogus_certificates
		monitor_report("_error_untrusted_certificate",
			       sprintf("%O certificate could not be verified",
				       hostname));
#endif
#ifdef _flag_log_bogus_certificates
		log_file("CERTS", S("%O %O %O\n", ME, hostname, cert));
#else
		P1(("TLS: %s presented untrusted certificate.\n", hostname))
		P2(("%O\n", cert))
#endif
#if 0 //def _flag_reject_bogus_certificates
		// QUIT is wrong...
		QUIT
		return 1;
#endif
	    }
	} else {
	    P0(("%O: no tls_certificate() for %O\n", ME, hostname))
	}
	logon(0);
    } else {
	// should not happen, if it does the driver is doing sth wrong
	PT(("%O tls_logon %d - MUST NOT HAPPEN\n", ME, result))
    }
}
#endif

// this one is completly specific to active.c
jabberMsg(XMLNode node) {
    /* this will be rarely used if we do dialback
     * due to active part beeing virtually write-only
     */
    P2(("%O jabber/active got %O\n", ME, node[Tag])) 
    object o;
    string t;
    switch (node[Tag]) {
    case "db:result":
	/* 10: Receiving Server informs Originating Server of the result
	 * we are originating server and are informed of the result
	 */
	dialback_outgoing = 0;
	if (node["@type"] == "valid") {
#ifdef LOG_XMPP_AUTH
	    D0( log_file("XMPP_AUTH", "\n%O auth dialback", ME); )
#endif 
	    authenticated = 1;
	    runQ();
	} else {
	    // else: queue failed
	    P0(("%O something gone wrong in active db:result: %O\n", ME, node))
	    authenticated = 0;
	    /* Note: At this point, the connection has either been 
	     * validated via a type='valid', or reported as invalid.
	     * If the connection is invalid, then the Receiving 
	     * Server MUST terminate both the XML stream and the 
	     * underlying TCP connection. 
	     */
	    emitraw("</stream:stream>");
	    remove_interactive(ME);
	    connect_failure("_dialback", "dialback gone wrong");
	}
	break;
    case "db:verify": // receiving step 9
	// t = NAMEPREP(node["@to"]) + ";" + node["@id"];
	t = node["@id"];
	o = gateways[t];
	if (objectp(o)) {
	    o -> verify_connection(node["@to"],
				   node["@from"],
				   node["@type"]);
	    // probably we can delete this...
	    m_delete(gateways, t);
#if 1 // not strictly necessary, but more spec conformant
	} else if (member(gateways, t)) {
	    P0(("%O found gateway for %O, but it is not an object: %O\n",
		ME, t, o))
	    m_delete(gateways, t);
#endif
	} else {
	    // wir haben keinen Gateway zu der ID... 
	    // was machen wir? loggen und ignorieren!
	    P0(("%O could not find %O in %O\n", ME, 
		t, gateways));
	}
	break;
#ifdef WANT_S2S_SASL
    case "success":
	if (node["@xmlns"] == NS_XMPP "xmpp-sasl") {
# ifdef LOG_XMPP_AUTH
	    D0( log_file("XMPP_AUTH", "\n%O auth SASL", ME); )
# endif 
	    // TODO: for digest-md5 we should check rspauth
	    emit("<stream:stream xmlns:stream='http://etherx.jabber.org/streams' "
		 "xmlns='jabber:server' xmlns:db='jabber:server:dialback' "
		 "to='" + hostname + "' "
		 "from='" + NAMEPREP(_host_XMPP) + "' "
		 "xml:lang='en' "
		 "version='1.0'>");
	    authenticated = 1;
	}
	break;
    case "challenge":
	PT(("%O got a sasl challenge\n", ME))
	if (node["@xmlns"] == NS_XMPP "xmpp-sasl") {
	    unless(t = node[Cdata]) {
		// none given
	    } else unless (t = to_string(decode_base64(t))) {
		// base64 decode error?
	    } else {
		// this one is shared across all those digest md5's
		mixed data;
		string secret;
		string response;
		PT(("decoded challenge: %O\n", t))
		data = sasl_parse(t);
		PT(("extracted %O\n", data))

		data["username"] = _host_XMPP;
		secret = config(XMPP + hostname, "_secret_shared");
		unless(secret) {
		    // mh... this is a problem!
		    // we only started doing this if we have a secret,
		    // so this cant be empty
		}
		data["cnonce"] = RANDHEXSTRING;
		data["nc"] = "00000001";
		data["digest-uri"] = "xmpp/" _host_XMPP;

		response = sasl_calculate_digestMD5(data, secret, 0);

		// ok, the username is our hostname
		// note: qop must not be quoted, as we are 'client'
		t = "username=\"" _host_XMPP "\","
		    "realm=\"" + data["realm"] + "\","
		    "nonce=\"" + data["nonce"] + "\","
		    "cnonce=\"" + data["cnonce"] + "\","
		    "nc=" + data["nc"] + ",qop=auth,"
		    "digest-uri=\"" + data["digest-uri"] + "\","
		    "response=" + response + ",charset=utf-8";
		PT(("%O sent rspauth %O\n", ME, response))
		emit("<response xmlns='" NS_XMPP "xmpp-sasl'>"
		     + encode_base64(t) + 
		     "</response>");
	    }
	}
	break;
    case "failure":
	// the other side has to close the stream
	monitor_report("_error_invalid_authentication_XMPP", sprintf("%O got a failure with xml namespace %O\n", ME, node["@xmlns"]));
	connect_failure("_invalid_authentication", "counterpart did not like our authorization");
	break;
#endif
    case "stream:error":
	if (ME) remove_interactive(ME);
	authenticated = 0;
	if (node["/connection-timeout"]) {
	    // this is normal, unless we had something to say,
	    // but we are an event-oriented system, so this
	    // "if" should never happen. then again, i've seen
	    // plenty of should-never-happens to happen, so..
	    // to put it in brian wilson words, god only knows  ;)
	    if (qSize(me)) 
		connect_failure("_timeout", "counterpart sent timeout");
	} else if (node["/not-authorized"]) {
	    connect_failure("_illegal_source",
			    "counterpart claims we are not authorized");
	} else if (node["/system-shutdown"]) {
	    connect_failure("_unavailable_restart",
			    "counterpart is doing a system shutdown");
	} else if (node["/host-unknown"]) {
	    // remote side runs a jabber server, but does not
	    // accept hostname
	    connect_failure("_host_unknown",
			    "counterpart does not recognize hostname");
	} else {
	    P0(("%O stream error, other side said %O\n", ME, node))
	    connect_failure("_unknown",
			    "counterpart stopped transaction for unknown reason");
	}
	break;
#ifdef SWITCH2PSYC
    case "switched":
# ifdef QUEUE_WITH_SCHEME
	o = ("psyc:" + host) -> circuit();
# else
	o = ("psyc:" + host) -> circuit(0, 0, 0, 0, host);  // whoami
# endif
	P1(("%O switched to %O for %O\n", ME, o, host))
	exec(o, ME);
	o -> logon();
	destruct(ME);
	return;
#endif
    default:
	P0(("%O: unexpected %O:%O\n", ME, node["@tag"], 
	    node["@xmlns"]))
	break;
    }
}

// prototype
int msg(string source, string mc, string data,
	    mapping vars, int showingLog, string target) {
    mixed t;
    string template, output;

    P2(("%O jabber/active:msg(%O,%O..)\n", ME, source, mc))
    /* TODO: use the language, i.e. sTextPath if _language is not
     * 	the current language 
     * 	sTextPath is quite expensive (string object lookup), 
     * 	but as we're parsing xml anyway...
     */ 
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
#ifdef PREFIXES
    // completely skip these methods
    if (abbrev("_prefix", mc)) return 1;
#endif
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

    // TODO: only _dialback_request_verify sollte hier betroffen sein
    if (abbrev("_dialback", mc)) {
	unless (interactive()) connect(); // ?
	if (ready) {
	    render(mc, data, vars, source);
	} else {
	    unless (dialback_queue) dialback_queue = ({ });
	    dialback_queue += ({ ({ source, mc, data, vars, target }) });
	}
	return 1;
    }

    // this should be part of render()/w()
    // but it has to happen before enqueuing (otherwise we had
    // problems with destructed objects)
    // "late enqueu" could be a solution to that problem
    determine_sourcejid(source, vars);
    determine_targetjid(target, vars);
    if (vars["_place"]) vars["_place"] = mkjid(vars["_place"]);

    // component needs to set authenticated as needed
    unless (authenticated) {
	if (interactive() && ready && dialback_outgoing == 0) {
	    start_dialback();
	}
#if _host_XMPP == SERVER_HOST
      // we can only do this if mkjid patches the psyc host into jabber host
      // but that is terrifically complicated and requires parsing of things
      // we already knew.. so let's simply enqueue with object id and reject
      // if the object got lost in the meantime
	vars["_source"] = UNIFORM(source);
	// hmm.. actually no.. since we also set _INTERNAL_source_jabber
	// this variable should never get used for anything anyway.. so
	// leaving it out should be completely harmless
#else
	// is this a bad idea? not sure.. we'll see..
	// if i don't have this here, a "tell lynX where it happened" will
	// be triggered from here -- best to never use _host_XMPP really ;)
	vars["_source"] = source;
	// behaviour has changed.. so maybe we don't need this any longer TODO
#endif
	return enqueue(source, mc, data, vars, showingLog, target);
    }
    return ::msg(source, mc, data, vars, showingLog, target);
}

open_stream(XMLNode node)
{
    mixed *t;
    string key;

    P2(("%O active for %O.\n", ME, hostname))
    D1( unless (interactive(ME))
	PP(("%O open_stream: should be interactive!\n", ME)); 
    )
    streamid = node["@id"];

    // actually, this is incorrect, als both parts of the version
    // are to be treated as separte ints ($4.4.1)
    // but as long as we only have 0 and 1.0...
    streamversion = to_float(node["@version"]);
    // note: flushing this Q should wait until stream features is completed	
    if (streamversion < 1.0) {
	process_dialback_queue();
	if (qSize(me)) 
	    start_dialback();
    } else { 
	// wait for stream:features
	nodeHandler = #'handle_stream_features;
    }
    return;
}
