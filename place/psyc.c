#include <net.h>

#define NAME "PSYC"
#define SILENT
#define PLACE_HISTORY_EXPORT
//#define HISTORY
#define HISTORY_METHOD	"_notice_update"
#define HISTORY_GLIMPSE 4

#ifdef BRAIN
//# define ON_ANY if (mayLog(mc)) mymsg(source, mc, data, vars);
//# define ALLOW_EXTERNAL_FROM	"psyc://fly.symlyn"
//# define ALLOW_EXTERNAL
#else
//# define CONNECT_DEFAULT
# define REDIRECT "psyc://psyced.org/@welcome"
#endif

#include <place.gen>

#ifdef BRAIN
// it was a bad idea anyway.. you don't wanna autorelease thru cvs..
#if 0
mymsg(source, mc, data, vars) {
	if (vars["_module"] == "psyconaut"
	    && strstr(vars["_files"], "psyconaut.exe") != -1) {
		vars["_origin"] = source || vars["_INTERNAL_source"];
		P0(("%O forwarding psyconaut %O\n", ME, vars))
		sendmsg("psyc://psyced.org/@psyconaut-release", mc,
		    data, vars);
	}
}
#endif

qAllowExternal(source, mc, vars) {
	P3(("qAllowExternal: %O,%O,%O\n", source,mc,vars))
	unless (stringp(source)) return 0;
	if (abbrev( "psyc://213.73.91.20:" , source)) return 1;
	if (abbrev( "psyc://fly.symlynx.com:" , lower_case(source))) return 1;
	return 0;
}
#endif
