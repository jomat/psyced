// $Id: accept.c,v 1.119 2008/08/03 14:21:59 lynx Exp $ // vim:syntax=lpc:ts=8
//
// this file contains the glue between LDMUD and psyced to
// connect the socket ports to appropriately named objects.
//
#include "/local/config.h"

#ifdef Dmaster
# undef DEBUG
# define DEBUG Dmaster
#endif

#include NET_PATH "include/net.h"
#include NET_PATH "include/services.h"
#include DRIVER_PATH "include/driver.h"
#include CONFIG_PATH "config.h"

#include DRIVER_PATH "sys/tls.h"

#if __EFUN_DEFINED__(psyc_parse)
# define AUTODETECT 1 // use TLS autodetect if libpsyc is available
#else
# define AUTODETECT 0
# ifdef SPYC_PATH
#  echo PSYC 1.0 will not work: libpsyc is not enabled in driver.
# endif
#endif

volatile int shutdown_in_progress = 0;

// not part of the master/driver API.. this is called from the library
void notify_shutdown_first(int progress) {
        P3(("%O notify_shutdown_first(%O)\n", ME, progress))
        shutdown_in_progress = progress;
}

#ifndef UID2NICK
# ifdef _userid_nick_mapping
#  define UID2NICK(uid) ([ _userid_nick_mapping ])[uid]
# endif
#endif

/*
 * This function is called every time a TCP connection is established.
 * It dispatches the ports to the protocol implementations.
 * input_to() can't be called from here.
 *
 * uid is only passed if USE_AUTHLOCAL is built into the driver.
 *
 * strange how int port and string service came into existence here
 * since the driver isn't passing such arguments and there is no
 * reason to call this from anywhere else. i presume they are a
 * mistake!
 */
object connect(int uid, int port, string service) {
    int peerport;
    mixed arg, t;

    unless (port) port = query_mud_port();
    // now that's a bit of preprocessor magic you don't need to understand..  ;)
    D2( if (uid) D("master:connected on port "+ port +" by uid "
		   + uid +"("+ service + ")\n");
	else) {
	    D3(D("master:connected on port "+port
		   +" by "+query_ip_name()+"\n");)
    }
#ifndef H_DEFAULT_PROMPT
    set_prompt("");
#endif

    if (shutdown_in_progress) {
        PT(("shutdown_in_progress(%O): putting connection from %O on hold\n",
            shutdown_in_progress, query_ip_name(ME)))
        // put the connection on hold to avoid further reconnects
        return clone_object(NET_PATH "utility/onhold");
    }

