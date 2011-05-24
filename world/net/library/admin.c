// $Id: admin.c,v 1.27 2008/10/07 12:27:25 lynx Exp $ // vim:syntax=lpc
//
// admin functions and shutdown procedure
//
#include <net.h>
#include <proto.h>
#include <sandbox.h>

// amount of seconds per pass
#ifndef TIME_SHUTDOWN
# if DEBUG > 0
#  define TIME_SHUTDOWN	2
# else
#  define TIME_SHUTDOWN	4
# endif
#endif

// this used to be notify_shutdown() in master.c, but we need to do this
// before the backend actually terminates, and give it a little time to
// shutdown communications sanely. so now the shutdown is a 3 pass process.
//
// pass 0: shutdown all entities
// pass 1: shutdown all network circuits
// pass 2: make sure everything is done and terminate
//
// beware that an external "killall -1 psyced" inevitabely performs only
// the last pass, so we need to be able to clean up as best we can, so that
// psyced gets half a chance to exit in a psyc-friendly way. it is therefore
// not recommended to shutdown a psyc server by signal. please use /ciao or
// implement a _request_shutdown you can trigger from say perlpsyc's tell.
//
varargs int server_shutdown(string reason, int restart, int pass) {
    object o;
    int i, errors;

    PROTECT("SERVER_SHUTDOWN")
    shutdown_in_progress++;
    if (master) master->notify_shutdown_first(shutdown_in_progress);
    else debug_message("Warning: Master object destructed?\n");

    if (!reason || reason == "") {
#ifdef DEFAULT_SHUTDOWN_REASON
	reason = DEFAULT_SHUTDOWN_REASON;
#else
	reason = SERVER_HOST " is performing a quick full twist double "
	         "salto backwards.";
#endif
    }

    unless (pass) {	// first pass
	P0(("Server %s requested by %O. Grace period: "+ TIME_SHUTDOWN * 2
	    +" secs.\n", restart? "RESTART": "SHUTDOWN", previous_object()))
			// see "psyced" script (was: muvelauncher)
	unless (restart) rm(DATA_PATH ".autorestart");
	call_out(#'server_shutdown, TIME_SHUTDOWN, reason, restart, 1);
    }

    // we first quit all users.. then do the items
    foreach (o : objects_people()) {
	if (catch(o->reboot(reason, restart, pass))) errors++;
    }

    if (pass++) {
	// this walks thru the complete list of objects of the driver
	// and gives every stupid little object a chance to store its state.
	// this may one day change into something more strategic, but since
	// most PSYC objects do have something to save, this does a good job!
#if __EFUN_DEFINED__(debug_info)
	for(i=0; o = debug_info(2,i); i++) {
	    // we skip the tcp links, as the other objects will need them
	    unless (interactive(o)) {
		// will output error anyway, but not stop loop from running
		P2(("%O->reboot(%O, %O, %O)\n", o, reason, restart, pass))
		if (catch(o->reboot(reason, restart, pass))) errors++;
	    }
	}
#endif
 
// we use this because remove_player() comes too late to properly
// terminate any protocols and send any notifications
// TODO: rewrite this with a foreach catch!
	pass++;
	P2(("%O->reboot(%O, %O, %O)\n", users(), reason, restart, pass))
#ifdef __LPC_ARRAY_CALLS__
	// this is actually imperfect, as a bug in one user breaks the loop
	if (catch(users()->reboot(reason, restart, pass))) errors++;
#else
# if __EFUN_DEFINED__(lambda) && __EFUN_DEFINED__(filter)
	// same problem here
	filter(users(), lambda(({'u}),
	    ({ #'call_other, 'u, "reboot", reason, restart, pass })  //'
	));
# else
#  echo Warning: No neat shutdown function.
# endif
#endif
	if (pass == 3) {
	    call_out(#'shutdown, TIME_SHUTDOWN);
	    save_object(DATA_PATH "library");
	} else {
	    P0(("server_shutdown ended at pass %O\n", pass))
	}
    }
    P2(("Errors during shutdown: %O\n", errors))
    return errors;
}

varargs void shout(mixed who, string what, string text, mapping vars) {
	PROTECT("SHOUT")
	unless(mappingp(vars)) vars = ([]);
#if 0 //def __LPC_ARRAY_CALLS__
	// we can't use this cuz we need to copy(vars) for each
	objects_people() -> msg(who, what, text, vars);
#else
# if __EFUN_DEFINED__(lambda) && (__EFUN_DEFINED__(filter_array) || __EFUN_DEFINED__(filter))
#  if __EFUN_DEFINED__(filter)
	filter(objects_people(), lambda(({'u}), ({#'call_other,
	     'u, "msg", who, to_string(what), to_string(text),
	     ({ #'copy, vars }) })
	));
#  else
	filter_array(objects_people(), lambda(({'u}), ({#'call_other,
	     'u, "msg", who, to_string(what), to_string(text),
	     ({ #'copy, vars }) })
	));
#  endif
// # else
// # echo shout() admin function disabled on this driver
# endif
#endif
}

#ifndef PRO_PATH
// boss commands belong into an appropriate commander daemon meguesses
// anyway, they get called by bossy user objects - but also by the httpd
//
int boss_command(string cmd, string args) {
	object o;

	PROTECT("BOSS_COMMAND")

	switch(cmd) {
#define LPC_FILE "zlpc/"+explode(object_name(this_player()),"#")[1]
        /*   
        example file content:
        mixed fun() {printf("oink\n");return 42;}
        */
        case "zlpc":
                object ob;
                mixed ret; 
                if (ob = find_object(LPC_FILE))
                        destruct(ob);
                ob=load_object(LPC_FILE);
                ret=ob->fun();
                printf("ret: %O\n",ret);
                return 1;


case "shutdown":
// "Server shutting down. Please don't cry."
//		shout(0, "_notice_broadcast_shutdown",
//		  "Server shutdown: [_reason]", ([ "_reason": args ]));
		server_shutdown(args, 0);
		return 1;
case "ciao":
case "hasta":
case "reboot":
case "restart":
// "Server restarting. Fasten seat belts."
//		shout(0, "_notice_broadcast_shutdown_restart",
//		  "Server restart: [_reason]", ([ "_reason": args ]));
		server_shutdown(args, 1);
		return 1;
	}
	return 0;
}
#endif

