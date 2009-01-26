#include <net.h>

#define NAME "AlJazeera"
#define SILENCE

#ifdef BRAIN
# define NEWSFEED_RSS "http://english.aljazeera.net/NR/exeres/4D6139CD-6BB5-438A-8F33-96A7F25F40AF.htm?ArticleGuid=55ABE840-AC30-41D2-BDC9-06BBE2A36665"
#else
# define CONNECT_DEFAULT
#endif

#include <place.gen>
