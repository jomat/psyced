// vim:foldmethod=marker:syntax=lpc:noexpandtab
// $Id: circuit.c,v 1.33 2008/03/29 16:00:02 fippo Exp $

#include "psyc.h"
#include <net.h>
#include <url.h>
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
#include "edit.i"

// this is completely anti-psyc. it should take mcs as arguments
// and look up the actual message from textdb.. FIXME
#define CIRCUITERROR(reason) { debug_message("PSYC CIRCUIT ERROR: " reason);   \
                             croak("_error_circuit", "circuit error: "         \
                                    reason);                                   \
                             return 0;                                         \
                           }
                            
mapping instate;
mapping outstate;

mapping legal_senders;


volatile int flags = 0;

void circuit_msg(string mc, mapping vars, string data); // prototype
varargs int msg(string source, string mc, string data,
    mapping vars, int showingLog, mixed target); // prototype
protected void quit();	// prototype
void runQ();

int isServer() { return 0; }

void feed(string data) {
    input_to(#'feed, INPUT_IGNORE_BANG);
    ::feed(data);
}

// yes, this is a funny implementation of croak
// it does not use msg(). Yes, that is intended
varargs mixed croak(string mc, string data, vamapping vars, vamixed source) {
    binary_message(sprintf("\n%s\n%s\n|\n", mc, data));
    remove_interactive(ME); 
    destruct(ME);
    return 0;
}

// gets called during socket logon
int logon(int success) {
    sAuthHosts(([ ])); // reset authhosts 
    legal_senders = ([ ]);
    instate = ([ "_INTERNAL_origin" : ME]);
    outstate = ([ ]);
#ifdef __TLS__
    mixed cert;
    if (tls_available() && tls_query_connection_state(ME) == 1 && mappingp(cert = tls_certificate(ME, 0))) {
	mixed m, t;
	if (cert[0] != 0) {
	    // log error 17 + cert here
	    // and goodbye.
	    P0(("%O encountered a cert verify error %O in %O\n", ME,
		cert[0], cert))
	    remove_interactive(ME);
	    return 0;
	}
	if (m = cert["2.5.29.17:dNSName"]) {
	    // FIXME: this does not yet handle wildcard DNS names
	    P1(("%O believing dNSName %O\n", ME, m))
	    // probably also: register_target?
	    // but be careful never to register_target wildcards
	    if (stringp(m)) 
		sAuthenticated(NAMEPREP(m));
	    else 
		foreach(t : m) 
		    sAuthenticated(NAMEPREP(t));
	}
//#ifdef _flag_allow_certificate_name_common	// to be switched this year
#ifndef _flag_disallow_certificate_name_common
	// assume that CN is a host
	// as this is an assumption only, we may NEVER register_target it
	// note: CN is deprecated for good reasons.
	else if (t = cert["2.5.4.3"]) {
	    P1(("%O believing CN %O\n", ME, t))
	    sAuthenticated(NAMEPREP(t));
	}
#endif
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

    peerip = query_ip_number(ME) || "127.0.0.1";

    enable_binary(ME);
    input_to(#'feed, INPUT_IGNORE_BANG);
    
    call_out(#'quit, 90);
    flags = TCP_PENDING_TIMEOUT;

    parser_init();

    // FIXME
    unless(isServer()) {
	emit("|\n"); // initial greeting
	msg(0, "_request_features", 0);
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
	monitor_report("_failure_network_circuit_disconnect",
	    object_name(ME) +" · lost PSYC circuit");
	return 0;   // unexpected
}

// respond to the first empty packet
first_response() {
    emit("|\n");
}

// processes routing header variable assignments
// basic version does no state
mapping process_header(mixed varops) {
    mapping vars = ([ ]);
    foreach(mixed vop : varops) {
	string vname = vop[0];
        switch(vop[1]) {
	case C_GLYPH_MODIFIER_ASSIGN:
            instate[vname] = vop[2];
	    // fall thru
	case C_GLYPH_MODIFIER_SET:
            vars[vname] = vop[2];
            break;
	case C_GLYPH_MODIFIER_AUGMENT:
	case C_GLYPH_MODIFIER_DIMINISH:
	case C_GLYPH_MODIFIER_QUERY:
            CIRCUITERROR("header modifier with glyph other than ':', this is not implemented")
            break;
	default:
            CIRCUITERROR("header modifier with unknown glyph")
	    break;
        }
	// FIXME: not every legal varname is a mmp varname
	// 	look at shared_memory("routing")
	if (!legal_keyword(vname) || abbrev("_INTERNAL", vname)) {
	    CIRCUITERROR("illegal varname in header")
	}
    }
    return vars;
}

// handling of packets received
void done(mixed header_vars, mixed varops, mixed method, mixed body) {
    string vname;
    mixed vop; // value operation
    mapping vars;
    string t;

    // check that method is a valid keyword
    if (method && !legal_keyword(method)) {
	CIRCUITERROR("non legal method");
    }
    // copy() + occasional double modifier ops should be more
    // efficient than merge at every packet --lynX
    // no one cares about "efficiency" here. please proof your 
    // bold statements with benchmarks anyway
    vars = header_vars + instate;

    // FIXME: this can happen earlier, e.g. in parse.c after
    // 		process_header
    // check _source/_context
    // this check can be skipped if _source and _context are empty
    if ((t = vars["_context"] || vars["_source"])) {
        array(mixed) u;
        unless (u = parse_uniform(t)) {
            CIRCUITERROR("logical source is not an uniform\n")
        }
        unless (qAuthenticated(NAMEPREP(u[UHost]))) {
            CIRCUITERROR("non-authenticated host\n")
        }
    }
    // check that _target is hosted by us
    // this check can be skipped if _target is not set 
    if ((t = vars["_target"])) {
        array(mixed) u;
        unless (u = parse_uniform(t)) {
            CIRCUITERROR("target is not an uniform\n")
        }
	// FIXME relaying support here?
        if (!(is_localhost(u[UHost])) || u[UHost] == "localhost") {
            CIRCUITERROR("target is not configured on this server\n")
	}
    }
    // FIXME: i dont like this block... maybe we decode each variable 
    // 		when setting it?
    // 		that would also fit with 0 as varname deletion
    // 		below
    foreach(vop : varops) {
	vname = vop[0];
	// TODO unpack _amount
	// TODO unpack _time
	if (abbrev("_list", vname)) {
	    mixed plist = list_parse(vop[2]);
	    if (plist == -1) {
		CIRCUITERROR("could not parse list");
	    }
	    vop[2] = plist;
	}
    }

    // FIXME deliver packet
    // this should be a separate function
    PT(("vars is %O\n", vars))
    PT(("method %O\nbody %O\n", method, body))
    PT(("packet done\n"))
    // delivery rules as usual, but
    if (vars["_context"]) {
	mixed context;
	mixed context_state;
	mixed source, target; 

	if (vars["_source"]) {
	    P0(("invalid _context %O with _source %O\n",
		context, vars["_source"]))
	    CIRCUITERROR("invalid usage of context with _source");
	} 

	context = find_context(vars["_context"]);
	if (!objectp(context)) {
	    P0(("context %O not found?!\n", vars["_context"]))
	    return;
	}
	context_state = context->get_state();

	// apply varops to context state
	foreach(vop : varops) {
	    vname = vop[0];
	    if (!legal_keyword(vname) || abbrev("_INTERNAL", vname)) {
		CIRCUITERROR("illegal varname in psyc")
	    }

	    switch(vop[1]) { // the glyph
	    case C_GLYPH_MODIFIER_SET:
		vars[vname] = vop[2];
		break;
	    case C_GLYPH_MODIFIER_ASSIGN:
		vars[vname] = context_state[vname] = vop[2];
		break;
	    case C_GLYPH_MODIFIER_AUGMENT:
		if (!abbrev("_list", vname)) {
		    CIRCUITERROR("psyc modifier + with non-list arg")
		}
		// FIXME: duplicates?
		context_state[vname] += vop[2];
		PT(("current state is %O, augment %O\n", context_state[vname], vop[2]))
		break;
	    case C_GLYPH_MODIFIER_DIMINISH:
		if (!abbrev("_list", vname)) {
		    CIRCUITERROR("psyc modifier + with non-list arg")
		}
		PT(("current state is %O, diminish %O\n", context_state[vname], vop[2]))
		foreach(mixed item : vop[2])
		    context_state[vname] -= ({ item });
		PT(("after dim: %O\n", context_state[vname]))
		break;
	    case C_GLYPH_MODIFIER_QUERY:
		CIRCUITERROR("psyc modifier ? not implemented")
		break;
	    }
	}
	vars = vars + context_state;
	// FIXME: is it legal to do this if this has _target?
	// 	there should be no mods then anyway
	context->commit_state(context_state);


	if (vars["_target"]) {
	    // FIXME: delivery copycat from below
	    // beware: source is set to 0 here as it may not be present
	    target = find_psyc_object(parse_uniform(vars["_target"]));
	    PT(("target is %O\n", target))
	    // FIXME: net/entity can not yet deal with 0 method
	    //
	    if (objectp(context)) {
		context->msg(0, method || "", body, vars, 0, target);
	    } else {
		// FIXME: proper croak back to sender here
		P0(("context %O for unicast to %O not found???\n", target))
	    }
	} else {
	    if (vars["_source_relay"]) {
		mixed localrelay;
		if ((localrelay = psyc_object(vars["_source_relay"]))) {
		    P0(("local relay %O\n", localrelay))
		    vars["_source_relay"] = localrelay;
		} else { // NORMALIZE UNIFORM
		    vars["_source_relay"] = lower_case(vars["_source_relay"]);
		}
	    }
	    if (objectp(context)) {
		// do we need more local object detection here?
		context -> castmsg(source, method || "", body, vars); 
	    } else {
		// empty contexts are not bad currently
		// in the current implementation it only means that no one 
		// interested in that context is online right now
		// FIXME: lines above are about the old stuff where we did
		// 	not have context state
	    }
	}
    } else {
	if (!vars["_target"] && !vars["_source"]) {
	    circuit_msg(method, vars, body); 
	} else {
	    string source;
	    mixed target;
	    if (!vars["_source"]) {
		// FIXME: where to set netloc in active
		if (!netloc) { // set in sender after _request_features
		    // FIXME: this is wrong
		    CIRCUITERROR("Did you forget to request circuit features?");
		}
		source = netloc; 
	    } else {
		// FIXME: a macro NORMALIZE_UNIFORM that may do lower_case please
		// 		not a simple lower_case
		source = lower_case(vars["_source"]);
	    }
	    // source was checked either via x509 or dns before
	    // so it is 'safe' to do this
	    register_target(source);


	    // deliver FIXME same code above
	    if (!vars["_target"]) {
		target = find_object(NET_PATH "root");
	    } else {
		target = find_psyc_object(parse_uniform(vars["_target"]));
	    }
	    PT(("target is %O\n", target))
	    // FIXME: net/entity can not yet deal with 0 method
	    if (objectp(target))
		target->msg(source, method || "", body, vars);
	    else {
		// FIXME: proper croak back to sender here
		P0(("target %O not found???\n", target))
	    }
	}
    }
    ::done(header_vars, varops, method, body);
}

// request sender authentication and/or target acknowledgement 
// from the remote side
void sender_verification(array(string) sourcehosts, array(string) targethosts)
{
    // FIXME: wrong variables here
    mapping vars = ([ "_list_sources_hosts" : sourcehosts,
		    "_list_targets_hosts" : targethosts,
		    "_tag" : RANDHEXSTRING ]);
    // assumption: we have already resolved all targethosts and 
    // they point to the remote ip
    foreach(string ho : targethosts) {
	sAuthenticated(ho);
    }

    msg(0, "_request_verification", 0, vars);
}

// receives a msg from the remote side
// note: this is circuit-messaging
void circuit_msg(string mc, mapping vars, string data) {
    switch(mc) {
    case "_request_verification":
	if (tls_query_connection_state(ME) == 0) {
	    array(string) targethosts = ({ });
	    foreach(string ho : vars["_list_targets_hosts"]) {
		if (is_localhost(ho)) {
		    targethosts += ({ ho });
		}
	    }
	    if (sizeof(vars["_list_sources_hosts"]) == 1) {
		// doing multiple resolutions in parallel is more complicated
		string ho = vars["_list_sources_hosts"][0];
		if (qAuthenticated(ho)) {
		    P0(("warning: trying to reverify authenticated host %O",ho))
		} else {
		    dns_resolve(ho, (: 
			// FIXME: psyc/parse::deliver is much better here
			mixed rv = (["_list_targets_accepted_hosts":targethosts]);

			if (vars["_tag"]) rv["_tag_reply"] = vars["_tag"];
			if ($1 == peerip) {
			    sAuthenticated(NAMEPREP(ho));
			    rv["_list_sources_verified_hosts"] = ({ ho });
			} else {
			    rv["_list_sources_rejected_hosts"] = ({ ho });
			}
			msg(0, "_notice_verification", 0, rv);
			return;
		    :));
		}
	    } else {
		// FIXME!!!!
		CIRCUITERROR("sorry, no more than one element in _list_sources_hosts currently");
	    }
	    // keep tag if present!!!
	    // resolve all of _list_sources_hosts
	    // look at _list_targets_hosts and determine localhostiness
	} else {
	    CIRCUITERROR("_request_verification is not allowed on TLS circuits.");
	    // _request_verification is not allowed on tls circuits
	}
	break;
    case "_notice_features":
	// FIXME: watch for _list_using_modules
	if (flags & TCP_PENDING_TIMEOUT) {
	    P0(("removing call out\n"))
	    remove_call_out(#'quit);
	    flags -= TCP_PENDING_TIMEOUT;
	}
	sTextPath();
	if (tls_query_connection_state(ME) == 0) {
	    // start hostname verification
	    // rather: look at Q and look for the hostnames we need
	    sender_verification(({ SERVER_HOST }), ({ peerhost }));
	} else {
	    if (function_exists("runQ")) {
		runQ();
	    }
	}
	break;
    case "_notice_verification":	
	P0(("_notice verification with %O\n", vars))
	if (function_exists("runQ")) {
	    runQ();
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

    unless(vars) vars = ([ ]);
    buf = psyc_render(source, mc, data, vars, showingLog, target);
#ifdef _flag_log_sockets_PSYC
    log_file("RAW_PSYC", "» %O\n%s\n", ME, buf);
#endif
    return emit(buf);
}
