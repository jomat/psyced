// This is just to emulate some historic Nemesis efuns
// used only when in MUD mode

// simplification - has to check ONCE_INTERACTIVE	TODO
int userp(object u) {
	if (!u) u = this_player();
	if (objectp(u)) return interactive(u);
	return 0;
}

