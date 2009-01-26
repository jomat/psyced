#include <net.h>
#define SILENCE
#define NAME "slashdot"

#ifdef BRAIN
# define NEWSFEED_RSS	"http://rss.slashdot.org/Slashdot/slashdot" 
# define RESET_INTERVAL	4 // that's quite often.. but that's slashdot style
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
