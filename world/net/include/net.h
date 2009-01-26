// $Id: net.h,v 1.148 2008/07/26 10:54:30 lynx Exp $ // vim:syntax=lpc:ts=8
#ifndef _INCLUDE_NET_H
#define _INCLUDE_NET_H

//efine	SERVER_VERSION_MINOR	"RC0i"
#define	SERVER_VERSION		"psyced/0.99"
#define	SERVER_DESCRIPTION	"PSYC-enhanced multicasting chat and messaging daemon"

#define TCP_PENDING_DISCONNECT	1
#define TCP_PENDING_TIMEOUT	2

#define RANDHEXSTRING sprintf("%x", random(__INT_MAX__))

// maybe we should expect psyc usage peak around 2030?
// or will psyc reform its protocol by then anyway?
// ok.. let's go for 2009 so we can renovate time formats
// around 2020.. by the way, 2009-02-14 is a saturday, so
// we'll be having a hot friday night party for psyc epoch!
//
#define	PSYC_EPOCH	1234567890	// 2009-02-14 00:31:30 CET

// TODO: new driver will be able to combine \n, too
// 	new expat parser will also be able to combine >
#define XML_CHARSET	"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 </=\"'?.:@-_!#"
// hmm.. common.c uses pcre with RE_UTF8 to ensure legal charset....

#ifndef _INCLUDE_AUTO_H
# include "auto.h"
#endif

#ifndef __PIKE__
# ifdef CONFIG_PATH
#  include CONFIG_PATH "config.h"
# else
#  include "/local/config.h"
# endif
#endif

// switching to UTF-8 should work now (if you keep .fmt files disabled!)
#ifndef SYSTEM_CHARSET
# define SYSTEM_CHARSET	"UTF-8"
//define SYSTEM_CHARSET	"ISO-8859-15"
#endif

#if defined(SYSTEM_CHARSET) && SYSTEM_CHARSET != "UTF-8"
# define TO_UTF8(s) convert_charset((s), SYSTEM_CHARSET, "UTF-8")
# define FROM_UTF8(s) convert_charset((s), "UTF-8", SYSTEM_CHARSET)
#else
# define TO_UTF8(s) (s)
# define FROM_UTF8(s) (s)
#endif

// this stuff is so popular.. can i really put it anywhere else?
#ifndef DEFAULT_CONTENT_TYPE
# define DEFAULT_CONTENT_TYPE	"text/html; charset=" SYSTEM_CHARSET
#endif

#if __EFUN_DEFINED__(stringprep)
// stringprep needs utf8 arguments
// this results in lots of conversions some of which look like
// system->utf­>system->utf. luckily UTF8 is our system charset.
// so FROM_UTF8 and TO_UTF8 are normally nullmacros (see above)
# include <idn.h>
// beware, these macros dont have error handling...
# define NODEPREP(s) FROM_UTF8(stringprep(TO_UTF8(s), STRINGPREP_XMPP_NODEPREP))
# define NAMEPREP(s) FROM_UTF8(stringprep(TO_UTF8(s), STRINGPREP_NAMEPREP))
# define RESOURCEPREP(s) FROM_UTF8(stringprep(TO_UTF8(s), STRINGPREP_XMPP_RESOURCEPREP))
#else
# define NODEPREP(s) lower_case(s)
# define NAMEPREP(s) lower_case(s)
# define RESOURCEPREP(s) (s)
#endif

#include "debug.h"

#ifdef PRO_PATH
# include "/pro/include/pro.h"
# ifndef HYBRID
#  define OPT_PATH PRO_PATH
# else
#  define OPT_PATH NET_PATH
# endif
#else
# define OPT_PATH NET_PATH
#endif

#ifdef CHATDOMAIN
# undef __DOMAIN_NAME__
# define __DOMAIN_NAME__ CHATDOMAIN
#endif

#ifndef SERVER_HOST
# ifndef CHATHOST
#  define CHATHOST __HOST_NAME__
# endif
# if __DOMAIN_NAME__ != "unknown"
#  define SERVER_HOST CHATHOST "." __DOMAIN_NAME__
# else
#  if __HOST_IP_NUMBER__ == "127.0.0.1"
#   define SERVER_HOST "localhost"
#  else
#   define SERVER_HOST CHATHOST
#  endif
# endif
#endif

#define HAS_PORT(PORT, PATH)    (defined(PATH) && defined(PORT) && PORT - 0)
// also need HAS_TLS_PORT() ?

#ifndef HTTP_URL
# if HAS_PORT(HTTP_PORT, HTTP_PATH)
#  define HTTP_URL "http://"+ (HTTP_PORT == 80 ? SERVER_HOST \
			     : (SERVER_HOST +":"+ to_string(HTTP_PORT)))
# else
#  define HTTP_URL "http://" SERVER_HOST
# endif
#endif

