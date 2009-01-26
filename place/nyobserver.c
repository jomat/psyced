#include <net.h>

#define NAME "NYObserver"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS "http://www.observer.com/index.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
