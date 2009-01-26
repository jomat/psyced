// $Id: echo.c,v 1.2 2005/03/14 10:23:26 lynx Exp $ // vim:syntax=lpc
//
// just a test daemon.. load it if you want urls of $echo type to work
// you can (currently) also use /msg echo:whatever locally
// put it into init.ls

create() {
	register_service("echo");
}

msg(source, mc, data, mapping vars) {
	P1(("%O echoing for %O", ME, source))
	sendmsg(source, "_notice_echo"+mc,
	    "Thanks for the "+data, vars);
}

