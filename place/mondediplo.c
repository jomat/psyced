#include <net.h>

#define NAME "MondeDiplo"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS "http://www.monde-diplomatique.fr/recents.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
