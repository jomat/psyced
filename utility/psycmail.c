// UNIX MAIL FILTER FOR MAIL RECEPTION NOTIFICATION
// http://about.psyc.eu/psycmail
//
//// written by Tobias 'heldensaga' Josefowitz and Carlo 'lynX' v. Loesch.
//// based on Carlo's original version in perl.
//
// this program is actually useful and in productive use -
// it can be used as filter by procmail and will forward
// sender and subject to an UNI on a psyc server - so it's
// some sort of a textual remote biff
//
// typical usage in .procmailrc:
//
//        :0 hc
//        |/usr/local/mbin/psycmail psyc://psyced.org/~user
//
// or in .forward:
//
//        \user,|"/usr/depot/mbin/psycmail psyc://psyced.org/~user"
//
// "standalone" implementation currently not using a psyc library
//
// this psycmail can also be compiled straight into procmail.
// simply put this file into the 'src' dir of procmail, then apply
// the procmail.patch. see also http://about.psyc.eu/procmail
//
#include <stdio.h>
#include <string.h>	/* was strings.h */
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>

#define unless(x)	if(!(x))
#define	M_NONE		0
#define	M_FROM		1
#define	M_SUBJECT	2

#ifdef PROG
# define USE_PSYCBIFF
#else
# define PROG		"psycmail"
#endif

/* PSYCBIFF API:	symlynX 2007
 *
 * 'relay'	- if given, a nearby server willing to relay your messages
 * 		  typically a psyced running on psyc://127.0.0.1
 * 'recipient'	- final destination of your message. if a relay is in place
 * 		  you can provide non-psyc recipients here, like xmpp:
 * 'from'	- a string describing the origin of the e-mail
 * 'subject'	- a string describing the subject of the e-mail
 *
 * return value:  anything which is not 0 means it didn't work
 */
int psycbiff(char *relay, char *recipient, char *from, char *subject) {
    int port = 4404, sck, i;
    char entity[101], hostname[101], hostname2[101], buffer[1001], *host, *uniform;
    struct hostent *hp;
    struct sockaddr_in sck_in;
    struct in_addr ip;

#ifdef PSYC_DEBUG
    fprintf(stderr, PROG ": relay %s, recipient %s, from %s, subject %s.\n",
	    relay, recipient, from, subject);
#endif
    sck = socket(PF_INET, SOCK_DGRAM, 0);
    if (sck == -1) {
	fprintf(stderr, PROG ": could not bind socket at line %d.\n", __LINE__ - 2);
	return 1;
    }

    /*	*relay ensures it's not an empty string -
	never happens when called from main() but
	this code is also used from procmail where
	a user may set PSYCRELAY=
    */
    uniform = relay && *relay? relay: recipient;

    if (strlen(uniform) > 100) {
	fprintf(stderr, PROG ": uniform '%s' is too long, line %d.\n", uniform, __LINE__ - 1);
	return 2;
    }
    unless (sscanf(uniform, "psyc://%100[^/]/%100s", hostname2, entity) == 2 ||
	    sscanf(uniform, "psyc://%100s", hostname2) == 1) {
	fprintf(stderr, PROG ": could not parse uniform '%s', line %d.\n", uniform, __LINE__ - 1);
	return 3;
    }
    i = strlen(hostname2)-1;
    if (i && hostname2[i] == '/') hostname2[i] = '\0';

    unless (index(hostname2, ':') == NULL) {
	unless (sscanf(hostname2, "%[^:]:%d", hostname, &port) == 2) {
	    fprintf(stderr, PROG ": could not parse host:port in '%s', line %d.\n", uniform, __LINE__ - 1);
	    return 4;
	}
    } else {
	// maybe just set hostname = hostname2 here? hmm...
	strncpy(hostname, hostname2, (sizeof hostname) - 1);
    }

    if (sscanf(hostname, "%d.%d.%d.%d", &i, &i, &i, &i) == 4) {
	unless (inet_aton(hostname, &ip)) {
	    fprintf(stderr, PROG ": inet_aton failed at line %d.\n", __LINE__ - 1);
	    return 5;
	}

	sck_in.sin_addr = ip;
    } else {
	hp = gethostbyname(hostname);

	unless (hp) {
	    sleep(5);
	    hp = gethostbyname(hostname);
	}

	if (hp) {
	    snprintf(buffer, (sizeof buffer) -1, "%d.%d.%d.%d",
		     hp->h_addr[0] & 255,
		     hp->h_addr[1] & 255,
		     hp->h_addr[2] & 255,
		     hp->h_addr[3] & 255);

	    unless (inet_aton(buffer, &ip)) {
		fprintf(stderr, PROG ": inet_aton failed of '%s' at line %d.\n", buffer, __LINE__ - 1);
		return 6;
	    }

	    sck_in.sin_addr = ip;
	} else {
	    fprintf(stderr, PROG ": could not resolve '%s' for '%s'\n", hostname, uniform);
	    return 7;
	}
    }

    sck_in.sin_port = htons(port);
    sck_in.sin_family = AF_INET;

    if (connect(sck, (struct sockaddr*) &sck_in, (sizeof sck_in)) == -1) {
	fprintf(stderr, PROG ": connect failed at line %d. errno: %d.\n", __LINE__ - 1, errno);
	return 8;
    }

    host = getenv("HOST");

    if (host == NULL) {
	host = "";
    }

    // could learn to extract u@dom from "from" so we can
    // set _source_relay mailto:u@dom and _nick_long <fullname>
    // see also psycmail.pl
    //
    snprintf(buffer, (sizeof buffer) - 1, ".\n\
:_target\t%s\n\
\n\
:_origin\t%s\n\
:_subject\t%s\n\
:_nick_alias\t%sMail\n\
_notice_received_email\n\
([_nick_alias]) [_origin]: [_subject]\n\
.\n", recipient, from, subject, host);

    if (send(sck, buffer, strlen(buffer), 0) == -1) {
	fprintf(stderr, PROG ": could not send at line %d.\n", __LINE__ - 1);
	return 9;
    }

    return 0;
}


