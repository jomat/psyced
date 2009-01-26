#include <net.h>
#define SILENCE
#define NAME "Oesterreich"

#ifdef BRAIN
# define NEWSFEED_RSS	"http://rss.orf.at/oesterreich.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
