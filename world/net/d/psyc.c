// vim:foldmethod=marker:syntax=lpc:noexpandtab

// this whole file is a mistake... and not in use...

#ifdef FORK // {{{

#include <net.h>
#include <uniform.h>
#include <services.h>

inherit NET_PATH "state";
inherit NET_PATH "queue2";

#include "../psyc/edit.i"

string psycName() { return ""; }

varargs mixed find_psyc_object(string t, object connection, array(mixed) u) {
	string r, svc, user;
	object o;

	PT((">> t: %O\n", t))

	unless (strlen(t) && u = (u || parse_uniform(t))) return 0;

	//unless (u[UScheme] && u[UScheme] == "psyc") {
	unless (u[UScheme] && strlen(u[UScheme])) {
	    if (connection)
		connection -> w("_failure_unsupported_target_relative",
				"No support for relative targets yet.");
	    return 0;
	    //QUIT
	}
	unless (u[UHost] && strlen(u[UHost])) {
	    if (connection)
		connection -> w("_failure_unsupported_notation_location",
				"No support for fancy UNLs yet.");
	    return 0;
	    //QUIT
	}
	// TODO: croak if host:port isnt us?
	// or implement proxy service?
	// do any checking for gateway schemes?
	user = u[UUser];
	r = u[UResource];
	if (r && strlen(r)) {
#if __EFUN_DEFINED__(trim)
# include <sys/strings.h>
		r = trim(r, TRIM_RIGHT, '/');
#else
		while (r[<1] == '/') r = r[..<2];
#endif
		if (strlen(r)) switch(r[0]) {
		case '^':
		case '~':
			if (user) {
			    if (connection)
				connection -> w("_error_invalid_uniform_user_duplicate",
						"Two users in uniform not allowed here.");
//			    QUIT
			    return 0;
			}
			user = r[1..];
			break;
		case '$':
			// target wird auf serv/args gesetzt
			// weiss noch nicht ob das gut ist
			t = r[1..];
			unless (sscanf(t, "%s/%s", svc, r))
			    svc = t;
    //P3(("find_service: %O for %O(%O)\n", o, svc, r))
			if (o = find_service(lower_case(svc)))
			    break;
			unless (user) {
			    if (connection)
				connection -> w("_failure_unavailable_service",
    "No service '[_service]' available here.", ([ "_service" : svc ]) );
			    return 0;
			}
		case '@':
			// t[0..0] = PLACE_PATH;
			r = PLACE_PATH + lower_case(r[1..]);
			if (o = find_object(r)) break;
			// should check for legal name instead
			// of catch. TODO
			catch(o = r -> load());
			// fall thru
		default:
			o = find_object(r);
			D2( if(!o)
			    D("OBJECT NOT FOUND: "+ r +"\n"); )
		}
		else unless (user) return 0;
		if (user && !objectp(o)) {
			o = summon_person(user);
//			o = find_person(user);
//			unless(o) o = createUser(user);
		}
	}
	return o;
}
// new psyc-parser to fit the needs ot the new api. operates on _one_ string
// containing _one_ paket (everything after the mc is considered the data)
//
// returns the mc, psyc-data remains in the buffer.
#define ERROR(x)	{ monitor_report("_error_broken_syntax", x+((connection) ? sprintf(" from %O.", connection) : ".")); return 0; }
//#define ERROR(x)	{ monitor_report("_error_broken_syntax", x); return 0; }

