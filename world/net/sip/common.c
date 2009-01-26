// $Id: common.c,v 1.2 2005/03/14 10:23:27 lynx Exp $ // vim:syntax=lpc

#include <net.h>
#include "sip.h"

mapping SIP_PHRASES;
mapping specialCases;

load() {
	specialCases = ([ "Cseq" : "CSeq", 
			  "Call-Id" : "Call-ID",
			  "Www-Authenticate" : "WWW-Authenticate",
			  ]);
	SIP_PHRASES = ([
		        //
			100: "Trying",
			180: "Ringing",
			181: "Call Is Being Forwarded",
			182: "Queued",
			183: "Session Progress",
			//	
			200: "OK",
			//
			300: "Multiple Choices",
			301: "Moved Permanently",
			302: "Moved Temporarily",
			303: "See Other",
			305: "Use Proxy",
			380: "Alternative Service",
			//
			400: "Bad Request",
			401: "Unauthorized",
			402: "Payment Required",
			403: "Forbidden",
			404: "Not Found",
			405: "Method Not Allowed",
			406: "Not Acceptable",
			407: "Proxy Authentication Required",
			408: "Request Timeout",
			409: "Conflict", 
			410: "Gone",
			411: "Length Required",
			413: "Request Entity Too Large",
			414: "Request-URI Too Large",
			415: "Unsupported Media Type",
			416: "Unsupported URI Scheme",
			420: "Bad Extension",
			421: "Extension Required",
			423: "Interval Too Brief",
			480: "Temporarily Unavailable",
			481: "Call/Transaction Does Not Exist",
			482: "Loop Detected",
			483: "Too Many Hops",
			484: "Address Incomplete",
			485: "Ambiguous",
			486: "Busy Here",
			487: "Request Terminated",
			488: "Not Acceptable Here",
			491: "Request Pending",
			493: "Undecipherable",
			//
			500: "Internal Server Error",
			501: "Not Implemented",
			502: "Bad Gateway",
			503: "Service Unavailable",
			504: "Server Time-out",
			505: "SIP Version not supported",
			513: "Message Too Large",
			//
			600: "Busy Everywhere",
			603: "Decline",
			604: "Does not exist anywhere",
			606: "Not Acceptable"
	]);
}

encodevar(varname) {
	int i;
	i = index(varname, '-');
	if(i != -1) 
		varname = upper_case(varname[0..0]) + varname[1..i] + 
				upper_case(varname[i+1..i+1]) + varname[i+2..];
	else varname = capitalize(varname);
	return specialCases[varname] || varname;
}

serialize(varnames, vars, data) {
	string varname;
	mixed value;
	string t;
	
	t = "";
	foreach(varname : varnames) {
		value = vars[varname];
		varname = encodevar(varname); 
		if (pointerp(value)) {
			string v;
			foreach (v : value) t += varname + ": " + v + CRLF;
		}
		else if (stringp(value)) t += varname + ": " + value + CRLF;
	}
	if (data && stringp(data)) {
		if (strlen(data)) t += "Content-Type: " + vars["content-type"] + CRLF;
		t += "Content-Length: " + strlen(data) + CRLF CRLF;
		// TODO: how to end packet?
		t += data;
	}
	return t;
}

string makeResponse(string prot, int code, mapping v, mixed data) {
	string reply;
	reply = prot + " " + code + " " + SIP_PHRASES[code] + CRLF +
		serialize(({ "from", "to", "via", "call-id", "cseq", 
			     "www-authenticate" }), v); 
	if (data && stringp(data)) {
		if (v["content-type"])
			reply += "Content-Type: " + v["content-type"] + CRLF;
		reply += "Content-Length: " + strlen(data);
		reply += data + CRLF CRLF;
	}
	return reply;
}


parseHeaders(string headers) {
	string line;
	mapping v;
	v = ([ ]);
	foreach(line : explode(headers, CRLF)) {
		string key, val;
		unless(sscanf(line, "%s:%t%s", key, val) == 2) {
			P3(("SIP parse: strange line %O\n", line))
			// TODO 400 bad request?
			continue;
		}
		key = lower_case(key);
		val = chomp(val);
		if (pointerp(v[ key ]) ) v[ key ] += ({ val });
		else if (v[ key ])  v[ key ] = ({ v[ key ], val });
		else v[ key ] = chomp(val);
	}
	return v;
}
