// vim:foldmethod=marker:syntax=lpc:noexpandtab
// $Id: library.i,v 1.172 2008/07/26 10:54:31 lynx Exp $

#include <psyc.h>
// #include "../base64.i"
#define QUIT	destruct(ME); return 1;

#if SYSTEM_CHARSET != "UTF-8"
# echo "FIXME: PSYC will be not be rendered in UTF-8."
#endif

volatile mapping obj2unl = ([]);
volatile mapping unl2obj = ([]);
// global location to identification resolver
volatile mapping unl2uni = ([]);

varargs void register_location(mixed unl, vamixed uni, vaint unsafe) {
	PROTECT("REGISTER_LOCATION")
	if (uni) {
	    if (unsafe && unl2uni[unl]) {
		P1(("register_location denied: %O says it is %O, but that is already registered for %O\n", uni, unl, unl2uni[unl]))
		return;
	    }
	    unl2uni[unl] = uni;
	}
	else m_delete(unl2uni, unl);
}

#ifndef _flag_disable_module_authentication
/** finds the _identification for a network source. if it cannot find
 ** the information in the cache, and a second argument is given with
 ** the _identification claimed by the source, that _identification
 ** will be asked for confirmation via PSYC. the result will obviously
 ** be available only at a later time. there is no callback function
 ** to this purpose. this function will ONLY return an identification
 ** when it is effectively different from the provided source.
 ** when a source is its own identification, this returns to avoid
 ** any extra processing. maybe one day we will return 1 if that's
 ** useful in any way, but you can check source == givenUNI yourself.
 ** the reason why the variable is called givenUNI and not identification
 ** is because a client may very well claim to be someone and actually not
 ** be that person. we are not sure about identification before the
 ** _request_authentication has returned.
 */
// this code is used intensely in FORK mode, whereas in !FORK,
// since entity.c, the givenUNI is never passed, thus inactive.
// unl2uni is only being used for local users' clients, not remotes
//
varargs mixed lookup_identification(mixed source, vamixed givenUNI) {
	PROTECT("LOOKUP_IDENTIFICATION")
	P2(("lookup_identification %O (%O) in %O\n", source, givenUNI, unl2uni))
	if (member(unl2uni, source)) return unl2uni[source];
	// ask a remote uni, but don't wait to complete
	if (givenUNI && source) {
		unl2uni[source] = 0; // ensures that we don't get here twice
			// now and in future always return 0 to avoid extra
			// processing of
		if (givenUNI == source) return 0;
                P1(("%O sending _request_authentication to %O for %O\n",
                    ME, givenUNI, source))
# ifndef __PIKE__ // TPD
		sendmsg(givenUNI, "_request_authentication", 0,
		    ([ "_location" : source ]));
# endif
	}
	// we could look again  ;)
	//if (unl2uni[source]) return unl2uni[source];
	return 0;
}
#endif

varargs string psyc_name(mixed source, vastring localpart) {
	P3((">> psyc_name(%O, %O)\n", source, localpart))
	string s;

	if (s = obj2unl[source]) return s;
#if DEBUG > 0
	if (stringp(source))
            raise_error("psyc_name called for string "+ source+ "\n");
#else
	if (stringp(source)) return source;
#endif
#ifdef PARANOID
	unless (objectp(source)) return 0;
#endif
	unless (s = localpart) {
		if (s = source->psycName()) {
			if (abbrev("mailto:", s)) {
				unl2obj[s] = source;
				return obj2unl[source] = s;
			}
		} else s = file_name(source);
	}
	//
	// since this is an outgoing UNL and we do not do
	// virtual psyc hosting we should be
	// allowed to leave out the myUNL here.
	// since we need to recognize ourselves in _context messages
	// even if our _source has been repatched, we need to know
	// all our "canonical" names. so this isn't even enough.
	// we probably need to our canonical hostname: myUNLCN.
	//
#ifndef __HOST_IP_NUMBER__
//	if (myUNLIP)
#endif
//	     unl2obj[myUNLIP +"/"+ s] = source;
	    // why store it with an ip? nobody uses that anyway!?
	    // is it being used anywhere in the parser? naaah.
	s = myUNL + s;
	P3((">>> psyc_name for %O is %O\n", source, s))
	unl2obj[s] = source;
	return obj2unl[source] = s;
}

