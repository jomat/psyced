// allow messages from trusted hosts.. that's just localhost by default
#define ALLOW_TRUSTED
//#define ALLOW_EXTERNAL_FROM	"psyc://localhost"

// notification rooms are usually not for chatting, but YMMV
#define FILTER_CONVERSATION

// we certainly don't want to see all the people entering and leaving
#define FILTER_PRESENCE

// let's keep a /history of the wiki notification messages
// it can be requested by users, but it is also available by javascript export
#define PLACE_HISTORY_EXPORT

// only store update notices, no conversations or enter/leave noise
#define HISTORY_METHOD		"_notice_update"

// show the last 3 on enter
#define HISTORY_GLIMPSE		3

// now build me my "chatroom" according to these settings
// see http://about.psyc.eu/Create_place for more options.
#include <place.gen>
