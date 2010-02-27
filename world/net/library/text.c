// $Id: text.c,v 1.21 2008/07/17 17:23:25 lynx Exp $ // vim:syntax=lpc
//
// the marvellous psyctext() function, fundamental of any PSYC implementation.
//
// this function does PSYC variable replacement for error messages
// and informational notices as described in the internet drafts
//
// this code performs okay, but i would rather have it written in C and
// linked to the driver. see also CHANGESTODO for thoughts on how to do a
// generic template replacement C function in a way that it does psyctext()
// effectively, but can also be used for other syntaxes.

// local debug messages - turn them on by using psyclpc -DDpsyctext=<level>
#ifdef Dpsyctext
# undef DEBUG
# define DEBUG Dpsyctext
#endif

#include <net.h>
#include <proto.h>

varargs string psyctext(string s, mapping m, vastring data,
			vamixed source, vastring nick) {
	string r, p, q, v;

#if DEBUG > 2
	PT(("psyctext(%O, .., %O, %O, %O) %O\n", s, data, source, nick, m))
#else
	P2(("psyctext(%O, .., %O, %O, %O)\n", s, data, source, nick))
#endif
	unless(s) return "";
	if (s == "") {
		if (data) s = data;
		else return 0;
	}
#if 0
	ASSERT("psyctext-mapping for "+ to_string(s),
	       mappingp(m) && widthof(m) == 1, m)
#else
	// easy on missing mapping
	unless (mappingp(m)) return s;
#endif
	r="";
	while (sscanf(s, "%s[%s]%s", p, v, s) && v) switch(v) {
	case "_data":
		r += p + (data || "");
		break;
	case "_nick":
#if 1 //def USE_THE_NICK
		r += p + (nick || m["_nick"] || "?");
#else
// doesn't work for "wax enters psyc://ve.symlynx.com/@Wax."
		// _nick can mean _source_relay instead of physical source
		// and in some dirty cases we do not even provide _source_relay
		q = m["_source_relay"] || m["_source"];
		PT(("trying %O for _nick\n", q))
		unless (q) {
			// so we are forced to use the _nick from the message
			q = m["_nick"] || nick;
			if (q) {
				r += p + q;
				break;
			}
			// no _nick? okay, then it has to be this one
			q = UNIFORM(source) || "?";
		}
		PT(("trying %O for _nick\n", q))
		if (previous_object())
		    q = previous_object()->uni2nick(q, m) || q;
		PT(("using %O for _nick\n", q))
		r += p + q;
#endif
		break;
	case "_source":
		// should this support _source_relay? var inheritance!
#ifdef USE_THE_NICK_TOO_MUCH
		r += p + (nick || m["_nick"] || m["_source"]
			       || UNIFORM(source) || "?");
#else
		q = m["_source"] || UNIFORM(source) || "?";
		if (previous_object())
		    q = previous_object()->uni2nick(q, m) || q;
		r += p + q;
#endif
		break;
	case "_source_relay":
#ifdef USE_THE_NICK_TOO_MUCH
		r += p + (nick || m["_nick"] || m["_source_relay"] || "?");
#else
		q = m["_source_relay"] || "?";
		if (previous_object())
		    q = previous_object()->uni2nick(q, m) || q;
		r += p + q;
#endif
		break;
	case "_target":
#ifdef USE_THE_NICK_TOO_MUCH
		r += p + (m["_nick_target"] || m["_target"] || "?");
#else
		q = m["_target"] || "?";
		if (previous_object())
		    q = previous_object()->uni2nick(q, m) || q;
		r += p + q;
#endif
		break;
	case "_context":
#ifdef USE_THE_NICK_TOO_MUCH
		r += p + (m["_nick_place"] || m["_context"] || "?");
#else
		q = m["_context"] || "?";
		if (previous_object())
		    q = previous_object()->uni2nick(q, m) || q;
		r += p + q;
#endif
		break;
	default:
		unless (member(m, v)) {
#ifdef _flag_debug_unresolved_psyctext
			raise_error(v +" unresolved in psyctext format\n");
#else
			PT(("psyctext: ignoring [%s] in %O for %O\n", v,
			    data, previous_object()))
#endif
			r += p + "["+v+"]";	// pretend we havent seen it
		}
		else if (stringp(m[v])) r += p + m[v];
		else if (pointerp(m[v])) {	// used by showMembers
			r += p + implode(m[v], ", ");
		}
		// if (member(m,v) && m[v]) r += p + m[v];
		else if (intp(m[v])) {		// used by /lu and /edit
			if (v == "_time_idle")
			    r += p + timedelta(m[v]);
			else if (abbrev("_time", v))	// should be _date
			    r += p + time_or_date(m[v]);
			else
			    r += p + to_string(m[v]);
		}
		else if (mappingp(m[v])) {	// made by psyc/parse
			r += p + implode(m_indices(m[v]), ", ");
		}
		else if (objectp(m[v])) r += p + psyc_name(m[v]);
		else {
			// in theory at this point we could perform
			// an inheritance search, but that's overkill.
			// instead we just bail out. for the psyc standard
			// on psyctext we will have to decide which option
			// is right: simplicity vs inheritance power. ouch.
			D1( if (v[0] == '_')
			    D(S("psyctext warning: invalid %s=%O for %O\n",
				v, m[v], previous_object() || ME)); )
			r += p + "["+v+"]";	// pretend we havent seen it
		}
	}
	if (s != "") r += s;
	P4(("\t-> %s\n", r))
	return r;
}

#if 0
// this doesn't help detect a problem.. this MAKES the problem!
// when the library offers a function of the same name as a method
// defined in an inheriting class, then the inherited class will
// execute the library function instead of finding the method in
// the inheriting class. so when you #if 1 this, you will see that
// person.c:w() resolves here instead of going into user.c:w().
// evil!!  -lynX
//
varargs void w(string mc, string data, mixed vars) {
	raise_error(sprintf("%O ended up in library/text:w(%s) for %O\n",
	    previous_object(), mc, this_interactive()));
}
#endif

// a simple implementation of perl's x operator
string x(string str, int n) {
    int i;
    string res = "";
    for (i = 0; i < n; i++) res += str;
    return res;
}
