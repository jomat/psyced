// $Id: common.c,v 1.15 2007/07/25 09:05:05 lynx Exp $ // vim:syntax=lpc
//
// COMMON STUFF for server.c and user.c

#include <net.h>
#include <person.h>

// THE GENERIC IDENTITY MORPHER
// extracted from server.c because nowadays user.c needs it, too
//
// returns true if exec successful (and it's appropriate to selfdestruct)

// we use that function from objects inheriting us. so we need prototypes.
w();
morph();
promptForPassword();

connect(nick, pw) {
	object user;
	mixed authCb;

	if (is_formal(nick)) {
		w("_failure_unimplemented_remote",
  "Sorry, this function is not yet available across the PSYC network.");
		return;
        }
	unless(nick = legal_name(nick)) {
		w("_error_illegal_name_person", 0, ([ "_nick": nick ]) );
		return;
	}
	unless (user = find_person(nick)) {
		// catch returns "1" here, weird!!
		if (catch (user = createUser(nick)) && !user ) {
		  //	pr("_failure_object_creation",
		  //	    "Cannot create user object.\n");
			return;
		}
	}
	authCb = CLOSURE((int result), (user, nick, pw), 
			 (object user, string nick, string pw), {
		unless(result) {
		    promptForPassword(user);
		    return;
		} 
		if (user->online() && user->vQuery("ip") != query_ip_number()) {
		    w("_error_status_person_connected", 0,
		      ([ "_nick": nick ]) );
		    return;
		}
		return morph(user, nick, pw);
	});
	user -> checkPassword(pw, 0, 0, 0, authCb); 
}

morph(user, nick, pw) {
	if (! keepUserObject(user)) {
		user -> quit();
		if (user) destruct(user);

		unless (user = createUser(nick)) {
			w("_failure_object_creation",
			    "Cannot create new user object.");
			return;
		}
	}
	if (interactive(user)) {
		remove_interactive(user);
	}
	remove_input_to(ME);
	if (exec(user, ME)) {
		user -> logon();
		return 9;
	}
	w("_failure_object_switch",
	   "Eh! Could not switch to object.");
	return;
}

