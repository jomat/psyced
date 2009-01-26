#include <net.h>

#define NAME "ForbesVideo"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS	"http://www.forbes.com/video/index.xml"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
