// we are currently experiencing some problems with this feed *shrug*
#if 0

#include <net.h>

#define SILENCE
#define NAME "Spiegel"

#ifdef BRAIN
# define NEWSFEED_RSS	"http://www.spiegel.de/schlagzeilen/index.rss"
# define RESET_INTERVAL	5 // they suggest 5 minutes
#else
# define CONNECT_DEFAULT
#endif

#endif

#include <place.gen>
