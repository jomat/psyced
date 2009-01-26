// this file should be the pike equivalent of net/psyc/server.c

#include <net.h>

import net.library;

#include "../../../world/net/psyc/server.c"

void create(Stdio.File sucket, string|void x) {
	s = sucket;
	set_server_uniform(x || "psyc://localhost");
	write("\n%O created for new connection\n", this_object());
//	peeraddr = query_ip_number();
	write("peer %O\n", s->query_address());
	write("self %O\n", s->query_address(1));
	s->set_nonblocking(read_cb, write_cb, close_cb);
	logon(0);
}

