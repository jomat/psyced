// $Id: legal.c,v 1.16 2008/02/10 14:27:17 lynx Exp $ // vim:syntax=lpc
#include <net.h>

// legal nickname/placename test..
string legal_name(string n) {
	int i;

	//PT(("legal_name(%O) in %O\n", n, ME))
	if (shutdown_in_progress) {
		P1(("not legal_name for %O: shutdown in progress\n", n))
		return 0;
	}
	if (!n || n == "") {
		P1(("not legal_name: %O is empty\n", n))
		return 0; // "somebody";
	}
#ifdef MAX_NAME_LEN
	if (strlen(n) > MAX_NAME_LEN) {
		P1(("not legal_name: %O exceeds MAX_NAME_LEN\n", n))
		return 0;
	}
#endif
	// this saves applet/user from malfunction!
	if (index("!=-", n[0]) != -1) {
		P1(("not legal_name: %O has !=- as first char.\n", n))
		return 0;
	}
	for (i=strlen(n)-1; i>=0; i--) {
		if (index("\
abcdefghijklmnopqrstuvwxyz\
ABCDEFGHIJKLMNOPQRSTUVWXYZ\
0123456789_-=+", n[i]) == -1) {
			P1(("not legal_name: %O has ill char at %d (%O).\n",
			    n, i, n[i]))
			return 0;
		}
	}
	return n;
}

array(string) legal_password(string pw, string nick) {
	int i;

	if (shutdown_in_progress) 
		return ({ "_failure_server_shutdown",
		    "Sorry, server is being shut down.\n" });

	if (!stringp(pw) || strlen(pw) < 3)
		return ({ "_error_illegal_password_size",
		    "That password is too short.\n" });

	if (index(pw, '<') != -1 || index(pw, '>') != -1)
		return ({ "_error_illegal_password_characters",
		    "Password contains illegal characters.\n" });

	if (nick && strstr(lower_case(pw), lower_case(nick), 0) != -1)
		return ({ "_error_illegal_password_username",
		    "That password contains your nickname.\n" });

	return 0;
}

/* check for legal PSYC keyword (var or mc names), see specs */
#ifndef legal_keyword		    // inline variant is better of course
int legal_keyword(string n) {
# if __EFUN_DEFINED__(regreplace)
#  include <regexp.h>
	string t = regmatch(n, "[^_0-9A-Za-z]");
	P1(("legal_keyword(%O) says %O\n", n, t))
	return !t;
# else
	int i;

	for (i=strlen(n)-1; i>=0; i--) {
		if (index("_\
abcdefghijklmnopqrstuvwxyz\
ABCDEFGHIJKLMNOPQRSTUVWXYZ\
0123456789", n[i]) == -1) {
			P1(("not legal_keyword: %O has ill char at %d (%O).\n",
			    n, i, n[i]))
			return 0;
		}
	}
	return 1;
# endif
}
#endif

