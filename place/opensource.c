#include <net.h>
#define SILENCE

#define	NAME		"OpenSource"

#ifdef BRAIN
# define NEWSFEED_RSS	"http://www.openbc.com/generated/rss/obc_de_net43-rssfeed0.91.xml"
# define RESET_INTERVAL	5*60	// 5 hours
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
