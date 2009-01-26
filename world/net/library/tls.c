#include <net.h> // vim syntax=lpc
mapping tls_certificate(object who, int longnames) {
    mixed *extra, extensions;
    mapping cert;
    int i, j;

    cert = ([ ]);
#if __EFUN_DEFINED__(tls_check_certificate)
# ifdef WANT_S2S_SASL
/*
 * some platforms (notably cygwin) still have a problem executing the following
 * #if ... even psyclpc bails out with an "illegal unary operator in #if"
 * which is nonsense. i simplify this by ifdeffing for psyclpc.
 */
//#  if (__VERSION_MAJOR__ == 3 && __VERSION_MICRO__ > 712) || __VERSION_MAJOR__ > 3
#  ifdef __psyclpc__
#   if __EFUN_DEFINED__(enable_binary) // happens to be committed at the same time
    extra = tls_check_certificate(who, 2);
#   else
    extra = tls_check_certificate(who, 1);
#   endif
#  else
    extra = tls_check_certificate(who);
#  endif
    unless (extra && sizeof(extra) > 2) return 0;
    cert[0] = extra[0];
    if (sizeof(extra) >= 4)
	cert[1] = extra[3];

    extensions = extra[2];
    extra = extra[1];

    for (i = 0; i < sizeof(extra); i += 3) {
	mixed t;

	t = cert[extra[i]];
	// THIS IS ALWAYS TRUE FOR DRIVER >= 3.3.712 
	// OTHERWISE YOU SHOULD NOT ENABLE S2S SASL. PERIOD.
	if (sizeof(extra) > i+2) {
	    unless (t) {
		cert[extra[i]] = extra[i+2];
	    } else if (stringp(t)) {
		cert[extra[i]] = ({ t, extra[i+2] });
	    } else if (pointerp(t)) {
		cert[extra[i]] += ({ extra[i+2] });
	    } else { 
		PT(("fippo says this should not happen but you know it always does! %O\n", ME))
	    }
	} else {
	    PT(("fippo says this should not happen(2) but you know it always does! %O\n", ME))
	}
    }
    if (longnames) {
	// set up short/long names
	for (i = 0; i < sizeof(extra); i +=3) { 
	    cert[extra[i+1]] = cert[extra[i]];
	}
    }
    for (i = 0; i < sizeof(extensions); i += 3) {
	string key, mkey;
	mixed *val;

	unless(extensions[i]) continue;
	key = extensions[i];
	val = extensions[i+2];
	for (j = 0; j < sizeof(val); j += 3) {
	    mixed t;

	    mkey = key + ":" + val[j];
	    t = cert[mkey];
	    unless (t) {
		cert[mkey] = val[j+2];
	    } else if (stringp(t)) {
		cert[mkey] = ({ t, val[j+2] });
	    } else if (pointerp(t)) {
		cert[mkey] += ({ val[j+2] });
	    } else {
		// should not happen
	    }
	}
    }
# else
#  echo No WANT_S2S_SASL ? You even have the right driver for it!
# endif
#else
# echo No tls_check_certificate available with this driver.
#endif
    P2(("cert is %O\n", cert))
    return cert;
}
