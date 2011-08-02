// vim:foldmethod=marker:syntax=lpc:noexpandtab
// $Id: circuit.c,v 1.38 2008/10/14 19:02:29 lynx Exp $

#include "psyc.h"
#ifdef LIBPSYC

#include <net.h>
#include <uniform.h>
#include <tls.h>
#include <text.h>

inherit NET_PATH "trust";
inherit NET_PATH "spyc/parse";
virtual inherit NET_PATH "output";

volatile string peerhost;
volatile string peeraddr;
volatile string peerip;
volatile int peerport;

volatile string netloc;

#ifndef NEW_RENDER
# define NEW_RENDER
#endif
#include "render.i"

// this is completely anti-psyc. it should take mcs as arguments
// and look up the actual message from textdb.. FIXME
#define CIRCUITERROR(reason) { \
                             croak("_error_circuit", "circuit error: "         \
                                    reason);                                   \
                             return 0;                                         \
                           }
                            
mapping instate = ([ ]);
mapping outstate;

volatile mapping legal_senders;

volatile array(mixed) verify_queue = ({ });

#ifdef __TLS__
volatile mixed certinfo;
#endif

volatile int flags = 0;

void circuit_msg(string mc, mapping vars, string data); // prototype
varargs int msg(string source, string mc, string data,
    mapping vars, int showingLog, mixed target); // prototype
protected void quit();	// prototype
void runQ();

int isServer() { return 0; }

void connection_peek(string data) {
#if __EFUN_DEFINED__(enable_binary)
    enable_binary(ME);
#else
    raise_error("Driver compiled without enable_binary()");
#endif
}

