// $Id: parse.c,v 1.51 2008/03/29 20:36:44 lynx Exp $ // vim:syntax=lpc
//
// this code is employed to parse both XML and XMPP.
// if expat has been provided at compiling time, it will try to use it.
//

// until you fix that TODO (please do!)
#undef __EXPAT__

#ifdef JABBER_PARSE
volatile XMLNode currentnode = 0;
volatile XMLNode *nodestack = ({ });
volatile int length = 0;
closure nodeHandler = #'jabberMsg;
# ifdef JABBER_TRANSPARENCY
volatile string innerxml, lasta, ixbuf;
# endif
#else
# include <net.h>
# include <xml.h>
inherit NET_PATH "xml/common";
volatile string charset;

# define XML_ERROR(code, long) \
        P0(("XML parse in %O: %s\n", ME, long))
#endif

#if !defined(__EXPAT__) || defined(JABBER_PARSE)
// DOM style XML parser
xmlparse(a) {
#ifndef JABBER_PARSE
	XMLNode currentnode = 0;
	XMLNode *nodestack = ({ });
#endif
        string t, tag, data = "", params = "";
        int pos, close;
        int list;

#ifdef JABBER_PARSE
# ifdef _flag_log_sockets_XMPP
        D0( log_file("RAW_XMPP", "\n» %O\t%s", ME, a); )
# endif
        length += sizeof(a);
        pos = index(a, '<', pos) + 1;
        data = xmlunquote(a[0..pos -2]);
        close = strlen(a) - 1;
# ifdef JABBER_TRANSPARENCY
	if (ixbuf) {
		if (lasta) ixbuf += lasta;
		lasta = a;
	}
# endif
#else
        pos = 0;
        close = -1;
	// jabber parser doesn't while, so it has one indent step less
	while(pos = index(a, '<', pos) + 1) {
            data += xmlunquote(a[close + 1..pos - 2]);
#endif
#if 1 //def HANDLE_CDATA
	    //
	    // http:/www.techjamaica.com/forums/external.php?type=rss2
	    // uses <![CDATA[<p>this is<br/>embedded html</p>]]> syntax
	    // to embed potentially broken html into xml. in fact most
	    // blogs produce this sort of rss code these days.
	    //
// do we want to support ![CDATA[ ]] for XMPP, too? ... then fix here!
	    if (a[pos..pos+7] == "![CDATA[") {
		    pos += 8;
		    close = strstr(a, "]]>", pos);
		    data += xmlunquote(a[pos..close-1]);
		    close += 2;
		    pos = close;	// this may seem optional.. but?
		    P4(("%O unCDATAfied %O\n", ME, data));
# ifndef JABBER_PARSE
		    continue;
# else
		    // ok, so this doesn't hurt at least..
		    // but should return here? and what?
# endif
	    }
#endif
#ifndef JABBER_PARSE
            close = index(a, '>', pos);
#endif
	    tag = a[pos..close-1];
	    pos = close+1;		// do not reparse seen things (opt)
	    sscanf(tag, "%s%t%s", tag, params); //|| (params = 0);
	    if (tag == "") return -1;
	    if (strlen(tag) && (tag[0] == '!' || tag[0] == '?')) {
#ifndef JABBER_PARSE
		// charset handling currently limited to news parsers
		if (lower_case(tag) == "?xml" &&
		  (sscanf(params, "%sencoding=\"%s\"%s", t, charset, t) >= 2 ||
		   sscanf(params, "%sencoding=\'%s\'%s", t, charset, t) >= 2)) {
		    charset = upper_case(charset);
		    if (charset != SYSTEM_CHARSET) {
			// ok, we believe it's working :)
			PT(("%O converting from charset %O\n", ME, charset))
			iconv(a, charset, SYSTEM_CHARSET);
		    }
		}
		else {
		    PT(("%O skipping funny %O tag (%O)\n", ME, tag, params))
		}
#endif
#ifdef JABBER_PARSE
	    } else if (strlen(tag) && tag == "/stream:stream"){
		    // close_stream();
		    // quit();
#endif
	    // tag is a close tag
	    } else if (strlen(tag) && tag[0] == '/') {
                P4(("should be closing tag %O and am closing %O\n",
                    currentnode[Tag], tag[1..]))
                if (!currentnode ||  currentnode[Tag] != tag[1..]) {
			XML_ERROR("xml-not-well-formed",
				 "Unbalanced XML encountered");
			PT(("%O closing %O instead of tag in %O\n", ME, tag,
			    currentnode))
#ifdef JABBER_PARSE
			// this will trigger disconnect in calling object
			return;
#endif
                } else {
                        // schliessender tag gefunden, die haben keine Parameter
                        if (strlen(data) && data != "\r\n" && data != "\n"){
				// we just concatenate the cdata!
				if (!stringp(currentnode[Cdata]))
                                    currentnode[Cdata] = data;
				else
				    currentnode[Cdata] += data;
                        }
			data = "";
#ifdef JABBER_PARSE
# ifdef JABBER_TRANSPARENCY
			// the two ifs can be optimized if we like this
			// approach better than three comparisons
                        if (sizeof(nodestack) == 0) {
	//		if (tag == "/iq" 
	//		    || tag == "/presence" 
	//		    || tag == "/message") {
				innerxml = ixbuf;
				ixbuf = lasta = 0;
				P4((" <%s>\n", tag))
				P4(("innerxml body %O\n", innerxml))
			}
# endif
#endif
                        if (sizeof(nodestack) == 0) {
#ifdef JABBER_PARSE
                                currentnode[NodeLen] = length;
                                // handle stuff
                                funcall(nodeHandler, currentnode);
                                currentnode = 0;
                                length = 0;
#else
				// we can probably break/return here
				break;
#endif
                        } else {
                                currentnode = nodestack[<1];
                                nodestack = nodestack[..<2];
                        }
                }
	    } else { // opening tag
                int selfclosing;
                XMLNode newnode;
                string key, val;
		mixed *ptmp;

		if (currentnode && data && data != "\r\n" && data != "\n") {
		    // we just concatenate the cdata!
		    // watch out, nearly identical code above
		    if (!stringp(currentnode[Cdata]))
			currentnode[Cdata] = data;
		    else
			currentnode[Cdata] += data;
		}
		data = "";

                if (strlen(params) && params[<1] == '/') {
                        params = params[..<2];
                        selfclosing = 1;
                } else if (tag[<1] == '/') {
                        tag = tag[..<2];
                        selfclosing = 1;
		}
		newnode = new_XMLNode;

                if (currentnode) {
			t = "/"+ tag;
                        nodestack += ({ currentnode });
                        if (mappingp(currentnode[t])) {
			    // transform
			    currentnode[t] = ({ currentnode[t], newnode });
                            currentnode = currentnode[t][<1];
			} else if (pointerp(currentnode[t])) {
			    // append
			    currentnode[t] += ({ newnode });
                            currentnode = currentnode[t][<1];
                        } else {
			    // create
                            currentnode[t] = newnode;
                            currentnode = currentnode[t];
                        }
                } else {
                        currentnode = newnode;
                }
                currentnode[Tag] = tag;
#if 1//def EXPERIMENTAL // yay, things change fast!
# ifndef JABBER_PARSE
		// this will still not be able to handle something like
		//	<img src='18072006.jpg' alt="5er &amp; s'Weggli" />
		// but who sends something like that?
                ptmp = regexplode(params, "[a-zA-Z0-9]+=\"[^\"]*\"");
		if (sizeof(ptmp) < 2 || sizeof(ptmp) % 2)
		    ptmp = regexplode(params, "[a-zA-Z0-9]+='[^']*'");
# else
		// this method breaks on something like
		//	<img src="18072006.jpg" alt="5er &amp; s'Weggli" />
                ptmp = regexplode(params, "[a-zA-Z0-9]+=(\"|')[^\"']*(\"|')");
# endif
                for (int i = 1; i < sizeof(ptmp); i += 2) {
                    int where = index(ptmp[i], '=');

                    key = ptmp[i][..where-1];
                    val = ptmp[i][where+1..];

                    if (val[0] != val[<1]) {
                        XML_ERROR("xml-not-well-formed", "Mismatching quotes")
			PT(("%O %O %O %O\n", ME, key, val, ptmp))
                    }
                    val = val[1..<2];
                    currentnode["@"+ key] = val;
                }
#else
		// this approach cannot handle param="string with spaces"
		foreach(string pa: explode(params, " ")) {
		    if(sscanf(pa, "%s=\"%s\"", key, val) == 2 ||
		       sscanf(pa, "%s=\'%s\'", key, val) == 2 ) {
			currentnode["@"+ key] = val;

		    }
		}
#endif
                if (selfclosing) {
                        if (sizeof(nodestack) == 0){
#ifdef JABBER_PARSE
                                currentnode[NodeLen] = length;
# ifdef JABBER_TRANSPARENCY
				ixbuf = lasta = 0;
				innerxml = ixbuf;
#endif
                                // handle stuff
                                funcall(nodeHandler, currentnode);
                                currentnode = 0;
                                length = 0;
#else
				PT(("nodestack empty\n"))
#endif
                        } else {
                                currentnode = nodestack[<1];
                                nodestack = nodestack[..<2];
                        }
#ifdef JABBER_PARSE
                } else if (currentnode[Tag] == "stream:stream") {
                        open_stream(currentnode);
                        nodestack = ({ }); // ?
                        currentnode = 0;
# ifdef JABBER_TRANSPARENCY
                } else // if (currentnode[Tag] == "iq" 
			//   || currentnode[Tag] == "presence"
			//   || currentnode[Tag] == "message") {
		   if (sizeof(nodestack) == 0) {
			ixbuf = ""; lasta = 0;
			P4((" <%s> ", currentnode[Tag]))
# endif
#endif
                }
	    }
#ifndef JABBER_PARSE
        }
	return currentnode;
#endif
}

