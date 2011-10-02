// vim:foldmethod=marker:syntax=lpc:noexpandtab
// $Id: parse.i,v 1.358 2008/12/27 00:42:04 lynx Exp $
//
#ifndef __PIKE__
# include <tls.h>
#endif

// PSYC MESSAGE PARSER - parses PSYC the old way
//
// THIS IS THE ORIGINAL LYNXISH PSYC PARSER

// this flag should enable forward-checks of dns resolutions..
// currently we don't have that, so this flag actually disables
// use of unprooven resolved hostnames and reduces everything to
// ip numbers.
//#define HOST_CHECKS

// just the plain ip number of the remote host
// ^^ glatte lüge!
volatile string peerhost;
#if 0 //defined(PSYC_TCP) && __EFUN_DEFINED__(strrstr)
volatile string peerdomain;
#endif
// remote port number
volatile int peerport;
// unresolved-ip or ip:port
volatile string peeraddr;
// holds last ip we were connected to
volatile string peerip;
#ifdef SYSTEM_SECRET
// how much can we trust the content of this packet?
volatile int ctrust;
volatile string checkpack;
volatile mixed vcheck;
#endif

// current variables (":"), permanent variables ("=", "+", "-")
//
volatile mapping cvars = 0, pvars = ([ "_INTERNAL_origin" : ME ]);
//
// a distinction between mmp and psyc-vars should be made

// current method and incoming buffer
volatile string mc, buffer;

#ifdef REPATCHING
// cache of patched remote uniforms
volatile mapping patches = ([]);
#endif

// parsing helpers..
volatile string lastvar, mod, origin_unl;
//
// list parsing helpers..
volatile array(mixed) list;
volatile mapping hash;
volatile int l = 0;
volatile int reject = 0, pongtime; //, routing;

#ifndef PSYC_TCP
// resolved UNL of remote server (psyc://hostname or psyc://hostname:port)
volatile string netloc;
# define QUIT return 1;			// udp only definition
#endif

// REPATCHING!				(temporarily called ifdef SCHWAN)
// we have two alternate ways to deal with unknown incoming connections
// the default one is to double check the hostname and return an error
// if the name isn't valid. the second one is to repatch unknown hosts
// into their ip numbers. the latter we obviously normally don't use
#ifndef REPATCHING
// prototype definition for #'getdata
protected int deliver(mixed ip, string host, string mc, string buffer, mapping cvars);
#endif

vamixed getdata(string a);

#ifdef __LDMUD__
// i think once upon a time the '&& vname' at the end wasn't necessary..
# define SCANFIT sscanf(a, "%1.1s%s%t%s", mod, vname, vvalue) && vname
#else
# define SCANFIT sscanf(a, "%1s%s%*[ \t]%s", mod, vname, vvalue) && vname
#endif

#define SPLITVAL(val) \
	unless (sscanf(val, "%s %s", val, vdata)) vdata = 0
// sollte es wirklich ein space sein? hmmm.. %t ist sowieso zu grob..

// i would like to revert to a parser that does not need to merge
// mappings at the end of each packet, just because someone
// may think a '=' does not affect a ':'. the mmp spec will
// clarify. it's not a reason to mess up a parser for!
// the simplicity of the parsers is an objective worth
// making some things clearer in the spec rather than here!
#ifdef DONT_MERGE
# if __EFUN_DEFINED__(copy)
#  define	INITCVARS		cvars = copy(pvars);
# else
#  define	INITCVARS		cvars = ([ ]) + pvars;
# endif
#else
// iff we stick to this ugly variant, we can at least simplify some operations
// which are being done on both pvars and cvars.. TODO
# define	MERGEVARS		cvars = pvars + cvars;
#endif
// el explains:
//        because in case we do a cvars = copy(pvars) in
//	  restart() operations on _state go wrong. imagine 
//	  someone sends this packet:
//
//	  :_source	psyc://someone
//	  =_source
//
//	  this would result in _source beeing empty, which is
//	  wrong. or do we enforce everyone to send permanent
//	  changes before a _set? or do we even prohibit sending
//	  the same var with different modifiers ?
// <lynX> yes, permanent changes need to come before the :
//	  the other way round is just begging to be misunderstood

// state inspector for 20after4
mixed qState(string vname) { return pvars[vname]; }

int restart() {
	// delete temporary variables for next msg
	cvars = ([ ]);
	// delete other stuff too
	buffer = "";
       	mc = 0;
//	routing = 1;    // unused as yet
#ifdef SYSTEM_SECRET
	checkpack = vcheck = 0;
	ctrust = trustworthy;
#endif
        return 1;
}

private int listAppend(string vname, string vvalue) {
	string vdata;

	unless (vname) {
		croak("_error_syntax_list_append",
	    "Invalid list append without preceding variable assignment.");
		QUIT
	}
	unless (list) {
		// eek. such an array has a size of ~35 kb right after
		// allocation. we should not do that!
		list = allocate(PSYC_LIST_SIZE_LIMIT);
		ASSERT("listAppend", stringp(cvars[vname]), buffer)
		SPLITVAL(cvars[vname]);
		list[l = 0] = cvars[vname];
		if (vdata) hash = ([ cvars[vname] : vdata ]);
	}
	SPLITVAL(vvalue);
	if (hash) hash[vvalue] = vdata;
	else if (vdata) {
		// should this ever happen?
		hash = mkmapping(list, allocate(sizeof(list)));
		hash[vvalue] = vdata;
	}
	list[++l] = vvalue;
	return 1;
}

