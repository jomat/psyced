#define _INCLUDE_DRIVER_H

// for debug outputs
#define DRIVER_TYPE	"amylaar"

// amylaar has closures
#define DRIVER_HAS_CLOSURES

// amylaar introduced those ugly but runtime-effective closures
#define DRIVER_HAS_LAMBDA_CLOSURES

// amylaar provides "compile_object" in master.c
#define DRIVER_HAS_RENAMED_CLONES
