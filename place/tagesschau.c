#define NAME "tagesschau"
#define SILENCE
#include <net.h>

#ifdef BRAIN
# define NEWSFEED_RSS	"http://www.tagesschau.de/newsticker.rdf"
	// the tagesschau newsticker occasionally has broken links
	// so we add a filter that skips those
# define NEWS_PUBLISH(link, headline, channel)	(strstr(link, ",,") != -1)
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
