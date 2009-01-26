// $Id: udp.c,v 1.15 2008/03/29 20:05:32 lynx Exp $ // vim:syntax=lpc
//
// interface to the erq (external request daemon) to handle
// multi-protocol UDP, since the driver only has one port
// which we use for native PSYC. okay fippo also uses it for SIP,
// but i'm not sure if that's going to stay that way.

#include <net.h>
#ifdef __ERQ_MAX_SEND__

#include <erq.h>
#include <errno.h>
#include <closures.h>

//#include NET_PATH "queue.c"
inherit NET_PATH "queue";


volatile private mapping udp_ports, alias;
#endif

// port || alias -> data
// port -> alias
public varargs int ticket(closure callback, int localport, string remotehost, int remoteport);
public int send(int localport, string host, int remoteport, string data);
private void sendUDP(int localport);
public varargs mixed listenPort(int port, mixed param, varargs mixed * args);
public int closePort(mixed port);
void process_erq(int port, string type, int * response_data, int len);
public void getRPort(int port);
public varargs mixed listPorts(int m);
private void reset();

#ifdef __ERQ_MAX_SEND__

#define TICKETS_MAX	100 // maximum size of the tickets array
#define TICKETS_PRE	50 // size to allocate
#define QUEUE_MAX	100
#define QUEUE_PRE	50

// sockets states
#define S_WAITING 	-1
#define S_ERROR 	0
#define S_OK 		1
#define S_BLOCKED 	2
#define S_SENDING 	3
#define S_CLOSED	4

// send-queue packet-array
#define P_LOCALPORT 	0
#define P_REMOTEHOST	1
#define P_REMOTEPORT 	2
#define P_DATA		3

// socket-array
#define STATE 		0
#define TICKET 		1
#define CALLBACK	2
#define PORT		3

//load() { return ME; }
/*
public void reset(int a) {
	if(!mappingp(udp_ports)) udp_ports = ([ ]);
	if(!mappingp(alias)) alias = ([ ]);
	::reset(a);
}
*/
public varargs mixed listPorts(int m) {
	string message, port;
	mapping states;
	reset();
	states = ([ -1 : "S_WAITING", 0 : "S_ERROR", 1 : "S_OK", 
		     2 : "S_BLOCKED", 3 : "S_SENDING", 4 : "S_CLOSED" ]);
	if (m) {
	    mixed value, ports;

	    ports = ([ ]);
	    foreach (port, value : udp_ports) {
		ports += ([ port : ([ "_port" : port,
				     "_state" : states[value[STATE]],
				     "_callback" : to_string(value[CALLBACK])
				   ]) ]);
	    }
	    return ports;
	}
	
	message = "===================\n\tPORT\tSTATE\tCALLBACK\n";
	foreach(port : m_indices(udp_ports)) {
		message += "\t"+port+"\t"+states[udp_ports[port][STATE]]+"\t"
			+to_string(udp_ports[port][CALLBACK])+"\n";
	}
	return message;
}

