Stdio.File s;
function inputto;

#include <net.h>
#include <url.h>

import net.library.uniforms;
import net.library;

#define PSYC_TCP
#define QUIT byebye("parse error"); return 0;
#include "../../../world/net/psyc/circuit.c"

// START EMULATION (TODO STUFF)

int emit(string whatever) {
        s->write(whatever);
	return 0;
}

string query_ip_number(object|void sucket) {
        mixed rc;
        unless (s) return 0; 
        rc = s->query_address();
        unless (rc) return 0;
        rc = rc / " ";
        if (rc) return rc[0];
}

// END EMULATION

void write_cb() {
}

void close_cb() {
	exit(0);
}

void next_input_to(function cb) {
	//write("next_input_to ("+ function_name(cb) +")\n");
	inputto = cb;
}

int read_cb(mixed id, string data) {
	// line comes with CR LF.. wicked!
	data = slice_from_end(data, 0, 3);
	// probably need to explode here, since input doesnt
	// always come line by line in pike TODO
	write(function_name(inputto) +"(\""+ data +"\")\n");
	inputto(data);
}

int doneParse(mixed ip, string host, string mc,
		 string data, mapping cvars) {
	write("done_parsing(%O, %O, %O, %O).\ncvars = %O\n",
		ip, host, mc, data, cvars);
	write("\nTest successful. Good bye.\n");
	exit(0);
}
                           
void remove_interactive(object who) {
	s->close();
}

protected int quit() {
	byebye("timeout");
}

protected void byebye(string reason) {
	write("%O self-destructing because of %s\n", ME, reason || "timeout");
	s->close();
	destruct(ME);
}

