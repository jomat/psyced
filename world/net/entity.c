// vim:foldmethod=marker:syntax=lpc
// $Id: entity.c,v 1.127 2008/08/05 12:24:16 lynx Exp $
//
// entity.c: anything that has a UNI (see http://about.psyc.eu/uniform)
// this file handles some low-level issues with being an entity:
//
// * resolve UNLs and UNRs into UNIs and back, so higher level
//   processing knows who they are dealing with, no matter which
//   agent operated on this person's behalf
// * state: handle the inter-entity MMP state variables, packet
//   ids and message history
// * trust network: figure out who we trust and who we can ask to find
//   out if someone is trustworthy etc etc

// local debug messages - turn them on by using psyclpc -DDentity=<level>
#ifdef Dentity
# undef DEBUG
# define DEBUG Dentity
#endif

#include <net.h>
#include <storage.h>
#include <uniform.h>

#ifdef ENTITY_STATE //{{{
# define HEADER_ONLY
# include "state.c"
# undef HEADER_ONLY
#endif //}}}

#ifndef MINIMUM_TRUST
# define MINIMUM_TRUST 5
#endif
#ifndef MAXIMUM_TRUST
# define MAXIMUM_TRUST 10
#endif

#ifdef NEW_QUEUE
inherit NET_PATH "queue2";
#else
inherit NET_PATH "queue";
#endif
inherit NET_PATH "name";
inherit NET_PATH "storage";
#ifdef ENTITY_STATE //{{{
volatile mapping _state, _ostate, _cstate, _icount, _count, _memory;
volatile mapping temp, unused;
#endif //}}}

// this ifdef disables uni2unl, yet it doesn't remove all the queuing
// and async auth requests. should it?
#ifdef UNL_ROUTING //{{{
// while unl2uni in psyc/library is currently only used for local users'
// clients (and by FORK code, which in exchange doesn't use this code here)
// this is the place where general UNI to UNL mapping is made.
//
// each user has its own because a UNI may choose to tell different people
// different things.. in fact, a UNL may have reasons to propose different
// UNIs, too, but right now we're only being half consequent.. also there
// are no known applications making use of this feature anyhow  ;)
volatile mapping uni2unl;
# ifdef USE_THE_RESOURCE
volatile string bare;
# endif
#endif //}}}

volatile string identification;
volatile mixed _tag, _source_tag;

#ifdef TAGGING
volatile protected mapping _tags;

/* currently, only net/jabber needs this */
void chain_callback(mixed tag, mixed chain_callback) {
    if (_tags[tag]) 
	_tags[tag] = ({ chain_callback }) + _tags[tag];
    else 
	_tags[tag] = ({ chain_callback });
}

mixed execute_callback(mixed tag, mixed cbargs) {
    mixed ret, cbchain;
    // API: return 0 if there was no callback, callback return value or 1 if successful ?
    unless(_tags[tag]) return 0;
    ret = cbargs;
    cbchain = _tags[tag];
    m_delete(_tags, tag);
    foreach(mixed cb : cbchain) {
	ret = apply(cb, ret);
    }
    return ret || 1;
}
#endif

#ifdef ENTITY_STATE //{{{
inherit NET_PATH "state";

// lpc really needs an inline modifier!
# define STATE(context)	((context) ? _cstate : _state)
# define OSTATE(target)	(member(_ostate, target) \
			 ? _ostate[target] \
			 : _ostate[target] = ([ ]))
# define OMEMORY(target)	(member(_memory, target) \
				 ? _memory[target] \
				 : _memory[target] = m_allocate(0, 2))
#endif //}}}

#ifndef _flag_disable_module_trust
// first try to implement trust for remote ( means.. not a direct peer
// inside the friendsnet ) objects
volatile mapping _trust;

