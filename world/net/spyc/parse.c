// vim:foldmethod=marker:syntax=lpc:noexpandtab
// $Id: parse.c,v 1.30 2008/12/18 18:16:14 lynx Exp $
//
#include "psyc.h"
#include <net.h>
#include <input_to.h>

private string buffer;
private string body_buffer;
int state;
int body_len;
int may_parse_more;

// tempoary used to hold assigment lists vname -> ({ glyph, state, vvalue })
array(mixed) tvars;
mapping hvars;

// this is completely anti-psyc. it should take mcs as arguments
// and look up the actual message from textdb.. FIXME
#define PARSEERROR(reason) { debug_message("PSYC PARSE ERROR: " reason "\n");  \
                             croak("_error_syntax_broken", "Failed parsing: "  \
                                    reason);                                   \
                             return 0;                                           \
                           }
                            
                        
#define DELIM S_GLYPH_PACKET_DELIMITER "\n"
#define C_LINEFEED '\n'
#define MVAR_GLYPH 0
#define MVAR_STATE 1
#define MVAR_VALUE 2


step(); // prototype

// reset parser state
void parser_reset() {
    if (state != PSYCPARSE_STATE_BLOCKED)
	state = PSYCPARSE_STATE_HEADER;
    body_len = 0;
    body_buffer = 0;
    may_parse_more = 1;
    tvars = ({ });
    hvars = ([ ]);
}

// initialize the parser
void parser_init() {
    buffer = "";
    parser_reset();
    state = PSYCPARSE_STATE_GREET; // AFTER reset
}

// it is sometimes useful to stop parsing
void interrupt_parse() {
    state = PSYCPARSE_STATE_BLOCKED;
}

// and resume after some blocking operation is done
void resume_parse() {
    state = PSYCPARSE_STATE_HEADER;
}

// input data to the buffer
void feed(string data) {
# ifdef _flag_log_sockets_SPYC
    log_file("RAW_SPYC", "» %O\n%s\n", ME, data);
# endif
    buffer += data;

    do {
	may_parse_more = 0;
	step();
    } while (may_parse_more);
}


// overload this as needed
varargs mixed croak(string mc, string data, vamapping vars, vamixed source) {
    return 0;
}


// called when a complete packet has arrived
void dispatch(mixed header_vars, mixed varops, mixed method, mixed body) {
    parser_reset();
}

// processes routing header variable assignments
// basic version does no state
mapping process_header(mixed varops) {
    mapping vars = ([ ]);
    // apply mmp state
    foreach(mixed vop : varops) {
	string vname = vop[0];
        switch(vop[1]) {
	case C_GLYPH_MODIFIER_SET:
            vars[vname] = vop[2];
            break;
	case C_GLYPH_MODIFIER_AUGMENT:
	case C_GLYPH_MODIFIER_DIMINISH:
	case C_GLYPH_MODIFIER_QUERY:
	case C_GLYPH_MODIFIER_ASSIGN:
            PARSEERROR("header modifier with glyph other than ':', this is not implemented")
            break;
	default:
            PARSEERROR("header modifier with unknown glyph")
	    break;
        }
	// FIXME: not every legal varname is a mmp varname
	// 	look at shared_memory("routing")
	if (!legal_keyword(vname) || abbrev("_INTERNAL", vname)) {
	    PARSEERROR("illegal varname in header")
	}
    }
    return vars;
}

