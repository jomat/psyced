/*
  current specification:
    http://tools.ietf.org/html/draft-hammer-oauth-10

  older ones:
    http://oauth.net/core/1.0a/
    http://oauth.net/core/1.0/
*/

#include <net.h>
#include <tls.h>

string consumer_key;
string consumer_secret;
string request_token_url;
string request_token;
string request_secret;
string access_token_url;
string access_token;
string access_secret;
string authorize_url;
string callback_url = "http://" + my_lower_case_host() + ":" + HTTP_PORT + "/oauth"; //TODO: https?
mapping oauth = ([]);
object user;

varargs void fetch(object ua, string url, string method, mapping oauth) {
    P3((">> oauth:fetch(%O, %O, %O)\n", url, method, oauth))
    unless (method) method = "GET";
    unless (oauth) oauth = ([]);

    oauth["consumer_key"] = consumer_key;
    if (access_token || request_token) oauth["token"] = access_token || request_token;
    string token_secret = access_token ? access_secret : request_token ? request_secret : "";
    oauth["timestamp"] = time();
    oauth["nonce"] = sprintf("%x", random(oauth["timestamp"] ^ 98987));
    oauth["signature_method"] = "HMAC-SHA1";
    oauth["version"] = "1.0";

    array(string) params = ({});
    foreach (string key : sort_array(m_indices(oauth), #'>)) //'))
	params += ({"oauth_" + key + "=" + urlencode(to_string(oauth[key]))});
    string base_str = method + "&" + urlencode(url) + "&" + urlencode(implode(params, "&"));
    oauth["signature"] = hmac_base64(TLS_HASH_SHA1, urlencode(consumer_secret) + "&" + urlencode(token_secret), base_str);

    params = ({});
    foreach (string key, string value : oauth)
	params += ({"oauth_" + key + "=\"" + urlencode(to_string(value)) + "\""});

    ua->fetch(url, method, (["Authorization": "OAuth " + implode(params, ",")]));
}

void parse_request_token(string body, mapping headers) {
    P3((">> oauth:parse_request_token(%O, %O)\n", body, headers))
    mapping params = ([]);
    url_parse_query(params, body);
    request_token = params["oauth_token"];
    request_secret = params["oauth_token_secret"];
    if (strlen(request_token) && strlen(request_secret)) {
	shared_memory("oauth_request_tokens")[request_token] = ME;
	sendmsg(user, "_notice_oauth_authorize_url", "Open [_url] to perform authorization.",
		(["_url": authorize_url + "?oauth_token=" + request_token]));
    } else {
	sendmsg(user, "_error_oauth_token_request", "OAuth failed: could not get a request token.");
    }
}

void parse_access_token(string body, mapping headers) {
    P3((">> oauth:parse_access_token(%O, %O)\n", body, headers))
    mapping params = ([]);
    url_parse_query(params, body);
    access_token = params["oauth_token"];
    access_secret = params["oauth_token_secret"];
    if (strlen(access_token) && strlen(access_secret)) {
	sendmsg(user, "_notice_oauth_success", "OAuth successful.");
    } else {
	sendmsg(user, "_error_oauth_token_access", "OAuth failed: could not get an access token.");
    }
}

void verified(string verifier) {
    P3((">> oauth:verified(%O)\n", verifier))
    object ua = clone_object(NET_PATH "http/fetch");
    ua->content(#'parse_access_token, 1, 1); //');
    fetch(ua, access_token_url, "POST", (["verifier": verifier]));
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
	fetch(ua, request_token_url, "POST", (["callback": callback_url]));
    }
    return ME;
}
