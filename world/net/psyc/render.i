// vim:foldmethod=marker:syntax=lpc:noexpandtab
// $Id: render.i,v 1.111 2008/12/01 11:31:33 lynx Exp $
//
// headermaker functions. we should call it render rather than edit.

volatile mapping isRouting = shared_memory("routing");

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
	routeMe = isRouting[key];
	P3(("isRouting[%O] = %O, render? %O\n",
	    key, routeMe, routeMe & PSYC_ROUTING_RENDER))
	if ((routeMe &&! (routeMe & PSYC_ROUTING_RENDER))
	   || abbrev("_INTERNAL", key)) return -1;
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
# ifndef NEW_LINE
			if (strlen(data) && char_from_end(data, 1) == '\n')
			    excessiveNewline = 1;
# endif
		}
#else
		PT(("non-string data: %O\n", data))
		data = data? to_string(data): "";
#endif
	}
	else if (data == S_GLYPH_PACKET_DELIMITER ||
# ifdef SPYC
		 // just some random limit that makes us prefer _length
		 // over scanning data for illegal characters
		  strlen(data) > 444 ||
# endif         
		 (strlen(data) > 1 &&
		  data[0] == C_GLYPH_PACKET_DELIMITER && data[1] == '\n')
		    || strstr(data, "\n" S_GLYPH_PACKET_DELIMITER "\n") != -1) {
		// we could check what people are typing in usercmd.. then
		// again, "illegal" data may also come in from XMPP, and
		// anything should be legal in PSYC.. so let's handle it here.
# ifdef SPYC
		needLen++;
# else
		// old psyc syntax has no clean solution to this problem,
		// so we just censor the message. use the new syntax!
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
# ifdef BETA
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
# ifdef BETA
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
#endif /* NEW_RENDER */

	if (mappingp(vars)) {
#if 0 //ndef EXPERIMENTAL
		if (member(vars, "_count"))
		    ebuf += "\n:_count\t" + vars["_count"];
#endif
#if __EFUN_DEFINED__(walk_mapping)
		// walk_mapping could be rewritten into foreach, but thats work
		walk_mapping(vars, #'build_header, vars);
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

// notice for completeness: the PSYC renderer does not convert_charset
// from SYSTEM_CHARSET to UTF-8, so to produce correct PSYC you must not
// switch to a different SYSTEM_CHARSET, or you have to fix that...

