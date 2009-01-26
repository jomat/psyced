#include <net.h>
#define NAME		"Wirtschaft"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS	"http://shortnews.stern.de/rss/Wirtschaft.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
