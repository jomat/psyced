#include <net.h>
#define NAME	"Brennpunkte"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS	"http://shortnews.stern.de/rss/Brennpunkte.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
