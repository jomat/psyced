// $Id: name.c,v 1.6 2008/05/11 08:56:48 lynx Exp $ // vim:syntax=lpc
//
// common subclass for anything that has a name - unnecessary? could be.
// jabber/active.c and jabber/mixin_parse.c inherit this separately from
// entity.c

// local debug messages - turn them on by using psyclpc -DDname=<level>
#ifdef Dname
# undef DEBUG
# define DEBUG Dname
#endif

#include <net.h>

volatile protected string _myNick;
volatile protected string _myLowerCaseNick;

// when inheriting this object, do use the functions/macros instead of
// accessing any variables, so one can attach some event handlers later on

object sName(string n) {
	P3(("%O sName(%O)\n", ME, n))
	_myNick = n;
	_myLowerCaseNick = lower_case(n);
	return ME;	// used by named_clone()
}

// used by -> calls from other objects
// should be replaced by MYNICK and MYLOWERNICK within the object

string qName() {
	// this outputs <qName> everytime it gets used where MYNICK should be
	D2( if (!previous_object() || previous_object() == ME) D(" <qName> "); )
	return _myNick;
}

string qNameLower() {
	D2( if (!previous_object() || previous_object() == ME)
		     D(" <qNameLower> "); )
	return _myLowerCaseNick;
}
