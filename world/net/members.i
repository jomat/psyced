//						vim:noexpandtab:syntax=lpc
// $Id: members.i,v 1.7 2007/10/08 11:00:30 lynx Exp $

renderMembers(members, nicks, noObjDet) {
    int i, useNicks;
    string *u = allocate(sizeof(members));

#ifdef PARANOID
    if (stringp(members)) {
	P0(("%O renderMembers with string members %O and nicks %O\n",
	    ME, members, nicks))
	return u;
    }
#endif
    unless (noObjDet) {
#if 0
	members = map(copy(members), #'fuserobj);
#else
	members = map(copy(members), (: return psyc_object($1) || $1; :));
#endif
    }

    useNicks = (pointerp(nicks) && sizeof(members) == sizeof(nicks));

    D3(
       if (!useNicks) {
	    if (pointerp(nicks)) {
		D(S("usercmd:%O: invalid size of argument 'nicks' in renderMembers(). will use ->qName().\n", ME));
	    } else {
		D(S("usercmd:%O: renderMembers() didn't get argument 'nicks'. will use ->qName().\n", ME));
	    }
	}
    )

    foreach (mixed entry : members) {
	string n;

	if (objectp(entry)) {
#if defined(ALIASES) && defined(USER_PROGRAM)
	    string tn, tn2;

	    if (!(n = raliases[tn = lower_case(tn2 = (useNicks ? nicks[i] : entry->qName()))]) && aliases[tn]) {
		n = psyc_name(entry);
	    } else unless (n) n = tn2;
#else
	    n = useNicks ? nicks[i] : entry->qName();
#endif
	} else if (entry) {
#if defined(ALIASES) && defined(USER_PROGRAM)
	    unless (n = raliases[entry])
#endif
		n = entry;
	}
	u[i++] = n;
    }
    
    return u;
}

