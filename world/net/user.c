// $Id: user.c,v 1.547 2008/12/22 11:11:36 lynx Exp $ // vim:syntax=lpc
//
// common subclass for PSYC clients, whatever the user interface

// local debug messages - turn them on by using psyclpc -DDuser=<level>
#ifdef Duser
# undef DEBUG
# define DEBUG Duser
#endif

#include <net.h>
#include <person.h>
#include <psyc.h>
#include <url.h>

inherit NET_PATH "person";
inherit NET_PATH "common";
inherit NET_PATH "sockets";

#include <text.h>
//virtual inherit NET_PATH "output";

#define NO_INHERIT
#include <user.h>
#undef NO_INHERIT

volatile mixed query;
volatile mapping tags;
volatile int showEcho;
volatile mixed beQuiet;

// jabber needs no private echo but plenty of public echo..
// net/jabber takes care of that, we just give it echo here.
// in psyc echoes are essential, so they are on.
// on irc it's better to turn them on, but you first need a
// client like irssi. for telnets and webchats you usually
// want echoes, but you can turn them off
#define ASSIGN_SHOWECHO(setting, scheme) switch(scheme) {	\
	case "psyc":						\
	case "jabber":						\
		showEcho = 1;					\
		break;						\
	case "irc":						\
		showEcho = setting == "on";			\
		break;						\
	default:						\
		showEcho = setting != "off";			\
	}

#include "usercmd.i"

sName(a) {
	// currently this sName gets called more than once
	//ASSERT("user:sName called again", !MYNICK, MYNICK)
	// i don't know if this should get cleaned up  TODO
	unless (places) {
	    places = ([ ]);
	    tags = ([ ]); // this doesn't need an extra if, does it?
	    cmdchar = '/';
	}
	return ::sName(a);
}

qHasCurrentPlace() {
	return 1;
}

qResource() {
    return v("scheme");
}

#ifdef HTTP_PATH
makeToken() {
	string token = sprintf("%x", random(time() ^ 98987));
	P2(("%O's new %O and old %O token\n", ME, token, v("token")))
	vSet("token", token);
	return token;
}                                       

validToken(token) {
	P2(("token %O == given %O?\n", v("token"), token))
	if (is_localhost(query_ip_number())) return 1;
	if (v("token") && token != v("token")) return 0;
	return 1;
}

volatile mapping descvars;

#ifndef DEFAULT_FILE_STYLE_EXAMINE
//# define DEFAULT_FILE_STYLE_EXAMINE "http://www.psyced.org/examine.css"
# define DEFAULT_FILE_STYLE_EXAMINE "/static/examine.css"
#endif
htDescription(anonymous, query, headers, qs, variant, vars) {
	string k, t, page, foto, buttons, profile, nick, type;
#ifndef HTTP_PATH
	int doTalk = 1;
#else
	int doTalk = 0;

	P3(("htDescription for %O showing %O or %O\n", qs, vars, descvars))
	if (query) sTextPath(query["layout"] || v("layout"),
			     query["lang"] || v("language"), "html");
	else sTextPath(v("layout"), v("language"), "html");
#endif
	unless (vars) {
		unless (descvars) return 0;
		// vars = descvars; descvars = 0;
		vars = copy(descvars); // will be modded later
		// right now we do not htquote the descvars.
		// seen how much creativity not quoting html can
		// produce we should probably at least allow friends
		// to send us html code in profiles.. whatcha think?
		// then again, maybe only certain fields should be
		// htmlized, otherwise psyc profiles end up heavily
		// polluted by html code.. extreme solution: let
		// every var have an _HTML trailed variant?!?
		// or go the other way: accept html code in psyc
		// profiles, but remove it when outputting to plain
		// devices? that's a smart thing to say. (buy the record!)
	}
	// store this before we delete some vars
	//doTalk = vars["_language"] || vars["_color"];

	// adventurous guess.. what if a user provides this? hmm
	// well it looks funny, that's all
	if (stringp(nick = vars["_nick_place"])) {
		type = "_place";
		// used by exa() in _PAGES_start_description
		vars["_nick"] = nick;
	} else {
		type = "_person";
		nick = vars["_nick"];
	}
	// we could also try _identification here, or even apply inheritance
	if (vars["_source"]) profile = ppl[ vars["_source"] ];
	if (!profile && stringp(nick)) profile = ppl[ lower_case(nick) ];

	page = listDescription(vars, 0, variant
		 ? "<a href=\"javascript:exa('%s')\">%s</a>"
		 : "<a href=\"%s\">%s</a>");	// direct psyc: links!
	if (anonymous) {
		// anonymous profile inspection from the web
		// we are rendering ourselves for somebody else
		// not the other way around
	} else if (type == "_place") {
		// new meaning of _context used here.. ay ay ay
		// we should only show one of these
		buttons = T("_HTML_examine_context_enter", "")
			+ T("_HTML_examine_context_leave", "");
	} else {
		string display = "_normal";

		switch (profile && profile[PPL_DISPLAY]) {
		case PPL_DISPLAY_NONE:
			// this, of course, needs to happen before we use
			// the "doTalk" variable
			doTalk = 0;
			display = "_none";
			break;
		case PPL_DISPLAY_SMALL:
			display = "_reduce";
			break;
		case PPL_DISPLAY_BIG:
			display = "_magnify";
			break;
		}
		vars["_display_none"] = "";
		vars["_display_reduce"] = "";
		vars["_display_magnify"] = "";
		vars["_display_normal"] = "";
		vars["_display"+display] = "selected";

		if (profile && profile[PPL_NOTIFY] >= PPL_NOTIFY_PENDING) {
		    buttons = T("_HTML_examine_friend_delete", "");
		    if (doTalk) buttons += T("_HTML_examine_talk", "");
		} else {
		    buttons = T("_HTML_examine_friend_new", "");
		    if (doTalk && !FILTERED(vars["_source"]))
			buttons += T("_HTML_examine_talk", "");
		}
#ifdef TELEPHONY_SECRET
		buttons += T("_HTML_examine_button_call", "");
#endif
		buttons += T("_HTML_examine_display", "");
	}

#ifndef _flag_enable_profile_table
	page = T("_list_description_on", "")
		+ page
		+ T("_list_description_off", "");
#else
	page = T("_list_description_on_table", "")
		+ page
		+ T("_list_description_off_table", "");
#endif
	if (buttons) page = T("_HTML_examine_buttons_start", "")
	       	+ buttons + T("_HTML_examine_buttons_end", "") + page;
	if (t = vars["_uniform_photo"] || vars["_uniform_photo_small"]) {
		foto = "<img class=\"Peup\" src=\""+ t +"\">";
		if (vars["_page_photo"])
		    foto = "<a target=\"_blank\" href=\""
			    + vars["_page_photo"] +"\">"+ foto +"</a>";
		// foto = "<span class=\"Peup\">"+ foto +"</span>\n";
	} else foto = "";
	if (variant) page = "<link rel=\"stylesheet\" href=\""+
	        (vars["_uniform_style"] || DEFAULT_FILE_STYLE_EXAMINE)
		+"\">\n"+ T("_PAGES_start_description"+ variant, "") + page
	       	+ T("_PAGES_end_description"+ variant, "");
	else {
		// we send only the "body" of the page to psyc clients.
		// psyczilla doesn't really need a form,
		// but the formatting goes wrong if there isn't one
		page = foto +"\n<form class='Pef'>\n"+ page +"</form>";
	}
	P4(("EXAMINE psyctext %O with %O\n", page, vars))
// <form target=cout_" CHATNAME " name=f" HTFORMOPTS "
#ifdef HTTP_PATH
	sTextPath(v("layout"), v("language"), v("scheme"));
#endif

// <input type=hidden name=user value=\""+qName()+"\">\n\
// <input type=hidden name=token value=\""+v("token")+"\">\n\
// <input type=hidden name=lang value=\""+v("language")+"\">\n\
//
	return psyctext(page, vars + ([
	"_FORM_start"                   : "\
<form class=\"Pef\" name=\"Pef\" action=\"\">\n\
<input type=hidden name=cmd value=\"\">\n",
	"_QUERY_STRING"                 : qs || "",
	"_HTML_photo"			: foto,
	"_nick_me"			: MYNICK,
	"_FORM_end"                     : "</form>\n",
		])
	);
}
#endif

