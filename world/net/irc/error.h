// $Id: error.h,v 1.3 2005/03/14 10:23:27 lynx Exp $ // vim:syntax=lpc
//
// automatically extracted from RFC 1459, chapter 6. thanks sique.

#define ERR_NOSUCHNICK      "401" // Used to indicate the nickname parameter
                                 // supplied to a command is currently unused.

#define ERR_NOSUCHSERVER    "402" // Used to indicate the server name given
				 // currently doesn't exist.

#define ERR_NOSUCHCHANNEL   "403" // Used to indicate the given channel name
                                 // is invalid.

#define ERR_CANNOTSENDTOCHAN "404" // Sent to a user who is either (a) not on
                                 // a channel which is mode +n or (b) not a
                                 // chanop (or mode +v) on a channel which
                                 // has mode +m set and is trying to send
                                 // a PRIVMSG message to that channel.

#define ERR_TOOMANYCHANNELS "405" // Sent to a user when they have joined
                                 // the maximum number of allowed channels
                                 // and they try to join another channel.

#define ERR_WASNOSUCHNICK   "406" // Returned by WHOWAS to indicate there
                                 // is no history information for that
                                 // nickname.

#define ERR_TOOMANYTARGETS  "407" // Returned to a client which is attempting
                                 // to send a PRIVMSG/NOTICE using the
                                 // user@host destination format and for a
                                 // user@host which has several occurrences.

#define ERR_NOORIGIN        "409" // PING or PONG message missing the
                                 // originator parameter which is required
                                 // since these commands must work
                                 // without valid prefixes.

#define ERR_NORECIPIENT     "411" // 412 - 414 are returned by PRIVMSG to
#define ERR_NOTEXTTOSEND    "412" // indicate that the message wasn't
#define ERR_NOTOPLEVEL      "413" // delivered for some reason.
#define ERR_WILDTOPLEVEL    "414" // ERR_NOTOPLEVEL and ERR_WILDTOPLEVEL
                                 // are errors that are returned when an
                                 // invalid use of "PRIVMSG $<server>" or
                                 // "PRIVMSG #<host>" is attempted.

#define ERR_UNKNOWNCOMMAND  "421" // Returned to a registered client to
                                 // indicate that the command sent is
                                 // unknown by the server.

#define ERR_NOMOTD          "422" // Server's MOTD file could not be opened
                                 // by the server.

#define ERR_NOADMININFO     "423" // Returned by a server in response to an
                                 // ADMIN message when there is an error in
                                 // finding the appropriate information.

#define ERR_FILEERROR       "424" // ???

#define ERR_NONICKNAMEGIVEN "431" // Returned when a nickname parameter
                                 // expected for a command and isn't found.

#define ERR_ERRONEUSNICKNAME "432" // Returned after receiving a NICK message
                                 // which contains characters which do not
                                 // fall in the defined set. See section
                                 // x.x.x for details on valid nicknames.

#define ERR_NICKNAMEINUSE   "433" // Returned when a NICK message is
                                 // processed that results in an attempt to
                                 // change to a currently existing nickname.

#define ERR_NICKCOLLISION   "436" // Returned by a server to a client when it
                                 // detects a nickname collision (registered
                                 // of a NICK that already exists by another
                                 // server).

#define ERR_USERNOTINCHANNEL "441" // Returned by the server to indicate that
                                 // the target user of the command is not on
                                 // the given channel.

#define ERR_NOTONCHANNEL    "442" // Returned by the server whenever a client
                                 // tries to perform a channel effecting
                                 // command for which the client isn't a
                                 // member.

#define ERR_USERONCHANNEL   "443" // Returned when a client tries to invite a
                                 // user to a channel they are already on.

#define ERR_NOLOGIN         "444" // Returned by the summon after a SUMMON
                                 // command for a user was unable to be
                                 // performed since they were not logged in.

#define ERR_SUMMONDISABLED  "445" // Returned as a response to the SUMMON
                                 // command. Must be returned by any server
                                 // which does not implement it.

#define ERR_USERSDISABLED   "446" // Returned as a response to the USERS
                                 // command. Must be returned by any server
                                 // which does not implement it.

#define ERR_NOTREGISTERED   "451" // Returned by the server to indicate that
                                 // the client must be registered before the
                                 // server will allow it to be parsed in
                                 // detail.

#define ERR_NEEDMOREPARAMS  "461" // Returned by the server by numerous
                                 // commands to indicate to the client that
                                 // it didn't supply enough parameters.

#define ERR_ALREADYREGISTRED "462" // Returned by the server to any link which
                                 // tries to change part of the registered
                                 // details (such as password or user
                                 // details from second USER message).

#define ERR_NOPERFORHOST    "463" // Returned to a client which attempts to
                                 // register with a server which does not
                                 // been setup to allow connections from the
                                 // host the attempted connection is tried.

#define ERR_PASSWDMISMATCH  "464" // Returned to indicate a failed attempt at
                                 // registering a connection for which a
                                 // password was required and was either not
                                 // given or incorrect.

