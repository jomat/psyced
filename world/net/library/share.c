// $Id: share.c,v 1.25 2008/12/04 14:20:52 lynx Exp $ // vim:syntax=lpc
//
// verrry simple way to keep single sets of data for all kinds of purposes.
//
// the trick is: if you put these mappings here, they only exist once in
// the entire system, whereas if defined in your application they will be
// recreated for each instance of your app.
//
// net/d/share was essentially a similar idea, but this works better in library.

// use something like this to access the mappings:
//
// volatile mapping mood2jabber;
// mood2jabber = shared_memory("mood2jabber");

#include <net.h>
#include <presence.h>
#include <peers.h>
#include <proto.h>
#include <psyc.h>

// share already contains "preset" data sets
volatile mapping share = ([
#ifdef JABBER_PATH
# ifndef _flag_disable_module_presence
	"jabber2avail": ([
		"chat"	: AVAILABILITY_TALKATIVE,
		0	: AVAILABILITY_HERE,
		"dnd"	: AVAILABILITY_BUSY,		// dnd?
		"away"	: AVAILABILITY_DO_NOT_DISTURB,	// nearby?
		"xa"	: AVAILABILITY_AWAY		// "not available"
	]),
	// map to http://www.jabber.org/jeps/jep-0107.html although that
	// is more like good ole mud feelings
	"mood2jabber": ([
		0	:	"neutral",		// unspecified mood
		// any suggestions on these?
		1	:	"depressed",		// not dead
		2	:	"angry",
		3	:	"sad",
		4	:	"moody",
		5	:	"neutral",		// not okay
		6	:	"interested",		// nothing like 'good'
		7	:	"happy",
		8	:	"excited",		// not bright
		9	:	"invincible",		// not nirvana
	]),
# endif // _flag_disable_module_presence
# ifndef _flag_disable_query_server
	"disco_identity" : ([
			    "administrator" 	: "admin",
			    "newbie" 		: "anonymous",
			    "person" 		: "registered",
			    "place"		: "irc", 
			    "news"		: "rss",
			    "chatserver"	: "im",
	]),
	"disco_features" : ([ 
			    "vCard"		: "vcard-temp",
			    "list_feature"	: "http://jabber.org/protocol/disco#info",
			    "list_item"		: "http://jabber.org/protocol/disco#items",
			    "ignore"		: "http://jabber.org/protocol/blocking",
			    "jabber-gc-1.0"	: "gc-1.0",
			    "jabber-muc"	: "http://jabber.org/protocol/muc",
			    "commands"		: "http://jabber.org/protocol/commands",
			    "membersonly"	: "muc_membersonly",
			    "nonanonymous"	: "muc_nonanonymous",
			    "moderated"		: "muc_moderated",
			    "unmoderated"	: "muc_unmoderated",
			    "public"		: "muc_public",
			    "private"		: "muc_hidden",
			    "persistent"	: "muc_persistent",
			    "temporary"		: "muc_temporary",
#ifndef _flag_disable_registration_XMPP
			    "registration"	: "jabber:iq:register",
#endif
			    "offlinestorage"	: "msgoffline",
			    "version"		: "jabber:iq:version",
			    "lasttime"		: "jabber:iq:last",
			    "time"		: "jabber:iq:time",
	]),
# endif // _flag_disable_query_server
#endif
#ifndef _flag_disable_module_presence
	// even if _degree_availability says it all, we still need
	// a string to feed to the textdb - even if it were only internal
	"avail2mc": ([
		AVAILABILITY_EXPIRED		: "_absent", // not true, but..
		AVAILABILITY_OFFLINE		: "_absent",
		AVAILABILITY_VACATION		: "_absent_vacation",
		AVAILABILITY_AWAY		: "_away",
		AVAILABILITY_DO_NOT_DISTURB	: "_here_busy",
		AVAILABILITY_NEARBY		: "_here_busy",
		AVAILABILITY_BUSY		: "_here_busy",
		AVAILABILITY_HERE		: "_here",
		AVAILABILITY_TALKATIVE		: "_here_talkative"
	]),
	// these mc mappings are only for the internal textdb
	"mood2mc": ([
		0	:	"",		// unspecified mood
		// any suggestions on these?
		1	:	"_dead",
		2	:	"_angry",   // tragic, enraged, sore, irate?
		3	:	"_sad",	    // gloomy, miserable?
		4	:	"_moody",   // pensive?
		5	:	"_okay",
		6	:	"_good",
		7	:	"_happy",
		8	:	"_bright",  // blissful? blessed? joyous?
		9	:	"_nirvana", // enlightened? paradisiacal?
	]),
#endif // _flag_disable_module_presence
#ifndef _flag_disable_module_friendship
	// see peers.h for these
	"ppl2psyc": ([
		PPL_DISPLAY :	"_display",
		PPL_NOTIFY  :	"_notification",
		PPL_EXPOSE  :	"_expose",
		PPL_TRUST   :	"_trust",
	]),
	"_display": ([
		PPL_DISPLAY_NONE	:	"_none",
		PPL_DISPLAY_SMALL	:	"_reduced",
		PPL_DISPLAY_REGULAR	:	"_normal",
		PPL_DISPLAY_BIG		:	"_highlighted",
	]),
	"_notification": ([
		PPL_NOTIFY_IMMEDIATE	:	"_immediate",
		PPL_NOTIFY_DELAYED	:	"_delayed",
		PPL_NOTIFY_DELAYED_MORE	:	"_delayed_more",
		PPL_NOTIFY_MUTE		:	"_mute",
		PPL_NOTIFY_PENDING	:	"_pending",
		PPL_NOTIFY_OFFERED	:	"_offered",
		PPL_NOTIFY_NONE		:	"_none",
	]),
#endif
	// this table defines variable names to go into the routing layer
	// of PSYC. default is to handle them locally in the routing layer.
	// PSYC_ROUTING_MERGE means to merge them into end-to-end vars at
	// parsing time. PSYC_ROUTING_RENDER to accept and render them from
	// application provided vars. same data structure also in Net::PSYC.pm
	"routing": ([
	       "_amount_fragments" : PSYC_ROUTING,
	       "_context" : PSYC_ROUTING + PSYC_ROUTING_MERGE,
	       "_count" : PSYC_ROUTING + PSYC_ROUTING_MERGE,
	       // the name for this is supposed to be _count, not _counter
	       // this is brought in by ppp - let's ignore it for now
	       "_counter" : PSYC_ROUTING, // don't merge, don't render..
	       "_fragment" : PSYC_ROUTING,
	       "_length" : PSYC_ROUTING,
	       "_source" : PSYC_ROUTING + PSYC_ROUTING_MERGE,
	       "_source_identification" :
		    PSYC_ROUTING + PSYC_ROUTING_MERGE + PSYC_ROUTING_RENDER,
	       "_source_relay" : PSYC_ROUTING + PSYC_ROUTING_MERGE,
	       // until you have a better idea.. is this really in use?
	       "_source_relay_relay" :
		    PSYC_ROUTING + PSYC_ROUTING_MERGE + PSYC_ROUTING_RENDER,
#ifdef NEW_RENDER
	       "_tag" :
		    PSYC_ROUTING + PSYC_ROUTING_MERGE + PSYC_ROUTING_RENDER,
	       "_tag_relay" :
		    PSYC_ROUTING + PSYC_ROUTING_MERGE + PSYC_ROUTING_RENDER,
	       // should be obsolete, but.. TODO
	       "_tag_reply" :
		    PSYC_ROUTING + PSYC_ROUTING_MERGE + PSYC_ROUTING_RENDER,
#endif
	       "_target" : PSYC_ROUTING + PSYC_ROUTING_MERGE,
	       "_target_forward" : PSYC_ROUTING + PSYC_ROUTING_MERGE
				    + PSYC_ROUTING_RENDER,	// new!
	       "_target_relay" : PSYC_ROUTING + PSYC_ROUTING_MERGE,
	       "_understand_modules" : PSYC_ROUTING,
	       "_using_modules" : PSYC_ROUTING,
	]),
	"oauth_request_tokens": ([ ]),
]);

