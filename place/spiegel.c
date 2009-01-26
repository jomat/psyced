#include <net.h>
#define SILENCE
#define NAME "Spiegel"

#ifdef BRAIN
# define NEWSFEED_RSS	"http://www.spiegel.de/schlagzeilen/rss/0,5291,,00.xml" 
# define RESET_INTERVAL	5 // they suggest 5 minutes
#else
# define CONNECT_DEFAULT
#endif

/*
 * http://www.spiegel.de/dertag/0,1518,271804,00.html
 */
#include <place.gen>
