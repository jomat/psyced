#ifndef _INCLUDE_TEXT_H
#define _INCLUDE_TEXT_H
#ifndef __PIKE__

#ifndef NO_INHERIT
virtual inherit NET_PATH "textc";
#endif


// #ifndef DEBUG
#if 1
# define T(mc, fmt) \
	((objectp(_tob) && member(_tdb, mc)) ? _tdb[mc] : getText(mc, fmt))
#else
# define T(mc, fmt) \
	S("\n<<< %O >>>\n%s", _tdb, getText(mc, fmt))
#endif
// #endif

#endif // __PIKE__
#endif
