// $Id: master.c,v 1.67 2008/01/05 12:42:17 lynx Exp $ // vim:syntax=lpc
//
// part of the PSYC multicast implementation strategy. accepts remote slave
// rooms to link into here. the point is to not have the traffic of every
// single user entering and leaving. if you like that, the automatic and
// transparent group/master and slave does the job for you. these two
// distribution systems co-exist and in fact together create a very
// scalable multicast solution without using the "IP Multicast" protocol.
//
#include <net.h>
#ifdef ORIGINAL_MASTERS

#include <status.h>
#include <driver.h>
#ifdef WE_PROBABLY_DONT_NEED_THIS
# include <uniform.h>
#endif

virtual inherit NET_PATH "place/storic";

private volatile mapping l;
private volatile int netppl;

qJunction() { return 0; }

isLink(source) {
	P2(("isLink %O in %O: %O\n", source, l, l[source]))
	return member(l, source);
}

histClear(a, b, source, vars) {
	if (b < 50) return;
        netmsg(ME, "_notice_place_history_cleared", "--", ([
			"_amount_messages" : a ]) );
                        // invisible msg
	return ::histClear(a, b, source, vars); 
}

netsize() { return netppl+size(); }

save() {
	if (l) {
		vSet("links", l);
		P2(("saving links: %O\n", v("links")))
	}
	return ::save();
}

reboot(reason, restart, pass) {
	if (mappingp(l)) {
		if (restart)
		    netmsg(ME, "_notice_unlink_shutdown_temporary",
	    "Server restart in progress. Connection to [_source] dismantled.",
			([]));
		else
		    netmsg(ME, "_notice_unlink_shutdown",
	    "Server shutdown in progress. Connection to [_source] dismantled.",
			([]));
		l = 0;
	}
	// would destruct junctions before slave::reboot() has been called?
	// no.. because we no longer selfdestruct in reboot
	return ::reboot(reason, restart, pass);
}

load(a) {
	mixed rc;
	rc = ::load(a);
	unless(l) {
		netppl=0;
#if 1
	// die frage lautet: wollen wir das?
	// ja, dann bleiben alle slaves up to date
	// und sollte ein temporärer dabei sein, fliegt er hiermit raus
		l = v("links");
		if (mappingp(l)) {
			P2(("reestablishing links: %O\n", l))
			netmsg(ME, "_notice_unlink_restart_complete",
			    "Connection to [_source] reestablished.", ([]));
		}
	// wir lassen uns jeden link mit nem _request_link vom slave bestätigen
	// also wird "l" nach gebrauch gelöscht!
#endif
		l = ([]);
	}
	return rc;
}
static eachppl(link, size) { netppl += to_int(size); }

msg(source, mc, data, mapping vars) {
        P2(("master:msg(%O, %O, %O, %O)\n", source, mc, data, vars))
	if (stringp(source)) {
	    string relay;
	    int isli = member(l, source);
	    P2(("%O isli:%O source:%O links:%O\n", ME, isli, source, l))
#if 1 // is this stuff doing something useful?
	    if (!(qJunction() && stringp(vars["_source_relay"])
			 && is_formal(vars["_source_relay"]))
		     && isli && (relay = vars["_source_relay"])) {
	    //if (isli && (relay = vars["_source_relay"])) {
#if 0 //def WIR_TESTEN
		    P0(("%O: source %O (%O) und relay %O (%O) vertauscht\n", ME,
			 source, vars["_source"], relay, vars["_source_relay"]))
#else
		    P1(("%O: source %O und relay %O vertauscht\n", ME,
			 source, relay))
#endif
		    // one day we will have to take heed who has the
		    // privilege of linking us, as this is again a
		    // nice chance for spoofing.. but then again if someone
		    // dares to, he quickly ends up on the blacklist
		    // TODO: we need to check when exactly we need that
		    // 		or optimize on source == source_relay 
		    // 		on link level
		    vars["_source_relay"] = source;
		    source = relay;
	    } else
#endif
		switch(mc) {
// should also handle _request_enter i suppose
	    case "_request_link":
		// when source is an ip number we could do a 10 second
		// call_out and retry after resolving that ip number..
		// or we expect all links to resolve at psyc negotiation
		// time before getting here.. better solution..
		unless(isli) {
		    // save();
		    //unless (qSilent()) // || v("quiet"))
			monitor_report("_notice_link_report",
			  S("%O linked by %s (%O)", ME, source, l[source]));
// let ppl in room see what's happening..?
// this also goes out to all links, thus serves also as echo
// but this requires fiddling around on the linked slave whether its an
// controlmsg... so we simply send it separately
// reminds of _notice_place_enter* in place/basic
		    // could send identification here for junctions?
		    unless(v("_filter_presence"))
		    	castmsg(source, "_notice_link_topic",
				"Link to [_source] established. [_topic]",
				([ "_topic" : (v("topic") || "") ]) );
		    sendmsg(source, "_notice_link",
			   "Link to [_source] established.", ([
			"_topic" : v("topic") || "", 
//			"_filter_conversation" : v("_filter_conversation"),
			"_filter_presence" : v("_filter_presence")
		    ]));
		} else {
		    P1(("%O link update: %s in %s\n", ME,l[source],source))
		}
		l[source] = vars["_amount_members"];
		unless(isli) save();
//		netppl=0; walk_mapping(l, "eachppl", ME);
		return 1;
	    case "_error_rejected_message_membership":
		// TODO: the place does not remember us...
		// maybe it does not want to get our news.
		// treat it like an unlink?
	    case "_request_unlink":
		if (member (l, source)) {
		    P1(("%O unlinked by %s (%O)\n", ME,source,l[source]))
		    m_delete(l, source);
		    save();
		    sendmsg(source, "_notice_unlink",
				"Link to [_source] dropped.");
		} else
		    D(S("%O got unexpected unlink from %s (%O)\n",
			ME, source, vars));
		netppl=0; walk_mapping(l, "eachppl", ME);
		return 1;
	    }
	}

#ifdef WE_PROBABLY_DONT_NEED_THIS
	if (stringp(source)) {
	    string host;

	    host = parse_uniform(source, 1)[UHost];

	    unless (chost(host)) {
		dns_resolve(host,
			    (: 
			     register_host($6, $1);

			     if ($1 != -1) {
				 ::msg($2, $3, $4, $5);
				 debug_message(sprintf("MASTER: %O resolves to %O\n", $6, $1));
			     } else {
				 debug_message(sprintf("MASTER: %O does not resolve. ignoring. (closure).\n", $6));
			     }
			     return; :), source, mc, data, vars, host);
		return;
	    } else if (chost(host) == -1) {
		P1(("MASTER: %O does not resolve. ignoring.\n", host))
		return;
	    }
	}
#endif

	return ::msg(source, mc, data, vars);
}

