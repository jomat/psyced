// $Id: reply.h,v 1.9 2006/01/13 20:28:57 lynx Exp $ // vim:syntax=lpc
//

// many useful and broken numerics have been added since the IRC RFCs
// were written. http://www.alien.net.au/irc/irc2numerics.html provides
// some orientation. we are implementing some things on experimental
// basis and only on prescription by adding -DIRC_BEYOND_RFC to your
// ldmud flags or #define IRC_BEYOND_RFC to your local.h

#ifdef IRC_BEYOND_RFC
// nothing right now.
#endif

#ifndef IRC_STRICTLY_RFC
# define RPL_ISUPPORT	"005"	// the de facto standard for server2client
				// settings
#endif

// added manually. not in RFC 1459.

#define RPL_CHANNELMODEUNKNOWN "472"	// Unsupported MODE

// automatically extracted from RFC 1459, chapter 6. thanks sique.

#define RPL_USERHOST        "302" // Reply format used by USERHOST to list
				 // replies to the query list. The reply
				 // string is composed as follows: <reply>
				 // ::= <nick>['*'] '=' <'+'|'-'><hostname>
				 // The '*' indicates whether the client has
				 // registered as an Operator. The '-' or
				 // '+' characters represent whether the
				 // client has set an AWAY message or not
				 // respectively.

#define RPL_ISON            "303" // Reply format used by ISON to list
				 // replies to the query list.

#define RPL_AWAY            "301" // These replies are used with the AWAY
#define RPL_UNAWAY          "305" // command (if allowed). RPL_AWAY is sent
#define RPL_NOWAWAY         "306" // to any client sending a PRIVMSG to a
				 // client which is away. RPL_AWAY is only
				 // sent by the server to which the client
				 // is connected. Replies RPL_UNAWAY and
				 // RPL_NOWAWAY are sent when the client
				 // removes and sets an AWAY message.

#define RPL_WHOISUSER       "311" // Replies 311 - 313, 317 - 319 are all
#define RPL_WHOISSERVER     "312" // replies generated in response to a
#define RPL_WHOISOPERATOR   "313" // WHOIS message.  Given that there are
#define RPL_WHOISIDLE       "317" // enough parameters present, the answering
#define RPL_ENDOFWHOIS      "318" // server must either formulate a reply out
#define RPL_WHOISCHANNELS   "319" // of the above numerics (if the query nick
				 // is found) or return an error reply.
				 // The '*' in RPL_WHOISUSER is there as
				 // the literal character and not as a wild
				 // card.  For each reply set, only
				 // RPL_WHOISCHANNELS may appear more than
				 // once (for long lists of channel names).
				 // The '@' and '+' characters next to the
				 // channel name indicate whether a client
				 // is a channel operator or has been granted
				 // permission to speak on a moderated
				 // channel.  The RPL_ENDOFWHOIS reply is
				 // used to mark the end of processing a
				 // WHOIS message.

#define RPL_WHOWASUSER      "314" // When replying to a WHOWAS message, a
#define RPL_ENDOFWHOWAS     "369" // server must use the replies
				 // RPL_WHOWASUSER, RPL_WHOISSERVER or
				 // ERR_WASNOSUCHNICK for each nickname in
				 // the presented list. At the end of all
				 // reply batches, there must be
				 // RPL_ENDOFWHOWAS (even if there was only
				 // one reply and it was an error). 


#define RPL_LISTSTART       "321" // Replies RPL_LISTSTART, RPL_LIST,
#define RPL_LIST            "322" // RPL_LISTEND mark the start, actual
#define RPL_LISTEND         "323" // replies with data and end of the
				 // server's response to a LIST command. If
				 // there are no channels available to
				 // return, only the start and end reply
				 // must be sent.

#define RPL_CHANNELMODEIS   "324" // When sending a TOPIC message to
#define RPL_NOTOPIC         "331" // determine the channel topic, one of two
#define RPL_TOPIC           "332" // replies is sent. If the topic is set,
				 // RPL_TOPIC is sent back else RPL_NOTOPIC.

#define RPL_INVITING        "341" // Returned by the server to indicate that
				 // the attempted INVITE message was
				 // successful and is being passed onto the
				 // end client.

#define RPL_SUMMONING       "342" // Returned by a server answering a SUMMON
				 // message to indicate that it is summoning
				 // that user.

