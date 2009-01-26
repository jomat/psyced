// $Id: server.c,v 1.56 2007/06/15 14:45:09 lynx Exp $ // vim:syntax=lpc
/*
 * rudimentary prototype of an SMTP server
 * all your protocols belong to us
 */
#include <net.h>
#include <server.h>
#include <person.h>

// does mailto:*place@host work for addressing places?
// unfortunately # is not allowed in mailto: urls according to the NEW URI RFC
#define GROUP_PREFIX		'*'

#ifndef MAX_EMAIL_LINES
# ifdef _flag_optimize_protection_SMTP
#  define MAX_EMAIL_LINES	5
# else
#  define MAX_EMAIL_LINES	50
# endif
#endif
#ifndef MAX_EMAIL_SIZE
# ifdef _flag_optimize_protection_SMTP
#  define MAX_EMAIL_SIZE	50 * 7
# else
#  define MAX_EMAIL_SIZE	50 * 70
# endif
#endif

#ifndef _time_logon_delay_SMTP
# define _time_logon_delay_SMTP	20
#endif

// handle st00pid optional brackets
#define ADDRPARSE(XXX, USER, HOST) (\
	sscanf(XXX, "<%s@%s>", USER, HOST) || \
	sscanf(XXX, "%s@%s", USER, HOST) || \
	sscanf(XXX, "<%s>", USER) || \
	(USER = XXX) )

qScheme() { return "smtp"; }

volatile mapping vars;
volatile object target;
volatile string origin, text, header;
volatile int bodycount;		// yeaaahhh!! bodycount's in the house!!

volatile mapping head2vars = ([
	"subject":	"_subject",
	"content-type":	"_type_content",
	"message-id":	"_ID_SMTP",
]);

#ifdef SMTP_ALIASES
volatile mapping smtp_aliases = ([
	SMTP_ALIASES
]);
#endif

// the most important thing in an smtp daemon is.. the spam filter
//
// our current strategy is:
// 	reject mails with attachments.
//	reject non-plain mails. 
//	reject mails that exceed MAX_EMAIL_LINES lines.
//
// leaves you with only the shortest V1AGRA candidates.
//
spam(mc, extra, message) {
	P1(("%O spam(%O, %O..\n", ME, mc, extra))
	write(message);
#if 0
	monitor_report("_warning_abuse"+ mc,
	   S("%O rejecting %s mail from %O for %O.",
	    query_ip_number(), a, origin, vars["_nick_target"]));
#else
	// seems to work generally, so lets reduce the
	// reporting to a logfile..
	log_file("SMTP_SPAM", "%s\t%s\t%s\t%s\t%O\n",
		 query_ip_number() || "(null)",
		 vars["_nick_target"] || "(null)",
		 mc, origin || "(null)", extra);
#endif
	QUIT
}