// in most situtation this is correct function to find local psyc objects
object psyc_object(string uniform) {
        // unl2obj can become too big and cause a sprintf error here
        // so you don't want this debug output on a production server!
	//PT(("psyc_object(%O) in %O\n", uniform, unl2obj))
	return unl2obj[uniform];
}

// you probably want to use the much simpler psyc_object() above
object find_psyc_object(array(mixed) u) {
	P3((">> find_psyc_object(%O)\n", u))
	string t, r, svc, user;
	object o;

	user = u[UNick];
	r = u[UResource];
	if (r && strlen(r)) {
#if __EFUN_DEFINED__(trim)
# include <sys/strings.h>
		r = trim(r, TRIM_RIGHT, '/');
#else
		while (char_from_end(r, 1) == '/') r = slice_from_end(r, 0, 2);
#endif
		if (strlen(r)) switch(r[0]) {
		case '^':
			break;
		case '~':
#ifdef _flag_enable_module_microblogging
			if (u[UChannel]) {
			    t = lower_case(r +"#"+ u[UChannel]);
			    r = PLACE_PATH + t;
			    if (o = find_object(r)) break;
			    unless (t = legal_name(t, 1)) break;
			    // untreated catch? interesting..
			    catch(o = r -> load(t));
			}
#endif
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
//			unless (user) {
//			    croak("_failure_unavailable_service",
//    "No service '[_service]' available here.", ([ "_service" : svc ]) );
//			    return restart();
//			}
			return 0;		// TODO!
#ifndef __PIKE__
		case '@':
			// t[0..0] = PLACE_PATH;
			r = PLACE_PATH + lower_case(r[1..]);
			if (o = find_object(r)) break;
			// should check for legal name instead
			// of catch. TODO
			catch(o = r -> load());
			if (objectp(o)) break;
			// fall thru
#endif
		default:
			o = find_object(r);
			D2( if(!o)
			    D("OBJECT NOT FOUND: "+ r +"\n"); )
		}
	}
	else unless (user) {
		//return 0;	// return SERVER_UNIFORM !?
		return find_target_handler("/");
	}
	if (!objectp(o) && user) {
		// psyc/parse used to do this here
		o = summon_person(user); //, PSYC_PATH "user");
                P2(("%O summoning %O: %O\n", ME, user, o))
	}
	P3((">>> found psyc object: %O\n", o))
	return o;
}

#ifndef __PIKE__
// library transmits udp packets itself
#include "render.i"
#endif

