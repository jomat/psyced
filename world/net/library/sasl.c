// $Id: sasl.c,v 1.17 2006/10/18 17:52:15 fippo Exp $ // vim:syntax=lpc

#include <net.h>

mapping sasl_parse(string t) {
    string vname, vvalue;
    mapping data = ([ ]);

    while (sscanf(t, "%s=%s,%.0t%s", vname, vvalue, t) >= 2) {
	sscanf(vvalue, "\"%s\"", vvalue);
	data[vname] = vvalue;
    }
    sscanf(t, "%s=%s", vname, vvalue);
    sscanf(vvalue, "\"%s\"", vvalue);
    data[vname] = vvalue;

    return data;
}

varargs string sasl_calculate_digestMD5(mapping data, string secret, int calcresponse, string prehash) {
    /* 
     * calculate the digest-md5 response or rspauth value for data + secret
     * set calcresponse to 1 if you want to calculate the rspauth,
     * 	0 if you want to calculate the expected response
     * this function assumes that you have checked the necessary data
     * and will only complain about missing data with DEBUG > 3
     */
    string HA1, HA2;
    string t, t2;
  
#if DEBUG > 2
    // data checks
    unless(mappingp(data) && data["username"] && data["realm"] && data["nonce"]
	    && data["cnonce"] && data["digest-uri"] && data["nc"] 
	    && data["qop"] && data["qop"] == "auth" // unimplemented otherwise
	  ) {
	    raise_error("sasl_calculate_digestMD5 called with invalid data\n"
			+ sprintf("data is %O\n\n", data));
    }
#endif
    if (prehash) {
	/* according to RFC 2831, 3.9  Storing passwords this may also
	 * come directly from a database / storage
	 */
	t = prehash;
    } else {
	t = md5(data["username"] + ":" + data["realm"] + ":" + secret);
    }
    t2 = "";

    // we need octet form
    // only works with ldmud after 3.3.611
    for (int i = 0; i < 32; i += 2)
	t2 += sprintf("%c", to_int("0x" + t[i..i+1]));

    HA1 = md5(t2 + ":" + data["nonce"] + ":" + data["cnonce"]);
    if (calcresponse)
	t = ":";
    else
	t = "AUTHENTICATE:";
    t += data["digest-uri"];
    // TODO: qop == "auth-int"
    HA2 = md5(t);
    P2(("sasl: t is %O, t2 is %O\n", t, t2))
    P2(("sasl: HA1 is %O, HA2 is %O\n", HA1, HA2))

    // watch out, int(nc) should always be one as we dont have
    // subsequent auth (see rfc 2831)
    t = sprintf("%s:%s:%s:%s:%s:%s", 
		HA1, data["nonce"], data["nc"],
		data["cnonce"], data["qop"], HA2);
    P2(("sasl: response will be md5(%O)\n", t))
    return md5(t);
}

/* this is testing the behaviour with the example data from RFC 2831
 * run this whenever you change the code above
 */
#ifdef TESTSUITE
int sasl_test(void) {
    string t = "realm=\"elwood.innosoft.com\",nonce=\"OA6MG9tEQGm2hh\","
	    "qop=\"auth\",algorithm=md5-sess,charset=utf-8";
    string rsp;
    mapping data;

    // check parsing
    data = sasl_parse(t);
    unless (data["realm"] == "elwood.innosoft.com"
		&& data["algorithm"] == "md5-sess"
		&& data["charset"] == "utf-8"
		&& data["nonce"] == "OA6MG9tEQGm2hh"
		&& data["qop"] == "auth"
		&& sizeof(data) == 5) {
	P0(("SASL TEST: parsing failed!! got %O\n", data))
	return -1;
    }
  
    // now add the necessary values from rfc 2831
    data["username"] = "chris";
    data["cnonce"] = "OA6MHXh6VqTrRk";
    data["nc"] = "00000001";
    data["digest-uri"] = "imap/elwood.innosoft.com";

    // and calculate the response
    rsp = sasl_calculate_digestMD5(data, "secret", 0);
    unless (rsp == "d388dad90d4bbd760a152321f2143af7") {
	P0(("SASL TEST: calc0 failed!! got response %O\n", rsp))
	return -2;
    }

    rsp = sasl_calculate_digestMD5(data, "secret", 1);
    unless (rsp == "ea40f60335c427b5527b84dbabcdfffd") {
	P0(("SASL TEST: calc1 failed!! got rspauth %O\n", rsp))
	return -3;
    }
    P2(("SASL TEST successful.\n"))
    return 1;
}
#endif