varargs mixed shared_memory(mixed datensatz, mixed value) {
	if (value) {
		share[datensatz] = value;
		// we don't need a function to delete a data set, do we?
		// anyway, this could be it:
		//unless (datensatz) return m_delete(share, value);
		// usage: shared_memory(0, datensatz);
	}
	if (datensatz) return share[datensatz];
	else return share;
}

#ifdef PSYC_SYNCHRONIZE
// one day this could be merged with monitor_report() into
// channels of the same context
static object sync;

int synchro_report(string mc, string text, mapping vars) {
	PT(("SYNCHROCAST %O from %O\n", mc, previous_object()))
	unless (sync) sync = load_object(PLACE_PATH "sync");
	if (sync) {
	    sync->msg(previous_object(), mc, text, vars);
	    return 1;
	}
	// if you really get here, you will have to parse this
	// file and resync from there
	log_file("SYNC_PANIC", "\n\n"+ time() +"\n"+ make_json( ({
	    psyc_name(previous_object()), mc, text, vars
	}) ));
	// hey! a new family.. call it _emergency? naah.. long live unix
	monitor_report("_panic_unavailable_synchronization",
			"Could not synchronize a "+mc);
	return 0;
}

int synchronize_contact(string mynick, string contact, int pplix, mixed value) {
	string s = share["ppl2psyc"][pplix];

	unless (s) return 0;
	if (share[s]) value = share[s][value];
	else value = PPLDEC(value);
	synchro_report("_notice_synchronize_contact",
	    "[_nick] has changed [_setting] for \"[_contact]\".", ([
		"_nick": mynick, "_contact": contact,
		"_setting": s, "_value": value,
	]));
	return 1;
}
#endif

