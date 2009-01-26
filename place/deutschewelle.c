#include <net.h>
#define SILENCE

#define NAME "deutschewelle"

#ifdef BRAIN
# define NEWSFEED_RSS	"http://rss.dw-world.de/rdf/rss-en-all"
# define RESET_INTERVAL	40  // minutes
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
