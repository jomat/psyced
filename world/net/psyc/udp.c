// vim:foldmethod=marker:syntax=lpc:noexpandtab
// $Id: udp.c,v 1.36 2008/08/05 12:21:34 lynx Exp $
//
#include "common.h"
#include <net.h>
#include <person.h>
#include <services.h>
#include <uniform.h>
#include <psyc.h>

// SECURITY CONCERN:
// originating udp host and port numbers can rather easily be spoofed
// and currently many routers/gateways won't bother delivering a packet which
// couldn't possibly have come from that direction. UDP may generally
// need to be considered not trustworthy, even if it apparently comes
// from localhost. okay, hopefully our kernel is smart enough not to
// believe an incoming packet to be from localhost. it's his job!

#ifdef __PIKE__
import net.psyc.common;
#else
inherit PSYC_PATH "common";
#endif

// network location of remote server (hostname or hostname:port)
volatile mapping namecache = ([]);
// volatile int timeoutPending;

// TODO? should keep caches of host:port per remote target somewhat like tcp

#ifdef FORK
# include "routerparse.i"
#else
# include "parse.i"
#endif

object load() { return ME; } // avoid a find_object call in obj/master

parseUDP2(host, ip, port, msg) {
	string *m;
	int i, l
#ifdef FORK
		, parsed
#endif
			;

	unless (stringp(host)) host = ip;

	P3(( "parseUDP(%O,%O): %O\n", ip,port,msg ))

	pvars = ([ ]);

	// this call is a performance killer
	// should UDP get used heavily.. TODO
	// needs hash-based host matching, not linear
	unless (hostCheck(ip, port, 1)) {
		log_file("PSYCBLOCK", "UDP from "+ip
		    +" ignored » "+msg+" «\n");
	//	netloc = "psyc://"+ip+":"+port;
	//	sendmsg(netloc, "_error_illegal_source",
	//	   "Sorry, my configuration does not permit to talk to you.\n");
		return;
	}

	//netloc = "psyc://"+ip;
	peeraddr = peerhost = host;
	if (port != PSYC_SERVICE) {
	    peerport = port;
	    peeraddr += ":"+port+"d";	// this is a UDP peerport
	}
	// else: we presume a UDP 4404 also listens on TCP

#ifndef GAMMA
	P1(("./psyc/udp.c: paranoid extra restart\n"))
	restart();		// leading . does that anyway
#endif
	pvars["_INTERNAL_source"] = "psyc://"+peeraddr+"/";
	// m = regexplode(msg, "\r?\n"); ?
	m = explode(msg, "\n");

	for (l=0; l<sizeof(m); l++) {
		// i'd like to know why no-mc has to be an empty string
		// rather than NULL from now on.
		// should you find out, pls tell me.. -lynX
#ifdef FORK
		if (parsed == 2) getdata(m[l]);
		else unless (parsed = mmp_parse(m[l])) {
#else
		if (sizeof(mc)) getdata(m[l]);
		else unless (parse(m[l])) {
#endif
			log_file("PSYCHACK", "Can't parseUDP(%s): %O in %O\n",
			     peeraddr,m[l],msg );
			return;
		}
	}
}

parseUDP(ip, port, msg) {
    dns_rresolve(ip, #'parseUDP2, ip, port, msg);
}

#if 0
// in other words.. this shouldnt get called :)
pr(mc, fmt, a,b,c,d,e,f,g,h,i,j,k) {
	D("PSYC/UDP pr("+mc+", "+S(fmt, a,b,c,d,e,f,g,h,i,j,k)+")\n");
}
#endif

// no syntax error messages via udp
croak(a,b,c,d,e,f) {}

