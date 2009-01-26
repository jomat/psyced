#include <net.h>

#define NAME "DigDL"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS	"http://blogs.forbes.com/digitaldownload/index.rdf"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
