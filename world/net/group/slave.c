// vim:foldmethod=marker:syntax=lpc:noexpandtab
// $Id: slave.c,v 1.59 2008/04/22 22:43:56 lynx Exp $
//
// generic context slave as described in a posting to psyc-dev years ago.
// it receives the single copy of a message sent out by the group master
// and fans it out to local recipients. that's why local recipients need
// to create and join this manager when they enter a room.
//

// local debug messages - turn them on by using psyclpc -DDcontext=<level>
#ifdef Dcontext
# undef DEBUG
# define DEBUG Dcontext
#endif

#include <net.h>
#include <presence.h>

#ifdef PERSISTENT_SLAVES
# define PARANOID_PERSISTENT_SLAVES
# include <uniform.h>
inherit NET_PATH "storage";

private volatile string _file;

private string *_save_members;
private string _name;
private int _revision = -1;
#endif

#ifdef CONTEXT_STATE
private volatile mapping cast_state;
private volatile mapping temp_state;
inherit NET_PATH "state";
#endif

private volatile mapping _members;

void create() {
	unless(mappingp(_members)) _members = ([ ]);
#ifdef CONTEXT_STATE
	unless(mappingp(cast_state)) cast_state = ([ ]);
	unless(mappingp(temp_state)) temp_state = ([ ]);
#endif
}

#ifdef PERSISTENT_SLAVES
varargs void load(string context, array(mixed) u) {
	string dir = DATA_PATH "slaves/";
	string entity;

	unless (u) u = parse_uniform(context);
	dir += u[UHost];
	if (u[UPort]) dir += "-"+ u[UPort];
	mkdir(dir); // make sure that directory exists
	_name = context;
	entity = u[UUser] || u[UResource];
	_file = dir +"/"+ (entity? sha1(entity): "_");
	::load(_file);
	foreach (string m : _save_members) {
	    object o = summon_person(m[1..]);
	    _members[o] = m;
	}
	P4(("loaded cslave %O with %O\n", _name, _members))
}

protected save() {
    P4(("cslave:save() %O\n", _members))
    unless (_file) return;
    if (sizeof(_members) == 0) {
	rm(_file);
	_revision = -1;
	// FIXME: could clean up directory if it is empty
    } else {
	_save_members = m_values(_members);
	::save(_file);
    }
}

# ifndef PARANOID_PERSISTENT_SLAVES
remove() {
	if (_file) save(_file);
}

reset() {
	if (_file) save(_file);
}
# endif
#endif

void insert_member(mixed member, mixed origin, mixed data) {
	P4(("%O enters cslave\n", member))
	// we use the values from the members mapping as counters
	// for their individual psyc-state
#ifdef PERSISTENT_SLAVES
	_members[member] = member->psycName();
#else
	_members[member] = 0;
#endif
#ifdef CONTEXT_STATE
	if (stringp(member)) {
	    mapping v = ([ ]);

// this is not how we do it
//	    foreach(string key, mixed value : cast_state)
//		v["="+key] = value;
// no var ops embedded in sendmsg.

	    sendmsg(member, 0, 0, v + ([ "_context" : ME , "_target" : member ]));
	}
#endif
#ifdef PARANOID_PERSISTENT_SLAVES
	// paranoid slaves
	save();
#endif
}

void remove_member(mixed member, mixed origin) {
	PT(("%O leaves context slave\n", member))
	m_delete(_members, member);

#ifdef PARANOID_PERSISTENT_SLAVES
	save();
#endif
}