// leavePlace: extracted from _echo_place_leave handling so we can
// do it without depending on the place's answer
//
// "impatient leave" is an apparently necessary strategy:
// _ when servers crash, the UNI would not reissue a join request because
//   it still sees the context in its places. this also helps with remote MUCs.
// _ the /cycle feature of some IRC clients expects no delays between direct
//   subsequent PART and JOIN commands, but if psyced hasn't received leave
//   confirmation yet, it used to simply ignore the JOIN.
//
// both issues could probably also be solved with glue around
// "polite leave", but it's a lot more complicated and maybe it's
// the wrong strategy anyhow.
//
protected leavePlace(where) {
	P2(("%O leavePlace's %O\n", ME, where))
	if (place == where) {
		// current place.. psyced specific thing
		place = 0;
//		vDel("place");	// wrong! dann kommen die
//		 		   leute nich mehr in ihren
//		 		   letzten raum rein!
	}
	// leave the context slave, so we immediately stop getting
	// the context's traffic even before it knows about us leaving
	deregister_context(ME, where);
	// after this step any castmsg from 'where' will be considered spam.
	// luckily we're not in the context slave anymore.
	m_delete(places, where);
}

/** PSYC-conformant message receiving function
 **
 ** after doing basic PSYC, ppl and lastlog processing in
 ** person:msg() we're here to do the output..
 */
