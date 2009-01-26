#include <net.h>
#include <text.h>
#include <ht/http.h>

// phone call thing. we need to pass the query data to a flash film
// but we don't have any flash film yet. TODO
//
htget(prot, query, headers, qs) {
	htredirect(prot, "/static/myVideo.swf", "stupid flash film", 0);
	return 1;
}
