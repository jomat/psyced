// $Id: master.c,v 1.61 2008/03/11 16:15:15 lynx Exp $ // vim:syntax=lpc:ts=8
//
#ifdef INIT_FILE
#undef INIT_FILE
#endif
#define INIT_FILE "/local/init.ls"

// protos
mixed valid_read(string path, string eff_user, string call, object caller);
mixed valid_write(string path, string eff_user, string call, object caller);

// bei amylaar und ldmud braucht master.c den absoluten pfad..
#include "/local/config.h"
#include NET_PATH "include/net.h"
#include DRIVER_PATH "sys/driver_hook.h"
#include DRIVER_PATH "sys/debug_message.h"

// go fetch the rest of the master object code
#ifdef PRO_PATH
inherit PRO_PATH "master";
#else
# include DRIVER_PATH "master/psycmuve.i"
#endif
#include DRIVER_PATH "master/classic.i"

/* Since the normal operation mode of psyced is not to have any
 * access by "wizards" all of the security functions of LPMUD are
 * disabled by the following dummy functions
 */

int valid_exec(string name, object ob, object obfrom) {
//  switch(name) {
//    case "secure/login.c":
//    case "obj/master.c":
#ifdef SANDBOX // sucks here. i'd love to use uids, but we get a $%!!"
	       // program name string
#ifndef __COMPAT_MODE__
	if (name[0] == '/') name = name[1..];
#endif
	if (abbrev(name, "users/")) return 0;
#endif
	if (!interactive(ob)) {
	    return 1;
        }
//  }
    return 0;
}

mixed valid_write(string path, string eff_user, string call, object callo) {
	P4(("valid_write(%O,%O,%O,%O)\n", path,eff_user,call,caller))
#ifdef SANDBOX
	int i;

	if (stringp(eff_user)) {
	    if (eff_user[0] == '/') {
		return 1;
	    } else if (eff_user[0] == '@') {
		string file = eff_user[1..];

		PT((">> %O %O\n", path[12..], file));
		return abbrev(DATA_PATH "place/", path) && path[12..] == file;
	    }
	}

	return 0;
#else
	return 1;
#endif
}

mixed valid_read(string path, string eff_user, string call, object callo) {
	P4(("valid_read(%O,%O,%O,%O)\n", path,eff_user,call,caller))
#ifdef SANDBOX
	int i;

	if (stringp(eff_user)) {
	    if (eff_user[0] == '/') {
		return 1;
	    } else if (eff_user[0] == '@') {
		string file = eff_user[1..];

		PT((">> %O %O\n", path[12..], file));
		return abbrev(DATA_PATH "place/", path) && path[12..] == file;
	    }
	}

	return 0;
#else
	return 1;
#endif
}

mixed privilege_violation(string op, mixed who, mixed arg3, mixed arg4) {
	P4(("privilege %s(%O,%O) granted to %O\n", op,arg3,arg4,who))
#ifdef SANDBOX	
	unless(objectp(who)) {
	    P0(("¶¶ master: privilege_violation(%O, %O, %O) from !obj %O\n", op, arg3, arg4, who));
	    return 0;
	}

	if (stringp(geteuid(who)) && geteuid(who)[0] == '/') {
	    return 1;
	}

	return 0;
#else
	return 1;
#endif
}

string *include_dirs() {
    return ({
#ifdef PRO_PATH
		PRO_PATH "include/",
#endif
#ifdef NET_PATH
		NET_PATH "include/",
#endif
#ifdef DRIVER_PATH
		DRIVER_PATH "include/",
		DRIVER_PATH "sys/",	// this should be faded out TODO
		DRIVER_PATH,
#else
		"/sys/",
#endif
		"/include/"
	});
}

#if 0
void log_error(string file, string err, int warn) {
	debug_message(S("\n\n\n\n\n%O %O in %O\n", warn ? "WARNING" : "ERROR",
	    err, file), 1+2+4);
}
#endif

void runtime_error(string error, string program,
		   string current_object, int line) {
	string msg;

	if (program && current_object) {
		msg = sprintf("EXCEPTION in %s:%d (%s):\n\t%s",
		    program, line, current_object, error);
		debug_message(msg, DMSG_STDERR | DMSG_LOGFILE); // | DMSG_STAMP);
		//, DMSG_STDOUT | DMSG_STDERR | DMSG_LOGFILE | DMSG_STAMP);
		// DMSG_DEFAULT: /* log to stdout and .debug.log */

		// it's on old mud tradition to show the error to the current player
		// but in psycworld these are protocols of potentially very delicate
		// syntaxes, so let's be kind and not break them.
#if DEBUG > 0
		// let's tell the object about it,
		// hoping that it will not recurse into a further error	-lynX
		// obviously i forgot master can be interactive itself!
		if (this_interactive() && this_interactive() != ME)
		    this_interactive()->runtime_error(error, program, current_object, line, msg);
#endif
#if 0
	} else {
		// the else case isn't interesting as the driver puts a comment
		// about it into the debug log anyway. looks like this:
		//   "Object 'psyc:...' the closure was bound to has been destructed"
		//   "No program to trace."
		// and happens when an outgoing connection has been made for a circuit
		// that has been destroyed in the meantime.. like by shutdown()
		// i don't think we can avoid such a circumstance really, so consider
		// it a regular behaviour - a warning message, but nothing's wrong really.
		// i mean.. we don't want to wait for connections to establish while we
		// are shutting down.. do we?
		msg = "EXCEPTION: "+ error;
		debug_message(msg, DMSG_LOGFILE | DMSG_STAMP);
#endif
	}
}

