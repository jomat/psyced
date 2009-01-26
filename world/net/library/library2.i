//						vim:noexpandtab:syntax=lpc
// $Id: library2.i,v 1.19 2007/10/08 11:00:31 lynx Exp $

#include <sandbox.h>

// one day this could be merged with PSYC_SYNCHRONIZE into
// channels of the same context
static object monitor;

void monitor_report(string mc, string text) {
#if DEBUG < 2
	debug_message("MONITOR: "+ text +"\n");
#endif
	log_file("MONITOR", mc +"\t"+ text +"\n");
#ifndef __PIKE__    // TPD
	unless (monitor) monitor = load_object(PLACE_PATH "monitor");
	if (monitor) monitor->msg(previous_object() || query_server_unl(),
                                     mc, text, ([]));
#endif
}

