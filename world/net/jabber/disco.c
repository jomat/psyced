// $Id: disco.c,v 1.43 2008/09/12 15:54:38 lynx Exp $ // vim:syntax=lpc
//
// this gets included by user.c and gateway.c. you can distinguish this
// by ifdeffing USER_PROGRAM. it may be renamed into disco.i one day.
//
//#include <net.h>
//#include "jabber.h"

#ifdef USER_PROGRAM
# define SEND(TARGET, MC, DATA, VARS, SOURCE) w(MC, DATA, VARS, SOURCE)
#else
# define SEND(TARGET, MC, DATA, VARS, SOURCE) sendmsg(TARGET, MC, DATA, VARS, SOURCE)
#endif

/* the WHOLE disco stuff in here should be solved by psycig 
 * messages instead of dirty call_other
 */
disco_info_root(vars) {
    string featurelist;
    featurelist = "<feature var='http://jabber.org/protocol/muc'/>"
#ifndef REGISTERED_USERS_ONLY
# ifndef _flag_disable_registration_XMPP
		"<feature var='jabber:iq:register'/>"
# endif
#endif
#ifndef VOLATILE
		"<feature var='msgoffline'/>"
#endif
#ifndef ERQ_WITHOUT_SRV
		"<feature var='dnssrv'/>"
#endif
#ifdef __IDNA__
		"<feature var='stringprep'/>"
#endif
#ifdef WANT_S2S_SASL
		"<feature var='" NS_XMPP "xmpp-sasl#s2s'/>"
#endif
		"<feature var='jabber:iq:last'/>"
		"<feature var='jabber:iq:version'/>"
		"<feature var='jabber:iq:time'/>"
		"<feature var='vcard-temp'/>"
		"<feature var='" NS_XMPP "xmpp-sasl#c2s'/>";
#if __EFUN_DEFINED__(tls_available)
    if (tls_available()) {
	featurelist += "<feature var='" NS_XMPP "xmpp-tls#c2s'/>"
		"<feature var='" NS_XMPP "xmpp-tls#s2s'/>";
#if HAS_PORT(JABBERS_PORT, JABBER_PATH)
	featurelist += "<feature var='sslc2s'/>";
#endif
    }
#endif
    vars["_list_features"] = featurelist;
}

disco_info_place(name, vars) {
    vars["_nick_place"] = name;
    vars["_list_features"] = "<feature var='http://jabber.org/protocol/muc'/>";
    // potential features are listed on jabber.org/registrar/disco-features.htm
    // useful for us:
    // muc_membersonly, muc_nonanonymous, muc_open,
    // muc_persistent, muc_public, muc_temporary,
    // muc_unmoderated, 
    // muc_unsecure (does not have a password!)
    // we should query the room about these
    // object o = find_place(name);
}

disco_info_person(name, vars) {
    string featurelist;

    /* we could query the user for certain information... */
    vars["_nick"] = name; // publicname?

    featurelist = "<feature var='vcard-temp'/>"
	    // http://www.jabber.org/jeps/jep-0096.html -- File Transfer
	    // is handled by the new iq innerxml trick. probably more
	    // JEPs can be implemented this way. some may actually already
	    // work now and we only need to add their <feature> here
	    // I am not sure if the server should answer here...
	    // this probably needs to be done by the jabber client
	    // (which can receive disco's also)
// how should oob() ever get to do its work if it doesn't say oob here?
// oob is proxied to client
#if 0
	    "<feature var='jabber:iq:oob'/>"
	    "<feature var='jabber:x:oob'/>"
#endif
//	    "<feature var='http://jabber.org/protocol/si/profile/file-transfer'/>"
	    ;
    vars["_list_features"] = featurelist;
}

disco_items_root(vars) {
    // TODO: 
    // here we should use the list of chatrooms from
    // CONFIG_PATH "places.h"
#ifdef PUBLIC_PLACES
    // see also: library advertised_places()
#endif
    // is it safe to use _host_XMPP here?
    vars["_list_item"] = "<item name='Chatrooms' jid='" _host_XMPP "'/>";
}

disco_items_place(name, vars) {
    // unused currently
    vars["_list_item"] = "";
}

disco_items_person(name, vars) {
    // possibly used in that pubsub thingie
    vars["_list_item"] = "";
}

#if 0 // you loose if you dont use transparency :-)
// http://www.jabber.org/jeps/jep-0066.html -- Out of Band Data
//
// der job ist es den mist durchzureichen, leider auch nicht trivial
// this might work, haven't used a client who uses it. the templates
// to render it back into jabber aren't done yet.
oob(XMLNode node, mapping vars) {
	string source, target;
	mixed *u;

	unless (vars) vars = ([ ]);
	vars["_INTERNAL_target_jabber"] = target = node["@to"];
	vars["_INTERNAL_source_jabber"] = source = node["@from"];
	vars["_nick"] = source; // too lazy to parse now

	if (node["/query"]) {
		vars["_uniform"] = node["/query"]["@url"];
		vars["_description"] = node["/query"]["@desc"];
	}
	switch(node["@type"]) {
	case "set":
		SEND(XMPP + target, "_notice_available_uniform",
		     "[_nick] points you to [_uniform]: [_description].",
		    vars, XMPP + source);
		break;
	case "result":
		// jabber only provides the iq-id here. no filenames.
		SEND(XMPP + target, "_notice_accepted",
		    "[_nick] has accepted something.",
		    vars, XMPP + source);
		break;
	case "error":
		switch(node["/error"]["@type"]) {
		case "cancel":
			SEND(XMPP + target,
			     "_error_rejected_delivery_uniform",
			    "[_nick] has not accepted [_uniform].",
			    vars, XMPP + source);
			break;
		default:
			SEND(XMPP + target,
			     "_failure_unsuccessful_delivery_uniform",
			    "[_nick] has not received [_uniform].",
			    vars, XMPP + source);
		}
		break;
	}
}
#endif
