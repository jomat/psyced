//						vim:noexpandtab:syntax=lpc
// $Id: header.i,v 1.5 2007/10/08 11:00:31 lynx Exp $

#include <ht/http.h>

volatile int headerDone = 0;

http_ok(string prot, string type, string extra) {
	string h;

	// yes, this is compatible to pre-HTTP/1.0 browsers. sick, i know.
	if (!prot || headerDone++) return;

	h = type || extra ? htheaders(type, extra) +"\n"
	       	: "Content-type: " DEFAULT_CONTENT_TYPE "\n\n";
	emit(HTTP_SVERS " 200 Sure\n"+ h);
}

varargs http_error(string prot, int code, string comment, string html) {
	string out;

	out = "<body text=white bgcolor=black link=green vlink=green>\n";
	if (html) out = sprintf("<title>%s</title>\n%s%s", comment, out, html);
	else out = sprintf("\
<title>error %d</title>\n\
%s\n\
<table width=\"100%%\" height=\"90%%\"><tr><th><h1><br>\n\n\
%s\n\n\
</h1></th></tr></table>\n\
",
		code, out, comment
	);
		// <a href=\"mailto:%s?subject=%s\">%s</a>\n
		//, WEBMASTER_EMAIL, comment, WEBMASTER_EMAIL

	// yes, this is compatible to pre-HTTP/1.0 browsers. sick, i know.
	if (!headerDone++ && prot) {
		// I used to output the comment, but Id have to cut out the
		// newline from the db
		emit(sprintf(HTTP_SVERS " 200 Actually %03d but MSIE steals my error page\n%s\n%s", code, htheaders(), out));
        } else emit(out);
}
