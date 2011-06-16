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
 * client.c will register a scheme. Incoming psyc messages are passed to
 * client_user.c#username for each user
 * client_user.c#username manages a bunch of client_connection.c, each one of them logs in as
 * user to the irc server
 */
#include <debug.h>
#pragma no_clone

void create() {
  register_scheme("irc");
}

varargs int msg(string source, string mc, string data, mapping vars, int showingLog, mixed target) {
  object client=find_object(object_name()+"_user.c#"+vars["_nick"]);
  if (!client) {
    named_clone("/net/irc/client_user.c",vars["_nick"]);
    client=find_object(object_name()+"_user.c#"+vars["_nick"]);
    client->set_parameters((["owner":vars["_nick"]]));
  }
  client->msg(source,mc,data,vars,showingLog,target);
  return 0;
}
