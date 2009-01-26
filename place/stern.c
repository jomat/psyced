#include <net.h>
#define SILENCE
#define NAME "STERN"

#ifdef BRAIN
# define NEWSFEED_RSS	"http://www.stern.de/standard/rss.php?channel=all"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
