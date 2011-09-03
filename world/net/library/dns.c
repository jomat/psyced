// vim:syntax=lpc
// info: to unfold and view the complete file, hit zR in your vim command mode.
//
// $Id: dns.c,v 1.113 2008/09/12 15:54:39 lynx Exp $
//
// {{{ meta-bla about foldmethod=marker
// <lynX> hm.. find ich folding jetzt eher nützlich oder lästig? muss ich
//	noch herausfinden. auf jeden fall sind die farben eher unpraktisch.
//	steht sicher irgendwo in irgendwelchen files definiert.. juchei
//	übertreiben sollte man's jedenfalls nicht, oder was soll das
//	"whatis comment" anderes erreichen als dass der kommentar nicht
//	gelesen wird obwohl er extra dafür geschrieben wurde?
// }}}

// und dann ist da noch myUNL, myLowerCaseHost.
// in der psyclib. letzteres gehört wohl eher hier her.

// {{{ includes
#include "net.h"
#include "proto.h"
#include "closures.h"
#include <dns.h>
#include <erq.h>
#include <sandbox.h>
// }}}

// {{{ queue-inherit
inherit NET_PATH "queue";
// }}}

// {{{ variables
//
// this mapping has to be *volatile* or it will carry old hostnames
// that may no longer be valid, then cause wild illogical behaviour
volatile mapping localhosts = ([
   "localhost":	"127.0.0.1",
   "127.0.0.1":	"localhost",
   // unusual but valid syntax for localhost
   // then again usually any 127.* leads to localhost so it's
   // pointless and after all it can even be good for testing
   // the switch protocol on oneself..  ;)
   "127.0.1":	"localhost",
   "127.1":	"localhost",
// "0": 	??
#ifdef __HOST_IP_NUMBER__
# if __HOST_IP_NUMBER__ != "127.0.0.1"
    __HOST_IP_NUMBER__ : 1,
# endif
#endif
// the hostnames need to be in lowercase... lets do it later
//  SERVER_HOST : 1,
#if defined(_host_XMPP)
# if _host_XMPP != SERVER_HOST
//  _host_XMPP : 1,
# endif
#endif
]);

volatile mapping host2canonical = ([ ]);
volatile object hostsd;
// }}}

// {{{ query_server_ip && myip && register_localhost
#ifndef __HOST_IP_NUMBER__
volatile string myIP;
#else
# define myIP __HOST_IP_NUMBER__
#endif

string query_server_ip() { return myIP; }

// take care to always pass a lowercased (virtual) hostname
varargs void register_localhost(string name) { //, string ip) {
	//unless (ip) ip = myIP;
	localhosts[name] = 1; //ip;
	// either we have a static ip, then we already know it, or we
	// have a public ip hidden by a firewall or nat, then we can
	// find it out using #'set_my_ip - but what if by dsl relogin
	// or something like that our public ip address changes? then
	// we have an old ip number which by very unlikely circumstance
	// could possibly turn into a security problem. but what's worse:
	// we might get used having the public ip in here, and then it
	// suddenly no longer is (because there is an old one instead)
	// i have the impression we are well off if we only know all of
	// our virtual host names - we don't need our ip numbers really
	//localhosts[ip] = name;
}
// }}}

#ifndef SANDBOX
// {{{ function: del_queue
void del_queue(string queue) {
    qDel(queue);
}
// }}}
#endif

// {{{ function: legal_host
int legal_host(string ip, int port, string scheme, int udpflag) { // proto.h!
	P3(("legal_host %O %O %O %O\n", ip, port, scheme, udpflag))
	// do not allow logins during shutdown
//	if (shutdown_in_progress) return 0;
	// we sincerely hope our kernel will drop udp packets that
	// spoof they are coming from localhost.. would be ridiculous if not
//	if (ip == "127.0.0.1" || ip == "127.1" || ip == "0") return 9;
#ifndef _flag_disable_trust_localhost
	if (localhosts[ip]) return 9;
	if (ip == myIP || !ip) return 8;
#endif
	unless (hostsd) hostsd = DAEMON_PATH "hosts" -> load();
	P4(("legal_host asks %O\n", hostsd))
	return hostsd -> legal_host(ip, port, scheme, udpflag);
}
// }}}

