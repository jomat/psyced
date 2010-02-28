// $Id: threads.c,v 1.41 2008/01/05 12:42:17 lynx Exp $ // vim:syntax=lpc
//
#include <net.h>
#include <person.h>
#include <status.h>
#include <lastlog.h>

#ifndef DEFAULT_BACKLOG
# define DEFAULT_BACKLOG 10
#endif

#ifndef STYLESHEET
# define STYLESHEET (v("_uniform_style") || "/static/examine.css")
#endif

inherit NET_PATH "place/owned";

qHistoryPersistentLimit() {
    return 0;
}

canPost(snicker) {
    return qAide(snicker);
}

canDeleteOwn(snicker) {
    return qAide(snicker);
}

canDeleteEverything(snicker) {
    return qOwner(snicker);
}

int mayLog(string mc) {
    return abbrev("_notice_thread", mc) || abbrev("_message", mc);
}

int showWebLog() {
    return 1;
}

int numEntries() {
    return logSize("_notice_thread");
}

create() {
	P3((">> threads:create()\n"))
	::create();

	//index entries from 1
	logSet(0, ({0, 0, 0, 0}));
}

varargs array(mixed) entries(int limit, int offset, int reverse, int parent, int id) {
    P3((">> entries(%O, %O, %O)\n", limit, offset, parent))
    array(mixed) entries = ({}), entry, children, child;
    mapping vars;
    int i, n = 0, o = 0;
    int from = id || logSize() - 1;
    int to = id || parent || 0;
    for (i = from; i >= to; i--) {
	unless (logPick(i)) continue;
	entry = logPick(i);
	unless (abbrev("_notice_thread", entry[LOG_MC])) continue;
	PT((">>> entry %O: %O\n", i, entry))
	vars = entry[LOG_VARS];
	if (vars["_parent"] != parent) continue;
	if (o++ < offset) continue;
	children = ({});
	if (member(vars, "_children")) {
	    foreach (int c : vars["_children"]) {
		if (child = logPick(c)) {
		    children += ({ child + ({ entries(0, 0, reverse, c) }) });
		}
	    }
	}
	PT((">>> adding %O: %O\n", i, entry))
	if (reverse) {
	    entries += ({ entry + ({ children }) });
	} else {
	    entries = ({ entry + ({ children }) }) + entries;
	}
	if (limit && ++n >= limit) break;
    }
    PT((">>> entries: %O\n", entries))
    return entries;
}

varargs array(mixed) entry(int id) {
    return entries(0, 0, 0, 0, id);
}

varargs int addEntry(mixed source, string snicker, string text, string title, int parent_id) {
    P3((">> addEntry(%O, %O, %O, %O, %O)\n", source, snicker, text, title, parent_id))
    int id = logSize();
    string mc = "_notice_thread_entry";
    string data = "[_nick] [_action]: ";

    mapping vars = ([
		     "_id": id,
		     "_text": text,
		     "_nick": snicker,
		     "_action": "adds", //TODO: add a /set'ting for it, or find a better name
		     ]);

    if (parent_id) {
	P3((">>> parent_id: %O\n",  parent_id))
	array(mixed) parent;
	unless (parent = logPick(parent_id)) return 0;
	P3((">>> parent: %O\n",  parent))
	unless (parent[LOG_VARS]["_children"]) parent[LOG_VARS]["_children"] = ({ });
	parent[LOG_VARS]["_children"] += ({ id });
	save();

	mc += "_reply";
	data = member(parent[LOG_VARS], "_title") ?
	    "[_nick] [_action] in reply to #[_parent] ([_parent_title]): " :
	    "[_nick] [_action] in reply to #[_parent]: ",
	vars += ([ "_parent": parent_id ]);
    }

    if (title && strlen(title)) {
	vars += ([ "_title": title ]);
	data += "[_title]\n[_text]";
    } else {
	data += "[_text]";
    }

    data += " (#[_id] in [_nick_place])";

    castmsg(source, mc, data, vars);
    return 1;
}

