// $Id: decode.c,v 1.30 2008/07/28 20:30:48 lynx Exp $ // vim:syntax=lpc
//
// generic CTCP implementation, also includes color code filter.
// msa (Markku Savela, if i remember the spelling right) came up
// with the crazy idea of using binary code 1 as the escape code
// for CTCP, so we put that code into the variable msa.
//
#include "irc.h"
#include <net.h>

volatile string msa, msare, cc, bc, uc;

// belongs into create() really..
decodeInit() {
	msa = " "; msa[0] = 0x01;
	msare = msa+".*"+msa;
	cc = " "; cc[0] = 0x03;		// same as msa
	bc = " ", bc[0] = 0x02;		// same as msa
	uc = " ", uc[0] = 0x1F;		// same as msa
}

// to be inherited
ctcp(type, text, target, req, srcnick, source) {
	mapping vars;

	P2(("%O %s %O -> %O: %O %O\n", ME,
	     req ? "REQ" : "REP",
	     srcnick, target, type, text))

	vars = ([ "_nick" : srcnick,
		  "_value" : text || "",
		  "_type" : type ]);

	sendmsg(target, (req? "_request": "_status") +"_legacy_CTCP",
	     "[_nick] "+ (req? "requests": "sends")+
	     " IRC legacy command '[_type]'.",
		 vars, source );
}

// to be inherited
version(text, target, req, srcnick, source) {
	if (req && !text) {
		P2(("%O VREQ(1=%O) %O -> %O: %O\n", ME, req,
		     srcnick, target, text))

		sendmsg(target, "_request_version",
		     "[_nick] requests your version.",
		    ([ "_nick" : srcnick ]), source );
#ifndef _flag_disable_request_version_IRC
	} else if (target == query_server_unl()) {
		if (text) vSet("agent", text);
#endif
	} else {
		PT(("%O VREP(0=%O) %O -> %O: %O\n", ME, req,
		     srcnick, target, text))

		// generally unused as we don't send ctcp version to our clients
		sendmsg(target, "_status_version_agent",
    "Version: [_nick] is using \"[_version_agent]\" ([_version]).", ([
		       "_nick"		: srcnick,
		       "_version"	: SERVER_VERSION,
		       "_version_server": SERVER_DESCRIPTION,
		       "_version_agent"	: text
		    ]), source );
	}
	return;
}

// text, recipient string from irc, req: PRIVMSG/_request:1, NOTICE/_status:0
decode(text, rcpt, req) {
	// filtering first.. 'cause some crazy people may be putting
	// color codes inside of ctcp requests etc.

	// filtering for mirc color codes
	// these will never make it into a psyc standard ;)
	// so they need to be removed
	unless (strstr(text, cc) == -1) {
	    text = regreplace(text, cc + "[0-9][0-9],[0-9][0-9]", "", 1);
	    text = regreplace(text, cc + "[0-9][0-9],[0-9]", "", 1);
	    text = regreplace(text, cc + "[0-9],[0-9][0-9]", "", 1);
	    text = regreplace(text, cc + "[0-9],[0-9]", "", 1);
	    text = regreplace(text, cc + ",[0-9][0-9]", "", 1);
	    text = regreplace(text, cc + ",[0-9]", "", 1);
	    text = regreplace(text, cc + "[0-9][0-9]", "", 1);
	    text = regreplace(text, cc + "[0-9]", "", 1);
	    text = regreplace(text, cc, "", 1);
	}

	// filtering for irc bold code
	unless (strstr(text, bc) == -1) 
	    text = regreplace(text, bc, "", 1);

	// filtering for irc underline code
	unless (strstr(text, uc) == -1)
	    text = regreplace(text, uc, "", 1);

	// it is more correct to check the whole text string for msa,
	// but in practice no irc entity ever produces such a string
	// with CTCP interspersed into the plain text, so were much
	// more efficient to just look at the first character
	if (text[0] == 0x01) {
		string ctcpcode, ctcptext;
		array(string) extract;

		extract = regexplode(text, msare);
		unless (sizeof(extract) >= 2) return; // unbalanced msa
		ctcpcode = extract[1][1..<2];
		sscanf(ctcpcode, "%s %s", ctcpcode, ctcptext);
		ctcpcode = upper_case(ctcpcode);
		P2(("CTCP %O w/ value %O found.\n", ctcpcode, ctcptext))

		switch (ctcpcode) {
		    case "ACTION":
			action(ctcptext, rcpt);
			break;
		    case "VERSION":
			unless (rcpt) break;
			version(ctcptext, rcpt, req);
			break;
		    case "PING":
			if (rcpt) 
			    if (req)
				ping(rcpt, ctcptext);
			    // else ignore it for now as we cant tag it
			break;
		    case "XCHAT":
			// xchat sends an additional non-standard reply
			// to the ctcp version request. we need to ignore
			// this, instead of routing it to the server root.
			break;
		    // this is an EXPERIMENTAL extension from fippo, but
		    // since this code can only be triggered by the
		    // appropriate irssi script, we can just leave it here
		    case "TYPING":
			PT(("ctcp typing %O\n", ctcptext))
			unless(rcpt) break;
			switch(ctcptext) {
			case "INACTIVE":
			case "PAUSE":
			case "COMPOSE":
			case "ACTIVE":
			case "GONE":
			    tell(rcpt, "", 0, 0, "_notice_typing_" + lower_case(ctcptext));
			    break;
			default:
			    ctcp(ctcpcode, ctcptext, rcpt, req);
			}
			break;
		    default:
			unless (rcpt) break;
			ctcp(ctcpcode, ctcptext, rcpt, req);
		}
		return 0;
	}
	return text;
}

// ein klein wenig ratlosigkeit
#if 1
irctext(template, vars, data, source) {
	int i;
	string output = psyctext(template, vars, data, source);
	for (i=strstr(output, "%"); i>=0; i=strstr(output, "%", i)) {
		output[i] = 0x01; // support for msa's CTCP char
	}
	return output;
}
#else
ircp(output) {
	int i;
	for (i=strstr(output, "%"); i>=0; i=strstr(output, "%", i)) {
		output[i] = 0x01; // support for msa's CTCP char
	}
	return emit(output);
}
#endif
