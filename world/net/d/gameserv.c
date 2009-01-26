// $Id: gameserv.c,v 1.12 2007/09/18 08:37:57 lynx Exp $ // vim:syntax=lpc
//
// game server gateway or something. quite amazing.
// serves also as demonstration on how to use the udp library.
//
#include <net.h>

#if defined(__ERQ_MAX_SEND__) && !defined(MUD)

#include <closures.h>

inherit NET_PATH "udp";

#define P_PING		0
#define P_FRAGS		1
#define P_NAME		2
#define P_DEATHS	3
#define P_TEAM		4

protected mapping servers, alias, ut, ut_player, q3, bf_player;

void create() {
    servers = ([ ]);
    alias = ([ ]);
    q3 = ([ "hostname" : "_name", "mapname" : "_map", "clients" : "_players", "sv_maxclients" : "_maxplayers" ]);
    ut = ([ "hostname" : "_name", "mapname" : "_map", "numplayers" : "_players", "maxplayers" : "_maxplayers" ]);
    ut_player = ([ "player" : P_NAME, "frags" : P_FRAGS, "ping " : P_PING ]);
    bf_player = ([ "player" : P_NAME, "score" : P_FRAGS, ]);
}

/*
GSERV: player_11 => -'-=||=<Dr.ArAgOrN
GSERV: frags_11 => 56
GSERV: ping_11 =>  69
GSERV: team_11 => 1
GSERV: mesh_11 => Male Soldier
GSERV: skin_11 => SoldierSkins.Gard
GSERV: face_11 => SoldierSkins.Wraith
*/

public string list_servers() {
	reset();
	string message = "SERVERS:\n";
	foreach(string key : m_indices(servers)) {
//		message += " "+key+"\t"+servers[key]+"\n";
		message += sprintf("%s\t%s\n",to_string(key),servers[key]["_name"]);
	}
	message += "\nALIASES:\n";
	foreach(string key : m_indices(alias)) {
//		message += " "+key+"\t"+alias[key]+"\n";
		message += sprintf("%s\t%s\n",key,alias[key]);
	}
	return message;
}

