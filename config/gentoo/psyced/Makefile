# $Id: Makefile,v 1.20 2008/11/20 13:45:58 lynx Exp $
#
# This Makefile serves the purpose to produce the files ebuild needs to be
# happy. So it is an ebuildbuilder. Targets are: ebuild, Manifest and clean.
#
# 'make' needs to be called in gentoo/config to generate the ebuilds and files
# directory, then ../Makefile will transfer this to portage where 'make' will
# call ebuild to create digests and the Manifest. After that you are ready to
# emerge. From nothing. fischerspooner rocks.   -lynX

P=psyced
V=20090617

# this part can be executed once the ebuild is in portage

it: Manifest
#	# testing it
	emerge -av $P

Manifest: Makefile ChangeLog files/psyced.ini
#	# generate manifest
	ebuild $P-$V.ebuild digest

# this part is to be executed in the psyced config/gentoo dir
# to create the files necessary for the ebuild to run

ebuild: $P-$V.ebuild files/psyced.ini

$P-$V.ebuild:
#	# let's keep it this simple, please
	-ln ../$P.ebuild $P-$V.ebuild

# no dependency on ../../psyced.ini here or it won't run in portage
files/psyced.ini:
	-cp ../../psyced.ini $@

files:
	-mkdir files
# we don't want to have each of those in version control, so this is a place too
#	echo "MD5 bacce96afd75b8034a9247883f3765e1 psycmuve.99-gamma.tar.gz 587447" >files/digest-psycmuve.99-gamma

clean:
	-rm $P-$V.ebuild
	-rm -r files

