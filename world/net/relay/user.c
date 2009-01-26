// $Id: user.c,v 1.3 2006/11/05 21:33:19 fippo Exp $
//
// <lynX> relay users serve the purpose of implementing all the features of
// a PSYC/Jabber identity for users of an IRC network whose identities are
// auth'd by a NickServ. you may of course use it for other types of
// authenticated networks or systems. further integration between this and
// the system in question are of course welcome to be implemented.
//
// right now this simply forwards messages. we soon have to add a way for
// the owner of this to issue commands.
//
#include <net.h>
#include <user.h>

volatile object relay;

qScheme() { return "relay"; }

msg(source, mc, data, vars, showingLog, target) {
	if (showingLog) return;
	unless (relay) {
		//relay = RELAY_OBJECT -> load();
		relay = find_object(RELAY_OBJECT);
		PT(("%O using relay %O\n", ME, relay))
		unless (relay) return;	// we have a problem
	}
	PT(("%O relaying %O from %O\n", ME, mc, source))
	//vars["_nick_target"] = MYNICK;
	
	/* we need to remember this for QUIT handling */
	if (abbrev("_echo_place_enter", mc)) 
	    unless (places[source]) 
		places[source] = vars["_nick_place"];
	relay->msg(source || previous_object(), mc, data, vars,
		  showingLog, MYNICK); // we need mynick here for ircgate
				      // instead of target
	// we aren't calling ::msg() thus all user functions are ineffective
	// as of now
	return;
}
