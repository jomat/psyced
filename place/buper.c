#define NAME "Buper"

#include <place.gen>

/* fun room */

msg(source, mc, data, mapping vars) {
	string a;
	if (stringp(data)) {
	    // does this need special treatment now that system
	    // charset is utf8 ? iconv before and after ? lol.
	    data = regreplace(data, "(^| )([aeiouüöäAEIOUÜÖÄ])", "\\1b\\2", 1);
	    data = regreplace(data, "\\<[a-zA-Z]", "b", 1);
	}
	return ::msg(source, mc, data, vars);
}
