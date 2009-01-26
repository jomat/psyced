// $Id: text.c,v 1.92 2008/12/01 11:31:32 lynx Exp $ // vim:syntax=lpc
//
// text database handler
//
// cloned on demand by compile_object in master.c
// accessed by all sorts of items using the textc.c

#ifdef Dtext
# undef DEBUG
# define DEBUG Dtext
#endif

#include <net.h>
#include <lang.h>
//#include <files.h>

// textcache currently causes some minor bugs
// so we disable it until one day someone cares to fix it
#ifndef TEXTCACHE
# define NOTEXTCACHE
#endif

#ifndef FILESYSTEM_CHARSET
# define FILESYSTEM_CHARSET SYSTEM_CHARSET
//# define FILESYSTEM_CHARSET "ISO-8859-15"
#endif

inherit NET_PATH "storage";

volatile string path;
volatile object* tob;
#ifdef NOTEXTCACHE
volatile mapping fs;
#endif

link() { return _v; }

#ifdef EXPERIMENTAL	// not being used anywhere currently
renderDB() {
	string k, t;
	string out = "<PSYC:TEXTDB> ## vim:syntax=mail\n";

	foreach(k : sort_array(m_indices(fs), #'>)) {
		t = fs[k];
		if (t) t = chop(t);
		else {
			P1(("renderDB: skipping %O\n", k))
			continue;	    // entry marked non-existing
		}

		out += "\n"+ k +"\n|"+
		       implode(explode(t, "\n"), "\n|")
		       +"\n";
	}
	return out;
}
#endif

#ifndef NO_TEXTDB_FILES
parseDB(string in, string filename) {
	string e, mc;

	P2(("%O parseDB %s\n", ME, filename))
	unless (sscanf(in, "<PSYC:TEXTDB>%s\n\n%s", e, in))
	    raise_error("garbage in "+ filename +" header\n");
	// comments are permitted after the file tag until the
	// empty line. please prefix them with ## even if there
	// is no check for that here. there are also per-entry
	// comments, but they have to be indented like the rest.
	D3( if (strlen(e)) PP(("%O: %s comment:%s\n", ME, filename, e)); )
# ifdef NOTEXTCACHE
	in = explode(in, "\n\n");
	foreach(e : in) {
	    if (sscanf(e, "%s\n|%s", mc, e)) {
# if DEBUG > 0
		string t;
		array(string) a = explode(e, "|\n");

		if (sizeof(a) > 1) foreach(t : a[1..])
		    if (strlen(t) && t[0] != '|') {
			P0(("ERROR in %s: | before mc after "
			    "%s\n", filename, mc))
		}
# endif
# ifdef NEW_LINE
# echo NEW_LINE currently doesn't work as it breaks the ## comment syntax
		// Odd server stuff: "# shut up"
		// should i pursue fixing NEW_LINE or keep the trailcutting
		// business..? since only net/jabber does the chomp() it's not
		// a big deal.. however NEW_LINE is the way to ensure that
		// psyced does not modify outgoing data that *intentionally*
		// had an extra trailing newline! so at some point we should
		// probably get this stuff working.  TODO
		fs[mc] = replace(e, "\n|", "\n");
# else
		fs[mc] = replace(e, "\n|", "\n") + "\n";
#endif
	    } else unless(e == "") {
		// allow one trailing newline in database format
		P0(("ERROR in %s after %O: %O...\n", filename, mc,
		    strlen(e) > 23? e[.. 23] : e))
	    }
	    P4(("parseDB %O into %O for %O\n", e, fs[mc], mc))
	}
	return ME;
# else
	PT(("Thanx for the "+ filename +" but I can't use it.\n"))
	return 0;
# endif
}
#endif

sPath(pa) {
	array(string) p;

#ifdef NOTEXTCACHE
	fs = ([]);
#endif
	if (pa) {
#ifndef NO_TEXTDB_FILES
	    string in, filename;

	    filename = pa +".textdb";
	    if (in = read_file(filename)) {
# ifndef NO_SMART_MSA
		// ok, it was a stupid idea to think the textdb will
		// never need to look at parts of the path.. the alternative
		// would be to find an ascii character that is never needed
		// in any textdb format. haha. impossible.
		if (in && strstr(pa, "/irc") > 1) for (int i = strstr(in, "%");
			     i >= 0; i = strstr(in, "%")) {
		    in[i] = 0x01; // this used to be in net/irc/user, but
				  // it didn't destinguish between data
				  // and template. and replacing the %s
				  // once for the whole textdb is better
				  // than both solutions before
		}
# endif
# if SYSTEM_CHARSET != FILESYSTEM_CHARSET
		P2(("SYSTEM_CHARSET %O, FILESYSTEM_CHARSET %O\n",
		    SYSTEM_CHARSET, FILESYSTEM_CHARSET))
		iconv(in, FILESYSTEM_CHARSET, SYSTEM_CHARSET);
# endif
		parseDB(in, filename);
	    }
	    D2( else P2(("Why don't you make me a neat "+ filename +" ?\n")) )
#endif
	    path = pa + "/";
	}
	p = explode(path, "/");
	if (sizeof(p) != 4 || strlen(p[1]) < 2) {
		tob = 0;
	} else {
		string la1, la2, la3, heritage;

                if (strlen(p[1]) > 2) {
                    // support for sprachvariationen
                    la2 = p[1][0..1];
                    if (la2 != "en") la3 = "en";
                }
                else {
                    if (p[1] != "en") la3 = "en";
                }
                tob = ({});

#define SUMMON_LANGUAGES(LAY, SCM) \
    if (la1) tob += ({ summon(LAY, la1, SCM) }); \
    if (la2) tob += ({ summon(LAY, la2, SCM) }); \
    if (la3) tob += ({ summon(LAY, la3, SCM) }); \

                SUMMON_LANGUAGES(p[0], p[2])
                la1 = p[1];
                if (p[0] != "default") {
                        // new database inheritance feature, no nesting allowed
                    heritage = lookup("_INHERIT", "");
                    if (strlen(heritage) > 1) {
                        D1(D("DB INHERIT: "+heritage+" for "+pa+".\n");)
                        SUMMON_LANGUAGES(heritage, p[2])
                    }
                    else heritage = 0;
                    SUMMON_LANGUAGES("default", p[2])
                }
                if (p[2] != "plain") {
                    SUMMON_LANGUAGES(p[0], "plain")
                    if (heritage) {
                        D1(D("DB INHERIT: "+heritage+"/plain for "+pa+".\n");)
                        SUMMON_LANGUAGES(heritage, "plain")
                    }
                    if (p[0] != "default") {
                        // la3 = 0; // default/en/plain is the default anyway
                        SUMMON_LANGUAGES("default", "plain")
                    }
                }
	}
	P4(("sPath: %O - tob %O\n", path, tob))
}

summon(layout, lang, scheme) {
	string t;
	t = layout+"/"+lang+"/"+scheme+"/text";
	D3(D("summon "+t+"\n");)
	t -> load();
	return find_object(t);
}

lookup(string mc, mixed fmt, object ghost, object curse) {
	string file, in;
#ifdef NOTEXTCACHE
	int dont_cache = 0;
#endif /* NOTEXTCACHE */

	// this would return some other database's data, too
	//
	//if (ghost && _v[mc]) return _v[mc];
	//
	// we *must* load the file anew, also to make sure any
	// recursions get handled correctly!

	P4(("DB %O = %O lookup(%O, %O, %O)\n", ME, path, mc, fmt, ghost))
	// this happens at init time, especially for _INHERIT .. problem?
	// DB net/text#146 != "LynX/en/plain/" lookup("_INHERIT", "", 0)
	D2( if (path && object_name(ME) != path+"text")
	    PP(("DB %O != %O lookup(%O, %O, %O)\n", ME, path, mc,fmt,ghost)); )
#ifdef NOTEXTCACHE //rudimentary fs cache here. just prevents useless
		   // disk reads.
	if (member(fs, mc)) {
	    in = fs[mc];
	} else {
#ifndef NO_TEXTDB_FMT_FILES
	    file = path + replace(mc[1..], "_", "/") + ".fmt";
	    in = read_file(file);
# if SYSTEM_CHARSET != FILESYSTEM_CHARSET
	    if (in && strlen(in)) {
		P0(("Warning: Expensive convert_charset for %s\n", file))
		iconv(in, FILESYSTEM_CHARSET, SYSTEM_CHARSET);
	    }
# endif
	    fs[mc] = in;
#else
	    P4(("DB: No template for %s: %s\n", path, mc));
# if DEBUG > 0
	    // this warns us, when we forgot to integrate a .fmt file
	    file = path + replace(mc[1..], "_", "/") + ".fmt";
	    if (in = read_file(file))
		PP(("Warning: Unused fmt in %s: %O\n", file, in));
# endif
	    in = 0;
#endif
	}
#else
//	psyc = mc[0] == 'x';
//	file = path + replace(mc[1+psyc..], "_", "/"); // + ".fmt";
//	file += psyc ? ".psyc" : ".fmt";
	file = path + replace(mc[1..], "_", "/") + ".fmt";
	in = read_file(file);
#endif
	if (in) {
		if (in == "\n" || in == "") {
			// request for "no output"
			P2(("DB %O: no output for %O\n", ME, mc || file))
			in = 0;
		} else {
			string before, code, after;

			// new: data base comment syntax, '##' at start of line
			while(sscanf(in, "##%s\n%s", code,in)) { };
			while(sscanf(in, "%s\n##%s\n%s", before,code,after)) {
				in = before +"\n"+ after;
			}

			while(sscanf(in, "%s{_%s}%s", before, code, after)
			    && code) switch(code) {
#ifdef IMG_URL
			case "URL_img":
				in = before + IMG_URL + after;
				break;
#endif
#ifdef APP_URL
			case "URL_applet":
				in = before + APP_URL + after;
				break;
#endif
#ifdef SYSTEM_CHARSET
			case "VAR_charset":
				in = before + SYSTEM_CHARSET + after;
				break;
#endif
#ifdef PRO_PATH
			case "VAR_layout":
				in = before + get_layout(ghost||path) + after;
				break;
#endif
#ifdef CHATNAME
			case "VAR_host":
				in = before + CHATNAME + after;
				break;
#endif
#ifdef SERVER_VERSION
			// not yet in use anywhere
			case "VAR_server_version":
				in = before + SERVER_VERSION + after;
				break;
#endif
#ifdef SERVER_VERSION_MINOR
			// even less in use anywhere
			case "VAR_server_version_minor":
				in = before + SERVER_VERSION_MINOR + after;
				break;
#endif
			case "VAR_provider":
#ifndef PROVIDER_NAME
#define PROVIDER_NAME "symlynX communications"
#endif
				in = before + PROVIDER_NAME + after;
				break;
			case "VAR_server":
#ifndef SERVER_HOST
#define SERVER_HOST __HOST_IP_NUMBER__	// never happens
#endif
#ifdef _host_XMPP
				in = before + SERVER_HOST + after;
				break;
#else
#define _host_XMPP SERVER_HOST
#endif
			case "VAR_server_XMPP":
				in = before + _host_XMPP + after;
				break;
			case "VAR_server_uniform":
				in = before + query_server_unl() + after;
				break;
			case "VAR_method":
				PT(("using %O for %O\n", mc, code))
				code = mc[1..];
				// fall thru
			default:
#ifdef NOTEXTCACHE
				dont_cache = 1;
#endif /* NOTEXTCACHE */
				// when recursive use default text
				if (mc[1..] == code) {
					if (curse == ME) {
					    P2(("%O ran into own curse.\n",
						ME))
					    return;
					}
					// let's curse the current object.
					// this ensures that the correct order
					// of inheritance is maintained.
					if (ghost) in = ghost-> lookup(mc,
					    fmt, 0, ME);
					else in = lookup(mc, fmt, 0, ME);
				}
				//
				// kinda server-side includes here..  ;)
				//
				// they need to start at the top of
				// the database hierarchy again
				else
				if (ghost) in= ghost-> lookup("_"+code);
				else in= lookup("_"+code);
				if (!in || !strlen(in)) in = before + after;
				else in = before + chomp(in) + after;
			}
			if (in == "" || in == "\n") {
				// request for "no output" II
				P2(("DB %O: late no output for %O\n",
				    ME, mc || file))
				in = 0;
			}
		}

#ifdef NO_TEXTDB_FILES
# ifndef NO_SMART_MSA
#  echo Warning: % characters will be converted to msa
		if (in) for (int i = strstr(in, "%");
			     i >= 0; i = strstr(in, "%")) {
		    in[i] = 0x01; // this used to be in net/irc/user, but
				  // it didn't destinguish between data
				  // and template. and having the msa char
				  // in the template is nicer than replacing
				  // it every time (... we should fix
				  // the textcache ...).
		}
# endif
#endif
#ifndef NOTEXTCACHE
		fmt = _v[mc] = in;
#else /* NOTEXTCACHE */
		fmt = in;
#endif /* NOTEXTCACHE */
		P2(("DB %O: got %O\n", ME, mc || file))
	} else {
		int i;

		if (curse) {
			P2(("recursive: %s in %O for %O\n",
				   mc, ME, curse))
		}
		// held re-enabled this line.
		if (ghost) return -1;
		in = -1;
		for (i=0; i < sizeof(tob); i++) {
			unless (tob[i]) {
			    sPath();
			    unless (tob[i])
				raise_error("Unable to rehash my databases\n");
			    PT(("Textdb rehashed for %O\n", ME))
			}
			// the missing 'curse' treatment may also be the reason
			// why the textcache was no longer working
                        if (curse != tob[i])
			    in = tob[i] -> lookup(mc, fmt, ME, curse);
			if (in != -1) {
#ifndef NOTEXTCACHE
				fmt = _v[mc] = in;
#else /* NOTEXTCACHE */
				dont_cache = 1;
				fmt = in;
#endif /* NOTEXTCACHE */
				break;
			}
		}
		if (in == -1) {
		    if (fmt) {
			P2(("DB %O: no %O » %O\n", ME, mc || file, fmt))
#ifndef NOTEXTCACHE
			_v[mc] = fmt;
#else /* NOTEXTCACHE */
			// _v[mc] = fmt;
#endif /* NOTEXTCACHE */
		    } else {
			string crazy;

			// attention! here comes real psyc inheritance!
			crazy=mc;
			for (i = strlen(crazy)-1; i>1; i--) {
			    if (crazy[i] == '_') {
				--i;
				crazy = crazy[..i];
				P2(("DB %O: no %s, trying %s\n",
					ME, mc, crazy))
				if (ghost) fmt = ghost -> lookup(crazy, fmt);
				else fmt = lookup(crazy, fmt);
#ifndef NOTEXTCACHE
				if (_v[mc] = fmt) return fmt;
#else /* NOTEXTCACHE */
				// is this cache legal?
				//if (_v[mc] = fmt) return fmt;
				if (fmt) return fmt;
#endif /* NOTEXTCACHE */
			    }
			}
			unless (fmt) {
			    P3(("(harmless) textdb returns \"[%s]\" for"
			        " %O, %O, %O\n", mc, file, ghost, curse))
#if DEBUG > 3
			    // this warning is currently useless as net/irc
			    // first requests this sort of template, then
			    // throws it away. we'd have to change the flow
			    // of operations (the textdb API really) to make
			    // this warning useful again.
			    if (strlen(mc) && mc[0] == '_')
				raise_error("DEBUG: textdb incomplete?\n");
#endif
			    return "["+mc+"]";
			}
		    }
		}
	}
        if (fmt && strlen(fmt)) {
            // if (abbrev("_TEXT_", mc) || abbrev("_MISC_", mc)) {
	    if (mc[1] <= 'Z' && mc[1] >= 'A') {
//		D(S("chomping: %O for %O\n", fmt, mc));
#ifndef NOTEXTCACHE
		_v[mc] = fmt = chomp(fmt);
#else /* NOTEXTCACHE */
		fmt = chomp(fmt);
		unless (dont_cache) _v[mc] = fmt;
#endif /* NOTEXTCACHE */
            }
	}
	P4(("DB lookup in %O finally returns %O\n", ME, fmt))
	return fmt;
}

#if 0 //def NOTEXTCACHE
void create() {
    if(clonep(ME)) fs = DAEMON_PATH "share" -> get("tdbFS");
    ::create();
}
#endif
