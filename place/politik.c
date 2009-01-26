#include <net.h>
#define NAME           "Politik"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS  "http://shortnews.stern.de/rss/Politik.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
