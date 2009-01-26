// $Id: hosts.c,v 1.27 2008/03/11 13:42:25 lynx Exp $ // vim:syntax=lpc
//
// keeper of list of offensive hosts

#include <net.h>
#include CONFIG_PATH "hosts.h"

#ifdef legal_host
# undef legal_host
#endif

#ifndef DISABLED_HOSTS
# define DISABLED_HOSTS
#endif
#ifndef ENABLED_HOSTS
# define ENABLED_HOSTS
#endif

#ifdef PSYC_ENABLED_HOSTS
# define PSYC_TRUSTWORTHY_HOSTS PSYC_ENABLED_HOSTS
# define PSYC_PRIVATE
#endif

#define DATA_FILE	DATA_PATH "hosts"

#define I_HOST		0
#define I_REASON	1
#define I_NEXT		2

array(string) disabled_hosts = ({ DISABLED_HOSTS });
mixed *disabled_list;
volatile array(string) enabled_hosts = ({ ENABLED_HOSTS });
volatile int loaded = 0;

#ifdef PSYC_TRUSTWORTHY_HOSTS
volatile array(string) psyc_trustworthy_hosts = ({ PSYC_TRUSTWORTHY_HOSTS });
#endif

create() { load(); }

load() {
	// P1(("disabled_hosts is %O\n", disabled_hosts))
	unless (loaded) {
	    loaded++;
	    restore_object(DATA_FILE);
	    unless (disabled_list) {
		mixed *cur = disabled_list = allocate(3);

		foreach (string host : disabled_hosts) {
		    cur = cur[I_NEXT] = allocate(3);
		    cur[I_HOST] = host;
		    // easier to read. no info where there is no info.
		    cur[I_REASON] = ""; // "Old block, no reason given.";
		}
		disabled_hosts = 0;
	    }
	}
	return ME;
}

legal_host(host, port, scheme, udp) {
	int i, k, l;
	mixed *cur = disabled_list;

	if (udp) udp = 3; // UDP is spoofable, see net/psyc/udp.c
	l = strlen(host);
	P3(("legal_host(%O,%O,%O,%O) called\n", host, port, scheme, udp))
#ifdef PSYC_TRUSTWORTHY_HOSTS
	if (scheme == "psyc") {
	    for (i=sizeof(psyc_trustworthy_hosts)-1; i>=0; i--) {
		k = strlen(psyc_trustworthy_hosts[i]) - 1;
		P3(("trustworthy_host(%O): %O,%O,%O,%O,%O\n",
			host, i, k, l, psyc_trustworthy_hosts[i], host[0..k]))
		if (k < l && host[0..k] == psyc_trustworthy_hosts[i])
		    return 9 - udp;
	    }
# ifdef PSYC_PRIVATE
	    return 0;
# endif
	}
#endif
	while (cur = cur[I_NEXT]) {
		k = strlen(cur[I_HOST]) - 1;
		P3(("disabled_host(%O): %O,%O,%O,%O,%O\n",
			host, cur, k, l, cur[I_HOST], host[0..k]))
#if 1
		// #define DIABLED_HOSTS "" blocks everyone.. imho this is not
		// expected behaviour
		if (k != -1 && k < l && host[0..k] == cur[I_HOST]) return 0;
#else // ungetestet.. wäre aber genauer
		if (k < l && host[0..k] == cur[I_HOST]) {
		    if (host == cur[I_HOST]) return 0;
		    if (host[k] == '.') return 0;
		}
#endif
	}
	if (enabled_hosts && sizeof(enabled_hosts)) {
		for (i=sizeof(enabled_hosts)-1; i>=0; i--) {
			k = strlen(enabled_hosts[i]) - 1;
			P4(("enabled_host(%O): %O,%O,%O,%O,%O\n",
				host, i, k, l, enabled_hosts[i], host[0..k]))
			if (k != -1 && k < l && host[0..k] == enabled_hosts[i])
				 return 7 - udp;
		}
//		why should activated ENABLED_HOSTS exclude everyone else??
//
//		return 0;
	}
	return 5 - udp;
}

list() {
	mapping show = ([ ]);
	mixed *cur = disabled_list;

	if (enabled_hosts && sizeof(enabled_hosts)) {
	    previous_object()->pr("_list_hosts_enabled_amount", // _tab
		"Access permitted from %d IP nets\n", sizeof(enabled_hosts));
	    previous_object()->pr("_list_hosts_enabled",    // _tab
		"Enabled IP nets: %O\n", enabled_hosts);
	}
/*
	int i;
	string l;

	for (i=sizeof(disabled_hosts)-1; i>=0; i--) {
		if (l) l += ", \""+ disabled_hosts[i] +"\"";	
		else l = "\""+ disabled_hosts[i] +"\"";
	}
	unless (l) l = "none.";
*/
	while (cur = cur[I_NEXT])
	    show[cur[I_HOST]] = cur[I_REASON];
	previous_object()->pr("_list_hosts_disabled",   // _tab
		"Blocked IP nets: %O\n", show);
	// "Blocked IP nets: %-*#s\n", implode(disabled_hosts, "\n")
#ifdef PSYC_TRUSTWORTHY_HOSTS
	previous_object()->pr("_list_hosts_trustworthy_psyc",   // _tab
		"PSYC-trustworthy nets: %O\n", psyc_trustworthy_hosts);
#endif
}

modify(match, reason) {
	int i;
	mixed *pre, *cur = disabled_list;

	//for (i=sizeof(disabled_hosts)-1; i>=0; i--) {
	while ((pre = cur) && cur = cur[I_NEXT]) {
		if (cur[I_HOST] == match) {
		    pre[I_NEXT] = cur[I_NEXT];

#ifdef SAVE_FORMAT
			save_object(DATA_FILE, SAVE_FORMAT);
#else
			save_object(DATA_FILE);
#endif
			log_file("BLOCK", "[%s] %O unblocked by %O\n",
				ctime(), match, previous_object());
			monitor_report("_notice_block_off",
			    match+" unblocked by "+
			    object_name(previous_object()));
			return -1;
		}
	}
	unless (reason) return -2;
	disabled_list[I_NEXT] = ({ match, reason, disabled_list[I_NEXT] });
#ifdef SAVE_FORMAT
	save_object(DATA_FILE, SAVE_FORMAT);
#else
	save_object(DATA_FILE);
#endif
	log_file("BLOCK", "[%s] %O blocked by %O\n",
		ctime(), match, previous_object());
	monitor_report("_notice_block_on",
	    match+" blocked by "+object_name(previous_object()));
	return 1;
}
