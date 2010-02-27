/* identi.ca client, uses the twitter api
 * http://status.net/wiki/Twitter-compatible_API
 *
 * - register app @ https://identi.ca/settings/oauthapps
 * - then in local.h #define IDENTICA_KEY & IDENTICA_SECRET
 */

#include <net.h>

inherit NET_PATH "twitter/client";

object load(object usr, string key, string secret, string request, string access, string authorize) {
    name = "identica";
    display_name = "identi.ca";
    api_base_url = "http://identi.ca/api";

    consumer_key = IDENTICA_KEY;
    consumer_secret = IDENTICA_SECRET;
    request_token_url = api_base_url + "/oauth/request_token";
    access_token_url = api_base_url + "/oauth/access_token";
    authorize_url = api_base_url + "/oauth/authorize";

    return ::load(usr, key, secret, request, access, authorize);
}
