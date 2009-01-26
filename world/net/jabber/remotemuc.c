#include <net.h>

/* a cache for the remote muc 
 * yes, this is for a SINGLE person
 * yes, that's horrible
 *
 * at least we get to implement a member list and
 * even /history if we'd like to have that..  ;)
 */

// FIXME: should set "owner nick" and tag so 
// 	we can detect when the remote room does not 
// 	support tagging or when we are kicked
// 	and this could even do echo detection
// 	NOTE: this should work when using the
// 		isecho check

object owner;
mapping membercache = ([ ]);

void insert_member(mixed member, mixed origin, mixed data) {
    owner = member;
}

void remove_member(mixed member, mixed origin) {
    owner = 0;
    membercache = 0;
    destruct(ME);
}

castmsg(source, method, data, mapping vars) {
    P3(("remotemuc proxying %O\n", method))
    int isecho = (source == vars["_context"]);
    if (isecho) 
	source = owner;
    switch(method) {
    case "_message_public":
	break;
    case "_notice_place_enter":
	if (isecho) {
	    // can happen if room does not support tagging
	    P0(("%O joined via _notice_place_enter, this is strange\n", vars["_context"]))
	} else if (membercache[source]) {
	    P4(("skipping notice place enter from person already in room\n"))
	    return;
	} else 
	    membercache[source] = vars["_nick"];
	break;
    case "_notice_place_leave":
	if (isecho) {
	    // got kicked or room does not support tagging
	    P0(("%O left via _notice_place_leave, this is strange\n", vars["_context"]))
	}
	m_delete(membercache, source);
	break;
    }
    if (!owner) {
	P4(("remotemuc castmsg %O and there is no owner\n", method))
	return; 
    }
    owner -> msg(source, method, data, vars );

}

mapping qMembers() {
    P4(("membercache is %O\n", membercache))
    return m_indices(membercache);
}
