// $Id: uniform.c,v 1.43 2008/03/29 20:36:43 lynx Exp $ // vim:syntax=lpc
//
// URLs.. URIs.. UNLs.. UNIs.. maybe even URNs..
// the fact they wear a uniform is the only thing these items have in common
// after all.. they are not always resources, not always locators,
// not always identificators.. but one thing is for sure, they have a
// common format.. the uniform :)
//
// TODO: first move everything called _uniform or url somewhere into here
// then rename everything into uniform, also uniform.c and uniform.h..

#include <net.h>
#include <uniform.h>

string legal_url(string url, string scheme) {
	if (scheme &&! abbrev(scheme+":", url)) return 0;
	if (index(url, '"') >= 0) return 0;
	if (index(url, ' ') >= 0) return 0;	// just
	if (index(url, '\t') >= 0) return 0;	// paranoid
	return url;
}

/** pass it a URL string and it will find out if that string
 ** is a uniform, and if so return an array as defined in uniform.h
 ** 
 ** <fippo> what about using 
 ** 	http://www.gbiv.com/protocols/uri/rfc/rfc3986.html#regexp ?
 ** <lynX> sweet, i really like that. can we do that?
 **	only we have to change that regexp a lot. like we don't
 **	need $1, $6 and $8, we need USlashes instead of $3, but
 **	what's tough is, we need UQuery to support ; or ? and
 **	we need UUser, UPass, UPort, UTransport, and UHostPort.
 **	all of that isn't in that regexp yet...
 */
varargs array(mixed) parse_uniform(string url, vaint tolerant) {
	array(string) u = allocate(USize);
	string t;
	string s;

	u[UString] = url;
	if (sscanf(url, "%s:%s", u[UScheme], t) != 2) {
		if (tolerant) t = url;
		else return 0;
	}
	ASSERT("parse_uniform w/out scheme", u[UScheme] || tolerant, url)
	P3(("parse_uniform %s of %O (tolerant: %O)\n",
	    url, u[UScheme], tolerant))
	if (abbrev("//", t)) {
		t = t[2..];
		u[USlashes] = "//";
	} else u[USlashes] = "";
	switch(u[UScheme]) {
	case "sip":
		sscanf(t, "%s;%s", t, u[UQuery]);
		break;
	case "telnet":
		break;
	case "psyc":
		sscanf(t, "%s#%s", t, u[UChannel]);
		break;
#if 0 //def MUCSUC
	case "xmpp":
		sscanf(t, "%s#%s", t, u[UChannel]);
		break;
#endif
	//case "mailto":
	default:
		sscanf(t, "%s?%s", t, u[UQuery]);
	}
	u[UBody] = t;
	sscanf(t, "%s/%s", t, u[UResource]);
#if 0
//	int n;
	if (-1 != (n = member(u[UResource], '#'))) {
	    u[UChannel] = u[UResource][n+1..]; // strlen checken?? 
	}
#endif
	u[UUserAtHost] = t;
	if (sscanf(t, "%s@%s", s, t)) {
		unless (sscanf(s, "%s:%s", u[UUser], u[UPass]))
		    u[UUser] = s;
	}
	u[UHostPort] = t;
//	if (complete) u[UCircuit] = u[UScheme]+":"+u[UHostPort];
	u[URoot] = u[UScheme]+":"+u[USlashes]+u[UHostPort];
	if (sscanf(t, "%s:%s", t, s)) {
		unless (sscanf(s, "%d%s", u[UPort], u[UTransport]))
		    u[UTransport] = s;
		unless (strlen(u[UTransport])) u[UTransport] = 0;
	}
	u[UHost] = t;
	P4(("parse_uniform %s = %O (tolerant: %O)\n", url, u, tolerant))
	u[UNick] = u[UUser]
		    || (strlen(u[UResource]) && u[UResource][1 ..]);
		    // || u[UBody]; -- not so good
	return u;
}

string render_uniform(array(mixed) u) {
	string s, t;
// wird aufgerufen wenn dieser string nicht mehr gültig ist:
//	if (u[UString]) return u[UString];
	unless (s = u[UHost]) return 0;
	if (u[UUser]) s = u[UPass] ? (u[UUser]+":"+u[UPass]+"@"+s)
					 : (u[UUser]+"@"+s);
	if (u[UScheme]) s = u[UScheme]+"://"+s;
	t = u[UPort] ? to_string(u[UPort]) : "";
	if (u[UTransport]) t += u[UTransport];
	if (t != "") s += ":"+t+"/";
	if (u[UResource]) s += u[UResource];
	if (u[UChannel]) s += "#"+ u[UChannel];
	if (u[UQuery]) s += "?"+ u[UQuery];
	D2( if (u[UString] == s) D("render_uniform: das war umsonst..\n"); )
	P3(("render_uniform %O = %s\n", u, s))
	return u[UString] = s;
}

// convert GENERIC psyc url stringp()s to objects (if local)
#if 0
mixed urlobject(string url) {
    array(mixed) u; 
    object o;
    
    // we avoid errors! we rule!
    u = parse_uniform(url);
    unless (query_udp_port() == (u[UPort] || PSYC_PORT))
	    return url;
    unless (u[UHost] == __HOST_IP_NUMBER__ 
	|| lower_case(u[UHost]) == lower_case(SERVER_HOST))
	    return url;

    if (u[UResource][0] == '~')
	o = find_person(u[UResource][1..]);
    else if (u[UUser]) o = find_person(u[UUser]);
    else o = find_object(u[UResource]);
    
    unless(objectp(o)) return url;
    return o;
}
#endif

