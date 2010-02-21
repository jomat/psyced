// $Id: threads.c,v 1.41 2008/01/05 12:42:17 lynx Exp $ // vim:syntax=lpc
//
#include <net.h>
#include <person.h>
#include <status.h>

inherit NET_PATH "place/owned";

#ifndef DEFAULT_BACKLOG
# define DEFAULT_BACKLOG 5
#endif

// datenstruktur für threads?
//
// bestehende struktur ist: großes array von entries.
//
// wie wärs mit mapping mit key=threadname und value=array-of-entries
// subjects werden abgeschafft: sie sind der name des threads
// wer einen thread in seinem reply umnennen will legt in wirklichkeit
// einen neuen thread an, meinetwegen mit "was: old thread"
//
// der nachteil an solch einer struktur wäre, dass man neue comments
// in alten threads nicht so schnell findet - man ist auf die notification
// angewiesen, was andererseits die stärke von psycblogs ist.
// man könnte die notifications zudem noch in die history einspeisen..
//
// nachteile an der bestehenden struktur ist: 1. threadname in jeder
// entry, 2. threads nur mittels durchlauf des ganzen blogs darstellbar 
//
// momentmal.. das was du "comments" nennst sind doch schon die threads!

protected mapping* _thread;

volatile int last_modified;
volatile string webact;

create() {
	P3((">> threads:create()\n"))
	::create();
	unless (pointerp(_thread)) _thread = ({ });
}

cmd(a, args, b, source, vars) {
	P3((">> threads:cmd(%O, %O, %O, %O, %O)", a, args, b, source, vars))
// TODO: multiline-sachen irgendwie
	mapping entry; 
	int i = 0;
	int num_entries;
	//unless (source) source = previous_object();
	switch (a) {
	case "entries":
		num_entries = sizeof(args) >= 2 ? to_int(args[1]) : DEFAULT_BACKLOG;
		// _thread[<5..]
		foreach( entry : _thread[<num_entries..] ) {
			sendmsg(source, "_list_thread_item", 
					"#[_number] - [_author][_sep][_thread]: [_text] ([_comments])",
					([ 
						"_sep" : strlen(entry["thread"]) ? " - " : "",
						"_thread" : entry["thread"],
						"_text" : entry["text"],
						"_author" : entry["author"],
						"_date" : entry["date"],
						"_comments": sizeof(entry["comments"]),
						"_number" : i++,
						"_nick_place" : MYNICK ]) );
		}
		return 1;
	case "entry":
		unless (sizeof(args) > 1){
			sendmsg(source, "_warning_usage_entry",
					"Usage: /entry <threadid>", ([ ]));
			return 1;
		}
		int n = to_int(args[1]);
		entry = _thread[n];
		sendmsg(source, "_list_thread_item",
				"#[_number] [_author][_sep][_thread]: [_text] ([_comments])",
				([
					"_sep" : strlen(entry["thread"]) ? " - " : "",
					"_thread" : entry["thread"],
					"_text" : entry["text"],
					"_author" : entry["author"],
					"_date" : entry["date"],
					"_comments": sizeof(entry["comments"]),
					"_number" : n,
					"_nick_place" : MYNICK ]) );

		if (entry["comments"]) {
		    foreach(mapping item : entry["comments"]) {
			sendmsg(source, "_list_thread_comment",
					"> [_nick]: [_text]",
					([
						"_nick" : item["nick"],
						"_text" : item["text"],
						"_nick_place" : MYNICK ]) );
		    }
		}
		return 1;
	case "thread":
		unless (sizeof(args) > 2){
			sendmsg(source, "_warning_usage_thread",
					"Usage: /thread <threadid> <title>", ([ ]));
			return 1;
		}
		return setSubject(to_int(args[1]), ARGS(2));
	case "comment":
		unless (sizeof(args) >= 2) {
		    sendmsg(source, "_warning_usage_reply",
	                            "Usage: /comment <threadid> <text>", ([ ]));
		    return 1;
		}
		return addComment(ARGS(2), SNICKER, to_int(args[1]));
	case "blog":
	case "submit":
	case "addentry":
		unless (canPost(SNICKER)) return;
		unless (sizeof(args) >= 1) {
		    sendmsg(source, "_warning_usage_submit", 
	                            "Usage: /submit <text>", ([ ]));
		    return 1;
		}
		return addEntry(ARGS(1), SNICKER);
	// TODO: append fuer multiline-sachen
#if 0
	case "iterator":
		unless (canPost(SNICKER)) return;
		sendmsg(source, "_notice_thread_iterator",
			"[_iterator] blog entries have been requested "
			"since creation.", ([
			    // i suppose this wasn't intentionally using
			    // MMP _count so i rename it to _iterator
			    "_iterator" : v("iterator")
			]) );
		return 1;
#endif
	case "deblog":
	case "delentry":
		unless (canPost(SNICKER)) return;
		// ist das ein typecheck ob args ein int is?
		if (sizeof(regexp( ({ args[1] }) , "^[0-9][0-9]*$"))) {
		    unless (delEntry(to_int(args[1]), source, vars)) {
			    sendmsg(source,"_error_invalid_thread_item",
				"There is no such thread item.", ([ ]));
		    } else {
			sendmsg(source, "_notice_thread_item_removed", 
			    "Thread item [_number] has been removed.", 
			    ([ "_number" : ARGS(1) ]) );
		    }
		}
		return 1;
	}
	return ::cmd(a, args, b, source, vars);
}

