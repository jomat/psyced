// $Id: fetch.c,v 1.42 2008/12/10 22:53:33 lynx Exp $ // vim:syntax=lpc
//
// generic HTTP GET client, mostly used for RSS -
// but we could fetch any page or data with it, really
// tobij even allowed the object to have the URL as its object name. fancy!  ;)

#ifdef Dfetch
# undef DEBUG
# define DEBUG Dfetch
#endif

#include <ht/http.h>
#include <net.h>
#include <uniform.h>
#include <services.h>
#include <regexp.h>

virtual inherit NET_PATH "output"; // virtual: in case we get inherited..
inherit NET_PATH "connect";
//inherit NET_PATH "place/master";

inherit NET_PATH "queue";

// additional headers. we keep them lower-case to ensure we have no
// double items in there. HTTP ignores case by spec.
volatile mapping rheaders = ([ "user-agent": SERVER_VERSION ]);

volatile mapping headers, fheaders;
volatile string http_message;
volatile int http_status, port, fetching, ssl;
volatile string buffer, thehost, url, fetched, host, resource, method;
volatile mixed rbody;
volatile int stream;

int parse_status(string all);
int parse_header(string all);
int buffer_content(string all);

string qHost() { return thehost; }

varargs void fetch(string murl, string meth, mixed body, mapping hdrs, int strm) {
	method = meth || "GET";
	rbody = body;
	stream = strm;
	if (hdrs) rheaders += hdrs;
	if (url != murl) {
		// accept.c does this for us:
		//url = replace(murl, ":/", "://");
		// so we can use this method also in a normal way
		url = murl;
		// resource may need to be re-parsed (other params)
		resource = 0;
		// re-parse the hostname?
	       	//thehost = port = 0;
	}
	P3(("%O: fetch(%O)\n", ME, url))
	unless (fetching) connect();
}

object load() { return ME; }

void sAuth(string user, string password) {
	rheaders["authorization"] = "basic "+ encode_base64(user +":"+ password);
}

string sAgent(string a) { return rheaders["user-agent"] = a; }

// net/place/news code follows.

void connect() {
	mixed t;

	fetching = 1;
	ssl = 0;
	unless (thehost) {
		unless (sscanf(url, "http%s://%s/%!s", t, thehost)) {
			P0(("%O couldn't parse %O\n", ME, url))
			return 0;
		}
		//thehost = lower_case(thehost); // why? who needs that?
		ssl = t == "s";
	}
	P4(("URL, THEHOST: %O, %O\n", url, thehost))
	unless (port) {
		unless (sscanf(thehost, "%s:%d", thehost, port) == 2)
		    port = ssl? HTTPS_SERVICE: HTTP_SERVICE;
		rheaders["host"] = thehost;
	}
	P2(("Resolving %O and connecting.\n", thehost))
	::connect(thehost, port);
}

// some people think these are case sensitive.. let's fix it for them (only works for most cases)
string http_header_capitalize(string name) {
    return regreplace(name, "(^.|-.)", (: return upper_case($1); :), 1);
}

