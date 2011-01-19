// $Id: user.c,v 1.15 2008/12/09 19:27:32 lynx Exp $ // vim:syntax=lpc
//
// should be a dummy user object since all user objects
// must be able to handle PSYC clients

#include "common.h"
#include <net.h>
#include <user.h>

qHasCurrentPlace() { return 0; }                

logon() {
#ifdef NO_EXTERNAL_LOGINS
	return destruct(ME);
#endif
#if 0
	// psyc users dont have their own socket, so the driver 
	// does not call disconnected() for them - this enables the
	// psyc socket to do that
// basically a good idea, but the wrong place to do this. since we
// want to be notified about any of n possible psyc clients we need
// to do this in linkSet().  --lynX
	if (this_interactive()) this_interactive()->register_link(ME);
	// connection that is creating us, died while we got here.
	// rare, but does indeed happen sometimes.
	else return destruct(ME);
// i presume the else case is better handled by disconnected() --lynX
#endif
	// no lang support here either
	vSet("scheme", "psyc");
	return ::logon();
}

// errors only, it says
pr(mc, fmt, a,b,c,d,e,f,g,h) {
#if 1 //ndef DEVELOPMENT
	//if (abbrev("_message",mc)) return;
	foreach (string location : v("locations")[0])
	    sendmsg(location, mc+"_print", sprintf(fmt, a,b,c,d,e,f,g,h) );
#else
	// checkVar() still calls pr() .... grmlblmblm TODO
	raise_error("pr() called\n");
#endif
}

wAction(mc, data, vars, source, variant, nick) { return 0; }
