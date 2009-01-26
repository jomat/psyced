// $Id: driver.h,v 1.1 2007/09/16 20:02:12 lynx Exp $ // vim:syntax=lpc:ts=8
#ifndef _INCLUDE_DRIVER_H
#define _INCLUDE_DRIVER_H

// for debug outputs
#define DRIVER_TYPE	"pike"

// the following ifdefs are prototypical

// this driver has closures
#define DRIVER_HAS_CLOSURES

// amylaar-style runtime closures
#undef DRIVER_HAS_LAMBDA_CLOSURES

// mudos-style readable closures
#define DRIVER_HAS_INLINE_CLOSURES

// amylaar provides "compile_object" in master.c
#undef DRIVER_HAS_RENAMED_CLONES

// the function(&var) syntax
#define DRIVER_HAS_CALL_BY_REFERENCE

#endif
