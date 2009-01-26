// $Id: slave.c,v 1.64 2008/03/28 20:05:44 lynx Exp $ // vim:syntax=lpc
//
#define TIME_UPDATE_MINWAIT	33	// 33 seconds

#include <net.h>
#include <status.h>

virtual inherit NET_PATH "place/storic";

protected volatile string master, mastertrail;
protected volatile int members, servers;
private volatile int lastup;

histClear(a, b, source, vars) { if (b > 49) return ::histClear(a, b, source, vars); }

// the flag allows forced update and avoids call_out loops
#define UPDATE_NOW	0
#define UPDATE_SOON	1
#define UPDATE_REMINDER	2

qJunction() { return 0; }

update(flag) {
	int t;
	unless(master) return;
	t = time() - lastup;
	P2(("update(%O) - %O to go.\n", flag, TIME_UPDATE_MINWAIT - t))

	if (flag == UPDATE_NOW || t > TIME_UPDATE_MINWAIT) {
		lastup = time();
		sendmsg(master, "_request_link", 0,
			 ([ "_amount_members": to_string(size()) ]) );
	} else if (flag == UPDATE_SOON)
		call_out(#'update, TIME_UPDATE_MINWAIT+1-t, UPDATE_REMINDER);
}

void create() {
	lastup = time();	// delay linkup by 7 seconds (see below)
	::create();
}

#if 0
void reset(int again) {
	if (again) update(UPDATE_SOON);	// just in case..
	else lastup = time();	// delay linkup by 7 seconds (see below)
	return ::reset(again);
}
#endif

sIdentification(it) {
	identification = it;
}

sMaster(link, modflag) {
	unless (abbrev("psyc:", link))
	    link = "psyc://"+ link +"/"+ psycName();
	mastertrail = master = lower_case(link); // lower_case UNI policy
	identification = link;
	set_context(ME, link);
	// some kinda kludge until the psyc layer gives us the true UNI
//	if (abbrev("psyc://ve.", master)) mastertrail = master[10..];
//	P2(("mastertrail = %O\n", mastertrail))
	//call_out(#'update, 7, UPDATE_NOW);
	update(UPDATE_NOW);
}

mixed isValidRelay(mixed x) { return x == master || x == ME; }

msg(source, mc, data, vars) {
	P2(("slave:msg(%O, %O, %O, %O)\n", source, mc, data, vars))
	// if (vars["_context"]) return -1; // may not happen
	// status update from master / identification
	if (master && source == master ||
	    identification && source == identification) {
		/* a message from our master / identification to us
		 * usually a status update or some other kind of control msg
		 * this control msg logic has moved around, but hasn't changed
		 */
		switch(mc) {
		case "_notice_place_topic":
			vSet("topic-user", vars["_nick"]);
			// fall thru
		case "_notice_place_topic_official":
			vSet("topic", vars["_topic"]);
			break;
		case "_notice_place_topic_removed":
			vSet("topic-user", vars["_nick"]);
			break;
		case "_notice_link_topic":
			vSet("topic", vars["_topic"]);
			// fall thru
		case "_notice_link":
			if (vars["_filter_presence"] &&
				to_int(vars["_filter_presence"]) != 0) 
			    vSet("_filter_presence", source);
			else
			    vDel("_filter_presence");
			//m_delete(vars, "_filter_presence");
			break;
		case "_notice_place_topic_removed_official":
			vDel("topic");
			break;
		case "_error_necessary_link_place":
		case "_error_rejected_message_membership":
		// add protection from loops?
			castmsg(ME, "_warning_place_link_lost",
		"Master link [_master] has forgotten about us. Hold on.",
			    ([ "_master" : master ]) );
			// fall thru
		case "_notice_unlink_restart":
		case "_notice_unlink_restart_complete":
			// update (UPDATE_NOW);
			update (UPDATE_SOON);
			return 1;
		case "_notice_place_history_cleared":
			::histClear(vars["_amount_messages"], 60, source, vars);
			return 1;
		case "_status_place_filter_presence":
			if (vars["_filter_presence"] &&
				to_int(vars["_filter_presence"]) != 0) 
			    vSet("_filter_presence", source);
			else
			    vDel("_filter_presence");
			return 1;
		}

		if (abbrev("_notice_place_enter", mc)) {
			// master notifies us that _source_relay was allowed
			// to enter - kind of fake
			string relay = vars["_source_relay"];
			m_delete(vars, "_source_relay");
			vars["_nick_place"] = MYNICK;
			return ::msg(relay, mc, data, vars);
		}
		if (abbrev("_notice_place_leave", mc)) {
			string relay = vars["_source_relay"];
			m_delete(vars, "_source_relay");
			vars["_nick_place"] = MYNICK;
			return ::msg(relay, mc, data, vars);
		}
		return castmsg(source, mc, data, vars);
	}
	// someone is sending a message via us. 
	// we may send it to the master and if we dont return will pass
	// it on to local msg()
	// TODO: need to do some kind of checks if we want to forward
	// 	sth for the sender
	if ((!vars["_context"] // should not happen, but...
		// things not to forward
		&& !(abbrev("_request_leave", mc) 
		     || abbrev("_request_context_leave", mc))
	     // things we want to forward 
	    	||abbrev("_message", mc) 
	    	|| !v("_filter_presence") && (abbrev("_request_enter", mc)
				 || abbrev("_request_context_enter", mc))
	    )
	   ) { // source != master implied above
	    
		// we let upstream handle it
		// TODO: we _must_ be linked to do this
		// 	looks like a job for the notorious queue
		//
		// 	otherwise a slave may send an _request_enter to
		// 	the master which is silent and should therefore
		// 	drop the packet.
		// 	and the user is left alone
		// 	the master could catch such behaviour... but...
		// 	complicates things and is more harmful than useful
		//
		// hrmpf. IMHO we should send it via master, if 
		// this does not work then debug it!
		// hey, its lynX who told me to do that :)
#if 0
			sendmsg(master, mc, data, vars + ([
			    "_location": query_ip_name(source),
			    "_source_relay": ME,
		        ]), source);
#else
		sendmsg(identification, mc, data, vars + ([
			    // what about peer scheme, host and -port
			    // ah, did just that in usercmd sayvars
			    "_location": objectp(source) ?
				query_ip_name(source) : source,
			    "_source_relay": vars["_source_relay"] || source
			      ]));
#endif
		// if (abbrev("_message", mc)) return 1;
		return 1;
#if 0 // wo war der Sinn von dem Code?
		D3(else D(S("slave:msg from %O master %O, mastertrail %O\n",
			source, master, mastertrail));)
		if (source == master || (vars["_source"] == master &&
			   stringp(source) && trail(mastertrail, source))) {
		    string relay = vars["_source_relay"];

		    // if this were psyc 1.1, we wouldnt receive messages
		    // relayed, but have proper _context. i guess we want
		    // to implement both forms and maybe one day disallow
		    // one or the other on a per-room basis.
		    if (relay) {
			// dieser teil war auskommentiert..
			// was will mir das sagen? ich werde bugs auslösen?
			// jopp
			// vars["_source_relay"] = master;
			// master could spoof local objects
//			source = psyc_object(relay) || relay;
			// geht das? hu?
			vars["_source_relay"] = vars["_source"];
			P2(("slave:msg(%O,%O,%O) relay=%O\n",
			     source, mc, data, relay))
		    }
		}
#endif
	}
	// hm... can we simply do that?
	return ::msg(source, mc, data, vars);
}

castmsg(source, mc, data, vars) {
	P2(("slave:castmsg(%O, %O, %O, %O)\n", 
	    source, mc, data, vars))
	
	if (member(vars, "_amount_members")) {
		members = to_int(vars["_amount_members"]);
		m_delete(vars, "_amount_members");
	}
	if (member(vars, "_amount_servers")) {
			servers = to_int(vars["_amount_servers"]);
			m_delete(vars, "_amount_servers");
	}
	::castmsg(source, mc, data, vars);
}

enter(source,mc,data,vars) {
	update (UPDATE_SOON);
	return ::enter(source,mc,data,vars);
}
leave(source,mc,data,vars) {
	update (UPDATE_SOON);
	return ::leave(source,mc,data,vars);
}

memberInfo(person) {
	unless (members && servers) return ::memberInfo(person);
	unless (person) person = previous_object();

	sendmsg(person, "_status_place_net_members_amount",
"[_nick_place] contains [_amount_members] people \
on [_amount_servers] servers.", ([
               "_nick_place": MYNICK,
           "_amount_members": members,
           "_amount_servers": servers,
        ]) );
        return 0;
}

showStatus(verbosity, al, source, mc, data, vars) {
	if (members && master) {
		sendmsg(source, "_status_place_link",
			"Network link to [_link] active.",
			([ "_link": master ]) );
	}
	return ::showStatus(verbosity, al, source, mc, data, vars);
}

qAllowExternal(source, mc) {
	if (source == master) return 1;
}

reboot(reason, restart, pass) {
	if (master) {
		sendmsg(master, "_request_unlink");
		master = 0;
	}
	return ::reboot(reason, restart, pass);
}
