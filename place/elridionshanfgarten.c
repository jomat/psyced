#define NAME "ElridionsHanfGarten"
#define PLACE_HISTORY
#define HISTORY_GLIMPSE 8

#include <place.gen>

msg(source, mc, data, mapping vars) {
	string a;
	a = ::msg(source, mc, data, vars);
	if (stringp(data)) {
		if (strstr(lower_case(data), "marihuana") != -1) {
			castmsg(ME, "_notice_kidding_CSU", "Halt - CSU! Hat hier jemand Marihuana gesagt?", ([ ]));
		}
	}
	return a;
}
