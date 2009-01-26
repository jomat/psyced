#include <net.h>
#define NAME "Golem"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS	"http://www.golem.de/golem_backend.rdf"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>