volatile string lastlink;

localmsg(source, mc, data, mapping vars) {
	if (mappingp(vars)) {
		m_delete(vars, "_amount_members");
		m_delete(vars, "_amount_servers");
	} else vars = ([]);
	::castmsg(source, mc, data, vars);
}

#if 1
static eachlink(link, size, source, mc, data, vars) {
	// we don't send to empty slaves..
	// but for now we let them stay linked
	// we could as well delete them.. hmm TODO
	// and what about keeping history, topic and
	// other control information in sync?
	// okay.. it's not that simple
	//unless (size) return;

	unless (sendmsg(link, mc, data, vars, source)) {
		// when send_udp fails, we get to know this at the *next* attempt
		// to send a message. at least that's what the linux kernel seems to do
		// most important thing, if it happens we need to resend the packet! 
		//P0(("%O: transmission damaged to %s (%O)\n", ME, link, size))
		monitor_report("_failure_damaged_transmission", S("%O: transmission damaged to %s (%O)", ME, link, size));
		unless (sendmsg(link, mc, data, vars, source)) {
			D2( D("ERROR! resend failed a priori. this shouldn't happen.\n"); )
			// if we got here, then we are probably not running linux
			// let's assume this link is really broken and remove it
			m_delete(l, link);
		} else if (lastlink) {
		    // resend was successful, supposedly, or it didnt generate an immediate error
		    // so we can look into deleting the link that actually caused the problem
		    // on linux that's the one we sent something to *before*

		    if (l[lastlink]) {
			l[lastlink] = 0;	// give it one more try
		    } else {
			P0(("transmission damaged to %s, removed probable cause of trouble: %s\n", link, lastlink))
			m_delete(l, lastlink);
		    }
		}
	}
	lastlink = link;
}
#endif

netmsg(source, mc, data, mapping vars) {
	mapping myv;

#if 0 //def DRIVER_HAS_INLINE_CLOSURES
	vars["_amount_members"] = netppl+size();
	vars["_amount_servers"] = 1+sizeof(l);
	walk_mapping(l, (: sendmsg($1, $3,$4,$5,$6) :),
				mc,data,vars,source );
	D("using closure\n");
#else

#ifdef QUIET_REMOTE_MEMBERS
	if (abbrev("_notice_place_enter", mc)) return;
	if (abbrev("_notice_place_leave", mc)) return;
#endif

	myv = vars + ([
	   "_amount_members": netppl+size(),
	   "_amount_servers": 1+sizeof(l),
	]);
	lastlink = 0;
	// here you can see some pre-foreach LPC code  :)
	walk_mapping(l, #'eachlink, source, mc, data, myv);
#endif
}

castmsg(source, mc, data, mapping vars) {
	// TODO: psyc 1.1 / context oriented thingies will want to set
	// context for the delivery to the slaves
	// WHAT about junctions? will they set themselves as context 
	// aswell?
	if (l) {
		P2(("master:castmsg(%O,%O) w/_context %O and _nick_place %O\n",
		    source, mc, vars["_context"], vars["_nick_place"]))
		// why is this necessary here again?
		// and shouldn't it be the identification rather than ME?
		vars["_context"] = ME;
		// this one's even worse
		vars["_nick_place"] = MYNICK;
		netmsg(source, mc, data, vars);
	}
	localmsg(source, mc, data, vars);
}

memberInfo(person) {
	if (netppl) {
		unless (person) person = previous_object();
		sendmsg(person, "_status_place_net_members_amount", 
		    "[_nick_place] contains [_amount_members] people \
on [_amount_servers] servers.", ([
				   "_nick_place": MYNICK,
			       "_amount_members": netppl+size(),
			       "_amount_servers": 1+sizeof(l),
		]) );
		// return 0;
	}
	return ::memberInfo(person);
}

#if 0
showStatus(person, al, mc, data, vars) {
	::showStatus(person, al, mc, data, vars);
	if (l && al) walk_mapping(l, "eachstatus", ME, person);
	return 1;
}
#else
cmd(a, args, b, source, vars) {
	if (b) switch(a) {
case "links":
case "li":
		if (sizeof(args) > 1) return 0;
		if (l) walk_mapping(l, "eachstatus", ME, source);
		return 1;
	}
	return ::cmd(a, args, b, source, vars);
}
#endif
static eachstatus(link, size, person) {
	sendmsg(person, "_status_place_link_slave",
	    "Link [_link] has [_amount_members] members.",
	    ([ "_link": link, "_amount_members": size ]) );
}

mixed isValidRelay(mixed x) { return x == ME || member(l, x); }

#else
# include "storic.c"

// with cslaves we no longer keep extra track of "netppl"
netsize() { return size(); }
#endif
