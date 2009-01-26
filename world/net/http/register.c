// this code contributed from symlynX webchat. currently not in use.

#include <ht/http.h>
#include <net.h>
#include <text.h>
#include <person.h>

htget(prot, query, headers, qs) {
	string nick, pw, email, lang, layout;
	mixed user;
	array(string) t;

	if (mappingp(query)) {
		nick = query["user"];
		email = query["email"];
                pw = query["pass", 0];
		lang = query["lang", 0];
        }
        sTextPath(0, lang, "ht");

	htok(prot);
	unless (nick) {
		printf(hthead("register") + T("_PAGES_register_nickless",
			"Who are you?<br>"));
		return 1;
	}
	if (!pw || pw == "" && !email) {
		write( hthead("register") + psyctext(
		     T("_PAGES_register_form", 0), ([
			      "_nick": nick,
			"_FORM_start": "\
<form name=f method=GET action=\""+object2url(ME)+"\">\n\
<input type=hidden name=user value=\""+nick+"\">\n\
<input type=hidden name=lang value=\""+lang+"\">\n\
<input type=hidden name=layout value=\""+layout+"\">",
			"_EDIT_email": email || "",
	 "_EDIT_subscription_politik": "checked",
      "_EDIT_subscription_wirtschaft": "checked",
     "_EDIT_subscription_vermischtes": "checked",
			  "_FORM_end": "</form>",
		]) ));
		return 1;
	}
	if (t = legal_password(pw, nick)) {
		write(hthead("illegal password") +
		      T(t[0],t[1]) + T("_HTML_back", "") );
		return 1;
	}
	if (pw != query["pass2"]) {
		write(hthead("mistyped") +
		      T("_error_invalid_password_repetition",
  "Sorry, but the repetition does not match the password. Please check.") +
			    T("_HTML_back", "") );
		return 1;
	}
	unless (user = find_person(nick)) {
		write(hthead("register") +
		  T("_PAGES_register_offline",
		 	"You're not in the chat?<br>Please enter it.\n"));
		return 1;
	}
	if (user ->checkPassword()) {
		string token;

		user->vSet("password", pw);
		if (email) user->vSet("email", email);
#ifdef NEWSTICKER
		user->subscribe(!member(query, "sub-dpa-PL"), "dpa-PL", 1);
		user->subscribe(!member(query, "sub-dpa-WI"), "dpa-WI", 1);
		user->subscribe(!member(query, "sub-dpa-VM"), "dpa-VM", 1);
#endif
		return 1;
	}
	write( hthead("register") + T("_PAGES_register_taken",
		    "This nickname is already registered."));
	return 1;
}