void feed(string data) {
    input_to(#'feed, INPUT_IGNORE_BANG);
    ::feed(data);
}

// yes, this is a funny implementation of croak
// it does not use msg(). Yes, that is intended
varargs mixed croak(string mc, string data, vamapping vars, vamixed source) {
    PT(("croak(%O) in %O (%O)\n", mc, ME, query_ip_name()))
    unless (data) data = T(mc, "");
    binary_message(sprintf("\n%s\n%s\n|\n", mc, data));
    // right behaviour for all croaks!?
    remove_interactive(ME);
//  destruct(ME);
    return 0;
}

// request sender authentication and/or target acknowledgement 
// from the remote side
void sender_verification(string sourcehost, mixed targethost)
{
    unless(interactive()) {
	    verify_queue += ({ ({ sourcehost, targethost }) });
	    return;
    }
    mapping vars = ([ "_uniform_source" : sourcehost,
		    "_uniform_target" : targethost,
		    "_tag" : RANDHEXSTRING ]);
    P0(("sender_verification(%O, %O)\n", sourcehost, targethost))
    // since we send packets to them we should trust them to
    // send packets to us, eh?
    if (stringp(targethost)) {
	    targethost = parse_uniform(targethost);
    }
    sAuthenticated(targethost[UHost]);
    msg(0, "_request_authorization", 0, vars);
}

// gets called during socket logon
int logon(int failure) {
    sAuthHosts(([ ])); // reset authhosts 
    legal_senders = ([ ]);
    instate = ([ "_INTERNAL_origin" : ME ]);
    outstate = ([ ]);
#ifdef __TLS__
    P0(("circuit logon %O %O\n", tls_available(), tls_query_connection_state(ME)))
    // FIXME: needs to handle the not-detected case
    if (tls_available()) {
	    if (tls_query_connection_state(ME) == 0 && !isServer()) {
		    P0(("%O turning on TLS\n", ME))
		    tls_init_connection(ME, #'logon);
		    return 1;
	    } else if (tls_query_connection_state(ME) == 1) {
		    certinfo = tls_certificate(ME, 0);
		    P0(("certinfo: %O\n", certinfo))
	    }
    }
#endif

    peerip = query_ip_number(ME) || "127.0.0.1";

    input_to(#'feed, INPUT_IGNORE_BANG);
    
    call_out(#'quit, 90);
    flags = TCP_PENDING_TIMEOUT;

    parser_init();

    // FIXME
    unless(isServer()) {
	emit("|\n"); // initial greeting
	if (sizeof(verify_queue)) {
	    foreach(mixed t : verify_queue) {
		sender_verification(t[0], t[1]);
	    }
	    verify_queue = ({ });
	}
    }
    return 1;
}

int disconnected(string remaining) {
	// i love to copy+paste source codes! thx for NOT sharing.. grrr
#if DEBUG > 0
	if (remaining && (!stringp(remaining) || strlen(remaining)))
	    PP(("%O ignoring remaining data from socket: %O\n", ME,
		remaining));
#endif
	// wow.. a sincerely expected disconnect!
	if (flags & TCP_PENDING_DISCONNECT) return 1;
#ifdef _flag_enable_report_failure_network_circuit_disconnect
	monitor_report("_failure_network_circuit_disconnect",
	    object_name(ME) +" · lost PSYC circuit");
#else
	P1(("%O disconnected unexpectedly\n", ME))
#endif
        return 0;   // unexpected

}

// respond to the first empty packet
first_response() {
    emit("|\n");
}

#define PSYC_TCP
#include "dispatch.i"

// receives a msg from the remote side
// note: this is circuit-messaging
void circuit_msg(string mc, mapping vars, string data) {
    mapping rv = ([ ]);
    mixed *su;
    mixed *tu;
    switch(mc) {
    case "_request_authorization":
	if (vars["_tag"]) {
		rv["_tag_relay"] = vars["_tag"];
	}
	if (!vars["_uniform_source"] && vars["_uniform_target"]) {
		CIRCUITERROR("_request_authorization without uniform source and/or target?!");
	}

	rv["_uniform_target"] = vars["_uniform_target"];
	rv["_uniform_source"] = vars["_uniform_source"];

	tu = parse_uniform(vars["_uniform_target"]);
	if (!(tu && is_localhost(tu[UHost]))) {
		msg(0, "_error_invalid_uniform_target", "[_uniform_target] is not hosted here.", rv);
		return;
	}
	su = parse_uniform(vars["_uniform_source"]);
	// qAuthenticated does that:u[UHost] = NAMEPREP(u[UHost]);
	if (qAuthenticated(su[UHost])) {
		// possibly different _uniform_target only
		if (flags & TCP_PENDING_TIMEOUT) {
			P0(("removing call out\n"))
					remove_call_out(#'quit);
			flags -= TCP_PENDING_TIMEOUT;
		}
		msg(0, "_status_authorization", 0, rv);
#ifdef __TLS__
	} else if (tls_query_connection_state(ME) == 1 
		   && mappingp(certinfo)
		   && certinfo[0] == 0
		   && certificate_check_name(su[UHost], certinfo, "psyc") == 1) {
		sAuthenticated(su[UHost]);
		if (flags & TCP_PENDING_TIMEOUT) {
			P0(("removing call out\n"))
			remove_call_out(#'quit);
			flags -= TCP_PENDING_TIMEOUT;
		}
		msg(0, "_status_authorization", 0, rv);
#endif
	} else {
		// FIXME: lynX wants to do that only for trusted hosts
		string ho = su[UHost];
		// FIXME: this actually needs to consider srv, too...
		dns_resolve(ho, (: 
				 // FIXME: psyc/parse::deliver is much better here
				 P0(("resolved %O to %O, expecting %O\n", ho, $1, peerip))
				 if ($1 == peerip) {
					sAuthenticated(ho);
					if (flags & TCP_PENDING_TIMEOUT) {
						P0(("removing call out\n"))
						remove_call_out(#'quit);
						flags -= TCP_PENDING_TIMEOUT;
					}
					msg(0, "_status_authorization", 0, rv);
				 } else {
				 	msg(0, "_error_invalid_uniform_source", 0, rv);
				 }
				 return;
				 :));
	}
	break;
    case "_status_authorization":	
	P0(("_status authorization with %O\n", vars))
	// this means we can send from _uniform_source to _uniform_target
	// we already did sAuthenticated _uniform_target before so we can't get
	// tricked into it here
	if (function_exists("runQ")) {
	    runQ(); 
	    // actually runQ(_uniform_source, _uniform_target)
	}
	break;
    default:
	P0(("%O got circuit_msg %O, not implemented\n", ME, mc))
	break;
    }
}

// delivers a message to the remote side
varargs int msg(string source, string mc, string data,
    mapping vars, int showingLog, mixed target) {

    string buf = "";
    mixed u;

    unless(vars) vars = ([ ]);
    buf = render_psyc(source, mc, data, vars, showingLog, target);
#ifdef _flag_log_sockets_SPYC
    log_file("RAW_SPYC", "« %O\n%s\n", ME, buf);
#endif
    return emit(buf);
}

#endif // LIBPSYC