#define RPL_VERSION         "351" // Reply by the server showing its version
				 // details. The <version> is the version of
				 // the software being used (including any
				 // patchlevel revisions) and the
				 // <debuglevel> is used to indicate if the
				 // server is running in "debug mode". The
				 // "comments" field may contain any
				 // comments about the version or further
				 // version details.

#define RPL_WHOREPLY        "352" // The RPL_WHOREPLY and RPL_ENDOFWHO pair
#define RPL_ENDOFWHO        "315" // are used to answer a WHO message. The
				 // RPL_WHOREPLY is only sent if there is an
				 // appropriate match to the WHO query. If
				 // there is a list of parameters supplied
				 // with a WHO message, a RPL_ENDOFWHO must
				 // be sent after processing each list item
				 // with <name> being the item. 

#define RPL_NAMREPLY        "353" // To reply to a NAMES message, a reply
#define RPL_ENDOFNAMES      "366" // pair consisting of RPL_NAMREPLY and
				 // RPL_ENDOFNAMES is sent by the server
				 // back to the client. If there is no
				 // channel found as in the query, then only
				 // RPL_ENDOFNAMES is returned. The
				 // exception to this is when a NAMES
				 // message is sent with no parameters and
				 // all visible channels and contents are
				 // sent back in a series of RPL_NAMEREPLY
				 // messages with a RPL_ENDOFNAMES to mark
				 // the end. 

#define RPL_LINKS           "364" // In replying to the LINKS message, a
#define RPL_ENDOFLINKS      "365" // server must send replies back using the
				 // RPL_LINKS numeric and mark the end of
				 // the list using an RPL_ENDOFLINKS reply.

#define RPL_BANLIST         "367" // When listing the active 'bans' for a
#define RPL_ENDOFBANLIST    "368" // given channel, a server is required to
				 // send the list back using the RPL_BANLIST
				 // and RPL_ENDOFBANLIST messages. A
				 // separate RPL_BANLIST is sent for each
				 // active banid. After the banids have been
				 // listed (or if none present) a
				 // RPL_ENDOFBANLIST must be sent. 

#define RPL_INFO            "371" // A server responding to an INFO message
#define RPL_ENDOFINFO       "374" // is required to send all its 'info' in a
				 // series of RPL_INFO messages with a
				 // RPL_ENDOFINFO reply to indicate the end
				 // of the replies. 

#define RPL_MOTDSTART       "375" // When responding to the MOTD message and
#define RPL_MOTD            "372" // the MOTD file is found, the file is
#define RPL_ENDOFMOTD       "376" // displayed line by line, with each line
				 // no longer than 80 characters, using
				 // RPL_MOTD format replies. These should be
				 // surrounded by a RPL_MOTDSTART (before
				 // the RPL_MOTDs) and an RPL_ENDOFMOTD
				 // (after).  

#define RPL_YOUREOPER       "381" // RPL_YOUREOPER is sent back to a client
				 // which has just successfully issued an
				 // OPER message and gained operator status.

#define RPL_REHASHING       "382" // If the REHASH option is used and an
				 // operator sends a REHASH message, an
				 // RPL_REHASHING is sent back to the
				 // operator.

#define RPL_TIME            "391" // When replying to the TIME message, a
				 // server must send the reply using the
				 // RPL_TIME format above. The string
				 // showing the time need only contain the
				 // correct day and time there. There is no
				 // further requirement for the time string.

#define RPL_USERSSTART      "392" // If the USERS message is handled by a
#define RPL_USERS           "393" // server, the replies RPL_USERSTART,
#define RPL_ENDOFUSERS      "394" // RPL_USERS, RPL_ENDOFUSERS and
#define RPL_NOUSERS         "395" // RPL_NOUSERS are used. RPL_USERSSTART
				 // must be sent first, following by either
				 // a sequence of RPL_USERS or a single
				 // RPL_NOUSER. Following this is
				 // RPL_ENDOFUSERS.   

