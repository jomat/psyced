# $Id: Makefile,v 1.37 2007/08/14 11:28:00 lynx Exp $

p=/usr/local/portage
e=/etc/make.conf
G=lynx -source

# We currently operate on /usr/local/portage but we should add more
# PSYC packages and run our own PSYC overlay...

install: $p $p/dev-lang/psyclpc $p/net-im/psyced
	@echo -- We are done with portage installation. You can now emerge
	@echo -- from here or postpone to a better moment in life.
	emerge -av psyced
	@echo -- Done emerging. Contratulations on your new psyced! --

uninstall: new

new:
# It complains for missing packages, but does no harm.
	-emerge --unmerge psyced psyclpc ldmud
# This one complains, too. Should we make it shut up?
# We can make it shut up next time when we do our own PSYC overlay.
	-rm -r $p/net-im/psyced $p/dev-lang/psyclpc $p/games-mud/ldmud

ldmud: $p $p/games-mud/ldmud
	@echo -- Warning: You are installing LDMud which is no longer the
	@echo -- the best choice as a psyced driver!
	emerge -av ldmud

### NO SERVICEABLE PARTS BELOW ###

$p:
	@echo -- Apparently you have not installed custom ebuilds yet.
	mkdir $@
	@echo -- A $@ directory has therefore been generated for you.
	echo "" >>$e
	echo "# Added by psyced's config/gentoo/Makefile" >>$e
# Does it replace $p correctly in $e? Yes it does. Good.
	echo "PORTDIR_OVERLAY=$p" >>$e
	@echo -- PORTDIR_OVERLAY has been appended to $e to activate $@.

$p/dev-lang/psyclpc: psyclpc.ebuild psyclpc/Makefile Makefile
	(cd psyclpc;make ebuild)
	-mkdir $p/dev-lang
	rm -rf $@
	cp -rp psyclpc $@
	-(cd $@;make Manifest)

$p/games-mud/ldmud: ldmud.ebuild ldmud/Makefile Makefile
	(cd ldmud;make ebuild)
	-mkdir $p/games-mud
	rm -rf $@
	cp -rp ldmud $@
	-(cd $@;make Manifest)

$p/net-im/psyced: psyced.ebuild psyced/Makefile Makefile
	(cd psyced;make ebuild)
	-mkdir $p/net-im
	rm -rf $@
	cp -rp psyced $@
	-(cd $@;make Manifest)

# the lower part of the Makefile serves the purpose of
# managing this package itself, not to install something.

sane: new clean

clean:
	(cd psyced;make $@)
	(cd psyclpc;make $@)
# Leave it alone or we won't be able to recreate the erq directory  ;)
#	(cd ldmud;make $@)

up:
	-rm ldmud/ldmud-*.ebuild psyced/psyced-*.ebuild psyclpc/psyclpc-*.ebuild
	-mkdir psyced/files
	# If 'cvs update' updates this Makefile itself, it may be necessary
	# to run 'make up' twice as the update requirements may have changed.
	cvs update
	$G http://www.psyced.org/dist/config/psyced.ini >psyced/files/psyced.ini
	@(cd psyced;make ebuild)
	@(cd psyclpc;make ebuild)
# We could extend the procedure to actually run the ebuild at building time
# so that we get to have the digests in there.. TODO
	# Now you may have to unmerge and delete psyclpc and psyced from your
	# portage overlay to have them generated anew.
	# Use 'make new' to do that.

oldup:
	@(cd ldmud;make ebuild)
	$G http://www.psyced.org/dist/config/psyced.settings >ldmud/files/psyced.settings

