#include <net.h>
#include <services.h>
inherit NET_PATH "connect";
inherit NET_PATH "queue";

/*
 * this connects to a nntp server as client and does some fancy things.
 * the ultimate goal is to peer a nntp server and get notifications 
 * or even the whole articles fed into PSYC
 */
#define NEWSSERVER	"news.btx.dtag.de"
#define NEWSGROUP	"de.comm.chatsystems"

int busy; 

request(cmd, cb) {
    if (busy)
	return enqueue(ME, ({ cmd, cb }));
    // should be an input_to to idle()
    remove_input_to(ME);
    binary_message(cmd + "\n");
    next_input_to(cb);
}

// call this whenever you're idle
do_request() {
    mixed *a;
    a = shift(ME);

    request(a...);
}

idle(a) {
    int numeric;

    PT(("server said %O\n", a))
    sscanf(a, "%d %s\n", numeric, a);
    switch(numeric) {
    case 200:
	// HACK
	// fetchXHeader("Subject", 0, 0, "<dj4ve0$t40$1@online.de>");
	// HACK 2
	// fetchGroup(NEWSGROUP);
	// this is what should be really done
	busy = 0;
	do_request();
    default:
	next_input_to(#'idle);
    }
}

string *buffer;
doBuffer(a, whendone) {
    PT(("dobuffer %O\n", a))
    if (a == ".") {
	apply(whendone, buffer, 0); // dont flatten
	buffer = ({ }); 
	next_input_to(#'idle);
	return;
    }
    buffer += ({ a });

    input_to(#'doBuffer, INPUT_IGNORE_BANG, whendone);
}

// NNTP library below

_gotGroup(a) {
    int numeric;

    sscanf(a, "%d%t%s", numeric, a);
    switch(numeric) {
    case 211:
	{
	    int num, low, high;
	    if (sscanf(a, "%d %d %d %s", num, low, high, a) == 4) {
		gotGroup(a, num, low, high);
	    } else {
		P0(("%O broken 211 command?\n"))
	    }
	}
	break;
    default:
	PT(("_gotGroup status code %d - %O\n", numeric, a))
	break;
    }
    // 211 -> ok
    // 411 -> No such group %s
}

gotGroup(group, num, low, high) {
    PT(("got group %O with %d articles from %d to %d\n", group, num, low, high))
    // HACK 3
    fetchXHeader("Subject", high - 5, high);
}

fetchGroup(group) {
    request(sprintf("GROUP %s", group), #'_gotGroup);
}

_gotXHdr2(buffer) {
    PT(("_gotXHdr2 %O\n", buffer))
}

_gotXHdr(a) {
    int numeric;
    PT(("_gotXHdr(%O)\n", a))
    sscanf(a, "%d %s", numeric, a);
    switch(numeric) {
    case 430: // no such article
    case 221:
	buffer = ({ }); 
	input_to(#'doBuffer, INPUT_IGNORE_BANG, #'_gotXHdr2);
	break;
    }
}

fetchXHeader(what, low, high, id) {
    string q;
    q = "XHDR " + what + " ";
    if (id) 
	q += id;
    else if (low == 0 && high == 0) 
	; // NOOP
    else if (high == 0) 
	q += low + "-";
    else if (low == 0) 
	q += "-" + high;
    else
	q += low + "-" + high;
	
    request(q, #'_gotXHdr);
}

logon(result) {
    int ret;
    ret = ::logon(result);
    busy = 1;
    next_input_to(#'idle);
    fetchGroup(NEWSGROUP);
    return ret;
}

create() {
    qCreate();
    qInit(ME, 30);
    connect(NEWSSERVER, NNTP_SERVICE);
    return 1;
}
