#ifndef PSYC_H
#define PSYC_H

/* psyc.h: Unlike other files in this directory, this file is licensed under
 * the MIT license as documented in detail in ../psyc/LICENSE.
 */

#ifndef PSYC_LIST_SIZE_LIMIT
# define PSYC_LIST_SIZE_LIMIT 404
#endif

#ifdef SPYC
# define C_GLYPH_PACKET_DELIMITER	'|'
# define S_GLYPH_PACKET_DELIMITER	"|"
#else
# define C_GLYPH_PACKET_DELIMITER	'.'
# define S_GLYPH_PACKET_DELIMITER	"."
#endif

#define	C_GLYPH_SEPARATOR_KEYWORD	'_'
#define	S_GLYPH_SEPARATOR_KEYWORD	"_"

#define	C_GLYPH_MODIFIER_SET		':'
#define	S_GLYPH_MODIFIER_SET		":"

#define	C_GLYPH_MODIFIER_ASSIGN		'='
#define	S_GLYPH_MODIFIER_ASSIGN		"="

#define	C_GLYPH_MODIFIER_AUGMENT	'+'
#define	S_GLYPH_MODIFIER_AUGMENT	"+"

#define	C_GLYPH_MODIFIER_DIMINISH	'-'
#define	S_GLYPH_MODIFIER_DIMINISH	"-"

#define	C_GLYPH_MODIFIER_QUERY		'?'
#define	S_GLYPH_MODIFIER_QUERY		"?"

#define PSYC_ROUTING			1
#define PSYC_ROUTING_MERGE		2
#define PSYC_ROUTING_RENDER		4

// I thought about changing all occurrencies of these chars in parse.i
// but it only makes parse.i less readable and changes of these
// modifiers are just not to be expected. It is however recommended
// to use these macros if you look at the psyc syntax anywhere outside
// the core psyc parser, like when splitting methods by '_'.
//
// here are the macros to implement method inheritance in a loop around
// a switch (see http://about.psyc.eu/Inheritance and "try and slice")

#ifdef EXPERIMENTAL 
# define PSYC_TRY(mc) \
	family = mc; \
	while (family) { \
		glyph = -4; \
		switch(family)

# define PSYC_SLICE_AND_REPEAT \
        default: \
		log_file("SLICE", "%s:%O slicing %O in %O\n", \
		    __FILE__, __LINE__, family, ME); \
                glyph = rmember(family, C_GLYPH_SEPARATOR_KEYWORD); \
                if (glyph > 1) family = family[.. glyph-1]; \
                else family = 0; \
	} \
	if (glyph == -4) family = 0; // got here by break;

#else
// this disables method inheritance
# define PSYC_TRY(mc) switch(mc)
# define PSYC_SLICE_AND_REPEAT
#endif

#endif /* PSYC_H */
