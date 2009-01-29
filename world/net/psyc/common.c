// vim:foldmethod=marker:syntax=lpc:noexpandtab
// $Id: common.c,v 1.67 2008/08/05 12:21:34 lynx Exp $
//
// common code for UDP and TCP server

#include "common.h"
#include <net.h>
#include <url.h>
#include <psyc.h>

#ifdef __MCCP__
# include <telnet.h>
#endif

#ifndef __PIKE__
#ifdef FORK
virtual inherit NET_PATH "state";
#endif
virtual inherit NET_PATH "trust";
#endif

// protos.
varargs mixed croak(string mc, string data, vamapping vars, vamixed source);
void connect_failure(string mc, string reason);
void runQ();

// microspect0rs.
string qScheme() { return "psyc"; }

int isServer() { return 0; }

int greet() { return 0; }

// rootMsg: handles messages to the psyc server root node
// lynX:
// i dont want this to be call_other'd, so i decided to inherit it
// into all psyc servers
// fippo:
// maybe we need a more generic root object to be inherited by
// all psyc and jabber servers 
// it's there: net/root - yet it is not suitable for everything in here

varargs int rootMsg(mixed source, string mc, string data,
	     vamapping vars, vamixed target) {
	string t;
#ifdef SCHWAN
	mixed *u;
#endif
	//PT((">>> %O::rootMsg(%O, %O, %O, %O, %O)\n", ME, source, mc, data, vars, target))
	switch(mc) {
	case "_status_circuit":
		if (member(vars, "_understand_modules")) {
		    unless (pointerp(vars["_understand_modules"]))
			vars["_understand_modules"] = 
			    explode(vars["_understand_modules"], ";");
#ifdef __MCCP__
		    if (member(vars["_understand_modules"], "_compress") != -1) {
			    if(query_mccp(ME) || tls_query_connection_state(ME) > 0) break;				
#ifdef FORK
			    croak("", "", ([ "+_using_modules" : "_compress" ]));
#else
			    croak("", "", ([ "_using_modules" : "_compress" ]));
#endif
			    enable_telnet(0);
			    start_mccp_compress(TELOPT_COMPRESS2);	
		    }
#endif
		}

		break;
	case "_notice_link_established":
	case "_notice_circuit_established":
#if 0	
		// note: module negotation is started by client
		if (pointerp(vars["_understand_modules"]) && !isServer()) {
			string module;
			foreach(module : vars["_understand_modules"]) {
#ifdef __MCCP__
				if (module == "_compress") {
					croak("_request_compression",
				    "Requesting compression using [_method]", 
						([ "_method" : "zlib" ]),
						SERVER_UNIFORM);
					return;
				}
#endif
#ifdef __TLS__
// sigh... we need tls client functionality for that!
				if (module == "_encrypt") {
				    croak("_request_circuit_encrypt",
					"Requesting transport layer security",
					([ ]), SERVER_UNIFORM);
					return;
				}
#endif
			}
		}
#endif
		if (source == SERVER_UNIFORM) {
#ifndef __PIKE__ // TPD
			if (function_exists("connect_failure"))
			    connect_failure("_self",
				"That sender claims to be me!\n");
#endif
			// It may look tempting to do a register_localhost
			// here, but since we didn't make a serious attempt
			// to ensure we are talking to ourselves an outside
			// host may want to fool us into thinking an address
			// is us, then inject messages with localhost trust
			// level. We're not going to do that. For automatic
			// virtual host detection we need to smarten up still!
			PT(("SUICIDE %O\n", ME))
			destruct(ME);
			return 1;
		}
#ifndef __PIKE__ // TPD
		if (function_exists("runQ")) {
			P2(("%s in %O: runQ()\n", mc, ME));
			runQ();
		} else
#endif
	       	{
			P2(("%s in %O: no runQ to execute\n", mc, ME));
		}
#ifdef SCHWAN
		if (vars["_source_verbatim"] 
		    && u = parse_uniform(vars["_source_verbatim"])) {
		    if (u[UHost]) {
			register_host(u[UHost]);
		    }
		}
#endif
		break;
#ifdef GAMMA
	case "_notice_authentication":
		P0(("rootMsg got a _notice_authentication. never happens since entity.c\n"))
		register_location(vars["_location"], source, 1);
		break;
	case "_error_invalid_authentication":
		monitor_report(mc, psyctext("Breach: [_source] reports invalid authentication provided by [_location]", vars, data, source));
		break;
#endif
#if 0
	case "_request_session_compression":
	case "_request_session_compress":
#ifdef __MCCP__
		// dont try to compress an already compressed stream...
		// and compressing an sslified stream is pretty 
		// meaningless 
		if(query_mccp(ME) || tls_query_connection_state(ME) > 0) break;
		// reply with a _status_compression and start compression 
		croak("_notice_circuit_compress", 
			"Will restart with compression using [_method]",
			([ "_method" : vars["_method"] || "zlib" ]),
			SERVER_UNIFORM);
		enable_telnet(0);
		start_mccp_compress(TELOPT_COMPRESS2);
		// greet(); // TODO: calling it directly kills pypsyc 
		call_out(#'greet, 1);
		break;
	case "_notice_circuit_compress":
		// TODO: we have to make sure that we had wanted to do
		// compression to this host
		//start_mccp_compress(TELOPT_COMPRESS2);
		P0(("%O is getting compressed from the other side. ouch!\n", ME))
#else
		croak("_error_unavailable_circuit_compress",
			"Did I really flaunt compression to you?",
			([ ]), SERVER_UNIFORM);
#endif
		break;
	case "_request_circuit_encryption":
	case "_request_circuit_encrypt":
#ifdef __TLS__
		// answer with either _status_tls and go ahead, we are
		// server side of the handshake
		// or reply _error_tls_unavailable
		int t;
		if (t = tls_query_connection_state(ME) == 0) {
			croak("_notice_circuit_encrypt",
				"Enabling TLS encryption.", ([ ]), 
				SERVER_UNIFORM);
			tls_init_connection(ME);
			// here we could actually need lars style 
			// to call greet when ready()
		} else if (t > 0) {
			/* sendmsg(source, "_error_tls_active",
				"TLS is already active",
				([ ]), SERVER_UNIFORM); */
			P0(("received %O for %O who already has TLS\n", mc, ME))
		} else {
			// negative numbers (current behaviour of ldmud)
			// should never make it here
			P0(("negative numbers are impossible here :)\n"))
		}
#else
		// we can not be advertising it
		croak("_error_unavailable_circuit_encrypt",
			"Can not remember telling you I had TLS.",
			([ ]), SERVER_UNIFORM);
#endif
		break;
	case "_notice_circuit_encrypt":
#if __EFUN_DEFINED__(tls_init_connection)
		PT(("is my connection secure %O\n", tls_query_connection_state()))
		// we just leave the socket to verreck, because closing it
		// would probably only incite the other side to reconnect
		tls_init_connection(ME, #'greet);
#endif
		break;
#endif
	case 0: // this is kinda funky!
		if (member(vars, "_using_modules")) {
		    unless (pointerp(vars["_using_modules"]))
			vars["_using_modules"] = ({ vars["_using_modules"] });
		    
#ifdef __TLS__
		    if (member(vars["_using_modules"], "_encrypt") != -1) {
			if (tls_query_connection_state(ME) == 0) {
			    croak("", "", ([ "_using_modules" : "_encrypt" ]));
			    tls_init_connection(ME);
			}	
# ifdef __MCCP__
		    } else
# else
		    } 
# endif
#endif
#ifdef __MCCP__
		    if (member(vars["_using_modules"], "_compress") != -1) {
			P0(("%O is getting compressed from the other side."
			    "ouch! Closing Connection!\n", ME))
			croak("_error_compress_unavailable",
			  "I don't want to receive compressed data.");
			destruct(ME);
		    }
#endif
		    
		}
		break;
	default:
//		sendmsg(source, "_error_unsupported_method",
//			"Don't know what to do with '[_method]'.",
//			([ "_method" : mc ]), "");
		P1(("rootMsg(%O,%O,%O,%O,%O)\n", source,mc,data,vars,target))
#if 0 //def EXPERIMENTAL
		// circuit root wasn't successful, let's pass the message
		// on to the server root entity
		return sendmsg("/", mc, data, vars, source);
#else
		t = "Circuit got "+ mc;
		if (target) t += " to "+ target;
		unless (source) source = vars["_INTERNAL_source"]
				      || vars["_INTERNAL_origin"];
		if (source) t += " from "+ to_string(source);
		monitor_report("_notice_forward"+ mc, psyctext(data ?
				 t+": "+data : t, vars, "", source));
		return 0;
#endif
	}
	return 1;
}

void internalError() {
	write("_failure_broken_parser\n.\n\n");
#ifndef __PIKE__    // TPD
	// reicht das schon, oder müssen wir weitere maßnahmen ergreifen?
	remove_interactive(ME);
#endif
}

#ifdef FORK // {{{
void Assign(mixed source, string key, mixed value) {
}
void negotiate(mixed modules) {
#ifdef __TLS__
    unless (pointerp(modules))
	modules = ({ modules });
    foreach (string key : modules) {
	if ("_encrypt" == key) {
	    if (tls_query_connection_state(ME) == 0) {
		
		croak("", "", "", ([ "+_using_modules" : "_encrypt", "_target" : ""  ]));
		
		tls_init_connection(ME);
	    }
	    return;
	}
#ifdef __MCCP__
	if ("_compress" == key) {
	    P0(("%O is getting compressed from the other side."
		"ouch! Closing Connection!\n", ME))
	    croak("_error_compress_unavailable",
	      "I don't want to receive compressed data.");
	    destruct(ME);
	    return;
	}
#endif
    }
#endif
}

void gotiate(mixed modules) {
    unless (pointerp(modules))
	modules = ({ modules });
    foreach (string key : modules) {
#ifdef __MCCP__
	if ("_compress" == key) {
	    if(query_mccp(ME) 
#ifdef __TLS__
	       || tls_query_connection_state(ME) > 0
#endif
	       ) break;				
	    croak("", "", "", ([ "+_using_modules" : "_compress" ]));
	    enable_telnet(0);
	    start_mccp_compress(TELOPT_COMPRESS2);	
	}
	return;
#endif
    }
}

void Diminish(mixed source, string key, mixed value) {
}
void Reset() {
    
}

#endif // FORK }}}
