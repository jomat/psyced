// vim:foldmethod=marker:syntax=lpc:noexpandtab
// $Id: master.c,v 1.143 2008/04/22 22:43:56 lynx Exp $
//
// this is a simpler version of group/master, and it is actually in use
// [actual as in effectively, not current, which is the german meaning]
//
// part of the implementation of PSYC multicasting.. send stuff back where
// the _enter came from, send just one copy no matter how many have entered
// from there. the group/slave will take care of the other side.
//
// group/master and group/slave are used for smaller size rooms, where each
// person present should be visible. if you need a large scale multicast
// solution, build up a link network using place/master and place/slave.
//
// see http://about.psyc.eu/Routing for the big picture on routing.
//
// should we rename the directories into net/context/master and slave, since a
// context sounds more abstract than a group? i wouldn't say that a user's
// context delivering presence subscriptions is a "group", but it certainly
// is a "context."

// local debug messages - turn them on by using psyclpc -DDcontext=<level>
#ifdef Dcontext
# undef DEBUG
# define DEBUG Dcontext
#endif

#include <net.h>
#include <person.h>
#include <status.h>
#include <uniform.h>

#ifdef BETA
# define ENFORCE_UNIFORM
#endif

#ifdef CONTEXT_STATE // {{{
# define HEADER_ONLY
# include "../state.c"
# undef HEADER_ONLY
#endif // }}}

inherit NET_PATH "entity";

#ifdef HISTORY_COUNT
int _ccount; // persistent!
#endif

// these data structures are allocated for every single place and user
// as route is used by all of them to do presence or general smarticasting.
#ifdef PERSISTENT_MASTERS
mapping _routes;
#else
volatile mapping _routes, _u;
#endif

#ifdef CONTEXT_STATE // {{{
// why volatile.. that breaks the whole meaning of state
volatile mapping _costate, _cmemory;
volatile mapping ctemp, cunused;
#endif // }}}

#ifdef USE_SPYC
mapping _state; // an alternative to ifdef CONTEXT_STATE ?
#endif

#ifdef PERSISTENT_SLAVES
int revision; // persistent revision counter
#endif

stoned();

// used by /routes and /reload
routes(recover) {
	if (recover) {
		PT(("%O recovering routes %O\n", ME, recover))
		ASSERT("routes() recovering", mappingp(recover), recover)
		_routes = recover;
	}
	return _routes;
}

create() {
	_routes = ([ ]);
#ifdef HISTORY_COUNT
	_ccount = 0;
#endif
#ifdef CONTEXT_STATE // {{{
	_costate = ([ ]);
	ctemp = ([ ]);
	_cmemory = m_allocate(0, 2);
#endif // }}}
#ifdef USE_SPYC
	_state = ([ ]);
#endif
	::create();
}

