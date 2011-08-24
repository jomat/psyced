#include <net.h>
#include <ht/http.h>

inherit NET_PATH "http/fetch";

int buffer_content(string data) {
	P2(("%O got data:\n%O\n", ME, data))

	mixed *waiter;
	foreach (waiter : qToArray(ME)) {
		funcall(waiter[0], data, waiter[1] ? fheaders : copy(fheaders), http_status, 1);
	}
	next_input_to(#'buffer_content); //'
	return 1;
}

disconnected(string data) {
	P2(("%O got disconnected:\n%O\n", ME, data))
	headers["_fetchtime"] = isotime(ctime(time()), 1);
	if (headers["last-modified"])
	    rheaders["if-modified-since"] = headers["last-modified"];
	//if (headers["etag"])
	//    rheaders["if-none-match"] = headers["etag"]; // heise does not work with etag

	fheaders = headers;
	buffer = headers = 0;
	switch (http_status) {
	default:
		mixed *waiter;
		while (qSize(ME)) {
			waiter = shift(ME);
			P2(("%O calls back.. body is %O\n", ME, data))
			funcall(waiter[0], data, waiter[1] ? fheaders : copy(fheaders), http_status, 0);
		}
		if (http_status == R_OK) break;
		// doesn't seem to get here when HTTP returns 301 or 302. strange.
		// fall thru
	case R_NOTMODIFIED:
		qDel(ME);
		qInit(ME, 150, 5);
	}
	fetching = 0;
	return 1;       // presume this disc was expected
}
