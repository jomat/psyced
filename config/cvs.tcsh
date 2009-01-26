# you can also create a ~/.cvsrc where you keep your favorite
# cvs flags. we recommend to put 'cvs -z9' in there

#alias	cup	cvs -q update -d  # was, until de/irc was deleted
alias	cup	cvs -q update -dP

alias	ci	cvs ci
alias	ci+	cvs ci -m +
alias	co	cvs co

# recursive cvsrm is a shell script
#alias	crm	'rm -rf \!*;cvs rm \!*'

alias	cmd	'mkdir \!*;cvs add \!*;cd \!*'
alias	cmf	'$EDITOR \!*;cvs add \!*'
alias	cvsaddr 'cvs add `find . -name "*CVS*" -prune -o -print`'
# how to add a binary file.. cause i never remember!!
alias	cvsaddbin 'cvs add -kb'

alias	canno	'cvs annotate \!*|vim -R -'
alias	cdif	'cvs diff -bur HEAD \!* |& egrep -v " (Diffing |no longer exists)"|vim -R "+set syntax=diff" -'

alias	Ci	'(cd $PSYCEDHOME;ci)'
alias	Cup	'(cd $PSYCEDHOME;cup)'
alias	Cdif	'(cd $PSYCEDHOME;cdif)'

alias	ctoc	'(cd $CVSHOME;cvs diff -bur HEAD CHANGESTODO|vim -R "+set syntax=diff" -)'
alias	ctodo	'(cd $PSYCEDHOME;cvs update CHANGESTODO;x CHANGESTODO;cvs ci -m + CHANGESTODO)'

