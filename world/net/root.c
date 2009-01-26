// $Id: root.c,v 1.30 2008/03/11 13:42:25 lynx Exp $ // vim:syntax=lpc
//
// system root object that responds to psyc://host/ and similar addresses
//
// TODO: move rootMsg() from common.c into here, or the other way around
//
#include <net.h>
#include <psyc.h>
//
// we don't make use of this yet, but since the server root is an entity
// too, and it needs to deal with its own server trust network, we should
// use this.
//inherit NET_PATH "entity"

msg(source, mc, data, vars, target) {
	mapping rv = ([ "_nick" : query_server_unl() ]);
	mixed t;
	string family;
	int glyph;

	if (vars["_tag"]) rv["_tag_reply"] = vars["_tag"];
	unless (source) source = vars["_INTERNAL_source"]
			      || vars["_INTERNAL_origin"];

	PT(("%O msg(%O, %O, %O, %O, %O)\n",
	    ME, source, mc, data, vars, target))
	PSYC_TRY(mc) {
	case "_notice_authentication":
		P0(("%O got a _notice_authentication. never happens since entity.c\n", ME))
		register_location(vars["_location"], source, 1);
		return 1;
	case "_error_invalid_authentication":
		monitor_report(mc, psyctext("Breach: [_source] reports invalid authentication provided by [_location]", vars, data, source));
		return 1;
	    // the following two requests look like PSYC but they are
	    // actually implementing XMPP disco features. it is as yet
	    // undecided, whether PSYC should solve this type of necessity
	    // in the same way or come up with something different -
	    // more of the subscribe and push type.
	case "_request_list_feature":
		rv["_identity"] = "chatserver";
#ifdef CHATNAME
		rv["_name"] = CHATNAME;
#else
		rv["_name"] = SERVER_VERSION;
#endif
		rv["_list_feature"] = ({ "list_feature", "list_item", "version",    // _tab
					"time", "lasttime" }); 
#ifndef REGISTERED_USERS_ONLY
		rv["_list_feature"] += ({ "registration" });
#endif
#ifndef VOLATILE
		rv["_list_feature"] += ({ "offlinestorage" });
#endif
		sendmsg(source, "_notice_list_feature_server",
			"[_nick] is a [_identity] called [_name] offering features [_list_feature].",
			rv);
		return 1;
	case "_request_list_item":
#ifdef PUBLIC_PLACES
		t = advertised_places();
		rv["_list_item"] = allocate(sizeof(t)/2);   // _tab
		rv["_list_item_description"] = allocate(sizeof(t)/2);   // _tab
		
		for (int i = 0; i < sizeof(t)/2; i++) {
		    rv["_list_item"][i] = is_formal(t[2*i]) ? t[2*i] : find_place(t[2*i]);
		    rv["_list_item_description"][i] = t[2*i + 1];
		}
#else
		rv["_list_item"] = ({ });
		rv["_list_item_description"] = ({ });
#endif
		sendmsg(source, "_notice_list_item", 
			"[_nick] has lots of items: [_list_item_description].",
			rv);
		return 1;
	// move to a common msg() for all entities including root?
	case "_request_version":
		rv["_version_description"] = SERVER_DESCRIPTION;
		rv["_version"] = SERVER_VERSION;
		sendmsg(source, "_status_version",
    "Version: [_nick] is hosted on a \"[_version_description]\" ([_version]).", rv);
		return 1;
	case "_request_ping":
		sendmsg(source, "_echo_ping",
				"[_nick] pongs you.", rv);
		return 1;
	case "_request_description_time":
#ifdef __BOOT_TIME__	// all recent ldmuds have this
		rv["_time_boot"] = __BOOT_TIME__;
		rv["_time_boot_duration"] = time() - __BOOT_TIME__;
		sendmsg(source, "_status_description_time", 0, rv);
#else
		sendmsg(source, "_error_request_description_time", 0, rv);
#endif
		return 1;
	case "_query_users_amount":	// old, please remove
	case "_request_user_amount":
#ifdef NO_MARKETING // they dont like giving this numbers to everyone :-)
		rv["_amount_users_online"] = amount_people();
#else
		rv["_amount_users_online"] = -1;
#endif
		rv["_amount_users_registered"] = -1; // how to get this?
		// <kuchn> maybe read in the user directory so you have the amount of registered users.
		sendmsg(source, "_status_user_amount",
		    "There are [_amount_users_online] users online.", rv);
		return 1;
	case "_error":
	case "_warning":
	case "_failure":
	case "_failure_unsuccessful_delivery":
	case "_failure_unsuccessful_delivery_resolve":
	case "_failure_unsupported_function_root":
		t = "Root got "+ (vars["_method_relay"] || mc);
		if (vars["_target_relay"]) t += " to "+ to_string(vars["_target_relay"]);
		if (vars["_source_relay"]) t += " from "+ to_string(vars["_source_relay"]);
		if (source) t += " via "+ to_string(source);
		//monitor_report("_notice_forward"+ mc, psyctext(data ?
		//		 t+": "+data : t, vars, "", source));
		monitor_report("_notice_forward"+ (vars["_method_relay"]
					   || mc || "_method_missing"), t);
		return 1;
	case "_notice_forward":
		P1(("%O got a %O.. eh!???\n", ME, mc))
		return 1;
	PSYC_SLICE_AND_REPEAT
	}
	P1(("%O unexpected msg(%O, %O, %O, %O, %O)\n",
	    ME, source, mc, data, vars, target))
	rv["_method_relay"] = mc;
	rv["_target_relay"] = target;
	rv["_source_relay"] = source;
//              sendmsg(source, "_error_unsupported_method",
//                      "Don't know what to do with '[_method]'.",
//                      ([ "_method" : mc ]), "");
	sendmsg(source, "_failure_unsupported_function_root", 0, rv);
	return 0;
}