int delEntry(int id, mixed source, mapping vars)  {
    array(mixed) entry;
    unless (entry = logPick(id)) return 0;

    string unick;
    unless (canDeleteEverything(SNICKER))
	unless (canDeleteOwn(SNICKER) && lower_case(psyc_name(source)) == lower_case(entry[LOG_SOURCE][LOG_SOURCE_UNI]))
	    return 0;

    logSet(id, ({0,0,0,0}));
    save();
    return 1;
}

sendEntries(mixed source, array(mixed) entries, int level) {
    P3((">> sendEntries(%O, %O)\n", source, entries))
    mapping vars;
    int n = 0;
    unless(source && entries) return n;
    foreach(array(mixed) entry : entries) {
	PT(("entry: %O\n", entry))
	vars = entry[LOG_VARS];
	sendmsg(source, regreplace(entry[LOG_MC], "^_notice", "_list", 1),
		"[_indent][_nick]: " + (vars["_title"] ? "[_title]\n" : "") + "[_text] (#[_id])",
		vars + ([ "_level": level, "_indent": x("  ", level) ]));
	if (sizeof(entry) >= LOG_CHILDREN + 1) sendEntries(source, entry[LOG_CHILDREN], level + 1);
	n++;
    }
    return n;
}

_request_entries(source, mc, data, vars, b) {
    int num = to_int(vars["_num"]) || DEFAULT_BACKLOG;
    sendEntries(source, entries(num));
    return 1;
}

_request_entry(source, mc, data, vars, b) {
    unless (vars["_id"] && strlen(vars["_id"])) {
	sendmsg(source, "_warning_usage_entry",
		"Usage: /entry <id>", ([ ]));
	return 1;
    }

    int id = to_int(vars["_id"]);
    unless(sendEntries(source, entry(id))) {
	sendmsg(source, "_error_thread_invalid_entry",
		"#[_id]: no such entry", (["_id": id]));
    }

    return 1;
}

_request_addentry(source, mc, data, vars, b) {
    P3((">> _request_addentry(%O, %O, %O, %O, %O)\n", source, mc, data, vars, b))
    unless (canPost(SNICKER)) return 0;
    unless (vars["_text"] && strlen(vars["_text"])) {
	sendmsg(source, "_warning_usage_addentry",
		"Usage: /addentry <text>", ([ ]));
	return 1;
    }
    addEntry(source, SNICKER, vars["_text"], vars["_title"]);
    return 1;
}

_request_comment(source, mc, data, vars, b) {
    P3((">> _request_comment(%O, %O, %O, %O, %O)\n", source, mc, data, vars, b))
    unless (vars["_id"] && strlen(vars["_id"]) &&
	    vars["_text"] && strlen(vars["_text"])) {
	sendmsg(source, "_warning_usage_reply",
		"Usage: /comment <id> <text>", ([ ]));
	return 1;
    }

    int id = to_int(vars["_id"]);
    string snicker = SNICKER;
    P3((">>> id: %O, vars: %O\n", id, vars));
    unless (addEntry(source, snicker, vars["_text"], vars["_title"], id))
	sendmsg(source, "_error_thread_invalid_entry",
		"#[_id]: no such entry", (["_id": id]));

    return 1;
}

_request_delentry(source, mc, data, vars, b) {
    P3((">> _request_delentry(%O, %O, %O, %O, %O)\n", source, mc, data, vars, b))
    unless (canPost(SNICKER)) return 0;
    unless (vars["_id"] && strlen(vars["_id"])) {
	sendmsg(source, "_warning_usage_delentry",
		"Usage: /delentry <id>", ([ ]));
	return 1;
    }
    int id = to_int(vars["_id"]);
    if (delEntry(id, source, vars)) {
	sendmsg(source, "_notice_thread_entry_removed",
		"Entry #[_id] has been removed.",
		([ "_id" : id ]) );
    } else {
	sendmsg(source, "_error_thread_invalid_entry",
		"#[_id]: no such entry", (["_id": id]));
    }
    return 1;
}

#if 0
_request_title(source, mc, data, vars, b) {
    P3((">> _request_title(%O, %O, %O, %O, %O)\n", source, mc, data, vars, b))
    unless (vars["_id"] && strlen(vars["_id"])) {
	sendmsg(source, "_warning_usage_title",
		"Usage: /title <id> <title>", ([ ]));
	return 1;
    }

    int id = to_int(vars["_id"]);
    unless (setTitle(id, vars["_title"]))
	sendmsg(source, "_error_thread_invalid_entry",
		"#[_id]: no such entry", (["_id": id]));

    return 1;
}
#endif

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

