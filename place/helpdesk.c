#define NAME "HelpDesk"
#define PLACE_HISTORY_EXPORT
#define HISTORY_GLIMPSE 0

// HelpDesk currently disabled.. all questions are about PSYC anyway
#define REDIRECT "psyc://psyced.org/@welcome"

#include <place.gen>

#if 0
static mapping adms;

enter(a) {
	unless (mappingp(adms))
		adms = ([ ]);
		
	if (boss(a)) 
		adms += ([ a ]);

	if (sizeof(adms))
		sendmsg(a, "_status_available_help", "Stell deine Frage! Sie wird bald beantwortet werden.", ([ "_helpav" : 1 ]));
	else
#ifdef WEBMASTER_EMAIL
		sendmsg(a, "_failure_unavailable_help",
		"Zur Zeit ist leider niemand online, der dir helfen kann. "
		"Bitte mail deine Frage / dein Problem an [_email_support]!", 
		([ 
		"_helpav" : 0, 
		"_email_support" : WEBMASTER_EMAIL 
		]));
#endif
	return ::enter(a);
}

leave(a) {
	if(boss(a))
		adms -= ([ a ]);
	return ::leave(a);
}
#endif

