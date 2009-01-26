// $Id: driver.h,v 1.10 2007/05/07 20:20:42 lynx Exp $ // vim:syntax=lpc:ts=8
#ifndef _INCLUDE_DRIVER_H
#define _INCLUDE_DRIVER_H

// for debug outputs
#ifdef __psyclpc__
# define DRIVER_TYPE	"psyclpc"
#else
# define DRIVER_TYPE	"ldmud"
#endif

// this driver has closures
#define DRIVER_HAS_CLOSURES

// amylaar-style runtime closures
#define DRIVER_HAS_LAMBDA_CLOSURES

// mudos-style readable closures
#define DRIVER_HAS_INLINE_CLOSURES

// amylaar provides "compile_object" in master.c
#define DRIVER_HAS_RENAMED_CLONES

// the function(&var) syntax
#define DRIVER_HAS_CALL_BY_REFERENCE

// macros to see if a protocol port is available
//#define HAS_PORT(PORT, PATH)	(defined(PATH) && defined(PORT) && PORT - 0)
#ifdef __TLS__
# define HAS_TLS_PORT(PORT)	(defined(PORT) && PORT - 0)
#else
# define HAS_TLS_PORT(PORT)	0
#endif

#ifdef SIMUL_EFUN_FILE
#undef SIMUL_EFUN_FILE
#endif
#define SIMUL_EFUN_FILE DRIVER_PATH "library/library.c"
//#ifndef SPARE_SIMUL_EFUN_FILE
//#define SPARE_SIMUL_EFUN_FILE "obj/spare_library"
//#endif

#endif
