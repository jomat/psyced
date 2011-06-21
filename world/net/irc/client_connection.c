/* IRC client for psyced
 * see net/irc/client.c for details
 */

#include <debug.h>
#include <interface.h>
#include <input_to.h>
#include "/local/config.h"
#include "/net/ident/ident.h"

virtual inherit NET_PATH "output";

struct server_s { 
  string id;
  string host; 
  int port; 
  string nick; 
  object master;  // this points to the master (named clone of client_user.c)
  // TODO: ssl, ipv6, password, username 
  int connected;  // < 0 connecting, 0 not connected, >0 connected
                  // not to be confused with interactive() for the socket, this is the irc layer
  string owner;
};

struct channel_s {
  mapping users; /* ([ "foo":(<struct user_s>) ]) */
  string tag;
  string key;  // TODO: yeah, these keys have to be set and perhaps even saved,
               //       nothing implemented right now, but if they are set, it could work
};

struct user_s {
  int prefix;
  // TODO: modes, …
};

int last_message_timestamp;
struct server_s server;
mapping channels=([]);  /* ([ "#foo":(<struct channel_s>) ]) */

/*
int emit(string m) {
  P2(("IRC emitting to %s: %s\n",server->host,m));
  return ::emit(m);
}
*/

void update_nick(string nick, static string chan) {
  if (!chan)
    return;
  if (!structp(channels[chan]))
    channels[chan]=(<channel_s>users:([]));
  if ('+'==nick[0]||'@'==nick[0])
    channels[chan]->users[nick[1..]]=(<user_s>prefix:nick[0]);
  else
    channels[chan]->users[nick]=(<user_s>);
}

void set_owner(string s) {
  server->owner=s;
}

void set_parameters(mapping m) {
  server=(<server_s>
    host:m["host"]
    ,port:m["port"]
    ,nick:m["nick"]
    ,id:m["id"]
    ,master:m["master"]
    ,owner:m["owner"]
  );
}

int irc_privmsg(string msgtarget, string message) {
  // PRIVMSG <msgtarget> <message>
  // Sends <message> to <msgtarget>, which is usually a user or channel.[28]
  // Defined in RFC 1459
  // TODO: sanity checks

  emit("PRIVMSG "+msgtarget+" :"+message+"\n");
  return 0;
}