msg(source, mc, data, vars){
	P3(("thread:msg(%O, %O, %O, %O)", source, mc, data, vars))
	// TODO: die source muss hierbei uebereinstimmen mit dem autor
	if (abbrev("_notice_authentication", mc)){
		sendmsg(source, "_notice_place_blog_authentication_success", "([_entry]) has been authenticated", 
				([ "_entry" : "1" ]) );	
		return;
	}
	if (abbrev("error_invalid_authentication", mc)) {
		sendmsg(source, "_notice_place_blog_authentication_failure", "Warning, someone pretends to blog as you", ([ ]) );
		return;
	}
	return ::msg(source, mc, data, vars);
}

#if 0
listLastEntries(number) {
    mapping* entries;
    int i;
    entries = _thread || ({ }); 

    unless (sizeof(entries)) return 1;
    
    i = v("iterator") || 0;
    vSet("iterator", i + 1);

    return entries[<number..];
}
#endif 
#if 0
allEntries(source) {
    mapping* entries;
    mapping ar;
	int i = 0;

    entries = _thread || ({ });

    unless (sizeof(entries)) return 1;

    foreach (ar : entries) {
	sendmsg(source, "_message_", "([_number]) \"[_topic]\", [_author]", ([ // ??
	    "_topic" : ar["topic"],
		"_text" : ar["text"],
		"_author" : ar["author"],
		"_date" : ar["date"],
		"_number" : i++,
		"_nick_place" : MYNICK ]) );
    }
    return 1;
}
#endif 

#if 0
addForum() {
	// suggested protocol message for the buha forum
	// (creation of a new thread)

:_target     psyc://psyced.org/@buha
:_encoding   utf-8
 
:_nick_forum morpheus
:_category   Test-Forum
:_thread     Test-Thread_
:_page_thread https://www.buha.info/board/showthread.php?t=1

_notice_thread
[_nick_forum] hat einen neuen Thread in [_category] erstellt: [_thread] ([_page_thread])
.

}
#endif

setSubject(num, thread) {
	mapping* entries;

	entries = _thread || ({ });
	// TODO: das hier muss sicherer
	entries[num]["thread"] = thread;
	_thread = entries;
	save();
	return 1;
}

// TODO: topic uebergeben
addEntry(text, unick, thread) {
	mapping* entries;
	mapping newentry = ([	"text" : text,
				"author" : unick,
				"date" : isotime(ctime(), 1),
				"thread" : thread || "",
			     ]);
	entries = _thread || ({ });
	entries += ({ newentry }); 
	_thread = entries;
	save();
	castmsg(ME, "_notice_thread_item",
		"[_nick] adds an entry in \"[_thread]\" of [_nick_place].", ([
			"_entry" : text,
			"_thread" : thread,
			"_nick" : unick,
		]) );
	return 1;
}

addComment(text, unick, entry_id) {
	mapping entry;
	
	if (sizeof(_thread) > entry_id) {
		entry = _thread[entry_id];
		unless (entry["comments"]) {
			entry["comments"] = ({ });
		}
		entry["comments"] += ({ (["text" : text, "nick" : unick ]) });
		// vSet("entries", entries);
		castmsg(ME, "_notice_thread_comment",
	    "[_nick] adds a comment in \"[_thread]\" of [_nick_place].", ([
				"_entry" : entry["text"],
				"_thread" : entry["thread"],
				"_comment" : text,
				"_nick" : unick,
		]) );
		save();
		return 1;
	}
	return -1;
}

