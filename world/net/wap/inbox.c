// $Id: inbox.c,v 1.4 2007/07/12 06:08:29 lynx Exp $ // vim:syntax=lpc
//
// WAP account management
//
#include <sys/time.h>
#include <net.h>
#include <text.h>
#include <person.h>
#include "wap.h"

inherit NET_PATH "lastlog";
inherit WAP_PATH "common";

volatile string inbox_buffer;

showInbox() {
	int count;
	array(mixed) log = user->logQuery();

	HTOK;
	HEADER_WML("inbox");
	NAV_WML;
	
	if (user->vQuery("new")) {
		logInit(copy(log));
		inbox_buffer = "";
		
		count = logView(user->vQuery("new"), 1);
		
		write(inbox_buffer);
	}
	
	unless (count) {
		write("No new messages.");
	}

	FOOTER_WML;
}

msgView(source, mc, data, vars, showingLog) {
	array(int) t = localtime(vars["_time_INTERNAL"]);
	
	// we want the newest message on top.. buffering output
	inbox_buffer = sprintf(" - %d:%d, %d.%d.%d<br/>%s<br/><br/>",
		t[TM_HOUR], t[TM_MIN], t[TM_MDAY], t[TM_MON], t[TM_YEAR], data) + inbox_buffer;
	// make person's nick clickable
	inbox_buffer = SEND_PERSON(vars["_nick_verbatim"] || vars["_nick"]) + inbox_buffer;
}

authChecked(val) {
	if (::authChecked(val)) {
		showInbox();
	}
}
