// $Id: md5.c,v 1.6 2007/08/10 19:07:44 lynx Exp $ // vim:syntax=lpc:ts=8
#include <net.h>

inherit NET_PATH "spawn";

handback(mixed digest, closure callback) {
    funcall(callback, digest);
    unspawn();
}


dmd5(mixed text, closure callback) {
    // init(); .... init????
    spawn("md5.pl", 0, lambda(({ 'digest }), 
			      ({ symbol_function("handback", ME), 'digest,
			       callback })));
    send(text);
}
