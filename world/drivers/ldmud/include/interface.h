// $Id: interface.h,v 1.54 2008/08/26 13:42:24 lynx Exp $ // vim:syntax=lpc:ts=8
#define _INCLUDE_INTERFACE_H

// let's stay compatible for a while
// -> if we want to do so, we got to check for the LDMUD version. 3.2.9 doesn't
// get it.
//#if __VERSION_MAJOR__ == 3 && __VERSION_MINOR__ > 2
# define SAVE_FORMAT     0
//#endif

#ifndef __psyclpc__
# define DRIVER_VERSION  "ldmud/" __VERSION__
#else
# define DRIVER_VERSION  "psyclpc/" __VERSION__

# define hex2int(HEX)	to_int("0x"+ HEX)
#endif

// driver abstraction kit -- abstraction layer from driver details

#pragma combine_strings
#if DEBUG > 0
# pragma verbose_errors
//#else
//# pragma no_verbose_errors
#endif

// [-1] vs [<1] syntax kludge
#define char_from_end(WHAT, NUMBER)     WHAT[< NUMBER]
#define slice_from_end(WHAT,FROM,TO)    WHAT[FROM ..< TO]

#define	next_input_to(CALLBACK)	input_to(CALLBACK, INPUT_IGNORE_BANG)

#if __EFUN_DEFINED__(strstr)
// rexxism: is small an abbreviation of big?
# define abbrev(SMALL, BIG)     (strstr(BIG, SMALL) == 0)
// the same thing at the tail of the string
# define trail(SMALL, BIG)      (strstr(BIG, SMALL, -strlen(SMALL)) != -1)
#else
# define abbrev(SMALL, BIG)     (SMALL == BIG[0..strlen(SMALL)-1])
# define trail(SMALL, BIG)      (SMALL == BIG[<strlen(SMALL)..])
#endif

// generic string replacer
#define replace(s, o, n)	implode(explode(s, o), n)
// these are historic, from the days when explode() used to
// throw away delimiters at the edge of the string
//#define	prereplace(s)		(" "+s+" ")
//#define	postreplace(s)		(s = s[1..<2])
//
// backward compat -- useful when writing multi-driver code
#define	prereplace(s)		(s)
#define	postreplace(s)		(s)

// let's use index() for strings and arrays
// to avoid confusion with mapping-member semantics
//
#define	index member
#define rindex rmember

// old stuff
#if ! __EFUN_DEFINED__(send_udp)
# define send_udp(host, port, msg)	send_imp(host, port, msg)
#endif
#if ! __EFUN_DEFINED__(query_udp_port)
# define query_udp_port			query_imp_port
#endif

// compare strings ignoring case
#define	stricmp(one, two)		(lower_case(one) != lower_case(two))

#define legal_keyword(WORD)	(!regmatch(WORD, "[^_0-9A-Za-z]"))

#if ! __EFUN_DEFINED__(clonep)
# define clonep(ob)	(objectp(ob) && member(file_name(ob), '#') >= 0)
#endif

// to_string behaves differently with ldmud.. sigh
#define	o2s(any)	((objectp(any)?"/":"")+to_string(any))

// object to http URL (without http://host:port) conversion macros
#define object2url(ob) replace( stringp(ob)?ob:("/"+to_string(ob)), "#", "," )
#define url2object(ob) replace( ob, ",", "#" )

#define file_name(ob)	object_name(ob)

// see net/library for verbose version of this function
#if 1
# define is_formal(n)		((index(n,':')!=-1||index(n,'.')!=-1) && n)
#else
# define is_formal(n)		((index(n,':')!=-1||index(n,'@')!=-1) && n)
#endif
#define lower_uniform(n)	(is_formal(n) && lower_case(n))

// nosave? static? volatile. only for variables, not methods!
// another nice word for the opposite of persistent would be "shed"
#define	volatile	nosave

// every lpc dialect has its own foreach syntax. aint that cute?
#define each(ITERATOR, LIST)		foreach(ITERATOR : LIST)
#define mapeach(KEY, VALUE, MAPPING)	foreach(KEY, VALUE : MAPPING)

#define	array(TYPE)	TYPE *

// the simple concept of the caller isn't so simple in lpc
#define caller (extern_call() ? previous_object() : this_object())

// a pike compatible closure syntax.. using a macro.. ain't that smarty?
#define CLOSURE(args, tcontext, context, code) function mixed args code

// pike's varargs syntax requires me to do some tricks here
#define	vaclosure	closure
#define	vamapping	mapping
#define	vastring	string
#define	vaobject	object
#define	vamixed		mixed
#define	vaint		int

// extracts hh:mm:ss format from ctime output
#define hhmmss(CTIME)   CTIME[11..18]
// extracts hh:mm format from ctime output (for idle times)
#define hhmm(CTIME)     CTIME[11..15]
// typical timestamp string: hhmm or iso-date if older than 24 hours
#define time_or_date(TS) \
    (time() - TS > 24*60*60 ? isotime(TS, 0) : hhmm(ctime( TS )))

#if __EFUN_DEFINED__(convert_charset)
# ifdef TRANSLIT // TRANSLIT has no effect whatsoever. grrr!
#  define iconv(s, FROM, TO) (s = convert_charset(s, FROM, TO +"//TRANSLIT"))
# else
#  define iconv(s, FROM, TO) {                                  \
        string itmp;                                            \
        if (catch(itmp = convert_charset(s, FROM, TO); nolog)) {\
            P1(("catch! iconv %O %O in %O\n", FROM, TO, ME))    \
	} else s = itmp;                                        \
}
# endif
#else
# define iconv(s, FROM, TO)
# define convert_charset(s, FROM, TO)	(s)
#endif

#define	AMOUNT_SOCKETS	sizeof(users())

#ifdef DEBUG
# if DEBUG > 0
// enables you to output useful things like in your P1() etc.
#  define DEBUG_TRACE	debug_info(DINFO_TRACE, DIT_STR_CURRENT)
# else
#  define DEBUG_TRACE	"(DEBUG_TRACE disabled: DEBUG level below 1)"
# endif
#endif

// some ldmud versions previous to 610 have a problem with digest-md5
#if __VERSION_MAJOR__ < 4 && __VERSION_MICRO__ < 611
# echo Warning: Your driver is so old, it cannot do DIGEST-MD5
# define _flag_disable_authentication_digest_MD5
#endif
