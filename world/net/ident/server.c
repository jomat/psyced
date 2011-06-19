// http://tools.ietf.org/html/rfc1413
#include <net.h>
#include <server.h>
#include "ident.h"
#define IDLE_TIMEOUT 60  // 60-180 is recommended

void timeout() {
  destruct(this_object());
}

void parse_input_delayed(string s) {
  P2(("identd input %s\n",s));
  next_input_to(#'parse_input_delayed);
  while (remove_call_out(#'timeout) != -1);
  call_out(#'timeout,IDLE_TIMEOUT);
  int lport, rport;

  if(4!=sscanf(s,"%d%~s,%~s%d",lport,rport)) {
    timeout();
    return;
  }

  string id;
  if (!(id=IDENT_MASTER->query_connection_id(lport,rport))) {
    emit(sprintf("%d,%d:ERROR:NO-USER\n",lport,rport)); // TODO: there could be other errors
    return;
  }

  emit(sprintf("%d,%d:USERID:UNIX:%s\n",lport,rport,id));  // choose one: http://tools.ietf.org/html/rfc1340#page-87
}

// looks like there happened some kind of race condition a la
// the remote connects to identd while the connection isn't registered
// yet
void parse_input(string s) {
  call_out(#'parse_input_delayed,1,s);
}

int logon (int flag) {
  P2(("identd connection\n"));
  next_input_to(#'parse_input);
  call_out(#'timeout,IDLE_TIMEOUT);
  return 1;
}
