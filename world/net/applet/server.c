// $Id: server.c,v 1.14 2007/09/27 11:01:04 lynx Exp $ // vim:syntax=lpc
#include <net.h>
#include <person.h>
#include <server.h>

volatile array(string) legal_vars = ({
	"password", "place", "agent", "layout", "language"
});
volatile mapping vars;

qScheme() { return "applet"; }

createUser(nick) {
	return named_clone(APPLET_PATH "user", nick);
}

keepUserObject(user) {
//	P1(("app: %O, %O\n",user, abbrev(APPLET_PATH "user", o2s(user))))
	return abbrev(APPLET_PATH "user", o2s(user));
	// return user->vQuery("scheme") == "applet";
}

// the following is new..

logon() {
	vars = ([]);
	return ::logon();
}

hello(in) {
	string vnam, vval;

	P2(("%O applet says: %O\n", ME, in))
	if (sscanf(in, "=_%s%t%s", vnam, vval)) {
		if (vval && strlen(vval)) {
//		    D2(D("applet says: '"+in+"'\n");)
		    if (vnam == "lang") vnam = "language";
		    if (index(legal_vars, vnam) != -1) { vars[vnam] = vval; }
		}
		next_input_to(#'hello);
		return 1;
	}
	if (member(vars, "language"))
	    sTextPath(vars["layout"] || qLayout(), vars["language"], qScheme());

	return ::hello(in);
}

promptForPassword() {
	// don't impose room on registered users
	vars = m_delete(vars, "place");

	if (member(vars, "password")) return password(vars["password"]);

//	pr("_query_password_person",
//	   "Please supply password for %s.\n", nick);
	return ::promptForPassword();
}

morph() {
	// don't overwrite password
	vars = m_delete(vars, "password");

	if (user) {
		user -> vDel("agent");	// is this really useful?
		if (vars) user -> vMerge(vars);
	}
	return ::morph();
}

pr(mc, fmt, a,b,c,d,e,f,g,h,i,j,k) {
	if (mc) {
		unless (fmt = T(mc, fmt)) return;
		if (abbrev("_query", mc)) fmt = "|?"+ fmt;
	}
	printf(fmt, a,b,c,d,e,f,g,h,i,j,k);
	return 1;
}
