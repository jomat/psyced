// this place is configured in a suitable way for receiving
// syslog events from the syslog2psyc tool in perlpsyc
//
#include <net.h>

#ifdef ADMINISTRATORS
# define	PLACE_OWNED	ADMINISTRATORS
#else
# echo place/syslog has no owners!
#endif

#define NAME	"syslog"
#define PRIVATE
#define SECURE
#define RESTRICTED

#define PLACE_HISTORY_EXPORT
#define HISTORY_GLIMPSE 7
#define HISTORY_METHOD  "_notice_system"

#ifdef BRAIN
# define ALLOW_EXTERNAL_FROM	"psyc://psyced.org"
#else
# define ALLOW_EXTERNAL_FROM	"psyc://localhost"
#endif

#include <place.gen>
