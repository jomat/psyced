#ifndef _INCLUDE_PATH_H
#define	_INCLUDE_PATH_H

#define	NET_PATH	"/net/"
#define	PLACE_PATH	"/place/"
#define	SERVICE_PATH	"/service/"
#define	DATA_PATH	"/data/"
#define	CONFIG_PATH	"/local/"
#define	DAEMON_PATH	NET_PATH "d/"
#define	GATEWAY_PATH	NET_PATH "gateway/"

// protocol for synchronous conferencing
#define	PSYC_PATH	"/net/psyc/"

// experimental PSYC 1.0 interface
#define	SPYC_PATH	"/net/spyc/"

// irc server emulation
#define	IRC_PATH	"/net/irc/"

// jabber server emulation
#define	JABBER_PATH	"/net/jabber/"

// telnet access
#define	TELNET_PATH	"/net/tn/"

// java applet server, uses a very simple protocol
#define	APPLET_PATH	"/net/applet/"

// accept messages and simple mails via smtp
#define	SMTP_PATH	"/net/smtp/"

// experimental: access message log via pop3
#define	POP3_PATH	"/net/pop/"

// experimental: serve as a sip "proxy"
#define	SIP_PATH	"/net/sip/"

// allow access to subscribed threaded discussion groups via nntp
#define	NNTP_PATH	"/net/nntp/"

// accept messages and allow lastlog access via wap
#define	WAP_PATH	"/net/wap/"

// simple http server
#ifndef HTTP_PATH
# define    HTTP_PATH	"/net/http/"
#endif

#endif
