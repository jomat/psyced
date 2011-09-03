// vim:foldmethod=marker:syntax=lpc:noexpandtab
// $Id: library.i,v 1.344 2008/09/12 15:54:38 lynx Exp $

#include <net.h>
#include <services.h>
#include <person.h>
#include <uniform.h>

#ifdef _uniform_node
# define myUNL  _uniform_node
#else
volatile string myUNL;
#endif
volatile string myUNLIP;
volatile string myLowerCaseHost;

volatile mapping targets = ([]);
volatile mapping schemes = ([]);
volatile mapping services = ([]);

#ifdef CACHE_PRESENCE
// volatile until we have a logic which at boot-time figures out if we
// have been down only a few seconds (thus keep the presence data) or
// if it was longer than that (thus delete the cache)
volatile mapping presence_cache = ([ ]);
#endif

/* used for things that have a context but are no places
 * could possible be used to keep room members as well
 * so they are persistent during reloads
 * if the index is a local user this will contain the 
 * group of his online friends
 */
volatile mapping contexts = ([]);

// in lack of a better name..
mapping confmap = ([]);

// system net/queue of outgoing messages per circuit
// shared between circuits but kept here for storage between reboots
mapping systemQ;

// protos
//string legal_name(string n, int isPlace);
//string legal_mailto(string a);
int psyc_sendmsg(mixed target, string method, mixed data, mapping vars,
	        int showingLog, mixed source, array(mixed) uniform);

// register a delivery object for a UNL
// first argument *must* be lower_case
varargs int register_target(string uniform, vaobject handler, vaint shy) {
	PROTECT("REGISTER_TARGET")
#if DEBUG > 0
        unless (uniform)
            raise_error("register_target without uniform\n");
#endif
#if 0
        if (SERVER_UNIFORM == uniform)
            raise_error("register_target for root!?\n");
#endif
	if (targets[uniform]) {
		D2( unless (handler) handler = previous_object();
		    if (targets[uniform] != handler)
		    // TODO:: way higher debug level here!
		    PP(("register_target: %O(%O) wants to register %O, "
		        "but it already belongs to %O(%O), "
			"replacing handler.\n",
			handler, query_ip_name(handler), uniform,
			targets[uniform], query_ip_name(targets[uniform]))); )
		if (shy) return 2;
	}
	unless (handler) handler = previous_object();
	P2(("register_target(%O) by %O\n", uniform, handler))
	targets[uniform] = handler;
#if 0 // this shouldn't be necessary TODO
	uniform = lower_case(uniform);
	if (targets[uniform]) return 1;
	targets[uniform] = handler;
#endif
	return 1;
}

object find_target_handler(string target) { return targets[target]; }

// LOCALHOST STUFF

string query_server_uniform() { return myUNL; }     // proto.h!
string query_server_uniform_ip() { return myUNLIP; }
string my_lower_case_host() { return myLowerCaseHost; }

static varargs void ready_freddie(vamixed ip) {
        object root;
        mixed t, h;

	// why PROTECT("SET_MY_IP") a static function?
#ifndef __HOST_IP_NUMBER__
    // once is not enough.. what if the dsl line crashes and now has a new one?
    // we could be so smart not to need restarts! we don't use myIP for all that
    // much.. so maybe it's fine just ignoring it.. as long as we know all of
    // our hostnames..
	unless (stringp(ip)) raise_error("Cannot resolve my own hostname!\n");
	myIP = ip;
	register_localhost(myLowerCaseHost, ip);
	myUNLIP = "psyc://"+ ip;
	if (query_udp_port() != PSYC_SERVICE)
	    myUNLIP += ":" + query_udp_port();
	myUNLIP += "/";
	D("Dynamic Server UNLs: "+ myUNL +" and "+ myUNLIP +"\n");
#endif
#ifndef __PIKE__
        // we need to load the root object async from create()
        // so it can use the library itself. that's why we first
        // register all targets to the master, then remap them.
        root = load_object(NET_PATH "root");
        psyc_name(root, "");
        mapeach(t, h, targets) {
            if (h == master) targets[t] = root;
        }
	D("»»» Root entity ready.\n");
        P4(("targets remapped to root entity: %O\n", targets))
#endif
}

// no way! you can't have a msg() in library because lastlog's call on msg()
// ends up here instead of resolving into a user->msg(). ok, you can fix
// that, but essentially there is a risk of breaking some function calls
// somewhere if you define popular lfuns also in the library.
//
//int msg(mixed source, string mc, mixed data, mapping vars, mixed target) {
//      raise_error("got msg() in library!\n");
//}

