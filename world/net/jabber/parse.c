#include <net.h>
#include <xml.h>

// dummy declarations
open_stream(node) { ; }

volatile string innerxml;
volatile closure nodeHandler;

protected int _xml_start, _xml_end, _xml_offset;
string buffer = "";
XMLNode _node;
volatile XMLNode *_nodestack = ({ });

void xml_onStartTag(string tag, mapping params) {
    string t = "/"+ tag;
    if (!_node && tag == "stream:stream") {
	_node = new_XMLNode;
	_node[Tag] = tag;
	foreach(string key, string val : params)
	    _node["@" + key] = val;

	open_stream(_node);
	_node = 0;
	buffer = buffer[expat_position() + expat_eventlen()..];
	return;
    }

    if (_node) {
	_nodestack += ({ _node });
	if (!_node[t]) {
	    /* no child with that name */
	    _node[t] = new_XMLNode;
	    _node = _node[t];
	} else {
	    if (!nodelistp(_node[t])) {
		/* just a single node with that name, convert it 
		 */
		_node[t] = ({ _node[t] });
	    }
	    _node[t] += ({ new_XMLNode });
	    _node = _node[t][<1];
	}
    } else {
	_node = new_XMLNode;
	_nodestack = ({ });
	_xml_start = expat_position() + expat_eventlen();
	_xml_offset = expat_position();
    } 
    _node[Tag] = tag;
    foreach(string key, string val : params)
	_node["@" + key] = val;
}

void xml_onEndTag(string tag) {
    if (!_node && tag == "stream:stream") {
	// is there anything to be done here? like sending a closing
	// ack? we did not do it before but... lynX wrote that stream
	// closing handshake xep so he probably implemented it, too...
	return;
    }
    if (sizeof(_nodestack) > 0) {
	_node = _nodestack[<1];
	_nodestack = _nodestack[..<2];
    } else {
	_xml_end = expat_position();
	innerxml = buffer[_xml_start-_xml_offset..(_xml_end-_xml_offset-1)];
	
	P2(("innerxml %O\n", innerxml))
	funcall(nodeHandler, _node);
	_node = 0;
	buffer = "";
	innerxml = 0;
    }
}

void xml_onText(string data) {
    if (!_node)
	return; // whitespace usually
    if (_node[Cdata]) 
	_node[Cdata] += data;
    else
	_node[Cdata] = data;
}

feed(a) {
	int d;

	if (!_node && a == "\n") {
	    /* whitespace ping */
	} else {
	    buffer += a;
	    d = expat_parse(a);
	    if (!d) {
		mixed e = expat_last_error();
		PT(("parse error while parsing %O:\n%O\n", a, e))
	    }
	    // TODO: error handling muss hier her
	}
	/*
#if __EFUN_DEFINED__(convert_charset) && SYSTEM_CHARSET != "UTF-8"
		if (catch(a = convert_charset(buffer[0..pos - 1],
				      "UTF-8", SYSTEM_CHARSET); nolog)) {
			P1(("catch! iconv %O in %O\n", a, ME))
			//QUIT
			a = buffer; // let's give it a try
		}
		xmlparse(a);
#else
		xmlparse(buffer[0..pos - 1]);
#endif
*/
#ifdef INPUT_NO_TELNET
        input_to(#'feed, INPUT_IGNORE_BANG | INPUT_CHARMODE | INPUT_NO_TELNET);
#else
	input_to(#'feed, INPUT_IGNORE_BANG | INPUT_CHARMODE);
#endif
}
