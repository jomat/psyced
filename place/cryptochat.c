// this room lets people in who are either connected via a SSL/TLS
// protocol or are coming from the localhost (probably SSH users).
//
// both cases are no absolute guarantee for safety.. it is still
// in the hands of each user in the room to safeguard true secrecy
//
// -lynX 2004

#define NAME	"CryptoChat"
#define SECURE
#include <place.gen>

