#include "base64.c"

#if 0 // inline this (see below)
static string xx2c(string xx) {
    string c = " ";
    c[0] = hex2int(xx);
    return c;
}
#endif

string hmac_bin(int method, string key, string arg) {
    string c = " ";
    return regreplace(hmac(method, key, arg), "..", (:
	    c[0] = hex2int($1);
	    return c;
						    :), 1);
}

string hmac_base64(int method, string key, string arg) {
    return encode_base64((int *)hmac_bin(method, key, arg));
}
