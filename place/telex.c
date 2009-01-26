#define NAME "TELEX"
#define PLACE_HISTORY
#define ON_ANY telexify(&data, &vars);

/*
#ifdef BRAIN
# define MASTER
#else
# define CONNECT_DEFAULT
#endif
*/

#include <place.gen>

/*** 6-BIT ROOM FOR 60S/70S COMPUTER *** ONLY SUPPORTS UPPER CASE TEXT ***/

mapping cache = ([ "lynX" : "lynX" ]);

telexify(data, vars) {
	string k, val;

	foreach (k, val: vars) if (stringp(val)) {
		if (cache[val]) vars[k] = cache[val];
		else vars[k] = cache[val] = upper_case(val);
	}
        if (stringp(data)) {
		data = upper_case(data);
        }
}
