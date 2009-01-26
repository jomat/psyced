// $Id: parse.c,v 1.12 2005/06/07 07:03:02 fippo Exp $ // vim:syntax=lpc
//
// the actual RSS parser
// why the file is called "parse.c" i don't know
// it certainly wouldn't be a good idea to have several parsers in one file
// so you may as well rename it into rss.c? TODO
//
#include <net.h>
#include <xml.h>
inherit NET_PATH "xml/common";

// DOM style XML parser
xmlparse(a) {
	// this one is very similar to the jabber parser
	// from a syntax point of view
        string tag, data, params;
        int pos, close;
        int list;

	XMLNode currentnode = 0;
	XMLNode nodestack = ({ });
        params = "";
        pos = 0;
        close = -1;
        
	while(pos = strstr(a, "<", pos) + 1) {
            data = xmlunquote(a[close + 1..pos - 2]);
            close = strstr(a, ">", pos);
        
	    sscanf(a[pos..close - 1], "%s%t%s", tag, params) || tag = a[pos..close-1];
	    if(tag == "") return -1;
	    if (strlen(tag) && (tag[0] == '!' || tag[0] == '?')){
                // P2(("skipping tag starting with ! or ?\n"))
	    } else if (strlen(tag) && tag[0] == '/'){
                P4(("should be closing tag %O and am closing %O\n",
                    currentnode[Tag], tag[1..]))
                if (!currentnode ||  currentnode[Tag] != tag[1..]) {
			// unbalanced xml?
                } else {
                        // schliessender tag gefunden, die haben keine Parameter
                        if (strlen(data) && data != "\r\n" && data != "\n"){
                                // not sure if this works correct
                                unless(pointerp(currentnode[Cdata]))
                                        currentnode[Cdata] = data;
                                else
                                    currentnode[Cdata] += ({ data });
                        }
                        if (sizeof(nodestack) == 0) {
				// we can probably break/return here
				break;
                        } else {
                                currentnode = nodestack[<1];
                                nodestack = nodestack[..<2];
                        }
                }
	    } else { // opening tag
                int selfclosing;
                mixed newnode;
                string key, val;

                if (strlen(params) && params[<1] == '/') {
                        params = params[..<2];
                        selfclosing = 1;
                        newnode = new_XMLNode;
                } else if (tag[<1] == '/') {
                        tag = tag[..<2];
                        selfclosing = 1;
                        newnode = new_XMLNode;
                } else {
                        newnode = new_XMLNode;
                }
                if(currentnode){
                        nodestack += ({ currentnode });
                        if (pointerp(currentnode[Child][tag])) {
                            unless (nodelistp(currentnode[Child][tag])) {
                                // tranform
                                currentnode[Child][tag] = ({ currentnode[Child][tag], newnode });
                            } else {
                                // append
                                currentnode[Child][tag] += ({ newnode });
                            }
                            currentnode = currentnode[Child][tag][<1];
                        } else {
                            currentnode[Child][tag] = newnode;
                            currentnode = currentnode[Child][tag];
                        }
                } else {
                        currentnode = newnode;
                }
                currentnode[Tag] = tag;
                foreach(string pa: explode(params, " ")) {
                    if(sscanf(pa, "%s=\"%s\"", key, val) == 2 ||
                       sscanf(pa, "%s=\'%s\'", key, val) == 2 ) {
                        currentnode[Param][key] = val;

                    }
                }
                if (selfclosing) {
                        if (sizeof(nodestack) == 0){
				PT(("nodestack empty\n"))
                        } else {
                                currentnode = nodestack[<1];
                                nodestack = nodestack[..<2];
                        }
                }
	    }
        }
	return currentnode;
}

