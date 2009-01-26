// $Id: master.c,v 1.4 2005/03/14 10:23:27 lynx Exp $ // vim:syntax=lpc
//
#include <net.h>

#ifdef MASTER_LINK

inherit PSYC_PATH "active";

reset() {
	if (!interactive()) {
		D("..\n(reconnecting master link)\n");
		connect(MASTER_LINK);
	}
	// return ::reset();
}

#endif
