#include <net.h>
#include <person.h>
#include <status.h>

#define BLAME "!configuration"
#define DONT_REWRITE_NICKS
#define PLACE_HISTORY
#define PLACE_OWNED
#define HISTORY_GLIMPSE 12

#include <uniform.h>

inherit NET_PATH "place/threads";

volatile mixed lastTry;

volatile string owner;
volatile string channel;

load(name, keep) {
    P3((">> userthreads:load(%O, %O)\n", name, keep))

    sscanf(name, "~%s#%s", owner, channel);
    vSet("owners", ([ owner: 0 ]));
    vSet("privacy", "private");

    vSet("_restrict_invitation", BLAME);
    vSet("_filter_conversation", BLAME);

    return ::load(name, keep);
}

enter(source, mc, data, vars) {
    P3((">> userthreads:enter(%O, %O, %O, %O)\n", source, mc, data, vars))
    object p = summon_person(owner, NET_PATH "user");
    string src = objectp(source) ? psyc_name(source) : source;

    unless (v("privacy") == "public" ||
	    (p && (p == source || qAide(src) || (objectp(source) && qAide(source->qNameLower()))
		   || p->qFriend(source) || p->qFollower(source)))) {
	sendmsg(source, "_error_place_enter_necessary_invitation",
		"[_nick_place] can only be entered upon invitation.",
		([ "_nick_place" : qName() ]) );
	if (source != lastTry) {
	    castmsg(ME, "_failure_place_enter_necessary_invitation",
		    "Admission into [_nick_place] denied for uninvited user [_nick].",
		    vars);
	    lastTry = source;
	}
	return 0;
    }

    if (p == source) {
	p->sChannel(MYNICK);
    }

    return ::enter(source, mc, data, vars);
}

_request_add(source, mc, data, vars, b) {
    P3((">> userthreads:_request_add(%O, %O, %O, %O, %O)\n", source, mc, data, vars, b))
    unless (vars["_person"]) {
	sendmsg(source, "_warning_usage_add", "Usage: /add <person>", ([ ]));
	return 1;
    }

    object p = summon_person(owner, NET_PATH "user");
    object target = summon_person(vars["_person"], NET_PATH "user");
    string ni;
    if (objectp(target)) {
	ni = target->qName();
    } else {
	target = vars["_person"];
	mixed *u = parse_uniform(target);
	if (u) ni = u[UNick];
    }

    unless (ni && p && (p->qFriend(target) || p->qFollower(target))) {
	sendmsg(source, "_error_add",
		"Error: [_person] is not a friend or follower.", ([ "_person": vars["_person"]]));
	return 1;
    }

    string _mc = "_notice_place_enter_automatic_subscription_follow";
    sendmsg(target, _mc, "You're now following [_nick_place].", (["_nick_place" : qName()]), source);
    //insert_member(target, mc, 0, (["_nick": ni]), ni);

    return 1;
}

_request_remove(source, mc, data, vars, b) {
    P3((">> userthreads:_request_remove(%O, %O, %O, %O, %O)\n", source, mc, data, vars, b))
    unless (vars["_person"]) {
	sendmsg(source, "_warning_usage_remove", "Usage: /remove <person>", ([ ]));
	return 1;
    }

    object p = summon_person(owner, NET_PATH "user");
    object target = summon_person(vars["_person"], NET_PATH "user") || vars["_person"];

    unless (p && (qMember(target) || p->qFriend(target) || p->qFollower(target))) {
	sendmsg(source, "_error_add",
		"Can't remove: [_person] is not a friend or follower.", ([ "_person": vars["_person"]]));
	return 1;
    }

    string _mc = "_notice_place_leave_automatic_subscription_follow";
    sendmsg(target, _mc, "You're no longer following [_nick_place].", (["_nick_place" : qName()]), source);
    remove_member(target);

    return 1;
}

// set privacy: private or public
//  - private: only friends & invited people can enter (default)
//  - public: anyone can enter
_request_privacy(source, mc, data, vars, b) {
    P3((">> userthreads:_request_privace(%O, %O, %O, %O, %O)\n", source, mc, data, vars, b))
    string p = vars["_privacy"];
    if (p == "public" || p == "private") {
	vSet("privacy", p);
	save();
    }
    sendmsg(source, "_status_privacy", "Privacy is: [_privacy].", (["_privacy": v("privacy")]));
    return 1;
}

htMain(int last) {
    return htmlEntries(_thread, last, 1, channel);
}

canPost(snicker) {
    return qOwner(snicker);
}

isPublic() {
    return vQuery("privacy") == "public";
}

qChannel() {
    return channel;
}

qHistoryGlimpse() {
    return HISTORY_GLIMPSE;
}
