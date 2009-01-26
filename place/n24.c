#include <net.h>
#define SILENCE
#define NAME "N24"

#ifdef BRAIN
# define NEWSFEED_RSS "http://www.n24.de/rss/?rubrik=home"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