    // we dont want the telnet machine most of the time
    // but disabling and re-enabling it for telnet doesn't work
    switch(port) {
#if HAS_PORT(PSYC_PORT, PSYC_PATH) && AUTODETECT
    case PSYC_PORT:
#endif
#if HAS_PORT(PSYCS_PORT, PSYC_PATH)
    case PSYCS_PORT:	// inofficial & temporary
	// make TLS available even on the default psyc port using the autodetection feature
	if (tls_available()) {
# if __EFUN_DEFINED__(tls_want_peer_certificate)
		tls_want_peer_certificate(ME);
# endif
		t = tls_init_connection(this_object());
		if (t < 0 && t != ERR_TLS_NOT_DETECTED) {
			PP(( "TLS on %O: %O\n", port, tls_error(t) ));
		}
	}
#endif // fall thru
#if HAS_PORT(PSYC_PORT, PSYC_PATH) &&! AUTODETECT
    case PSYC_PORT:
#endif
#if HAS_PORT(PSYC_PORT, PSYC_PATH) || HAS_PORT(PSYCS_PORT, PSYC_PATH)
# ifdef DRIVER_HAS_CALL_BY_REFERENCE
	arg = ME;
	query_ip_number(&arg);
	// this assumes network byte order provided by driver
	peerport = pointerp(arg) ? (arg[2]*256 + arg[3]) : 0;
	if (peerport < 0) peerport = 65536 + peerport;
	// no support for non-AF_INET nets yet
	if (peerport == PSYC_SERVICE) peerport = 0;
# else
	// as long as the object names don't collide, this is okay too
	peerport = 65536 + random(9999999);
# endif
# ifdef DRIVER_HAS_RENAMED_CLONES
	unless (service) service = "psyc";
	t = "S:"+ service + ":"+ query_ip_number();
	// tcp peerports cannot be connected to, so we use minus
	if (peerport) t += ":-"+peerport;
# else
	t = clone_object(PSYC_PATH "server");
# endif
	// the psyc backend distinguishes listen ports from peers using minus
	D3(D(S("%O -> load(%O, %O)\n", t, query_ip_number(), -peerport));)
	return t -> load(query_ip_number(), -peerport);
#endif

// dedicated SPYC port.. should not be used, we have AUTODETECT
#if HAS_PORT(SPYCS_PORT, SPYC_PATH)
    case SPYCS_PORT:	// interim name for PSYC 1.0 according to SPEC
# if __EFUN_DEFINED__(tls_want_peer_certificate)
        tls_want_peer_certificate(ME);
# endif
	t = tls_init_connection(this_object());
	if (t < 0 && t != ERR_TLS_NOT_DETECTED) PP(( "TLS on %O: %O\n",
			   port, tls_error(t) ));
#endif // fall thru
#if HAS_PORT(SPYC_PORT, SPYC_PATH)
    case SPYC_PORT:
#endif
#if HAS_PORT(SPYC_PORT, SPYC_PATH) || HAS_PORT(SPYCS_PORT, SPYC_PATH)
# if __EFUN_DEFINED__(enable_binary)
	enable_binary(ME);
# endif
# ifdef DRIVER_HAS_CALL_BY_REFERENCE
	arg = ME;
	query_ip_number(&arg);
	// this assumes network byte order provided by driver
	peerport = pointerp(arg) ? (arg[2]*256 + arg[3]) : 0;
	if (peerport < 0) peerport = 65536 + peerport;
	// no support for non-AF_INET nets yet
	if (peerport == PSYC_SERVICE) peerport = 0;
# else
	// as long as the object names don't collide, this is okay too
	peerport = 65536 + random(9999999);
# endif
# ifdef DRIVER_HAS_RENAMED_CLONES
	t = "S:spyc:"+query_ip_number();
	// tcp peerports cannot be connected to, so we use minus
	if (peerport) t += ":-"+peerport;
# else
	t = clone_object(SPYC_PATH "server");
# endif
	// the psyc backend distinguishes listen ports from peers using minus
	D3(D(S("%O -> load(%O, %O)\n", t, query_ip_number(), -peerport));)
	return t -> load(query_ip_number(), -peerport);
#endif

#if HAS_PORT(POP3S_PORT, POP3_PATH)
    case POP3S_PORT:
	t = tls_init_connection(this_object());
	if (t < 0 && t != ERR_TLS_NOT_DETECTED) PP(( "TLS on %O: %O\n",
			   port, tls_error(t) ));
	return clone_object(POP3_PATH "server");
#endif
#if HAS_PORT(POP3_PORT, POP3_PATH)
    case POP3_PORT:
	return clone_object(POP3_PATH "server");
#endif

#if HAS_PORT(SMTPS_PORT, NNTP_PATH)
    case SMTPS_PORT:
	t = tls_init_connection(this_object());
	if (t < 0 && t != ERR_TLS_NOT_DETECTED) PP(( "TLS on %O: %O\n",
			   port, tls_error(t) ));
	return clone_object(SMTP_PATH "server");
#endif
#if HAS_PORT(SMTP_PORT, SMTP_PATH)
    case SMTP_PORT:
	return clone_object(SMTP_PATH "server");
#endif

// heldensagas little http app
#if HAS_PORT(SHT_PORT, SHT_PATH)
    case SHT_PORT:
	return clone_object(SHT_PATH "server");
#endif

#if HAS_PORT(NNTPS_PORT, NNTP_PATH)
    case NNTPS_PORT:
	t = tls_init_connection(this_object());
	if (t < 0 && t != ERR_TLS_NOT_DETECTED) PP(( "TLS on %O: %O\n",
			   port, tls_error(t) ));
	return clone_object(NNTP_PATH "server");
#endif
#if HAS_PORT(NNTP_PORT, NNTP_PATH)
    case NNTP_PORT:
	return clone_object(NNTP_PATH "server");
#endif

#if HAS_PORT(JABBERS_PORT, JABBER_PATH)
    case JABBERS_PORT:
	t = tls_init_connection(this_object());
	if (t < 0 && t != ERR_TLS_NOT_DETECTED) PP(( "TLS on %O: %O\n",
			     port, tls_error(t) ));
	return clone_object(JABBER_PATH "server");
#endif
#if HAS_PORT(JABBER_PORT, JABBER_PATH)
    case JABBER_PORT:
# if __EFUN_DEFINED__(enable_telnet)
	enable_telnet(0);   // are you sure!???
# endif
	return clone_object(JABBER_PATH "server");
#endif

#if HAS_PORT(JABBER_S2S_PORT, JABBER_PATH)
    case JABBER_S2S_PORT:
# ifdef DRIVER_HAS_CALL_BY_REFERENCE
	arg = ME;
	query_ip_number(&arg);
	// this assumes network byte order provided by driver
	peerport = pointerp(arg) ? (arg[2]*256 + arg[3]) : 0;
	if (peerport < 0) peerport = 65536 + peerport;
	if (peerport == JABBER_S2S_SERVICE) peerport = 0;
# else
	// as long as the object names don't collide, this is okay too
	peerport = 65536 + random(9999999);
# endif
# if __EFUN_DEFINED__(enable_telnet)
	enable_telnet(0);
# endif
	t = "S:xmpp:"+query_ip_number();
	// it's just an object name, but let's be consequent minus peerport
	if (peerport) t += ":-"+peerport;
# ifdef _flag_log_sockets_XMPP
	SIMUL_EFUN_FILE -> log_file("RAW_XMPP", "\n\n%O: %O -> load(%O, %O)",
				ME, t,
#  ifdef _flag_log_hosts
                                query_ip_number(),
#  else
                                "?",
#  endif
                                -peerport);
# endif
	P3(("%O -> load(%O, %O)\n", t, query_ip_number(), -peerport))
	return t -> load(query_ip_number(), -peerport);
#endif
#if 0 //__EFUN_DEFINED__(enable_binary)
    // work in progress
    case 8888:
        enable_binary();
        enable_telnet(0);
        return clone_object(NET_PATH "socks/protocol");
    case 1935:
        enable_binary();
        enable_telnet(0);
        return clone_object(NET_PATH "rtmp/protocol");
#endif
#if HAS_PORT(IRCS_PORT, IRC_PATH)
    case IRCS_PORT:
	t = tls_init_connection(this_object());
	if (t < 0 && t != ERR_TLS_NOT_DETECTED) PP(( "TLS on %O: %O\n",
			     port, tls_error(t) ));
	return clone_object(IRC_PATH "server");
#endif
#if HAS_PORT(IRC_PORT, IRC_PATH)
    case IRC_PORT: // we could enable AUTODETECT for this..
# if 0 // __EFUN_DEFINED__(enable_telnet)
	enable_telnet(0);	// shouldn't harm.. but it does!!!
# endif
	return clone_object(IRC_PATH "server");
#endif

#if HAS_PORT(APPLET_PORT, APPLET_PATH)
    case APPLET_PORT:
# if __EFUN_DEFINED__(enable_telnet)
	// enable_telnet(0);
# endif
	return clone_object(APPLET_PATH "server");
#endif

#if HAS_PORT(TELNETS_PORT, TELNET_PATH)
    case TELNETS_PORT:
	t = tls_init_connection(this_object());
	if (t < 0 && t != ERR_TLS_NOT_DETECTED) PP(( "TLS on %O: %O\n",
			     port, tls_error(t) ));
	// we could do the UID2NICK thing here, too, but why should we?
	// what do you need tls for on a localhost tcp link?
        return clone_object(TELNET_PATH "server");
#endif
#if HAS_PORT(TELNET_PORT, TELNET_PATH)
    case TELNET_PORT: // we could enable AUTODETECT for this.. (wait 4s)
//	set_prompt("> ");
        t = clone_object(TELNET_PATH "server");
# ifdef UID2NICK
        if (uid && (arg = UID2NICK(uid))) { t -> sName(arg); }
# endif
	return t;
#endif

#if HAS_PORT(HTTP_PORT, HTTP_PATH) && AUTODETECT
    case HTTP_PORT: // AUTODETECT on the HTTP port
#endif
#if HAS_PORT(HTTPS_PORT, HTTP_PATH)
    case HTTPS_PORT:
	t = tls_init_connection(this_object());
	if (t < 0) {
		D1( if (t != ERR_TLS_NOT_DETECTED) PP(( "TLS(%O) on %O: %O\n",
					t, port, tls_error(t) )); )
#if !HAS_PORT(HTTP_PORT, HTTP_PATH)
		// if we have no http port, it may be intentional
                return (object)0;
#endif
	}
	D2( else if (t > 0) PP(( "Setting up TLS connection in the background.\n" )); )
	D2( else PP(( "Oh yeah, I'm initializing an https session!\n" )); )
	return clone_object(HTTP_PATH "server");
#endif
	/* don't fall thru. allow for https: to be available without http: */
#if HAS_PORT(HTTP_PORT, HTTP_PATH) &&! AUTODETECT
    case HTTP_PORT:
	return clone_object(HTTP_PATH "server");
#endif

#if HAS_PORT(MUDS_PORT, MUD_PATH)
    case MUDS_PORT:
	t = tls_init_connection(this_object());
	if (t < 0 && t != ERR_TLS_NOT_DETECTED) PP(( "TLS on %O: %O\n",
			     port, tls_error(t) ));
	return clone_object(MUD_PATH "login");
#endif
#if HAS_PORT(MUD_PORT, MUD_PATH)
    default:
	// if you want to multiplex psyced with an LPMUD game
//	set_prompt("> ");
	return clone_object(MUD_PATH "login");
#endif
    }
    
