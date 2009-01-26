#include <net.h>
#include <proto.h>

// pike code can be made to run in lpc and the other way round..
#include "jsonparser.pike"

// see http://about.psyc.eu/Serialisation on the merits of JSON
//
// this function is useful if we decide to use json for complex data
// (or rather cannot come up with something better)
//
string make_json(mixed d) {
	if (stringp(d)) {
#if 1
		string *x;

		// this code runs on the presumption that the regexplode
		// will hardly ever encounter anything. should these chars
		// be commonplace, then the 7 regreplace approach would be
		// more efficient.
		x = regexplode(d, "(\"|\\\\|\n|\t|\b|\r|\f)");
		if (sizeof(x) > 1) {
		    int i;

		    for (i=sizeof(x)-2; i >= 0; i-=2) {
			switch(x[i]) {
			case "\"": x[i] = "\\\""; break; // common stuff
			case "\n": x[i] = "\\n"; break;
			case "\t": x[i] = "\\t"; break;

			case "\b": x[i] = "\\b"; break;	// these chars should
			case "\r": x[i] = "\\r"; break; // not appear in psyc
			case "\f": x[i] = "\\f"; break; // strings anyway.. !?
			case "\\": x[i] = "\\\\"; break;
			default:
			    raise_error(S("encountered %O in %O\n", x[i], d));
			}
			P4((" .. encountered %O", x[i]));
		    }
		    P4((" .. result: %O\n", d))
		}
		return "\""+ implode(x, "") +"\"";
#else
    // what i don't like about this thing is 7 calls of regreplace
    // for every damn little string.
		d = regreplace(d, "\n", "\\n", 1);
		d = regreplace(d, "\t", "\\t", 1);
		d = regreplace(d, "\b", "\\b", 1);
		d = regreplace(d, "\r", "\\r", 1);
		d = regreplace(d, "\f", "\\f", 1);
		//d = regreplace(d, "\u", "\\u", 1);	// unicode..???
		d = regreplace(d, "\"", "\\\"", 1);
		d = regreplace(d, "\\\\", "\\\\\\\\", 1);
		return "\""+ d +"\"";
#endif
	} else if (intp(d)) {
		return to_string(d);    
	} else if (pointerp(d)) {
		return "[ " + implode(map(d, #'make_json), ", ") + " ]";
	} else if (mappingp(d)) {
#if 0
		return "{ " + implode(map(d, 
		      (: return $1 + " : " + make_json($2[0]); :)), 
				  ", ") + " }";
#else
//		mixed x = map(d,
//		    (: return make_json($1) +" : "+ make_json($2); :));
		// let's presume a key of type string doesnt contain evil chars
		mixed x = map(d,
		    (: return (stringp($1) ? ("\""+ $1 +"\"") : make_json($1))
				     +":"+ make_json($2); :));
		P3(("\nintermediate: %O\n", m_values(x)))
		return "{"+ implode(m_values(x), ",") +"}";
#endif
	}
}

// see also net/place/storic.c for a JSON generator limited and optimized
// to the output of HTML-quoted strings.
