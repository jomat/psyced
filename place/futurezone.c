#include <net.h>
#define SILENCE
#define NAME "futureZone"

#ifdef BRAIN
# define NEWSFEED_RSS	"http://rss.orf.at/futurezone.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
