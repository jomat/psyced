// test for junction'd rooms

#include <net.h>

#define NAME	"tuXedo" // junction
#define PLACE_HISTORY
#define HISTORY_GLIMPSE 7

#ifdef SYMLYNX
# define PLACE_OWNED	ADMINISTRATORS
//# define MASTER
# echo tuXedo is a masterplace.
//# define ALLOW_EXTERNAL_FROM	"psyc://andrack.tobij.de"
#else
# define JUNCTION
# define CONNECT	"psyc://ve.symlynX.com/@" NAME
# echo tuXedo is a junction for ve.symlynX.com.
#endif

#include <place.gen>
