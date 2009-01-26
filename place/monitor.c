#define ALLOW_EXTERNAL
#define SILENT
#define PRIVATE

// if you don't want your users to be able to enter this and
// receive monitor reports, you have to restrict entry here
#include <net.h>
#ifndef ADMINISTRATORS
# ifdef CONFIG_PATH
#  include CONFIG_PATH "admins.h"
# endif
#endif
#ifdef ADMINISTRATORS
# define PLACE_OWNED ADMINISTRATORS
# define RESTRICTED
#else
# echo Warning: Monitor room is not restricted.
#endif

#if 0
#define ON_ANY \
	D(S("monitor catch: %O got %O from %O\n", ME, mc, source));	\
	if (abbrev("_error", mc) || abbrev("_failure", mc)	\
				 || abbrev("_warning", mc)) {	\
		D(S("monitor1: %O got %O from %O\n", ME, mc, source));	\
		return 0;			\
	}
#endif

#include <place.gen>

error(source, mc, data, vars) {
	//D("monitor error handling 2 got called\n");
	unless (source == ME) castmsg(source, mc, data, vars);
	//return ::error(source, mc, data, vars);
}

cmd(a, args, b, source) {
	if (b && a == "report") {
		monitor_report("_warning_monitor_report_test",
		     S("%O is testing monitor_report: %O",
			source || previous_object(), args));
		return 1;
	}
	return ::cmd(a, args, b, source);
}
