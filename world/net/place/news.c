// $Id: news.c,v 1.99 2008/02/09 15:15:41 lynx Exp $ // vim:syntax=lpc
/*
 *	a place that polls an RSS / RDF xml page that 
 *	contains "news" from sites like slashdot, heise.de, etc 
 *	for a compilation of available RSS feeds take a look at
 *	http://www.webreference.com/services/news/
 *
 *	technical information on RSS: 
 *	http://docs.kde.org/en/3.1/kdenetwork/knewsticker/introduction.html#rssfiles
 *
 *	TODO: check version tag and parse according to that	
 *
 * Examples for qNewsfeed()
 *	"http://www.heise.de/newsticker/heise.rdf"
 *	"http://slashdot.org/slashdot.rss"
 *	
 * Cool feeds: 
 * 	"http://www.webreference.com/services/news/index.html"
 */

#include <net.h>
#include <status.h>
#include <storage.h>
#include <xml.h>
#include <text.h>	// NO_INHERIT, but virtual takes care of that

inherit NET_PATH "place/master";
inherit NET_PATH "xml/parse";

protected mapping _news;		// ein wenig mehr ordnung im .o file
protected mapping known;		// links of known news
volatile object feed;
volatile string prefix, modificationtime2;

handleNews(buffer);

// always show amount, never names.
makeMembers() { return ::makeMembers(1); }

publish(link, headline, channel) {
#if 1
	// this is specific to shortnews.de who do a little privacy
	// intrusion by tagging their rss feeds with personalized "u_id"
	int uid = strstr(link, "&u_id");
	if (uid != -1) link = link[0 .. uid-1];
#endif
    PT(("%O publish %O\n", ME, link))
	// this could move out into a NEWS_PUBLISH() for torrents
	if (trail(".torrent", link))
	    castmsg(ME, "_notice_offered_file_torrent",
		"([_nick_place]) [_file_title] [_uniform_torrent]",
		([ "_file_title": headline,
		   "_uniform_torrent": link,
		   "_channel_title": channel ])
	    );
	else castmsg(ME, "_notice_headline_news", 
		"([_nick_place]News) [_headline] [_page_news]",
		([ "_headline": headline,
		   "_page_news": link,
		   "_channel": channel ])
	);
}

