// $Id: user.c,v 1.15 2008/12/09 19:27:32 lynx Exp $ // vim:syntax=lpc
//
// handler for PSYC clients

#include "common.h"
#include <net.h>
#include <user.h>

qHasCurrentPlace() { return 0; }                

logon() {
#ifdef NO_EXTERNAL_LOGINS
	return destruct(ME);
#endif
	// psyc users dont have their own socket, so the driver 
	// does not call disconnected() for them - this enables the
	// psyc socket to do that
	if (this_interactive()) this_interactive()->do_notify_on_disconnect(ME);
	// connection that is creating us, died while we got here.
	// rare, but does indeed happen sometimes.
	else return destruct(ME);

	// no lang support here either
	vSet("scheme", "psyc");
	return ::logon();
}

// errors only, it says
pr(mc, fmt, a,b,c,d,e,f,g,h) {
#if 1 //def PRO_PATH
	if (abbrev("_message",mc)) return;
	if (v("location"))
	    sendmsg(v("location"), mc+"_print", sprintf(fmt, a,b,c,d,e,f,g,h) );
#else
	// checkVar() still calls pr() .... grmlblmblm TODO
	raise_error("pr() called\n");
#endif
}

wAction(mc, data, vars, source, variant, nick) { return 0; }