// parse the header part of the packet 
// i.e. that is all mmp modifiers
// switch to content buffering mode afterwards
void parse_header() {
    if (!strlen(buffer)) return;
    if (buffer[0] == C_GLYPH_PACKET_DELIMITER) {
	if (strlen(buffer) < 2) 
	    return;
	if (buffer[1] == C_LINEFEED) {
	    buffer = buffer[2..];
	    hvars = process_header(tvars);
	    tvars = ({ });
	    dispatch(hvars, tvars, 0, 0);
	} else {
            // this one is sth like |whatever
            // actually this should be noglyph i think
            PARSEERROR("strange thing")
	}
    } else if (buffer[0] == C_LINEFEED) {
	// state transition from parsing header to buffering body
	buffer = buffer[1..];
	hvars = process_header(tvars);
	tvars = ({ });
	// FIXME: validate source/context here
	state = PSYCPARSE_STATE_CONTENT;
	if (hvars["_length"]) 
	    body_len = to_int(hvars["_length"][MVAR_VALUE]);
	step();
    } else { // parse mmp-header
	int fit;
	int glyph;
	string vname, vvalue;
	switch(buffer[0]) {
	case C_GLYPH_MODIFIER_SET:
	case C_GLYPH_MODIFIER_ASSIGN:
	case C_GLYPH_MODIFIER_AUGMENT:
	case C_GLYPH_MODIFIER_DIMINISH:
	case C_GLYPH_MODIFIER_QUERY:
	    glyph = buffer[0];
	    buffer = buffer[1..];
	    break;
	default:
	    PARSEERROR("noglyph")
	}
	fit = sscanf(buffer, "%.1s%t", vname);
	if (fit != 1) {
	    PARSEERROR("vname")
	}
	buffer = buffer[strlen(vname)..];
	switch(buffer[0]) {
	case '\t':
	    fit = sscanf(buffer, "\t%s\n%.0s", vvalue, buffer);
	    if (fit != 2) {
		PARSEERROR("simple-arg")
	    }
	    break;
	case '\n': // deletion
	    // this is currently implemented as "vvalue is 0" internally
	    // and must handled in dispatch() when merging
	    buffer = buffer[1..];
	    break;
	default:
	    PARSEERROR("arg")
	}
	tvars += ({ ({ vname, glyph, vvalue }) });
	step();
    }
}

// parse all psyc-modifiers
// this differes from the mmp modifiers in the sense that
// a) packet is known to be complete here
// b) psyc modifiers may have binary args
void parse_psyc() {
    while(1) { // slurp in all psyc-modifiers
	int fit, len;
	int glyph;
	string vname, vvalue;
	switch(body_buffer[0]) {
	case C_GLYPH_MODIFIER_SET:
	case C_GLYPH_MODIFIER_ASSIGN:
	case C_GLYPH_MODIFIER_AUGMENT:
	case C_GLYPH_MODIFIER_DIMINISH:
	case C_GLYPH_MODIFIER_QUERY:
	    glyph = body_buffer[0];
	    body_buffer = body_buffer[1..];
	    break;
	default:
	    // this is the method
	    return;
	}
	fit = sscanf(body_buffer, "%.1s%t", vname);
	if (fit != 1) {
	    PARSEERROR("vname")
	}
	body_buffer = body_buffer[strlen(vname)..];
	switch(body_buffer[0]) {
	case ' ':
	    fit = sscanf(body_buffer, " %d\t%.0s", len, body_buffer);
	    if (fit != 2) {
		PARSEERROR("binary-arg length")
	    }
	    if (len < 0) {
		PARSEERROR("negative binary length")
	    }
	    if (strlen(body_buffer) < len) {
		PARSEERROR("not enough to read binary arg, may not happen")
	    }
	    vvalue = body_buffer[..len-1];
	    body_buffer = body_buffer[len..];
	    if (body_buffer[0] != C_LINEFEED) {
		PARSEERROR("binary terminal")
	    }
	    body_buffer = body_buffer[1..];
	    break;
	case '\t':
	    fit = sscanf(body_buffer, "\t%s\n%.0s", vvalue, body_buffer);
	    if (fit != 2) {
		PARSEERROR("simple-arg")
	    }
	    break;
	case '\n':
	    switch(glyph) {
	    case C_GLYPH_MODIFIER_ASSIGN:
		// delete this context's state
		PARSEERROR("tbd")
		// unfortunately the routing hasn't been processed at
		// this moment yet, so we don't know whose context this
		// is and if it is legitimate. we first have to fix the
		// processing of the routing layer before we can implement
		// anything of this
		continue;
	    case C_GLYPH_MODIFIER_SET:
		// remember to temporarily ignore state
		PARSEERROR("tbd")
		continue;
	    case C_GLYPH_MODIFIER_QUERY:
		// mark that the next reply packet should
		// contain a state sync
		PARSEERROR("tbd")
		continue;
	    default:
		PARSEERROR("undefined operation")
	    }
	    return;
	default:
	    PARSEERROR("arg")
	}
	tvars += ({ ({ vname, glyph, vvalue }) });
    }
}