connect() {
    unless (feed) {
	feed = qNewsfeed() -> load();
	feed -> sAgent(SERVER_VERSION " builtin RSS to PSYC gateway - We'd prefer you to push your news in realtime instead of having us poll for it! See http://about.psyc.eu/Newscasting about that.");
	feed->content(#'handleNews, 1);
    } else {
	feed->refetch(#'handleNews);
    }
}

handleNews(buffer) {
	// called when connection is closed
	// put your logic here
	int i;
	mapping new, diff, items;
	string href;
	XMLNode item;
	prefix = "";
	// D2(D("strlen(buffer) = " + strlen(buffer) + "\n");)

	if (intp(_news = xmlparse(buffer))) {
	    P0(("%O could not parse %O\n", ME, qNewsfeed()))
	    return;
	}

	unless(v("channel")) {
	    XMLNode c;

	    // net/place/news.c:67: (s)printf(): BUFF_SIZE overflowed...
	    // program: net/place/news.c, object: place/gmaps line 67
	    // _news is too big to debug this way!  :(
	    P4(("%O got _news %O\n", ME, _news))
	    if (c = _news["/channel"]) {
		vSet("channel", ([ "title" : c["/title"][Cdata],
			    "link" : c["/link"][Cdata],
			    "description" : c["/description"][Cdata] ]));
	    }
	    else {
		P0(("%O cannot find channel data in %O\n", ME, qNewsfeed()))
	        return;
	    }
	}
	// compare and see if there is new data
	new = ([ ]);
	items = ([ ]);
	unless(known) known = ([ ]);
	P2(("known: %O\n", known))
	foreach(item : (_news["/item"] || _news["/channel"]["/item"])) {
	    // breaks here sometimes, but then the RSS must be stupid
	    href = item["/link"][Cdata];
	    new[href] = 1;
	    items[href] = item;
	}
	foreach(href : new) {
	    unless(known[href]) {
		string l = items[href]["/link"][Cdata];
		string t = items[href]["/title"][Cdata];

		if (strlen(l) > 5 && stringp(t))
		    publish(l, replace(t, "\n", " "), v("channel")["title"]);
		else {
			PT(("%O encountered funny link %O or title %O\n",
			    ME, l, t))
		}
	    }
	}
	known = new;
	save(); // to be discussed if we really need this
}

showStatus(verbosity, al, person, mc, data, vars) {
	if (mappingp(_news) && verbosity & VERBOSITY_NEWSFEED && _news[prefix + "_title"])
	    sendmsg(person, "_status_place_description_news_rss",
	    "RSS Newsfeed for [_news_channel_title]: \"[_news_channel_description]\"\n"
	    "Available from [_link_news_rss]."
	    " Last check: [_time_fetch]. Last change: [_time_modification]", ([
		       "_news_channel_title" : v("channel")["_title"],
		 "_news_channel_description" : v("channel")["_description"],
			"_time_modification" : feed->qHeader("modificationtime") || modificationtime2,
			       "_time_fetch" : feed->qHeader("_fetchtime"),
			    "_link_news_rss" : qNewsfeed()
	]) );
	return ::showStatus(verbosity, al, person, mc, data, vars);
}

// overrides mayLog in storic.c
mayLog(mc) {
	return abbrev("_notice_headline", mc) ||
		mc == "_notice_offered_file_torrent";
}

//#if 0
cmd(a, args, b, source, vars) {
    switch (a) {
	case "news":
		connect();
		return 1;
    }

    return ::cmd(a, args, b, source, vars);
}
//#endif

msg(source, mc, data, vars) {
	// receive pings from blogs etc.
	if (abbrev("_notice_update", mc)) {
//		mixed *u;
//
//		unless(vars["_location_feed"]) return;
//		u = parse_uniform(vars["_location_feed"]);
		if(qAllowExternal(source, mc, vars))
			connect();
		// would be better if the blog delivered the whole story
		// then we wouldn't even need a "news" room for this
		return;
	}
	return ::msg(source, mc, data, vars);
}

// new: receive and process http://www.xmlrpc.com/weblogsCom pings
//
// the bad news about pings: they contain 0 information other than
// some unspecified event happened.
// the good news about pings: they aren't worthy of parsing them
htpost(prot, query, headers, qs, data, noprocess) {
	if (headers["content-type"] == "text/xml") {
		string s, t;

		// parses both unworthy trashy formats:
		if (stringp(data)) sscanf(data, "%s>http://%s</%s", t, s, t);
		htok(prot);
		if (stringp(data) && strstr(data, "SOAP-ENV") != -1) {
		    //sscanf(data, "%s>http://%s</weblogurl>%s", t, s, t);
		    write(T("_XML_RPC_weblog_pong_SOAP", 0));
		} else {
		    //sscanf(data, "%s<value>http://%s</value>%s", t, s, t);
		    write(T("_XML_RPC_weblog_pong", 0));
		}
		// now check the credentials and do the update
		unless (same_host(query_ip_number(), qNewsfeed()->qHost())) {
		    P0(("%O got unsolicited XML from %O promoting %O\n",
			ME, query_ip_name(), s || data))
		    return 1;
		}
		P0(("%O got XML from %O promoting %O\n",
		    ME, query_ip_name(), s || data))
		connect();
		// not disabling reset yet. TODO
		return 1;
	}
	P0(("%O got htpost from %O containing %O\n", ME, query_ip_name(), data))
	//return ::htpost(prot, query, headers, qs, data, noprocess);
}

