//
// <kuchn> additional user modules putted into net/user/ (initial revision):
// 	configure.c, a web based user settings/profile changer, called by net/http
//
// i'm unhappy with the way of authing.. first i thought this could be inheriated somehow to be a part of the user object, but thats schmarn.
// but i think psyc auth would be a nice way here..
//
#include <ht/http.h>
#include <net.h>
#include <text.h>
#include <person.h>

create() {
    sTextPath(0, 0, "html");
}

htget(prot, query, headers, qs) {
    htok3(prot, "text/html", "Cache-Control: no-cache\n");
	w("_PAGES_user_header");

	unless(auth(prot, query, headers, qs))
	{
		w("_PAGES_user_footer");
		write("PROB");
	}
}

checkAuth(val, prot, query, headers, qs, user) {
	unless(val) {
		w("_PAGES_user_login_failed");
		w("_PAGES_user_footer");
		return;
	}

	switch(query["action"]) {
	case "settings":
		if(query["set"] == "1")
			settings(prot, query, headers, qs, user);
		else
			w("_PAGES_user_settings_body", ([
				"_username" : query["username"],
				"_password" : query["password"],
				
				"_speakaction" : user->vQuery("speakaction") || "",
				"_commandcharacter" : user->vQuery("commandcharacter") || ""
			]) );
		break;
	case "profile":
		if(query["set"] == "1")
			profile(prot, query, headers, qs, user);
		else
			w("_PAGES_user_profile_body", ([
				"_username" : query["username"],
				"_password" : query["password"],

				"_me" : user->vQuery("me") || "",
				"_publicpage" : user->vQuery("publicpage") || "",
				"_publictext" : user->vQuery("publictext") || "",
				"_publicname" : user->vQuery("publicname") || "",
				"_animalfave" : user->vQuery("animalfave") || "",
				"_popstarfave" : user->vQuery("popstarfave") || "",
				"_musicfave" : user->vQuery("musicfave") || "",
				"_privatetext" : user->vQuery("privatetext") || "",
				"_likestext" : user->vQuery("likestext") || "",
				"_dislikestext" : user->vQuery("dislikestext") || "",
				"_privatepage" : user->vQuery("privatepage") || "",
				"_email" : user->vQuery("email")  || "",
				"_color" : user->vQuery("color") || "",
				"_language" : user->vQuery("language")  || "",
				"_telephone" : user->vQuery("telephone")  || ""
			]) );
		break;
	default:
		w("_PAGES_user_index", ([ "_username" : query["username"], "_password" : query["password"] ]));
		break;
	}
	
	w("_PAGES_user_footer");
}

auth(prot, query, headers, qs) {
	if(! stringp(query["username"]) || ! stringp(query["password"]))
	{
		w("_PAGES_user_login_body");
		return 0;
	}
	
	if(query["username"] == "" || query["password"] == "")
	{
		w("_PAGES_user_login_empty");
		return 0;
	}

	object user = summon_person(query["username"]);
	if(! user || user->isNewbie())
	{
		w("_PAGES_user_login_notregistered");
		return 0;
	}

	user->checkPassword(query["password"], "plain", "", "", #'checkAuth, prot, query, headers, qs, user);
	return 1;
}

settings(prot, query, headers, qs, user) {
	foreach(string k, string v : query) {
		int ok = 0;

		switch(k) {
		case "password":
		case "speakaction":
		case "commandcharacter":
			ok = 1;
			break;
		}
		
		if(! ok)
			continue;

		if(v == "")
		{
			if(k == "speakaction")
				user->vDel(k);
		}
		else
			user->vSet(k, v);	// password won't be set? humm.
	}
	
	w("_PAGES_user_settings_changed", ([ "_username" : query["username"], "_password" : query["password"] ]) );
}

profile(prot, query, headers, qs, user) {
	foreach(string k, string v : query) {
		int ok = 0;

		switch(k)
		{
		case "me":
		case "publicpage":
		case "publictext":
		case "publicname":
		case "animalfave":
		case "popstarfave":
		case "musicfave":
		case "privatetext":
		case "likestext":
		case "dislikestext":
		case "privatepage":
		case "email":
		case "telephone":
			ok = 1;
			break;
		}

		if(! ok)
			continue;

		if(v == "")
			user->vDel(k);
		else
			user->vSet(k, v);
	}

	w("_PAGES_user_profile_changed", ([ "_username" : query["username"], "_password" : query["password"] ]) );
}

w(mc, vars) { write(psyctext(T(mc, ""), vars)); }
