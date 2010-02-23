# collection of aliases for version control systems: cvs, svn, git, hg

alias	difvu	'vim -R "+set syntax=diff"'

# update the repository
alias	cup	cvs -q update -dP
alias	sup	svn update
#lias	gup	git pull
alias	gup	'(git fetch origin && git diff master..origin/master && git merge -s resolve origin) |& difvu -'

# check in check out
alias	ci	cvs ci
alias	ci+	cvs ci -m +
alias	co	cvs co
alias	gi	'git commit -a;git push'
alias	gi+	'git commit -a -m +;git push'
alias	go	'git checkout'	    # go <branch>
# branching
alias	gcb	'git checkout -b'   # create <branch>
alias	gb	'git branch -a'	    # list all branches

# cvs make and add
#alias	cmd	'mkdir \!*;cvs add \!*;pushd \!*'
#alias	cmf	'$EDITOR \!*;cvs add \!*'
#alias	cvsaddr 'cvs add `find . -depth -name "*CVS*" -prune -o -print`'
# if that failed, here's more
##alias	cvsfr	'cvs add `find . -name "*CVS*" -prune -o -type f -print`'
# how to add a binary file, cause i never remember!
#alias	cvsaddbin 'cvs add -kb'
# crm replaced by cvsrm* scripts

# diff: see what's going on
alias	cdiff	'cvs diff -bpu8r'
alias	gdiff	'git diff -b'
alias	hdiff	'hg diff -b'
# difr <revision>
alias	cdifr	'cdiff \!* |& egrep -v " (Diffing |no longer exists)"|& difvu -'
alias	sdifr	'svn diff -r \!* |& difvu -'
alias	gdifr	'gdiff \!* |& difvu -'
alias	hdif	'hdiff \!* |& difvu -'
# used the most
alias	cdif	'cdifr HEAD'
alias	sdif	'sdifr HEAD'
alias	gdif	'gdifr HEAD'

# edit all files that contain a certain keyword
# uses the +/searchstring syntax supported by vi and other smart editors
alias	gred	'$EDITOR "+/\!*" `grep -swIrl \!* .`'
# grep -r is a bit stupid: it follows symlinks, then shows us files twice
# so using the git full text index may be smarter and faster
alias	ggred	'$EDITOR "+/\!*" `git grep -w -l \!*`'
# otherwise use rgrep -e from http://perl.pages.de

# cvs specific
alias	ctodo	'(cd $CVSHOME;$EDITOR CHANGESTODO)'
alias	cblame	'cvs annotate \!*|& $PAGER'
alias	cvsdeath 'find . -name CVS -print -prune -exec rm -r {} \;'

# how to deal with remote gits:
# first, set up "git remote add <nick> <url>"
# then "git fetch <nick>" and view with "gdifR <nick>"
alias	gdifR	'git diff master..\!:1/master |& difvu -'
# to merge all the changes, use "gmergR <nick>"
alias	gmergR	'git merge -s resolve \!:1/master'

