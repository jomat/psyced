// vim:foldmethod=marker:syntax=lpc:noexpandtab
// $Id: routerparse.i,v 1.15 2008/02/06 12:16:16 lynx Exp $
//
// THIS FILE IS FORK ONLY, thus currently not in use.
//
// TODO: both parsers need to croak on incoming _INTERNAL vars!!

#ifdef FORK // {{{
// MMP PARSER - parses the routing header of PSYC, also known as MMP.
//
// THIS IS THE INNOVATIVE RID'N'SAGE PARSER REWRITE
// they wanted to have it completely seperate from the original parse.i
// so *keep* it seperate in its own file, routerparse.i!
//
// unfortunately it handles context counters in a non-universal way.. TODO
// also it misses support for _trust

// this flag should enable forward-checks of dns resolutions..
// currently we don't have that, so this flag actually disables
// use of unprooven resolved hostnames and reduces everything to
// ip numbers.
//#define HOST_CHECKS

#ifndef PSYC_LIST_SIZE_LIMIT
# define PSYC_LIST_SIZE_LIMIT 404
#endif

// just the plain ip number of the remote host
// ^^ glatte lüge!
volatile string peerhost;
#if 0 //defined(PSYC_TCP) && __EFUN_DEFINED__(strrstr)
volatile string peerdomain;
#endif
// remote port number
volatile int peerport;
// unresolved-ip or ip:port
volatile string peeraddr;
// holds last ip we were connected to
volatile string peerip;
// how much can we trust the content of this packet?
volatile int ctrust;

volatile closure _deliver;
volatile object _psyced;
#define PSYCED (_psyced ? _deliver : (_psyced = DAEMON_PATH "psyc" -> load(), _deliver = symbol_function("deliver", _psyced)))


// current variables (":"), permanent variables ("=", "+", "-")
//
volatile mapping cvars = 0, pvars = ([ "_INTERNAL_origin" : ME ]), nvars = 0;

//
// a distinction between mmp and psyc-vars should be made

// current method and incoming buffer
volatile string buffer;

// cache of patched remote uniforms
volatile mapping patches = ([]);

// parsing helpers..
// MMP
volatile string lastvar, lastmod, checkpack, origin_unl;
volatile mixed lastvalue;
// list parsing helpers..
volatile array(mixed) list;
volatile mapping hash;
volatile int l = 0;
//volatile int pongtime; // TODO: FORK is missing the PONG

# ifndef PSYC_TCP
// resolved UNL of remote server (psyc://hostname or psyc://hostname:port)
volatile string netloc;
# define QUIT return 0;
# endif

// prototype definition for #'getdata
getdata(string a);
restart();

#ifdef __LDMUD__
# define SCANFIT (sscanf(a, "%1.1s%s%t%s", mod, vname, vvalue) || sscanf(a, "%1.1s%s", mod, vname))
#else
# define SCANFIT (sscanf(a, "%1s%s%*[ \t]%s", mod, vname, vvalue) || sscanf(a, "%1s%s", mod, vname))
#endif

#define SPLITVAL(val) \
	unless (sscanf(val, "%s %s", val, vdata)) vdata = 0
// sollte es wirklich ein space sein? hmmm.. %t ist sowieso zu grob..

# ifndef PSYC_TCP
#  define UDPRETURN(x)	return x;
#  define ERROR(m) UDPRETURN(0)
# else
#  define UDPRETURN(x)
#  define ERROR(m) { croak("_error_syntax_broken", "Failed in parsing " \
		      "[_modifier], closing connection.", \
		      ([ "_modifier" : intp(m) \
		       ? to_string(({m})) \
		       : to_string(m) ])); \
		    monitor_report("_error_syntax_broken", \
		   sprintf("MMP parsing failed. closing connection: %O", ME)); \
		    destruct(ME); \
		  }
# endif

