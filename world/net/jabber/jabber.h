// $Id: jabber.h,v 1.75 2008/09/12 15:54:39 lynx Exp $ // vim:syntax=lpc
//
// REMINDER:
// there are plenty of calls to lower_case in the code, that is because
// jabber requires all hostnames to be lowercased, or otherwise the
// protocol won't work! TODO: optimize this
//
#ifndef _JABBER_H
#define _JABBER_H

#define	JABBER_TRANSPARENCY	// switching this off improves parser
				// performance but kills file transfers

// local debug messages - turn them on by using psyclpc -DDjabber=<level>
#ifdef Djabber
# undef DEBUG
# define DEBUG Djabber
#endif

#include <net.h>
#include <xml.h>
#include <sys/time.h>

#if !__EFUN_DEFINED__(tls_available)
# undef WANT_S2S_TLS
#endif

#ifndef NO_INHERIT
// should this be virtual?
virtual inherit JABBER_PATH "common";
#endif

#define COMBINE_CHARSET	XML_CHARSET

// eigentlich schon ein fall für textdb
// siehe auch MISC/jabber/conference.fmt
#define PLACE "place"

#define XMPP	"xmpp:"

#define IQ_OFF	"</query></iq>"

#define NS_XMPP "urn:ietf:params:xml:ns:"

#define IMPLODE_XML(list, tag) pointerp(list) ? tag + implode(list, "</" + tag[1..] + tag) + "</" + tag[1..] : tag[..<2] + "/>"

#define JABBERTIME(gm) sprintf("%d%02d%02dT%02d:%02d:%02d", gm[TM_YEAR], gm[TM_MON] + 1, gm[TM_MDAY], gm[TM_HOUR], gm[TM_MIN], gm[TM_SEC])

#define xbuddylist v("peoplegroups")

// usage: STREAM_ERROR("system-shutdown", "bye!")
// TODO: would be nice to see
#define STREAM_ERROR(condition, textual) emit("<stream:error><" condition " "\
	"xmlns='urn:ietf:params:xml:ns:xmpp-streams'/>"\
	"<text xmlns='urn:ietf:params:xml:ns:xmpp-streams'>" +(textual)+ \
	"</text></stream:error></stream:stream>");

#define SASL_ERROR(condition) emit("<failure "\
				   "xmlns='urn:ietf:params:xml:ns:xmpp-sasl'>"\
				   "<" + condition + "/>"\
				   "</failure>"\
				   "</stream:stream>");
#ifndef PREFERRED_HASH
# define PREFERRED_HASH 1 // sha1 is usually available
#endif
#ifdef SYSTEM_SECRET
# if __EFUN_DEFINED__(hmac)
#  define DIALBACK_KEY(id, source, target) hmac(PREFERRED_HASH, hash(PREFERRED_HASH, SYSTEM_SECRET), target + " " + source + " " + id)
# else
#  define DIALBACK_KEY(id, source, target) sha1(sha1(SYSTEM_SECRET) + " " + target + " " + source + " " + id) 
# endif
#else
  // ssotd is already a hash and therefore, the length is sufficient
# if __EFUN_DEFINED__(hmac)
#  define DIALBACK_KEY(id, source, target) hmac(PREFERRED_HASH, server_secret_of_the_day(), target + " " + source + " " + id)
# else 
#  define DIALBACK_KEY(id, source, target) sha1(server_secret_of_the_day() + " " + target + " " + source + " " + id)
# endif
#endif

#ifndef _host_XMPP
# define _host_XMPP SERVER_HOST
#endif

// this is not ready for is_localhost
#define is_localhost(a) (a) == _host_XMPP


#define JABSOURCE "_INTERNAL_source_jabber"
#define JABTARGET "_INTERNAL_target_jabber"

#define JID_HAS_RESOURCE(x) (index(x, '/') != -1)
#define JID_HAS_NODE(x) apply(jid_has_node_cl, x)

#ifdef _flag_provide_places_only
# define PLACEPREFIX ""
# define ISPLACEMSG(x) 1
# define FIND_OBJECT(x) find_place(x) 
# define PREFIXFREE(x) (x)
#else
// changing the place prefix requires changing it also in the output of
// uniforms. depending on the character you choose you may also have to
// implement URI (un)escaping first.
# define PLACEPREFIX "*"
# define FIND_OBJECT(x) (x[0] == '*' ? find_place(x[1..]) : summon_person(x) )
# define ISPLACEMSG(x) (x && x[0] == '*')
# define PREFIXFREE(x) (x[1..])
#endif

// using channels is funny.. but xmpp: doesn't define channels really.
// isn't it fine if we just use xmpp: resources in _context to achieve this?
//efine MUCSUC_SEP "#_"
#define MUCSUC_SEP "/"

#endif // _JABBER_H
