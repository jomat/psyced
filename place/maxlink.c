// dieser raum implementiert eine kommunikation mit dem netzwerkport der
// MAX software.. ein kultiges ding mit dem man graphisch objektorientiert
// midi, audio, video zusammenstöpseln und generieren kann. also nicht zu
// verwechseln mit der raytracing software "3d studio max." kiritan, der VJ
// der euRoClAsh.com parties, gedenkt chatmaterial als input für seine
// visuals zu verwenden. dies ist der nötige code dazu.
//
// heldensaga hat sich entschlossen diesen code ganz ausführlich zu
// kommentieren, weswegen er generell als leitfaden gelten kann wie man
// gateway-räume zu verrückten zwecken erzeugen kann (siehe aber auch
// gatebot.c für befehlsgewalt in gateways sowie die net/protokoll
// verzeichnisse für richtig integrierte implementationen von protokollen).
//
// net.h beliefert uns mit allem möglichen zeugs, unter anderem mit unless
// und NET_PATH. es ist die grundvoraussetzung, damit LPC was von PSYC weiß.
#include <net.h>

// wir müssen start() als prototyp definieren, weil wir es (im place.gen)
// benutzen, bevor wir es definiert haben
start();

// hier bitten wir darum, dass start() aufgerufen wird, wenn der raum geladen
// wird
#define CREATE	start(1);
// NET_PATH "connect" beliefert uns mit der fähigkeit, (tcp)verbindungen zu
// öffnen
inherit NET_PATH "connect";
// place.gen beliefert uns mit den unteren ebenen des raums, die unter anderem
// die nachricht an alle anwesenden user verteilen und joins/leaves handhaben
#include <place.gen>

// wir brauchen noch INPUT_IGNORE_BANG, das is so ne LPC-eigenheit
#include <input_to.h>

// hier sagen wir, wohin wir verbinden wollen
#define TO_HOST	"example.org"
#define TO_PORT 4444

// emit als alias für binary_message, binary_message sendet an unsere
// verbindung (zu max/scp)
#define emit	binary_message

// has_con ist 1 wenn wir eine verbindung haben, is_con ist 1, wenn wir uns
// gerade verbinden
volatile int has_con, is_con;
// im prinzip sind beide variablen nicht notwendig, weil sie folgenden
// bytecodes entsprechen:
//	has_con == interactive()
//	is_con == find_call_out(#'connect)
// und in LPC sind bytecodes ja schneller & effizienter als variablen  ;)
// blödsinn. is_con ist solange true bis die verbindung established _ist_.
// find_call_out(#'connect) != -1 ist solange true bis wir versuchen zu
// verbinden. um die zeit in der schwebe abzudecken, brauchen wir also is_con.

// diese funktion wird aufgerufen, wenn der raum geladen wird
// sowie jedes mal wenn jemand etwas tippt und wir nicht verbunden sind
start(when) {
    // wenn wir uns grade im verbindungsaufbau befinden oder schon eine
    // verbindung haben, wäre ein neuer verbindungsaufbau reichlich sinnlos
    unless (has_con || is_con) {
	is_con = 1;
	// im prinzip das gleiche wie connect(TO_HOST, TO_PORT). allerdings
	// wird der aktuelle event-loop (die befehlskette) unterbrochen.
	// eigentlich brauchen wir die unterbrechung hier nicht, aber schaden
	// tuts auch nicht. connect() wird also erst ausgeführt wenn die
	// dinge, die uns hierhergeführt haben (vermutlich ein user der diesen
	// raum als erster betreten hat), abgearbeitet sind.
	call_out(#'connect, when, TO_HOST, TO_PORT);
    }
}

// wenn die gegenseite uns was schickt, kommt es hier an..
// wir schicken das einfach als debugmeldung auf die console
// und sichern ab, dass die nächste zeile auch wieder hier ankommen wird
// man könnte hier natürlich auch psyc-nachrichten erzeugen..
input(t) {
    P1(("%O got %O\n", ME, t))
    input_to(#'input, INPUT_IGNORE_BANG);
}

// diese funktion wird aufgerufen, wenn die verbindung zustande gekommen ist
// oder nicht aufgebaut werden konnte
logon (f) {
    is_con = 0;
    // wenn ::logon(f) (logon() aus einer unteren ebene, NET_PATH "connect")
    // nicht wahr ist, konnte die verbindung nicht hergestellt werden
    unless (::logon(f)) {
	return;
    }
    // wir haben eine verbindung
    has_con = 1;
    // alle eingaben aus dieser verbindung nach input() schicken
    input_to(#'input, INPUT_IGNORE_BANG);
}

// msg() wird aufgerufen, wenn jemand eine nachricht an den raum sendet
msg(source, mc, data, mapping vars) {
    if (abbrev("_message", mc)) {
	// wenn wir eine verbindung haben
	if (has_con) {
	    // und text (data) mitgesendet wurde (und data ein string ist)
	    if (stringp(data)) {
		string sendout;

		// werfen wir sonderzeichen heraus oder wandeln sie um
		sendout = replace(data, ";", ":");
		sendout = replace(sendout, ",", "");
		sendout = replace(sendout, "\\", "");
		sendout = replace(sendout, "ä", "ae");	// is das noch
		sendout = replace(sendout, "ö", "oe");	// aktuell mit utf8 !?
		sendout = replace(sendout, "ü", "ue");
		sendout = replace(sendout, "ß", "ss");
		sendout = replace(sendout, "Ä", "Ae");
		sendout = replace(sendout, "Ö", "Oe");
		sendout = replace(sendout, "Ü", "Ue");

		// und senden die nachricht an max/scp
		emit("<" + vars["_nick"] + "> " + sendout + ";\n");
	    }
	    // wenn nicht (keine verbindung), versuchen wir, eine aufzubauen
	} else {
	    // versuche in 48 sekunden erst wieder, sonst könnte das weh tun
	    start(48);
	}
    }
    // wir leiten die nachricht an die unteren ebenen des raumes weiter, die
    // sie an alle anwesenden user verteilen etc etc
    return ::msg(source, mc, data, vars);
}

// disconnect() wird aufgerufen, wenn die verbindung abbricht
disconnect() {
    has_con = 0;
    // wir versuchen die verbindung wieder aufzubauen
    //start(4);
    // nö wir warten ob jemand was tippt, also die verbindung haben will
}
