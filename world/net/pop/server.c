// $Id: server.c,v 1.14 2008/01/05 13:44:39 lynx Exp $ // vim:syntax=lpc
//
// pop3 implementation for using psyc with mails. experimental.
//
#include <net.h>
#include <text.h>
#include <person.h>

#define NO_INHERIT
#include <server.h>

inherit NET_PATH "lastlog";
//inherit NET_PATH "name";
//inherit NET_PATH "common";

volatile string nick, password;
volatile object user;
volatile array(mixed) log;
volatile array(string) rendered;
volatile mapping message;

qScheme() { return "pop3"; }

parse(a) {
	string cmd, fullargs;
	string *args;
	
	unless (sscanf(a, "%s%t%s", cmd, fullargs)) cmd = a;
	if (fullargs) args = explode(fullargs, " ");
	if (log) pop3cmd(cmd, args); else authenticate(cmd, args);
	input_to(#'parse, INPUT_IGNORE_BANG);
}

pop3cmd(cmd, args) {
#	define MSG_QUIT "Hope to see you again."
#	define MSG_UNKNOWN "Command not recognized."
#	define MSG_WRONGMSGID "No such message."
	
	P2(("pop3cmd %s %O\n", cmd, args));
	int i;

	switch(cmd = upper_case(cmd)) {
		case "HELP":
			write("+OK This is POP3. What else do you need to know?"
#if HAS_PORT(POP3_PORT, POP3_PATH)
			    " Plain on "+POP3_PORT+"."
#endif
#if HAS_PORT(POP3S_PORT, POP3_PATH)
			    " Encrypted by SSL on "+POP3S_PORT+"."
#endif
			    "\n");
			break;
		case "LIST":
			write("+OK\n");
			// if list is supposed to show all messages, why
			// are you looking at "new" ?
			if(user->vQuery("new")) {
				for(i=0;i<user->vQuery("new");i++) {
				    // 1234?? TODO is presume
					write(i+"_"+RANDHEXSTRING+RANDHEXSTRING+"@"+SERVER_HOST+" 1234\n");		// <MsgID> <Byte>
				}
			}
			write(".\n");				// .
			break;
		case "STAT":
			if(user->vQuery("new")) {
				for(i=0;i<user->vQuery("new");i++) {
					// render each msg so we can output its real length..
					write("+OK "+i+"_"+RANDHEXSTRING+RANDHEXSTRING+"@"+SERVER_HOST+" 1234\n");	// +OK <MsgID> <Byte>
				}
			} else {
				write("+OK 0 0\n");
			}
			break;
		case "RETR":
			if (sizeof(args) == 1) {
				i = to_int(args[0])-1; // log starts at 0
				// check rendered first?
				array(mixed) p = logPick(i);
				if (p) {
					write("+OK Here we go..\n");
					//printf("Temporary: %O", p);
					write(psyctext( T(p[1], ""), p[3],
						       	p[2], p[0] ));

					write("\n.\n");
					// löschen. im gegensatz zu email-servern werden nachrichten nicht aufgehoben.
					//delete(msgid);
				} else {
					write("-ERR "+MSG_WRONGMSGID+"\n");
				}
			} else {
				write("-ERR Which message do you want?\n");
			}
			break;
		case "DELE":	// braucht man das? geht das überhaupt?
			if (sizeof(args) == 1) {
				if(1) { // wenn msg mit id existiert
					write("+OK Message tagged for deletion.\n");
					//delete(msgid);
				} else {
					write("-ERR "+MSG_WRONGMSGID+"\n");
				}
			} else {
				write("-ERR Which message do you want to delete?\n");
			}
			break;
		case "RSET":	// alle mails, die zum löschen markiert waren, wieder als nicht gelöscht taggen
				// (gelöscht wird erst nach trennung nach 'DELE').
			write("+OK Deletion aborted.\n");
			break;
		case "CAPA":
			write("+OK\n");
			write("HELP\nLIST\nSTAT\nRETR\nDELE\nRSET\nCAPA\nNOOP\nQUIT\n");
			write(".\n");
			break;
		case "QUIT":
			write("+OK "+MSG_QUIT+"\n");
			QUIT;
		case "NOOP":
			write("+OK Don't do this often, you should know what you want.\n");
			break;
		default:
			write("-ERR "+MSG_UNKNOWN+"\n");
	}
}

authenticate(cmd, args) {
	object palo;
	P2(("pop3auth %s %O\n", cmd, args));

	switch(cmd = upper_case(cmd)) {
		case "USER":
			if (sizeof(args) == 1) {
				//if(!nick) {	// sucks in some clients..
					//palo = find_person(args[0]);
					palo = summon_person(args[0]);
					if (palo && !palo->isNewbie()) {
						nick = args[0];
						write("+OK Username ok.\n");
					} else {
						write("-ERR User "+args[0]+" is not registered.\n");
					}
				//} else {
				//	write("-ERR You are already identified as "+nick+".\n");
				//}
			} else {
				write("-ERR I need your username.\n");
			}
			break;
		case "PASS":
			if (sizeof(args) == 1) {
				if(!password) {
#ifdef ASYNC_AUTH
					mixed authCb;
					user = find_person(nick);
					authCb = CLOSURE((int result), (), (),
					    {
						unless(result) {
						    write("-ERR Login failed. Wrong password?\n");
						} else if (log = user -> logQuery()) {
						    if (log) logInit(copy(log));
						    write("+OK Successfully logged in.\n");
						    sTextPath(0, user->vQuery("language"), "pop3");
						} else {
						    write("-ERR Login failed. No messages available.\n");
						}
						return 1;
					    });
					user -> checkPassword(args[0], 0, 0, 0, authCb);
#else
					write("-ERR ASYNC AUTH necessary.\n");
#endif
				} else {
					write("-ERR You are already logged in.\n");
				}
			} else {
				write("-ERR There are NO empty passwords.\n");
			}
			break;
		case "CAPA":
			write("+OK\n");
			if (!nick) write("USER\n");
			else if (!password) write("PASS\n");
			write("CAPA\nQUIT\n.\n");
			break;
		case "QUIT":
			write("+OK "+MSG_QUIT+"\n");
			QUIT;
		case "TEST":

			break;
		default:
			write("-ERR "+MSG_UNKNOWN+"\n");
	}
}

logon() {
	write("+OK Welcome to the POP3 implementation in PSYC on " SERVER_HOST ".\n");
	next_input_to(#'parse);
}

