// $Id: outputb.c,v 1.2 2007/08/27 16:54:11 lynx Exp $
//
// buffered output object
// inspired by fippo's async_http_base.c
//
// activate by passing a buffer init string, usually init_buffer("").
// write to buffer using any of emit or write
// flush to object or myself when done, using flush_buffer().
//
// if you want to use w() and textdb, consider that you have to
// #include <text.h> AFTER inheriting this, so that w() feeds this emit().

#include <net.h>

virtual inherit NET_PATH "output";

protected string _buffer;

#define BUFF(DATA) \
	P3(("outputb in %O adding %O\n", ME, DATA)) \
	_buffer += DATA; \
	return 1;

void init_buffer(string what) {
	// initiate buffering mode
	_buffer = what;
}

int emit(string output) {
	if (_buffer) {
		BUFF(output)
	}
	else return ::emit(output);
}

// not sure if i like this one.. it may be incorrect to map to emit
int write(mixed output) {
	if (_buffer) {
		BUFF(output)
	}
	else {
		efun::write(output);
		return 1;
	}
}

flush_buffer(object out) {
	P2(("flush_buffer for %O and data %O\n", out, _buffer))
	if (out) out->emit(_buffer);
	else ::emit(_buffer);
	// terminate buffering mode
	_buffer = 0;
}
