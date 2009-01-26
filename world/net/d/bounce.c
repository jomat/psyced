// $Id: bounce.c,v 1.6 2005/03/14 13:30:46 lynx Exp $ // vim:syntax=lpc
//
// currently specific to the IRC implementation: recognize when a
// person has several clients kicking each other out automatically.
// yes, IRC client coders do implement things like these.
//
#include <net.h>

volatile mapping bounces = ([ ]);

#ifndef MAX_BOUNCES
# define MAX_BOUNCES	3
#endif
#ifndef BOUNCE_INTERVAL
# define BOUNCE_INTERVAL	60
#endif

reset() {
    foreach (string nick : bounces) {
	unless (bounces[nick]) m_delete(bounces, nick);
    }
}

checkBounce(nick) {
    string n = lower_case(nick);
    if (bounces[n] < (MAX_BOUNCES + 2)) {
	bounces[n]++;
	call_out(lambda(({}),
			 ({ #'--,
			  ({ #'[, bounces, n })
			 })), BOUNCE_INTERVAL);
    }
    return (bounces[lower_case(nick)] > MAX_BOUNCES);
}
