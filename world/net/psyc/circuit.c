// vim:foldmethod=marker:syntax=lpc:noexpandtab
//
// The PSYC Circuit Implementation.
// Includes renderers and parsers for the PSYC syntax.
//
// $Id: circuit.c,v 1.202 2008/12/04 14:20:53 lynx Exp $

// local debug messages - turn them on by using psyclpc -DDcircuit=<level>
#ifdef Dcircuit
# undef DEBUG
# define DEBUG Dcircuit
#endif

#include "common.h"
#include <net.h>
#include <services.h>
#include <person.h>
#include <driver.h>
#include <uniform.h>
#include <text.h>
#include <psyc.h>

protected void quit();	// prototype

#ifdef __PIKE__
import net.psyc.common;
#else
// since this doesn't exec() the user object
// it needs to do its own network output
virtual inherit NET_PATH "output";
inherit PSYC_PATH "common";
#endif

// thats also why we do not inherit the generic server!
//
#ifndef QUIT
//define	QUIT	remove_interactive(ME); return 1;
# define	QUIT	destruct(ME); return 1;
#endif

//volatile mapping namecache = ([]);

#ifdef FORK
volatile mapping _o, memory;
// why do we need to do this, el?
//psycName() { return ""; }
#else
//# define MMP_STATE
#endif
#ifdef MMP_STATE
// first steps at making use of TCPs persistence
volatile string lastSource;
volatile string lastTarget;
volatile string lastContext;
#endif

int isServer() { return 0; }

volatile int flags = 0;

#define PSYC_TCP
// contains PSYC parser
#ifdef FORK
# include "routerparse.i"
#else
# include "parse.i"
#endif

