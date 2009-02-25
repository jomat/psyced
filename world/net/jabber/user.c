// $Id: user.c,v 1.303 2008/09/12 15:54:39 lynx Exp $ // vim:syntax=lpc:ts=8
#include "jabber.h"
#include "user.h"
#include "person.h"
#include <url.h>
#include <peers.h>

// important to #include user.h first
// then we also repatch _host_XMPP so disco.c does the right thing for us
#undef _host_XMPP
#define _host_XMPP SERVER_HOST

volatile string prefix;	// used anywhere?
volatile string tag;
volatile string resource; 
volatile string myjid, myjidresource; 

volatile int isplacemsg;

volatile int hasroster = 0; // client has requested roster

volatile mapping affiliations = ([ ]); // hack to support affiliations

#include JABBER_PATH "disco.c"
// #include NET_PATH "members.i"    // isn't this include redundant?

sTag(t) { tag = t; }
sResource(r) { resource = r; }
qResource() { return resource; }

qHasCurrentPlace() { return 0; }

showFriends() {}

/* it should be posible to do some things in a way that can be shared between
 * user and active/gateway... mostly stuff like MUC-support, version requests
 * and such -- TODO
 * what should be strictly in user.c is mainly roster managment
 */

msg(source, mc, data, mapping vars, showingLog) {
    int ret;
    string jid, buddy;
    string packet;
    mixed t;

    P2(("%s beim jabber:user:msg\n", mc))
    // net/group/master says we should copy vars if we need to
    // mess with it. yes we certainly do! major bugfix here :)
    vars = copy(vars);

    // looks quite similar to the stuff in active.c, but active 
    // is much more elaborated
    if (vars["_place"]) vars["_place"] = mkjid(vars["_place"]);

    if (abbrev("_message_echo_public", mc) || abbrev("_jabber", mc)) {
	return render(mc, data, vars, source);
    } else if (abbrev("_message_echo_private", mc) && v("echo") != "on") {
	return;
    } else if (abbrev("_notice_place_enter", mc)) {
	mc = "_notice_place_enter";
	vars["_source_relay"] = mkjid(source);
    } else if (abbrev("_echo_place_enter", mc)) {
#if 1
	// we need ::msg to do the check only - this is a dirty hack
	// see ::msg _echo_place_enter comment on enterMembers
	if (::msg(source, "_echo_place_enter_INTERNAL_CHECK", data, vars, showingLog) == -1)
	    return;
	// there probably is a nicer way to do this, but for now this will do
#endif
#if 0
	affiliations[source] = vars["_duty"];
	mc = "_echo_place_enter";
	vars["_source_relay"] = myjid;
#else
# ifdef ENTER_MEMBERS
	string template;
	if (vars["_list_members"]) {
	    jid = mkjid(source);
	    array(string) rendered;
	    // find local objects, they should be displayed as locals
	    vars["_list_members"] = map(copy(vars["_list_members"]), 
					(: return psyc_object($1) || $1; :));
	    // PARANOID check missing, but we no longer need it
	    rendered = renderMembers(vars["_list_members"],
				     vars["_list_members_nicks"]);
	    packet = "";

	    // this one is different from active.c
	    // we want local users to be shown with nicknames, even
	    // if we are in a remote room
	    // TODO: there does not seem to be proper support for 
	    // 		room-local nicknames and clashnick 
	    // 		(this is already in active.c)
	    // 		this also affects other places... probably it's best
	    // 		to include the 210 status code?
	    template = T("_notice_place_enter", "");
	    for(int i = 0; i < sizeof(vars["_list_members"]); i++) {
#if 0
		if (rendered[i] == vars["_nick"]) {
		    // this only happens with local users
		    rendered[i] = SERVER_UNIFORM +"~"+ rendered[i];
		}
#endif
		packet += psyctext(template, ([ 
			"_source_relay" : mkjid(vars["_list_members"][i]),
			"_INTERNAL_source_jabber" : jid + "/" + RESOURCEPREP(rendered[i]),
			"_INTERNAL_target_jabber" : myjidresource,
			"_duty" : "none"
			]));
	    } 
	}
	template = T("_echo_place_enter", "");
	packet += psyctext(template, ([
			"_source_relay" : myjid,
			"_INTERNAL_source_jabber" : jid + "/" + RESOURCEPREP(vars["_nick_local"] || vars["_nick"]),
			"_INTERNAL_target_jabber" : myjidresource,
			"_duty" : affiliations[source] || "none"
			]));
	emit(packet);
# endif
	return 1;
#endif
    } else if (abbrev("_notice_place_leave", mc)) {
	mc = "_notice_place_leave";
    }
    switch (mc) {
    case "_status_person_present":
    case "_status_person_present_implied":
    case "_status_person_absent":
    case "_status_person_absent_recorded":
	PT(("%O got %O\n", ME, mc))

	// actually.. we never send _time_idle with this
        if (member(vars, "_time_idle")) {
            t = vars["_time_idle"];
            if (stringp(t)) {
                t = to_int(t);
                PT(("_time_idle %O == %O, right?\n", vars["_time_idle"], t))
            }
            t = gmtime(time() - t);
            vars["_INTERNAL_time_jabber"] = JABBERTIME(t);
        } else {
            vars["_INTERNAL_time_jabber"] = JABBERTIME(gmtime(time()));
        }
	break;
    case "_notice_friendship_established":
	// TODO:
	// it should be checked that this request is valid
	// but for this hack xbuddylist[buddy] != 0 is enough
	ret = ::msg(source, mc, data, vars, showingLog);
	buddy = objectp(source) ? source -> qName() : source;
	jid = mkjid(source);
	emit(sprintf("<iq type='set'>"
		     "<query xmlns='jabber:iq:roster'>"
		     "<item jid='%s' subscription='both'>%s"
		     "</item></query></iq>", 
		     jid, IMPLODE_XML(xbuddylist[buddy], "<group>") || ""));
	return ret;
    case "_notice_friendship_removed":
	ret = ::msg(source, mc, data, vars, showingLog);
	buddy = objectp(source) ? source -> qName() : source;
	jid = mkjid(source);
	// is this supposed to delete the groups as well?
	// ::msg will delete this person from ppl so if
	// we want to use pplgroups for channel routing
	// we need to keep the two structures in sync..
	// sind sie... wenn das hier dazu führt, dass 
	// der status im ppl auf "nix subscribed" gesetzt
	// wird statt es zu loeschen
	emit("<iq type='set'>"
	     "<query xmlns='jabber:iq:roster'>"
	     "<item jid='"+ jid +"' subscription='none'>"
	     "</item></query></iq>");
	if (xbuddylist[buddy]) {
	    m_delete(xbuddylist, buddy);
	}
	return ret;
# ifndef ENTER_MEMBERS
    case "_status_place_members":
	int i;  
	string skip;
	array(string) rendered;
	string template, packet;
											// this would have been done by ::msg
#ifdef PARANOID
	unless (pointerp(vars["_list_members"])) 
	    vars["_list_members"] = ({ vars["_list_members"] });
#endif
				// copy still necessary here? TODO
	vars["_list_members"] = map(copy(vars["_list_members"]),
				    (: return psyc_object($1) || $1; :));
#ifdef PARANOID
	unless (pointerp(vars["_list_members_nicks"]))
	    vars["_list_members_nicks"] = ({ vars["_list_members_nicks"] });
#endif
	rendered = renderMembers(vars["_list_members"],
				 vars["_list_members_nicks"]);
	packet = "";
	
	// this one is different from active.c
	// we want local users to be shown with nicknames, even
	// if we are in a remote room
	jid = mkjid(source);
	template = T("_status_place_members_each", "");
	for(i = 0; i < sizeof(vars["_list_members"]); i++) {
	    if (vars["_list_members"][i] == ME) {
		skip = vars["_list_members_nicks"][i];
		continue;
	    }
	    packet += psyctext(template, ([ 
			"_source_relay" : mkjid(vars["_list_members"][i]),
			"_INTERNAL_source_jabber" : jid + "/" + RESOURCEPREP(rendered[i]),
			"_INTERNAL_target_jabber" : myjidresource,

			"_duty" : "none"
			]));
	}
	template = T("_status_place_members_self", "");
	packet += psyctext(template, ([
			"_source_relay" : myjid,
			"_INTERNAL_source_jabber" : jid + "/" + RESOURCEPREP(vars["_nick_local"] || vars["_nick"]),
			"_INTERNAL_target_jabber" : myjidresource,
			"_duty" : affiliations[source] || "none"
			]));
	emit(packet);
	return;
# endif
    }
    // ::msg should get called at the *beginning* of this function
    // but that would cause some new problems..
    return ::msg(source, mc, data, vars, showingLog);
}

