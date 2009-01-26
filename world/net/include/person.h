// $Id: person.h,v 1.29 2008/07/26 10:54:30 lynx Exp $ // vim:syntax=lpc:ts=8
//
#ifndef _INCLUDE_PERSON_H
#define _INCLUDE_PERSON_H

#ifdef MUD
# define find_person(N)		find_living("psyc:"+lower_case(N))
# define register_person(N) set_living_name("psyc:"+lower_case(N))
	// ; enable_commands()
#endif

#include "presence.h"

// should the following 2 defines be in user.h?
#define FRIEND_NICK		0
#define FRIEND_AVAILABILITY	1

#if !defined(VOLATILE) && !defined(RELAY) && !defined(_flag_disable_module_nickspace)
# define ALIASES
#endif

#ifdef DATA_FOR_THE_MASSES
# define PERSON_DATA_FILE(name)  (DATA_PATH "person/"+ name[0..0] +"/"+ name)
#else
# define PERSON_DATA_FILE(name)  (DATA_PATH "person/"+ (name))
#endif

// shouldn't alias be using psyc_name() instead of glueing urls?
#ifdef ALIASES
# define DEALIAS(to, from)	{ string t;\
    to = raliases[t = lower_case(from)]\
	 || (aliases[t] ? query_server_unl() +"~"+ from : from);\
}
#endif

#ifdef RELAY
# define ONLINE	(availability != 0)
#else
# define ONLINE	(ME && (interactive(ME) || v("locations")[0]))
#endif

#ifdef NO_NEWBIES
# define IS_NEWBIE	0
#else
# define IS_NEWBIE	(!v("password"))
#endif

#define NO_SUCH_USER	(!ONLINE && IS_NEWBIE)
// this sounds very logical, but i doubt it is what we want or need
//#define ONLINE	(availability > AVAILABILITY_OFFLINE && ME && (interactive(ME) || v("locations")[0]))
// the way user.c uses ONLINE for example doesn't look like this is good
// such a change requires thoughtwork and testing

    // privacy concerns:
   // we should not return the exact number of seconds etc.
#define CALC_IDLE_TIME(t) \
	t = time() - v("aliveTime");\
	t = t < 30 ? 0 : t < 300 ? 300 : (t + random(200) - 100);


// used by myLogAppend below:
#ifndef _limit_amount_log
# define _limit_amount_log 777
#endif
#ifndef _limit_amount_log_persistent
# define _limit_amount_log_persistent 100
#endif

// should "new" become part of the lastlog.c mechanism? how?
// should we simply use timestamp of last logout? that works
// better with places for history-while-i-was-away too.  TODO
// (implement binary search for finding the oldest unread entry..)
// so we could throw this away as soon as we have timestamp-based lastlog.
#define	myLogAppend(source, mc, data, vars) {\
	logAppend(source, mc, data, vars, _limit_amount_log); \
	unless (ONLINE) vInc("new"); \
}


#endif
