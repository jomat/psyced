// $Id: server.c,v 1.159 2008/10/01 10:59:24 lynx Exp $ // vim:syntax=lpc:ts=8
#include "jabber.h"	
#include "server.h"	// inherits net/server

#include "person.h" // find_person
#include "url.h"
#include <sys/tls.h>

volatile string authtag;
volatile string resource;
volatile string streamid;
volatile string pass;
volatile string language;
int reprompt;

volatile string sasluser;
volatile string saslchallenge;

#ifdef __TLS__
volatile mixed certinfo;
#endif

qScheme() { return "jabber"; }
// qName() { } // we need that in common.c - probably no more
  
void create() {
	unless (clonep()) return;
	// not multilingual yet - TODO
	sTextPath(0, 0, "jabber");
}

#ifdef __TLS__
// similar code in other files
tls_logon(result) {
    if (result == 0) {
# ifdef WANT_S2S_SASL	/* hey wait.. this is not S2S here!? */
        certinfo = tls_certificate(ME, 0);
# endif
        P3(("%O tls_logon fetching certificate: %O\n", ME, certinfo))
# ifdef ERR_TLS_NOT_DETECTED
    } else if (result == ERR_TLS_NOT_DETECTED) {
	// no encryption. no problem.
# endif
    } else if (result < 0) {
        P1(("%O TLS error %d: %O\n", ME, result, tls_error(result)))
        QUIT // don't fall back to plaintext instead
    } else {
        P0(("tls_logon with result > 0?!?!\n"))
        // should not happen
    }
}
#endif

string *splitsasl(string data) {
	string *result = ({ "" });
	int *decoded;
	int i, j;

	decoded = decode_base64(data);
	foreach(i : decoded) {
		if (i == 0) {
			j++;
			result += ({ "" });
			continue;
		}
		result[j] += sprintf("%c", i);
	}
	return result;
}

promptForPassword(user) {
	P2(("promptForPassword with %O\n", user))
	if (reprompt == 1 || pass) {
		w("_error_invalid_password", "Invalid password.\n", 
		  ([ "_tag_reply" : authtag || "", "_nick" : nick, 
		   "_resource" : resource ]) );
		emitraw("</stream:stream>");
		QUIT
		return; // ?
	}
	P2(("nick %O, pass %O\n", nick, pass))
	unless (pass) {
		reprompt = 1;
		w("_query_password", 0, //"Please provide your password.", 
	  		([ "_nick": nick, "_tag_reply" : authtag || "" ]) );
	}
	return 1;
}

logon(a) {
#ifdef _flag_log_sockets_XMPP
	log_file("RAW_XMPP", "\n%O logon\t%O", ME, ctime());
#endif
	P3(("logon(%O) beim jabber:server.c\n", a))
	set_combine_charset(COMBINE_CHARSET);
#ifdef INPUT_NO_TELNET
	input_to(#'feed, input_to_settings =
		 INPUT_IGNORE_BANG | INPUT_CHARMODE | INPUT_NO_TELNET);
#else
	enable_telnet(0, ME);
	input_to(#'feed, input_to_settings =
		 INPUT_IGNORE_BANG | INPUT_CHARMODE);
#endif
	return ::logon(a);
}

createUser(nick) {
	return named_clone(JABBER_PATH "user", nick);
}


userLogon() {
	user->sTag(authtag);
	user->sResource(resource);
	return ::userLogon();
}

#ifdef GAMMA
authChecked(result, varargs array(mixed) args) {
	// a point where we could be sending our jabber:iq:auth reply
	// instead of letting _notice_login do that
	PT(("%O got authChecked %O, %O\n", ME, result, args))
	return ::authChecked(result, args...);
}
#endif

