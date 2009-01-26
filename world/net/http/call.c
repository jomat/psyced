#include <net.h>
#include <text.h>
#include <ht/http.h>

protected mapping sessions = ([ ]);

string make_session(string nick, int expiry, string jack) {
    string sid;
#ifndef TELEPHONY_EXPIRY
# define TELEPHONY_EXPIRY expiry - time()
#endif
    while (sessions[sid = RANDHEXSTRING]);
    sessions[sid] = ({ nick, expiry, jack });
    call_out( (: return m_delete(sessions, sid); :), TELEPHONY_EXPIRY);
    return sid;
}

htget(prot, query, headers, qs) {
#ifdef TELEPHONY_SERVER
	string sid = query["sid"];
	if (!(sid && sessions[sid])) {
	    // no session found
	    return;
	}
	if (sessions[sid][1] < time()) {
	    // session expired
	    return;
	}

	string ni = sessions[sid][0];
	string t = query["thats"];

	unless (t) {
		object uo = find_person(ni);
		if (!uo || !sendmsg(uo, "_notice_answer_talk_click",
		  "Your phone call request has been clicked upon.", ([
		    "_time_expire" : to_string(sessions[sid][1]),
		    "_check_call"  : sessions[sid][2], 
		    "_session"     : sid,
		  ]))) {
			hterror(prot, R_GATEWTIMEOUT,
			    "User cannot be reached.");
			return 1;
		}
	}
	htok3(prot, 0, "Expires: 0\n");
	localize(query["lang"], "html");
	w("_HTML_head");
	w("_HTML_call", 0, ([
//	    "_path_object": "/static/call.swf",
//	    "_amount_width_object": "330",
//	    "_amount_height_object": "121",
	    "_path_object_local": "/static/local.swf",
	    "_path_object": "/static/remote.swf",
	    "_amount_width_object": "320",
	    "_amount_height_object": "240",
	    // url = rtmp server
	    "_uniform_server_media": TELEPHONY_SERVER,
	    // stream infos (thats, user, expiry, jack)
	    "_QUERY_STRING": qs,
	    "_role": t ? "I": "U",
	    "_nick": ni,
	    "_time_expire": to_string(sessions[sid][1]),
	    "_check_call" : sessions[sid][2],
	]));
	w("_HTML_tail");
#else
	hterror(prot, R_NOTIMPLEM, // "_error_unavailable_telephony"
	     "Sorry, no telephony configured on this server.");
#endif
	return 1;
}
