#include <net.h>
#define SILENCE
#define NAME "EuroNews-IT"

#ifdef BRAIN
# define NEWSFEED_RSS	"http://www.euronews.net/rss/euronews_it.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