#ifndef USE_PSYCBIFF
int main(int argc, char **argv) {
    char from[301] = "", subject[301] = "", buffer[1001], key[31], value[301];
    char *relay = NULL, *target;
    int last = M_NONE, space;

    switch (argc) {
    case 2:
	target = argv[1];
	break;
    case 4:
	if (!strcmp(argv[1], "-p")) {
	    relay = argv[2];
	    target = argv[3];
	    break;
	} 
	// fall thru
    default:
	fprintf(stderr, "UNIX MAIL FILTER FOR MAIL RECEPTION NOTIFICATION\n\
\n\
typical usage in .procmailrc:\n\
	:0 hc\n\
	|%s psyc://psyced.org/~user\n\
\n\
or in .forward:\n\
	\\user,|\"%s psyc://psyced.org/~user\"\n\
\n\
you can also use its proxy relay mode, primarily for non-psyc targets:\n\
	%s -p psyc://localhost xmpp:user@example.org\n\
\n",
		argv[0], argv[0], argv[0]);
	return 1;
    }

    while (fgets(buffer, (sizeof buffer) -1, stdin) != NULL) {
	    // this sscanf is quite a hack, %s does only match non-whitespace-
	    // characters. as there shouldn't be \n's in buffers read by fgets,
	    // i'm just matching everything but \n for the value...
	    // anybody got an idea how to make it better?
	if (last != M_NONE && buffer[0] == ' ') {
	    if (last == M_FROM && ((space = (sizeof from) - strlen(from)) > 1)) {
		strncat(from, buffer, space >= strlen(buffer) ?
			strlen(buffer) - 1 : space - 1);
		continue;
	    }
	    if (last == M_SUBJECT && ((space = (sizeof subject) - strlen(subject))
				    > 1)) {
		strncat(subject, buffer, space >= strlen(buffer) ?
			strlen(buffer) - 1 : space - 1);
		continue;
	    }
	} else last = M_NONE;

	if (sscanf(buffer, "%30[^:]: %300[^\n]", key, value) == 2) {
	    if (strcasecmp(key, "subject") == 0) {
		strncpy(subject, value, (sizeof subject) - 1);
		last = M_SUBJECT;
	    } else if (strcasecmp(key, "from") == 0) {
		strncpy(from, value, (sizeof from) - 1);
		last = M_FROM;
	    }
	} else {
	    if(strlen(buffer) == 1) {
		break;
	    }
	}
    }

    return psycbiff(relay, target, from, subject);
}
#endif

