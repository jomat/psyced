// vim:foldmethod=marker:syntax=lpc:noexpandtab
//
// yeah yeah twitter.. why twitter?
// http://about.psyc.eu/Twitter

#include <net.h>

persistent int lastid;

volatile object feed;

parse(string body, mapping headers) {
	mixed wurst;
	string nick;
	object o;
	mapping d, p;
	int i;

	if (!body || body == "") {
		P1(("%O failed to get its timeline from %O.\n", ME,
		    previous_object()))
		PT(("Got headers: %O", headers))
		return;
	}
//#if DEBUG > 0
	rm(DATA_PATH "timeline.json");
	write_file(DATA_PATH "timeline.json", body);
	P4((body))
//#endif
	unless (pointerp(wurst = parse_json(body))) {
		P1(("%O failed to parse its timeline.\n", ME))
		return;
	}
	i=sizeof(wurst)-1;
	if (wurst[i]["id"] <= lastid) {
		P1(("%O received old stuff.\n", ME))
		return;
	}
	lastid = wurst[i]["id"];
	save_object(DATA_PATH "twitter");
	for (; i>=0; i--) {
		d = wurst[i];
		unless (mappingp(d)) {
			P1(("%O got a broken tweet: %O.\n", ME, d))
			continue;
		}
		p = d["user"];
		unless (mappingp(p)) {
			P1(("%O got a userless tweet.\n", ME))
			continue;
		}
		unless (nick = p["screen_name"]) {
			P1(("%O got a nickless tweeter.\n", ME))
			continue;
		}
		PT((" %O", nick))
		o = find_place(nick);

		// _notice_update_twitter ?
		sendmsg(o, "_message_twitter", d["text"], ([
					// should i send text as _action?
		    "_nick": nick,
		    // _count seems to be the better word for this
		    "_amount_updates": p["statuses_count"],
		    "_amount_followers": p["followers_count"],
		    "_amount_sources": p["friends_count"],
		    "_color": "#"+ p["profile_sidebar_fill_color"],
		    "_description": p["description"] || "",
		    "_page": p["url"] || "",
		    "_name": p["name"] || "",
		    // "_contact_twitter": p["id"],
		    "_description_agent_HTML": d["source"],
		    "_reference_reply": d["in_reply_to_screen_name"],
		    // "_twit": d["id"],
		    "_uniform_photo": p["profile_image_url"] || "",
		    "_uniform_photo_background":
			p["profile_background_image_url"] || ""
		]), "/");				// send as root

		// der spiegel u.a. twittern übrigens in latin-1
		// während psyc utf-8 erwartet.. eine char guess engine
		// muss her.. FIXME
	}
}

fetch() {
	P3(("%O ready to fetch from %O\n", ME, feed))
	call_out( #'fetch, 3 * 59 );	// odd is better
	feed -> content( #'parse, 0, 1 );
	feed -> fetch("http://twitter.com/statuses/friends_timeline.json"
		      "?count="+( lastid? ("23&since_id="+ lastid) : "23"));
}

create() {
	mapping config;
	object o = find_object(CONFIG_PATH "config");

	if (o) config = o->qConfig();
	if (!config) {
		P1(("\nNo configuration for twitter gateway found.\n"))
		//destruct(ME);
		return;
	}
	restore_object(DATA_PATH "twitter");

	// we could even choose to inherit this instead...
	feed = clone_object(NET_PATH "http/fetch");
	//feed -> sAgent(SERVER_VERSION " builtin Twitter to PSYC gateway");
	feed -> sAuth(config["nickname"], config["password"]);
	call_out( #'fetch, 14 );
}