void disconnect(object ob, string remaining) {
	string host = query_ip_name(ob);
	string name = object_name(ob);

	PT(("disconnected: %O from %O%s\n", ob, host,
	    remaining && strlen(remaining) ?
	      " with "+ strlen(remaining) +" bytes remaining" : ""))
					// happens when first clone fails
	unless (ob && objectp(ob) && ob != ME) return;
	unless (ob->disconnected(remaining)) {
		// disconnected() must return true when the
		// socket disconnection was expected and is no
		// cause for concern. if we got here, it is.
		//
		// 'remaining' is empty in most cases, still
		// we don't output 'remaining' on console
		// as it occasionally triggers a utf8 conversion error
		// instead we drop this into a logfile
		SIMUL_EFUN_FILE -> log_file("DISC", "%O %O %O\n",
					    name, host, remaining);
	}
}

// even though the name of the function is weird, this is the
// place where UDP messages arrive
//
// how to multiplex InterMUD and PSYC on the same udp port:
// PSYC UDP packets always start with ".\n", just forward them to
// the PSYC UDP server daemon.
//
volatile object psycd;
#ifdef SPYC_PATH
volatile object spycd;
#endif
#ifdef SIP_PATH
volatile object sipd;
#endif

void receive_udp(string host, string msg, int port) {
	if (strlen(msg) > 1 && msg[1] == '\n') switch(msg[0]) {
#ifdef SPYC_PATH
	case '|':
		unless (spycd) {
			spycd = SPYC_PATH "udp" -> load();
			PT(("SPYC UDP daemon created.\n"))
			unless (spycd) return;
		}
		spycd -> parseUDP(host, port, msg);
		return;
#endif
	case '.':
		unless (psycd) {
			psycd = PSYC_PATH "udp" -> load();
			PT(("PSYC UDP daemon created.\n"))
			unless (psycd) return;
		}
		psycd -> parseUDP (host,port,msg);
		return;
	}
#ifdef SIP_PATH
	string mc, rcpt;

	if (abbrev("SIP", msg) || 
	    sscanf(msg, "%s sip:%s", mc, rcpt) == 2) {
		unless (sipd) {
			sipd = SIP_PATH "udp" -> load();
			PT(("SIP UDP daemon created.\n"))
			unless (sipd) return;
		}
		sipd -> parseUDP (host, port, msg, mc, rcpt);
		return;
	}
#endif
	P1(("Caught unknown UDP packet from %s:%d\n", host,port))
	P2(("Content: %O\n", msg))
	SIMUL_EFUN_FILE -> log_file("UDP", "[%s] %s:%d %O\n", ctime(), host, port, msg);
	return;
}

#ifndef __LDMUD__
// older versions may still need this..
void receive_imp(string host, string msg, int port) {
	receive_udp(string host, string msg, int port);
}
#endif

// this master function is called at the end of the shutdown procedure
void notify_shutdown(string crash_reason) {
    object o;
    int i;

    P3(("notify_shutdown(%O) from %O\n", crash_reason, previous_object()))
    if (previous_object() && previous_object() != this_object())
	return;
#if DEBUG > 0
    if (crash_reason) PP(("CRASH! %O\n", crash_reason));
#endif
    SIMUL_EFUN_FILE -> log_file("LOGON", "[%s] _ SERVER SHUTDOWN (%O)\n", ctime(), crash_reason);
#ifdef DEBUG_LOG
    SIMUL_EFUN_FILE -> log_file(DEBUG_LOG, "[%s] _ SERVER SHUTDOWN (%O)\n", ctime(), crash_reason);
#endif
    // walk thru the shutdown path a third time in case this is a
    // shutdown by kill -1 process.
    SIMUL_EFUN_FILE -> server_shutdown(4404, 2);
    // save_wiz_file();
}

// called when memory gets low or something like that.. never seen this happen
void slow_shut_down(int minutes) {
    SIMUL_EFUN_FILE -> shout(0, "_notice_broadcast_shutdown_panic",
	"Server is slowly running out of memory. Restart imminent.");
    SIMUL_EFUN_FILE -> server_shutdown(1, 0);
}

