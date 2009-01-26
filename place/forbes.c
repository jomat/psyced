#include <net.h>

#define NAME "Forbes"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS	"http://www.forbes.com/feeds/mostemailed.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
