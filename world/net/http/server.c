// $Id: server.c,v 1.64 2008/05/13 09:51:07 lynx Exp $ // vim:syntax=lpc
//
// yes, psyced is also a web server, like every decent piece of code.  ;)
//
#include <ht/http.h>
#include <net.h>
#include <text.h>

#include "header.i"

volatile string url, qs, prot, method, body;
volatile mixed item;
volatile mapping headers;
volatile int length;

qScheme() { return "html"; }

quit() {
    D2(D("««« HTTP done.\n");)
    destruct(ME);
}
// gets called from async apps
done() { quit(); }

// this could be improved to implement HTTP/1.1
parse_nothing(input) {
	P2(("=== HTTP ignored %O from %s\n", input, query_ip_name(ME)))
	next_input_to(#'parse_nothing);
}
parse_body_length(input) {
    //P4(("parse_body_length(%O)\n", input))
    body += input;
    if (strlen(body) >= length) {
	    process();
	    next_input_to(#'parse_nothing);
    } else
	input_to(#'parse_body_length, INPUT_IGNORE_BANG
				     | INPUT_CHARMODE | INPUT_NO_TELNET);
}
parse_body_url(input) {
	qs = input;
	if (method == "post") method = "get"; // call htget() anyway
	process();
	next_input_to(#'parse_nothing);
}
parse_body_raw(input) {
	body += input;
	next_input_to(#'parse_body_raw);
	// this loop terminates with TCP disconnected()
}
disconnected(remainder) {
        D2(D("««« HTTP got disconnected.\n");)
	if (stringp(remainder)) {
		body += remainder;
		process();
		call_out(#'quit, 333);
	} else quit();
        return 1;   // expected death of socket
}
timeout() {
       	// try using incomplete post
	if (stringp(body) && strlen(body)) process();
	quit();
}

parse_wait(null) { // waiting to send my error message here
	if (null == "") {
		http_error("HTTP/1.0", 405,
			   "Invalid Request (Hello Proxyscanner)");
		quit();
	}
	// why wait? we can throw the message on the socket and kill it
	next_input_to(#'parse_wait);
}

parse_header(input) {
	if (input != "") {
		string name, contents;

		// %.0t = catch zero to endless whitespace characters
		sscanf(input, "%s:%1.0t%s", name, contents);
		if (contents) {
			P3(("headers[%O] = %O\n",name,contents))
			headers[lower_case(name)] = contents;
		} else {
	//              http_error(prot, R_BADREQUEST,
	//                      "invalid header '"+ input +"'");
	//              QUIT; return 1;
			P1(("Invalid HTTP header %O from %s\n",
			    input, query_ip_name(ME)))

		}
		next_input_to(#'parse_header);
		return;
	}
#if 0
	if (method == "post" && (length = to_int(headers["content-length"])) &&
	    headers["content-type"] == "application/x-www-form-urlencoded")
#else
	if (length = to_int(headers["content-length"]))
#endif
	{
		input_to(#'parse_body_length,
		  INPUT_IGNORE_BANG | INPUT_CHARMODE | INPUT_NO_TELNET);
	} else if (headers["content-type"] ==
		   "application/x-www-form-urlencoded") {
		next_input_to(#'parse_body_url);
	} else if (method == "post" || method == "put") {
		next_input_to(#'parse_body_raw);
	} else {
		process();
	}
}

parse_request(input) {
	P2(("=== HTTP got: %O from %s\n", input, query_ip_name(ME)))

	// reset state. in case we support HTTP/1.1. do we?
        method = item = url = prot = qs = 0;
        headers = ([]);
	body = "";

        if (!input || input=="") {
                // should return error?
                input_to(#'parse_request);
	    	// lets just ignore the empty line
                return 1;
        }
	input = explode(input, " ");
	switch(sizeof(input)) {
	default:
		prot = input[2];
		next_input_to(#'parse_header);
	case 2:
		// earlier HTTP versions have no headers
		// ok, it's excentric to support HTTP/0.9 but
		// it is practical for debugging and doesn't
		// cost us a lot extra effort
		url = input[1];
		unless (sscanf(url, "%s?%s", item, qs)) item = url;
		method = lower_case(input[0]);
		break;
	case 1:
                // should return error!
		quit();
	}
	P4(("=== HTTP user requested url: %O\n", url))
	if (method == "connect") next_input_to(#'parse_wait);
	else if (!prot) body();	// HTTP/0.9 has no headers
	else next_input_to(#'parse_header);
}

process() {
    string t, ext;
    mapping query = ([]);
    object o;
    int done = 1;

    // take defaults from cookie, then override by query string
    // lynXism cookie behaviour, normal one is below
    t = headers["cookie"];
    P4(("found cookie: %O\n", t))
    if (t && sscanf(t, "psyced=\"%s\"", t)) {
	P3(("got cookie: %O\n", t))
	query = url_parse_query(query, t);
	P4(("parsed cookie: %O\n", query))
    }
#ifdef GENERIC_COOKIES	// we might need them someday..?
    // if within the same domain other cookies are being used, like
    // by including google-analytics, then we might be receiving them
    // here and have no friggin' idea what they are good for.
    // thus: we *need* a way to ensure a cookie is our own.
    // FIXME: this is not really compliant 
    else if (t) {
	mapping cook = ([ ]);
	string k, v;
	while(t && sscanf(t, "%s=%s;%t%s", k, v, t) >= 2) {
	    cook[k] = v;
	}
	if (sscanf(t, "%s=%s", k, v))
	    cook[lower_case(k)] = v; // case insensitive
	cook[0] = headers["cookie"]; // save cookie-string
	headers["cookie"] = cook;
    }
#endif
    if (qs) {
	P3(("got query: %O\n", qs))
	query = url_parse_query(query, qs);
    }
    if (method == "post" && headers["content-type"] == "application/x-www-form-urlencoded") {
	query = url_parse_query(query, body);
    }
    P4(("parsed query: %O\n", query))
    switch (item) {
case "/favicon.ico":
#if 0
	htredirect(prot, "http://www.psyced.org/favicon.ico",
	    "This one looks neat", 1);
	quit();
	return 1;
#else
	item = "/static/favicon.ico";
	break;
#endif
case "/":
case "":
	// should we look for text/wml in the accept: and go directly
	// to /net/wap/index ?
	//
	http_ok(prot);
	sTextPath(0, query["lang"], "html");
	write(  //T("_HTML_head", "<title>" CHATNAME "</title><body>\n"
		//		"<center><table width=404><tr><td>") +
	      T("_PAGES_index", "<i><a href=\"http://www.psyc.eu\"><img src="
		"\"static/psyc.gif\" width=464 height=93 border=0></a><p>"
		"<a href=\"http://www.psyced.org\">psyced</a></i> -"
	        " your multicast capable web application server.") );
	      // T("_HTML_tail", "</td></tr></table></center></body>"));
	quit();
	return 1;
case "/static": // really don't like to do this, but the IE stores directories
		// (history) without trailing slash, even if the url originaly
		// has one, at least IIRC.
	htredirect(prot, "/static/", "use the trailing slash", 1);
	quit();
	return 1;
case "/static/":
	item = "/static/index.html";
	break;
case "/oauth":
	object oauth;
	http_ok(prot);
	//PT((">>> looking up token %O in shm: %O\n", query["oauth_token"], shared_memory("oauth_request_tokens")))
	if (oauth = shared_memory("oauth_request_tokens")[query["oauth_token"]]) {
	    //PT((">>> oauth: %O\n", oauth))
	    oauth->verified(query["oauth_verifier"]);
	    m_delete(shared_memory("oauth_request_tokens"), query["oauth_token"]);
	    write("OAuth succeeded, you can now return to your client.");
	} else {
	    write("OAuth failed: token not found");
	}
	quit();
	return 1;
    }
    string name;
    switch (item[1]) {
    case '~':
	    string channel, nick = item[2..];
	    if (sscanf(item, "/~%s/%s", nick, channel)) {
		name = "~" + nick + "#" + channel;
	    } else if (o = summon_person(nick, NET_PATH "user")) {
		o->htinfo(prot, query, headers, qs, channel);
		quit();
		return 1;
	    }
	    //fall thru
    case '@':
	    unless(name) name = item[2..];
	    o = find_place(name);
	    break;
    default:
	    if (abbrev("/static/", item)) {
		if (file_size(item) > 0) {
		    if (sscanf(item, "%!s.%s", ext)) {
			while (sscanf(ext, "%!s.%s", ext)) ;
		    }
		    http_ok(prot, content_type(ext), 0);
		    binary_message(read_file(item));
		    quit();
		    return 1;
		}
	    } else if (sscanf(item, "/%s/%s.page", ext, t) == 2) {
		http_ok(prot);
		sTextPath(0, query["lang"] || ext, "html");
		t = replace(t, "/", "_");
		write(T("_HTML_head", "<title>" CHATNAME "</title><body>\n"
					"<center><table width=404><tr><td>") +
		      T("_PAGES_"+t, "[no such page]\n") +
		      T("_HTML_tail", "</td></tr></table></center></body>"));
		quit();
		return 1;
	    }
    }

    if (index(item, ':') != -1) {
	http_error(prot, 501, "Not Implemented. Whatever you are trying "
		   "there, this server won't help you.");
	quit();
	return;
    }

    unless (o) o = item -> load();
    if (objectp(o) || o = find_object(item))
	done = o->htget(prot, query, headers, qs) != HTMORE;

    if (done)
	quit();
    else
	remove_call_out(#'timeout);
    return 1;
}

emit(a) { return binary_message(a); }

logon() {
    D2(D("»»» HTTP request:\n");)

    // bigger buffer (for psyc logo)
    set_buffer_size(32768);
    // unfortunately limited to a compilation limit
    // so we would have to push large files in chunks
    // using heart_beat() or something like that	TODO
    
    next_input_to(#'parse_request);
    call_out(#'timeout, 23);
}

