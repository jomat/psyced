#include <net.h>
#define SILENCE

#define	NAME		"minusvisionen"

#ifdef BRAIN
# define NEWSFEED_RSS	"http://www.minusvisionen.de/rss.xml" 
# define RESET_INTERVAL	17 * 60	    // many hours
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
