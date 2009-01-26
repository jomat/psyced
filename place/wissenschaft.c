#include <net.h>
#define	NAME	"Wissenschaft" // necessary for CONNECT_DEFAULT
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS	"http://rss.orf.at/science.xml"
//# define NEWSFEED_RSS	"http://shortnews.stern.de/rss/" NAME ".xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