static string spsyc_parse(string buf, mapping vars, int iscontext, string source,
		   object connection, object o) {
	string mod, var, val, mc, line, lastmod, lastvar;
	mixed lastval;
	int a = 0, b;

	while (a < strlen(buf) && !mc && ((b = strstr(buf, "\n", a)) != -1
	    || b = strlen(buf))) { 

	    val = "";
	    line = buf[a..b-1];
	    switch(buf[a]) {
	    case ':':
	    case '=':
	    case '+':
	    case '-':
		unless (sscanf(line, "%1.1s%s%t%s", mod, var, val)
		    ||  sscanf(line, "%1.1s%s", mod, var)) {
		    if (connection)
			connection -> w("_error_broken_syntax", 
					"You are in league with Ratzinger,\
					the evil lord or broken psyc."); 
		    // <lynX> runtime error erzeugen!?? wie kommt ihr darauf
		    // das sei eine gute idee? damit man nen grund hat große
		    // teure catches zu machen? ich will in meinen logs nur
		    // runtime fehler von bugs - ist schon schlimm genug das
		    // schrottige http-zugriffe sowas auslösen, aber logs
		    // voller fehler von buggy psyc implementationen, nein!
		    // man sagt der gegenseite dass was faul ist, killt evtl
		    // die verbindung falls der fehler nicht recoverbar ist,
		    // reported oder loggt das problem, aber dann is gut!
		    ERROR(S("PSYC parsing failed in '%s'", line))
		}
		unless (sizeof(var)) {
		    if (mod == "-") {
			if (connection)
			    connection -> w("_error_broken_syntax", 
					    "Diminishing of lists is not supported."); 
			ERROR("PSYC parsing failed (Invalid list diminish)")
		    } else unless (lastvar) {
			if (connection)
			    connection -> w("_error_broken_syntax", 
					    "Wrong list continuation."); 
			ERROR("PSYC parsing failed (Invalid list continuation)")
		    } else unless (lastmod == mod) {
			if (connection)
			    connection -> w("_error_broken_syntax", 
					    "Inconsistent list continuation. Do not mix modifiers."); 
			ERROR("PSYC parsing failed (Invalid list continuation)")
		    }
			
		    if (pointerp(lastval)) {
			lastval += ({ val });
		    } else {
			lastval = ({ lastval, val });
		    }
		    a = b+1;
		    continue;
		}
		
		break;
	    case '\t':
		unless (lastvar) {
		    if (connection)
			connection -> w("_error_broken_syntax", 
					"Continuation without variable."); 
		    ERROR("PSYC parsing failed (Invalid variable continuation)")
		}
		if (pointerp(lastval))
		    lastval[<1] += "\n"+line[1..];
		else lastval += "\n"+line[1..];
		a = b+1;
		continue;
	    case '_':
		int i;
		for (i = b - a - 1; i>=0; i--) {
		    unless (line[i] == '_' ||
			   (line[i] >= 'a' && line[i] <= 'z') ||
			   (line[i] >= '0' && line[i] <= '9') ||
			   (line[i] >= 'A' && line[i] <= 'Z')) {
			if (connection)
			    connection -> w("_error_illegal_method",
				    "[_method] is not a valid method name.",
					    ([ "_method" : line ]));
			ERROR("PSYC parsing failed (Invalid method '"
				    + line +"')\n")
		    }
		}
		mc = line;
		switch(mc) {
		case "_conversation":   // we will soon have to decide its 
		case "_converse":	// final name...
		case "_talk":
		    mc = "_message";
		    break;
		default:
		    if (abbrev("_conversation", mc)) {
			mc[..12] = "_message";
		    } else if (abbrev("_converse", mc)) {
			mc[..8] = "_message";
		    } else if (abbrev("_talk", mc)) {
			mc[..4] = "_message";
		    }
		}
		break;
	    default:
		if (connection)
		    connection -> w("_error_broken_syntax", 
				    "Unsupported modifier '[_modifier]'", 
				    ([ "_modifier" : line[0] ]));
		ERROR("PSYC parsing failed (Unknown modifier '" 
			    + line[0] + "')")
	    }
	    if (lastmod) switch(lastmod[0]) {
	    case '=':
		if (o)
		    o -> Assign(source, lastvar, lastval, iscontext);
	    case ':':
		vars[lastvar] = lastval;
		break;
	    case '+':
		if (o)
		    o -> Augment(source, lastvar, lastval, iscontext);
		break;
	    case '-':
		if (o)
		    o -> Diminish(source, lastvar, lastval, iscontext);
		break;
	    }

	    lastmod = mod;
	    lastvar = var;
	    lastval = val;
	    a = b+1;
	}
	unless (mc) {
	    // malformed psyc-packet in buf. get on it!
	    if (connection)
		connection -> w("_error_broken_syntax",
				"Received packet without method.");
	    ERROR("PSYC parsing failed (Method missing)")
	}

	if (strlen(buf) == b) { // no data. packet ends with mc
	    buf = "";
	} else {
	    // still data in buf. buf[b] is the last \n before the psyc-data
	    buf = buf[b+1..];
	}
	
	return mc;
}