jabberMsg(XMLNode node) {
	XMLNode helper;
	string id;
	mixed t;

	id = node["@id"] || ""; // tag?
	switch (node[Tag]) {
	case "iq":
	    if (node["/bind"]) {
		// suppresses the jabber:iq:auth reply in the SASL case
		authtag = -1;
		unless (sasluser) {
		    // not-allowed stanza error?
		    return 0;
		}
		helper = node["/bind"];
		// see XMPP-core, 7. Resource binding
		if (helper["/resource"])
		    resource = helper["/resource"][Cdata];
		// assign a resource
		if (!stringp(resource) || resource == "")	
		    resource = "PSYC";
		nick = sasluser;
		sasluser = "";	    // why an empty string? explanation needed

		emit("<iq type='result' id='"+ id +"'>"
		     "<bind xmlns='" NS_XMPP "xmpp-bind'><jid>"+
		     nick +"@" SERVER_HOST "/"+ resource +"</jid>"
		     "</bind></iq>");
		return 0;
	    } else if (node["/session"]) {
		unless(user) return 0; // what then?
		emit("<iq type='result' id='"+ id +"' from='"
		     SERVER_HOST "'/>");
		user -> vSet("language", language);
		return morph();
	    }
	    switch (node["/query"]["@xmlns"]) {
	    // old-school style.. for clients that don't like SASL, like kopete, jabbin
	    case "jabber:iq:auth":
		    authtag = id;
		    if (node["@type"] == "get"){
			// hello(nick) ?
                        w("_query_password", 0,
                            ([ "_nick": nick, "_tag_reply": authtag || "" ]), "");
		    } else if (node["@type"] == "set") {
			helper = node["/query"];
			resource = helper["/resource"][Cdata];
			nick = helper["/username"][Cdata];
			if (mappingp(helper["/password"]) && 
			    (pass = helper["/password"][Cdata])) {
			    hello(nick, 0, pass);
			} else if (mappingp(helper["/digest"]) && 
				 (pass = helper["/digest"][Cdata])) {
			    P3(("jabber:iq:auth/digest got %O\n", pass))
			    hello(nick, 0, pass, "sha1", streamid);
			} else {
			    // TODO: write an error?
			    P0(("jabber:server iq auth set without pass or digest\n"))
			}
		    }
		    return 1;
	    case "jabber:iq:register":
		if (node["@type"] == "get"){
		    string packet;
#if defined(REGISTERED_USERS_ONLY) || defined(_flag_disable_registration_XMPP)
		    // super dirty.. this should all be in textdb
		    packet = sprintf("<iq type='result' id='%s'>"
				     "<query xmlns='jabber:iq:register'/>"
	 "<error code='501>Registration by XMPP not permitted.</error>" IQ_OFF,
				     id);
#else
		    packet = sprintf("<iq type='result' id='%s'>"
				     "<query xmlns='jabber:iq:register'>" 
				     "<instructions>You dont even need to register, "
				     "this is psyced. Manual at http://help.pages.de</instructions>"
				     "<name/><email/><username/><password/>" IQ_OFF,
				     id);
#endif
		    emit(packet);
		} else if (node["@type"] == "set"){
		    string packet;
		    packet = sprintf("<iq type='error' id='%s'>"
				     "<query xmlns='jabber:iq:register'>",
				     id);
		    unless ((helper = node["/query"])
				&& (t = helper["/username"][Cdata])
				&& (user = summon_person(t))) {
			// internal server error
			STREAM_ERROR("internal-server-error", 
				     "Oh dear! Internal server error")
			QUIT	// too hard?
		    }
		    unless (user -> isNewbie()) {
			// already registered to someone else
			packet += "<error code='406' type='cancel'>"
				"<conflict xmlns='" NS_XMPP "xmpp-stanzas'/>"
				"</error>"
				IQ_OFF;
			emit(packet);
			QUIT
		    } else unless ((t = helper["/username"]) &&
				   t[Cdata] &&
				   (t = helper["/password"]) &&
				   t[Cdata]) {
			packet += "<error code='406' type='modify'>"
				"<not-acceptable xmlns='" NS_XMPP "xmpp-stanzas'/>"
				"</error>"
				IQ_OFF;
			emit(packet);
			QUIT
		    } else {
#if defined(REGISTERED_USERS_ONLY) || defined(_flag_disable_registration_XMPP)
			// TODO: generate some error as above
#else
			user -> vSet("password", t[Cdata]);
			if (t = helper["/email"]) {
			    user -> vSet("email", helper["/email"]);
			}
			// maybe immediate save is not really a good idea
			// user -> save();
			emit(sprintf("<iq type='result' id='%s'/>", id));
#endif
		    }
		    user = 0;
		}
	    }
	    break;
	case "starttls":
#if __EFUN_DEFINED__(tls_available)
	    if (tls_available()) {
		emitraw("<proceed xmlns='" NS_XMPP "xmpp-tls'/>");
		// we may not write until tls_logon is called!
		tls_init_connection(ME, #'tls_logon);
	    } else {
		P1(("%O received a 'starttls' but TLS isn't available.\n", ME))
	    }
#else
	    emitraw("<failure xmlns='" NS_XMPP "xmpp-tls'/></stream:stream>");
	    destruct(ME);
#endif
	    break;
	case "auth":
	    switch (node["@mechanism"]) {
	    case "DIGEST-MD5":
		// TODO: time-based nonce
		saslchallenge = RANDHEXSTRING;
		emit("<challenge xmlns='" NS_XMPP "xmpp-sasl'>" + 
		     encode_base64(sprintf("realm=\"%s\",nonce=\"%s\","
					   "qop=\"auth\",charset=utf-8,"
					   "algorithm=md5-sess", 
					   SERVER_HOST,
					   saslchallenge)
				   ) + "</challenge>");
		break;
	    case "PLAIN":
		string *creds; 
		creds = splitsasl(node[Cdata]);
		// check that creds[2] is valid for user creds[1] 
		// and in case of success:
		if ((sizeof(creds) == 3)
		    && (user = find_person(creds[1])
			|| (user = createUser(creds[1])))
#ifdef ASYNC_AUTH
		   ) {
		    mixed authCb = CLOSURE((int result), (mixed creds), (creds),
			{
			    if (result) {
				sasluser = creds[1];
				emitraw("<success xmlns='" NS_XMPP "xmpp-sasl'/>");
			    } else {
				sasluser = 0;
				SASL_ERROR("temporary-auth-failure")
				QUIT
			    }
			    return;
			});
		    user -> checkPassword(creds[2], "plain", 0, 0, authCb);
#else
		    && user -> checkPassword(creds[2], "plain")) {
			sasluser = creds[1];
			emitraw("<success xmlns='" NS_XMPP "xmpp-sasl'/>");
#endif
		} else {
		    SASL_ERROR("invalid-mechanism")
		    QUIT
		} 
		break;
#ifdef __TLS__
	    case "EXTERNAL":
# if 0
		// TODO: basically this works but is untested due to
		// lack of clients
		// also, it uses email addresses instead of xmpp jids
		// and i am not sure if I should check who signed the cert,
		// as I probably want my users only to use certs created by me
		unless (node[Cdata]) {
		    SASL_ERROR("incorrect-encoding")
		    QUIT
		} else unless (mappingp(certinfo) && certinfo[0] == 0
			    && certinfo["1.2.840.113549.1.9.1"]) {
		    SASL_ERROR("invalid-mechanism")
		    QUIT
		} else {
		    string deco, ho, u;
		    // TODO: there could be a try-except block here with
		    // incorrect-encoding sasl error
		    deco = to_string(decode_base64(node[Cdata]));
		    // TODO: the right thingie could be a list!
		    unless (deco == certinfo["1.2.840.113549.1.9.1"]) {
			// TODO: not sure about this one
			SASL_ERROR("invalid-mechanism")
			QUIT
		    }
		    // lets see if its one of our users
		    sscanf(deco, "%s@%s", u, ho);
		    unless (is_localhost(lower_case(ho))) {
			// wrong host
			SASL_ERROR("invalid-authzid")
			QUIT
                                // TODO: consider legalized names
		    } else unless ( ///// legal_name(u)) {
			SASL_ERROR("not-authorized")
			QUIT
		    } else {
			user = find_person(u) || createUser(u);
			sasluser = u;
			emitraw("<success xmlns='" NS_XMPP "xmpp-sasl'/>");
		    }
		}
# else
		SASL_ERROR("invalid-mechanism")
		QUIT
# endif
		break;
#endif
#ifndef REGISTERED_USERS_ONLY
	    case "ANONYMOUS":
		unless(node[Cdata]) {
		    SASL_ERROR("incorrect-encoding")
		    QUIT    // i suppose this was missing here  --lynX
		} else {
		    string u;

		    t = to_string(decode_base64(node[Cdata]));
		    u = legal_name(t);
		    unless(u) {
			if (t) {
			    SASL_ERROR("not-authorized")
			} else {
			    SASL_ERROR("invalid-authzid")
			}
			QUIT
		    }
		    PT(("wants to auth anonymously as %O\n", u))
		    unless (user = summon_person(u)) {
			SASL_ERROR("temporary-auth-failure")
			QUIT
		    }
		    unless (user -> isNewbie()) {
			SASL_ERROR("not-authorized")
			QUIT
		    }
		    sasluser = u;
		    emitraw("<success xmlns='" NS_XMPP "xmpp-sasl'/>");
		}
		break;
#endif
	    default:
		SASL_ERROR("invalid-mechanism")
		QUIT
		break;
	    }
	    break;
	case "response":
	    // this does not behave according to rfc3920 but rather the 3920bis
	    // version
	    if (node[Cdata] && (t = to_string(decode_base64(node[Cdata])))) {
		mapping creds = sasl_parse(t);
		mixed authCb = CLOSURE((int result), (mixed creds), (creds),
		    {
			if (result) {
			    P3(("digest md5 success\n"))
			    sasluser = creds["username"];
			    P3(("result is %O\n", result))
			    emit("<success xmlns='" NS_XMPP "xmpp-sasl'>"
				 + encode_base64("rspauth=" + result) +
				 "</success>");
			} else {
			    P0(("digest md5 failure: %O\n", creds))
			    sasluser = 0;
			    SASL_ERROR("invalid-authzid")   // why do we get here?
			    QUIT
			}
			return 0;   // ignored, but avoids a warning
		    });

		user = find_person(creds["username"]) || createUser(creds["username"]);
		user -> checkPassword(1, "digest-md5", 0, creds, authCb);
	    } else {
		SASL_ERROR("not-authorized")
		QUIT
	    }
	    break;
	default:
	    /* angeblich werden andere Pakete in diesen Zustand gequeried 
	     * aber das glaube ich nicht dass das wirklich so ist
	     * wie auch immer haetten wir sonst mittlerweile net/queue fuer
	     * diesen job
	     */
	    P0(("jabber/server:jabberMsg default case\n"))
	}
	// return ::jabberMsg(from, cmd, args, data, all);
	return 0;
}

open_stream(XMLNode node) {
	string features;
	string header;

	P2(("%O open_stream from %O\n", ME, query_ip_name()))
	streamid = RANDHEXSTRING;

	unless(node["@xmlns"] == "jabber:client") {
	    // TODO: is this a fatal error?
	}
	switch (node["@xml:lang"]) {
	case "DE": // what about DE_at etc?
	case "de":
	case "german":
	case "German": // curious... isnt the xml:lang supposed to be 'de'?
		language = "de";
		break;
	default:
		if (!language) language = "en";
		break;
	}
	features = "";
	header = "<stream:stream " 
		"from='" SERVER_HOST "' "
		"xmlns='jabber:client' "
		"xmlns:stream='http://etherx.jabber.org/streams' ";
	if (node["@version"] == "1.0") {
	    header += "version='1.0' ";
	    features = "<stream:features>";
#if __EFUN_DEFINED__(tls_available)
	    if (tls_available() && tls_query_connection_state(ME) == 0)
		features += "<starttls xmlns='" NS_XMPP "xmpp-tls'/>";
#endif
	    if (sasluser) {
		features += "<bind xmlns='" NS_XMPP "xmpp-bind'/>";
		features += "<session xmlns='" NS_XMPP "xmpp-session'/>";
	    } else {

		features += "<mechanisms xmlns='" NS_XMPP "xmpp-sasl'>"
#ifndef _flag_disable_authentication_digest_MD5
		      "<mechanism>DIGEST-MD5</mechanism>"
#endif
		      "<mechanism>PLAIN</mechanism>";
#ifndef REGISTERED_USERS_ONLY
		    // sasl anonymous
		      "<mechanism>ANONYMOUS</mechanism>";
#endif
#if __EFUN_DEFINED__(tls_available)
		if (tls_available() && tls_query_connection_state(ME) > 0
			&& mappingp(certinfo) && certinfo[0] == 0
			&& certificate_check_jabbername(0, certinfo)) {
		    features += "<mechanism>EXTERNAL</mechanism>";
		}
#endif
		features += "</mechanisms>";
		features += "<auth xmlns='http://jabber.org/features/iq-auth'/>";
#ifndef REGISTERED_USERS_ONLY
		features += "<register xmlns='http://jabber.org/features/iq-register'/>";
#endif
	    }
	    features += "</stream:features>";
	}
	header += "id='" + streamid + "'>";
	emit(header + features);
}

// overrides certificate_check_jabbername from common.c with a function
// that is approproate for authenticating users
certificate_check_jabbername(name, certinfo) {
    // plan: prefer subjectAltName:id-on-xmppAddr, 
    // 		but allow email (1.2.840.113549.1.9.1)
    // 		and subjectAltName:rfc822Name
    return 0;
}
