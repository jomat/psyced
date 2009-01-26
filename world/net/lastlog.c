// $Id: lastlog.c,v 1.29 2008/07/17 17:09:15 lynx Exp $ // vim:syntax=lpc
//
// generic implementation of a log of last messages,
// also known as lastlog in irc tradition
// stores log in a non-static variable ready for save_object
// used by user.c for /log and storic.c for /history in rooms
// memory allocation could be improved in a similar way to net/queue.c

// local debug messages - turn them on by using psyclpc -DDlastlog=<level>
#ifdef Dlastlog
# undef DEBUG
# define DEBUG Dlastlog
#endif

#include <net.h>

protected array(mixed) _log;

msgView(source, mc, data, vars, showingLog) {
	return msg(source, mc, data, vars, showingLog);
}

logAppend(source, mc, data, vars, maxlen, timevar, isAJob) {
	P4(("logAppend(%O, %O, %O, %O..) in %O\n", source, mc, data, vars, ME))
	// bug in emit(.., source) somewhere.. when displaying log the sources
	// are not advertised ahead. don't know where to fix it, so for now i
	// just turn off the lastlog feature in ircgate mode.
#ifndef ADVERTISE_PSYCERS
	//
	unless(timevar) timevar = "_time_INTERNAL";
	if (maxlen && sizeof(_log) > maxlen * 4)
	    _log = _log[ (maxlen/2) * 4 ..];

	// if (mappingp(vars)) vars = vars + ([ timevar : time() ]);
	if (mappingp(vars)) vars[timevar] = time();
	else vars = ([ timevar : time() ]);

# ifndef UNSAFE_LASTLOG
	// NEW: appending an array with 2 elements (source, psyc_name of it)
	// if source is an object, so we can gracefully deliver their room
	// history entries to remote psycers even if the object is long gone.
	_log += ({ (objectp(source) ? ({ source, psyc_name(source) }) : source),
	    mc, data, vars });
# else /* UNSAFE_LASTLOG */
	_log += ({ source, mc, data, vars });
# endif /* UNSAFE_LASTLOG */
#endif /* RELAY */
}

logClear(a) {
	int amount;

	if (stringp(a)) {
		// invitation for a grep-clear .. but hey who wants that
		unless (sscanf(a, "%d", amount)) amount = 0;
	} else amount = a;
	amount *= 4;
	if (amount <= 0 || amount > sizeof(_log)) _log = ({ });
	else _log = _log[0 .. sizeof(_log)-amount-1];
}

logInit(takeThis) {
	if (pointerp(takeThis)) _log = takeThis;
	else if (!_log || sizeof(_log)%4 ) _log = ({ });
	// D(S("\n%O :logInit: %O\n", ME, _log));
}

logClip(maxlen, cutlen) {
	int howmany;

	howmany = sizeof(_log);
	if (maxlen && howmany > maxlen * 4) {
		unless (cutlen) cutlen = maxlen;
		_log = _log[howmany-cutlen * 4 ..];
		return cutlen;
	}
	return howmany / 4;
}

// TODO: logView *since* timestamp. see also user.c:disconnected()
//
// uh. vim lpc syntax file doesn't like default in variablenames. renamed.
logView(a, showingLog, defAmount) {
	string grep;
	int i, ll;

	if (stringp(a)) {
		unless (sscanf(a, "%d", ll)) grep = a;
	} else ll = a;

// paranoid but.. you never know what happens to a .o file
#if DEBUG > 1
	if (sizeof(_log) % 4) {
		D("Log corrupted in "+to_string(ME)+"!\n");
		_log = ({ });
		return;
	}
#endif
	// <lynX> there were requests for smarter matching using regexps..
	//   i think we should enlarge the _request_do_log variable set
	//   by specifying which variables ought to be searched rather
	//   than trying an odd choice like below. the command variant
	//   in that case would probably be of the unix option syntax
	//   kind (we could use a getopt-implementation then). and most
	//   of all we need a timestamp-based replay function which
	//   finds the starting position in the log by a binary search
	if (grep) {
		string text, t;
		mapping m;

		ll = 0; for(i=0; i<sizeof(_log); i+=4) {
			if (mappingp(m = _log[i+3])) if (
			    ((text = _log[i+2]) && strstr(text, grep) >= 0)
			 || ((t = m["_nick"]) && strstr(t, grep) >= 0)
			 || ((t = m["_nick_target"]) && strstr(t, grep) >= 0)
			 || ((t = m["_action"]) && strstr(t, grep) >= 0)
			    ) {
#ifndef UNSAFE_LASTLOG
				msgView((pointerp(_log[i]) 
					 ? _log[i][0] || _log[i][1]
					 : _log[i]),
#else /* UNSAFE_LASTLOG */
# if UNSAFE_LASTLOG == "again"
				// in case we have to switch back ..
				msgView((pointerp(_log[i]) ? 0 : _log[i]),
# else
				msgView(_log[i],
# endif
#endif /* UNSAFE_LASTLOG */
				    _log[i+1],
				    text,
				    _log[i+3], showingLog);
				ll++;
			}
		}
		return ll;
	}
	P4(("\n%O :logView: %O\n", ME, _log))
	unless (ll) ll = defAmount || 15;
	ll *= 4;
	if (sizeof(_log) < ll) {
		ll = sizeof(_log);
		i = 0;
	} else {
		i = sizeof(_log) - ll;
	}
	while (i < sizeof(_log)) {
#ifndef UNSAFE_LASTLOG
		msgView((pointerp(_log[i])
			 ? _log[i++][0] || _log[i-1][1]
			 : _log[i++]),
#else /* UNSAFE_LASTLOG */
		msgView(_log[i++],
#endif /* UNSAFE_LASTLOG */
		    _log[i++],
		    _log[i++],
		    _log[i++], showingLog);
	}
	return ll / 4;
}

// pick a single message. used by POP3
logPick(i) {
	i *= 4;
	if (i < 0) {
		i = sizeof(_log) + i;
		if (i < 0) return 0;
	}
	if (i > sizeof(_log)) return 0;
#ifndef UNSAFE_LASTLOG
	return ({ (pointerp(_log[i])
		   ? _log[i++][0] || _log[i-1][1]
		   : _log[i++]),
		_log[i++], _log[i++], _log[i++] });
#else /* UNSAFE_LASTLOG */
	return ({ _log[i++], _log[i++], _log[i++], _log[i++] });
#endif /* UNSAFE_LASTLOG */
}

// used to make a temporary copy of the log, in POP3
public logQuery() { return _log; }

public logSize() { return sizeof(_log) / 4; }

