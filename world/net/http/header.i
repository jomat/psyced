//						vim:noexpandtab:syntax=lpc
// $Id: header.i,v 1.10 2008/08/05 12:21:33 lynx Exp $

#include <ht/http.h>

volatile int headerDone = 0;

http_ok(string prot, string type, string extra) {
	string out;

	// yes, this is compatible to pre-HTTP/1.0 browsers. sick, i know.
	if (!prot || headerDone++) return;

	out = type || extra ? htheaders(type, extra) +"\n"
	       	: "Content-type: " DEFAULT_CONTENT_TYPE "\n\n";
	emit(out = HTTP_SVERS " 200 Sure\n"+ out);
	P3((out))
}

varargs http_error(string prot, int code, string comment, string html) {
	string out;

	// apparently there isn't a single app that calls this with "html"
	P2(("hterror(%O,%O,%O,%O) in %O\n", prot,code,comment,html, ME))
#if defined(T)
	// use the textdb if available
	out = psyctext( T("_PAGES_error",
			      "<html><title id='code'>[_code]</title>\n"
		"<body><h1 id='comment'>[_comment]</h1></body></html>\n"),
		    ([ "_comment": comment, "_code": code ]) );
#else
	// use some hardcoded defaults
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
#endif
	// yes, this is compatible to pre-HTTP/1.0 browsers. sick, i know.
	if (!headerDone++ && prot) {
		// I used to output the comment, but Id have to cut out the
		// newline from the db
		emit(out = sprintf(HTTP_SVERS " 200 Actually %03d but MSIE steals my error page\n%s\n%s", code, htheaders(), out));
		P3((out))
        } else emit(out);
}
