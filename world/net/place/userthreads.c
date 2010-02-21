#include <net.h>
#include <person.h>
#include <status.h>

#define BLAME "!configuration"
#define DONT_REWRITE_NICKS

inherit NET_PATH "place/threads";

volatile mixed lastTry;

volatile string owner;
volatile string channel;

load(name, keep) {
    P3((">> userthreads:load(%O, %O)\n", name, keep))

    sscanf(name, "~%s#%s", owner, channel);
    vSet("owners", ([ owner: 0 ]));

    vSet("_restrict_invitation", BLAME);
    vSet("_filter_conversation", BLAME);

    return ::load(name, keep);
}

enter(source, mc, data, vars) {
    P3((">> userthreads:enter(%O, %O, %O, %O)\n", source, mc, data, vars))
    object p = summon_person(owner, NET_PATH "user");
    string src = psyc_name(source);

    unless (p && (p == source || qAide(src) || p->qFriend(source) || p->qFollower(source))) {
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

cmd(a, args, b, source, vars) {
    P3((">> threads:cmd(%O, %O, %O, %O, %O)", a, args, b, source, vars))

    switch (a) {
	case "add": // or similar
	    // add follower to this channel, TODO
    }

    return ::cmd(a, args, b, source, vars);
}

canPost(snicker) {
    return qOwner(snicker);
}
