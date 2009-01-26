// $Id: debug.h,v 1.23 2008/01/07 14:15:48 lynx Exp $ // vim:syntax=lpc:ts=8
/* simplified form of conditional compilation */

#ifdef CONSOLE_CHARSET
# if CONSOLE_CHARSET != SYSTEM_CHARSET
#  define ICONV(TEXT)	convert_charset(TEXT, SYSTEM_CHARSET, CONSOLE_CHARSET)
# else
#  define ICONV(TEXT)	TEXT
# endif
#else
# define ICONV(TEXT)	TEXT
#endif

#ifdef __LDMUD__
//# define	D(text)		log_file(DEBUG_LOG, text)
# define	D(text)		debug_message(ICONV(text), 1+4) // stdout + file
# define	PV(args)	debug_message(ICONV(sprintf args), 4)	// file only
#else
# define	D(text)		debug_message(ICONV(text))
# define	PV(args)	debug_message(ICONV(sprintf args))
#endif

// obsolete..?
#define S	sprintf

// the cool new thang:
#define PP(args)	D(sprintf args)
// the old thang, but it's still being used in JaZ/* and suchlike
#define P		printf

#ifndef DEBUG_FLAGS
# ifdef DEBUG
#  ifndef DEVELOPMENT
#   define PT(MSG)
#   define DT(CODE)
#  else
#   define PT(MSG)	PP(MSG);	/* temporary massive debug mode */
#   define DT(CODE)	CODE
#  endif
#  if DEBUG == 1
#   define DEBUG_FLAGS 0x03
#  else
#   if DEBUG == 2
#    define DEBUG_FLAGS 0x07
#   else
#    if DEBUG == 3
#     define DEBUG_FLAGS 0x0f
#    else
#     if DEBUG == 4
#      define DEBUG_FLAGS 0x1f
#     else
#      define DEBUG_FLAGS 0x01
#     endif
#    endif
#   endif
#  endif
# else
#  define PT(MSG)
#  define DT(CODE)
#  define DEBUG_FLAGS 0x00 /* no debugging */
# endif
#endif

#if DEBUG_FLAGS & 0x01
# define D0(CODE)	CODE
# define P0(MSG)	PP(MSG);
#else
# define D0(CODE)
# define P0(MSG)
#endif

#if DEBUG_FLAGS & 0x02
# define D1(CODE)	CODE
# define P1(MSG)	PP(MSG);
// just can't go into interface.h 
# pragma save_types
# pragma warn_deprecated
#else
# define D1(CODE)
# define P1(MSG)
#endif

#if DEBUG_FLAGS & 0x04
# define D2(CODE)	CODE
# define P2(MSG)	PP(MSG);
#else
# define D2(CODE)
# define P2(MSG)
#endif

#if DEBUG_FLAGS & 0x08
# define D3(CODE)	CODE
# define P3(MSG)	PV(MSG);
#else
# define D3(CODE)
# define P3(MSG)
#endif

#if DEBUG_FLAGS & 0x10
# define D4(CODE)	CODE
# define P4(MSG)	PV(MSG);
#else
# define D4(CODE)
# define P4(MSG)
#endif

#if DEBUG > 0
# ifdef STRICT
#  define ASSERT(NAME,COND,VALUE) { unless (COND) { \
    PP(("Assertion %s failed in %O: %O\n", NAME, ME, VALUE)); \
    raise_error("Assertion failed (strict mode).\n"); } }
# else
#  define ASSERT(NAME,COND,VALUE) { unless (COND) \
    PP(("Assertion %s failed in %O: %O\n", NAME, ME, VALUE)); }
# endif
#else
# define ASSERT(NAME,CONDITION,VALUE)
#endif

// #define WHERE_AM_I \
//    PP(("FILE " __FILE__ " DIR " __DIR__ " PATH0 " __PATH__(0) "\n"));

