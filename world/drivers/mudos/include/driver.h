#define _INCLUDE_DRIVER_H

// for debug outputs
#define DRIVER_TYPE	"MudOS"

// amylaar has closures
#define DRIVER_HAS_CLOSURES

// mudos provides "compile_object" in master.c, too!!
#define DRIVER_HAS_RENAMED_CLONES

// MudOS is unable to #include NET_PATH "something.i"
#define DRIVER_HAS_BROKEN_INCLUDE

// MudOS socket interface
#define DRIVER_HAS_SOCKET

