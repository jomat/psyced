// $Id: sip.h,v 1.8 2005/04/11 18:52:15 fippo Exp $ // vim:syntax=lpc
#ifndef _INCLUDE_SIP_H
#define _INCLUDE_SIP_H

#define SIP "sip:"
#define SIP_UDP "SIP/2.0/UDP"
#define CRLF "\r\n"
#define SIP_MAGIC_COOKIE "z9hG4bK"

#define send_udp_nonblocking(ho, po, buf) dns_resolve(ho, (: if ($1 != -1) { \
				send_udp($1, $2, $3); \
			     } return; :), po, buf);
#endif