msg(source, mc, data, mapping vars, showingLog) {
	string nick, nick2, family;
	mixed pal, t, variant;
	int glyph;

	P3(("%O user:msg(%O,%O,%O,%O)\n", ME, source, mc, data, vars))
	// context checking since ip paranoia is applied to context
	// not source whenever context is available..
	//
	// this should probably also apply for objectp sources since
	// one day even they might be spoofed by a smart context..
	// but right now sources are not resolved to objects
	//
	// also.. this belongs into person.c if it werent for the
	// rplaces ... hmmm!
	// Note: with context mcast we dont need this any more as we
	// will only get valid contexts
	if (vars["_context"]) {
	    if (showingLog) {
		// sometimes lastlog comes with a _context which i
		// need to rename here as an indicator for the final
		// renderer that this is a retransmission
		vars["_INTERNAL_context"] = vars["_context"];
		m_delete(vars, "_context");
	//	unless (source) source = vars["_source_relay"];
#if DEBUG > 0
	    } else if (!objectp(vars["_context"]) &&
		!places[vars["_context"]] &&
	       	// !psyc_object(vars["_context"]) &&
		!abbrev("_notice_place", mc)) {	// just _notice_place? TODO
		    string ctx, chan;

//		    monitor_report("_warning_abuse_invalid_context",
//			S("%O: Invalid context %O in %O from %O (%O)",
//			ME, vars["_context"], mc, source, data));
		    if ((sscanf(vars["_context"], "%s#%s", ctx, chan) == 2)
		       	&& places[ctx]) { 
			    P1(("got valid context %O with channel %O\n",
			       	ctx, chan))
		    } else {
     P1(("Invalid context: %O says %s(%O) with %O ... stack %O ... places %O\n",
				source, to_string(mc), data, vars,
			       	caller_stack(1), places))
			    return 0;
		    }
#endif
	    }
	    unless (source) source = vars["_source_relay"] || vars["_context"];
	}
	// before? after?
	unless (source) source = vars["_INTERNAL_source"];
	// call the psyc handler in person.c
	// it returns some extra info concerning display
	t = source; // keep the real source before entity.c puts the UNI into it
	// and here we call person::msg() without ensuring that local sources
	// are objectp. this may cause problems. shouldn't we have all the
	// location resolution happen BEFORE we let person act on things?
	// this looks like a big TODO here.
	
	// <fippo>: if _INTERNAL_target_resource is set, calling person::msg
	// 	is INAPPROPRIATE. This is the current terminology of 
	// 	net/jabber, but we will have to do something similar in net/psyc
	//
	// 	Rationale: if this variable is set, we dont want the uni to 
	// 	handle this.
#ifdef PRO_PATH
	unless (variant = ::msg(&source, &mc, &data, &vars, showingLog))
	    return 0;
	if (member(vars, "_authenticated")) variant = "_important";
#else
	unless (::msg(&source, &mc, &data, &vars, showingLog)) {
		P3(("%O ::msg no display for %O's %O\n", ME, source, mc))
		return 0;
	}
	variant = "";
#endif
	P4(("after p:msg(%O,%O,%O,%O) -> %O\n", source,mc,data,vars, variant))
//	D2(unless (showingLog) D(S("user:msg(%O,%O,%O..) -> %O\n",
//	       source, mc, data, variant));)
	nick = vars["_nick"];

#ifdef ALIASES
# ifdef BRAIN
	unless (mappingp(v("aliases"))) vSet("aliases", ([]));
# endif
#endif
	if (stringp(t)
#ifdef SLAVE
	    // we trust user if he got relayed from local room object
	    &&! objectp(vars["_context"])
#endif
#ifndef UNSAFE_LASTLOG
	    // evil scary rewrite to support changed lastlog behaviour ,)
	    // TODO:: watch for _source_relay||(source resp. t)
	    &&! (stringp(source)
		 && abbrev(SERVER_UNIFORM +"~", source))
#endif
	  ) {
		if (data && index(data, '\n') != -1)
			data = replace(data, "\n", " ");
		// this used to trigger a systemwide _request_authentication
		//t = lookup_identification(source, vars["_source_identification"]);
		// now uni::msg does these kind of things
		if (t != source) {
		    if (objectp(source)) {
#ifdef ALIASES
			string t2;

			if (t2 = raliases[nick = source->qNameLower()]) {
			    nick = t2;
			} else if (aliases[lower_case(nick)]) {
			    nick = t;
			}
#else
			nick = source->qName();
#endif
		    } else {
#ifdef ALIASES
			nick = raliases[source] || source;
#else
			nick = source;
#endif
		    }

#ifndef TRUST_PSYC_HACK
# ifdef ALIASES
		} else if (raliases[source]) {
			nick = raliases[source];
# endif
		} else {
			// short term solution (nick manager to follow)
			//if (nick) nick = "("+ source + ") "+ nick;
			// middle term solution
			if (nick && nick != source) {
				// this ensures we always escape the
				// original nick, not an already escaped one
				// other solution would probably be not
				// to send the same vars to all users..
				// eeeeh.. probably the better solution
				unless (vars["_nick_verbatim"])
				    vars["_nick_verbatim"] = nick;
				vars["_INTERNAL_nick_plain"] = nick;
				// sometimes we get here with source being
				// a context - that of course looks very
				// stupid - how does it happen?  TODO
				// beQuiet is -1 exactly with those schemes
				// where this nick patching habit is completely
				// inappropriate. we may want to change the
				// var name.
				if (beQuiet != -1)
				    nick = psyctext(
				       T("_MISC_identification_remote",
				       "«[_source]» [_nick]"), ([
					"_source":source,
					"_nick":vars["_nick_verbatim"] ]));
			}
			else nick = stringp(source) ? source : to_string(source);
# if 0 //DEBUG > 1
			// checking for validity of messages should
			// happen at psyc-parsing level.. maybe? YEEEES!!
			// and.. vars["_context"] is zero when showing log!
			// member might help.
			if (abbrev("_message", mc)) {
				D("patching remote mc\n");
				mc = member(vars, "_context") ?
				     "_message_public" : "_message_private";
			}
			// this just wont work.. it even breaks
			// remote _message_echo_private's
# endif
#endif
		}
		nick2 = nick;
		// belongs into person.c?
		if (mc == "_message") mc = vars["_context"] ?
					"_message_public" : "_message_private";
		P3(("%O got msg(%O,%O,%O,%O)\n", ME,source,mc,data,vars))
	}
	else if (nick && nick != MYNICK) {
#ifdef ALIASES
	    string tn;
	    // ein kleines bisschen hilflosigkeit (&&! showingLog)
	    //
	    // why are summoned_persons incarnated as users, not net/person??
	    // any reason? don't want to think over it, but it kinda sucks for
	    // aliases.. says tobij. lynX adds: well in fact with some clean
	    // up summoned persons could be reverted to NOT be users again.
	    // so please go ahead! TODO
	    if (raliases[tn = lower_case(nick)] &&! showingLog) {
		nick2 = nick = raliases[tn];
	    } else if (aliases[tn] &&! showingLog) {
		nick2 = nick = UNIFORM(source);
	    } else
#endif
	    nick2 = vars["_nick_stylish"] || vars["_nick_local"] || nick;
	} else nick2 = nick = MYNICK;
	P3(("q/n/n2: %O,%O,%O\n", MYNICK,nick,nick2))

	t = vars && vars["_context"] || source;
#ifdef SANDBOX
	unless (objectp(t) && stringp(geteuid(t)) && geteuid(t)[0] == '/') {
#else // duh. we cannot combine this, ldmud complains, might be a preprocessor
      // bug or just my very own stupidity.
	unless (objectp(t)) {
#endif
	    string rnick;
	    array(mixed) u;
#ifdef SANDBOX

	    if (objectp(t)) {
		rnick = object_name(t);
		rnick = rnick[rmember(rnick, '/')+1..];

		if (!vars["_nick_place"]
		    || stricmp(rnick, vars["_nick_place"])) {
		    // TODO: we shouldn't raise_error here, as we shouldn't
		    // allow unprivileged places to annoy users.
		    // what to do? enforce rnick? drop msg?
		    raise_error(sprintf("INVALID _nick_place in msg by %O(%O)\n",
					t, rnick));
		}
	    } else
#endif
	    if (stringp(t) && (rnick = (u = parse_uniform(t, 1))[UResource])
		       && strlen(rnick)
		       && rnick[0] == '@') {
		string hnick = rnick[1..];

		rnick = vars["_nick_place"];

		if (!rnick || stricmp(hnick, rnick)) {
		    rnick = hnick;
		}

		vars["_nick_place"] = u[URoot] + "/@" + rnick;
	    }
	}

	// verrry similar code in psyctext()
	// didn't we want to eliminate these?
	// also we have the textdb with inheritance, so we could deal with
	// these things in a proper way!
	t = vars["_time_place"];
	if (!t && showingLog) {
		// _time_log is just for psyc clients: since we had to
		// delete _time_INTERNAL from the psyc out, we need a
		// variable for the case when this _really_ is a /log output
		unless (t = vars["_time_log"]) {
			if (t = vars["_time_INTERNAL"]) {
				//m_delete(vars, "_time_INTERNAL");
				vars["_time_log"] = t;
			} else {
				// sort-of impossible condition
				P0(("%O got showingLog w/out _time in %O\n",
				    ME, vars))
			}
		}
		P3(("%O time %O data %O from %O\n", ME, t, data, vars))
	}
	PSYC_TRY(mc) {
case "_jabber_iq_error": // DONT reply
		P2(("%O got %O", ME, mc))
		break;
	    // TODO: add the xmlns to unsupported method format
	    // 		technisch gesehen sollten wir auch die resource
	    // 		wieder zurueckgeben an die das gesendet wurde...
	    // 		aber kriegen wir das hier?
case "_jabber_iq_get":
case "_jabber_iq_set":
		sendmsg(source, "_error_unsupported_method", 0,
			([ "_tag_reply" : vars["_tag"] ]));
		break;
case "_jabber":
		P1(("%O got %O", ME, mc))
		break;
case "_message_private_question":
		variant = "_ask" + variant;
		m_delete(vars, "_action");
		// fall thru
case "_message": 	// sollte schon vorher abgefangen worden sein
		mc = "_message_private";
		// fall thru
case "_message_private":
		// question recognition takes place on sender side
		// for public talk, so it should also for private talk..
#if 0
# ifdef BRAIN
		P2(( "this shouldn't happen - the brain aliases bug\n" ))
		unless (v("aliases")) vSet("aliases", ([]));
		// for some reason v("aliases") can be undefined here
		// but it never happens on my test server
# endif
		if (objectp(source)) {
		    if (v("aliases")[nick]) { // nick collision detected!
			nick = psyc_name(source);
		    } else {
			nick = raliases[lower_case(nick)] || nick2;
			// patchen in den vars.. vorsicht!
			vars["_nick_long"] = vars["_nick"];
		    }
		} else if (stringp(source)) {
		    nick = raliases[source] || nick2;
		} else {
		    nick = nick2;
		}
#else
		nick = nick2;
#endif
#ifdef QUESTION_RECOGNITION_ON_RECEPTION
		if (variant == "" && data && index(data, '?') != -1) {
			// mc += "_ask";
			variant = "_ask";
			m_delete(vars, "_action");
			break;
		}
#endif
		break;
case "_message_echo_private_question":
		variant = "_ask" + variant;
		m_delete(vars, "_action");
		mc = mc[..<10];
case "_message_echo":
case "_message_echo_private":
		unless (showingLog || showEcho) return 1;
#if 0 //def CAN_WE_DARE_THIS // why do we have this block at all!?
     // this is the wrong way and the wrong place to create _source_relay
		pal = vars["_nick_target"];
		if (pal) {
			if (wAction(mc, data, vars, source, variant, nick))
				return 1;
#ifdef QUESTION_RECOGNITION_ON_RECEPTION
			if (variant == "" && index(data, '?') != -1) {
				w(mc+"_ask", data,
				  ([ "_nick_target" : pal,
				        "_time_log" : vars["_time_log"],
				    "_source_relay" : source ]));
				return 1;
			}
#endif
			// should have been "vars + ([" as below for public?
			w(mc, data, ([ "_nick_target" : pal,
				          "_time_log" : vars["_time_log"],
				      "_source_relay" : source ]));
			return 1;
		}
		return 0; // dont walk into _message_public if !pal
#else
		break;
#endif
case "_message_public_question":
#ifndef NO_PUBLIC_QUESTIONS
		variant = "_ask" + variant;
		m_delete(vars, "_action");
#endif
		mc = "_message_public";
		// fall thru
case "_message_public":
		// could theoretically move into the if, but the devil etc.
		//unless (ONLINE) vInc("new");
		if (source != ME) {
		    if (qHasCurrentPlace()) {
			mixed room;
			room = vars["_context"] || vars["_INTERNAL_context"];
			if (room && room != place) {	// && place  ..not!
			    vSet("otherplace", room);
			    // rplaces will normally do the job
			    room = places[room] || 
				    (objectp(room) ? room->qName()
						   : to_string(room));
			    w((data ? "_message_public_other":
				     "_message_public_other_action")+variant,
					 data, vars +
			       ([ "_nick": nick2, "_nick_place": room ]) );
			    return 1;
			}
		    }
		    nick = nick2;
		    break;
		}
		if (vars["_nick_local"] && MYLOWERNICK != lower_case(vars["_nick_local"])) {
			// should we have special templates for this?
			//mc = "_message_echo_public_masquerade";
			nick = vars["_nick_local"];
			// or should we rather make sure that all clients
			// learn to detect echo themselves, since future
			// multicast routing may not give us the possibility
			// to patch methods on the way to the client
			// like we do here:
		} else mc = "_message_echo_public";
		// fall through for echoes coming from rooms
case "_message_echo_public":
		unless (showingLog || showEcho || vars["_INTERNAL_force_echo"])
		    return 1;
		break;
case "_message_announcement":
case "_message_behaviour_warning":
case "_message_behaviour_punishment":
case "_message_behaviour":
		unless (stringp(data)) variant = "_default";
		else variant = "";
		nick = nick2;
//		fmt = nick ? "%s announces: %s\n"
//			: "*** Announcement: %s ***\n";
		break;
case "_request_message_public_question":
case "_request_message":
		mc = "_request_message_public";
case "_request_message_public":
		break;
case "_request_description":
		// if (source == ME) return 1;
		return 1;
case "_status_description":
case "_status_description_person":
case "_status_description_place":
		P4(("x-ret %O\n", vars))
		if (vars["_tag_reply"]) {
		    sscanf(vars["_tag_reply"], "%s %s", variant, t);
		    //PT(("format %O tag %O\n", variant, t))
		    descvars = vars;
#ifdef GAMMA
		    descvars["_source"] = source;
#endif
		    switch (variant) {
		    default:
			// client doesn't want HTML
			if (v("locations")[0]) sendmsg(v("locations")[0],
				mc, data, ([
				        "_tag_reply": t,
				     "_source_relay": source,
				    "_uniform_style": vars["_uniform_style"]
			]) + vars);
		    case "_HTML":
			// client wants HTML
			if (v("locations")[0]) sendmsg(v("locations")[0],
				mc+"_HTML", htDescription(), ([
				        "_type_data": "text/html",
				        "_tag_reply": t,
				     "_source_relay": source,
				    "_uniform_style": vars["_uniform_style"]
			]));
		    case "_surf":
			// we're doing a /surf, so stop here
			return 1;
		    case "0":
		    case 0:
			// fall thru to manual examine
		    }
		}
		// internal transformation of a single msg into a list of msgs
		// hmmm.. well, that's how psyc to client happens here
		mc = "_list"+ mc[7..];
		w(mc+"_on", data, ([		// used by irc whois..
		   "_source_relay": source,
		   "_nick" : vars["_nick"] || vars["_nick_place"],
		   "_name_public" : vars["_name_public"] || "",
		   "_action_motto" : vars["_action_motto"] || "",
		   "_description_motto" : vars["_description_motto"] || ""
		]));
		listDescription(vars, 1);
		w(mc+"_off", 0, ([
		     "_source_relay": source,
		     "_nick" : vars["_nick"] ]));
		return 1;
case "_status_place_members_none_automatic":
		if (beQuiet != -1) {
			PT(("%O skipping %O from %O\n", ME, mc, source))
			return 1;
		}
		break;
case "_status_place_members_automatic":
		if (beQuiet != -1) {
			PT(("%O skipping %O from %O\n", ME, mc, source))
			return 1;
		}
		// fall thru
case "_status_place_members":
#ifdef PARANOID
		//unless (pointerp(vars["_list_members"])) break;
//		t = implode(renderMembers(vars["_list_members"],
//		     vars["_list_members_nicks"], objectp(source)), ", ");
		if (stringp(t = vars["_list_members"])) {   // _tab
		    vars["_list_members"] = ({ t });
		} else unless (pointerp(vars["_list_members"])) break;
		if (stringp(t = vars["_list_members_nicks"])) {
		    vars["_list_members_nicks"] = ({ t });
		} // no else break here by intention, renderMembers() won't be
		  // scared by that.
#endif
		// same code in _echo_place_enter etc.
		// renderMembers is also useful for psyc clients
		vars["_list_members_nicks"] =
		    renderMembers(vars["_list_members"],
		     vars["_list_members_nicks"], objectp(source));
		break;
		// if (v("verbatimuniform") == "on")
		//     t = replace(t, "@", "%");
//		w(mc, "In [_nick_place]: [_list_members_nicks]", ([
//			  "_nick_place" : vars["_nick_place"],
//			  "_list_members_nicks" : t ]) );
		// irc-thang: w(mc + "_end", 0, vars);
//		return 1;
#ifdef TELEPHONY_SECRET
# include <sys/tls.h>

case "_notice_answer_call_click":
case "_notice_answer_call":
		t = vars["_time_expire"];
		if (to_int(t) < time()) {
			P1(("Expired phone call for %O from %O: %O\n",
			    ME, source, t))
			return 1; // call expired
		}
		string t2, t3;
		t2 = "?thats=me&user="+ MYNICK +"&expiry="+ t +"&jack="+
		    (t3 = hmac(TLS_HASH_SHA256, TELEPHONY_SECRET,
			 "I"+ t +":"+ MYNICK));
		P1(("hmac/call %O for %O\n", t3, "I"+t+":"+MYNICK))
		t3 = NET_PATH "http/call"->make_session(MYNICK, to_int(t), t3);
		w("_notice_answer_call_link", 0, ([
                    "_page_call": (
#ifdef __TLS__
                        (tls_available() && HTTPS_URL) ||
#endif
                        HTTP_URL) + NET_PATH +"http/call?thats=me&sid=" +t3,
# ifdef TELEPHONY_SERVER
                    "_uniform_call": TELEPHONY_SERVER + t2,
# endif
		 ]));
		return 1;
#endif
case "_notice_received_email":
case "_notice_email_received":
		if (stringp(vars["_origin"])) vars["_origin"] =
		    decode_embedded_charset(vars["_origin"]);
		if (stringp(vars["_subject"])) vars["_subject"] =
		    decode_embedded_charset(vars["_subject"]);
		break;
case "_notice_presence_here_quiet":
		mc = mc[..<7];	// _quiet is just internal
		// humm.. not since i started using it for CACHE_PRESENCE
		// problem? should i use _request_friend_present instead?
                // fall thru
case "_notice_presence":
case "_status_presence":
		break;
case "_notice_person_absent_netburp":
		if (vars["_context"] != place) return 1;
		break;
#if 1
case "_failure_unsuccessful_delivery":
case "_failure_network_connect_invalid_port":
// is this the right place to do this? i have seen a recursion where
// person.c was never asked for opinion, so i'm putting this into user.c
		// TODO: walk thru entire locations mapping!
		if (vars["_source_relay"] == v("locations")[0]) {
		    P0(("%O got %O, deleting 0 from %O\n", ME,
			mc, v("locations")))
		    m_delete(v("locations"), 0);
                }
		// fall thru - not strictly necessary but adds a feature
#endif
case "_failure_redirect_permanent":
		// we currently have no implementation for permanent changes
case "_failure_redirect_temporary":
case "_failure_redirect":
		// current policy: we trust local senders to do this
		if (objectp(source) && vars["_source_redirect"]) {
			PT(("%O REDIRECTED TO %O\n", ME,
			    vars["_source_redirect"]))
			// how did we get here without a vars["_method_relay"]?
			mc = vars["_method_relay"];
			// intended for _enter and _leave, but right now
			// only places send this, so it's ok to simplify
			if (mc && abbrev("_request", mc)) {
				// but we use placeRequest here, because we
				// need a _tag
				placeRequest(vars["_source_redirect"],
					     mc, 0, 1);
			} // else.. we'll see
#ifdef GAMMA // enough to make the messages visible?
			return 1;
#endif
		}
		break;
case "_echo_place_enter_INTERNAL_CHECK": // only do the check and dont write anything
		pal = 3093;
		// _unicast comes from MUCs and carries a faked _context
		// that's why it has to be a _notice even if it's an _echo
		// maybe psycmatch() could solve this issue instead of abbrev()
case "_echo_place_enter_unicast":
case "_notice_place_enter_unicast_INTERNAL_ECHO":	// MUC join
case "_echo_place_enter_automatic":
case "_echo_place_enter_automatic_subscription":
case "_echo_place_enter_subscribe":	// /sub place differs from auto-
					// joining a subscribed place after
					// logon
		variant = 2933; // do not make it the current place
case "_echo_place_enter":		// = /c <channel>
case "_echo_place_enter_other":	// = /c ??
case "_echo_place_enter_login":	// primary join, but used to be quiet..
		if (beQuiet != -1 && vars["_context"] != place) {
			if (beQuiet == source) pal = 3093;
			else beQuiet = source;
		}
case "_echo_place_enter_follow":	// = /f
case "_echo_place_enter_home":	// = /h
case "_echo_place_enter_join":	// = /j <channel>
		P3(("%O %O %O\n", ME, mc, vars))
# ifndef TAGS_ONLY
		if (ME == source // || psyc_object(source) == ME
		// weil bei remotes _source_relay der string von ME ist..
		// TODO: die Policy dass die Raeume das echo nach dem
		// 	castmsg direkt an uns sendet sollte den Code 
		// 	hier vereinfachen
		// 	u.u. auch deshalb, weil wir unterschiedliche
		// 	cases machen koennen fuer _notice und _echo
			  // || vars["_nick_verbatim"] == MYNICK
			     || ME == vars["_source_relay"]
			     || ME == psyc_object(vars["_source_relay"])) {
# else
		if (objectp(source) || tags[vars["_tag_reply"]]
				    || tags[vars["_tag"]] // BUGGY PLACES
		    ) {
# endif
			mixed p; 
			p = vars["_context"] || source;
			t = vars["_nick_place"];
# if 0 // DEBUG_FLAGS & 0x02 // abgesehen davon wurde der auch bei D0...
			// dieser unless leuchtet mir nicht mehr ein..
			// warum sollte context oder source jemals ME sein?
			unless (t || p == ME) {
				log_file("USER", "%O < broken (%O,%s,%O,%O)\n",
					 ME, source, mc, data, vars);
				return;
			}
# endif
			if (variant != 2933) { // primary join operation
				place = p;
				vSet("place", objectp(place) ? place->qName() : place);
				P3(("%O goes to %O (%O)\n", ME,
					 place, v("place")))
			} else {
				P3(("%O just joins %O (%O)\n", ME, p, t))
			}
#ifdef ENTER_MEMBERS
			// net/jabber does some funky things that do
			// not fit in here.
			if (variant != 2933) switch(v("scheme")) {
			case "jabber":
			    break;
			case "irc":
			case "psyc":
			    // this is a nice service the UNI provides
			    if (sizeof(vars["_list_members"])) {    // _tab
				vars["_list_members_nicks"] =
				 renderMembers(vars["_list_members"],
				  vars["_list_members_nicks"], objectp(source));
			    }
			    break;
			default:
			    // this part belongs into the client emulator
			    // instead (excluding the renderMembers)
			    if (sizeof(vars["_list_members"])) {    // _tab
				vars["_list_members_nicks"] =
				 renderMembers(vars["_list_members"],
				  vars["_list_members_nicks"], objectp(source));
				    w("_status_place_members", 0, vars);
			    } else if (vars["_amount_members"])
				w("_status_place_members_amount", 0, vars);
			    else
				w("_status_place_members_none", 0, vars);
			}
#endif
	// so much semantics in here.. shouldnt this be in person.c ?
	// wasn't user.c:msg just meant to be about display? TODO
			// incoming context isnt always objectp()
			// even for local contexts
			// then again.. as of now it should
			//rplaces[psyc_name(p)] = t;
			unless (places[p]) {
				places[p] = t;
				// D(S("p, t: %O, %O\n", p, t));
			} else {
				// we have already joined this
				return 1;
			}
			if (stringp(source)) {
			    P3(("%O joins mcast group for %O\n", ME, source))
			    register_context(ME, source);
			}
			variant = "";
			P4(("enter: places = %O after %s(%O) from %O\n",
			    places, mc, vars, source))
		} else {
			// untagged or wrong tag
			P0(("%O got untagged %s from %O: %O\n", ME, mc, source, vars))
			// eh... da stimmt was net
			return -1;
		}
		if (pal == 3093) return 1;
		pal = 0;
		break;
case "_notice_place_enter_automatic":
case "_notice_place_enter_automatic_subscription":
		P3(("auto-enter(%O,%O,%O,%O) for %O in %O\n", source,
		       mc, data, vars, ME, place))
case "_notice_place_enter_subscribe":	
case "_notice_place_enter":		// = /c <channel>
case "_notice_place_enter_other":	// = /c
case "_notice_place_enter_login":	// primary join, but used to be quiet..
		// _unicast comes from MUCs and carries a faked _context
case "_notice_place_enter_unicast":
//case "_notice_place_enter_quiet":
		if (beQuiet != -1 && vars["_context"] != place) {
			if (beQuiet == source) pal = 3093;
			else beQuiet = source;
		}
case "_notice_place_enter_follow":	// = /f
case "_notice_place_enter_home":	// = /h
case "_notice_place_enter_join":	// = /j <channel>
		P2(("msg.enter(%O,%O,%O,%O) for %O in %O\n", source,
		      mc, data, vars, ME, place))
				  // der neue parse.i liefert source als object
		if (pal == 3093) return 1;
		vSet("otherplace", vars["_context"] || source);
		variant = "";
		pal = 0;
		break;
case "_notice_place_leave_automatic":
case "_notice_place_leave_automatic_subscription":
		P2(("auto-leave(%O,%O,%O,%O) for %O in %O\n",
		       source, mc, data, vars, ME, place))
case "_notice_place_leave_logout":
case "_notice_place_leave_netburp":
case "_notice_place_leave_disconnect":
		// _unicast comes from MUCs and carries a faked _context
case "_notice_place_leave_unicast":
		if (beQuiet != -1 && vars["_context"] != place) {
			if (beQuiet == source) pal = 3093;
			else beQuiet = source;
		}
		// fall thru
case "_echo_place_leave_follow":
case "_echo_place_leave_home":
case "_echo_place_leave_invalid":
case "_echo_place_leave_netburp":
case "_echo_place_leave_other":
case "_echo_place_leave_reload":
case "_echo_place_leave_reload_server":
case "_echo_place_leave_subscribe":
case "_echo_place_leave_banned":
		// stellt sich die frage, ob _notice und _echo wirklich
		// beide existieren sollten, nur weil es ein unicast ist.
		// _notice kann man doch auch unicasten, und man würde sich
		// diese dämliche verdoppelung sparen.
case "_notice_place_leave_follow":
case "_notice_place_leave_home":
case "_notice_place_leave_invalid":
case "_notice_place_leave_other":
case "_notice_place_leave_reload":
case "_notice_place_leave_reload_server":
case "_notice_place_leave_subscribe":
case "_notice_place_leave_banned":
		variant = 0;	// no double variants
		// fall further
case "_echo_place_leave":
case "_notice_place_leave":
		// P1(("vars %O != %O(%O)\n", vars,place,v("place")))
#if 0
		if ((ME == source || (member(rplaces, source) && nick ==
				  MYNICK)) && (t = vars["_nick_place"])) {
#else
		if (ME == source && member(places, t = vars["_context"])
		    || member(places, t = source)) {
#endif
			if (abbrev("_notice_place_leave", mc)
			   &&! abbrev("_notice_place_leave_reload", mc))
			    mc[0..6] = "_echo";
#if 0
			P2(("%O %s's %O\n", ME, mc, t))
			m_delete(places, t); // places -= ([ t ]);
			P4(("%O %O: place %O, t %O, context %O\n", ME, mc,
			    place, t, vars["_context"]))
					// t should be more accurate
			if (place == (vars["_context"] || source)) {
				place = 0;
//				vDel("place");	// wrong! dann kommen die
//				 		   leute nich mehr in ihren
//				 		   letzten raum rein!
				// leave the context slave
				// BUG!! this does not belong within this if
                                deregister_context(ME, vars["_context"] || source);
						    // t should be more accurate
			}
#else
			leavePlace(t);
#endif
		}
		if (pal == 3093) return 1;
		pal = 0;
		break;
case "_notice_list_feature_server":
case "_notice_list_feature":
case "_notice_list_item":
case "_notice_list":
case "_notice":
case "_status_place_topic_official":
case "_status_place_topic":
case "_status_place":
case "_status":
		break; // optimization: avoid frequent unnecessary slicing
	PSYC_SLICE_AND_REPEAT
	}
#if 0 // reserved for the future when aliases are also affecting places
	if (v("aliases")[nick])) {
	    if (objectp(source)) { // this should alway be true
				   // as /alias doesn't allow aliases to
				   // contain ":", but you can never know.
		// nick collision!
		nick = psyc_name(source);
	    }
	}
#endif
	// imho macht das hier probleme bei _echo_place_enter
	// da ist der raum die source, es ist kein _context dabei
	// vars["_nick"] ist der nick des joinenden (==qName())
	// u:msg _nick "fippo" is changed into 
	// "«psyc://goodadvice.pages.de/@your-community» fippo"
	// IMHO this stuff should be hacked into an _INTERNAL var,
	// not a real var - fippo
	D2( if (vars["_nick"] != nick)
	    D(S("u:msg _nick %O is changed into %O\n", vars["_nick"], nick)); )

#if 0 //ndef GAMMA
	// DANGEROUS CHANGE! I added this line to make sure
	// remote UNI is shown on private messages. let's see
	// if it breaks anything --- 2003-05-09
	//
	// for irssi w/ no-irssi-echo-chatting a backup comes in handy.
	// --- 2005-01-07
	unless (vars["_nick_verbatim"]) vars["_nick_verbatim"] = vars["_nick"];
#endif
	vars["_nick"] = nick;
	// this at least works also for HTTP

	unless (wAction(mc, data, vars, source, variant, nick)) {
	    if (!data && vars["_action_possessive"])
		w(mc+"_action_possessive", data, vars, source, showingLog);
	    else
		w(variant ? mc+variant : mc, data, vars, source, showingLog);
	}
	return 1;
}

#ifndef GAMMA
// print() is for "important" output which gets lastlogged
//
// if no lastlog mechanism is available,
// you may choose to use this thing here
//
print(fmt, a,b,c,d,e,f,g,h,i,j,k) {
	string m = sprintf(fmt, a,b,c,d,e,f,g,h,i,j,k);
	P1(("user:print(%O,%O,%O..)\n", fmt,a,b))
	emit(m);
	return m;
}
#endif

// pr() has replaced p(), P() and print() in most cases.
// it uses the PSYC "message code" (actually a PSYC method)
// for classification of output going to the user
// (a bit like lastlog_level in ircII, remember?)
// but all of this should become w() now, since sprintf isnt flexible enuff
//
pr(mc, fmt, a,b,c,d,e,f,g,h,i,j,k) {
	P2(("user:pr(%O,%O,%O,%O..)\n", mc,fmt,a,b))
	if (mc) {
#ifdef PRO_PATH
		mapping di = printStyle(mc);
		if (di["_method"]) mc = di["_method"];
#endif
		unless (fmt = T(mc, fmt)) return;
#ifdef PRO_PATH
		if (di["_prefix"]) fmt = di["_prefix"]+fmt;
#endif
	}
	return emit(sprintf(fmt, a,b,c,d,e,f,g,h,i,j,k));
}

// the next generation comes along: w() expects
// a properly psyc-formatted string in data where it can replace
// the vars into and then output it. the text database can provide
// a "localized" format string using psyc-formatting, too.
//
w(string mc, string data, mapping vars, mixed source, int showingLog) {
	string template, output, type, loc;
	mapping di = printStyle(mc); // display infos
	int t;

	unless (vars) vars = ([]);
	else t = vars["_time_log"] || vars["_time_place"];
	// would be nicer to have _time_log in /log rather than showingLog
	if (!t && showingLog) t = vars["_time_INTERNAL"];
	if (t && intp(t)) di["_prefix"] = time_or_date(t) +" ";
#if 0
	template = T(di["_method"] || mc, 0);
#else
	template = T(di["_method"] || mc, "");
#endif
	P3(("%O user:w(%O,%O,%O,%O) - %O\n", ME,mc,data,vars,source, template))

#ifndef _flag_enable_alternate_location_forward
	// why did it say.. if (mc != lastmc && ...
	if (mappingp(v("locations")) && sizeof(v("locations"))) {
	    string nudata = data;
	   
	    // this little thing enables languages for psyc clients etc.
	    if (template && strlen(template) && !abbrev("_message", mc))
		nudata = template;

	    //PT(("%O user:w(%O,%O..%O) - %O\n", ME,mc,data,source, template))
	    P4(("%O user:w locations %O\n", ME, v("locations")))
	    foreach (type, loc : v("locations")) {
		// check uniformness of location here?
		// no! no broken location should have made it into the
		// mapping at this point. if you need to check it, check
		// it in sLocation()
#if DEBUG > 0
		if (!loc) {
			// oh.. happens on beta?
			P1(("%O late deletion of a %O zero location - should never happen\n", ME, type))
			m_delete(v("locations"), type);
			continue;
		}
#endif
		// allow for intp(type) then we can have several
		// see-all clients, not just [0]. needs support from person.c
		if (intp(type) || strstr(mc, type) != -1) {
		    // we dont need no rendered data! who needs that anyway?
		    // so this whole thing probably belongs
		    // back into person:msg()
//		    P3(("PSYCW: %s -> %O == %O? (%O)\n", mc, loc, source, vars))
		    // do we want to see stupid echoes of our stuff?
//		    if (source == loc) continue;
		    // this is the place to generate _source_relay for clients
		    //
		    // this causes /history to not provide the true originator
		    // of a message, as the room providing history already IS a
		    // _source_relay. so do we really need a hierarchy of relays?
		    // who's going to vouch for them? :)
		    //
		    if (source) {
#if 0 //ndef EXPERIMENTAL
			// _source_relay already contains context submitter
			if (member(vars, "_source_relay")) {
				if (source == vars["_source_relay"]) {
					// shouldn't happen:
					m_delete(vars, "_source_relay_relay");
				} else {
					vars["_source_relay_relay"] =
						   vars["_source_relay"];
					vars["_source_relay"] = source;
				}
			} else {
				vars["_source_relay"] = source;
				// shouldn't happen:
				m_delete(vars, "_source_relay_relay");
			}
#else
			// whoever gave us _source_relay was trustworthy
			unless (member(vars, "_source_relay")) {
				vars["_source_relay"] = source;
			}
#endif
		    }
#ifdef LPC3
		    // i have no idea how a _message_private can trigger this
		    // code but it does. member(vars, "_context") should not
		    // return true in that case. very strange!! therefore
		    // i add "&& vars["_nick_place"]" ... sigh!
		    if (member(vars, "_context") &&! vars["_context"]
			&& vars["_nick_place"]) {
			    // the context was an object, but got lost
			    // during ldmud's lastlog persistence. let's
			    // reconstruct it!
			    vars["_INTERNAL_context"] = SERVER_UNIFORM;
			    // the place nick shouldn't be missing, but
			    // sometimes it does. let's figure out why
			    if (stringp(vars["_nick_place"]))
			    	vars["_INTERNAL_context"] += "@"+
					lower_case(vars["_nick_place"]);
			    else {
				    P1(("%O encountered context reconstruction from nickless context (%O, %O, %O)\n", ME, mc, nudata, vars))
			    }
		    }
#else
# echo No LPC? Wow. Good luck!
#endif
#ifndef _flag_disable_circuit_proxy_multiplexing
		    // this is necessary when a single proxy is emulating
		    // several clients for several users. to figure out
		    // which context stuff is forwarded to which client
		    // we need to add this _target_forward. this is not the
		    // way psyc should operate in the long term. psyc clients
		    // should be integrated into the context distribution
		    // tree themselves, thus the proxy would manage a cslave
		    // for them instead of accepting forwards from each UNI
		    vars["_target_forward"] = loc;
		    // maybe this can be avoided when no _context is set...?
#endif
		    P3(("%O user:w forwarding %O to %O\n", ME, mc, vars["_target_forward"]))
		    sendmsg(loc, mc, nudata, vars);
//		    PT(("PSYCW: %s -> %O (%O)\n", mc, loc, vars))
#if DEBUG > 1
		    log_file("PSYCW", "%s(%O) %O » %O\n", mc, type,
			     nudata, type && loc);
#endif
		}
	    }
	}
#endif // !_flag_enable_alternate_location_forward

	unless (interactive(ME)) return 1;
	// who is that output for anyway?

	output = psyctext(template, vars, data, source);
	if (di["_prefix"]) output = di["_prefix"]+output;

	if (output) {
		if (template == "") {
		    // desperate security precaution until we have known
		    // methods. quite bad for english language users.
		    // the additional check for [_nick] is very desperate
		    // and should disappear sometime soon.. TODO
		    //
		    // we could aswell refine this by taking trustworthiness
		    // into consideration.. showing the localhost url for CVS
		    // notifications is just plain wrong
		    //
		    if (!stringp(source)|| (vars["_INTERNAL_trust"] >5
			 || (vars["_nick"] && data && strlen(data)
			     && strstr(data, "[_nick]") != -1)))
			output += "\n";
		    else {
			//PT(("notemp %O %O %O\n", source, vars, data))
			output = "«"+source+"» "+ output +"\n";
		    }
		}
		if (output != "")
#ifdef RELAY
		    emit(output, source);
#else
		    emit(output);
#endif
	}
	D2(else PP(("user:w zero output in %O for %O (%O)\n", ME, source, mc));)
	return 1;
}

wAction(mc, data, vars, source, variant, nick) {
	P3(("wAction(%O,%O..%O,%O,%O)\n", mc, data, source, variant, nick))
		ASSERT("sLocation", v("locations"), v("locations"))
	if (vars["_action"]) {
		mapping va;

		ASSERT("wAction", stringp(vars["_action"])
				&& strlen(vars["_action"]), vars["_action"])
		if (nick) {
#ifdef VARS_IS_SACRED
			// still holding on to shared vars. probably bad idea.
			va = copy(vars);
#else
			va = vars;
#endif
			va["_nick"] = nick;
		} else va = vars;

		if (data && data != "")
		    w(mc+"_text_action"+variant, data, va, source);
		else {
		    if (va["_nick_target"])
			w(mc+"_action"+variant,
#ifdef GAMMA
			    0,
#else
			     // bei _message gehört sich das nicht
			     "[_nick_target]: [_nick] [_action].",
#endif
			     va, source);
		    else
			w(mc+"_action"+variant,
#ifdef GAMMA
			  "[_nick] [_action].",
#else
			  0,
#endif
			  va, source);
		}
		return 1;
	}
	return 0;
}

#if DEBUG > 0
// coming from master/master.c
runtime_error(error, program, current_object, line, sprintfed) {
	w("_failure_exception_runtime", sprintfed);
	return 1;
}
#endif

logon() {
	string t;

	P2(("LOGON %O from %O\n", ME, query_ip_name() ))
	unless (legal_host(query_ip_number(), 0, 0, 0)) {
		// this happens when people reconnect during the shutdown
		// procedure.. and also when they are banned, but huh..
		// that hardly ever happens  :)
		w("_error_rejected_address",
		  "You are temporarily not permitted to connect here.");
		  //"I'm afraid you are no longer welcome here.");
		return remove_interactive(ME);
		// and the object will deteriorate when user gives up..
	}
	// shouldn't this be qScheme() instead? little paranoid TODO
	// but then we would have to move qScheme() from the server.c's
	// into the common.c's .. well, we could do that some day
	t = v("scheme");
	ASSIGN_SHOWECHO(v("echo"), t)
	if (t == "irc" || t == "psyc" || t == "jabber")
	     beQuiet = -1; // never turn off less interesting enter/leave echoes
	// makeToken() isn't a good idea here, apparently
	sTextPath(v("layout"), v("language"), t);
	// cannot if (greeting) this since jabber:iq:auth depends on this
	// (use another w() maybe?)
	w("_notice_login", 0, ([ "_nick": MYNICK,
	      "_page_network": "http://www.psyc.eu/",
	      "_name_network": "PSYC" ]), ME );
	while (remove_call_out(#'quit) != -1);
#ifndef _flag_disable_info_session
	if (t == "irc" || t == "tn") w("_warning_usage_set_charset",
	   0, ([ "_charset": v("charset") || SYSTEM_CHARSET ]));
#endif
	autojoin();
	cmdchar = (v("commandcharacter") ||
		   T("_MISC_character_command", "/"))[0..0];
#ifndef _flag_disable_info_session
	if (greeting) {
		w("_warning_usage_set_language",
 "Mittels \"/set language de\" kann zur deutschen Sprache gewechselt werden.");
		if (cmdchar != "/") w("_warning_modified_command_character",
 "Beware, \"[_command_character]\" is configured as your command character.",
		    ([ "_command_character" : cmdchar ]) );
	}
#endif
	cmdchar = cmdchar[0];

	actchar = v("actioncharacter") || T("_MISC_character_action",":");
	actchar = actchar == "off" ? 0 : actchar[0];

	// this code appears twice
	if (v("query")) w("_status_query_on",
		"Dialogue with [_nick_target] continued.", 
		([ "_nick_target" : v("query") ])
		);

#ifdef JABBER_PATH
	// currently only used by net/jabber/user.c
	unless(v("peoplegroups")) vSet("peoplegroups", ([]));
#endif
#ifdef _flag_log_hosts
	vSet("ip", query_ip_number(ME) ||
		(this_interactive() && query_ip_number(this_interactive())) );
#endif
	::logon( query_ip_name(ME) ||
		(this_interactive() && query_ip_name(this_interactive())) );
}

// net/jabber/user filters this call, so this is never executed for
// jabber users. how sad, why miss out on all newscasts? we should
// refine this so at least newscasts are kept. or should we solve
// that by +enrol? that would be jabber style.  TODO
// btw irc has it's own autojoin, which is a little different
autojoin() {
#ifndef _flag_disable_place_enter_automatic
	string s;
	object o;

# ifdef FORCE_PLACE
	// per local policy, all users must join the default place
	// a community-esq feature
	vSet("place", DEFPLACE);
# endif
	P2(("autojoin with %O %O %O\n", v("place"), place, places))
# ifndef GAMMA
	unless (v("place"))
	  vSet("place", T("_MISC_defplace", DEFPLACE));
# endif
	// see also http://about.psyc.eu/Client_coders#Room_members
	if (sizeof(places)) {
#if 0
		if (v("scheme") != "irc" && v("scheme") != "psyc" &&
		    v("scheme") != "jabber") {
			// ouch.. this has to be a notice! TODO!
			sendmsg(place, "_status_person_present_netburp",
			"[_nick] turns alive again.", ([
				 "_nick" : MYNICK ]) );
#if 0
			// fake it - use _echo_place_enter_relinked?
			// this message should only be displayed for IRC users
			// well, psyc clients need it too..
			foreach (o, s : places) 
				msg(o, "_echo_place_enter_relinked", 
				    "You reenter [_nick_place] after interruption.",
				  ([ "_nick_place" : s,
				     "_source_relay" : ME ]));
#endif
		} else {
#endif
			foreach (o, s : places) {
# ifdef FORCE_PLACE_RESET
// we could have a v("leaveonreconnect") instead of an ifdef for this
				placeRequest(o,
#  ifdef SPEC                  
                                     "_request_context_leave"
#  else                        
                                     "_request_leave"
#  endif
                                     "_netburp", 0, 1);
# else
				P2(("%O relinking %O\n", ME, o))
				placeRequest(o,
#  ifdef SPEC
                                     "_request_context_enter"
#  else
                                     "_request_enter"
#  endif
                                     "_relink", 0, 1);
# endif
			}
//		}
	}
	else {
# ifndef _flag_disable_place_default
#  ifdef GAMMA
		unless (v("place"))
		  vSet("place", T("_MISC_defplace", DEFPLACE));
#  endif
		// re-entering your last place is unusual by irc
		// habits, but since we don't want people to set up
		// autojoins they may find this quite practical until
		// they decide upon subscribe or not. the requested
		// history is probably not necessary for ircers, but
		// shouldn't harm either. jabberers never get here.
#  ifndef _limit_amount_history_place_default
#   define _limit_amount_history_place_default 5	// what about history glimpse?
			// history glimpse is how much a room will
			// allow (at max), this is how much the client
			// requests. maybe this should go, _login kind of
			// says it, too.
#  endif
		teleport(v("place"), "_login", 0, 0, ([ "_amount_history":
			  _limit_amount_history_place_default ]));
# endif
		// subscriptions are stored in lowercase, warum auch immer
		if (sizeof(v("subscriptions"))) {
//			string t1;
			P3(("places %O subscriptions %O\n",
			    places, v("subscriptions")))
			// code to avoid "double join" in particular for irc
//			t1 = lower_case(v("place") || "");
			// used to also handle v("home") which was complete
			// non-sense...
			foreach (s in v("subscriptions"))
//			    if (s != t1)
			    {
				P3(("enter: %O\n", s))
				// teleport() weeds out all places we are in
				// and avoids so called 'double joins' which
				// are bad for IRC
				teleport(s, "_automatic_subscription",
					 1, 1);
				// so we don't use this stuff anymore:
				//placeRequest(s,
				//   "_request_enter_automatic_subscription");
			}
		}
	}
#endif // _flag_disable_place_enter_automatic
}

quit(immediate, variant) {
	string s;
	object o;

	// this change has something to do with SANE_QUIT and fixing
	// the double joins.. says tobij. just a micro-hint.
	//if (immediate) return ::quit(immediate);
	if (find_call_out(#'quit) != -1) return ::quit(immediate);

	if (leaving++) {
		D1( D("recursive quit() in net/user: intercepted\n"); )
		return 0; // ignore potential errors from emit()
	}
	P2(("QUIT(%O,%O) in %O: %O\n", immediate, variant, ME, places))
	unless (variant) variant = "_logout";
//	if (v("subscriptions")) foreach (s in v("subscriptions"))
//	    placeRequest(s, "_request_leave_automatic_subscription", 1);
#ifdef SUBSCRIBE_PERMANENT
     // this may even work, but irc clients go crazy at seeing double joins
     // we should make this a feature of /detach instead, maybe?
	int stayinalive = 0;

     // so, SUBSCRIBE_PERMANENT won't work for now.
	if (sizeof(v("subscriptions")) && widthof(v("subscriptions"))) {
	    foreach (o, s : places) {
		P3(("Stay in %O, %O?\n", o, s))
		if (v("subscriptions")[lower_case(s)] != SUBSCRIBE_PERMANENT)
		    placeRequest(o,
#ifdef SPEC                  
                             "_request_context_leave"
#else                        
                             "_request_leave"
#endif
                            + variant, 1, 1);
		else
		    stayinalive++;
	    }
	} else
#endif
	foreach (o, s : places)
	    placeRequest(o,
#ifdef SPEC                  
                             "_request_context_leave"
#else                        
                             "_request_leave"
#endif
                            + variant, 1, 1);
	leaving = 0; // sonst gehen die announce's nicht raus
	//place = 0;  // maybe? dunno.
	return ::quit(immediate, variant);
}

// driver calls us here to tell us we lost the connection
// if you don't like this default behaviour, override it
//
// we also call this manually from _request_unlink_disconnect
disconnected(remainder) {
	// user did not detach his client properly. we'll make a wild guess
	// at how many messages he may have missed - enough to make the user
	// check the lastlog if that's not enough.
#ifdef GAMMA
	// FIXME: problem with jabber/user running into some bug when
	// lastlog messages are shown.. which at this point of course
	// creates a recursion - thus, eliminating the otherwise useful
	// feature of replay-on-tcp-loss here.
	if (find_call_out(#'quit) == -1 && v("scheme") != "jabber") {
		// can't use "leaving" here because it only serves the
		// purpose of detecting recursion within quit(), thus
		// gets resetted before arriving here.
		P1(("unexpected disconnect in %O\n", ME))
		vInc("new", 7);
	}
#endif
	// actually - we could show all messages since last activity
	// from user. TODO
#ifdef AVAILABILITY_OFFLINE
	if (availability == AVAILABILITY_OFFLINE) {
		P1(("i think i am already offline, so i won't quit (%O)\n", ME))
		return;
	}
#endif
	if (find_call_out(#'quit) != -1) return;
//	if (place) sendmsg(place, "_notice_place_leave_disconnect",
//		"[_nick] disconnects from the twilight zone.",
//			([ "_nick" : MYNICK ]) );
#if 1
	// disconnect is never a good way of quitting anyway
	//call_out(#'quit, 10, 1);
	// okay but why immediate here?
	call_out(#'quit, 10, 0, "_disconnect");
#else
	quit(0, "_disconnect");
#endif
        return 0;   // unexpected
}

#ifndef _flag_disable_module_presence
// usually called from logon, unless set to manual
announce(level, manual, verbose, text) {
	::announce(level, manual, verbose, text);
	unless (ONLINE) return;
	// psyc://x-net.hu/~tg probably just ran into a bug:
	// i forgot to put this if here.
	if (verbose) return showFriends(1);
}
#endif

// belongs into person.c as it is being called from there..?
static showFriends(verbose) {
	int i, idle, lnum, isaway;
	string *list;
	array(mixed) k;
	mixed o, n, *fmt;

	P3(("showFriends in %O: %O\n", ME, friends))
	k = m_indices(friends);
	unless(k && sizeof(k)) return;

	fmt = allocate(2);
	fmt[1] = T("_list_friends_present_each", "%s\n");
	fmt[1] = (fmt[1] == "%s\n") ? 0 : fmt[1][0..<2];
	fmt[0] = T("_list_friends_away_each", "%s\n");
	fmt[0] = (fmt[0] == "%s\n") ? 0 : fmt[1][0..<2];

	list = allocate(6, "");
	list[2] = ([ ]); list[5] = ([ ]);
	for(i=sizeof(k); i;) {
	    // there is a 'bug' inside which exposes idletimes of people you have offered
	    // friendship to - just check the lvl
	    o = k[--i];
#ifdef AVAILABILITY_NEARBY
	    isaway = friends[o, FRIEND_AVAILABILITY] < AVAILABILITY_NEARBY;
#else
	    isaway = 0;
#endif
	    list[isaway * 3 + 2][o] = friends[o, FRIEND_NICK];

	    if (objectp(o)) {
#ifdef ALIASES
		string n2;

		n = o->qNameLower();
		if (n && !(n2 = raliases[n]) && aliases[n]) {
		    n2 = psyc_name(o);
		}
		if (n2) n = n2;
#else
		unless(stringp(n = friends[o, FRIEND_NICK])) friends[o, FRIEND_NICK] = n = o->qName();
#endif
		list[isaway * 3] += ", "+ n;
		list[isaway * 3 + 1] += ", "+ (fmt[isaway] ? sprintf(fmt[isaway],n,n,n) : n);
		// for once we leave out the CALC_IDLE_TIME.. ;)
		if (verbose && idle = o->vQuery("aliveTime"))
		    list[isaway * 3 + 1] += " [" + timedelta(time() - idle) + "]";
	    } else {
		// TODO: do handling of remote nicks here
		// aliases + nickname here?
		// can we do the aliases bit when *storing* the friends?
//		unless (stringp(n = friends[o])) friends[o] =
		n = to_string(o);
#ifdef ALIASES
		n = raliases[lower_case(n)] || n;
#endif
		list[isaway * 3] += ", "+ n;
		list[isaway * 3 + 1] += ", "+ (fmt[isaway] ? sprintf(fmt[isaway],n,n,n) : n);
	    }
	}
	// if (list2 == "") list2 = list1;
	if (strlen(list[1]) > 2)
	    w("_list_friends_present", "Friends online: [_friends].",
	      ([ "_friends" : list[1][2..],
	         "_list_friends" : m_indices(list[2]),  // _tab
		 "_list_friends_nicknames" : m_values(list[2]) ]) );
	// man könnte ja die blöden textstrings dahinterschreiben
	// wenn es nicht zuviele sind..
	if (strlen(list[4]) > 2)
	    w("_list_friends_away", "Friends away: [_friends_away].",
	      ([ "_friends_away" : list[4][2..],
	         "_list_friends_away" : m_indices(list[5]), // _tab
		 "_list_friends_nicknames_away" : m_values(list[5]) ]) );
//	    pr("_list_friends_present", "Friends online: %s\n", list2[2..]);
	return list[0][2..];
}

listDescription(vars, eachout, nicklink) {
	string k, na, va, buf, fmt, t;

	buf = "";
	P3(("%O listDescription(%O,%O,%O)\n", ME, vars, eachout, nicklink))
#ifndef _flag_enable_profile_table
	// html version with more intelligent tables:
	// <tr id="[_key]"><td class="ldpek">[_name_key]</td><td class="ldpev">[_value]</td></tr>
	fmt = T("_list_description_each_item", "[_key]: [_value]\n");
#else
	// maybe someday we'll use psyctext() instead, but for now this
	// is sooo much faster. too bad the %22 trick doesn't work for utf8 
	fmt = T("_list_description_each", "%22s: %s\n");
#endif
	if (vars["_identification_alias"]) {
	    m_delete(vars, "_identification_scheme_XMPP");
	    //m_delete(vars, "_identification_scheme_SIP");
	    //m_delete(vars, "_identification_scheme_IRC");
	}
	if (vars["_host_name"] &&! boss(ME)) {
	    m_delete(vars, "_host_IP");
	}
#if 1 // fix and remove this.. TODO
	if (vars[0]) {
		P1(("Encountered vars[0] containing %O\n", vars[0]))
		vDel(vars, 0);
	}
#endif
	foreach (k : sort_array(m_indices(vars), #'>)) {
		va = vars[k];
		switch(k) {
		case "_color":
			if (v("scheme") == "applet") break;
			// else fall thru and filter
		case "_nick":
		case "_nick_stylish":
		case "_uniform_style":
		case "_uniform_photo_small":
		case "_INTERNAL_type_image_photo":
			va = 0;
			break;
		case "_uniform_photo":
			P4(("photo is %O\n", va))
			if (nicklink) va = 0;
			break;
		case "_language":
			if (va == v("language")) va = 0;
			break;
		case "_protocol_agent":
			if (va == v("scheme")) va = 0;
			break;
		case "_agent_design":
			if (va == v("layout")) va = 0;
			break;
		case "_action_motto":
			va = vars["_nick"] +" "+ vars[k];
			k = "_motto";
			break;
		case "_list_groups":    // _tab
#if 0
			if (nicklink) {
			    int i;
			    string n, u;
			    string s = SERVER_UNIFORM;

			    va = copy(va);
			    for (i=sizeof(va)-1; i>=0; --i) {
				n = va[i];
#if 1
				// TODO? this doesn't handle jids but i am
				// also not sure if jids belong here anyway
				u = is_formal(n) ? n : s +"@"+ n;
				va[i] = sprintf(nicklink, u, n, n);
#else
				va[i] = sprintf(nicklink, n, n, n);
#endif
			    }
			}
			va = implode(va, ", ");
			k = "_groups";
			break;
#endif
			// fall thru, since (currently) all entries are formal
			// the special case for ~ will never happen
		case "_list_friends":   // _tab
			if (nicklink) {
			    int i;
			    string n, u;
			    string s = SERVER_UNIFORM;

			    va = copy(va);
			    for (i=sizeof(va)-1; i>=0; --i) {
				n = va[i];
#if 1
				// TODO? this doesn't handle jids but i am
				// also not sure if jids belong here anyway
				u = is_formal(n) ? n : s +"~"+ n;
				va[i] = sprintf(nicklink, u, n, n);
#else
				va[i] = sprintf(nicklink, n, n, n);
#endif
			    }
			}
			va = implode(va, ", ");
			//k = "_friends";
			k = k[sizeof("_list")..];   // _tab
			break;
		case "_time_idle":
		case "_time_age":
			if (intp(va)) va = timedelta(va);
			break;
		case "_INTERNAL_image_photo":
			t = T("_list_description_image", "");
			// this scheme cannot display this
			if (t == "") { va = 0; break; }
			va = psyctext(t, ([
			    "_image_base64": va,
			    "_type_image": vars["_INTERNAL_type_image_photo"]
			]));
			k = "_image_photo";
			break;
		//default:
		}
		if (stringp(va)) {
			na = T("_TEXT"+k, "");
			unless (na) {
				P1(("Description: No _TEXT for %O\n", k))
			} else if (na != "") {
			    if (eachout) w("_list_description"+k,
			      //   sprintf("%22s: [%s]",na,k),
				   sprintf("%22s: %s",na,va),
				   ([ k: va, // "_text"+k: na
				    ]));
			    else
#ifndef _flag_enable_profile_table
				buf += psyctext(fmt, ([ "_key": k,
					"_name_key": na, "_value": va ]) );
#else
				buf += sprintf(fmt, na, va);
#endif
			}
			// else.. no output encoding
		}
	}
	return buf;
}

// no longer in use
qPlace() { if (place) return v("place"); }

qPublicInfo(showguests, showplace, showinvisibles) {
	mapping m;
	int newb, t;

	if (!ONLINE) return 3;	// gets called when an offline user
					// has been summoned by /tell
	newb = IS_NEWBIE;
	if (!showguests && newb) return 2;
	if (!showinvisibles && (v("guest") || v("visibility") == "off"))
	    return 1;

	CALC_IDLE_TIME(t);
	m = ([	"nick" : MYNICK, "lowerNick" : MYLOWERNICK,
		"me" : v("me"), "age" : v("age"),
		"registered" : !newb,
	    // actual aliveTime is for /lu usage only.. not for public output
	       	"idleTime" : t, "aliveTime" : v("aliveTime"),
		"name": v("publicname") || v("longname"),
		"operator" : boss(ME) > 9,
	]);
#ifdef SHOW_REMOTE_PLACES_IN_PEOPLE_LIST
	if (showplace) m["place"] = objectp(place)
					? place->qPublicName() : place;
#else
	if (showplace && objectp(place)) m["place"] = place->qPublicName();
#endif
#if 0 // __EFUN_DEFINED__(tls_query_connection_state)
	if (interactive(ME) && tls_query_connection_state()) m["encrypted"] = 1;
#endif
	return m;
}

