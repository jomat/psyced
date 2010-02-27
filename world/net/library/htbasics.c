// $Id: htbasics.c,v 1.5 2007/06/28 20:17:50 lynx Exp $ // vim:syntax=lpc
#include <net.h>
#include <ht/http.h>

// html-escaping of generic strings     -lynx
// to make sure they won't trigger
// html commands
//
varargs string htquote(string s, int newlines) {
	ASSERT("htquote", stringp(s), s)
        s = replace(s, "&", "&amp;");
//      s = replace(s, "\"", "&quot;"); //"
        s = replace(s, "<", "&lt;");
        s = replace(s, ">", "&gt;");
	if (newlines) s = replace(s, "\n", "<br>\n");
        return s;
}

#ifdef HTTP_PATH
# include HTTP_PATH "library.i"
#endif
