// $Id: textc.c,v 1.32 2008/04/11 18:48:26 lynx Exp $ // vim:syntax=lpc
//
// text database client

#include <net.h>
#include <lang.h>

volatile mixed _tob;
volatile mapping _tdb;
volatile string _tpath;

#define NO_INHERIT
#include <text.h>
#undef NO_INHERIT

#ifdef HTFORWARD
virtual inherit NET_PATH "outputb";
#else
virtual inherit NET_PATH "output";
#endif

#ifndef T
// usually defined in text.h
T(mc, fmt) {
	D(S("T(%O,%O) in %O: tob=%O tdb=%O\n", mc, fmt, ME, _tob, _tdb));
	return (_tob && member(_tdb, mc) ? _tdb[mc] : getText(mc, fmt));
}
#endif

// also used for pplframe
setText(string mc, string fmt) {
	if (_tdb) {
		_tdb[mc] = fmt;
		return 1;
	} else
	    return 0;
}

getText(string mc, string fmt) {
	P4(("getText(%O,%O) in %O: tob=%O tdb=%O tpa=%O\n",
	     mc, fmt, ME, _tob, _tdb, _tpath))
	P3(("Ask %O in getText(%O,%O)\n", _tob, mc, fmt))
	unless (objectp(_tob)) {
		if (_tob) {
			PT(("getText: db inactive for %O looking up %O\n",
			    ME, mc))
			return fmt; // deactivated
		}
		unless (_tpath) {
			_tob = "deactivated";
			P1(("getText: db deactivated for %O looking up %O\n",
			    ME, mc))
			return fmt;
		}
		// _tpath -> sPath(_tpath);
		_tpath -> load();
//	P0(("[*] getText(%O,%O) in %O: tob=%O tdb=%O tpa=%O\n",
//	     mc, fmt, ME, _tob, _tdb, _tpath))
		_tob = find_object(_tpath);

		// this obtains a shared copy of the text db
		_tdb = _tob -> link();
//		P0(("TEXTC in %O: tob=%O tdb=%O\n", ME, _tob, _tdb))
		if (member(_tdb, mc)) return _tdb[mc];
	}
#if DEBUG > 1
	if (member(_tdb, mc))
	     D(S("getText(%O,%O) has it!? in %O: o=%O t=xO !?\n",
		mc, fmt, ME, _tob, _tdb));
#endif
	return _tob -> lookup(mc, fmt);
	// urn member(_tdb, mc) ? _tdb[mc] : (_tob -> lookup(mc, fmt));
}

// new sTextPath function
localize(lang, scheme) {
	return sTextPath(
#ifdef PRO_PATH
		    get_layout(ME),
#else
		    0,
#endif
			lang, scheme || ME->qScheme());
}

sTextPath(layout, lang, scheme) {
	string p;

	P2(("sTextPath(%O,%O,%O) in %O\n", layout,lang,scheme, ME))
	unless (layout) layout = "default";
	// if (lang && index( ({ LANGUAGES }), lang) < 0) lang = 0;
	unless (lang) lang = DEFLANG;
	switch(scheme) {
	case "ht": case "html": case "wml":
	case "irc": case "ircgate":
	case "jabber":
	case "simple":
		// these have their own databases
		break;
	default:
		// optimization: several protocols use the plain db anyway
		scheme = "plain";
	}
	p = layout +"/"+ lang +"/"+ scheme +"/text";
	if (p = sTextPath2(p)) {
		setText("_VAR_layout", layout);
		setText("_VAR_lang", lang);
		setText("_VAR_scheme", scheme);
	}
	return p;
}

sTextPath2(p) {
	if (p != _tpath) {
#ifdef TEXT_PATH
		_tpath = TEXT_PATH + p;
#else
		_tpath = p;
#endif
		// make sure the new textdb is actually going to be loaded
		_tob = 0;
		P3(("sTextPath(%O) in %O\n", _tpath, ME))
	}
	D2(else D(S("sTextPath(%O) again in %O\n", _tpath, ME));)
	return p;
}

// pr() should replace p(), P() and print()
// using the PSYC "message code" (actually a PSYC method)
// for classification of output going to the user
// (remember the LASTLOG_LEVELs in ircII?)
//
// this is the default pr() function to be
// overridden when protocol needs require it
//
pr(mc, fmt, a,b,c,d,e,f,g,h,i,j,k) {
	unless (fmt = T(mc, fmt)) return;
	printf(fmt, a,b,c,d,e,f,g,h,i,j,k); // should use emit?
	return 1;
}

// used by net/server mostly
// this function does not perform any convert_charset!
w(string mc, string data, mapping vars, mixed source) {
	string template = T(mc, "");
	string output = psyctext(template, vars, data, source);

#ifdef PREFIXES
	if (template == "") output += abbrev("_prefix", mc) ? " " : "\n";
#endif
	//PT(("textc:w(%O,%O,%O,%O) - %O\n", mc,data,vars,source, template))

       	// used by net/server.c - it needs emit() here
	if (interactive(ME)) emit(output);
	else {
#if 0 //def DEVELOPMENT	--- TODO TODO TODO
	    raise_error("please rewrite this code to not depend on write()\n");
#else
	    // also used by htget in net/place/owned and it needs write() here
	    // but this fails dramatically if the event is coming from a psyc
	    // or jabber socket - big bang boom. so we gotta stop this practice
	    write(output);
#endif
	}
	return 1;
}

qTob() { return _tob; }
