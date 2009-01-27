# contributed by fippo

Stdio.File socket;
int count;
array(int) start, stop;

void logon(int success) {
    socket->set_nonblocking(read_cb);
    socket->write(".\n");
}

void read_cb(int id, string data) {
    //werror("read_cb:\n%s\n---\n", data);
    count++;
    if (count == 2) {
	start = System.gettimeofday()[..1];
	socket->write(":_target\tpsyc://localhost/~fippo\n\n_request_version\n.\n");
    }
    if (count > 2) {
	stop = System.gettimeofday()[..1];
	werror("latency is %d\n", (stop[0] - start[0])*1000000 + (stop[1] - start[1]));
	if (count < 500) {
	    socket->write(":_target\tpsyc://localhost/\n\n_request_version\n.\n");
	    start = stop;
	}
    }

}

int main() {
    socket = Stdio.File();
    socket->async_connect("localhost", 4404, logon);

    return -1;
}
