// $Id: common.c,v 1.8 2005/03/14 10:23:28 lynx Exp $ // vim:syntax=lpc
//
// yes even we can't avoid having XML parsing classes
// used by jabber and RSS (place/news)
//
#include <interface.h>

#include "xml.h"

string xmlquote(string s) {
	// return xml escaped version of s
	s = replace(s, "&", "&amp;");
	s = replace(s, "<", "&lt;");
	s = replace(s, ">", "&gt;");
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
