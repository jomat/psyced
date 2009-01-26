/* This shouldn't get called, the master should name its library.c directly */

#include <net.h>

#ifdef DRIVER_HAS_BROKEN_INCLUDE
# ifdef MUDOS
#  include "/drivers/mudos/library/library.c"
# else
#  include <library.c>
# endif
#else
# include DRIVER_PATH "library/library.c"
#endif

