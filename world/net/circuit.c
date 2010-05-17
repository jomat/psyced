// vim:foldmethod=marker:syntax=lpc:noexpandtab
// $Id: circuit.c,v 1.107 2008/07/04 18:37:21 lynx Exp $
//
// net/circuit - generic circuit manager
//
// net/circuit implementiert ein protokollabstrahiertes management von
// psyc objekten hinter TCP verbindungen (oder anderen virtual circuit
// technologien, die wir aber vermutlich nie sehen werden), es versucht
// nach verlorener verbindung diese evtl wieder aufbauen, bzw baut die
// erste verbindung auf wenn ein objekt auf dem server eine wünscht.
// aus diesem grund führt es eine message queue für noch auszuliefernde
// nachrichten. sollte die verbindung nicht herstellbar sein wird es
// den unglücklichen sources eine fehlermeldung zurückschicken (mailer
// daemon lässt grüssen).
//
// also.. maybe one day this object will take care of _context
// optimizations.. just maybe.. maybe the places will do..

// local debug messages - turn them on by using psyclpc -DDcircuit=<level>
#ifdef Dcircuit
# undef DEBUG
# define DEBUG Dcircuit
#endif

#if defined(DEBUG) && DEBUG > 1
# define CONNECT_RETRY          2	// seconds for testing
#else
# define CONNECT_RETRY          8	// = 8+8+random(8) to start with
#endif
#ifndef MAX_RETRY
# define MAX_RETRY	3	// retry in 1 + 3 + 9 minutes
#endif

#include <net.h>
#include <dns.h>

inherit NET_PATH "connect";

volatile mixed me;
volatile string host;
volatile int port;
volatile int retry;

volatile int waitforme;
volatile int time_of_connect_attempt;

#ifdef NEW_QUEUE
inherit NET_PATH "queue2";
#else
inherit NET_PATH "queue";
#endif

#if 0 //def PSYC_SYNCHRONIZE
# define ISSYNC issync
volatile int issync;
#else
# define ISSYNC 0
#endif

#ifndef ERQ_WITHOUT_SRV
string hostname;	// original hostname

srv_choose(mixed *hostlist, string transport) {
	string srvhost; 
	int srvport;

	P3(("srv_choose: %O for %O\n", hostlist, ME))
	unless (pointerp(hostlist)) {
		return connect(hostname, port, transport);
	}       
	// choose a host according to prio/weight/bla
	// actually, the dns resolver should already order them
	// in a load balancing way
	srvhost = hostlist[0][DNS_SRV_NAME];
	srvport = hostlist[0][DNS_SRV_PORT];
	return connect(srvhost, srvport, transport);
}
#else
# echo Warning: No SRV enabled. Will not be able to talk to jabber.ccc.de etc.
#endif

