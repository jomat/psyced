#include <net.h>
#define	NAME		"enWikiRecent"

#ifdef BRAIN
// irc://irc.wikimedia.org/en.wikinews
# define	CONNECT_IRC	"irc.wikimedia.org"
# define	CHAT_CHANNEL	"en.wikinews"
#else
# define	CONNECT_DEFAULT
#endif

#include <place.gen>
