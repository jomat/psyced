#if 0
#include <net.h>
#define SILENCE
#define NAME "Spiegel-EN"
#define TITLE "Spiegel International"
#define DESCRIPTION "Europe's Largest News Magazine in English Edition"

#ifdef BRAIN
# define NEWSFEED_RSS	"http://www.spiegel.de/international/index.rss"
# define RESET_INTERVAL	10 // they suggest 5 minutes
#else
# define CONNECT_DEFAULT
#endif
#endif

#include <place.gen>
