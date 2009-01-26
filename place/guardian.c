#include <net.h>
#define NAME "Guardian"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS	"http://www.guardian.co.uk/rss"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
