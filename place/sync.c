#include <net.h>

// you shouldn't let your users be able to enter this and
// receive synchronization reports. you have to restrict entry here.

#ifndef ADMINISTRATORS
# ifdef CONFIG_PATH
#  include CONFIG_PATH "admins.h"
# endif
#endif

#ifdef ADMINISTRATORS
# define PLACE_OWNED ADMINISTRATORS
# define RESTRICTED
#else
# echo Warning: Sync room is not restricted.
#endif

#define PRIVATE
#define NICKLESS
#define TRUSTED
#define ALLOW_EXTERNAL_LOCALS	// this allows /m from ircers.. todo
				// but _message is irrelevant anyway here

// history triggers a recursion. why i can't tell.
//#define PLACE_HISTORY
//#define HISTORY_METHOD  "_notice_synchronize"

#include <place.gen>

#ifdef _flag_enable_request_list_user_registered

msg(source, mc, data, mapping vars) {
    P2(("%O got %O from %O\n", ME, mc, source))
    switch(mc) {
    case "_request_list_user_registered":
	if (isMember(source) || vars["_INTERNAL_trust"] > 5) {
	    // TODO: i think we need an api for "all registered users"
	    // which could also do caching
	    mixed files = map(get_dir(DATA_PATH "person/", 0x01), 
			      (: return $1[..<3]; :));
	    mapping rv = ([ ]);
	    if (vars["_tag"]) rv["_tag_reply"] = vars["_tag"];
	    rv["_list_user"] = files;
	    sendmsg(source, "_notice_list_user_registered", 0, rv);
	} else {
	    P0(("%O not trusted to do request list user, trust is %O\n", 
		source, vars["_INTERNAL_trust"]))
	}
	break;
    default:
	return ::msg(source, mc, data, vars);
    }
}

#endif

