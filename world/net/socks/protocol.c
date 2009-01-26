// vim:syntax=lpc:ts=8
/* implementation of the socks5 protocl
 * http://tools.ietf.org/html/rfc1928
 */
#include <net.h> 
#include <input_to.h>
#include "socks.h"

volatile int state;
volatile int addrtype;
volatile string buffer;

volatile mapping authmechs = ([ AUTHMECH_ANON : 1, AUTHMECH_USERPASS :1 ]);
volatile mapping addresstypes = ([ ADDR_IPV4 : 1, ADDR_DOMAINNAME : 1 ]);
volatile mapping commands = ([ CMD_CONNECT : 1, CMD_BIND : 1 ]);


void connectResult(string ho, int po, int result) {
    array(int) repl = ({ SOCKS5_VER, REPLY_SUCCESS, 0, addrtype});
    switch(addrtype) {
    case ADDR_IPV4:
	repl += ({ 0, 0, 0, 0 });
	sscanf(ho, "%d.%d.%d.%d", repl[4], repl[5], repl[6], repl[7]);
	break;
    case ADDR_DOMAINNAME:
	repl += ({ strlen(ho) }) + to_array(ho);
	break;
    }
    repl += ({ po >> 8,  po % 256 });
    binary_message(repl);

    state = STATE_READY;
}

// do connect - if you want to
void do_connect(string ho, int po) {
    // when done call connectResult with error code as defined in section 6 
    // of RFC
    // FIXME: this needs to be implemented according to programmers wishes
    P2(("socks: do_connect(%O, %O)\n", ho, po))
    state = STATE_CONNECT_PENDING;
    connectResult(ho, po, 0x00);
}

// do bind - if you want to
void do_bind(string ho, int po) {
    // FIXME
}

void parseNegotiation() {
    int version, nmethods;
    array(int) methods = ({ });

    P2(("socks::parseNegotiation\n"))

    if (strlen(buffer) < 2) {
	return;
    }

    version = buffer[0];
    nmethods = buffer[1];
    P2(("socks version %d, nmethods %d\n", version, nmethods))

    if (strlen(buffer) < 2+nmethods) {
	return;
    }
    for (int i = 2; i < 2+nmethods; i++) {
	methods += ({ buffer[i] });
    }

    buffer = buffer[2+nmethods..];
    P3(("methods: %O\n", methods))
    for (int j = 0; j < nmethods; j++) {
	// FIXME: implement support for a preferred authmethod here
	if (authmechs[methods[j]]) {
	    if (methods[j] == AUTHMECH_ANON) {
		P2(("socks -> STATE_REQUEST\n"))
		state = STATE_REQUEST;
	    } else if (methods[j] == AUTHMECH_USERPASS) {
		P2(("socks -> STATE_AUTH_USERPASS\n"))
		state = STATE_AUTH_USERPASS;
	    }
	    binary_message( ({ SOCKS5_VER, methods[j] }) );
	    return;
	}
    }
    binary_message( ({ SOCKS5_VER, AUTHMECH_INVALID }) );
    return;
}

int authenticate(string user, string pass) {
    return 1;
}

void parseUserPass() {
    int version, l, l2;
    string user, pass;

    P2(("socks::parseUserPass\n"))
    if (strlen(buffer) < 2)
	return;
    version = buffer[0];
    l = buffer[1];
    if (strlen(buffer) < 3 + l)
	return;

    user = buffer[2..2+l];
    P2(("user %O\n", user))
    l2 = 3 + l + buffer[3+l];
    if (strlen(buffer) < l2)
	return;
    pass = buffer[3+l..l2];
    P2(("pass %O\n", pass))
    buffer = buffer[l2..];

    if (authenticate(user, pass)) {
	state = STATE_REQUEST;
	binary_message( ({ SOCKS5_VER, AUTHMECH_INVALID }) );
    } else {
	binary_message( ({ SOCKS5_VER, AUTHMECH_INVALID }) );
	remove_interactive(ME);
    }
}

void parseRequest() {
    int version, cmd, rsvd;

    string connhost;
    int connport;

    if (strlen(buffer) < 4) {
	P2(("socks - parseRequest needs more\n"))
	return;
    }
    version = buffer[0];
    cmd = buffer[1];
    rsvd = buffer[2];
    addrtype = buffer[3];
    P2(("socks::parseRequest(%d, %d, %d, %d)\n", version, cmd, rsvd, addrtype))

    unless(addresstypes[addrtype]) {
	SOCKS_ERROR(REPLY_ADDR_NOT_SUPPORTED);
	return;
    }

    switch(addrtype) {
    case ADDR_IPV4:
	if (strlen(buffer) < 10) return;
	connhost = sprintf("%d.%d.%d.%d", buffer[4], buffer[5], buffer[6], buffer[7]);
	connport = (buffer[8] << 8) + buffer[9];
	buffer = buffer[10..];
	PT(("host %O port %d\n", connhost, connport))
	break;
//    case ADDR_IPV6:
//	if (strlen(buffer) < 12) return;
//	break;
    case ADDR_DOMAINNAME:
	if (strlen(buffer) < 5 || strlen(buffer) < (5 + buffer[4])) return;
	connhost = buffer[5..(5 + buffer[4])];
	buffer = buffer[(5+buffer[4])..];
	connport = (buffer[0] << 8) + buffer[1];
	buffer = buffer[2..];
	break;
    default:
	SOCKS_ERROR(REPLY_ADDR_NOT_SUPPORTED);
	break;
    }

    unless(commands[cmd]) {
	SOCKS_ERROR(REPLY_CMD_NOT_SUPPORTED);
    }
    switch(cmd) {
    case CMD_CONNECT:
	do_connect(connhost, connport);
	break;
    case CMD_BIND:
	do_bind(connhost, connport);
	break;
    }
}


// handle data
void handle(string data) {
    P2(("handle %O\n", data))

}

void read_callback(string data) {
    P3(("read_callback with %d bytes\n", strlen(data)))

    input_to(#'read_callback, INPUT_IGNORE_BANG);

    if (state == STATE_READY) {
	handle(data);
	return;
    } else {
	buffer += data;
	if (state == STATE_INITIAL) {
	    parseNegotiation();
	}
	if (state == STATE_AUTH_USERPASS) {
	    parseUserPass();
	}
	if (state == STATE_REQUEST) {
	    parseRequest();
	}
    }
}

create() {
}

void logon(int success) {
    input_to(#'read_callback, INPUT_IGNORE_BANG);
    state = STATE_INITIAL;
    buffer = "";
    P2(("socks logon\n"))
}

disconnected(remainder) {
    // notify any peer if desired
    return 1;   // ignore unless you have a better plan
}
