#include <net.h>
#define SILENCE
#define NAME "EuroNews-EN"

#ifdef BRAIN
# define NEWSFEED_RSS	"http://www.euronews.net/rss/euronews_en.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
