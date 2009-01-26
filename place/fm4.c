#include <net.h>
#define SILENCE
#define NAME "FM4"

#ifdef BRAIN
# define NEWSFEED_RSS	"http://rss.orf.at/fm4.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
