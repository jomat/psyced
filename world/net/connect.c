// $Id: connect.c,v 1.54 2008/12/10 22:53:33 lynx Exp $ // vim:syntax=lpc
//
// net/connect: generic handler for active connections
// most methods are intended for overloading except for the connect2()
//

// local debug messages - turn them on by using psyclpc -DDconnect=<level>
#ifdef Dconnect
# undef DEBUG
# define DEBUG Dconnect
#endif

#include <net.h>
#include <errno.h>

virtual inherit NET_PATH "trust";

volatile mixed is_connecting;

connect(host, port, transport);

protected int block() { destruct(ME); return 0; }

protected connect_failure(mc, text) {
	is_connecting = 0;
	P1(("connect failure %O in %O, %O.\n", mc, ME, text))
	monitor_report("_failure_network_connect"+ mc,
	    object_name(ME) +" · "+ text);
}

// psyc circuits call this "manually" via psyc/active
protected int logon(int failure) {
	if (is_connecting == "s") {
		is_connecting = 0;
#if __EFUN_DEFINED__(tls_init_connection)
		P2(("%O connected to %O from %O. TLS requested.\n", ME,
		     query_ip_number(ME), query_mud_port(ME)))
		tls_init_connection(ME, #'logon);
#else
		connect_failure("_unsafe", "security not available");
		return 0;
#endif
	}
	is_connecting = 0;
	if (failure == -1 || !interactive(ME)) {
		P3(("Warning: Failed connect attempt in %O\n", ME))
#if __EFUN_DEFINED__(errno)
		connect_failure("_attempt", sprintf("connect failed: errno %s",
						    errno()));
#else
		connect_failure("_attempt", "connect failed");
#endif
		return 0;
	}
	P2(("%O connected to %O from %O\n", ME,
	     query_ip_number(ME), query_mud_port(ME)))

	// this used to catch connections being set up while a shutdown
	// has been initiated in the meantime, but it was doing the wrong
	// things to get there, so that's changed. we should put the connection
	// on hold in that case. we need to implement daemon restart behaviour
	// in psyclpc and as we do that we might aswell provide an efun to
	// query shutdown progress  TODO
#if __EFUN_DEFINED__(query_shutdown_progress)
	// go on hold maybe?
	if (query_shutdown_progress()) return 0;
#endif
	unless (hostCheck(query_ip_number(ME), query_mud_port(ME)))
	    return block();
#if __EFUN_DEFINED__(enable_telnet)
	enable_telnet(0, ME);
#endif
	return 1;
}

// seems to me all of this could as well happen in library/dns?
// any use for it?
protected canonical_host(cane, ip, host) {
	string lh;

	register_host(ip, cane);
	register_host(host, cane);
	lh = lower_case(host);		// the host we are given is probably
					// lc'd already..
	if (lh != host) register_host(lh, cane);
	unless (intp(cane)) {
		register_host(cane, cane);
		lh = lower_case(cane);
		if (lh != cane) register_host(lh, cane);
	}
}

private connect2(ip, port, host) {
	int rc;

	P3(("%O connect2(%O, %O, %O) == %O\n", ME, ip, port, host, chost(ip)))
	unless (stringp(ip)) {
		connect_failure("_resolve", host+" does not resolve");
		return;
	}
	// why are we checking the port _after_ dns resolution?
	if (port <= 0) {
		// similar message in psyc/library.i
		connect_failure("_invalid_port_connect",
			       	"no connectable port given");
		return;
	}
	if (interactive()) return -8;
	unless (hostCheck(ip, port)) {
	    P2(("%O stopped connect attempt to %O on %O\n", ME, ip, port))
	    connect_failure("_block", "Blocked by your server policy.");
	    destruct(ME);
	    return;
	}
#if __EFUN_DEFINED__(net_connect)
	P3(("REALLY calling net_connect(%O, %O)\n", ip, port))
	rc = net_connect(ip, port);
	switch(rc) {
	case 0:
		P3(("%O connecting(%O, %O, %O) == %O\n",
		     ME, ip, port, host, chost(ip)))
		break;
	case EMFILE:
		P1(("EMFILE. resubmitting connect to %s:%O\n", host, port))
#if 0 // better solution, but can lead to race conditions. ok, kind of race
      // conditions, so all in all it's worse. maybe.
		// circuit.c is supposed to handle reconnect attempts
		// but this internal failure is probably better solved
		// by immediate resubmission of the connect request
		// (2 seconds = one cycle) rather than treating it like
		// a connect_failure.
			// why aren't we calling connect2() here?
		call_out((: is_connecting = 0;
			  connect($1, $2); return; :), 2, host, port);
		// wenn outgoing connections in ldmud's comm.c is raised or
		// even dynamic, EMFILE should simply no longer happen.
#else
		// this should work too, except for one-time tcp connections
		// like net/http/fetch. the advantage of this approach is
		// that a real EMFILE, when incoming descriptors are exhausted,
		// does not cause an obnoxious call_out loop.
		connect_failure("_attempt_exceeded",
	    "Too many pending connections right now. Trying again later.");
#endif
		break;
	default:
		connect_failure("_call", "connect to "+ip+" returns "+ rc);
	}
#else
	P0(("%O connect to %O, %O, %O stopped:\n\t\
Driver does not provide net_connect()\n", ME, ip, port, host))
#endif
	// ldmud says we sometimes get here even if we got destructed *shrug*
	if (ME && !chost(ip)) dns_rresolve(ip, #'canonical_host, ip, host);
}

connect(host, port, transport) {
	P4(("%O connect:connect(%O, %O, %O)\n", ME, host,port,transport))
	if (interactive() || !host || !port || is_connecting) return -8;
	is_connecting = transport || 1;
	P3(("%O connect:connect(%O, %O, %O). resolving host.\n", ME,
	    host,port,transport))
	// even on reconnect we don't cache the dns host data as it
	// may be a dynamic dns host currently rebooting..
	dns_resolve(host, #'connect2, port, host);
	return 0;
}

disconnected(remaining) {
	P2(("%O got disconnected(%O). it was %s connected.\n", ME, remaining,
	    query_once_interactive(ME) ? "once" : "never"))
	connect_failure("_disconnect", "lost connection");
	return 0;   // unexpected
}

