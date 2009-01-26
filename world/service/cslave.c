// vim:syntax=lpc
#include <net.h>

load() {
    return ME;
}

create() {
    PT(("]] %O loaded\n", ME))
}

msg(source, mc, data, vars) {
    string c;

    PT(("]] %O:msg(%O, %O, %O, %O)\n", ME, source, mc, data, vars))

    if (mappingp(vars) && (c = vars["_context"]) && (c = find_context(c)))
	return c->castmsg(source, mc, data, vars);

    return 0;
}

