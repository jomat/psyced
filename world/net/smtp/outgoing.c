// outgoing mail queue for psyced.

// i *could* rewrite this to use net/spool.c
// but then i'd have such a mess in my room

#include <net.h>
#include <text.h>
virtual inherit NET_PATH "output";

#define MAX_SPOOL	23
#define	SPOOL_FILE	DATA_PATH "mail_queue"

#if defined(DEBUG) && DEBUG > 1
# define QUEUE_POLL_TIME	9
# define COLLECT_TIME		15
# define CONNECT_RETRY		99
#else
# define QUEUE_POLL_TIME	33
# define COLLECT_TIME		60 * 2
# define CONNECT_RETRY		60 * 7
#endif

#ifndef DEFAULT_MAIL_SENDER
# define DEFAULT_MAIL_SENDER	"psyc-daemon@" SERVER_HOST
#endif
#ifndef DEFAULT_MAIL_SENDER_NAME
# define DEFAULT_MAIL_SENDER_NAME "PSYC server on " SERVER_HOST
#endif
#ifndef DEFAULT_MAIL_SUBJECT
# define DEFAULT_MAIL_SUBJECT	"Messages for [_target]"
#endif

#ifndef SMTP_RELAY
# define SMTP_RELAY "127.0.0.1"
#endif

#define S_OFF	0
#define S_CONN	101
#define S_READY	102
#define S_SOML	103
#define S_FROM	104
#define S_TO   105
#define S_DATA 106
#define S_MESS 107
#define S_DONE 108

#define FILE_FORMAT	0
#define Q_SOURCE	0
#define Q_METHOD	1
#define Q_DATA		2
#define Q_VARS		3
#define Q_TARGET	4
#define Q_URL_LOGIN	5
//#define Q_SENDER	99

mapping spool;
int time_of_connect_attempt;
int file_format;

#ifdef Q_SENDER
static string sndr;
#else
# define sndr DEFAULT_MAIL_SENDER
#endif
static string rcpt, target;
static int state;

#ifdef OFFLINE
#  define	mwrite(TEXT)	D(TEXT)
#else
# if defined(DEBUG) && DEBUG > 1
mwrite(str) { log_file("SENDMAIL", str); emit(str); }
# else
#  define	mwrite(TEXT)	emit(TEXT)
# endif
#endif

reset(a) {
//	if (a) {
#ifndef OFFLINE
		P2(("RESET: saving mail queue into "+ SPOOL_FILE +"\n"))
		save_object(SPOOL_FILE);
#endif
//		return;
//	}
}

create() {
#ifdef DEFAULT_LAYOUT
	sTextPath (DEFAULT_LAYOUT, DEFLANG, "smtp");
#else
	sTextPath (0, 0, "smtp");
#endif
	unless (spool) {
		restore_object(SPOOL_FILE);
		if (file_format != FILE_FORMAT) spool = 0;
	}
	D2( if (spool) log_file("SENDMAIL", "*** Restored spool %O\n",
		     spool && m_indices(spool) ); )
	file_format = FILE_FORMAT;
	if (spool) qjack(1);
}

load() { return ME; }

static connect() {
#ifndef OFFLINE
	if (interactive()) return 1;
	// could this be using net/connect?
	if (time() > time_of_connect_attempt + CONNECT_RETRY) {
		int res;

		time_of_connect_attempt = time();
		save_object(SPOOL_FILE);
		D1( D("SMTP: attempting connect to "+SMTP_RELAY+"\n"); )
		if (res = net_connect(SMTP_RELAY, 25)) {
			D0(D("SMTP connect error #"+ res
			     +" to " SMTP_RELAY ".\n");)
			log_file("SENDMAIL", "*** No connect ("+res+") at "+
				 ctime(time())+"\n");
			return 0;
		}
		state=S_CONN;
		return 1;
	}
	return 0;
#endif
}

