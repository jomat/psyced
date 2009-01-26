// psyced supports multilingual message output
// these are the languages currently available
//	de_f = formal german: Sie-Form
//	en_g = geek english: looks like irc even in telnet, applet etc.
//
//	it   =	italian is 30% done, the rest is german,
//		so it is useless to most. don't use it yet.

#define	LANGUAGES	"de", "de_f", "en", "en_g", "it"

// if you define this variable in your local.h,
// telnet and other accesses will default to using
// that language instead of english

#ifndef DEFLANG
# define DEFLANG "en"
#endif
