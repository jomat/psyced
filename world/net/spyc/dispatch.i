// included by TCP circuit *and* UDP daemon     // vim:syntax=lpc

void dispatch(mixed header_vars, mixed varops, mixed method, mixed body) {
    string vname;
    mixed vop; // value operation
    string t;
    mapping vars;
    string family;
    int glyph;

    // check that method is a valid keyword
    if (method && !legal_keyword(method)) {
	DISPATCHERROR("non legal method");
    }
#ifdef PSYC_TCP
    // copy() + occasional double modifier ops should be more
    // efficient than merge at every packet --lynX
    // no one cares about "efficiency" here. please proof your 
    // bold statements with benchmarks anyway
    vars = header_vars + instate;
#else
    vars = header_vars;
#endif

    // FIXME: this can happen earlier, e.g. in parse.c after
    // 		process_header
    // check _source/_context
    // this check can be skipped if _source and _context are empty
    if ((t = vars["_context"] || vars["_source"])) {
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
    // check that _target is hosted by us
    // this check can be skipped if _target is not set 
    if ((t = vars["_target"])) {
        array(mixed) u;
        unless (u = parse_uniform(t)) {
            DISPATCHERROR("target is not an uniform\n")
        }
	// FIXME relaying support here?
        if (!is_localhost(u[UHost])) {
            DISPATCHERROR("target is not configured on this server\n")
	}
    }
    // FIXME: i dont like this block... maybe we decode each variable 
    // 		when setting it?
    // 		that would also fit with 0 as varname deletion
    // 		below
    foreach(vop : varops) {
	vname = vop[0];

	// psyc type conversion implementation ( http://about.psyc.eu/Type )
	// this does not support register_type() yet, but it is feasible
	PSYC_TRY(vname) {
	case "_uniform":
	case "_page":
	case "_entity":
	    // TODO: check legal uniform
	    break;
	case "_nick":
	    // TODO: check legal nick
	    break;
	case "_degree":
	    // only honour the first digit
	    if (strlen(vop[2]) && vop[2][0] >= '0' && vop[2][0] <= '9')
		vop[2] = vop[2][0] - '0';
	    else {
		PT(("type parser _degree: could not handle value %O\n",
		    vop[2]))
		vop[2] = 0;
	    }
	    break;
	case "_time":
	case "_amount":
	    vop[2] = to_int(vop[2]);
	    break;
	case "_list":
	    mixed plist = list_parse(vop[2]);
	    if (plist == -1) {
		DISPATCHERROR("could not parse list");
	    }
	    vop[2] = plist;
	    break;
	PSYC_SLICE_AND_REPEAT
	}
    }

    // FIXME deliver packet
    // this should be a separate function
    PT(("SPYC vars is %O\n", vars))
    PT(("SPYC method %O\nbody %O\n", method, body))
    // delivery rules as usual, but
    if (vars["_context"]) {
	mixed context;
	mixed context_state;
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
	context_state = context->get_state();

	// apply varops to context state
	foreach(vop : varops) {
	    vname = vop[0];
	    if (!legal_keyword(vname) || abbrev("_INTERNAL", vname)) {
		DISPATCHERROR("illegal varname in psyc")
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
		    DISPATCHERROR("psyc modifier + with non-list arg")
		}
		// FIXME: duplicates?
		context_state[vname] += vop[2];
		PT(("current state is %O, augment %O\n", context_state[vname], vop[2]))
		break;
	    case C_GLYPH_MODIFIER_DIMINISH:
		if (!abbrev("_list", vname)) {
		    DISPATCHERROR("psyc modifier + with non-list arg")
		}
		PT(("current state is %O, diminish %O\n", context_state[vname], vop[2]))
		foreach(mixed item : vop[2])
		    context_state[vname] -= ({ item });
		PT(("after dim: %O\n", context_state[vname]))
		break;
	    case C_GLYPH_MODIFIER_QUERY:
		DISPATCHERROR("psyc modifier ? not implemented")
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
    ::dispatch(header_vars, varops, method, body);
}

