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

int qHistoryPersistentLimit() { return 0; }

canPost(snicker) { return qAide(snicker); }
canReply(snicker) { return qAide(snicker); }
canEditOwn(snicker) { return qAide(snicker); }
canEditAll(snicker) { return qOwner(snicker); }
canDeleteOwn(snicker) { return qAide(snicker); }
canDeleteAll(snicker) { return qOwner(snicker); }

int mayLog(string mc) {
    if (abbrev("_notice_thread", mc))
	return regmatch(mc, "_edit\\b") ? 0 : 1;

    return abbrev("_message", mc);
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

load(name, keep) {
    int ret = ::load(name, keep);

    unless (v("addaction")) vSet("addaction", "adds");
    unless (v("editaction")) vSet("editaction", "edits");
    unless (vExist("showform")) vSet("showform", 1);
    unless (vExist("showcomments")) vSet("showcomments", 1);

    return ret;
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

varargs int addEntry(mixed source, mapping vars, string _data, string _mc) {
    P3((">> addEntry(%O, %O, %O, %O)\n", source, vars, _data, _mc))
    string mc = "_notice_thread_entry";
    string data = "[_nick] [_action]: ";
    vars["_id"] = logSize();
    vars["_action"] ||= v("addaction");
    // this should only be set after a reply
    m_delete(vars, "_children");

    if (vars["_parent"]) {
	array(mixed) parent;
	vars["_parent"] = to_int(vars["_parent"]);
	unless (parent = logPick(vars["_parent"])) return 0;
	PT((">>> parent: %O\n",  parent))
	unless (parent[LOG_VARS]["_children"]) parent[LOG_VARS]["_children"] = ({ });
	parent[LOG_VARS]["_children"] += ({ vars["_id"] });

	mc += "_reply";
	data = member(parent[LOG_VARS], "_title") ?
	    "[_nick] [_action] in reply to #[_parent] ([_parent_title]): " :
	    "[_nick] [_action] in reply to #[_parent]: ";
    }

    if (strlen(vars["_title"])) {
	data += "[_title]\n[_text]";
    } else {
	data += "[_text]";
    }
    data += " (#[_id] in [_nick_place])";

    if (_mc) mc += _mc;
    if (_data) data = _data;

    castmsg(source, mc, data, vars);
    return 1;
}

int editEntry(mixed source, mapping vars, string data)  {
    P3((">> editEntry(%O, %O, %O)\n", source, vars, data))
    array(mixed) entry;
    vars["_id"] = to_int(vars["_id"]);
    unless (entry = logPick(vars["_id"])) return 0;

    string unick;
    unless (canEditAll(SNICKER))
	unless (canEditOwn(SNICKER) && lower_case(psyc_name(source)) == lower_case(entry[LOG_SOURCE][LOG_SOURCE_UNI]))
	    return 0;

    if (strlen(data)) entry[LOG_DATA] = data;
    foreach (string key : vars)
	if (key != "_children") entry[LOG_VARS][key] = vars[key];

    save();
    castmsg(source, entry[LOG_MC] + "_edit", entry[LOG_DATA], vars + ([ "_action": v("editaction") ]));
    return 1;
}

int delEntry(int id, mixed source, mapping vars)  {
    array(mixed) entry;
    unless (entry = logPick(id)) return 0;

    string unick;
    unless (canDeleteAll(SNICKER))
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
		"[_indent][_nick]: "+ (vars["_title"] ? "[_title]\n" : "") +"[_text] (#[_id])",
		vars + ([ "_level": level, "_indent": repeat("  ", level), "_postfix_time_log": 1 ]));
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

_request_entry_add(source, mc, data, vars, b) {
    P3((">> _request_addentry(%O, %O, %O, %O, %O)\n", source, mc, data, vars, b))
    unless (canPost(SNICKER)) return 0;
    unless (vars["_text"] && strlen(vars["_text"])) {
	sendmsg(source, "_warning_usage_entry_add",
		"Usage: /addentry <text>", ([ ]));
	return 1;
    }
    addEntry(source, vars, data);
    return 1;
}

_request_entry_reply(source, mc, data, vars, b) {
    P3((">> _request_entry_reply(%O, %O, %O, %O, %O)\n", source, mc, data, vars, b))
    unless (canReply(SNICKER)) return 0;
    unless (vars["_parent"] && strlen(vars["_text"])) {
	sendmsg(source, "_warning_usage_entry_reply",
		"Usage: /comment <id> <text>", ([ ]));
	return 1;
    }

    unless (addEntry(source, vars))
	sendmsg(source, "_error_thread_invalid_entry",
		"#[_id]: no such entry", (["_id": vars["_id"]]));

    return 1;
}

_request_entry_edit(source, mc, data, vars, b) {
    P3((">> _request_title(%O, %O, %O, %O, %O)\n", source, mc, data, vars, b))
    unless (canEditOwn(SNICKER)) return 0;
    unless (vars["_id"] && strlen(vars["_id"])) {
	sendmsg(source, "_warning_usage_entry_edit",
		"Usage: /editentry <id> <text>", ([ ]));
	return 1;
    }

    unless (editEntry(source, vars))
	sendmsg(source, "_error_thread_invalid_entry",
		"#[_id]: no such entry", (["_id": vars["_id"]]));

    return 1;
}

_request_entry_del(source, mc, data, vars, b) {
    P3((">> _request_entry_del(%O, %O, %O, %O, %O)\n", source, mc, data, vars, b))
    unless (canPost(SNICKER)) return 0;
    unless (vars["_id"]) {
	sendmsg(source, "_warning_usage_entry_del",
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

_request_set_addaction(source, mc, data, vars, b) {
    return _request_set_default_text(source, mc, data, vars, b);
}

_request_set_editaction(source, mc, data, vars, b) {
    return _request_set_default_text(source, mc, data, vars, b);
}

_request_set_showcomments(source, mc, data, vars, b) {
    return _request_set_default_bool(source, mc, data, vars, b);
}

_request_set_showform(source, mc, data, vars, b) {
    return _request_set_default_bool(source, mc, data, vars, b);
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

varargs array(mixed) htmlComments(array(mixed) entries, int submit, int level, int n) {
    array(mixed) ret;
    mapping entry, vars;
    string ht = "", style;
    foreach(entry : entries) {
	vars = entry[LOG_VARS];
	ht = htquote(vars["_text"]);
        ht = replace(ht, "\n", "<br/>\n");

	style = level ? "style='padding-left: "+ level +"em'" : "";
	ht += "<div class='comment' title='"+ isotime(ctime(vars["_time_place"]), 1) +"' "+ style +"><span class='comment-author'>"+ vars["_nick"] +"</span>: <span class='comment-text'>" + (submit ? "<a class='comment-form-toggle' onclick='toggleForm(this.parentNode, "+ vars["_id"] +")'>&raquo;</a>" : "")+ ht +"</span></div>\n";
	n++;
	if (sizeof(entry) >= LOG_CHILDREN + 1) {
	    ret = htmlComments(entry[LOG_CHILDREN], submit, level + 1, n);
	    ht += ret[0];
	    n = ret[1];
	}
    }
    return ({ ht, n });
}

varargs string htmlEntries(array(mixed) entries, int submit, int show_comments, int nojs, string chan, string submit_target, string url_prefix) {
    P3((">> threads:htmlentries(%O, %O, %O, %O, %O, %O)\n", entries, submit, nojs, chan, submit_target, url_prefix))
    string text, ht = "";
    string id_prefix = chan ? chan +"-" : "";
    unless (url_prefix) url_prefix = "";
    unless (nojs) ht +=
	"<script type='text/javascript'>\n"
	  "function $(id) { return document.getElementById(id); }"
	  "function toggle(e) { if (!e) return 0; if (typeof e == 'string') e = $(e); e.className = e.className.match('hidden') ? e.className.replace(/ *hidden/, '') : e.className + ' hidden'; return 1; }\n"
	  "function toggleForm(e, id) { toggle(e.nextSibling) || e.parentNode.appendChild($('entry-form')) && ($('entry-form').className=''); $('form-parent').value = id }"
	"</script>\n";

    if (submit) ht +=
	"<form id='entry-form' class='hidden' action='"+ url_prefix +"'"+
	(0 && submit_target
	 //FIXME: cmd is executed twice, because after a set-cookie it's parsed again
	 ? "onsubmit=\"cmd('comment '+ $('form-parent').value +' '+ this.previousSibling.value, '"+ submit_target +"')\""
	 : "method='post'") +
	">"
	  "<input type='hidden' name='request' value='post' />"
	  "<input type='hidden' id='form-parent' name='_parent' value='' />"
	  "<textarea name='_text' autocomplete='off'></textarea>"
	  "<input type='submit' value='Send'>"
	"</form>";

    mapping entry, vars;
    foreach (entry : entries) {
	P3((">>> entry: %O\n", entry))
	vars = entry[LOG_VARS];
        text = replace(htquote(vars["_text"]), "\n", "<br/>\n");

	array(mixed) comments = ({ "", 0 });
	if (sizeof(entry) >= LOG_CHILDREN + 1) comments = htmlComments(entry[LOG_CHILDREN], submit);

	ht +=
	    "<div class='entry'>\n"
	      "<div class='header'>\n"
	        "<a href=\""+ url_prefix +"?id="+ vars["_id"] +"\">"
	          "<span class='id'>#"+ vars["_id"] +"</span> - \n"
		  "<span class='author'>"+ vars["_nick"] +"</span>\n"
	          + (vars["_title"] && strlen(vars["_title"]) ? " - " : "") +
	          "<span class='title'>"+ htquote(vars["_title"] || "") +"</span>\n"
	        "</a>"
	      "</div>\n"
	      "<div class='body'>\n"
		"<div class='text'>"+ text +"</div>\n"+
	        (show_comments ?
		 "<div id='comments-"+ id_prefix + vars["_id"] +"' class='comments'>"+ comments[0] +
	         (submit ? "<a onclick=\"toggleForm(this, "+ vars["_id"] +")\">&raquo; reply</a>" : "") +
	         "</div>\n" : "") +
	      "</div>\n"
	      "<div class='footer'>\n"
		"<span class='date'>"+ isotime(ctime(vars["_time_place"]), 1) +"</span>\n"
		"<span class='comments-link'>"
		  "<a " +
	          (show_comments
		   ? "onclick=\"toggle('comments-"+ id_prefix + vars["_id"] +"')\""
		   : "href='"+ url_prefix +"?id="+ vars["_id"] +"''") +
		   ">"+ comments[1] +" comments</a>"
		"</span>\n"
	      "</div>\n"
	    "</div>\n";
    }
    P3((">>> ht: %O\n", ht))
    return "<div class='threads'>"+ ht +"</div>";
}

string htmlForm(int link, int title) {
    if (link)
	return
	    "<div class='threads'><a class='add-entry' href='?request=form'>Add entry</a></div>\n";
    return
      "<div class='threads'>"
	"<div class='entry entry-form'>\n"
	  "<div class='header'>Add entry</div>\n"
	  "<div class='body'>\n"
	    "<form method='post'>\n"
	      "<input type='hidden' name='request' value='post' />\n" +
	      (title ?
	       "Title:<br><input type='text' name='_title'><br>\n"
	       "Text:<br/>" : "") +
	      "<textarea name='_text'></textarea>\n"
	      "<input type='submit' value='Send' />\n"
	    "</form>\n"
          "</div>\n"
	  "<div class='footer'>\n"
	  "</div>\n"
	"</div>\n"
      "</div>\n";
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
    string ht;
    foreach (entry : entries) {
	vars = entry[LOG_VARS];
	ht = htquote(vars["_text"]);
	// does RSS define <br/> for linebreaks?
        //ht = replace(ht, "\n", "<br/>\n");
	rss +=
	    "\n<item>\n"
	      "\t<title>"+ (vars["_title"] || "no title") +"</title>\n"
	      "\t<link>http://"+ HTTP_OR_HTTPS_URL +"/"+ pathName() +  "?id="+ vars["_id"] +"</link>\n"
	      "\t<description>"+ ht +"</description>\n"
	      "\t<dc:date>"+ isotime(ctime(vars["_time_place"]), 1) +"</dc:date>\n"
	      "\t<dc:creator>"+ vars["_nick"] +"</dc:creator>\n"
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
	// should probably be htquoted too
	js += "new Entry("+ vars["_id"] +","
		"\""+ vars["_title"] +"\","
		"\""+ vars["_nick"] +"\","
		+ isotime(ctime(vars["_time_place"]), 1) +","
		"\""+ vars["_text"] +"\"),\n";
	}

    return js[..<3] +");";
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

varargs string htMain(int limit, int offset, int submit, string chan) {
    return htmlEntries(entries(limit, offset, 1), submit, v("showcomments"), 0, chan);
}

varargs void displayMain(int limit, int offset, int submit) {
    write(htMain(limit, offset, submit));
}

void displayForm(int link, int title) {
    write(htmlForm(link, title));
}

string htEntry(int id, int submit) {
    return htmlEntries(entry(id), submit, 1);
}

void displayEntry(int id, int submit) {
    write(htEntry(id, submit) || "No such entry.");
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
void displayHeader(string class) {
    w("_HTML_head_threads",
      "<html><head>"
        "<link rel='stylesheet' type='text/css' href='"+ STYLESHEET +"'>"
        "<title>"+ MYNICK +"</title>"
      "</head>\n"+
      "<body class='threads "+ class +"'>\n"
      "<h1><a href='/"+ pathName() +"'>"+ MYNICK +"</h1>\n");
}
void displayFooter() {
    w("_HTML_tail_threads", "</body></html>");
}

static object checkToken(mapping query) {
	string nick;
	object user;
	if (nick = query["user"]) user = find_person(nick);
	if (user && user->validToken(query["token"])) return user;
	return 0;
}

htget(prot, query, headers, qs, data) {
    mapping entrymap;
    mixed target;
    string nick;
    object user;
    int a;
    int limit = to_int(query["limit"]) || DEFAULT_BACKLOG;
    int offset = to_int(query["offset"]);
    int authed = checkToken(query) ? 1 : 0;
    unless (isPublic() || authed) {
	write("<h1>404</h1>");
	return 1;
    }

    string webact = PLACE_PATH + MYLOWERNICK;
    sTextPath(query["layout"], query["lang"], "html");

    // Kommentare anzeigen
    if (query["id"]) {
	htok(prot);
	// kommentare + urspruengliche Nachricht anzeigen
	displayHeader("entry");
	displayEntry(to_int(query["id"]), authed);
	//logView(a < 24 ? a : 12, "html", 15);
	displayFooter();
	return 1;
    }

    if (query["request"] == "post") {
	htok(prot);

	// TODO: remote user auth
	unless (user = checkToken(query)) {
	    write("Not authenticated!\n");
	    return 1;
	}
	unless (canPost(query["user"])) {
	    write("You are not owner or aide of this place.\n");
	    return 1;
	}

	object vars = ([]);
	// add query params beginning with _ as vars
	foreach (string key, string value : query)
	    if (abbrev("_", key)) vars[key] = value;

	vars["_nick"] = user->qName();
	addEntry(user, vars);
    }
    // neuen Eintrag verfassen
    if (query["request"] == "form") {
	htok(prot);
	displayHeader("entry-add");
	displayForm(0, 1);
	displayFooter();
	return 1;
    }

    ::htget(prot, query, headers, qs, data, 1);	// no processing, just info

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
	// check If-Modified-Since header
	htok3(prot, "text/xml", "");
	rssExport(limit, offset);
    } else {
	// normaler Export
	//P2(("all entries: %O\n", _thread))
	htok3(prot, "text/html", "Cache-Control: no-cache\n");
	displayHeader("entries");
	if ((user = checkToken(query)) && canPost(user->qName()))
	    displayForm(!v("showform"));
	// display the blog
	displayMain(limit, offset, checkToken(query) ? 1 : 0);
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