parse(a) {
	/*
	 * this is the parser for the command mode
	 */
	string cmd, fullargs;
	string *args;
	
	unless (sscanf(a, "%s%t%s", cmd, fullargs)) cmd = a;
	if (fullargs) args = explode(fullargs, " ");
	if (smtpcmd(cmd, args, fullargs, a))
	    next_input_to(#'parse);
}

#if !__EFUN_DEFINED__(trim)
# define trim(x) x
#endif

body(a) {
	if (a != ".") {
		if (++bodycount > MAX_EMAIL_LINES)
		    return spam("_illegal_length", vars["_subject"],
				"254 Insufficient storage for long messages\n");
		if (a != "") {
			text += a + "\n";
			if (strlen(text) > MAX_EMAIL_SIZE)
			    return spam("_illegal_size", vars["_subject"],
				"254 Insufficient storage for long messages\n");
		}
		next_input_to(#'body);
		return;
	}
	PT(("SMTP %O content %O vars %O\n", query_ip_number(), text, vars))
	unless (objectp(target) || (strlen(vars["_nick_target"]) &&
	    (target = vars["_nick_target"][0] == GROUP_PREFIX
	     ? find_place(vars["_nick_target"]) :
	    summon_person(vars["_nick_target"])))) {
		// re-summon may happen when a user logged out while the
		// mail was coming in. unlikely. even unlikelier that it
		// gets here:
		P0(("SMTP %O cannot re-summon username %O\n", query_ip_number(), user))
		write("550 Technical problems summoning recipient\n");
	} else {
		string subject;

		text = trim(chomp(text));
		subject = trim(vars["_subject"] || "");
		if (strlen(text) < 3 && strlen(subject) < 3) {
		    return spam("_illegal_content_empty", subject+text+a,
			"254 What is this? An SMTP Presence Notification? Nice, we don't have that, yet. We have to invent it first!\n");
		}
		vars["_content"] = text;
		vars["_subject"] = decode_embedded_charset(subject);
			    // more likely to have resolved by now
			    // even better to call async DNS here.. one day
		vars["_host_source"] = query_ip_name();
		write("250 accepted for delivery\n");
		// certainly not the definitive mc, but at least person
		// doesn't reply with a _message_echo_private which
		// then goes out to spammers and godknowswho.. although
		// in some cases that's sexy.
		//
		// uncool about this one: it doesn't get logged. but
		// a _message wouldn't have the template. we need
		// the mc reform!!
		sendmsg(target, "_notice_email_delivered",
			//"([_method_SMTP] from [_host_source] to [_nick_target]@[_host_target]) [_subject]\n[_content]\n",
			//		[_nick] wird erweitert um die source??!?
			"([_method_SMTP] from [_nick] to [_nick_target]) [_subject]\n[_content]\n",
			    // "smtp:" for SOML-capable senders?
			vars, "mailto:"+ origin);
		// a log to get a clue if everything is working fine.
	        // sendmail keeps such logs too
		log_file("SMTP_OK", "%s\t%s\t%s\t%s\n",
			 vars["_host_source"] || "(null)", vars["_nick_target"] || "(null)",
			 vars["_host_target"] || "(null)", origin || "(null)");
		target = 0;
		vars = ([]);
	}
	return next_input_to(#'parse);
}

headers(a) {
	string t;

	if (a == "") {
		next_input_to(#'body);
		bodycount = 0;
		return;
	}
	next_input_to(#'headers);
	unless (sscanf(a, "%s:%.0t%s", t, a)) {
		// a continuation line..
		if (header) {
			if (vars[header]) vars[header] += a;
			else {
			    // this cannot happen really
			    P0(("SMTP %O line cont for %O w/out start: %O\n",
			       	query_ip_number(), header, a))
			}
		} else {
		    // skip! this is a continuation of a header that
		    // we have rejected according to head2vars!
		    // so we get here continously and there's nothing wrong
		    // with it!
		}
		return;
	}
	if (header = head2vars[lower_case(t)]) switch(header) {
	case "_type_content":
		unless (abbrev("text/plain", a))
		    return spam("_illegal_type_content", a,
		      "254 Insufficient storage for non-plain messages\n");
		// we don't need to store this one
		header = 0;	// no line conts either
		break;
// don't operate on headers yet as a line continuation may follow
//	case "_subject":
//		vars["_subject"] = decode_embedded_charset(a);
//		break;
	default:
		vars[header] = a;
	}
}

smtpcmd(cmd, args, all, asis) {
	object o, palo;
	string user, host, mail;
	P2(("smtpcmd %s %O\n", cmd, args))

	switch(cmd = upper_case(cmd)) {
case "RSET":
		origin = 0;
		target = 0;
case "NOOP":
		write("250 SYS 64738\n");
		break;
case "HELO":
case "EHLO":
		if (sizeof(args) == 1) {
			write("250 "+ SERVER_HOST +" Hello "
			      + query_ip_number() +"\n");
			    // let's not put useless work on erq
			    // +query_ip_name()+" ["+query_ip_number()+"].\n";
		} else write("501 Syntax: HELO hostname\n");
		break;
case "MAIL":
case "SEND":
case "SOML":
case "SAML":
		vars["_method_SMTP"] = cmd;
		if (upper_case(all[0..4]) == "FROM:") {
			origin = all[5] == ' ' ? all[6..] : all[5..];
			host = query_ip_number();
			if (ADDRPARSE(origin, user, host)) {
				vars["_nick"] = origin = user +"@"+ host;
				P3(("smtpcmd %s %O » %s\n", cmd, all, origin))
				write("250 Sender superficially ok\n");
				break;
			}
		}
		write("501 Syntax: MAIL FROM: <address>\n");
		break;
case "RCPT":
//		unless (origin) {
//			write("503 Need MAIL before RCPT\n");
//			break;
//		}
		if (upper_case(all[0..2]) == "TO:") {
		    target = all[3] == ' ' ? all[4..] : all[3..];
		    if (ADDRPARSE(target, user, host)) {
#ifdef SMTP_ALIASES
			string rcpt;

			vars["_nick_target"] = user;
			if (user = smtp_aliases[rcpt = lower_case(user)]) {
				vars["_nick_target_alias"] = user;
			} else user = rcpt;
#else
			vars["_nick_target"] = user;
			user = lower_case(user);
#endif
			if (strlen(user)) {
				// find_person and legal_name are not necessary
				// as summon_person / find_place do all of that
				if (user[0] == GROUP_PREFIX)
				    target = find_place(user[1..]);
				else
				    target = summon_person(user);
				if(target) {
					if(user[0] == GROUP_PREFIX ||
					   !target->isNewbie()) {
						vars["_host_target"] = host || "";
						write("250 Recipient apparently ok\n");
					} else {
						target = 0;
						spam("_unknown_name_user", 0,
			  "550 User "+user+" unknown (not registered).\n");
					}
					break;
				}
			}
			target = 0;
			spam("_unknown_name_user", 0,
			     "550 User or room "+user+" unknown (/not registered).\n");
			// spam does a QUIT in this case. too harsh?
			break;
		    }
		}
		write("501 Syntax: RCPT TO: <address>\n");
		break;
case "DATA":
		unless (origin && target) {
			spam("_insufficient_data", 0,
			     "503 Watcha tryinta hack?\n");
			// spam does a QUIT in this case. too harsh?
			break;
		}
		//write("354 Enter mail, end with '.' on a line by itself.\n");
		write("354 Alright now, "+ query_ip_name()
		      +", tell me the truth.\n");
		// I'm reading your lips, baby. So you better not spam me.
		// But most of all the purpose is to activate erq for name
		// resolution at this point, since we will want the hostname
		// later. Of course we might as well go thru async DNS later
		// when we are absolutely sure we are going to keep the
		// message. TODO one day.
		header = text = "";
		next_input_to(#'headers);
		return 0;
case "QUIT":
		write("221 "+ SERVER_HOST +" says: I like your pink tie\n");
		QUIT
case "HELP":
		write("221 Ask http://about.psyc.eu/SMTP for help\n"
    "221 Commands: HELO, MAIL, SEND, SOML, SAML, RCPT, RSET, DATA, QUIT\n");
		break;
default:
		// some try binary overflow attacks here.. looks a bit ugly
		// but obviously has null effect on an ldmud
		P1(("SMTP %O got %O from %O\n", ME, asis, query_ip_number()))
		//write("500 Command not recognized.\n");
		write("500 Huh?\n");	// why even talk to guys like that?
		QUIT
		break;
	}
	return 1;
}

greet() {
    remove_input_to(ME, #'quit);
    next_input_to(#'parse);
    vars = ([ ]);
    // should provide a hint for upgrading to PSYC protocol.. TODO
    write("220 ESMTP "+ SERVER_HOST +" makes me feel http://www.psyced.org\n"
#ifdef _flag_optimize_protection_SMTP
	// multiline greetings is a SPAM deterrent provision. they are not
	// explicitely permitted in the RFC, but aren't forbidden either.
	// cheap SMTP implementations like python2.4/smtplib.py choke on this.
# define MORE "220 "	// 211? just as evil..
      MORE "This is the psyced receptionist talking\n"
      MORE "You are best known as "+ query_ip_name() +"\n"
      MORE "Thank you for waiting. It's a spam deterrent provision\n"
      MORE "Somebody also said that multiline greetings embarrass spammers\n"
      MORE "If you like to spam, consider figure 1 at http://b-5.de/fig1/\n"
# undef MORE
#endif
    );
}

logon() {
	P2(("SMTP logon from %O\n", query_ip_number()))
#if DEBUG > 1
	greet(); // make it easier for developers trying this out.
#else
	call_out(#'greet, _time_logon_delay_SMTP);
       	// now wait a minute! .. almost
	// this throws out spammers who think they can talk before
	// being greeted
	next_input_to(#'quit);
#endif
}