delEntry(int number, source, vars)  {
    array(string) entries, authors, a;
    string unick;
    int size;

    entries = _thread || ({ });

    unless (size = sizeof(entries)) return 0;
    if (number >= size) return 0;

    if (canPost(unick = lower_case(SNICKER))) {
	unless (lower_case(entries[number]["author"]) == unick) return 0;
    }

    _thread = entries[0..number-1] + entries[number+1..];
    //_thread[number] = 0;
    save();

    return 1;
}

htget(prot, query, headers, qs, data) {
	mapping entrymap;
	mixed target;
	string nick;
	int i;
	int a;
	mapping* entries;
	
	int num_entries = query["last"] ? to_int(query["last"]) : DEFAULT_BACKLOG;
	unless (webact) webact = PLACE_PATH + MYLOWERNICK;
					// shouldnt it be "html" here?
	sTextPath(query["layout"] || MYNICK, query["lang"], "ht");

	// Kommentare anzeigen
	if (query["comments"]) {
		htok(prot);
		// kommentare + urspruengliche Nachricht anzeigen
		displayHeader();
		displayComments(_thread[to_int(query["comments"])]);
		// eingabeformular ohne betreff
		write("<form action='" + webact + "' method='GET'>\n"
			"<input type='hidden' name='request' value='post'>\n"
			"PSYC Uni: <input type='text' name='uni'><br>\n"
			"<input type='hidden' name='reply' value='" + query["comments"] +"'>\n"
			"<textarea name='text' rows='14' cols='80'>Enter your text here</textarea><br>\n"
		    "<input type='submit' value='submit'>\n"
			"</form>\n");
		write("<br><hr><br>");
		logView(a < 24 ? a : 12, "html", 15);
		displayFooter();
		return 1;
	}

	// formularbehandlung
	if (query["request"] == "post" && query["uni"]) {
		htok(prot);
		/*
		sendmsg uni -> _request_authentication mit thread und text drin
		dann auf die antwort warten die nen vars mapping mit thread + text hat wieder
		*/
		if (nick = legal_name(target = query["uni"])) {
			target = summon_person(nick);
			nick = target->qNick();
		} else {
			nick = target;
//			write("Hello " + query["uni"] + "<br>\n");
//			write("Remote auth doesn't work yet. TODO!!!\n");
//			return 1;
		}
#ifdef OWNED
		if (canPost(nick)) {
#endif
#if 0 
		sendmsg(target, "_request_authentication", "please auth me!", 
					(["_host_IP" : query_ip_number(),
					  "_blog_thread" : query["thread"],
					  "_blog_text" : query["text"] ]));
		write("your submit is avaiting authentication by " + query["uni"] + "<br>\n");
#endif // 0
		if (target->checkAuthentication(ME, ([ "_host_IP" : query_ip_number() ]) ) > 0) {
			// check ob reply auf irgendwas ist...
			if (query["reply"]) {
				addComment(query["text"], query["uni"], to_int(query["reply"]));
			} else {
				addEntry(query["text"], query["uni"], query["thread"]);
			}
			write("authentication successful!\n");
		} else {
			write("not authenticated!\n");
		}
#ifdef OWNED
		} else {
			write("You are not owner or aide of this place.\n");
		}
#endif
		return 1;
	}
	// neuen Eintrag verfassen
	if (query["request"] == "form") {
		htok(prot);
		displayHeader();
		write("<form action='" + webact + "' method='GET'>\n"
      "<input type='hidden' name='request' value='post'>\n"
      "PSYC Identity:<br><input type='text' name='uni' size=60><br>\n"
      "Thread:<br><input type='text' name='thread' size=60><br>\n"
      "Text:<br>\n<textarea name='text' rows='14' cols='64'></textarea><br>\n"
      "<input type='submit' value='CREATE MESSAGE'>\n"
      "</form>\n");
		displayFooter();
		return 1;
	}
	::htget(prot, query, headers, qs, data, 1);	// no processing, just info
	// javascript-export
	if (query["export"] == "javascript") {
	    // check If-Modified-Since header
		htok3(prot, "application/x-javascript", "Cache-Control: no-cache\n");
		jscriptExport(num_entries);
	} else if (query["export"] == "rss" || query["export"] == "rdf") {
		// export als RSS
		// scheinbar gibt es ein limit von 15 items / channel
		// htquote auch hier anwenden
		// check If-Modified-Since header
		htok3(prot, "text/xml", "");
		rssExport(num_entries);
	} else {
	    // normaler Export
		P2(("all entries: %O\n", _thread))
		htok3(prot, "text/html", "Cache-Control: no-cache\n");
		displayHeader();
		// display the blog
		displayMain(num_entries);
		// display the chatlog
		logView(a < 24 ? a : 12, "html", 15);
		displayFooter();
	}
	return 1;
}

