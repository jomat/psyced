#define _INCLUDE_INTERFACE_H

#define AMYLAAR

// manual change here.. hmmm
#define DRIVER_VERSION	"LPMUD/3.2.1.125"

// driver abstraction kit -- abstraction layer from driver details

#define	next_input_to(CALLBACK)	input_to(CALLBACK);

#pragma combine_strings
#pragma verbose_errors

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
#define	prereplace(s)		(" "+s+" ")
#define replace(s, o, n)	implode(explode(s, o), n)
#define	postreplace(s)		(s = s[1..<2])

// let's use index() for strings and arrays
// to avoid confusion with mapping-member semantics
//
#define	index member
#define	rindex rmember

#define send_udp(host, port, msg)	send_imp(host, port, msg)
#define query_udp_port			query_imp_port

// compare strings ignoring case
#define	stricmp(one, two)		(lower_case(one) != lower_case(two))

#define clonep(ob)	(objectp(ob) && member(file_name(ob), '#') >= 0)

#define	o2s(ob)		to_string(ob)

// object to (relative) http URL conversion macros
#define object2url(ob)	replace( to_string(ob), "#", "," )
#define url2object(ob)	replace( to_string(ob), ",", "#" )

// varargs introduced in 3.2.1@132
#define	varargs

#define	AMOUNT_SOCKETS	sizeof(users())

