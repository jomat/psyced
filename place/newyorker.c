#include <net.h>

#define NAME "NewYorker"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS "http://feeds.newyorker.com/services/rss/feeds/online.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
