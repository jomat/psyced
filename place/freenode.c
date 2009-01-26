#include <net.h>
#define NAME "freenode"

// 
// «freenode:TomSawyer» TomSawyer sagt Dir: you're able to talk to me, not because I'm an admin, but because I have /msg nickserv set unfiltered on
// «freenode:TomSawyer» TomSawyer sagt Dir: once you register, /msg nickserv set unfiltered on
// «freenode:TomSawyer» TomSawyer sagt Dir: that will ensure you are able to get messages from unregistered users
//
// ok weiss bescheid, aber ist eh netter wenn man unseren gate nur als
// registrierter user benutzen kann.. basst scho.. jdf gut zu wissen
//

#ifdef BRAIN
# echo BRAIN: connecting to freenode IRC server
//# define CONNECT_IRC	"irc." NAME ".net"
# define CONNECT_IRC	"calvino.freenode.net"
//# define CHAT_CHANNEL	"esp"
//# define CHAT_CHANNEL	"23c3"
# define PASS_IRC	IRCGATE_FREENODE
#else
# echo SLAVE: connecting to psyced.org for freenode gateway
# define CONNECT_DEFAULT
#endif

#include <place.gen>
