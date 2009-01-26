// vim:syntax=lpc
#ifndef HEADER_ONLY
# include <net.h>
#endif
#ifndef STATE_MAX2
# define STATE_MAX2 9
#endif
#ifndef STATE_MIN2
# define STATE_MIN2 2
#endif
#ifndef HEADER_ONLY
// generic state-functions. 

// just simple check for one-time reoccurance
//
// this is the least _every_ out-state should do!
// 	- set assigned vars to "" if not in vars
// 	- filter out vars that are assigned
// 	- handle state-modifiers
//
// one more thing: dont you ever explicitly use : in varible names, 
// its default! 

void _augment(mapping state, string key, mixed value) {

    unless (member(state, key)) {
        state[key] = ({ value });
        return;
    }
    if (pointerp(state[key])) {
        state[key] += ({ value });
    } else if (mappingp(state[key])) { // prob..
        // what value do vars have when they are additions to a
        // mapping. we need to think about that!
    } else state[key] = ({ state[key], value });
}

void _diminish(mapping state, string key, mixed value) {               
    int i;

    if (member(state, key)) {
        if (pointerp(state[key])
            && (i = member(state[key], value) != -1)) {
            state[key] = state[key][0 .. i-1]
                                + state[key][i+1 .. ];
            if (sizeof(state[key]) == 0) {
                m_delete(state, key);
            }
        } else if (mappingp(state[key])) { // not used yet
            m_delete(state[key], value);
            if (sizeof(state[key]) == 0) {
                m_delete(state, key);
            }
        } else if (state[key] == value) {
	    m_delete(state, key);
        }
    }
}

void send_filter1(mapping state, mapping vars) {
    string key;
    mixed val;

    P4(("send_filter(%O, %O)\n", state, vars))
    foreach (key, val : vars) {
	switch(key[0]) {
	case '_':
	    break;
	case '=':
	    if ("" == val) {
		m_delete(state, key[1..]);
		break;
	    }
	    if (member(state, key[1..]) && state[key[1..]] == val) {
		m_delete(vars, key);
		break;
	    }
	    state[key[1..]] = val;
	    break;
	case '+':
	    _augment(state, key, val);
	    break;
	case '-':
	    _diminish(state, key, val);
	    break;
	case ':':
	    raise_error("state.c: WARNING: leave out the : in variable names."
			" it is default!\n");
	default:
	}
    }

    foreach (key, val : state) {
	if (member(vars, key)) {
	    if (vars[key] == val && 
		(!member(vars, "="+key) || vars["="+key] == vars[key]))
		m_delete(vars, key);

	} else unless(member(vars, "="+key) 
		      || member(vars, "+"+key) || member(vars, "-"+key)) {
	    vars[key] = "";
	}
    }
}

// we should _not_ call this method before all objects in vars have
// be transformed into their string representation. seems to make some trouble.
// TODO
varargs void send_filter2(mapping state, mapping vars, mapping timemachine,
			  mapping filter) {
    string key;
    mixed val;

     foreach (key, val : copy(vars)) {
	switch(key[0]) {
	case '_':
	    if (filter && member(filter, key)) break; // continue;
	    if (timemachine[key, 1] == -1) break;
	    if (member(timemachine, key)) {
		if (timemachine[key] == val) {
		    if (timemachine[key, 1] == STATE_MAX2) { 
			break;
		    }
		    if (timemachine[key, 1] == STATE_MIN2) {
			m_delete(vars, key);
			vars["="+key] = val;
			state[key] = val;
		    }
		    timemachine[key, 1]++;
		} else if (timemachine[key, 1] - 1 == STATE_MIN2) {
		    vars["="+key] = "";
		    m_add(timemachine, key, val, 1);
		} else if (timemachine[key, 1] <= STATE_MIN2) {
		    m_add(timemachine, key, val, 1);
		} else timemachine[key, 1]--; 
	    } else m_add(timemachine, key, val, 1);
	    break;
	case '=':
	    if ("" == val) {
		m_delete(state, key[1..]);
		break;
	    }
	    if (member(state, key[1..]) && state[key[1..]] == val) {
		m_delete(vars, key);
		break;
	    }
	    m_add(timemachine, key, val, -1);
	    state[key[1..]] = val;
	    break;
	case '+':
	    _augment(state, key, val);
	    break;
	case '-':
	    _diminish(state, key, val);
	    break;
	case ':':
	    PT(("state.c: WARNING: leave out the : in variable names."
		" it is default!"))
	    break;
	default:
	    raise_error("Illegal variable modifierer in '" + key + 
			"' encountered in state.c.\n");
	}
    }

    foreach (key, val : state) {
	if (member(vars, key)) {
	    if (vars[key] == val && 
		(!member(vars, "="+key) || vars["="+key] == vars[key]))
		m_delete(vars, key);

	} else unless(member(vars, "="+key) 
		      || member(vars, "+"+key) || member(vars, "-"+key)) {
	    // 
	    unless (timemachine[key] == -1)
		vars[key] = "";
	}
    }
}   
#endif
