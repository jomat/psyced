#include "jabber.h"
#include <net.h> // vim:set syntax=lpc
#include <uniform.h>
#include "presence.h"
#include <time.h>

// necessary to implement a minimum set of commands for remote jabber users
// #undef USER_PROGRAM
// #undef MYNICK
// #define MYNICK  <yournicknamevariable>
inherit NET_PATH "name";

volatile string origin;
volatile string nickplace;
volatile mixed place;
// shared_memory()
volatile mapping jabber2avail, avail2mc;

// we can leave out the variables if these are static anyway
#define cmdchar '/'	// to remain in style with /me
#define actchar ':'	// let's try some mudlike emote for jabber
#define NICKPLACE nickplace
#ifndef T
# define T(MC, TEXT) TEXT   // do not use textdb
#endif
#include NET_PATH "usercmd.i"

void create() {
    jabber2avail = shared_memory("jabber2avail");
    avail2mc = shared_memory("avail2mc");
}

jabberMsg(XMLNode node, mixed origin, mixed *su, array(mixed) tu) {
    string target, source;
    string mc, data;
    string body;
    int isplacemsg;
    mapping vars;
    object o;
    XMLNode helper;
    mixed t;

    target = node["@to"];
    source = node["@from"];
    unless(origin)
	origin = XMPP + source;
#define MYORIGIN origin
// #define MYORIGIN XMPP + su[UUserAtHost]

    unless(su) su = parse_uniform(origin);
    origin = XMPP;
    if (su[UUser]) {
	origin += NODEPREP(su[UUser]) + "@";
    }
    origin += NAMEPREP(su[UHost]);

    if (su[UResource]) {
	origin += "/" + RESOURCEPREP(su[UResource]);
    }
    su = parse_uniform(origin);
    if (node["/nick"] && 
	    node["/nick"]["@xmlns"] == "http://jabber.org/protocol/nick" &&
	    node["/nick"][Cdata]) {
	sName(node["/nick"][Cdata]);
	vars = ([ "_nick": MYNICK ]);
    } else if (su[UUser]) {
	sName(su[UUser]);
	vars = ([ "_nick": MYNICK ]);
	// if i turn on this line, stuff no longer arrives at local target
	//if (su[UResource]) vars["_identification"] = origin;
	// but.. this is the way it should work.. right?!
    } else {
	vars = ([ "_nick" : su[UString] ]);
    }
#ifdef USE_THE_RESOURCE
    if (su[UResource]) {
	su[UResource] = RESOURCEPREP(su[UResource]);
	// entity.c currently needs both _source_identification and
	// _INTERNAL_identification to let it know the UNI is safe to use
	// this is good because PSYC clients will get to see the
	// _source_identification on the way out, too, while the fact
	// the id can be trusted will be removed
	vars["_INTERNAL_identification"] =
	  vars["_source_identification"] = XMPP + su[UUserAtHost];
	vars["_INTERNAL_source_resource"] = su[UResource];
	//vars["_location"] = origin;
	P2(("UNI %O for UNR %O\n", vars["_source_identification"], source))
    }
#endif

    unless(tu) tu = parse_uniform(XMPP + target);

    if (tu[UUser]) tu[UUser] = NODEPREP(tu[UUser]);
    // TODO: probably we need nameprep here also
    //

    if (tu[UResource]) {
	tu[UResource] = RESOURCEPREP(tu[UResource]);
	vars["_INTERNAL_target_resource"] = tu[UResource];
    }

    isplacemsg = ISPLACEMSG(tu[UUser]);
    /* I wonder if it makes sense to split this this into several functions,
     * at least for IQ it would make sense, dito for presence
     * also we should try to maximize shared code with jabber/user.c
     */
    switch (node[Tag]) {
    case "message":
	D2( if (isplacemsg) D("place"); )
	P2(("message %O from %O to %O\n",
	    node["@type"], origin, target))

#if 0
	// this check is completly insufficient and doesn't work anyway...
	if (node["/x"] && nodelistp(node["/x"])) // jabber:x:oob
	    vars["_uniform"] = node["/x"]["/url"];
#endif
	switch (node["@type"]) {
	case "error":
	    // watch out, do not create circular error messages
	    unless (o = summon_person(tu[UUser])) return;
	    if (node["/error"]) {
		// _nick_target? why?
		vars["_nick_target"] = origin;

		if (xmpp_error(node["/error"],
			       "service-unavailable")) {
		    if (node["/text"]) {
			vars["_text_XMPP"] = node["/text"][Cdata];
			mc = "_failure_unavailable_service_talk_text";
			data = "Talking to [_nick_target] is not possible: [_text_XMPP]";
		    } else {
			mc = "_failure_unavailable_service_talk";
//			data = "Talking to [_nick_target] is not possible. You may have to establish friendship first.";
// google talk sends this quite frequently:
//  * when a friendship exchange hasn't been done
//  * when a friend has gone offline
// and you never know if a message has been delivered to the
// recipient just the same! so here's a more accurate error message,
// effectively giving you less information, since that's what we have here.
			data = "Message to [_nick_target] may not have reached its recipient.";
		    }
		} else if (1) { // TODO: what was that error?
		    PT(("gateway TODO <error> in <message>: %O\n",
		       	node["/error"]))
		    mc = "_error_unknown_name_user";
		    data = "Can't find [_nick_target].";
		} else {
		    mc = "_jabber_message_error";
		    data = "[_nick] is sending you a jabber message error.";
		    // TODO: we can grab the error code / description
		    vars["_jabber_XML"] = innerxml;
		}
		sendmsg(o, mc, data, vars, origin);
	    }
	    break;
	case "groupchat": // _message_public
	    if (node["/body"] && !pointerp(node["/body"]))
		body = node["/body"][Cdata];
	    else {
		body = 0;
		P4(("no body in %O\n", node))
	    }
	    if (isplacemsg) {
		// lots of these should be handled by placeRequest/input
		// instead of sendmsg
		// let usercmd know which room we are operating on..
		unless (place = FIND_OBJECT(tu[UUser])) {
		    P0(("could not create place.. from %O to %O saying %O\n",
			source, target, body))
		    break;
		}
		P2(("groupchat to %O\n", place))
		// eg this should be a placeRequest("_set_topic", ...)
		if (node["/subject"]
			&& stringp(node["/subject"][Cdata])) {
#if 0
		    PT(("attempt by %O to change subject in %O lost: %O\n", 
			ME, place, node))
#else
		    vars["_topic"] = node["/subject"][Cdata];
		    sendmsg(place, "_request_set_topic", 0, vars, origin);
#endif
		    break;
		}
		PT(("input¹ %O\n", body))
		if (stringp(body) && strlen(body)) {
#ifdef BETA
		    if (body[0] == '\n') body = body[1..];
#endif
		    if (body[0] == cmdchar) {
			// '/ /usr' notation is a USER_PROGRAM feature
			// so we have to redo it here
			if (strlen(body) > 1 && body[1] == ' ') {
			    body = body[2..];
			    // fall thru
			} else {
			    parsecmd(body[1..]);
			    return 1;
			}
		    }
		    sendmsg(place, "_message_public", body,
			    vars, origin);
		}
	    } else { // remote join stuff room message
		o = summon_person(tu[UUser]);
		// design decision: show them with full room nickname
		if (su[UResource])
		    vars["_nick"] = su[UResource];
		vars["_nick_place"] = vars["_INTERNAL_identification"] || origin;

#if __EFUN_DEFINED__(mktime)
		if (helper = getchild(node, "x", "jabber:x:delay")) {
		    string fmt = helper["@stamp"];
		    int *time = allocate(TM_MAX);
		    int res;

		    // xep 0091 style CCYYMMDDThh:mm:ss
		    // 20080410T19:12:22
		    res = sscanf(fmt, "%4d%2d%2dT%2d:%2d:%2d", 
				 time[TM_YEAR], time[TM_MON], 
				 time[TM_MDAY], time[TM_HOUR],
				 time[TM_MIN], time[TM_SEC]);
		    if (res == 6  && (res = mktime(time)) != -1) {
			vars["_time_place"] = res; //helper["@stamp"];
		    }
		}
#endif
#ifdef MUCSUC
		// now using channels for unicast context emulation
		vars["_context"] = XMPP+ su[UUserAtHost]
				     +MUCSUC_SEP+ tu[UUser];
		o = find_context(vars["_context"]);
		if (!o) {
		    P0(("%O could not find the personal remotemuc for %O\n",
			ME, vars["_context"]))
		    return;
		}
		P3(("xmpp castmsg %O\n", o))
#endif
		if (!su[UResource] && node["/subject"]) {
		    /* a message from the room with subject (and in theory
		     * no body) is the topic
		     */
		    vars["_topic"] = node["/subject"][Cdata];
#ifdef MUCSUC
		    o->castmsg(origin, "_status_place_topic", 0, vars);
		    //sendmsg(o, "_status_place_topic", 0, vars, origin);
		} else {
		    o->castmsg(origin, "_message_public", body, vars);
		    //sendmsg(o, "_message_public", body, vars, origin);
#else
                    sendmsg(o, "_status_place_topic", 0, vars, origin);
                } else {
                    sendmsg(o, "_message_public", body, vars, origin);
#endif
		}
		// innerxml pass-thru
	//	vars["_jabber_XML"] = innerxml;
	//	sendmsg(o, "_jabber_message_groupchat", 0, vars, origin);
	    }
	    break;
	case 0: // _message_private which may have a subject
	case "chat": // _message_private
	default: 
	    if (isplacemsg) {
#if 1 // STRICTLY UNFRIENDLY NON-FUN TREATMENT
		sendmsg(XMPP + source, "_failure_unsupported_function_whisper", 
			"Routing private messages through groupchat managers is dangerous to your privacy and therefore disallowed. Please communicate with the person directly.",
			([ "_INTERNAL_source_jabber" : target,
			   "_INTERNAL_target_jabber" : source ]),
			ME);
#else // MAKE FUN OF ST00PID JABBER USERS VARIANT
		// handle this by doing "flüstern" in room  ;)
		// <from> whispers to <to>: <cdata>
		P0(("private message in place.. from %O to %O\n",
		    source, target))
		// stimmt das? egal..
		o = FIND_OBJECT(tu[UUser]);
		vars["_nick_target"] = tu[UResource];
		sendmsg(o, "_message_public_whisper",
	    // "[_nick] tries to whisper to [_nick_target] but it fails",
			node[Cdata], vars, origin);
		// cmd("/whisper", ...?)
#endif
	    } else if (!tu[UUser]) {
		// stricmp is better than lower_case only when both sides
		// have to be lowercased..
		if (lower_case(tu[UResource]) == "echo") {
		    sendmsg(origin, "_message_private",
			    node["/body"][Cdata], 
			    ([ "_INTERNAL_source_jabber" : target,
			     "_INTERNAL_target_jabber" : source ]), ME);
		} else if (node["/body"] 
			   && node["/body"][Cdata][0] != cmdchar) {
		    // monitor_report will log this to a file 
		    // if no admin is listening
		    monitor_report("_request_message_administrative",
				   sprintf("%O wants to notify the administrators of %O", origin, node["/body"][Cdata]));
		}
	    } else { 
		// no relaying allowed, so we ignore hostname
		o = summon_person(tu[UUser]);
		// xep 0085 typing notices - we even split active into a separate message
		// for now. could be sent as a flag
		if ((node[t="/composing"] || node[t="/active"] || 
			node[t="/paused"] ||node[t="/inactive"] ||node[t="/gone"]) &&
			node[t]["xmlns"] == "http://jabber.org/protocol/chatstates") {
		    // ...
		    sendmsg(o, "_notice_typing_" + t[1..], 0, vars); 
		}
		// there are some messages which dont have a body
		// we dont care about those
		unless (node["/body"]) return;
		ASSERT("Cdata", mappingp(node["/body"])
				&& stringp(node["/body"][Cdata]), node)
		body = node["/body"][Cdata];

		if (strlen(body) && body[0] == cmdchar) {
		    body = body[1..];
		    if (abbrev("me ", body)) {
			// doesn't cmd() handle this?
			vars["_action"] = body[3..];
			body = 0;
#ifdef USERCMD_IN_JABBER_CONVERSATION
		    } else {
			// this doesn't take care of '/ /usr' notation!
			parsecmd(body);
			break;
#else
			// fall thru
			// the /bin/whatever will be treated as normal text
			// so nusse is happy
#endif
		    }
		}

		if (helper = getchild(node, "x", "jabber:x:signed")) {
			vars["_signature"] = helper[Cdata];
			vars["_signature_encoding"] = "base64";

		}
		if (helper = getchild(node, "x", "jabber:x:encrypted")) {
			vars["_data_openpgp"] = helper[Cdata];
			// syntactical note: i would prefer to have this var
			// called _data_openpgp:_encoding
			vars["_encoding_data_openpgp"] = "base64";
			mc = "_notice_private_encrypt_gpg";
			// well... we need to put this stuff here and cant 
			// have it in the textdb...
			// I would appreciate if we could do something like 
			// body = 0 and the fmt would be fetched from the 
			// textdb...
			// also, this eludes the users language setting
			// (this problem also occurs with presence 
			//  notifications)
			body = "openpgp encrypted message data follows\n"
				"--- BEGIN OPENPGP BLOCK ---\n"
				"[_data_openpgp]\n"
				"--- END OPENPGP BLOCK ---";
		};
		// shouldn't we use /tell?
		sendmsg(o, mc || "_message_private", body,
			vars, origin);
	    }
	    ; // break??

	}
	break;
    case "presence":
	if (!isplacemsg && getchild(node, "x", "http://jabber.org/protocol/muc#user")) {
	    isplacemsg = 2;
	}
	D2( if (isplacemsg) D("place"); )
	P2(("presence %O from %O to %O\n", 
	    node["@type"],
	    XMPP + source,
	    target))
	// su = parse_uniform(XMPP + source);
	// see also: XMPP-IM §2.2.1 Types of Presence
	switch (node["@type"]) {
	case "error": 
	    // TODO: 
	    // for now we ignore it at least
	    // so there wont be circular error messages
	    if (tu[UUser]) {
		o = summon_person(tu[UUser]);
		// the following should catch errors - in theory, requires testing
		if (o) {
		    int cb_ret;
		    mixed err;
		    err = catch(
			cb_ret = o->execute_callback(node["@id"], ({ vars["_INTERNAL_identification"], vars, node }) )
		    );
		    if (err) {
			P0(("%O caught error during callback execution: %O\n", ME, err))
		    }
		    if (err || cb_ret) {
			return 1;
		    }
		}
	    }
	    if (tu[UResource]) {
		// innerxml
		vars["_jabber_XML"] = innerxml;
		//sendmsg(o, "_jabber_presence_error", 0, vars, origin);
		P1(("%O presence error. innerxml proxy to %O please: %O\n",
		    ME, node["@to"], innerxml))
	    }
	    break;
	case "subscribe": // _request_friendship
	    if (isplacemsg) {
		// autojoins dont work that way - what are 
		// those clients (ichat, gaim) trying to do?
		// what's the appropriate stanza error?
		// btw, text elemnent in stanzas errors SHOULD
		// NOT be displayed to the user (see rfc3920 §9.3)
		P2(("%O encountered presence %O for place %O\n",
		    ME, node["@type"], tu[UUser]));
		o = FIND_OBJECT(tu[UUser]);
		if (o->qNewsfeed())
		    sendmsg(origin, "_notice_friendship_established", 0, 
			    ([ "_INTERNAL_source_jabber" : target,
		    	       "_INTERNAL_source_jabber_bare" : target,
			       "_INTERNAL_target_jabber" : source ]), 
			    ME);
		else 
		    sendmsg(origin, "_error_unsupported_method_request_friendship", 0,
			    ([ "_INTERNAL_source_jabber" : target,
			       "_INTERNAL_target_jabber" : source]), 
			    ME); 
		return;
	    }
	    unless(tu[UResource]) {
		o = summon_person(tu[UUser]);
		if (su[UResource]) {
		    P0(("encountered _request_friendwith with resource from %O to %O\n", source, target))
		    // return;
		}
		sendmsg(o, "_request_friendship", 0, vars, MYORIGIN);
	    } else {
		// not sure if that's valid.. so let's look out for it
		P0(("%O Surprise! Encountered friendship w/out resource: %O\n",
		    ME, node))
	    }
	    break;
	case "subscribed": // _notice_friendship_established
	    if (isplacemsg) {
		P2(("%O encountered presence %O for place %O\n",
		    ME, node["@type"], tu[UUser]));
		sendmsg(origin, "_error_unsupported_method_notice_friendship_established", 0,
			([ "_INTERNAL_source_jabber" : target,
			   "_INTERNAL_target_jabber" : source]), 
			ME); 
		return;
	    }
	    unless(tu[UResource]) {
		o = summon_person(tu[UUser]);
		if (su[UResource]) {
		    P0(("encountered _notice_friendship_established with resource from %O to %O\n", source, target))
		    // return;
		}
		sendmsg(o, "_notice_friendship_established", 0, vars, MYORIGIN);
	    } else {
		// not sure if that's valid
	    }
	    break;
	case "unsubscribe": // _notice_friendship_removed
	    if (isplacemsg) {
		// TODO: wouldn't it be better to use _jabber_presence_error
		// here in conjunction with _jabber_XML?
		//
		// like for subscribe, this might be useful for newsfeed
		// 	if place->qNewsfeed() schicke ein unsubscribed zurueck
		o = FIND_OBJECT(tu[UUser]);
		if (o->qNewsfeed()) 
		    sendmsg(origin, "_notice_friendship_established", 0, 
			    ([ "_INTERNAL_source_jabber" : target,
		    	       "_INTERNAL_source_jabber_bare" : target,
			       "_INTERNAL_target_jabber" : source ]), 
			    ME);
		else 
		    sendmsg(origin, "_error_unsupported_method_notice_friendship_removed", 0,
			    ([ "_INTERNAL_source_jabber" : target,
			       "_INTERNAL_target_jabber" : source]), 
			    ME); // should it be tu[UString] instead? TODO
	    }
	    /*
	     * mh... this may be one-sided... but PSYC
	     * does not have one-sided subscription
	     * so... fall thru
	     */
	case "unsubscribed": // _notice_friendship_removed
	    if (isplacemsg) {
		// ignore it
	    } else {
		unless (o = summon_person(tu[UUser])) return;
		vars["_possessive"] = "the";
		if (su[UResource]) {
		    P0(("encountered _notice_friendship_removed with resource from %O to %O\n", source, target))
		    // return;
		}
		sendmsg(o, "_notice_friendship_removed", 0, vars, MYORIGIN);
	    }
	    break;
	case "unavailable": // _notice_presence_absent / _notice_place_leave
	    if (isplacemsg == 1) {
		o = FIND_OBJECT(tu[UUser]);
#ifndef DONT_REWRITE_NICKS
		vars["_nick_local"] = tu[UResource]; // it's a matter of case
#endif
		sendmsg(o,
#ifdef SPEC                  
                     "_request_context_leave"
#else                        
                     "_request_leave"
#endif
                    , 0, vars, origin);
	    } else if (isplacemsg == 2) { // remote join stuff
		o = summon_person(tu[UUser]);
		vars["_nick"] = su[UResource];
		vars["_nick_place"] = vars["_INTERNAL_identification"] || origin;
#ifdef MUCSUC
		vars["_context"] = XMPP+ su[UUserAtHost]
				      +MUCSUC_SEP+ tu[UUser];
#else
		vars["_context"] = vars["_nick_place"];
#endif
		if (o && o->execute_callback(node["@id"], ({ o, vars, node }))){
		    return 1;
		}
#ifdef MUCSUC
		o = find_context(XMPP+ su[UUserAtHost]
				   +MUCSUC_SEP+ tu[UUser]);
		if (o) 
		    o->castmsg(origin, "_notice_place_leave", 0, vars);
		else {
		    P0(("%O could not find the personal remotemuc for %O bis\n",
			ME, vars["_context"]))
		}
#else
		sendmsg(o, "_notice_place_leave_unicast", 0, vars, origin);
#endif
	    } else {
#ifdef AVAILABILITY_OFFLINE
		o = summon_person(tu[UUser]);
		// http://www.psyc.eu/presence
		vars["_degree_availability"] = AVAILABILITY_OFFLINE;
# ifdef CACHE_PRESENCE
		persistent_presence(XMPP + su[UUserAtHost],
				   AVAILABILITY_OFFLINE);
# endif
		vars["_description_presence"] =
		    (node["/status"] && node["/status"][Cdata]) ?
		    node["/status"][Cdata] : ""; // "Get psyced!";
		vars["_INTERNAL_XML_description_presence"] =
		    xmlquote(vars["_description_presence"]);
		vars["_INTERNAL_mood_jabber"] = "neutral";
		sendmsg(o, "_notice_presence_absent", 0,
			vars, origin);
#endif
	    }
	    break;
	case "probe":
	    if (isplacemsg) {
		// this is actually not an error with newsfeed 
		// being subscribable
		P2(("%O encountered presence %O for place %O\n",
		    ME, node["@type"], tu[UUser]));
	    } else {
		// probe SHOULD only be generated by server but gmail
		// sends it from a generated resource string. also jabber.org
		// let's clients send it occasionally
		o = summon_person(tu[UUser]);
		sendmsg(o, "_request_status_person", 0,
			vars, origin); 
		// XMPP + su[UUserAtHost]);
		// maybe we can fix gmail presence by passing the UNR
		// instead of the UNI in source
	    }
	    break;
	default: // this is bad!
	    P2(("jabber presence isplacemsg %O\n", isplacemsg))
	    if (isplacemsg == 1) {
		// TODO: houston... there is no way to
		// decide whether this is a join or a 
		// status change... so the current 
		// behaviour of the rooms will send member
		// list and history on each status change...
#if 0
		// was not that a good idea...
		if (node["/status"]) { 
		    P2(("skipping status change in place\n"))
		    return;
		}
#endif
		if (helper = getchild(node, "x", "http://jabber.org/protocol/muc")) {
		    if (helper["/password"])
			vars["_password"] = helper["/password"][Cdata];
		    if (helper["/history"]) {
			// FIXME: support for other modes
			if (t = helper["/history"]["@maxstanzas"])
			    vars["_amount_history"] = t;
		    }
		}
		o = FIND_OBJECT(tu[UUser]);

		// lets see, if it works with lower_case
		// it seems clients dont care about the case...
		// but at least normal muc components care about case
		// did i mention that muc is silly?
//		if (lower_case(vars["_nick"]) != lower_case(tu[UResource])) 
		// yes! this is a good use for stricmp! ;)
		// YACK!!! this does not work as intended.
		// lynx, fix it please!!!
		// hm.. the definition of stricmp is inverted.. oops
#ifdef DONT_REWRITE_NICKS
		if (stricmp(vars["_nick"], tu[UResource])) {
		    // as everything else is much too complicated:
		    sendmsg(XMPP + source, "_error_unavailable_nick_place", 0,
			    ([ "_INTERNAL_source_jabber" : target,
			       "_INTERNAL_target_jabber" : source ]), 
			    o);
		    return;
		}
#else
		vars["_nick_local"] = tu[UResource]; // it's a matter of case
#endif
//		if (node["/show"]) {
		    // then it should be a availability change
		    // yet... are there possibly clients that try sending
		    // this upon the initial enter?
		    // -- yes, if they're in global away and try to join
		    // did I mention that muc is a silly protocol?
//		}
		P4(("_request_enter from %O to %O: %O\n", ME, o, vars))
		// dont send me a memberlist if i am a member already
#ifndef _limit_amount_history_place_default
# define _limit_amount_history_place_default 5	
#endif
		unless(vars["_amount_history"]) 
		    vars["_amount_history"] = _limit_amount_history_place_default;
		sendmsg(o,
#ifdef SPEC
                        "_request_context_enter"
#else
                        "_request_enter"
#endif
                        "_again", 0,
			vars, origin);
	    } else if (isplacemsg == 2) { // remote join stuff
#ifdef MUCSUC
		object ctx = find_context(XMPP+ su[UUserAtHost]
					     +MUCSUC_SEP+ tu[UUser]);
		if (!ctx) {
		    P0(("%O could not find the remotemuc for %O tris\n",
			ME, vars["_context"]))
		    return;
		}
#endif
		o = summon_person(tu[UUser]);
		vars["_nick"] = su[UResource];
		vars["_nick_place"] = vars["_INTERNAL_identification"] || origin;
#ifdef MUCSUC
		if (ctx) // get memberlist from remote slave
		    vars["_list_members"] = vars["_list_members_nicks"] = ctx->qMembers() + ({ tu[UUser] });
		if (o && o->execute_callback(node["@id"], ({ vars["_context"], vars, node }))) return 1;
		m_delete(vars, "_list_members");
		m_delete(vars, "_list_members_nicks");
		if (o = ctx) 
		    o->castmsg(origin, "_notice_place_enter", 0, vars);
		else {
		    // this can happen when joining
		    PT(("%O could not find the remotemuc for %O (yet)\n",
			ME, vars["_context"]))
		}
# else
		vars["_context"] = vars["_nick_place"];
		if (o && o->execute_callback(node["@id"], ({ vars["_INTERNAL_identification"], vars + ([ "_list_members" : 0, "_list_members_nicks" : 0 ]), node }))) return 1;
		// comes with a faked _context for logic in user.c
		sendmsg(o, "_notice_place_enter_unicast", 0, vars, origin);
#endif
	    } else {
		int isstatus;
		/* see http://www.psyc.eu/presence */
		// if the node contains a x element in the
		// jabber:x:delay namespace this is a
		// _status_presence_here
		o = summon_person(tu[UUser]);
		if (helper = getchild(node, "x", "jabber:x:delay")) {
		    isstatus = 1;
		}
//		if (!intp(isstatus)) {
		    // parse jabbertime and convert to timestamp
		    // we also know since when he has
		    // been available			TODO
//		}
		vars["_description_presence"] =
		    (node["/status"] && node["/status"][Cdata]) ?
		    node["/status"][Cdata] : ""; // "Get psyced!";
		vars["_INTERNAL_XML_description_presence"] =
		    xmlquote(vars["_description_presence"]);
		vars["_degree_availability"] = jabber2avail[node["/show"]
						&& node["/show"][Cdata]];
		// this message is too verbose, let's put in into
		// debug log so we can see it in relation to the
		// bug we experienced before (does it still exist?)
//		PV(("p-Show in %O, origin %O, isstatus %O, vars %O\n",
//			ME, origin, isstatus, vars));
		// the info hasn't proved useful  :(
		vars["_INTERNAL_quiet"] = 1; 
		vars["_INTERNAL_mood_jabber"] = "neutral";
		sendmsg(o, (isstatus? "_status_presence": "_notice_presence")
			+ (avail2mc[vars["_degree_availability"]] || "_here"), 0,
			    vars, origin);
#ifdef CACHE_PRESENCE
		persistent_presence(XMPP + su[UUserAtHost],
				    vars["_degree_availability"]);
#endif
	    }
	    break;
	}
	break;
    case "iq":
    {
	mixed iqchild = getiqchild(node);
	string xmlns = iqchild ? iqchild["@xmlns"] : 0;
	// TODO: maybe this should be handled by several functions
	// iq_get, iq_set, iq_result, iq_error
	t = node["@type"];
	if (t == "result" || t == "error") {
	    if (tu[UUser]) 
		o = FIND_OBJECT(tu[UUser]);
	    if (o && o->execute_callback(node["@id"], ({ origin, vars, node })))
		return 1;
	    vars["_tag_reply"] = node["@id"];
	} else {
	    vars["_tag"] = node["@id"]; 
	}
	// mh... we don't get that child with a result if the 
	// entity that we have asked does not have a vCard
	// cool protocol!
	switch(xmlns) {
	case "vcard-temp":
	{
	    mixed mvars;
	    // innerxml note: only result is a possible candidate
	    switch (t) {
	    case "result":
		// this should not happen any longer since _request_description is chained
		P3(("vCard result from %O to %O\n", source, target))
		// only do the work if we find a rcpt
		unless (o = summon_person(tu[UUser])) return;
		mvars = convert_profile(node["/vCard"], "jCard");
		PT(("extracted from vCard: %O\n", mvars))
		mvars["_nick"] = su[UUser] || origin;
		sendmsg(o, "_status_description_person", 0, mvars, origin);
		break;
	    case "get":
		P3(("vCard request from %O to %O\n",
		    source, target))
		// target must be a 'bare' jid, but hey... we dont
		// care about those rules anyway
		if (isplacemsg) return;
		if (tu[UResource]) return;
		unless (tu[UUser]) return;
		o = summon_person(tu[UUser]);
		unless (o) return; // TODO
		sendmsg(o, "_request_description_vCard", 0, vars, origin);
		break;
	    case "set": 
		// a remote entity trying to do a set? haha!
		// just be gentle and ignore it
		P0(("%O Surprise! Encountered vCard set: %O\n", ME, node))
		break;
	    case "error":
		// this should not happen any longer since _request_description is chained
		if (node["/error"]) {
		    unless (o = summon_person(tu[UUser])) {
			// watch out, do not create circular error messages
			P0(("%O vCard error from %O to %O\n",
			    ME, source, target))
			return;
		    }
		    vars["_nick_target"] = MYORIGIN; // should be origin probably
		    if (xmpp_error(node["/error"], 
				   "service-unavailable")) {
			mc = "_failure_unavailable_service_description";
		    } else {
			mc = "_error_unknown_name_user";
		    }
		    sendmsg(o, mc, 0, vars, origin);
		}

		break;
	    }
	    break;
	}
	case "http://jabber.org/protocol/disco#info":
	    if (iqchild["@node"])
		vars["_target_fragment"] = iqchild["@node"];
	    if (tu[UUser])
		o = FIND_OBJECT(tu[UUser]);
	    else
		o = "/" + (tu[UResource] || "");
	    switch(node["@type"]) {
	    case "get":
		sendmsg(o, "_request_list_feature", 0, vars, origin);
		break;
	    case "set": 	// doesnt make sense
	    case "result": 	// handled by callback usually
	    case "error":  	// dito
		break;
	    }
	    break;
	case "http://jabber.org/protocol/disco#items":
	    if (iqchild["@node"])
		vars["_target_fragment"] = iqchild["@node"];
	    if (tu[UUser]) 
		o = FIND_OBJECT(tu[UUser]);
	    else
		o = "/" + (tu[UResource] || "");
	    switch(node["@type"]) {
	    case "get":
		sendmsg(o, "_request_list_item", 0, vars, origin);
		break;
	    case "set": 	// doesnt make sense
	    case "result": 	// handled by callback usually
	    case "error":  	// dito
		break;
	    }
	    break;
	case "jabber:iq:version":
	    switch(t) {
	    case "get":
		if (tu[UUser])
		    o = FIND_OBJECT(tu[UUser]);
		else 
		    o = "/" + (tu[UResource] || "");
		PT(("sending _request_version to %O\n", o))
		sendmsg(o, "_request_version", 0, vars, origin);
		break;
	    case "set":
		// UHM???
		P0(("encountered jabber:iq:version set\n"))
		break;
	    case "result":
	    case "error":
		P0(("got jabber:iq:version result/error without tag\n"))
		break;
	    }
	    break;
	case "jabber:iq:last":
	    switch(t) {
	    case "get":
		if (isplacemsg || is_localhost(lower_case(target))) 
		    o = "/" + (tu[UResource] || "");
		else 
		    o = summon_person(tu[UUser]);
		sendmsg(o, "_request_description_time", 0, vars, origin); 
		break;
	    case "set":
		break;
	    case "result":
		o = summon_person(tu[UUser]);
		vars["_time_idle"] = node["/query"]["@seconds"];
		sendmsg(source, "_status_description_time", 0, vars, origin);
		break;
	    case "error":
		break;
	    }
	    break;
	case "urn:xmpp:ping":
	    if (tu[UUser]) 
		o = FIND_OBJECT(tu[UUser]);
	    else
		o = "/" + (tu[UResource] || "");
	    switch(t) {
	    case "get":
	    case "set": // I dont know why xep 0199 uses set... its a request
		sendmsg(o, "_request_ping", 0, vars, origin);
		break;
		break;
	    case "result": // caught by tagging
		break;
	    case "error": // caught by tagging
		break;
	    }
	    break;
	default: 
	    // isn't this dangerous now that we send a resource 
	    if (tu[UResource]) {
		vars["_jabber_XML"] = innerxml;
		o = summon_person(tu[UUser]);
		sendmsg(o, "_jabber_iq_"+ t,
			"[_source] is sending you a jabber iq "+t, vars, origin);
	    } else {
		switch(t) {
		case "get":
		case "set":
		       // see XMPP-IM §2.4 
		       // (whereas we are rather recipient than router)
		       // send service-unavailable stanza error
		    sendmsg(origin, "_error_unsupported_method", 0, 
			    ([ "_INTERNAL_source_jabber" : target,
			       "_INTERNAL_target_jabber" : source,
			       "_tag_reply" : node["@id"] ]));
		    break;
		case "result":
		   // usually we dont do requests where we dont
		   // understand the answer
		   // hence this is usually caught by TAGGING
		    P0(("%O iq result from %O to %O\n", ME, source, target))
		    break;
		case "error":
		   // dont create circular error messages and hence: ignore
		    P0(("%O iq error from %O to %O\n", ME, source, target))
		    break;
		default:
		    P0(("%O ignores unknown iq: %O\n", ME, t))
		    break;
		}
	    }
	    break;
	}
	break;
    }
    default:
	// mh... this might be interesting...
	break;
    } 
    return 1;
}
