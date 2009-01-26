#include <net.h>
#define	NAME	"Auto"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS	"http://shortnews.stern.de/rss/Auto.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
