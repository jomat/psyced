// $Id: ghost.c,v 1.9 2008/04/24 15:19:36 lynx Exp $
//
// <lynX> ghost users serve the purpose of implementing all the features of
// a PSYC/Jabber identity for users of an IRC network whose identities are
// auth'd by a NickServ.

#define GHOST

#include <net.h>

#undef SERVER_HOST
#define SERVER_HOST IRCGATE_NICK

volatile object relay;

#include "user.c"

protected int emit(string output) {   
	unless (relay) {
		//relay = RELAY_OBJECT -> load();
		relay = find_object(RELAY_OBJECT);
		PT(("%O using relay %O\n", ME, relay))
		unless (relay) return quit();	// we have a problem
	}
	relay->emit(output, remotesource);
}

#if 0
msg(source, mc, data, vars, showingLog, target) {
	if (showingLog) return;
	PT(("%O relaying %O from %O\n", ME, mc, source))
	//vars["_nick_target"] = MYNICK;
#ifdef IRCGATE_CHANNELS
	/* we need to remember this for QUIT handling */
	if (abbrev("_echo_place_enter", mc)) 
	    unless (places[source]) 
		places[source] = vars["_nick_place"];
#endif
	relay->msg(source || previous_object(), mc, data, vars,
		  showingLog, MYNICK); // we need mynick here for ircgate
				      // instead of target
	// we aren't calling ::msg() thus all user functions are ineffective
	// as of now
	return;
}
#endif
