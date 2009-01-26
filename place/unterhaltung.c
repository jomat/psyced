#include <net.h>
#define NAME "Unterhaltung" // necessary for CONNECT_DEFAULT
#define SILENCE

#ifdef BRAIN
// # define NEWSFEED_RSS	"http://www.stern.de/standard/rss.php?channel=unterhaltung"
# define NEWSFEED_RSS	"http://shortnews.stern.de/rss/Entertainment.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
