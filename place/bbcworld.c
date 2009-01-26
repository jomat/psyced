#include <net.h>

// unfortunately http://www.bbcworld.com/Pages/News.aspx?feedName=world
// doesn't exist as feed, so this is really BBC UK's world feed.
#define NAME "BBCworld"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS	"http://newsrss.bbc.co.uk/rss/newsplayer_uk_edition/world/rss.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
