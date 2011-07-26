// vim:foldmethod=marker:syntax=lpc:noexpandtab
// $Id: person.c,v 1.722 2008/12/18 17:45:45 lynx Exp $
//
// person: a PSYC entity representing a human being

/*
	the difference from person.c to user.c is currently
	defined as follows: person.c implements the UNI logic
	of PSYC, whereas user.c *only* does "client" logic for
	dumb protocols. so everything that needs to work for an
	external real PSYC clients belongs into here, and everything
	that is not needed by a real PSYC client belongs outside.
	this distinction is particularely tricky for the display
	function w() which is actually more like a message to
	oneself, therefore needs a minimum implementation which
	forwards the stuff to the current location. so we also
	need textdb support in here.. well.. TODO someday..
	whenever we start using person without user.

	there's a thought in my head that says it may become
	useful to split this file some more so one can
	implement "caches" for remote psyc people that are
	*not* linked and represented by this server. just one
	out of many possible approaches to solve the problem
	of resolving nicknames into remote users. the other is
	to keep nick-to-uni/unl data structures in each object.
	but even basic information like uni and unl is already
	more than just one string, so it's prolly worth an
	object to handle it..
*/

// local debug messages - turn them on by using psyclpc -DDperson=<level>
#ifdef Dperson
# undef DEBUG
# define DEBUG Dperson
#endif

#include <driver.h>
#include <errno.h>
#include <net.h>
#include <person.h>
#include <psyc.h>
#include <storage.h>
#include <uniform.h>

#if __EFUN_DEFINED__(tls_query_connection_info)
# include <sys/tls.h>
#endif

inherit NET_PATH "group/master";
inherit NET_PATH "lastlog";

volatile mixed place;
volatile mapping places;

volatile int leaving, greeting;
volatile string lastmc;

volatile string nonce;

// this flag seems to indicate that any announce() has gone out
// not sure if ONLINE would do as well.. TODO
// TODO: remove logged_on, if (availability) provides same info
// but it probably doesn't - as avail is independent..
volatile int logged_on = 0;

#ifndef _flag_disable_module_presence
volatile int mood = MOOD_UNSPECIFIED;
// mood is one digit as defined in http://www.psyc.eu/presence
volatile mapping avail2mc;	// shared_memory()
# ifdef JABBER_PATH
volatile mapping mood2jabber;	// shared_memory()
# endif
# ifdef LASTAWAY
volatile mapping lastaway;
# endif
volatile mixed availability;
#endif // _flag_disable_module_presence

// complex data structure of peers. see peer.h
volatile mapping ppl;

// friends contains the currently available peers.
// technically, friends is a multi-dimensional mapping:
//	uniforms or objects pointing to nick and availability.
// it should be 0-dimensional instead, only containing objects:
//	local entities vs. context slaves for remote entities.
// in both cases providing the typical state information:
//	availability, mood, presence text, icons and photos...
volatile mapping friends;

#ifdef RELAY
volatile string remotesource;
#endif
#ifdef PRO_PATH
volatile string odata;
#endif

#ifdef ALIASES
volatile mapping aliases, raliases;
#endif


// this becomes somewhat confusing...
// user.h contains log and ppl stuff that a psyc person needs, too

#define NO_INHERIT
#include <user.h>
#undef NO_INHERIT

// local prototypes
logon(string host);
qFriends();
quit(int immediate, string variant);

// outgoing psyc object name
psycName() { return "~"+ MYLOWERNICK; }

// this looks trivial but it may get more complex one day than
// just to look at the existence of a password.. so proper
// abstraction must be maintained. in fact the pro variant of
// psyced accepts other forms of registration, too..
//
isNewbie() { return IS_NEWBIE; }
// "novice" maybe a nicer word?

#include <trust.h>

int get_trust(string id, string trustee, string profile) {
	int trust;

	unless (profile) profile = ppl[id];
	if (profile && stringp(profile)) {
		trust = profile[PPL_TRUST];
		if (trust == PPL_TRUST_DEFAULT) trust = profile[PPL_NOTIFY];
		else trust += TRUST_OVER_NOTIFY;
		PT(("%O get_trust(%O, %O, %O) = %d from profile\n", ME,
			id, trustee, profile, trust - '0'))
		return trust - '0';
	}
#ifndef _flag_disable_module_trust
	trust = ::get_trust(id, trustee);
	PT(("%O get_trust(%O, %O, %O) = %d from trustiness\n", ME,
		id, trustee, profile, trust))
	return trust;
#else
	return 0;
#endif
}

array(string) exposeGroups(int trustiness) {
#ifdef MAX_EXPOSE_GROUPS
	array(string) list = allocate(MAX_EXPOSE_GROUPS); 
#else
	array(string) list = allocate(sizeof(places));
#endif
	int defaul, i = 0;
	string loc, name;
	mixed k, t;

	defaul = trustiness + ( v("groupsexpose") || DEFAULT_EXPOSE_GROUPS )
		 > EXPOSE_THRESHOLD;
	foreach (k, name : places) {
		if (objectp(k)) loc = psyc_name(k); // doesnt happen
		else loc = k;
		t = PPL_EXPOSE_DEFAULT;
		// the user can have a profile for a place uniform
		if (ppl[loc]) t = ppl[loc][PPL_EXPOSE];
		PT(("%O -» %O, %O\n", k, t, to_int(t)))
		if (t == PPL_EXPOSE_DEFAULT && defaul)
		    list[i++] = loc;
		else {
		    t = t - '0';
		    if (t && t + trustiness > EXPOSE_THRESHOLD)
			list[i++] = loc;
		}
#ifdef MAX_EXPOSE_GROUPS
		if (i == MAX_EXPOSE_GROUPS) return list;
#endif
	}
	PT(("exposeGroups outbound » %O «\n", list))
	return i && list[.. i-1];
}

array(string) exposeFriends(int trustiness) {
	string profile;
	mixed k, t;
	int defaul, i = 0;
#ifdef MAX_EXPOSE_FRIENDS
	array(string) list = allocate(MAX_EXPOSE_FRIENDS); 
#else
	array(string) list = allocate(sizeof(ppl));
#endif

	defaul = trustiness + ( v("friendsexpose") || DEFAULT_EXPOSE_FRIENDS )
		 > EXPOSE_THRESHOLD;
	P4(("%O's exposeFriends(%O) -» %O .. (%O || %O)\n",
	     ME, trustiness, defaul,
		v("friendsexpose"), DEFAULT_EXPOSE_FRIENDS ))
	foreach (k, profile : ppl)
	    // only expose those we are also friends with
	    // this automatically skips places
	    if (profile[PPL_NOTIFY] >= PPL_NOTIFY_FRIEND) {
		t = profile[PPL_EXPOSE];
//              PT(("%O -» %O, %O\n", k, t, to_int(t)))
		if (t == PPL_EXPOSE_DEFAULT && defaul)
		    list[i++] = UNIFORM(k);
		else {
		    t = t - '0';
		    if (t && t + trustiness > EXPOSE_THRESHOLD)
			list[i++] = UNIFORM(k);
		}
#ifdef MAX_EXPOSE_FRIENDS
		if (i == MAX_EXPOSE_FRIENDS) return list;
#endif
	}
	P4(("exposeFriends outbound » %O «\n", list))
	return i && list[.. i-1];
}

// when calling this externally you are committing a capital crime if
// you pass false information here. please pass the identification of
// the caller (either object for local or UNI for remote). you can use
// lookup_identification() to figure those out. don't pass a profile,
// that's just for internal calls. you may as well leave source out.
// see also qPublicInfo()
qDescription(source, vars, profile, itsme) {
	mapping dv;
	int trust, idle = 0;

#if 0
	unless(profile) {
		// wieso der wiederholte lookup?
		// .. bei examine via web ist das noch nicht gesetzt
		P1(("%O description() request from %O for %O\n",
		    ME, previous_object(), source))
		if (source) {
			if (objectp(source))
			    profile = ppl[ source->qNameLower() ];
			else profile = ppl[source];
		}
	} else
#endif
	//  unless (stringp(profile)) profile = 0;
	// "ignore" == PPL_DISPLAY_NONE has already been handled in msg()

	// is "knowing the nickname" a sufficient reason to give some
	// kind of an answer, even if stripped down? not really.
	// it's like EXPN and VRFY in SMTP. doomed to die by abuse.

		// TRUST_MYSELF in a way, but we have the '0' offset here
	if (itsme) trust = '9' + TRUST_OVER_NOTIFY; // that's me
	else if (member(vars, "_trust") && intp(trust = vars["_trust"])) {
		// lynx fragt:	das ist niemals ein _trust, welches mir der
		//		anfragende untergejubelt hat?
		// el verwirrt:	ne, es sei denn nen object tut das
		trust += '0';
	} else trust = get_trust(objectp(source) ?
			 vars["_nick"] : source, 0, profile) + '0';

	if (v("visibility") == "off" && trust < PPL_NOTIFY_FRIEND)
	    return 0;
	// we could greatly simplify all these decisions what belongs into
	// the outgoing description for whom, if we introduced a psyc2trust
	// mapping into profiles.gen with 'trust <digit>' values for each
	// entry. then avoid repetitive tasks like de facto duplicating the
	// set2psyc mapping here by writing up these lines:
	dv = ([ "_nick": v("name"),
		// lynX, you can't add more fields here if you're not sure
		// they actually exist!  :(  --lynX
		"_identification": // v("identification") ||
		     v("_source"),
// similar code also in net/place/basic.c
#ifdef IRC_PATH
		// TODO: right now we have the jid twice, but we should rather
		// make _identification_scheme_* vars for each of these,
		// then define a syntax which allows templates to collect
		// all matches of _identification_scheme* or _identification*
		// into a list-string for display.
		// what about "Identifications: [_identification*]" ?
		"_identification_alias":
// we need to make this http url more interesting first..
#if 0
# if HAS_PORT(HTTP_PORT, HTTP_PATH)
		     HTTP_URL +"/~"+ MYNICK +" "+
# else
#  if HAS_PORT(HTTPS_PORT, HTTP_PATH)
		     HTTPS_URL +"/~"+ MYNICK +" "+
#  endif
# endif
#endif
# if 0 // def SIP_PATH
		     "sip:"+ MYLOWERNICK +"@"+ SERVER_HOST +" "+
# endif
# ifdef _host_XMPP
		     "xmpp:"+ MYLOWERNICK +"@"+ _host_XMPP +" "+
# else
#  ifdef JABBER_PATH
		     "xmpp:"+ MYLOWERNICK +"@"+ SERVER_HOST +" "+
#  endif
# endif
		     "irc://"+ SERVER_HOST +"/~"+ MYNICK,
#endif
#ifdef JABBER_PATH
		"_identification_scheme_XMPP":
# ifdef _host_XMPP
		     MYLOWERNICK +"@" _host_XMPP
# else
		     MYLOWERNICK +"@" SERVER_HOST
# endif
#endif
		// yes, putting description data into an XML structured tree
		// may also be a solution, but then we lose the advantage of
		// a flat name access. it would require use to implement
		// xpath to do our psyctext templates. jeez! so, as long as
		// the psyc inheritance plan solves our problems, let's steer
		// clear of xml, although we're not religious about this.
		// xml may be an answer sometime somewhere.
	]);
	if (v("scheme")) {
		dv["_protocol_agent"] = v("scheme");
		if (v("layout"))
		    dv["_agent_design"] = v("layout");
	}
#if __EFUN_DEFINED__(tls_query_connection_info)
	if (interactive(ME) && tls_query_connection_state(ME)) {
		array(mixed) tls = tls_query_connection_info(ME);

		if (tls[TLS_COMP] > 1)
		    dv["_circuit_compression"] =
		    TLS_COMP_NAME(tls[TLS_COMP]);
		if (tls[TLS_CIPHER])
		    dv["_circuit_encryption_cipher"] =
		    intp(tls[TLS_CIPHER]) ? TLS_CIPHER_NAME(tls[TLS_CIPHER])
					: tls[TLS_CIPHER];
		if (tls[TLS_PROT])
		    dv["_circuit_encryption_protocol"] =
		    intp(tls[TLS_PROT]) ? TLS_PROT_NAME(tls[TLS_PROT])
					: tls[TLS_PROT];
	}
#endif
	// should idle time be visible to friends only?
	// or should it be a /set visibility medium feature?
	if (v("me")) dv["_action_motto"] = v("me");
	if (v("languages"))
	    dv["_language_alternate"] = v("languages");
	if (v("affiliation"))
	    dv["_affiliation"] = v("affiliation");
	if (v("publicpage"))
	    dv["_page_public"] = v("publicpage");
	if (v("publictext"))
	    dv["_description_public"] = v("publictext");
	if (v("mottotext"))
	    dv["_description_motto"] = v("mottotext");
	if (v("publicname"))
	    dv["_name_public"] = v("publicname");
	if (v("stylefile"))
	    dv["_uniform_style"] = v("stylefile");
	if (v("miniphotofile"))
	    dv["_uniform_photo_small"] = v("miniphotofile");
	if (v("photopage"))
	    dv["_page_photo"] = v("photopage");
	
	if (trust >= PPL_NOTIFY_FRIEND) {
		unless (v("exposetime")) {
			CALC_IDLE_TIME(idle);
			if (idle) 
			    dv["_time_idle"] = idle;
// if you need it as timedelta, create it out of _time_idle
//			if (idle > 30)
//			    dv["_INTERNAL_time_idle"] = timedelta(idle);
		}
		if (v("age"))
		    dv["_time_age"] = v("age");
		if (v("privatetext"))
		    dv["_description_private"] =
				  v("privatetext");
		if (v("likestext"))
		    dv["_description_preferences"] =
				  v("likestext");
		if (v("dislikestext"))
		    dv["_description_preferences_not"] =
				  v("dislikestext");
		if (v("privatepage"))
		    dv["_page_private"] = v("privatepage");
		if (v("keyfile"))
		    dv["_uniform_key_public"] = v("keyfile");
		if (v("email"))
		    dv["_identification_scheme_mailto"] = v("email");
		    // this will most likely be renamed
		if (v("photofile"))
		    dv["_uniform_photo"] = v("photofile");
		if (v("animalfave"))
		    dv["_favorite_animal"] = v("animalfave");
		if (v("popstarfave"))
		    dv["_favorite_popstar"] = v("popstarfave");
		if (v("musicfave"))
		    dv["_favorite_music"] = v("musicfave");
		if (v("color"))
		    dv["_color"] = v("color");
		if (v("language"))
		    dv["_language"] = v("language");
		if (v("timezone"))
		    dv["_address_zone_time"] = v("timezone");
		if (v("telephone"))
		    dv["_contact_telephone"] = v("telephone");
		// this is not compatible with vCard right now
		// i just felt spontaneous.. sorry ;)
		// so this may disappear in favour of such horrid
		// things like /set skype ...
		if (v("identities"))
		    dv["_contact_identities"] = v("identities");

		if (v("groupsexpose") != "off" && sizeof(places)) {
			array(string) l = exposeGroups( to_int(trust)-'0' );
			if (l) dv["_list_groups"] = l;	// _tab
		}
// hab mir überlegt das tobijschiger zu lösen.. der /x selbst zeigt keine
// freunde an, sondern wir schicken einen _request_description mit friendivity
// level ab und lösen einen castmsg() aus. dann erfahren die freunde, dass
// jemand gerne mehr über sie wissen will, und sie können entscheiden darauf
// zu antworten. also das erforschen der freundschaften selbst als friendcast.
// dazu muss ein neuer befehl her.. /examine <user> friends oder /discover?
// die frage ist auch, antwortet man gleich mit descriptions? für guis wär
// das toll: kann man den friendspace gleich grafisch aufbereiten mit fotos -
// oder antwortet man erstmal dezent mit einem "hallo, mich gibt es"
//
// weiterer update: hat das noch irgendeinen sinn im dezentralen state?
// also im plex in dem wir über freundesfreunde per push bereits bescheid
// wissen sollten falls keiner was dagegen hat?
//
		// plenty to be done here, but this is a simple start
		if (v("friendsexpose") != "off" && sizeof(ppl)) {
			array(string) l = exposeFriends( to_int(trust)-'0' );
			if (l) dv["_list_friends"] = l;	// _tab
		}
	}
	else unless (FILTERED(source)) {
		if (v("color"))
		    dv["_color"] = v("color");
		if (v("language"))
		    dv["_language"] = v("language");

		// we don't know if this person is a friend of a friend, but..
		// you can set your level high enough if you don't mind at all
		if (intp(v("friendsexpose")) && v("friendsexpose") > 0) {
			array(string) l = exposeFriends( 0 );
			if (l) dv["_list_friends"] = l;	// _tab
		}
	}
	if (boss(source)) {
		if (v("agent"))
		    dv["_version_agent"] = v("agent");
		if (v("forwarded"))
		    dv["_host_forwarded"] = v("forwarded");
		if (v("host"))
		    dv["_host_name"] = v("host");
		// else?
		if (v("ip"))
		    dv["_host_IP"] = v("ip");
		if (v("gender"))
		    dv["_gender"] = v("gender");
		if (v("birth"))
		    dv["_date_birth"] = v("birth");
		if (v("region"))
		    dv["_geographic_region"] = v("region");
		if (v("fname"))
		    dv["_name_first"] = v("fname");
	}

#ifdef _flag_enable_module_microblogging
	mapping channels = ([ ]);
	foreach (string c : v("channels")) {
	    object p = find_place(c);
	    unless (objectp(p) && (p->isPublic() || (source && p->qMember(source))) /*&& p->numEntries() > 0*/) continue;
	    channels += ([ p->qChannel(): p->entries(10, 0, 1)]);
	}
	// don't make_json for anonymous queries which are handled locally
	dv["_channels"] = source ? make_json(channels) : channels;
	dv["_profile_url"] = HTTPS_OR_HTTP_URL + "/~" + MYNICK;
#endif
//	PT(("sending: %O\n", dv))
	return dv;
}

