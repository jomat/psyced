// vim:foldmethod=marker:syntax=lpc:noexpandtab
//
// yeah yeah twitter.. why twitter?
// http://about.psyc.eu/Twitter

#include <net.h>

persistent mixed lastid;

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
		PT(("Got headers: %O\n", headers))
		return;
	}
//#if DEBUG > 0
	rm(DATA_PATH "twitter/timeline.json");
	write_file(DATA_PATH "twitter/timeline.json", body);
	P4((body))
//#endif
	unless (pointerp(wurst = parse_json(body))) {
		monitor_report("_failure_network_fetch_twitter_broken",
		    "[_source] failed to parse its timeline");
		return;
	}
	unless (sizeof(wurst)) {
		monitor_report("_failure_network_fetch_twitter_empty",
		    "[_source] received an empty structure.");
		return;
	}
	// this used to fail on MAX_INT turning the ints to negative.. it would work for
	// a while longer using floats, but since floating point mantissa in lpc is only
	// 32 bits wide, it's just a question of time until we hit that roof again (when
	// status_id reaches 4294967296). so let's try strings instead. funny to run into
	// such a weird problem only after years that twitter has been in existence.
	// twitterific may have run into the same problem, as the timing of its breakdown
	// matches ours.
	if (lastid && wurst[0]["id"] <= lastid) {
		P1(("%O received %d old updates (id0 %O <= lastid %O).\n",
		    ME, sizeof(wurst), wurst[0]["id"], lastid))
		return;
	}
	lastid = wurst[0]["id"];
	P2(("%O -- new lastid %O\n", ME, lastid))
	save_object(DATA_PATH "twitter");
	for (i=sizeof(wurst)-1; i>=0; i--) {
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
		P4((" %O", nick))
		o = find_place(nick);

		// _message_twitter ? not so convincing.. a lot of the
		// things are converted rss newsfeeds, and when private
		// people are "chatting" over twitter, they are still
		// "broadcasting" each message to a random conjunction
		// of friends and strangers (we don't follow private
		// twitters with this gateway!) ... thus it is quite
		// appropriate that twitters are not given the same
		// relevance as a _message. still you can /highlight
		// particular senders in your client...
		//
		sendmsg(o,
		//  "_notice_headline_twitter", "([_nick]) [_headline]",
		    "_message_twitter", d["text"],
		  ([
		    "_headline": d["text"], // should i send text as _action?
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
	P2(("%O going to fetch from %O since %O\n", ME, feed, lastid))
	call_out( #'fetch, 4 * 59 );	// odd is better
	feed -> content( #'parse, 1, 1 );
	// twitter ignores since_id if count is present. stupid.
	feed -> fetch("http://twitter.com/statuses/friends_timeline.json?"
		 // +( lastid? ("since_id="+ lastid) : "count=23"));
		  "count="+( lastid? ("23&since_id="+ lastid) : "23"));
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

