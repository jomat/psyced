// $Id: interface.h,v 1.32 2008/04/18 13:34:38 lynx Exp $ // vim:syntax=lpc:ts=8
// several things in here do not look like they were optimized for pike.. TODO

#define _INCLUDE_INTERFACE_H

#define DRIVER_VERSION  ("pike/"+ __VERSION__)

// driver abstraction kit -- abstraction layer from driver details

// [-1] vs [<1] syntax kludge
#define	char_from_end(WHAT, NUMBER)	WHAT[- NUMBER]
#define	slice_from_end(WHAT,FROM,TO)	WHAT[FROM .. sizeof(WHAT)-TO]

// pike also provides search() says eMBee....
#define abbrev(SMALL, BIG)     (SMALL == BIG[0..sizeof(SMALL)-1])
#define trail(SMALL, BIG)      (SMALL == BIG[sizeof(BIG)-sizeof(SMALL) ..])

// is it this simple? let's try
#define strstr			search
#define raise_error(ERROR)	throw(ERROR)
#define shutdown()		exit(0)
#define clone_object(XXX)	XXX()
#define named_clone(XXX, NAME)	XXX()->sName(NAME)
#define file_name(OBJECT)	to_string(OBJECT)
#define copy(WHATEVER)		copy_value(WHATEVER)
#define	find_object(XXX)	((object) XXX)
#define	m_indices		indices
#define	m_values		values

// generic string replacer
//#define replace(s, o, n)	implode(explode(s, o), n)
// exists as a system function in pike with exact same arguments, phew!

// backward compat -- useful when writing multi-driver code
#define	prereplace(s)		(s)
#define	postreplace(s)		(s)

// let's use index() for strings and arrays
// to avoid confusion with mapping-member semantics
//
#define	index(STRING, CHAR) member(STRING, CHAR)
#define	rindex(STRING, CHAR) rmember(STRING, CHAR)
//
// but pike doesn't have member, it's called has_index()
// we keep on using index() and member() in all languages
#define	member(HAYSTACK, NEEDLE)	has_index(HAYSTACK, NEEDLE)
// you might also want to have a look on has_value(). well. maybe not.

// strlen() is obsolet/deprecated in pike, sizeof() is suggested.
// well, i suggest we keep on using strlen() for strings, as we never know what
// other languages are popping up that psyced has to be ported to.
#define strlen(str)	sizeof(str)

// pike doesn't know previous_object. but we can emulate it.
#define previous_object()	function_object(backtrace()[-2][2])

// compare strings ignoring case
#define	stricmp(one, two)		(lower_case(one) != lower_case(two))

// implode & explode are operator operations in pike.
#define implode(what, with)	((what) * (with))
#define explode(what, with)	((what) / (with))

// brilliant. exactly the same interface. saga did a good job!  :)
#define	dns_resolve	Protocols.DNS.async_host_to_ip
#define	dns_rresolve	Protocols.DNS.async_ip_to_host

// this is useless really, since programs will never run as objects (?)
//#define clonep(ob)	(!programp(ob))
// so we have to rewrite some stuff..
// none of our objects supports isClone()
// this is just a first approach to the problem
#define clonep(ob)	(ob->isClone())

// all of this object naming stuff won't work unless we name our clones
// at creation time.. so plenty to be done here
//
// to_string behaves differently with ldmud.. sigh
#define	o2s(any)	((objectp(any)?"/":"")+sprintf("%O",any))
// object to http URL (without http://host:port) conversion macros
#define object2url(ob) replace( stringp(ob)?ob:("/"+sprintf("%O",ob)), "#", "," )
#define url2object(ob) replace( ob, ",", "#" )

// nosave? static? volatile. no idea if pike has something like this
#define	volatile	static

// every lpc dialect has its own foreach syntax. aint that cute?
#define	each(ITEM, LIST)		foreach(LIST; ; ITEM)
#define	mapeach(KEY, VALUE, MAPPING)	foreach(MAPPING; KEY; VALUE)

#define	pointerp	arrayp

// thoughts on closures in lpc and pike.
//
// the new context closure syntax of ldmud looks either like this:
//	int x=5;
//	closure cl = function int : (int x) { return x++; };
//
// or even mudossish like this:
//	int x=5;
//	closure cl = (: return x++; :);
//
// while this is pike instead:
//	int x=5;
//	function factory(int y) { return lambda() { return y++; }; };
//	function cl = factory(x);
//
// are you sure we cannot return a function without naming it factory first?

#define CLOSURE(args, tcontext, context, code) lambda tcontext { return lambda args code; } context

// typical usage:
// 	int x=5;
//	function f = CLOSURE((int y), (int x), (x), { return x * y; });

#define virtual
#define closure		function

// pike's varargs syntax requires me to do some tricks here
#define varargs
#define vaclosure	function|void
#define vamapping       mapping|void
#define vaobject        object|void
#define vastring        string|void
#define vamixed         mixed|void
#define vaint		int|void

#define	to_string(XXX)		((string) XXX)
#define	to_int(XXX)		((int) XXX)

// ... this stuff may want to be done in a more pikey way
// extracts hh:mm:ss format from ctime output
#define hhmmss(CTIME)   CTIME[11..18]
// extracts hh:mm format from ctime output (for idle times)
#define hhmm(CTIME)     CTIME[11..15]
// typical timestamp string: hhmm or iso-date if older than 24 hours
#define time_or_date(TS) \
    (time() - TS > 24*60*60 ? isotime(TS, 0) : hhmm(ctime( TS )))

// TODO *** stuff that needs to be solved better *** //
#define interactive(ME) ME
//#define find_service(NAME) 0
//#define find_person(NAME) find_object(NAME)
#define query_udp_port() 4404
#define T(METHODTODO, DEFAULTTEXT)    DEFAULTTEXT
#define OSTYPE      "TODO"
#define MACHTYPE    "TODO"

// leaving out a number here crashes pike.. amazing
#define	DEBUG 1
#define	DEVELOPMENT
