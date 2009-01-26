// $Id: common.c,v 1.2 2006/09/29 08:51:40 lynx Exp $ // vim:syntax=lpc
//
// WAP library
//
#include <sys/time.h>
#include <net.h>
#include <text.h>
#include <person.h>
#include "wap.h"

volatile object user;

volatile string username;
volatile string password;

htget(prot, query, headers, qs) {
	username = query["u"];
	password = query["p"];
	
	if (username && password) {
		checkPassword(username, password);
	} else {	
		queryLogin();
	}
}

printMsg(msg) {
	HTOK;
	HEADER_WML("info");
	NAV_WML;
	
	write(msg);
	
	FOOTER_WML;
}

authChecked(val) {
	unless (val) {
		HTOK;
		HEADER_WML("login");
		write("Login failed.<br />");
		LOGIN_WML;
		FOOTER_WML;
		return 0;
	}

	return 1;
}

queryLogin() {
	HTOK;
	HEADER_WML("login");
	LOGIN_WML;
	FOOTER_WML;
	return;
}

checkPassword(username, pass) {
	user = summon_person(username);

	if(!user || user->isNewbie()) {
		queryLogin();
		return;
	}

	user->checkPassword(pass, "plain", "", "",  #'authChecked);
}
