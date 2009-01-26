#include <net.h>
#define SILENCE
#define NAME "EuroNews-ES"

#ifdef BRAIN
# define NEWSFEED_RSS	"http://www.euronews.net/rss/euronews_sp.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