varargs string htmlComments(array(mixed) entries, int level) {
    mapping entry, vars;
    string ht = "", style;
    foreach(entry : entries) {
	vars = entry[LOG_VARS];
	style = level ? "style='padding-left: " + level + "em'" : "";
	ht += "<div class='comment' title='" + isotime(ctime(vars["_time_place"]), 1) + "' " + style + "><span class='comment-author'>" + vars["_nick"] + "</span>: <span class='comment-text'>" + htquote(vars["_text"], 1) + "</span></div>\n";
	if (sizeof(entry) >= LOG_CHILDREN + 1) ht += htmlComments(entry[LOG_CHILDREN], level + 1);
    }
    return ht;
}

varargs string htmlEntries(array(mixed) entries, int nojs, string chan, string submit, string url_prefix) {
    P3((">> threads:htmlentries(%O, %O, %O, %O, %O)\n", entries, nojs, chan, submit, url_prefix))
    string text, ht = "";
    string id_prefix = chan ? chan + "-" : "";
    unless (url_prefix) url_prefix = "";
    unless (nojs) ht +=
	"<script type='text/javascript'>\n"
	  "function toggle(e) { if (typeof e == 'string') e = document.getElementById(e); e.className = e.className.match('hidden') ? e.className.replace(/ *hidden/, '') : e.className + ' hidden'; }\n"
	"</script>\n";

    mapping entry, vars;
    foreach (entry : entries) {
	P3((">>> entry: %O\n", entry))
	vars = entry[LOG_VARS];

	text = htquote(vars["_text"], 1);

	string comments = "";
	if (sizeof(entry) >= LOG_CHILDREN + 1) comments = htmlComments(entry[LOG_CHILDREN]);

	ht +=
	    "<div class='entry'>\n"
	      "<div class='header'>\n"
	        "<a href=\"" + url_prefix + "?id=" + vars["_id"] + "\">"
	          "<span class='id'>#" + vars["_id"] + "</span> - \n"
		  "<span class='author'>" + vars["_nick"] + "</span>\n"
	          + (vars["_title"] && strlen(vars["_title"]) ? " - " : "") +
	          "<span class='title'>" + htquote(vars["_title"] || "") + "</span>\n"
	        "</a>"
	      "</div>\n"
	      "<div class='body'>\n"
		"<div class='text'>" + text + "</div>\n"
		"<div id='comments-" + id_prefix + vars["_id"] + "' class='comments'>" + comments +
	        (submit && strlen(submit) ?
		  "<a onclick=\"toggle(this.nextSibling)\">&raquo; reply</a>"
		  "<div class='comment-submit hidden'>"
		    "<textarea autocomplete='off'></textarea>"
		    //FIXME: cmd is executed twice, because after a set-cookie it's parsed again
	            "<input type='button' value='Send' onclick=\"cmd('comment " + vars["_id"] + " '+ this.previousSibling.value, '" + submit + "')\">"
		  "</div>" : "") +
	        "</div>\n"
	      "</div>\n"
	      "<div class='footer'>\n"
		"<span class='date'>" + isotime(ctime(vars["_time_place"]), 1) + "</span>\n"
		"<span class='comments-link'>"
		  "<a onclick=\"toggle('comments-" + id_prefix + vars["_id"] + "')\">" + sizeof(vars["_children"]) + " comments</a>"
		"</span>\n"
	      "</div>\n"
	    "</div>\n";
    }
    P3((">>> ht: %O\n", ht))
    return "<div class='threads'>" + ht + "</div>";
}

