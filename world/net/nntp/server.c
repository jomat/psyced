// $Id: server.c,v 1.15 2006/08/24 11:43:36 lynx Exp $ // vim:syntax=lpc
//
// NTTP server. currently only works with net/place/threads.
/*
 *	This may become a working nntp server one day,
 *	even morphing into a user (AUTHINFO)
 */
#include <net.h>
#include <server.h>
#include <person.h>

qScheme() { return "nntp"; }

//protected object user;
//string nick
string password;
int authenticated;
object currentgroup;

mapping post_headers;
string post_text;

parse(a) {
	/*
	 * this should probably be moved to cmd and to a user.c
	 * but currently it is too small
	 */
	string cmd, fullargs;
	string *args;
	
	unless (sscanf(a, "%s%t%s", cmd, fullargs)) cmd = a;
	if (fullargs) args = explode(fullargs, " ");
	if (authenticated) nntp_cmd(cmd, args, a);
	else authenticate(cmd, args);
	next_input_to(#'parse);
}

post_body(a) {
	object group;
	string dummy1, dummy2;
	int replyid;
	
	if (a == ".") {
		post_headers["from"] = user -> qName();
		group = find_object("place/" + post_headers["newsgroups"]);
		P2(("group: %O\n", group))
		if (post_headers["in-reply-to"]) {
			sscanf(post_headers["in-reply-to"], "<%s$%d@%s>",
			       dummy1, replyid, dummy2);
			P2(("its a reply to %d!\n", replyid - 1))
			group -> addComment(post_text, post_headers["from"],
					    replyid - 1);
		} else {
			group -> addEntry(post_text, post_headers["from"], 
				  post_headers["subject"]);
		}
		write("240 Article posted successfully.\n");
		// P2(("headers: %O\n", post_headers))
		return next_input_to(#'parse);
	}
	unless (post_text) post_text = "";
	post_text += a + "\n";
	next_input_to(#'post_body);
}

post(a) {
	string key, value;
	if (a == "") {
		return next_input_to(#'post_body);
	}
	sscanf(a, "%s:%t%s", key, value);
	unless (post_headers) post_headers = ([ ]);
	post_headers[lower_case(key)] = value;
	P2(("%s => %s\n", key, value))
	next_input_to(#'post);
}

authenticate(cmd, args) {
	mixed authCb;
	P2(("authenticate %s, %O\n", cmd, args))
	switch(upper_case(cmd)) {
case "AUTHINFO":
		unless (args) return -1;
		if (args[0] == "user") {
			nick = args[1];
			unless (user = find_person(nick)) {
				write("502 No permission\n");
				return quit();
			}
		} else if (args[0] == "pass") {
			password = args[1];
		}
#ifdef ASYNC_AUTH
		authCb = CLOSURE((int result), (authenticated, password), 
				 (int authenticated, string password), 
		    {
			if(result) {
			    write("281 Authorization accepted.\n");
			    authenticated = 1;
			    password = "";
			} else {
			    write("381 PASS required.\n");
			}
			return 1;
		    });
		user -> checkPassword(password, "plain", 0, 0, authCb);
#else
	        write("500 ASYNC_AUTH required.\n");
		return quit();
#endif
		break;
case "MODE":
		// mode reader
		write("200 Ok\n");
		break;
case "QUIT":
		return quit();
default:
		write("480 User and password still required, authinfo command\n");
		break;
	}
}

nntp_cmd(cmd, args, all) {
	object o;
	string group; 
	P2(("nntp_cmd %s %O\n", cmd, args))
	unless (authenticated) return -1;
	switch(upper_case(cmd)) {
case "GROUP":
		unless (args[0]) return;
		group = "place/" + args[0];
		o = group -> load();
		// watch out, saga suspects this might not work in some
		// cases
		if (objectp(o) || o = find_object(group) ) {
			// place exists and provides nntp support
			currentgroup = o;
			currentgroup -> nntpget("GROUP");
			// some kind of vSet("place")...
		} else {
			// place does not exist or does not provide nntp
			write("411 No such news capable place\n");
		}
		break;
		// mysterious article pointer stuff somewhere here
case "XOVER":
		unless (currentgroup) {
			// write error!
			break;
		}
		write("224 data follows\n");
		// data here
		currentgroup -> nntpget("XOVER");
		write(".\n");	
		break;
#if 0
case "NEWNEWS":
		group = "place/" + args[0];
		o = group -> load()
		if (objectp(o) || o = find_object(group)) {
			write("230 New news by message id follows\n");
			o -> nntpget
			write(".\n");
		} else write("411 No such news capable place\n");
		break;
#endif
case "HEAD":
		write("221 1 <foo@host> Article retrieved; head follows.\n");
		// header here
		write(".\n");
		break;
case "BODY":
		write("222 1 <foo@host> Article retrieved; body follows.\n");
		// body here
		write(".\n");
		break;
case "ARTICLE":
		unless(currentgroup) {
			// error!
			break;
		}
		currentgroup -> nntpget("ARTICLE", args[0]);
		break;
case "HELP":
		write("100 This server provides no commands\n");
		break;
case "MODE":
		write("200 Ok\n");
		break;
case "NEWGROUPS":	// besser als garnix?
case "LIST":
		write("215 Newsgroups in form 'group high low flags'.\n");
		// kind of: list of places that are public and provide
		// nntp export
		// this is an example that does not even work proper
		// maybe this should be implemented in user object
		// and show the subscribed places
		foreach(group : user -> vQuery("subscriptions")) {
			o = find_object(PLACE_PATH + group);
			if (objectp(o)) o -> nntpget("LIST");
		}
		write(".\n");
		break;
case "POST":
		write("340 Ok\n");
		post_headers = ([ ]);
		post_text = "";
		next_input_to(#'post);
		break;
case "QUIT":
		return quit();
default:
		if (all && strlen(all)) write("500 command not recognized\n");
		break;
	}
}

logon() {
	// write("201 Hello PSYC-NNTP world, posting is not implemented yet\r\n");
	write("480 User and password still required, authinfo command\n");
	next_input_to(#'parse);
}

quit() {
	write("205 You just lost that magic feeling.\n");
	QUIT
}