rssExport(last) {
	int i;
	int len;
	
	len = sizeof(_thread);
	if (last > len) last = len;
	write("<?xml version=\"1.0\" encoding=\"" SYSTEM_CHARSET "\" ?>\n"
	      "<rdf:RDF\n"
	      "xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"\n"
	      "xmlns=\"http://purl.org/rss/1.0/\">\n\n"
	      "<channel>\n"
	      "\t<title>PSYC - Protocol for Synchronous Conferencing</title>\n"
	      "\t<link>http://www.psyc.eu</link>\n"
	      "\t<description>News about the PSYC project</description>\n"
	      "</channel>\n");
	for (i = len - last; i < len; i++) {
		write("\n<item>\n"
		      "\t<title>"+ _thread[i]["thread"]  +"</title>\n"
		      "\t<link>http://" + SERVER_HOST + ":33333" + webact +  "?comments=" + i + "</link>\n"
		      "\t<description>" + _thread[i]["text"] + "</description>\n"
		      "\t<dc:date>" + _thread[i]["date"] + "</dc:date>\n"
		      "\t<dc:creator>" + _thread[i]["author"] + "</dc:creator>\n");
		write("</item>\n");
	}
			
	write("</rdf:RDF>\n");	
}


jscriptExport(last) {
	mapping item;
	string buf = "";
	
	// htok3(prot, "application/x-javascript", "Cache-Control: no-cache\n");
	write("function Entry(thread, author, date, text) {\n"
			"\tthis.thread = thread;\n"
			"\tthis.author = author;\n"
			"\tthis.date = date;\n"
			"\tthis.text = text;\n"
			"}\n\n"
			"document.blogentries = new Array(\n");
	foreach (item : _thread[<last..]) {
		buf += "new Entry(\"" + item["thread"] + "\", \""
			    	+ item["author"] + "\", \""
				+ item["date"] + "\", \""
				+ item["text"] + "\"),\n";
	}
	buf = buf[..<3] + ");";

	write(buf);
}

#if 0
displayMain(last) {
	int i;
	int len;

	len = sizeof(_thread);
	if (last > len) last = len;
	P2(("len %d, last %d\n", len, last))
	for (i = len - last; i < len; i++) {
		write("<table><tr><td class='blogthread'>" + _thread[i]["thread"]
			+ "</td>"
			  "<td class='blogauthor'>" + _thread[i]["author"] + "</td>"
			  "<td class='blogdate'>" + _thread[i]["date"] + "</td></tr>" 
			  "<tr><td class='blogtext' colspan=3>" + _thread[i]["text"]
			+ "</td></tr>"
			  "<tr><td colspan=3 align='right'>");
		write("<a href='" + webact + "?comments=" + i + "'>there are " + sizeof(_thread[i]["comments"]) + " comments</a>");
		write("</td></tr></table>\n");
	}
}
#endif

