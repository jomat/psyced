// vim:foldmethod=marker:syntax=lpc:noexpandtab
//
// http://localhost:33333/net/twitter/listing shows a list of friends

#include <net.h>
#include <ht/http.h>
#include <text.h>

volatile object fetcha;
volatile mixed wurst;

parse(string body) {
	if (!body || body == "") {
		P1(("%O failed to get its listing.\n", ME))
		return;
	}
//#if DEBUG > 0
	rm(DATA_PATH "twitter/friends.json");
	write_file(DATA_PATH "twitter/friends.json", body);
	P4((body))
//#endif
	unless (pointerp(wurst = parse_json(body))) {
		P1(("%O failed to parse its listing.\n", ME))
		return;
	}
#ifdef DEVELOPMENT
	write_file(DATA_PATH "twitter/friends.parsed", sprintf("%O\n", wurst));
#endif
	P1(("%O sorting %O subscription names ", ME, sizeof(wurst)))
	wurst = sort_array(wurst, (:
		unless (mappingp($1)) return 0;
		unless (mappingp($2)) return 1;
//		PT(("%O got %O vs %O\n", ME, $1, $2))
		P1(("."))
		return  lower_case($2["screen_name"] || "") >
			lower_case($1["screen_name"] || "");
	:) );
	P1((" done!\n"))
}

htget(prot, query, headers, qs, data, noprocess) {
	string nick;
	mapping d; //, s;
	int i;

	//sTextPath(query["layout"] || "twitter", query["lang"], "html");
	localize(query["lang"], "html");

	unless (pointerp(wurst)) {
		hterror(R_TEMPOVERL,
			"Haven't successfully retrieved data yet.");
		return;
	}
	htok(prot); // outputs utf-8 header, but..
	w("_HTML_listing_head_twitter");
	for (i=sizeof(wurst)-1; i>=0; i--) {
		d = wurst[i];
		unless (mappingp(d)) {
			P1(("%O got a broken entry: %O.\n", ME, d))
			continue;
		}
//
// user "foebud" has no updates ;)
//
//		s = d["status"];
//		unless (mappingp(s)) {
//			P1(("%O got a statusless entry: %O.\n", ME, d))
//			continue;
//		}
		unless (nick = d["screen_name"]) {
			P1(("%O got a nickless tweeter.\n", ME))
			continue;
		}
		w("_HTML_listing_item_twitter", 0, ([
					// should i send text as _action?
		    "_nick": nick,
		    "_amount_updates": d["statuses_count"],
		    // _count_subscribers seems to be better for this
		    // or should it be _recipients? _targets?
		    "_amount_followers": d["followers_count"],
		    "_amount_sources": d["friends_count"],
		    // shows how old listing is.. hmm
		    //"_description_update": s["text"] || "",
		    "_color": "#"+ d["profile_sidebar_fill_color"],
		    "_description": d["description"] || "",
		    "_uniform_context": SERVER_UNIFORM +"@"+ nick,
		    "_page": d["url"] || "",
		    "_name": d["name"] || "",
		    // "_contact_twitter": d["id"],
		    "_reference_reply": d["in_reply_to_screen_name"],
		    // "_twit": d["id"],
		    "_uniform_photo": d["profile_image_url"] || "",
		    "_uniform_photo_background":
			d["profile_background_image_url"] || ""
		]));
	}
	w("_HTML_listing_tail_twitter");
	return 1;
}

fetch() {
	fetcha -> content( #'parse, 0, 1 );
	fetcha -> fetch("http://twitter.com/statuses/friends.json?count=200");
}

create() {
	mapping config;
	object o = find_object(CONFIG_PATH "config");

	if (o) config = o->qConfig();
	if (!config) {
		P1(("\nNo configuration for twitter gateway found in %O.\n", o))
		//destruct(ME);
		return 1;
	}

       	string body = read_file(DATA_PATH "twitter/friends.json");
       	if (body) return parse(body);

	// we could even choose to inherit this instead...
	fetcha = clone_object(NET_PATH "http/fetch");
	//fetcha -> sAgent(SERVER_VERSION " builtin Twitter to PSYC gateway");
	fetcha -> sAuth(config["nickname"], config["password"]);
	call_out( #'fetch, 14 );
}

