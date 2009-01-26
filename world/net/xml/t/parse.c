// $Id: parse.c,v 1.10 2005/03/14 10:23:28 lynx Exp $ // vim:syntax=lpc
//
// the actual RSS parser
// why the file is called "parse.c" i don't know
// it certainly wouldn't be a good idea to have several parsers in one file
// so you may as well rename it into rss.c? TODO
//
#include <net.h>
inherit NET_PATH "xml/common";

rssparse(str) {
        // DOM-style XML parser for parsing RSS and RDF
	// see CHANGESTODO for discussion on how to make it compliant
        mapping dom;
        string namespace;
        string tag, lasttag, data, params;
        int pos, close;

        pos = 0;
        close = -1;
        namespace = "";
        dom = ([ ]);
        while(pos = strstr(str, "<", pos) + 1){
                // D2(D("looping xmlparser...\n");)
                data = xmlunquote(str[close + 1..pos - 2]);
                close = strstr(str, ">", pos);
                sscanf(str[pos..close - 1], "%s%t%s", tag, params) || tag = str[pos..close - 1];
                if (strlen(tag) && (tag[0] == '!' || tag[0] == '?' || tag[0..2] == "rdf" || tag[0..2] == "rss")) {
                        if(tag[0..2] == "rdf" || tag[0..2] == "rss")
				dom["type"] = tag[0..2];
                } else if (strlen(tag) && tag[0] == '/') {
                        // D2(D("closing " + tag + "\n");)
                        // closing tag
                        if (tag[1..] != lasttag){
                                // D2(D("warning: XML may be malformed\n");)
                        	;
			} else {
                                // handle data
                                if (data != "\n") {
                                        if (stringp(dom[namespace]) )
                                                dom[namespace] = ({ dom[namespace], data });
                                        else if (pointerp(dom[namespace]))
                                                dom[namespace] += ({ data });
                                        else
                                                dom[namespace] = data;
                                }
                                namespace = namespace[..<strlen(lasttag) + 2];
                                lasttag = explode(namespace, "_")[<1];
                        }
                } else {
                        // open tag
                        if ((params && params[<1] == '/') || tag[<1] == '/') {
                                // better than before, but not really
				// correct to simply skip it
				continue;
                        }
                        // D2(D("opening " + tag + "\n");)
                        namespace += "_" + tag;
                        lasttag = tag;
                }
                pos = close;
        }
        // P2(("DOM: %O\n", dom))
        return dom;
}
