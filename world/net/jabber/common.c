// $Id: common.c,v 1.276 2008/12/01 11:31:33 lynx Exp $ // vim:syntax=lpc:ts=8
#define NO_INHERIT
#include "jabber.h"
#undef NO_INHERIT

#include <net.h>
#include <text.h>
//virtual inherit NET_PATH "output";
#include <uniform.h>

#ifdef __psyclpc__
# if __VERSION_MICRO__ > 4
	// since this file is in the psyced distribution we can't
	// be sure we _really_ have RE_UTF8 unless we check the
	// driver version.. sigh
#  include <sys/regexp.h>
# endif
#endif

jabberMsg(); 

inherit NET_PATH "xml/common";

volatile string buffer = "";
volatile closure jid_has_node_cl = (: int t, t2;
			   t = index($1, '@');
			   if (t == -1) return 0;
			   t2 = index($1, '/');
			   if (t2 == -1 || t2 > t) return 1;
			   return 0; :);

qScheme() { return "xmpp"; } // habber.. chabber.. xabber?
// wie schreibt man das wie alvaro es spricht?

// all objects should call this for unwelcome hosts. TODO
// i wonder if gateway.c does inherit net/connect though. prolly not.
block() {
	STREAM_ERROR("policy-violation", "This is way beyond imagination.")
	// cant we :: that?
	destruct(ME);
	return 0;
	//return ::block();
}

int emit(string message) {
#if __EFUN_DEFINED__(convert_charset) && SYSTEM_CHARSET != "UTF-8"
        // apparently render() does this for us
//	iconv(message, SYSTEM_CHARSET, "UTF-8");
#endif
#ifdef RE_UTF8
	string t, err;
	// according to http://www.w3.org/TR/xml/#charsets
	// remove illegal unicode chars --// thx elmex
	err = catch(t = regreplace(message, "[^\\x{9}\\x{A}\\x{D}\\x{20}-\\x{D7FF}\\x{E000}-\\x{FFFD}\\x{10000}-\\x{10FFFFFF}]+", "*", RE_GLOBAL | RE_UTF8); nolog);
	if (err || t != message) {
		// Info: Chars filtered to %O. Message was %O.
		log_file("CHARS_XMPP", "[%s] %O %O %O\n", ctime(),
			 ME, err, message);
		if (t) message = t;
		// we get here when somebody has configured utf8 even though
		// he is actually sending latin. would be nicer to figure this
		// out at parsing time rather than at rendering time, and to
		// generate an _error back to sender in that case.  TODO
		//monitor_report("_error_invalid_data_charset", ...what?);
		P1(("catch! invalid chars going out to %O\n", ME))
		return 0; // do not emit
	}
#endif
#ifdef _flag_log_sockets_XMPP
	log_file("RAW_XMPP", "\n« %O\t%s", ME, message);
#endif
	return ::emit(message);
}

// don't check message, use this only where you are 100% sure
// to be sending safe data
int emitraw(string message) {
#ifdef _flag_log_sockets_XMPP
	log_file("RAW_XMPP", "\n« %O\t%s", ME, message);
#endif
	return ::emit(message);
}

