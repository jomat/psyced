#include <net.h>
#define SILENCE
#define NAME "EuroNews-RU"

#ifdef BRAIN
# define NEWSFEED_RSS	"http://www.euronews.net/rss/euronews_ru.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
