// $Id: proto.h,v 1.22 2008/05/07 10:50:29 lynx Exp $ // vim:syntax=lpc:ts=8
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
varargs object find_person(string name, vaint lowercazed);
#ifndef hex2int
int hex2int(string hex);
#endif
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
string query_server_uniform();
varargs mixed sendmsg(mixed target, string mc, mixed data, mapping vars,
	    mixed source, int showingLog, closure callback, varargs array(mixed) extra);
varargs void shout(mixed who, string what, string text, mapping vars);
varargs int server_shutdown(string reason, int restart, int pass);
string timedelta(int secs);

#else

void _psyc_dns_resolve(string hostname, closure callback, varargs array(mixed) extra);
void _psyc_dns_rresolve(string ip, closure callback, varargs array(mixed) extra);
#ifndef hex2int
int _psyc_hex2int(string hex);
#endif
int _psyc_legal_host(string ip, int port, string scheme, int udpflag);
#ifdef varargs
void _psyc_log_file(string file,string str,
        mixed a,mixed b,mixed c,mixed d,mixed e,mixed f,mixed g,mixed h);
#else
void _psyc_log_file(string file, string str, varargs array(mixed) args);
#endif
string _psyc_make_json(mixed d);
void _psyc_monitor_report(string mc, string text);
object* _psyc_objects_people();
varargs string _psyc_psyc_name(mixed source, vastring localpart);
string _psyc_query_server_uniform();
varargs mixed _psyc_sendmsg(mixed target, string mc, mixed data, mapping vars,
            mixed source, int showingLog, closure callback, varargs array(mixed) extra);
varargs void _psyc_shout(mixed who, string what, string text, mapping vars);
varargs int _psyc_server_shutdown(string reason, int restart, int pass);
string _psyc_timedelta(int secs);

#endif

#endif