// der castmsg muss irgendwie anders.. sonst kriegen wir den state nicht hin.
castmsg(source, mc, data, vars) {
	mixed route;
	mixed noa;	// Nickname of local user Or Amount of people on route
			// in TIDILY mode should be Mapping of people on route

	P2(("%O castmsg(%O,%O,%O..) for %O\n", ME, source,mc,data, _routes))
	D4(P2(("%O vars = %O\n", ME, vars)))
	// _context is a routing variable, so we use it internally with objectp
	vars["_context"] = ME;

#ifdef PERSISTENT_SLAVES
	vars["_number_revision"] = revision;
#endif

	// in theory.. m_delete(vars, "_source") but not necessary
	m_delete(vars, "_target");
#ifdef HISTORY_COUNT
	vars["_count"] = _ccount++;  // wraps at MAXINT.. right? hehe
#endif

	foreach (route, noa : _routes) {
#if defined(TIDILY) && defined(MEMBERS_BY_SOURCE)
	    P4(("place:each(%O,t=%O,s=%O,mc=%O,d=%O,v=%O)\n",
		    mappingp(noa) ? sizeof(noa) : noa, route, source, mc,
		    data, vars))
#else
	    P4(("place:each(%O,t=%O,s=%O,mc=%O,d=%O,v=%O)\n",
		    noa, route, source, mc, data, vars))
#endif
	    // if (route == source) return; // skip sender
	    if (!route) {
		    if (stringp(noa)) {
			    // object has been destructed
			    // maybe there's a new one
			    if (route = find_person(noa)) {
#ifndef PERSISTENT_MASTERS
#ifdef MEMBERS_BY_NICK
				    _u[noa] = route;
#else
				    _u[route] = noa;
#endif
#else
				    P1(("%O lost route to %O\n", ME, noa))
#endif
			    } else {
				    // why source? that's silly!
				    call_out(#'stoned, 1, source, noa);
				    continue;	// oops! why was this a return; !???
			    }
		    } else { // if (mappingp/intp(noa)) // noa contains (_amount_)members
			    monitor_report("_failure_destruct_circuit",
				psyctext("[_nick_place] detected the loss "
					 "of a circuit with [_amount_members] "
					 "members.",
				    ([	"_nick_place" : MYNICK,
#if defined(TIDILY) && defined(MEMBERS_BY_SOURCE)
					"_amount_members" : sizeof(noa),
#else
					"_amount_members" : noa,
#endif
					])));
			    m_delete(_routes, route);
		    }
	    }
	    P4(("group/master sees vars as %O\n", vars))
	    // btw, fippo, could you please resume what happens if we
	    // try not to copy(vars) ? i like docs in the middle of source
	    // that give me a reason not to break it  ;)
	    if (objectp(route)) {
		    route -> msg(source, mc, data, copy(vars));
	    } else {
		    // gateways to other schemes please make their own
		    // copy of the vars if they need to mess with them.
		    sendmsg(route, mc, data, vars, source);
	    }
	}
	return 1;
}

#if 0 //def PERSISTENT_MASTERS
protected load(file) {
	::load(file); 
	DT(if (sizeof(_routes))
	   PP(("Found routes in %O: %O\n", ME, _routes || "nothing"));)
}
#endif

#if defined(TIDILY) && defined(MEMBERS_BY_SOURCE)
# define RM(SOURCE, ORIGIN) \
	m_delete(_routes[ORIGIN], SOURCE); \
	unless (sizeof(_routes[ORIGIN])) m_delete(_routes, ORIGIN);
#else
# define RM(SOURCE, ORIGIN) \
	if (_routes[ORIGIN]) \
	    unless (--_routes[ORIGIN]) m_delete(_routes, ORIGIN);
#endif

remove_member(source, origin) {
	mixed t;
#ifdef ENFORCE_UNIFORM
	if (objectp(source)) source = psyc_name(source);
#endif
	P2(("%O remove_member(%O, %O)\n", ME, source, origin))
	if (origin && (
#if 1 //defined(TIDILY) && defined(MEMBERS_BY_SOURCE)
	   stringp(t = origin) ||
#endif
	   t = origin->qOrigin())
	   && member(_routes, t)) {
		RM(source, t);
	} else if (member(_routes, source)) {
		// local object or xmpp user
		m_delete(_routes, source);
	} else if (stringp(source)) {
		// guessing route from hostname of psyc address
		// try to avoid using remove_member() without origin
		// but this becomes necessary when we want to
		// remove an object or route manually
		mixed u;

		if (u = find_target_handler(source)) {
			t = u->qOrigin();
			RM(source, t);
			P1((
		"%O guessing origin %O for %O, successful? routes are %O\n",
			    ME, t, source, _routes))
		} else {
			t = 0;
			if (u = parse_uniform(source)) t = u[URoot];
			unless (t) t = source;
			/* 
<fippo> das hier geschah, als ich bei frischgestartetem server mit einem remote
jabberisten die freundschaft zum user#fippo entfernte:
    EXCEPTION at line 185 of net/group/master.c in object net/psyc/user#fippo:
    Bad arg 1 to m_delete(): got 'number', expected 'mapping'.
<lynX> die datenstruktur war nicht initialisiert? da muss doch vorher schon
    an anderer stelle was schiefgelaufen sein...?
			 */
			RM(source, t);
			P1((
		"%O parsing origin %O from %O, successful? routes are %O\n",
			    ME, t, source, _routes))
		}
	} else {
		// happens when doing /unfr. it's when the implied unfriend
		// comes back from the other side.
		P2(("%O encountered unnecessary remove of %O from %O\n",
		    ME, source, _routes))
	}
	P3(("%O -> _routes = %O\n", ME, _routes))
}

insert_member(source, origin) {
    mixed t;

#ifdef ENFORCE_UNIFORM
    // when storing objects into _routes stupid things can happen:
    // when a user is created, all her local friends are cloned and
    // placed into her context. if one of those local friends logs in
    // under different access a different user object is created -
    // the old one is destroyed and, guess what, this context mapping
    // is not updated. always using uniforms is a way to solve this
    // problem.
    //
    if (objectp(source)) source = psyc_name(source);
#endif
    P2(("%O insert_member(%O, %O)\n", ME, source, origin))
    if (objectp(origin) && (t = origin->qOrigin())) {
#if defined(TIDILY) && defined(MEMBERS_BY_SOURCE)
	unless (member(_routes, t)) _routes[t] = m_allocate(1, 0);
	m_add(_routes[t], source);
#else
	_routes[t]++;
#endif
    } else if (stringp(origin)) {
#if defined(TIDILY) && defined(MEMBERS_BY_SOURCE)
	unless (member(_routes, origin)) _routes[origin] = m_allocate(1, 0);
	m_add(_routes[origin], source);
#else
	_routes[origin]++;
#endif
    } else {
#ifdef PERSISTENT_MASTERS
	_routes[source] = 1; // gimme the nick
#else
	_routes[source] = 1; // castmsg expects the nick here. duh.
			     // should we do that for objectp(source)?
#endif
    }

#ifdef PERSISTENT_SLAVES
    // would be nice to do it here, but that's not correct
    //revision += 1;
#endif
    P3(("%O -> _routes = %O\n", ME, _routes))
}

#ifdef USE_SPYC
get_state() {
	PT(("cstate for %O picked up by %O: %O\n", ME,
	    previous_object(), _state))
	return _state;
}
commit_state() {
	PT(("cstate for %O committed by %O: %O\n", ME,
	    previous_object(), _state))
	_state = ([ ]);
}
#endif

// code duplicaton is faster than others
#ifdef CONTEXT_STATE // {{{
//
// <lynX> code duplication is okay for enhanced efficiency, but please not
//        by mouse pasting. use #include "shared_code.i" please. TODO.
//        you can even ifdef small differences of the two versions!
//
int outstate(string target, string key, mixed value, int hascontext) {
    int mod;

    unless (hascontext) 
	return ::outstate(target, key, value, hascontext);
    
    if (key[0] == '_') {
	mod = ':';
    } else {
	mod = key[0];
	key = key[1..];
    }

    unless (ctemp) ctemp = ([ ]);

    unless (cunused) cunused = copy(_costate);
    m_delete(cunused, key);

    switch(mod) {
    case ':':
	if (_cmemory[key, 1] == -1) break;
	if (member(_cmemory, key)) {
	    if (_cmemory[key] == value) {
		if (_cmemory[key, 1] == STATE_MAX2) { 
		    break;
		}
		if (_cmemory[key, 1] == STATE_MIN2) {
		    ctemp["="+key] = value;
		    _costate[key] = value;
		    return 0;
		}
		_cmemory[key, 1]++;
	    } else if (_cmemory[key, 1] - 1 == STATE_MIN2) {
		ctemp["="+key] = "";
		m_add(_cmemory, key, value, 1);
	    } else if (_cmemory[key, 1] <= STATE_MIN2) {
		m_add(_cmemory, key, value, 1);
	    } else _cmemory[key, 1]--; 
	} else m_add(_cmemory, key, value, 1);
	break;
    case '=':
	if ("" == value) {
	    m_delete(_costate, key);
	    break;
	}
	if (member(_costate, key) && _costate[key] == value) {
	    return 0;
	}
	m_add(_cmemory, key, value, -1);
	_costate[key] = value;
	break;
    case '+':
	_augment(_costate, key, value);
	break;
    case '-':
	_diminish(_costate, key, value);
	break;
    default:
	raise_error("Illegal variable modifier in '" + key + 
		    "' encountered in group/master:outstate.\n");
	return 0;
    }
    return 1;
}

mapping state(string target, int hascontext) {
    mapping t2 = ([ ]), t;

    unless (hascontext) return ::state(target, hascontext);

    if (cunused) {
	foreach (string key : cunused) {
	    t2[":" + key] = "";
	}

	cunused = 0;
    }

    t = ctemp;
    ctemp = ([ ]);

// rock hard optimization
    unless (sizeof(t))
	return t2;
    unless (sizeof(t2))
	return t;
    return t2 + t;
}

#endif // }}}
