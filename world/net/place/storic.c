// $Id: storic.c,v 1.102 2007/09/08 21:25:20 lynx Exp $ // vim:syntax=lpc
//
// dont ask what "storic" is supposed to mean..
// it's a room object that keeps a lastlog, a history of the last messages
//
// this is the old storic implementation, used in inheritance chains.
// some old code may need it, but don't write new code on top of this.

#define BASIC

#define PLACE_FILTERS
#define PLACE_HISTORY
#define PLACE_HISTORY_EXPORT
#define PLACE_LOGGING
#define PLACE_STORE_COMMAND
#include "archetype.gen"
