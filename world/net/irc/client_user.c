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
  // TODO: ipv6, password, username
  mapping autocmds;
  int tls;
};

mapping servers = ([]);
mapping server_connections = ([]);
string owner;

void create_connection(string id,struct server_s server,status connect) {
    server_connections[id] = clone_object("/net/irc/client_connection.c");
    server_connections[id]->set_parameters(
    (["host":server->host
      ,"port":server->port
      ,"nick":server->nick
      ,"autocmds":server->autocmds
      ,"master":this_object()
      ,"id":id
      ,"tls":server->tls
    ]));
    server_connections[id]->set_owner(owner);
    connect&&server_connections[id]->connect();
}

varargs void create_connections(status connect) {
  map(servers,#'create_connection,connect);
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
#   define IRC_SCHEME_ANSWER(s) do { sendmsg(owner_o,"_message_private",(s),([ "_nick": "irc:" ])); } while (0)
#   define IRC_SCHEME_SHOW_SERVER(s,s_s) \
          map(explode(sprintf("%O",s_s),"\n")[1..<2] \
            ,function void(string e) { \
              IRC_SCHEME_ANSWER(sprintf("%O",e)[1..<2]); \
          }); \
          IRC_SCHEME_ANSWER("  / current state:"); \
          IRC_SCHEME_ANSWER(sprintf("  / object: %O",server_connections[s]));\
          IRC_SCHEME_ANSWER("  / interactive: "+(server_connections[s]?interactive(server_connections[s]):"no connection")); \
          IRC_SCHEME_ANSWER("  / logged in: "+(server_connections[s]?server_connections[s]->query_connected()+" (0: disconnected, <0: pending, >0 connected)":"no connection")); \
          IRC_SCHEME_ANSWER("  / tls_query_connection_state: "+(server_connections[s]?tls_query_connection_state(server_connections[s]):"no connection"));

    /*
     * I want a interface to configure the irc: scheme like servers and so on...
     */
#    define IRC_SCHEME_HELPTEXT \
    ({" help"\
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
     ," set <server> <property> <values>"\
     ,"        changes settings for the given keywords"\
     ,"        set bla autocmds 1 privmsg nickserv :identfy mysecret"\
     ,"        set foo host foo.example.com"\
     ,""\
     ," add <server> <property> <key> <values>"\
     ,"        currently just adds a autocmd, i. e. to identify after 3 s"\
     ,"        add bla autocmds 3 privmsg nickserv :identify foo"\
     ,"        note: you can't add two commands in the same second"\
     ,""\
     ," del <server> <property> <key>"\
     ,"        currently just delete the autocmd at the given second"\
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
          case "help":
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
              IRC_SCHEME_SHOW_SERVER(s,s_s);
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
            IRC_SCHEME_ANSWER("if you really want to reconnect *all* your connections, type reconnect -f");
            break;
          case "reconnect -f":
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
              IRC_SCHEME_SHOW_SERVER(data[5..],servers[data[5..]]);
              return 1;
            } else if ("add "==data[0..3]) {
              string *sa=explode(data," ");
              int sas=sizeof(sa);
              if (2==sas) {
                if (servers[data[4..]]) {
                  IRC_SCHEME_ANSWER("sawwry, this serverid already exists: "+data[4..]);
                  return 0;
                }
                servers[data[4..]]=(<server_s>tls:1,port:6697,nick:owner,host:data[4..]);
                IRC_SCHEME_ANSWER(data[4..]+" added");
                return 1;
              } else if (4<sas) {
                switch (sa[2]) {
                  case "autocmds":
                    int key=to_int(sa[3]);
                    if (1>key) {
                      IRC_SCHEME_ANSWER("1 has to be greater than "+sa[3]);
                      return 1;
                    }
                    if (servers[sa[1]]->autocmds) {
                      if (servers[sa[1]]->autocmds[key])
                        IRC_SCHEME_ANSWER("overwritten "+key+" with "+servers[sa[1]]->autocmds[key]);
                      else
                        IRC_SCHEME_ANSWER("added autocmd");
                      servers[sa[1]]->autocmds+=([key:implode(sa[4..]," ")]);
                    } else {
                      servers[sa[1]]->autocmds=([key:implode(sa[4..]," ")]);
                      IRC_SCHEME_ANSWER("set autocmd");
                    }
                    break;
                  default:
                    IRC_SCHEME_ANSWER(sa[2]+" is currently not a writeable setting");
                }
                return 1;
              } else {
                IRC_SCHEME_ANSWER("hint: add <server> <property> <key> <values>");
                IRC_SCHEME_ANSWER("or:   add <server>");
                return 1;
              }
            } else if ("del "==data[0..3]) {
              string *sa=explode(data," ");
              int sas=sizeof(sa);
              if (!servers[sa[1]]) {
                IRC_SCHEME_ANSWER(sa[1]+" does not exits, add it for example or try again in a minute");
                return 1;
              }
              if (1==sas) {
                m_delete(servers,data[4..]);
                IRC_SCHEME_ANSWER("if "+data[4..]+" was there, now it's gone");
                return 1;
              } else if (4==sas) {
                int key=to_int(sa[3]);
                if (servers[sa[1]]->autocmds && servers[sa[1]]->autocmds[key]) {
                  m_delete(servers[sa[1]]->autocmds,key);
                  IRC_SCHEME_ANSWER("deleted key");
                } else {
                  IRC_SCHEME_ANSWER("couldn't find key");
                  return 1;
                }
              } else {
                IRC_SCHEME_ANSWER("hint: del <server> <property> <key>");
                IRC_SCHEME_ANSWER("or:   del <server>");
              }
              return 1;
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
                case "tls":
                case "port":
                  IRC_SCHEME_ANSWER(sa[2]+" for "+sa[1]+" is now: "+(int)sa[3]+" was: "+servers[sa[1]]->(sa[2])); 
                  servers[sa[1]]->(sa[2])=(int)sa[3];
                  break;
                case "host":
                case "nick":
                  IRC_SCHEME_ANSWER(sa[2]+" for "+sa[1]+" is now: "+sa[3]+" was: "+servers[sa[1]]->(sa[2])); 
                  servers[sa[1]]->(sa[2])=sa[3];
                  break;
                case "autocmds":
                  int key=to_int(sa[3]);
                  if (1>key) {
                    IRC_SCHEME_ANSWER("1 has to be greater than "+sa[3]);
                    return 1;
                  }
                  servers[sa[1]]->autocmds=([key:implode(sa[4..]," ")]);
                  break;
              default:
                  IRC_SCHEME_ANSWER(sa[2]+" is currently not a writeable setting");
              }
              return 1;
            } else if ("app "==data[0..3]||"append "==data[0..6]) {
            } else if ("del "==data[0..3]) {
            } else if ("reconnect "==data[0..9]) {
              IRC_SCHEME_ANSWER("trying to reconnect "+data[10..]);
              if (objectp(server_connections[data[10..]])) {
                IRC_SCHEME_ANSWER(sprintf("destructing %O",server_connections[data[10..]]));
                destruct(server_connections[data[10..]]);
              }
              if (!servers[data[10..]]) {
                IRC_SCHEME_ANSWER(data[10..]+" does not exist");
                return 1;
              }
              create_connection(data[10..],servers[data[10..]],1);

              return 1;
            }
            IRC_SCHEME_ANSWER("What?");
            return 0;
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

