// $Id: mailcast.c,v 1.7 2008/01/05 12:42:17 lynx Exp $

#include <net.h>
#include <person.h>
#include <status.h>

inherit NET_PATH "place/basic";

// noch nicht ganz klar ob das nicht einfach ein job der /history ist..
// und wir ganz generell hin und wieder mehrfache histories brauchen
//
array(mapping) mails = ({ });
//
// wieviele mails sollen gespeichert bleiben?
// ...waer gut, wenn man das im raumfile definieren koennt und hier bleibts..
//
#ifndef _limit_amount_history_mailcast
#define _limit_amount_history_mailcast 7
#endif

msg(source, mc, data, mapping vars) {
	if (abbrev("_notice_email_delivered", mc)) {
		// eine _notice_email_delivered wird im server-fenster angezeigt, nicht im raum..
		// naja, der raum ist eh nicht zum chatten da..
		// net/irc sollte das problem geschickter lösen dann..
		// und nicht hier sowas komisches da:
		castmsg(source, "_message_public_mail",
			vars["_subject"]+":\n"+vars["_content"],
			vars);
		if (sizeof(mails) >= _limit_amount_history_mailcast) {
			mails[0] = 0;
			mails -= ({ 0 });
		}
		mails += ({ ([
			"_source" : source,
			"_subject" : vars["_subject"],
			"_content" : vars["_content"]
		]) });
		save();
		return;
	}
	return ::msg(source, mc, data, vars);
}

htget(prot, query, headers, qs, data, noprocess) {
	htok(prot);

#ifndef STYLESHEET
# define STYLESHEET (v("_uniform_style") || "http://www.psyced.org/mail.css")
#endif
	write("<link rel='stylesheet' type='text/css' href='"+
		     STYLESHEET +"'>\n");

// typically use textdb here to obtain different layouts
//
//	write("<table border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"
//		" <tr>\n  <td>Source</td>\n  <td>Subject</td>\n  <td>Content</td>\n </tr>\n");
	for (int i=sizeof(mails)-1; i>=0; i--) {
		mapping mail = mails[i];
		string t = htquote(mail["_content"]);
		t = replace(t, "\n", "<br/>\n");

//		write(" <tr>\n  <td>"+ mail["_source"] +"</td>\n  <td>"+ mail["_subject"] +"</td>\n  <td>"+ t +"</td>\n </tr>\n");
		write("<ul>\n<li>"+ mail["_source"] +"</li>\n<li>"+ mail["_subject"] +"</li>\n<li>"+ t +"</li>\n</ul>\n");
	}
//	write("</table>");
	return 1;
}