// {{{ function: legal_domain
int legal_domain(string host, int port, string scheme, int udpflag) {
	// we sincerely hope our kernel will drop udp packets that
	// spoof they are coming from localhost.. would be ridiculous if not
	//
	// TODO: use is_localhost here?
#ifndef _flag_disable_trust_localhost
	if (host == "localhost" || host == SERVER_HOST) return 9;
#endif
//	if (host == myLowerCaseHost) return 8;
//	if (lower_case(host) == myLowerCaseHost) return 7;

//	unless (hostsd) hostsd = DAEMON_PATH "hosts" -> load();
//	return hostsd -> legal_domain(host, port, scheme, udpflag);
	return 5; // TODOOOOOO
}
// }}}

// {{{ function: chost
// {{{ whatis comment
/** provides us with the "canonical" name of a host as given by DNS PTR,
 ** not to be confused with the aliasing facility "CNAME" - not sure
 ** about this function name but don't know any better right now.
 ** this function does NOT do any DNS resolutions itself, it only consults
 ** a cache which is kept up to date by the dns_*resolve functions.
 ** it is therefore nice and fast.
 */
// }}}
string chost(string host) {
    return host2canonical[host];
}
// }}}

// {{{ function: same_host
// {{{ whatis comment
/** figures out if two hostnames (aliases or its ip) are the same host.
 ** this is essentially chost(a) == chost(b) with one function call less.
 */
// }}}

// {{{ remarks
// we should probably lower_case() hosts, as there has been trouble w/
// hostnames once lower_cased() (extracted from unl, probably) and
// !lower_cased() when we were testing _identification w/ pypsycd.
// of course we'd then have to do host = lower_case(host) in register_host(),
// too. }}}
int same_host(string a, string b) {
    //mixed c = host2canonical[lower_case(a)];
    mixed c = host2canonical[a];

    P2(("same_host %O -> %O ==? %O -> %O\n", a, c, b, host2canonical[b]))
    //return stringp(c) && c == host2canonical[lower_case(b)];
    return stringp(c) && c == host2canonical[b];
}
// }}}

