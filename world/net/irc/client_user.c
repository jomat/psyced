/* IRC client for psyced
 * see net/irc/client.c for details
 */
#include <debug.h>

// all the known servers for this user are stored in the servers mapping.
// The key is the server id (usually it's hostname and eventually port) and
// the value of the type struct server_s
struct server_s {
  string host;
  int port;
  string nick;
  // TODO: ssl, ipv6, password, username
};

mapping servers = ([]);
mapping server_connections = ([]);
string owner;

varargs void create_connections(status connect) {
  map(servers, function void(string id, struct server_s server) {
    server_connections[id] = clone_object("/net/irc/client_connection.c");
    server_connections[id]->set_parameters(
    (["host":server->host
      ,"port":server->port
      ,"nick":server->nick
      ,"master":this_object()
      ,"id":id
    ]));
    server_connections[id]->set_owner(owner);
    connect&&server_connections[id]->connect();
  });
}

void create() {
  if (!clonep()) // we _must_ be a named clone for each user
    return;
  sscanf((string)this_player(),"%~s#%s",owner);

  P2(("owner of %O is %O\n",this_object(),owner));
  if (catch(servers=restore_value(find_person(owner)->vQuery("_irc_servers")))) {
    P1(("couldn't load irc servers\n"));
    servers=([]);
  }

  // create connections to all known servers
  create_connections();
}

void set_parameters(mapping m) {
  owner=m["owner"];
  map(servers,function void(string id, struct server_s server) {
    server_connections[id]->set_owner(owner);
    server_connections[id]->connect();
  });
}

