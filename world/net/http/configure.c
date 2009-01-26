// $Id: configure.c,v 1.59 2008/04/22 22:43:56 lynx Exp $ // vim:syntax=lpc
//
// web configurator interface. does a lot of cool things for you,
// and it should always learn to do some more. if you don't need or
// like it, simply don't load it (from init.ls, or by fetching the url).
//
#include <ht/http.h>
#include <net.h>
#include <driver.h>

#if HAS_PORT(HTTP_PORT, HTTP_PATH) || HAS_PORT(HTTPS_PORT, HTTP_PATH)

#include <text.h>
#include <person.h>

#include <misc.h>
inherit NET_PATH "sockets";

#ifdef WEB_CONFIGURE
			// should this use something else?
# define CONFIGFILE	CONFIG_PATH "local.h"

volatile protected mapping description;

void create() {
    sTextPath(0, 0, "html");
    description = ([ 
// this has all moved into psyced.ini
//	"CHATNAME" : "Name of this Chat Community",
//	"DEFPLACE" : "Standard place unregistered users go to",
//	"ADMINISTRATORS" : "List of administrators <small>(example: "
//	"&quot;admin1&quot;, &quot;admin2&quot;, "
//	"&quot;admin3&quot;)</small>",
//	"__DOMAIN_NAME__" : "Domain name of this server",
//	"SERVER_HOST" : "Address of this server",
//	"OSTYPE" : "Your Operating System",
//	"MACHTYPE" : "The flavour of your processor",
// optimise on tunings, like this one?
	"MAX_VISIBLE_USERS" : "MAX_VISIBLE_USERS",
// a bit absurd: specify a file on the fs instead of having a motd textarea
	"MOTD_FILE" : "The file that contains the IRC-MOTD",
    ]);
}

denyAccess(prot) {
    hterror(prot, 200, "You are not allowed to use the configure-page. "
	    "You need to be from <i>localhost</i> to do that.");
}

parseConfig(fname) {
    mapping cv;
    int size;

    cv = ([ ]);

    if (file_size(fname) <= 0) {
	return ([ ]);
    }

    foreach(string line : explode(read_file(fname), "\n")) {
	mixed key, value;

	if (sscanf(line, "#define%t%s%t%s", key, value)
	    || sscanf(line, "#define%t%s", key)
	    || sscanf(line, "#%tdefine%t%s%t%s", key, value)
	    || sscanf(line, "#%tdefine%t%s", key)
	    ) {
		//if (dummy=="" && index(key, ' ')==-1 && index(key, '\t')==-1)
			    // && (member(description, key) != -1))
		    cv[key] = value;
	}
    }

    return cv;
}

saveConfig(string fname, mapping config, mapping del) {
    string file, key, value, tail;
    int added;
    
    unless (mappingp(del)) {
	del = ([ ]);
    }

    if (file_size(fname) <= 0) {
	if (abbrev("place/", fname)) {
	    file = w("_PAGES_configure_room_new");
	} else {
	    return;
	}
    } else {
	file = read_file(fname);
    }
    added = 0;
    foreach (key, value : config) {
	// re scheint keine + syntax zu beherrschen
	P2(("===> %O\n", value))
	if (to_string(to_int(value)) != value && value != "") {
	    unless (value[0] == '"')
		    value = "\"" + value;
			
	    unless (value[<1] == '"')
		    value += "\"";
	}

	if (sizeof(regexp(({ file }),
			  "#([ \t]*)define([ \t][ \t]*)"+key+
			  "([ \t][ \t]*)[^\n]*"))) {
	    tail = "([ \t][ \t]*)[^\n]*";
	} else if (sizeof(regexp(({ file }),                  
				 "#([ \t]*)define([ \t][ \t]*)"+key))) {
	    tail = "";
	} else {
	    // adden
	    file = regreplace(file,
			      "#include <place.gen>",
			      "#define "+key+" "+ 
			      (value != "" ? " "+value : "")+
			      "\n#include <place.gen>", 0);
	    added = 1;
	    
	}
	
	unless (value == "" && added == 0) {
	    file = regreplace(file,
		"#([ \t]*)define([ \t][ \t]*)"+key+tail, 
		"#\\1define "+key+"\t"+value, 0);
	} else if (added == 0){
	    file = regreplace(file,
		"#([ \t]*)define([ \t][ \t]*)"+key+tail, 
		"#\\1define "+key, 0);
	}
	// old ldmud requires 4th argument
//	if (t == file) printf("Could not change %s into %O\n", key, value);
    }
    
    foreach (key, value : del) {
	file = regreplace(file,
		"#([ \t]*)define([ \t][ \t]*)"+key+"[^\n]*\n",
		"", 0);
    }
    rm(fname);
    write_file(fname, file);
    return w("_PAGES_configure_advice_restart") + "<font size=6><pre>\n" +
	   "Congratulations, this is your new configuration file:\n" +
	   "</pre><font color=green><plaintext>\n" + file;
}

