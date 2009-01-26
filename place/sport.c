#include <net.h>
#define NAME           "Sport"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS  "http://shortnews.stern.de/rss/Sport.xml"
#else                                                                          +# define CONNECT_DEFAULT
#endif

#include <place.gen>