// called from smtp_sendmsg()
// this doesn't support the user's aliases, but doing so pretty
// much means a major architectural change.. or a dirty ->aliasresolve()
enqueue(target, method, data, vars, source, rcpt, urllogin) {
	int i = 0;

	// gets checked in /set and mailto:
#ifdef BRAIN
	// but we check again since brain has old user files
	// with unchecked adresses
	unless (rcpt = legal_mailto(rcpt)) return 0;
#endif
	PT(("smtp/enqueue: from %O to %O (%O)\n", source, target, rcpt))
	unless (spool) {
		spool = ([]);
		call_out("qjack", COLLECT_TIME, 1);
	}
	if (spool[rcpt]) {
		i = spool[rcpt][1]++;
		if (i >= MAX_SPOOL) return 0;
		spool[rcpt][i] = ({
		    source, method, data, vars, target, urllogin });
	} else {
		spool[rcpt] = allocate(MAX_SPOOL);
		spool[rcpt][1] = 3;
		if (objectp(target)) target = psyc_name(target);
		spool[rcpt][2] = ({
		    source, method, data, vars, target, urllogin });
	}
	spool[rcpt][0] = COLLECT_TIME + time();
	return connect();
}

qjack(arg) {
	unless (spool) return;
	if (arg) call_out("qjack", QUEUE_POLL_TIME, arg+1);

#ifndef OFFLINE
	unless (interactive()) return connect();
	D2( D("jacking Q: "+ state + ", idle "+ query_idle(ME) + "\n"); )
	unless (state == S_READY) return 0;
#endif
	foreach(rcpt : spool) {
		if (spool[rcpt][0] < time()) {
#ifdef OFFLINE
			rendermsg(rcpt);
			done();
#else
			target = spool[rcpt][2][Q_TARGET];
//			if (objectp(target)) target = psyc_name(target);
# ifdef Q_SENDER
			// do we want them to downgrade communication to email?
			sndr = spool[rcpt][2][Q_SENDER];
			unless (sndr) sndr = DEFAULT_MAIL_SENDER;
# endif
			mwrite("SOML FROM: <"+sndr+">\n");
			state = S_SOML;
#endif
			return 1;
		}
	}
	D2( D("nothing ready yet\n"); )
}

static done() {
	unless (spool) return;
	m_delete(spool, rcpt);
	if (sizeof(spool) == 0) spool = 0;
#ifndef OFFLINE
	D2( save_object(SPOOL_FILE); )
#endif
}

static abort(code, msg) {
	done();	// we cannot risk to have this person block outgoing mail
#ifdef PRO_PATH
	sendmsg(query_monitor_unl(), "_error_transaction_mail_monitor",
		"Received [_code_smtp] '[_text_smtp]' while delivering to [_recipient_mailto] ([_recipient]).", ([
	      "_code_smtp": code, "_text_smtp": msg,
	      "_recipient_mailto" : "mailto:"+rcpt,
	      "_recipient" : target ]) );
#endif
	mwrite("QUIT\n"); // restart mailer connection (not necessary?)
	state=S_DONE;
}