int greet() {
	string usingmods;

	sTextPath();
// we should know our port numbers from command line
// and with the new ports.h we do.. we just have to use it here..
// ehm.. how do we to_string() a dozen numbers efficiently? we don't huh?
// we patch ports.h to also provide #define PSYC_PORT_S etc? we let
// psyconf generate this string? how do we specify TLS ports? TODO
//
#define UNDERPROTS      USINGPROTS ";PSYC/0.9 UDP IP/4;" \
			"XMPP-S2S/1;IRC/2;XMPP-C2S/1;Chatlet;Telnet;" \
			"HTTP/1.0;WAP"
#define USINGPROTS      "PSYC/0.9 TCP IP/4"

#ifdef PRO_PATH
# if defined(BRAIN) && !defined(SLAVE) && !defined(VOLATILE)
// for the time being.. a special case with port numbers
#  define PROTS "PSYC/0.9:4404 TCP IP/4;PSYC/0.9:4404 UDP IP/4;" \
     "XMPP-S2S/1:5269;IRC/2:6667;XMPP-C2S/1:5222;Chatlet:2008;Telnet:23;" \
     "HTTP/1.0:33333;XMLpsyc/0.3:1404;WAP"
// and the gateways brain provides..
// I would like to change that into old list-syntax again.. or lets implement
// this @_var stuff.. this damages my brain
#  define SCHEMES ":_understand_schemes\taim;icq;irc;efnet;euirc;" \
        "freenode;quakenet;galaxynetwork;klingons\n"
#  define TSCHEMES "Gateways provided: [_understand_schemes].\n"
# else
#  define	SCHEMES	""
#  define	TSCHEMES ""
#  define PROTS UNDERPROTS ";XMLpsyc/0.3"
# endif
#else
# define	SCHEMES	""
#  define	TSCHEMES ""
# define PROTS UNDERPROTS
#endif

// we only understand circuit-level (MMP) _state, the one that is so easy
// that we can expect any TCP-MMP implementation to provide it, but PSYC
// state which needs to be stored per logical source and target still needs
// to be implemented. see also http://about.psyc.eu/State
//#define UNDERMODS "_state;_context"
#define UNDERMODS "_context"
	usingmods = UNDERMODS;

#if defined(__MCCP__) && !defined(FORK) 
# define ZIPMOD ";_compress"
	if (query_mccp(ME)) usingmods += ZIPMOD;
#else
# define ZIPMOD
#endif
#if defined(__TLS__)
# define TLSMOD ";_encrypt"
	if (tls_query_connection_state(ME) > 0) usingmods += TLSMOD;
#else
# define TLSMOD
#endif

// TODO: if negotiation of _using_characters is implemented, then change this
// _available_characters into _understand_characters. same goes for
// _available_protocols, although i don't expect anyone to need this.
// 
// the text messages in here are to say hello to clients.
// servers will just skip them..
//
// und eigentlich ist das alles nur eitelkeit, die vielen vars. sagt el.  ;)
#ifdef FORK // {{{
	emit(S_GLYPH_PACKET_DELIMITER "\n");
	emit("\
=_source	"+ SERVER_UNIFORM +"\n\
=_target_peer	psyc://"+ peeraddr +"/\n\
=_available_characters	UTF-8\n\
=_available_protocols	" PROTS "\n\
" SCHEMES "\
=_understand_modules	" UNDERMODS TLSMOD "\n\
=_using_characters	" SYSTEM_CHARSET "\n\
=_using_protocols	" USINGPROTS "\n\
=_using_modules	"+ usingmods +"\n\
\n\
:_implementation	" SERVER_VERSION " " DRIVER_VERSION " " OSTYPE " " MACHTYPE "\n\
:_page_description	http://www.psyc.eu/\n\
_notice_circuit_established\n\
Circuit to [_source] running [_implementation] established.\n\
Available protocols: [_available_protocols].\n" S_GLYPH_PACKET_DELIMITER "\n");
#else // }}}
	// should we rename _target into _target_raw here? maybe, then again
	// all subsequent traffic still goes to this target unless the
	// other side tells us her name
	emit(S_GLYPH_PACKET_DELIMITER "\n");
	emit("\
:_source	"+ SERVER_UNIFORM +"\n\
:_target_peer	psyc://"+ peeraddr +"/\n"
"\n\
:_implementation	"+ SERVER_VERSION +" "+ DRIVER_VERSION +" "+ OSTYPE +" "+ MACHTYPE +"\n\
:_page_description	http://www.psyc.eu/\n\
_notice_circuit_established\n\
Hello [_target_peer].\n\
Circuit to [_source] running [_implementation] established.\n" S_GLYPH_PACKET_DELIMITER "\n");
	// ;ISO-8859-1;ISO-8859-15\n
	emit("\
:_source	"+ SERVER_UNIFORM +"\n\
\n\
:_available_hashes	"
#if __EFUN_DEFINED__(sha1)
			       "sha1;"
#endif
#if __EFUN_DEFINED__(md5)
			       "md5;http-digest;digest-md5"
#endif
				"\n\
:_available_characters	UTF-8\n\
:_available_protocols	" PROTS "\n\
" SCHEMES "\
:_understand_modules	" UNDERMODS TLSMOD "\n\
:_using_characters	" SYSTEM_CHARSET "\n\
:_using_protocols	" USINGPROTS "\n\
:_using_modules	"+ usingmods +"\n\
_status_circuit\n\
" TSCHEMES "\
Available protocols: [_available_protocols].\n" S_GLYPH_PACKET_DELIMITER "\n");
#endif // !FORK
#ifdef _flag_log_sockets_PSYC
	log_file("RAW_PSYC", "« %O greeted.\n", ME);
#endif
	return 1;
}

static varargs int block(vastring mc, vastring reason) {
	// we used to get here at shutdown time because legal_host was
	// saying no.. but it's better to do it differently..
	P0(("Circuit blocked TCP PSYC connection from %O in %O (%O).\n",
	    query_ip_number(ME), ME, mc))
#ifdef DEVELOPMENT
	unless (ME)
	    raise_error("blocked destructed object?\n");
	unless (interactive(ME))
	    raise_error("blocked non-interactive?\n");
#endif
	write(S_GLYPH_PACKET_DELIMITER "\n\
\n\
:_source_redirect   psyc://ve.symlynX.com\n\
_error_illegal_source\n\
Sorry, my configuration does not allow me to talk to you.\n\
Try the [_source_redirect] server instead!\n" S_GLYPH_PACKET_DELIMITER "\n");
	QUIT
}

