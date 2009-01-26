// $Id: owned.c,v 1.101 2007/09/08 21:25:20 lynx Exp $ // vim:syntax=lpc
//
// a room that has an owner and friends to help him
// traditional psycmuve set-up, just in case old place code uses it
// don't use this for new code
//
#define BASIC

#define PLACE_FILTERS
#define PLACE_HISTORY
#define PLACE_HISTORY_EXPORT
#define PLACE_SCRATCHPAD
#define PLACE_LOGGING
#define PLACE_OWNED
#define PLACE_STORE_COMMAND
#define PLACE_STYLE_COMMAND

#include "archetype.gen"
