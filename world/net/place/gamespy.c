// $Id: gamespy.c,v 1.13 2008/01/05 12:42:17 lynx Exp $ // vim:syntax=lpc
//
// gamespy gateway in a room, or something
//
#include <net.h>
#include <person.h>
#include <status.h>
inherit NET_PATH "place/owned";

mapping servers;

void create() {
	if (!servers) servers = ([]);
	::create();
}

cmd(a, args, b, source, vars) {
// TODO: multiline-sachen irgendwie
	D2(D("===========================\n");)
    //unless (source) source = previous_object();
    switch (a) {
	case "add":
		unless (qAide(SNICKER)) return;
		if(!add(args)) {
			sendmsg(source, "_warning_usage_gamespy_add",
				"Usage: /add «gametype» «ip»:«port» [«name»]");
		} else {
			castmsg(source, "_notice_gamespy_added",
				    "Successfully added [_type]-Server [_host]",
							([ "_type" : args[1],
							   "_host" : args[2],							 
							 ]));
		}
		return 1;
	case "delete":
	case "del":
	
		return 1;
	case "server":
	case "serverinfo":
		if(sizeof(args) < 2) {
			sendmsg(source, "_warning_usage_gamespy_info",
                            "Usage: /info «ip»:«port» OR: /info «name»");
			return 1;
		} else {
			mapping info = DAEMON_PATH "gameserv"->info(args[1]);
			if(!sizeof(m_indices(info))) {
				sendmsg(source, "_error_gamespy_info",
                            "I dont know that gameserver! Use /add to add it!");	
				return 1;
			}
			sendmsg(source, "_notice_gamespy_info",
							"================\nhost\t[_host]:[_port]\n"
							"name\t[_name]\nmapname\t[_map]\n"
							"players\t[_players]/[_maxplayers]",
                            info);			
			return 1;
		}
	case "servers":
	case "listservers":
		D2(D(DAEMON_PATH "gameserv"->list_servers());)
		return 1;
	}

	return ::cmd(a, args, b, source, vars);
}

add(args) {
	array(string) host;
	if(sizeof(args) < 3)
				return 0;
	host = explode(args[2],":");
	if(sizeof(host) != 2)
				return 0;
	if(sizeof(args) >= 4) {
		DAEMON_PATH "gameserv"->add_server(args[1],host[0],to_int(host[1]),args[3]);
	} else {
		DAEMON_PATH "gameserv"->add_server(args[1],host[0],to_int(host[1]));
	}
	return 1;
}

