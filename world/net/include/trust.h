// $Id: trust.h,v 1.2 2008/07/26 13:18:06 lynx Exp $ // vim:syntax=lpc:ts=8
//
#ifndef _INCLUDE_TRUST_H
#define _INCLUDE_TRUST_H

// all of these need new names to fit the tuning masterplan
#ifndef MAX_EXPOSE_GROUPS
# define MAX_EXPOSE_GROUPS      4       // just the top four  ;)
#endif
#ifndef DEFAULT_EXPOSE_GROUPS           // change this value in your local.h
# define DEFAULT_EXPOSE_GROUPS  4       // if you want groups to be exposed by
#endif                                  // default like on irc
#ifndef MAX_EXPOSE_FRIENDS
# define MAX_EXPOSE_FRIENDS     8       // just the top eight  ;)
#endif
#ifndef DEFAULT_EXPOSE_FRIENDS
# define DEFAULT_EXPOSE_FRIENDS 4       // do not expose yet, until we have
					// profiles worth exposing for
#endif
#define TRUST_OVER_NOTIFY       3       // how much /trust counts more
					// than notify. the normal value for
					// a notify friendship is 8. if a
					// medium trust is equivalent to
					// that, 3 needs to be added to
					// trust 5 to reach notify 8.
#define TRUST_MYSELF            (9 + TRUST_OVER_NOTIFY)
					// maximum trust
#ifndef EXPOSE_THRESHOLD
# define EXPOSE_THRESHOLD       TRUST_MYSELF // at least show it to myself
#endif

#endif
