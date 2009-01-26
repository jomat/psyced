// $Id: user.c,v 1.13 2008/01/05 13:38:10 lynx Exp $ // vim:syntax=lpc
//
#include <net.h>
#include <user.h>
#include <person.h>
#include <status.h>

// Danny of Gueldenland is trying to run psyced inside a living MUD.
// this is our attempt to come up with some glue.

// let's work with aggregated objects.. we keep the actual player object
// in here so we can send output to it. the MUD subsystem will clone a
// net/mud/user for each player and attach it.
volatile object player;

#ifndef GUELDENLAND

// how to use: create a user object with this library function (=simulefun):
// 	psycid = named_clone("/net/mud/user", playerName);
// then attach the player to it:
//	psycid->attach(player);
attach(myplayer) {
	if (player) tell_object(player, "You have been detached by "+
			object_name(previous_object()) +"\n");

	player = myplayer;
	vSet("scheme", "mud");
	vDel("layout");
	vDel("agent");

	tell_object(player, "Your PSYC identification is: "+
	    psyc_name(ME)+ "\n");
}

// how to send commands to the PSYC user from the MUD player:
// you can either send commands including a command character to
// the "input()" method, or send commands to the "cmd()" method.
// see usercmd.i for details.
#if 0
input(a, dest) {
	if (!a || a=="") {
		showStatus(VERBOSITY_STATUS_AUTOMATIC);
	} else {
		::input(a, dest);
	}
	return 1;
}
#endif

// raw output functions, used by higher level output functions
protected emit(message) {
#if __EFUN_DEFINED__(convert_charset)
	if (v("charset") && v("charset") != SYSTEM_CHARSET) {
	    iconv(message, SYSTEM_CHARSET, v("charset"));
	    P4(("output in %O = %O\n", v("charset"), message))
	}
#endif
	// as simple as that, we output the stuff to our player's socket
	tell_object(player || ME, message);
}


#else

// why not just clonep() ?
int imacloney() {
	return program_name(this_object()) != object_name(this_object())+".c";
}

object in_the_beginning_the_light_elves_created_themselves() {
	object real;

	if(member(call_other("/secure/simul_efun", "users"), this_interactive()) < 0) return 0;

	if(!imacloney()) {
		real = find_object("/net/mud/gluser#"+getuid(this_interactive()));
		if(real) ; // do something about re-attaching possibly
		else real = named_clone("/net/mud/gluser", getuid(this_interactive()));
		if(real) real->in_the_beginning_the_light_elves_created_themselves();
		return real;
	}

	if(player && (getuid(player) != getuid(this_interactive()))) {
		tell_object(player, "Dein I4-Link wollte sich klauen lassen...\n");
		destruct(this_object());
		return 0;
	}

	if(player) {
		player = this_interactive();
		return this_object();
	}

	player = this_interactive();

// will find_living() be able to find psyc:nick like this?
	set_living_name("\npsyc:"+getuid(player));

	tell_object(player, "Deine I4-Persoenlichkeit ist "+psyc_name(ME)+"\n");

	vSet("scheme", "tn");
//	vDel("layout");
	vDel("agent");
	
	"/net/gl-psyc/simul_psyc"->register_psyc_user();

	if(!mappingp(friends)) friends = ([ ]);

	logon("gl.mud.de");

	return this_object();
}

string psycName() {
	return "~"+capitalize(getuid(player));
}

int handle_mud_command(string str) {
	if(player && player != this_interactive()) return 0;

	str = player->_unparsed_args();

	if(!str || str == "") {
		showStatus(VERBOSITY_STATUS_AUTOMATIC);
		return 1;
	}

	if(str[0..0] == "!" && query_wiz_level(player)>=90) {
		parsecmd(str[1..]);
		return 1;
	}

	return 1;
}

int online() {
	return player && interactive(player) && !player->QueryProp("invis");
}

int remove() {
	if(imacloney()) {
		if(player && interactive(player)) {
		    tell_object(player,
		       "Hops, Deine PSYC-Identität hat sich aus dem Staub gemacht...\n");
		}
		quit();
		if(this_object()) destruct(this_object());
	}
	return 0;
}

object sName(string orc) {
	if(file_size(PERSON_DATA_FILE(orc)) < 0) {
		vSet("name", capitalize(lower_case(orc)));
		vSet("password", md5(to_string(random(10000))));
		save();
	}
	::sName(orc);
	return this_object();
}

int input(string dwarf, string dest) {
			    // huh, is this really a gueldenland library macro?
	if(this_interactive() != player) GOFUCKYOURSELF 1;
			    // well as long as you don't expect other people
			    // to code things like that..
	if(!dwarf || dwarf=="")
	    showStatus(VERBOSITY_STATUS_AUTOMATIC);
	else ::input(dwarf, dest);
	return 1;
}

protected void emit(string message) {
	if(!player) return;
	tell_object(player, break_string(message, 78));
}

#endif
