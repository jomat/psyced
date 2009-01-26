
Since you unpacked this you probably want to install it.
You can go straight to the INSTALL.txt file for instructions.
If it is missing, try [1]http://muve.pages.de/INSTALL.html



                PROTOCOL for SYNCHRONOUS CONFERENCING
            ___   __  _   _   __    _    _ _  _ _   _ ___
            |  \ (__   \ /   /      |\  /| |  | |   | |
            |__/    \   V   |       | \/ | |  |  \ /  |-
            |    (__/   |    \__    |    | |__|   V   |__

          The MULTI USER VIRTUAL ENVIRONMENT driver is here!
            =============================================



This is 'psycMUVE', the Multi User Virtual Environment for PSYC.
It is a server and gateway implementation of PSYC.

The latest version is available on [2]http://muve.pages.de/download

There is no download by FTP, since FTP has no advantage
over HTTP/1.1 (you can use HTTP REGET these days).

The project homepage is [3]http://muve.pages.de
The protocol homepage is [4]http://psyc.pages.de
The user manual resides at [5]http://help.pages.de

The Multi-User Virtual Environment for PSYC-Users isn't just a PSYC server;
it also simulates the functionality of PSYC clients allowing users of various
sorts of more or less dumb applications to enter the PSYCspace.

The professional edition of it has been in use as a Webchat for several years.
The HTML webchat, however, is not part of the freeware edition.

psycMUVE is implemented in LPC and uses a driver called LDMUD.
See INSTALL for details.

The files in the distribution directory are:
  AGENDA.txt    : future plans (not a TODO really)
  BANNER.txt    : advertisement or welcome message
  COPYLEFT.txt  : GNU GENERAL PUBLIC LICENSE
  INSTALL.txt   : installation hints and notes
  LICENSE.txt   : something you are supposed to read (copyright info)
  README.txt

  install.sh    : an installation script (ksh/bash).
  [6]makefile   : some useful functions (optional).
  bin/          : various scripts
                  but the only one you really need, "psycmuve",
                  will be created by 'install.sh'.
  config/       : depot of configurations.
                  also contains some tcsh and powwow settings.
  data/         : this is where MUVE stores your user and room data.
  local/        : your local configuration of the server
                  is created by 'install.sh' but you can also make
                  it a symlink into a config/something directory
  log/          : where the server logfiles end up.
                  may be a symlink into the /var partition.
  place/        : here you can implement your own room objects in lpc.
                  some examples of public rooms are waiting for you there.
  [7]run/       : the ldmud equivalent of a CGI directory. ldmud can spawn
                  a subprocess to do some jobs which are too hard to achieve
                  in LPC-world. we currently don't use this as ldmud provides
                  MD5 and SHA1 itself. in theory we could implement CGI for
                  the builtin webserver, but it is much better to code these
                  things in LPC. [8]world/net/jabber/component.c shows how sha1
.pl
                  is spawned when the driver does not provide SHA1.
  utility/      : the applet code and other things that may be useful.
  [9]world/     : this is the directory tree that is visible from
                  within the lpc interpreter and therefore contains
                  all the actual lpc program code.
       data/            : symlink to data/
       [10]default/     : the text database for multiple languages and formats
       [11]drivers/     : glue code to interface LPC drivers to psycMUVE
       local/           : symlink to local/
       log/             : symlink to log/
       [12]net/         : all of the psycMUVE code is in a "net" hierarchy
                        : so it can be merged with an existing MUD
       obj/             : just in case you misconfigured your driver
       place/           : symlink to place/
       [13]static/      : contains static files for httpd export
                        : you can use them with the internal httpd
                        : or copy them to yours

Don't be irritated by the fact that traditional LPC drivers keep
their LPC files with a ".c" suffix and data files with a ".o" suffix.
More oddities are described in http://muve.pages.de/DEVELOP if you want
to find your way around the psycMUVE source code.

   --
   http://muve.pages.de/README.html
   last change by lynx on fly at 2005-09-28 20:58:37 MEST

References

   1. http://muve.pages.de/INSTALL.html
   2. http://muve.pages.de/download
   3. http://muve.pages.de/
   4. http://psyc.pages.de/
   5. http://help.pages.de/
   6. http://muve.pages.de/dist/makefile
   7. http://muve.pages.de/dist/run/
   8. http://muve.pages.de/dist/world/net/jabber/component.c
   9. http://muve.pages.de/dist/world/
  10. http://muve.pages.de/dist/world/default/
  11. http://muve.pages.de/dist/world/drivers/
  12. http://muve.pages.de/dist/world/net/
  13. http://muve.pages.de/dist/world/static/
