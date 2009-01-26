// $Id: test.c,v 1.34 2008/01/26 12:02:07 lynx Exp $ // vim:syntax=lpc
//
// room to test some library functions etc
// originally started by heldensaga
//

// ON_COMMAND is good here, because in the meantime the cmd() API
// has been extended by the vars argument, so with the previous
// style of overloading cmd() you just killed basic functions
// of the room like /topic
#define ON_COMMAND	if (mycmd(command, args, source)) return 1;
// same goes for msg() and ON_ANY
#define ON_ANY		if (mymsg(source, mc, data, vars)) return 1;

#define ALLOW_EXTERNAL
#define PLACE_SCRATCHPAD
//#define MODERATED
//#define PRIVATE
#define NAME   "TEST"
//#define LINK SERVER_HOST

//#define WHAT "_version"
#define WHAT "_description"

#include <net.h>

#ifdef HTMORE
inherit NET_PATH "outputb";
// order is relevant. outputb needs to be inherited BEFORE textc!
#include <text.h>
#endif

#include <place.gen>

mymsg(source, mc, data, vars) {
	PT(("Got %O in %O from %O\n", mc, ME, vars))
	if (mc == "_request_test") {
		unless (source) source = vars["_INTERNAL_origin"];
//		sendmsg(source, "_status_protocol_test_escaping_fancy",
//		    "This message is a test. Please ignore.", ([
//			 "_fancy": "\n\t\n\t\n\t\n\t\t\t\n\n\n"
//		]) );
		sendmsg(source, "_status_protocol_test_trippy_body"
			"_can_your_parser_handle_complete_silly_methods_as_it"
			"_should_and_treat_this_just_like_any_status_packet",
		    "This is a protocol test.\n"
		    "Notice the space after the dot:\n"
		    ". \n"
    ":_source\txmpp:bill@mikerawsoft.com\n"
    ":_action\tIf you think this is an action..\n"
    "_message_fake\n"
    "... then your parser got it wrong, because all of what you see\n"
    "here is just a multiline message body which legally happens\n"
    "to look like a PSYC packet itself. Dommage!",
			([ "_test": "I am a test variable" ]) );
		return 1;
	}
	return 0;
}

