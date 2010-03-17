#include <net.h>
#define NAME "RendezVous"

#ifdef BRAIN
// psyc://psyced.org/@rendezvous currently disabled
//  ("BRAIN" is only defined on psyced.org)
# define REDIRECT "psyc://psyced.org/@welcome"
#else
// this is the default configuration of the default chatroom of psyced.

// allowing /history in the default place admins are going to use is
// dangerous to privacy, because they haven't learned about /history yet.
// shutting out all remotes by default is an option to go with it, but
// it can be nice to come in and say hello to a new admin. so i'm leaving
// both commented out.
//
//# define PLACE_HISTORY    // if you want to keep a /history
//# define LOCAL	    // if you don't want to allow remote users here
//# define PLACE_MASQUERADE // currently brokenish
#endif

#include <place.gen>	// now generate the place according to the rules