// parse completed content
void parse_content() {
    int fit;
    string method, body;

    // at this point we should check for relaying, then potentially
    // route the data without parsing it.. TODO

    parse_psyc();
    // ASSERT strlen(buffer)
    if (body_buffer[0] == C_LINEFEED) {
	PARSEERROR("empty method")
    }

    // FIXME: i am not sure if this is correct...
    if (body_buffer == S_GLYPH_PACKET_DELIMITER) {
	P0(("encountered packet delimiter in packet body? how's that?\n"))
	dispatch(hvars, tvars, 0, 0);
	return;
    }
    fit = sscanf(body_buffer, "%.1s\n%.0s", method, body_buffer);
    if (fit != 2 || !legal_keyword(method)) {
	croak("_error_illegal_method",
	      "That's not a valid method name.");
	return; // NOTREACHED
    }

    // mhmm... why does body_buffer still contain the newline?
    // because the newline is by definition not part of the body!
    if (strlen(body_buffer)) body = body_buffer[..<2];
    dispatch(hvars, tvars, method, body);
}

// buffer content until complete
// then parse it
// note: you could overload this, if you just want to route
// 	the packet
void buffer_content() {
    int t;
    if (body_len) {
	if (strlen(buffer) >= body_len + 3) {
	    // make sure that the packet is properly terminated
	    if (buffer[body_len..body_len+2] != "\n" DELIM) {
		PARSEERROR("packet delimiter after binary body not found")
	    }
	    body_buffer = buffer[..body_len];
	    buffer = buffer[body_len+3..];
	    parse_content();
	} else {
	    P4(("buffer_content: waiting for more binary data\n"))
	}
    } else if ((t = strstr(buffer, "\n" DELIM)) != -1) { 
	body_buffer = buffer[..t];
	buffer = buffer[t+3..];
	parse_content();
	P4(("buffer_content: packet complete\n"))
    } else {
	P4(("buffer_content: waiting for more plain data. buffer %O vs %O\n", to_array(buffer), to_array("\n" DELIM)))
    }
}

// respond to the first empty packet
void first_response() { 
    P0(("parser::first_response called. overload this!")) 
}

// parser stepping function
void step() {
    P3(("%O step: state %O, buffer %O\n", ME, state, buffer))
    if (!strlen(buffer))
	return;
    switch(state) {
    case PSYCPARSE_STATE_HEADER:
	parse_header();
	break;
    case PSYCPARSE_STATE_CONTENT:
	buffer_content();
	break;
    case PSYCPARSE_STATE_BLOCKED:
	// someone requested to stop parsing - e.g. _request_features circuit 
	// message
	break;
    case PSYCPARSE_STATE_GREET: // wait for greeting
	if (strlen(buffer) < 2)
	    return;
	if (buffer[0..1] == DELIM) {
	    state = PSYCPARSE_STATE_HEADER;
	    buffer = buffer[2 ..];
	    first_response();
	    step();
	} else {
	    croak("_error_syntax_initialization",
		"The protocol begins with a pipe and a line feed.");
	}
	break;
    default: // uhm... if we ever get here this is the programmers fault
	break;
    }
}

// FIXME should be in a standalone module
//#define PARSEERROR(args)	debug_message(sprintf("LIST PARSE ERROR: " args));
#define LISTSEP '|'
#define LISTPARSE_FAIL -1
#define LISTMODE_PLAIN 0
#define LISTMODE_BINARY 1
// list parsing function - val is assumed to be stripped of the final LF
mixed list_parse(string val) {
    mixed *lv = ({ });
    if (val[0] == LISTSEP) 
	return explode(val[1..], "|");
    while(strlen(val)) {
	int fit, len;
	fit = sscanf(val, "%d %s", len, val);
	if (fit != 2) {
            // invalid binary fit
	    return LISTPARSE_FAIL;
	}
	if (len < 0 || len > strlen(val)) {
            // invalid binary length
	    return LISTPARSE_FAIL;
	}
	if (len != strlen(val) && val[len] != LISTSEP) {
            // listsep not found after binary
	    return LISTPARSE_FAIL;
	}
	lv += ({ val[..len-1] });
	val = val[len+1..];
    }
    return lv;
}

#ifdef SELFTESTS
test() {
    list_parse("|psyc://example.symlynX.com/~jim|psyc://example.org/~judy");
    list_parse("5\tabcde|4\tabcd");
}
#endif
