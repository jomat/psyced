# $Id: Makefile,v 1.16 2007/04/08 08:37:08 lynx Exp $
#
# This Makefile serves the purpose to produce the files ebuild needs to be
# happy. So it is an ebuildbuilder. Targets are: ebuild, Manifest and clean.
#
# 'make' needs to be called in gentoo/config to generate the ebuilds and files
# directory, then ../Makefile will transfer this to portage where 'make' will
# call ebuild to create digests and the Manifest. After that you are ready to
# emerge. From nothing. fischerspooner rocks.	-lynX

P=ldmud
V=3.3.714

# this part can be executed once the ebuild is in portage

it: Manifest
#	# testing it
	emerge -av $P

Manifest: Makefile ChangeLog files/psyced.settings files/erq
#	# generate manifest
	ebuild $P-$V.ebuild digest

# this part is to be executed in the psyced config/gentoo dir
# to create the files necessary for the ebuild to run

ebuild: $P-$V.ebuild files/psyced.settings files/erq

$P-$V.ebuild:
#	# let's keep it this simple, please
	-ln ../$P.ebuild $P-$V.ebuild

# no dependency on ../../psyced.settings here or it won't run in portage
files/psyced.settings: files
#	# we keep it elsewhere
	-cp ../../psyced.settings $@

files/erq:
	-mkdir $@
	-cp -p ../../../utility/erq/*.* $@

files:
	-mkdir files
# we don't want to have each of those in version control, so this is a place too
#	echo "MD5 419eb1c21747fe2cd6de8089b47271ff ldmud-3.3.687.tar.bz2 2107415" >files/digest-ldmud-3.3.687
#	echo "MD5 ab0027411928bef96aa41bf316cde954 ldmud-3.3.712.tar.gz 26515635" >files/digest-ldmud-3.3.712

clean:
	-rm $P-$V.ebuild
	-rm -r files

