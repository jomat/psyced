// $Id: xmlrpc.c,v 1.24 2008/04/22 22:43:14 lynx Exp $ // vim:syntax=lpc
//
// TODO: shares to much code with url fetcher
// 	 in the ideal world this would only contain marshal/unmarshal code
// 	 for xmlrpc
// 	 possibly we can come up with an marshal/unmarshal api and use
// 	 the same framework for xml-rpc, soap and other things
//
#include <ht/http.h>
#include <net.h>
#include <uniform.h>
#include <xml.h>

#include <lpctypes.h>

virtual inherit NET_PATH "output"; // virtual: in case we get inherited..
inherit NET_PATH "http/fetch";
inherit NET_PATH "xml/parse";

volatile string postbody;
volatile closure callback;

int parse_status(string all);
int parse_header(string all);
int buffer_content(string all);

// recursively dump a structure
dump(mixed value) {
    /* TODO: what happens if we feed a recursive structure here? */
    string t;
    switch(typeof(value)) {
    case T_NUMBER: 
	return sprintf("<value><int>%d</int></value>", value);
    case T_FLOAT:
	return sprintf("<value><double>%O</double></value>", value);
    case T_STRING:
	// in theory we would have to convert this to utf-8!
	// must to encode '<' and '&'
	return sprintf("<value><string>%s</string></value>", value);
    case T_MAPPING: // -> struct
	// NOTE: THIS DOES NOT SUPPORT mappings with m_width > 2
	t = "<value><struct>\n";
	foreach(string key, mixed val : value) {
	    t += sprintf("<member>\n"
			 "<name>%s</name>\n"
			 "%s\n"
			 "</member>\n",
			 key, dump(val));
	}
	t += "</struct></value>";
	return t;
    case T_POINTER: // array
	t = "<value><array><data>\n";
	foreach(mixed val : value) {
	    t += dump(val) + "\n";
	}
	t += "</data></array></value>";
	return t;
    default:
	PT(("do not know how to serialize %O type %d\n", value, typeof(value)))
	PT(("yes, this is a bug!\n"))
	break;
    }
    return ""; 
}

marshal(params) {
    string s = "<params>\n";
    foreach(mixed param : params) {
	s += sprintf("<param>\n%s\n</param>\n", dump(param));
    }
    return s + "</params>\n"; 
}

void fetch(string murl) {
    if (url) return;
    url = murl;
}

void request(string method, mixed params, closure cb) { // TODO: errback API 
    if (fetching) {
	enqueue(ME, ({ method, params, cb }));
	return;
    }
    callback = cb;
    postbody = sprintf("<?xml version='1.0'?>\n"
		       "<methodCall>\n"
		       "<methodName>%s</methodName>\n"
		       "%s\n"
		       // last is a \r\n!
		       "</methodCall>\r\n", method, marshal(params));
    P3(("%O: request(%O)\n", ME, url))
    connect(); 
}

