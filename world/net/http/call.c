#include <ht/http.h>
#include <net.h>
#include <text.h>

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

mixed answer(string sid, int yesno, int thatsme, string variant) {
	if (!(sid && sessions[sid])) {
	    return -1; // no session found
	}
	if (sessions[sid][1] < time()) {
	    return -2; // session expired
	}
	string ni = sessions[sid][0];
	unless (thatsme) {
		string mc;
		object uo = find_person(ni);

		if (!uo) return -3;
		mc = yesno? "_notice_answer_call": "_notice_reject_call";
		if (variant) mc += variant;

		if (!sendmsg(uo, mc, 0, ([
		    "_time_expire"  : to_string(sessions[sid][1]),
		    "_check_call"   : sessions[sid][2], 
		    "_token_call"   : sid,
		  ]))) return -4;  // sendmsg failed;
	}
	return ni;
}

htget(prot, query, headers, qs) {
#ifdef TELEPHONY_SERVER
	string sid = query["sid"];
	string t = query["thats"];
	mixed ni = answer(sid, !query["reject"], t, "_click");

	if (intp(ni)) {
		hterror(prot, R_GATEWTIMEOUT, "User cannot be reached.");
		return 1;
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
