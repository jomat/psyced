// $Id: time.c,v 1.3 2007/09/18 08:37:58 lynx Exp $ // vim:syntax=lpc

#include <net.h>

volatile mapping cmonth;

// produces a date/time according to ISO 8601
varargs string isotime(mixed ctim, int long) {
	array(string) t;
	string res;

	unless (ctim) ctim = ctime();
	else if (intp(ctim)) ctim = ctime(ctim);
	if (ctim[8] == ' ') ctim[8] = '0';
	t = explode(ctim, " ");
	unless (cmonth) cmonth = ([
	    "Jan" : "01", "Feb" : "02", "Mar" : "03", "Apr" : "04",
	    "May" : "05", "Jun" : "06", "Jul" : "07", "Aug" : "08",
	    "Sep" : "09", "Oct" : "10", "Nov" : "11", "Dec" : "12"
	]);
	res = t[4] +"-"+ cmonth[t[1]] +"-"+ t[2];

	// official would be to have "T" instead of " " there,
	// but all the world prefers readable ISO timestamps.
	return long ? res+" "+t[3] : res;
}

string timedelta(int secs) {
	string t = "";
	int y, d, h, m;

	if (secs < 60) return "--:--";
	if (y = secs/86400/365) {
		t += y + "y ";
		secs = secs % (86400*365);
	}
//	if (d = secs/86400/30) {
//		t += d + "M ";
//		secs = secs % (86400*30);
//	}
	if (d = secs/86400) {
		if (y) return t + d +"d";
		t += d + "d ";
		secs = secs % 86400;
	}
	if (h = secs/3600) secs = secs % 3600;
	if (d) return t+ to_string(h) +"h";
	// if (m = secs/60) secs = secs % 60;
	// t += sprintf("%02d:%02d:%02d", h, m, secs);
	m = secs/60;
	t += sprintf("%02d:%02d", h, m);
	// t += hhmm(ctime(secs - 60*60));
	return t;
}

