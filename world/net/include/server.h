// $Id: server.h,v 1.5 2006/08/24 11:43:36 lynx Exp $ // vim:syntax=lpc:ts=8

#ifdef NO_INHERIT
# include <text.h>
#else
# define NO_INHERIT
# include <text.h>
# undef NO_INHERIT

  inherit NET_PATH "server";
#endif

#define	QUIT	destruct(ME); return 0;

#ifndef TIME_LOGIN_IDLE
# define TIME_LOGIN_IDLE 44
#endif
