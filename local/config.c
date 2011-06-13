// $Id: config.c,v 1.9 2006/11/07 07:58:36 lynx Exp $ vim:syntax=lpc
#include <net.h>

/* a data file or an include can be read by any file in the system but
 * we want this information to be readable by the gateway code only
 * that's why it has to be lpc code
 */

#ifdef GATEWAY_PATH

/* if you are positive that you want to run your own gateways to
 * legacy messaging systems, please insert your gateway credentials
 * into the fields below and activate the code by turning #if 0 to #if 1
 *
 * update: these bot-style gateways are not functional. don't switch
 * them to 1 as either the python scripts or the jabber code isn't
 * up to date with them.
 */

# define USE_ICQ_GATEWAY	0	// don't change
# define USE_AIM_GATEWAY	0	// don't change

qConfig() {
	string p = file_name(previous_object());
# ifdef __COMPAT_MODE__
	p = "/"+p;
# endif
	P3(("\n%O: config requested by %s\n", ME, p))
# if USE_ICQ_GATEWAY
	if (abbrev(GATEWAY_PATH "icq", p)) return
	       (["host" : "icq.localhost",
                 "port" : 5234,
                 "scheme" : "icq",
                 "name" : "icqlinker",
                 "secret" : "myicqsecret",
		 "nickname" : "your uin here",
                 "password" : "and your password please" ]);
# endif
# if USE_AIM_GATEWAY
	if (abbrev(GATEWAY_PATH "aim2", p)) return
	       (["host" : "aim.localhost",
                 "port" : 5233,
                 "scheme" : "aim",
                 "name" : "aimlinker",
                 "secret" : "myaimsecret",
		 "nickname" : "screen name",
                 "password" : "and your password please" ]);
# endif
}

load() {
# if USE_ICQ_GATEWAY
	D(" " GATEWAY_PATH "icq");
	load_object(GATEWAY_PATH "icq");
# endif
# if USE_AIM_GATEWAY
	D(" " GATEWAY_PATH "aim2");
	load_object(GATEWAY_PATH "aim2");
# endif
# ifdef RELAY_OBJECT
	D(" " RELAY_OBJECT "\n");
	call_out(load_object, 0, RELAY_OBJECT);
# endif
}

#endif
