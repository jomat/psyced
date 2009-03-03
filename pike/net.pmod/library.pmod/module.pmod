#include <net.h>
#include <uniform.h>
#include <sandbox.h>

// more elegant way to do it
import net.library.uniforms;
// but for now let's just include like crazy
#include "../../../local/config.h"

#define SERVER_HOST "localhost"		// TPD
#undef JABBER_PATH

// this library file clearly no longer belongs into drivers/ldmud  ;)
#include "../../../world/drivers/ldmud/library/library.c"

#include "../../../world/net/library/legal.c"
#include "../../../world/net/library/text.c"
#include "../../../world/net/library/library2.i"
#include "../../../world/net/psyc/library.i"
#include "../../../world/net/library.i"

// EMULATION.. really needs port of library, circuit etc
int trustworthy = 0;
string myuni;
volatile string myIP;

void debug_message(string text, vaint flags) {
	debug_write(text);
}

int is_localhost(string host) {
	return 0;   // TODO
}

void register_localhost(string a, vastring b) {
	//debug_write("register_localhost(%O, %O)\n", a, b);
	return;   // TODO
}

void set_server_uniform(string bla) {
	myuni = bla;
}

string query_ip_name(vamixed ip) {
	return ip;
}

// emulation of LPC write_file efun.
int write_file(string file, string str, vaint flags) {
	Stdio.File fd;
	debug_write("write_file(%O, %O, %O)\n", file, str, flags);
	// should prefix file access with $sandbox
#if 0
	if (flags) rm(file);
	unless (fd = Stdio.File()) return 0;
	unless (fd->open(file, "wa")) return 0;
	if (fd->write(str) == -1) {
		debug_write("Something went wrong in write_file(%O). %O\n",
		    file, errno());
		return 0;
	}
	fd->close();
#endif
	return 1;
}

void rootMsg(mixed source, string mc, string data, vamapping vars, vamixed t) {
        debug_write("wrong.. rootMsg %s\n", mc);
}

void sTextPath() {
	debug_write("sTextPath\n");
}

void sAuthhosts(mapping whatever) {
	debug_write("sAuthhosts %O\n", whatever);
}

int hostCheck(string peerip, int peerport) {
	return 1;
}

// EMULATION

