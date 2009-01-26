#!/bin/env pike

// well, i'm using it. makes cdifs a lot nicer.
// saga.
//
// oh interesting. how does it work? does it have
// a usage: message?	-lynX

#define unless(x)	if(!(x))

int main() {
    int flag, i = -1, j, haveCuts;
    Stdio.File conf = Stdio.File();
    array(string) data;
    array(int) cuts = ({});
    mapping donts = ([]);

    unless (getenv("HOME")) {
	werror("well, set $HOME, svp.\n");
	return 1;
    }

    unless (conf->open(getenv("HOME") + "/.cvsdont", "r")) {
	werror("well, could not open " + getenv("HOME") + "/.cvsdont\n");
	return 2;
    }

    foreach (conf->read() / "\n", string line) {
	donts[line] = 1;
    }

    data = Stdio.stdin->read() / "\n";

    if (data[-1] == "") {
	data = data[0..sizeof(data) - 2];
    }

    foreach (data, string line) {
	i++;
	unless (flag) {
	    if (sizeof(line) > 10 && line[0..9] == "RCS file: " && donts[line[10..sizeof(line) - 1]]) {
		flag = !flag;
		cuts += ({ i - 2 });
	    }
	} else {
	    if (line[0] == '=') {
		flag = !flag;
		cuts += ({ i - 1 });
	    }
	}
    }

    i = -1;
    flag = 0;

    haveCuts = sizeof(cuts);

#ifdef DEBUG
    werror("%O\n", cuts);
#endif

    if (haveCuts & 1) {
	cuts += ({ sizeof(data) });
	haveCuts++;
    }

#ifdef DEBUG
    werror("%O\n", cuts);
#endif

    foreach (data, string line) {
	i++;

#ifdef DEBUG
	if (haveCuts > j) {
	    werror("} %d %d (%d)\n", i, cuts[j], (haveCuts - 1) > j);
	} else {
	    werror("}} %d %d\n", i, j);
	}
#endif

	if (haveCuts > j && cuts[j] == i) {
#ifdef DEBUG
	    if ((haveCuts - 1) > j) {
		werror("1] %d %d\n", j, cuts[j]);
		werror("2] %d\n", cuts[j+1]);
	    } else {
		werror("] %d %d(%d)\n", j, haveCuts, sizeof(cuts));
	    }
#endif

	    unless ((haveCuts - 1) > j && i == cuts[j + 1]) {
#ifdef DEBUG
		werror(">> %d\n", i);
		werror(") %d %d %d %d\n", i, j, cuts[j], flag);
#endif
		flag = !flag;
	    } else {
#ifdef DEBUG
		werror("> %d %d\n", i, ((haveCuts - 1) > j) ? cuts[j+1] : 0);
#endif
		j++;
	    }
	    j++;
	}

	unless (flag || line[0] == '?') {
	    write("%s\n", line);
	}
    }


    return 0;
}