static void create() {
	PROTECT("CREATE")
	string t;

#ifndef __PIKE__
        master = previous_object();
	restore_object(DATA_PATH "library");
	systemQ = qCreate();    // should all entity queues be persistent
                                // instead of having q AND systemQ here?
#endif // __PIKE__
#ifdef TESTSUITE
	ME->base64_self_test();
	if (md5("foobar") != "3858f62230ac3c915f300c664312c63f")
	    raise_error("MD5 is br0ken!!11!!!\n");
# ifndef _flag_disable_authentication_digest_MD5
	sasl_test();
# endif
	//P4(("%O\n", make_json( ([ 7:"33\n44\t21", "!":93 ]) )))
	printf("Testing make_json: Is %O equal to %O ?\n",
	   make_json( ([ "7":"33\n44\t21", "!":93, "struct":([ "yeh":({1,2}) ]) ]) ),
		"{\"7\":\"33\\n44\\t21\",\"!\":93,\"struct\":{\"yeh\":[1,2]}}");
	printf("Testing parse_json: Is %O equal to %O ?\n",
	   parse_json( "{\"7\":\"33\\n44\\t21\",\"!\":93,\"struct\":{\"yeh\":[1,2]}}"),
	    ([ "7":"33\n44\t21", "!":93, "struct":([ "yeh":({1,2}) ]) ]) );
#endif
#ifdef PRO_PATH
	ME->pro_create();
#endif
#ifdef MUDOS
	D("Trying to run under MUDOS. Probably won't get very far..\n");
#endif
#ifdef __LDMUD__
# ifndef __psyclpc__
#  echo Warning: You aren't using a psyclpc driver. This may cause trouble.
# else
#  ifdef __NO_SRV__
#   echo Warning: DNS SRV is unavailable.
#   ifdef JABBER_PATH
#    echo ... XMPP will not work correctly!!
#   endif
#  endif
# endif
//# echo Running under LDMUD. Good choice for a driver.
//# if !__EFUN_DEFINED__(errno)
//#  echo Warning: This driver is missing the errno function.
//# endif
# if !__EFUN_DEFINED__(net_connect)
#  echo Warning: This driver is missing the net_connect function.
# endif
# if !__EFUN_DEFINED__(convert_charset)
#  echo Warning: This driver is missing iconv (convert_charset) support.
#  ifdef JABBER_PATH
#   echo ... XMPP will not work correctly!!
#  endif
# endif
#endif
#ifdef AMYLAAR
# if __EFUN_DEFINED__(lambda)
	D("Running under Amylaar LPMUD. You should upgrade to psyclpc.\n");
# else
	D("Unrecognized LPC driver. Using Amylaar settings.\n");
# endif
#endif
#ifdef VOLATILE
	D("VOLATILE flag set: Server will not save any data.\n");
#endif
	myLowerCaseHost = NAMEPREP(SERVER_HOST);
	register_localhost(myLowerCaseHost);
#ifdef __PIKE__
        //debug_write("Creating psyced library in Pike.\n");
#else
        register_target("");
        register_target("/");
	register_target(myLowerCaseHost);
#endif
#ifdef LOCAL_HOSTS      // compatible to the other stuff in hosts.h
# echo Setting up local host aliases.
        foreach(string x : ({ LOCAL_HOSTS }))
            if (index(x, '.') > 0) register_localhost(lower_case(x));
            else debug_message("Invalid local hostname '"+x
                               +"' in #define LOCAL_HOSTS\n");
#else
# ifdef VIRTUAL_HOSTS    // is this being used anywhere?
        foreach(string x : explode(VIRTUAL_HOSTS, " "))
            if (index(x, '.') > 0) register_localhost(lower_case(x));
            else debug_message("Invalid virtual hostname '"+x
                               +"' in #define VIRTUAL_HOSTS\n");
# endif
#endif
#if SYSTEM_CHARSET != "UTF-8"
	D("»»» System charset: " SYSTEM_CHARSET "\n");
#endif
#ifdef myUNL
	unless (trail("/", myUNL)) {
		debug_message("*** Invalid server uniform: '"+ myUNL
			      +"' has to end in a '/' ***\n");
		shutdown();
	}
#else
	myUNL = "psyc://"+ myLowerCaseHost;
	if (query_udp_port() != PSYC_SERVICE)
	    myUNL += ":" + query_udp_port();
        register_target(myUNL);
	myUNL += "/";
        register_target(myUNL);
#endif
#ifdef __HOST_IP_NUMBER__
	myUNLIP = "psyc://"+ __HOST_IP_NUMBER__;
	if (query_udp_port() != PSYC_SERVICE)
	    myUNLIP += ":" + query_udp_port();
        register_target(myUNLIP);
	myUNLIP += "/";
        register_target(myUNLIP);
# if __HOST_NAME__ != SERVER_HOST
	D("»»» Using IP# " __HOST_IP_NUMBER__ " known as " __HOST_NAME__ " but given as " SERVER_HOST ".\n");
# else
	D("»»» Using IP# " __HOST_IP_NUMBER__ " known as " __HOST_NAME__ ".\n");
# endif
	D("»»» Static Server UNLs: "+ myUNL +" and "+ myUNLIP +"\n");
        // the callout delay is too long.. maybe we should plug this
        // into some other place? sigh.
	call_out(#'ready_freddie, 0);
#else
	//register_localhost(myLowerCaseHost, "127.0.0.1"); // inbetween...
# ifndef __PIKE__
	dns_resolve(myLowerCaseHost, #'ready_freddie);
# endif
#endif
#ifdef JABBER_PATH
	register_target("xmpp:"+ myLowerCaseHost);
# ifdef _host_XMPP
	t = NAMEPREP(_host_XMPP);
        register_localhost(t);
	register_target(t);
	register_target("xmpp:"+ t);
# endif
#endif
	// base64decode("test2000");
	// D("Digest: ->"+ make_digest("ABCDEFG", "MD5") +"<-\n");
//#ifdef PSYC_SYNCHRONIZE
//	// we could generalize this into a _request_circuit_persistent
//	// which tells both sides of the circuit to disable 'pushback'
//	// and keep queues persistent forever
//	//call_out(#'psyc_sendmsg, 1, PSYC_SYNCHRONIZE, "_request_synchronize",
//	call_out(#'sendmsg, 1, PSYC_SYNCHRONIZE, "_request_synchronize",
//	    "I'm up and ready for some hot sync action!", ([ ]),
//	    SERVER_UNIFORM); //, parse_uniform(PSYC_SYNCHRONIZE));
//# echo PSYC_SYNCHRONIZE activated.
//#endif
}

// the key is intended to be a uniform, the setting a (psyc) keyword
varargs mixed config(mixed key, string setting, mixed value) {
	unless (key) {
		P1(("config for %O: %O -> %O from %O. map is %O.\n",
		    key, setting, value, previous_object(), confmap))
		return 0;	// no uncontrolled write access to confmap
	}
	if (value) {
		P2(("config for %O: %O -> %O from %O. was: %O\n",
		    key, setting, value, previous_object(), confmap[key]))
		unless (member(confmap, key)) confmap[key] = ([]);
		return confmap[key][setting] = value;
	}
	if (setting) {
		unless (member(confmap, key)) return 0;
		return confmap[key][setting];
	}
	return copy(confmap[key]);	// no uncontrolled write access
}

# ifndef __PIKE__
/*
 * what this notify context stuff is about:
 * we could multicast presence
 * this will build the data structure necessary for this
 * currently only for remote users 
 * for local users we dont need to use this bandwidth optimization
 * but it may reduce code if we can streamline this
 *
 * TODO: delete local user from this context if he goes offline
 *
 */

// set local object as handler for context
void set_context(object localo, mixed context) {
	P3(("set %O as context for %O\n", localo, context))
	PROTECT("SET_CONTEXT")
	contexts[context] = localo;
}

// calling "join" is inappropriate for places, isnt it?
varargs void register_context(object localuser, mixed context, array(mixed) u) {
	P3(("register %O in context %O\n", localuser, context))
	PROTECT("REGISTER_CONTEXT")
	unless(contexts[context]) {
            contexts[context] = clone_object(NET_PATH "group/slave");
#ifdef PERSISTENT_SLAVES
            /* persistent cslaves code following */
            contexts[context]->load(context, u);
#endif
        }
	contexts[context] -> insert_member(localuser);
}

void deregister_context(object localuser, mixed context) {
	PROTECT("DEREGISTER_CONTEXT")
	if(objectp(contexts[context])) contexts[context] -> remove_member(localuser);
	// TODO: clean up empty contexts?
}

mixed find_context(mixed context) {
	P3(("searching for %O in %O\n", context, contexts))
		// do we really want that qName() in here?
	return contexts[objectp(context) ? context -> qName() : context ];
}

#ifdef CACHE_PRESENCE
/* this is a rather centralistic approach to presence caching
 * probably it is better to do this in the respective group/slave clones
 */
varargs int persistent_presence(mixed who, int availability) {
	// anything else but PROTECT() is too expensive
	PROTECT("PERSISTENT_PRESENCE")
	// only cache for remote people here
	// we could also store a timestamp here so we know how up2date this
	// information is.. for now let's simply delete the cache if the
	// server restart took too long
	if (availability) presence_cache[who] = availability;
	// timestamps: return AVAILABILITY_UNKNOWN if the information is too old?
	// it is to be proven if timestamps are useful at all, so better
	// gain some experience without them, first.
	return presence_cache[who];
}
#endif

#ifdef SYSTEM_SECRET
// just use the define directly..
//string server_secret_of_the_day() { return SYSTEM_SECRET; }
#else
# ifdef RANDHEXSTRING
// used by jabber's dialback thang
// in contrast, this is ultimatevily important
volatile string ssotd;

string server_secret_of_the_day() {
	// if available, use sha256 for hashing please
	unless (ssotd) ssotd = RANDHEXSTRING;
	return ssotd;
}
# endif
#endif

mapping system_queue() {
        P3(("system_queue: %O\n", systemQ))
	PROTECT("SYSTEM_QUEUE")
	return systemQ;
}

// create a named clone -- amylaar only
object named_clone(string file, string name) {
//	string rc;
	// should use return value?
	P3(("named_clone %O for %O. stack: %O\n", file, name,
	    caller_stack()))
	PROTECT("NAMED_CLONE")
	P2(("legal_name(%O) for named_clone of %O\n", name, file))
	unless (name = legal_name(name)) return (object)0;
	file += "#" + name;
//	D(name +" is to be summoned --------------------------------\n");
//	rc = file -> sName(name);
//	return find_object(file);
	return file -> sName(name);
}

#endif //PIKE

#ifndef USE_LIVING
volatile mapping people = ([]);

void register_person(string name, object o) {
	PROTECT("REGISTER_PERSON")
        if (o) people[name] = o;
        else m_delete(people, name);
        P3(("register_person(%O, %O)\n", name, o))
}

// .... the lowercazed optimization actually doesn't
// help, there is hardly a use for it.. so it may disappear again.
varargs object find_person(string name, vaint lowercazed) {
        if (!stringp(name))
          return 0;
        if (!lowercazed) name = lower_case(name);
        return people[name];
}

int amount_people() { return sizeof(people); }

array(object) objects_people() {
        PROTECT("OBJECTS_PEOPLE")
        return m_values(people);
}
#endif

//object relay;

// could get used by servers aswell, huh?
varargs object summon_person(string nick, vamixed blueprint) {
	object o;
	P2(("summon_person %O of %O for %O\n", nick, blueprint,
	    previous_object()))
	PROTECT("SUMMON_PERSON")
	unless (stringp(nick)) {
		D(S("%O: request from %O to summon_person %O, %O\n",
		    ME, previous_object(), nick, blueprint));
		return 0;
	}
	if (o = find_person(nick)) return o;
#if 0 //def RELAY_OBJECT
//	unless (relay) relay = find_object(RELAY_OBJECT);
//	unless (relay) return 0;
//	return relay;
//	return RELAY ":"+ nick;
	return named_clone(NET_PATH "irc/ghost", nick);
#else
				    // should it be "person" ?
	unless(blueprint) blueprint = DEFAULT_USER_OBJECT;
		// no, because person currently doesnt work standalone
	return named_clone(blueprint, nick);
#endif
}

// look up interface.h for a macro doing the same job
// legal_url() is similar to this, too
#ifndef is_formal
string is_formal(string nicki) {    // formerly known as is_uniform()
	// uniform does not check for objects, so you MUST do that
	// yourself first.
	//unless (stringp(nicki)) return 0;
	//
	// checking for colon after scheme should actually be enough
	// but we also check for user@host syntax, which in the long
	// term could be interpreted as "find out which scheme works"
	// for this person (psyc, xmpp, mailto..)
	//
# echo We don't get here anyway.
# if 1
	if (index(nicki, ':') != -1 || index(nicki, '.') != -1)
	    return nicki;
# else
	if (index(nicki, ':') != -1 || index(nicki, '@') != -1)
	    return nicki;
# endif
	return 0;
}
#endif
#ifndef lower_uniform
// same thing, but returns lowercased name
// not useful for xmpp urls, but elsewhere sometimes
string lower_uniform(string nicki) {
	if (index(nicki, ':') != -1 || index(nicki, '@') != -1)
	    return lower_case(nicki);
	return 0;
}
#endif

void register_scheme(string scheme) {
	PROTECT("REGISTER_SCHEME")
	services[scheme] = schemes[scheme] = previous_object();
}

void register_service(string service) {
	PROTECT("REGISTER_SERVICE")
	services[service] = previous_object();
}

object find_service(string service) {
	object o;

	// schemes are outgoing gateways, but also
	// incoming gateways are services, therefore
	// newsfeeds etc need to do a register_service()
	// currently we just keep them in the same mapping
	// and see if thats good or bad..
	if (services[service]) return services[service];
#ifndef __PIKE__
#ifdef SERVICE_PATH
	else {
	    if (o = ((SERVICE_PATH + service) -> load()))
		return services[service] = o;
	}
#endif
#endif // PIKE
	//return find_object("/service/"+ service);
}

#ifndef ADMINISTRATORS
#define ADMINISTRATORS
#endif
#ifndef OPERATORS
#define OPERATORS
#endif
volatile array(mixed) admins = ({ ADMINISTRATORS });
volatile array(mixed) opers = ({ OPERATORS });

// returns "percentage" of bosslihood
// currently only 0, 50 and 100 is used
//
int boss(mixed guy) {
	if (objectp(guy)) {
#ifdef BOSS_HOST_IP
		if (query_ip_number(guy) == BOSS_HOST_IP) return 90;
#endif
		guy = guy -> qName();
	}
	if (stringp(guy)) guy = lower_case(guy);
	if (index(opers, guy) >= 0) return 50;
	if (index(admins, guy) >= 0) return 100;
	return 0;
}

mixed find_place(mixed a) {
	P3((">> find_place(%O)\n", a))
	string path, err, nick;
	object o;

	if (objectp(a)) return a;
	if (path = lower_uniform(a)) return path;
#ifdef _flag_enable_module_microblogging
	if (sscanf(a, "~%s", nick) && legal_name(nick)) a += "#follow";
#endif
	unless (a = legal_name(a, 1)) return 0;
	path = PLACE_PATH + lower_case(a); // assumes amylaar
	o = find_object(path);
	if (o) return o;
//	PT(("trying to load %O from %O..\n", a, path))
	// return ME seitens des places wäre netter..
#ifndef __PIKE__    // TPD
	err = catch(o = path -> load(a));
#endif //PIKE
	D1(if (err) D("Error loading place: "+err);)
	if (objectp(o)) {
	    ASSERT("find_place", o == find_object(path), path)
	    return o;
	}
	P1(("Warning: place %O did not return ME on load()\n", path))
	return find_object(path);	// auch egal
}

#ifdef PUBLIC_PLACES
// format "localname/uniform", "description", ...
volatile array(string) _places = ({ PUBLIC_PLACES });

// the name 'public_places()' confuses vim's syntax detection
// of lpc.. because 'public' is an lpc keyword.. heehee
array(string) advertised_places() { return _places; }
#endif

#if !defined(SMTP_PATH) || !defined(PRO_PATH)
string legal_mailto(string a) {
	unless (stringp(a) && strlen(a) > 5) return 0;
	if (index(a, '@') < 1 ||
	    index(a, '.') < 3 ||
	    index(a, ' ') >= 0) return 0;
	return lower_case(a);
}
#endif

#ifndef hex2int
// thanks to saga this does now convert hex to integer.. :)
//
// modern ldmud now offers hex2int in form of
// #define hex2int(HEX) to_int("0x"+ HEX)
// i think we should provide hex2int() either
// as macro or library function depending on
// ldmud version.
int hex2int(string hex) {
	int x, i, r, len = strlen(hex);

	each (x, lower_case(hex)) {
		unless ((x>='0' && x<='9') || (x>='a' && x<='f')) return 0;

		if (x >= 'a' && x <= 'f') {
		    x -= 87;
		} else {
		    x -= 48;
		}

//		D(S("x: %d\n", x));
		r += to_int(x * pow(16, len - ++i));
	}
	return r;
}
#endif

#if 0
// only used by /lu these days
int greater_user(object a, object b) { return file_name(a) > file_name(b); }
object* sorted_users() { return sort_array(users(), #'greater_user; }
#endif

volatile mixed mailer;

// simple SMTP interface.. used from person:logAppend - why target?
// assumes that mailto: rcpt has already been checked with legal_mailto() !
int smtp_sendmsg(string target, string method, string data,
	     mapping vars, string source, string rcpt, string loginurl) {
#ifdef SMTP_PATH
	unless (mailer) mailer = SMTP_PATH "outgoing" -> load();
	return mailer -> enqueue(target, method, data, vars, source, rcpt,
				 loginurl);
#else
	return 0;
#endif
}

int xmpp_sendmsg(mixed target, string mc, mixed data, mapping vars,
	     mixed source, array(mixed) u, int showingLog, string otarget) {
	string tmp;
	object o;

	// we use C:xmpp:host(:port) as an object name
	// we can change that if it is confusing
	//tmp = u[UCircuit] || "xmpp:"+ u[UHostPort];

        // xmpp://user@host is not valid. should probably send an error
        if (u[USlashes] == "//") {
            P2(("xmpp uniform uslashes set, bailing out\n"))
            return -1;
        }
	tmp = "xmpp:"+ u[UHostPort];
	if (o = targets[tmp]) {
	    P2(("%O to be delivered on %O\n",
		otarget, o ))
	// in XMPP this is sufficient since other servers on the same IP
	// need to have a different domain name
	} else if (is_localhost(u[UHost])) {
	    unless (u[UUser]) {
		P1(("Intercepted %O to %O from %O\n", mc, target, source))
		// 0 makes sendmsg try to relay via xmpp.scheme.psyced.org
		// but fippo doesn't like that
		return -4;
	    }
	    // this is a lot simpler than find_psyc_object()
	    if (u[UUser][0] == '*' || u[UUser][0] == '#')	// DASH_COMPAT
		o = find_place(u[UUser][1..]);
	    else
		o = summon_person(u[UUser]);
	    // we get here when net/jabber/* doesn't do this job itself
	    // as in fact it probably shouldn't in many cases.
	} else {
#ifdef JABBER_PATH
# ifdef QUEUE_WITH_SCHEME
	    o = ("C:"+tmp)-> circuit(u[UHost], u[UPort]
		  || JABBER_S2S_SERVICE, 0, "xmpp-server",
		  tmp, systemQ);
# else
	    o = ("C:"+tmp)-> circuit(u[UHost], u[UPort]
		  || JABBER_S2S_SERVICE, 0, "xmpp-server",
		  u[UHostPort], systemQ);
# endif
	    register_target(tmp, o);
	    // traditional co-hosting using port
	    // number in uniform is not legal in jabber
	    if (u[UPort])
		register_target("xmpp:"+ u[UHost], o);
#else
	    // we could as well return 0 here, then sendmsg would
	    // ask xmpp.scheme.psyced.org for relay services..
	    // but fippo doesn't like that
	    return source -> w("_error_unavailable_scheme", 
		   "Scheme '[_scheme]' is not available",
		   ([ "_scheme" : "xmpp" ]));
#endif
	}
	register_target(target, o);
	return o->msg(source,mc,data,vars,
		      showingLog,otarget);
}

// PSYC-enabled message delivery function
varargs mixed sendmsg(mixed target, string mc, mixed data, vamapping vars,
	    vamixed source, vaint showingLog, vaclosure callback,
            varargs vamixed extra) {	// proto.h!
	mixed tmp;
	array(mixed) u;
	object o;

	P3(("sendmsg(%O,%O,%O,..,%O,%O,%O)\n",
	    target, mc, data, source, showingLog, callback))

#ifdef SANDBOX
	// we can't avoid having this check in here, so we better avoid
	// having two parallel security systems for the sandbox and thus
	// disallow sending psyc messages without using sendmsg()..
	// this keeps the sandbox tidy and intelligible  :)
	//
	if (extern_call() && (!geteuid(previous_object())
			      || stringp(geteuid(previous_object()))
			      && geteuid(previous_object())[0] != '/')) {
	    unless (source == previous_object()
		    || source == vars["_context"]) {
		raise_error(sprintf("INVALID SENDMSG by %O(%O) (pretended "
				    "to be target/context %O/%O\n",
				    previous_object(),
				    geteuid(previous_object()),
				    target,
				    vars["_context"]));
	    }
	}
#endif

	unless (source) source = previous_object();
	// entity.c doesn't allow vars to be missing so we might
	// just aswell enforce it in the whole psyced source that
	// vars always need to be given as mapping. TODO
	// i changed the behaviour of entity.c because vars are missing
	// everywhere..
	unless (mappingp(vars)) vars = ([]);
#ifdef TAGGING
        /* <fippo> I dont remember exactly why I did not want this 
         *      for stringp sources... but for pushback, it should 
         *      execute even for stringp(source)
	 * <lynX> was objectp(source), changed to just 'source' to
	 *      ensure origin hasn't been destructed in the meantime
	 *      should we restore it in such a case?
         */
#endif
	// target = lower_case(target) ist fuer xmpp nicht
	// gut, weil der resource-teil dort case-sensitive
	// ist... der node@domain-Part aber nicht
	if (stringp(target)) {
		int i;
		string otarget;
		
#ifdef _flag_encode_uniforms_IRC
	// TODO: move this to net/irc if anyone cares
                        // ist das nicht ein alter hack fuer net/irc?
                        // wieso issn der in der lib?
                // alt aber immer noch aktiv... sagt fippo 2007-05-20
		// 2007-12-27 problem with this when people 
		// 	use a msn transport which encodes their 
		// 	buddies as xmpp:user%host@msntransport
		// 	this code simply does not belong here, but 
		// 	should be in irc/user::sendmsg and check
		// 	for verbatimuniform setting
		if ((i = index(target, '%')) != -1) {
		    target[i] = '@';
		}
                        // vielleicht geht !verbatimuniform sonst nicht
                        // vielleicht weiß fippo genaueres
#endif
		otarget = target;
		target = lower_case(target);
#if 1
		//D("sendmsg for "+target+"\n");
		tmp = targets[target];
		if (tmp) { // && interactive(tmp)) {
		    P2(("delivery agent %O for %O (%s)\n", tmp, target,
			mc || "0"))
		    return tmp->msg(source, mc, data, vars, showingLog, otarget);
		}
		// no, we don't do any cleaning of the targets mapping
		// because reconnecting will probably fill the slot again
		//
		// and this also is of no use:
		//else if (tmp = find_object(target)) target = tmp;
		// object names are sound and smoke.
#else
//		if (member(targets, target)) {
//			tmp = targets[target];
//			if (tmp) return tmp -> msg(source, mc, data, vars,
//						   showingLog, target);
//			else m_delete(targets, target);
//		}
#endif
		if (u = parse_uniform(target)) {
			P4(("sendmsg: %O parsed as %O\n", target, u))
	//				previous_object()->w(
	//				     "_error_invalid_uniform",
	//			     "Looks like a malformed URL to me.");
	//				return 0;
	//			}
			switch(u[UScheme]) {
#ifdef PSYC_PATH
			case "psyc":
				return psyc_sendmsg(target, mc, data, vars,
						    showingLog, source, u);
#endif
			case 0:
#if 0 //def DEVELOPMENT
				// we get here when doing remote messaging
				// in xmpp.. and in fact, in net/jabber we
				// don't *know* which scheme needs to be
				// used.
				//
				raise_error("scheme 0 is a bug\n");
				//
				// TODO: we had this error, and maybe it's
				// because user@host addressing does get here
				// so i'm not completely sure it is the right
				// thing to do to just treat it like xmpp, but
				// let's give that a try.
#endif
				//
				// fall thru
			case "xmpp":
#ifdef SWITCH2PSYC
				// maybe we should treat all of this just as if
				// no scheme was given (jid treatment below)
				P4(("LOOKing for %O in %O\n",
				    "psyc://"+u[UHost]+"/", targets))
				tmp = targets["psyc://"+u[UHost]+"/"];
				if (tmp) {
				    // are we talking to our own net/root?
				    // in the past we used interactive() here
				    unless (clonep(tmp)) {
					    tmp = summon_person(u[UNick]);
					    unless (tmp) {
						// doesn't have to be an error.. use jid treatment below?
					        sendmsg(source, "_failure_unavailable_route_place_XMPP",
							"Sorry, you can't talk to a local chatroom via XMPP uniform.");
						return 0;
					    }
				    }
				    PT(("SWITCH2PSYC delivery %O for %O (%s)\n", tmp, target,
					mc || "0"))
				    // we should very probably generate a redirect here instead! TODO
				    return tmp->msg(source, mc, data, vars, showingLog, otarget);
				}
#endif
				// actually jabber does not allow other ports
				// in the url.. but anyway, here
				// comes a jabber implementation which does.
				// so you can only use it to debug jabber
				// code of other psyceds  ;)
				//
				if (xmpp_sendmsg(target, mc, data, vars,
				     source, u, showingLog, otarget)) return 5;
				break;
#ifdef SMTP_PATH
			case "mailto":
				unless (tmp = legal_mailto(u[UUserAtHost])) {
					sendmsg(source, "_error_invalid_mailto",
	 "Sorry, that doesn't look like a valid email address to me.");
					return 0;
				}
				o = summon_person(tmp, SMTP_PATH "user");
				unless (o) return 0;
				register_target(target, o);
				o -> msg(source, mc, data, vars);
				return 3;
#endif
#ifdef RTMP_PATH
                        case "rtmp":
                                return 0;  // unreachable
#endif
			}
			if (schemes[u[UScheme]])
			    return schemes[u[UScheme]]->msg(source,
				 mc, data, vars, showingLog, target);
// ifdeffing out isnt the smartest of options, but for now..
#if defined(GATEWAY_RELAYING) && !defined(BRAIN)
// also, gateway discovery works but gateway relaying on the receiving
// side isn't. additionally, the other psyc boys don't like this approach.
			if (u[UScheme]
# if __EFUN_DEFINED__(regmatch)
			     &&! regmatch(u[UScheme], "[^a-z0-9]")
# else
			     &&! sizeof(regexp( ({ u[UScheme] }), "[^a-z0-9]"))
# endif
			) {
				// current gateway discovery strategy
				u = parse_uniform("psyc://"+ u[UScheme] +
					 ".scheme.psyced.org/$"+ u[UScheme] +
					 "/"+ u[UBody]);
				return psyc_sendmsg(target, mc, data, vars,
						    showingLog, source, u);
			}
#endif
			sendmsg(source, "_error_unknown_scheme",
			     "'[_scheme]' in '[_source_invalid]' is not a supported protocol scheme.",
			     ([ "_scheme": u[UScheme], "_source_invalid": target ]));
		   	return 0;
		}
#if defined(PSYC_PATH) && defined(JABBER_PATH) && !defined(RELAY)
		else if (index(target, '@') != -1 || index(target, '.') != -1) {
			string host;

			// simple user@host notation.
			// this is probably a jabber client talking.
			//
			// first check for special jid2psyc syntaxes
			switch(target[0]) {
			case '#':	// DASH_COMPAT
			    sscanf(target, "#%s@%s", tmp, host);
			case '*':
			    sscanf(target, "*%s@%s", tmp, host);
			    tmp = "psyc://" + host + "/@" + tmp;
			    return psyc_sendmsg(tmp, mc, data, vars,
				     showingLog, source, parse_uniform(tmp));
			case '^':
			    target[0] = '~';
			case '~':
			case '$':
			    sscanf(target, "%s@%s", tmp, host);
			    tmp = "psyc://" + host + "/" + tmp;
			    return psyc_sendmsg(tmp, mc, data, vars,
				     showingLog, source, parse_uniform(tmp));
			}
			// we pretend target is xmpp: to figure out
			// which object to incarnate
			    // should we check targets[] again!??
			u = parse_uniform("xmpp:"+ target);
			//target = "xmpp:"+ target;
			//u = parse_uniform(target);
			// but we leave the target as u@h
			// in case xmpp manages to switch to psyc
			// .. then again, not necessary as we defined a
			// psyc user to be the same as an xmpp user
			// whenever a switch is possible
			// ok, but later targets will deliver the u@h without
			// xmpp: to jabber anyway, so mkjid() needs to be able
			// to deal with that
                        // TODO: if xmpp-s2s is not active, this should
                        // probably assume that this is a psyc address
			if (xmpp_sendmsg(target, mc, data, vars, source, u,
					 showingLog, otarget)) return 6;
		}
#endif
#ifdef RELAY
		// we get here for some funny reasons when doing /nick.. TODO
		P1(("CAUGHT sendmsg(%O,%O,%O,%O,%O,%O,%O)\n",
		    target, mc, data, vars, source, showingLog, callback))
#else
		    // we could even start looking for local nicknames here..
		    // but this would circumvent /alias
		sendmsg(source, "_error_unknown_name_user",
		      "[_nick_target] isn't available.",
		      ([ "_nick_target" : target ]));
#endif
		return 0;
	}
	if (objectp(target)) {
		target -> msg(source, mc, data, vars, showingLog);
		// make sure msg is treated as successfully delivered:
		return 2;
	}
	D2(else D(S("sendmsg encountered %O as target for (%O,%s,%O,%O)\n",
		target, source, mc, data, vars));)
	return 0;
}

#ifndef __PIKE__

#ifdef PSYC_PATH
# ifdef DRIVER_HAS_BROKEN_INCLUDE
#  include "/net/psyc/library.i"
# else
#  include PSYC_PATH "library.i"
# endif
#endif

#ifdef IRC_PATH
# ifdef DRIVER_HAS_BROKEN_INCLUDE
#  include "/net/irc/library.i"
# else
#  include IRC_PATH "library.i"
# endif
#endif

string decode_embedded_charset(string text) {
    int i;
    string work;
    array(string) exploded1;

    // we only support the system charset currently .. iconv to follow TODO
#if SYSTEM_CHARSET == "UTF-8"
    exploded1 = regexplode(text, "=\\?[uU][tT][fF]-8\\?Q\\?[^?]*\\?=");
#else
    exploded1 = regexplode(text, "=\\?[iI][sS][oO]-8859-15?\\?Q\\?[^?]*\\?=");
#endif
    text = "";

    each (work, exploded1) {
	string *exploded2;
	string s;
	int j = 0;

	if (++i % 2) {
	    unless (work == " ") text += work;
	    continue;
	}

	exploded2 = regexplode(replace(slice_from_end(work, 15, 3), "_", " "),
                               "=[A-Z0-9][A-Z0-9]");
	work = "";

	each (s, exploded2) {
	    if (++j % 2) {
		work += s;
		continue;
	    }
	    
	    s = s[1..];
	    work += sprintf("%c", hex2int(s));
	}
	text += work;
    }

    return text;
}

// things only the twitter gateway needs...
//
// this is compatible to Perl's <=> operator:
// Binary "<=>" returns -1, 0, or 1 depending on whether the left argument
// is numerically less than, equal to, or greater than the right argument.
int bignum_cmp(string a, string b) {
	int i;
	if (a == b) return 0;
	// calling strlen is probably faster than
	// allocating a local variable to "cache" it
	if (strlen(a) > strlen(b)) return 1;
	if (strlen(a) < strlen(b)) return -1;
	for (i=0; i<strlen(a); i++) if (a[i] != b[i]) break;
	P4(("bignum_cmp\n%O vs\n%O at %O is %c vs %c\n", a, b, i, a[i], b[i]))
	if (a[i] > b[i]) return 1;
	return -1;
}

#endif // __PIKE__

object library_object() {
    return ME;
}
