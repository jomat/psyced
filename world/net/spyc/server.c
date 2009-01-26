// vim:foldmethod=marker:syntax=lpc:noexpandtab
// $Id: server.c,v 1.14 2008/03/11 13:42:27 lynx Exp $
//
// the thing that answers on port 4404 of psyced.

#include "psyc.h"
#include <net.h>
#include <services.h>
#define NO_INHERIT
#include <server.h>

// receiving variant
inherit NET_PATH "spyc/circuit";

// keep a list of objects to ->disconnected() when the driver tells us
volatile array(object) disconnect_notifies;

void do_notify_on_disconnect(object user) {
        unless(disconnect_notifies)
	   disconnect_notifies = ({ });
        disconnect_notifies += ({ user });
}

// only used by list_sockets()
string qName() {
	switch (sizeof(disconnect_notifies)) {
	case 0:
		return 0;
	case 1:
		return to_string( disconnect_notifies[0] );
//	default:
	}
	return to_string( sizeof(disconnect_notifies) );
}

//isServer() { return peerport < 0; }
int isServer() { return 1; }

load(ho, po) {
	D0 ( if (peerport)
		 PP(("%O loaded twice for %O and %O\n", ME, peerport, po)); )
	peerport = po;
//	P3(("loaded server on %O\n", peerport))
	return ME;
}

protected quit() { QUIT }

// self-destruct when the TCP link gets lost
disconnected(remaining) { 
	int rc;

	P2(( "%O got disconnected.\n", ME)) 
	// emulate disconnect() for net/psyc/user
	if (disconnect_notifies) {
	   foreach (object t : disconnect_notifies) 
		if (t) t->disconnected();
	}
	rc = ::disconnected(remaining);
	destruct(ME);
	return rc;
}

static void resolved(mixed host, mixed tag) {
	PT(("resolved %O to %O\n", peerip, host))
	string numericpeeraddr;
	mixed uni, psycip;

	unless (stringp(host)) {
#if 1 //ndef BETA
		if (host == -2) {
			monitor_report("_warning_invalid_hostname",
				S("%O: %O has an invalid IN PTR", ME,
				  query_ip_number(ME)));
# ifndef STRICT_DNS_POLICY
			// we could instead lower the trust value for this
			// host so that it can no longer send messages in
			// a federated way, but still link into an identity
			// as a client....... TODO.. like this?
			if (trustworthy < 6) trustworthy = 1;
			host = peerip;
# else
			croak("_error_invalid_hostname",
  "Your IP address points to a hostname which doesn't point back to the IP.\n"
  "You could be trying to spoof you're in somebody else's domain.\n"
  "We can't let you do that, sorry.");
			return;
# endif
		}
		else if (host == -1) host = peerip;
		else {
			P0(("resolved(%O) in %O. but that's impossible.\n",
			    host, ME))
			croak("_failure_invalid_hostname",
			  "Resolving your IP address triggered an internal error.");
			return;
		}
#else
		// we sent them to beta, so let beta be easy on them
		host = peerip;
#endif
	}
	// maybe dns_rresolve should only return lower_case'd strings
	// then we no longer have to do that everywhere else	TODO
	host = lower_case(host);
#ifdef EXTRA_RRESOLVE
	EXTRA_RRESOLVE(host)
#endif
	if (trustworthy < 6) {
	    if (trustworthy = legal_domain(host, peerport, "psyc", 0)) {
		if (trustworthy < 3) trustworthy = 0;
	    }
	    else {
	       	croak("_error", "Sei nicht so ein Mauerbluemchen");
		return;
	    }
	}
	// the resolver does not register automatically, so here we go
	register_host(peerip, host);
	register_host(host);
//	register_target( "psyc://"+peeraddr );
	numericpeeraddr = peeraddr;
	peeraddr = peerhost = host;
	// peerport has either positive or negative value
	if (peerport && peerport != PSYC_SERVICE) peeraddr += ":"+peerport;
	netloc = "psyc://"+peeraddr+"/";
	register_target( netloc );
#if 1 // OPTIONAL
	// should this server be connected to a psyc client, then the new
	// resolved name of the connection may have to be recognized as
	// location for the person. finally this code should be unnecessary
	// as you should never want to _link a person before seeing the
	// _notice_circuit_established. maybe we should even enforce that.
	// anyway, here's an attempt to cope with such a situation.. maybe
	// it turns out useful someday (lynX after a quick patch by elrid).
	//
	psycip = "psyc://"+numericpeeraddr+"/";
	if (uni = lookup_identification(psycip)) {
		register_location(netloc, uni);
		// cleanup? are you sure we will never need this again?
		register_location(psycip, 0); //cleanup
	}
	    
#endif
	// PIKE TPD: says psyc://127.0.0.1/ here .. should say
	//		  psyc://localhost:-23232/ instead
	P2(("%O resolves as %O (UNI %O)\n", ME, netloc, uni))
	if (flags & TCP_PENDING_TIMEOUT) {
	    P0(("removing call out\n"))
	    remove_call_out(#'quit);
	    flags -= TCP_PENDING_TIMEOUT;
	}
	resume_parse();
	sTextPath();

	// FIXME: determine response to greeting
	// 	instead of this dummy
	msg(0, "_notice_features", 0, tag ? ([ "_tag_reply" : tag ]) : 0);
}


void circuit_msg(string mc, mapping vars, string data) {
    switch(mc) {
    case "_request_features": // only servers handle _request_features
	interrupt_parse();
	dns_rresolve(peerip, #'resolved, vars && vars["_tag"]);
	break;
    default:
	return ::circuit_msg(mc, vars, data);
    }
}
