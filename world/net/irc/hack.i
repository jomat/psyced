//						vim:noexpandtab:syntax=lpc
// $Id: hack.i,v 1.27 2007/10/08 11:00:31 lynx Exp $

// hacky compatibility with old-school
// MYNICK returns * for non-identified users
// none of these should be used
//
// well, shouldn't we just define those?

pr(mc, fmt, a,b,c,d,e,f,g,h) {
	IRCD( D("IRC»pr\n") );
	// skipping printStyle
	unless (fmt = T(mc, fmt)) return;
	emit(sprintf(psyc2irc(mc, 0) +" "+ MYNICK +" :"+ fmt +"\n",
	       a,b,c,d,e,f,g,h));
}

p(fmt, a,b,c,d,e,f,g,h) {
	string s = sprintf("NOTICE "+MYNICK+" :"+ fmt +"\n", a,b,c,d,e,f,g,h);
	IRCD( D("IRC»p "+s) );
	emit(s);
}

reply(num, rest) {
	string s = sreply(num, rest);
	IRCD( D("IRC»r "+s) );
	emit(s);
}

sreply(num, rest) {
	return SERVER_SOURCE + num + " " + MYNICK + " " + rest + "\n";
}

