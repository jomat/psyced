#include <net.h>

#define NAME "BBCvideo"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS	"http://newsrss.bbc.co.uk/rss/newsplayer_uk_edition/front_page/rss.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
