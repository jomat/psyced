#include <net.h>
#define NAME		"Gesundheit"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS	"http://shortnews.stern.de/rss/Gesundheit.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