parse(str) {
	int code;
	string msg;

	input_to(#'parse);
	if (str) {
		D2( log_file("SENDMAIL", "« "+ str+"\n"); )
		if (sscanf(str, "%d%t%s", code, msg) !=2)
			    sscanf(str, "%d",code);
	}
	if (!code) code=250;	// is this smart?

	switch(state) {
	case S_CONN:
		state=S_READY;
		mwrite("HELO " SERVER_HOST "\n");
		break;
	case S_READY:
		if (code!=250) abort(code, msg);
		else qjack(0);
		break;
	case S_SOML:
		if (code!=250) {
		    P3(("SOML to %O returns %s\n", query_ip_number(), str))
		    state = S_FROM;
		    mwrite("MAIL FROM: <"+sndr+">\n");
		    break;
		}
		// else we were successful, so let's fall thru
		P0(("Yoohoo! %O is a mailer who understands SOML!\n",
			query_ip_number()))
	case S_FROM:
		state=S_TO;
		if (code!=250) abort(code, msg);
		else mwrite("RCPT TO: <"+rcpt+">\n");
		break;
	case S_TO:
		state=S_DATA;
		if (code!=250) abort(code, msg);
		else mwrite("DATA\n");
		break;
	case S_DATA:
		state=S_MESS;
		if (code!=354) abort(code, msg);
		else rendermsg(rcpt);
		break;
	case S_MESS:
		state=S_READY;
		if (code!=250) abort(code, msg);
		done();
		break;
	case S_DONE:
		// 221 2.0.0 fly.symlynX.com closing connection
		state=S_OFF;
		break;
	case S_OFF:
		remove_interactive(ME);
		D2(D("SMTP had to force connection close: "+str+"\n");)
		break;
	default:
		D2(D("SMTP should not get here: "+state+"\n");)
	}
}

w(string mc, string data, mapping vars, mixed source) {
	string output, nick = 0;
	string template = T(mc, "");

	// source can be 0
	if (stringp(source) && mappingp(vars)) nick = psyctext(
	     T("_MISC_identification_remote",
	       "<[_source]> [_nick]"), ([
		"_source":source, 
		"_nick": (vars["_nick_verbatim"] || vars["_nick"])
	]));
	output = psyctext( template, vars, data, source, nick );
        P3(("SMTP:w(%O,%O,%O,%O) - %O\n", mc,data,vars,source, template))
        if (template == "") output += abbrev("_prefix", mc) ? " " : "\n";
        return output;
}

// TODO: to be formally correct all umlauts in the header would have to
//       be escaped with that ugly embedded encoding trick. do we care?
rendermsg(rcpt) {
	string m, t2;
	int i, l, o, t;

	mwrite("From: " DEFAULT_MAIL_SENDER " ("
			DEFAULT_MAIL_SENDER_NAME ")\n");
	l = spool[rcpt][1];
	if (l > MAX_SPOOL) {
		o = l - MAX_SPOOL;
		mwrite("X-Amount-Messages-Omitted: "+ o +"\n");
		l = MAX_SPOOL;
	}
	for (i=2; i<l; i++) {
		unless (m) {
			// m = spool[rcpt][i][Q_DATA];
			m = w(  spool[rcpt][i][Q_METHOD],
				spool[rcpt][i][Q_DATA],
				spool[rcpt][i][Q_VARS],
				spool[rcpt][i][Q_SOURCE] );
			mwrite("To: "+rcpt+" ("+ target +")\n");
			       // psyc_name(spool[rcpt][i][Q_TARGET])
			t = index(m, '\n');
//			PP(("smtp-out: %d == %d ? %s", t, strlen(m), m));
#ifdef SEND_FIRST_MSG_AS_SUBJECT
			if (t+1 == strlen(m)) mwrite("Subject: "+m);
			else mwrite("Subject: "+m[..t]);
#endif
		} else 
			m += w( spool[rcpt][i][Q_METHOD],
				spool[rcpt][i][Q_DATA],
				spool[rcpt][i][Q_VARS],
				spool[rcpt][i][Q_SOURCE] );
			//m += "\n" + spool[rcpt][i][Q_DATA];
		//D("m is -"+m+"-\n");
		unless (t2) t2 = spool[rcpt][i][Q_URL_LOGIN];
	}
//	mwrite("Date:\t"+mctime(time())+"\n");
//	mwrite("X-Mailer: psycMUVE eMail Gateway\n");
//	mwrite("X-Chatter: "+src->qName()+"\n");
//write("Reply-To: "+sndr+"."+lower_case(CHATNAME)+"@"+SMTP_RELAY+"\n");

	if (o) m += w("_warning_omitted_messages",
		"[_amount_messages_omitted] messages omitted.",
		([ "_amount_messages_omitted" : o ]) );
#ifdef DEFAULT_URL_LOGIN
# ifndef _flag_disable_mail_signature
	m += "\n--\n"+ w( "_info_mail_signature", "Reply by E-Mail is not possible. Please log in:\n[_URL_login]",
		([ "_URL_login": (t2 || DEFAULT_URL_LOGIN) ]) ) + "\n";
# endif
#endif
	t2 = "Content-Type: text/plain; charset=" SYSTEM_CHARSET "\n"
#ifndef SEND_FIRST_MSG_AS_SUBJECT
		"Subject: "+ w("_info_mail_subject", DEFAULT_MAIL_SUBJECT,
				([ "_target": target || rcpt || ME ]) ) +
#endif
		"Content-Length: "+( strlen(m) )+"\n"
		"\n"+ m +".\n";
	mwrite(t2);
}

logon() { input_to(#'parse); }