qLocation(string service) {
	ASSERT("qLocation", v("locations"), v("locations"))
	// which location should we return? it just returns one for now. (only used from sip/udp)
	if (member(v("locations"), service) && sizeof(v("locations")[service])) return m_indices(v("locations")[service])[0];
}

// returns 0 if that was just an update. 1 on success.
// this was originally used by sip/udp only, then slowly
// integrated into existing code
sLocation(string service, mixed data) {
	ASSERT("sLocation", v("locations"), v("locations"))
	// should this function also call register_location ?
	// yes because a delivery error should remove clients
	// from the location table, too.. not just proper
	// unlink requests  FIXME
	if (member(v("locations"), service) && member(v("locations")[service], data)) return 0;
	unless (data) {
		//string retval = v("locations")[service];
		m_delete(v("locations"), service);
		return 1; //retval;
	}
	if (member(v("locations"), service)) {
	    v("locations")[service] += ([ data ]);
	} else {
	    v("locations")[service]  = ([ data ]);
	}
	return 1;
}

static linkSet(service, location, source) {
	P2(("linkSet(%O, %O, %O) called in %O: linking.\n",
	    service, location, source, ME));
	// sLocation?
	unless (location) location = source;
	else unless (source) unless (source = location)
	    raise_error("You have to provide either source or location!\n");
	if (member(v("locations"), service)) {
	    v("locations")[service] += ([ location ]);
	} else {
	    v("locations")[service]  = ([ location ]);
	}
	register_location(location, ME);
	// let me know if you lose the connection to my link!
	object circuit = find_target_handler(location);
	if (objectp(circuit)) {
	    P3(("linkSet: registering disc notify for %O from %O.\n",
		location, circuit))
	    circuit -> register_link(ME);
	}
	// probably should go to the user instead
	else P1(("Warning for %O: Location %O will ghost.\n", MYNICK, location))
	if (service) sendmsg(source, "_notice_link_service", 0,
			   ([ "_service" : service,
			     "_location" : location,
			"_identification": v("_source"),
				 "_nick" : MYNICK ]));
	else {
		sendmsg(source, "_notice_link", 0, ([
			     "_location" : location,
			"_identification": v("_source"),
				 "_nick" : MYNICK ]));
		// <el> PSYCion users dont have queries.
		// until there are more clients this will 
		// be enough
		vDel("query");
		// <lynX> clients either use _request_input
		// and thus support current place and query,
		// or otherwise _message to places and people
		// and never use _request_input, therefore
		// not get into trouble with query & place....?
// grmbl, this is making tons of trouble
//		vDel("place");
		// <lynX> psyced acts indeed too complicated
		// for simple clients on link when a place is set
	}
	P2(("locations after linkSet: %O\n", v("locations")))
}
linkDel(service, source, variant) {
        P3(("linkDel(%O, %O, %O) called in %O!\n", service, source, variant, ME))
	string mc = "_notice_unlink";
	service = service || 0;

	unless (member(v("locations"), service)) {
		P4(("linkDel(%O, %O) called in %O: no such candidate!\n",
		    service, source, ME));
		return 0;
	}

	int n = 0;
	foreach(string candidate : v("locations")[service]) {
	    P4(("linkDel(%O, %O) in %O: unlinking %O ? handler = %O\n",
		service, source, ME, candidate, find_target_handler(candidate)));
	    if ((objectp(source) && find_target_handler(candidate) != source) ||
		(source && !objectp(source) && candidate != source)) continue;
	    P2(("linkDel(%O, %O) in %O: unlinking %O.\n",
		service, source, ME, candidate));
	    // sLocation?
	    register_location(candidate, 0);
	    // maybe actual deletion would need to be delayed after
	    // letting locations know. they might still be sending
	    // stuff to us, right?
	    m_delete(v("locations")[service], candidate);
	    unless (sizeof(v("locations")[service]))
		m_delete(v("locations"), service);
	    if (variant) mc += variant;
	    if (service) sendmsg(candidate, mc, 0,
				 ([ "_service" : service,
				    "_location_service" : candidate,
				    "_identification" : v("_source") ]));
	    else sendmsg(candidate, mc, 0,
			 ([ "_location" : candidate,
			    "_identification" : v("_source") ]));
	    n++;
	}
	return n;
}
static linkCleanUp(variant) {
	mixed type, loc;

	foreach (type, loc : v("locations")) {
		P2(("linkCleanUp(%O) to %O's ex-%O-client %O\n",
		    variant, ME, type, loc))
		linkDel(type, 0, variant);
	}
}

// extend sName() from name.c
sName2(a) {
	int e;
	string b;

	if (MYNICK) return 0;

	b = lower_case(a);
	e = load(PERSON_DATA_FILE(b));
	if (e && e != ENOENT) {
		log_file("PANIC", "load(%O) returned %O\n", a, e);
		// D(S("PANIC! load(%O) returned %O!\n", a, e));

		this_player()->w("_failure_object_restore_err" + e,
"Object could not be restored!");

#ifdef PANIC_ON_NO_ADMIN
		shutdown();
#endif
		destruct(ME);
		return 0;
	}
#ifdef PERSISTENT_MASTERS
	// in persons it is easier to reconstruct it from scratch
	_routes = ([ ]);
#endif
	// backwards compatibility to the times before we had tokens
	// unless (v("password")) vSet("password", v("token"));

	// let user objects set it anew, otherwise we'll have probs
	// distinguishing net/user from net/something/user
	vDel("scheme");
#ifdef BRAIN
	vDel("location");	// clean out from earlier versions
				// we now use v("locations")[_service]
#endif
#if SYSTEM_CHARSET == "ISO-8859-15"
	if (v("charset") == "ISO-8859-1") vDel("charset");
#endif
	if ( (b = v("name")) &&! stricmp(a, b) ) {
		// support for stored nickname writing style
		a = b;
		P3((" [ %O:sName %O ] ", ME, a))
	} else {
		// support for "cp oldnick.o newnick.o" w/out patching
		vSet("name", a);
		P3((" [ %O:sName %O override ] ", ME, a))
	}
#ifdef ALIASES
	if (v("aliases")) {
		string k, l;

		foreach (k, l : v("aliases")) {
		    aliases[lower_case(k)] = l;
		    raliases[l] = k;
		}
	} else vSet("aliases", ([]));
#endif
#ifndef _flag_disable_module_presence
	// let's see if there's anything bad about
	// persistent mood & availability.. if not,
	// we should remove the local vars.
	mood = v("mood") || MOOD_UNSPECIFIED;
	availability = v("availability");
#endif // _flag_disable_module_presence

#ifdef _flag_enable_module_microblogging
	unless (v("channels")) vSet("channels", ([]));
#endif
	// protection against file read errors
	if (IS_NEWBIE) {
		if (boss(a)) {
			log_file("PANIC", "load(%O) failed\n", a);

			this_player()->w("_failure_object_restore_admin",
"You are registered as admin, but I could not restore your data!");
			// raise_error("boss without password\n");
#ifdef PANIC_ON_NO_ADMIN
			shutdown();
#endif
			destruct(ME);
			return 0;
		}
#ifdef _flag_enable_administrator_by_nick
		else if (strstr(lower_case(a), "admin") != -1) {
			this_player()->w("_failure_object_create_admin",
"This nickname is available to administrators only.");
			destruct(ME);
			return 0;
		}
#endif
	} else {
		if (v("emailvalidity") && v("email"))
		    register_target("mailto:"+ lower_case(v("email")));
	}
	::sName(a);
	register_person(MYLOWERNICK, ME);

	// maybe use v("identification") here?
	vSet("_source", psyc_name(ME));

	// has to happen *after* psyc_name() ... fixes ~0 bug ... thx tg
	if (v("locations")) linkCleanUp("_crash");
	else vSet("locations", ([ ]));

	return MYNICK; // means new name accepted
}

remove() {
#ifndef USE_LIVING
	register_person(MYLOWERNICK, 0);
#endif
}

// this is called by all login procedures, even psyc
//
// fippo has extended it with a closure to be able to do asynchronous
// credentials checks, but it is rather obvious that no other piece of
// code in psyced requires such a hard and ugly approach to solve such a
// problem, so this could probably be solved in a more elegant way..
//
// as I just noticed this... this MUST be asynchronous. What if you 
// need to lookup the authentication data from a database first? 
// from ldap? IF you find a more elegant solution to this go ahead, but 
// until then this is necessary.
//
checkPassword(try, method, salt, args, cb, varargs cbargs) {
	string HA1, HA2;
	string t1, rc;

	P3(("%O checkPassword(%O,%O,%O,%O,%O,%O)\n", ME,
	    try,method,salt,args,cb,cbargs))
#ifdef ASYNC_AUTH
    // und endlich darf saga einen komma-operator im psyced bewundern:
# define ARETURN(RET) {\
	P2(("returning async auth %O to %O in %O\n", RET, cbargs, ME)) \
	return apply(cb, RET, cbargs), RET; \
}
#else
# echo Warning: ASYNC_AUTH not activated.
# define ARETURN(RET) return RET;
#endif
#ifdef NO_EXTERNAL_LOGINS
	ARETURN(0)		// used by some MUDs
#endif
	// why here?
	//while (remove_call_out(#'quit) != -1);
#ifndef REGISTERED_USERS_ONLY
# ifdef AUTH_HMAC_SECRET
        if (IS_NEWBIE && method != "hmac-sha1-shared") ARETURN(1)
# else
	if (IS_NEWBIE) ARETURN(1) // could auto-register here..
