// $Id: proto.h,v 1.20 2007/09/30 17:06:11 lynx Exp $ // vim:syntax=lpc:ts=8
//
// sometimes prototypes are needed. i keep them here and include
// them also in the files that *define* the function so that the
// compiler will recognize any misdefinitions.

#ifndef _INCLUDE_PROTO_H
#define _INCLUDE_PROTO_H

#ifndef MUD

// alphabetical order.
#ifndef __PIKE__
void dns_resolve(string hostname, closure callback, varargs array(mixed) extra);
void dns_rresolve(string ip, closure callback, varargs array(mixed) extra);
#endif
int hex2int(string hex);
varargs string isotime(mixed ctim, int long);
int legal_host(string ip, int port, string scheme, int udpflag);
#ifdef varargs
void log_file(string file,string str,
        vamixed a,vamixed b,vamixed c,vamixed d,
	vamixed e,vamixed f,vamixed g,vamixed h);
#else
void log_file(string file, string str, varargs array(mixed) args);
#endif
string make_json(mixed d);
void monitor_report(string mc, string text);
array(object) objects_people();
varargs string psyc_name(mixed source, vastring localpart);
string query_server_unl();
varargs mixed sendmsg(mixed target, string mc, mixed data, mapping vars,
	    mixed source, int showingLog, closure callback, varargs array(mixed) extra);
varargs void shout(mixed who, string what, string text, mapping vars);
varargs int server_shutdown(string reason, int restart, int pass);
string timedelta(int secs);

#else

// danny, do you really need this?
void _psyc_dns_resolve(string hostname, closure callback, varargs array(mixed) extra);
void _psyc_dns_rresolve(string ip, closure callback, varargs array(mixed) extra);
int _psyc_legal_host(string ip, int port, string scheme, int udpflag);
#ifdef varargs
void _psyc_log_file(string file,string str,
        mixed a,mixed b,mixed c,mixed d,mixed e,mixed f,mixed g,mixed h);
#else
void _psyc_log_file(string file, string str, varargs array(mixed) args);
#endif
void _psyc_monitor_report(string mc, string text);
object* _psyc_objects_people();
string _psyc_query_server_unl();
varargs mixed _psyc_sendmsg(mixed target, string mc, mixed data, mapping vars,
            mixed source, int showingLog, closure callback, varargs array(mixed) extra);

#endif

#endif
