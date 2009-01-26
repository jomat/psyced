#ifndef _INCLUDE_SERVICES_H
#define _INCLUDE_SERVICES_H

// like /etc/services

// reserved for MSP == http://www.faqs.org/rfcs/rfc1312.html
#define	MSP_SERVICE		18	// PSYC port of lords again.. maybe?
#define	FTP_DATA_SERVICE	20
#define	FTP_SERVICE		21
#define	SSH_SERVICE		22
#define	TELNET_SERVICE		23
#define	SMTP_SERVICE		25
#define	DOMAIN_SERVICE		53	// DNS name service
#define	GOPHER_SERVICE		70
#define	FINGER_SERVICE		79
#define	HTTP_SERVICE		80
#define	POP2_SERVICE		109
#define	POP3_SERVICE		110
#define	NNTP_SERVICE		119
#define	EXTRA_IRC_SERVICE	194	// the official but unused irc port
#define	HTTPS_SERVICE		443	// http over TLS/SSL
#define	SMTPS_SERVICE		465
#define	NNTPS_SERVICE		563	// nntp over TLS/SSL
#define	TELNETS_SERVICE		992
#define	IRCS_SERVICE		994	// irc protocol over TLS/SSL
#define	POP3S_SERVICE		995
#define	PSYC_SERVICE		4404	// PSYC port of commons
#define	JABBER_SERVICE		5222
#define	JABBER_S2S_SERVICE	5269	// obscene interserver jabber port
#define	IRC_SERVICE		6667	// de facto irc port
#define	PSYCS_SERVICE		9404	// PSYC interim port of lords

#endif