// this assumes the old ldmuddish charmode+combine-charset
// if we ever get a input_bytes it needs to be rewrittn
feed(a) {
	int pos;
	buffer += a;

	while ((pos = strstr(buffer, ">") + 1) > 0){
		if (strstr(buffer, "<") == -1) {
			/* XML is a braindead spec
			 * > MAY be encoded
			 * 'this may be fixed in future versions'
			 * of the xmpp spec
			 */
			buffer = buffer[0..pos-2] + "&gt;"; // + buffer[pos..] <-- empty
			continue;
		}
#if __EFUN_DEFINED__(convert_charset) && SYSTEM_CHARSET != "UTF-8"
		if (catch(a = convert_charset(buffer[0..pos - 1],
				      "UTF-8", SYSTEM_CHARSET); nolog)) {
			P1(("catch! iconv %O in %O\n", a, ME))
			//QUIT
			a = buffer; // let's give it a try
		}
		xmlparse(a);
#else
		xmlparse(buffer[0..pos - 1]);
#endif
                buffer = buffer[pos..];
        }
#ifdef INPUT_NO_TELNET
        input_to(#'feed, INPUT_IGNORE_BANG | INPUT_CHARMODE | INPUT_NO_TELNET);
#else
	input_to(#'feed, INPUT_IGNORE_BANG | INPUT_CHARMODE);
#endif
}


#define JABBER_PARSE
#define XML_ERROR(code, long) \
	P0(("%O aborting XML parse: %s\n", ME, long)) \
	STREAM_ERROR(code, "") \
	remove_interactive(ME);
#include NET_PATH "xml/parse.c"
#undef JABBER_PARSE

jabberMsg(XMLNode node) {
	P2(("common:jabberMsg (should not happen) %O\n", node[Tag]))
}

// TODO: move this to separate file and determine which parts can be ifdeffed
varargs string mkjid(mixed who, mixed vars, mixed ignore_context,  string target, string jabberhost) {
    /* Die große Aufgabe: wie quetschen wir die gehaltvolle
     * Information aus dem vars-mapping gegeben durch 
     * _nick, nick_place, source && context in einen 
     * einzigen String der Form 
     * user@host/resource?
     */
	string t, *u;
	if (!who || who == "") return "";
	unless (jabberhost) jabberhost = _host_XMPP;
   	P3(("%O mkjid(%O, %O, %O, %O, %O)\n", ME, who, vars, ignore_context, target, jabberhost))
	if (!ignore_context && vars && vars["_nick_place"] 
	    && vars["_context"]) {
		// dies ist eine nachricht die multicastet wird.
		// jabber-roomids: #room@jabber.host/nickname
		if (objectp(vars["_context"])) {
		    t = PLACEPREFIX + NODEPREP(vars["_nick_place"]) +"@"+ NAMEPREP(jabberhost);
		} else if (u = parse_uniform(vars["_context"])) {
		    if (u[UScheme] == "psyc")
			t = PLACEPREFIX + NODEPREP(u[UNick]) + "@" + NAMEPREP(u[UHost]);
		    else // here we presume we have a u@h or xmpp:
			t = NODEPREP(u[UUser]) + "@" + NAMEPREP(u[UHost]);
		} else {
		    P0(("%O mkjid should not happen 1\n"))
		    t = PLACEPREFIX + vars["_nick_place"] + "@impossible";
		}
		return t;
	} else if (objectp(who)) {
		// we could optimize here with a jabberInfo() that returns
		// an array of ({ isplace, nickname, resource }) so we
		// don't have to make guesses at t[0] and splices at t[1..] etc
		//
		t = who->psycName();
		unless (t) return NAMEPREP(jabberhost); // ?
		if (t[0] == '@'){
		    t = PLACEPREFIX + NODEPREP(t[1..]) +"@"+ NAMEPREP(jabberhost);
		    // jabber-roomids: #room@host/nickname
		    // it seems that those are not case-sensitive
		    // we could probably use clash nick here, 
		    // but it's difficult
		} else {
		    string r;
		    // jabber-user: nick@host/resource (letztere optional)
		    t = NODEPREP(t[1..]) +"@"+ NAMEPREP(jabberhost);
		    // this call_other sucks for several reasons (other
		    // than being a call other):
		    // TODO: pass resource in vars if and only if
		    // 		needed
		    // 		TODO: mkjid needs a MAJOR rewrite
		}
		return t; 
	} else unless (stringp(who)) {
		P1(("%O unknown type for mkjid(%O, %O, %O, %O, %O)\n", ME, who, vars, ignore_context, target, jabberhost))
		//return NAMEPREP(jabberhost);	--- maybe better?
		return ""; 
	} else if (u = parse_uniform(who, 1)) {
		// jabber-userjids: nick@host/resource (letztere optional)
		t = u[UResource];
		unless (strlen(t)) t = 0;
		// this _SHOULD_ recognize its own host.. then again, we
		// simply avoid sending local sources as uniforms, please!
		unless (t) {
		    if (u[UUser])
			return NODEPREP(u[UUser]) + "@" + NAMEPREP(u[UHost]);
		    else
			return NAMEPREP(u[UHost]);
		}
		// this almost works... but it seems the resource is
		// case sensitive ???
		// er... what is that for?
		// ah... used for example when doing /version xmpp:host
		if (u[UScheme] == "xmpp" || !u[UScheme]) { // no scheme = pure jid
		    t = RESOURCEPREP(t);
		    if (u[UUser])
			return NODEPREP(u[UUser]) +"@"+ NAMEPREP(u[UHost]) +"/"+ t;
		    else
			return NAMEPREP(u[UHost]) + "/" + t;
		}
		// here we wildly presume this is a psyc: uniform
		if (t[0] == '@') {
		    t = PLACEPREFIX + NODEPREP(t[1..]) + "@" + NAMEPREP(u[UHost]);
		    return t;
		} else
		    return NODEPREP(t[1..]) +"@"+ NAMEPREP(u[UHost]);
	}
		// argument already _is_ a jid..
		// .. no wait i let parse_uniform handle that
//	if (index(who, '@') > 0) return who; // oops, PREPping is missing
		// argument is just a local username
	return NODEPREP(who) +"@"+ NAMEPREP(jabberhost);
}

void determine_sourcejid(mixed source, mapping vars) {
    mixed t;
    unless (vars["_INTERNAL_source_jabber_bare"]) {
	vars["_INTERNAL_source_jabber_bare"] = mkjid(source, vars);
	P4(("determine_sourcejid: %O\n", vars["_INTERNAL_source_jabber_bare"]))
    }
    unless (vars["_INTERNAL_source_jabber"]) {
	// append the resource to the bare jid
	vars["_INTERNAL_source_jabber"] = vars["_INTERNAL_source_jabber_bare"];

	if (vars["_nick_place"] && (t = vars["_nick_local"] || vars["_nick"])) {

	    vars["_INTERNAL_source_jabber"] += "/" + RESOURCEPREP(t);
	} 
	else if (vars["_INTERNAL_source_resource"])
	    vars["_INTERNAL_source_jabber"] += "/" + RESOURCEPREP(vars["_INTERNAL_source_resource"]);
    }
}

void determine_targetjid(mixed target, mapping vars) {
    string full;
    unless(vars["_INTERNAL_target_jabber_bare"]) {
	int d;
	full = mkjid(target, vars, 1);
	// f*** uni2unl logic
	if ((d = strstr(full, "/")) != -1)
	    vars["_INTERNAL_target_jabber_bare"] = full[..(d-1)]; 
	else
	    vars["_INTERNAL_target_jabber_bare"] = full; 
	P4(("determine_targetjid: %O\n", vars["_INTERNAL_target_jabber_bare"]))
    }
    unless (vars["_INTERNAL_target_jabber"]) {
	// append resource to the bare jid
	vars["_INTERNAL_target_jabber"] = vars["_INTERNAL_target_jabber_bare"];
	if (strstr(full, "/") != -1)
	    vars["_INTERNAL_target_jabber"] = full; // just to be sure
	else if (vars["_INTERNAL_target_resource"])
	    vars["_INTERNAL_target_jabber"] += "/" + vars["_INTERNAL_target_resource"];
    }
}

internalError() {
	STREAM_ERROR("internal-server-error", "something gone wrong internally")
	remove_interactive(ME);
}

render(string mc, string data, mapping vars, mixed source) {
        string template, output;

	unless(vars["_tag"]) vars["_tag"] = ""; // dont display [_tag]
	template = T(mc, "");
	if (!strlen(template) || template[0] != '<') {
		// generation of default psyc messages
		output = psyctext(template, vars, data, source);
		if (!stringp(output) || output=="")
		    return P2(("jabber:w() inherited no output\n"));
		output = "<message to='"+ vars["_INTERNAL_target_jabber"]
		   +"' from='"+ vars["_INTERNAL_source_jabber"] +"' type='"
		   + (ISPLACEMSG(vars["_INTERNAL_source_jabber"]) && vars["_nick"] ?
		      "groupchat" : "chat")
		   +"'><body>"+
#ifdef NEW_LINE
		   xmlquote(output)
#else
		   // was: chomp after xmlquote.. but why?
		   xmlquote(chomp(output))
#endif
		   +"</body></message>";
#if DEBUG > 1
		// most of these message we are happy with, so we don't need this log
		log_file("XMPP_TODO", "%O %s %s\n", ME, mc, output);
#endif
	} else {
		// hack for a special case where status update contains <, >
		// if this kind of problem recurrs, we should quote every
		// single damn variable
		if (vars["_description_presence"])
		    vars["_INTERNAL_XML_description_presence"] =
		      xmlquote(vars["_description_presence"]);
		if (stringp(data)) data = xmlquote(data);
		else if (vars["_action"])
		    data = "/me " + xmlquote(vars["_action"]);
		output = psyctext(template, vars, data, source);
		if (!stringp(output) || output=="")
		    return P2(("jabber:w() no output\n"));
#if 0
		if (strstr(output, "r00t") >= 0) {
			P0(("common:render(%O, %O, %O, %O) -> %O\n", mc,
			     data, vars, source, output))
		}
#endif
	}
#if __EFUN_DEFINED__(convert_charset) && SYSTEM_CHARSET != "UTF-8"
	if (catch(output = convert_charset(output,
		     SYSTEM_CHARSET, "UTF-8"); nolog)) {
	    sendmsg(source, "_failure_unsuccessful_conversion_charset",
		"Could not convert your message to UTF-8 for XMPP delivery.",
		vars);
            P1(("catch! iconv %O from %O in %O\n", output,
		SYSTEM_CHARSET, ME))
	    return 0;
	}
#endif
	emit(output);
	return 1;
}

/* TODO:
 * it could be useful to have an error condition mapping (jep-0086) 
 * to check both old-style and xmpp-style errors
 * at least two uses of that in gateway.c
 */
xmpp_error(node, xmpperror) {
    unless (mappingp(node)) {
	// psyced.org logs claim.. this does happen!?
	P0(("%O encountered funny xmpp_error %O %O\n",
	    ME, node, xmpperror))
	return 1;
    }
    if (node["/" + xmpperror]) return 1;
    // shared_memory()? doesn't matter, switch is fine too. not worth changing
    switch(xmpperror) {
    case "bad-request":
	return node["@code"] == "400";
    case "conflict":
	return node["@code"] == "409";
    case "feature-not-implemented":
	return node["@code"] == "501";
    case "forbidden":
	return node["@code"] == "403";
    case "gone":
	return node["@code"] == "302";
    case "internal-server-error":
	return node["@code"] == "500";
    case "item-not-found":
	return node["@code"] == "404";
    case "jid-malformed":
	return node["@code"] == "400";
    case "not-acceptable":
	return node["@code"] == "406";
    case "not-allowed":
	return node["@code"] == "405";
    case "not-authorized":
	return node["@code"] == "401";
    case "payment-required":
	return node["@code"] == "402";
    case "recipient-unavailable":
	return node["@code"] == "404";
    case "redirect":
	return node["@code"] == "302";
    case "registration-required":
	return node["@code"] == "407";
    case "remote-server-not-found":
	return node["@code"] == "404";
    case "remote-server-timeout":
	return node["@code"] == "504";
    case "resource-constraint":
	return node["@code"] == "500";
    case "service-unavailable":
	return node["@code"] == "503";
    case "subscription-required":
	return node["@code"] == "407";
    case "undefined-condition":
	return node["@code"] == "500";
    case "unexpected-request":
	return node["@code"] == "400";
    }
    return 0;
}

#ifdef WANT_S2S_TLS
certificate_check_jabbername(name, cert) {
    mixed t;
    /* this does not support wildcards if there is more than one
     * id-on-xmppAddr/CN
     * API Note: name MUST be an utf8 string
     */
    unless(name) return 0;
    name = NAMEPREP(name);
    unless(cert && mappingp(cert)) return 0;
    if ((t = cert["2.5.29.17:1.3.6.1.5.5.7.8.5"])) { // id-on-xmppAddr
	PT(("id-on-xmppAddr %O found\n", t))
# ifdef LOG_XMPP_AUTH
	D0( log_file("XMPP_AUTH", "\n%O try SASL external with id-on-xmppAddr", ME); )
# endif 
	if (pointerp(t)) {
	    if (member(t, name) != -1) return 1;
	    foreach(string cn : t) {
		if (NAMEPREP(cn) == name) return 1;
	    }
	} 
	else if (name == NAMEPREP(t))
	    return 1;
    } 
    if ((t = cert["2.5.29.17:dNSName"])) { // dNSName, wildcard allowed
	if (pointerp(t)) {
	    foreach(string t2 : t) {
		if (strlen(t2) > 2 && t2[0] == '*' && t2[1] == '.') 
		    if trail(NAMEPREP(t2[2..]), name) 
			return 1;
		if (name == NAMEPREP(t2)) 
		    return 1;
	    }
	} else {
	    if (strlen(t) > 2 && t[0] == '*' && t[1] == '.') {
		return trail(NAMEPREP(t[2..]), name);
	    }
	    if (name == NAMEPREP(t)) 
		return 1;
	}
    } 
    if ((t = cert["2.5.4.3"])) { // common name
	string idn;
# ifdef LOG_XMPP_AUTH
	D0( log_file("XMPP_AUTH", "\n%O try SASL external with CN", ME); )
# endif 
	if (pointerp(t)) { // does that happen?!
	    if (member(t, name) != -1) return 1;
	    foreach(string cn : t) {
		idn = NAMEPREP(idna_to_unicode(cn));
		if (idn == name) return 1;
	    }
	    return 0;
	} 
#ifdef __IDNA__
	idn = NAMEPREP(idna_to_unicode(t));
#else
	idn = NAMEPREP(t);
#endif
	if (strlen(idn) > 2 && idn[0] == '*' && idn[1] == '.')
	    return trail(idn[2..], name);
	if (idn == name) 
	    return 1;
    }
    return 0;
}
#endif

/* get first child of a node used for <iq/>
 * "first" is actually inaccurate, since there is no defined order in mappings,
 * so we select the child, that is not an error
 */
getiqchild(node) {
    mixed res;
    foreach(mixed key, mixed val : node) {
	unless(stringp(key) && key[0] == '/') continue;
# if DEBUG > 1
	if (res) {
	    P0(("%O encountered iq get with more than one child!\n%O\n",
		ME, node))
	}
# endif
	if (node["@type"] != "error" && key != "error") {
	    res = val;
# if DEBUG < 2
	    break;
# endif
	}
    }
    return res;
}

/* get child with specific xmlns
 */
getchild(node, child, ns) {
    foreach(mixed key, mixed val : node) {
	if (key == "/" + child) {
	    if (nodelistp(val)) {
		foreach(mixed h : val)
		    if (h["@xmlns"] == ns)
			return h;
	    } else if (val["@xmlns"] == ns) {
		return val;
	    }
	    break;
	}
    }
    return 0;
}
