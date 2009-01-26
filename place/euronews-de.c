#include <net.h>
#define SILENCE
#define NAME "EuroNews-DE"

#ifdef BRAIN
# define NEWSFEED_RSS	"http://www.euronews.net/rss/euronews_ge.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
