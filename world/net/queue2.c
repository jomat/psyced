// $Id: queue2.c,v 1.19 2008/03/29 20:05:32 lynx Exp $ // vim:syntax=lpc
//
// the *better* queue

#include <net.h>

#define Q_TOP		0
#define Q_BOTTOM	1
#define Q_SIZE		2
#define Q_MAX		3
#define Q_WIDTH		4

//#define I_PRE		0
#ifdef I_PRE
# define I_DATA		1
# define I_NEXT		2
#else
# define I_DATA		0
# define I_NEXT		1
#endif

volatile mapping q;

mapping qDebug() { return q; }

static varargs int qInit(mixed name, int max, int stinkt) {
    if (member(q, name)) return 0;
    q[name, Q_MAX] = max;
    return 1;
}

static int enqueue(mixed name, mixed item) {
    mixed *a, *n;

    P4(("queue2:enqueue(%O.. in %O from %O\n",
       	name, ME, previous_object()))
    if (q[name, Q_MAX] && (q[name, Q_SIZE] == q[name, Q_MAX])) {
	P1(("queue %O has reached maximum size (enqueue(%O))\n", name, item))
	return 0;
    }

#ifdef I_PRE
    n = allocate(3);
#else
    n = allocate(2);
#endif
    n[I_DATA] = item;
    if (a = q[name, Q_BOTTOM]) {
#ifdef I_PRE
	n[I_PRE] = a;
#endif
	q[name, Q_BOTTOM] = a[I_NEXT] = n;
    } else
	q[name, Q_BOTTOM] = q[name, Q_TOP] = n;

    q[name, Q_SIZE]++;

    return 1;
}

static mixed qExists(varargs mixed *names) {
    foreach(mixed name : names)
	if (member(q, name)) return name;
    return 0;
}

static mixed qSize(varargs mixed *names) {
    if (sizeof(names) == 1) {
	return q[names[0], Q_SIZE];
    }
    foreach (mixed name : names)
	if (q[name, Q_SIZE]) return name;
    return 0;
}

static int qRename(mixed old, mixed new) {
    if (!member(q, old) || member(q, new)) return 0;
    q += ([ new : q[old, Q_TOP]; q[old, Q_BOTTOM]; q[old, Q_SIZE];
		  q[old, Q_MAX] ]);
    m_delete(q, old);
    return 1;
}

static mixed shift(mixed name) {
    mixed *a
#ifdef I_PRE 
	    , *b
#endif
		;

    if (a = q[name, Q_TOP]) {
	if (a[I_NEXT]) {
#ifdef I_PRE
	    q[name, Q_TOP] = b = a[I_NEXT];
	    b[I_PRE] = 0;
#else
	    q[name, Q_TOP] = a[I_NEXT];
#endif
	} else
	    q[name, Q_TOP] = q[name, Q_BOTTOM] = 0;
	q[name, Q_SIZE]--;
	return a[I_DATA];
    }
    return 0;
}

static int unshift(mixed name, mixed item) {
    mixed *a, *n;

    if (q[name, Q_MAX] && (q[name, Q_SIZE] == q[name, Q_MAX])) {
	P1(("queue %O has maximum size (unshift(%O))\n", name, item))
	return 0;
    }

#ifdef I_PRE
    n = allocate(3);
#else
    n = allocate(2);
#endif
    n[I_DATA] = item;

    if (a = q[name, Q_TOP]) {
	n[I_NEXT] = a;
#ifdef I_PRE
	q[name, Q_TOP] = a[I_PRE] = n;
#else
	q[name, Q_TOP] = n;
#endif
    } else
	q[name, Q_TOP] = q[name, Q_BOTTOM] = n;
    
    q[name, Q_SIZE]++;

    return 1;
}

static int qDel(mixed name) {
    ASSERT("qDel (mappingp(q))", mappingp(q), q)
    m_delete(q, name);
    return 1;
}

static mapping qCreate() {
    unless (mappingp(q) && widthof(q) == Q_WIDTH) q = m_allocate(0, Q_WIDTH);
    P3(("qCreate in %O produces %O (%O)\n", ME, q, widthof(q)))
    return q;
}

static mixed *qToArray(mixed name) {
    mixed *tmp, *cur;
    int max;

    ASSERT("qToArray (mappingp(q))", mappingp(q), q)
    unless (member(q, name)) return 0;

    tmp = allocate(max = q[name, Q_SIZE]);

    cur = q[name];

    for (int i = 0; i < max; i++) {
	tmp[i] = cur[I_DATA];
	cur = cur[I_NEXT];
    }
    
    return tmp;
}