castmsg(source, method, data, mapping vars) {
	/*
	 * implement group manager logic 
	 * and fan out everything else?
	 * fan out could happen here and group manager handled by 
	 * higher level objects
	 */
	mixed o;
#ifdef XMPPERIMENTAL
	PT(("%O group slave _routes is %O\n", ME, _members))
#else
	P3(("%O group slave msg(%O, %O, ...)\n", ME, source, method))
#endif
#ifdef CACHE_PRESENCE
	// before lynX starts complaining: 
	// we could cache the presenity value here
	if (vars["_degree_availability"])
	    persistent_presence(source, vars["_degree_availability"]);
				    //, vars["_degree_mood"]);
# if 0 // else? why should we have presence without _degree_availability ?
	else switch(method) {
	case "_notice_presence_here":
	    persistent_presence(source, AVAILABILITY_HERE);
	    break;
	case "_notice_presence_here_busy":
	    persistent_presence(source, AVAILABILITY_BUSY);
	    break;
	case "_notice_presence_away":
	case "_notice_presence_away_manual":
	case "_notice_presence_away_automatic":
	    persistent_presence(source, AVAILABILITY_AWAY);
	    break;
	case "_notice_presence_absent_vacation":
	    persistent_presence(source, AVAILABILITY_VACATION);
	    break;
	case "_notice_presence_absent":
	    persistent_presence(source, AVAILABILITY_OFFLINE);
	    break;
	}
# endif
#endif

#ifdef PERSISTENT_SLAVES
	/* the lazy update strategy */
	if (vars["_number_revision"] && !intp(vars["_number_revision"]))
	    vars["_number_revision"] = to_int(vars["_number_revision"]);
	if (vars["_number_revision"] && _revision != vars["_number_revision"]) {
	    int delta;
	    /* initialize */
	    if (_revision == -1) {
		_revision = vars["_number_revision"];
	    } else {
		delta = vars["_number_revision"] - _revision;
		/* counter increment */
		if (delta == 1) {
		    _revision = vars["_number_revision"];
		} else if (delta > 1 || delta < 1) {
		    P0(("warning: %O revision mismatch! have %O, master has %O, delta is %d\n",
			_name, _revision, vars["_number_revision"], delta))
		    monitor_report("_failure_slave_revision",
			    object_name(ME) +" · "+ sprintf("revision mismatch with delta %d", delta));

		    // TODO
		    // for now, we just log and accept those
		    _revision = vars["_number_revision"];
		}
	    }
	}
	if (vars["_number_revision"] > 0) {
	    PT(("%O (%s) counter revision is %O\n", ME, method, vars["_number_revision"]))
	}
	m_delete(vars, "_number_revision");
#endif
	foreach(o : _members) {
		if (objectp(o))
#ifdef CONTEXT_STATE
			    // may need copy(vars) also
		    o -> msg(source, method, data, cast_state + vars );
		else
		    sendmsg(o, method, data, vars + temp_state, source);	
#else
		    o -> msg(source, method, data, copy(vars));
		else
		    sendmsg(o, method, data, vars, source);	
#endif
		/* if one or more of our local users have joined this
		 * place, we trust the place and allow it to use us as
		 * a relay for people who are topologically close to us.
		 * that's how remote users may end up in this structure, too.
		 */
	}
#ifdef CONTEXT_STATE
	temp_state = ([ ]);	
#endif
	return 1;
}

#ifdef CONTEXT_STATE // {{{
// This won't work ... we have to have _one_ single cast-state
// which is synced with objects entering the context.. 
//
// TODO

void Reset(mixed source) {
    cast_state = ([ ]);
    foreach(mixed o : _members) {
	unless (objectp(o))
	    sendmsg(o, 0, 0, ([ "_count" : _members[o] = 0 ]), source);
    }
}

void Assign(mixed source, string key, mixed value) {

    // we have to check here for source == our context
    // + we expect msg to be called between state-changes from different
    // packets to reset temp_state
    
    if (stringp(value) && value == "" )
	m_delete(cast_state, key);
    else 
	cast_state[key] = value;

    temp_state["="+key] = value;
}

void Augment(mixed source, string key, mixed value) {
    _augment(cast_state, key, value);
    _augment(temp_state, "+"+key, value);
}

void Diminish(mixed source, string key, mixed value) {
    int i;

    _diminish(cast_state, key, value);
    if (member(temp_state, "-"+key)) {
	PT(("PANIC! Received list-diminish as a list!"))
    } else temp_state["-"+key] = value;
}

#endif // }}}
