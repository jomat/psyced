#include <net.h>
#include <text.h>
#include <person.h>

htget(prot, query, headers, qs) {
	string nick, t;
	object user;

	if (nick = query["user"]) user = find_person(nick);
	unless (user) {
		// should be a different mc here..
		t = "_error_invalid_authentication_token";
	} else unless (user->validToken(query["token"])) {
		t = "_error_invalid_authentication_token";
	} else {
		PT(("replacing cookie %O\n", headers["cookie"]))
		htok3(prot, 0, "Set-Cookie: psyced=\""+ qs +"\";\n");
		t = "_PAGES_login";
	}
	htok3(prot, 0, "Expires: 0\n");
	localize(query["lang"], "html");
	w("_HTML_head");
	w(t);
	w("_HTML_tail");
	return 1;
}
