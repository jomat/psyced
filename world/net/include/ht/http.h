// $Id: http.h,v 1.7 2008/04/22 22:43:56 lynx Exp $ // vim:syntax=lpc:ts=8
/*
 * NAME:	http.h
 * DESCRIPTION:	macros for HTTP
 */

#ifndef _INCLUDE_HT_HTTP_H
#define _INCLUDE_HT_HTTP_H

#define HTTP_SERVER	SERVER_VERSION " " DRIVER_VERSION
#define HTTP_SVERS	"HTTP/1.0"

// HTTP server replies

#define R_OK		200
#define R_CREATED	201
#define R_ACCEPTED	202
#define R_PARTIAL	203
#define R_NORESPONSE	204

#define R_MOVED		301
#define R_FOUND		302
#define R_METHOD	303
#define R_NOTMODIFIED	304

#define R_BADREQUEST	400
#define R_UNAUTHORIZED	401
#define R_PAYMENTREQ	402
#define R_FORBIDDEN	403
#define R_NOTFOUND	404

#define R_INTERNALERR	500
#define R_NOTIMPLEM	501
#define R_TEMPOVERL	502
#define R_GATEWTIMEOUT	503

#ifndef hthead
# define hthead(TITLE)	"<title>" CHATNAME " - "+( TITLE )+"</title>"
#endif

// local debug messages - turn them on by using psyclpc -DDhttp=<level>
#ifdef Dhttp
# undef DEBUG
# define DEBUG Dhttp
#endif

#endif
