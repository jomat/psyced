#include <net.h>
#define SILENCE
#define NAME "ZeitVideo"

#ifdef BRAIN
# define NEWSFEED_RSS "http://newsfeed.zeit.de/video/index"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>

#ifdef BRAIN

publish(link, headline, channel) {
	if (strstr(link, "&from=rss", -12) != -1)
	    link = link[0 .. <12];
	return ::publish(link, headline, channel);
}

#endif

