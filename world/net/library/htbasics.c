// $Id: htbasics.c,v 1.5 2007/06/28 20:17:50 lynx Exp $ // vim:syntax=lpc
#include <net.h>
#include <ht/http.h>

// html-escaping of generic strings     -lynx
// to make sure they won't trigger
// html commands. this should become an inline macro, maybe use
// regreplace if that is faster, or even better, be implemented in C
// together with an auto hyperlink and a linebreak conversion option.
//
varargs string htquote(string s) {
	ASSERT("htquote", stringp(s), s)
        s = replace(s, "&", "&amp;");
//      s = replace(s, "\"", "&quot;"); //"
        s = replace(s, "<", "&lt;");
        s = replace(s, ">", "&gt;");
        return s;
}

#ifdef HTTP_PATH
# include HTTP_PATH "library.i"
#endif
