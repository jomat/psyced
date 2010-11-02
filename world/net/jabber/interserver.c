// $Id: interserver.c,v 1.13 2008/10/01 10:59:24 lynx Exp $ vim:syntax=lpc
//
// common things for interserver jabber.. included or maybe later inherited by 
// active.c and gateway.c. i am sure fippo will find some more nice things to
// extrapolate into here..  :)

volatile int flags = 0;

#ifdef XMPP_BIDI
volatile int bidi; // is this stream bidirectional?
#endif

reboot(reason, restart, pass) {
        if (pass == 0) {
	    if (interactive(ME)) {
		flags |= TCP_PENDING_DISCONNECT;
		STREAM_ERROR("system-shutdown", (restart ? "Server restart: "
			: "Server shutdown: ")+ reason);
		// shutdown order pretty much ensures no other data will be
		// sent over this socket after this message. but it isn't
		// really enforced.. TODO?
	    }
	} else
	    destruct(ME);
}

int clean_up(int refcount) {
    if (interactive(ME)) {
	// closing the socket without asking the other side will raise
	// the chances of losing packets. we're not playing that game,
	// and i'm afraid every jabber server in the world needs to be
	// fixed if jabber is one day trying to become a reliable thing.  -lynX
#if 0
	// that's the 0190 incompliant way to do it
	STREAM_ERROR("connection-timeout", "just reconnect if you like")
	remove_interactive(ME);
#else
	// this instead will initiate a clean shutdown of the connection
	// and is therefore the correct way to timeout a connection.
	PT(("%O cleaning up: closing stream\n", ME))
	// close the stream according to XEP 0190
	emitraw("</stream:stream>");
	// flag says the stream is in closing phase and nothing may be
	// delivered on it. but we aren't enforcing this. TODO!
	flags |= TCP_PENDING_DISCONNECT;
#endif
    } else if (clonep(ME)) {
	PT(("%O cleaning up: self destruct\n", ME))
	destruct(ME);
    }
    return 0;
}
