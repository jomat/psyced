#include <net.h>

inherit NET_PATH "library/base64";

#include HTTP_PATH "library.i"

string hmac_bin(int method, string key, string arg) {
    return regreplace(hmac(method, key, arg), "..", #'xx2c, 1); //'
}

string hmac_base64(int method, string key, string arg) {
    return encode_base64((int *)hmac_bin(method, key, arg));
}
