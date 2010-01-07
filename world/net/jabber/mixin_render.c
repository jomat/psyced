#include "jabber.h"
#include <net.h>
#include <uniform.h>

// just renderMembers
#include NET_PATH "members.i"

int msg(string source, string mc, string data,
	    mapping vars, int showingLog, string target) {
    mixed t;

    switch (mc) {
    case "_status_description_time":
    case "_status_person_away":
    case "_status_person_present": 
    case "_status_person_present_implied": 
#if 0
	if (strstr(vars["_INTERNAL_target_jabber"], "@") == -1) {
	    P3(("skipping status person* to %O\n", vars["_INTERNAL_target_jabber"]))
	    return 1;
	}
#endif
	// reply to a presence probe
	if (member(vars, "_time_idle")) {
	    t = vars["_time_idle"];
	    if (stringp(t)) {
		t = to_int(t);
		PT(("_time_idle %O == %O, right?\n", vars["_time_idle"], t))
	    }
	    t = gmtime(time() - t);
	    vars["_INTERNAL_time_jabber"] = JABBERTIME(t);
	} else {
	    return 1;
	}
	break;
    case "_status_legacy_CTCP":
    case "_request_legacy_CTCP":
	// ignore these
	return 1;
	break;
    case "_request_examine": // don't use this, please remove in 2009
    case "_request_description": // this is the one.
	mc = "_request_description_vCard";	// pending rename.. TODO
	unless (vars["_tag"]) vars["_tag"] = RANDHEXSTRING;
	source->chain_callback(vars["_tag"], (:
		if ($3["@type"] == "result") {
		    mixed mvars = convert_profile($3["/vCard"], "jCard");
		    return ({ $1, "_status_description_person", 0, mvars + $2 });
		}
		else {
		    string err_mc;
		    if (xmpp_error($3["/error"], 
				   "service-unavailable")) {
			err_mc = "_failure_unavailable_service_description";
		    } else {
			err_mc = "_error_unknown_name_user";
		    }
		    return ({ $1, err_mc, "Received no description from [_nick].", $2 });
		}
	:));
	break;
    case "_request_version":
	unless (vars["_tag"]) vars["_tag"] = RANDHEXSTRING;
	source->chain_callback(vars["_tag"], (: 
		if ($3["@type"] == "result") {
		    // string v = "unknown";
		    XMLNode helper = $3["/query"];
		    if (helper["/name"]) 
			$2["_version_description"] = helper["/name"][Cdata]; 
		    else 
			$2["_version_description"] = "-";

		    if (helper["/version"]) 
			$2["_version"] = helper["/version"][Cdata];
		    else
			$2["_version"] = "-";

		    /*
		    if (helper["/os"]) {
			v += " on " + (helper["/os"][Cdata] || "unknown OS");
		    } */
		    return ({ $1, "_status_version", 
    "Version: [_nick] is using \"[_version_description]\" ([_version]).", 
			    $2 });
		}
		else 
		    return ({ $1, "_failure_unavailable_version", "Received no version from [_nick].", $2 });
	:));
	break;
    case "_request_user_amount":
	source->chain_callback(vars["_tag"], (: 
		if ($3["@type"] == "result" && $3["/query"]) {
		    XMLNode helper;
		    helper = $3["/query"];
		    if (helper["/stat"] && nodelistp(helper["/stat"])) 
			foreach(helper : helper["/stat"]) {
			    switch(helper["@name"]) {
			    case "users/total":
				$2["_amount_users_registered"] = helper["@value"];
				break;
			    case "users/online":
				$2["_amount_users_loaded"] = helper["@value"];
				break;
			}
		    }
		    return ({ $1, "_status_user_amount", 0, 
                        // generic formats shouldn't be hanging around in net/jabber...!?
                        // "There are [_amount_users_loaded] people loaded, [_amount_users_registered] accounts on this server.", 
			    $2 });
		} else 
		    return ({ $1, "_error_request_users_amount", 
			    "[_nick] does not support querying users amount.", 
			    $2 });

		    :));
	break;
    case "_request_list_feature":
	if (vars["_node"]) mc += "_node";
	source->chain_callback(vars["_tag"], (: 
		if ($3["@type"] == "result" && $3["/query"]) {
//		    string variant;
		    XMLNode helper = $3["/query"];

		    if (helper["/identity"]) {
			if (nodelistp(helper["/identity"])) {
			    // FIXME
			} else {
			    $2["_identity"] = helper["/identity"]["@category"] + "/" + helper["/identity"]["@type"];
			    $2["_name"] = helper["/identity"]["@name"];
#if 0 // nice variable, but please do something with it sometime soon  :)
			    switch(helper["/identity"]["@category"]) {
			    case "account":
				variant = "_person";
				break;
			    case "conference":
				variant = "_place";
				break;
			    case "headline":
				variant = "_newsfeed";
				break;
			    case "server":
				variant = "_server";
				break;
			    }
#endif
			}
		    }
		    $2["_list_feature"] = ({ });
		    if (helper["/feature"] && nodelistp(helper["/feature"]))
			foreach(helper : helper["/feature"]) {
			    $2["_list_feature"] += ({ helper["@var"] });
			}
		    return ({ $1, "_notice_list_feature",
			    "[_nick] is a [_identity] called [_name] offering features [_list_feature].",
			    $2 });

		} else 
		    return ({ $1, "_error_request_list_feature", 
			    "[_nick] does not support querying features.", 
			    $2 });

		    :));
	break;
    case "_request_list_item":
	if (vars["_node"]) mc += "_node";
	source->chain_callback(vars["_tag"], (: 
		if ($3["@type"] == "result" && $3["/query"]) {
		    XMLNode helper;
		    helper = $3["/query"];
		    if (helper["@node"]) $2["_node"] = helper["@node"];
		    $2["_list_item"] = ({ });
		    $2["_list_item_description"] = ({ });
		    $2["_list_item_node"] = ({ });
		    if (helper["/item"] && nodelistp(helper["/item"]))
			foreach(helper : helper["/item"]) {
			    $2["_list_item"] += ({ XMPP + helper["@jid"] });
			    $2["_list_item_description"] += ({ helper["@name"] });
			    $2["_list_item_node"] += ({ helper["@node"] });
			}
			// die darstellung ist fuer telnetter suboptimal...
		    return ({ $1, "_notice_list_item",
			    "[_nick] has lots of items: [_list_item_description].",
			    $2 });
		} else 
		    return ({ $1, "_error_request_list_item", 
			    "[_nick] does not support querying items.", 
			    $2 });

		    :));
	break;
    case "_request_registration":
	if (!vars["_username"]) { 
	    mc = "_request_registration_query";
	    source->chain_callback(vars["_tag"], (: 
		    if ($3["@type"] == "result" && $3["/query"]) {
			XMLNode helper;
			helper = $3["/query"];
			if (helper["/instructions"])
			    $2["_instructions"] = helper["/instructions"][Cdata];
			else 
			    $2["_instructions"] = "No instructions available.";

			return ({ $1, "_notice_registration",
				"[_nick] provides the following registration instructions: [_instructions]",
				$2 });

		    } else
			return ({ $1, "_error_query_registration", 
				"[_nick] does not support registration.",
				$2 });
			:));
	} else {
	    source->chain_callback(vars["_tag"], (: 
		    if ($3["@type"] == "result") {
			return ({ $1, "_status_registration",
				"Your registration at [_nick] was successful.",
				$2 });
		    } else
			return ({ $1, "_error_query_registration", 
				"[_nick] does not support registration.",
				$2 });
			:));
	}

	break;
    case "_request_ping":
	source->chain_callback(vars["_tag"], (:
		    $2["_time_ping"] = vars["_time_ping"];
		    if ($3["@type"] == "result") 
			return ({ $1, "_echo_ping",
				"[_nick] pongs you.", $2 });
		    else
			return ({ $1, "_failure_unsupported_ping", 
				"[_nick] does not support ping.", $2 });
			:));

	break;
    case "_request_description_time":
	source->chain_callback(vars["_tag"], (:
		    if ($3["@type"] == "result" && $3["/query"]) {
			$2["_time_idle"] = $3["/query"]["@@seconds"];
			return ({ $1, "_status_description_time", 
				"[_nick] has been alive about [_time_idle] ago.", $2 });
		    } else
			return ({ $1, "_failure_unsupported_description_time", 
				"[_nick] does not support querying idle time.",
				$2 });
			:));
	break;
#ifndef _flag_disable_module_authentication
    case "_request_authentication":
	// TODO: XEP 0070 says we should use <message/> when the recipient is a bare jid
	// 	but I prefer the iq method
	source->chain_callback(vars["_tag"], (:
		    if ($3["@type"] == "result")
			return ({ $1, "_notice_authentication", 0, $2 });
		    else
			return ({ $1, "_error_invalid_authentication", 0, $2 });
			:));
	break;
#endif
#ifndef _flag_disable_query_server
    case "_notice_list_feature":
    case "_notice_list_feature_person":
    case "_notice_list_feature_place":
    case "_notice_list_feature_server":
    case "_notice_list_feature_newsfeed":
	mixed id2jabber = shared_memory("disco_identity");
	mixed feat2jabber = shared_memory("disco_features");
	vars["_identity"] = id2jabber[vars["_identity"]] || vars["_identity"];
	vars["_list_feature"] = implode(map(vars["_list_feature"], 
					     (: return "<feature var='" + feat2jabber[$1] + "'/>"; :)), "");
	break;
#endif
    case "_notice_list_item":	
	t = "";
	// same stuff in user.c (what happened to code sharing?)
	for (int i = 0; i < sizeof(vars["_list_item"]); i++) {
	    t += "<item name='" + xmlquote(vars["_list_item_description"][i]) + "'";
	    if (vars["_list_item"] && vars["_list_item"][i])
		t += " jid='" + mkjid(vars["_list_item"][i]) + "'";
	    if (vars["_list_item_node"] && vars["_list_item_node"][i])
		t += " node='" + vars["_list_item_node"][i] + "'";
	    t += "/>";
	}
	vars["_list_item"] = t;
	break;
    case "_notice_headline_news":
	vars["_page_news"] = xmlquote(vars["_page_news"]);
	break;
//  case "_notice_received_email":
//	vars["_subject"] = xmlquote(vars["_subject"]);
//	vars["_origin"] = xmlquote(vars["_origin"]);
//	break;
    case "_status_place_description_news_rss":
	vars["_link_news_rss"] = xmlquote(vars["_link_news_rss"]);
	break;
    case "_status_description_person":
	mc = "_status_description_vCard";
	vars["_INTERNAL_data_XML"] = convert_profile(vars, 0, "jCard");
	break;
#ifndef ENTER_MEMBERS
    case "_status_place_members":       
        string skip, clashnick, placejid;
        array(string) rendered;
        mixed u;
        int i;

        P2(("_status_place_members from %O to %O\n", source, target))
        u = parse_uniform(target);
        if (u) clashnick = u[UUser];
        else clashnick = 0;
	// PARANOID? noooo
        rendered = ({array(string)})renderMembers(vars["_list_members"], vars["_list_members_nicks"], 1);
	placejid = mkjid(source);
        for(i = 0; i < sizeof(vars["_list_members"]); i++) {
            // see JEP-0045 6.3.3 last paragraph
            // probably their assumption is about a linear 
            // list where this is always sent to the last 
            // (ie newest member)
# ifdef USE_THE_RESOURCE
            if (stringp(vars["_list_members"][i])
		&& abbrev(vars["_list_members"][i], target))

# else       
            if (target == vars["_list_members"][i])
# endif      
            {   
                skip = vars["_list_members_nicks"][i];
                continue;
            }
            // here we also have to do collision decection
            // (local user with the same nick as remote user)
            // and adjust rendered[i] accordingly
            if (rendered[i] == clashnick) {
                // this only happens with local users
		rendered[i] = SERVER_UNIFORM +"~"+ clashnick;
            }
            render("_status_place_members_each", "", ([
		"_INTERNAL_target_jabber": vars["_INTERNAL_target_jabber"],
		"_INTERNAL_source_jabber": placejid + "/" + RESOURCEPREP(rendered[i]),
		"_nick": rendered[i],
		"_source_relay": mkjid(vars["_list_members"][i]),
		"_duty": "none" ]), source);
        }
        vars["_INTERNAL_source_jabber"] += "/" + skip;
        vars["_source_relay"] = mkjid(target);
        vars["_duty"] = "none";
        render("_status_place_members_self", "", vars, source);
        return 1;
#endif
    case "_status_presence_here":
    case "_notice_presence_here":
	// _notice_presence_here requires an additional jabberig
	// messsage at least if this is the login announce
	// TODO: this one is temporary
    case "_notice_presence_here_quiet":
#ifdef XMPPERIMENTAL
	emit("<presence from='" + vars["_INTERNAL_source_jabber"] + "'/>"
	     "<presence from='" + vars["_INTERNAL_source_jabber"] +
	     "' type='probe'/>");
	return 1;
#else
	// notiz: _request_status_person muss von der UNI erfolgen,
	// 	nicht von der UNL, damit die Antwort auch wieder
	// 	an diese geht
	// 	und eigentlich muss das nur beim logon erfolgen...
	P4(("%O _request_status_person to be done: %O,%O,%O,%O.. %O\n", ME, source,mc,data,target, vars))
	//
	// is this correct here, or do we need something smarter?  -lynX
	vars["_INTERNAL_target_jabber_bare"] = vars["_INTERNAL_target_jabber"];
	// apparently we get the bare one here, anyway.. so we could
	// throw out this special case of *_bare variable. then again,
	// maybe i am wrong. how can i ensure? TODO	-lynX
	msg(source, "_request_status_person", "", vars, showingLog, target);
#endif
	// TODO: we dont explicitly need this for CACHE PRESENCE
	// on the other hand syncing with jabber is much harder O(n)
	// than it is with psyc O(1)
	break;
    case "_message_public":
	// TODO: this needs to be applied to questions & actions also
	if (vars["_time_place"]) {
	    // see JEP-0045, 6.3.11 Discussion History
	    // and JEP-0091 Delayed Delivery
	    mc = "_message_public_history";
	    t = gmtime(vars["_time_place"]);
	    vars["_INTERNAL_time_place_jabber"] = JABBERTIME(t);
	} else if (!vars["_context"]) {
	    mc = "_request_message_public";
	}
	break;
    case "_request_execute":
	// oh yes, this is the jabber way to do it!!!!111!
	// taken from the xep 0045 "irc command mapping"
	mixed args = explode(data, " ");
	switch(args[0]) {
	case "topic": 
	    // <lynX> might as well convert to _request_do_topic but
	    // (a) we don't have that yet
	    // (b) might even share code with place/basic to do that
	    //     as any place needs to be able to understand both
	    //     spec'd commands (_do) and ad hoc commands (_execute)
	    mc = "_request_execute_topic";
	    data = ARGS(1);
	    break;
	case "kick": // TODO: we could add a callback for this
	    mc = "_request_execute_kick";
	    vars["_nick_target"] = is_formal(args[1]) ? parse_uniform(args[1])[UNick] : args[1];
	    vars["_reason"] = ARGS(2);
	    break;
	case "ban": // TODO: we could add a callback for this
	    // mh... we have a hard time finding out the real jid 
	    // of the participant, so this is deactivated for now
	    return 0;
	case "invite": 
	    // TODO: we should do invite via the places!
	    return 0;
	default:
	    return 0; // pushback
	}
	break;
    default:
	// reihenfolge bitte nach wahrscheinlichkeit der mc TODO ;)
	if (abbrev("_message_private", mc)) {
	    // generate echo here as jabber does  not provide echo (apart from MUC)
	    sendmsg(source, "_message_echo" + mc[8..], data, vars, 
		    target);
	} else if (abbrev("_notice_place_leave", mc)) {
	    /*
	    if (mc == "_notice_place_leave_invalid")
		vars["_INTERNAL_source_jabber"] = vars["_INTERNAL_source_jabber_bare"] + "/" + vars["_nick_local"];
	    */
	    mc = "_notice_place_leave"; // remove this if textdb inheritance works
	} else if (abbrev("_notice_place_enter", mc)) {
	    vars["_source_relay"] = mkjid(source);
	    mc = "_notice_place_enter";
	} else if (abbrev("_echo_place_enter", mc)) {
#if 0
	    vars["_source_relay"] = mkjid(target);
	    mc = "_echo_place_enter";
	    vars["_duty"] = "none";
#else
# ifdef ENTER_MEMBERS
	    if (vars["_list_members"]) {
		string placejid = mkjid(source);
		string *rendered;
	    // i wish we could leave the clash strategy to mkjid...
		// PARANOID? noooo
		rendered = ({array(string)})renderMembers(vars["_list_members"], vars["_list_members_nicks"], 1);
		for(int i = 0; i < sizeof(vars["_list_members"]); i++) {
		    // here we also have to do collision decection
		    // (local user with the same nick as remote user)
		    // and adjust rendered[i] accordingly
		    if (rendered[i] == vars["_nick"]) {
			// this only happens with local users
			rendered[i] = SERVER_UNIFORM +"~"+ rendered[i];
		    }
		    render("_notice_place_enter", "",
		   ([ "_INTERNAL_target_jabber": vars["_INTERNAL_target_jabber"],
		      "_INTERNAL_source_jabber" : placejid + "/" + RESOURCEPREP(rendered[i]),
		      "_nick" : rendered[i],
		      "_source_relay" : mkjid(vars["_list_members"][i]),
		      "_duty" : "none" ]),
		    source);
		}
	    }
	    vars["_source_relay"] = mkjid(target);
	    vars["_duty"] = "none";
	    /*
	    vars["_INTERNAL_source_jabber"] = mkjid(source,
		 ([ "_nick" : vars["_nick_local"] || vars["_nick"] ]),
		 0, 0, target),
		 */
	    render("_echo_place_enter", "", vars, source);
# endif
	    return 1;
#endif
	} else if (abbrev("_error_place_enter", mc)) {
	    mc = "_error_place_enter";
	    // can we render data string here?
	    data = psyctext("", vars, data, source);
	    // vielleicht sollte man hier das ganze nochmal als 
	    // msg() hinterherschieben, irgendwie finden jabba-clients
	    // es nicht notwendig, den text zu zeigen...
	    // traurig eigentlich
	} else if (abbrev("_notice_presence_away", mc)) {
	    // mh... for muve2muve it would be better to 
	    // extend jabber with an 'automatic' flag
	    // but they could use psyc anyway :)
	    mc = "_notice_presence_away";
	}
	else if (abbrev("_request_enter", mc)       /* remote jabber join */
                || abbrev("_request_context_enter", mc)) {
	    mc = "_request_enter";
#ifdef MUCSUC
	    t = XMPP+ vars["_INTERNAL_target_jabber_bare"]
		     +MUCSUC_SEP+ vars["_nick"];
	    P4(("MUCSUC/render1: %O of %O\n", t, 0 && vars))
	    set_context(clone_object(JABBER_PATH "remotemuc"), t);
#endif
	    source->chain_callback(vars["_tag"], (: 
		P3(("remote jabber part echo with %O!\n", $2)) 
		// FIXME: 
		// 	if there is a 201 status code we need 
		// 	to send extra mumbo jumbo
		$2["_tag_reply"] = $3["@id"];
		$2["_nick_place"] = $2["_INTERNAL_identification"];
#ifdef MUCSUC
		$2["_source_relay"] = source;
		t = XMPP+ vars["_INTERNAL_target_jabber_bare"]
			 +MUCSUC_SEP+ vars["_nick"];
		P4(("MUCSUC/render2: %O of %O\n", t, 0 && vars))
		// echo place enter has not context (yet)
		//$2["_context"] = vars["_INTERNAL_target_jabber_bare"] + ...
//		m_delete($2, "_nick");
#else
		t = $1;
#endif
		if ($3["@type"] == "error") {
		    // FIXME: could remove context
		    //
		    // also, we should implement the full choice of errors and
		    // map them to appropriate psyc errors.. instead we just
		    // have this silly lazy coder's message:
		    return ({ t, "_failure_place_enter_XMPP", 
			    "[_nick_place] could not be entered for jabberish reasons.",
			    $2 });
		} else {
#ifdef MUCSUC
		    return ({ t, "_echo_place_enter", 0, $2 });
#else // ugly old code
		    // since we fake _context we also have to use _notice, not _echo
		    // can we send _echo_place_enter_unicast and instead
		    // make sure we don't provide a _context?
		    return ({ $1, "_notice_place_enter_unicast_INTERNAL_ECHO",
			"[_nick] enters a unicast chatroom at [_nick_place].", 
			    $2 });
#endif
		}
	    :));
	} else if (abbrev("_request_leave", mc)
                || abbrev("_request_context_leave", mc)) {
	    mc = "_request_leave";
	    source->chain_callback(vars["_tag"], (: 
		P3(("remote jabber part echo with %O!\n", $2)) 
		$2["_tag_reply"] = $3["@id"];
		$2["_nick_place"] = $2["_INTERNAL_identification"];
		$2["_nick"] = $2["_INTERNAL_source_resource"];
#ifdef MUCSUC
		// single-user channels on top of xmpp URI scheme.. funny
		// whatever does the job.
		$2["_context"] = XMPP+ vars["_INTERNAL_target_jabber_bare"]
			    // should i try $2["_nick"] instead?
			    +MUCSUC_SEP+ vars["_nick"];
		P3(("MUCSUC/render3: %O\n", $2["_context"]))
#else
		$2["_context"] = $2["_INTERNAL_identification"];
#endif
		return ({ $1,
			    "_notice_place_leave", 
			    "You leave [_nick_place].", 
			    $2 });
	    :));
	} /* remote jabber join */
	else if (abbrev("_message_announcement", mc)) return 1;
	else if (abbrev("_failure_redirect", mc)) {
	    if (vars["_tag_reply"]) { // wild guess that it is an iq then 
		mc = "_jabber_iq_error";
// <lynX> was spricht dagegen _failure_redirect als <redirect/> auszugeben?
// <fippo> ich denke nicht, dass es irgendwer vernünftig implementiert...
//	außerdem musst du die jid des raumes in dem konkreten fall rausfinden
		vars["_jabber_XML"] = "<error type='modify'><gone xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'/><text xmlns='urn:ietf:params:xml:ns:xmpp-stanzas' xml:lang='en'>" + psyctext(data, vars) + "</text></error>";
	    }
	}
    }
    render(mc, data, vars, source);
    return 1;
}
