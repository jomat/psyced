/* Why do we use C for shell scripts?
 * Well.. why not?
 * Because people like me need to add something and end up spending half
 * an hour for a 30 second change.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#define unless(x)	if (!(x))

void cvscp(char *filename) {
    char cmdline[1000];
    printf(">>> %s\n", filename);

    snprintf(cmdline, (sizeof cmdline) - 1, "cp %s ../skel", filename);

    if (system(cmdline)) {
	printf(">> Could not update %s. Exiting.\n", filename);
	exit(1);
    }
}

void webcp(char *filename) {
    char cmdline[1000];
    printf(">>> %s\n", filename);

    snprintf(cmdline, (sizeof cmdline) - 1, "lynx -dump http://www.psyced.org/%s.html > %s.txt", filename, filename);

    if (system(cmdline)) {
	printf(">> Could not update %s. Exiting.\n", filename);
	exit(1);
    }
}

int main(int argc, char **argv) {
    time_t t;
    struct tm *lt;
    char ft[200], s[1000];
    int l, hadskel;

    time(&t);
    lt = localtime(&t);
    /* new format without dashes to accomodate gentoo versioning style */
    strftime(ft, 199, "%Y%m%d", lt);
/*  strftime(ft, 199, "%F", lt); */

    if (chdir("skel")) {
	hadskel = 0;

	printf(">> Welcome. Today is %s. I will create a skel and data directory here.\n", ft);
	if (argc != 2) {
	    printf(">> Usage: %s psyced-<date>.tar.bz2\n", argv[0]);
	    return 1;
	}

	l = strlen(argv[1]);
	if (l < 10 || l > 30) {
	    puts(">> This doesn't look like a psyced-<date>.tar.bz2.");
	    return 1;
	}

	puts(">> Let me unpack this.");

	snprintf(s, (sizeof s) - 1, "tar xvfj %s", argv[1]);
	if (system(s)) {
	    printf(">> Could not untar %s. Exiting.\n", argv[1]);
	    return 1;
	}

	argv[1][l - sizeof("tar.bz2")] = 0;
	if (rename(argv[1], "skel")) {
	    printf(">> Could not rename %s. Exiting.\n", argv[1]);
	    return 1;
	}

	if (chdir("skel")) {
	    puts(">> Could not enter skel/. Exiting.");
	    return 1;
	}
    }
    else {
	hadskel = 1;
	puts(">> You already have a skel, good. Skipping unpack operation.");
    }

    if (!unlink(".config")) {
	puts(">> Deleted a .config that someone left lying around.");
    }

    if (mkdir("../data", S_IRWXU)) {
	puts(">> Could not create data/. Already there?");
	/* return 1; */
    } else {
	puts(">> Extracting data");

	if (system("tar xf data.tar -C../data")) {
	    puts(">> Error in extracting data.tar. Exiting.");
	    return 1;
	}
    }

    if (chdir("../data")) {
	puts(">> Could not enter data/. Exiting.");
	return 1;
    }

    /* we don't want to update CHANGESTODO. we put a fake file
       there so it doesn't download the real thing, later we delete it
    system("cp BANNER CHANGESTODO");
       hmm, doesn't really work like that
     */

    puts(">> Now is your chance to inspect a cvs diff. Suspend now.");
    sleep(4);

    puts(">> Doing a CVS update");
    if (system("cvs -q update -dP")) {
	puts(">> Error during CVS update. Exiting.");
	return 1;
    }

    /* because we want to keep it out of the snapshots anyway */
    unlink("CHANGESTODO");
    /* system("rm .#CHANGESTODO*") */

    puts(">> Now is your chance to make manual changes. Suspend now.");
    sleep(4);

    puts(">> Creating new data.tar");
    
    if (system("tar cf ../skel/data.tar --owner=root --group=root .")) {
	puts(">> Error during archiving. Exiting.");
	return 1;
    }

    puts(">> Updating files from CVS");

    cvscp("install.sh");

    puts(">> Cleaning up data/");
    chdir("..");
    if (system("rm -rf data")) {
	puts(">> Could not remove data/. Exiting.");
	return 1;
    }

    if (chdir("skel")) {
	puts(">> Could not enter skel/. Exiting.");
	return 1;
    }

    puts(">> Updating files from Webserver");

    webcp("README");
    webcp("INSTALL");
    webcp("FIRSTSTEPS");

    puts(">> Creating archive");

    snprintf(s, (sizeof s) - 1, "../psyced-%s", ft);
    if (mkdir(s, S_IRWXU)) {
	printf(">> Could not create %s/. Exiting.\n", s);
	return 1;
    }

    snprintf(s, (sizeof s) - 1, "tar cf - . | tar xf - -C../psyced-%s", ft);
    if (system(s)) {
	puts(">> Could not copy data for tarring. Exiting.");
	return 1;
    }

    chdir("..");

    snprintf(s, (sizeof s) - 1, "tar cfj psyced-%s.tar.bz2 --owner=root --group=root psyced-%s", ft, ft);

    if (system(s)) {
	printf(">> Could not create psyced-%s.tar.bz2. Exiting.\n", ft);
	return 1;
    }

    printf(">> psyced-%s.tar.bz2 has been created. Cleaning up.\n", ft);

    snprintf(s, (sizeof s) - 1, "rm -rf psyced-%s", ft);
    if (system(s)) {
	printf(">> Could not remove psyced-%s. Exiting.\n", ft);
	return 1;
    }

    if (!hadskel) {
	if (system("rm -r skel")) {
	    puts(">> I'm done, but I could not remove the skel directory.");
	    return 1;
	}
    }
    puts(">> Done.");

    return 0;
}