// not exactly beautiful to have it here, but still more efficient than
// having hordes of if (psycd) checks..
int deliver(mixed ip, string peerip, object connection, string peeraddr,
		 string host, string data, mapping mvars) {
	string psycaddr;
	PT(("deliver(%O, %O, %O, %O)\n", ip, host, data, mvars))
	mixed o, target = mvars["_target"];
	int trustworthy = (member(mvars, "_INTERNAL_trust")) 
			    ? to_int(mvars["_INTERNAL_trust"])
			    : 0;
	
	

	if (ip == -1) {
	    monitor_report("_error_something_because_somebody_was_once_again_too_lazy_to_look_up_a_method_which_most_likely_already_exists_in_library_dns_c", S("%O could not resolve %O.",
						 ME, host));
	    
	    if (objectp(connection))
		connection->w("_error_invalid_host", "Could not resolve [_host].",
		  ([ "_host": host ]));
	    return 1;
	}

	if (ip && ip != peerip) {
	    monitor_report("_warning_rejected_relay_incoming",
		"Dropped a packet from "+ peerip +
		" trying to relay for "+ host);
	    P0(("Dropped a packet from "+ peerip +
		" trying to relay for "+ host +"\n"+
		" in parse:deliver(%O, %O, %O, %O)\n",
		    ip, host, data, mvars))
	    if (connection)
		connection->w("_error_rejected_relay_incoming",
		"You are not allowed to relay for [_host].",
		    ([ "_host": host ]));
	    return 1;
	} else if (psycaddr = mvars["_context"] || mvars["_source"]) {
	    // schmorp??
	    if (connection)
				// is psycaddr already lc?
		register_target(lower_case(psycaddr), connection);
	} else psycaddr = "psyc://"+peeraddr;

	if (member(mvars, "_source_identification")) {
	    mixed t = lookup_identification(psycaddr,
					    mvars["_source_identification"]);
	    unless (t) {
		// the i: is a safety measure in case of a supergau
		// that mvars["_source_identification"] could somehow resemble
		// some other queue in the system.. not convinced..
		// see also two more occurences in this file.
		//
		string n = "i:"+mvars["_source_identification"]+psycaddr;
		unless (qExists(n)) qInit(n, 100, 5);
		enqueue(n, ({ ip, peerip, connection, peeraddr, host, data, 
			    mvars }));
		return 0;
	    } else if (t == mvars["_source_identification"]) {
		// TODO: increase trustworthy ness
		mvars["_source_technical"] = psycaddr;
		mvars["_source"] = mvars["_source_identification"];
		m_delete(mvars, "_source_identification");
	    } else {
		PT(("someone (%O) claims an identification (%O) though he is "
		    "already"
		    " registered as someone else. Will we allow multiple"
		    " identifications per unl? Should - in theory - be possible"
		    " and okay. dropping this message.\n", 
		    psycaddr, mvars["_source_identification"]))
		return 0;
	    }
	}

	{
	    string state, err, mc;
	    mixed source, t;
	    mixed *u;
	    int iscontext = 0; // if we are in context-state
	    mapping psycvars = ([ "_INTERNAL_origin" : connection ]);

	    // this is our new context-routing without target.
	    // 1 _source
	    // 2 _target
	    // 4 _context
	    switch(
		   (member(mvars, "_source") ? 1 : 0) | 
		   (member(mvars, "_target") ? 2 : 0) |
		   (member(mvars, "_context") ? 4 : 0)
		   ) {
	    case 5: 
# if DEBUG > 0
		PT(("Received both context(%s) and source(%s) from %O\n",
		    mvars["_context"], mvars["_source"], psycaddr))
# endif
	    case 4:
		t = mvars["_context"];
		state = t;
		iscontext = 1;
		o = find_context(t);
		unless (o) {
			// no one is interested / online
                        // is this an abuse or a feature?
                        // fippo: feature, context may be 0
                        // if no one who is normally listening is online
			PT(("Dropped messages from %s because there was no\
			     context slave for %s.\n", 
			    psycaddr, mvars["_context"]))
			return 1;
		}
		break;
	    case 6: // this is unicast from context to target in the context's
		    // state
		state = mvars["_context"];
		iscontext = 1;
	    case 3:
		// "Link to [_source] established.. uses this
		// and place/slave too (although improperly)
		//m_delete(mvars, "_source");
		unless (iscontext) {
		    source = lower_case(mvars["_source"]);
		    state = source;
		}
	    case 2:
		t = mvars["_target"];
		// this is a message from the rootobj but since its target is
		// not our rootmsg we need to offer some kind of source.
		// TODO: Why not use ME here ??
		unless (source)
		    source = psycaddr;
		if (trustworthy > 7) {
		    P2(("%O relaying permitted for %O to %O via %O\n",
			ME, source, t, o))
		    // We should do mmp-routing here.. but find out before if
		    // there is any connections registered for that target/host
		}
		u = parse_uniform(t);
		if (u && (u[UResource] == "" || !u[UResource])) {
		    // rootMSG. that parse_uniform makes me quite unhappy.
		    // we may check the string t only for the existance of a
		    // resource since we allready know that it is a valid url.
		    // TODO
		} else unless (o = psyc_object(t) || find_psyc_object(t)) {
		    PT(("Found no recipent for %O. Dropping message.\n", t))
		    return 1;
		}
		
		break;
	    case 1:
		// this is a message to our rootmsg from someone
		source = lower_case(mvars["_source"]);
		break;
	    case 0:
		// going to our rootmsg from hell
		source = psycaddr;
		break;
	    default:
		PT(("Unimplemented routing-scheme (_source, _target and _context are all present)\n"))
		return 1;
	    }
	    // can this safely move into one of the ifs above? TODO
	    if (t = mvars["_source_relay"]) {
		if (t = psyc_object(t)) {
		    P2(("local _source_relay %O from %O, cont %O\n",
			 t, source, mvars["_context"]))
		    mvars["_source_relay"] = t;
		}
	    }
	    
	    unless (mc = spsyc_parse(&data, psycvars, iscontext, state, 
					     connection, o)) {
		if (connection)
		    destruct(connection);
		return 1;
	    }

# ifdef PSYC_TCP
	    P2(("TCP[%s] => %O: %s\n", peeraddr, o || t, mc))
# else
	    PT(("UDP[%s] => %O: %s\n", peeraddr, o || t, mc))
# endif
	
# ifndef GOOD_IDEA
	    // why do we parse for auth mcs here if we already do it
	    // in common::rootMsg? if this should be here instead,
	    // you can make these checks a bit more efficient if
	    // you first ensure that the target of this message is
	    // the server root. it's also nicer to the users that
	    // you're not looking at their messages here..  ;)
	    //
	    if (mc == "_error_invalid_authentication") {
		string n;
		unless (member(mvars, "_source") && 
			member(psycvars, "_location")) {
		    PT(("got _error_invalid_authentication without proper"
			"vars.\n"))
		    return 0;

		}
		n = "i:"+mvars["_source"]+psycvars["_location"];
		unless (qExists(n)) {
		    PT(("got _error_invalid_authentication for something we "
			"never requested.\n"))
		    return 0;
		}
		sendmsg(psycvars["_location"], "_error_invalid_source_identification",
			"Your identification [_location] seems to be bogus. "
			"Du kommst hier ned rein!\n", 
			([ "_location" : mvars["_source"]]));
		qDel(n); 
		return 1;
	    } else if (mc == "_notice_authentication") {
		string n;
		unless (member(mvars, "_source") && 
			member(psycvars, "_location")) {
		    PT(("got _notice_authentication without proper"
			"vars.\n"))
		    return 0;
		}
		n = "i:"+mvars["_source"]+psycvars["_location"];
		unless (qExists(n)) {
		    PT(("got _notice_authentication for something we "
			"never requested.\n"))
		    return 0;
		}
		register_location(psycvars["_location"], mvars["_source"]);
		while (qSize(n))
		    apply(#'deliver, shift(n));
		return 1;
	    }
# endif

	    // we could put these mmp-vars into cvars only to avoid this loop
	    foreach (string key : mvars) {
		if (mergeRoutingVar(key))
		    psycvars[key] = mvars[key];
	    }
	    PT(("deliver ending with target: %O and mc: %O\n", o, mc))

	    if (objectp(o))
		o -> msg(source, mc, (data == "" ? 0 : data), psycvars, 0, 
			 source);
	    else if (connection) {
		// per-connection "server root" which actually handles
		// connection feature negotiation
		connection -> rootMsg(source, mc, data == "" ? 0 : data, 
				      psycvars, source);
	    }
	     
	    return 1;
	}
}

/*
 * wrong order of arguments.. elsaga: how did this ever work!?
int msg(mixed target, string mc, mixed data, mapping vars,
		 mixed source, mixed target2) {
 */
int msg(mixed source, string mc, mixed data, mapping vars,
		 mixed showingLog, mixed target) {
	string sname;
	int port;
	string user, host, obj;
	string buf, room;
	object o;
	mixed t;
	array(mixed) u = parse_uniform(target);

	unless (u[UHost]) {
		sendmsg(source, "_failure_unsupported_notation_location",
		     "PSYC server-independent uniform network "
		     "identificators not implemented yet.", 0, 0);
		return 0;
	}
#if DEBUG > 0
	unless (stringp(mc)) {
		P0(("psyc_sendmsg got mc == %O from %O.\n"
		    "probably a mistake when converting a walk_mapping.\n",
		    mc, previous_object()))
		return 0;
	}
#endif
	sname = psyc_name(source);
//	host = lower_case(u[UHostPort]);
//	unless (u[URoot]) u[URoot] = "psyc://"+host;

	// TODO: no need for vars
	unless (mappingp(vars)) vars = ([  ]);
	P3(("looking for %O(%O) in %O..\n", target, u[UHostPort], targets))
//	if (member(targets, t2 = u[URoot])) {
//		t = targets[t2];
//		if (t) {
	unless (port = u[UPort]) port = PSYC_SERVICE;
	else if (port < 0) {
		sendmsg(vars["_context"] || source, 
			"_failure_network_connect_invalid_port",
		     "No connectable port known for [_source_relay].",
			 ([ "_source_relay" : target || u[UHost] ]), ME);
				// ME shouldn't be necessary! TODO
		return 0;
	}
	host = lower_case(u[UHost]);
	user = u[UUser];
	obj = u[UResource];
	// was: obj = u[UResource];
	// but probably caused the erratic rootMsg() events
	//obj = u[UUser] ? "~" + u[UUser] : u[UResource];
	//
//	if (host == SERVER_HOST) host = "127.0.0.1";
#if 1
//
// at this point i should be able to recognize targets which are my own!
// net/gateway/aim would use that..  TODO
//
	// see also legal_domain and legal_host
	if (query_udp_port() == port && (host == "127.0.0.1"
			|| host == "127.1"
#ifdef __HOST_IP_NUMBER__
			|| host == __HOST_IP_NUMBER__
#else
			|| host == myIP
#endif
		//	|| host == SERVER_HOST
		//	|| lower_case(host) == myLowerCaseHost
			|| host == my_lower_case_host() || host == "localhost")) {
	    if (stringp(obj)) {
		if (obj[<1] == '/') obj = obj[..<2];
		if (strlen(obj)) switch(obj[0]) {
		case '^':
		case '~':
			unless(user) user = obj[1..];
			break;
		case '$':
			// no services available yet
			// but here's the support for them
			obj[0..0] = "/service/";
			break;
		case '@':
			// t[0..0] = PLACE_PATH;
			obj = PLACE_PATH + lower_case(obj[1..]);
			if (o = find_object(obj)) break;
			// should check for legal name instead
			// of catch. TODO
			catch(obj -> load());
			// fall thru
		default:
			o = find_object(obj);
			D2( if(!o) D(S("OBJECT NOT FOUND: %O\n", t)); )
		} else unless(user) {
//			rootMsg(source, mc,
//			     buffer == "" ? 0 : buffer, cvars);
			return 0;
		}
	    }
	    // user@ is ignored if /@ or /~ is given..
	    if (user && !objectp(o)) {
		    o = summon_person(user); //, PSYC_PATH "user");
		    //o = find_person(user);
		    //unless(o) o = createUser(user);
	    }
	    // cache the resulting object for the url
	    register_target(lower_case(target), o);
	    return sendmsg(o, mc, data, vars, source);
	    // or deliver directly?
	}
#endif
	// okay so we have no existing connection..
	// current rule of thumb to decide whether to open up a tcp
	// also all localhost traffic should be handled by udp TODO
#ifdef _flag_enable_routing_UDP
	unless (u[UTransport] == "d" || abbrev("_notice", mc)) {
#endif
	    dns_resolve(host, (:
		object o;
		string addr = $1;
		string hopo = $2;
		string psycaddr = "psyc://"+ addr;

		// if ($3 && $3 != PSYC_SERVICE) {
		if ($9) {
			hopo += ":"+$9;
			addr += ":"+$9;
		}
		unless (o = find_target_handler(psycaddr)) {
		    o = ("psyc:"+hopo) -> circuit($2, $3, 0, "psyc", 0,
						  system_queue());
		}
		register_target($4, o);
		register_target("psyc://"+hopo, o);
		register_target(psycaddr, o);
		P2(("delivering %O(%O) over %O\n", hopo, $1, o))
		// psyc/circuit.c will register psyc://IP:PORT at logon()
		// the last thing missing is a way to register all CNAMEs
		return o->msg($5, $6, $7, $8, 0, $4);
	    // ip=1  2     3       4      5     6    7      8    9
	    :),
		host, port, target, sname, mc, data, vars, u[UPort]
	    );
	    return 1;
#ifdef _flag_enable_routing_UDP
	}
	// fall thru to UDP
# if DEBUG > 1
	if (port) {
		D(S("UDP[%s:%d] <= %O: %s\n", host, port, source, mc));
	} else {
		D(S("UDP[%s] <= %O: %s\n", host, source, mc));
	}
# endif
	if (objectp(source)) buf = make_psyc(mc, data, vars, source);
        else buf = make_psyc(mc, data, vars);
	buf = make_mmp(buf, vars);

	if (host == "localhost") return send_udp(host, port, buf);
	P3(("send_udp %O:%O packet:\n%O", host,port,buf))
	dns_resolve(host, (: if ($2 < 0) {
				send_udp($1, $2, $3);
			     }
			     return; :), port, buf);
	return 1;
#endif /* _flag_enable_routing_UDP */
}

object load() {
    return ME;
}

int outstate() {
    return 1;
}

mapping state() {
    return ([]);
}

void create() {
    qCreate();
}

#endif // }}}