#define RPL_TRACELINK       "200" // The RPL_TRACE* are all returned by the
#define RPL_TRACECONNECTING "201" // server in response to the TRACE message.
#define RPL_TRACEHANDSHAKE  "202" // How many are returned is dependent on
#define RPL_TRACEUNKNOWN    "203" // the the TRACE message and whether it was
#define RPL_TRACEOPERATOR   "204" // sent by an operator or not. There is no
#define RPL_TRACEUSER       "205" // predefined order for which occurs first.
#define RPL_TRACESERVER     "206" // Replies RPL_TRACEUNKNOWN,
#define RPL_TRACENEWTYPE    "208" // RPL_TRACECONNECTING and
#define RPL_TRACELOG        "261" // RPL_TRACEHANDSHAKE are all used for
				 // connections which have not been fully
				 // established and are either unknown,
				 // still attempting to connect or in the
				 // process of completing the 'server
				 // handshake'. RPL_TRACELINK is sent by any
				 // server which handles a TRACE message and
				 // has to pass it on to another server. The
				 // list of RPL_TRACELINKs sent in response
				 // to a TRACE command traversing the IRC
				 // network should reflect the actual
				 // connectivity of the servers themselves
				 // along that path. RPL_TRACENEWTYPE is to
				 // be used for any connection which does
				 // not fit in the other categories but is
				 // being displayed anyway.      

#define RPL_STATSLINKINFO   "211" 
#define RPL_STATSCOMMANDS   "212" 
#define RPL_STATSCLINE      "213" 
#define RPL_STATSNLINE      "214" 
#define RPL_STATSILINE      "215" 
#define RPL_STATSKLINE      "216" 
#define RPL_STATSYLINE      "218" 
#define RPL_ENDOFSTATS      "219" 
#define RPL_STATSLLINE      "241" 
#define RPL_STATSUPTIME     "242" 
#define RPL_STATSOLINE      "243" 
#define RPL_STATSHLINE      "244" 

#define RPL_UMODEIS         "221" // To answer a query about a client's own
				 // mode, RPL_UMODEIS is sent back.

#define RPL_LUSERCLIENT     "251" // In processing an LUSERS message, the
#define RPL_LUSEROP         "252" // server sends a set of replies from
#define RPL_LUSERUNKNOWN    "253" // RPL_LUSERCLIENT, RPL_LUSEROP,
#define RPL_LUSERCHANNELS   "254" // RPL_USERUNKNOWN, RPL_LUSERCHANNELS and
#define RPL_LUSERME         "255" // RPL_LUSERME. When replying, a server
				 // must send back RPL_LUSERCLIENT and
				 // RPL_LUSERME. The other replies are only
				 // sent back if a non-zero count is found
				 // for them.    

#define RPL_ADMINME         "256" // When replying to an ADMIN message, a
#define RPL_ADMINLOC1       "257" // server is expected to use replies
#define RPL_ADMINLOC2       "258" // RPL_ADMINME through to RPL_ADMINEMAIL
#define RPL_ADMINEMAIL      "259" // and provide a text message with each.
				 // For RPL_ADMINLOC1 a description of what
				 // city, state and country the server is in
				 // is expected, followed by details of the
				 // university and department
				 // (RPL_ADMINLOC2) and finally the
				 // administrative contact for the server
				 // (an email address here is required) in
				 // RPL_ADMINEMAIL.

