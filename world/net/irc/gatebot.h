// $Id: gatebot.h,v 1.26 2008/02/01 15:56:50 lynx Exp $ // vim:syntax=lpc

//#define IRCGATE_HIDE

#ifdef UNIXUSER
# define IRCGATE_USERID UNIXUSER
#else
# define IRCGATE_USERID "psyced"
#endif

#define CHAT_CHANNEL
#define CHAT_CHANNEL_MODE "t"

//#define DEF_WHOMASK "*.symlynX.com"

#include "irc.h"
#undef NO_INHERIT

#ifdef SERVER
# ifndef IRCGATE_NICK
#  define IRCGATE_NICK "PSYC.EU"
# endif
#else
# ifndef IRCGATE_NICK
#  if CHATNAME == "psyco"
#   define IRCGATE_NICK "PSYCgate"
#  else
#   define IRCGATE_NICK "|"+CHATNAME
#  endif
# endif
#endif

#define URL_DOC "http://www.psyc.eu/ircgate"
#define IRCGATE_NAME "[PSYC] gateway * /m " IRCGATE_NICK " help"

