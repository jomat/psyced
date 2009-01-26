// $Id: profile.c,v 1.2 2007/04/11 13:48:57 lynx Exp $ vim:syntax=lpc
//
// <kuchn> profile.c, a web based user settings/profile changer,
//	   called from net/http/server.
//
// TODO: please make use of the new convert_profile(vars, 0, "set") and
// convert_profile(settings, "set", 0) to convert a mapping of settings
// into a mapping of PSYC variables and back. this should make a lot of
// code in here unnecessary. avoiding replication is g00000d.
//
// i'm unhappy with the way of authing.. first i thought this could be
// inherited somehow to be a part of the user object, but thats schmarn.
// but i think psyc auth would be a nice way here..
// 	using checkPassword() the way you do is fine! we are not planning
// 	to use a web-based editor for remote PSYC items.. and the
// 	asynchronicity of it isn't something HTTP can easily handle.
// 	HTTP isn't as cool as PSYC you know? hahahahahahahahahahah

// as long as this is in development it could cause security breaches
// in production servers. so please only use this on experimental servers.
#ifdef EXPERIMENTAL

#include <net.h>
#include <text.h>
#include <person.h>

object user;

create() {
    sTextPath(0, 0, "html");
}

htget(prot, query, headers, qs) {
    htok3(prot, "text/html", "Cache-Control: no-cache\n");
	w("_PAGES_user_header");

	auth(prot, query, headers, qs);
}

checkAuth(val, prot, query, headers, qs, user) {
	if(! val)
	{
		w("_PAGES_user_login_failed");
		return;
	}

	w("_PAGES_user_header");

	switch(query["action"]) {
	case "settings":
		if(query["set"] == "1")
			settings(prot, query, headers, qs, user);
		else
			w("_PAGES_user_settings_body", ([
				"_username" : query["username"],
				"_password" : query["password"],
				
				"_speakaction" : user->v("speakaction") ? user->v("speakaction") : "",
				"_commandcharacter" : user->v("commandcharacter") ? user->v("commandcharacter") : ""
			]) );
		break;
	case "profile":
		if(query["set"] == "1")
			profile(prot, query, headers, qs, user);
		else
			w("_PAGES_user_profile_body", ([
				"_username" : query["username"],
				"_password" : query["password"],

				"_me" : user->v("me") ? user->v("me") : "",
				"_publicpage" : user->v("publicpage") ?  user->v("publicpage") : "",
				"_publictext" : user->v("publictext") ? user->v("publictext") : "",
				"_publicname" : user->v("publicname") ? user->v("publicname") : "",
				"_animalfave" : user->v("animalfave") ? user->v("animalfave") : "",
				"_popstarfave" : user->v("popstarfave") ? user->v("popstarfave") : "",
				"_musicfave" : user->v("musicfave") ? user->v("musicfave") : "",
				"_privatetext" : user->v("privatetext") ? user->v("privatetext") : "",
				"_likestext" : user->v("likestext") ? user->v("likestext") : "",
				"_dislikestext" : user->v("dislikestext") ? user->v("dislikestext") : "",
				"_privatepage" : user->v("privatepage") ? user->v("privatepage") : "",
				"_email" : user->v("email") ? user->v("email") : "",
				"_color" : user->v("color") ? user->v("color") : "",
				"_language" : user->v("language") ? user->v("language") : "",
				"_telephone" : user->v("telephone") ? user->v("telephone") : ""
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

	user = summon_person(query["username"]);
	if(!user || user->isNewbie())
	{
		w("_PAGES_user_login_notregistered");
		return 0;
	}

	int ok = 1;
	user->checkPassword(query["password"], "plain", "", "", #'checkAuth, prot, query, headers, qs, user);
}

settings(prot, query, headers, qs, user) {
	foreach(string k, string v : query) {
		int ok = 0;
		
		switch(k) {
		case "password":
			ok = 1;
			break;
		case "speakaction":
			ok = 1;
			break;
		case "commandcharacter":
			ok = 1;
			break;
		}
		
		if(! ok)
			continue;
		
		if(v == "")
		{
			//if(v != "password")
			//	user->vDel(k);
			// do nothing? maybe every setting is needed.. then this would be bad.
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
			ok = 1;
			break;
		case "publicpage":
			ok = 1;
			break;
		case "publictext":
			ok = 1;
			break;
		case "publicname":
			ok = 1;
			break;
		case "animalfave":
			ok = 1;
			break;
		case "popstarfave":
			ok = 1;
			break;
		case "musicfave":
			ok = 1;
			break;
		case "privatetext":
			ok = 1;
			break;
		case "likestext":
			ok = 1;
			break;
		case "dislikestext":
			ok = 1;
			break;
		case "privatepage":
			ok = 1;
			break;
		case "email":
			ok = 1;
			break;
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

#endif
