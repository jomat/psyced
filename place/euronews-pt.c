#include <net.h>
#define SILENCE
#define NAME "EuroNews-PT"

#ifdef BRAIN
# define NEWSFEED_RSS	"http://www.euronews.net/rss/euronews_po.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
