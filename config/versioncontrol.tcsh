alias	difvu	'vim -R "+set syntax=diff"'

alias	cup	cvs -q update -dP
alias	sup	svn update
alias	gup	'(git fetch origin && git diff master..origin/master && git merge origin) |& difvu -'

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

alias	cblame	'cvs annotate \!*|& $PAGER'

alias   cdiff   'cvs diff -bpu8r'
alias   gdiff   'git-diff -b'
alias   cdifr   'cdiff \!* |& egrep -v " (Diffing |no longer exists)"|& difvu -'
alias   sdifr   'svn diff -r \!* |& difvu -'
alias   gdifr   'gdiff \!* |& difvu -'
alias   cdif    'cdifr HEAD'
alias   sdif    'sdifr HEAD'
alias   gdif    'gdifr HEAD'

alias	Ci	'(cd $PSYCEDHOME;ci)'
alias	Cup	'(cd $PSYCEDHOME;cup)'
alias	Cdif	'(cd $PSYCEDHOME;cdif)'

alias	ctoc	'(cd $CVSHOME;cvs diff -bur HEAD CHANGESTODO|& difvu -'
alias	ctodo	'(cd $PSYCEDHOME;cvs update CHANGESTODO;$EDITOR CHANGESTODO;cvs ci -m + CHANGESTODO)'
alias	todo	'(cd $PSYCEDHOME;$EDITOR CHANGESTODO)'

# edit all files that contain a certain keyword
# uses the +/searchstring syntax supported by vi and other smart editors
alias   gred	'$EDITOR "+/\!*" `grep -swIrl \!* .`'
# grep -r is a bit stupid: it follows symlinks, then shows us files twice
# so using the git full text index may be smarter and faster
alias	ggred	'$EDITOR "+/\!*" `git grep -w -l \!*`'

