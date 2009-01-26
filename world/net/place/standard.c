// $Id: standard.c,v 1.52 2007/09/08 21:25:20 lynx Exp $ // vim:syntax=lpc
//
// the room that used to get cloned when there is no specific program available.
// it has no owners and it is private by default.
//
// this is no longer the default place code. it is kept here in case you
// need to go back to the old style. don't inherit this.

#define BASIC
#define PLACE_MASQUERADE
#define PLACE_MASQUERADE_COMMAND
#define PLACE_PUBLIC_COMMAND
//#define PLACE_STORE_COMMAND
#include "archetype.gen"
