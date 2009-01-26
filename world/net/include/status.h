#ifndef _INCLUDE_STATUS_H
#define _INCLUDE_STATUS_H

#include <storage.h>

#define SNICKER	(objectp(source) ? vars["_nick"] : source)

// 'verbosity' is an internal selection scheme to select aspects of a place
// status. the place status however should probably become its description
// as provided by /x or /surf, and the description data should in long term
// be multicast to all cslaves. thus all we really need is a way to ask our
// local cslave for the data we want to see. that's why we come full circle
// back to the current implementation of verbosity: since it is a local
// operation, it is not useful to define a generic key selector protocol
// for _request_description, but rather act out of current status of things
// and maybe mc extensions, which is what 'verbosity' does.

// place status flags
#define VERBOSITY_MEMBERS		1
#define VERBOSITY_TOPIC			2
#define VERBOSITY_HISTORY		4
#define VERBOSITY_UNIFORMS		8
#define	VERBOSITY_NEWSFEED		16
#define	VERBOSITY_MASQUERADE		32
#define	VERBOSITY_FILTER		64
#define	VERBOSITY_LOGGING		128
#define	VERBOSITY_PUBLICIZED		256
// user status flags
#define	VERBOSITY_FRIENDS		512
#define	VERBOSITY_FRIENDS_DETAILS	1024	// idle times of friends
#define	VERBOSITY_PLACE			2048	// info about current place
#define	VERBOSITY_EVENTS		4096	// last invitation etc.
#define	VERBOSITY_PRESENCE		8192	// showMyPresence(0)
#define	VERBOSITY_PRESENCE_DETAILS	16384	// showMyPresence(1)

// special case. never use in masks etc.
#define	VERBOSITY_AUTOMATIC	65536	// NOT in ALL!

#define	VERBOSITY_MEDIUM	VERBOSITY_TERSE | VERBOSITY_TOPIC
#define	VERBOSITY_ALL		(32768-1)

// masks for various occasions
#if 1 //def ENTER_MEMBERS
# define VERBOSITY_TERSE	0
# define VERBOSITY_BIG		VERBOSITY_MEDIUM
#else
# define VERBOSITY_TERSE	VERBOSITY_MEMBERS
# define VERBOSITY_BIG		VERBOSITY_MEDIUM | VERBOSITY_HISTORY
#endif

// each occasion has its mask or selection
// these are for places
#define	VERBOSITY_REQUEST_MEMBERS	VERBOSITY_MEMBERS
#define	VERBOSITY_ENTER_AUTOMATIC	VERBOSITY_TERSE
#define	VERBOSITY_ENTER			VERBOSITY_BIG
#define	VERBOSITY_STATUS_TERSE		VERBOSITY_TERSE
#define	VERBOSITY_STATUS		VERBOSITY_ALL - VERBOSITY_HISTORY
#define	VERBOSITY_IRCGATE_LOGON		VERBOSITY_TERSE | VERBOSITY_UNIFORMS
#define	VERBOSITY_IRCGATE_USER		VERBOSITY_ALL

// automatic user status in telnet & mud interfaces
#define VERBOSITY_STATUS_AUTOMATIC	VERBOSITY_PRESENCE | VERBOSITY_FRIENDS | VERBOSITY_MEMBERS

// these come from usercmd.i
#define	VERBOSITY_COMMAND_STATUS	VERBOSITY_ALL
#define	VERBOSITY_COMMAND_STATUS_TERSE	VERBOSITY_STATUS_AUTOMATIC
#define	VERBOSITY_COMMAND_STATUS_INFO	VERBOSITY_PRESENCE_DETAILS | VERBOSITY_FRIENDS_DETAILS | VERBOSITY_EVENTS
#define	VERBOSITY_COMMAND_FRIENDS	VERBOSITY_FRIENDS_DETAILS
// currently not necessary:
//efine	VERBOSITY_COMMAND_PRESENCE	VERBOSITY_PRESENCE_DETAILS
//efine	VERBOSITY_COMMAND_MEMBERS	VERBOSITY_MEMBERS

#endif
