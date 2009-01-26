// $Id: common.c,v 1.10 2007/08/15 09:47:02 lynx Exp $ // vim:syntax=lpc
//
// yes even we can't avoid having XML parsing classes
// used by jabber and RSS (place/news)

// hmm.. why are you including interface.h directly?
// it comes automatically with net.h
#include <interface.h>

#include "xml.h"

string xmlquote(string s) {
	// return xml escaped version of s
	s = replace(s, "&", "&amp;");
	s = replace(s, "<", "&lt;");
	s = replace(s, ">", "&gt;");
	// looks like these only need to be quoted if
	// the string is to be used in a attribute/param
	// but it doesnt hurt anyway so...
	s = replace(s, "\"", "&quot;");
	s = replace(s, "'", "&apos;");
	return s;
}

string xmlunquote(string s) {
	// return unquoted xml version of s
	s = replace(s, "&amp;", "&");
	s = replace(s, "&lt;", "<");
	s = replace(s, "&gt;", ">");
	s = replace(s, "&quot;", "\"");
	s = replace(s, "&apos;", "'");
	// should this take care of &#223;-style thingies
	// s = regreplace(s, "&#223;", 223);
	s = regreplace(s, "&#[0-9][0-9][0-9];",
		       (: return sprintf("%c", to_int($1[2..<2])); :), 1);
	return s;
}