htMain(last) {
	int i;
	int len;
	string t;
	string ht =
	  "<script type='text/javascript'>"
	    "function toggle(e) { e = document.getElementById(e); e.className = e.className.match('hidden') ? e.className.replace(/ *hidden/, '') : e.className + ' hidden'; }"
	  "</script>";

	len = sizeof(_thread);
	if (last > len) last = len;
	
	// reverse order
	for (i = len-1; i >= len - last; i--) {
		P3((">>> _thread[%O]: %O\n", i, _thread[i]))
		mapping item = _thread[i];
		t = htquote(item["text"]);
		t = replace(t, "\n", "<br>\n");
		t = replace(t, "<", "&lt;");
		t = replace(t, ">", "&gt;");

		string c = "";
		if (item["comments"])
		    foreach(mapping comment : item["comments"])
			c += "<div class='comment'><span class='comment-author'>" + comment["nick"] + "</span>: <span class='comment-text'>" + comment["text"] + "</span></div>\n";

		ht += "<div class='entry'>\n"
			"<div class='title'>\n"
		          "<span class='author'>" + item["author"] + "</span>\n"
		          "<span class='subject'>" + htquote(item["thread"]) + "</span>\n"
		        "</div>\n"
		        "<div class='body'>\n"
		          "<div class='text'>" + t + "</div>\n"
		          "<div id='comments-" + i + "' class='comments hidden'>" + c + "</div>\n"
		        "</div>\n"
		        "<div class='footer'>\n"
		          "<span class='date'>" + item["date"] + "</span>\n"
			  "<span class='comments-link'>"
		            "<a onclick=\"toggle('comments-"+i+"')\">" + sizeof(item["comments"]) + " comments</a>"
		          "</span>\n"
			"</div>\n"
		      "</div>\n";
	}
	return "<div class='threads'>" + ht + "</div>";
}

htComments(data) {
    mapping item;
    string ht = "";

    write("<b>" + data["author"] + "</b>: " + data["text"] + "<br><br>\n");
    if (data["comments"]) {       
	foreach(item : data["comments"]) {
	    ht += "<b>" + item["nick"] + "</b>: " + item["text"] + "<br>\n";
	}
    } else {
	ht += "no comments...<br>\n";
    }
    return ht;
}

displayMain(last) {
    write(htMain(last));
}

displayComments(data) {
    write(htComments(data));
}

nntpget(cmd, args) {
	mapping item;
	int i;
	P2(("calling nntpget %s with %O\n", cmd, args))
	switch(cmd) {
case "LIST":
		write(MYNICK + " 0 1 n\n");
		break;
case "ARTICLE":
		i = to_int(args) - 1;
		P2(("i is: %d\n", i))
		P2(("entries: %O\n", _thread))
		item = _thread[i];
		write(S("220 %d <%s%d@%s> article\n", 
			i + 1, MYNICK, i + 1, SERVER_HOST));
		write(S("From: %s\n", item["author"]));
		write(S("Newsgroups: %s\n", MYNICK));
		write(S("Subject: %s\n", item["thread"]));
		write(S("Date: %s\n", item["date"]));
		write(S("Xref: %s %s:%d\n", SERVER_HOST, MYNICK, i + 1));
		write(S("Message-ID: <%s$%d@%s>\n", MYNICK, i+1, SERVER_HOST));
		write("\n");
		write(item["text"]);
		write("\n.\n");
		break;
case "GROUP":
		write(S("211 %d 1 %d %s\n", sizeof(_thread), 
			sizeof(_thread), MYNICK));
		break;
case "XOVER":
		for (i = 0; i < sizeof(_thread); i++) {
			item = _thread[i];
	                P2(("item: %O\n", item))
			write(S("%d\t%s\t%s\t%s <%s%d@%s>\t1609\t22\tXref: news.t-online.com\t%s:%d\n", 
				i+1, item["thread"],
				item["author"], item["date"],
				MYNICK, i+1,
				SERVER_HOST, MYNICK, i+1));
		}
		break;
default:
		P2(("unimplemented nntp command: %s\n", cmd))

	}
}

#ifndef STYLESHEET 
# define STYLESHEET (v("_uniform_style") || "/static/examine.css")
#endif

// wir können zwei strategien fahren.. die technisch einfachere ist es
// die reihenfolge der elemente festzulegen und für jedes ein w(_HTML_xy
// auszuspucken. flexibler wär's stattdessen wenn jede seite ein einziges
// w(_PAGES_xy ausgeben würde in dem es per [_HTML_list_threads] oder
// ähnlichem die blog-elemente per psyctext-vars übergibt ... dann kann
// es immernoch per {_HTML_head_threads} header und footer einheitlich
// halten. womöglich kann man auch nachträglich plan A in plan B
// umwandeln..... hmmm -lynX
//
displayHeader() {
	w("_HTML_head_threads",
	    "<html><head><link rel='stylesheet' type='text/css' href='"+
		STYLESHEET +"'></head>\n"+
	    "<body class='threads'>\n\n");
}
displayFooter() {
	w("_HTML_tail_threads", "</body></html>");
}

canPost(snicker) {
    return qAide(snicker);
}