private closure gen_callback(string id,string type) {
	return lambda(({ 'flag, 'data, 'l_port, 'ip, 'r_port  }),
				  ({ CL_IF, ({ #'==, 'flag, 1 }),
                 ({ symbol_function("parse_"+type, ME), id, 'data, 'l_port,'ip, 'r_port }),
                 ({ #'return })
                 })
                );
}

parse_bf(string id, string data, int l_port, int ip, int r_port) {
	D2(D(data);)
}

parse_q3a(string id, string data, int l_port, int ip, int r_port) {
    array(string) temp;
    int size;
    temp = explode(data, "\n");
    string * info = explode(temp[1], "\\");
    size = sizeof(info);
    for(int i = 1; i < size; i = i + 2) {
        servers[id][(q3[info[i]] || info[i])] = info[i+1];
        D2(D("GSERV: "+info[i]+" => "+info[i+1]+"\n");)
    }
    if(temp[0] == "ÿÿÿÿstatusResponse") {
        int ping, frags;
        string name;
        foreach(string player : temp[2..]) {
            if(sscanf(player,"%U %U \"%s\"",frags,ping,name) == 3) {
                name = regreplace(name,"\\^[0-9]","",1);
                D2(D("PLAYER: "+frags+"\t"+ping+"\t"+name+"\n");)
                servers[id]["p"] += ({ ({ frags,ping,name }) });
            }
        }
    }
    if(servers[id]["_name"])
                alias[servers[id]["_name"]] = id;
	if(servers[id]["sv_privateClients"])
		    servers[id]["_maxplayers"] = to_int(servers[id]["_maxplayers"]) - to_int(servers[id]["sv_privateClients"]);
	servers[id]["time"] = time();
    return 1;
}

parse_ut(string id, string data, int l_port, int ip, int r_port) {
    array(string) temp;
    int size,num;
	string key, value;
    temp = explode(data, "\\")[1..];
    size = sizeof(temp);
    if(temp[size - 2] != "final") {
        closure cb = gen_callback(id,"ut");
        ticket(cb,l_port,ip,r_port);
    }
    for(int i = 0; i < size - 2; i = i + 2) {
		if(sscanf(temp[i],"%s_%U",key,num) == 2 && ut_player[key]) {
			if(sizeof(servers[id]["p"]) - 1 < num) servers[id]["p"][num] = allocate(10);
			servers[id]["p"][num][ut_player[key]] = temp[i+1];			
		} else {
        	servers[id][(ut[temp[i]] || temp[i])] = temp[i+1];
        	D2(D("GSERV: "+temp[i]+" => "+temp[i+1]+"\n");)
		}
    }
	if(servers[id]["_name"])
                alias[servers[id]["_name"]] = id;
	servers[id]["time"] = time();
    return 1;
}

public void del_server(string id) {
    m_delete(servers, id);
    foreach(string key  : m_indices(alias)) {
        if(alias[key] == id)
                m_delete(alias, key);
    }
}

public mapping info(string id) {
    if(servers[id] || servers[alias[id]]) {
		D2(D("wuuusch!\n");)
		return servers[id] || servers[alias[id]];	
	}
	return ([]);
}

public mapping* players(string id) {
    if(pointerp(servers[id]["p"]))
        return servers[id]["p"];
    return ({ });
}

public int update_server(string id) {
	closure cb;
	switch(lower_case(servers[id]["_type"])) {
	case "q1":
	case "quake1":
		cb = gen_callback(id,"q1");
			
		break;
	case "qw":
	case "quakew":
	case "quakeworld":
		cb = gen_callback(id,"qw");
	
		break;
	case "q2":
	case "quake2":
		cb = gen_callback(id,"q2");
		
		break;
	case "q3":
	case "q3a":
	case "quake3":
	case "quake3arena":
		cb = gen_callback(id,"q3a");
		ticket(cb,"q3a", servers[id]["_host"], servers[id]["_port"]);
		ticket(cb,"q3a", servers[id]["_host"], servers[id]["_port"]);
		send("q3a", servers[id]["_host"], servers[id]["_port"], 
			 to_string(({ 255,255,255,255 })) + "getinfo");
		send("q3a", servers[id]["_host"], servers[id]["_port"], 
			 to_string(({ 255,255,255,255 })) + "getstatus");
		break;
	case "unreal":
		cb = gen_callback(id,"ut");
		ticket(cb,"u", servers[id]["_host"], servers[id]["_port"]);
		send("u", servers[id]["_host"], servers[id]["_port"], "\\status\\");
		break;
	case "ut":
	case "unrealtournament":
		cb = gen_callback(id,"ut");
		ticket(cb,"ut", servers[id]["_host"], servers[id]["_port"] + 1);
        send("ut", servers[id]["_host"], servers[id]["_port"] + 1, "\\status\\");
		break;
	case "ut2003":
	case "unrealtournament2003":
		cb = gen_callback(id,"ut2003");
		ticket(cb,"ut2003", servers[id]["_host"], servers[id]["_port"] + 1);
		send("ut2003", servers[id]["_host"], servers[id]["_port"] + 1, "x" + to_string(({ 0,0,0,0 })));
		break;
	case "bf":
	case "bf1942":
		cb = gen_callback(id,"bf");
		ticket(cb,"bf", servers[id]["_host"], servers[id]["_port"] + 8433);
		send("bf", servers[id]["_host"], servers[id]["_port"] + 8433, "\\status\\");
		break;
	default:
		del_server(id);

		break;
	}
	
	return 1;
}

public varargs int add_server(string type, string r_host, int r_port, string name) {
    string id;
    reset();
    id = to_string(r_host + ":" + r_port);
    if(name) alias[name] = id;
    if(servers[id]) {
		return update_server(id);
    }
    servers[id] = ([
                  "_type" : type,
                  "_host" : r_host,
                  "_port" : r_port,
				  "time" : time(),
				  "p" : ({ }),
                  ]);
    return update_server(id);
/*  cb = lambda(({ 'flag, 'data, 'l_port, 'ip, 'r_port  }),
                ({ CL_IF, ({ #'==, 'flag, 1 }),
                 ({ symbol_function("parseGS", ME), id, 'data, 'l_port,'ip, 'r_port }),
                 ({ #'return })
                 })             
                ); */
}

load() {
    reset();
    register_scheme("gameserv");
    return ME; 
}

#else

public varargs int add_server(string type, string r_host, int r_port, string name) {
    D2(D("You need an erq to use d/gameserv.c\n");)
    return 0;
}

public string list_servers() {
    D2(D("You need an erq to use d/gameserv.c\n");)
    return "";
}
public void del_server(string id) {
    D2(D("You need an erq to use d/gameserv.c\n");)
}
public mapping info(string id) {
    D2(D("You need an erq to use d/gameserv.c\n");)
    return ([ ]);
}
public mapping* players(string id) {
    D2(D("You need an erq to use d/gameserv.c\n");)
    return ([ ]);
}
public int update_server(string id) {
    D2(D("You need an erq to use d/gameserv.c\n");)
    return 0;
}

#endif

msg(source, mc, data, mapping vars, showingLog, target) {
	D2(D("GAMESPY-D: HERE I AM!\n");)
}