// try to join _one_ channel with an optional key
varargs int irc_join(string channel, string tag, string key) {
  // JOIN <channels> [<keys>]
  // Makes the client join the channels in the comma-separated list <channels>, specifying the passwords, if needed, in the comma-separated list <keys>. If the channel(s) do not exist then they will be created.
  // Defined in RFC 1459
  if (1>server->connected) {
    call_out(#'irc_join,1,channel,tag,key);
    P2(("not connected to %s, not joining the channel\n",server->host));
    return 2;
  }
  P2(("join irc channel %s on %s\n",channel,server->host));
 
  if (!channel) {
    P2(("no channel specified\n"));
    return 1;
  }

  if (key)
    emit(sprintf("JOIN %s %s\n",channel,key));
  else
    emit(sprintf("JOIN %s\n",channel));
  if (structp(channels[channel]))
    channels[channel]->tag=tag;
  else
    channels[channel]=(<channel_s> tag:tag,users:([]));
  return 0;
} 

int irc_nick(string nick) {
  // NICK <nickname> [<hopcount>] (RFC 1459)
  // NICK <nickname> (RFC 2812)
  // Allows a client to change their IRC nickname. Hopcount is for use between servers to specify how far away a nickname is from its home server.[20][21]
  // Defined in RFC 1459; the optional <hopcount> parameter was removed in RFC 2812
  if (!interactive()) {
    call_out(#'irc_nick,1,nick);
    return 0;
  }
 
  if (!nick) {
    P2(("no nick specified\n"));  // TODO....
    return 1;
  }

  emit(sprintf("NICK %s\n",nick));
}

int irc_user(string user, string mode, string realname) {
  // USER <username> <hostname> <servername> <realname> (RFC 1459)
  // USER <user> <mode> <unused> <realname> (RFC 2812)
  // This command is used at the beginning of a connection to specify the username, hostname, real name and initial user modes of the connecting client.[43][44] <realname> may contain spaces, and thus must be prefixed with a colon.
  // Defined in RFC 1459, modified in RFC 2812
  if (!interactive()) {
    call_out(#'irc_user,1,user,mode,realname);
    return 0;
  }
 
  if (!user||!mode||!realname) {
    P2(("not enough parameters specified\n")); // TODO...
    return 1;
  }

  emit(sprintf("USER %s %s 0 :%s\n",user,mode,realname));
}

int connect() {
  while (remove_call_out(#'connect) != -1);

  if (net_connect(server->host,server->port)) {
    P3(( "couldn't connect to server, reconnecting in 10 seconds...\n"));  // TODO...
    call_out(#'connect,10);
    return 1;
  }
}

varargs void show_members(mixed whom,string room,mixed vars) {
  if (vars&&"W"==vars["_tag"]) {
    // BIG FAT TODO:
    // if a client sends a WHO the _tag will be set to "W"
    // if a _status_place_members is sent like here, a RPL_NAMREPLY
    // is sent to the client, but the correct answer would be a RPL_WHOREPLY
    //
    // some clients (like weechat) periodically issue WHOs, and RPL_NAMREPLY
    // are scrolling live in the chanwindow and are quite annoying
    return;
  }
  
  if (!structp(channels[room])) {
    // no data available for the channel
    // TODO: think if this can happen and if it'd be useful to ask for the names
    return;
  }
  string *members=({});
  string *nicks=({});
  map(channels[room]->users,function void(string nick,struct user_s user,string *members,string *nicks) {
    //members+=({"irc:~"+nick+"@"+server->id});   // TODO: configure user display
    members+=({user->prefix?sprintf("%c%s",user->prefix,nick):nick /* TODO: i think it would be nicer in RPL_WHOREPLY or so … */});
    nicks+=({nick});
  },&members,&nicks);
  if ('#'==room[0])
    room[0]='*';
  sendmsg
    (whom
    ,"_status_place_members"
    ,"In [_nick_place]: [_list_members_nicks]."
    ,(["_nick_place":"irc:"+room+"@"+server->id
      ,"_target":"irc:"+room+"@"+server->id
      ,"_list_members":members
      ,"_list_members_nicks":nicks
  ]));
}

int parse_answer(string s) {
  P2(("Parsing: %O\n",s));
  if (s[0..4] == "PING ") {
    emit("PONG "+s[5..]+"\n");
    return 0;
  }

  P2(("owner ist %O\n",server->owner));
  string origin, command;
  sscanf(s,"%s %s %~s",origin,command);
  switch (command&&upper_case(command)) {
    case 0:
      return 0;
    case "JOIN":
      // :ehlo!ehlo@dionisos.jmt.gr JOIN :#jomat
      P2(("irc server wants us to join a room\n"));
      // TODO: sanity checks
      // :_target psyc://host1/~user
      // :_context  psyc://host2/@place
      //
      // :_tag  somethingwicked
      // :_nick user
      // =_nick_place place
      // =_description   This is a great place to be.
      // _echo_place_enter
      // You have entered [_nick_place], [_nick]. [_description]
      // .
      string where,msg,from_nick;
      sscanf(s,":%s!%~s %~s :%s",from_nick,where);

      if(!where||!from_nick)
        return 1;

      string tag=structp(channels[where])?channels[where]->tag:"";;

      if ('#'==where[0])
        where[0]='*';

      P2(("trying to enter %s\n", "irc:"+where+"@"+server->id));
      // sendmsg(target, mc, data, vars, source, showingLog, callback)
      sendmsg
        (find_person(server->owner) /*server->master*/
        ,"_echo_place_enter"
        ,"welcome to this room"
        ,(["_nick_place": "irc:"+where+"@"+server->id
          ,"_context": "irc:"+where+"@"+server->id
          ,"_nick":server->owner  //"irc:~"+server->nick+"@"+server->id
          ,"_tag":tag
      ]));
      return 0;
    case "PRIVMSG":
      //varargs mixed sendmsg(mixed target, string mc, mixed data, mapping vars,
      //          mixed source, int showingLog, closure callback, varargs array(mixed) extra);
      // TODO... yeah

      // :jomat!~jomat@lethe.jmt.gr PRIVMSG #jomat :alarmowitsch
      // :irc:*maeh@foo.bar!*@* PRIVMSG #irc:~oha@foo.bar :#jomat :aaaaaaaaaaaaaa
      string where,where_o,msg,from_nick;
      sscanf(s,":%s!%~s %~s %s :%s",from_nick,where,msg);
      if ('#'==(where_o=where)[0])
        where[0]='*';

      if (where==server->nick)
        sendmsg(find_person(server->owner),"_message_private",msg,([ "_nick": "irc:~"+from_nick+"@"+server->id ]));
      else {
        sendmsg(find_person(server->owner),"_message_public",msg
          ,([ "_nick_place": "irc:"+where+"@"+server->id
          ,"_nick":from_nick ]));
        //sendmsg(find_person(server->owner),"_message_public",msg
        //  ,([ "_nick_place": "irc:"+where+"@"+server->id
        //  ,"_nick":"irc:~"+from_nick+"@"+server->id ]));  // TODO: make it configurable
        //sendmsg(find_person(server->owner),"_message_public",msg,
        //  (["_nick_place":"irc:"+where+"@"+server->id
        //   ,"_nick":channels[where_o]->users[from_nick]->prefix
        //     ?sprintf("%c%s",channels[where_o]->users[from_nick]->prefix,from_nick)
        //     :from_nick
        //]));  // dirty hack to add the prefix to the nick
      }
      return 0;
    // http://www.alien.net.au/irc/irc2numerics.html
    case "001":
      // RPL_WELCOME   RFC2812
      // :Welcome to the Internet Relay Network <nick>!<user>@<host>
      // The first message sent after client registration. The text used varies widely 
      server->connected=1;
      // channel tag key
      map(channels,function void(string id, struct channel_s channel) {
        irc_join(id,channel->tag,channel->key);
      });
      return 0;
    case "353":
      /*
       * 353     RPL_NAMREPLY
       *                 "<channel> :[[@|+]<nick> [[@|+]<nick> [...]]]"
       *                 :ray.blafasel.de 353 jnick = #jchan :jnick @jmt
       * 366     RPL_ENDOFNAMES
       *                 "<channel> :End of /NAMES list"
       *                 :space.blafasel.de 366 jmt #ccc :End of /NAMES list.
       *
       *         - To reply to a NAMES message, a reply pair consisting
       *           of RPL_NAMREPLY and RPL_ENDOFNAMES is sent by the
       *           server back to the client.  If there is no channel
       *           found as in the query, then only RPL_ENDOFNAMES is
       *           returned.  The exception to this is when a NAMES
       *           message is sent with no parameters and all visible
       *           channels and contents are sent back in a series of
       *           RPL_NAMEREPLY messages with a RPL_ENDOFNAMES to mark
       *           the end.
       */
      string chan,nicks;
      sscanf(s,":%~s %~s %~s %~s %s :%s",chan,nicks);
      map(explode(nicks," "),#'update_nick,chan);
      return 0;
    case "366":
      show_members(find_person(server->owner),explode(s," ")[3]);
      return 0;
    case "433":
      // ERR_NICKNAMEINUSE   RFC1459
      // <nick> :<reason>
      // Returned by the NICK command when the given nickname is already in use 
      // TODO: inform the user
      server->nick+="_";
      irc_nick(server->nick);
      return 0;
    default:
      sendmsg(find_person(server->owner),"_message_private",s,([ "_nick": "irc:@"+server->id ]));
  }
  return 1;
}

void disconnected(string remaining) {
  server->connected=0;
  P2 (( "%O disconnected from IRC server, reconnecting in 10 seconds\n", this_object() ));  // TODO...
  call_out(#'connect,10);
}

/*
 * each time the irc server says something, this function is called
 */
int server_answer(string s) {
  last_message_timestamp=time();

  // Occasionally the server seems to send more than one command at once
  // TODO: have an eye on it if it's really needed
  map(explode(s,"\n"),function int(string msg) {
    return parse_answer(msg);
  });
  next_input_to(#'server_answer);
  return 1;
}

void create() {
  if (!clonep())
    return;
}

string parse_destination(string target) {
  if (!target)
    return 0;

  string destination;
  sscanf(target,"irc:%s@%+~s",destination);
  // Remember: "Note: The prefix "*" is mapped to the irc channel prefix "#""
  if ('*'==destination[0])
    destination[0]='#';
  else if ('~'==destination[0])
    destination=destination[1..];
  return destination;
}

// the user want's to say something to the irc:
varargs int msg(string source, string mc, string data, mapping vars, int showingLog, mixed target) {
  P2(("IRC client connection got a message----------------------\nsource %O\n"                                           
        "--------mc %O\n--------data %O\n--------vars %O"     
        "\n--------showingLog %O\n--------target %O\n--------\n"
        ,source,mc,data,vars,showingLog,target)); 
// source net/irc/user#jomat
// mc "_request_enter_join"
// data 0
// vars ([ /* #1 */
//   "_group": "irc:*jomat@blafasel",
//   "_nick": "jomat",
//   "_flag": "_quiet",
//   "_tag": "40f7eed0dfbf2800"
// ])
// showingLog 0
// target "irc:*jomat@blafasel"
  switch (mc) {
    case "_request_members":  /* user wants to see the nicks of a room */
      string chan;
      sscanf(vars["_target"]?vars["_target"]:target,"irc:%s@%~s",chan);
      show_members(source,chan,vars);
      return 0;
    case "_message_private":
    case "_message_public_question":
    case "_message_public":
      string destination=parse_destination(target);
      if (""==destination) // msg to the server
        emit(data+"\n");
      else
        irc_privmsg(destination,data);
      source->msg(this_object(),regreplace(mc,"(_message_)(.*)","\\1echo_\\2",0),data,vars);
      return 0;
    case "_request_enter":
    case "_request_enter_join":
    case "_request_enter_subscribe":
      irc_join(parse_destination(target),vars["_tag"],0 /* TODO: add key foo */);
    break;
  }
  return 1;
}

int logon() {
  if (!interactive()) {
    P3(("connection in background failed, will try to reconnect in 10 seconds\n"));  // TODO...
    call_out(#'connect,10);
    return 0;
  }
  IDENT_MASTER->update_connection(server->port,server->owner);
  server->connected=-1;
  irc_nick(server->nick);
  irc_user(server->nick,"*",server->nick);
  next_input_to(#'server_answer);
  return 1;
}

void irc_quote(string id,string what) {
  emit(what);
}

int query_connected() {
  return server->connected;
}