#define ERR_YOUREBANNEDCREEP "465" // Returned after an attempt to connect and
                                 // register yourself with a server which
                                 // has been setup to explicitly deny
                                 // connections to you.

#define ERR_KEYSET          "467" // Any command requiring operator
                                 // privileges to operate must return this
                                 // error to indicate the attempt was
                                 // unsuccessful.

#define ERR_CHANNELISFULL   "471" //     

#define ERR_UNKNOWNMODE     "472" //    

#define ERR_INVITEONLYCHAN  "473" //     

#define ERR_BANNEDFROMCHAN  "474" //

#define ERR_BADCHANNELKEY   "475" //     

#define ERR_NOPRIVILEGES    "481" //     

#define ERR_CHANOPRIVSNEEDED "482" // Any command requiring 'chanop'
                                 // privileges (such as MODE messages) must
                                 // return this error if the client making
                                 // the attempt is not a chanop on the
                                 // specified channel.

#define ERR_CANTKILLSERVER  "483" // Any attempts to use the KILL command on
                                 // a server are to be refused and this
                                 // error returned directly to the client.

#define ERR_NOOPERHOST      "491" // If a client sends an OPER message and
                                 // the server has not been configured to
                                 // allow connections from the client's host
                                 // as an operator, this error must be
                                 // returned.

#define ERR_UMODEUNKNOWNFLAG "501" // Returned by the server to indicate that
                                 // a MODE message was sent with a nickname
                                 // parameter and that the a mode flag sent
                                 // was not recognized.

#define ERR_USERSDONTMATCH  "502" // Error sent to any user trying to view or
                                 // change the user mode for a user other
                                 // than themselves.


#if 0
#define ERROR_LIST ([\
  ERR_NOSUCHNICK:       "%s :No such nick/channel",\
  ERR_NOSUCHSERVER:     "%s :No such server",\
  ERR_NOSUCHCHANNEL:    "%s :No such channel",\
  ERR_CANNOTSENDTOCHAN: "%s :Cannot send to channel",\
  ERR_TOOMANYCHANNELS:  "%s :You have joined too many channels",\
  ERR_WASNOSUCHNICK:    "%s :There was no such nickname",\
  ERR_TOOMANYTARGETS:   "%s :Duplicate recipients. No message \
delivered",\
  ERR_NOORIGIN:         ":No origin specified",\
  ERR_NORECIPIENT:      ":No recipient given (<command>)",\
  ERR_NOTEXTTOSEND:     ":No text to send",\
  ERR_NOTOPLEVEL:       "%s :No toplevel domain specified",\
  ERR_WILDTOPLEVEL:     "%s :Wildcard in toplevel domain",\
  ERR_UNKNOWNCOMMAND:   "%s :Unknown command",\
  ERR_NOMOTD:           ":MOTD File is missing",\
  ERR_NOADMININFO:      "%s :No administrative info available",\
  ERR_FILEERROR:        ":File error doing %s on %s",\
  ERR_NONICKNAMEGIVEN:  ":No nickname given",\
  ERR_ERRONEUSNICKNAME: "%s :Erroneus nickname",\
  ERR_NICKNAMEINUSE:    "%s :Nickname is already in use",\
  ERR_NICKCOLLISION:    "%s :Nickname collision KILL",\
  ERR_USERNOTINCHANNEL: "%s %s :They aren't on that channel",\
  ERR_NOTONCHANNEL:     "%s :You're not on that channel",\
  ERR_USERONCHANNEL:    "%s %s :is already on channel",\
  ERR_SUMMONDISABLED:   ":SUMMON has been disabled",\
  ERR_USERSDISABLED:    ":USERS has been disabled",\
  ERR_NOTREGISTERED:    ":You have not registered",\
  ERR_NEEDMOREPARAMS:   "%s :Not enough parameters",\
  ERR_ALREADYREGISTRED: ":You may not reregister",\
  ERR_YOUREBANNEDCREEP: ":You are banned from this server",\
  ERR_KEYSET:           "%s :Channel key already set",\
  ERR_CHANNELISFULL:    "%s :Cannot join channel (+l, Limit exceeded)",\
  ERR_UNKNOWNMODE:      "%s :is unknown mode char to me",\
  ERR_INVITEONLYCHAN:   "%s :Cannot join channel (+i, Invite channel)",\
  ERR_BANNEDFROMCHAN:   "%s :Cannot join channel (+b, You are banned from \
channel)",\
  ERR_BADCHANNELKEY:    "<channel> :Cannot join channel (+k, wrong channel \
key)",\
  ERR_NOPRIVILEGES:     ":Permission Denied- You're not an IRC operator",\
  ERR_CHANOPRIVSNEEDED: "%s :You're not channel operator",\
  ERR_CANTKILLSERVER:   ":You cant kill a server!",\
  ERR_UMODEUNKNOWNFLAG: ":Unknown MODE flag",\
  ERR_USERSDONTMATCH:   ":Cant change mode for other users",\
])
#endif
