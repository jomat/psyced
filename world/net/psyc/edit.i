// vim:foldmethod=marker:syntax=lpc:noexpandtab
// $Id: edit.i,v 1.111 2008/12/01 11:31:33 lynx Exp $
//
// headermaker functions. we should call it render rather than edit.

volatile mapping isRouting = shared_memory("routing");

#ifdef FORK
// these macros support state modifiers in varnames.. we'll need that later
#define isRoutingVar(x) (stringp(x) && strlen(x) > 1 && member(isRouting, (x[0] == '_') ? x : x[1..]))
#define mergeRoutingVar(x) (stringp(x) && strlen(x) > 1 && (isRouting[(x[0] == '_') ? x : x[1..]] & PSYC_ROUTING_MERGE)
#endif

volatile string rbuf, ebuf;  // pike has no pass-by-reference

static int build_header(string key, mixed val, mapping vars) {
        string s, klopp;
	int routeMe = 0;
#ifdef SPYC
	int needLen = 0;
#endif

	unless (stringp(key)) {
	    s = sprintf("%O encountered key %O value %O (rbuf %O ebuf %O)\n",
		ME, key, val, rbuf, ebuf);
	    log_file("PANIC", s);
	    monitor_report("_failure_invalid_variable_name", s);
	    return -9;
	}
#ifndef FORK
	routeMe = isRouting[key];
# if 1 //def EXPERIMENTAL
	P3(("isRouting[%O] = %O, render? %O\n",
	    key, routeMe, routeMe & PSYC_ROUTING_RENDER))
	if ((routeMe &&! (routeMe & PSYC_ROUTING_RENDER))
	   || abbrev("_INTERNAL", key)) return -1;
# else
	if (routeMe || abbrev("_INTERNAL", key)) return -1;
# endif
#endif /* ! FORK */
	P2(("build_header(%O, %O) into %s vars\n", key, val,
	    routeMe ? "routing" : "entity"))
	if (key[0] == '_') key = ":"+key;
	if (objectp(val)) klopp = psyc_name(val);
	else if (stringp(val)) {
            if (key == "_nick" && (s = vars["_INTERNAL_nick_plain"])) {
                    P1(("%O sending %O instead of %O\n", ME, s, val))
                    klopp = s;
            } else {
// yeah but we haven't implement the other stuff yet
// and the parser can handle it
#if 0 //def EXPERIMENTAL
                    // according to http://about.psyc.eu/Grammar
                    if (index(val, '\n') >=0)
                        raise_error("Multiline variables no longer permitted.\n");
#else
                    // indenting of multiline variables ...
                    // hardly ever happens, that's why indexing first is
                    // more efficient
# ifdef SPYC
		    // better even to not look into the variable but we
		    // need to implement types for that, first
                    if (index(val, '\n') >=0) {
			needLen = 1;
		    }
		    klopp = val;
# else
                    if (index(val, '\n') >=0)
                        klopp = replace(val, "\n", "\n\t");
		    else
			klopp = val;
# endif
#endif
            }
	} else if (intp(val)) {
#ifdef SPYC
	    klopp = abbrev("_date", key) ? to_string(val - PSYC_EPOCH)
					 : to_string(val);
#else
	    klopp = to_string(val);
#endif
	} else {
            mixed k,d, sep;

            // using list syntax here without testing for
            // _list in name.. bad?

            // fippo likes this variant, but it gives + a wrong
            // meaning (non-persistent!)
            // also psyc/parse cannot parse this, and shouldnt start
            //
#ifndef SPYC
            //sep = key[0] == '_' ? "\n+\t" : "\n"+key[0..]+"\t";
            sep = "\n"+key[0..0]+"\t";
            //P2(("sep %O, key[0] %O\n", sep, key[0]))
#else
	    needLen = 1;	// is it always necessary?
	    // no, but it does not hurt much either
#endif
            if (pointerp(val)) {
		klopp = "";
		each(d, val) {
#ifndef SPYC
                    // good-old "simple" http://about.psyc.eu/List syntax
                    klopp += sep + UNIFORM(d);
#else
                    k = objectp(d)? psyc_name(d): to_string(d);
		    klopp += strlen(k) +" "+ k + "|";
#endif
		}
#ifdef SPYC
		klopp = klopp[..<2];
#endif
            }
            else if (mappingp(val)) {
#if 1
                raise_error("Mappings currently not transmittable.\n");
#else
		klopp = "";
                mapeach(k, d, val) {
                    // inofficial table syntax
                    klopp += sep + UNIFORM(k);
                    if (d) klopp += " " + UNIFORM(d);
                }
#endif
	    }
            // it could lead to problems if vars is reused since val may
            // be an array .. pointer ... so we put stuff in klopp instead
#ifndef SPYC
            klopp = strlen(klopp) > 3 ? klopp[3..] : "";
//		if (sizeof(val) && stringp(val[0])) val = implode(val, " , ");
//		else return;
#endif
	}
#ifndef SPYC
	if (klopp) klopp = "\n"+ key +"\t"+ klopp;
	else klopp = "\n"+ key;
#else
	if (klopp) klopp = "\n"+ key +
		   (needLen? (" "+strlen(klopp)+"\t") : "\t") + klopp;
	else klopp = "\n"+ key +"\t";            // with or without TAB here?
#endif
#if 1 //def EXPERIMENTAL
	if (routeMe) rbuf += klopp;
	else ebuf += klopp;
#else
	ebuf += klopp;
#endif
	P4(("build_header (%O, %O) rbuf %O\n", key, val, rbuf))
	return 0;
}