// returns -1 if there was no information for somebody...
// otherwise the level of trust \elem [0,10]
//
// is supposed to be overloaded by person/place for ignrores/friendships
// and banned people
int get_trust(string who, string by_whom) {
    mixed t;
    unless (member(_trust, who)) return -1;

    if (by_whom) {
	if (t = psyc_object(by_whom)) by_whom = t->qName();

	unless (member(_trust[who], by_whom)) return -1;	
	t = get_trust(by_whom, 0);

	if (t <= 0)
	    return 0;
	return _trust[who][by_whom]* t / MAXIMUM_TRUST;
    }

    // should be cache the maximum.. maybe yes.. otherwise we end up
    // calculating this stuff over and over again
    return max(m_values(_trust[who]));
}
#endif

sendmsg(target, mc, data, vars, source, showingLog, callback) {
    string t;

    P3(("uni(%O): sendmsg(%O, %O, %O, %O, %O, %O)\n", ME, target, mc, data, vars, source, showingLog))

#ifdef UNL_ROUTING //{{{
    if (t = uni2unl[target]) {
	// maybe we should keep the _identification of the target somewhere
	// in vars. but where? _target_identification ? and who needs that?
	//
	// great, if I ping xmpp:fippo@amessage.de this makes 
	// it xmpp:fippo@amessage.de/foo, but I DONT WANT THAT!
	//
	// also in order to allow for multiple psyc clients we need
	// this if here, but i think we should rather ensure these
	// candidates are not in uni2unl[]..	--lynX
#if 1 //FIXME
	if (!objectp(target) || (objectp(target)
				 && target->vQuery("scheme") != "psyc"))
#endif
	    target = t;
    }
#endif //}}}
#ifdef FORK //{{{
# if 0
    if (vars) { // shouldn't _count be set for msgs with empty vars, too?
	// these are overwritten by source, target. maybe we should stop
	// objects from reusing vars since that could easily be abused
	// to broatcast junk.
	m_delete(vars, "_source");
	m_delete(vars, "_target");
	// <lynX> why are you manually deleting some vars here which are also
	// defined in isRouting and do not delete _context or _counter?
    } else vars = ([ ]); 
# endif
#endif //}}}
    if (mappingp(vars)) {
	    // uhm. better rename simul-efun-sendmsg and let sendmsg be defined in
	    // each object, doing return real_sendmsg(...); .. agree, but have to
	    // figure out a name for it.. maybe system_sendmsg? or submitmsg?
	    if (_tag && target == _source_tag &&! vars["_INTERNAL_tag_skip"]) {
		    unless (vars) vars = ([]);
		    vars["_tag_reply"] = _tag;
		    unless (vars["_INTERNAL_tag_again"]) _tag = 0;
	    }
#if 1 //DEBUG > 2
    } else {
	// dump trace w/out error...!?
	P1(("uni:sendmsg(%O) called without vars\n", mc))
	// vars = ([ ]); -- server works better with debug? no way!
	//raise_error("uni:sendmsg called without vars\n");
#endif
    }
#ifdef TAGGING
    if (callback) {
	// if the caller supplies a _tag, it is responsible for 
	// uniqueness within the user object
	if (!vars["_tag"]) {
	    while(_tags[vars["_tag"] = RANDHEXSTRING]);
	}
	_tags[vars["_tag"]] = ({ callback });
    }
#endif
    return library_object()->sendmsg(target, mc, data, vars, source, showingLog);
}

#ifdef EXPERIMENTAL
// it's here, it may be nice or not.. we don't know..
// because nothing is using it 
// <fippo> nothing is using it, because commiting code
// 	without any infrastructure is pointless.
mapping reply(mapping vars) {
    mapping rvars = ([]);

    if (member(vars, "_tag")) 
	rvars["_tag_reply"] = vars["_tag"];
    if (member(vars, "_INTERNAL_source_resource"))
	rvars["_INTERNAL_target_resource"] = vars["_INTERNAL_source_resource"];
    if (member(vars, "_INTERNAL_target_resource"))
	rvars["_INTERNAL_source_resource"] = vars["_INTERNAL_target_resource"];

    rvars["_target"] = vars["_source_identification_reply"] || vars["_source_reply"] || vars["_source"];
    return rvars;
}
#endif