# ifdef PSYC_TCP
void
# else
mixed
# endif
mmp_parse(string a) {
    string mod, vname, vvalue;

    P3(("MMP>> %O\n", a))
    
    if (sizeof(a)) switch(a[0]) {
    case '=':
    case '+':
    case '-':
# ifndef PSYC_TCP
	// hier könnten wir jedenfalls nen error ausgeben
	// udp und state ist keine gute kombination
# endif
    case ':':
	unless (2 <= sscanf(a, "%1.1s%s%t%s", mod, vname, vvalue) 
	    ||	2 == sscanf(a, "%1.1s%s", mod, vname)) {
	    ERROR(a[0]); 
	}
	if (vvalue == "") vvalue = 0;
	P3(("G: %O %O %O\n", mod, vname, vvalue))
	unless (vname) {
	    unless (lastvar && a[0] != '-' && lastmod
		    && lastmod[0] == a[0]) ERROR(a[0]);
	    if (pointerp(lastvalue)) {
		lastvalue += ({ vvalue });
	    } else {
		lastvalue = ({ lastvalue, vvalue });
	    }
	    return;
	}
	
	break;
    case '\t':
	unless (lastvar) {
	    ERROR(a[0]);
	}
	if (pointerp(lastvalue))
	    lastvalue[<1] += "\n"+a[1..];
	lastvalue += "\n"+a[1..];
	return;
    default:
	ERROR(a[0]);
    }

    if (lastmod) switch(lastmod[0]) {
# ifdef PSYC_TCP
    case '=':
	unless (lastvalue) {
	    m_delete(pvars, lastvar);
	    break;
	}
	if (lastvar == "_understand_modules") {
	    gotiate(lastvalue);
	} else if (lastvar == "_use_modules") {
	    negotiate(lastvalue);
	}
	pvars[lastvar] = lastvalue;
# endif
    case ':':
	if (lastvalue)
	    cvars[lastvar] = lastvalue;
	else
	    m_add(nvars, lastvar);
	break;
# ifdef PSYC_TCP
    case '+':
	_augment(pvars, lastvar, lastvalue);
	if (lastvar == "_understand_modules") {
	    gotiate(lastvalue);
	} else if (lastvar == "_use_modules") {
	    negotiate(lastvalue);
	}
	break;
    case '-':
	_diminish(pvars, lastvar, lastvalue);
	break;
# endif
    }


    unless (sizeof(a)) {
# ifdef PSYC_TCP
	next_input_to(#'getdata);
# endif
	// TODO init cvars = copy(pvars)
	cvars = (pvars + cvars) - nvars;
	UDPRETURN(2)
    } else if (a[0] == '.') {
# ifdef PSYC_TCP
	next_input_to(#'mmp_parse);
# endif
	restart();	
	UDPRETURN(1)
    } else {
	lastmod = mod;
	lastvar = vname;
	lastvalue = vvalue;
# ifdef PSYC_TCP
	next_input_to(#'mmp_parse);
# endif
	UDPRETURN(1)
    }
# ifdef PSYC_TCP
    if (timeoutPending) remove_call_out(#'quit);
# endif
}

getdata(string a) {
	P4(("GETDATA: %O\n", a));
	if (a != ".") {
		if (buffer == "")
		    buffer = a;
		else
		    buffer += "\n"+a;
# ifdef PSYC_TCP
		next_input_to(#'getdata);
# endif
	} else {
		array(mixed) u;
		string t = cvars["_context"] || cvars["_source"];

# ifdef PSYC_TCP
		// let's do this before we deliver in case we run into
		// a runtime error (but it's still better to fix it!)
		next_input_to(#'mmp_parse);
# endif
		if (!t || trustworthy > 5) {
			funcall(PSYCED, 0, 0, ME, peeraddr, peerhost, 
				buffer, cvars);
		} else unless (u = parse_uniform(t)) {
			P1((">> parse_uniform %O %O\n", t, u))
			croak("_error_invalid_uniform",
			     "Looks like a malformed URL to me.");
			QUIT
		} else dns_resolve(u[UHost], PSYCED, peerip, ME, peeraddr,
				   u[UHost], buffer, cvars);
		restart();
	}
	return 1;
}

restart() {
	// delete temporary variables for next msg
	cvars = ([ "_INTERNAL_trust" : trustworthy ]);
	nvars = m_allocate(0, 1);
	lastvalue = lastvar = lastmod = 0;
	// delete other stuff too
	buffer = "";
	ctrust = trustworthy;
	return 1;
}

# ifdef PSYC_TCP
startParse(string a) {
	if (a == ".") restart();
	else {
		croak("_error_syntax_initialization",
		    "The protocol begins with a dot on a line by itself.");
		QUIT
	}
	next_input_to(#'mmp_parse);
}
# endif

// also overrides createUser in net/server.c
createUser(string nick) {
	D2(D("creating " PSYC_PATH "user for "+ nick +"\n");)
	return named_clone(PSYC_PATH "user", nick);
}

qOrigin() { return origin_unl; }

sOrigin(origin) { P3(("sOrigin(%O) (%O) in %O\n", origin, origin_unl, ME))
    unless (origin_unl) origin_unl = origin; }

#endif /* FORK }}} */
