#include <net.h>

inherit NET_PATH "http/oauth";

object load(object usr, string key, string secret, string request, string access, string authorize) {
    consumer_key = TWITTER_KEY;
    consumer_secret = TWITTER_SECRET;
    request_token_url = "http://twitter.com/oauth/request_token";
    access_token_url = "http://twitter.com/oauth/access_token";
    authorize_url = "http://twitter.com/oauth/authorize";

    return ::load(usr, key, secret, request, access, authorize);
}

void parse_home(string body, string headers) {
    P3(("twitter/client:parse_home(%O, %O)\n", body, headers))
}

void home() {
    object ua = clone_object(NET_PATH "http/fetch");
    ua->content(#'parse_home, 1, 1); //');
    fetch(ua, "http://api.twitter.com/1/statuses/home_timeline.json");
}
