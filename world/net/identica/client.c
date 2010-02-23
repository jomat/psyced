/* identi.ca client, uses the twitter api
 * http://status.net/wiki/Twitter-compatible_API
 *
 * - register app @ http://identi.ca/settings/oauthapps
 * - then in local.h #define IDENTICA_KEY & IDENTICA_SECRET
 */

#include <net.h>

inherit NET_PATH "twitter/client";

object load(object usr, string key, string secret, string request, string access, string authorize) {
    name = "identica";
    display_name = "identi.ca";
    api_base_url = "http://identi.ca/api";

    unless (consumer_key) consumer_key = IDENTICA_KEY;
    unless (consumer_secret) consumer_secret = IDENTICA_SECRET;
    unless (request_token_url) request_token_url = api_base_url + "/oauth/request_token";
    unless (access_token_url) access_token_url = api_base_url + "/oauth/access_token";
    unless (authorize_url) authorize_url = api_base_url + "/oauth/authorize";

    return ::load(usr, key, secret, request, access, authorize);
}
