// $Id: classic.i,v 1.28 2007/07/20 16:42:27 lynx Exp $ // vim:syntax=lpc:ts=8
//
// this used to be the place for lpmud master object code which hasn't
// been changed for psyced purposes. but everything has changed.

#include DRIVER_PATH "include/driver.h"
#include DRIVER_PATH "sys/wizlist.h"
// #include DRIVER_PATH "sys/functionlist.h"
#include DRIVER_PATH "sys/erq.h"

//#ifdef __COMPAT_MODE__
//# define COMPAT_FLAG
//#endif

#if !defined(COMPAT_FLAG) && !defined(NO_NATIVE_MODE) && \
    !__EFUN_DEFINED__(creator) && __EFUN_DEFINED__(geteuid)
#define NATIVE_MODE
#echo Driver compiled in historically so-called native mode. Bad.
#endif

/*
 * This is the LPmud master object, used from version 3.0.
 * It is the second object loaded after void.c.
 * Everything written with 'write()' at startup will be printed on
 * stdout.
 * 1. reset() will be called first.
 * 2. flag() will be called once for every argument to the flag -f
 * 	supplied to 'parse'.
 * 3. epilog() will be called.
 * 4. The system will enter multiuser mode, and enable log in.
 */

/* amylaar: allow to change simul_efun_file without editing patchlevel.h */
#ifndef SIMUL_EFUN_FILE
#echo master/classic.i: encountered undefined SIMUL_EFUN_FILE
#define SIMUL_EFUN_FILE "obj/library"
#endif

//#ifndef SPARE_SIMUL_EFUN_FILE
//#define SPARE_SIMUL_EFUN_FILE "obj/spare_library"
//#endif
/*
 * Give a path to a simul_efun file. Observe that it is a string returned,
 * not an object. But the object has to be loaded here. Return 0 if this
 * feature isn't wanted.
 */
string get_simul_efun() {
    string fname;
    string error;

    fname = SIMUL_EFUN_FILE ;
    if (error = catch(fname->start_simul_efun())) {
	write("Failed to load " + fname + "\n");
	write("error message :"+error+"\n");
#ifdef SPARE_SIMUL_EFUN_FILE
	fname = SPARE_SIMUL_EFUN_FILE;
	if (error = catch(fname->start_simul_efun())) {
	    write("Failed to load " + fname + "\n");
	    write("error message :"+error+"\n");
	    shutdown();
	    return 0;
	}
#endif
    }
    P2(("System library: %s\n", fname))
    return fname;
}

void dangling_lfun_closure() {
    raise_error("dangling lfun closure\n");
}

