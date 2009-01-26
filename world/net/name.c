// $Id: name.c,v 1.5 2006/03/09 10:14:37 lynx Exp $ // vim:syntax=lpc
//
// common subclass for anything that has a name - unnecessary? could be

#include <net.h>

volatile protected string _myNick;
volatile protected string _myLowerCaseNick;

// when inheriting this object, do use the functions/macros instead of
// accessing any variables, so one can attach some event handlers later on

object sName(string n) {
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
	return _myLowerCaseNick;
}
