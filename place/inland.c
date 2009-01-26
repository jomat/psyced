#include <net.h>
#define NAME		"Inland"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS	"http://shortnews.stern.de/rss/Inlandspolitik.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
