#include <net.h>
#include <text.h>
#include "wap.h"

htget(prot, query, headers) {
	sTextPath(query["layout"], query["lang"], "wml");
	PT(("%O using %O\n", ME, _tpath))
	if (strstr(headers["user-agent"], "Mozilla") != -1) {
		htok(prot);
		w("_error_invalid_agent_HTML");
		return 1;
	}
	HTOK;
	// TODO!!! index should support all of barts new features!
	w("_CARD_index", 0, ([
	    "_host":		query_ip_name()			|| "?",
	    "_version_agent":	headers["user-agent"]		|| "?",
	    "_list_charset":	headers["accept-charset"]	|| "?", // _tab
	]));
	return 1;
}

