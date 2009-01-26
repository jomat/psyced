// $Id: user.c,v 1.12 2007/09/22 09:35:56 lynx Exp $ // vim:syntax=lpc
#include <net.h>
#include <user.h>

input(a, dest) {
	next_input_to(#'input);
	return ::input(a, dest);
}

logon() {
	vSet("scheme", "applet");
//	vDel("layout");

	next_input_to(#'input);
	return ::logon();
}

msg(source, mc, data, mapping vars, showingLog) {
	if (abbrev("_notice_place", mc))  {
// this ifdef is no longer necessary since /silence does the job
#ifndef EVENTS
		// especially for the applet:
		//	generate the +/- messages for ppl joining/leaving
		//
		if (abbrev("_notice_place_leave", mc))
		    emit("|-"+ vars["_nick"] +"\n");
		else if (abbrev("_notice_place_enter", mc))
		    emit("|+"+ vars["_nick"] +"\n");

		// and don't generate the traditional blurb ?
		//return;
#else
	//	if (!boss(ME)) return;
#endif
	}
	return ::msg(source, mc, data, vars, showingLog);
}

// print function using textdb and VECP-methods
pr(mc, fmt, a,b,c,d,e,f,g,h,i,j,k) {
	P3(("pr(%O,%O,%O,%O,%O..)\n", mc, fmt, a,b,c))
	if (mc) {
		unless (fmt = T(mc, fmt)) return;
#ifndef EVENTS
		if (abbrev("_echo_place_leave", mc)) {
			clearRoom();
			// warum eigentlich nur diese beiden?
			if (abbrev("_echo_place_leave_navigate", mc) ||
			    abbrev("_echo_place_leave_teleport", mc)) return;
			// weil sonst die logout-meldung den bach runtergeht
		}
#endif
		if (!(abbrev("_message", mc)
# ifdef PREFIXES
		       	|| abbrev("_prefix", mc)
# endif
			)) {

			if (abbrev("_list", mc) || abbrev("_echo", mc))
			    fmt = "|* "+ fmt;
			else {
				if (abbrev("_query", mc))
				    fmt = "|?"+ fmt;
				else
				    fmt = "|!"+ fmt;	// errors and such
			}
		}
	}
	return emit(sprintf(fmt, a,b,c,d,e,f,g,h,i,j,k));
}

#ifndef EVENTS
showRoom() {
	array(string) list;
	int i;

	unless (place) return 0;
	list = place -> names();
	p("|=%s\n", list ? implode(list, ", ") : "");
	return 1;
}

clearRoom() {
//	array(string) list;
//	int i;

//	unless (place) return 0;
//	list = place -> names();
//	for (i = sizeof(list)-1; i>=0; i--) emit("|-"+ list[i] +"\n");
	write("|=\n");
	return 1;
}
#endif
