#include <net.h>

#define NAME "CNET"
#define SILENCE

/* this is an example for an RSS-based newsfeed room for PSYC
 * just copy it and enter your favourite RSS URL. you can also
 * specify a RESET_INTERVAL in minutes. please don't run your
 * own news gateway if one already exists - PSYC packets are
 * much much more efficient than polling RSS files, therefore
 * if you like getting cnet news, simply /subscribe or /enrol
 * to psyc://psyced.org/@cnet (until cnet catch the
 * drift and provide such a PSYC news service themselves ;))
 * see also http://rss.news.com/
 *
 * by the way, RSS is not the only newsfeed interface to PSYC -
 * in the perlpsyc distribution is an email filter script which
 * parses dpa news coming by email and creates PSYC notices out
 * of it. the best idea would obviously be if publishing tools
 * learned how to notify changes directly to a PSYC newsroom.
 * it's really simple.. just connect and dump a few lines!
 */
#ifdef BRAIN
# define NEWSFEED_RSS	"http://news.com.com/2547-1_3-0-5.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
