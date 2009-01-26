#define _INCLUDE_INTERFACE_H

// driver abstraction kit -- abstraction layer from driver details

// generic string replacer
#define	prereplace(s)		(" "+s+" ")
#define replace(s, o, n)	implode(explode(s, o), n)
#define	postreplace(s)		(s = s[1..<2])

// let's use index() for strings and arrays
// to avoid confusion with mapping-member semantics
// in fact, with mudos we cant use member()
//
#define	index	strsrch
//efine	rindex	TODO

// amylaar provides strstr() to find a string in an other string
// in mudos this is done using strsrch
#define strstr	strsrch

// rexxism: is small an abbreviation of big?
#define abbrev(SMALL, BIG)     (strstr(BIG, SMALL) == 0)
// the same thing at the tail of the string
#define trail(SMALL, BIG)      (strstr(BIG, SMALL, -strlen(SMALL)) != -1)

// the way i use member is usually to see if a key is defined
// in a mapping, so here goes:
//
#define member(map, key)		(!undefinedp(map[key]))

// compare strings ignoring case
#define	stricmp(one, two)		(lower_case(one) != lower_case(two))

// amylaar mappings have other efun names than mudos (why?)
#define	m_delete	map_delete
#define	m_indices	keys
#define mappingp	mapp
#define walk_mapping	map_mapping

// debugging disabled for now	:-(
#define	D(text)	

// efuns mudos doesn't support
#define regreplace(str, patt, repl, f) \
	...

#pragma save_binary
// #pragma warnings
#pragma optimize all

#if DEBUG > 1
# pragma error_context
#endif

// this is said to work, but doesn't
//	#pragma no_save_types
//	#pragma no_strict_types

// MudOS doesnt have this:
#define __EFUN_DEFINED__(whatever) 0

// every lpc dialect has its own foreach syntax. aint that cute?
#define each(ITERATOR, LIST)		foreach(ITERATOR in LIST)
#define mapeach(KEY, VALUE, MAPPING)	foreach(KEY, VALUE in MAPPING)