// this logon() is either called in psyc/server, so it has no args
// or it is actively called from psyc/active, when no failure has
// taken place. thus, the argument is useless.
int logon(int neverfails) {
#ifdef __TLS__
	mixed cert;
#endif
	object cur;
	int isserver;
	mixed m;
	string t;

	unless (ME) {
		// this happens when shutdown has been initiated during
		// establishment of the circuit. we get the callback,
		// although we have already been destroyed. unfortunately
		// we can't even find out which connection it was.
		P3(("logon(%O) called in destructed object\n", neverfails))
		return 0;
	}
	isserver = isServer();
	peerip = query_ip_number(ME) || "127.0.0.1";
	P3(("%O logon from %O\n", ME, peerip))

	// in case of psyc/active this hostCheck has already performed
	// in net/connect
	if (isserver &&! hostCheck(peerip, peerport)) {
		// also gets here when ME is 0
		return block();
	}

#ifdef __TLS__
	sAuthHosts(([ ])); // reset authhosts 
	if (tls_available() && tls_query_connection_state(ME) == 1 && mappingp(cert = tls_certificate(ME, 0))) {
	    if (cert[0] != 0) {
		// log error 17 or 18 + cert here
		P0(("%O encountered a cert verify error %O in %O\n", ME,
		    cert[0], cert))
		// and goodbye.
# ifdef _flag_enable_certificate_any
		remove_interactive(ME);
		return 0;
# endif
	    }
	    if (m = cert["2.5.29.17:dNSName"]) {
		// FIXME: this does not yet handle wildcard DNS names
		P1(("%O believing dNSName %O\n", ME, m))
		// probably also: register_target?
		// but be careful never to register_target wildcards
		if (stringp(m)) sAuthenticated(NAMEPREP(m));
		else foreach(t : m) sAuthenticated(NAMEPREP(t));
	    }
//#ifdef _flag_allow_certificate_name_common	// to be switched this year
# ifndef _flag_disallow_certificate_name_common
	    // assume that CN is a host
	    // as this is an assumption only, we may NEVER register_target it
	    // note: CN is deprecated for good reasons.
	    else if (t = cert["2.5.4.3"]) {
		P1(("%O believing CN %O\n", ME, t))
		sAuthenticated(NAMEPREP(t));
	    }
# endif
	    if (m = tls_query_connection_info(ME)) {
		P2(("%O is using the %O cipher.\n", ME, m[TLS_CIPHER]))
		// shouldn't our negotiation have ensured we have PFS?
		if (stringp(t = m[TLS_CIPHER]) &&! abbrev("DHE", t)) {
//			croak("_warning_circuit_encryption_cipher",
//			"Your cipher choice does not provide forward secrecy.");
			monitor_report(
			    "_warning_circuit_encryption_cipher_details",
			    object_name(ME) +" · using "+ t +" cipher");
			//debug_message(sprintf(
			//  "TLS connection info for %O is %O\n", ME, m));
			//QUIT	// are we ready for *this* !???
		}
	    }
	}
#endif

	cvars = ([]);
	pvars = ([ "_INTERNAL_origin" : ME ]);

#if defined(MMP_STATE) && !defined(FORK)
	lastSource = lastTarget = lastContext = 0;
#endif
	next_input_to(#'startParse);
	// even active connections want to time out to avoid lockups
	// but quit() should check if there is a queue to return! TODO
	call_out(#'quit, 90);
	flags |= TCP_PENDING_TIMEOUT;
//	unless(peeraddr) {
	    peeraddr = peerhost = peerip;
	    // peerport value is positive for real listen() ports 
	    if (peerport) peeraddr += ":"+peerport;
//	}
#ifdef FORK // {{{
	// init the out-state. these will be sent by greet()
	_o = ([
		"_source" : SERVER_UNIFORM,
		"_target" : "psyc:"+ peeraddr +"/",
	     ]);
	memory = copy(_o);
#if 0
	memory = ([ "_source" : SERVER_UNIFORM; 4,
		    "_target" : "psyc:" + peeraddr +"/"; 4,
		  ]);
#endif
#endif // }}}
	if (cur = find_target_handler( "psyc://"+ peeraddr +"/" )) {
	    unless (cur->isServer()) {
		cur->takeover();
	    }
	    if (!interactive(cur)) {
		destruct(cur);
	    }
	}
	register_target( "psyc://"+peeraddr+"/" , ME );

	// merge with isserver-if further up? not sure
	// if order of actions has some side fx..
	unless (isserver) greet();
	return 0;
}

#ifndef FORK // edit.i is included into the library. should be enough
#include "edit.i"

// called from sendmsg() either by registered target or psyc: scheme
//int delivermsg(string target, string mc, string data,
varargs int msg(string source, string mc, string data,
    mapping vars, vaint showingLog, vamixed target) {
	string buf, context;
	mixed rc;

#ifdef __TLS__
	P2(( (tls_query_connection_state() ? "TLS": "TCP") +
	    "[%s] <= %s: %s %O\n", peeraddr || "error",
		to_string(source), mc || "-", data))
		// to_string(vars["_source_relay"] || source)
#else
	P2(( "TCP[%s] <= %s: %s %O\n", peeraddr || "error",
		to_string(source), mc || "-", data))
#endif
	buf = "";
#ifndef NEW_RENDER
	ASSERT("mc", mc, "Message from "+source+" w/out mc")
	if (!stringp(data)) {
		if (abbrev("_message", mc)) data = "";
		else {
			data = T(mc, "") || "";
			P3(("fmt from textdb for %O: %O\n", mc, data))
		}
	} else if (data == S_GLYPH_PACKET_DELIMITER || data[0..1] == S_GLYPH_PACKET_DELIMITER "\n"
		     || strstr(data, "\n" S_GLYPH_PACKET_DELIMITER "\n") != -1) {
# if 0 // one day we shall be able to parse that, too
		vars["_length"] = strlen(data);
# else
		P1(("%O: %O tried to send %O via psyc. censored.\n",
		    previous_object() || ME, vars["_nick"] || vars, data))
//		data = "*** censored message ***";
		return 0;
# endif
	}
	// modular message protocol layer
	//
	// this stuff should not be seperate from the one done
	// for UDP!!!  TODO
# if 1
	if (context = vars["_INTERNAL_context"]) {
		P4(("retransmit: %O - deleting source\n", data))
		unless(vars["_source_relay"])
		    vars["_source_relay"] = source;
		// public lastlog and history are sent with _context and _target
		source = 0;
	}
	else if (context = vars["_context"]) {
		P4(("1st transmit: %O - deleting source and target\n", data))
		// we're not multipeering, so no sources here.
		unless(vars["_source_relay"])
		    vars["_source_relay"] = source;
		source = 0;
//		if (vars["_INTERNAL_context"]) context = 0;   // EXPERIMENTAL
//		else {
			// at least we get to see when he does that
//			vars["_INTERNAL_target"] = target;
			// oh he does it a lot currently
			P2(("psycrender removing _target %O for %O in %O\n",
			    target, context, ME))
			// history in fact is a state sync so it
			// should be sent with _context AND _target TODO
			target = 0;
//		}
	}
# else
	context = vars["_context"];
# endif
# ifndef PRE_SPEC
	if (context) {
		buf+= ":_context\t"+ UNIFORM(context) +"\n";
		if (source) buf += ":_source_relay\t"+ UNIFORM(source) +"\n";
		// should it be _target_forward here?
		if (target) buf += ":_target_relay\t"+ target +"\n";
	} else {
		if (source) buf += ":_source\t"+ UNIFORM(source) +"\n";
		if (target) buf += ":_target\t"+ target +"\n";
	}
# else
   // is MMP_STATE a relict of pre-FORK days?
#  if defined(MMP_STATE)
	if (source != lastSource) {
		lastSource = source;
		buf += "=_source\t"+ UNIFORM(source) +"\n";
	}
	if (target != lastTarget) {
		lastTarget = target;
		buf += "=_target\t"+ (target || "") +"\n";
	}
	if (context != lastContext) {
		lastContext = context;
		buf += "=_context\t"+ UNIFORM(context) +"\n";
	}
#  else
	if (source) buf += ":_source\t"+ UNIFORM(source) +"\n";
	if (target) buf += ":_target\t"+ target +"\n";
	if (context) buf+= ":_context\t"+ UNIFORM(context) +"\n";
	if (vars["_source_relay"])
	    buf += "\n:_source_relay\t"+ UNIFORM(vars["_source_relay"]);
#  endif /* MMP_STATE */
# endif /* !PRE_SPEC */
#endif /* !NEW_RENDER */
	rc = psyc_render(source, mc, data, vars, showingLog, target);
	unless (rc) return 0;
	buf += rc;
	P4(("psyc_render %O for %O\n", rc, buf))
#if 0
# ifdef NEW_LINE
	buf += "\n" S_GLYPH_PACKET_DELIMITER "\n";
# else
#  ifdef SPYC
#   echo net/spyc Warning: Erroneous extra newlines will be transmitted.
	buf += "\n" S_GLYPH_PACKET_DELIMITER "\n";
#  else
//#   echo net/psyc Warning: Using inaccurate newline guessing strategy.
	// textdb still provides formats with extra trailing newline.
	// catching this at this point is kind of wrong. it doesn't
	// take into consideration data that intentionally ends with
	// a newline. This is a minor inconvenience, but still.. FIXME
	//
	if (strlen(data)) {
	    PT(("Newline guessing for %O (%O) %O\n", data, char_from_end(data, 1), '\n'))
	}
	if (strlen(data) && char_from_end(data, 1) != '\n') {
		buf += "\n" S_GLYPH_PACKET_DELIMITER "\n";
	} else {
//		PT(("Guessed %O\n", buf))
		buf += S_GLYPH_PACKET_DELIMITER "\n";
	}
#  endif
# endif
#endif
# ifdef _flag_log_sockets_PSYC
	log_file("RAW_PSYC", "« %O\n%s\n", ME, buf);
# endif
	//PT(("» %O\t%s\n", ME, buf))
	return emit(buf);
}
#else /* FORK {{{ */

varargs int msg(string source, string mc, string data,
    mapping vars, int showingLog, mixed target) {
	string buf;
	mapping mvars = copy(vars);

	P2(("TCP[%s] <= %s: %s\n", peeraddr || "error",
		to_string(source), mc || "-"))
		// to_string(vars["_source_relay"] || source)

	// <lynX> yet another place where sources are rendered..
	// but it is no longer compliant to the specs. don't use this.
	vars["_source"] = UNIFORM(source);
	unless (member(vars, "_context"))
	    vars["_target"] = UNIFORM(target);

	// do state only if source is an object.
	if (objectp(source)) buf = make_psyc(mc, data, vars, source);
	else buf = make_psyc(mc, data, vars);

#ifdef _flag_log_sockets_PSYC
	log_file("RAW_PSYC", "« %O\n%s\n", ME, buf);
#endif
	return emit(make_mmp(buf, vars, ME)); 
}

int send(string data, mapping vars) {
#ifdef _flag_log_sockets_PSYC
	log_file("RAW_PSYC", "« %O send(%O)\n", ME, data);
#endif
	return emit(make_mmp(data, vars, ME)); 
}

int outstate(string key, string value) {
	if (member(_o, key)) {
		m_delete(memory, key);
		if (_o[key] == value) return 0;
	}
	return 1;
}

mapping state() {
	mapping t = ([]);
	string key;
	
	foreach (key : memory)
	    t[key] = "";

	memory = copy(_o);
	return t;
}
#endif	// FORK }}}

void reboot(string reason, int restart, int pass) {
	P3(("reboot(%O, %O, %O) in %O\n", pass, restart, reason, ME))
	// this gets called in blueprints too, so we have to make sure
	// we ARE indeed connected somewhere.
	if (pass == 1 && interactive(ME)) {
	    // same in person.c
            if (restart)
                croak("_warning_server_shutdown_temporary",
                  "Server restart: [_reason]", ([ "_reason": reason ]) );
            else
                croak("_warning_server_shutdown",
                  "Server shutdown: [_reason]", ([ "_reason": reason ]) );
	    flags |= TCP_PENDING_DISCONNECT;
	    croak("_request_circuit_shutdown",
		    // the text message is to be removed  ;)
		  "Please close this socket as you read this.");
	}
}

// this is used to issue errors to the other side such as
// _error_invalid_method_compact. it is good to have!
varargs mixed croak(string mc, string data, vamapping vars, vamixed source) {
	PT(("%O croak(%O, %O ..)\n", ME, mc,data))
	return msg(0, mc, data, vars || ([]));
        //return delivermsg(0, mc, data, vars || ([]));
}

int disconnected(string remaining) {
#if DEBUG > 0
	if (remaining && (!stringp(remaining) || strlen(remaining)))
	    PP(("%O ignoring remaining data from socket: %O\n", ME,
		remaining));
#endif
#ifdef _flag_log_sockets_PSYC
	log_file("RAW_PSYC", "%O disconnected.\n", ME);
#endif
	// wow.. a sincerely expected disconnect!
	if (flags & TCP_PENDING_DISCONNECT) return 1;
#ifndef _flag_disable_report_failure_network_circuit_disconnect
	monitor_report("_failure_network_circuit_disconnect",
	    object_name(ME) +" · lost PSYC circuit");
#endif
	return 0;   // unexpected
}

