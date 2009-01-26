#include <net.h>
#include <text.h>
#include <person.h>

htget(prot, query, headers, qs) {
	string nick, t;
	object user;

	htok3(prot, 0, "Expires: 0\n");
	if (nick = query["user"]) user = find_person(nick);
	unless (user) {
		// should be a different mc here..
		t = "_error_invalid_authentication_token";
	} else unless (user->validToken(query["token"])) {
		t = "_error_invalid_authentication_token";
	} else if ((t = query["cmd"]) && strlen(t)) {
		user->parsecmd(t);	// htcmd?
		t = "_echo_execute_web";
	} else if (t = user->htDescription(prot, query, headers, qs, "")) {
		P4(("result: %O\n", t))
		write(t);
		return 1;
	}
	localize(query["lang"], "html");
	w("_HTML_head");
	w(t || "_failure_unavailable_description");
	w("_HTML_tail");
	return 1;
}
