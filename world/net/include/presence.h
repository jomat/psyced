// $Id: presence.h,v 1.9 2007/07/25 09:57:32 lynx Exp $ // vim:syntax=lpc:ts=8
//
#ifndef _INCLUDE_PRESENCE_H
#define _INCLUDE_PRESENCE_H

// similar to what is defined as "availability"
// in http://www.psyc.eu/presence
// yeah! we WILL be getting there one happy day.

#define AVAILABILITY_EXPIRED		0	// as yet unused.
#define AVAILABILITY_UNKNOWN		0	// in use internally.
#define AVAILABILITY_OFFLINE		1	// in use.
#define AVAILABILITY_VACATION		2	// activated, better name?
#define AVAILABILITY_AWAY		3	// in use.
		    // UNAVAILABLE ?
#define AVAILABILITY_DO_NOT_DISTURB	4	// activated, better name?
#define AVAILABILITY_NEARBY		5	// activated.
#define AVAILABILITY_BUSY		6	// in use.
#define AVAILABILITY_HERE		7	// in use.
		    // AVAILABLE ?
#define AVAILABILITY_TALKATIVE		8	// activated, name?
#define AVAILABILITY_REALTIME		9	// as yet unused, name?

// more unused stuff from http://www.psyc.eu/presence
#define	MOOD_JUCHEI			8
#define	MOOD_BASSTSCHO			6
#define	MOOD_NAJA			4
#define	MOOD_LEXTSMIAMOARSCHI		2
// see also english wording currently in net/library/share.c

#endif
