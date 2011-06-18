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
  object connection; // this points to the connection object
  // TODO: ssl, ipv6, password, username
};

mapping servers = ([]);
string owner;

void create() {
  if (!clonep()) // we _must_ be a named clone for each user
    return;

# if 1
  // TODO: store servers in user object and provide interface to edit it
  servers=([/* "blafasel": (<server_s> host:"ray.blafasel.de",port:6667,nick:"jmt")
           ,*/ "blafasel": (<server_s> host:"space.blafasel.de",port:6667,nick:"jmt") ]);
# endif

  // create connections to all known servers
  map(servers, function void(string id, struct server_s server) {
    server->connection = clone_object("/net/irc/client_connection.c");
    server->connection->set_parameters(
    (["host":server->host
      ,"port":server->port
      ,"nick":server->nick
      ,"master":this_object()
      ,"id":id
    ]));
  });
}

void set_parameters(mapping m) {
  owner=m["owner"];
  map(servers,function void(string id, struct server_s server) {
    server->connection->set_owner(owner);
    server->connection->connect();
  });
}

varargs int msg(string source, string mc, string data, mapping vars, int showingLog, mixed target) {
  P2(("IRC client user got a message----------------------source\n%O\n"                                           
        "--------mc\n%O\n--------data\n%O\n--------vars\n%O"
        "\n--------showingLog\n%O\n--------target\n%O\n--------\n"
        ,source,mc,data,vars,showingLog,target));
  string id=explode(target?target:vars["_nick_place"],"@")[1];
  if (!servers[id]) {  // TODO: I could try to parse the URI better to add servers automatically or whatever...
    sendmsg(source, "_error_unknown_name_user",    // TODO: should i think of something else?
      "Server ID [_nick_target] unknown.",
      ([ "_nick_target" : id ]));
    return 1;
  }
  return servers[id]->connection->msg(source,mc,data,vars,showingLog,target);
}

struct server_s query_server(string id) {
  return servers[id];
}

void irc_quote(string id,string what) {
  servers[id]->connection->irc_quote(what);
}

