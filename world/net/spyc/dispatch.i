// included by TCP circuit *and* UDP daemon     // vim:syntax=lpc

volatile mapping routing = shared_memory("routing");

// processes routing header variable assignments
// basic version does no state
mixed process_routing_modifiers(mapping rvars, mapping vars) {
    foreach (mixed vname : m_indices(rvars)) {
	if (!routing[vname]) {
	    DISPATCHERROR("illegal varname in routing header")
	}

	switch (rvars[vname, 1]) {
	    case C_GLYPH_MODIFIER_ASSIGN:
		// TODO: delete if empty?
		if (mappingp(instate))
		    instate[vname] = rvars[vname];
		// fall thru
	    case C_GLYPH_MODIFIER_SET:
		vars[vname] = rvars[vname];
		break;
	    case C_GLYPH_MODIFIER_AUGMENT:
	    case C_GLYPH_MODIFIER_DIMINISH:
	    case C_GLYPH_MODIFIER_QUERY:
		DISPATCHERROR("header modifier with glyph other than ':' or '=', this is not implemented")
		break;
	    default:
		DISPATCHERROR("header modifier with unknown glyph")
		break;
        }
    }

    if (mappingp(instate))
	vars += instate;
    return 1;
}

mixed process_entity_modifiers(mapping evars, mapping vars, mapping cstate) {
    // apply evars to context state
    foreach (mixed vname : m_indices(evars)) {
	if (routing[vname] || abbrev("_INTERNAL", vname)
#ifndef LIBPSYC
	    || !legal_keyword(vname)
#endif
	    ) {
	    DISPATCHERROR("illegal varname in entity header")
	}

	if (!mappingp(cstate) && evars[vname, 1] != C_GLYPH_MODIFIER_SET) {
	    DISPATCHERROR("entity modifier with glyph other than ':' and there's no _context set")
	}

	switch (evars[vname, 1]) { // the glyph
	    case C_GLYPH_MODIFIER_ASSIGN:
		// TODO: delete if empty?
		cstate[vname] = evars[vname];
		// fall thru
	    case C_GLYPH_MODIFIER_SET:
		vars[vname] = evars[vname];
		break;
	    case C_GLYPH_MODIFIER_AUGMENT:
		if (!abbrev("_list", vname)) {
		    DISPATCHERROR("psyc modifier + with non-list arg")
		}
		// FIXME: duplicates?
		cstate[vname] += evars[vname];
		PT(("current state is %O, augment %O\n", cstate[vname], evars[vname]))
		break;
	    case C_GLYPH_MODIFIER_DIMINISH:
		if (!abbrev("_list", vname)) {
		    DISPATCHERROR("psyc modifier + with non-list arg")
		}
		PT(("current state is %O, diminish %O\n", cstate[vname], evars[vname]))
		foreach(mixed item : evars[vname])
		    cstate[vname] -= ({ item });
		PT(("after dim: %O\n", cstate[vname]))
		break;
	    case C_GLYPH_MODIFIER_QUERY:
		DISPATCHERROR("psyc modifier ? not implemented")
	}
    }

    if (mappingp(cstate))
	vars += cstate;
    return 1;
}

