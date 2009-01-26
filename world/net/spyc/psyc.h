/* this is the development directory for
 * psyc 1.0.
 *
 * the entire net/speck directory has to go
 * when net/psyc stops being 0.9 and starts
 * being 1.0.
 *
 * please try to make changes by #ifdef SPYC
 * where possible, only do a plugin replacement
 * of parse.i
 *
 * why do we need this directory? let me explain:
 * if old and new psyc are to co-exist, we need
 * differing path names for objects to assign
 * to the differing ports. even if we merge the
 * parsers and make them coexist by detecting the
 * first incoming byte, then we still need a way
 * to distinguish outgoing PSYC and SPYC.
 * also, merging the two parsers is not likely or
 * useful - they are so totally different in
 * structure - but we can exec the proper ones
 * from psyclpc or internally, after looking at
 * the first byte.
 */

#define	SPYC

#define PSYCPARSE_STATE_HEADER 0
#define PSYCPARSE_STATE_CONTENT 1
#define PSYCPARSE_STATE_BLOCKED 2
#define PSYCPARSE_STATE_GREET 3

#include <psyc.h>
