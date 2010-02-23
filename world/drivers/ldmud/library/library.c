// $Id: library.c,v 1.43 2007/10/08 07:27:59 lynx Exp $ // vim:syntax=lpc:ts=8
//
#ifndef __PIKE__

#include "/local/config.h"
#include "/local/hosts.h"
//#include NET_PATH "include/proto.h"
#include NET_PATH "include/net.h"
#include DRIVER_PATH "include/driver.h"
#include <sandbox.h>

#include <driver.h>

#ifdef PRO_PATH
inherit PRO_PATH "library2";
inherit PRO_PATH "http/library2";
#else
# ifdef SANDBOX
inherit NET_PATH "library/sandbox";
# endif
inherit NET_PATH "library/base64";
inherit NET_PATH "library/hmac";
inherit NET_PATH "library/dns";
inherit NET_PATH "library/htbasics";
inherit NET_PATH "library/json";
inherit NET_PATH "library/profiles";
# ifdef JABBER_PATH // supposed to change
inherit NET_PATH "library/sasl";
# endif
inherit NET_PATH "library/share";
inherit NET_PATH "library/signature";
# ifdef __TLS__
inherit NET_PATH "library/tls";
# endif
inherit NET_PATH "library/text";
inherit NET_PATH "library/time";
inherit NET_PATH "library/uniform";
#endif

#endif //PIKE

// the system master object 
volatile object master;

volatile int shutdown_in_progress = 0;

volatile string logpath;

#ifndef __PIKE__

#ifdef MUD
# include "/include/auto.h"
# include "/sys/library.c"
#endif

#include NET_PATH "library/admin.c"
#ifndef PRO_PATH
# include NET_PATH "library/legal.c"
#endif

#endif //PIKE

// added sprintf-support	-lynx
//
// if the driver has no varargs support,
// "varargs" should be defined as empty string (see interface.h)
//
#ifdef varargs
void log_file(string file,string str,
    vamixed a,vamixed b,vamixed c,vamixed d,
    vamixed e,vamixed f,vamixed g,vamixed h)
#else
void log_file(string file, string str, varargs mixed* args)	// proto.h!
#endif
{
#if 0 //def COMPAT_FLAG
//  if (sizeof(regexp(({file}), "/")) || file[0] == '.' || strlen(file) > 30 )
    if (file[0] == '/' || strstr(file, "..") >= 0) {
        write("Illegal file name to log_file("+file+")\n");
        return;
    }
#endif
    PROTECT("LOG_FILE")
#ifdef varargs
    if (a) str = sprintf(str, a,b,c,d,e,f,g,h);
#else
    if (args && sizeof(args)) str = apply(#'sprintf, str, args);
#endif
#if defined(MUD) && __EFUN_DEFINED__(set_this_object)
    // we don't need this type of security in a regular psyced
    if (previous_object()) set_this_object(previous_object());
#endif
#ifdef SLAVE
    unless (logpath) {
	logpath = "/log/"+ __HOST_IP_NUMBER__ +"-"+ query_udp_port() +"/";
	mkdir(logpath);
	mkdir(logpath + "place");
    }
    write_file(logpath + file +".log", str);
#else
    write_file("/log/"+ file +".log", str);
#endif
    D3( debug_message(file +"## "+ str) );
//#ifdef PSYC_SYNCHRONIZE
//    synchro_report("_notice_system_psyced", str,
//	([ "_file": file ]) );
//#endif
}

#ifndef __PIKE__

#include "classic.i"
#ifdef USE_LIVING
# include "living.i"
#endif

#include NET_PATH "library.i"

#ifdef PRO_PATH
# include PRO_PATH "library.i"
#else
# include NET_PATH "library/library2.i"
#endif

#endif //PIKE