mixed process_var_types(mapping evars) {
    string family;
    int glyph;

    // FIXME: i dont like this block... maybe we decode each variable 
    // 		when setting it?
    // 		that would also fit with 0 as varname deletion
    // 		below
    foreach (mixed vname : m_indices(evars)) {
	// psyc type conversion implementation ( http://about.psyc.eu/Type )
	// this does not support register_type() yet, but it is feasible
	PSYC_TRY(vname) {
	case "_uniform":
	case "_page":
	case "_entity":
	    if (!parse_uniform(evars[vname]))
		croak("_error_illegal_uniform");
	    break;
	case "_nick":
	    if (!legal_name(evars[vname]))
		croak("_error_illegal_nick");
	    break;
#ifndef LIBPSYC
	case "_degree":
	    // only honour the first digit
	    if (strlen(evars[vname]) && evars[vname][0] >= '0' && evars[vname][0] <= '9')
		evars[vname] = evars[vname][0] - '0';
	    else {
		PT(("type parser _degree: could not handle value %O\n",
		    evars[vname]))
		evars[vname] = 0;
	    }
	    break;
	case "_date":
	    evars[vname] = to_int(evars[vname]) + PSYC_EPOCH;
	    break;
	case "_time":
	case "_amount":
	    evars[vname] = to_int(evars[vname]);
	    break;
	case "_list":
	    mixed plist = list_parse(evars[vname]);
	    if (plist == -1) {
		DISPATCHERROR("could not parse list");
	    }
	    evars[vname] = plist;
	    break;
#endif
	PSYC_SLICE_AND_REPEAT
	}
    }
    return 1;
}

void dispatch(mapping rvars, mapping evars, mixed method, mixed body) {
    string vname;
    mixed vop; // value operation
    string t;
    mapping vars = ([ ]);

    PT((">> dispatch(%O, %O, %O, %O)\n", rvars, evars, method, body))

    // check that method is a valid keyword
    if (method && !legal_keyword(method)) {
	DISPATCHERROR("non legal method");
    }

    // FIXME: this can happen earlier, e.g. in parse.c after
    // 		process_header
    // check _source/_context
    // this check can be skipped if _source and _context are empty
    if ((t = rvars["_context"] || rvars["_source"])) {
        array(mixed) u;
        unless (u = parse_uniform(t)) {
            DISPATCHERROR("logical source is not a uniform\n")
        }
#ifdef USE_VERIFICATION
# ifdef PSYC_TCP
        unless (qAuthenticated(NAMEPREP(u[UHost]))) {
            DISPATCHERROR("non-authenticated host\n")
        }
# else
        // TODO?
# endif
#endif
    }

    if (!process_routing_modifiers(rvars, vars))
	return;

#ifndef LIBPSYC
    if (!process_var_types(evars))
	return;
#endif

    // check that _target is hosted by us
    // this check can be skipped if _target is not set
    if ((t = vars["_target"])) {
        array(mixed) u;
        unless (u = parse_uniform(t)) {
            DISPATCHERROR("target is not a uniform\n")
        }
	// FIXME relaying support here?
        if (!is_localhost(lower_case(u[UHost]))) {
            DISPATCHERROR("target is not configured on this server\n")
	}
    }

    // FIXME deliver packet
    // this should be a separate function
    PT(("SPYC vars is %O\n", vars))
    PT(("SPYC method %O\nbody %O\n", method, body))
    // delivery rules as usual, but
    if (vars["_context"]) {
	mixed context;
	mixed cstate;
	mixed source, target; 

	if (vars["_source"]) {
	    P0(("invalid _context %O with _source %O\n",
		context, vars["_source"]))
	    DISPATCHERROR("invalid usage of context with _source");
	} 

	context = find_context(vars["_context"]);
	if (!objectp(context)) {
	    P0(("context %O not found?!\n", vars["_context"]))
	    return;
	}

	cstate = context->get_state();
	process_entity_modifiers(evars, vars, cstate);
	// FIXME: is it legal to do this if this has _target?
	// 	there should be no mods then anyway
	// It can have only entity vars, so no _target.
	context->commit_state(cstate);

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
	process_entity_modifiers(evars, vars, 0);

	if (!vars["_target"] && !vars["_source"]) {
#ifdef PSYC_TCP
	    circuit_msg(method, vars, body); 
#else
            P1(("Ignoring a rootMsg from UDP: %O,%O,%O\n", method, vars, body))
#endif
	} else {
	    string source;
	    mixed target;
	    if (!vars["_source"]) {
		// FIXME: where to set netloc in active
		if (!netloc) { // set in sender after _request_features
		    // FIXME: this is wrong
		    DISPATCHERROR("Did you forget to request circuit features?");
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
    ::dispatch(rvars, evars, method, body);
}
