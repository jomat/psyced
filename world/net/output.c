// $Id: output.c,v 1.20 2008/08/05 12:21:33 lynx Exp $ // vim:syntax=lpc
//
// binary output functions, used by user.c and psyc/server.c

#include <net.h>

// protected..? now used by psyc/library.
int emit(string message) {
	 unless (strlen(message)) {
		PT(("%O got empty emit call\n", ME))
#if DEBUG > 1
		tell_object(ME, "**EMPTY**");
		raise_error("empty");
#endif
		return 0;
	}

#if 0 //def EMIT_CAN_WRITE
// ... no.. emit can't write(). it fails when a user disconnects
// and is still in the room, then the output goes
// to the sender's socket. terrible.
//
	// also: gets in conflict when both ME and this_interactive()
	// are given and not the same, like in user->htget().
	// in that case you _have_ to use write() or
	// this_interactive()->emit()
	if (!interactive(ME)) {
		// when a place calls emit() it probably intends to
		// output to this_interactive() .. but we don't wanna guess!
		if (this_interactive()) {
			write(message);
			return 1;
		}
		return 0;
	}
#endif
#if __EFUN_DEFINED__(binary_message)
	int l, rc;

	l = strlen(message);
	rc = binary_message(message);
//	D("\t"+message);
# ifdef VOLATILE
	if (rc != l && rc != -1 && ME) ME->quit();
	// macht ärger: if (ME) destruct(ME);
# else
	D4( if (rc != l && rc != -1 && ME)
	  PP(("emit: (%O) %O of %O returned for %O from %O.\n",
	      rc > 256 ? rc/256 : rc, rc, l, ME->qName(), ME->vQuery("host"))));
# endif
	return rc == l;
#else
	tell_object(ME, message);
	return 1;
#endif
}

// right now no purpose in classic psyc server
printStyle(mc) { return ([]); }

#ifndef GAMMA
// p() is for "unimportant" output
//
// each user.c should override this method with its own variant
// if special formatting (html, irc protocol..) is appropriate
// otherwise this will output "unformatted" text, and should be
// called as such
//
// does anything use this anymore? TODO
string p(string fmt,	string a,string b,string c,string d,string e,string f,
			string g,string h,string i,string j,string k) {
	string message;
	PT(("p(%O,%O,%O,%O..)\n", fmt, a,b,c))
	message = sprintf(fmt, a,b,c,d,e,f,g,h,i,j,k);
	emit(message);
	return message;
}
#endif

#ifdef LPC3
// hm, this could move away TODO
telnet_negotiation(int cmd, int option, array(int) optargs) {
	P2(("telnet_negotiation(%O,%O,%O)\n", cmd,option,optargs))
// und heute sahen wir sowas:
//	telnet_negotiation(250,31,({ /* #1, size: 4 */
//	  0,
//	  81,
//	  0,
//	  30
//	}))

}
#endif
