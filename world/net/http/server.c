// $Id: server.c,v 1.64 2008/05/13 09:51:07 lynx Exp $ // vim:syntax=lpc
//
// yes, psyced is also a web server, like every decent piece of code.  ;)
//
#include <ht/http.h>
#include <net.h>
#include <server.h>
#include <text.h>

#include "header.i"

volatile string url, file, qs, version, method, body = "";
volatile mapping headers;
volatile int length;

// we're using #'closures to point to the functions we're giving the
// next_input_to(). as i don't want to restructure the whole file, i need
// to predefine some functions.
//
// quite stupid indeed, as they don't got any modifiers or whatever :)
parse_url(input);
parse_header(input);
parse_body(input);
devNull();

qScheme() { return "html"; }

logon() {
    D2(D("»»» New SmallHTTP user\n");)
    if(query_mud_port() == HTTPS_PORT && !tls_query_connection_info(this_object()))
    {   
        emit(
          "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">"
          "<html><head>"
          "<title>400 Bad Request</title>"
          "</head><body>"
          "<h1>Bad Request</h1>"
          "<p>Your browser sent a request that this server could not understand.<br />"
          "Reason: You're speaking plain HTTP to an SSL-enabled server port.<br />"
          "Instead use the HTTPS scheme to access this URL, please.<br />"
          "</body></html>"
        );
        QUIT
        return;
    }

    // bigger buffer (for psyc logo)
    set_buffer_size(32768);
    // unfortunately limited to a compilation limit
    // so we would have to push large files in chunks
    // using heart_beat() or something like that	TODO
    
    next_input_to(#'parse_url);
    call_out(#'quit, TIME_LOGIN_IDLE);
}

disconnected(remainder) {
        // TODO: shouldn't ignore remainder
        D2(D("««« SmallHTTP got disconnected.\n");)
        destruct(ME);
        return 1;   // expected death of socket
}

// gets called from async apps
done() { quit(); }

#if DEBUG > 1
quit() {
    D2(D("««« SmallHTTP user done.\n");)
    ::quit();
}
#endif

void create() {
    if (clonep(ME)) headers = ([ ]);
}

parse_wait(null) { // waiting to send my error message here
    if (null == "") {
	http_error("HTTP/1.0", 405, "Invalid Request (Welcome Proxyscanner)");
	quit();
    }
    next_input_to(#'parse_wait);
}

parse_url(input) {
    P3(("=== SmallHTTP got: %O\n", input))
    unless (sscanf(input, "%s%t%s%tHTTP/%s", method, url, version)) quit();
    switch (method) {
	case "CONNECT":
	    next_input_to(#'parse_wait);
	    return;
	case "GET":
	case "POST":
	    break;
	default:
	    quit();
	    return;
    }

    version = "HTTP/" + version;

    P2(("=== SmallHTTP user requested url: %O\n", url))
    next_input_to(#'parse_header);
}

parse_header(input) {
    string key, val;

    P4(("parse_header(%O)\n", input))
    
    unless (input == "") {
	if (sscanf(input, "%s:%1.0t%s", key, val)) {
	    headers[lower_case(key)] = val;
	}

	next_input_to(#'parse_header);
    } else {
	if (method == "POST" && (length = to_int(headers["content-length"])) &&
	    headers["content-type"] == "application/x-www-form-urlencoded") {
	    input_to(#'parse_body, INPUT_IGNORE_BANG | INPUT_CHARMODE | INPUT_NO_TELNET);
	} else {
	    process();
	    next_input_to(#'devNull);
	}
    }
}

parse_body(input) {
    //P4(("parse_body(%O)\n", input))
    body += input;
    if (strlen(body) == length)
	process();
    else
	input_to(#'parse_body, INPUT_IGNORE_BANG | INPUT_CHARMODE | INPUT_NO_TELNET);
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
    if (sscanf(url, "%s?%s", file, qs)) {
	P3(("got query: %O\n", qs))
	query = url_parse_query(query, qs);
    } else {
	file = url;
    }
    if (method == "POST" && headers["content-type"] == "application/x-www-form-urlencoded") {
	query = url_parse_query(query, body);
    }
    P4(("parsed query: %O\n", query))
    switch (file) {
case "/favicon.ico":
#if 0
	htredirect(version, "http://www.psyced.org/favicon.ico",
	    "This one looks neat", 1);
	quit();
	return 1;
#else
	file = "/static/favicon.ico";
	break;
#endif
case "/":
case "":
	// should we look for text/wml in the accept: and go directly
	// to /net/wap/index ?
	//
	http_ok(version);
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
	htredirect(version, "/static/", "use the trailing slash", 1);
	quit();
	return 1;
case "/static/":
	file = "/static/index.html";
	break;
case "/oauth":
	object oauth;
	http_ok(version);
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
    switch (file[1]) {
    case '~':
	    string channel, nick = file[2..];
	    if (sscanf(file, "/~%s/%s", nick, channel)) {
		name = "~" + nick + "#" + channel;
	    } else if (o = summon_person(nick, NET_PATH "user")) {
		o->htinfo(version, query, headers, qs, channel);
		quit();
		return 1;
	    }
	    //fall thru
    case '@':
	    unless(name) name = file[2..];
	    o = find_place(name);
	    break;
    default:
	    if (abbrev("/static/", file)) {
		if (file_size(file) > 0) {
		    if (sscanf(file, "%!s.%s", ext)) {
			while (sscanf(ext, "%!s.%s", ext)) ;
		    }
		    http_ok(version, content_type(ext), 0);
		    binary_message(read_file(file));
		    quit();
		    return 1;
		}
	    } else if (sscanf(file, "/%s/%s.page", ext, t) == 2) {
		http_ok(version);
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

    if (index(file, ':') != -1) {
	http_error(version, 501, "Not Implemented. Whatever you are trying "
		   "there, this server won't help you.");
	quit();
	return;
    }

    unless (o) o = file -> load();
    if (objectp(o) || o = find_object(file))
	done = o->htget(version, query, headers, qs) != HTMORE;

    if (done)
	quit();
    else
	remove_call_out(#'quit);
    return 1;
}

// wozu binary_message nochmal durch eine funktion jagen? lieber so nennen:
emit(a) { return binary_message(a); }

devNull() {
    next_input_to(#'devNull);

    D2(D("=== SmallHTTP just ignored some input\n");)
}
