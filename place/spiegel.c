#include <net.h>
#define SILENCE
#define NAME "Spiegel"

#ifdef BRAIN
# define NEWSFEED_RSS	"http://www.spiegel.de/schlagzeilen/index.rss"
# define RESET_INTERVAL	5 // they suggest 5 minutes
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
