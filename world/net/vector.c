// vim:syntax=lpc
#include <net.h>

// this isn't actually faster than expanding arrays. in fact, it's way slower.
// do not use until i find a search tree which makes this faster than array
// concatenation!

#ifndef BASE
# define BASE		8
#endif

#define I_TO		0
#define I_SUB		1
#define I_ISVALUE	2
#define I_NEXT		3

#if BASE == 2
# define BP(x) ((x) ? (2<<((x)-1)) : 1)
#elif BASE == 8
# define BP(x) ((x) ? (8<<(3*((x)-1))) : 1)
#else
# define NEED_TP
# define BP(x)	tp(x)
#endif

private int _length, _max, _levels, _height;
private mixed *structure;

int length() { return _length; }

#ifdef NEED_TP
private int tp(int p) {
    int res = BASE;

    //PT(("}} %O\n", p))

    unless (p) {
	return 1;
    }

    while (--p) {
	res *= BASE;
    }

    return res;
}
#endif

void vAdd(mixed item) {
    mixed *cur;
    int level;

    //PT(("adding %O, length %O, max %O\n", item, _length, _max))
    unless (_length) {
	_max = BASE;
	_length++;
	structure = ({ 0, item, 1, 0 });
	return;
    }

    if (_length == _max) {
	int tmax = _max;

	_max *= BASE;
	_height++;

	structure = ({ tmax-1, structure, 0, 0 });
    }

    cur = structure;

    for (;;) {
	//PT(("%O, %O\n", cur[I_TO], _length))
	if (cur[I_TO] < _length) {
	    unless (cur[I_NEXT]) {
		int t;

		//cur = cur[I_NEXT] = ({ (t = cur[I_TO]) + to_int(pow(10, _height-level)), 0, 0, 0 });
		cur = cur[I_NEXT] = ({ (t = cur[I_TO]) + BP(_height-level), 0, 0, 0 });

		//PT((">> %O; %O, %O\n", _height, level, cur))
		while (_height-level++) {
		    //cur = cur[I_SUB] = ({ t + to_int(pow(10, _height-level)), 0, 0, 0 });
		    cur = cur[I_SUB] = ({ t + BP(_height-level), 0, 0, 0 });
		}

		cur[I_SUB] = item;
		cur[I_ISVALUE] = 1;
		
		_length++;
		return;
	    }
	    cur = cur[I_NEXT];
	} else if (cur[I_ISVALUE]) {
	    raise_error("DUH?\n");
	    cur[I_SUB] = item;
	    _length++;
	    return;
	} else {
	    cur = cur[I_SUB];
	    level++;
	}
    }
}

void debug() {
    PT((">> %O\n", structure))
    PT((">> %d, %d, %d, %d\n", BP(0), BP(1), BP(2), BP(3)))
}

vGet(int index) {
    mixed *cur = structure;
    int steps;

    if (index >= _length) {
	raise_error("index out of bounds\n");
    }

    for (;;) {
	if (cur[I_TO] < index) {
	unless (pointerp(cur[I_NEXT])) {
	    PT((":: %O\n", cur))
	}
	    cur = cur[I_NEXT];
	    steps++;
	} else if (cur[I_ISVALUE]) {
	    PT(("did %d steps\n", steps))
	    return cur[I_SUB];
	} else {
	    cur = cur[I_SUB];
	    steps++;
	}
    }
}