varargs int msg(string source, string mc, string data, mapping vars, int showingLog, mixed target) {
  P2(("IRC client user got a message----------------------source\n%O\n"                                           
        "--------mc\n%O\n--------data\n%O\n--------vars\n%O"
        "\n--------showingLog\n%O\n--------target\n%O\n--------\n"
        ,source,mc,data,vars,showingLog,target));
  if ("irc:"==target||"irc:"==vars["_nick_target"]) { // TODO: the scheme...
    object owner_o=find_person(owner);
#   define IRC_SCHEME_ANSWER(s) sendmsg(owner_o,"_message_private",s,([ "_nick": "irc:" ]));
#   define IRC_SCHEME_SHOW_SERVER(s) \
          map(explode(sprintf("%O",s),"\n")[1..<2] \
            ,function void(string e) { \
              IRC_SCHEME_ANSWER(sprintf("%O\n",e)); \
            });
    /*
     * I want a interface to configure the irc: scheme like servers and so on...
     */
#    define IRC_SCHEME_HELPTEXT \
    ({" help"\
     ," ?"\
     ,"        shows this help"\
     ,""\
     ," list"\
     ," ls"\
     ,"        lists your servers"\
     ,""\
     ," list long"\
     ," ll"\
     ,"        detailed listing of your servers"\
     ,""\
     ," show <server>"\
     ,"        shows details for just one server"\
     ,""\
     ," add <server>"\
     ,"        adds a new server id. Set it up"\
     ,"        with set <server> <property> <value>"\
     ,"        some default values might be set, look out"\
     ,""\
     ," del <server>"\
     ,"        deletes the server with all the settings"\
     ,""\
     ," set <server> <property> <value>"\
     ,"        changes settings for the given keywords"\
     ,""\
     ," save"\
     ,"        saves your server settings permanently"\
     ,""\
     ," load"\
     ,"        loads server settings."\
     ,"        be careful, any unsaved changes will be overwritten"\
    })
    switch (mc) {
      case "_message_private":
        sendmsg(source,"_message_echo_private",data,vars);
        switch (data) {
          case "jomat":
            return 1;
          case "help":
          case "?":
            map(IRC_SCHEME_HELPTEXT,function void(string s) {
              IRC_SCHEME_ANSWER(s);
            });
            break;
          case "list":
          case "ls":
            IRC_SCHEME_ANSWER("Your current serverids: "
              +implode(m_indices(servers),", "));
            break;
          case "list long":
          case "ll":
            IRC_SCHEME_ANSWER("Your servers en detail:");
            map(servers,function void(string s,struct server_s s_s) {
              IRC_SCHEME_ANSWER("ID "+s);
              IRC_SCHEME_SHOW_SERVER(s_s);
            });
            break;
          case "save":
            find_person(owner)->vSet("_irc_servers",save_value(servers));
            IRC_SCHEME_ANSWER("servers saved");
            break;
          case "load":
            if (catch(servers=restore_value(find_person(owner)->vQuery("_irc_servers")))) {
              IRC_SCHEME_ANSWER("servers not loaded, try to save some first");
            } else {
              IRC_SCHEME_ANSWER("servers loaded");
            }
            break;
          case "show":
            IRC_SCHEME_ANSWER("hint: show <server>");
            break;
          case "reconnect":
            //TODO: hmm... don't be so cruel:
            map(server_connections,function void(string id,object o) {
              destruct /* see the sad smiley? → */    (o);
            });  /* ← anotherone */
            create_connections(1);
            break;
          default:
            if ("show "==data[0..4]) {
              if (!servers[data[5..]]) {
                IRC_SCHEME_ANSWER("no such server: "+data[5..]);
                return 1;
              }
              IRC_SCHEME_SHOW_SERVER(servers[data[5..]]);
            } else if ("add "==data[0..3]) {
              if (servers[data[4..]]) {
                IRC_SCHEME_ANSWER("sawwry, this serverid already exists: "+data[4..]);
                return 1;
              }
              servers[data[4..]]=(<server_s>port:6667,nick:owner);
              IRC_SCHEME_ANSWER(data[4..]+" added");
            } else if ("del "==data[0..3]) {
              m_delete(servers,data[4..]);
              IRC_SCHEME_ANSWER("if "+data[4..]+" was there, now it's gone");
            } else if ("set "==data[0..3]) {
              string *sa=explode(data," ");
              if (sizeof(sa)<3) {
                IRC_SCHEME_ANSWER("hint: set <server> <property> <value>");
                return 1;
              }
              if (!servers[sa[1]]) {
                IRC_SCHEME_ANSWER(sa[1]+" does not exits, add it for example");
                return 1;
              }
              switch (sa[2]) {
                case "host":
                case "port":
                case "nick":
                  IRC_SCHEME_ANSWER(sa[2]+" for "+sa[1]+" is now: "+sa[3]+" was: "+servers[sa[1]]->(sa[2])); 
                  servers[sa[1]]->(sa[2])=sa[3];
                  break;
                default:
                  IRC_SCHEME_ANSWER(sa[2]+" is currently not a writeable setting");
              }
            }
        }
        break;
      case "_request_friendship":
        sendmsg(source,"_notice_friendship_established","Friendship established",vars);
        break;
    }
    return 1;
  }

  if ("_message_echo_private"==mc && "irc:"==vars["_nick"]) {
    return 1;
  }

  string id;
  if (catch(id=explode(target?target:vars["_nick_place"],"@")[1])) {
    sendmsg(source, "_error_unknown_name_user",    // TODO: should i think of something else?
      "Server ID [_nick_target] unknown to irc scheme.",
      ([ "_nick_target" : target?target:vars["_nick_place"] ]));
    return 1;
  }
  if (!servers[id]) {  // TODO: I could try to parse the URI better to add servers automatically or whatever...
    sendmsg(source, "_error_unknown_name_user",    // TODO: should i think of something else?
      "Server ID [_nick_target] unknown to irc scheme.",
      ([ "_nick_target" : id ]));
    return 1;
  }
  return server_connections[id]->msg(source,mc,data,vars,showingLog,target);
}

struct server_s query_server(string id) {
  return servers[id];
}

void irc_quote(string id,string what) {
  server_connections[id]->irc_quote(what);
}