int logon(int arg) {
	buffer = "";
	headers = ([ ]);
	http_status = 500;

	// this is all not https: compatible..
	unless(::logon(arg, 1)) return -1;
	unless (url) return -3;
	unless (resource) sscanf(url, "http://%s/%s", host, resource); 
	// do somthing
	emit("POST /" + resource + " HTTP/1.0\r\n"
		 + "Host: " + host + "\r\n"
		 + "Content-Type: text/xml\r\n"
		 + "Content-Length: " + sizeof(postbody) + "\r\n");
	emit("\r\n");
		 //+ "User-Agent: " + SERVER_VERSION + "\r\n");
	emit(postbody);
	next_input_to(#'parse_status);
	return 0; // duh.
}

mixed * innerMarshal(mixed val) {
    mixed args;
    string type;
    // TODO: methodSignature behaves strangely
    XMLNode value;
    foreach(mixed key, mixed vval : val) {
	if (stringp(key) && key[0] == '/') {
	    type = key[1..];
	    value = val[key];
	    break;
	}
    }
    switch(type) {
    case "boolean": // map to 0, 1
	// should probably check if this string is really "0" or "1"
    case "int":
	return ({ to_int(value[Cdata]) });
    case "double":
	// sigh... this is not double
	return ({ to_float(value[Cdata]) });
    case "dateTime.iso8601": // uhm... what do we map this to?
	// unix timestamp probably?
	break;
    case "base64":
	// probably containing binary data
	return ({ decode_base64(value[Cdata]) });
    // these two are complicated
    case "array":
	args = ({ });
	value = value["/data"]["/value"];
	unless(nodelistp(value)) value = ({ value });
	foreach(mixed v : value) {
	    args += innerMarshal(v);
	}
	return ({ args });
    case "struct": // mapping
	args = ([ ]);
	unless(nodelistp(value["/member"])) 
	    value["/member"] = ({ value["/member"] });
	foreach(mixed v: value["/member"]) {
	    args[v["/name"][Cdata]] = innerMarshal(v["/value"])[0];
	}
	return ({ args });
    case "fault": // something is wrong
	break;
    default: // if no type is indicated, the type is string
    case "string": // utf-8 encoded!
	return ({ value[Cdata] });
    }
    return ({ });
}

mixed unMarshal(XMLNode parsed) {
    XMLNode params;
    mixed *helper;

    mixed args = ({ }); // could be pre-allocated
    unless(parsed["/fault"]) {
	if (parsed["/params"]) {
	    params = parsed["/params"]["/param"];
	} else {
	    params = ({ });
	}
	unless(nodelistp(params)) params = ({ params }); // special case
	foreach(mixed param : params) {
	    args += innerMarshal(param["/value"]); 
	}
	// should probably watch for HTTP 200
	return args;
    } else {
	// TODO: fault handling
	return -1;
    }
}

int disconnected(string remainder) {
    mixed *args;

    headers["_fetchtime"] = isotime(ctime(time()), 1);
    if (headers["last-modified"])
	    modificationtime = headers["last-modified"];
    if (headers["etag"]) 
	    etag = headers["etag"]; // heise does not work with etag

    fetched = buffer;
    fheaders = headers;
    buffer = headers = 0;
    fetching = 0;
    if (pointerp(args)) // no fault
	funcall(callback, args...);

    if (qSize(ME)) {
	args = shift(ME);
	call_out(#'request, 0, args...); // at next heart_beat
	// request(args...);
    }
    return 1;   // kind of expected
}

// helper methods, useful at least for ORA meerkat
// could do caching for these things
void listMethods(closure cb) {
    request("system.listMethods", ({ }), cb);
}

void methodHelp(string method, closure cb) {
    request("system.methodHelp", ({ method }), cb);
}

void methodSignature(string method, closure cb) {
    request("system.methodSignature", ({ method }), cb);
}


varargs string content(closure cb, int force, int willbehave) { return ""; } // makes no sense

create() { 
    qCreate();
    qInit(ME, 150, 5);
#ifdef MODULE_TESTS
    selftest(); 
#endif
}

/*--------------------------------------------------------------------
 * MODULE TESTS
 * you should run them whenever you change the code
 * if they fail, debug!
 * do not check in code that does not pass the tests
 *--------------------------------------------------------------------*/
#ifdef MODULE_TESTS
// automatic test cases
// add some if you like
selftest() {
    // self tests... 
    // be careful, the order for structs is not guaranteed
    int i, result;
    mixed *structures = ({ 
			    ({ 1, 2, 3, 4 }),
//			    ({ (["a" : 1, "b" : "boo" ]) }), 
			    ({ 1, 1.5, "a", ({ 1 }), (["a" : "foo" ]) })
			});
    mixed *strings = ({
"<params>\n<param>\n<value><int>1</int></value>\n</param>\n<param>\n<value><int>2</int></value>\n</param>\n<param>\n<value><int>3</int></value>\n</param>\n<param>\n<value><int>4</int></value>\n</param>\n</params>\n",

// "<params>\n<param>\n<value><struct>\n<member>\n<name>a</name>\n<value><int>1</int></value>\n</member>\n<member>\n<name>b</name>\n<value><string>boo</string></value>\n</member>\n</struct></value>\n</param>\n</params>\n",

"<params>\n<param>\n<value><int>1</int></value>\n</param>\n<param>\n<value><double>1.5</double></value>\n</param>\n<param>\n<value><string>a</string></value>\n</param>\n<param>\n<value><array><data>\n<value><int>1</int></value>\n</data></array></value>\n</param>\n<param>\n<value><struct>\n<member>\n<name>a</name>\n<value><string>foo</string></value>\n</member>\n</struct></value>\n</param>\n</params>\n"
			       });
    
    i = 0;
    foreach(mixed test : structures) {
	result = marshal(test) == strings[i];
	if (result) {
	    PT(("passed marshal test %d\n", i))
	} else {
	    PT(("marshal test %d failed\n", i))
	    PT(("%O\n%O\n", marshal(test), strings[i]))
	    break;
	}
	i++;
    }
    i = 0;
    foreach(mixed test : strings) {
	// set callback internally! it is safe to do so as we are 
	// doing only synchronus operations
	XMLNode x = new_XMLNode;
	x["/params"] = xmlparse(test);
	result = sprintf("%O", unMarshal(x)) == sprintf("%O", structures[i]);
	if (result) {
	    PT(("passed unmarshal test %d\n", i))
	} else {
	    PT(("unmarshal test %d failed\n", i))
	    PT(("%O\n%O\n", unMarshal(x), structures[i]))
	    break;
	}
	i++;
    }
}
#endif
