#include <net.h>

#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS	"http://www.nature.com/news/rss.rdf"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
