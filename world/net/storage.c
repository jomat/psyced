// $Id: storage.c,v 1.26 2008/12/16 11:58:52 lynx Exp $ // vim:syntax=lpc
//
// common subclass for anything that uses persistent data storage
//
// using only one mapping for less backwards compatibility problems

#include <net.h>
#include <errno.h>

// should you like to do a completely different implementation of
// net/storage, just #define _implementation_net_storage in local.h
// to point to it
#ifdef _implementation_net_storage
# include _implementation_net_storage
#else


#ifndef STORAGE_SQL_OBJECT
# ifdef STORAGE_SQLITE
#  define STORAGE_SQL_OBJECT DAEMON_PATH "sqlite"
# elif defined(STORAGE_MYSQL)
#  define STORAGE_SQL_OBJECT DAEMON_PATH "mysql"
# endif
#endif

#ifdef STORAGE_SQL_OBJECT
volatile object sqlob = load_object(STORAGE_SQL_OBJECT);
#endif

protected mapping _v;

vInit() { _v = ([]); }

v(k, c) {
	// happens in net/irc/common .. you should generally avoid this
	P2(("->v(%O) method used in %O\n", k, ME))
	if (c) _v[k] = c;
	return _v[k];
}

vQuery(k, c) {
	D2( if (c) raise_error("second arg in vQuery given\n"); )
	// D2(if(!_v) raise_error("storage not initialized\n"); )
	if (mappingp(_v)) return _v[k];
}

vSet(k, c) {
	P3(("%O->vSet(%O,%O)\n", ME, k, k == "log" ? "<log>" : c))
	D1( if (!c) D(S("vSet w/out value from %O into %O->vSet(%O,%O)\n",
			previous_object(), ME, k,c));)
	// D1( if (!c) raise_error("second arg in vSet not given ("+c+")\n"); )
	D2( if(!_v) raise_error("storage not initialized\n"); )
	return _v[k] = c;
}

vDel(k) {
	P3(("%O->vDel(%O)\n", ME, k))
	_v = m_delete(_v, k);
}

vAppend(k, c) { return _v[k] += c; }

// used by msg() in user.c
vInc(k, howmuch) {
	unless (howmuch) howmuch = 1;
	if (member(_v,k)) return _v[k] += howmuch;
	else return _v[k] = howmuch;
}
// vDec(k) { _v[k]--; }

vExist(k) {
    return member(_v, k);
}

vEmpty() { return sizeof(_v) == 0; }
vSize() { return sizeof(_v); }

// this is supposed to be a read-only access
// for performance the mapping returned is currently writeable
// never (ab)use this!
//
vMapping() { return _v; }

vMerge(mapping vars) { _v += vars; }

// in case of future extension..
protected load(file) {
	int e;

	P2(("LOAD %O <= %O\n", ME, file))
	/* WARNING
	 * THIS IS ABSOLUTELY EXPERIMENTAL
	 * and abuses the sync, blocking implementations of mysql and sqlite
	 */
#if defined(STORAGE_SQLITE) || defined(STORAGE_MYSQL) 
	// restore _v and _log (in theory, _log would have to be initialized 
	// in lastlog.c
	// TODO: should place and person overload their storage init?
	if (abbrev(DATA_PATH "person/", file)) {
	    string who;
	    mixed res;
	    
	    who = file[strlen(DATA_PATH) + 7..]; // + 7 -> "person/"
	    /* put your query for loading _v here */
	    // slightly different syntaxes, eh? 
# ifdef STORAGE_SQLITE 
	    res = sqlob->query("SELECT "
			       "`_nick`, `_password`, `_prehash` "
			       "FROM `person` "
			       "WHERE `_nick` = '%s' LIMIT 1", who);
# elif defined(STORAGE_MYSQL) 
	    res = sqlob->query("SELECT "
			       "`_nick`, `_password`, `_prehash` "
			       "FROM `person` "
			       "WHERE `_nick` = '%s' LIMIT 1", who);
# endif
	    unless(res && !intp(res) && sizeof(res)) return; // no userdata
	    res = res[0];
	    /* currently, we use the same logic for building _v from
	     * the sql result
	     */

	    _v["nick"] = res[0];
	    _v["password"] = res[1];
	    _v["prehash"] = res[2];
	    
	    /* now the logic for loading _log */
	    logInit();
	    return;
	}
# if 0
	} else if (abbrev(DATA_PATH "place/", file)) {
	    sl_open(DATA_PATH "place-sqlite");
	    // restore _v and _log
# endif
#else
# if __EFUN_DEFINED__(errno)
	errno(); // clear errno
	if (restore_object(file)) return;
	e = errno();
	D1( if (e && e != ENOENT)
	    D(S("restore from %s returns %d\n", file, e )); )
	return e ? e : -1;
# else
	if (restore_object(file)) return;
	return ENOENT;
# endif
#endif
}

protected save(file) {
#ifdef VOLATILE
	P2(("VOLATILE NO SAVE %O => %O\n", ME, file))
#else
	P2(("SAVE %O => %O\n", ME, file))
# if __EFUN_DEFINED__(errno)
	int e;

	errno(); // clear errno
#  ifdef SAVE_FORMAT
	save_object(file, SAVE_FORMAT);
#  else
	save_object(file);
#  endif
	e = errno();
	D1( if (e && e != ENOENT) D("save in "+file+" returns "+e+"\n"); )
	return e;
# else
#  ifdef SAVE_FORMAT
	save_object(file, SAVE_FORMAT);
#  else
	save_object(file);
#  endif
	return 0;
# endif
#endif
}

void create() { if (!_v && clonep(ME)) vInit(); }


#endif // _implementation_net_storage
