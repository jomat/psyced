#include <net.h>
#define NAME "Lebenskunst"	// necessary for places that connect
#define SILENCE

#ifdef BRAIN
// # define NEWSFEED_RSS	"http://www.stern.de/standard/rss.php?channel=lifestyle"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
