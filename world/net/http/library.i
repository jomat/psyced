//						vim:noexpandtab:syntax=lpc
// $Id: library.i,v 1.40 2008/04/27 08:20:47 lynx Exp $
//
// HTTP function library  -lynx
//
// (extension to "simul_efun")

#include <net.h>
#include <services.h>
#include <proto.h>

#include "driver.h"
//#include CONFIG_PATH "ports.h"
#include "ht/http.h"

#if !HAS_PORT(HTTP_PORT, HTTP_PATH)
# undef HTTP_PORT
# define HTTP_PORT 44444	// should return null instead?
#endif

// not worthy of summoning uniform.h for this one..
string htuniform(object server, string prot, mapping headers) {
	    // it is really totally crazy to trust what the browser says
	string host = headers["host"];
	int port = HTTP_PORT;
#if __EFUN_DEFINED__(tls_query_connection_state)
#ifndef HTTPS_HOST
	string scheme = "http://";
# endif
	unless (server) server = this_interactive();
	if (tls_query_connection_state(server)) {
#ifdef HTTPS_HOST
		return HTTPS_HOST;
#else
		scheme = "https://";
# if HAS_TLS_PORT(HTTPS_PORT)
		port = HTTPS_PORT;
# endif
#endif
	}
# define SCHEME	scheme
# define PORT port
#else
# define SCHEME	"http://"
# define PORT HTTP_PORT
#endif
	unless (host) {
#ifdef HTTP_HOST
		return HTTP_HOST;
#else
		host = SERVER_HOST;
		// HTTP_SERVICE is 80 from services.h
		if (port != HTTP_SERVICE) host += ":"+to_string(port);
#endif
	}
	return SCHEME + host;
}

varargs string htheaders(string type, string extra) {
	string out;

	unless (stringp(type)) type = DEFAULT_CONTENT_TYPE;
	out = "Content-type: "+ type +"\n";

//	if (length) {
//		printf("Content-length: %d\n", length);
//	}

	if (extra) out += extra;
	return out;
}

varargs string htheaders2(string type, string extra) {
	return htheaders(type, extra)
		+ "Server: " HTTP_SERVER "\nDate: "+ ctime(time()) +"\n";
}

void htrequireauth(string prot, string type, string realm) {
	if (prot) write(HTTP_SVERS + " " + to_string(R_UNAUTHORIZED) + 
			"Your password, please\n");
	if (type == "digest") {
		write("WWW-Authenticate: Digest realm=\"" + realm + 
		      "\", nonce=\"" + time() + "\"\n");	
	} else {
		write("WWW-Authenticate: Basic realm=\"" + realm + "\"\n");
	}
}

varargs string htredirect(string prot, string target, string comment, int permanent, string extra) {
	if (!comment) comment = "Check this out";
	if (!target) target = "/";
	if (!extra) extra = "";

	if (prot) {
		printf("%s %d %s\n%s", HTTP_SVERS,
		  permanent ? R_MOVED : R_FOUND, comment, htheaders());
	}
	printf("\
Location: %s\n%s\
\n\
<a href=\"%s\">%s</a>.\n\
",
		target, extra, target, comment);
	return 0;
}

// written and donated by _Marcus_
//
// thanx!!

/* convert %XX escapes to real chars */
static int xx2l(string str) {
	int     x;

	str=lower_case(str); 
	if (str[0]>='0' && str[0]<='9') x=(str[0]-'0')*16;
	if (str[0]>='a' && str[0]<='f') x=(str[0]-'a'+10)*16;
	if (str[1]>='0' && str[1]<='9') x+=str[1]-'0';
	if (str[1]>='a' && str[1]<='f') x+=str[1]-'a'+10;
	return x;
}

/* Content-Encoding: url-encoded */
string urldecode(string txt) {
	string  *xx;
	int     i;

	txt=implode(explode(txt,"+")," ");
#if __EFUN_DEFINED__(regexplode)
	xx=regexplode(txt,"%..");
	for (i=1;i<sizeof(xx)-1;i+=2)
		xx[i]=sprintf("%c",xx2l(xx[i][1..]));
	txt=implode(xx,"");
#endif
	return txt;
}

#if 0	// inline this instead, see below
static string c2xx(string c) {
    return "%"+ upper_case(sprintf("%x", c[0]));
}
#endif

string urlencode(string txt) {
    return regreplace(txt, "[^A-Za-z0-9._~-]", (:
		"%"+ upper_case(sprintf("%x", $1[0])) :), 1);
}

#ifndef DEFAULT_HT_TYPE
# define DEFAULT_HT_TYPE	"text/plain"
#endif
string content_type(string suffix) {
    switch(lower_case(suffix)) {
case "html":
	return "text/html";
case "xml":
	//return "application/xml";
	return "text/xml"; // what type is to be used here?
case "bz2":
	return "application/x-bzip2";
case "class":
	return "application/octet-stream";
case "css":
	return "text/css";
case "gif":
	return "image/gif";
case "gz":
	return "application/x-gzip";
case "jpeg":
    case "jpg":
	return "image/jpeg";
case "js":
	return "application/x-javascript";
case "pdf":
	return "application/pdf";
case "png":
	return "image/png";
case "ps":
    case "eps":
	return "application/postscript";
case "tar":
	return "application/tar";
case "zip":
	return "application/zip";
case "swf":
        return "application/x-shockwave-flash";
default:
	return DEFAULT_HT_TYPE;
    }
    return 0;
}

mapping url_parse_query(mapping query, string qs) {
    foreach (string pair : explode(qs, "&")) {
	string key, val;

	if (sscanf(pair, "%s=%s", key, val)) {
	    P3(("query: pair: %s, %s\n", urldecode(key),
		   urldecode(val)))
	    query[urldecode(key)] = urldecode(val);
	} else {
	    P3(("query: single: %s\n", urldecode(pair)))
	    query[urldecode(pair)] = 1;
	}
    }
    return query;
}

varargs string make_query_string(mapping params, int sort) {
    string q = "";
    array(mixed) keys = m_indices(params);
    if (sort) keys = sort_array(keys, #'>);

    foreach(string key : keys)  {
	q += (strlen(q) ? "&" : "") + urlencode(to_string(key)) + "=" + urlencode(to_string(params[key]));
    }
    return q;
}

object check_query_token(mapping query) {
        string nick;
        object user;

        if (nick = query["user"]) user = find_person(nick);
        if (user && user->validToken(query["token"])) return user;
        return 0;
}