# endif
#endif
	if (!try || try == "" || v("password") == "") ARETURN(0)

	switch(method) {
#if __EFUN_DEFINED__(sha1)
case "SHA1":
case "sha1":
		P3(("SHA1 given %O vs calculated %O\n",
		    try, sha1(salt + v("password"))));
		rc = try == sha1(salt + v("password"));
		ARETURN(rc)
# ifdef TLS_HASH_SHA1
case "HMAC-SHA1":
case "hmac-sha1":
		ARETURN(try == hmac(TLS_HASH_SHA1, v("password"), salt))
#  ifdef AUTH_HMAC_SECRET
#   define _flag_disable_registration
case "hmac-sha1-shared":
		if (try == hmac(TLS_HASH_SHA1, AUTH_HMAC_SECRET, salt + MYNICK)) {
		    if (IS_NEWBIE) {
			vSet("password", "");
			save();
		    }
		    ARETURN(1)
		} else ARETURN(0)
#  endif
# endif
#else
# echo Driver is missing SHA1 support (needed for jabber)
#endif
#if __EFUN_DEFINED__(md5)
case "MD5":
case "md5":
		rc = try == md5(salt + v("password"));
		ARETURN(rc)
case "http-digest": // see RFC 2617
		unless(mappingp(args)) ARETURN(0)
		// v("name") == args["username"] ???
		HA1 = md5(args["_username"] + ":" + args["_realm"] + ":" + v("password"));
		HA2 = md5(args["_method"] + ":" + args["_uri"]);
		rc = try == md5(HA1 + ":" + salt + ":" + HA2);
		ARETURN(rc)
// SASL digest-md5. fippo hotzenplotzt: digest-md5 ist ein sasl bastard
//					den sogar die ietf abschaffen will
case "digest-md5":
		// this assumes try set to 1 and args
		// containing the sasl_parse'd response
		// this will return the rspauth value if successful
		t1 = sasl_calculate_digestMD5(args, v("password"), 0, v("prehash"));
		P3(("sasl macht %O != %O\n", t1, args))
		if (args["response"] == t1) {
		    rc = sasl_calculate_digestMD5(args, v("password"), 1, v("prehash"));
		    ARETURN(rc)
		} else ARETURN(0)
#endif
default:
		P4(("plain text pw %O == %O?\n", try, v("password")))
#ifdef PASSWORDCHECK
		PASSWORDCHECK(v("password"), try)
#else
		if (try == v("password")) ARETURN(1);
#endif
	}
	if (v("token") == try) {
		// D("entry by token accepted once\n");
//		vDel("token");
		if (v("tokencredit") == v("email")) {
		    unless (v("emailvalidity")) {
			// first time.. allocate mailaddr  TODO
			// for now we just save() into both files
		    }
		    vSet("emailvalidity", time());
		}
		vDel("tokencredit");
		// save(); -- doesnt work here.. strange
		ARETURN(1);
	}
	ARETURN(0);
}




/***

the stuff that follows used to be in user.c

***/

#ifndef _flag_disable_module_presence
static presence_management(source, mc, data, vars, profile, avail) {
	int display = 1;
	string t;

	unless (avail)
	    avail = vars["_degree_availability"] || AVAILABILITY_HERE;
	// don't display if we already know it  .... optTODO
	if (friends[source, FRIEND_AVAILABILITY] == avail) display = 0;
	else if (v("presencefilter") == "all") display = 0;
	P3(("pmgmt in %O from %O display %O filter %O\n", ME, source,
	    display, v("presencefilter")))

	if (profile && profile[PPL_NOTIFY] >= PPL_NOTIFY_FRIEND) {
#if 0 //def ALIASES.. tobij.. what does this do?
	    // same code a few lines below
	    // and it is still not enough.. there are so many places
	    // where friends[] is accessed.. we need a more generic
	    // plan for handling aliases.. it's making me crazy. TODO
	    if (t = raliases[source]) 
		friends[source, FRIEND_NICK] = t;
	    else if ((t = vars["_nick"]) && aliases[lower_case(t)])
		friends[source, FRIEND_NICK] = 1;
	    else
#endif
		t = vars["_nick"];

		friends[source, FRIEND_NICK] = t || 1;
#ifdef IRC_FRIENDCHANNEL
		vars["_degree_availability_old"] = friends[source, FRIEND_AVAILABILITY];
#endif
		friends[source, FRIEND_AVAILABILITY] = avail;
#if 0 // alle freunde sind in unserem castmsg bereits drin, allein schon
// weil sie unsere freunde sind, und umgekehrt wir in ihrem.
// genau das wird in zeile 1666 etwa erledigt.
// ihre jeweilige derzeitige presence hat damit nichts zu tun -
// diese stelle ist so gesehen also redundant.
// gibt es gründe weswegen wir redundant sein sollten!??
// kann jemand während seiner abwesenheit aus unserer cast group
// gefallen sein, weil er nicht erreichbar war oder sowas?
// wenn sowas denkbar ist, dann wäre diese maßnahme zur
// rehabilitierung der person angemessen..  -lynX
//
// ausserdem verursachte das fiese bugs, weil unsachgemaess der nick
// herangezogen wurde statt der source
		// add our friend to group/master, syncing..
		if (t2 = parse_uniform(t)) {
			insert_member(t, t2[URoot]);	  // outgoing presence
			if (t2[UScheme] == "psyc")
			    register_context(ME, t, t2[URoot]);  // incoming presence
		} else
		    insert_member(summon_person(t) || t);
#endif
	} else {
		// let's look at this a bit more..
		P1(("%O received %O from stranger (%O w/profile %O)\n",
		   ME, mc, source, profile))
		display = 0;
	}
	return display;
}
#endif // _flag_disable_module_presence

