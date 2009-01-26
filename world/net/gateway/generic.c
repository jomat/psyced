// $Id: generic.c,v 1.36 2008/02/08 12:53:25 lynx Exp $ // vim:syntax=lpc
//
// this is a generic robot that provides a few commands to interface
// a centralistic messaging system to the PSYC. since the commercial
// system have a political interest not to support this, this is only
// available as an IRC gatebot these days. See net/irc/gatebot.c

#include <net.h>
#include <text.h>
#include <closures.h>

#define URL_DOC "http://www.psyc.eu/gatebot"
#define INFO "\
Gateway to the Protocol for SYnchronous Conferencing.\n\
Find more information at " URL_DOC

virtual inherit NET_PATH "output";

#ifndef SERVER_UNL
# define SERVER_UNL query_server_unl()
#endif

volatile object psycer;
volatile string joe, joe_nick;
volatile mixed joe_unl;
volatile mapping talk = ([]);
volatile closure sort_by_name;

queryLastServed() { return joe; }

static input(text) {
	string com, rest;

	if (!sscanf(text, "%s %s", com, rest)) com=text;
	unless (talk[joe]) com = upper_case(com);
	switch(com) {
// we grant no administrative functions from the legacy system as we
// don't have proper authentication there..
//
#if 0 //def GATEWAY_COMMAND_DISCONNECT
case GATEWAY_COMMAND_DISCONNECT:
		reply("Bye bye..");
		return disc();
#endif
#if 0 //def GATEWAY_COMMAND_DONATE
case GATEWAY_COMMAND_DONATE:
		reply("Cool man.");
		return chop(joe);
#endif
case "M":
case "MSG":
case "T":
case "TALK":
case "QUERY":
case "Q":
case ".":
case "QUIT":
case "TELL":	return tell(rest);
case "H":
case "HELP":	return help();
case "S":
case "ST":
case "STATUS":
case "N":
case "NA":
case "NAMES":	return stat();
case "W":
case "WHO":	return who();
	}
	if (talk[joe]) {
		// i want regular echoes back.. hmmm
//		reply("You tell "+ talk[joe]+": "+text);
		sendmsg(talk[joe], "_message_private", text,
		    ([ "_nick" : joe_nick ]), joe_unl);
	} else unknownmsg(text);
}

static help() {
#ifdef INFO
	string t;
	foreach(t : explode(INFO, "\n")) reply(t);
#endif
#ifdef WEBMASTER_EMAIL
	reply("This gateway is operated by " WEBMASTER_EMAIL);
	    // " on "+ SERVER_UNL);
#endif
//	reply("Can you imagine this is the new version of the first ever IRC bot written in LPC?");
	reply("Available commands: WHO, STATUS, TELL/MSG, TALK/QUERY, HELP");
}

static tell(a) {
	string whom, text;
	mixed p;
	
	if (!a || !sscanf(a, "%s %s", whom, text)) {
		whom = a;
		text = 0;
	}
	unless (whom) {
		if (talk[joe]) {
		    reply("Okay. Query terminated.");
		    talk[joe] = 0;
		} else {
		    reply("Usage: T(ELL) or MSG <psycer> <textmessage>");
		    reply("Usage: T(ALK) or QUERY <psycer>");
		    reply("<psycer> may either be a nickname on "+ SERVER_UNL +
			  " or a uniform network identification anywhere"
			  " in PSYCspace.");
		}
		return;
	}
	unless (is_formal(p = whom)) {
		p = summon_person(whom);
		if (!p) return reply(whom+" ain't here on "+ SERVER_UNL);
	}
	if (text) {
		reply("You tell "+ UNIFORM(p) +": "+text);
		sendmsg(p, "_message_private", text,
		    ([ "_nick" : joe_nick ]), joe_unl);
	}
	else talkto(p);
	// log_file("IRCTELL", ctime(time())+" IN : "+joe+" talks to "+whom);
}

static stat() {
    reply("Gateway "+ psyc_name(ME) +" is connected to "+ query_ip_number());
}

// hi there möchtegernhacker, the WHO function has no serious purpose
// as it only lists the local users anyway, so the real intention is to
// give you a very easy means to disconnect the gateway from your IRC
// network. if you don't like PSYC, simply issue a few WHOs and the
// gatebot will get killed by excess flood. if the gateway is alive and
// functioning properly, it's a sign that your IRC network isn't so rude
// after all.
static who() {
	mapping uv;
	mixed *u;
        int all;
        mixed idle;
        string desc;

	unless (closurep(sort_by_name)) 
		sort_by_name = lambda(({ 'a, 'b}),
			({ (#',),
			 ({ CL_NIF, ({ #'mappingp, 'a }), ({ #'return, 0 }) }),
			 ({ CL_NIF, ({ #'mappingp, 'b }), ({ #'return, 1 }) }),
			 ({ #'return, ({ (#'>),
				       ({ CL_LOWER_CASE, ({ (#'||), ({ CL_INDEX, 'a, "name" }), "" }) }),
				       ({ CL_LOWER_CASE, ({ (#'||), ({ CL_INDEX, 'b, "name" }), "" }) }),
				       }) })
			 }));

	reply("--- /who of local users of "+ SERVER_UNL);
	u = objects_people();
	all = sizeof(u) < 23;
	u = sort_array(u->qPublicInfo(all), sort_by_name);
	foreach (uv : u) if (mappingp(uv)) {
		desc = uv["me"];
		if (desc || all) {
//			if (idle = uv["aliveTime"]) {
//				idle = hhmm(ctime(time() - idle - 60*60));
//				if (idle == "00:00") idle = "--:--";
//			}
//			idle = uv["aliveTime"] ?
//			     timedelta( time()-uv["aliveTime"] ) : "??:??";
			idle = intp(uv["idleTime"]) ?
			     timedelta( uv["idleTime"] ) : "??:??";
			if (desc)
			    reply( S("[%s] %s %s.",
				idle, uv["nick"], desc) );
			else
			    reply( S("[%s] %s is online.",
				idle, uv["nick"]) );
		}
	}
	reply("--- end of /who");
	return 1;
}

unknownmsg(text) {
	reply("No target set for your message. Please use 'TALK <<psyc:address>>' to define one. Try HELP for more help.");
	// D0( D(joe+" tells gatebot: "+text); )
	monitor_report("_warning_unexpected_gateway_message", "Gateway: "+
	    joe_unl+" says: "+text);
}

talkto(p) {
	reply("Query with "+ UNIFORM(p)+ " begun. Send '.' to stop.");
	reply("Try HELP (uppercase) for more help.");
	talk[joe] = p;
	P4(("talk is %O\n", talk))
}