logon() {
    // language support is in server.c
    vSet("scheme", "jabber");
    vDel("layout");
    vDel("agent");
    vDel("query");  // server-side query would drive jabbers crazy as well
    vDel("place");  // we do use this, but we don't autojoin it
#ifdef INPUT_NO_TELNET
    input_to(#'feed, INPUT_IGNORE_BANG | INPUT_CHARMODE | INPUT_NO_TELNET);
#else
    // enable_telnet(0, ME);
    input_to(#'feed, INPUT_IGNORE_BANG | INPUT_CHARMODE);
#endif
    nodeHandler = #'jabberMsg;
    set_prompt("");
    myjid = NODEPREP(MYLOWERNICK) +"@" + NAMEPREP(SERVER_HOST);
    myjidresource = myjid +"/"+ RESOURCEPREP(resource);
    P2(("%O ready to rumble (%O)\n", myjidresource, ME))
    return ::logon();
}

autojoin() {
    // jabber as a protocol is unable to recognize a forced join
    // into a room. there is a method for autojoin, but its 
    // client-based and does not work this way
    return 1;
}

jabberMsg(XMLNode node) {
    switch (node[Tag]) {
    case "iq":
	tag = node["@id"];
	return iq(node);
    case "message":
	return message(node);
    case "presence":
	return presence(node);
    default:
	return ::jabberMsg(node);
    }
}

presence(XMLNode node) {
    // this ignores the resource completely
    // but as long as it works...
    // we could check that... but hey...
    string target, host, res, nick, text;

    isplacemsg = ISPLACEMSG(node["@to"]);
    // note: same code is in mixin_parse...
    if (!isplacemsg && getchild(node, "x", "http://jabber.org/protocol/muc#user")) {
	isplacemsg = 2;
    }
#ifndef _flag_disable_presence_directed_XMPP
    if (node["@to"]) {
	target = jid2unl(node["@to"]);
	if (isplacemsg) {
	    mixed *u = parse_uniform(XMPP + node["@to"]);
	    P2(("some kind of place stuff\n"))
	    if (node["@type"] == "unavailable") {
		P2(("requesting to leave %O\n", target))
		placeRequest(target,
# ifdef SPEC                  
                             "_request_context_leave"
# else                        
                             "_request_leave"
# endif
                            , 1);	
		place = 0; // should we do it when we receive the notice?
			  // anyway, w/out this we show up as still being
			 // in that room in /p
	    } else {
# ifdef ENTER_MEMBERS
		// TODO: this might be needed and should work for remote rooms
		// doing it in local rooms is a bad idea
		if (is_formal(target))
		    placeRequest(target,
#  ifdef SPEC
                         "_request_context_enter"
#  else
                         "_request_enter"
#  endif
                        "_again", 0, 1, ([
			"_nick" : MYNICK,
			"_nick_local" : u[UResource]
			]));
		else
		    placeRequest(target,
#  ifdef SPEC
                                 "_request_context_enter"
#  else
                                 "_request_enter"
#  endif
                                 "_again", 0, 1);
# else
		P2(("teleporting to %O\n", target))
		teleport(target, "_join", 0, 1);
# endif
	    }
# ifndef _flag_disable_module_friendship
	} else if (node["@type"] == "subscribe") {
	    // was: friend(({ jid2unl(node["@to"]) }), 0);
	    friend(0, jid2unl(node["@to"]));
	} else if (node["@type"] == "unsubscribe") {
	    friend(1, jid2unl(node["@to"]));
# endif // _flag_disable_module_friendship
	} else if (abbrev(XMPP, target)) {
	    // if the person is not on our buddylist,
	    // this is usually a muc join
	    // but i am not sure if there are other uses of
	    // presence in jabber
# ifdef JABBER_TRANSPARENCY
	    mapping vars = ([ "_nick" : MYNICK ]);
	    mixed *u = parse_uniform(XMPP + node["@to"]);
	    P3(("jtranz presence to %O\n", target))

	    // TODO: presence out! im prinzip dem places mapping
	    // funktinal sehr aehnlich, sofern das ziel nicht 
	    // auf unserer buddylist ist
	    // wir muessen halt beim logout unavailable schicken
	    // an alle auf der liste
//	    unless(mappingp(presence_out)) presence_out = ([ ]);
	    vars["_jabber_XML"] = innerxml;

	    // TODO: wir fliegen wir mit _host_XMPP auf die 
	    // 	NASE. 
	    //	wir muessen die resource in die vars stecken...
	    vars["_INTERNAL_source_jabber"] = myjidresource;
	    if (node["@type"] == "unavailable") {
		// TODO: delete this from presence out
		sendmsg(target,
			"_jabber_presence_unavailable",
			"[_nick] is sending you a jabber presence of type unavailable.",
			vars);
	    } else {
		// TODO: add this to presence out
		sendmsg(target,
			"_jabber_presence",
			"[_nick] is sending you a jabber presence.",
			vars);
	    }
# endif
	} else {
	// TODO: what can we do in this case?
	// we can look at our buddylist, if target is a member 
	// then this is a directed presence
	}
    } /* end of directed presence handling */
#endif // _flag_disable_presence_directed_XMPP
#ifdef AVAILABILITY_AWAY
    else if (node["/show"]) { 
	// else this is one of the so-called "presence broadcasts"
	// we will never support stupid broadcasts although we could 
	// do some fancy castmsg now
	switch (node["/show"][Cdata]) {
	case "away":
	case "xa":
	case "dnd":
	    // we simply map them to away
	    vSet("me", (node["/status"] && 
			node["/status"][Cdata]) || "away");
	    announce(AVAILABILITY_AWAY);
	    break;
	case "chat":
	    // chattativ? ab in den flirt-raum mit dir!
	    break;
	default:
	    // away loeschen wenn gesetzt
	    break;
	}
    } else if (node["@type"] == "unavailable") {
	// hey... we should logout those guys ;)
	// announce OFFLINE manually here? no.. see comment in person:quit()
	// _flag_enable_manual_announce_XMPP is nothing we really need.. i think
	return quit(); 
    } else {
	// TODO: quiet?
	announce(AVAILABILITY_HERE);
    }
#endif // AVAILABILITY_AWAY
}


message(XMLNode node) {
    XMLNode helper;
    string target, host, nick;
    string res;
    mixed t, msg;
    mixed *u;

    unless (node["@to"] && strlen(node["@to"])) {
	// this could well become valid in terms of a psyc friendcast
	P0(("%O jabber message() without 'to'-attribute\n"))
	return;
    }
    target = jid2unl(node["@to"]);

    unless (u = parse_uniform(XMPP + node["@to"])) { D("impossible!\n"); }
    isplacemsg = ISPLACEMSG(node["@to"]); 

    if (is_localhost(lower_case(node["@to"]))) { 
	// it's too unusual to have commands without cmdchar
	// so let's use input() instead of cmd() here
	// IMHO this should check if input is a cmd
	if (u[UResource] == "announce") {
	    // setting a MOTD
	    P1(("%O TODO: setting a MOTD %O\n", ME, node))
#if 0
	} else if (u[UResource] == "announce/online") {
	    // sending a message to all online users
	    parsecmd("yell " + node["/body"][Cdata]);
#endif
	} else {
	    // was input() up to recently.
	    if (node["/body"]) parsecmd(node["/body"][Cdata]);
	    else {
		P1(("%O got msg w/out body: %O\n", ME, node))
	    }
	}
	return;
#ifndef STRICT_NO_JID_HACKS
    } else if (node["@to"] == "xmpp") {
	// mh... someone talking to our virtual xmpp gateway :-)
	P1(("%O talking to me?? %O\n", ME, node))
#endif
    }

    // lets ignore typing notifications and such
    // they are evil for s2s karma
    unless((msg = node["/body"])) {
	P1(("%O talking, but nothing to say %O\n", ME, node))
	return;
    }
    unless(stringp(msg = msg[Cdata])) {
	P1(("%O why isn't msg a string!? %O of %O\n", ME, msg, node))
	return;
    }
    if ((helper = getchild(node, "x", "jabber:x:oob")) && helper["/url"]) {
	// now this is what i call a dirty hack,
	// still better than ignoring it however..
	//msg += " < "+ node["/x"]["/url"] +" >";
	// we need formats for _message, huh?
	// so we can put this into a var where it belongs..
	// well, we don't have psyc-cmd() yet anyhow, so let's
	// just append it in case it's an argument for a command
	msg += " " + helper["/url"][Cdata];
    }
    if (node["@type"] == "groupchat") {
	P4(("groupchat in %O: %O, %O. place is %O\n",
		ME, target, v("place"), place))
	// _message_public
	// TODO: this wont work with remote places at all...
	if (isplacemsg) {
	    P4(("isplacemsg in %O: %O == %O (%O) ?\n", ME,
					target, v("place"), place))
	    if ((!v("place") || stricmp(target, v("place")))
			&& (t = find_place(target))) {
		place = t;
		vSet("place", target);
		P4(("find_place in %O: %O is the place for love\n", ME, t))
	    }
	    input(msg);
	} else {
#ifdef JABBER_TRANSPARENCY
	    mapping vars = ([ "_nick" : MYNICK ]);

	    P3(("jtranz groupchat to %O\n", target))
	    vars["_jabber_XML"] = innerxml;
	    // TODO: dirty hack
	    vars["_INTERNAL_target_jabber"] = target[5..]; 
	    vars["_INTERNAL_source_jabber"] = myjidresource;
	    sendmsg(target,
		    "_jabber_message_groupchat",
		    "[_nick] is sending you a jabber groupchat message.",
		    vars);
#endif
	}
    } else {
	if (isplacemsg) {		// damn similar code in gateway.c!!!!
#if 0 // STRICTLY UNFRIENDLY NON-FUN TREATMENT
		sendmsg(ME, "_failure_unsupported_function_whisper",
			"Routing private messages through groupchat managers is dangerous to your privacy and therefore disallowed. Please communicate with the person directly.");
#else // MAKE FUN OF ST00PID JABBER USERS VARIANT
		mixed o = find_place(u[UUser][1..]);
		if (objectp(o) && node["/body"]) {
			// handle this by doing "flüstern" in room  ;)
			// <from> whispers to <to>: <cdata>
			P0(("private message in place.. from %O to %O\n",
			    ME, o))
			sendmsg(o, "_message_public_whisper",
				node["/body"][Cdata], ([ "_nick_target": u[UResource] || u[UUser]]));
		}
#endif
		return 1;
	}

	// _message_private
	nick = jid2unl(node["@to"]);
	// P2(("nick: %s\n", nick))
	// TODO: vars["_time_INTERNAL"] => jabber:x:delay, JEP-0091
#if 0
	if (msg[0..2] == "/me") {
		parsecmd(msg, nick);
	} else {
	    tell(nick, msg); 
	}
#else
	input(msg, nick);
#endif
    }
}	

iq(XMLNode node) {
    string target;
    string friend;
    XMLNode helper;
    XMLNode firstchild;
    string t;
    string packet;
    string template;

    mixed *vars;

    vars = ([ "_nick": MYNICK ]);
    target = jid2unl(node["@to"]);
    isplacemsg = stringp(target) && strlen(target) && ISPLACEMSG(target);

    P3(("%O IQ node %O\n", ME, node))
    firstchild = getfirstchild(node);
    unless(firstchild) {
	switch(node["@type"]) {
	case "get":
	    break;
	case "set":
	    break;
	case "result":
	    break;
	case "error":
	    break;
	}
	return;
    }
    helper = firstchild;

    switch(firstchild["@xmlns"]) {
    case "jabber:iq:version": 
	switch(node["@type"]) {
	case "get":
	    // TODO: as we need to fiddle with every tag, we probably could 
	    // 		overload sendmsg() of uni.c
	    m_delete(vars, "_tag");
	    sendmsg(target, "_request_version", 0, vars, 0, 0,
		    (: 
			$4["_tag_reply"] = tag;
			msg($1, $2, $3, $4);
			return; 
		    :)); 
	    break;
	case "set":
	    // HUH?
	    break;
	case "result":
	    break;
	case "error":
	    break;
	}
	break;
    case "jabber:iq:last":
	// TODO: request idle time
	break;
    case "urn:xmpp:ping":
	switch(node["@type"]) {
	case "get":
	case "set": // I dont know why xep 0199 uses set... its a request
		    // it got fixed in version 0.2 of the xep
	    w("_echo_ping", 0, vars);
	    break;
	}
	break;
#if !defined(REGISTERED_USERS_ONLY) && !defined(_flag_disable_registration_XMPP)
    case "jabber:iq:register":
	switch(node["@type"]) {
	case "get":
	    if (node["@to"] && node["@to"] != myjid) {
		// query this information from someone else
		// e.g. a transport
	    } else {
		emit(sprintf("<iq type='result' id='%s'>"
			     "<query xmlns='jabber:iq:register'>"
			     "<password/>"
			     "<instructions/>"
			     "<name>%s</name>"
			     "<email>%s</email>"
			     "</query></iq>", 
			     tag, 
			     v("publicname") || "", 
			     v("email") || ""));
	    }
	    break;
	case "set":
	    if (node["@to"] && node["@to"] != myjid) {
		// send this information from someone else
		// e.g. a transport
	    } else {
		if (t = helper["/email"])
		    vSet("email", t[Cdata]);
		if (t = helper["/name"])
		    vSet("publicname", t[Cdata]);
		    // TODO: better use legal_password() for that
		if ((t = helper["/password"]) && strlen(t[Cdata])) 
			vSet("password", t[Cdata]);
		    save();
		    emit(sprintf("<iq type='result' id='%s'/>", tag));
	    }
	    break;
	case "result":
	    break;
	case "error":
	    break;
	}
#endif // jabber:iq:register
    case "jabber:iq:roster":
	switch(node["@type"]) {
	case "get":
	    // TODO: this assumes that to is unset and the query is 
	    // to itself
	    hasroster = 1; 
	    packet = sprintf("<iq type='result' to='%s' id='%s'>"
			     "<query xmlns='jabber:iq:roster'>",
			     myjid, tag);
	    // TODO: listAcq does the same basically
	    foreach (friend : ppl) {
		mixed *u;
		string ro;
		mixed *va;
		string variant = "";

		if (is_formal(friend)) t = mkjid(friend);
		else 
		    t = friend + "@" SERVER_HOST;
		switch (ppl[friend][PPL_NOTIFY]) {
		case PPL_NOTIFY_MUTE:
		    break;
		case PPL_NOTIFY_DELAYED_MORE:
		case PPL_NOTIFY_DELAYED:
		    // wieso sind die mute genauso wie die 
		    // delayed?
		    variant = "_delayed";
		    break;
		case PPL_NOTIFY_IMMEDIATE:
		    variant = "_immediate";
		    break;
		// these two are handled differently
		case PPL_NOTIFY_OFFERED: 
		case PPL_NOTIFY_PENDING: 
		    continue;
		default:
		    // va["_acquaintance"] = "none";
		}
		// TODO: subscription states
		//
		// we append _roster because this output format is not 
		// compatible
		// to the /show command.
		template = T("_list_acquaintance_notification" + variant + "_roster", "");
		ro = psyctext(template, ([
	"_friend" : t,
	"_list_groups" : IMPLODE_XML(xbuddylist[friend], "<group>") || "",
#ifdef ALIASES
	"_nick" : raliases[friend] || friend
#else
	"_nick" : friend
#endif
		]));
		if (ro) packet += ro;
		else {
		    PT(("%O empty result for %O with buddy %O\n",
			ME, "_list_acquaintance_notification" + variant + "_roster", friend))
		}
	    }
	    emit(packet + IQ_OFF);
	    // here we should redisplay requests skipped above
	    // oder kann man die einfach reinmixen - nein
	    // foreach skipped
	    // 	_list_acquaintance_notification_pending
	    // 	_list_acquaintance_notification_offered
	    packet = "";
	    // send presence for online friends
	    // P2(("friends: %O\n", friends))
	    template = T("_notice_presence_here_plain",
			 "<presence to='[_INTERNAL_target_jabber]' "
			 "from='[_INTERNAL_source_jabber]'/>");
	    foreach(friend : m_values(friends)) {
		if (friend) 
		    packet += psyctext(template, ([
			  "_INTERNAL_target_jabber" : myjid, 
			  "_INTERNAL_source_jabber" : mkjid(friend),
			  "_INTERNAL_mood_jabber" : "neutral"
		    ]));
	    }
	    break; // showFriends() ?
	case "set":
	    helper = helper["/item"];
	    if (helper && helper["@subscription"] == "remove") {
		string buddy = jid2unl(helper["@jid"]);
#ifndef _flag_disable_module_friendship
		P2(("remove %O from roster\n", helper["@jid"]))
		friend(1, buddy);
#endif
		m_delete(xbuddylist, buddy);
		emit(sprintf("<iq type='result' id='%s'/>", tag));
	    } else {
		// note: "<group/> is opaque to jabber servers
		string subscription;
		string jid;
		string buddy;
		string name;

		jid = helper["@jid"];
		buddy = jid2unl(jid);
		unless (xbuddylist[buddy]) 
		    subscription = "none";
		else 
		    subscription = "both";

		// we should store the alias name also.. TODO
		if (helper["/group"]) {
		    if (nodelistp(helper["/group"])) {
			xbuddylist[buddy] = ({ });
			foreach(XMLNode iter : helper["/group"]) {
			    if (t = iter[Cdata])
				xbuddylist[buddy] += ({ t });
			}
		    } else if (t = helper["/group"][Cdata]) {
			xbuddylist[buddy] = ({ t });
		    } else
			xbuddylist[buddy] = ({ });
		} 
		name = helper["@name"];
		// USE ALIAS HERE
		// if (name) xbuddylist[buddy][XBUDDY_NICK] = name;
		// maybe we need to do this...
		// exodus always sends _all_ groups a nick
		// has to be in. but on the other hand we 
		// may want to act differently with psyc clients
		//
		// roster push
		// maybe we can just implode the former "groups" here
		unless (stringp(jid)) {
		    P0(("invalid jid for %O in %O\n", name, ME))
		    break;
		}
		unless (stringp(subscription)) {
		    P0(("invalid subscription for %O in %O\n", jid, ME))
		    break;
		}
		unless (stringp(name)) {
		    // nicht schlimm.. hat der user das alias-feld
		    // einfach leergelassen
		    P4(("no name %O for %O in %O\n", name, jid, ME))
		    name = "";
		}
		emit(sprintf("<iq type='set'>" // TODO: needs a id
			     "<query xmlns='jabber:iq:roster'>"
			     "<item jid='%s' name='%s' "
			     "subscription='%s'>%s</item></query></iq>",
			    jid, name, subscription, 
			    IMPLODE_XML(xbuddylist[buddy], "<group>")));
		if (stringp(tag))
		    emit(sprintf("<iq type='result' id='%s'/>", tag));
		save();
	    }
	    break;
	case "result":
	    // may happen in reply to roster pushes
	    break;
	case "error":
	}
	break;
    case "http://jabber.org/protocol/disco#info":
	switch(node["@type"]) {
	case "get":
	    if (!node["@to"])
		sendmsg(ME, "_request_list_feature", 0, vars);
	    else if (is_localhost(lower_case(node["@to"]))) 
		sendmsg("/", "_request_list_feature", 0, vars);
	    /* else... TODO */
	    break;
	case "set":
	    break;
	case "result":
	    break;
	case "error":
	    break;
	}
	break;
    case "http://jabber.org/protocol/disco#items":
    // send a list of rooms to the client
	switch(node["@type"]) {
	case "get":
	    if (!node["@to"]) 
		// "my" places - let person.c handle this
		sendmsg(ME, "_request_list_item", 0, vars);
	    else if (is_localhost(lower_case(node["@to"]))) 
		// server's places - let root.c handle this
		sendmsg("/", "_request_list_item", 0, vars);
	    /* else... TODO */
	    break;
	case "set": // never saw this
	    break;
	case "result":
	    break;
	case "error":
	    break;
	}
	break;
    case "jabber:iq:private":
	switch(node["@type"]) {
	case "get":
	    helper = helper["/storage"];
	    if (helper && helper["@xmlns"] =="storage:bookmarks") {
		// private xml storage, used for storing conference 
		// room bookmarks and autojoining them 
		packet = sprintf("<iq type='result' id='%s'>"
				 "<query xmlns='jabber:iq:private'>"
				 "<storage xmlns='storage:bookmarks'>",
				 tag);
		// hey wait.. we are sending the list of places here..
		// why is it i have never seen a jabber client actually
		// executing autojoins?  FIXME
		if (v("subscriptions")) 
		    foreach (string s in v("subscriptions")) {
			string jid;
			if (is_formal(s)) { // abbrev psyc:// is true
			// watch out, this assumes that only psyc rooms
			// can be entered remote
			// is this way valid for remote psyc rooms?
			// should use makejid or whatever this is called
			jid = PLACEPREFIX + s[7..];
			} else {
			    jid = PLACEPREFIX + s + "@" SERVER_HOST;
			}
			packet += sprintf("<conference name='%s' "
					  "autojoin='1' "
					  "jid='%s'>"
					  "<nick>%s</nick>"
					  "</conference>",
					  s, jid, MYNICK);
			}
		emit(packet + "</storage></query></iq>");
	    } else {
		vars["_INTERNAL_source_jabber"] = node["@to"] || myjid;
		vars["_INTERNAL_target_jabber"] = myjidresource;
		vars["_tag_reply"] = tag;
		render("_error_unsupported_method", 0, vars);
	    }
	    break;
	case "set":
	    /* updating your subscriptions / private xml storage */
	    helper = helper["/storage"];
	    if (helper && helper["@xmlns"] =="storage:bookmarks"){
		// TODO:
	    }
	    vars["_INTERNAL_source_jabber"] = node["@to"] || myjid;
	    vars["_INTERNAL_target_jabber"] = myjidresource;
	    vars["_tag_reply"] = tag;
	    render("_error_unsupported_method", 0, vars);
	    break;
	case "result": // never saw this
	    break;
	case "error": // never saw this
	    break;
	}
	break;
    case "vcard-temp":
	switch(node["@type"]) {
	case "get":
	    vars["_tag"] = node["@id"];
	    if (node["@to"] && node["@to"] != myjid) {
		/* request vcard from someone else */	
	    } else {/* fake your own vcard */
		emit("<iq type='result' id='" + tag + "' "
			 "to='" + (node["@from"] || myjid) + "' "
			 "from='" + myjid + "'>"
			 "<vCard xmlns='vcard-temp'>"
			 "</vCard></iq>");

	    }
	    break;
	case "set": 
	    if (node["@to"] && node["@to"] != myjid) {
		// setting the vcard of someone else is an error
		emit("<iq type='error' id='" + tag + "'>"
		     "<x xmlns='vcard-temp'/>"
		     "<error code='403' type='cancel'/>"
		     "<forbidden xmlns='" NS_XMPP "xmpp-stanzas'/>"
		     "</error>"
		     "</iq>");
	    } else { // update your own vcard
		mixed mvars;
		// watch out, dont use this email for v("email")-setting
		// this one is public while v("email") is more
		// private
		mvars = convert_profile(node["/vCard"], "jCard");
		PT(("%O received vCard from client (%O)\n", ME, sizeof(mvars)))
		emit("<iq type='result' id='" + tag + "'/>");
		request(ME, "_store", mvars);
		return;
	    }
	    break;
	case "result": // never saw this
	    break;
	case "error": // never saw this
	    break;
	default:
	    break;
	}
	break;
#if 0
    case "urn:xmpp:blocking":
	/* JEP-0191 style privacy lists 
	 * When implementing this, pay special attention to
	 * http://www.xmpp.org/extensions/xep-0191.html#matching
	 * although I dont think that we will implement the resource part
	 *
	 * TODO: I would like to have an qPerson api that does this matching
	 * 	automatically ontop of ppl
	 *
	 * yuck... totgeburt
	 */
	switch(node["@type"]) {
	case "get": /* return the whole blocklist */
	    packet = "<iq type='result'";
	    if (tag) packet += " id='" + tag + "'";
	    packet += "><blocklist xmlns='urn:xmpp:blocking'>";
	    foreach(mixed p, mixed val : ppl) {
		if (val[PPL_DISPLAY] == PPL_DISPLAY_NONE) 
		    packet += "<item jid='" + mkjid(p) + "'/>";
	    }
	    packet += "</blocklist></iq>";
	    break;
	case "set":
	    if (firstchild["/item"] && !nodelistp(firstchild["/item"])) 
		firstchild["/item"] = ({ firstchild["/item"] });
	    unless(firstchild["/item"]) { /* clear the blocklist */
		foreach(mixed p, mixed val : ppl) {
		    if (val[PPL_DISPLAY] == PPL_DISPLAY_NONE)
			sPerson(p, PPL_DISPLAY, PPL_DISPLAY_DEFAULT);
		}
	    } else {
		int block = firstchild[Tag] == "block";
		foreach (helper : firstchild["/item"]) {
		    /* add/remove each item to/from the blocklist */
		    if (block) {
			/* TODO: 
			 * integrate with friendship (/cancel) here!
			 */
			sPerson(helper["@jid"], PPL_DISPLAY, PPL_DISPLAY_NONE);
		    } else
			sPerson(helper["@jid"], PPL_DISPLAY, PPL_DISPLAY_DEFAULT);
		}
	    }
	    emit("<iq type='result'" + (tag ? "id='" + tag + "'" : "") + "/>");
	    /* push the updated items to other clients - that is delta 
	     * just like roster
	     */
	    // as we only have a single client this does not apply currently
	    // do pushes have to go to the 'current' client also?!
	    break;
	case "result":
	    break;
	case "error":
	    break;
	}
	break;
#endif
    default:
	switch(node["@type"]) {
	case "get":
	    if (node["@to"]) {
#ifdef JABBER_TRANSPARENCY
		vars["_jabber_XML"] = innerxml;
		sendmsg(target,
			"_jabber_iq_get",
			"[_nick] is sending you a jabber iq get.", vars);
		P0(("iq get to %O from %O\n", target, ME))
#else
		/* what else? */
#endif
	    } else { /* service-unavailable */
		vars["_INTERNAL_source_jabber"] = node["@to"]||myjid;
		vars["_INTERNAL_target_jabber"] = myjid;
		vars["_tag_reply"] = tag;
		render("_error_unsupported_method", 0, vars);
	    }
	    break;
	case "set":
	    if (node["@to"]) {
#ifdef JABBER_TRANSPARENCY
		vars["_tag"] = tag;
		vars["_jabber_XML"] = innerxml;
		sendmsg(target, "_jabber_iq_set",
			"[_nick] is sending you a jabber iq set.", vars);
		return;
#else
		/* what else? */
#endif
	    } else { /* service-unavailable */
		vars["_INTERNAL_source_jabber"] = node["@to"]||myjid;
		vars["_INTERNAL_target_jabber"] = myjid;
		vars["_tag_reply"] = tag;
		render("_error_unsupported_method", 0, vars);
	    }
	    break;
	case "result":
	    if (node["@to"]) {
#ifdef JABBER_TRANSPARENCY
		vars["_tag_reply"] = tag;
		vars["_jabber_XML"] = innerxml; // || "" ???
		sendmsg(target, "_jabber_iq_result",
			"[_nick] is sending you a jabber iq result.", vars);
#else 
		/* what else? */
#endif
	    } else {
		vars["_INTERNAL_source_jabber"] = node["@to"]||myjid;
		vars["_INTERNAL_target_jabber"] = myjid;
		vars["_tag_reply"] = tag;
		render("_error_unsupported_method", 0, vars);
	    }
	    break;
	case "error":
	    vars["_tag_reply"] = tag;
	    if (node["@to"]) {
#ifdef JABBER_TRANSPARENCY
		vars["_jabber_XML"] = innerxml;
		sendmsg(target, "_jabber_iq_error",
			"[_nick] is sending you a jabber iq error.", vars);
#else
		/* what else? */
#endif
	    }
	    break;
	}
    }
}

// this isn't really used consistently...
string jid2unl(string jid) {
    string node, host, resource;
    string t;

    P3(("jid2unl saw %O\n", jid))
    unless(jid) return 0;
    //
    // TODO: what if jid == SERVER_HOST?
    // 		or alike?
    sscanf(jid, "%s@%s", node, host);
    unless (host) { // it is a bare hostname
	host = jid;
	node = "";
    }
// never happens, because node is "" not 0
//  unless (node) {
//	return "psyc://" + host;
//  }
    sscanf(host, "%s/%s", host, resource);
#if 1 // wieso 0??? sendmsg() hätte das hinkriegen sollen.. war der gedanke
    if (is_localhost(lower_case(host))) { // local user
	P4(("is_localhost? %O, YES, then use %O\n", host, node))
	// TODO: what about returning objects?
	if (strlen(node) && ISPLACEMSG(node)) return PREFIXFREE(node);
	return node;
    }
# if 1
    else if (strlen(node) && ISPLACEMSG(node)) {
	return "psyc://" + host + "/@" + PREFIXFREE(node);
    }
# endif
    P4(("is_localhost? %O, NO, don't use %O\n", host, node))
#endif
#if 0
    else if (host == "sip." SERVER_HOST) {
	return "sip:" + replace(node, "%", "@");
    }
    else
#endif
#ifndef STRICT_NO_JID_HACKS
    if (host == "xmpp") {
	// send message to xmpp:node while replacing first % with @
	unless(node) return 0; // iq query to "xmpp", which is our
				// virtual gateway
	t = XMPP + replace(node, "%", "@");
	if (resource) t += "/" + resource;
	    	// should it be RESOURCEPREP(resource) here? --lynX
	return t; 
    }
    else
#endif
#if 0 // sendmsg does this part, too
    // TODO: maybe we need some kind of support for remote
    // rooms here. As usual, we will use the # prefix for that
    if (node[0] == '*' || node[0] == '~' || node[0] == '$') {
	return "psyc://" + host + "/" + node;
    }
#endif
    // leave it to sendmsg() to figure out what to do
    return jid;
}

varargs string mkjid(mixed who, mixed vars, mixed ignore_nick, mixed ignore_context,  string target) {
    if (stringp(who) && strlen(who) && !isplacemsg) {
	string t;
	mixed *u = parse_uniform(who);

	unless (u) {
		P3(("mkjid leaving %O as is\n", who))
		return who; // it already _is_ a jid!
	}
	switch(u[UScheme]) {
	case "psyc":
#if 0
		// let psyc users be jidded as ~nick@host? would be cleaner
		// but no.. we want to be jabber-upgradable and SWITCH2PSYC
		// is a reality.. vive la legacy compatibility!
		t = u[UResource] +"@"+ u[UHost];
#else
		if ((t = u[UResource]) && strlen(t) > 1) {
		    // or let psyc users be the same person as on xmpp?
		    // YES we want transparent upgrades from xmpp to psyc!
		    if (t[0] == '@')
			t = PLACEPREFIX+ t[1..] +"@"+ u[UHost];
		    else
			t = t[1..] +"@"+ u[UHost];
		} else {
		    // the usual "shouldn't happen" case which however does
		    t = u[UHost];
		}
#endif
		// ... and what about the resource in jabber sense?
		P3(("mkjid encoding %O as %O (psyc)\n", who, t))
		return t;
#if 1 // let xmpp users have their normal jids
	case "xmpp":
	case 0:
		t = u[UUserAtHost] || u[UUser] +"@"+ u[UHost];
		t = u[UResource] ? t+"/"+u[UResource] : t;
		    // should we use RESOURCEPREP(resource) here? --lynX
		P3(("mkjid encoding %O as %O (xmpp)\n", who, t))
		return t;
#endif
	default:
		// do funny user%host@scheme encoding
		// could also croak back and deny this communication..
		if (u[UUser]) {
		    t = u[UUser] +"%"+ u[UHost] +"@"+ u[UScheme];
		    t = u[UResource] ? t+"/"+u[UResource] : t;
		    // should we use RESOURCEPREP(resource) here? --lynX
		}
		else if (u[UResource]) {
		    t = u[UResource] +"%"+ u[UHost] +"@"+ u[UScheme];
		}
		else t = u[UHost] +"@"+ u[UScheme];
		P3(("mkjid encoding %O as %O\n", who, t))
		return t;
	}
    }
    P3(("calling regular ::mkjid for %O\n", who))
    return ::mkjid(who, vars, ignore_nick, ignore_context, target, SERVER_HOST);
}

// message rendering a la jabber
w(string mc, string data, mapping vars, mixed source) {
    mixed t;
    unless (mappingp(vars)) vars = ([]);
    else if (vars["_nick_verbatim"]) vars["_nick"] = vars["_nick_verbatim"];
    // ^^ this is a temporary workaround until we fix the real problem!

    switch (mc) {
    case "_notice_login":
	// suppresses the jabber:iq:auth reply in the SASL case
	unless (stringp(tag)) return;
	break;
    case "_error_status_place_matches":
	PT(("still _error_status_place_matches?\n"))
	return;
    case "_echo_request_friendship":
	vars["_list_groups"] = xbuddylist[vars["_nick"]] || "";
	vars["_nick"] = mkjid(vars["_nick"]);
	break;
	/* copy/paste from active.c, BAD */
    case "_notice_list_feature":
    case "_notice_list_feature_person":
    case "_notice_list_feature_place":
    case "_notice_list_feature_server":
    case "_notice_list_feature_newsfeed":
#ifndef _flag_disable_query_server
	mixed id2jabber = shared_memory("disco_identity");
	mixed feat2jabber = shared_memory("disco_features");
	unless (mappingp(id2jabber)) return 1;
	vars["_identity"] = id2jabber[vars["_identity"]] || vars["_identity"];
	vars["_list_feature"] = implode(map(vars["_list_feature"], 
	     (: return "<feature var='" + feat2jabber[$1] + "'/>"; :)), "");
	break;
#else
	return 1;
#endif
    case "_notice_list_item":
	t = "";
	// same stuff in user.c (what happened to code sharing?)
	for (int i = 0; i < sizeof(vars["_list_item"]); i++) {
	    t += "<item name='"+ xmlquote(vars["_list_item_description"][i]) +"'";
	    if (vars["_list_item"] && vars["_list_item"][i])
		t += " jid='" + mkjid(vars["_list_item"][i]) + "'";
	    if (vars["_list_item_node"] && vars["_list_item_node"][i])
		t += " node='" + vars["_list_item_node"][i] + "'";
	    t += "/>";
	}
	vars["_list_item"] = t;
	break;
#ifdef PREFIXES
    default:
	// until we have a better way to deal with them..
	if (abbrev("_prefix", mc)) return;
#endif
    }

    if (source)
	determine_sourcejid(source, vars);
    else {
	vars["_INTERNAL_source_jabber"] = SERVER_HOST;
	vars["_INTERNAL_source_jabber_bare"] = SERVER_HOST;
    }
    // determine_targetjid(target, vars);
    unless (vars["_INTERNAL_target_jabber"]) 
	vars["_INTERNAL_target_jabber"] = myjidresource;
    unless (vars["_INTERNAL_target_jabber_bare"])
	vars["_INTERNAL_target_jabber_bare"] = myjid;

    if (vars["_place"]) vars["_place"] = mkjid(vars["_place"]);
    // TODO: this tag strategy needs to be reworked...
    // the unlesses are because sometimes the psyced does tagging internally
    // or if the packet is arriving from remote
    unless (vars["_tag"]) vars["_tag"] = tag;
    unless (vars["_tag_reply"]) vars["_tag_reply"] = tag;
    if (vars["_list_groups"])
	vars["_list_groups"] = IMPLODE_XML(vars["_list_groups"], "<group>");

    return render(mc, data, vars, source);
}
