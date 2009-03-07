// $Id: root.c,v 1.36 2008/10/01 10:59:47 lynx Exp $ // vim:syntax=lpc
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

msg(source, mc, data, vars, showingLog, target) {
	mapping rv = ([ "_nick" : SERVER_UNIFORM ]);
	mixed t;
	string family;
	int glyph;

	if (vars["_tag"]) rv["_tag_reply"] = vars["_tag"];
	unless (source) source = vars["_INTERNAL_source"]
			      || vars["_INTERNAL_origin"];

	P2(("%O msg(%O, %O, %O, %O, %O)\n",
	    ME, source, mc, data, vars, target))
	PSYC_TRY(mc) {
#ifndef _flag_disable_module_authentication
	case "_notice_authentication":
		P0(("%O got a _notice_authentication. never happens since entity.c\n", ME))
		register_location(vars["_location"], source, 1);
		return 1;
	case "_error_invalid_authentication":
		monitor_report(mc, psyctext("Breach: [_source] reports invalid authentication provided by [_location]", vars, data, source));
		return 1;
#endif
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
		rv["_list_feature"] = ({
			// pidgin sends _request_list_item even
			// if list_item was not negotiated..
			// so we might aswell give up trying not to provide it
				        "list_item",
#ifndef _flag_disable_query_server
				       	"version",    // _tab
					"time", "lasttime"
#endif
#if !defined(REGISTERED_USERS_ONLY) && !defined(_flag_disable_registration_XMPP)
					"registration",
#endif
#ifndef VOLATILE
					"offlinestorage",
#endif
					"list_feature"		// _tab
		}); 
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
#ifndef _flag_disable_query_server
	// move to a common msg() for all entities including root?
	case "_request_version":
		rv["_version_description"] = SERVER_DESCRIPTION;
		rv["_version"] = SERVER_VERSION;
		sendmsg(source, "_status_version",
    "Version: [_nick] is hosted on a \"[_version_description]\" ([_version]).", rv);
		return 1;
        // same code in person.c
	case "_request_ping":
		sendmsg(source, "_echo_ping",
				"[_nick] pongs you.", rv);
		return 1;
	case "_request_description_time":
# ifdef __BOOT_TIME__	// all recent ldmuds have this
		rv["_time_boot"] = __BOOT_TIME__;
		rv["_time_boot_duration"] = time() - __BOOT_TIME__;
		sendmsg(source, "_status_description_time", 0, rv);
# else
		sendmsg(source, "_error_request_description_time", 0, rv);
# endif
		return 1;
	case "_query_users_amount":	// old, please remove
	case "_request_user_amount":
# ifdef _flag_disable_query_amount_users_online
		rv["_amount_users_loaded"] = -1;
# else
		// this actually shows the number of loaded user entities
		// which is higher than the actual number of users online
		rv["_amount_users_loaded"] = amount_people();
# endif
		rv["_amount_users_registered"] = -1; // how to get this?
		// <kuchn> maybe read in the user directory so you have the amount of registered users.
		// <lynX> i think it isn't anybody's business...
		//	  in the name of privacy we should be
		//	  giving out random numbers instead of -1..  ;)
		sendmsg(source, "_status_user_amount", 0, rv);
    // ircds tell a lot of things in reply to /lusers:
    //
    // 251 x :There are 25079 listed and 22070 unlisted users on 38 servers
    // 252 x 39 :flagged staff members
    // 254 x 22434 :channels formed
    // 255 x :I have 2180 clients and 0 servers
    // 265 x :Current local  users: 2180  Max: 2714
    // 266 x :Current global users: 47149  Max: 55541
    // 250 x :Highest connection count: 2715 (2714 clients) (228789 since server was (re)started)
		return 1;
#endif // _flag_disable_query_server
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
	case "_request_legacy_CTCP":
//		sendmsg(source, "_status_legacy_CTCP", 0,
//			    ([ "_type": vars["_type"],
//			      "_value": "You must be mislead." ]));
		// fall thru
	case "_status_legacy_CTCP":
		P1(("%O got CTCP %O from %O: %O\n", ME, vars["_type"],
		    source, vars["_value"]))
		return 1;
	PSYC_SLICE_AND_REPEAT
	}
	rv["_method_relay"] = mc;
	rv["_target_relay"] = target || vars["_target"];
	rv["_source_relay"] = source || vars["_source"];
	if (vars["_context"]) {
		// we get here when one or more local recipients have been
		// inserted into a context by local hostname. group/master
		// will then expect a context slave here, but we don't have
		// slaves for local objects. this is a data or configuration
		// mistake - local objects should have been resolved into
		// object pointers before getting here. this can however
		// happen when moving account data between server installations,
		// or when a user intentionally inserts a local user by full
		// url. that should be handled.. TODO
		rv["_context_relay"] = vars["_context"];
		P1(("Invalid route in context: msg(%O, %O, %O, %O, %O) -> %O\n",
		    source, mc, data, vars, target, rv))
		sendmsg(source, "_failure_invalid_route", 0, rv);
		return 1;
	} else {
		P1(("%O unexpected msg(%O, %O, %O, %O, %O)\n",
		    ME, source, mc, data, vars, target))
		sendmsg(source, "_failure_unsupported_function_root", 0, rv);
	}
	return 0;
}
