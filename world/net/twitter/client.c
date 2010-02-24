/* twitter client
 * http://apiwiki.twitter.com/Twitter-API-Documentation
 *
 * - register @ https://twitter.com/apps
 *   - settings:
 *     - app name: e.g. psyc://your.host/
 *     - app type: browser
 *     - callback url: http://your.host/oauth
 *                     (actually the url psyced sends will be used but you have to type in something)
 *     - access type: read/write
 * - then in local.h #define TWITTER_KEY & TWITTER_SECRET
 */
#include <net.h>
#include <ht/http.h>

inherit NET_PATH "http/oauth";

string name = "twitter";
string display_name = "twitter";
string api_base_url = "http://api.twitter.com/1";

int status_max_len = 140;

object load(object usr, string key, string secret, string request, string access, string authorize) {
    unless (consumer_key) consumer_key = TWITTER_KEY;
    unless (consumer_secret) consumer_secret = TWITTER_SECRET;
    unless (request_token_url) request_token_url = "http://twitter.com/oauth/request_token";
    unless (access_token_url) access_token_url = "http://twitter.com/oauth/access_token";
    unless (authorize_url) authorize_url = "http://twitter.com/oauth/authorize";

    return ::load(usr, key, secret, request, access, authorize);
}

void parse_status_update(string body, string headers, int http_status) {
    P3(("twitter/client:parse_status_update(%O, %O, %O)\n", body, headers, http_status))
    if (http_status != R_OK)
	sendmsg(user, "_error_"+name+"_status_update", "Error: failed to post status update on [_name].", (["_name": display_name]));
}

void status_update(string text) {
    P3(("twitter/client:status_update()\n"))
    if (status_max_len && strlen(text) > status_max_len) text = text[0..status_max_len-4] + "...";

    object ua = clone_object(NET_PATH "http/fetch");
    ua->content(#'parse_status_update, 1, 1); //');
    fetch(ua, api_base_url + "/statuses/update.json", "POST", (["status": text]));
}

#if 1 //not used, just an example
void parse_home_timeline(string body, string headers, int http_status) {
    P3(("twitter/client:parse_home_timeline(%O, %O, %O)\n", body, headers, http_status))
}

void home_timeline() {
    P3(("twitter/client:home_timeline()\n"))
    object ua = clone_object(NET_PATH "http/fetch");
    ua->content(#'parse_home_timeline, 1, 1); //');
    fetch(ua, api_base_url + "/statuses/home_timeline.json");
}
#endif
