// $Id: irc.h,v 1.19 2008/08/11 09:13:12 lynx Exp $ // vim:syntax=lpc

// local debug messages - turn them on by using psyclpc -DDirc=<level>
#ifdef Dirc
# undef DEBUG
# define DEBUG Dirc
#endif

#include <net.h>

#ifndef NO_INHERIT
inherit IRC_PATH "common";
#endif

#define easychannel2place(name)	(index("+&#", name[0]) != -1 ? name[1..] : 0)
#define channel2place(name)	(name[0] == '#' ? name[1..] : 0)
#define place2channel(name)	("#"+name)

#ifdef _flag_encode_uniforms_IRC
# define uniform2irc(UNIT) (v("verbatimuniform") == "off"? replace(UNIT, "@", "%") : UNIT)
#else
# define uniform2irc(UNIT) (UNIT)
#endif

#ifdef SERVER
# define SERVER_SOURCE ""
#else
# ifdef RELAY
#  define SERVER_SOURCE ":" IRCGATE_NICK " "
# else
#  define SERVER_SOURCE ":" SERVER_HOST " "
# endif
#endif

#define	MAX_IRC_BYTES	512

#ifndef IRCD
# define	IRCD	D3
#endif

