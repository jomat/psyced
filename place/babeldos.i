//                                              vim:noexpandtab:syntax=lpc
// $Id: babeldos.i,v 1.3 2007/10/08 11:00:30 lynx Exp $

/*
 * this is one of the most impressive abuses of psyc rooms i've seen
 * so far.. have fun with it.. then maybe patch it in a fashion that
 * it becomes potentially actually useful. yet another drunken master-
 * piece coded by heldensaga and elridion.
 */

#include <input_to.h>
#include <ht/http.h>
#define INHERIT_CONNECT

start();
babel();

#define CRESET	start();
inherit "net/connect";
#include <place.gen>

#define emit	binary_message

volatile int has_con, is_translating, is_con;
volatile mixed *current;
volatile string krank, buffer;

start() {
    qInit(ME, 1000, 100);
    unless (has_con || is_con) {
	is_con = 1;
	call_out(#'connect, 0, "babelfish.altavista.com", 80);
    }
}

escape(data) {
    return regreplace(data, "[^a-zA-Z0-9]", (: return sprintf("%%%x", $1[0]); :), 1);
}

disconnect() {
    has_con = 0;
    switch (pointerp(current) ? current[0] : "") {
	string k;
case TRANSLATION:
	    if (sscanf(buffer, "%~s<td bgcolor=white class=s><div style=padding:10px;>%s</div></td>%~s", k)) {
		krank = k;
	    }
#ifdef TRANSLATION2
	    babel(TRANSLATION2, current[1], current[2], krank, current[4]);
	    break;
case TRANSLATION2:
	    if (sscanf(buffer, "%~s<td bgcolor=white class=s><div style=padding:10px;>%s</div></td>%~s", k)) {
		krank = k;
	    }
#endif
	    ::msg(current[1], current[2], replace(krank, "&quot;", "\""), current[4]);
    default:
	    current = 0;
	    start();
    }
}

parse(data) {
    buffer += data + " ";
    input_to(#'parse, INPUT_IGNORE_BANG);
}

logon(int f) {
    is_con = 0;
    unless (::logon(f)) {
	return;
    }
    has_con = 1;
    input_to(#'parse, INPUT_IGNORE_BANG);

    if (qSize(ME)) {
	funcall(lambda(({}), ({ #'babel }) + shift(ME) ));
    } else {
	is_translating = 0;
    }
}

translat(source, mc, data, mapping vars, count) {
    if (is_translating) {
	//debug_message("enqueing: " + data + "\n");
	enqueue(ME, ({ TRANSLATION, source, mc, data, vars }));
    } else {
	//debug_message("babeling: " + data + "\n");
	is_translating = 1;
	babel(TRANSLATION, source, mc, data, vars);
    }

    return 1;
}

babel(mode, source, mc, data, mapping vars) {
    unless (has_con) {
	unshift(ME, ({ mode, source, mc, data, vars }));
	start();
	return;
    }

    buffer = "";
    krank = 0;
    current = ({ mode, source, mc, data, vars });
    emit("GET /tr?lp=" + mode + "&urltext=" + escape(data) + " HTTP/1.1\r\n"
	 "Host: babelfish.altavista.com\r\n\r\n");
}

msg(source, mc, data, mapping vars) {
    if (abbrev("_message_public", mc) && data) {
	return translat(source, mc, data, vars);
    }
    return ::msg(source, mc, data, vars);
}
