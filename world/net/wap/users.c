// $Id: users.c,v 1.7 2007/10/02 10:35:46 lynx Exp $ // vim:syntax=lpc
//
// inspect local /who list from a WAP phone
//
#include <net.h>
#include "wap.h"

htget(prot, query, headers) {
	array(string) u;
	string list;
	string n;
	int i;

	HTOK;
	write(WML_START + "<card title='" CHATNAME " chatters'><p>");

	u = objects_people();
	list = "";
	for (i = sizeof(u)-1; i>=0; i--) {
		n = u[i]->qName();
		if (n) {
			if (list != "") list += ",\n";
			list += n;
		}
	}
	write(list);

	write("</p></card>\n" + WML_END);

	log_file("WAP", "[%s] %s users %O\n",
#ifdef _flag_log_hosts
                 query_ip_name(),
#else
                 "?",
#endif
                 ctime(), headers);
	return 1;
}
