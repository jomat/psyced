/* IRC client for psyced
 *
 * URI format described here: http://about.psyc.eu/IRC_URI
 *
 * irc:<destination>@<authority>
 *
 *  IRC-URI-reference := "irc" ":" opaque-string
 *  opaque-string     := access | identity
 *
 *  access            := hier_part  ; see RFC 2396 and other IRC URI specs
 *  identity          := destination "@" authority  
 *  destination       := "~" nick | channel_prefix channel | reserved_prefix reserved_destination
 *  nick              := 1*(unreserved | escaped)
 *                       ; Note: Following characters in nicknames (as defined in RFC 2812) 
 *                       ; can only be used in URI nicknames in the escaped form:
 *                       ; "[" | "]" | "\" | "`" | "^" | "{" | "|" | "}"
 *                       ; See also the note about allowed characters in nicks below.
 *
 *   channel           := 1*(unreserved | escaped)
 *                        ; Note: All special characters in the channelname (which are not unreserved) 
 *                        ; need to be escaped.
 *                        ; See also the note about allowed characters in channels below.
 *
 *   channel_prefix    := "*" | "!" | "+" | "&"
 *                        ; These are the currently defined channel prefixes. 
 *                        ; Note: The prefix "*" is mapped to the irc channel prefix "#"
 *
 *   reserved_destination := 1*(unreserved | escaped)
 *   reserved_prefix   := alphanum | prefix_mark | ";" | "?" | ":" | "@" | "=" | "$" | ","
 *                        ; reserved_prefix is basically uric (as defined in RFC 2396) 
 *                        ; except "%" and "/" and the characters in channel_prefix
 *
 *   prefix_mark       := "-" | "_" | "." | "'" | "(" | ")"
 *
 *   ; For definition of alphanum see RFC 2396.
 *   ; For definition of unreserved and escaped see RFC 2396.
 *   ; For definition of the authority part see RFC 2396. The authority part should
 *   ; be a host of the IRC network or some other authority which allows you to reach the destination
 *
 *
 *
 * Some notes:
 * I'm testing with irssi and weechat as psyced client mostly, so if you want to try out this stuff
 * it'd be a good idea to use an irc client at least at first. Telnet could work sometimes, too.
 * I know that it's a little bit unusable with psi as xmpp client, because you can't add irc:~nick@server
 * jids with it... TODO
 * 
 * How this thing is constructed:
 * client.c will register the irc: scheme. Incoming psyc messages are passed to
 * client_user.c#username for each user
 * client_user.c#username manages a bunch of client_connection.c, each one of them logs in as
 * user to the irc server
 *
 * Some TODOs and what won't work:
 *  a lot of messages (QUIT, NOTICE, PART, …)
 *
 * How this thing is used:
 *  set up your servers: /query irc: help
 *  save your servers: /query irc: save
 *  reconnect the connections: /query irc: reconnect
 *  either/or
 *   join a room like irc:*roomname@serverid (replace the # of #roomname by a *)
 *   query some user like irc:~username@serverid
 */
#include <debug.h>
#include "client.h"
#pragma no_clone

void create() {
  register_scheme("irc");
# if ENABLE_SILC
  register_scheme("silc");
# endif /* if ENABLE_SILC */
}

varargs int msg(string source, string mc, string data, mapping vars, int showingLog, mixed target) {
  P4(("IRC client.c %O got a message----------------------\nsource %O\n"                                                               
        "--------mc %O\n--------data %O\n--------vars %O"    
        "\n--------showingLog %O\n--------target %O\n--------\n"
        ,this_object(),source,mc,data,vars,showingLog,target)); 

  string nick=vars["_nick"];
  if (!nick)
    if (source)
      sscanf(sprintf("%O",source),"%~s#%s",nick);
    else {
      P1(("%O unknown nick in msg\n"));
      return 0;
   }

  object client=find_object(object_name()+"_user.c#"+nick);
  if (!client) {
    named_clone("/net/irc/client_user.c",nick);
    client=find_object(object_name()+"_user.c#"+nick);
    client->set_parameters((["owner":nick]));
  }
  client->msg(source,mc,data,vars,showingLog,target);
  return 0;
}
