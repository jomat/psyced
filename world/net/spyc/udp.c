// vim:foldmethod=marker:syntax=lpc:noexpandtab
// $Id: udp.c,v 1.3 2008/02/19 16:28:41 lynx Exp $

#include "psyc.h"
#include <net.h>
#include <url.h>
#include <text.h>

inherit NET_PATH "spyc/parse";

#define CIRCUITERROR(reason) { debug_message("PSYC CIRCUIT ERROR: " reason);   \
                             croak("_error_circuit", "circuit error: "         \
                                    reason);                                   \
                             return;                                           \
                           }

string netloc;

parseUDP2(host, ip, port, msg); // prototype

object load() { return ME; } // avoid a find_object call in obj/master

parseUDP(ip, port, msg) {
    dns_rresolve(ip, #'parseUDP2, ip, port, msg);
}

// respond to the first packet - or don't do it
first_response() { }

parseUDP2(host, ip, port, msg) {
    // this is an atomic operation. It is never interrupted by another 
    // call to parseUDP2. Or at least it is not designed to be.
    unless(stringp(host))
	host = ip; // FIXME: we reject tcp from hosts with invalid pointers
	// but not udp???
    netloc = "psyc://" + host + ":" + to_string(-port) + "/";
    P0(("parseUDP2 from %O == %O  port %O\n", host, ip, port))
    parser_init();
    feed(msg);
}

void done(mixed varops, mixed method, mixed body) {
    string vname;
    mixed vop; // value operation
    mapping vars = ([ ]); // udp is stateless
    string t;

    // FIXME: ideally, those unpacks would happen after the checks
    // 		that get a packet rejected
    // FIXME: this allows binary lists in mmp
    foreach(vname, vop : varops) {
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
    // apply mmp state
    foreach(vname, vop : varops) {
        if (vop[1] == 1) // vname was encountered in psyc part
            continue;
        switch(vop[0]) {
	case C_GLYPH_MODIFIER_SET:
            vars[vname] = vop[2];
            break;
	case C_GLYPH_MODIFIER_ASSIGN:
	case C_GLYPH_MODIFIER_AUGMENT:
	case C_GLYPH_MODIFIER_DIMINISH:
	case C_GLYPH_MODIFIER_QUERY:
            CIRCUITERROR("stateful operation in udp mode")
            break;
        }
	// FIXME: not every legal varname is a mmp varname
	// 	look at shared_memory("routing")
	if (!legal_keyword(vname) || abbrev("_INTERNAL", vname)) {
	    CIRCUITERROR("illegal varname in header")
	}
    }

    // check _source/_context
    // this check can be skipped if _source and _context are empty
    if ((t = vars["_context"] || vars["_source"])) {
        array(mixed) u;
        unless (u = parse_uniform(t)) {
            CIRCUITERROR("logical source is not an uniform\n")
        }
    }
    // check that _target is hosted by us
    // this check can be skipped if _target is empty
    if ((t = vars["_target"])) {
        array(mixed) u;
        unless (u = parse_uniform(t)) {
            CIRCUITERROR("target is not an uniform\n")
        }
	// FIXME relaying support here?
        unless (is_localhost(u[UHost])) {
            CIRCUITERROR("target is not configured on this server\n")
	}
    }
    foreach(vname, vop : varops) {
        if (vop[1] == 0) // vname was encountered in mmp header
            continue; 

        switch(vop[0]) { // the glyph
	case C_GLYPH_MODIFIER_SET:
            vars[vname] = vop[2];
            break;
	case C_GLYPH_MODIFIER_ASSIGN:
	case C_GLYPH_MODIFIER_AUGMENT:
	case C_GLYPH_MODIFIER_DIMINISH:
	case C_GLYPH_MODIFIER_QUERY:
            CIRCUITERROR("stateful operation in udp mode")
            break;
        }
	if (!legal_keyword(vname) || abbrev("_INTERNAL", vname)) {
	    CIRCUITERROR("illegal varname in psyc")
	}
    }
    PT(("vars is %O\n", vars))
    PT(("method %O\nbody %O\n", method, body))
    PT(("packet done\n"))
    // delivery rules as usual, but
    if (vars["_context"]) {
	mixed context;
	mixed source, target; 

	if (vars["_source"] && vars["_target"]) {
	    P0(("invalid _context %O with _source %O, _target %O\n",
		context, vars["_source"], vars["_target"]))
	    CIRCUITERROR("invalid usage of context with _source and _target");
	} else if (vars["_source"]) {
	    P0(("invalid _context %O with _source %O and empty _target\n",
		context, vars["_source"]))
	    CIRCUITERROR("invalid usage of context with _source");
	} else if (vars["_target"]) {
	    // as we don't have psyc e2e state yet...
	    P0(("unicast from context %O to target %O not implemented yet\n",
		context, vars["_target"]))
	    CIRCUITERROR("unicast from context to single member not implemented yet");
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
	    context = find_context(vars["_context"]);
	    if (objectp(context)) {
		// FIXME: we need lots of local object detection here
		context -> castmsg(source, method || "", body, vars); 
	    } else {
		// empty contexts are not bad currently
		// in the current implementation it only means that no one 
		// interested in that context is online right now
	    }
	}
    } else {
	if (!vars["_target"] && !vars["_source"]) {
	    CIRCUITERROR("circuit_msg over udp?");
	} else {
	    string source;
	    mixed target;
	    if (!vars["_source"]) {
		source = netloc; 
	    } else {
		// FIXME: a macro NORMALIZE_UNIFORM that may do lower_case please
		// 		not a simple lower_case
		source = lower_case(vars["_source"]);
	    }

	    // FIXME
	    if (!vars["_target"])
		return;
	    // deliver
	    target = find_psyc_object(parse_uniform(vars["_target"]));
	    PT(("target is %O\n", target))
	    // FIXME: net/entity can not yet deal with 0 method
	    if (objectp(target))
		target->msg(source, method || "", body, vars);
	    else {
		P0(("target %O not found???\n", target))
	    }
	}
    }
    ::done(varops, method, body);
}