private int conclude() {
	P3(("pp::conclude lastvar:%O l:%O hash:%O\n", lastvar, l, hash))
	unless (lastvar) return 0;
        // if (abbrev("_INTERNAL", lastvar)) not necessary because
        // the following check only allows lowercase for now
	if ( !legal_keyword(lastvar) ) {
		croak("_error_illegal_protocol_variable",
    "You are not allowed to present a variable named [_variable_name].",
			([ "_variable_name": lastvar ]));
		QUIT
	}
	if (l) {
		// alright, we have a list to close.. was: listEnd()
		if (mappingp(hash)) {
			cvars[lastvar] = hash; 
			hash = 0;
		} else
		    cvars[lastvar] = list[0..l];
		list = 0; 
		l = 0;
		P2(("%O concluded %O (%O)\n", ME, lastvar, cvars[lastvar]))
		//
		// at this point we would need to know if the mod was =
		// to also store the list in pvars, but since i'm generally
		// doubtful if this is the right way to go, i'll leave it to
		// a later day.. TODO
	// hooray, we get to check some var type families for sanity
	} else if (abbrev("_degree", lastvar)) {
		mixed t = cvars[lastvar];
		// allow for unset degree '-' ? not unless we know what for.
		if ((intp(t) && t>=0) || sscanf(t, "%1d", cvars[lastvar])) {
			// accept
			if (mod != ":") pvars[lastvar] = cvars[lastvar];
		} else {
			reject++;
			P1(("%O failed to parse %O: %O\n", ME, lastvar, t))
			croak("_error_type_degree",
	"Your value for variable [_variable] does not qualify for a degree.",
			    ([ "_variable": lastvar ]));
			m_delete(cvars, lastvar);
			//if (mod != ":") m_delete(pvars, lastvar);
		}
	} else if (abbrev("_list", lastvar)) {	// _tab
		// we only get here if the _list has one or zero members
#ifdef PARANOID
		unless (stringp(cvars[lastvar])) {
		    PT(("%O wrrrooonnggg attempt to fix %O (%O)\n", ME,
		       	lastvar, cvars[lastvar]))
		} else
#endif
		if (strlen(cvars[lastvar]))
		    cvars[lastvar] = ({ cvars[lastvar] });
		else
		    cvars[lastvar] = ({ });
		P2(("%O incoming %O (%O)\n", ME, lastvar, cvars[lastvar]))
// an empty variable is an empty variable, not zero
//	} else unless (strlen(cvars[lastvar])) {
//		cvars[lastvar] = 0;
	}
	P3(("pp::conclude cvars:%O\n", cvars))
	lastvar = 0;
        return 0;
}

varargs void diminish(string vname, vastring vvalue) {
    string vdata; // dummy

    // D4(D("diminish("+vname+","+vvalue+")\n");)
    if (!vname || vname=="") vname = lastvar;
    else {
	    conclude();
	    lastvar = vname;
    }
    if (!member(pvars, vname)) {
	    D4( D("diminish: no such variable '"+vname+"'\n"); )
	    return 0;
    }
    if (vvalue && vvalue != "") {
	SPLITVAL(vvalue);
	if (pvars[vname] == vvalue) {
		m_delete(pvars, vname);
		m_delete(cvars, vname);
	} else if (mappingp(pvars[vname])) {
		m_delete(pvars[vname], vvalue);
		// should be same mapping anyway
		m_delete(cvars[vname], vvalue);
	} else {
		log_file("PSYC", "%O DIMINISH %O from %O\n",
		    query_ip_name(), vvalue, pvars[vname]);
		// this is critical enough to let our counterpart know
		croak("_failure_unsupported_modifier_diminish",
		    "Sorry, can't diminish that.");
	}
    } else {
	if (member(pvars, vname)) {
		m_delete(pvars, vname);
		m_delete(cvars, vname);
	} else {
		// javapsyc currently sends diminish twice, bug..
		D3( D("diminish: no such variable '"+vname+"'\n"); )
		// D3 means we don't want to know about it  ;-)
	}
    }
}