static varargs string psyc_render(mixed source, string mc, mixed data,
			      mapping vars, int showingLog, vastring target) {
		  // vaobject obj, vastring target, vaint hascontext)
	P4(("%O psyc_render %O for %O\n", ME, vars, previous_object()))
	string t, context;
	int needLen = 0;
#ifndef NEW_LINE
	int excessiveNewline = 0;
#endif

	rbuf = ebuf = "";
#ifdef NEW_RENDER
	ASSERT("mc", mc, "Message from "+ to_string(source) +" w/out mc")
	if (!stringp(data)) {
#ifdef T // what lynX wants to say here: do we have the textdb
		if (abbrev("_message", mc)) data = "";
		else {
			data = T(mc, "") || "";
			P3(("edit: fmt from textdb for %O: %O\n", mc, data))
			if (strlen(data) && char_from_end(data, 1) == '\n')
			    excessiveNewline = 1;
		}
#else
		PT(("non-string data: %O\n", data))
		data = data? to_string(data): "";
#endif
	}
	else if (data == S_GLYPH_PACKET_DELIMITER ||
		  (data[0] == C_GLYPH_PACKET_DELIMITER && data[1] == '\n')
		    || strstr(data, "\n" S_GLYPH_PACKET_DELIMITER "\n") != -1) {
		// this check shouldn't be necessary here: we should check what
		// people are typing in usercmd
# ifdef SPYC
		needLen++;
# else
		P1(("%O: %O tried to send %O via psyc. censored.\n",
		    previous_object() || ME, vars["_nick"] || vars, data))
		data = "*** censored message ***";
		return 0;
# endif         
# ifndef NEW_LINE
	} else
#  ifdef SPYC
	    if (!needLen)
#  endif
	{
//#   echo net/psyc Warning: Using inaccurate newline guessing strategy.
		// textdb still provides formats with extra trailing newline.
		// catching this at this point is kind of wrong. it doesn't
		// take into consideration data that intentionally ends with
		// a newline. This is a minor inconvenience, but still.. FIXME
		//
		P4(("Newline guessing for %O (%O) %O\n", data,
		    char_from_end(data, 1), '\n'))
		if (strlen(data) && char_from_end(data, 1) == '\n')
		    excessiveNewline = 1;
# endif
	}
# if 1
	if (context = vars["_INTERNAL_context"]) {
		P4(("retransmit: %O - deleting source\n", data))
# ifdef ALPHA
		if (source != context && !vars["_source_relay"])
		    vars["_source_relay"] = source;
# else
		unless(vars["_source_relay"])
		    vars["_source_relay"] = source;
# endif
		// public lastlog and history are sent with _context and _target
		source = 0;
	}       
	else if (context = vars["_context"]) {
		P4(("1st transmit: %O - deleting source and target\n", data))
		// we're not multipeering, so no sources here.
# ifdef ALPHA
		if (source != context && !vars["_source_relay"])
		    vars["_source_relay"] = source;
# else
		unless(vars["_source_relay"])
		    vars["_source_relay"] = source;
# endif
		source = 0;
//              if (vars["_INTERNAL_context"]) context = 0;   // EXPERIMENTAL
//              else {
			// at least we get to see when he does that
//                      vars["_INTERNAL_target"] = target;
			// oh he does it a lot currently
			P2(("psycrender removing _target %O for %O in %O\n",
			    target, context, ME))
			// history in fact is a state sync so it
			// should be sent with _context AND _target TODO
			target = 0;
//              }       
	}
# else  
	context = vars["_context"];
# endif 
# ifndef PRE_SPEC
	if (context) {
		rbuf += "\n:_context\t"+ UNIFORM(context);
		t = source || vars["_source_relay"];
		if (t) rbuf += "\n:_source_relay\t"+ UNIFORM(t);
		// resend of /history transmitted according to spec:
		if (showingLog && target) rbuf += "\n:_target\t"+ target;
		// usually the same as context or a different channel of
		// context or the actual recipient of a multicast
		// ... not interesting in any case
		//else if (target) rbuf += "\n:_target_relay\t"+ target;
	} else {
		if (source) rbuf += "\n:_source\t"+ UNIFORM(source);
		if (target) rbuf += "\n:_target\t"+ target;
		// this is necessary for message forwarding ( /set id )
		if (t = vars["_source_relay"])
		    rbuf += "\n:_source_relay\t"+ UNIFORM(t);
	}
# else
	if (source) rbuf += "\n:_source\t"+ UNIFORM(source);
	if (target) rbuf += "\n:_target\t"+ target;
	if (context) rbuf+= "\n:_context\t"+ UNIFORM(context);
	if (t = vars["_source_relay"])
	    rbuf += "\n:_source_relay\t"+ UNIFORM(t);
# endif /* PRE_SPEC */
#endif /* NEW_RENDER */

	if (mappingp(vars)) {
#if 0 //ndef EXPERIMENTAL
		if (member(vars, "_count"))
		    ebuf += "\n:_count\t" + vars["_count"];
#endif
#if __EFUN_DEFINED__(walk_mapping)
#ifndef FORK // NO STATE
		// walk_mapping could be rewritten into foreach, but thats work
		walk_mapping(vars, #'build_header, vars);
#else /* FORK {{{ */
# if 0 //ndef EXPERIMENTAL
		// At least the psyced needs _count first to handle that properly
		m_delete(vars, "_count");
# endif
		foreach (string key, mixed value : vars) {
                    // why does FORK weed out _INTERNAL in two places? ah nevermind
		    if (abbrev("_INTERNAL_", key)) {
			// equal on a line by itself to mean "clear all state"
			if (key == "_INTERNAL_state_clear")
			    ebuf = "\n="+ ebuf;
			continue;
		    }

		    // CONTEXT_STATE and ENTITY_STATE here
		    if (!isRoutingVar(key) && (!objectp(obj) 
			    || obj->outstate(target, key, value, hascontext)))
			build_header(key, value);
		}
		if (objectp(obj))
		    walk_mapping(obj->state(target, hascontext), #'build_header);
#endif /* FORK }}} */
#else                            // PIKE, MudOS...
                mixed key, val;

		mapeach(key, val, vars) {
                        build_header(key, val, vars);
                }
#endif
	}
        if (data == "") ebuf += "\n"+ mc;
	else ebuf += "\n"+ mc + "\n"+ data;

#ifdef SPYC 	// || MODULE_LENGTH
	if (needLen || strlen(ebuf) + strlen(rbuf) > 555)
	    return ":_length\t"+ strlen(ebuf) + rbuf +"\n"+
		ebuf +"\n" S_GLYPH_PACKET_DELIMITER "\n";
	else
#endif
#ifndef NEW_LINE
	if (excessiveNewline) return rbuf[1 ..] +"\n"+
		ebuf + S_GLYPH_PACKET_DELIMITER "\n";
	else
#endif
	if (strlen(rbuf)) return rbuf[1 ..] +"\n"+
		ebuf +"\n" S_GLYPH_PACKET_DELIMITER "\n";
	return	ebuf +"\n" S_GLYPH_PACKET_DELIMITER "\n";
}

#ifdef FORK // {{{

static varargs string mmp_make_header(mapping vars, object o) {
	buf = "";
	foreach (string key, mixed value : vars) {
	    if (abbrev("_INTERNAL_", key)) continue;
	    if (isRoutingVar(key) && (!objectp(o) || o->outstate(key, value)))
		build_header(key, value);
	}
	if (objectp(o))
	    walk_mapping(o->state(), #'build_header);
	return buf;
}

varargs string make_mmp(string data, mapping vars, object o) {
    // we could regreplace here and do some funny nntp-like encoding of
    // leading dots.. but we should simply implement _length instead. one day.
    if (data == "." || data[0..1] == ".\n" || strstr(data, "\n.\n") != -1) {
# if 0 // one day we shall be able to parse that, too
	    vars["_length"] = strlen(data);
# else
	    P1(("%O: %O tried to send %O via psyc. censored.\n",
		previous_object() || ME, vars["_nick"] || vars, data))
	    // this message makes some people feel like they missed out
	    // on something..
	    //data = "*** censored message ***";
	    data = "";
# endif
    }
    return mmp_make_header(vars, o) 
	    + ((data && "" != data) ? "\n"+data+"\n.\n"  : "\n.\n");
}

varargs string make_psyc(string mc, string data, mapping vars, object o) {
    unless (stringp(data))
	data = "";
    return psyc_make_header(vars, o, vars["_target"], member(vars, "_context")) 
	    + ((mc) ? mc +"\n"+data : "");
}

#endif /* FORK }}} */

// notice for completeness: the PSYC renderer does not convert_charset
// from SYSTEM_CHARSET to UTF-8, so to produce correct PSYC you must not
// switch to a different SYSTEM_CHARSET, or you have to fix that...

