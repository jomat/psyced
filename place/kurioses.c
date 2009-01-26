#include <net.h>
#define NAME		"Kurioses"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS	"http://shortnews.stern.de/rss/Kurioses.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