public varargs int ticket(closure callback, mixed localport, string remotehost, int remoteport) {
	mixed key;
	
	reset();
	unless(udp_ports[to_string(localport)]) {	
		if(!alias[to_string(localport)]) {
			if(stringp(localport)) {
				unless (listenPort(0,localport,callback))
					return 0;
			} else {
				unless (listenPort(localport,callback))
					return 0;
			}
		} else {
			localport = alias[to_string(localport)];
		}
	}
	localport = to_string(localport);
	if(remotehost && sscanf(remotehost,"%~D.%~D.%~D.%~D") != 4) {
		closure c = lambda(
		   ({ 'name }),
		   ({ 
			CL_IF, 'name,
				({ (#'funcall), #'ticket,
						callback, localport, 'name, 
						remoteport
				})
			}));
		dns_resolve(remotehost, c);
		return 1;
	}
	if(remoteport) {
		key = localport + ":" + remotehost + ":" + remoteport;
	} else if(remotehost) {
		key = localport + ":" + remotehost;
	} else {
		key = localport;
	}
	unless(qExists( key )) {
		qInit(key, TICKETS_MAX, TICKETS_PRE);		
	}
	enqueue(key, callback);
	return 1;
}

public int send(mixed localport, string remotehost, int remoteport, string data) {
	
	unless(udp_ports[to_string(localport)]) {	
		if(!alias[to_string(localport)]) {
			if(stringp(localport)) {
				unless (listenPort(0,localport))
					return 0;
			} else {
				unless (listenPort(localport))
					return 0;
			}
		} else {
			localport = alias[to_string(localport)];
		}
	}
	localport = to_string(localport);
	if(remotehost && sscanf(remotehost,"%~D.%~D.%~D.%~D") != 4) {
		closure c = lambda(
		   ({ 'name }),
		   ({ 
			CL_IF, 'name,
			    ({ (#'funcall), #'send, 
					    localport, 'name,
					    remoteport, data
			    })
		   }) );
		dns_resolve(remotehost, c);
		return 1;
	}	
	if(udp_ports[localport][STATE] != S_BLOCKED) {
		if(!enqueue(":" + localport,({
				 localport,remotehost,remoteport,
				 to_array(data)[0..strlen(data)-1],
				 }) )) return 0;
		sendUDP(localport);
		return 1;
	}
	return 0;
}

private void sendUDP(mixed localport) {
	string packet;
	mixed data;
	int * ip;
	unless(udp_ports[localport]) return 0;
	switch(udp_ports[localport][STATE]) {
	case S_OK:
		if(!qSize( ":" + localport )) 
					return;
		data = shift( ":" + localport );
		ip = map(explode(data[P_REMOTEHOST],"."),#'to_int);
		packet = udp_ports[localport][TICKET] 
				+ ip[0..3] 
				+ ({ data[P_REMOTEPORT] / 256, data[P_REMOTEPORT] & 255 }) 
				+ data[P_DATA];
		unless (send_erq(ERQ_SEND, packet,
				 lambda(({ 'data, 'len }),
				({
				 #'process_erq, localport, 
				 ERQ_SEND, 'data, 'len
				 })
				))) {
			P0(("%O failed to ERQ_SEND!\n", ME))
		}
		udp_ports[localport][STATE] = S_SENDING;
		D2(D("============\nOutgoing UDP-packet on port "+localport
		     +"\nto: "+data[P_REMOTEHOST]
		     +":"+data[P_REMOTEPORT]+"\n============\n");)
		return;
	case S_ERROR:
		
		return;
	case S_WAITING:
		
		return;
	}
}

public void getRPort(int port) {
	// first we will try to bind port 3645
	// if that fails ++ until it works fine ,)
	// then we will send a packet over the new
	// unknown port which is still represented by udp_ports[0]
	// to our new port 3645+n. Tada! we know our former unknown
	// port. Then we just change 0 to x.
	// 
	// and everything put into one large lambda.. .]
	// -el
			
	if (send_erq(ERQ_OPEN_UDP,({ port/256, port&255 }),
			 lambda(({ 'data, 'len }),
	    ({	(#',), 
		    ({ (#'=),'data,({ (#'map),'data,#'&,255 }) }),
		    ({	#'switch, ({ CL_INDEX, 'data, 0 }),
			    ({	ERQ_OK	}),
			    ({ (#',),
			     ({ #'send,0,"127.0.0.1",port,"PORT" }),
			     ({ (#'=), ({ CL_INDEX, ({ CL_INDEX, udp_ports, port }), TICKET }), ({ CL_L_RANGE, 'data, 1 }) }),
			     ({ (#'=), ({ CL_INDEX, ({ CL_INDEX, udp_ports, port }), STATE }), S_OK }),							 
			    }),
			    (#'break),
			    ({ ERQ_E_UNKNOWN }),
			    ({ CL_IF, ({ #'==,({ CL_INDEX, 'data, 1}), 98 }),
				({ #'getRPort, port+1 }),
				({ (#'return),0 }),
			    }),
			    (#'break),
			    ({ ERQ_STDOUT }),
			    ({	(#',),
				({ (#'=),'r_host,({ (#'sprintf),"%d.%d.%d.%d", ({ CL_INDEX, 'data, 1 }), ({ CL_INDEX, 'data, 2 }),	({ CL_INDEX, 'data, 3 }), ({ CL_INDEX, 'data, 4 }) }) }),
				({ CL_IF, ({ #'&&, ({	#'==,'r_host,"127.0.0.1" }),({	#'==,({	(#'to_string),({ CL_RANGE,'data,7,11 }) }),"PORT" }), }),
				    ({ (#',),
					({ (#'=),'l_port, ({	(#'+), ({ CL_INDEX,'data,6 }),({	(#'*),256,({ CL_INDEX,'data,5 }) }) }) }),
					({ #'qRename,":0",	({ (#'+),":",'l_port }) }),
					({ (#'=), ({ CL_INDEX, udp_ports, 'l_port }), ({ CL_INDEX, udp_ports, to_string(0) }) }),
					({ (#'m_delete), udp_ports, to_string(0) }),
					({ #'closePort, port}),
					({ (#'=), 'callback, ({ CL_INDEX, ({ CL_INDEX, udp_ports, 'l_port }), CALLBACK }) }),
					({ #'closePort, 'l_port }),
					({ #'listenPort, 'l_port, 'callback }),
					({ (#'funcall), 'callback, 0, 'l_port }),
				    }),
				    ({ (#'return),0 })
				}),
			    }),
			    (#'break),
			}) 
		    }))))
       	{
		qInit(":" + port, QUEUE_MAX, QUEUE_PRE);
		udp_ports += ([ port : ({ S_WAITING,0,lambda(({ 'data, 'len }), ({ #'process_erq, port, ERQ_OPEN_UDP, 'data, 'len }) )}) ]);
	} else {
		P0(("%O failed to ERQ_OPEN_UDP!\n", ME))
		return 0;
	}

	/*	switch(data[0]) {
	 *	case ERQ_OK:
	 *	send data to that port over unknown port which is still 
	 *	represented by udp_ports[0] callback must be the 0 to x changer!
	 *	break;
	 *	case ERQ_E_UNKNOW:
	 *	if(data[1] == 98) { // 98 == Adress allready in use
	 *		bind port+1
	 *		recall myself
	 *		}
	 *	break;
	 *	case ERQ_STDOUT:
	 *		extract port and make the movement!
	 *	break;
	 *	}
	 */
}

public varargs mixed listenPort(int port, mixed param, varargs mixed * args) {
	unless (mappingp(udp_ports))
		reset();
	if(qExists(":" + port) && udp_ports[to_string(port)]) return 0;
	if(!port && !param) return 0; 
	// doesnt make any sence to bind to a
	// random port without getting to know
	// which it is.
	mixed id;
	closure callback;
	if(!port && stringp(param)) {
		while (udp_ports[param]) {
			param = to_string(({ random(150)+1,random(150)+1 }));
		}
		id = param;
		if(sizeof(args) && closurep(args[0])) callback = args[0]; 
	} else {
		id = to_string(port);	
	}
	if(closurep(param)) callback = param;
	
	D2(D("============\nlistenPort("+(port || id)
	     +") called!\n============\n");)
	closure c = lambda(
		   ({ 'data, 'len }),
		   ({
			#'process_erq, id, 
			ERQ_OPEN_UDP, 'data, 'len
			})
		   );								
	if (send_erq(ERQ_OPEN_UDP,({ port/256, port&255 }),c)) {
		qInit(":" + id, QUEUE_MAX, QUEUE_PRE);
		udp_ports += ([ id : ({ S_WAITING,0,callback || 0 }) ]);
		if(stringp(param)) return id;
		return 1;
	} else {
		P0(("%O failed to ERQ_OPEN_UDP!\n", ME))
		return 0;
	}
	
}

public int closePort(mixed port) {
	port = to_string(port);
	if(!udp_ports[port] && (!alias[port] && !udp_ports[alias[port]])) return 0;
	D2(D("============\nclosePort("+port+") called!\n============\n");)
	send_erq(ERQ_KILL,
		 (udp_ports[port][TICKET] || udp_ports[alias[port]][TICKET]) 
		    + ({0,0,0,0}),
		 lambda(({ 'data, 'len }),
			({ #'process_erq, port,
			 ERQ_KILL, 'data, 'len })
			));
	udp_ports -= ([ port ]);
	udp_ports -= ([ alias[port] ]);
	alias -= ([ port ]);
	qDel(":" + port);
}

void process_erq(mixed port, string type, int * response_data, int len) {
	response_data = map(response_data,#'&,255);
	port = to_string(port);
	int remoteport;
	string remotehost;
	mixed callback;

	switch (response_data[0]) {
// BIND and SEND
	case ERQ_OK: // port bound || package sent !
		switch (type) {
		case ERQ_OPEN_UDP:
			D2(D("============\nPORT \""+port+"\" bound for ticket: \""+to_string(response_data[1..])+"\"\n============\n");)
			udp_ports[port][TICKET] = response_data[1..];
			udp_ports[port][STATE] = S_OK;
			if(port == "0" && udp_ports[port][CALLBACK]) { 
				// no alias, cb
				getRPort(3654);
			} else {
				sendUDP( port );
			}
			break;
		case ERQ_SEND:
			udp_ports[port][STATE] = S_OK;
			sendUDP( port );
			break;
		case ERQ_KILL:
			break;
		}
		return;
	case ERQ_E_UNKNOWN: // unknown error while binding || sending
		switch(response_data[1]) {
		case EADDRINUSE:
			D2(D("Port "+port+" allready in use by someone else!\n");)
		default:
			D2(D("Unknown error while Binding or Sending!"
			     +to_string(response_data[1..])+"\n");)
		}
		return;
// BIND
	case ERQ_E_NSLOTS:// The max number of child processes is exhausted.
		udp_ports -= ([ port ]);
		return;
	case ERQ_E_ARGLENGTH: // The port number given does not consist of two bytes
		return;		
// SEND
	case ERQ_E_TICKET: // ticket invalid!
		if(!unshift(":" + port )) { 
			D2(D("PANIC! Mysteriously a UDP-packet got lost in the"
			     "queue!\n");) 
		}
		if(!to_int(port) || to_string(to_int(port)) != port) {
			listenPort(0, port );
		} else {
			listenPort( to_int(port), udp_ports[port][CALLBACK] || 0);
		}
		udp_ports -= ([ port ]);
		return;
	case ERQ_E_INCOMPLETE: // only a part of the message has been sent
		return;
	case ERQ_E_WOULDBLOCK: // erq allready has a packet in the queue..
		if(!unshift( port )) {
			D2(D("You are too fast, young Jedi!\n");)
		}
		return;
	case ERQ_E_PIPE: // pipe error. <info>
		
		return;
// INCOMING UDP 
	case ERQ_STDOUT: // incoming data on udp-port
		remotehost = sprintf("%d.%d.%d.%d", response_data[1], 
				     response_data[2], response_data[3], 
				     response_data[4]);
		remoteport = response_data[5] * 256 + response_data[6];
		
		string key = qSize(port + ":" + remotehost + ":" + remoteport,
				   port + ":" + remotehost,
				   port);
		if(key) {
			callback = shift( key );
		} else {
			callback = udp_ports[port][CALLBACK];
		}
		D2(D("============\nIncoming UDP-packet:\n from: "+port + ":" 
		     + remotehost + ":" + remoteport +"\nticket: \""+ key 
		     + "\" callback: "+to_string(callback)+"\n============\n");)
		if(closurep(callback))
			funcall(callback,1,to_string(response_data[7..]), port, 
				remotehost, remoteport);
		return;
	}
}

void create() {
	alias = ([ ]);
	udp_ports = ([ ]);
}

load() { return ME; }

#else
public varargs int ticket(closure callback, int localport, string remotehost, int remoteport) {
    D2(D("You need an erq to use d/udp.c\n");)
    return 0;
}
public int send(int localport, string host, int remoteport, string data) {
    D2(D("You need an erq to use d/udp.c\n");)
    return 0;
}
public varargs mixed listenPort(int port, mixed param) {
    D2(D("You need an erq to use d/udp.c\n");)
    return 0;
}
public void getRPort(int port) {
    D2(D("You need an erq to use d/udp.c\n");)
}
public varargs mixed listPorts(int m) {
    D2(D("You need an erq to use d/udp.c\n");)
    if (m)
	return ([ ]);
    return "";
}
load() {
    return ME;
}
#endif

void reset() { }
