// spawn demonstration place as described in
// http://about.psyc.eu/Spawn

#include <net.h>

inherit NET_PATH "spawn";

launch();

#define NAME "SpawnDemo"
#define CRESET launch();
#include <place.gen>

incoming(response) {
	castmsg(ME, "_notice_application_listener", response, ([]));
}

launch() {
#if 0	// make this '1' if you have a listener.sh installed in run
	spawn("listener.sh", 0, #'incoming);
#endif
}

