// $Id: base64.c,v 1.7 2005/11/13 19:52:44 lynx Exp $ // vim:syntax=lpc
//
// base64 encoder/decoder
// 2004 - Dan@Gueldenland - df@erinye.com
//
#include <net.h>

volatile string * char64 = efun::explode("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/", "");

volatile mapping deco64 = funcall( (:
		mapping x = ([ ]);
		int i;
		for(i=0; i<64; ++i) {
			x[char64[i][0]] = i+1;
		}
		x['='] = 1;
		return x;
	:) );

static int deco(int i) {
	i = deco64[i];
	if(!i) raise_error("Oh crap.\n");
	return i-1;
}

string encode_base64(int * b) {
	int bbb, i=0, s=sizeof(b);
	string res = "";

	while(i+3 <= s) {
		bbb =  (b[i++] & 0xff) << 16;
		bbb |= (b[i++] & 0xff) << 8;
		bbb |=  b[i++] & 0xff;
		res += char64[(bbb & 0xfc0000) >> 18] + char64[(bbb & 0x3f000) >> 12] + char64[(bbb & 0xfc0) >> 6] + char64[bbb & 0x3f];
	}
	if(s - i == 2) {
		bbb = ((b[i++] & 0xff) << 16) | ((b[i++] & 0xff) << 8);
		res += char64[(bbb & 0xfc0000) >> 18] + char64[(bbb & 0x3f000) >> 12] + char64[(bbb & 0xfc0) >> 6] + "=";
	} else if(s - i == 1) {
		bbb = ((b[i] & 0xff) << 16);
		res += char64[(bbb & 0xfc0000) >> 18] + char64[(bbb & 0x3f000) >> 12] + "==";		
	}

	return res;
}

// this doesn't really enforce base64, but if it finds = on positions they
// shouldn't be on, this'll get sullen.
int * decode_base64(string b) {
	int * res = ({ });
	int i=0, s, next;

	b = replace(b, "\n", "");
	b = replace(b, "\r", "");

	s = strlen(b);

	while(i+4 <= s) {
		res += ({ (deco(b[i]) << 2) | (deco(b[i+1]) >> 4) });
		next = ((deco(b[i+1]) << 4) & 0xff) | (deco(b[i+2]) >> 2);
		unless (!next && b[i+2] == '=') {
		    res += ({ next });
		} else {
		    return res;
		}
		next = ((deco(b[i+2]) << 6) & 0xc0) | deco(b[i+3]);
		unless (!next && b[i+3] == '=') {
		    res += ({ next });
		} else {
		    return res;
		}
		i += 4;
	}

	return res;
}

#if DEBUG > 1
void base64_self_test() {
	int i, j, cool;
	string x, y, z;

	for(i=0; i<10; ++i) {
		x = "";
		j = random(100) + 5;
		while(j>0) {
			x += to_string(({ 32 + random(96) }));
			--j;
		}
		y = encode_base64(to_array(x));
		z = to_string(decode_base64(y));
		if(z == x) ++cool;
		else {
		    P2(("uncool:\n%O !=\n%O\n", x, z));
		    P3(("base: %O\n", y))
		    P3(("strlen: %O, %O, last: %O\n", strlen(x), strlen(z), z[<1]))
		}
	}
	P2(("base64_self_test: %d out of 10 successful.\n", cool));
}
#endif

