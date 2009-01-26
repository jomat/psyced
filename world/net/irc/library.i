//						vim:noexpandtab:syntax=lpc
// $Id: library.i,v 1.25 2008/08/23 19:07:27 lynx Exp $
//
// this gets included into the system function library
// also known as "simul_efun" in mud-speak.

#define NO_INHERIT
#include "irc.h"	// gets numeric codes
#include "error.h"	// gets numeric codes
#include "reply.h"	// gets numeric codes

// one global method mapping
volatile mapping p2i;

string psyc2irc(string mc, mixed source) {
	mixed c;
	// looks like this should entirely evaporate into textdb...
	unless (p2i) p2i = ([	// sorted by psyc method
"_list_places_members"		: RPL_NAMREPLY,
	      // asking for a prompt is an error in irc protocol
"_query_password"		: ERR_PASSWDMISMATCH,
"_status_place_topic"		: RPL_TOPIC,
//"_error_necessary_membership"	: ERR_NOSUCHCHANNEL,
"_error_unavailable_nick_place"	: ERR_BANNEDFROMCHAN, // pretty close
//"_status_place_members"	: RPL_NAMREPLY,

// not accurate, because it is the /motto command
//"_echo_set_description"		: RPL_NOWAWAY,
//"_echo_set_description_none"          : RPL_UNAWAY,
// not accurate, because it only reflects the existence of a text msg
//"_status_presence_description"	: RPL_NOWAWAY,
//"_status_presence"            	: RPL_UNAWAY,
// and what's worse, it's not working.. let's try something else..
// i'll just add these messages to /irchere and /ircgone
"_echo_set_description_away"            : RPL_NOWAWAY,
"_echo_set_description_away_none"       : RPL_UNAWAY,
	]);
	if (c = p2i[mc]) return SERVER_SOURCE + c;
//	if (abbrev("_notice_place_enter", mc)) c = "JOIN";
//	else if (abbrev("_notice_place_leave", mc)) c = "PART";
#ifdef _flag_enable_notice_from_source_IRC
        // the code below will no longer generate server notices for remote
        // sources - fippo likes it better that way 
//	if (!source) source = SERVER_SOURCE;
	if (!source || objectp(source)) source = SERVER_SOURCE;
	// why aren't we trying to display something useful for an object here?
	//
        // i bet irssi wont like the !*@* (the weird problem we had with it
        // about artifical nickname changes for same ident...)
        //else source = ":" + source + "!*@* ";
	else source = ":" + source + " "; 
	return source +"NOTICE";
#else  
        // unfortunately, "pidgin" displays the non-server notices in an
        // very annoying manner - one new window for each friend...
	// will have to postpone this change until pidgin is fixed
	return SERVER_SOURCE +"NOTICE";
#endif
}

