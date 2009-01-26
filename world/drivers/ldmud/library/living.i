// $Id: living.i,v 1.16 2007/09/18 09:49:17 lynx Exp $ // vim:syntax=lpc:ts=8
//
// i shouldnt be using this old code for finding people..
// then again does it really harm?
// at least we throw out the living() lpmud things..
// why does it do the recursive call_out? just because of eval_cost?
// then we dont need that either..
//
// no, we don't need the whole cleaning thing, and we don't need
// the ability to handle several objects by the same living_name
// either. time to get rid of living.i for psyced.  -lynX 2005
//
// instead, we could use availability here.

#define living(O) objectp(O)

#ifdef USE_LIVING
#define	TIME_CLEAN_LIVING	60 * 60 * 4
volatile mapping living_name_m;
#endif
volatile mapping name_living_m;

void start_simul_efun() {
    PROTECT("START_SIMUL_EFUN")

    name_living_m = ([]);
#ifdef USE_LIVING
    living_name_m = ([]);
    if (find_call_out("clean_simul_efun") < 0)
	call_out("clean_simul_efun", TIME_CLEAN_LIVING / 2);
    this_object()->start_simul_efun_dr();
#endif
}

#ifdef USE_LIVING
static void clean_name_living_m(string *keys, int left) {
    int i, j;
    mixed a;

    PROTECT("CLEAN_NAME_LIVING")
    if (left) {
	if (pointerp(a = name_living_m[keys[--left]]) && member(a, 0)>= 0) {
	    i = sizeof(a);
	    do {
		if (a[--i])
		    a[<++j] = a[i];
	    } while (i);
	    name_living_m[keys[left]] = a = j > 1 ? a[<j..] : a[<1];
	}
	if (!a)
	    efun::m_delete(name_living_m, keys[left]);
	call_out("clean_name_living_m", 1, keys, left);
    } else {
	// apparently no problem.. this stuff seems to be working
	P2(("clean_name_living_m ended with %O people\n",
	    sizeof(name_living_m)))
    }
}

static void clean_simul_efun() {
    PROTECT("CLEAN_SIMUL_EFUN")
    /* There might be destructed objects as keys. */
    //
    // actually there never should.. so it should be okay to turn off
    // the whole clean_simul_efun() thing  -lynX 2005

    m_indices(living_name_m);
    remove_call_out("clean_simul_efun");
    PT(("clean_name_living_m started for %O people\n", sizeof(name_living_m)))
    if (find_call_out("clean_name_living_m") < 0) {
	call_out(
	  "clean_name_living_m",
	  1,
	  m_indices(name_living_m),
	  sizeof(name_living_m)
	);
    }
    call_out("clean_simul_efun", TIME_CLEAN_LIVING);
}
#endif

/* disable symbol_function('set_living_name, SIMUL_EFUN_OBJECT) */
void register_person(string name, object o) {
    string old;
    mixed a;
    int i;

    PROTECT("SET_LIVING_NAME") // do we need that for protected sefuns?
#ifdef USE_LIVING
    if (old = living_name_m[o]) {
	if (pointerp(a = name_living_m[old])) {
	    a[member(a, o)] = 0;
	} else {
	    efun::m_delete(name_living_m, old);
	}
    }
    living_name_m[o] = name;
    if (a = name_living_m[name]) {
	if (!pointerp(a)) {
	    name_living_m[name] = ({a, o});
	    return;
	}
	/* Try to reallocate entry from destructed object */
	if ((i = member(a, 0)) >= 0) {
	    a[i] = o;
	    return;
	}
	name_living_m[name] = a + ({o});
	return;
    }
#endif
    name_living_m[name] = o;
}

// was: find_living()
// .... the lowercazed optimization actually doesn't
// help, there is hardly a use for it.. so it may disappear again.
varargs object find_person(string name, int lowercazed) {
    mixed *a, r;
    int i;

    if (!lowercazed) name = lower_case(name);
    r = name_living_m[name];
#ifdef USE_LIVING
    if (pointerp(r)) {
	if ( !living(r = (a = r)[0])) {
	    for (i = sizeof(a); --i;) {
		if (living(a[<i])) {
		    r = a[<i];
		    a[<i] = a[0];
		    return a[0] = r;
		}
	    }
	}
	return r;
    }
    return living(r) && r;
#else
    return r;
#endif
}

int amount_people() { return sizeof(name_living_m); }
object* objects_people() {
    PROTECT("OBJECTS_PEOPLE")
#ifdef USE_LIVING
    return m_indices(living_name_m);
#else
    return m_values(name_living_m);
#endif
}

