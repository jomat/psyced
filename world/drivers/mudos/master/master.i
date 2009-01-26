#include "/obj/mudos/master2.i"

// disabled in main.c
static void crash(string error, object command_giver, object current_object) {
#ifndef DEBUG
	log_file("CRASH", MUD_NAME + " crashed on: " + ctime(time()) +
		", error: " + error + "\n");
        if (command_giver)
	    log_file("CRASH",
		 "this_player: " + file_name(command_giver) + "\n");
        if (current_object)
	    log_file("CRASH",
		 "this_object: " + file_name(current_object) + "\n");
#endif
	// users() -> pr("_notice_announce_crash", "System is crashing.\n");
}

// void error_handler( mapping error, int caught ) {

