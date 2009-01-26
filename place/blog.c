// experimental: example of a blog type room
// which probably needs some work to get running
//
#include <net.h>

#define NAME "blog"
#define THREADS
#define HISTORY_GLIMPSE 12

#ifdef ADMINISTRATORS
    // psyconf puts ADMINISTRATORS into psyconf.h
# define PLACE_OWNED ADMINISTRATORS
#else
    // example set-up
# define PLACE_OWNED "fippo", "lynx", "bartman"
#endif

#define UNIFORM_STYLE "http://www.your-community.de/fippo/blog.css"
#include <place.gen>
