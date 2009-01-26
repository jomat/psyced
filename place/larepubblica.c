#include <net.h>
#define SILENCE
#define NAME "laRepubblica"

#ifdef BRAIN
# define NEWSFEED_RSS "http://www.repubblica.it/rss/homepage/rss2.0.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