msg(source, mc, data, vars) {
    string t, sid;

    P3(("uni::msg(%O, %O, %O, %O)\n", source, mc, data, vars))
#ifdef _flag_log_flow_messages
    log_file("FLOW_MESSAGES", "%s %O ( %O « %O )\n", mc, strlen(data),
	     ME, source);
#endif
#ifdef UNL_ROUTING //{{{
# ifdef USE_THE_RESOURCE
    // too expensive to m_delete if this is going to be re-set anytime soon
    if (bare) bare = uni2unl[bare] = 0;
# endif
#endif //}}}
#ifdef CAST_STATE
    // FIXME: we've got the cast state already made up by the
    // cslave, but we need to route it down to the client...
    // if there is any!
#endif

    // deletes _tag if none is available
    if (_tag = vars["_tag"]) {
	P2(("tag %O for %O stored in %O\n", _tag, source, ME))
	_source_tag = source;
    }
#ifdef TAGGING
      // allow _tag to trigger this? this is to circumvent some bug
      // that doesn't provide us with _tag_reply as it should.. huh?
      // but maybe it's okay anyway.. we'll see
      //
      // <fippo>: it is a bug on psyced.org only. As soon as you fix it,
      //          I will happily remove the || _tag
      // <fippo>: not on psyced.org only. seems to happen if a second remote
      // 	joins
    if ((t = vars["_tag_reply"] || vars["_tag"]) && _tags[t]) {
	if (execute_callback(t,
	     ({ source, mc, data, vars }))) return 0;
    }
#endif

#ifdef FORK //{{{
    mixed route = vars["_context"] || source;

    unless(member(_icount, route)) _icount[route] = 0;
    if (member(vars, "_count")
	    && vars["_count"] != to_string(_icount[route]++)) {
	sendmsg(route, 0, 0, ([ "_count" : 0 ]));
	if (vars["_count"] == "0") {
	    Reset(route, vars["_context"]);
	    _icount[route] = 0;
	}
    }
    m_delete(vars, "_count");
    m_delete(vars, "_target"); // TODO:: find out if there's a better solution,
			       // like not having target in the vars at all.
    
    if (member(STATE(vars["_context"]), route)) {
	//vars = _state[source] + vars; // overwrite by :
	P3(("STATE-VARS: %O\n", STATE(vars["_context"])[route]));
	foreach (mixed x, mixed y : STATE(vars["_context"])[route])
	    unless (member(vars, x)) vars[x] = y;
    }
#else //FORK }}}
# ifndef _flag_disable_module_authentication
    // person.c only calls this for stringp(source), so why check here again?
    // because place/basic.c calls this for all sorts of sources. why this
    // inconsistency? and what about local string sources? TODO
    unless (objectp(source)) {
	if (abbrev("_notice_authentication", mc)) {
	    string l = vars["_location"];
	    unless (qExists(l)) return 0;
#ifdef UNL_ROUTING //{{{
	    uni2unl[source] = l;
#endif //}}}
	    // i guess this should have been here.. let's try that
	    register_location(l, source, 1);
	    while (qSize(l))
		apply(#'msg, shift(l));
	    qDel(l);
	    return 0;
	} else if (abbrev("_error_invalid_authentication", mc)) {
	    string l = vars["_location"];
	    P1(("_error_invalid_authentication (%O) for %O (location does "
		"%Oexist)\n", ME, l, qExists(l) ? "" : "not"))
	    if (qExists(l)) qDel(l);
	    return 0;
	} else if (sid = vars["_source_identification"]) {
//		   && !(abbrev("_status_place_identification", mc)
//			|| abbrev("_status_description_person", mc))
		//   && !abbrev("_status", mc))
	    if (sid == source) {
		    t = 0;
		    // redundant _source_identification, pls fix the sender
		    // <fippo> I like debug code breaking...
		    //log_file("LUNITAR", "%O in %O (%O)\n", sid, ME, v("agent"));
		    // return in this case?
	    }
#ifdef UNL_ROUTING //{{{
	    else if (uni2unl[sid] == source) t = sid;
# ifdef USE_THE_RESOURCE
	    // our jabber gateway doesn't lie about identifications.
	    // just use it instead of the UNR
	    else if (vars["_INTERNAL_identification"]) {
		// this must not get active if _nick_place is set!
		if (!vars["_nick_place"]) {
		    bare = t = sid;
		    uni2unl[t] = source;
		    P2(("USE_THE_RESOURCE in %O: use %O instead of %O\n",
			ME, t, source))
		} else
		    t = source;
	    }
# endif
#endif //}}}
	    else {
		// this will not trigger a systemwide _request_authentication
		t = lookup_identification(source);
		if (t == sid) {
#ifdef UNL_ROUTING //{{{
		    uni2unl[t] = source;
#endif //}}}
		} else unless (objectp(t)) {
	// we should intercept when the UNI is claimed to be on the
	// same server where the location is - that's always legitimate
	// and at the same time plain humbug. so we can either accept
	// it or reject it. we shouldn't issue a _request_authentication
	// in that case.  TODO: FIXME QUICK  ;)
		    P3((">>> qExists %O %O\n", qExists(source), source))
		    unless (qExists(source)) {
                        P1(("%O sending _request_authentication to %O for %O\n",
                                                ME, sid, source))
			library_object()->sendmsg(sid, "_request_authentication", 0,
						  ([ "_location" : source ]));
			qInit(source, 30, 5);
		    }
		    enqueue(source, ({ source, mc, data, vars }));
		    return 0;
		}
	    }
	    if (t) {
#  if 1
		// we can either decide to see our own locations as source
		// since that's what the code in person.c already does, we'll
		// try this option first
		    if (t == ME) vars["_source_identification"] = ME;
		    else {
			    // is there any known reason why this was missing?
			    vars["_location"] = source;
			    m_delete(vars, "_source_identification");
			    source = t;
			    // don't trust what the client says
			    if (objectp(t)) vars["_nick"] = t->qName();
			    // user.c does something very similar - so this
			    // shouldn't be necessary, yet if i remove that
			    // line a client can propose its own _nick and
			    // will be successful. hm!
		    }
#  else //{{{
		// or copy them into a var, then compare everywhere
		// this requires a rewrite of all of the v("locations") code
		    vars["_location"] = source;
		    m_delete(vars, "_source_identification");
		    source = t;
		    // don't trust what the client says
		    if (objectp(t)) vars["_nick"] = t->qName();
#  endif //}}}
		// do we really want to delete it also in the else case?
	    } else m_delete(vars, "_source_identification");
	}
    }
# endif // _flag_disable_module_authentication
#endif // !FORK

#ifndef _flag_disable_module_trust
    // this mechanism does not work for objectp(source) because uni::msg is not
    // called for objects. TODO
    // this stuff works alot like _request_auth.. i still think there might be
    // some code to share
    if (member(vars, "_trustee") && vars["_trustee"]) { 
	string snicker = objectp(source) ? vars["_nick"] : source;
	string trustee = vars["_trustee"];
	int trustiness = get_trust(snicker, trustee);

	P4(("trustee: %O -> %O\n", trustee, trustiness))
	if (trustiness != -1) {
	    vars["_trust"] = trustiness;
	} else { // do we want to check for get_trust(source) first.. maybe
		 // he is a friend..
	    string trustee_nick = trustee;
	    //
	    // TODO we dont even need to ask the trustee if we dont
	    // trust him anyway...  
	    if (is_formal(trustee)) {
		mixed *u = parse_uniform(trustee);
		
		unless (u) {
		    return 1;
		    // evil, there is no trustiness
		}

		// an ideal psyc parser would recognize local _trustee
		// and have it replaced by object.. but that's not the case
		// we _could_ however do that by using _uniform_trustee
		// and doing is_localhost on all _uniform's at parsing time
		if (is_localhost(lower_case(u[UHost]))) trustee_nick = u[UNick];
	    }
	    if (get_trust(trustee_nick, 0) < MINIMUM_TRUST) return 1;
	    // entweder trustee ist ein local nick, dann kriegen wir den
	    // get_trust, aber sendmsg failed hier unten.. oder trustee
	    // ist eine uni, dann kann er zwar senden, findet die person
	    // aber nicht im ppl[]

	    unless (qExists("t"+snicker)) { 
		qInit("t"+snicker, 30, 5);
		enqueue("t"+snicker, ({ source, mc, data, vars }));
		P4(("%O no trustiness for %O in %O. Queue: %O\n",
		    ME, snicker, _trust, qDebug()))
		// seeing this message is never useful, really
		// the question is answered automatically, so a user
		// would only get confused..
		sendmsg(trustee, "_request_trustiness", 
		 0 && "Do you trust [_identification] also known as [_nick]?",
			([ "_identification" : source, "_nick" : MYNICK ]));
		// we are lazy here.. but one trust request must be enough.. 
		// for now
	    } else
		enqueue("t"+snicker, ({ source, mc, data, vars }));
	    return 0;
	}
    }
    
    if (abbrev("_notice_trustiness", mc)) {
	int trustiness = to_int(vars["_trustiness"]);
	mixed l = vars["_identification"];
	string s = objectp(source) ? vars["_nick"] : source;

	if (objectp(l)) l = l->qName();
	P4(("%O: %O got a trustiness of %d from %O.\n", ME,
	    l, trustiness, source))
	unless (l) {
		P0(("%O got %O without _identification\n", ME, mc))
		return 0;
	}
	if (!member(_trust, l)) {
	    _trust[l] = ([ s : trustiness ]);
	} else {
	    _trust[l][s] = trustiness;
	}
	l = "t"+l;
	P4(("%O: Queue %O from %O.\n", ME, l, qDebug()))
	if (qExists(l)) {
	    while (qSize(l))
		apply(#'msg, shift(l));
	    qDel(l);
	}
	// dont drop the packet.. maybe we want to see this information
	// return 0;
    } else if (abbrev("_request_trustiness", mc)) {
	if (get_trust(objectp(source) ? vars["_nick"]
		       : source, 0) < MINIMUM_TRUST) {
	    sendmsg(source, "_failure_trustiness",
		    "I dont know anything about [_identification]!", ([
			 "_identification" : vars["_identification"]
	    ]));
	    return 0;
	}

	int trustiness = get_trust(objectp(vars["_identification"]) ?
		vars["_identification"]->qName() : vars["_identification"], 0);
	P4(("%O: %O asked me for trustiness (%d) of %O.\n", ME, source,
	    trustiness, vars["_identification"]))
	if (-1 != trustiness) {

	    sendmsg(source, "_notice_trustiness", "I trust [_identification] like [_trustiness] times bigger than others.", 
		    ([ "_identification" : vars["_identification"],
			         "_nick" : MYNICK,
		           "_trustiness" : trustiness ])); 
	}
    }
#endif // _flag_disable_module_trust
    return 1;
}

create() {
    // would be nice to skip this for blueprints, but at this point we have
    // no way to distinguish original places from blueprints - we have no
    // qName() yet - so all we could do would be to look at our object path.
    //unless (clonep() && qName()) return; // do not initialize blueprints
    // a different approach would be to rename this into eCreate() and have
    // it called in the right places.  TODO (cleanup)
#ifdef UNL_ROUTING //{{{
    ASSERT("entity::create() !uni2unl", !uni2unl, uni2unl)
    uni2unl = ([ ]);
#endif //}}}
#ifndef _flag_disable_module_trust
    _trust = ([ ]);
#endif
#ifdef ENTITY_STATE //{{{
    // wieso packen wir die counter nicht mit ins state-array. also 
    // mehrdimensional ?
    _state = ([ ]);
    _ostate = ([ ]);
    _cstate = ([ ]);
    _icount = ([ ]);
    _count = ([ ]);
    _memory = ([ ]);
    temp = ([]);
#endif //}}}
#ifdef NEW_QUEUE
    qCreate();
#endif
#ifdef TAGGING
    _tags = ([ ]);
#endif
}

#ifdef ENTITY_STATE //{{{

// reset the state.. which means: _count == 0
void Reset(mixed source, mixed isContext) {
    
    PT(("Reset(%O) in %O\n", source, ME));
    
    STATE(isContext)[source] = ([ ]);
}

void Assign(mixed source, string key, mixed value, mixed isContext) {
    mapping state = STATE(isContext);
    
    PT(("Assign(%O, %O, %O) in %O\n", source, key, value, ME));
    unless (member(state, source)) {
	state[source] = ([]);
    }
    if (value) {
	state[source][key] = value;
    } else m_delete(state[source], key);
}

void Augment(mixed source, string key, mixed value, mixed isContext) {
    mapping state = STATE(isContext);

    PT(("Augment(%O, %O, %O) in %O\n", source, key, value, ME));
    unless (member(state, source)) {
	state[source] = ([]);
    }
    _augment(state[source], key, value);
}

void Diminish(mixed source, string key, mixed value, mixed isContext) {
    int i;
    mapping state = STATE(isContext);

    PT(("Diminish(%O, %O, %O) in %O\n", source, key, value, ME));
    if (member(state, source)) {
	_diminish(state[source], key, value);
    }
}

int outstate(string target, string key, mixed value, int hascontext) {
    mapping timemachine = OMEMORY(target);
    mapping state = OSTATE(target);
    mapping t = temp[target];
    int mod;

    if (hascontext) {
	//PT(("%O sends messages in a context without using group/master.c.", ME))
	return 1;
    }
    
    if (key[0] == '_') {
	mod = ':';
    } else {
	mod = key[0];
	key = key[1..];
    }

    unless (t) t = temp[target] = ([ ]);

    unless (unused) unused = copy(state);
    m_delete(unused, key);

    switch(mod) {
    case ':':
	if (timemachine[key, 1] == -1) break;
	if (member(timemachine, key)) {
	    if (timemachine[key] == value) {
		if (timemachine[key, 1] == STATE_MAX2) { 
		    break;
		}
		if (timemachine[key, 1] == STATE_MIN2) {
		    t["="+key] = value;
		    state[key] = value;
		    return 0;
		}
		timemachine[key, 1]++;
	    } else if (timemachine[key, 1] - 1 == STATE_MIN2) {
		t["="+key] = "";
		m_add(timemachine, key, value, 1);
	    } else if (timemachine[key, 1] <= STATE_MIN2) {
		m_add(timemachine, key, value, 1);
	    } else timemachine[key, 1]--; 
	} else m_add(timemachine, key, value, 1);
	break;
    case '=':
	if ("" == value) {
	    m_delete(state, key);
	    m_delete(timemachine, key);
	    break;
	}
	if (member(state, key) && state[key] == value) {
	    return 0;
	}
	m_add(timemachine, key, value, -1);
	state[key] = value;
	break;
    case '+':
	_augment(state, key, value);
	break;
    case '-':
	_diminish(state, key, value);
	break;
    default:
	raise_error("Illegal variable modifier in '" + key + 
		    "' encountered in entity:outstate.\n");
	return 0;
    }
    return 1;
}

mapping state(string target, int hascontext) {
    mapping t, t2 = ([ ]);
    mapping timemachine = OMEMORY(target);

    if (hascontext) {
	PT(("%O sends messages in a context without using group/master.c.", ME))
	return ([]);
    }

    if (unused) {
	foreach (string key : unused) {
	    // variables assigned manually are taken to be permanent.
	    if (timemachine[key, 1] != -1)
		t2[":" + key] = "";
	}

	unused = 0;
    }

    t = temp[target] || ([ ]);
    m_delete(temp, target);

// rock hard optimization
    unless (sizeof(t))
	return t2;
    unless (sizeof(t2))
	return t;
    return t2 + t;
}
#endif // ENTITY_STATE }}}