void reconnect() {
	if (ISSYNC || retry++ < MAX_RETRY) {
		waitforme = waitforme * 2 + random(waitforme);
		call_out(#'connect, waitforme);
		P2(("%O trying to reconnect in %d secs\n", ME, waitforme))
		return;
	}
	connect_failure("_repeated",
	    "Could not establish a circuit to "
	    + host +":"+ port);
}

void pushback(string failmc) {
	mixed *t, o;
	mapping vars;

	P2(("%O pushback of queue(%O) (size %O).\n", failmc, me, qSize(me)))
	while(qSize(me) && (t = shift(me))) {
	    // is it okay to append pushback mc?
	    // or should we even append the mc of the respective message!?
	    string mc = "_failure_unsuccessful_delivery" + failmc;

	    P3(("failure... delivering queue(%O) back (%O to go). next: %O\n",
	       me, qSize(me), t))
	    // 0:source, 1:method, 2:data, 3:vars, 4:target
	    if (mappingp(t[3])) {
		    vars = t[3];
		    o = vars["_context"];
	    } else vars = ([]);
	    // is o || t[0] correct? the following can happen if
	    // we are using mcast:
	    // suppose we have some people from our host in a 
	    // room on the remote server. If the connect fails
	    // we should notify them all that the communication
	    // is broken currently
	    // on the other hand, if this is used for persons
	    // it could possibly result something anti-privacy
	    //
	    // so do we want to search for a local handler of t[4] first?
	    if (t[1] == mc) {
		    // this happens when the place is stupid enough to
		    // castmsg the failure.
		    P1(("%O caught attempt to resend %O to %O || %O\n",
			ME, mc, o, t[0]))
	    } else sendmsg(o || t[0], mc, // ok, should this message really
					  // have its original target as
					  // source, should _source be in
					  // the passed mapping vars
					  // additionaly to just being passed
					  // as source, and:
					  // rooms need to know ME for removing
					  // all users with this circuit from
					  // their mcast structueres, but is
					  // _INTERNAL_origin the correct
					  // name in this case (especially if
					  // we decide that source shouldn't
					  // be the original target)?
	       "Could not establish a circuit to [_host] in order to deliver a [_method_relay] to [_source].",
	       ([ "_method_relay": t[1],
		  "_data_relay" : t[2],
		// we used to add the vars to the error message,
		// but that causes a problem in places with nick
		// apparently coming from a different source..
		  "_nick": vars["_nick_target"],
		  "_source": t[4],
		  "_host": host,
		  "_source_origin": SERVER_UNIFORM,
		  "_INTERNAL_origin" : ME,
		  // so that tag-operated queues can be rolled back
		  "_tag_reply" : t[3]["_tag"],
		  "_tag_relay" : t[3]["_tag"] || t[3]["_tag_reply"],
		  "_source_relay": t[0] || t[3]["_source"] ]), t[4]);
			    // + vars
	}
//	retry = 0;
	// P2(("%O qDel(%O) and autodestruct\n", ME, me))
	qDel(me);	// not sure if this is necessary but looks safer
	destruct(ME);
	// alright. so this is where we want to do something
	// differently, like remember that this host didn't work.  TODO
}

// wouldn't it be nicer to also pass real vars here?
void connect_failure(string mc, string reason) {
	::connect_failure(mc, reason);
	if (abbrev("_attempt", mc)) reconnect();
	else pushback(mc);
}

int msg(string source, string method, string data,
	    mapping vars, int showingLog, mixed target) {
	P1(("%O:msg() shouldn't get called. overload me!\n", ME))
	return 0;
}

circuit(ho, po, transport, srv, whoami, sysQ, uniform) {
	P2(("%O circuit(%O, %O, %O, %O)\n", ME, ho, po, transport, srv))
	if (me) {
		// happens apparently when a racing condition occurs
		// during upgrade from xmpp to psyc.. hm! queue fails
		// to deliver in that case and waits for next chance TODO
		P1(("%O loaded twice for %O and %O\n", ME, me, whoami))
		return ME;
	}
	q = mappingp(sysQ) ? sysQ : system_queue();
#if 0
	unless(whoami) whoami = ME; 
#else
	unless(whoami) whoami = ho || ME; 
#endif
	qInit(me = whoami, 3303, 12);
	D1( if (q[me]) PP(("%O using %O's queue: %O\n", ME, me, sizeof(q[me]) > 2 ? sizeof(q[me]) : q[me])); )
	waitforme = CONNECT_RETRY;
	retry = 0;
#if 0 //def PSYC_SYNCHRONIZE
	issync = stringp(uniform) && abbrev(PSYC_SYNCHRONIZE, uniform);
	D1( if (issync) PP(("The synchronizer, that's %O\n", uniform)); )
#endif
	if (ho) connect(ho, po, transport, srv);
	return ME;
}

// who needs this? who calls this? /rm and derivatives. shutdown() too.
// net/psyc/active because net/psyc/server has its own
quit() {
	P2(("%O quit.\n", ME))
	remove_interactive(ME);
	//destruct(ME);
}

runQ() {
	mixed *t, source;

	D2(unless (me) raise_error("unitialized circuit\n");)
	P3(( "%O runQ of size %O\n", ME, qSize(me)))
	// causes an exception when q is too big
	P4(( "%O\n", q))
	while (qSize(me) && (t = shift(me))) {
		// 0:source, 1:method, 2:data, 3:vars, 4:target
		// revert to string source if the object has destructed
		source = t[0] || t[3]["_source"];
#ifdef FORK
		msg(source, t[1], t[2], t[3], 0, t[4], t[5]);
#else
		msg(source, t[1], t[2], t[3], 0, t[4]);
#endif
#if 1
		// <lynX> i need to know if this could ever happen, and if so
		// if it is harmless anyhow. see below.
		D1( if (member(t[3], "_source_relay") &&! t[3]["_source_relay"])
		    PP(("runQ in %O: lost _source_relay for %O from %O to %O\n",
			ME, t[1], source, t[4])); )
#endif
	}
	retry = 0;	// should we get disconnected restart from 8 seconds
	waitforme = CONNECT_RETRY;
}

connect(ho, po, transport, srv) {
	if (interactive()) return -8;
	P3(("connect: %O, %O, %O, %O for %O\n", ho, po, transport, srv, ME))
	if (time() < time_of_connect_attempt + waitforme) return -2;
	if (ho) {	// paranoid: stringp(ho) && strlen(ho)) {
		if (po) port = po;
#ifndef ERQ_WITHOUT_SRV
		if (srv) {
		    hostname = lower_case(ho);
		    host = 0;
		} else 
#endif
		host = lower_case(ho);
		P2(("connect.%s:\t%O, %O, %O\t%O\n", srv || "to",
				 ho, po, transport, ME))
	}
#ifndef ERQ_WITHOUT_SRV
	P4(("connect->srv_choose? depends on srv %O\n", srv))
	if (srv) return dns_srv_resolve(hostname, srv,    // _psyc._tls.domain
			transport == "s" ? "tls" : "tcp", #'srv_choose,
		       	transport);
#endif
	if (::connect(host, port, transport) >= 0)
	     time_of_connect_attempt = time();
}

disconnected(remaining) {
	int rc = 0; // unexpected

	if (qSize(me)) {
		rc = ::disconnected(remaining);
		reconnect();
	}
	else {
		P2(("%O circuit to %O disconnected and terminated\n", ME, me))
		// we should unregister all our hostnames and ip numbers
		// so that they get checked again once when a new circuit
		// is established... TODO
		destruct(ME);
	}
	return rc;
}

// varargs just to support mvars in FORK mode
varargs int enqueue(mixed source, string method, string data,
	    mapping vars, int showingLog, mixed target, mapping mvars) {
	// psyc_sendmsg() sets vars["_source"] in case
	// the source got destructed in the meantime.
	// other enqueuers should do the same i guess.
	// in fact it would be nicer to do it here at
	// enqueue, but psyc_sendmsg() has already done
	// the psyc_name calculation, so better use that.
	// let's see if this code gets used.. yep it does
#if 0 // DEBUG > 0
// with proper routing it is now quite common to have a public message
// with a context *instead* of a source, so it's pointless to complain...
//
//	unless (vars["_source"]) {
//	    P1(("tell lynX: enqueue without _source for %O from %O in %O\n",
//	       	method, source, ME))
//	    //vars["_source"] = psyc_name(source);
//	    raise_error("tell lynX where it happened!!\n");
//	} 
#endif
	P4(("enqueue for %O\n", source))
	connect(); // will only connect if we once had been connected before
	return ::enqueue(me, ({ source, method, data, vars, target, mvars }) );
}

