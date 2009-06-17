// $Id: peers.h,v 1.19 2007/08/10 09:27:04 lynx Exp $ // vim:syntax=lpc:ts=8
//


// http://about.psyc.eu/Peer
#ifndef PEER_H
#define PEER_H
#define PEER_ALIAS	0
#define PEER_PRESENCE	1
#define PEER_FLAGS	2
#define PEER_CHANNELS	3
#define PEER_CUSTOM	4
#define PEER_SIZE	5


#define PPL_DISPLAY	0

#define PPL_DISPLAY_NONE	'0'
#define PPL_DISPLAY_SMALL	'2'
#define PPL_DISPLAY_REGULAR	' '
#define PPL_DISPLAY_BIG		'6'
#define PPL_DISPLAY_DEFAULT	PPL_DISPLAY_REGULAR

#define PPL_NOTIFY	1

// this model does not handle the "None + Pending Out/In" state in
// http://xmpp.org/rfcs/rfc3921.html#substates - in psyc, if two
// people intend to subscribe to each other, they are either upgraded
// to friendship aka "Both," or rather - the actual subscription state
// on the other side is not stored here, except for the special case
// of PPL_NOTIFY_OFFERED.
//
// if a full implementation of XMPP requires local storage of whether the
// other side intends to send us presence (even though she can actually do
// whatever she wants, so the information doesn't seem very useful and is
// in fact very likely to go out of sync), we'd have to add a new flag class.
// something like PPL_SUBSCRIBED or PPL_FOLLOW.
//
// this all clashes with the PSYC model of context subscriptions - we should
// throw away all of these PPL_ subscription flags, and model all xmpp
// friendship states with generic context subscriptions - no matter if we
// are dealing with people, places or other pubsub apps. seen from this
// perspective, "None + Pending Out/In" is equivalent to a pair of
// _request_context_subscribe's which haven't been answered yet. we need
// a generic per-entity way to store these states, below user level.
//
#define PPL_NOTIFY_IMMEDIATE	'8'
#define PPL_NOTIFY_DEFAULT	PPL_NOTIFY_IMMEDIATE
#define PPL_NOTIFY_DELAYED	'6'
#define PPL_NOTIFY_DELAYED_MORE	'4'
#define PPL_NOTIFY_FRIEND	PPL_NOTIFY_DELAYED_MORE
#define PPL_NOTIFY_MUTE		'2'
#define PPL_NOTIFY_PENDING	'1'	// friendship request sent
#define PPL_NOTIFY_OFFERED	'0'	// friendship request received
#define PPL_NOTIFY_NONE		' '

#define TIME_DELAY_NOTIFY	(60 * 3)	// 3 minutes

#define PPL_EXPOSE	2

// this stuff isn't implemented yet, but we could do it like this..
#define PPL_EXPOSE_DEFAULT	' '	// depends on /set friendsexpose
#if 0
#define PPL_EXPOSE_NEVER	's'	// keep this friendship secret
#define PPL_EXPOSE_LINK		'l'	// let a client app know this friend
// just compare friendivity with the to_int value of the entry..
#define PPL_EXPOSE_FRIEND	'0'	// let friends see this friend
#define PPL_EXPOSE_FRIEND1	'1'	// let friends of friends..
#define PPL_EXPOSE_FRIEND2	'2'	// friendivity level 2
#define PPL_EXPOSE_FRIEND3	'3'	// friendivity level 3
#define PPL_EXPOSE_FRIEND4	'4'	// friendivity level 4
#define PPL_EXPOSE_FRIEND5	'5'	// friendivity level 5 etc.
#define PPL_EXPOSE_PUBLIC	'P'	// show this friend to anyone
#define PPL_EXPOSE_FLAUNT	'F'	// flaunt this friend on websites
#endif
// right now the semantics of expose are as follows: if the value isn't
// default, then it is '0' - '9' with 0 meaning secret and the rest whatever

#define PPL_TRUST	3		// values from '0' to '9'

#define PPL_TRUST_DEFAULT	' '

#define PPL_ANY		33333
#define PPL_JSON	12345
#define PPL_COMPLETE	4404

#define PPLDEC(CHAR)	CHAR == ' ' ? "-" : (CHAR - '0')
#endif