// called by driver at shutdown for every user
// but *after* all network sockets have been shut
// so its mostly useless
void remove_player(object victim) {
	if (victim) {
		// this message is normal for psyc circuits
		// they need to be available til the bitter end
		P2(("%O found still alive after reboot()\n", victim))
		//catch(victim->reboot()); .. pointless to try again
	}
	// if (victim) destruct(victim);
}

/* This should be called when everything else is done,
 * but standard drivers do not provide that, and its not worth a patch
 */
void preload_done() {
#ifdef MASTER_LINK
	D1( D("Loading master link.\n"); )
	(PSYC_PATH "active") -> connect(MASTER_LINK);
#endif
#if DEBUG > 1
	D("Loading done. Dumping objects.\n");
	debug_info(5, "objects", "/log/objects.dump");
#else
	// D("Starting service.\n");
#endif
	SIMUL_EFUN_FILE -> log_file("LOGON", "[%s] ^ SERVER START\n", ctime());
#ifdef DEBUG_LOG
	SIMUL_EFUN_FILE -> log_file(DEBUG_LOG,"[%s] ^ SERVER START\n",ctime());
#endif
#ifdef HALT
	shutdown();
#endif
}

mixed current_time;

string *epilog(int eflag) {
    if (eflag) return ({});

//#if __VERSION_MINOR__ > 2
    // as long as the driver doesn't provide it
    // we have to call it ourselves *before* preloading
    // which means that all involved compilations will have wrong times
    preload_done();
//#endif
    D1( D("Preloading from " INIT_FILE "\n"); )
#ifndef BROKEN_RUSAGE
    D1( current_time = rusage(); )
    D1( current_time = current_time[0] + current_time[1]; )
#endif
    return explode(read_file(INIT_FILE), "\n");
}

void preload(string file) {
    D1( int last_time; )

    if (strlen(file) && file[0] != '#') {
	if (file[0..1] == "./") file = file[2..];
        D1( last_time = current_time; )
        P1(("Loading: %s", file))
//	call_other(file, "");
	call_other(file, "load");
#ifndef BROKEN_RUSAGE
        D1( current_time = rusage(); )
        D1( current_time = current_time[0] + current_time[1]; )
#endif
        P1((" %.2f\n", (current_time - last_time)/1000.))
    }
}

