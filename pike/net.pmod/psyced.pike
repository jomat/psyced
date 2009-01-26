// first step at making parts of psyced available for pike programmers.
//
// in theory this could progress up to a point where all of psyced also
// runs on pike by preprocessor and include magic - then one could even
// think of forking away from the psyclpc-driven psyced to a native pike
// psyced. but as you can see it's a long way and it isn't a good idea
// to give up psyced as it is, being a fabulous piece of software.  ;)

Stdio.Port listener;

void create(int port) {
	write("The psyced net/psyc/parse module is connected to port "
	    + port +" right now.\n");
	listener = Stdio.Port(port, accept);
}

void accept() {
	net.psyc.server(listener->accept());
}

