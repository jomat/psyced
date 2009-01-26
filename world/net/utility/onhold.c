// $Id: onhold.c,v 1.2 2007/10/02 10:35:46 lynx Exp $ // vim:syntax=lpc:ts=8
//
// keep a connection on hold. used during shutdown period
// to avoid clients and such to reconnect before we are
// actually restarting.
//
#include <net.h>

onhold(in) {
	next_input_to(#'onhold);
#if DEBUG > 0
# ifdef _flag_log_hosts
	log_file("ONHOLD", "[%s] %s (%O) %O\n", ctime(),
             query_ip_name(ME) || "?", query_mud_port(ME), in);
# endif
#endif
}

logon() {
	P1(("On hold: %s (%O)\n", query_ip_name(ME) || "?", query_mud_port(ME)))
	next_input_to(#'onhold);
}
