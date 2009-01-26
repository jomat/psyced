#include <net.h>
#define NAME           "Kultur"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS  "http://shortnews.stern.de/rss/Kultur.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
