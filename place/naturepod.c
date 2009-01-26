#include <net.h>

#define NAME	"NaturePod"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS	"http://www.nature.com/nature/podcast/rss/nature.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