vamixed parse(string a) {
	int i;
	string vname, vdata;
	mixed vvalue;

#ifdef _flag_log_sockets_PSYC
	log_file("RAW_PSYC", "» %O\t%s\n", ME, a);
#endif
//	P3(("CVARS (%O):\n %O, %O\n", a, query_ip_number(ME), ME))
	D3( if (a && strlen(a) > 2) D(S("pp:parse %O\n", a)); )
#ifdef SYSTEM_SECRET
	if (checkpack) checkpack += a + "\n";
#endif
//	if (a == ".") {
//	    PT((">>>>> RESTARTING because of .. strange '.\\n'\n"))   
//	    restart();
//	}
//	else if (a[<1] == '\\') {
//		buffer += a[..<2];
//		D1( D("psyc-parse-multiline: "+buffer+"\n"); )
//	}
//	else
	if (!a || a == "") {
		// at this point we move on from mmp_parse to psyc_parse.
		// <lynX> with bkstate coming our way we will soon have
		// to put the mmp variable state away somewhere
		// and go fetch the psyc variable state for the current
		// source/target pair from somewhere else.	TODO
//              routing = 0;    // unused as yet
	} else switch(a[0]) {
	case ':':
		unless (SCANFIT) {
		    // compatible to new syntax
		    if (vname && strlen(vname)) {
		       	m_delete(cvars, vname);
			lastvar = 0;
		    } else {
			P1(("PSYC got %O from %O\n", a, query_ip_number()))
			croak("_error_syntax_set",
				"Couldn't parse that variable setting.");
			QUIT
		    }
		}
		if (vname != "") {
			conclude();
			// intermediate hack in lack of real type support
			// which needs to be done in net/spyc
			if (abbrev("_time", vname)) vvalue = to_int(vvalue);
// unused as yet:	else if (abbrev("_date", vname))
//			    vvalue = PSYC_EPOCH + to_int(vvalue);
			cvars[lastvar = vname] = vvalue;
#ifdef SYSTEM_SECRET
			unless (vcheck) {
				// ignore checksum if origin is trustworthy
				// already.. function only available in ':'
				if (trustworthy<8 && abbrev("_check", vname)) {
					vcheck = vname;
					checkpack = "";
					// pvars invalid in checked packets
					// huh? das ist doch quatsch!
					//cvars = ([]);
					// _INTERNAL_origin will be undefined!
				} else vcheck = -1;
				// only first variable setting may set checksum
			}
#endif
		} else listAppend(lastvar, vvalue);
		break;
	case '=':
		// currently psyced cannot handle "=_target" (to clear a var)
		// it requires a whitespace after that (like a \t char).. TODO
		unless (SCANFIT) {
			croak("_error_syntax_assign",
				"Couldn't parse that variable assignment.");
			QUIT
		}
		if (vname != "") {	// // ensure varname is legal?
			conclude();
			lastvar = vname;
			pvars[vname] = vvalue;
#ifdef DONT_MERGE
			cvars[vname] = vvalue;
#endif
		} else listAppend(lastvar, vvalue);
		break;
	case '+':
		unless (SCANFIT) {
			croak("_error_syntax_augment",
				"Couldn't parse that variable addition.");
			QUIT
		}
		if (vname != "") {
			conclude();
			lastvar = vname;
		} else vname = lastvar;

		unless (mappingp(pvars[vname])) {
			SPLITVAL(pvars[vname]);
			pvars[vname] = ([ pvars[vname] : vdata ]);
		}
		SPLITVAL(vvalue);
		pvars[vname][vvalue] = vdata;
#ifdef DONT_MERGE
		cvars[vname] = pvars[vname];
#endif
		break;
	case '-':
		if (SCANFIT) 
		    diminish(vname, vvalue);
		else
		    diminish(a[1..]);
		break;
	case '?':
		// should generate a query message of it own?
		D2( D(a+" from PSYC/TCP ignored..\n"); )
		log_file("PSYC", "%O QUERY %O\n", query_ip_name(), a);
		break;
	case '\t':
	case ' ':	// var continuation
		// tab is preferred, but space is also allowed
		vvalue = a[1..];
		P4(("continuation.. %s %O %O\n", mod, lastvar, vvalue))
		if (mod == "-") {
//			croak("_error_syntax_diminish_continued",
//			  "Line continuation not defined for diminish.");
//			QUIT
			log_file("PSYC",
 "%O SEMANTIX _error_syntax_diminish_continued\n", query_ip_name());
			// since diminish of lists only uses first word
			// as list key, anything that follows must be junk
			// so we throw it away
			break;
		}
#ifdef DONT_MERGE
		cvars[lastvar] += "\n" + vvalue;
		if (mod != ":") pvars[lastvar] = cvars[lastvar];
#else
		if (mod == ":") cvars[lastvar] += "\n" + vvalue;
		else pvars[lastvar] += "\n" + vvalue;
#endif
		break;
	case '.':
		// empty packet, at least without mc..
		// since we dont distinguish between mmp and psyc vars here.. 
		conclude();
		if (sizeof(cvars) == 0) {
#ifndef __PIKE__
		    if (peerip && pongtime + 120 < time()) {
			if (same_host(SERVER_HOST, peerip)) {
			    P1(("Another PSYC node on my IP? Or am I talking to myself? %O\n", ME))
			    // not ponging to ping then...
			} else {
#ifdef PSYC_TCP
			    P2(("%O sending TCP PONG to %O=%O\n",
			       	ME, peerip, peerhost))
			    emit(".\n");
#else
			    P1(("%O sending UDP PONG to %O=%O\n",
			       	ME, peerip, peerhost))
			    send_udp(peerip, peerport, ".\n");
#endif
			}
			pongtime = time();
		    }
#endif
		} else if (!member(cvars, "_source")) {
		    // this is a change of state. 
		    MERGEVARS // until we have proper array support..
		    rootMsg(0, 0, 0, cvars);
		    // why do we deliver an empty message anyway? shouldn't
		    // we deliver the + - and = events instead? i mean..
		    // figuring out which vars have changed is so much hairier!?
		}
		restart();
		break;
	case '_':
	// default:
		conclude();
		if (cvars["_length"]) {
			croak("_failure_unsupported_module_length",
			    "Sorry, cannot deal with _length yet.");
			QUIT
		}
		MERGEVARS
#ifdef BITKOENIG_SYNTAX
		sscanf(a, "%s %s", a, buffer);
#endif
		if (!legal_keyword(a)) {
			croak("_error_illegal_method",
				"That's not a valid method name.");
			QUIT
		}
		mc = a;
#ifdef PSYC_TCP
		if (abbrev("_data", mc)) {
		    croak("_failure_unsupported_method_data",
		      "Sorry, cannot deal with binary data yet.");
		    QUIT
		}
                next_input_to(#'getdata);
#endif
		conclude();
		P3(("»pp:method(m:%O, v:%O)\n", mc, cvars))
		switch(mc) {
// okay.. this change needs more preparation.. psyced aint ready for this :(
// sorry ppl  -lnyX
//	case "_message_public":
//	case "_message_private":
	case "_conversation":   // we will soon have to decide its final name...
	case "_converse":
	case "_talk":
			mc = "_message";
			break;
	case "_notice_circuit_established":
			unless (origin_unl) origin_unl = cvars["_source"];
			break;
	case "_request_circuit_shutdown":
#ifdef PSYC_TCP
			PT(("%O got %s <- selfdestructing.\n", ME, mc))
			destruct(ME);
#else
			P0(("%O got %s, but we HAVE no circuit!\n", ME, mc))
#endif
			return 1;
		}
		return 1;
	default:
		log_file("PSYC", "%O SYNTAX %O\n", query_ip_name(), a);
		croak("_error_invalid_method_compact",
			"Compact methods undefined as yet.");
		QUIT
	}
#ifdef PSYC_TCP
	next_input_to(#'parse);
	if (flags & TCP_PENDING_TIMEOUT) {
		remove_call_out(#'quit);
		flags -= TCP_PENDING_TIMEOUT;
	}
#endif
#ifndef __PIKE__
	return 1;
#endif
}

#ifdef REPATCHING // {{{
private repatch(string t) {
	//if (patches[t]) {
	if (0) {
		D3(D("psyc:parse * repatched "+t+" to "+patches[t]+"\n");)
	} else {
		array(mixed) u = parse_uniform(t);

#if 0 //defined(PSYC_TCP) && __EFUN_DEFINED__(strrstr)
		unless (peerdomain) {
//		    if (abbrev("ve.", peerhost)) {
			int x = strrstr(peerhost, ".");
			if (x>=0) x = strrstr(peerhost, ".", -x);
			if (x>=0) peerdomain = peerhost[x..];
//		    }
		    else peerdomain = "X";
		    P3(("peerdomain %O of %O\n",
			 peerdomain, peerhost))
		}
#endif
		// wir vertrauen der port nummer!
		P3(("····	UHost=%O peerhost=%O URoot=%O\n",
		     u[UHost], peerhost, u[URoot]))
		if (u[UHost] != peerhost
		    && !same_host(u[UHost], peerhost)
#if 0 //defined(PSYC_TCP) && __EFUN_DEFINED__(strrstr)
		    && !trail(peerdomain, u[UHost])
#endif
		    && find_target_handler(u[URoot]) != ME) {
			u[UHost] = peerhost;
			patches[t] = render_uniform(u);
			D2(D("psyc:parse * patched "+t+" to "+patches[t]+"\n");)
		} else {
			P3(("psyc:parse * "+t+" not patched\n"))
			patches[t] = t;
		}

		register_target(patches[t]);
	}
	P3(("repatch: %O -> %O = %O\n", ME, peerhost, patches[t]))
	return patches[t];
}
#endif // }}}

vamixed getdata(string a) {
#ifdef _flag_log_sockets_PSYC
	log_file("RAW_PSYC", "» %O\t%s\n", ME, a);
#endif
	if (a != ".") {
#ifdef SYSTEM_SECRET
		if (checkpack) checkpack += a + "\n";
#endif
		if (buffer != "") buffer += "\n";
		buffer += a;
#ifdef PSYC_TCP
                next_input_to(#'getdata);
#endif
	} else {
#ifndef REPATCHING
		array(mixed) u;
		string t = cvars["_context"] || cvars["_source"];

# ifdef PSYC_TCP
		// let's do this before we deliver in case we run into
		// a runtime error (but it's still better to fix it!)
                next_input_to(#'parse);
# endif
		if (reject) {
			// packet has been rejected by parser for semantic reasons
			reject = 0;
			restart();
			return 1;
		}
		if (!t || trustworthy > 5) {
			deliver(0, 0, mc, buffer, cvars);
		} else unless (u = parse_uniform(t)) {
			croak("_error_invalid_uniform",
			     "Looks like a malformed URL to me.");
			QUIT
			// henner suggests this shouldn't be a QUIT.. TODO
#ifdef __PIKE__
                }
		deliver(0, 0, mc, buffer, cvars);
#else
# ifdef PSYC_TCP
		// Authenticated
		} else if (qAuthenticated(u[UHost])) {
			if (u[UTransport] && (u[UTransport] !=
#  if __EFUN_DEFINED__(tls_query_connection_state)
			    tls_query_connection_state() ? "s" :
#  endif
			    "c")) {
				P1(("%O sends me wrong transport %O in %O\n",
				    ME, u[UTransport], t))
				croak("_error_invalid_uniform_transport",
			 "[_uniform] promotes wrong transport '[_transport]'.",
				     ([   "_uniform": t,
					"_transport": u[UTransport] ]));
				// QUIT
			} else {
				P2(("%O has accepted %O by qAuthenticated auth\n", ME, t))
				deliver(0, 0, mc, buffer, cvars);
			}
#  if __EFUN_DEFINED__(tls_query_connection_state)
		} else if (tls_query_connection_state()) {
#   ifdef _flag_report_bogus_certificates
			monitor_report("_warning_invalid_host_certificate",
			    S("%O presented an uncertified host %O.",
			      ME, u[UHost]));
#   else
			P1(("%O presented an uncertified host %O.",
			      ME, u[UHost]))
#   endif
#   ifdef _flag_log_bogus_certificates
			log_file("CERTS", S("%O %O %O\n",
				    ME, u[UHost], tls_certificate(ME, 0)));
#   endif
	// vs. _flag_reject_bogus_certificates huh?
#   ifndef _flag_allow_invalid_host_certificate
			croak("_error_invalid_host_certificate",
			     "[_host] is not an authenticated source host.",
			     ([ "_host": u[UHost] ]));
			QUIT
#   endif
#  endif
# endif
		// fippo suggests we should have a protocol that enables
		// hostnames on the other side first, before we deliver data
		// to it. currently this could be abused for a DoS attack.
		//
		// in the case of udp without udp circuits, such an exchange
		// doesn't make sense: udp psyc is intended to be lightweight.
		// in that case we shouldn't resolve here, we should simply
		// reject the source information, or repatch it. the fact that
		// we are receiving messages is still useful. better even if
		// we had a cryptographic way to protect udp packets, making
		// even potential ip spoofing irrelevant.   -lynX
		//
		} else dns_resolve(u[UHost], #'deliver,
				   u[UHost], mc, buffer, cvars);
#endif
		restart();
	}
	return 1;
}
// we are in ifndef REPATCHING
// this additional function to handle name resolution does not
// exist in REPATCHING mode. see explanation at the top of the file.
protected int deliver(mixed ip, string host, string mc, string buffer, mapping cvars) {
	string psycaddr;

# if 0 //def __PIKE__
        // temporary!
        return doneParse(ip, host, mc, buffer, cvars);
# else
//	PT(("deliver(%O, %O, %O, %O, %O)\n", ip, host, mc, buffer, cvars))
        // when erq isn't running we get ip == host .. maybe return -1 instead?
        // and why return -1 for failed dns when we could use 0 ? FIXME
        // no wait.. why is ip==0 being let thru in this code? did i forget
        // about something?
//# ifdef PSYC_TCP
	if (ip) {
//# endif
            if (ip == -1) {
                monitor_report("_warning_invalid_host",
                    S("%O could not resolve %O.", ME, host));
                croak("_error_invalid_host", "Could not resolve [_host].",
                        ([ "_host": host ]));
                QUIT
                return 1;
            } else if (ip == host) {
                // Happens when 'erq' could not be spawned. We allow
                // the message through anyway (thus spoofable) and
                // just complain about it.
                monitor_report("_warning_unavailable_resolution",
                    S("%O could not resolve %O. Good luck!", ME, host));
                // This is a clear case of a _failure, but if you insist
                // on running your psyced without dns resolution, who are
                // we to intentionally break your experience of unsafety?
            } else if (ip == peerip) {
                //PT(("deliver: from %O\n", peerhost))
# ifdef STRICT_DNS_POLICY
                if (peerhost == peerip) {
                    P0(("deliver: got %O from %O but host %O is not resolved yet.\n", mc, ip, peerhost))
                    croak("_error_invalid_host_slow",
                          "Could not resolve [_host] quickly enough for you!",
                            ([ "_host": peerip ]));
                    QUIT
                    //return 1;
                }
# endif
//# ifdef PSYC_TCP
            } else {
                monitor_report("_warning_rejected_relay_incoming",
                    "Dropped a packet from "+ peerip +
                    " trying to relay for "+ host +" ("+ ip +")");
                P3(("Dropped a packet from "+ peerip +
                    " trying to relay for "+ host +" ("+ ip +")"+"\n"+
                    " in parse:deliver(%O, %O, %O, %O, %O)\n",
                        ip, host, mc, buffer, cvars))
#ifdef REJECT_IP_NOT_HOST
                croak("_error_rejected_relay_incoming",
                    "Your address [_host_IP] is not permitted to send as [_host].",
                        ([ "_host": host, "_host_IP": peerip ]));
                QUIT
                return 1;
#endif
            }
//# endif
        }
# ifdef PSYC_TCP
	else if (psycaddr = cvars["_context"] || cvars["_source"]) {
	    register_target(lower_case(psycaddr));
	}
# endif
# endif // PIKE
	{ // looks strange but is correct
#endif	// !REPATCHING
		mixed source, context, t, t2;
		object o;
		int i;

		P3(("parsed: (%O,%O,%O from %O)\n", mc, buffer, cvars, pvars))

#ifndef __PIKE__
#ifdef PSYC_TCP
		if (host) sAuthenticated(host); // dns resolution just once
#endif
#ifdef SYSTEM_SECRET
		if (checkpack) {
			// TODO: extend logic to allow for per-host secrets
			// for example using the /config command..
			switch (vcheck) {
#if __EFUN_DEFINED__(hmac)
# ifndef DONT_TRUST_SHA224
			case "_check_SHA224":
				break;
# endif
# ifndef DONT_TRUST_SHA256
			case "_check_SHA256":
				ctrust = (hmac(TLS_HASH_SHA256,
// TODO: hash(TLS_HASH_SHA256, SYSTEM_SECRET) only needs to be computed once
//			    for the entire system, and stored in the library.
					   hash(TLS_HASH_SHA256, SYSTEM_SECRET),
					   checkpack) == lower_case(cvars[vcheck])) ? 8: 0;
				break;
# endif
# ifndef DONT_TRUST_SHA512
			case "_check_SHA512":
				break;
# endif
#endif
			// should move out of #ifdef SYSTEM_SECRET if
			// anyone actually likes to use this
			case "_check_integrity_SHA1":
				if (sha1(checkpack) !=
				    lower_case(cvars[vcheck])) {
					croak("_error_invalid_integrity",
				"Strange, integrity check [_integrity] failed.",
					([ "_integrity": cvars[vcheck] ]) );
					return 1;
				}
				break;
#ifndef DONT_TRUST_SHA1
			case "_check_SHA1":
				ctrust = (sha1(SYSTEM_SECRET + checkpack) ==
				    lower_case(cvars[vcheck])) ? 8 : 0;
				break;
#endif
			case "_check_fake":
				ctrust = (md5(SYSTEM_SECRET) ==
				    lower_case(cvars[vcheck])) ? 8 : 0;
				break;
			case "_check":
#ifndef DONT_TRUST_MD5
			case "_check_MD5":
				ctrust = (md5(SYSTEM_SECRET + checkpack) ==
				    lower_case(cvars[vcheck])) ? 8 : 0;
				//PT(("checksum %O == %O for %O + %O results in %O\n", md5(SYSTEM_SECRET + checkpack), cvars, SYSTEM_SECRET, checkpack, ctrust))
				break;
#endif
			default:
				// _warn_unsupported_check_method ?
				// ctrust remains at host-ip level
				break;
			}
			//PT(("psyc:checksum result %O with packet contents %O\n", ctrust, checkpack))
			// a quick solution.. maybe we should do SASL instead, or maybe
			// we should let the other side specify its =_trust whenever the
			// packet is auth'd by a checkpack
			if (mc == "_request_circuit_trust") {
// oops.. major error in logic here.. i should be sending a challenge first!!
				if (ctrust > 6) {
					trustworthy = ctrust;
					croak("_echo_circuit_trust",
					      "You have gained my trust.");
					return 1;
				}
				croak("_failure_invalid_circuit_trust",
                            "Sorry, could not verify your authorization.");
				return 1;
			}
		}
	    	if (ctrust) cvars["_INTERNAL_trust"] = ctrust;
#else
                // good placement here?
		if (mc == "_request_circuit_trust") {
                        croak("_failure_invalid_circuit_trust",
                            "Sorry, could not verify your authorization.");
                        return 1;
                }
#endif
#endif  // PIKE
#if 1 // def DONT_MERGE
		// das klappert irgendwie nicht als pvar TODO
		cvars["_INTERNAL_origin"] = ME;
		// wieso nicht? warum nicht lieber den bug suchen statt
		// hier was rumzuhacken? vermutlich kann man es nicht
		// oben zuweisen, muss also in ein create()
#endif
		// this may only be set by user:msg()
		m_delete(cvars, "_nick_verbatim");
		// psyced never acts as multiplexing client proxy (as yet)
		// so we have no reason to accept this from elsewhere
		// and risk to forward it somewhere (we could aswell rename
		// into _INTERNAL_target_forward to ensure it not causing
		// an outgoing security problem and still be available)
		m_delete(cvars, "_target_forward");
#ifdef PSYC_TCP
# ifdef REPATCHING
                next_input_to(#'parse);
# endif
# if 0 //ndef HOST_CHECKS
		unless (netloc) {
		    // lets see if the ip number has a name
		    if (query_ip_name() != query_ip_number()) {
			netloc = "psyc://"+query_ip_name();
			// peerport has either positive or negative value
			if (peerport) netloc += ":"+peerport;
			D2(D(netloc+ " resolved name registered\n");)
			register_target( netloc );
		    }
		}
# endif
#endif
		// backward? compatibility to jaPSYC library
//		if (abbrev("_diminish", mc)) {
//			diminish(mc[9..], buffer);
//			return restart();
//		}
		if (t = cvars["_context"]) {
		    // detect when _context contains a local object
		    // which of course should never happen according
		    // to routing rules. should we check for psyc_object()?
		    // no, we should check the validity of the hostnames
		    // sent to us, then it couldn't possibly be a local object.
		    if (objectp(t)) {
			    // and no, it's not in pvars
			    PP(("psyc:parse got object!? %O.", t));
			    context = t;
		    // lowercase policy
		    } else context = cvars["_context"] = lower_case(t);
		}
		if (t = cvars["_source"]) {
		    // "Link to [_source] established.. uses this
		    // and place/slave too (although improperly)
		    //m_delete(cvars, "_source");

		    // if a context is talking, we recognize local sources
		    // this may not be up to date with new routing syntax  :(
		    unless (context && (source = psyc_object(t))) {
#if 0 // currently we don't use any relative object specs
			i = strstr(t, "//");
			if (i < 0) { // a relative object spec! experimental!!
				// lets guess the remote UNL
				source = "psyc://"+peeraddr;
				// we allow with or without leading slash
				if (t[0] != '/') source += "/";
				// we used to care about a trailing slash, why?
				//if (t[strlen(t)-1] != '/') t += "/";
				// maybe we should strip trailing slashes instead
				source += t;
			} else if (i == 0) { // a shortened object spec
				source = "psyc:"+t;
			} else source = t;
#endif
			// lowercase policy
			source = lower_case(t);
		    }
#if 0
		} else {
			source = "psyc://"+peeraddr+"/";
# ifdef REPATCHING
			patches[source] = source;	// dont parse this
# endif
#endif
		}
		// checking _source_XXX etc should be generically done using a
		// flag in the routing vars share thing..
		// also, can these safely move into one of the ifs above? TODO
		if (t = cvars["_source_relay"]) {
		    // recognize local psyc object
		    if (t = psyc_object(t)) {
			P2(("local _source_relay %O for %O from %O, cont %O\n",
			     t, mc, source, context))
			cvars["_source_relay"] = t;
		    } else if (stringp(t)) {
		       	// otherwise: lowercase policy
			cvars["_source_relay"] = lower_case(t);
		    }
		}
		if (t = cvars["_source_identification"]) {
		    // recognize local psyc object
		    if (t = psyc_object(t)) {
			P2(("local _source_identification %O for %O from %O, cont %O\n",
			     t, mc, source, context))
			cvars["_source_identification"] = t;
		    } else if (stringp(t)) {
		       	// otherwise: lowercase policy
			cvars["_source_identification"] = lower_case(t);
		    }
		}
		P3(("parse1: source is %O, context is %O\n", source, context))
                // other side doesn't *have* to provide a source..
		if (trustworthy > 6 && (t2 = context || source))
		    // according to http://about.psyc.eu/Routing we should
		    // not receive a message with both context and source in
		    // a long time, so the order of OR is irrelevant here.
                    register_target(t2);
		    // cache the physical source string so we don't need to
		    // parse it later
#ifdef REPATCHING // {{{
		else {
		    // virtual hosting support müsste evtl hierhin.. TODO
		    if (context) {
			// now it becomes the job of the person.c to figure
			// out if the context is legitimate and trustworthy
			// psyc1.1 type multicasting not supported here
			context = cvars["_context"] = repatch(context);
		    } else if (source) {
			D1( if (objectp(source)) D("something went wrong.\n"); )
			cvars["_source_verbatim"] = source;
			source = repatch(source);
		    }
		}
		P3(("parse2: source is %O, context is %O\n", source, context))
#endif // REPATCHING }}}
		if (t = cvars["_target"]) {
		    // this isn't used anywhere, apparently
		    // but the recipient may care to know how he got called
		    //m_delete(cvars, "_target");
		    o = psyc_object(t); // find the object matching the _target
		    if (!o && strlen(t)) {
			mixed u = parse_uniform(t, 1);
// be tolerant, allow for scheme-independent notation
//			//unless (u[UScheme] && u[UScheme] == "psyc") {
//			unless (u[UScheme] && strlen(u[UScheme])) {
//				croak("_failure_unsupported_target_relative",
//				    "No support for relative targets yet.");
//				QUIT
//			}
			unless (u[UHost] && strlen(u[UHost])) {
				croak("_failure_unsupported_notation_location",
				    "No support for fancy UNLs yet.");
				QUIT
			}
#ifdef NONIX
			if (strlen(u[UResource]) > 1)
			    cvars["_INTERNAL_nick_target"] = u[UNick];
#endif
			P3(("DEBUG: is_localhost is %O for %O of %O\n",
			    is_localhost(u[UHost]), u[UHost], u))
			unless (is_localhost(u[UHost])) {
                            if (trustworthy > 7) {
                                P1(("RELAYING permitted for %O to %O (%O)\n",
                                    source, t, ME))
                                unless (cvars["_source_relay"])
                                    cvars["_source_relay"] = source;
                                source = cvars["_source"] || SERVER_UNIFORM;
				// relay the message!
				// this is used by procmail for example, whenever
				// it needs to send to an xmpp: recipient.
                                sendmsg(t, mc, buffer==""? 0: buffer, cvars, source);
                                return restart();
                            }
                            P1(("RELAYING denied from %O to %O (%O)\n",
                                source, t, ME))
			    monitor_report("_warning_unsupported_relay",
				  S("%O is trying to find %O here. Relaying denied.\n", ME, t));
			    croak("_failure_unsupported_relay",
				//"Well done mate, you crashed me.");
				"Relaying denied: [_host] is not a hostname of ours.",
				    ([ "_host": u[UHost] ]));
#if 0
			    // TODO: we quit here to not do the same hash-lookup
			    // in rootmsg again.
			    // (didn't get it? nevermind.. it's just el's sick humor)
			    QUIT
#else
			    // we do not QUIT here as an evil attacker may
			    // CNAME his evil.com to us and try to disrupt
			    // our communications with some popular server
			    // by making us drop an otherwise very popular
			    // circuit. then again, what if a sender SHOULD
			    // not send to us with any other hostname but
			    // the one we announced ourselves as _source
			    // when we sent our first greeting() ? then we
			    // could just dump "illegal" transmissions.
			    // well, we don't need to be so harsh against
			    // multi domain hosters really: relaying is
			    // denied by default so the attacker needs to
			    // be a user on the sending server. in the end
			    // it's a question of trust: don't let zero
			    // trust users send funny amounts of data.
			    return 1;
#endif
			}
			// .. yes.. add is_localhost check here, but without callback
			// as we need a whitelist of allowed localhosts anyway..
			// or would you like it if somebody added a CNAME stinkpfau.go
			// to your server? no. are you going to blacklist him? no
			// just like a webserver you will act depending on your configured
			// hostnames, even if we let the end user configure it by specifying
			// the hostname he likes his identities to be shown as...
			// .. /set hostname psyc.duftepfau.go ... or was it /set id ?
			//
			// implement proxy service? it already exists for trustworthy senders
			// do any checking for gateway schemes? TODO
			if (u) o = find_psyc_object(u);
		    }
		    if (o) {
#ifdef PSYC_TCP
			P2(("TCP[%s] => %O: %s\n", peeraddr, o || t, mc))
#else
			PT(("UDP[%s] => %O: %s\n", peeraddr, o || t, mc));
#endif
			P4(("%O's %s(%O) to %O\n", source, mc, buffer, o))
#ifdef TAGGING
// should we no longer use the monster sprintf to keep our callbacks, then
// the callbacks should in many cases ensure they got the reply from a
// legitimate sender.. or we'll have a most interesting mitm-zone here..
// then again, in some other cases, that is exactly what tags are for!
// 			FIXME: there is code for executing callbacks in 
// 				entity.c - why is this here?
                        if (cvars["_tag_reply"] && 
                                objectp(o) && o->execute_callback(cvars["_tag_reply"], 
                                                 ({ source, mc, buffer == "" ? 0 : buffer, cvars }))) {
                            // NOOP
                        } else
#endif
			if (objectp(o))
			    o -> msg(source, mc,
				 buffer == "" ? 0 : buffer, cvars, 0, t);
			else
			    PP(("%O's %s(%O) to %O\n", source, mc, buffer, o));
#if 0
			} else if (stringp(o) && strlen(o)) {
			    // should croak using sendmsg, huh?
			    croak("_error_unknown_target",
				"No such object: [_target_given]",
				([ "_target_given": t ]) );
			    // how does one refer to the incoming _target
			    // variable? _given doesnt look good to me.
                        // new: see _error_illegal_uniform below
#endif
#ifdef REPATCHING
			return restart();
#else
			return 1;
#endif
		    } else {
                        if (source) sendmsg(source, "_error_illegal_uniform",
                            "[_uniform] is not a legal address here.",
                              ([ "_uniform": cvars["_target"],
				"_tag_reply": cvars["_tag"] ]) );
			else croak("_error_illegal_uniform",
                            "[_uniform] is not a legal address here.",
                              ([ "_uniform": cvars["_target"],
				"_tag_reply": cvars["_tag"] ]) );
                    }
		} 
		if (context) {
#ifndef __PIKE__ // TPD
		    o = find_context(context);
		    unless(o) {
			// no one is interested / online
			// is this an abuse or a feature?
			// fippo: feature, context may be 0
			// if no one who is normally listening is online
			return restart();
		    }
		    if (objectp(o)) 
			o -> castmsg(source, mc, 
				 buffer == "" ? 0 : buffer, cvars);
		    // this effectively ignores any _targets that may have
		    // been submitted with the message.. do we like that? TODO
#endif
		    return restart();
		}
		// else if (t)  return _error_rejected_relay_outgoing TODO
		t2 = SERVER_UNIFORM;
		// this part is new and maybe can be optimized.. TODO
                // the main optimization would be to check for is_localhost
                // with host part of target
                if (t == t2 || (char_from_end(t2, 1) == '/' && t+"/" == t2)) {
                    // i know that this check sucks... but as we dont normalize 
                    // uniforms...
                    // it would be much better to check if only host is set
                    // and then use is_localhost
                    sendmsg("/", mc, buffer == "" ? 0 : buffer, cvars, source);
                    return restart();
                }
		rootMsg(source, mc, buffer == "" ? 0 : buffer, cvars, t);
#ifdef REPATCHING
		restart();
#endif
	}
	return 1;
}

#if __EFUN_DEFINED__(psyc_parse)
// temporary new "lfun" called from driver's comm.c to peek into new connection
// only exists if libpsyc is provided
void connection_peek(string data) {
	P4((">> peek: %O\n", data));

	if (data[0] == C_GLYPH_NEW_PACKET_DELIMITER) {
		enable_binary(ME);
	}
}
#endif

#ifdef PSYC_TCP
vamixed startParse(string a) {
	if (a == ".") {		// old S_GLYPH_PACKET_DELIMITER
		restart();
		if (isServer()) greet();
	}
# if defined(SPYC_PATH) && __EFUN_DEFINED__(psyc_parse)
	else if (a[0] == C_GLYPH_NEW_PACKET_DELIMITER) {
		object o = clone_object(SPYC_PATH "server");
		unless (o && exec(o, ME) && o->logon(0)) {
			croak("_failure_object_creation_server",
			    "Could not instantiate PSYC parser. Weird.");
			QUIT
		}
		// if (isServer()) o->greet();
		o->feed(a);
		return 1;
	}
# endif
# ifdef SWITCH2PSYC
	    // minor bug: we send the . right after <switched/> without a
	    // newline as xmpp doesn't require that and psyc doesn't want
	    // it. still ldmud puts the . in the wrong buffer, so let's
	    // just be tolerant and accept an empty newline for starting
	    // under this circumstance. however we should fix this one day.
	else if (a == "") restart();
	    // how can we avoid the buffer from being re-input to the
	    // exec'd object? this however circumvents it
	    // in fact it seems to be a bug in older ldmud versions
	    // newer versions instead have the bug of providing an empty buffer
	else if (abbrev("<?xml", a)) {
	    PT(("SWITCH2PSYC: re-encountered old buffer. ignoring: %O\n", a))
            next_input_to(#'startParse);
	    return 1;
	}
# endif
# ifdef MESSAGE_SEND_PROTOCOL
	// only activate this if you use the PSYC port of lords
	// minimal compliant MSP implementation	;)
	// would love to code a bit more of it, but it is completely pointless
	else if (a[0] == 'B' || a[0] == 'A') {
		write("-MSP is of historic and cultural value, but please upgrade to PSYC.\n");
		QUIT
	}
#endif
	else {
		PT(("PSYC startParse got %O from %O\n", a, query_ip_number()))
		croak("_error_syntax_initialization",
		    sprintf("Received unknown protocol initialisation %O. The old protocol begins with a dot in a line by itself.",a));
		// experiencing a loop here, because some implementations
		// try immediate reconnect. idea: in most places where we
		// QUIT we should put the tcp link on hold instead, and
		// close it only several seconds later. TODO
		QUIT
	}
	pvars["_INTERNAL_source"] = "psyc://"+peeraddr+"/";
        next_input_to(#'parse);
        return 1;
}
#endif

string qOrigin() { return origin_unl; }

void sOrigin(string origin) { unless (origin_unl) origin_unl = origin; }
