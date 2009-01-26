// $Id: text.c,v 1.17 2008/04/11 10:37:25 lynx Exp $ // vim:syntax=lpc
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
//
#include <net.h>
#include <proto.h>

varargs string psyctext(string s, mapping m, vastring data,
			vamixed source, vastring nick) {
	string r, p, q, v;

	P3(("psyctext(%O)\n", s))
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
	while (sscanf(s, "%s[%s]%s", p, v, s) && v) {
		if (v == "_nick") r += p + (nick || m["_nick"]);
		else if (v == "_data") r += p + (data || "");
		else unless (member(m, v)) {
		    if (v == "_source") r += p + to_string(source);
		    else {
			PT(("psyctext: ignoring [%s]\n", v))
			r += p + "["+v+"]";	// pretend we havent seen it
		    }
		}
		else if (stringp(m[v])) r += p + m[v];
		else if (pointerp(m[v])) {	// used by showMembers
			r += p + implode(m[v], ", ");
		}
		// if (member(m,v) && m[v]) r += p + m[v];
		else if (intp(m[v])) {		// used by /lu and /edit
			if (v == "_time_idle") r += p + timedelta(m[v]);
			else if (abbrev("_time", v)) {
				// verrry similar code in net/user.c
				if (time() - m[v] > 24*60*60)
				    r += p + isotime( m[v], 0 );
				else
				    r += p + hhmmss(ctime( m[v] ));
				 // r += p + hhmm(ctime(m[v]));
			} else
			    r += p + to_string(m[v]);
		}
		else if (mappingp(m[v])) {	// made by psyc/parse
			r += p + implode(m_indices(m[v]), ", ");
		}
		// used in rare cases where _identification is output
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
	// oh my god seit wann gibt's denn das!?
	raise_error(sprintf("%O ended up in library/text:w(%s) for %O\n",
	    previous_object(), mc, this_interactive()));
}
#endif

