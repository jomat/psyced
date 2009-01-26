// vim:foldmethod=marker:syntax=lpc:noexpandtab
// $Id: udp.c,v 1.7 2008/07/17 15:07:59 lynx Exp $

#include "psyc.h"
#include <net.h>
#include <url.h>
#include <text.h>

inherit NET_PATH "spyc/parse";

string netloc;

object load() { return ME; } // avoid a find_object call in obj/master

// respond to the first packet - or don't do it
first_response() { }

parseUDP2(host, ip, port, msg) {
    // this is an atomic operation. It is never interrupted by another 
    // call to parseUDP2. Or at least it is not designed to be.
    unless(stringp(host))
	host = ip; // FIXME: we reject tcp from hosts with invalid pointers
	// but not udp???
    netloc = "psyc://" + host + ":" + to_string(-port) + "/";
    P0(("parseUDP2 from %O == %O  port %O\n", host, ip, port))
    parser_init();
    feed(msg);
}

parseUDP(ip, port, msg) {
    P0(("parseUDP from %s:%O %O\n", ip, port, msg))
    dns_rresolve(ip, #'parseUDP2, ip, port, msg);
}

// dispatch for UDP isn't really working, but it is completely
// ignoring the routing vars.. TODO FIXME
#define PSYC_UDP
#include "dispatch.i"
