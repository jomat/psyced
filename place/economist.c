#include <net.h>

#define NAME "Economist"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS "http://www.economist.com/rss/news_analysis_and_views_rss.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
