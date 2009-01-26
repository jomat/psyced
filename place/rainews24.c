#include <net.h>

#define NAME "RAInews24"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS "http://www.rainews24.it/ran24/rainews24_2007/RSS/ultime.asp"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
