/* This shouldn't get called, the driver should call its master.c directly */

#ifdef MUDOS
# include <net.h>
# include <services.h>
#else
/* bei amylaar und ldmud braucht master.c den absoluten pfad.. */
# include "/local/config.h"
# include NET_PATH "include/net.h"
# include NET_PATH "include/services.h"
# include DRIVER_PATH "include/driver.h"
#endif

#ifdef DRIVER_HAS_BROKEN_INCLUDE
# ifdef MUDOS
#  include "/drivers/mudos/master/master.c"
# else
#  include <master.c>
# endif
#else
# include DRIVER_PATH "master/master.c"
#endif

