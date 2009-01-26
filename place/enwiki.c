#include <net.h>
#define NAME "enWiki"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS	"http://feeds.feedburner.com/WikinewsLatestNews"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
