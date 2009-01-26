// vim:syntax=lpc
//
// $Id: udp.c,v 1.25 2005/03/14 10:23:26 lynx Exp $
//
// imitates the old udp.c.. but used the new daemon
//
//
#include <net.h>

volatile object udpd;

public varargs int ticket(closure callback, int localport, string remotehost, int remoteport);
public int send(int localport, string host, int remoteport, string data);
public varargs mixed listenPort(int port, mixed param, varargs mixed * args);
public int closePort(mixed port);
public void getRPort(int port);
public mixed listPorts(int m);

udpInit() {
    udpd = DAEMON_PATH "udp" -> load();
    unless (udpd) {
	udpd = find_object(DAEMON_PATH);
	D0(unless (udpd) {
	    P0(("Failure. Could not find UDPd (%O)\n", DAEMON_PATH "udp"))
	})
    }
}

public varargs int ticket(closure callback, int localport, string remotehost, int remoteport) {
    unless (udpd) {
	udpInit();
	unless (udpd)
	    return -2;
    }
    return udpd->ticket(callback, localport, remotehost, remoteport);
}

public int send(int localport, string host, int remoteport, string data) {
    unless (udpd) {
	udpInit();
	unless (udpd)
	    return -2;
    }
    return udpd->send(localport, host, remoteport, data);
}
public varargs mixed listenPort(int port, mixed param, varargs int * args) {
    unless (udpd) {
	udpInit();
	unless (udpd)
	    return -2;
    }
    if (sizeof(args)) {
	// does this work?
	return apply(symbol_function("listenPort", udpd), port, param, args);
    } else {
	return udpd->listenPort(port, param);
    }
}

public int closePort(mixed port) {
    unless (udpd) {
	udpInit();
	unless (udpd)
	    return -2;
    }
    return udpd->closePort(port);
}

public void getRPort(int port) {
    unless (udpd) {
	udpInit();
	unless (udpd)
	    return;
    }
    return udpd->getRPort(port);
}

public mixed listPorts(int m) {
    unless (udpd) {
	udpInit();
	unless (udpd)
	    return -2;
    }
    return udpd->listPorts(m);
}
