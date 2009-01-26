// $Id: send.c,v 1.8 2007/04/20 11:49:35 lynx Exp $ // vim:syntax=lpc
//
// WAP msg delivery agent.
//
#include <net.h>
#include <person.h>
#include "wap.h"

inherit WAP_PATH "common";

volatile string recipient = "";
volatile string message = "";

htget(prot, query, headers, qs) {
	// we don't want strings containing 0
	recipient = query["r"] || "";
	message = query["m"] || "";

	::htget(prot, query, headers, qs);
}

authChecked(val)
{
	if(::authChecked(val)) {
		// send message or display send-form?
		if (recipient != "" && message != "") {
			checkMessage(recipient, message);
		} else {
			showCompose(recipient, message);
		}
	}
}

checkMessage(recipient, message) {
	mixed palo;
	
	unless (stringp(recipient) && stringp(message)) {
		showCompose(recipient, message);
	}
	
	unless (palo = is_formal(recipient)) {
		if (recipient = legal_name(recipient)) {
			unless (palo = find_person(recipient)) {
				if (palo = summon_person(recipient, load_name())) {
					if (palo->isNewbie()) {
						destruct(palo);
						showCompose(recipient, message);
						return;
					}
				}
			}
		}
	}
	
	sendmsg(palo, "_message_private", message, ([
		"_nick" : username,
		"_nick_source" : query_ip_name()
		]), user);
	
	printMsg("Message delivered.<br/>");
}

// shows msg-form, default values possible
showCompose(recipient, message, notice) {
	HTOK;
	HEADER_WML("send msg");
	
	write("Recipient:<br/><input type='text' name='r' emptyok='false' value='"+recipient+"'/><br/>");
	write("Text:<br/><input type='text' name='m' emptyok='false' value='"+message+"'/><br/>");
	write("<a href='send?u="+username+"&amp;p="+password+"&amp;r=$(r)&amp;m=$(m)'>[send]</a>");

	FOOTER_WML;
}