#else /* !defined(__EXPAT__) || defined(JABBER_PARSE) */

volatile mixed node = 0;
volatile mixed *nodestack = ({ });

void onStart(string elem, string *params) {
    string t = "/"+ elem;

    if (node) {
	nodestack += ({ node });
	if (!node[t]) {
	    /* no child with that name */
	    node[t] = new_XMLNode;
	    node = node[t];
	} else {
	    if (!nodelistp(node[t])) {
		/* just a single node with that name, convert it 
		 */
		node[t] = ({ node[t] });
	    }
	    node[t] += ({ new_XMLNode });
	    node = node[t][<1];
	}
    } else {
	node = new_XMLNode;
	nodestack = ({ });
    } 
    node[Tag] = elem;
    // TODO: das hier funktioniert mit der neuen API nicht so
    node[Param] = params;
}

void onEnd(string elem) {
    if (sizeof(nodestack) > 0) {
	node = nodestack[<1];
	nodestack = nodestack[..<2];
    } 
    /* else we are finished? */
}

void onText(string text) {
    if (node[Cdata]) 
	node[Cdata] += text;
    else
	node[Cdata] = text;
}

xmlparse(a) {
    PT(("expat xmlparse\n"))
    int d;
    node = 0;
    nodestack = ({ });
    d = expat_parse(a, #'onStart, #'onEnd, #'onText);
    return node;
}

#endif