// TODO: fix markup, not displayed correctly (in firefox at least)
string rssEntries(array(mixed) entries) {
    string rss =
	"<?xml version=\"1.0\" encoding=\"" SYSTEM_CHARSET "\" ?>\n"
	"<rdf:RDF\n"
	  "xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"\n"
	  "xmlns=\"http://purl.org/rss/1.0/\">\n\n"
	"<channel>\n"
	  "\t<title>PSYC - Protocol for Synchronous Conferencing</title>\n"
	  "\t<link>http://www.psyc.eu</link>\n"
	  "\t<description>News about the PSYC project</description>\n"
	"</channel>\n";

    mapping entry, vars;
    foreach (entry : entries) {
	vars = entry[LOG_VARS];
	rss +=
	    "\n<item>\n"
	      "\t<title>"+ (vars["_title"] || "no title") +"</title>\n"
	      "\t<link>http://" + HTTP_OR_HTTPS_URL + "/" + pathName() +  "?id=" + vars["_id"] + "</link>\n"
	      "\t<description>" + vars["_text"] + "</description>\n"
	      "\t<dc:date>" + isotime(ctime(vars["_time_place"]), 1) + "</dc:date>\n"
	      "\t<dc:creator>" + vars["_nick"] + "</dc:creator>\n"
	    "</item>\n";
    }

    rss += "</rdf:RDF>\n";
    return rss;
}

string jsEntries(array(mixed) entries) {
    string js =
	"function Entry(id, thread, author, date, text) {\n"
	  "\tthis.id = id;\n"
	  "\tthis.thread = thread;\n"
	  "\tthis.author = author;\n"
	  "\tthis.date = date;\n"
	  "\tthis.text = text;\n"
	"}\n\n"
	"document.blogentries = new Array(\n";

    mapping entry, vars;
    foreach (entry : entries) {
	vars = entry[LOG_VARS];
	js += "new Entry(" + vars["_id"] + ","
		"\"" + vars["_title"] + "\","
		"\"" + vars["_nick"] + "\","
		+ isotime(ctime(vars["_time_place"]), 1) + ","
		"\"" + vars["_text"] + "\"),\n";
	}

    return js[..<3] + ");";
}

varargs string jsonEntries(int limit, int offset) {
    return make_json(entries(limit, offset));
}

varargs void jsonExport(int limit, int offset) {
    write(jsonEntries(limit, offset));
}

varargs void jsExport(int limit, int offset) {
    write(jsEntries(entries(limit, offset)));
}

varargs void rssExport(int limit, int offset) {
    write(rssEntries(entries(limit, offset, 1)));
}

varargs string htMain(int limit, int offset, string chan) {
    return htmlEntries(entries(limit, offset, 1), 0, chan);
}

varargs void displayMain(int limit, int offset) {
    write(htMain(limit, offset));
}

string htEntry(int id) {
    return htmlEntries(entry(id));
}

void displayEntry(int id) {
    write(htEntry(id) || "No such entry.");
}

// wir können zwei strategien fahren.. die technisch einfachere ist es
// die reihenfolge der elemente festzulegen und für jedes ein w(_HTML_xy
// auszuspucken. flexibler wär's stattdessen wenn jede seite ein einziges
// w(_PAGES_xy ausgeben würde in dem es per [_HTML_list_threads] oder
// ähnlichem die blog-elemente per psyctext-vars übergibt ... dann kann
// es immernoch per {_HTML_head_threads} header und footer einheitlich
// halten. womöglich kann man auch nachträglich plan A in plan B
// umwandeln..... hmmm -lynX
//
void displayHeader() {
    w("_HTML_head_threads",
      "<html><head><link rel='stylesheet' type='text/css' href='"+ STYLESHEET +"'></head>\n"+
      "<body class='threads'>\n\n");
}
void displayFooter() {
    w("_HTML_tail_threads", "</body></html>");
}

