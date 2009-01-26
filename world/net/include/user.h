// $Id: user.h,v 1.15 2007/06/26 15:50:30 lynx Exp $ // vim:syntax=lpc:ts=8
//
// if you are including this, you are in some way implementing
// the user object or a derivate of the user object. that's
// what this macro is for.
#define USER_PROGRAM

#include <storage.h>
#include <peers.h>

#ifdef NO_INHERIT
# include <text.h>
#else
# define NO_INHERIT
# include <text.h>
# undef NO_INHERIT
inherit OPT_PATH "user";
#endif

#define FILTER_NONE		"off"
#define FILTER_STRANGERS	"strangers"

#ifdef _flag_filter_strangers
# define FILTERED(ENTITY)	v("filter") != FILTER_NONE
#else
# define FILTERED(ENTITY)	v("filter") == FILTER_STRANGERS
#endif

#define	SUBSCRIBE_NOT		0
#define	SUBSCRIBE_TEMPORARY	1
#define	SUBSCRIBE_PERMANENT	2

