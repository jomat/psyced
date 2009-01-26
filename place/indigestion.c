#include <place.gen>

msg(source, mc, data, mapping vars) {
	int rc = ::msg(source, mc, data, vars);
	if (abbrev("_message", mc) && stringp(data)) {
#if __EFUN_DEFINED__(crypt)
	    castmsg(ME, "_notice_digest_crypt",
		"\tcrypt: [_text_crypt]", ([ "_text_crypt" : md5(data) ]) );
#endif
#if __EFUN_DEFINED__(md5)
	    castmsg(ME, "_notice_digest_md5",
		"\t  MD5: [_text_md5]", ([ "_text_md5" : md5(data) ]) );
#endif
#if __EFUN_DEFINED__(sha1)
	    castmsg(ME, "_notice_digest_sha1",
		"\t SHA1: [_text_sha1]", ([ "_text_sha1" : sha1(data) ]) );
#endif
	}
	return rc;
}