#ifndef HTTPS_URL
# if HAS_PORT(HTTPS_PORT, HTTP_PATH)
#  define HTTPS_URL "https://"+ (HTTPS_PORT == 443 ? SERVER_HOST \
			     : (SERVER_HOST +":"+ to_string(HTTPS_PORT)))
# else
#  define HTTPS_URL 0	// so that you can do
			//   ((tls_available() && HTTPS_URL) || HTTP_URL)
                        // ... what about ifdef __TLS__ ?
# endif
#endif

#ifdef MUD
# define	NO_NEWBIES
#endif

#ifdef RELAY
# define	NO_NEWBIES	// same as REGISTERED_USERS_ONLY ?
				// anyway, chance for some ifdef optimizations TODO
# define	IRCGATE_NICK		"PSYC.EU"
# undef		DEFAULT_USER_OBJECT
# define	DEFAULT_USER_OBJECT	IRC_PATH "ghost"
#endif

#ifndef DEFAULT_USER_OBJECT
# define DEFAULT_USER_OBJECT	PSYC_PATH "user"
#endif

#ifndef MAX_VISIBLE_USERS
# define MAX_VISIBLE_USERS 44
#endif

#ifndef PRO_PATH
// very specific to the way we do web applications
# define htok			this_interactive()->http_ok
# define htok3(prot,type,extra)	this_interactive()->http_ok(prot,type,extra)
# define hterror		this_interactive()->http_error
# define htmlhead		this_interactive()->http_head
# define htmlpage		this_interactive()->http_page
# define htmltail		this_interactive()->http_tail

# define HTERROR	 1  // return mc if necessary?
# define HTDONE		 0
# define HTMORE		-1
#endif

// even more specific to the way we parse commands
// combine the rest of the arguments back into one string
#define ARGS(x) implode(args[x..], " ")

// pick the psyc: uniform for objects, otherwise keep what we have
#define UNIFORM(x)  (objectp(x)? psyc_name(x): x)

// we currently make no distinction
// now we do.
//#define register_service(name)	register_scheme(name)

#ifndef MYNICK // under certain circumstances may be useful differently
# define MYNICK		_myNick		 // was qName()
# define MYLOWERNICK	_myLowerCaseNick // was lower_case(qName())
#endif

#define DIGESTAUTH_TIMEOUT	60

// sorry, but servers expect async auth and persons don*t provide without this
// define. so we need this defined until someone ifdefs the servers.
// i'll fix that friends thing (was the bug a friends thing?) tomorrow.
#ifndef ASYNC_AUTH
# define ASYNC_AUTH
#endif

#ifndef NO_CUTTING_EDGE
// should only affect how psyc clients are treated, but in fact the
// entire net/jabber subsystem depends on this
# define UNL_ROUTING
# define USE_THE_RESOURCE
# define MEMBERS_BY_SOURCE
# define TAGGING
# define TAGS_ONLY
# define NEW_QUEUE
//# define CACHE_PRESENCE
// gut gut.. dann testen wir diesen kram halt auch
# define QUIET_REMOTE_MEMBERS
// wir können viel herumphilosophieren was richtig wäre, aber tatsächlich
// effizienter verteilen werden wir auf kurze sicht eh nicht, so lets use this:
# define SMART_UNICAST_FRIENDS
# define SIGS
# define SWITCH2PSYC
# define WANT_S2S_TLS
# define WANT_S2S_SASL
# define ENTER_MEMBERS
# define PERSISTENT_MASTERS
# define NEW_LINK
# define NEW_UNLINK
# define NEW_RENDER
# define MUCSUC
#endif
#define GAMMA   // code that has left BETA and is in production use

#ifndef _flag_disable_log_hosts
# define _flag_log_hosts
#endif

#ifdef EXPERIMENTAL
	// fippo's brilliant single-user channel emulation for jabber MUCs
	// unfortunately it provides no advantages over the old method, yet.
	// would be cool to cache a member list at least!  TODO
# define PERSISTENT_SLAVES
// efine IRC_FRIENDCHANNEL  // hopelessly needs more work
# ifdef HTTP_PATH
#  define HTFORWARD	    // let person entity buffer output for http usage
# endif
#else
//# define PRE_SPEC	    // things that changed during the spec process
//# ifndef __PIKE__
//#  define USE_LIVING
//# endif
#endif

#ifdef __NO_SRV__	    // since psyclpc 4.0.4
# define ERQ_WITHOUT_SRV
#else
// still not official part of the ldmud distribution
# define ERQ_LOOKUP_SRV	13
// hmm seit psyclpc sollte das nicht mehr hier sein
// und warum 13 wenn es 17 sein könnte? :)
#endif

// still using rawp anywhere?
//#define	rawp(TEXT) { P1(("rawp? "+TEXT)) emit(TEXT); }

#endif
