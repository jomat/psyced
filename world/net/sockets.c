// $Id: sockets.c,v 1.26 2007/10/02 10:35:46 lynx Exp $ // vim:syntax=lpc
//
// this function is needed by net/user and net/http/configure (which doesn't
// inherit net/user.
// so i decided to move listUsers out into it's own object, which is to be
// inherited by those who need it.
// - saga
//
// listUsers() is now list_sockets() ...
//
#include <net.h>
#include <misc.h>

#if __EFUN_DEFINED__(tls_query_connection_info)
# include <sys/tls.h>
#endif

//#define NO_INHERIT	// virtual ain't workin' .. but leavin it out neither
#include <text.h>

static int smaller_object_name(object a, object b) {
	return object_name(a) < object_name(b);
}

list_sockets(guy, flags) {
	array(object) u;
	array(mixed) tls;
	mapping uv;
	string list, name, host, ip, idle, email, scheme, agent, layout;
	int i, skip = 0;
	mixed o;

	if (guy) {
		flags = 0;
		o = find_person(guy);
		if (o) u = ({ o });
		else w("_error_unknown_name_user", "Cannot reach [_nick_target].", ([ "_nick_target": guy ]) );
	}
	else u = sort_array(users(), #'smaller_object_name);

	unless (flags && intp(flags)) flags = SOCKET_LIST_ALL;
	list = "";
	for (i = sizeof(u)-1; i>=0; i--) {
		o = u[i];
		uv = o->vMapping();
		if (mappingp(uv) && name = uv["name"]) {
		    if (flags & SOCKET_LIST_USER) {
			email = uv["email"];
			scheme = uv["scheme"];
			agent = uv["agent"];
			layout = uv["layout"];

			host = uv["forwarded"];
			unless (host) host = uv["host"];
			ip = uv["ip"];
#if 1 //def _flag_log_hosts	// realtime inspection isn't logging
			unless (host) host = query_ip_name(o);
			unless(ip) ip = query_ip_number(o);
#endif
//			if (ip == host) ip = "";

			if (idle = uv["aliveTime"]) idle = timedelta(time() - idle);
			else if (idle = uv["idleTime"]) idle = timedelta(idle);
			if (boss(o)) name += "*";
			if (uv["visibility"] == "off") name = "°"+name;
			list += sprintf(T("_list_user_technical_person",
			  "\n%4.4s%s %s %s %s (%s) <%s> %s"),
//				scheme ? (layout && scheme=="ht" ?
//					  layout : scheme) : "-",
				(scheme == "ht" ? (layout || scheme)
					: scheme) || "",
#if __EFUN_DEFINED__(tls_query_connection_state)
				interactive(o) &&
				  tls_query_connection_state(o) ? "!" : " ",
#else
				" ",
#endif
				idle ? ((stringp(idle) ?
				     idle : to_string(idle))) : "",
				name || "???",
				host || "",
				ip == host ? "=" : (ip || ""),
				stringp(email)? email: "-",
				stringp(agent)? agent: "");
		    } else skip++;
		}
		else {
		    name = o->qName();
#if __EFUN_DEFINED__(tls_query_connection_info)
		    tls = interactive(o) && tls_query_connection_info(o);
#endif
		    if (name? flags & SOCKET_LIST_GHOST : flags & SOCKET_LIST_LINK)
			list += sprintf(T("_list_user_technical_ghost",
			  "\n%s %s %O %s (%s) %s %s"),
#if __EFUN_DEFINED__(tls_query_connection_state)
			    interactive(o) && 
			      tls_query_connection_state(o) ? "!" : " ",
#else
			    " ",
#endif
			    (name && to_string(name)) || "",
			    o,
#if 1 //def _flag_log_hosts	// realtime inspection isn't logging
			    query_ip_name(o) || "",
			    query_ip_number(o) || "",
#else
                            "", "",
#endif
#if __EFUN_DEFINED__(tls_query_connection_info)
			    tls ? intp(tls[TLS_PROT]) ?
				 TLS_PROT_NAME(tls[TLS_PROT]) ||
				 to_string(tls[TLS_PROT]) : tls[TLS_PROT] : "",
			    tls ? intp(tls[TLS_CIPHER]) ?
				 TLS_CIPHER_NAME(tls[TLS_CIPHER])
				 || to_string(tls[TLS_CIPHER])
				 : tls[TLS_CIPHER] : ""
#else
			    "", ""
#endif
			);
		    else skip++;
		}
	}
	if (list != "") {
		w("_list_user_technical",
    "*** Connections: [_amount_connections] ([_amount_connections_skipped] skipped), Load: [_server_load]\n[_users]", ([
			    "_amount_connections": sizeof(u),
			    "_amount_connections_skipped": skip,
			    "_server_load": query_load_average(),
			    "_users": list[1..]
		]) );
		return 1;
	}
}