mycmd(a, args, source) {
    unless (source) source = previous_object();	// really needed???

    switch (a) {
#ifdef EXPERIMENTAL
case "tdb":
	write_file("/default/out.textdb",
		   "default/en/plain/text"->renderDB());
	return 1;
#endif
case "ba":
case "bas":
case "base":
case "base64":
	if (sizeof(args) > 1) {
	    return base64(0, ARGS(1), source);
	} else {
	    sendmsg(source, "_warning_usage_base64", "Usage: /base64 <text>. See also /unbase64.");
	    return 1;
	}
case "unba":
case "unbase64":
	if (sizeof(args) > 1) {
	    return base64(1, ARGS(1), source);
	} else {
	    sendmsg(source, "_warning_usage_unbase64", "Usage: /unbase64 <code>");
	    return 1;
	}
case "ro":
case "rot":
case "rot13":
	if (sizeof(args) > 1) {
	    return rot13(ARGS(1), source);
	} else {
	    sendmsg(source, "_warning_usage_rot13", "Usage: /rot13 <text>");
	    return 1;
	}
#if __EFUN_DEFINED__(tls_refresh_certificates)
// just a test. should be elsewhere.
case "recerts":
	tls_refresh_certificates();
	return 1;
#endif
case "dr":
	if (sizeof(args) > 1) {
	    dns_resolve(args[1], lambda(
		({ 'name }),
		({ #'sendmsg, source, "_status_dns", ({ #'+, args[1] + " has been resolved to ", 'name })
		}) ));
	    return 1;
	}
    }
}

base64(decode, text, source) {
    int value, *binary, i;
    string how;
		// how does this behave with binary data? we'll see
    if (decode) {
	text = to_string(decode_base64(text));
	how = "decoded";
    } else if (value = to_int(text)) {
	i=0; binary = allocate(4+ strlen(text));
	// wrong byte order, but it's just a test anyway
	while (binary[i++] = value % 256) value = value / 256;
	binary = binary[.. i-2];
	PT(("binary for %O is %O. i is %O\n", value, binary, i))
	text = encode_base64(binary);
	how = "encoded_integer";
    } else {
	text = encode_base64(to_array(text));
	how = "encoded";
    }
    sendmsg(source, "_notice_service_base64_"+ how,
	    "base64 "+how+": [_result]", ([ "_result" : text ]) );
    return 1;
}

rot13(text, source) {
    int i, length;
    mapping codetable;

    codetable = ([ // we'll need this for (de)crypting
		 "a" : "n",
		 "A" : "N",
		 "b" : "o",
		 "B" : "O",
		 "c" : "p",
		 "C" : "P",
		 "d" : "q",
		 "D" : "Q",
		 "e" : "r",
		 "E" : "R",
		 "f" : "s",
		 "F" : "S",
		 "g" : "t",
		 "G" : "T",
		 "h" : "u",
		 "H" : "U",
		 "i" : "v",
		 "I" : "V",
		 "j" : "w",
		 "J" : "W",
		 "k" : "x",
		 "K" : "X",
		 "l" : "y",
		 "L" : "Y",
		 "m" : "z",
		 "M" : "Z",
		 "n" : "a",
		 "N" : "A",
		 "o" : "b",
		 "O" : "B",
		 "p" : "c",
		 "P" : "C",
		 "q" : "d",
		 "Q" : "D",
		 "r" : "e",
		 "R" : "E",
		 "s" : "f",
		 "S" : "F",
		 "t" : "g",
		 "T" : "G",
		 "u" : "h",
		 "U" : "H",
		 "v" : "i",
		 "V" : "I",
		 "w" : "j",
		 "W" : "J",
		 "x" : "k",
		 "X" : "K",
		 "y" : "l",
		 "Y" : "L",
		 "z" : "m",
		 "Z" : "M"
			 ]);

    length = strlen(text);
    for (i = 0; i <= length; i++) {
	if (codetable[text[i..i]]) {
	    text[i..i] = codetable[text[i..i]];
	}
    }

    sendmsg(source, "_notice_service_rot13",
	    "rot13 result: [_result]", ([ "_result" : text ]) );

    return 1;
}

#ifdef HTMORE
// tutorial for buffered output for asynchronous http operations
htget(prot, query, headers, qs) {
	object httpd = previous_object();
	string target = query["target"];

	if (query["scratchpad"]) return ::htget(prot, query, headers, qs);

	localize(query["lang"], "ht");
	// we could be doing htok() here and leave out httpd->http_ok() later
	unless (stringp(target) && is_formal(target)) {
		htok();
		httpd -> emit("<p>please provide a 'target'</p>\n");
		return 1;
	}
	sendmsg(target, "_request" WHAT, 0, ([ ]), 0, 0, (: 
		// start the buffering action
		init_buffer("");

		// gets ignored if htok() already ran earlier
		//httpd->http_ok(prot, "text/x-html", "");
		httpd->http_ok(prot);
		// htok(prot); won't work as it is called in this_interactive,
		// thus in the socket where the _status_version came from

		// regular w() feeding our buffering emit
		w("_PAGES_place_test", "<title>[_nick]</title>\n"
		    "<body bgcolor='yellow' text='black'>\n",
		    ([ "_nick": MYNICK ]));

		// buffered write().. but don't use this.. please port all
		// your write()'s to emit() as any object->call() containing
		// write()'s will result in the network socket (xmpp or psyc)
		// receving those writes instead of the web browser!!
		// write() only works here because we are still in our own
		// object.
		write("<p>_request" WHAT " result from "+ target +"</p>\n");

		// buffered emit. $4 is the vars.
		emit(sprintf(
		    "<div style='background: white;"
		    " margin: 44; padding: 44'>%O</div>\n", $4));

		// now expel everything to the web browser
		flush_buffer(httpd);

		// and close the socket
		return httpd -> done();
	:));
	return HTMORE;
}
#endif
