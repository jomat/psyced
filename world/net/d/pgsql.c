#include <net.h>

/* 
 * using one database connection is sufficient
 *
 * WARNING
 * ONLY USE THIS IF YOUR DRIVER HAS THE SYNC PG PACKAGE
 * (otherwise you dont have the pg_connect_sync and pg_query_sync efuns ;-)
 */

create() {
    int ret;
#if defined(STORAGE_PGSQL) 
    ret = pg_connect_sync(STORAGE_PGSQL_CONNECT);
    PT(("ret %d\n", ret))
#endif
}

mixed query(string q, varargs mixed args) {
    // TODO: it might be wise to db_conv_string on each arg to 
    // avoid sql injections
    return pg_query_sync(sprintf(q, args...));
}
