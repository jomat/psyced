#include <net.h>
#include <xml.h>

#include "profiles.i"

// convert a setting name from source format to target format, null == psyc
varargs string convert_setting(string name, string sf, string tf) {
	switch (sf) {
	case "set":
		name = set2psyc[name];
		break;
	case "jCard":
		name = jCard2psyc[name];
		break;
	case "vCard":
		name = vCard2psyc[name];
		break;
	case "LDAP":
		name = ldap2psyc[name];
		break;
	case 0:
		break;
	default:
		return 0;
	}
	unless (name) return 0;

	switch (tf) {
	case "set":
		name = psyc2set[name];
		break;
	case "jProf":
		name = psyc2jProf[name];
		break;
	case "jCard":
		name = psyc2jCard[name];
		break;
	case "vCard":
		name = psyc2vCard[name];
		break;
	case "LDAP":
		name = psyc2ldap[name];
		break;
	case 0:
		break;
	default:
		return 0;
	}
	return name;
}

// convert a profile from source format to target format, null == psyc
varargs mixed convert_profile(mixed p, string sf, string tf) {
	string k, vc, t, t2;
	XMLNode n, n2;
	mixed val;
	mapping vars = ([ ]);

// could also create arrays, but not sure if that's useful to anyone
#define append(v, nu) if (stringp(nu)) v = stringp(v) ? (v +" "+ nu) : nu;

	// first convert source format to psyc vars
	switch (sf) {
	case "jCard":
	    P3(("debug jCard2psyc with p = %O\n", p))
	    foreach( k,vc : jCard2psyc) {
		if (sscanf(k, "%s/%s", t, t2) && (n = p["/" + t])
		    && !nodelistp(n) && (n2 = n["/" + t2])) {
		    append(vars[vc], n2[Cdata]);
		} else if ((n = p["/" + k]) && !nodelistp(n)) {
		    append(vars[vc], n[Cdata]);
		}
	    }
	    break;
	case "set":
	    foreach( k,val : p ) {
		    t = set2psyc[k];
		    if (t) vars[t] = val;
		    else D(k +" can't be set2psyc? never mind.\n");
	    }
	    break;
	default:	// no source format, p contains the vars
	    vars = p;
	    break;
	}

	// then convert psyc vars to target format
	switch (tf) {
	case "jCard":
	    // am i allowed to add newlines?
	    t = "<vCard xmlns='vcard-temp'>";
	    foreach( vc,k : psyc2jCard) {
		// should be able to render ints too meguess
		if (stringp(vars[vc])) t += sprintf(k, vars[vc]);
	    }
	    return t + "</vCard>";
	case "jProf":
	    // see discussion in wiki:
	    // http://about.psyc.eu/Jabber#JEP_0154
	    t = "<field var='FORM_TYPE' type='hidden'>"
                "<value>http://jabber.org/protocol/profile</value>"
                "</field>";
	    foreach( vc,k : psyc2jProf) t += "<field var='"+ k +"'><value>"
					  + vars[vc] +"</value></field>";
	    return t;	// no wrapper?
	case "set":
	    p = ([]);
	    foreach( k,val : vars ) {
		    t = psyc2set[k];
		    if (t) p[t] = val;
		    else D(k +" can't be psyc2set? never mind.\n");
	    }
	    return p;
	default:	// no target format, return the psyc vars
	    return vars;
	}

	// other cases not supported yet
	raise_error("cannot convert from "+sf+" to "+tf+" (yet)\n");
	return 0;
}
