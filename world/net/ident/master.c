// TODO:
// shouldn't answer connection infos for foreign connections
// check if it really answers 0 if the connection is already gone
#pragma no_clone
#include <debug.h>

/* this mapping holds the infos for outgoing connections stored:
 * ([ int remote_ports:
 *     ([ int local_ports:
 *         ([ string id: "name"
 *           ,object conn: connection object ]) ]) ]);
 */
mapping ports=([]);

varargs void update_connection(int rport, string id, int lport, object conn) {
  if (!conn)
    conn=previous_object();

  if (!lport)
    lport=query_mud_port(conn);

  if(!ports[rport])
    ports[rport]=([]);
  if(!ports[rport][lport])
    ports[rport][lport]=([]);

  P2(("adding ident connection rport %O lport %O id %O conn %O\n",rport,lport,id,conn));

  ports[rport][lport]["id"]=id;
  ports[rport][lport]["conn"]=conn;
}

string query_connection_id(int lport, int rport) {
  if (!ports[rport]
      || !ports[rport][lport]
      || !ports[rport][lport]["conn"])
    return 0;
  return ports[rport][lport]["id"];
}

m(){return ports;}
