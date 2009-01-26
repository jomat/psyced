#include <net.h>

#define NAME "TurkishWeekly"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS "http://www.turkishweekly.net/rss/jtw-news-national10.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
