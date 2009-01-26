#include <net.h>

#define NAME "CNNvideo"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS	"http://rss.cnn.com/rss/cnn_freevideo.rss"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
