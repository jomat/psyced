#include <net.h>
#include <person.h>
#include <status.h>

inherit NET_PATH "place/threads";

load(name, keep) {
	P3((">> userthreads:load(%O, %O)\n", name, keep))
	string nick;

	if (sscanf(name, "~%s#updates", nick))
	    vSet("owners", ([ nick: 0 ]));

	return ::load(name, keep);
}