htget(prot, query, headers, qs) {
    string s; // output buffer
    int trustworthy = legal_host(query_ip_number(), HTTP_PORT, "http", 0);
    
    if (trustworthy < 7) {
	denyAccess(prot);
	return 1;
    }

    if (file_size(CONFIGFILE) <= 0) {
	hterror(prot, 404, "You don't have any configfile, do you?");
	return 1;
    }

    htok3(prot, "text/html", "Cache-Control: no-cache\n");

    switch (query["option"]) {
    case "user_mgm":
	    s = w("_PAGES_configure_user",
		  ([ "_title" : "User Management" ]));
	    break;
    case "list_users":
	    write(w("_PAGES_configure_user_list",
	      ([ "_title" : "User List" ])));
	    list_sockets();
	    return 1;
    case "block_modify":
	    if (query["ip"] && query["ip"] != 0) {
	 	object ob;

		DAEMON_PATH "hosts"->modify(query["ip"]);
		foreach (ob : users()) {
		    if (abbrev(query["ip"], query_ip_number(ob))) {
			log_file("BEHAVIOUR",
			    "[%s] %O blocks %O (%s)\n",
			      ctime(), ME, ob, query["ip"]);
			ob -> sanction("kill");
		    }
		}
	    }
    case "list_blocks":
	    write(w("_PAGES_configure_user_block_head",
		    ([ "_title" : "Block Management",
		       "_submit_value" : "Submit" ])));
	    DAEMON_PATH "hosts"->list();
	    return 1;
    case "user_password":
	    s = w("_PAGES_configure_user_password",
		  ([ "_title" : "Reset Password",
		     "_submit_value" : "Edit" ]));
	    break;
    case "user_password_edit":
	    {
		object o;

		unless (o = find_person(query["nick"])) {
		    unless (o = summon_person(query["nick"])) {
			s = w("_PAGES_configure_user_password_failure",
			  ([ "_title" : "Reset Password" ]));
			break;
		    } 
		}

		if (o->isNewbie()) {
		    destruct(o);
		    s = w("_PAGES_configure_user_password_failure",
		      ([ "_title" : "Reset Password" ]));
		    break;
		}

		s = w("_PAGES_configure_user_password_edit",
		      ([ "_title" : "Reset Password",
		         "_submit_value" : "Reset Password",
			 "_nick" : query["nick"] ]));
		break;
	    }
    case "user_password_save":
	    {
		object o;

		if (! query["password"] || strlen(query["password"]) <= 2) {
		    s = w("_PAGES_configure_user_password_failure_bad",
			  ([ "_title" : "Reset Password" ]));
		    break;
		}

		unless (o = find_person(query["nick"])) {
		    unless (o = summon_person(query["nick"])) {
			s = w("_PAGES_configure_user_password_failure",
			  ([ "_title" : "Reset Password" ]));
			break;
		    } 
		}

		if (o->isNewbie()) {
		    destruct(o);
		    s = w("_PAGES_configure_user_password_failure",
		      ([ "_title" : "Reset Password" ]));
		    break;
		}

		o->vSet("password", query["password"]);
		o->save();

		s = w("_PAGES_configure_user_password_save",
		      ([ "_title" : "Reset Password",
		         "_nick" : query["nick"],
			 "_password" : query["password"] ]));
		break;
	    }
    case "basic_mgm":
	    s = w("_PAGES_configure_basic",
		  ([ "_title" : "Basic Server Management" ]));
	    break;
    case "restart":
	    s = w("_PAGES_configure_head",
		  ([ "_title" : "Restart" ]));
	    s += "<b>Your chatserver is being restarted.</b>";
	    // write at the bottom wouldn't be called after shutdown, would it?
	    write(s);
	    shutdown();
	    return 1;
    case "list_udp":
	    {
		mapping ports;
		string port;

		s = w("_PAGES_configure_basic_udp_head",
		      ([ "_title" : "List UDP Ports" ]));
		ports = DAEMON_PATH "udp"->listPorts(1);

		foreach (port : m_indices(ports)) {
		    s += w("_PAGES_configure_basic_udp_entry",
			   ports[port]);
		}

		s += w("_PAGES_configure_basic_udp_tail");
			   
		break;
	    }
    case "room_mgm":
	    s = w("_PAGES_configure_room",
		  ([ "_title" : "Room Management" ]));
	    break;
    case "room_owned":
	    s = w("_PAGES_configure_room_owned",
		  ([ "_title" : "Manage Owned Rooms",
		     "_submit_value" : "Edit / Add" ]));
	    break;
    case "room_owned_save":
	    {
		mapping del;

		del = ([ ]);

		m_delete(query, "option");

		if (query["delete"]) {
		    rm("place/" + lower_case(query["NAME"]) + ".c");
		    s = w("_PAGES_configure_room_deleted",
		      ([ "_title" : "Room Deleted",
		         "_name" : query["NAME"] ]));
		    break;
		}

		if (query["ALLOW_EXTERNAL"]) {
		    query["ALLOW_EXTERNAL"] = "";
		} else {
		    del += ([ "ALLOW_EXTERNAL" ]);
		}

		if (query["REGISTERED"]) {
		    query["REGISTERED"] = "";
		} else {
		    del += ([ "REGISTERED" ]);
		}
		
		s = w("_PAGES_configure_head",
		      ([ "_title" : "Owned Room Management" ]));
		s += saveConfig("place/" + lower_case(query["NAME"]) + ".c", query, del);
		break;
	    }
    case "room_owned_edit":
	    {
		mapping rs;
		string name;
		mixed t;

		rs = parseConfig("place/" + lower_case(query["NAME"]) + ".c");
		P2(("===> %O\n", rs))
		unless ((t = isRoomType("OWNED", rs)) == 1) {
		    if (t != -1) {
			s = w("_PAGES_configure_room_error_type", 
			      ([ "_title" : "Manage Owned Rooms",
				 "_name" : query["NAME"],
				 "_expected" : "Owned",
				 "_type" : t
				 ]));
		    } else {
			s = w("_PAGES_configure_room_error_type_undefined",
			      ([ "_title" : "Manage Owned Rooms",
			       "_name" : query["NAME"],
			       "_expected" : "Owned" ]));
		    }
		    break;
		}

		if (rs["NAME"]) {
		    name = replace(rs["NAME"], "\"", "");
		} else {
		    name = query["NAME"] || "";
		}


		s = w("_PAGES_configure_room_owned_edit",
		      ([ "_title" : "Manage Owned Rooms",
			 "_submit_value" : "Save",
			 "_name" : name,
			 "_registered" : member(rs, "REGISTERED") ? " checked" : "",
			 "_owned" : replace(rs["OWNED"] || "", "\"", "&quot;"),
			 "_external" : member(rs, "ALLOW_EXTERNAL") ? " checked" : "" ]));
		break;
	    }   
    case "room_public":
	    s = w("_PAGES_configure_room_public",
		  ([ "_title" : "Manage Public Rooms",
		     "_submit_value" : "Edit / Add" ]));
	    break;
    case "room_public_save":
	    {
		mapping del;

		del = ([ ]);

		m_delete(query, "option");

		if (query["delete"]) {
		    rm("place/" + lower_case(query["NAME"]) + ".c");
		    s = w("_PAGES_configure_room_deleted",
		      ([ "_title" : "Room Deleted",
		         "_name" : query["NAME"] ]));
		    break;
		}

		if (query["HISTORY"]) {
		    query["HISTORY"] = "";
		} else {
		    del += ([ "HISTORY" ]);
		}

		if (query["ALLOW_EXTERNAL"]) {
		    query["ALLOW_EXTERNAL"] = "";
		} else {
		    del += ([ "ALLOW_EXTERNAL" ]);
		}

		if (query["REGISTERED"]) {
		    query["REGISTERED"] = "";
		} else {
		    del += ([ "REGISTERED" ]);
		}
		
		s = w("_PAGES_configure_head",
		      ([ "_title" : "Public Room Management" ]));
		s += saveConfig("place/" + lower_case(query["NAME"]) + ".c", query, del);
		break;
	    }
    case "room_public_edit":
	    {
		mapping rs;
		string name;
		mixed t;

		rs = parseConfig("place/" + lower_case(query["NAME"]) + ".c");
		P2(("===> %O\n", rs))
		unless (intp(t = isRoomType("PUBLIC", rs))) {
		    s = w("_PAGES_configure_room_error_type", 
			  ([ "_title" : "Manage Public Rooms",
			     "_name" : query["NAME"],
			     "_expected" : "Public",
			     "_type" : t
			     ]));
		    break;
		}

		if (rs["NAME"]) {
		    name = replace(rs["NAME"], "\"", "");
		} else {
		    name = query["NAME"] || "";
		}


		s = w("_PAGES_configure_room_public_edit",
		      ([ "_title" : "Manage Public Rooms",
			 "_submit_value" : "Save",
			 "_name" : name,
			 "_history" : member(rs, "HISTORY") ? " checked" : "",
			 "_registered" : member(rs, "REGISTERED") ? " checked" : "",
			 "_external" : member(rs, "ALLOW_EXTERNAL") ? " checked" : "" ]));
		break;
	    }
    case "save_config":
	{

	    s = w("_PAGES_configure_head",
		  ([ "_title" : "Edit Config" ]));
	    s += saveConfig(CONFIGFILE, query);
	    break;
	}
#if 0
// was um alles in der welt soll das?
    case "set":
	    
	m_delete(query, "option");

	break; // gehört da n break hin?
#endif
    case "edit_config":
	{
	    mapping cv;
	    cv = parseConfig(CONFIGFILE);
	// sowas gehört in externe dateien.. am besten .fmt wegen
	// mehrsprachigkeit und layouting und template syntax und und..
	    s = w("_PAGES_configure_head", ([ "_title" : "Edit Config" ]));

	    s += "<div align=\"center\" style=\"float:clear; text-align:center;\">\n"
		"<table bgcolor=\"#004411\" cellspacing=0 cellpadding=3>\n"
		"<form method=GET action=configure name=zero>\n"
		"<input type=hidden name=option value=\"save_config\">\n";

	    foreach(string key, string value : cv) {
		if (value) {
		    value = replace(value, "\"", "&quot;");

		    s += "<tr><td style=\"border-left: 1px solid #f90; border-top: 1px solid #f90;\"><b>" + key + ":</b></td><td style=\"border-top: 1px solid #f90; border-right: 1px solid #f90;\"><input type=\"text\" value=\"" + value 
			 + "\" name=\"" + key + "\"></td></tr>\n"
			"<tr><td colspan=2 style=\"padding-left:10px; border-bottom: 1px solid #fc0; border-right: 1px solid #f90; border-left: 3px solid #fc0;\">" + (description[key] || "") + "</td></tr>\n"
			"<tr><td><span style=\"font-size:1px;\">&nbsp;</span></td></tr>\n";
		}
	    }

	    s += "<tr><td colspan=2 align=center><p>&nbsp;</p>";
	    s += w("_PAGES_configure_submit",
		   ([ "_submit_value" : "Save" ]));
	    s += "</td></tr>\n"
		 "</form></table>\n"
		 "</div>\n";

	    break;
	}
    default:
	s = w("_PAGES_configure_menu", ([ "_title" : "Menu" ]));
    }

    write(s);
}

