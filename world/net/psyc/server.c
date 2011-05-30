// $Id: server.c,v 1.102 2008/08/05 12:15:11 lynx Exp $ // vim:syntax=lpc
//
// the thing that answers on port 4404 of psyced.

#include "common.h"
#include <net.h>
#include <services.h>

#define NO_INHERIT
#include <server.h>

#ifdef __PIKE__
inherit net.psyc.circuit;
#else
// receiving variant
inherit PSYC_PATH "circuit";
#endif

// keep a list of objects to ->disconnected() when the driver tells us
volatile array(object) disconnect_notifies;

void register_link(object user) {
	P4(("disconnect_notifies for %O in %O\n", user, ME))
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

#ifndef __PIKE__

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
	// emulate disconnected() for net/psyc/user
	if (disconnect_notifies) foreach (object t : disconnect_notifies) {
		P3(( "%O disconnecting %O\n", ME, t)) 
		if (t) t->link_disconnected();
	}
	::disconnected(remaining);
	QUIT // returns unexpected.. TODO
}
// actually bad considering that a packet may be in dns resolution
// and wants to callback deliver in here  TODO
// saga: jedenfalls müssen wir entweder sofort destructen oder aber alle targets abmelden, die wir je angemeldet haben
// lynX: oder die unterscheidung zwischen active und server abschaffen

static varargs int block(vastring mc, vastring reason) {
	P0(("Server blocked TCP PSYC connection from %O in %O (%O).\n",
	    query_ip_number(ME), ME, mc))
#ifdef SYMLYNX
# define TEST_REDIR ""
# define TEXT_REDIR ""
#else
# define TEST_REDIR ":_source_redirect\tpsyc://ve.symlynX.com\n"
# define TEXT_REDIR "Try the test server at [_source_redirect] instead!\n"
#endif
	unless(mc) mc = "_error_illegal_source";
	unless(reason) reason = "";
	emit(".\n\
\n\
" TEST_REDIR + mc +"\n\
Sorry, my configuration does not allow me to talk to you.\n\
"+ reason +"\n\
" TEXT_REDIR "\
.\n"
	);
	QUIT
}

#endif // PIKE

static void resolved(mixed host) {
	string netloc, numericpeeraddr;
	mixed uni, psycip;

	unless (stringp(host)) {
#if 1 //ndef SYMLYNX
		if (host == -2) {
			monitor_report("_warning_invalid_hostname",
				S("%O: %O has an invalid IN PTR", ME,
				  query_ip_number(ME)));
# ifndef STRICT_DNS_POLICY
#  ifndef __PIKE__
			// we could instead lower the trust value for this
			// host so that it can no longer send messages in
			// a federated way, but still link into an identity
			// as a client....... TODO.. like this?
			if (trustworthy < 6) trustworthy = 1;
#  endif
			host = peerip;
# else
			block("_error_invalid_hostname",
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
			block("_failure_invalid_hostname",
			  "Resolving your IP address triggered an internal error.");
			return;
		}
#else
		// we sent them to the test server, so let it be easy on them
		host = peerip;
#endif
	}
	// maybe dns_rresolve should only return lower_case'd strings
	// then we no longer have to do that everywhere else	TODO
	host = lower_case(host);
#ifdef EXTRA_RRESOLVE
	EXTRA_RRESOLVE(host)
#endif
#ifndef __PIKE__
	if (trustworthy < 6) {
	    if (trustworthy = legal_domain(host, peerport, "psyc", 0)) {
		if (trustworthy < 3) trustworthy = 0;
	    }
	    else {
	       	block();
		return;
	    }
	}
	// the resolver does not register automatically, so here we go
	register_host(peerip, host);
	register_host(host);
#endif // PIKE
//	register_target( "psyc://"+peeraddr );
	numericpeeraddr = peeraddr;
	peeraddr = peerhost = host;
	// peerport has either positive or negative value
	if (peerport && peerport != PSYC_SERVICE) peeraddr += ":"+peerport;
	netloc = "psyc://"+peeraddr+"/";
	register_target( netloc );
#ifndef _flag_disable_module_authentication // OPTIONAL
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
#endif // _flag_disable_module_authentication

	// PIKE TPD: says psyc://127.0.0.1/ here .. should say
	//		  psyc://localhost:-23232/ instead
	P2(("%O resolves as %O (UNI %O)\n", ME, netloc, uni))
	// in the old days: greet();
}

int logon(int nothing) {
	P2(("%O accepted TCP from %O (%s:%O)\n", ME,
	    query_ip_name(), query_ip_number(), peerport))
        if(query_mud_port() == PSYCS_PORT && !tls_query_connection_info(this_object()))
        {
            emit("This is TLS, you don't use TLS\n");
            QUIT
            return 0;
        }

	// we could set the next_input_to and reply with _failure until
	// hostname is resolved  .. TODO  ... no, we need some form
	// of queuing for the scripts which do not wait.. why? don't we
	// squeeze received packets thru dns-lambdas anyway?
	// peerport has either positive or negative value
	//peeraddr = peerip+":"+peerport;
	::logon(0);
#if 0 //def EXPERIMENTAL
	// added this because greet() happens after dns resolution and
	// some quick clients may not be waiting that long.. then again
	// if they do, they deserve other treatment
	sTextPath();
#endif
	dns_rresolve(peerip, #'resolved);
	return 1;   // success
}