void inaugurate_master(int arg)
{
    if (!arg) {
        if (previous_object() && previous_object() != this_object())
            return;
    }
#if 0 // enable_telnet is better for what we want
    set_driver_hook(H_TELNET_NEG, "telnet_negotiation");
#endif

#ifndef SANDBOX
    set_driver_hook(H_LOAD_UIDS, (: "/" :));
    set_driver_hook(H_CLONE_UIDS, (: "/" :));
#else
    set_driver_hook(
	H_LOAD_UIDS,
	function string (string obn) {
	    //string bp = program_name(obn), mp;
	    string bp = program_name(find_object(obn)), mp, t;
	    array(string) path;

#ifndef __COMPAT_MODE__
	    if (bp[0] == '/') bp = bp[1..];
#endif
	    path = explode(bp, "/");
	    path = sizeof(path) > 1 ? path[..<2] : ({ "/" });
	    mp = path[0];

	    switch (mp) {
		case "place":
		    return "/place";
		case "user":
		    sscanf(bp, "%!s/%s.c", t);
		    return "@" + t;
		case "net":
		    if (sizeof(path) > 2) switch (path[1]) {
			case "library":
			    return "/library";
			case "d":
			    return "/daemon";
		    }
		    return "/system";
		case "drivers":
		    if (abbrev("world/drivers/ldmud/master", bp)) {
			return "/master";
		    }
		    if (abbrev("world/drivers/ldmud/library", bp)) {
			return "/library";
		    }
	    }
	    return "/else";
    });
    set_driver_hook(H_CLONE_UIDS,
	function array(string) (object bp, string new) {
	    return ({ getuid(bp), geteuid(bp) });
    });
#endif
    set_driver_hook(H_INCLUDE_DIRS, include_dirs());
#ifdef SUPER_XMLRPC
    // such superfancy driver hacks make psyced less portable to muds  :(
    set_driver_hook(H_DEFAULT_METHOD, function int (mixed res,
						    object ob,
						    string fun,
						    varargs mixed *args) {
	if (program_name(ob) == NET_PATH "http/xmlrpc.c"[1..] && fun != "__INIT") {
	    res = ob->request(fun, args[..<2], args[<1]);
	    return 1;
	}
	return 0;
    });
#endif
    // we use create() even if we are in compat mode  -lynX 2004
    set_driver_hook(H_CREATE_SUPER, "create");
    set_driver_hook(H_CREATE_OB,    "create");
    set_driver_hook(H_CREATE_CLONE, "create");

    set_driver_hook(H_RESET,        "reset");
    set_driver_hook(H_CLEAN_UP,     "clean_up");

    //set_driver_hook(H_MODIFY_COMMAND_FNAME, "modify_command");

    // actually this should never be spit out.. we must realize we
    // are about to run out of sockets before it actually happens
    // and take action!  TODO
    set_driver_hook(H_NO_IPC_SLOT,  ".\n\n_failure_exhausted_sockets\n.\n\n");

    //set_driver_hook(H_NOTIFY_FAIL, "_failure_broken_parser\n.\n\n");
    set_driver_hook(H_NOTIFY_FAIL,
		unbound_lambda(({ 'entered_command, 'cmd_giver }),
	({ #'?, ({ #'=, 'cl, ({ #'symbol_function, "internalError",
				  ({ #'this_object }) }) }),
	 ({ #'funcall, 'cl, 'entered_command }),
	 ({ #'write, "Huh?\n" })
    })));

#ifdef H_DEFAULT_PROMPT
    set_driver_hook(H_DEFAULT_PROMPT, "");
#endif
}

string get_master_uid() { return "/master"; }

string get_bb_uid() { return "undefined"; }

#ifdef __EFUN_DEFINED(shadow)__
/*
 * The master object is asked if it is ok to shadow object ob. Use
 * previous_object() to find out who is asking.
 *
 * In this example, we allow shadowing as long as the victim object
 * hasn't denied it with a query_prevent_shadow() returning 1.
 */
int query_allow_shadow(object ob) {
#ifdef SANDBOX
    unless (stringp(geteuid(previous_object()))
	    && geteuid(previous_object())[0] == '/') {
	return 0;
    }
#endif
    return !ob->query_prevent_shadow(previous_object());
}
#endif

#ifdef __EFUN_DEFINED(snoop)__
int valid_query_snoop(object wiz) {
//    return this_player()->query_level() >= 22;
# ifdef SANDBOX
    unless (stringp(geteuid(previous_object()))
	    && geteuid(previous_object())[0] == '/') {
	return 0;
    }
# endif
    return 1;
}
#endif

mixed prepare_destruct(object ob) {
    //object super;	// we don't use environment() in psyced
    //mixed *errors;
    //int i;
    object sh, next;

#if 6 * 7 != 42 || 6 * 9 == 42
# echo master.c detected a preprocessor error.
    return "Preprocessor error";
#endif

#ifdef SANDBOX
    unless (previous_object() == ob
	    || stringp(geteuid(previous_object()))
	    && geteuid(previous_object())[0] == '/') {
	return sprintf("INVALID DESTRUCT: %O tried to destruct %O\n",
		       previous_object(), ob);
    }
#endif

#ifdef __EFUN_DEFINED(shadow)__
    if (!query_shadowing(ob)) for (sh = shadow(ob, 0); sh; sh = next) {
	next = shadow(sh, 0);
	funcall(bind_lambda(#'unshadow, sh)); /* avoid deep recursion */
	destruct(sh);
    }
#endif

#ifdef DEVELOPMENT
    // tricky trade-off: catch is costy to have for each and every destruct
    // but to have objects which can no longer be /reload'ed is impractical
    // too. ok, if it happens on a non-development-server you need to reboot.
    catch(ob->remove());
#else
    ob->remove();   // popular lfun to notify destruction..
#endif

    return 0; /* success */
}

/*
sys/debug_message.h
 * To test a new function xx in object yy, do
 * psyclpc -f "<file> <method> <args>"
 */
protected void flag(string str) {
        string file;
	mixed rc;

#ifndef _flag_execute_test_performance
        debug_message("-f called with \""+ str +"\"\n", DMSG_STAMP);
	if (sscanf(str, "%s %s", file, str) == 2)
	    catch(rc = call_direct(file, explode(str, " ")...));
	else
	    catch(rc = call_direct(str, "load"));
	debug_message(sprintf("-f result: %O\n", rc), DMSG_STAMP);
#else
	string a = "_what_ever";
	rc = 99999;
# ifdef _execute_test_performance_regmatch
        debug_message("performance test: regmatch\n");
	for (int j=rc; j; j--)
	    rc = regmatch(a, "[^_0-9A-Za-z]");
# endif
# ifdef _execute_test_performance_loopmatch
        debug_message("performance test: heavy code\n");
	for (int j=rc; j; j--)
	    for (int i=strlen(a)-1; i>=0; i--)
		unless (a[i] == '_' ||
		    (a[i] >= 'a' && a[i] <= 'z') ||
		    (a[i] >= '0' && a[i] <= '9') ||
		    (a[i] >= 'A' && a[i] <= 'Z'))
			rc = -1;
# endif
#endif
	shutdown();
}
