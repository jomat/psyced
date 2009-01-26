// $Id: sslserver.c,v 1.4 2006/04/18 13:32:31 fippo Exp $ // vim:syntax=lpc
//
// just a little trick to get TLS telnet to work properly

#include <net.h>

inherit TELNET_PATH "server";

/* do not write anything before handshake is complete!
 * TODO: what if handshake fails
 * TODO: is this object necessary for modern drivers?
 */

logon() {
	tls_init_connection(ME, #'::logon);
}
