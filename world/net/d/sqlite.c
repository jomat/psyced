#include <net.h>

#ifdef STORAGE_SQLITE
# ifndef SQLITE_DB_PERSON 
#  define SQLITE_DB_PERSON DATA_PATH "person-sqlite"
# endif
#endif

/* 
 * TODO: actually, this may need be a wrapper, as every object is only allowed
 * 	a single sqlite db
 */


create() {
#if defined(STORAGE_SQLITE) 
    sl_open(SQLITE_DB_PERSON);
#endif
}

mixed query(string q, varargs mixed args) {
    // TODO: it might be wise to db_conv_string on each arg to 
    // avoid sql injections
    return sl_exec(sprintf(q, args...));
}
