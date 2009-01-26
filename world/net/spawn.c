// $Id: spawn.c,v 1.17 2008/03/29 20:05:32 lynx Exp $ // vim:syntax=lpc
//
#include <net.h>
#include <erq.h>
#include <errno.h>

inherit NET_PATH "queue";

volatile private string ticket;
volatile private int spawned;
volatile private closure callback;
volatile private mixed lastid;

public int spawn(string command, string params, closure cb) {
    if (spawned)
		return 0;

    P0(("\n%O spawning %O\n", ME, command))
    unless (send_erq(ERQ_SPAWN, command + " " + params,
	     lambda(({ 'data }),
		    ({ symbol_function("parse_erq", ME), ERQ_SPAWN, 'data })
		    ))) {
	P0(("%O failed to send_erq(ERQ_SPAWN, %O..\n", ME, command))
    }

    callback = cb;
    spawned = 1;
    return 1;
}

public int qSend() {
    array(mixed) data;

    unless (ticket)
		return 0;

    P2(("ticket, data: %O,%O\n",ticket, data))
    unless (qExists("spawn") && qSize("spawn"))
		return 0;

    data = shift("spawn");
    P2(("DATA[0]: %O\n", data && data[0]))
    unless (send_erq(ERQ_SEND,
	     ticket + to_array(data[1] + "\n"),
	     lambda(({ 'data }),
                    ({ symbol_function("parse_erq", ME), ERQ_SEND, 'data, data[0] })
		   ))) {
	P0(("%O failed to send_erq..\n", ME))
    }
    return 1;
}

public int send(string data, mixed id) {
    P2(("id: %O\n", id))

    unless (qExists("spawn"))
	qInit("spawn", 100, 50);

    enqueue("spawn", ({ id, data }));
    qSend();
    return 1;
}

public int unspawn() {
    unless (ticket) 
	    return 0;

    P0(("%O stopping %O\n", ME, ticket))
    unless (send_erq(ERQ_KILL,
	     ticket + ({ 15 }),
	     lambda(({ 'data }),
                    ({ symbol_function("parse_erq", ME), ERQ_KILL, 'data })
                   ))) {
	P0(("%O failed to send_erq..\n", ME))
    }
    return 1;
}

public varargs parse_erq(int code, string data, mixed id) {
    data = map(data, #'&, 255);
    
    unless (data && sizeof(data) > 1) {
	// macgruder experienced that unspawn() triggers this...
	P1(("parse_erq got called with %O,%O,%O\n", code,data,id))
	return 0;
    }
    P2(("code, data[0], data[1..], id: %O, %O, %O, %O\n", code, data[0], to_string(data[1..]), id))
    
    switch (data[0]) {
    case ERQ_STDOUT:
	funcall(callback, to_string(data[1..]), lastid);
	lastid = 0;
    break;
    case ERQ_OK:
	switch (code) {
	case ERQ_SPAWN:
	    ticket = data[1..];	
	    qSend();
	    break;
	case ERQ_SEND:
	    lastid = id;
	    qSend();
	    break;
	}
    break;
    default:
    }
}

int qSpawned() {
    if (ticket) return 1;
    return 0;
}

