#include <net.h>
#define NAME		"Freizeit"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS	"http://shortnews.stern.de/rss/Freizeit.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
