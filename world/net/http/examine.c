#include <ht/http.h>
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
		user->parsecmd(t, query["dest"]);	// htcmd?
		t = "_echo_execute_web";
	} else if (t = user->htDescription(0, query, headers, qs, "")) {
		P4(("result: %O\n", t))
		// this is the thing!!
		write(t);
		return 1;
	}
	// show error message
	localize(query["lang"], "html");
	w("_HTML_head");
	w(t || "_failure_unavailable_description");
	w("_HTML_tail");
	return 1;
}
