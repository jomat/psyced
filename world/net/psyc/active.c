// vim:foldmethod=marker:syntax=lpc:noexpandtab
// $Id: active.c,v 1.41 2008/06/17 09:35:45 lynx Exp $
//
#include "common.h"
#include <net.h>
#include <services.h>

#ifdef SPYC
inherit SPYC_PATH "circuit";
#else
inherit PSYC_PATH "circuit";
#endif

inherit NET_PATH "circuit";

volatile object super;

void takeover() {
    super = previous_object();
    if (super == ME) /* TODO: prevent elsewhere */ {
	super = 0;
    }
    unless (interactive(ME)) runQ();
}

// we love multiple inheritance.. rock'n'roll!
int logon(int failure) {
	int ret;

	P2(("%O logon kriegt %O, prev: %O\n", ME, failure, previous_object()))
	ret = NET_PATH "circuit"::logon(failure);
	if (failure >= 0 && ret > 0) {
#if 0 // apparently wrong
		emit(".\n"); // should we do the greeting?
		PSYC_PATH "circuit"::logon(failure);

		peeraddr = peerhost = host;
		if (port && port != PSYC_SERVICE) peeraddr += ":"+port;
#else // probably better
		peeraddr = peerhost = host;
		peerport = port;
		if (port && port != PSYC_SERVICE) peeraddr += ":"+port;
		// circuit::logon now also implies a full greeting
		// therefore it needs peeraddr, and the emit is redundant
# ifdef SPYC
		SPYC_PATH "circuit"::logon(failure);
# else
		PSYC_PATH "circuit"::logon(failure);
# endif
#endif
		return 1;
	}
	return 0;
}

int msg(string source, string method, string data,
	mapping vars, int showingLog, string target) {
	P3(("active.c:msg(%O, %O, %O) in %O%s\n", source, method, data, ME,
	    (interactive()) ? "(connected)" : "(not connected)"))

	unless (interactive())
#ifdef FORK // {{{
	{
	    if (!member(vars, "_source"))
		vars["_source"] = UNIFORM(source);
	    unless (super)
		return enqueue(source, method, data, vars, showingLog, target);
	    return super->msg(source, method, data, vars, showingLog, target);
	}
	return ::msg(source, method, data, vars, showingLog, target);
#else // }}}
	{
	    P2(("%O is not interactive (no network connection)\n", ME))
	    if (!member(vars, "_source"))
		vars["_source"] = UNIFORM(source);
// this stuff is causing loops and i don't know how to fix it right now
//	    unless (super)
# ifdef SPYC
    // NOTE: SPYC uses a per-host verification and therefore 
    // 		may not send certain packets before verification
    // 		is done (those packets usually have source and target)
    // 		for now, this check works most of the time
# endif
		return enqueue(source, method, data, vars, showingLog, target);
//	    return super->msg(source, method, data, vars, showingLog, target);
	}
	return ::msg(source, method, data, vars, showingLog, target);
#endif // !FORK
}

#if 0
connect(host, port) {
	if (host) {
		// funky but who needs this?
		if (interactive()) {
		    if (ahost == host && aport == port) return -8;
		    else remove_interactive(ME);
		}
	...

	P1(("PSYC/TCP %O * net_connect(%O, %O) = %O\n",
		 ME, host, port, rc))
	return rc;
}
#endif
