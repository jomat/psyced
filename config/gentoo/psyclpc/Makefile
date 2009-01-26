# $Id: Makefile,v 1.7 2008/07/07 18:35:59 lynx Exp $
#
# This Makefile serves the purpose to produce the files ebuild needs to be
# happy. So it is an ebuildbuilder. Targets are: ebuild, Manifest and clean.
#
# 'make' needs to be called in gentoo/config to generate the ebuilds and files
# directory, then ../Makefile will transfer this to portage where 'make' will
# call ebuild to create digests and the Manifest. After that you are ready to
# emerge. From nothing. fischerspooner rocks.	-lynX

P=psyclpc
V=20080707

# this part can be executed once the ebuild is in portage

it: Manifest
#	# testing it
	emerge -av $P

Manifest: Makefile ChangeLog
#	# generate manifest
	ebuild $P-$V.ebuild digest

# this part is to be executed in the psyced config/gentoo dir
# to create the files necessary for the ebuild to run

ebuild: $P-$V.ebuild

$P-$V.ebuild:
#	# let's keep it this simple, please
	-ln ../$P.ebuild $P-$V.ebuild

clean:
	-rm $P-$V.ebuild

