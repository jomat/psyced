#include <net.h>

#define NAME "AlJazeera"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS "http://english.aljazeera.net/Services/Rss/?PostingId=2007731105943979989"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
