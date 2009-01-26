// $Id: trust.c,v 1.5 2008/01/22 13:13:09 lynx Exp $ // vim:syntax=lpc
//
// we're still not sure if this file deserves existence
// it's a common file between interserver actives and passives
// but also psyc/udp
//
#include <net.h>

volatile int trustworthy;

qScheme() { return 0; }

hostCheck(host, port, udpflag) {
        trustworthy = legal_host(host, port, qScheme(), 0);
	P3(("%O hostCheck %O, %O .. trust %O\n", ME,
	    host, port, trustworthy))
	return trustworthy;
}

volatile mapping authhosts;

void sAuthenticated(string hostname) { 
    P3(("sAuthenticated: %O\n", hostname))
    unless(authhosts && mappingp(authhosts)) authhosts = ([ ]);
    authhosts[hostname] = 1;
} 

int qAuthenticated(mixed hostname) {
    P3(("qAuthenticated %O, %O\n", hostname, authhosts))
    unless (authhosts && mappingp(authhosts)) return 0;
    return member(authhosts, hostname);
}

void sAuthHosts(mapping h) {
    authhosts = h;
}
  

// maybe canonical_host(cane, ip, host) belongs here too?

