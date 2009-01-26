// $Id: queue.c,v 1.40 2007/06/22 13:14:48 lynx Exp $ // vim:syntax=lpc

#include <net.h>

#define Q_ARRAY 0
#define Q_BEGIN 1
#define Q_END	2
#define Q_SIZE	3
#define Q_RSIZE	4
#define Q_MAX	5

#define STEP	10

#define push enqueue

static int enqueue(mixed name, mixed item);
static varargs int qInit(mixed name, int max, int pre);
static mixed qExists(varargs mixed * names);
static mixed qSize(varargs mixed * names);
static mixed shift(mixed name);
static varargs int unshift(mixed name);
static int enlarge(mixed name);
// tell me: is there a reason for qRename being public?
public int qRename(mixed old, mixed new);
static int qDel(mixed name);

volatile mapping q; // mapping containing queues

mapping qDebug() { return q; }

static varargs int qInit(mixed name, int max, int pre) {
	unless(mappingp(q)) q = ([ ]);
	if(q[name]) return 0;
	q += ([ name : allocate(pre); 0; -1; 0; pre; max ]);
	// name : array, bottom, top, q_size, r_size, max
	return 1;
}

static int enqueue(mixed name, mixed item) {
	int number;
	
	P3(("enqueue %O: %O\n", name, item))
	if(q[name, Q_SIZE] == q[name, Q_MAX]) {
		P1(("queue %O has reached maximum size when trying to add %O\n",
		    name, item))
		return 0; // maximum reached
	}
	// array needs to grow
	if(q[name, Q_SIZE] == q[name, Q_RSIZE]) {
		if(!enlarge(name)) {
			D2(D("queue: enlarge failed!\n");)
			return 0;
		}
	}
	if(q[name, Q_END] + 1 == q[name, Q_RSIZE]) {
		q[name, Q_END] = 0;
	} else {
		q[name, Q_END]++;
	}
	q[name, Q_ARRAY][q[name, Q_END]] = item;
	q[name, Q_SIZE]++;

//	D2(D("enqueue into "+to_string(name)+" Q_BEGIN:"+q[name, Q_BEGIN]+" into:"+q[name, Q_END]+"\n");)
	return 1;
}

static mixed qExists(varargs mixed * names) {
	mixed name;
	if (!mappingp(q)) return 0;
	foreach (name : names) {
		if(member(q, name)) return name;
	}
	return 0;
}

// should probably be renamed into qNotEmpty() or something
static mixed qSize(varargs mixed * names) {
	mixed name;
	if (!mappingp(q)) return 0;
	if (sizeof(names) == 1) {
	    return q[names[0], Q_SIZE];
	}
	foreach (name : names) {
		if(member(q, name) && q[name, Q_SIZE])	return name;
	}
	return 0;
}

public int qRename(mixed old, mixed new) {
	if(!q[old] || q[new]) return 0;
	//q[new] = q[old];
	// that doesn't work.. ldmud bug? well, some keys (like size) weren't
	// copied. therefore:
	q += ([ new : q[old, Q_ARRAY]; q[old, Q_BEGIN]; q[old, Q_END];
		      q[old, Q_SIZE]; q[old, Q_RSIZE]; q[old, Q_MAX] ]);
	m_delete(q,old);
	return 1;	
}

static int qDel(mixed name) {
	ASSERT("qDel (mappingp(q))", mappingp(q), q)
	//if (mappingp(q))
	m_delete(q,name);
	return 1;
}

static mixed shift(mixed name) {
	// NEVER use shift whithout 
	// qSize before!!	
	int number;
	q[name, Q_SIZE]--;
	
	number = q[name, Q_BEGIN];
	if((q[name, Q_BEGIN] + 1) == q[name, Q_RSIZE]) {
		q[name, Q_BEGIN] = 0;
	} else {
		q[name, Q_BEGIN]++;
	}
//	D2(D("shift from "+to_string(name)+" Q_BEGIN: " + q[name, Q_BEGIN] + " from:"+number+"\n");)
	return q[name, Q_ARRAY][number];
}

static varargs int unshift(mixed name, mixed item) {
//	D2(D("unshift Q_BEGIN:" + q[name, Q_BEGIN]+"\n");)
	if(q[name, Q_RSIZE] == q[name, Q_SIZE]) {
		if(!enlarge( name )) {
			return 0;
		}
	}
	if(q[name, Q_BEGIN] == 0) {
		q[name, Q_BEGIN] = q[name, Q_RSIZE] - 1;
	} else {
		q[name, Q_BEGIN]--;
	}
	if(item)
		q[name, Q_ARRAY][q[name, Q_BEGIN]] = item;
	q[name, Q_SIZE]++;
	return 1;
}

static int enlarge(mixed name) {
	int step;

	// das will ich mir doch mal näher ansehen..
	P1(("%O queue enlarge for %O\n", ME, name))
	if((q[name, Q_RSIZE] + STEP) > q[name, Q_MAX]) {
		step = q[name, Q_MAX] - q[name, Q_RSIZE];
	} else {
		step = STEP;
	}
	if(step == 0)
				return 0;
	if(q[name, Q_END] < q[name, Q_BEGIN]) {
		q[name, Q_ARRAY] = q[name, Q_ARRAY][q[name, Q_BEGIN]..] + q[name, Q_ARRAY][0..q[name, Q_END]] + allocate(step);
		q[name, Q_BEGIN] = 0;
		q[name, Q_END] = q[name, Q_SIZE] - 1;
		q[name, Q_RSIZE] += step;
		return 1;
	} else {
		q[name, Q_ARRAY] = q[name, Q_ARRAY][q[name, Q_BEGIN]..q[name, Q_END]] + allocate(step);
		
		q[name, Q_BEGIN] = 0;
        q[name, Q_END] = q[name, Q_SIZE] - 1;
        q[name, Q_RSIZE] += step;
		return 1;
	}	
}

static void qCreate() {
	unless (mappingp(q) && widthof(q) == 1) q = ([]);
}

// for compatibility to queue2, won't give performance boosts for
// array-append-like operations as queue2 (most probably) does
static mixed *qToArray(mixed name) {
    mixed *tmp, *qarr;
    int max, cur, mod;

    ASSERT("qToArray (mappingp(q))", mappingp(q), q)
    unless (member(q, name)) return 0;

    tmp = allocate(max = q[name, Q_SIZE]);
    qarr = q[name, Q_ARRAY];
    cur = q[name, Q_BEGIN];
    mod = q[name, Q_RSIZE];

    for (int i = 0; i < max; i++) {
	tmp[i] = qarr[cur++];
	cur %= mod;
    }

    return tmp;
}
