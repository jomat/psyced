// this code contributed from symlynX webchat. currently not in use.

#include <net.h>
#include <text.h>
#include <person.h>
#include <ht/http.h>
#include <url.h>

#define NO_INHERIT
#include <user.h>
#undef NO_INHERIT

// #define v(VAR)	user->vQuery(VAR)

htget(prot, query, headers, qs) {
	string nick, lang, layout, token;
	mixed user;
	mapping _v, sups;
	int newbie;

	P2(("edit query: %O\n", query))
	nick = query["user"];
	user = find_person(nick);
	token = query["token"];

	lang = query["lang"];
        sTextPath(0, lang, "ht");

	htok(prot);
	unless (nick && user && user->validToken(token)) {
		write( hthead("edit") +
		       T("_PAGES_edit_invalid", "Who are you?<br>"));
		return 1;
	}
	// unless (newbie = user->isNewbie()) _v = user->vMapping();
	_v = user->vMapping() || ([]);
	unless (lang) lang = v("language");

	//newbie = v("password") == 0;
	newbie = user->isNewbie();

	if (v("name")) nick = v("name");
	sups = v("subscriptions") || ([]);

#define FORM_HIDDEN "\n\
<input type=hidden name=user value=\""+nick+"\">\n\
<input type=hidden name=token value=\""+token+"\">\n\
<input type=hidden name=lang value=\""+lang+"\">\n\
<input type=hidden name=layout value=\""+layout+"\">"

#define FORM_OPTS " name=f method=GET action=\"/"+layout+"/modify\""

#ifdef VOLATILE
# define FORM_START "<form" FORM_OPTS
# define FORM_PAGE "_PAGES_register_newsletter"
#else
# define FORM_START "<form target=cout_" CHATNAME FORM_OPTS
# define FORM_PAGE newbie? "_PAGES_register_form": "_PAGES_edit_form"
#endif
	// here comes the most obvious demonstration
	// of the necessity of the psyctext parser  :)
	write(T("_SCRIPT_edit", 0)
	      + hthead(newbie? "register": "edit")
	      + psyctext(T(FORM_PAGE, 0), ([
//	"_FORM_start"			: FORM_START ">" FORM_HIDDEN,
//	"_FORM_start_check_email"	: FORM_START "\
 onSubmit=\"return checkEmail(f.email.value)\">" FORM_HIDDEN,
	"_FORM_start"			: FORM_START "\
 onSubmit=\"return check(f)\">" FORM_HIDDEN,
	"_FORM_end"			: "</form>",
	"_nick"				: nick,
	"_EDIT_nick"			: v("name") || nick,
	"_EDIT_nick_size"		: strlen(v("name") || nick),
	"_EDIT_email"			: v("email") || "",
#if 1
	"_EDIT_subscription_events_weekly"
		: member(sups,"events-weekly")? "checked": "",
	"_EDIT_subscription_politik_digital"
		: member(sups,"poldi")? "checked": "",
#ifdef VOLATILE
	"_EDIT_subscription_events"	: "checked",
#else
	"_EDIT_subscription_events"	: newbie||member(sups,"events")?
					    "checked": "",
# ifdef NEWSTICKER
	"_EDIT_subscription_politik"	: member(sups,"dpa-pl")? "checked": "",
	"_EDIT_subscription_wirtschaft"	: member(sups,"dpa-wi")? "checked": "",
	"_EDIT_subscription_vermischtes": member(sups,"dpa-vm")? "checked": "",
# endif
#endif
	"_EDIT_place_home"		: v("home") || "",
	"_EDIT_color"			: v("color") || "",
        // no longer up to date here..
        // might not behave exactly as intended
	"_EDIT_checkbox_filter"		: v("filter") == FILTER_STRANGERS ?
					    "checked" : "",
	"_EDIT_checkbox_colors_ignored"	: v("ignorecolors") ? "checked" : "",
	"_EDIT_checkbox_visibility"	: v("visibility") == "off" ?
					    "" : "checked",
	"_EDIT_action_speak"		: v("speakaction") || "",
	"_EDIT_action_motto"		: v("me") || "",
	"_EDIT_page_public"		: v("publicpage") || "",
	"_EDIT_page_private"		: v("privatepage") || "",
	"_EDIT_file_key_public"		: v("keyfile") || "",
	"_EDIT_description_private"	: v("privatetext") || "",
	"_EDIT_description_public"	: v("publictext") || "",
	"_EDIT_description_preferences"	: v("likestext") || "",
    "_EDIT_description_preferences_not"	: v("dislikestext") || "",
	"_EDIT_favorite_animal"		: v("animalfave") || "",
	"_EDIT_favorite_popstar"	: v("popstarfave") || "",
	"_EDIT_favorite_music"		: v("musicfave") || "",
	"_EDIT_page_start"		: v("startpage") || "",
#endif
		])
	));

	write("<div class='roomsub'>Room Subscriptions<br/>\n");
	foreach(mixed key, mixed val : v("subscriptions")) {
	    // note: the immediateness is not really used these days
	    write("<div class='each'><a href='");
	    unless(is_formal(key)) {
		write(query_server_unl() + "@" + key);
	    } else {
		write(key);
	    }
	    write("'>" + key + "</a></div>\n");
	}
	write("</div>\n");

	// build unified data structure for contacts
	mapping paliases = ([ ]);
	foreach(mixed key, mixed val : v("people")) {
	    paliases[key] = ({ 0, val });
	}
	foreach(mixed key, mixed val : v("aliases")) {
	    if (paliases[val])
		paliases[val][0] = key;
	    else
		paliases[val] = ({ key, 0 });
	}

	// make a link to a edit page for each contact?
	write("<div class='friends'>Friends<br/>\n");
	foreach(mixed  key, mixed val : paliases) {
	    // note: look at val to determine status
	    write("<div class='each'><a href='" + key);
	    write("'>" + (val[0] || key) + "</a>");
	    if (val[1])
		write(sprintf("subscription state %O\n", val[1]));
	    write("</div>\n");
	}
	write("</div>\n");
	return 1;
}
