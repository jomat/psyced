#include <net.h>

#define NAME "CNN"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS	"http://rss.cnn.com/rss/cnn_topstories.rss"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