    PP(("Received connection on port %O which isn't configured.\n",
	port));
    return (object)0;
}


#ifdef DRIVER_HAS_RENAMED_CLONES
// named clones		-lynx
object compile_object(string file) {
	string path, name;
	object rob;

	P3((">> compile_object(%O)\n", file))
# ifdef PSYC_PATH
	if (abbrev("S:psyc:", file)) {
		rob = clone_object(PSYC_PATH "server");
		D2(if (rob) PP(("NAMED CLONE: %O => %s\n", rob, file));)
		return rob;
	}
	if (abbrev("psyc:", file)) {
#  ifdef USE_SPYC
#  echo Using SPYC by default! Yeeha!
		rob = clone_object(SPYC_PATH "active");
#  else
		rob = clone_object(PSYC_PATH "active");
#  endif
		D2(if (rob) PP(("NAMED CLONE: %O => %s\n", rob, file));)
		return rob;
	}
# endif
# ifdef SPYC_PATH
	if (abbrev("S:spyc:", file)) {
		rob = clone_object(SPYC_PATH "server");
		D2(if (rob) PP(("NAMED CLONE: %O => %s\n", rob, file));)
		return rob;
	}
	if (abbrev("spyc:", file)) {
		rob = clone_object(SPYC_PATH "active");
		D2(if (rob) PP(("NAMED CLONE: %O => %s\n", rob, file));)
		return rob;
	}
# endif
# ifdef HTTP_PATH
        // match both http:/ and https:/ objects  ;D
	if (abbrev("http", file)) {
		rob = clone_object(HTTP_PATH "fetch");
		// driver has the habit of removing double slash in object name
		file = replace(file, ":/", "://");
		if (rob) rob->fetch(file[..<3]);
		return rob;
	}
	if (abbrev("xmlrpc:", file)) {
	    rob = clone_object(HTTP_PATH "xmlrpc");
	    if (rob) rob->fetch("http://" + file[7..<3]);
	    return rob;
	}
# endif
	if (sscanf(file, "place/%s.c", name) && name != "") {
#ifdef SANDBOX
		string t;
#endif
		unless (name = SIMUL_EFUN_FILE->legal_name(name, 1))
		    return (object)0;

#ifdef _flag_enable_module_microblogging
		string username, channel;
		if (sscanf(file, "place/~%s#%s", username, channel)) {
		    object p = SIMUL_EFUN_FILE->summon_person(username, NET_PATH "user");
		    unless (p && !p->isNewbie()) {
			P3(("PLACE %O NOT CLONED: %O isn't a registered user\n", name, username));
			return (object)0;
		    }

		    if (rob = clone_object(NET_PATH "place/userthreads")) {
			PP(("PLACE CLONED: %O becomes %O\n", rob, file));
			rob->sName(name);
			return rob;
		    } else {
			P3(("ERROR: could not clone place %O\n", name));
			return (object)0;
		    }
		}
#endif

#ifdef SANDBOX
		if (file_size(t = USER_PATH + name + ".c") != -1) {
		    rob = t -> sName(name);
		    D2(if (rob) PP(("USER PLACE loaded: %O becomes %O\n", rob, file));)
		} else {
#endif

#ifdef _flag_disable_places_arbitrary
		    P2(("WARN: cloned places disabled by #define %O\n", file))
		    return (object)0;
#else
#ifdef _path_archetype_place_default
		    rob = clone_object(_path_archetype_place_default);
#else
		    rob = clone_object(NET_PATH "place/default");
#endif
		    rob -> sName(name);
		    D2(if (rob) PP(("PLACE CLONED: %O becomes %O\n", rob, file));)
#endif
#ifdef SANDBOX
		}
#endif
		return rob;
	}
	if (sscanf(file, "%s/text.c", path) && path != "") {
		rob = clone_object(NET_PATH "text");
		rob -> sPath(path);
		D2(if (rob) PP(("DB CLONED: %O becomes %s/text\n", rob, path));)
		return rob;
	}
	if (sscanf(file, "%s#%s.c", path, name) && name != "") {
		unless (name = SIMUL_EFUN_FILE->legal_name(name))
		    return (object)0;
		rob = clone_object(path);
		rob -> sName(name);
		D2(if (rob) PP(("NAMED CLONE: %O becomes %s of %s\n",
				rob, name, path));)
		return rob;
	}
# ifdef JABBER_PATH
	if (abbrev("S:xmpp:", file)) {
		rob = clone_object(JABBER_PATH "gateway");
		D2(if (rob) PP(("NAMED CLONE: %O => %s\n", rob, file));)
		return rob;
	}
	if (abbrev("C:xmpp:", file)) {
		rob = clone_object(JABBER_PATH "active");
		D2(if (rob) PP(("NAMED CLONE: %O => %s\n", rob, file));)
		return rob;
	}
# endif
	P3(("WARN: could not create %O\n", file))
	return (object)0;
}
#endif