varargs int real_logon(int failure) {
	string scheme;

	headers = ([ ]);
	http_status = 500;
	http_message = "(failure)";	// used by debug only

	unless(::logon(failure)) return -1;
	unless (url) return -3;
	unless (resource) sscanf(url, "%s://%s/%s", scheme, host, resource); 

	string body = "";
	if (stringp(rbody)) {
	    body = rbody;
	} else if (mappingp(rbody) && sizeof(rbody)) {
	    body = make_query_string(rbody);
	    unless (rheaders["content-type"])
		rheaders["content-type"] = "application/x-www-form-urlencoded";
	}
	if (strlen(body)) rheaders["content-length"] = strlen(body);

	buffer = "";
	foreach (string key, string value : rheaders) {
	    buffer += http_header_capitalize(key) + ": " + value + "\r\n";
	}

	// we won't need connection: close w/ http/1.0
	//emit("Connection: close\r\n\r\n");		
	P2(("%O fetching /%s from %O\n", ME, resource, host))
	P4(("%O using %O\n", ME, buffer))
	emit(method + " /"+ resource +" HTTP/1.0\r\n"
	     + buffer + "\r\n" + body);

	buffer = "";
	next_input_to(#'parse_status);
	return 0; // duh.
}

varargs int logon(int failure, int sub) {
// net/connect disables telnet for all robots and circuits
#if 0 //__EFUN_DEFINED__(enable_telnet)
	// when fetching the spiegel rss feed, telnet_neg() occasionally
	// crashes. fixing that would be cool, but why have the telnet
	// machine enabled at all?
	enable_telnet(0);
#endif
	// when called from xmlrpc.c we can't do TLS anyway
	if (sub) return ::logon(failure);
	if (ssl) tls_init_connection(ME, #'real_logon);
	else real_logon(failure);
	return 0; // duh.
}

int parse_status(string all) {
	string prot;
	string state;

	sscanf(all, "%s%t%s", prot, state);
	sscanf(state, "%d%t%s", http_status, http_message);
	if (http_status != R_OK) {
		P0(("%O got %O %O from %O\n", ME,
		    http_status, http_message, host));
		monitor_report("_failure_unsupported_code_HTTP",
		    S("http/fetch'ing %O returned %O %O", url || ME,
		       http_status, http_message));
	}
	next_input_to(#'parse_header);
	return 1;
}

int parse_header(string all) { 
	string key, val;
	// TODO: parse status code
	if (all != "") {
		P2(("http/fetch::parse_header %O\n",  all))
		if (sscanf(all, "%s:%1.0t%s", key, val) == 2) {
			headers[lower_case(key)] = val;
			// P2(("ht head: %O = %O\n", key, val))
		}
		next_input_to(#'parse_header);
		return 1;
	} else {
		// das wollen wir nur bei status 200
		P2(("%O now waiting for http body\n", ME))
		next_input_to(#'buffer_content);
		return 1;
	}
	return 1;
}

int buffer_content(string data) {
	P2(("%O body %O\n", ME, data))
	if (stream) {
		mixed *waiter;
		foreach (waiter : qToArray(ME)) {
			funcall(waiter[0], data, waiter[1] ? fheaders : copy(fheaders), http_status, 1);
		}
	} else {
		buffer += data + "\n";
	}
	next_input_to(#'buffer_content);
	return 1;
}

disconnected(remainder) {
	P2(("%O got disconnected.. %O\n", ME, remainder))
	headers["_fetchtime"] = isotime(ctime(time()), 1);
	if (headers["last-modified"])
	    rheaders["if-modified-since"] = headers["last-modified"];
	//if (headers["etag"])
	//    rheaders["if-none-match"] = headers["etag"]; // heise does not work with etag

	if (stream) {
		fetched = remainder;
	} else {
		fetched = buffer;
		if (remainder) fetched += remainder;
	}
	fheaders = headers;
	buffer = headers = 0;
	switch (http_status) {
	default:
		mixed *waiter;
		while (qSize(ME)) {
			waiter = shift(ME);
			P2(("%O calls back.. body is %O\n", ME, fetched))
			funcall(waiter[0], fetched, waiter[1] ? fheaders : copy(fheaders), http_status);
		}
		if (http_status == R_OK) break;
		// doesn't seem to get here when HTTP returns 301 or 302. strange.
		// fall thru
	case R_NOTMODIFIED:
		qDel(ME);
		qInit(ME, 150, 5);
	}
	fetching = 0;
	return 1;       // presume this disc was expected
}

varargs string content(closure cb, int force, int willbehave) {
	if (cb) {
	    if (fetched) {
		if (force) {
		    funcall(cb, fetched, willbehave ? fheaders : copy(fheaders));
	}
	    } else {
		enqueue(ME, ({ cb, willbehave }));
	    }
	}
	return fetched;
}

varargs mapping headers(int willbehave) {
	return willbehave ? fheaders : copy(fheaders);
}

string qHeader(mixed key) {
	if (mappingp(fheaders)) return fheaders[key];
	return 0;
}

string qReqHeader(string key) {
	return rheaders[lower_case(key)];
}

void sReqHeader(string key, string value) {
	rheaders[lower_case(key)] = value;
}

varargs void refetch(closure cb, int willbehave) {
	enqueue(ME, ({ cb, willbehave }));
	unless (fetching) connect();
}

protected create() {
	qCreate();
	qInit(ME, 150, 5);
}