htget(prot, query, headers, qs, data) {
    mapping entrymap;
    mixed target;
    string nick;
    int a;
    int limit = to_int(query["limit"]) || DEFAULT_BACKLOG;
    int offset = to_int(query["offset"]);
    string webact = PLACE_PATH + MYLOWERNICK;
    // shouldnt it be "html" here?
    sTextPath(query["layout"] || MYNICK, query["lang"], "ht");

    // Kommentare anzeigen
    if (query["id"]) {
	htok(prot);
	// kommentare + urspruengliche Nachricht anzeigen
	displayHeader();
	displayEntry(to_int(query["id"]));
#if 0
	// eingabeformular ohne betreff
	write("<form action='" + webact + "' method='GET'>\n"
	      "<input type='hidden' name='request' value='post'>\n"
	      "PSYC Uni: <input type='text' name='uni'><br>\n"
	      "<input type='hidden' name='reply' value='" + query["comments"] +"'>\n"
	      "<textarea name='text' rows='14' cols='80'>Enter your text here</textarea><br>\n"
	      "<input type='submit' value='submit'>\n"
	      "</form>\n");
	write("<br><hr><br>");
#endif
	//logView(a < 24 ? a : 12, "html", 15);
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

    //::htget(prot, query, headers, qs, data, 1);	// no processing, just info

    string export = query["export"] || query["format"];
    if (export == "js") {
	// check If-Modified-Since header
	htok3(prot, "application/x-javascript", "Cache-Control: no-cache\n");
	jsExport(limit, offset);
    } else if (export == "json") {
	// check If-Modified-Since header
	htok3(prot, "application/json", "Cache-Control: no-cache\n");
	jsonExport(limit, offset);
    } else if (export == "rss" || export == "rdf") {
	// export als RSS
	// scheinbar gibt es ein limit von 15 items / channel
	// htquote auch hier anwenden
	// check If-Modified-Since header
	htok3(prot, "text/xml", "");
	rssExport(limit, offset);
    } else {
	// normaler Export
	//P2(("all entries: %O\n", _thread))
	    htok3(prot, "text/html", "Cache-Control: no-cache\n");
	displayHeader();
	// display the blog
	displayMain(limit, offset);
	// display the chatlog

	if (showWebLog()) logView(a < 24 ? a : 12, "html", 15);
	displayFooter();
    }
    return 1;
}

// don't know if this works, needs to be tested
void nntpget(string cmd, string args) {
    array(mixed) entry, entries;
    mapping vars;
    int i;
    P2(("calling nntpget %s with %O\n", cmd, args))
    switch(cmd) {
	case "LIST":
	    write(MYNICK + " 0 1 n\n");
	    break;
	case "ARTICLE":
	    i = to_int(args) - 1;
	    //P2(("i is: %d\n", i))
	    unless (entry = entry(i)) break;
	    vars = entry[LOG_VARS];
	    write(S("220 %d <%s%d@%s> article\n", 
		    i + 1, MYNICK, i + 1, SERVER_HOST));
	    write(S("From: %s\n", vars["_nick"]));
	    write(S("Newsgroups: %s\n", MYNICK));
	    write(S("Subject: %s\n", vars["_title"]));
	    write(S("Date: %s\n", isotime(ctime(vars["_time_place"]), 1)));
	    write(S("Xref: %s %s:%d\n", SERVER_HOST, MYNICK, i + 1));
	    write(S("Message-ID: <%s$%d@%s>\n", MYNICK, i+1, SERVER_HOST));
	    write("\n");
	    write(vars["_text"]);
	    write("\n.\n");
	    break;
	case "GROUP":
	    write(S("211 %d 1 %d %s\n", numEntries(), numEntries(), MYNICK));
	    break;
	case "XOVER":
	    entries = entries();
	    foreach (entry : entries) {
		unless (entry = entry(i)) break;
		vars = entry[LOG_VARS];
		write(S("%d\t%s\t%s\t%s <%s%d@%s>\t1609\t22\tXref: news.t-online.com\t%s:%d\n",
			i+1, vars["_title"],
			vars["_nick"], isotime(ctime(vars["_time_place"]), 1),
			MYNICK, i+1,
			SERVER_HOST, MYNICK, i+1));
	    }
	    break;
	default:
	    P2(("unimplemented nntp command: %s\n", cmd))
    }
}




/**** old stuff ****/

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

#if 0
_request_iterator(source, mc, data, vars, b) {
    unless (canPost(SNICKER)) return 0;
    sendmsg(source, "_notice_thread_iterator",
	    "[_iterator] blog entries have been requested "
	    "since creation.", ([
				 // i suppose this wasn't intentionally using
				 // MMP _count so i rename it to _iterator
				 "_iterator" : v("iterator")
				 ]) );
    return 1;
#endif

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
	sendmsg(source, "_message_", "([_id]) \"[_topic]\", [_author]", ([ // ??
	    "_topic" : ar["topic"],
		"_text" : ar["text"],
		"_author" : ar["author"],
		"_date" : ar["date"],
		"_id" : i++,
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
