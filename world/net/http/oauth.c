/*
  current specification:
    http://tools.ietf.org/html/draft-hammer-oauth-10

  older ones:
    http://oauth.net/core/1.0a/
    http://oauth.net/core/1.0/
*/

#include <net.h>
#include <tls.h>
#include <ht/http.h>

string consumer_key;
string consumer_secret;
string request_token_url;
mapping request_params = ([ ]);
mapping access_params = ([ ]);
string access_token_url;
string authorize_url;
string callback_url = "http://" + my_lower_case_host() + ":" + HTTP_PORT + "/oauth"; //TODO: https?
object user;

varargs void fetch(object ua, string url, string method, mapping post, mapping oauth) {
    P3((">> oauth:fetch(%O, %O, %O)\n", url, method, oauth))
    unless (method) method = "GET";
    unless (post) post = ([]);
    unless (oauth) oauth = ([]);
    mapping get = ([]);
    string qs, base_url = url;
    if (sscanf(url, "%s?%s", base_url, qs))
	url_parse_query(get, qs);

    oauth["oauth_consumer_key"] = consumer_key;
    string token;
    if (token = access_params["oauth_token"] || request_params["oauth_token"])
	oauth["oauth_token"] = token;
    string token_secret = access_params["oauth_token_secret"] || request_params["oauth_token_secret"] || "";
    oauth["oauth_timestamp"] = time();
    oauth["oauth_nonce"] = sprintf("%x", random(oauth["oauth_timestamp"] ^ 98987));
    oauth["oauth_signature_method"] = "HMAC-SHA1";
    oauth["oauth_version"] = "1.0";

    P3(("token: %O, token_secret: %O, access: %O, request: %O\n", token, token_secret, access_params, request_params))
    string base_str = method + "&" + urlencode(base_url) + "&" + urlencode(make_query_string(get + post + oauth, 1));
    oauth["oauth_signature"] = hmac_base64(TLS_HASH_SHA1, urlencode(consumer_secret) + "&" + urlencode(token_secret), base_str);

    string p = "";
    foreach (string key, string value : oauth)
	p += (strlen(p) ? "," : "") + key + "=\"" + urlencode(to_string(value)) + "\"";

    ua->fetch(url, method, post, (["authorization": "OAuth " + p]));
}

void parse_request_token(string body, mapping headers, int http_status) {
    P3((">> oauth:parse_request_token(%O, %O, %O)\n", body, headers, http_status))
    if (http_status == R_OK) {
	request_params = ([]);
	url_parse_query(request_params, body);
	if (strlen(request_params["oauth_token"]) && strlen(request_params["oauth_token_secret"])) {
	    shared_memory("oauth_request_tokens")[request_params["oauth_token"]] = ME;
	    //P3((">>> adding token: %O to shm: %O\n", request_params["oauth_token"], shared_memory("oauth_request_tokens")))
	    sendmsg(user, "_notice_oauth_authorize_url", "Open [_url] to perform authorization.",
		    (["_url": authorize_url + "?oauth_token=" + request_params["oauth_token"]]));
	    return;
	}
    }
    sendmsg(user, "_error_oauth_token_request", "OAuth failed: could not get a request token.");
}

void parse_access_token(string body, mapping headers, int http_status) {
    P3((">> oauth:parse_access_token(%O, %O, %O)\n", body, headers, http_status))
    if (http_status == R_OK) {
	access_params = ([]);
	url_parse_query(access_params, body);
	if (strlen(access_params["oauth_token"]) && strlen(access_params["oauth_token_secret"])) {
	    sendmsg(user, "_notice_oauth_success", "OAuth successful.");
	    return;
	}
    }
    sendmsg(user, "_error_oauth_token_access", "OAuth failed: could not get an access token.");
}

void verified(string verifier) {
    P3((">> oauth:verified(%O)\n", verifier))
    object ua = clone_object(NET_PATH "http/fetch");
    ua->content(#'parse_access_token, 1, 1); //');
    fetch(ua, access_token_url, "POST", 0, (["oauth_verifier": verifier]));
}

object load(object usr, string key, string secret, string request, string access, string authorize) {
    if (usr) user = usr;
    if (key) consumer_key = key;
    if (secret) consumer_secret = secret;
    if (request) request_token_url = request;
    if (access) access_token_url = access;
    if (authorize) authorize_url = authorize;

    if (request_token_url && user) {
	object ua = clone_object(NET_PATH "http/fetch");
	ua->content(#'parse_request_token, 1, 1); //');
	fetch(ua, request_token_url, "POST", 0, (["oauth_callback": callback_url]));
    }
    return ME;
}
