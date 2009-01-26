// $Id: auto.h,v 1.14 2007/09/07 09:07:32 lynx Exp $ // vim:syntax=lpc:ts=8
#define _INCLUDE_AUTO_H

// first identify the driver
// then load the driver abstraction kit
//
#ifdef __PIKE__

# include "../../drivers/pike/include/interface.h"
# define ME	this

#else // PIKE

#ifdef MUDOS
# define DRIVER_PATH	"/drivers/mudos/"
# include <interface.h>
#else
//# if __EFUN_DEFINED__(filter)
# ifdef __LDMUD__	// also matches __psyclpc__
//#  define LDMUD	// we can check for __LDMUD__ instead
#  ifdef MUD
#   define DRIVER_PATH	"/net/drivers/ldmud/"
#  else
#   define DRIVER_PATH	"/drivers/ldmud/"
#  endif
# else
#  define AMYLAAR
#  define DRIVER_PATH	"/drivers/amylaar/"
# endif
# ifdef DRIVER_PATH
#  include DRIVER_PATH "sys/input_to.h"
// hmm.. wanted to make this DEBUG>1 only, but.. doesn't work.. tant pis
#  include DRIVER_PATH "sys/debug_info.h"
#  include DRIVER_PATH "include/interface.h"
# endif
#endif

// useful global macros
#define ME	this_object()

#endif // PIKE

// perlisms for readability
#define	unless(COND)	if (!(COND))
#define	until(COND)	while (!(COND))

// more useful perlisms
#define	chop(STRING)	slice_from_end(STRING, 0, 2)
#define	chomp(STRING)	(char_from_end(STRING, 1) == '\n' ? chop(STRING) : STRING)

// extracts hh:mm:ss format from ctime output
#define	hhmmss(CTIME)	CTIME[11..18]
// extracts hh:mm format from ctime output (for idle times)
#define	hhmm(CTIME)	CTIME[11..15]
