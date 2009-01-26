// this code contributed from symlynX webchat. currently not in use.

#include <net.h>
#include <text.h>
#include <ht/html.h>
#include <ht/http.h>
#include <person.h>

volatile string nick;
volatile mixed user;

htget(prot, query, headers, qs) {
	string key, value, k;
	int okay;

	P2(("\n\nQuery: %O\n", query))
	nick = query["user"];
        localize(query["lang"], "ht");

	htok(prot);
	write (hthead("modifying "+nick+"'s settings") + htfs_on "<th>");
	unless (nick) {
		write( T("_PAGES_register_nickless", "Who are you?")
		     + htfs_off);
		return 1;
	}
	unless (user = find_person(nick)) {
		write( T("_PAGES_register_offline",
		 	"You're not in the chat?<br>Please enter it.")
		     + T("_HTML_back", "") + htfs_off );
		return 1;
	}
	unless (user -> validToken(query["token"])) {
		write( T("_error_invalid_authentication_token",
			    "Invalid token. Please log in anew.")
		     + htfs_off);
		return 1;
        }
	okay = 1;

	// endlich mal hier verallgemeinern. damn!  TODO
#ifdef NEWSTICKER
	user->subscribe(member(query, "sub-events") ? SUBSCRIBE_TEMPORARY : SUBSCRIBE_NOT, "events", 1);
	user->subscribe(member(query, "sub-events-weekly") ? SUBSCRIBE_TEMPORARY : SUBSCRIBE_NOT, "events-weekly", 1);
#endif
	// sonderfall wegen logischer inversion  :(
	key = "visibility";
	user -> vSet(key, query[key] || "off");

	unless (query["colors"]) user -> vDel("ignorecolors");
	unless (query["filter"]) user -> vDel("filter");

	// statt einzelauswertung sollte man einen _request_store
	// erzeugen ähnlich zu den vCards die jabber clients
	// schicken.
	//
	foreach (key : query) switch(k = lower_case(key)) {
	case "sub-poldi":
	case "sub-events":
	case "sub-events-weekly":
	case "sub-dpa-PL":
	case "sub-dpa-WI":
	case "sub-dpa-VM":
	case "opass":
	case "pass":
	case "pass2":
	case "token":
	case "layout":
	case "user":
	case "lang":	// should this be renamed to "language" ??
	case "x":
	case "y":
	case "visibility":
		break;
	case "me":
		value = query[key];
		if (!value || value == "" || value == "-")
		     user->vDel(k);
		else user->vSet(k, value);
		break;
	default:
//		if (trail("text", k)) {
//			// catch CR?
//		}
		value = query[key];
// D("checking ("+key+", "+value+")\n");
		if (user -> checkVar(&k, &value)) {
// D("valid as "+value+"\n");
			if (!value || value == "" || value == "-")
			     user->vDel(k);
			else user->vSet(k, value);
		} else okay = 0;
	}
#ifndef VOLATILE
	okay = okay && changePassword(query);
#endif
	if (okay) {
#ifndef VOLATILE
		user -> save();
#else
		log_file("NEWSLETTER", "[%s] %O %O\n\n",
			 ctime(), query_ip_name(), query);
#endif
		write( T("_PAGES_edit_stored",
			    "Settings successfully stored.")
		     + T("_HTML_back", "") + htfs_off );
	} else
	     write( T("_HTML_back", "") + htfs_off );
	return 1;
}

#ifndef VOLATILE
static changePassword(query) {
	string pw;
	string* t;

	pw = query["pass"];
	unless (pw && pw != "") return 1;

	if (t = legal_password(pw, nick)) {
		write( T(t[0],t[1]) );
		return 0;
	}
	if (pw != query["pass2"]) {
		write( T("_error_invalid_password_repetition",
  "Sorry, but the repetition does not match the password.<br>Please check.") );
		return 0;
	}
	if (user ->checkPassword(query["opass"])) {
		string token;

		user->vSet("password", pw);
	} else {
		printf( T("_error_invalid_password",
		  "You provided an invalid password, %s."), nick );
		return 0;
	}
	return 1;
}
#endif