#else

htget(prot, query, headers, qs) {
    hterror(prot, 501, "This server hasn't enabled webbased administration");
}

#endif

isRoomType(expected, mapping rs) {
    string type;

    unless (sizeof(rs)) {
	return 1;
    }
    
    foreach (type : filter(({ "OWNED", "MODERATED", "NEWSFEED_RSS",
				    "SLAVE", "CONNECTED_IRC", "THREADS",
				    "GAMESERV", "LINK", "PUBLIC" }), 
		 lambda( ({ 'x }), ({#'?, ({ #'!=, 'x, expected }) })) )) {
	if(member(rs, type))
		    return type[0..0] + lower_case(type[1..]);
    }
    if (member(rs, expected)) {
	return 1;
    }
    
    return -1;
}
 
w(mc, mixed vars, bla) {
    if (bla) {
	vars = bla;
    }

    if (mc == "_list_user_technical") {
	unless (mappingp(vars))
		vars = ([ ]);
	write(psyctext(T(mc, ""), vars));
	return;
    }

    unless (mappingp(vars)) {
	return T(mc, "");
    }

    return psyctext(T(mc, ""), vars);
}

// only used for HOSTS_D
pr(mc, text, vars) {
    string s, ip, reason;

    // we'll keep it this way, yet
    switch (mc) {
	case "_list_hosts_disabled":
		s = "";
		foreach (ip, reason: vars) {
		    s += w(mc,
			   ([ "_ip" : ip,
			      "_reason" : reason ]));
		}
    }

    write(s);
}

#endif
