// $Id: user.c,v 1.46 2008/12/01 11:31:33 lynx Exp $ // vim:syntax=lpc
//
// telnet roxx, still  ;)
//
#include <net.h>
#include <user.h>
#include <status.h>

input(a, dest) {
	next_input_to(#'input);
	if (!a || a=="") {
		unless (v("clearscreen") == "off") cat(TELNET_PATH "clear.vt");
		showStatus(VERBOSITY_STATUS_AUTOMATIC);
	} else {
		::input(a, dest);
	}
	prompt();
	return 1;
}

logon(failure) {
	// bei failure und fippo meint auch bei skurrilen tls zuständen
	if (this_interactive()) {
		set_prompt("");
		cat(TELNET_PATH "clear.vt");
	}
	vSet("scheme", "tn");
	vDel("layout");
	vDel("agent");
	::logon();
	// sTextPath(0, "de", "tn");

	next_input_to(#'input);
	prompt();
}

prompt() {
	// should we want to iconv the prompt,
	// then it is a good idea to cache it..
	//
	// we should get _notice's or signaling every time the
	// current place or query change, so we update the prompt.. TODO
#if 0
	if (ME && find_call_out(#'quit) == -1)
	    binary_message("\n"+( v("query") ? v("query")+" ~> " :
		( place ? v("place")+" @> " : CHATNAME " <> " )));
#else
	if (ME && find_call_out(#'quit) == -1) {
		string p = v("query");
		if (p)
		    p = "\n"+ p +" ~> ";
		else if (place && stringp(place))
		    return;
		else if (p = v("place"))
		    p = "\n"+ p +" @> ";
		else
		    p = "\n" CHATNAME " <> ";
		binary_message(p);
	}
#endif
}

w(string mc, string data, mapping vars, mixed source) {
	if (abbrev("_request_attention", mc)) vars["_beep"] = " ";
	return ::w(mc, data, vars, source);
}

// inherit output functions per protocol?
emit(message) {
#ifdef LETS_NOT_USE_TELL_OBJECT
	int l, rc;
#endif
	unless (strlen(message)) {
	    PT(("%O got empty emit call in tn/user\n", ME))
#if DEBUG > 1
	    tell_object(ME, "**EMPTY**");
	    raise_error("empty");
#endif
	    return;
	}
#if __EFUN_DEFINED__(convert_charset)
	if (v("charset") && v("charset") != SYSTEM_CHARSET) {
	    // this breaks when it encounters an old log or history which
	    // is already/still in the target charset thus not utf8. waah!
	    P3(("telnet»%s: %s\n", v("charset"), message || "(null!?)"))
	    iconv(message, SYSTEM_CHARSET, v("charset"));
	}
#endif
#ifdef LETS_NOT_USE_TELL_OBJECT
# echo using binary_message for telnet
	// this solution does NOT work when viewing /history with
	// prefixes. should we ever want to use this code again,
	// we need to do a regreplace from \n to \r\n
	message += "\r";
	l = strlen(message);
		// iso-latin-1 output with amylaar's gd
	rc = binary_message(message);
	D1( if (rc != l && rc != -1)
	     D(S("emit: %O of %O returned for %O from %O.\n",
			 rc, l, MYNICK, v("host"))); )
	return rc == l;
#else
# ifdef NEW_LINE
	message += "\n";
# endif
	tell_object(ME, message);
#endif
}

errorParse(s) {
#ifdef TN_FIX
    if (!s || s == "") {
#endif
	object o;
	exec(o = clone_object(TELNET_PATH "server"), ME);
	o->logon();
#ifdef TN_FIX
    } else if (s == "fixit") {
	next_input_to(#'input);
	prompt();
    } else {
	internalError();
    }
#endif
}

internalError() {
    emit(">> BROKEN PARSER: This server will not accept anything you send "
	  "unless you relogin. You can, however, press enter and then relogin "
	  "without reconnecting.\n"
#ifdef TN_FIX
	   " Also, you can order me to repair things by sending 'fixit' "
	   "(without ticks!)\n"
#endif
	  );
    next_input_to(#'errorParse);
}