#if 0
#define REPLY_MSGS ([\
    RPL_USERHOST         :  ":%s",\
    RPL_ISON             :  ":%s",\
    RPL_AWAY             :  "%s :%s",\
    RPL_UNAWAY           :  ":You are no longer marked as being away",\
    RPL_NOWAWAY          :  ":You have been marked as being away",\
    RPL_WHOISUSER        :  "%s %s %s * :%s",\
    RPL_WHOISSERVER      :  "%s %s :%s",\
    RPL_WHOISOPERATOR    :  "%s :is an IRC operator",\
    RPL_WHOISIDLE        :  "%s %s :seconds idle",\
    RPL_ENDOFWHOIS       :  "%s :End of /WHOIS list",\
    RPL_WHOISCHANNELS    :  "%s :%s",\
    RPL_WHOWASUSER       :  "%s %s %s * :%s",\
    RPL_ENDOFWHOWAS      :  "%s :End of WHOWAS",\
    RPL_LISTSTART        :  "Channel :Users Name",\
    RPL_LIST             :  "%s %d :%s",\
    RPL_LISTEND          :  ":End of /LIST",\
    RPL_CHANNELMODEIS    :  "%s %s %s",\
    RPL_NOTOPIC          :  "%s :No topic is set",\
    RPL_TOPIC            :  "%s :%s",\
    RPL_INVITING         :  "%s %s",\
    RPL_SUMMONING        :  "%s :Summoning user to IRC",\
    RPL_VERSION          :  "%d.%d %s :%s",\
    RPL_WHOREPLY         :  "%s %s %s %s %s %s :%d %s",\
    RPL_ENDOFWHO         :  "%s :End of /WHO list",\
    RPL_NAMREPLY         :  "%s :%s",\
    RPL_ENDOFNAMES       :  "%s :End of /NAMES list",\
    RPL_LINKS            :  "<mask> <server> :<hopcount> <server info>",\
    RPL_ENDOFLINKS       :  "<mask> :End of /LINKS list",\
    RPL_BANLIST          :  "<channel> <banid>",\
    RPL_ENDOFBANLIST     :  "",\
    RPL_INFO             :  ":%s",\
    RPL_ENDOFINFO        :  ":End of /INFO list",\
    RPL_MOTDSTART        :  ":- %s Message of the day - ",\
    RPL_MOTD             :  ":- %s",\
    RPL_ENDOFMOTD        :  ":End of /MOTD command",\
    RPL_YOUREOPER        :  ":You are now an IRC operator",\
    RPL_REHASHING        :  "%s :Rehashing",\
    RPL_TIME             :  "",\
    RPL_USERSSTART       :  ":UserID Terminal Host",\
    RPL_USERS            :  ":%-8s %-9s %-8s",\
    RPL_ENDOFUSERS       :  ":End of users",\
    RPL_NOUSERS          :  ":Nobody logged in",\
    RPL_TRACELINK        :  "Link %s %s %s",\
    RPL_TRACECONNECTING  :  "Try. <class> <server>",\
    RPL_TRACEHANDSHAKE   :  "H.S. <class> <server>",\
    RPL_TRACEUNKNOWN     :  "???? <class> [<client IP address in dot form>]",\
    RPL_TRACEOPERATOR    :  "Oper <class> <nick>",\
    RPL_TRACEUSER        :  "User <class> <nick>",\
    RPL_TRACESERVER      :  "Serv <class> <int>S <int>C <server> \
<nick!user|*!*>@<host|server>",\
    RPL_TRACENEWTYPE     :  "<newtype> 0 <client name>",\
    RPL_TRACELOG         :  "File <logfile> <debug level>",\
    RPL_STATSLINKINFO    :  "<linkname> <sendq> <sent messages> \
<sent bytes> <received messages> <received bytes> <time open>",\
    RPL_STATSCOMMANDS    :  "<command> <count>",\
    RPL_STATSCLINE       :  "C <host> * <name> <port> <class>",\
    RPL_STATSNLINE       :  "N <host> * <name> <port> <class>",\
    RPL_STATSILINE       :  "I <host> * <host> <port> <class>",\
    RPL_STATSKLINE       :  "K <host> * <username> <port> <class>",\
    RPL_STATSYLINE       :  "Y <class> <ping frequency> <connect \
frequency> <max sendq>",\
    RPL_ENDOFSTATS       :  "<stats letter> :End of /STATS report",\
    RPL_STATSLLINE       :  "L <hostmask> * <servername> <maxdepth>",\
    RPL_STATSUPTIME      :  ":Server Up %d days %d:%02d:%02d",\
    RPL_STATSOLINE       :  "O <hostmask> * <name>",\
    RPL_STATSHLINE       :  "H <hostmask> * <servername>",\
    RPL_UMODEIS          :  "<user mode string>",\
    RPL_LUSERCLIENT      :  ":There are <integer> users and <integer> \
invisible on <integer> servers",\
    RPL_LUSEROP          :  "<integer> :operator(s) online",\
    RPL_LUSERUNKNOWN     :  "<integer> :unknown connection(s)",\
    RPL_LUSERCHANNELS    :  "<integer> :channels formed",\
    RPL_LUSERME          :  ":I have <integer> clients and <integer> \
 servers",\
    RPL_ADMINME          :  "<server> :Administrative info",\
    RPL_ADMINLOC1        :  ":<admin info>",\
    RPL_ADMINLOC2        :  ":<admin info>",\
    RPL_ADMINEMAIL       :  ":<admin info>",\
    ])
#endif