// PSYC-conformant message receiving function
//
// this part of the standard message handler for people only
// handles stuff that doesnt get pr'inted..
//
// return value 1 means: go ahead and print something
// return value 0 means: ignore this silently
//
// BEWARE! the vars mapping is holy & untouchable!
// we never change it because we only use one instance in all of
// people's /log's to save a little memory and cpu, so be careful.
// 
// i have added a #define VARS_IS_SACRED which may be defined for
// certain occasions but normally isn't. but you should still be careful.
//
msg(mixed source, mc, data, mapping vars, showingLog) {
	int glyph, itsme = 0;
	string k, display, profile, family;
	mixed t, t2, psource;

	// wrong to initialize here.. TODO
	mixed rvars = ([ "_nick" : MYNICK ]); // replyvars

	display = "";
#if DEBUG > 1
	P3(("%O person:msg(%O,%O,%O..)\n", ME, source,mc,data))
	unless (mappingp(vars)) raise_error("vars are no mapping\n");
#else
	unless (mappingp(vars)) vars = ([]);	// should never happen
#endif
	// if (vars["_time"]) return 1;	// logView() in action
	if (showingLog) return display;	// logView() in action
	// btw, when reviewing log, all users are displayed equal size

#ifdef RELAY
	remotesource = 0;
#endif
	// person::msg() requires source to be non-zero and objectp for local
	// objects. this has to be ensured before getting here, but it isn't.
	// TODO!
#ifdef _flag_disable_module_trust
	if (stringp(source)) {
#endif
#if 0
		// has to be in user.c because of rplaces... hmm
		if (vars["_context"] && !objectp(vars["_context"]) &&
		    !rplaces[vars["_context"]] &&
		    !abbrev("_notice_place", mc)) {
			monitor_report("_warning_abuse_invalid_context",
			    S("Invalid context %O in %O apparently from %O",
				vars["_context"], mc, source));
			return 0;
		}
#endif
		psource = source;
		//t = lookup_identification(source, vars["_identification"]);
		// uni::msg finds the _identification for us
		// psource then still contains the UNL while source has the UNI
		// we sendmsg() to the UNI because uni::sendmsg will replace
		// that with UNL
		unless(::msg(&source, mc, data, vars)) return 0;
#ifndef _flag_disable_module_trust
	if (stringp(source)) {
#endif
		if (source == ME || ME == vars["_source_identification"]) itsme = 1;
		// how did we get here with ppl undefined?
		// oh we had an error earlier, that's why
		else if (ppl) {
			// entity.c handles _INTERNAL_identification for us
			profile = ppl[source];
#ifdef RELAY
			remotesource = vars && vars["_INTERNAL_identification"] ? vars["_INTERNAL_identification"] : source;
#endif
			P3(("%O profile for %O = %O is %O\n",
			    ME, source, psource, profile))
#ifdef HOST_IGNORE
			unless (profile) {
				// check if this user ignores all users
				// from a specific host, maybe cache the
				// result?
			}
#endif
		}
		P4(("%O got %O from %O. itsme: %O\n", ME, mc, source, itsme))
		// here comes the psyc intelligence
		PSYC_TRY(mc) {
#ifndef _flag_disable_module_authentication
case "_notice_processing_authentication":
			P1(("asyncAUTH %O: %O is processing %O\n", ME, source, vars))
			break;
case "_request_authentication":
case "_request_authenticate":
			t = checkAuthentication(source, vars);
			returnAuthentication(t, source, vars);
			if (t) return 0;
			// if automatic auth failed we display the request
			// to the user so he can authenticate manually...
			// but in fact he should have done /token first.
			break;
#endif
case "_request_location":
			// t == source if identification lookup didn't succeed?
			unless (member(friends, t || source)) {
#if 0
			    if (vars["_service"]) {
				if (services[vars["_service"]]) {
				    sendmsg(source, "_info_location_"
					    + vars["_service"],
					    "[_service] has location "
					    "[_location].",
					    ([
					     "_service" : vars["_service"],
					     "_location" : v("_source") + "/$"
					       + vars["_service"],
					    ]));
				} else {
				    sendmsg(source, "_error_location_"
					    + vars["_service"],
					    "No such service ([_service]).",
					    ([
					     "_service" : vars["_service"]
					    ]));
				}
			    } else 
#endif
			    {
				sendmsg(source,
				    "_error_rejected_query_location", 0, ([]));
			    }
			    return 0;
			}
			if (vars["_service"] && member(v("locations"), vars["_service"])) {
			    foreach (string location : v("locations")[vars["_service"]]) {
				// service
				sendmsg(source, "_info_location_"+ vars["_service"],
					"[_service] has location [_location].",
					([
					  "_service" : vars["_service"],
					  "_tag" : vars["_tag"],
					  "_location" : location,
					  ]));
			    }
			} else {
			    foreach (string location : v("locations")[0]) {
				sendmsg(source, "_info_location", 0,
					([
					  "_nick" : MYNICK,
					  "_tag" : vars["_tag"],
					  "_location" : location
					  ]));
			    }
			}
			return 0;
case "_request_link":
case "_set_password":
			P3(("_request_link for %O. vars %O\n", ME, vars))
	// TODO: shouldn't we use some kind of observer pattern on the
	// 	current_interactive to become aware of disconnects?
	// 	at least if the current interactive is not a server2server
	// 	socket this is necessary
#ifdef ASYNC_AUTH
			checkPassword(vars["_password"], vars["_method"], nonce, vars, (:
			    if ($1)
#else
			if (checkPassword(vars["_password"], vars["_method"], nonce, vars))
#endif
			{
				mixed *u;

				// TODO? add support for integer _service means multiple
				// catch-all clients possible. do we want this?
				if (vars["_service"]) {
				    //linkDel(vars["_service"]);
				    linkSet(vars["_service"], vars["_location"], source);
				    return 0;
				}
				// this code should also run for _service, but it
				// needs a reorg
				if (member(v("locations"), 0) && sizeof(v("locations")[0]) && !member(v("locations")[0], source)) {
					// alright. we have another client
					// already, or it is a ghost.
					if (!vars["_password"] && ONLINE) {
						// we are a newbie. reject the
						// kick-out request.
						sendmsg(source,
						    "_error_status_person_connected", 0,
						    ([ "_nick": MYNICK ]));
						return 0;
					}
					// we are a legitimate new client.
					// lets inform the old one
					//linkDel(0, t);
					// now we leave the old client circuit
					// to die off.. let's hope that's safe
				}
#ifdef _flag_disable_module_trust
				unless(stringp(source)) {
					m_delete(v("locations"), 0);
					return display;
				}
#endif
#if 0
// no new link should throw out interactives or other links!
// this must be from the days when we had no disconnect notification
// so it would remove ghosts of itself..
// his should be unnecessary by now!  --lynX 2011
				// <el> in other cases this is done by
				// morph. this may not be the very best
				// solution, but until person/user is rewritten
				// this is a fix for psyc-clients
				// ...
				// <lynX> in fact once there was a time when
				// psyc clients could co-exist with one legacy
				// client or access interface using that user.c
				// no matter which one it was. i don't know why
				// this no longer works, however i'd like doc
				// this in case one day we can if 0 this again.
				// .. or, as you correctly said, one day we
				// detach UNI from client interface and allow
				// all the protocols to coexist for each user
				// unlimitedly .. it's on the TODO
				//
				// TODO: fippo - darum geht FORCE PLACE in 
				// 		die hose
				// scheme != psyc
				// allows two psyc clients to be logged in 
				// concurrently and is very suspicious
				if (query_once_interactive(ME)
					|| (v("scheme") && v("scheme") != "psyc")) {
				    // temporary fix for initially created psyc
				    // users. (they dont have a scheme)
				    object o;
				    save();
				    if (interactive(ME)) {
					    // linkDel(); doesn't make sense here
					    remove_interactive(ME);
				    }
				    o = named_clone(PSYC_PATH "user", MYNICK);
				    //o->sName(MYNICK);
				    // scheme is not set if a psyc/user is
				    // initially created... 
				    o->vSet("scheme", "psyc");

				    o->msg(source, mc, data, vars);
				    return destruct(ME); 
				}
#endif
				// used by _request_authentication
				if (u = parse_uniform(source)) vSet("ip", u[UHost]);
				// nicht wirklich logisch, unsere varnames!
				if (t = vars["_version_agent"]
				     || vars["_implementation"])
				    vSet("agent", t);

				// in the protocol _nick at least determines
				// the layout (case etc.) of the nick.
				// if a client thinks he knows better than
				// the identity, and sends _nick on
				// _request_link, then the identity should
				// accept that probably.

				// there are discussions whether clients
				// or the identity should determine the case,
				// but _iff_ a client sends _nick, the identity
				// must follow, imho.			-- saga
				t = vars["_nick"];
				// das unwahrscheinlichste als erstes
				// prüfen ist effizienter:
				if (t && t != MYNICK
				    // redundant:
				    //&& !stricmp(v("name"), t)
				    // absichern und abstempeln:
				    && lower_case(t) == MYLOWERNICK) {
					if (::sName(t)) vSet("name", MYNICK);
				}

				// register location on IP address of source?
				// no: psyc/parse should always produce the
				// same source when the location speaks to us
				// thus if there is a problem, the solution
				// must be found elsewhere. also: registering
				// the whole IP will produce uncontrollable
				// side fx when several people come from the
				// same firewall or NAT.
    P2(("_request_link in %O: TI is %O, src is %O, vars is %O\n",
	ME, this_interactive(), source, vars))
#if 0
    ASSERT("origin == this_interactive",
	vars["_INTERNAL_origin"] == this_interactive(), vars);
				// language support for clients..
				// done in w() instead.
				vars["_INTERNAL_origin"]->sTextPath(v("layout"),
                                    v("language"), v("scheme"));
#endif
				// yeah right..
				//unless (interactive()) vSet("host", source);
				linkSet(0, vars["_location"], source);
				// moved logon after _notice_linked
				// which is more appropriate for most clients
				// lets see if theres any problem with that
				logon(source);
				// yes, there was:
				// qFriends() is empty before logon, so
				// we need to do this afterwards
// in http://about.psyc.eu/Client_coders kuchn states that this
// message is redundant anyway as we also have _list_friends_present.
#if 0 //ndef ASYNC_AUTH
				// TODO THIS IS BROKEN DUE TO ASYNC AUTH!
				// some closure expert should fix this
				if (t = (mixed)qFriends()) sendmsg(source, 
				    "_status_friends", 0,
					([ "=_friends": t ]) );	// _tab
				else sendmsg(source,
				    "_status_friends_none", 0, ([]));
#endif
				return 0;
			    }
			    if (vars["_password"])
				sendmsg(source,
				    "_error_invalid_password", 0,
				    ([ "_nick": MYNICK ]));
			    else {
				nonce = RANDHEXSTRING;
				if (v("me")) sendmsg(source, "_info_description", 0,
					     ([
						"_nick" : MYNICK,
						"_color" : v("color"),
						"_description_action" : v("me"),
						"_INTERNAL_tag_skip": 1,
#ifdef FORK
						"=_action" : v("speakaction")
#else
						"_action" : v("speakaction")
#endif
				]) );
				else sendmsg(source, "_info_nickname",
					0, // "Hello [_nick].",
					([
						"_nick" : MYNICK,
						"_color" : v("color"),
						"_INTERNAL_tag_skip": 1,
#ifdef FORK
						"=_action" : v("speakaction")
#else
						"_action" : v("speakaction")
#endif
				]) );
				sendmsg(source, "_query_password", 0, ([
				     "_nick": MYNICK,
			    // see Handbook of applied cryptography p 397
				     "_nonce" : nonce,
// fippo thinks it is much more natural to show them here				    
#if 1
				     "_available_hashes": ""
#if __EFUN_DEFINED__(sha1)
                                         "sha1;"
#endif
#if __EFUN_DEFINED__(md5)
                                         "md5;http-digest;digest-md5"
#endif
#endif
				]) );
			    }
#ifdef ASYNC_AUTH
			    return 0;
			:)); // dont display, dont log, it is handled async
#endif
			return 0;
// _request_do_exit currently logs out clients anyway
// don't use this:
case "_request_exit":
			if (itsme && member(v("locations"), 0) && member(v("locations")[0], source)) {
				linkDel(0, source);
				quit();
				return 0;
			} else {
				// report?
				P0(("%O got invalid %O from %O\n",
				    ME, mc, source))
			}
			break;
case "_request_unlink_disconnect":
case "_request_unlink":
			if (vars["_service"] &&
			    member(v("locations"), vars["_service"])) {
				if (member(v("locations")[vars["_service"]], source)
				    || checkPassword(vars["_password"])) {
					linkDel(vars["_service"], source);
					return 0;
				} else {
					// report?
					P0(("%O got invalid %O from %O for %O\n",
					    ME, mc, source, vars["_service"]))
				}
			} else if (member(v("locations"), 0)
				   && member(v("locations")[0], source)) {
				linkDel(0, source);
				if (mc == "_request_unlink_disconnect" && !ONLINE) {
					// manually calling disconnected() .. hmmm
					disconnected();
				}
			} else {
		//		sendmsg(source, "_error_unavailable_function",
		//		    "Who are you anyway?");
	P0(("%O got invalid %O from %O. locations are %O. vars are %O.\n",
				    ME, mc, source, v("locations"), vars))
			}
			break;
case "_request_input":
			if (itsme) {
				// this should be renamed into _context but
				// that cannot be done before _context is
				// renamed into _channel .. hehe
				if (stringp(t = vars["_focus"])) {
					// check if the uniform is one of
					// our places
					if (places[t]) {
					    vSet("place", place = t);
					} else {
					    // see if it is a local object
					    object o = psyc_object(t);

					    // object one of our places?
					    if (o && places[o]) {
						place = o;
						vSet("place", o->qName());
					    } else {
						// must be a person then
						ME->input(data, t);
						// should be able to put o||t
						// here.. TODO
						return 0;
					    }
					}
				}
				ME->input(data);
			}
			else sendmsg(source, "_error_rejected_input_person",
				     0, ([]));
			// fall thru
case "_set_identification":
case "_assign_identification":
			return 0;	// skip
case "_message_echo_private":
#ifdef _flag_enable_measurement_network_latency
			if (stringp(source) && vars["_time_sent"]
			    && time() - vars["_time_sent"] > 3) {
				P1(("Network latency from %s to %s was %O.\n",
				    MYNICK, source, time()-vars["_time_sent"]))
			}
#endif
			// fall thru
case "_message_echo_public":
case "_message_echo":
case "_message_twitter":
case "_message_public":
			// avoid treating this as _message here
			break;
case "_message_video":
case "_message_audio":
			// not being displayed to users other than psyc clients
			data = 0;
			break;
// we should judge our messages by their routing method, not by their
// name! thus, the _public and _private distinction has to exist only
// for display. FIXME
case "_message":
			// this is only visible in person.c, not user.c
			// therefore probably useless
			mc = "_message_private";
			// fall thru
case "_message_private":
			// this should get caught before even instantiating
			// the user.. but that's not always easy
			if (IS_NEWBIE && !ONLINE) {
	P0(("sent a message to a user who is neither online nor newbie\n"))
				sendmsg(source, "_error_unknown_name_user", 
					0,
				    ([ "_nick_target" : MYNICK ]) );
				// reset will destruct this.. no hurry
				return 1;
			}
			break;
#if 0 //def PSYC_SYNCHRONIZE
//case "_notice_synchronize_set":
//case "_notice_synchronize":
			// this abbrev actually might not work considering
			// that incoming TCP doesn't come with the proper
			// port number.. so we should be using trust here. TODO
			if (!itsme && abbrev(PSYC_SYNCHRONIZE, source)) {
				// it's not really me, but it does what i mean
				itsme = 27;
			}
#endif
#if 0		// did the same as the inheritance loop, but badly
default:
			unless (sscanf(mc, "_request_do%s", t2))
			    break;
			// fall thru
#endif
case "_request_do":
			// extract the command from actual mc
			if (mc != family) t2 = mc[strlen(family)..];
			// fall thru
case "_request_execute":
			if (itsme || vars["_INTERNAL_trust"] > 7) {
				// this should be renamed into _context but
				// that cannot be done before _context is
				// renamed into _channel .. hehe
				//
				// also this would be the perfect place to
				// make a distinction between _focus and _group
				// if we wanted to.. actually, no, we want _context
				if (stringp(t = vars["_focus"]
					     || vars["_group"])) {
					// check if the uniform is one of
					// our places
					if (places[t]) {
					    vSet("place", place = t);
					    PT(("REQ-EX place %O\n", t))
					} else {
					    // see if it is a local object
					    object o = psyc_object(t);
					    if (o) {
						// object one of our places?
						if (places[o]) {
						    place = o;
						    vSet("place", o->qName());
						    PT(("REQ-EX o'place %O\n", o))
						} else {
						    PT(("REQ-EX object %O not found in %s's places %O\n", o, MYNICK, places))
						}
					    } else unless (t2) {
						    // must be a person then
//						ME->parsecmd(data, t);
						PT(("REQ-EX person %O vs %O\n", t, o))
						// should be able to put o||t
						// here.. TODO
						parsecmd(data, t);
						return 0;
					    }
					}
				}
				else {
					PT(("REQ-EX non-string %O\n", t))
				}
//				ME->parsecmd(data);
				if (t2) {
				    unless (request(source, t2, vars, data)) {
					sendmsg(source,
					  "_failure_unsupported_request",
					  0, ([]));
				    }
				} else parsecmd(data);
			}
			// about time we provide some ctcp-like sth here.. ;)
			else {
				sendmsg(source,
				    "_failure_unsupported_execute_person",
				      0, ([ "_nick" : MYNICK ]) );
				monitor_report("_warning"+ mc,
				    "Received unexpected "+ mc +" from "+
				    source);	
			}
			return 0;	// skip
case "_echo_place_leave":
case "_echo_place_enter_automatic_subscription":
case "_echo_place_enter_automatic":
case "_echo_place_enter":
case "_echo_place":
case "_echo":
case "_notice_presence_absent":
case "_notice_presence_away":
case "_notice_presence_here_busy":
case "_notice_presence_here":
case "_notice_presence":
case "_notice":
case "_request_description_vCard":
case "_request_description_time":
case "_request_description":
case "_request_list_feature":
case "_request_version":
case "_request_status_person":
case "_request_status":
case "_request":
case "_status_place_members":
case "_status_place":
case "_status_presence_away":
case "_status_presence_absent":
case "_status_presence_here_busy":
case "_status_presence_here":
case "_status_presence":
case "_status":
			// optimization: reduce slicing here
			break;
		PSYC_SLICE_AND_REPEAT
		}
	}
	else if (source == ME) itsme = 1;
	else {
	    // this is the else of stringp, so we are an objectp here
	    if (stringp(vars["_nick"])) 
		profile = ppl[ lower_case(vars["_nick"]) ];
	}

	D4( unless (profile) PP(("%O nopro2 for %O from %O\n",
				 ME, vars["_nick"], source)); )
	//	{
	//	    string service;
	//	    if (sscanf(mc, "_request_link_%s", service)) {
	//		
	//	    }
	//	}

	if (profile) switch (profile[PPL_DISPLAY]) {
	    case PPL_DISPLAY_NONE:	// ignore function
    //		vars["_display"] = "_none";
		// TODO: implement /ignore <source> <mcmatch> ?
		// or should we have a general setting for it
		// as in /set ignoremethods _message;_notice_update_web
		// so we don't have to figure out a place to define
		// this per user? makes sense - since it only applies
		// to those who have the /ignore flag set in their profile
    // default ignore behaviour:
		if (vars["_context"]) {
		    // filter public conversation even from a boss
		    if (abbrev("_message", mc) ||
			// filter web updates and such from places
		       	abbrev("_notice_update", mc))
			    return 0;
		} else {
		    // filter private talk unless source is a boss
		    if (boss(source) < 60 &&
			abbrev("_message", mc))
			    return 0;
		    // this does not send back echo, which is a way
		    // for the other side to know it is being ignored.
		    // we might have to change that!  -lynX
		}
		break;
	    case PPL_DISPLAY_SMALL:
    //		vars["_display"] = "_reduce";
		display = "_reduce";
		break;
	    case PPL_DISPLAY_BIG:
    //		vars["_display"] = "_magnify";
		display = "_magnify";
		break;
	}

	/*
	 * syntax for _request_store:
	 *
	 * :_application_pypsyc_window_size	3000x2000
	 * :_application_pypsyc_skin		darkstar
	 * :_character_command			%
	 * _request_store
	 * .
	 *
	 * so whenever the client starts up it can connect
	 * the UNI and ask for its settings by issuing an
	 * empty _request_retrieve. one day we may specify
	 * subsets of the data pool.. using families i guess.
	 */
	PSYC_TRY(mc) {
case "_request_store":
		if (itsme || vars["_INTERNAL_trust"] > 7) {
			P1(("%O received %O, please migrate to _request_do\n",
			    ME, mc))
			request(source, "_store", vars, data);
		}
		return 0;  // or fall thru?
case "_request_retrieve":
		if (itsme || vars["_INTERNAL_trust"] > 7) {
			P1(("%O received %O, please migrate to _request_do\n",
			    ME, mc))
			request(source, "_retrieve", vars, data);
		}
		return 0;
#if 0
case "_request_call_link":
		// CALC_IDLE_TIME(t); unless (t)
		if (itsme || (profile && profile[PPL_NOTIFY]
					 >= PPL_NOTIFY_FRIEND)) {
			// this could be extended to actually have a
			// confirmation from the user before replying (in a
			// classic answer-call telephony paradigm). but the
			// psyc way is to be more trusty of friends, which
			// doesn't mean this can however be refined here, like..
			// by checking how idle i am. then again.. since we
			// are clicking on that user's psyced web access - he
			// could use that to trigger opening of his own window,
			// making this here unnecessary. ok. #if 0 this.
			sendmsg(source, "_echo_call_link_automatic");
			// http://about.psyc.eu/Telephony#Web-based_Telephony
		}
//case "_request_call":
		break;
#endif
case "_request_version":
		P2(("Got a version request by %O\n", source))
		if (v("agent") && (itsme || (profile && profile[PPL_NOTIFY]
					     >= PPL_NOTIFY_FRIEND))) {
			sendmsg(source, "_status_version_agent",
				0, ([
			     "_version_agent"	: v("agent"),
			     "_version_description"	: SERVER_DESCRIPTION,
			     "_version"		: SERVER_VERSION,
			     "_nick"		: MYNICK
			 ]) );
			return 0; // don't display.. it's a friend
		} else sendmsg(source, "_status_version",
			       0, ([
			     "_version_description"	: SERVER_DESCRIPTION,
			     "_version"		: SERVER_VERSION,
			     "_nick"		: MYNICK
		 ]) );
		return 0; // no, we don't send version requests end-2-end
			 // and we don't display version requests at all
			// we should if the remote side request it. 
			// if I request a clients version, i want the remote
			// client to answer, not the server making 
			// assumptions.
case "_request_examine":	// don't use this, should be removed in 2009
case "_request_description":
case "_request_description_vCard":
		t = qDescription(source, vars, profile || -1, itsme);
		if (mappingp(t))
		    sendmsg(source, "_status_description_person", 0, t);
		// else be quiet and do not reply
		display = 0; // or should we display it for non-friends?
		break;
case "_request_description_time":
		unless(source && profile 
		       && profile[PPL_NOTIFY] != PPL_NOTIFY_NONE
		       && profile[PPL_NOTIFY] != PPL_NOTIFY_OFFERED) {
			return 0;
		}
		CALC_IDLE_TIME(t);
		// dv["_time_idle"] = t;
		sendmsg(source, "_status_description_time", 
			0, ([
			"_nick" : MYNICK,
			"_tag_reply" : vars["_tag"],
			"_time_idle" : t ]) );
		display = 0;
		break;
case "_request_list_feature":	
		// TODO: wir muessen entscheiden, ob das an die uni 
		// oder die unl geht
		if (IS_NEWBIE) rvars["_identity"] = "newbie";
#ifdef VISIBLE_SHERIFFS
		// visible administratorship may be considered transparent
		// thus friendly in some circumstances, but it has also
		// led to vanity administratorships. current psyc policy
		// is to keep this information private.
		else if (boss(ME)) rvars["_identity" ] = "administrator";
#endif
		else rvars["_identity"] = "person";
		rvars["_name"] = v("longname") || MYNICK;
		rvars["_list_feature"] = ({ "vCard", "list_feature" });	// _tab

		sendmsg(source, "_notice_list_feature_person", 0, rvars);
		display = 0;
		break;
case "_request_list_item":
		// jabber-only: should produce a /list of places
		// (the subscriptions) so that the user can flag things
		// for autojoin. that's okay for chatrooms, but inappropriate
		// for newscasts. as you can tell, this is currently
		// just a placebo anyway.  FIXME
		rvars["_list_item"] = ({ });	// _tab
		rvars["_list_item_description"] = ({ });    // _tab
		sendmsg(source, "_notice_list_item", 0, rvars);
		display = 0;
		break;
case "_request_ping":
		sendmsg(source, "_echo_ping", 0, rvars);
		display = 0;
		break;
case "_notice_invitation":
		// even if invitations are filtered, if the two are
		// on the phone or something like that
		// they can agree out of band to just "/follow" blindly
		vSet("invitationplace", objectp(vars["_place"]) ?
				vars["_nick_place"] : vars["_place"]);

		// same filtering code as couple lines further below
		if ((
#ifndef _flag_enable_unauthenticated_message_private
		      IS_NEWBIE ||
#endif
		     (!itsme && FILTERED(source)) &&
		    (!profile || profile[PPL_NOTIFY] <= PPL_NOTIFY_PENDING))) {
			sendmsg(source, "_failure_filter_strangers", 0,
				([ "_nick" : MYNICK ]) );
			unless (boss(source))
			    return 0; // dont display, dont log
		}
		break;
case "_message_private_question":
case "_message_private":
		// same filtering code as couple lines above
		if ((
#ifndef _flag_enable_unauthenticated_message_private
		      IS_NEWBIE ||
#endif
		      (!itsme && FILTERED(source)) &&
		    (!profile || profile[PPL_NOTIFY] <= PPL_NOTIFY_PENDING))) {
PT(("_failure_filter_strangers to %O from %O\n", source, ME))
			sendmsg(source, "_failure_filter_strangers", 0,
				([ "_nick" : MYNICK ]) );
			unless (boss(source))
			    return 0; // dont display, dont log
		}
#ifndef LOCAL_ECHO
		D3( D(data+" ... welcome to the remote echo feature.\n"); )
		// dont know if i want to keep it this way, but it's a start..
# if 0 // def TAGGING
			// echo tagging doesnt work yet
		if (vars["_tag"]) 
		    // experimental _echo message with _tag_reply as provided
		    // by uni:sendmsg() 
		    sendmsg(psource || source, "_echo"+ mc[8..], 0);
		else
# else
#  ifndef PRO_PATH
		// why do we have psource here
		// if sendmsg in entity.c is supposed
		// to rewrite UNI into UNL??
		    sendmsg(psource || source, "_message_echo" + mc[8..], data, vars);
#  else
		    sendmsg(psource || source, "_message_echo" + mc[8..], odata || data, vars);
#  endif
# endif
		    // da muss die gegenseite selber dran denken:
		    //+ ([ "_nick_target" : MYNICK ]) );
#endif
#ifdef PSYC_SYNCHRONIZE
		// huch, wieso privmsg weiterleiten? grübel
//		vars["_source_relay"] = psource || source;
//		sendmsg(PSYC_SYNCHRONIZE, "_message_relay" + mc[8 ..],
//		       	data, vars);
//		m_delete(vars, "_source_relay");
#endif
		// remember last message sender
		// i have the vague impression it doesn't work for clients!?
		t = objectp(source)
		   	? ((vars && vars["_nick"]) || "(?)")
			: (source || psource);
		if (t == v("reply")) break;
		vSet("reply", t);
		// generation of "away" message in irc-speak
#ifndef _flag_enable_unauthenticated_message_private
		if (IS_NEWBIE) {
			 sendmsg(source, "_warning_unable_reply", 0,
				 ([ "_nick": MYNICK ]));
		// it is a litte noisy to send this every time
		} else
#endif
		if (profile && profile[PPL_NOTIFY] >= PPL_NOTIFY_FRIEND) {
		    if (ONLINE) {
			// dies sendet die "redline" bei *jeder* privmsg und muss
			// daher von den meisten user.c wieder gefiltert werden
			// ausser der client hat sowas wie eine statuszeile.
			// interpsyc und interjabber haben daran garantiert keine
			// freude, daher objectp. für interpsyc könnte man sich
			// überlegen dies ins _echo mit einzupflegen.. wär aber
			// nicht richtig hübsch, nein. gewiss nicht.
			if (objectp(source) && v("me"))
			    sendmsg(source, "_status_person_present",
					"[_nick] [_action].", ([
				 "_nick": MYNICK,
				 "_action": v("me") ]) );
		    } else {
			if (v("me")) {
			     sendmsg(source, "_status_person_absent_recorded",
				     0, 
				([ "_nick": MYNICK,
				 "_action": v("me") ]) );
			}
			else {
			     sendmsg(source, "_status_person_absent",
				     0,
				([ "_nick": MYNICK ]) );
			}
		    }
//		} else {
		    // they already got echo
		}
		break;
#if 0
// causes recursions
case "_message_friends":
		// sending message down the friendtree
		if (vars["_depth"] != "0") {
			//(int)vars["_depth"]--;
			castmsg("_message_friends", data, vars);
		}
		break;
#endif
#ifndef _flag_disable_module_presence
// this one needs to be decided upon..
case "_request_notification_subscribe":	// jaPSYC's Notification.java sollte
				// nicht mehr verwendet werden, dafür gibt
				// es Friends.java in jaPSYC. hier wird
				// immerhin der begrüßungszustand hergestellt.
case "_request_status":
case "_request_status_person":
case "_request_presence":
		if (itsme) return showMyPresence(1);
		// hunting a mysterious bug.. ONLINE below crashes sometimes
		// (for antagonist and cha0zz), and this should be the only
		// logical reason why that can happen..
		// according to trace we get here from sendmsg
		// _notice_presence_here_quiet (which makes no sense either)
		// and shortly before this is output:
		//
// p-Show in S:xmpp:213.180.203.18:-38016, origin "xmpp:cha0zz@jabber.ru/Home", isstatus 0, vars ([ /* #1 */
//  "_INTERNAL_identification": "xmpp:cha0zz@jabber.ru",
//  "_nick": "cha0zz",
//  "_source_identification": "xmpp:cha0zz@jabber.ru"
//])
		if (!profile || profile[PPL_NOTIFY] == PPL_NOTIFY_NONE
			     || profile[PPL_NOTIFY] == PPL_NOTIFY_OFFERED) {
			// happens when friendships are async.. we already
			// deal with this in _notice_presence so let's shut
			// up here.. careful: jabber usually sends the query
			// (this here) first and the presence notice after!
			P2(("%O got %O from %O and ignored it.\n",
			    ME, mc, source))
			return 0;
			// ldmud 3.3.609 crashes with a completely absurd error
			// after the 'return' statement. upgrade?
		}
#if 1 // PARANOID ?
		unless(mappingp(v("locations"))) {
			// how the hell did i get here with a broken mapping?
			monitor_report("_warning_abuse_invalid_friend",
			    S("Broken locations[] in %O (with %O from %O:%O)\n",
				ME, mc, source, profile));
			vSet("locations", ([]));
		}
#endif
		if (ONLINE) {
		    CALC_IDLE_TIME(t);
		    if (v("me")) sendmsg(source, "_status_person_" +
			    (stringp(availability) ? "away" : "present"),
			"[_nick] [_action].", ([
			     "_nick": MYNICK,
			     "_time_idle" : t,
			     "_action": v("me") ]) );
		    else sendmsg(source,
			"_status_person_present", "Present: [_nick].",
			([ "_nick" : MYNICK, "_time_idle" : t ]) );
		// both the _action and _offline trails are stupid choices
		// to postpone the rename of the 'recorded' methods which
		// obviously collide here
		} else if (v("me"))
		    sendmsg(source, "_status_person_absent_action",
			    "[_nick] [_action].",
			    ([ "_nick": MYNICK, "_action": v("me") ]) );
		else sendmsg(source, "_status_person_absent_offline", 0,
			    ([ "_nick" : MYNICK ]) );
		if (v("presencefilter") == "none")
		    w("_notice_requested_status_person", 0, vars, source);
		return 0;       // skip
case "_notice_presence_here":
case "_notice_presence_here_busy":
		if (objectp(source)) {
			// remember last notification sender
			vSet("greet", vars["_nick"]);
		} else {
			// should take into consideration host-trustfulness so
			// we can let localhost perlscripts psycnotify us. TODO
			unless (profile && profile[PPL_NOTIFY]
						>= PPL_NOTIFY_MUTE) {
			    if (profile &&
			       	profile[PPL_NOTIFY] == PPL_NOTIFY_OFFERED) {
				    // this happens when the other side is
				    // sending us presence and our user hasn't
				    // looked into '/show in' yet.
				    // let's pretty much ignore this.
				P1(("%s got again notified by %O (%O)\n",
				    MYNICK, source, profile))
				return 0;
			    } else if (profile &&
			       	profile[PPL_NOTIFY] == PPL_NOTIFY_PENDING) {
// sometimes friendships end up being asynchronous.. then we get notifies
// even though we think our friendship is still pending from our side.
// this is certainly not worth yelling and screaming about. let's just
// log that this has occured and then correct our data.
				log_file("NOTPENDING", "%O got %O from %O\n",
					ME, mc, source);
				profile[PPL_NOTIFY] = PPL_NOTIFY_DEFAULT;
				// fall thru to greet and everything  :)
			    } else {
				// so far this has never happened for spam but
				// always for async friendships (moving ids etc)
				log_file("NOTOFFERED", "%O got %O from %O\n",
					ME, mc, source);
//				P1(("%s got notified unilaterally by %O (%O)\n",
//				    MYNICK, source, profile))
				monitor_report("_warning_abuse_invalid_friend",
				    S("%s got notified unilaterally by %O (%O)",
					MYNICK, source, profile));
				// okay, the first time this happens we take
				// a look at it. it just MIGHT be spam.
				// after that we treat this as a friendship offer
				// so that we don't get bombarded by the same
				// reports over and over again.
				//
				// let's not intrude into people's lives as this
				// would exactly be in the interest of a spammer
				// but rather make the friendship available the
				// next time the user goes into the '/show in' menu
				sPerson(source, PPL_NOTIFY, PPL_NOTIFY_OFFERED);
				return 0;
			    }
			}
			vSet("greet", source);
		}
		// currently just for xmpp:
		// accept the notice without implying a presence reply.
		if (!vars["_INTERNAL_quiet"] && ONLINE) {
			P2(("%O got %O from %O.\n", ME, mc, source))
			CALC_IDLE_TIME(t);
			// TODO: update to current presence scheme..
			// right now these messages appear when a person logs
			// in - kind of impractical
			if (v("me")) sendmsg(source, "_status_person_" +
				(stringp(availability) ? "away" : "present"),
				    0, ([
					"_nick": MYNICK,
					"_time_idle" : t,
					"_action": v("me") ]) );
			else sendmsg(source,
				"_status_person_present", 0,
				([ "_nick" : MYNICK, "_time_idle" : t ]));
		}
		//if (member(friends, source)) display = 0;
		unless (presence_management(source, mc, data, vars, profile))
		    display = 0;
		P3(("post-pmgmt1 in %O from %O display %O logged_on %O\n", ME,
		    source, display, logged_on))
		return logged_on && display;
#endif // _flag_disable_module_presence
#ifdef _flag_enable_alternate_location_forward
	PSYC_SLICE_AND_REPEAT
	}
	// filtering needs to be done before forwarding to location.
	// the PSYC_TRY methods above handle that. we used to have
	// this part of code that seperates the first switch from
	// the second and does forwarding. we currently do not forward
	// messages from here though, we do it from w().
	// the other solution would be to forward in user.c
	// then we dont have to split the switches
	//P2((">>>> locations: %O\n", v("locations")))
	foreach (t : v("locations")[0]) {
	    if (t && t != source) {
		// no psyctext rendering happening in this variant
# ifdef _flag_enable_circuit_proxy_multiplexing
		vars["_target_forward"] = t;
# endif
		sendmsg(t, mc, data, vars, source);
		lastmc = mc;
		// net/user:msg shouldn't work on this any further,
		// should it?
		display = 0;
	    }
	}
	// here we can filter things that do not belong into the lastlog
	// but shouldn't we simply log _message's only?
	PSYC_TRY(mc) {
#endif
case "_notice_friendship_removed_implied":
case "_notice_friendship_removed":
case "_notice_context_leave":	// future potential names of this function
case "_notice_context_leave_friends":
case "_request_context_leave":
case "_request_context_leave_friends":
		if (profile && profile[PPL_NOTIFY] != PPL_NOTIFY_NONE) {
		    sPerson(source, PPL_NOTIFY, PPL_NOTIFY_NONE);
		    t = 1;
		} else {
		    t = 0; // this person isn't *really* a friend of ours
		}
		// the redundancy of friends/members and the group/master 
		// data structures is driving me crazy
		// especially as we have to make parse_uniform on every remove
		m_delete(friends, source); 
		if (objectp(source)) // is_formal
		    remove_member(source);
		else
		    remove_member(source, parse_uniform(source, 1)[URoot]);

		// do not show people that weren't our friends
		// do not reply with _removed_implied
		// maybe we should even move this line before the m_delete?
		unless (t) return 0;

		showFriends();
		// symmetric friendship removal unless we weren't friends already
		sendmsg(source, "_notice_friendship_removed_implied", 0,
			([ "_nick": MYNICK, "_possessive": "the" ]) );
		break;
case "_notice_friendship_established":
		if (!profile || profile[PPL_NOTIFY] != PPL_NOTIFY_OFFERED) {
		    PT(("%O shouldn't have gotten a %O from %O with PPL_NOTIFY %O (unless it's an acute case of jabber)\n",
			ME, mc, source,
		       	profile? profile[PPL_NOTIFY]: "(no profile)"))
		    return 0;
		}
# ifndef _flag_disable_module_presence
		// can we dare to make this a display-only event and
		// rely on the new _request_friendship_implied for
		// symmetric friendships? then we should be able to
		// break; out of here.. TODO.. but for now let's just
		// dont we lack some sPerson here?
		if (ONLINE) {
		    CALC_IDLE_TIME(t);
		    if (v("me")) sendmsg(source, "_status_person_" +
			    (stringp(availability) ? "away" : "present"),
			0, ([
			     "_nick": MYNICK,
			     "_time_idle" : t,
			     "_action": v("me") ]) );
		    else sendmsg(source,
			"_status_person_present", 0,
			([ "_nick" : MYNICK, "_time_idle" : t ]) );
		}
# endif // _flag_disable_module_presence
		return display;
		// dont fall thru ...?
case "_status_friendship_established":
case "_request_context_enter":	// future potential names of this function
case "_request_context_enter_friends":
case "_request_friendship":
case "_request_friendship_implied":
		t = objectp(source) ? source->qName() : source;
		// unless (t = vars["_nick"]) return 0;
		PT(("%s in %O from %s(%O)\n", mc, ME, t, profile))

		t2 = "_status_friendship_established";
		data = "[_nick] is your friend already."; 

		if (profile) switch (profile[PPL_NOTIFY]) {
		case PPL_NOTIFY_NONE:
			// in this case, ask the user!
			break;
		case PPL_NOTIFY_OFFERED:
			// ignore repeated offers..
			return 0;
		case PPL_NOTIFY_PENDING:
			sPerson(t, PPL_NOTIFY, PPL_NOTIFY_DEFAULT);
			t2 = "_notice_friendship_established";
			data = "[_nick] is your friend now."; 
		//default: // might also work if it were placed here..
		// but in all default cases the person should already be
		// in the data structures below
#ifdef ALIASES
			// same code a few lines above
			if (t = raliases[source]) 
			    friends[source, FRIEND_NICK] = t;
			else if ((t = vars["_nick"]) && aliases[lower_case(t)])
			    friends[source, FRIEND_NICK] = 1;
			else
#endif
			// nasty redundancy with group/master datastructures
			friends[source, FRIEND_NICK] = vars["_nick"] || 1;
#ifdef AVAILABILITY_HERE
			friends[source, FRIEND_AVAILABILITY] =
			    vars["_degree_availability"] || AVAILABILITY_HERE;
#endif
			if (objectp(source))
			    insert_member(source);
			else
			    insert_member(source, parse_uniform(source, 1)[URoot]);

#ifdef _flag_enable_module_microblogging
			string uni = psyc_name(ME);
			sendmsg(source, "_notice_place_enter_automatic_subscription_follow",
				"Following [_nick_place]", (["_nick": MYNICK, "_nick_place": uni + "#follow" ]));
			sendmsg(source, "_notice_place_enter_automatic_subscription_follow",
				"Following [_nick_place]", (["_nick": MYNICK, "_nick_place": uni + "#friends" ]));
#endif
			showFriends();
			// fall thru
		// all other cases describe established friendships,
		// but the other side may be confused or have lost its
		// state, so we let it know once more
		default:
			// this stops loops of _status_friendship_established
			// but maybe we should just never act on that mc
			if (mc != t2) {
				sendmsg(source, t2, data,
				    ([ "_nick": MYNICK ]) );
				// friendship with oneself..
				// whatever it may be good 4
				unless (itsme) {
				    w(t2, data, vars, source);
				    // special case for jabber once more..
				    // wenn das für jabber speziell ist
				    // kann man doch das format aufpeppen..
				    // dann müsste auch die methode _INTERNAL
				    // heissen und ich will nicht a priori
				    // ausschliessen, dass dies nochmal anderswo
				    // nützlich sein kann und ausserdem ist
				    // es sauberer so.
				    sendmsg(source,
					"_status_person_present_implied", 0,
					([ "_nick": MYNICK,
					   "_time_idle" : "0" ]) );
				}
				// why did we want to filter this?
				// weil das zeug nervt.
				//display = 0;
			}
			myLogAppend(source, mc, data, vars);
			// display sollte hier gleich 0 sein wenn man
			// schon mit der person befreundet ist...
			return display;
		}
		sPerson(t, PPL_NOTIFY, PPL_NOTIFY_OFFERED);
		myLogAppend(source, mc, data, vars);
		if (IS_NEWBIE) 
		    sendmsg(source, "_failure_necessary_registration", 0,
			    ([ "_nick_target": MYNICK ]) );
		return display;
#ifndef _flag_disable_module_presence
case "_status_person_absent":
case "_status_person_absent_recorded":
case "_status_person_absent_action":
case "_status_person_absent_offline":
case "_status_presence_absent_vacation":
case "_status_presence_absent":
case "_notice_presence_absent_vacation":
case "_notice_presence_absent":
		if (!profile || profile[PPL_NOTIFY] == PPL_NOTIFY_NONE
			     || profile[PPL_NOTIFY] == PPL_NOTIFY_OFFERED) {
			P2(("%O got %O from %O and ignored it.\n",
			    ME, mc, source))
			return 0;
		}
		P2(("%O got %O from %O.\n", ME, mc, source))
#ifdef IRC_FRIENDCHANNEL
		vars["_degree_availability_old"] = friends[source, FRIEND_AVAILABILITY];
#endif
		m_delete(friends, source); // in case he's a friend..
		// if (v("greet") == source) vSet("greet", 0);
		if (v("presencefilter") == "all") return 0;
#ifdef LASTAWAY
# ifdef PRO_PATH
		// the webchat has a status line and likes to show info
		// every time.. we could generalize this into a /set
		// showawayonce (hello 1990)
		unless (v("scheme") == "ht") {	// copy from below
# endif
			if (lastaway && lastaway["_description_presence"]
					 == vars["_description_presence"]
			    && lastaway["_nick"] == vars["_nick"]) {
				// we've seen this message already
				return 0;
			}
			lastaway = vars;
# ifdef PRO_PATH
		}
# endif
#endif
		return ""; // dont apply display var; // display, but dont log
case "_notice_presence":
case "_status_presence":
		P1(("TODO: family %O in mc %O with availability %O.\n",
		    family, mc, vars["_degree_availability"]))
		break;
case "_notice_presence_away_manual":
		t2 = "_manual";
case "_notice_presence_away_automatic":
		unless (t2) t2 = "_automatic";
		mc = "_notice_presence_away";
case "_status_person_away":
case "_status_presence_away":
case "_notice_presence_away":
		if ((v("presencefilter") == "on" && t2 != "_manual") ||
		    ((!v("presencefilter") || v("presencefilter") == "none")
		     && t2 == "_automatic")) {
			display = 0;  // optTODO (move first check up)
		}
#ifdef IRC_FRIENDCHANNEL
		vars["_degree_availability_old"] = friends[source, FRIEND_AVAILABILITY];
#endif
		t2 = vars["_degree_availability"] || AVAILABILITY_AWAY;
#ifdef LASTAWAY
# ifdef PRO_PATH
		unless (v("scheme") == "ht") {	// copy from above
# endif
			// actually.. announce() doesn't retransmit the same
			// messages.. so where could a double away come from..
			// jabber?
			if (lastaway && lastaway["_description_presence"]
					 == vars["_description_presence"]
			    && lastaway["_nick"] == vars["_nick"]) {
				// we've seen this message already
				display = 0;
			}
			lastaway = vars;
# ifdef PRO_PATH
		}
# endif
#endif
		// fall thru
//case "_notice_presence_here_busy":
//case "_notice_presence_here":
case "_status_presence_here":
case "_status_presence_here_busy":
case "_status_person_present":
		unless (presence_management(source, mc, data, vars,
					    profile, t2))
		    display = 0;
		P3(("post-pmgmt2 in %O from %O display %O logged_on %O\n", ME,
		    source, display, logged_on))
		return logged_on && display; // what about availability?
#endif // _flag_disable_module_presence
#ifdef _flag_enable_module_microblogging
case "_notice_place_enter_automatic_subscription_follow":
case "_notice_place_leave_automatic_subscription_follow":
		string src = objectp(source) ? psyc_name(source) : source;
		string nick = objectp(source) ? source->qNameLower() : source;
		string nick_place = vars["_nick_place"];
		if (qFriend(source) || qSubscription(src, 1) || qSubscription("~"+nick, 1)) {
		    P3((">>> %O: %O subscribed me to %O\n", ME, source, nick_place))
		    subscribe(abbrev("_notice_place_enter", mc) ?
			      SUBSCRIBE_TEMPORARY : SUBSCRIBE_NOT, nick_place);
		} else if (member(places, find_place(nick_place)) &&
			   (sscanf(nick_place, src+"#%s", t) || sscanf(nick_place, "~"+nick+"#%s", t))) {
		    placeRequest(nick_place,
#ifdef SPEC
				 "_request_context_leave",
#else
				 "_request_leave",
#endif
				 1);
		} else {
		    P3((">>> %O: got a %O from %O, but he is not a friend or not already following him.\n",
			ME, mc, source, nick_place))
		}
		return "";
#endif
case "_notice_mail":
		// on request by y0shi.. remember mail notifications
		// even when offline
		myLogAppend(source, mc, data, vars);
		// fall thru
case "_notice_place_enter_automatic_subscription":
case "_notice_place_enter_automatic":
case "_notice_place_enter_login":
case "_notice_place_enter":
case "_notice_place_leave_logout":
case "_notice_place_leave":
case "_notice_place":
case "_notice_list_feature_server":
case "_notice_list_feature":
case "_notice_list_item":
case "_notice_list":
case "_notice":
		// someone changed this to return display; without explanation
		// so i change it back until an explanation is given.
		// no voodoo hacking in the main psyc backbone!!
		return ""; // dont apply display var;
case "_echo_place_leave":
case "_echo_place_enter_login":
case "_echo_place_enter_automatic_subscription":
case "_echo_place_enter_automatic":
case "_echo_place_enter":
case "_echo_place":
case "_echo":
case "_status_place_topic_official":
case "_status_place_topic":
case "_status_place_members":
case "_status_place":
case "_status":
		// optimization: reduce slicing here
		break;
	PSYC_SLICE_AND_REPEAT
	}
	if (!vars["_time_place"]) {
		if (abbrev("_message", mc)) {
		    // forward to v("id") from here
		    if (!ONLINE && v("id"))
			sendmsg(v("id"), "_notice_forward"+mc, 0,
			    ([   "_data_relay": data,
		 // gets overwritten if sender already provided one.. problem?
		 // do we ever get here for _message_public anyway?
			       "_source_relay": source,
			     ]) + vars);
		    // one way to circumvent the object loss problem of
		    // ldmud persistence, the other is in user:w()
//		    if (objectp(vars["_context"]))
//			vars["_context_uniform"] = psyc_name(vars["_context"]);
		    // and it is better because this one operates for each
		    // and every message whereas the other only does its job
		    // on /lastlog request
		    myLogAppend(source, mc, data, vars);
		}
#if 0 //def JOBS
		if (abbrev("_game", mc))	// is a job?
		    logAppend(source, mc, data, vars, _limit_amount_log, 0, 1);
#endif
	}
	D3( else D(S("not logged: %O\n", mc)); )
	return display;
}

#ifndef _flag_disable_module_authentication
static returnAuthentication(result, source, vars) {
	vars["_trust_result"] = result;
	switch(result) {
	case 1..9:
		sendmsg(source, "_notice_authentication", 0, vars);
		break;
	case 0:
		/* should this be an _echo or _warn? */
		sendmsg(source, "_notice_processing_authentication", 0, vars);
		P1(("asyncAUTH in %O for %O, %O\n", ME, source, vars))
		break;
	case -9..-1:
		sendmsg(source, "_error_invalid_authentication", 0, vars);
		break;
	}
	return result;
}
//
// result values are like trustworthy:
//	9	absolute trustworthy identity
//	6	this guy knows my token, good
//	4,5	this guy knows my UNL
//	3	my UNL after dns resolution
//	2	this guy knows my ip number
//	0	result delayed
//	<0	errors
//
checkAuthentication(source, vars) {
	string h, t;

	P1(("%O got checkAuthentication from %O\n", ME, source))
	if ((t = vars["_nonce"])) {
		if (t == v("nonce")) {
		    vDel("nonce");
		    return 6;
		}
		return -1;
	}
	// a bit chaotic here.. the protocol needs a reorganization..
	// does that even work in case of async auth??? 
	// password auth is bad anyway
	if (vars["_password"]) {
		return checkPassword(vars["_password"], vars["_method"], nonce, vars,
		        symbol_function("returnAuthentication"), source, vars);
	}
	if (h = vars["_host_IP"]) {
		if (h == v("ip") || h == query_ip_number(ME)) return 2;
	}
	if (vars["_location"] && v("locations")[0]) {
		mixed *u;

		// we should probably foreach this too  TODO
		if (member(v("locations")[0], vars["_location"])) return 5;
		// assumes v("locations")[X] is lower_case according to policy
		if (member(v("locations")[0], lower_case(vars["_location"])))
		     return 4;
		unless (u = parse_uniform(vars["_location"])) return -1;

		if (sscanf(u[UHost], "%~D.%~D.%~D.%~D") == 4) {
		    // why not check for equality of v("ip") and u[UHost]
		    dns_rresolve(u[UHost], (: 
			unless (stringp($1)) return;
			register_host($4, $1);
			return returnAuthentication(same_host($1, $5)
						    && 3, $2, $3);
		    :), source, vars, u[UHost], v("ip"));
		} else {
		    dns_resolve(u[UHost], (: 
			register_host($1, $4);
			return returnAuthentication(same_host($4, $5)
						    && 3, $2, $3);
		    :), source, vars, u[UHost], v("ip"));
		}
		return 0; // result delayed
	}
	return -1;
}
#endif

// with both HTTP and PSYC the user might be "online" even though the
// object isn't "interactive" (connected to a TCP stream).
#if 1
online(notnewbie) {
	if (ONLINE) return 1;
	if (notnewbie &&! IS_NEWBIE) return -1;
	return 0;
}
#else
online() { return ONLINE; }
#endif

sName(a) {
	P2(("%O(%O): sName(%O) from %O\n", ME, MYNICK,
		     a, previous_object()))
	unless (a = sName2(a)) return ME;

	ppl = v("people");
	unless (ppl) {
		ppl = ([ ]);
		vSet("people", ppl);
	}
	//logInit();
	if (v("log")) { logInit(v("log")); vDel("log"); }
	else logInit();
	// this line defeats the ability to find out when someone was
	// online last.. so-called /seen.. as it clears the timestamp
//	vSet("aliveTime", time());		// this doesnt make sense!?
	return ME;
}

// this is not the logon function that gets called automatically.
// either the psyc server calls it for psyc users, or its called
// by the inheriting user object.
// argument should be the hostname or ip of the host
// the user is calling from (typically query_ip_name())
//
logon(host) {
#if 0
//	PT(("pre rename: %O\n", ME))
	// should i prefix it with a / ? maybe maybe
	rename_object(ME, psycName());
//	PT(("postrename: %O\n", ME))
#endif
#ifdef CACHE_PRESENCE
	int s, amount = 0, ask4upd8s = 0;
	mixed person;
	string profile;

	foreach(person, profile : ppl) {
#ifdef BRAIN
		// user%host rauspatchen? ne doch lieber offline
#endif
		// fippo thinks we should also show the presence of all
		// our enemies. i don't agree with that policy.  ;)
		if (profile[PPL_NOTIFY] < PPL_NOTIFY_MUTE) continue;
		amount++;
		// not exactly efficient having to ask for each.
		s = persistent_presence(person);
		// also: we keep asking about local users so we have
		// zero chance of coming with an ask4upd8s == 0
		// should we make local users *always* reply to a _notice?
		if (s == AVAILABILITY_UNKNOWN) {
		    ask4upd8s++;
		} else if (s > AVAILABILITY_OFFLINE) { // was: just "else"
		    friends[person, FRIEND_NICK] = 1;
		    friends[person, FRIEND_AVAILABILITY] = s;
		}
	}
	P2(("%O has %O friends of which %O have unknown state.\n",
	    ME, amount, ask4upd8s))
#endif
	// greeting function disabled for psyc clients. they are
	// auto-intelligent and they need the _notice_login
        greeting = v("greeting") == "on" || v("scheme") == "psyc" ||
            (!v("greeting") && (v("scheme") != "jabber" || IS_NEWBIE));
#ifndef _flag_disable_info_session
	if (greeting && v("lastTime")) {
		string hi, ctim;
# ifdef _flag_log_hosts
		hi = v("host")==v("ip") ? v("ip") : v("host")+" ("+v("ip")+")";
# else
		hi = "*";
# endif
		ctim = ctime(v("lastTime"));
		w("_notice_logon_last", 0,
		    ([ "_date" : isotime(ctim, 0),
		       "_time" : hhmmss(ctim),
		       "_time_unix" : v("lastTime"),
		       "_host" : hi ])
		);
# ifdef _flag_disable_notice_news_software
		if (v("softnews") != SERVER_VERSION) {
			w("_notice_news_software", 0, 
			  ([ "_version": SERVER_VERSION ]) );
			vSet("softnews", SERVER_VERSION);
		}
# endif
		vDel("lastHost");
	}
#endif
	vSet("lastTime", time());
	vSet("lastTime2", time());
	vSet("aliveTime", time());	// a better place to update this

#ifdef _flag_log_hosts
	if (host) vSet("host", host);
	else
#endif
	    host = "?";

	P2(("%O person:logon %s, logged_on = %O\n", ME,
	    IS_NEWBIE? "(newbie)": "(registered)",
	    logged_on ))
	log_file("LOGON", "[%s] %s %s %s(%s) %s/%s/%s \"%s\"\n", ctime(),
		logged_on ? "O" : IS_NEWBIE ? "*" : "+", MYNICK,
#ifdef _flag_log_hosts
	       	query_ip_number() || v("ip") || "?",
#else
		"?",
#endif
	       	host,
	       	v("layout") || "-", v("language") || "-",
		v("scheme") || "-", v("agent") || "" );
	unless (logged_on++) {
#ifdef PSYC_SYNCHRONIZE
		// do not submit sign_on if a person has only done a quick
		// reconnect
	// thought 1: should have its one channel?
	// thought 2: should log file be generated out of channel? no.. boh
		synchro_report("_notice_synchronize_sign_on",
"[_nick] has logged in from [_host_name] using [_version_agent] over [_protocol_agent].", ([
		    "_nick": MYNICK, "_time_age": v("age"),
		    "_version_agent": v("agent") || "?",
		    "_protocol_agent": v("scheme") || "?",
		    "_host_name": host,
# ifdef _flag_log_hosts
		    "_host_IP": v("ip") || query_ip_number(), 
# endif
		]));
#endif
	}

#ifdef SMART_UNICAST_FRIENDS
	// join cslaves for our remote friends and build our 
	// data structure for group/master so we can use it for 
	// castmsg()
	// bugs with notify / announce are most likely due to lack of sync
	foreach (string ni, string mode : ppl) {
	    array(mixed) u;
	    mixed o;

	    // D(S("walkPeople(%O,%O,%O,%O)\n", ni, mode, o, level));
	    unless (mode && strlen(mode) > PPL_NOTIFY
//		    && mode[PPL_NOTIFY] != PPL_NOTIFY_NONE) continue;
		    && mode[PPL_NOTIFY] >= PPL_NOTIFY_FRIEND) continue;
	    if (u = parse_uniform(ni)) {
		// <lynX> first we change the ppl, then we need this code
//		if (is_localhost(u[UHost])) {
//		    o = summon_person(u[UNick]);
//		    insert_member(o);
//		} else {
		    o = ni;
		    insert_member(o, u[URoot]);		// outgoing presence
		    // only psyc supports multicast presence
		    if (u[UScheme] == "psyc")
			register_context(ME, o, u);	// incoming presence
//		}
	    } else {
		o = summon_person(ni) || ni;
		insert_member(o);
		// <el> change the mapping?
		// <lynX> no, if a change needs to be made to old .o data
		// we should do it only once at load() time and simplify
		// check by raising a version counter
	    }
	}
	P3(("smarticast distribution structure in %O: %O\n", ME, _routes))
#endif
#ifndef _flag_disable_module_presence
	switch(v("scheme")) {
	case "jabber":
	case "psyc":
# ifdef _flag_enable_manual_announce_telnet
	case "tn":
# endif
# ifdef _flag_enable_manual_announce_IRC
	case "irc":
# endif
		showMyPresence(1);
		break;
	default:
# ifdef CACHE_PRESENCE
		announce(AVAILABILITY_HERE, 0, ask4upd8s == 0);
# else
		announce(AVAILABILITY_HERE);
# endif
		showMyPresence(0);
	}
	// showMyPresence(logged_on > 1);
#endif // _flag_disable_module_presence
	if (v("new")) {
		// TODO: displaying these messages seems to trigger a bug
		// in net/jabber/user whereas the connection breaks, still
		// receiving them is better than not.. huh?
		w("_status_log_new", 0,
		  ([ "_amount_new" : v("new") ]) );
		logView(v("new"), 1);
		vDel("new");
	} else {
#ifndef _flag_disable_info_session
		if (greeting &&! IS_NEWBIE)
		    w("_status_log_none");
#endif
	}
	nonce = 0;
	vDel("nonce");
}

// called by obj/master at shutdown
reboot(reason, restart, pass) {
	// clonep(): Objects with replaced programs no longer count as clones.
	// so this doesn't work!!!! wicked wicked wicked bug!
	//unless (clonep(ME)) return;
	if (blueprint(ME) == ME) return;
	P2(("%O shutting down\n", ME))

	// temporary, please remove after 2009-04
	if (!v("locations")) vSet("locations", ([]));

#if !defined(SLAVE) && !defined(_flag_disable_info_session)
	if (ONLINE) {
	    // same in net/psyc/circuit.c
	    if (restart)
		w("_warning_server_shutdown_temporary", 0,
		  ([ "_reason": reason ]) );
	    else
		w("_warning_server_shutdown", 0,
		  ([ "_reason": reason ]) );
	}
#endif
	// availability = 0;
	return quit(pass); // was: 2 for immediate landing
}

// called from usercmd.i ( /bye )
// called by server.c for scheme switch
//
// what do we do with those call_outs? here you go:
// x, y local users.
// R remote room.
// x and y are members of R.
// now x quits. y gets an _notice_place_leave w/ source x, context R, thus
// meaning he gets that _notice_leave from R, not x.
// if we'd destruct x immediately, object recognition cannot find x's object,
// so display will get fuckeredup. but we don't. we destruct after 20 seconds,
// which gives those _notice_leaves plenty of time for travelling through the
// whole internet, object recognition will find x's object, y will see nicely
// formatted output .. and not even care!
quit(immediate, variant) {
		    // keep an eye on the prototype right above checkPassword()
	int rc;

	P3(("person:QUIT(%O,%O) in %O\n", immediate,variant, ME))
#if 1
	// keeping services running while logging out should be possible..
	// will this unlink all main clients? should it?
	linkDel(0); // 0, previous_object()); <- this can be ME and will fail
#else
	if (sizeof(v("locations"))) { // this should only trigger at first pass
		linkCleanUp();
# if 1 //def PARANOID
		if (sizeof(v("locations"))) {
			P1(("%O * Hey, linkCleanUp left us with %O\n",
			    ME, v("locations")))
			// we cannot vDel("locations") because the ONLINE macro
			// breaks when we do
			vSet("locations", ([]));
		}
# endif
	}
#endif
	if (immediate == 1 || (immediate && find_call_out(#'quit) != -1)) { //'
		rc = save();
		if (sizeof(places)) {
			P2(("%O stayin' alive because of places %O"
			    " and corresponding permanent subscriptions %O\n",
			    ME, places, v("subscriptions")));
		} else {
			destruct(ME);
		}
		return rc;
	}
	if (leaving++) {
		P1(("intercepted recursive QUIT %O\n", ME || MYNICK ))
		return 0;
	}
	P4(("** QUITTING %O: av %O, sc %O\n", ME, availability, v("scheme")))
#ifndef	_flag_disable_module_presence
	switch(v("scheme")) {
	case 0:
		// scheme is 0 when a user entity has never been logged
		// in, like friends in a roster that get incarnated
		break;
	// jabber/user:quit() is called on @type 'unavailable'
	// so it relies on quit() to announce offline status
	//case "jabber":
	//	break;
# ifdef _flag_enable_manual_announce_telnet
	case "tn":
		break;
# endif
# ifdef _flag_enable_manual_announce_IRC
	case "irc":
		break;
# endif
	// psyc clients are supposed to explicitely set a user's presence
	// status before exiting, if the user wishes so. only if they are
	// not performing a clean exit, we will presume that an OFFLINE
	// availability is appropriate. maybe a separate AVAILABILITY_LOST
	// value would be nice. and we should have a delayed unavailability
	// automation feature here to avoid frequent relogin announcements.
	case "psyc":
		if (variant != "_disconnect") {
			P3(("++SKIP psyc client %O %O. announce yourself!\n",
			    variant, ME))
			break;
		}
		// fall thru in case of _disconnect
		// which indicates, we died an irregular death
	default:
# ifdef ALPHA
		if (availability > AVAILABILITY_OFFLINE) {
#  if 0 //def CACHE_PRESENCE
			announce(AVAILABILITY_OFFLINE, 0, ask4upd8s == 0);
#  else
			announce(AVAILABILITY_OFFLINE);
#  endif
		}
# else
		if (availability) {
			announce(AVAILABILITY_OFFLINE);
			availability = 0;
		}
# endif
	}
#endif // _flag_disable_module_presence
	// TODO: here we need to leave all our friends cslaves
	if (v("lastTime2")) {
		if (query_once_interactive(ME)) {
			int delta = time() - v("lastTime2");
			// we need to keep track of user's age for security
			// reasons
			vSet("age", delta + v("age"));
#ifndef _flag_disable_info_session
			w("_notice_session_end", 0,
			    ([ "_time_duration" : timedelta(delta) ]) );
#endif
		}
		vDel("lastTime2");	// don't keep this tmp copy
	}
#if DEBUG > 0
	rc = save();
#else
	if (immediate) rc = save();
#endif
	if (ME) { // in what situation are we !ME?
		  // well, sure, if the object is marked for destruction
		  // (by destruct(), of course). but when does that happen?
		// no longer log non-interactive quits
		// query_once_interactive(ME) doesn't work for psyc clients
		if (logged_on) {
			log_file("LOGON", "[%s] %s %s (%O) %O\n", ctime(),
			     IS_NEWBIE ?
				 (interactive(ME) ?  "/" : "#") :
				 (interactive(ME) ?  "-" : ">"), MYNICK, ME,
			     logged_on);
#ifdef PSYC_SYNCHRONIZE
			synchro_report("_notice_synchronize_sign_off"
					   + (variant || ""),
			    "[_nick] is logging out using [_version_agent].",
				     ([ "_nick": MYNICK,
					"_version_agent": v("agent") || "?" ]));
#endif
			logged_on = 0;
		}

		if (immediate) {
		    destruct(ME);
		} else {
		    remove_interactive(ME);
		    leaving = 0;
                    // return if there are some services/clients left
                    if (sizeof(v("locations"))) return rc;
                    vDel("scheme");
		    call_out(#'quit, 20, 1, variant); //'
		    return rc;
		}
	} else {
		// sometimes this stuff gets called when ME is already destroyed
		log_file("LOGON", "[%s] $ %s\n", ctime(), MYNICK);
		// no, apparently it never happens..
		P1(("Logging out a destroyed %O.\n", MYNICK))
	}
	P3(("QUIT %O\n", ME || MYNICK ))
	return rc;
}

save() {
	int howmany;

	if ( IS_NEWBIE || !MYNICK ) return 0;
	howmany = logClip(2 * _limit_amount_log_persistent,
			      _limit_amount_log_persistent);
	::save(PERSON_DATA_FILE(MYLOWERNICK));
	return howmany;
}

#ifndef _flag_disable_module_presence
showMyPresence(verbose) {
	if (v("presencetext"))
	    w("_status_presence_description",
	      "Your presence: [_description_presence]", ([
		"_nick": MYNICK, //"_time_idle" : t,
		"_degree_availability": availability,
		"_degree_mood": mood,
		"_description_presence": v("presencetext") ]));
	else if (verbose || mood || availability != AVAILABILITY_HERE)
			    // doubtlessly needs a nicer message.. or none
	    w("_status_presence", 0, // "I know this sounds funny, but your availability degree is [_degree_availability] and your mood is [_degree_mood]."
	     ([ "_nick": MYNICK, //"_time_idle" : t,
		"_degree_availability": availability,
		"_degree_mood": mood ]));
}

// more compliant to http://www.psyc.eu/presence these days
announce(level, manual, verbose, text) {
	mapping vars;
	int changed = 1;

	if (text && strlen(text)) vSet("presencetext", text);
	else {
		if (v("presencetext")) vDel("presencetext");
		else changed = 0;
		text = v("me");
		text = text ? (MYNICK +" "+ text +".") : "";
		    // fun: "Reclaim your chat. Use PSYC. PSYC delivers.";
	}
	if (!changed && availability == level) {
		// this check ensures that we do not send "fake" announces
		// for user entities which are being deallocated but
		// were never actually logged in (absent friends of users)
		// ... maybe this stops now that i added 'case 0:'
		//
		// we also get here when user objects are force quitted
		// by keepUserObject() even if they were created just now
		// ... see irc/server.. it's a FIXME
		//
		// unfortunately it seems to also affect other scenarios
		P3(("++SKIP %O announce %O(%O,%O) %O changed: %d, av: %O\n",
		    ME, level, manual, verbose, text, changed, availability))
		return 0;
	}
	if (level) vSet("availability", availability = level);
	else level = availability;	// sending EXPIRED not permitted here
	P2(("%O announce %O(%O,%O) %O changed: %d, mood: %O\n", ME,
	    level, manual, verbose, text, changed, mood))

# if 0 //def CACHE_PRESENCE
// this define could also by dynamic to reflect whether we need a
// reply from the other side or not
// for now, we expect replies but cache nonetheless.
// this is a temporary test behaviour
#  define _NOTICE_FRIEND_PRESENT \
	(verbose? "_notice_presence_here": "_notice_presence_here_quiet")
# else
#  define _NOTICE_FRIEND_PRESENT "_notice_presence_here"
# endif
# ifdef SMART_UNICAST_FRIENDS
	vars = ([	   "_nick": MYNICK,
			 "_source": v("_source"),
	   "_description_presence": text,
	    "_degree_availability": level ]);
	if (mood) vars["_degree_mood"] = mood;
#  ifdef JABBER_PATH
	vars["_INTERNAL_mood_jabber"] = mood2jabber[mood];
#  endif
#  ifdef PSYC_SYNCHRONIZE
	// 0 makes this message invisible when an admin is in the @sync
	synchro_report("_notice_presence_synchronize", 0, vars);
#  endif
	// manual = manual? "_manual": "_automatic"; ?
	// or just manual = manual? "": "_automatic"; ?
	switch(level) {
	case AVAILABILITY_AWAY:
		castmsg(manual? "_notice_presence_away_manual":
			     "_notice_presence_away_automatic", 0, vars);
			// "[_nick] is away. [_description_presence]", vars);
		break;
	default:
		// ignoring manual here.. TODO
		castmsg("_notice_presence"+ avail2mc[level], 0, vars);
		break;
	}
# else //{{{ SMART_UNICAST_FRIENDS
#  echo Warning: person.c running without SMART_UNICAST_FRIENDS
#  echo About time to delete this part of the code..
	foreach (string ni, string mode : ppl) {
	    object o;

	    // D(S("walkPeople(%O,%O,%O,%O)\n", ni, mode, o, level));
	    unless (mode && strlen(mode) > PPL_NOTIFY
		    && mode[PPL_NOTIFY] != PPL_NOTIFY_NONE) continue;

	    if( o = find_person(ni) ) {
		    unless( o->online() ) {
			    m_delete(friends, o);
			    continue;
		    }
	    }
	    else if (is_formal(ni)) {
		    o = ni;	// remote buddy
	    } else {
		    continue;			// offline local buddy
	    }

	    P3(("%O announce(%O): %O for %O (%O)\n", ME, level, o, ni,
		mode[PPL_NOTIFY]))
	    switch(level) {
	    case AVAILABILITY_OFFLINE:
		    if (friends[o, FRIEND_NICK])
			     sendmsg(o, "_notice_presence_absent", 0,
			     ([ "_nick": MYNICK ]) );
		    continue;
	    case AVAILABILITY_HERE:
		    switch (mode[PPL_NOTIFY]) {
		    case PPL_NOTIFY_IMMEDIATE:
			    sendmsg(o, _NOTICE_FRIEND_PRESENT, 0,
				     ([ "_nick": MYNICK ]) );
			    break;
		    case PPL_NOTIFY_DELAYED:
			    call_out(symbol_function("sendmsg"), 
				     TIME_DELAY_NOTIFY,
				    o, _NOTICE_FRIEND_PRESENT, 0,
				     ([ "_nick": MYNICK ]) );
			    break;
		    case PPL_NOTIFY_DELAYED_MORE:
			    call_out(symbol_function("sendmsg"), 
				     2 * TIME_DELAY_NOTIFY,
				    o, _NOTICE_FRIEND_PRESENT, 0,
				     ([ "_nick": MYNICK ]) );
			    // fall thru
		    case PPL_NOTIFY_MUTE:
			    break;
		    default:
			    // not the most efficient way to
			    // skip pends+offers TODO
			    continue;
		    }
		    if (objectp(o)) friends[o, FRIEND_NICK] = ni || 1;	// to be removed one day..?
		    // TODO: the most correct way would be to expect a _status_person_present
	    }
	}
# endif //}}} SMART_UNICAST_FRIENDS
#ifndef _flag_disable_module_presence
	if (verbose) showMyPresence(1);
#endif
	return ++logged_on; // might aswell use it as an announcement counter ;)
}
#endif // _flag_disable_module_presence

static qFriends() { 
	int i;
	string present;
	array(mixed) k;
	mixed o, n;

	P2(("%O's qFriends inbound » %O «\n", ME, friends))
	k = m_indices(friends);
	unless(k) return; 
	present = "";
	for(i=sizeof(k); i;) {
	    o = k[--i];
	    if (objectp(o)) {			// a local user
		// present += o -> psycName();
		n = friends[o, FRIEND_NICK];
		unless(stringp(n)) friends[o, FRIEND_NICK] = n = o->qName();
		present += " ~"+ lower_case(n);
	    } else if (stringp(o)) {
		present += " "+ o;		// a psyc: uni
	    }
	}
	if (strlen(present) < 3) return D3("~lynx");
	P2(("qFriends outbound » %s «\n", present[1..]))
	return present[1..];
}

#ifdef _flag_enable_module_microblogging
qFriend(person) {
	P3((">> qFriend(%O)\n", person))
	if (IS_NEWBIE) return 0;
	if (objectp(person)) person = person->qNameLower();
	return member(ppl, person) && ppl[person][PPL_NOTIFY] >= PPL_NOTIFY_FRIEND;
}

qFollower(person) {
	P3((">> qFollower(%O)\n", person))
	if (IS_NEWBIE) return 0;
	foreach (string c : v("channels")) {
		object p = find_place(c);
		P3((">>> c: %O, p: %O\n", c, p))
		if (p && p->qMember(person)) return 1;
	}
	return 0;
}

qSubscription(target, without_channel) {
	P3((">> qSubscription(%O, %O)\n", target, without_channel))
	if (IS_NEWBIE) return 0;
	unless (without_channel) return member(v("subscriptions"), target);
	string c;
	foreach (string t : v("subscriptions")) {
	    if (sscanf(t, target + "#%s", c)) return 1;
	}
	return 0;
}

sChannel(string channel) {
	P3((">> sChannel(%O)\n", channel))
	v("channels")[channel] = 1;
}
#endif

sPerson(person, ix, value) {
	P2(("%O: sPerson(%O, %O, %O) bfor: %O\n", MYNICK, person, ix, value, ppl))
	// TODO: we need some register_context / deregister_context here
	if (objectp(person)) person = person->qNameLower();
	else person = lower_case(person); // both nicks and UNIs. very good.

	unless( ppl[person] ) {
		ppl[person] = "    ";
	}

	ppl[person][ix] = value;
	if (ppl[person] == "    ") {
		m_delete(ppl, person);
//		save();
//		return 2;
	}
	save();
#ifdef PSYC_SYNCHRONIZE
	unless (synchronize_contact(MYNICK, person, ix, value)) {
		P1(("broken synchronize_contact for %O %O %O %O\n",
		    MYNICK, person, ix, value))
	}
#endif
	P2(("%O: sPerson(%O, %O, %O) afta: %O\n", MYNICK, person, ix, value, ppl))
	return 1;
}

htinfo(prot, query, headers, qs) {
	string me;

	P3(("htinfo(%O, %O, %O, %O)\n", prot, query, headers, qs))

#ifdef EXPERIMENTAL
// this extension is disputed and not in use
// we should probably do net/http/register instead..
	if (member(query, "register")) {

	    unless (IS_NEWBIE) {
		me = "This user is already registered by someone else.";
	    } else {
		string password = query["register"];

		// checkVar is stupid function. where does the w() go??? he? something not helpy helpy!
		if (checkVar("password", password)) {
		    me = "Hoooray, you registered yourself to something else which is somehow yourself. gaga.";

		    vSet("password", password);

		} else {
		    me = "this is not okay password!";
		}
	    }
	} else
#endif
	htok(prot);
        if (member(query, "motto")) {
		// intended for little (i)frames: just one line
		// showing nickname and motto message.
                me = v("publicpage") ? "<a href=\""+ v("publicpage")
			+"\" target=x>"+ MYNICK +"</a>" : MYNICK;
                if (v("me")) me += " "+ htquote(v("me"));

                write( ( // T("_HTML_status_head",
"<title>/~"+ MYNICK +"</title>\n\
<body bgcolor=#000000 text=#cccccc link=#ffffff vlink=#999999>\n\
<center><font face=helvetica>") +"["+ me + "]\n");
        } else {
		P3(("anonymous profile inspection in %O with %O and %O\n",
		    ME, query, headers))
		w("_notice_examine_web_person", 0, ([
	          "_web_on": headers["referer"] || headers["host"],
                  "_web_from": headers["user-agent"]
			 || query_ip_name(this_interactive()) || "",
                  "_nick" : MYNICK,
		]) );
                write( htDescription(1, query, headers, qs, "_anonymous",
                        qDescription(0, ([ "_trust": 0 ]), -1, 0)) );
        }
	return 1;
}

qNameLong() { return v("coolname") || v("longname") || MYNICK; }

void reset(int again) {
	P2((" [~ %O] ", ME))
	// clonep(): Objects with replaced programs no longer count as clones.
	// so this doesn't work!!!! wicked wicked wicked bug!
	//unless (clonep(ME)) return;
	if (blueprint(ME) == ME) return;
	// name::reset(again); storage::reset(again);
	if (again) {
		if (ONLINE) {
			P4(("RESET: saving %O\n", ME))
			save();
#if 0
		} else {
			P3(("RESET: quitting %O\n", ME))
			if (find_call_out(#'quit) == -1) quit(); //'
#endif
		}
	} else { // hmm.. why do we still use reset(0) for this?
		mixed o;
		P2(("CREATE: %O\n", ME))
		vInit();
#ifdef ALIASES
		vSet("aliases", ([]));
		aliases = ([]);
		raliases = ([]);
#endif
		vSet("locations", ([]));
		friends = m_allocate(0, 2);
		leaving = 0;
#ifndef _flag_disable_module_presence
		avail2mc = shared_memory("avail2mc");
# ifdef JABBER_PATH
		mood2jabber = shared_memory("mood2jabber");
# endif
#endif // _flag_disable_module_presence
		// _tags = ([ ]);	-- shouldnt be here
	}
}

void create() {
	P2((" {~ %O} ", ME))
	::create();
	return reset(0);
}

#if 1 //ndef XMPPERIMENTAL
// does this really apply to person only, or should
// it go into group/master? apparently places don't need this..
// but why does person need it?
//
insert_member(source, route) {
    P4(("%O insert_member(%O, %O)\n", ME, source, route))
    if (stringp(source) 
	    && !abbrev("psyc:", source)
//	    && !abbrev("xmpp:", source)
	    ) {
	P2(("%O person:insert_member(%O, %O) removing route\n",
	    ME, source, route))
	route = 0;
    }
    ::insert_member(source, route);
}
remove_member(source, route) {
    if (stringp(source) 
	    && !abbrev("psyc:", source)
//	    && !abbrev("xmpp:", source)
	    ) route = 0;
    ::remove_member(source, route);
}
#endif

castmsg(mc, data, vars) {
    vars["_nick"] = MYNICK;
    return ::castmsg(ME, mc, data, vars);
}