// {{{ function: register_host
varargs void register_host(string host, mixed canonical) {
    PROTECT("REGISTER_HOST")
    P2(("register_host %O, %O.\n", host, canonical))
    if (stringp(canonical)) {
	if (host2canonical[host] != canonical && host2canonical[host]) {
	    // 0 was already mapped to "andrack.tobij.de", now points ...
	    // dunno why that happens, but here we catch it
	    unless (host) return;

	    // this probably only happens if the server has been running
	    // for several weeks.. the fx are probably mostly harmless
	    // just some dynamic ip which gets reused by someone else
	    canonical = lower_case(canonical);
	    if (host2canonical[host] != canonical) {
		// with akamai sites this happens all the time and is harmless
#if 0
		monitor_report("_warning_overridden_register_host",
		    S("%O was already mapped to %O, now points to %O.",
			host, host2canonical[host], canonical));
#else
		P1(("%O was already mapped to %O, now points to %O.\n",
		    host, host2canonical[host], canonical))
#endif
	    }
	}
	else canonical = lower_case(canonical);
	host2canonical[host] = canonical;
	P3(("%O mapped to %O.\n", host, canonical))
	return;
    }
    else if (intp(canonical) && canonical < 0) {
	if (stringp(host2canonical[host])) {
	    // we do get to see this message sometimes.. weird thang dns
	    P1(("%O no longer resolves, we keep it mapped to %O.\n",
		    host, host2canonical[host]))
	    // no this doesn't seem to be triggered by timed out dyndns
	    // as dyndns usually carries around the old ip number
	    return;
	}
	// host did not resolve, let's store this fact
	host2canonical[host] = canonical;
	return;
    }

    // we won't need the lambda-closure because of my ingenious varargs design
    // in dns_resolve() and dns_rresolve()
#if 0
    dns_resolve(host, #'dns_rresolve, lambda(({ 'canonical }),
	({ #'=, ({ CL_INDEX, host2canonical, host }), 'canonical }) ) );
#else
    dns_resolve(host, #'dns_rresolve,
		(: if (stringp($1)) $2[$3] = $1;
		 return; :), host2canonical, host);
#endif
    return;
}
// }}}

// {{{ function: dns_refresh
/** wild idea of tobi, needs an admin command to trigger it */
void dns_refresh() {
    string host;

    PROTECT("DNS_REFRESH")
    
    foreach (host : m_indices(host2canonical)) { register_host(host); }
}
// }}}

// {{{ usage: dns_*resolve 
// we do know that lambda() might be a quite unfamiliar concept to the usual
// coder, so we made dns_resolve() and dns_rresolve() a little more handy.
// you can use inline-closures at the caller side, even if you need some
// "variables" or arguments which cannot be accessed by an unsual inline
// closure (with 3-3 style inline closures that works, but we encourage you
// not to use them for backward compatibility).
//
// so here is an usage example:
// dns_resolve(hostname, (: sendmsg($2, "_some_mc", $3 + " has been resolved "
// "to " + $1); :), source, hostname);
// $1 will be the dns_resolve result, $2 will be source and $3 will be
// hostname. as you already guessed, $2 and further are just the arguments
// you appended to the dns_resolve() call after the closure, and $1 is, as said,
// the dns_resolve() result.
// this works for any number of extra arguments.
//
// this is also works for dns_rresolve()
// }}}

// {{{ function: dns_resolve
// {{{ whatis comment
/** sends a request to the erq to resolve a hostname, on reception of the
 ** reply does the callback where it passes either the ip-number as string
 ** or -1 for failure, followed by any extra arguments you gave it
 */ // }}}
void dns_resolve(string hostname, closure callback, varargs array(mixed) extra) {
	closure c, c2;					// proto.h
	string queuename;

	P4(("dns_resolve called with (%O, %O, %O).\n", hostname, callback, extra))
	unless (stringp(hostname)) {
	    P1(("dns_resolve called with (%O, %O, %O). stopping.\n", hostname, callback, extra))
	    return;
	}
#ifdef __IDNA__
	if (catch(hostname = idna_to_ascii(TO_UTF8(hostname)); nolog)) {
	    P0(("catch: punycode %O in %O\n", hostname, ME))
	    return;
	}
#endif

	if (sizeof(extra)) {
	    c2 = lambda( ({ 'name }),
			 ({ #'funcall, callback, 'name }) + extra );
	} else {
	    c2 = callback;
	}

    // if unnecessary this can move into !__ERQ_MAX_SEND__
	switch(hostname) {
//	case myLowerCaseHost:	// TODO!!
#if defined(SERVER_HOST) && SERVER_HOST != "localhost"
	case SERVER_HOST:
# ifdef __HOST_IP_NUMBER__
	    funcall(c2, __HOST_IP_NUMBER__, callback);
# else
	    unless (myIP) break;
	    funcall(c2, myIP, callback);
# endif
	    P4(("self rresolution used\n"))
	    return;
#endif
	case "localhost":
	    P3(("localhost rresolution used\n"))
	    funcall(c2, "127.0.0.1", callback);
	    return;
	}

	if (sscanf(hostname,"%~D.%~D.%~D.%~D") == 4) {
	    // hostname is an IP already. Just call the callback.
	    funcall(c2, hostname, callback);
	    return;
	}
#ifdef __ERQ_MAX_SEND__
	queuename = "f" + hostname;
	if (qExists(queuename)) {
	    enqueue(queuename, c2);
	    return;
	} else {
	    qInit(queuename, 1000, 10);
	    enqueue(queuename, c2);
	}

       	c = lambda( ({ 'name }), ({ 
	(#'switch), ({ #'sizeof, 'name }),
	({ 4 }), // ERQ: name resolved!
	({ (#',),
		({ (#'=),'name,({ (#'map),'name,#'&,255 }) }),
		({ (#'=),'name,
		 ({
			(#'sprintf),"%d.%d.%d.%d",
						({ CL_INDEX, 'name, 0 }),
						({ CL_INDEX, 'name, 1 }),
						({ CL_INDEX, 'name, 2 }),
						({ CL_INDEX, 'name, 3 })
		  })
		 }),
		({ #'while, ({ #'qSize, queuename }), 42,
				 ({ #'catch,
				    ({ #'funcall, ({ #'shift, queuename }),
				       'name })
				 })
	        }),
		({ #'qDel, queuename })
	}),
	(#'break),
#   ifdef XERQ
	({ 6 + strlen(hostname) }), // XERQ
	({ (#',),
                ({ (#'=),'name,({ (#'map),'name,#'&,255 }) }),
                ({ (#'=),'name,
                 ({
                        (#'sprintf),"%d.%d.%d.%d",
                                                ({ CL_INDEX, 'name, 1 }),
                                                ({ CL_INDEX, 'name, 2 }),
                                                ({ CL_INDEX, 'name, 3 }),
                                                ({ CL_INDEX, 'name, 4 })
                  })
                 }),
		({ #'while, ({ #'qSize, queuename }), 42,
				 ({ #'catch,
				    ({ #'funcall, ({ #'shift, queuename }),
				       'name
				    })
				 })
		}),
		({ #'qDel, queuename })
        }),
	(#'break),
#   endif
	({ #'default }),
	({
	 (#',),
#   if DEBUG > 0
	 ({ #'debug_message, "ERQ could not resolve \""+ hostname +"\".\n" }),
#   endif
	 // host2canonical[hostname] = 0;		<--
	 ({ #'funcall, #'register_host, hostname, -1 }),
	 ({ #'while, ({ #'qSize, queuename }), 42,
    /* held sagt: also zuerst sagt er
		:* ERQ could not resolve
		:* und dann die while whiles sind die queues
		:* also angestaute einträge in der queue
		:* die er dann mit -1 aufruft
		:* nach den while whiles wird die queue gelöscht
		:* und der nächste dns_resolve-aufruf geht wieder an den erq
		:* ziel der queues ist in diesem falle kein caching, sondern
		:* nur lineares aufrufen der callbacks
		:* also, fifo
		:*/
//			 ({ #'debug_message, "while while "+ hostname + "\n"}),
			 ({ #'catch, ({ #'funcall, ({ #'shift, queuename }),
			  -1 }) }) }),
	 ({ #'qDel, queuename })
        }),
	(#'break)
	}) );
// this efun was introduced shortly after erq was fixed.. btw what does it do?
# if __EFUN_DEFINED__(reverse)
	// we are currently doing too many resolutions.. this is nice for
	// dyndns but silly if we have a valid tcp connection to the host
	// in question, thus it is proven that the ip hasn't changed.
	P3(("resolving "+hostname+"\n"))
	unless (send_erq(ERQ_LOOKUP, hostname, c))
# else
	// appending the zero byte fixes a bug in erq. sick!
	unless (send_erq(ERQ_LOOKUP, to_array(hostname) + ({ 0 }), c))
# endif
#else
	// it's all pretty useless if __ERQ_MAX_SEND__ is defined
	// even if ldmud was called with -N. what is this, a bug?
	P1(("no erq: returning unresolved %O\n", hostname))
#endif
	{
	    P1(("%O failed to ERQ_LOOKUP %O!\n", ME, hostname))
	    // if we cannot resolve using erq, we'll send back the hostname,
	    // so the driver can do a blocking resolve itself (needed for
	    // net_connect())
	    funcall(c2, hostname, callback);
#ifdef __ERQ_MAX_SEND__
	    qDel(queuename);
#endif
	}
	return;
}
// }}}

// {{{ function: dns_rresolve
// {{{ whatis comment
/** requests a reverse lookup of the given ip address from the erq process,
 ** on reply calls dns_resolve() to ensure the returned host name does indeed
 ** point to the ip address (protection against basic DNS spoofing), then
 ** does the callback, where it passes either the hostname of the ip as given
 ** by IN PTR, -1 for resolution failure, or -2 for a DNS spoofing attempt.
 ** extra arguments follow, if you provided any.
 */ // }}}
void dns_rresolve(string ip, closure callback, varargs array(mixed) extra) {
    closure rresolve, resolve;				// proto.h
    int i1, i2, i3, i4;
    string queuename;

    P4(("dns_rresolve called with (%O, %O, %O).\n", ip, callback, extra))
    unless (stringp(ip)) {
	P1(("dns_rresolve called with (%O, %O, %O). stopping.\n", ip, callback, extra))
	return;
    }
    extra = ({ #'funcall, callback, 'host }) + extra;
    // if unnecessary this can move into !__ERQ_MAX_SEND__
    switch(ip) {
#ifdef __HOST_IP_NUMBER__
# if __HOST_IP_NUMBER__ != "127.0.0.1"
    case __HOST_IP_NUMBER__:
# else
#  ifndef myIP
    case myIP:
#  else
    case 0:	// the hostname IS localhost, so this never happens
#  endif
# endif
	P4(("self resolution used\n"))
	if (sizeof(extra)) funcall(lambda( ({ 'host }), extra), SERVER_HOST);
	else funcall(callback, SERVER_HOST);
	return;
#endif
    case "127.0.0.1":
	P3(("localhost resolution used\n"))
	if (sizeof(extra)) funcall(lambda( ({ 'host }), extra), "localhost");
	else funcall(callback, "localhost");
	return;
    }
#ifdef __ERQ_MAX_SEND__
    unless (sscanf(ip, "%D.%D.%D.%D", i1, i2, i3, i4) == 4) {
	// that is not an ip. what shall we do?
	// we're just returning for now. suggestions -> changestodo
	P1(("dns_rresolve called with (%O, %O, %O). stopping.\n", ip, callback, extra))
	return;
    }

    queuename = "r" + ip;

    resolve = lambda( ({ 'host, 'ip, 'rip }),
		      ({ #'?, 
		       ({ #'==, 'ip, 'rip }),
		       ({ (#',),
		        ({ #'while, ({ #'qSize, queuename }),
		 	 42,
			 ({ #'catch,
			    ({ #'funcall, ({ #'shift, queuename }), 'host })
			 })
		        }),
			({ #'qDel, queuename }),
		       }),
		       ({ (#',),
			({ #'=, 'host, -2 }), // useless line
			({ #'while, ({ #'qSize, queuename }),
			 42,
			 ({ #'catch,
			    ({ #'funcall, ({ #'shift, queuename }), -2 })
			 }),
			}),
			({ #'qDel, queuename })
		       })
		      }) );

    if (qExists(queuename)) {
	enqueue(queuename, sizeof(extra)
		? lambda(({ 'host }), extra)
		: callback);
	return;
    } else {
	qInit(queuename, 1000, 10);
	enqueue(queuename, sizeof(extra)
		? lambda(({ 'host }), extra)
		: callback);
    }

    rresolve = lambda( ({ 'name }),
		 ({ #'funcall, (:
	    $1 = to_string($1[4..<2]);	// extract hostname from crazy
					// erq return format
	    unless ($1 == "") {
		dns_resolve($1, lambda( ({ 'name }),
		   ({ #'funcall, $4, $1, $2, 'name, $3 }) ));
	    } else {
		funcall($4, -1, 0, 0, $3);
	    }
    return; :), 'name, ip, callback, resolve }) );

    unless (send_erq(ERQ_RLOOKUP, ({ i1, i2, i3, i4 }), rresolve))
#else
	P1(("no erq: returning unrresolved %O\n", ip))
#endif
    {
	P1(("%O failed to ERQ_RLOOKUP!\n", ME))
	if (sizeof(extra)) {
	    funcall(lambda( ({ 'host }),
			    extra), ip);
	} else {
	    funcall(callback, ip);
	}
#ifdef __ERQ_MAX_SEND__
	qDel(queuename);
#endif
    }
    return;
}
// }}}

// {{{ function: is_localhost
#if 1
int is_localhost(string host) {
    // async discovery of virtual hosts is not necessary 
    // we should know all of our hostnames in advance for
    // security anyway
    return member(localhosts, lower_case(host));
}
#else
int is_localhost(string host, closure callback, varargs array(mixed) extra) {
    host = lower_case(host);
    if (member(localhosts, host)) {
	funcall(callback, 1, extra);
	return;
    }

    // this is maybe not enough... we should resolve myIP,.. too
    if (sscanf(host, "%~D.%~D.%~D.%~D") == 4) {
	// what needs to be done?? rresolve both host and __HOST_IP_NUMBER__ ?
	dns_rresolve(host, 
		     lambda(({ 'hostname }), 
			    ({ #'funcall, callback, 
			     ({ #'==, 'hostname, SERVER_HOST }) }) 
			    + (sizeof(extra) ? extra : ({ }) )
			    ));
    } else { 
# if !defined(__HOST_IP_NUMBER__) || __HOST_IP_NUMBER__ != "127.0.0.1"
	dns_resolve(host, 
		     lambda(({ 'hostname }), 
			    ({ #'funcall, callback, 
#  ifdef __HOST_IP_NUMBER__
				({ #'==, 'hostname, __HOST_IP_NUMBER__ })
#  else
				({ #'==, 'hostname, myIP })
#  endif
			    }) + (sizeof(extra) ? extra : ({ }) )
			   ));
# else
	funcall(callback, 0, extra);
# endif
    }
}
# if 0
// call either true or false
void if_localhost(string host, closure true, closure false, 
		  varargs array(mixed) extra) {
    is_localhost(host, lambda(({ 'bol }), 
		    ({ #'funcall, 
			({ #'?, 'bol, true, false }), 
		     }) + (sizeof(extra) ? extra : ({ }) ) ));
}
# endif
#endif // if 1 
// }}}

#ifndef ERQ_WITHOUT_SRV
// {{{ function: dns_srv_resolve
void dns_srv_resolve(string hostname, string service, string proto, closure callback, varargs array(mixed) extra)
{
#ifdef __ERQ_MAX_SEND__
    string req;
    int rc;

    // dumme bevormundung. wegen der musste ich jetzt ewig lang suchen:
    //unless (proto == "tcp" || proto == "udp") return;
    // da wir mit nem String arbeiten muessen

#ifdef __IDNA__
    if (catch(hostname = idna_to_ascii(TO_UTF8(hostname)); nolog)) {
	    P0(("catch: punycode %O in %O\n", hostname, ME))
	    return;
    }
#endif
    req = sprintf("_%s._%s.%s", service, proto, hostname);
    rc = send_erq(ERQ_LOOKUP_SRV, req, lambda(({ 'wu }),
	    ({ (#',),
		({ #'?, ({ #'<, ({ #'sizeof, 'wu }), 3 }), 
			({ (#',),
# if DEBUG > 1
			    ({ #'debug_message, "Could not srv_resolve _" +service+"._"+proto+"."+ hostname + "\n" }),
# endif
			    ({ #'funcall, callback, -1 }) + extra,
			    ({ #'return, 1 }),
			 }), 
		}),
		({ #'=, 'i, 0 }),
		({ #'=, 'resarr, quote(({ })) }),
		({ #'=, 'wu, ({ #'explode, ({ #'to_string, ({ #'[..<], 'wu, 0, 3 }) }), "\n" }) }),
		// wieso die if in der condition der while ???
		({ #'while, ({ #'?, ({ #'<, 'i, ({ #'sizeof, 'wu }) }) }), 42,
		    ({ (#',),
			({ #'+=, 'resarr, ({ #'({, ({ #'({, ({ #'[, 'wu, 'i }), ({ #'to_int, ({ #'[, 'wu, ({ #'+, 'i, 1 }) }) }), ({ #'to_int, ({ #'[, 'wu, ({ #'+, 'i, 2 }) }) }), ({ #'to_int, ({ #'[, 'wu, ({ #'+, 'i, 3 }) }) }) }) }) }),
			({ #'=, 'i, ({ #'+, 'i, 4 }) })
		    })
		}),
		// ({ #'funcall, callback, ({ #'sort_array, 'resarr, sortsrv }) }) + extra
		({ #'funcall, callback, 'resarr }) + extra
	    })
    ));
    unless (rc) {
	    P0(("%O failed to ERQ_LOOKUP_SRV!\n", ME))
	    funcall(callback, -1, extra);
    }
    // Rueckgabe vom erq bei Fehlschlag -> empty message wie bei ERQ_RLOOKUP
    // callback sollte erwarten:
    // ein Array von Arrays jeweils mit name, port, pref, weigth
    // oder ein einzelnes Array und man betrachtet jeweils vier Dinger
    // waer dufte wenn die lambda das splitten koennte... sonst muss es 
    // halt nen string sein der zurueckgegeben wird
    return;
#else
    P1(("no erq: aborting SRV check for %O\n", hostname))
    funcall(callback, -1, extra);
#endif
}
// }}}
#endif
