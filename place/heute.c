#define NAME "heute"
#define SILENCE

#include <net.h>
#ifdef BRAIN
//# define NEWSFEED_RSS	"http://bootleg-rss.g-blog.net/heute_t-online_de.php"
# define NEWSFEED_RSS	"http://www.heute.de/ZDFheute/inhalt/rss/20/0,6704,20,00.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