// target is lowercased already
int psyc_sendmsg(mixed target, string mc, mixed data, mapping vars,
		 int showingLog, mixed source, array(mixed) u) {
	string sname, host, buf, room;
	int port, usesrv = 1;
	object ob;
	mixed t;

	unless (u[UHost]) {
#ifndef __PIKE__ // TPD
		sendmsg(source, "_failure_unsupported_notation_location",
		     "PSYC server-independent uniform network "
		     "identificators not implemented yet.", 0, 0);
#endif
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
	sname = objectp(source) ? psyc_name(source) : sname;
//	host = lower_case(u[UHostPort]);
//	unless (u[URoot]) u[URoot] = "psyc://"+host;

	// TODO: vars should be enforced system-wide
	unless (mappingp(vars)) vars = ([  ]);

	// seid ihr euch wirklich sicher, dass ihr diese zeile entfernen wollt?
	vars["_target"] = target;
	// <lynX> this could help protect against sources who destruct while
	// the message is in the queue. is it being used or is it okay to do
	// this operation at enqueing time? in fact the templates who use it
	// get it from psyctext() which resolves _source as source.. so this
	// here should not be necessary.. then again we just calculated it
	// so we don't want it to be calculated again later.. so we should
	// rather adapt the other spots where it's used rather than here..
	// after all it's mostly speed improvements.. no big thing.. TODO
	vars["_source"] = sname;

	unless (port = u[UPort]) port = u[UTransport] == "s" ?
					PSYCS_SERVICE : PSYC_SERVICE;
	else {
	    usesrv = 0;
	    if (port < 0) {
		P2(("Minusport in %O or %O, _failure to %O || %O\n",
		    target, u[UHost], vars["_context"], source))
#ifndef __PIKE__ // TPD
		sendmsg(vars["_context"] || source, 
			"_failure_network_connect_invalid_port",
		     "No connectable port known for [_source_relay].",
			 ([ "_source_relay" : target || u[UHost] ]), ME);
				// ME shouldn't be necessary! TODO
#endif
		return 0;
	    }
	}
	host = lower_case(u[UHost]); // lower_case not necessary.. right?
	if (query_udp_port() == port && is_localhost(host)) {
	    // this happens when a psyc client sends to a local
	    // target that hasn't been incarnated yet...
	    ob = find_psyc_object(u);
	    // cache the resulting object for the url
	    if (ob) {
	P2(("psyc_sendmsg registering %O for %O found by parsing uniform\n",
                        target, ob))
                    register_target(target, ob);
            }
#ifndef __PIKE__ // TPD
	    return sendmsg(ob, mc, data, vars, source);
#endif
	    // or deliver directly?
	}
	// since the local URoot is in targets and pointing to net/root
	// we have to get here *after* making sure we are not addressing
	// a local entity.
	P3(("looking for %O(%O) in %O..\n", target, u[UHostPort], targets))
//	if (member(targets, t2 = u[URoot])) {
//		t = targets[t2];
//		if (t) {
	if (t = targets[u[URoot]]) {
			P2(("%O to be delivered on %O's circuit\n",
				    target, query_ip_name(t) || t ))
			register_target(target, t);
			    // was delivermsg(
			return t->msg(sname, mc, data, vars, 0, target);
// no more cleansing of targets mapping.. see also net/library.i
//		} else {
//			P1(("PSYC/TCP target %O gone!\n", t2))
//			m_delete( targets, t2 );
//		}
	}
#ifndef __PIKE__
	// okay so we have no existing connection..
	// current rule of thumb to decide whether to open up a tcp
	// also all localhost traffic should be handled by udp TODO
#ifdef _flag_enable_routing_UDP
	if (u[UTransport] == "c" ||
# ifndef _flag_disable_routing_TLS
	    u[UTransport] == "s" ||
# endif
	   (!u[UTransport] && !abbrev("_notice", mc)))
#else
	if (!u[UTransport] || u[UTransport] == "c"
# ifndef _flag_disable_routing_TLS
			   || u[UTransport] == "s"
# endif
	)
#endif
       	{
	    dns_resolve(host, (:
		object o;
		string hopo = $2;
		string psychopo;
		string psycippo = "psyc://"+ $1 +"/";

		// if ($3 && $3 != PSYC_SERVICE) {
		if ($9) {
			hopo += ":"+$9;
			psycippo += ":"+$9;
		}
		// hope it is correct to have a trailing slash here
		psychopo = "psyc://"+ hopo +"/";
		unless (o = find_target_handler(psycippo)) {
		    // we use psyc:host:port as an object name
		    // we can change that if it is confusing
#ifdef QUEUE_WITH_SCHEME
		    // this makes it look for psyc:host:port as its queue
		    o = ("psyc:"+hopo) -> circuit($2, $3, u[UTransport],
					    usesrv && "psyc", 0, systemQ, psychopo);
#else
		    // this makes it look for host:port as its queue
		    o = ("psyc:"+hopo) -> circuit($2, $3, u[UTransport],
					    usesrv && "psyc", hopo, systemQ, psychopo);
#endif //__PIKE__
		    // SRV tag "psyc" is actively being checked, but
		    // don't rely on it.. just use it as a fallback if
		    // nothing else is possible, but some clients may
		    // no longer be able to connect to you...
		}
#ifdef USE_SPYC
		o -> sender_verification(SERVER_UNIFORM, u[URoot]);
#endif
		register_target($4, o);
		register_target(psychopo, o);
		register_target(psycippo, o);
		P2(("delivering %O(%O) over %O\n", hopo, $1, o))
		P3(("targets = %O\n", targets))
		// psyc/circuit.c will register psyc://IP:PORT at logon()
		// the last thing missing is a way to register all CNAMEs
		return o->msg($5, $6, $7, $8, 0, $4);
	    // ip=1  2     3       4      5     6    7      8    9
	    :),
		host, port, target, sname, mc, data, vars, u[UPort]
	    );
	    return 1;
	}
	else
#ifdef _flag_enable_routing_UDP
	    if (u[UTransport] && u[UTransport] != "d")
#endif
	{
		P1(("Invalid transport %O in %O, _failure to %O || %O\n",
		    u[UTransport], target, vars["_context"], source))
		sendmsg(vars["_context"] || source, 
			"_failure_network_connect_invalid_transport",
		     "Unknown transport type '[_transport]' for [_source_relay].",
			 ([ "_source_relay" : target || u[UHost],
			    "_transport": u[UTransport] ]), ME);
				// ME shouldn't be necessary! TODO
		return 0;
	}
#ifdef _flag_enable_routing_UDP
	// fall thru to UDP
# if DEBUG > 1
	if (port) {
		D(S("UDP[%s:%d] <= %O: %s\n", host, port, source, mc));
	} else {
		D(S("UDP[%s] <= %O: %s\n", host, source, mc));
	}
# endif
#ifndef NEW_RENDER
	// this produced wrong _length. please discontinue.
	if (stringp(data) && strlen(data))
                    // why do we take guesses at data here? uncool!
	    data="\n"+data+ (data[strlen(data)-1] == '\n' ?
		      S_GLYPH_PACKET_DELIMITER "\n" :
		 "\n" S_GLYPH_PACKET_DELIMITER "\n");
	else data="\n" S_GLYPH_PACKET_DELIMITER "\n"; // TODO? look up textdb.

	if (room = vars["_context"]) {
		// this may have to change into a full psyc: URL
		if (objectp(room)) room = psyc_name(room);
		buf = S_GLYPH_PACKET_DELIMITER "\n"
		       	":_source_relay\t"+ sname +"\n"
		      + ":_context\t"+ room +"\n";
	} else
	    buf = S_GLYPH_PACKET_DELIMITER "\n"
		  ":_source\t"+ sname +"\n"
		  ":_target\t"+ target +"\n";
	t = render_psyc(source, mc, data, vars, showingLog, target);
	unless (t) return 0;
	buf += t;
#else
	buf = render_psyc(source, mc, data, vars, showingLog, target);
	unless (buf) return 0;
#endif /* NEW_RENDER */

	// we could store the result of the is_localhost above, right?
	if (is_localhost(host)) return send_udp(host, port, buf);
	PT(("dns_resolve + send_udp %O:%O packet:\n%s", host,port,buf))
	dns_resolve(host, (: if (stringp($1))
				send_udp($1, $2, $3);
			     else
				// if your driver complains about vars being
				// undefined at this point, you are probably
				// running an ldmud 3.2 which is not compatible
				// with psyced. pick a newer major version pls
				sendmsg(vars["_context"] || source, 
				    "_failure_network_send_resolve",
		     "[_source_host] for [_source_relay] does not resolve.",
				     ([ "_source_host" : host,
				       "_source_relay" : target||host ]), ME);
			    return; :), port, buf);
	return 1;
#endif /* _flag_enable_routing_UDP */
#endif // PIKE
}

/* this breaks /connect ...
object createUser(string nick) {
	PT(("creating " PSYC_PATH "user for "+ nick +"\n"))
	return named_clone(PSYC_PATH "user", nick);
}
*/

