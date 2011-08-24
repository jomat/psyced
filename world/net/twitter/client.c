/* twitter client
 * http://apiwiki.twitter.com/Twitter-API-Documentation
 *
 * - register @ https://twitter.com/apps
 *   - settings:
 *     - app name: e.g. psyc://your.host/
 *     - app type: browser
 *     - callback url: http://your.host/oauth
 *                     (actually the url psyced sends will be used but you have to type in something)
 * - then in local.h #define TWITTER_KEY & TWITTER_SECRET
 *
 * - enabling the user stream
 *   - #define TWITTER_ADMIN which should contain a user name who will receive oauth messages
 *   - #define USE_TWITTER_STREAM
 *   - add this to local/config.c:
# ifdef USE_TWITTER_STREAM
	D(" " NET_PATH "twitter/client\n");
	load_object(NET_PATH "twitter/client")->home_stream();
# endif
 */
#include <net.h>
#include <ht/http.h>

inherit NET_PATH "http/oauth";
inherit NET_PATH "queue";

persistent mixed lastid;

volatile string api_url = "https://api.twitter.com/1";
volatile string userstream_url = "https://userstream.twitter.com/2";
volatile string name = "twitter";
volatile string display_name = "twitter";

volatile int status_max_len = 140;
volatile int send_to_user = 0;
volatile int wait = 0;
volatile mapping friends;

user_stream();

string object_file_name() {
    return DATA_PATH "twitter/" + (user ? user->qNameLower() : "-default");
}

save() {
    mkdir(DATA_PATH "twitter");
    save_object(object_file_name());
}

create() {
    return load();
}

object load(object usr, mapping opts) {
#ifdef TWITTER_ADMIN
    unless (usr) usr = user = summon_person(TWITTER_ADMIN, NET_PATH "user");
#endif
    unless (mappingp(opts)) opts = ([]);
    send_to_user = opts["send_to_user"];

    unless (consumer_key) consumer_key = TWITTER_KEY;
    unless (consumer_secret) consumer_secret = TWITTER_SECRET;
    unless (request_token_url) request_token_url = "https://twitter.com/oauth/request_token";
    unless (access_token_url) access_token_url = "https://twitter.com/oauth/access_token";
    unless (authorize_url) authorize_url = "https://twitter.com/oauth/authorize";

    restore_object(object_file_name());
    qCreate();
    qInit(ME, 100, 5);

    return ::load(usr, opts);
}

void check_status_update(string body, string headers, int http_status) {
    P3(("twitter/client:parse_status_update(%O, %O, %O)\n", body, headers, http_status))
    if (http_status != R_OK)
	sendmsg(user, "_failure_update_"+ name, "Unable to post status update on [_name].", (["_name": display_name]));
}

void status_update(string text) {
    P3(("twitter/client:status_update()\n"))
    if (status_max_len && strlen(text) > status_max_len) text = text[0..status_max_len-4] + "...";

    object ua = clone_object(NET_PATH "http/fetch");
    ua->content(#'check_status_update, 1, 1); //');
    fetch(ua, api_url + "/statuses/update.json", "POST", (["status": text]));
}

parse_statuses(string data) {
	mixed wurst;
	string nick;
	object o;
	mapping d, p;
	int i;

	if (!data || data == "") return;

	wurst = parse_json(data);
	if (mappingp(wurst))
		wurst = ({ wurst });
	else unless (pointerp(wurst)) {
		monitor_report("_failure_network_fetch_twitter_broken",
		    "[_source] failed to parse a status message");
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
	// status_id reaches 4294967296). so let's use bignums instead. funny to run into
	// such a weird problem only after years that twitter has been in existence.
	if (lastid && bignum_cmp(to_string(wurst[0]["id"]), to_string(lastid)) <= 0) {
		P1(("%O received %d old updates (id0 %O <= lastid %O).\n",
		    ME, sizeof(wurst), wurst[0]["id"], lastid))
		return;
	}
	lastid = wurst[0]["id"];
	save();
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

		o = send_to_user ? user : find_place(nick);

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

parse_home_timeline(string body, string headers, int http_status) {
    P3(("twitter/client:parse_home_timeline(%O, %O, %O)\n", body, headers, http_status))
    if (http_status == 401) {
	oauth();
	home_timeline();
    }
    if (http_status != R_OK || !body || body == "") return;
    parse_statuses(body);
}

home_timeline(mixed *next) {
    P3(("twitter/client:home_timeline()\n"))
    if (!authorized) return enqueue(ME, ({ #'home_timeline })); //'}));

    object ua = clone_object(NET_PATH "http/fetch");
    ua->content(#'parse_home_timeline, 1, 1); //');
    if (next) ua->content(next, 1, 1); //');
    fetch(ua, api_url + "/statuses/home_timeline.json");
    return ua;
}

home_stream() {
    home_timeline(#'user_stream);
}

// handle one line in the user stream which contains a full message in json format
// or the user's friend if this is the first line in the stream
user_stream_data(string data, string headers, int http_status, int fetching) {
    P3(("twitter/client:user_stream_data(..., %O, %O, %O)\n%O\n", headers, http_status, fetching, data))

    if (http_status == R_OK && data && data != "") {
	if (!friends)
	    friends = parse_json(data);
	else
	    parse_statuses(data);
    }

    if (fetching) {
	wait = 0;
	return;
    } else {
	P1(("%O disconnected with status %d, headers: %O\n", ME, http_status, headers))
    }

    switch (http_status) {
	case 401: // unauthorized
	    oauth();
	    home_stream();
	case 403: // forbidden
	case 404: // unknown
	case 406: // not acceptable
	case 413: // too long
	case 416: // range unacceptable
	    return;
	case 200:
	    break;
	case 420: // rate limited
	case 500: // server internal error
	case 503: // service overloaded
	default:
	    wait *= 2;
    }
    if (!wait) wait = 10;
    if (wait > 240) wait = 240;

    P1(("%O reconnecting in %d seconds.\n", ME, wait))
    call_out(#'home_stream, wait); //');
}

user_stream() {
    P3(("twitter/client:user_stream()\n"))
    if (!authorized) return enqueue(ME, ({ #'user_stream })); //'}));
    friends = 0;
    object user_ua = clone_object(NET_PATH "http/fetch");
    user_ua->content(#'user_stream_data, 1, 1); //');
    fetch(user_ua, userstream_url + "/user.json", "GET", 0, 0, 1);
}

oauth_success() {
    P3(("twitter/client:oauth_success()\n"))
    save();
    mixed *waiter;
    while (qSize(ME)) {
	waiter = shift(ME);
	funcall(waiter[0]);
    }
}

oauth_error() {
    P3(("twitter/client:oauth_error()\n"))
    call_out(#'oauth, 60); //');
}
