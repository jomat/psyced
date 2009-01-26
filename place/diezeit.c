#include <net.h>
#define SILENCE
#define NAME "DieZeit"

#ifdef BRAIN
# define NEWSFEED_RSS "http://newsfeed.zeit.de/index"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
