#ifndef URL
#define	URL(urlstr)	parse_uniform(urlstr)

// essential parts, also used by render_uniform
#define	UScheme		0
#define	UUser		1
#define	UPass		2
#define	UHost		3
#define	UPort		4
#define	UTransport	5
#define	UResource	6
#define	UQuery		7
#define	UChannel	8

// convenient snippets of the URL
#define	UString		9	// the URL as such
#define	UBody		10	// the URL without scheme and '//'
#define	UUserAtHost	11	// mailto and xmpp style
#define	UHostPort	12	// just host:port (and transport)
#define	URoot		13	// root UNI of peer/server
#define	USlashes	14	// the // if the protocol has them
#define	UNick		15	// whatever works as a nickname
//efine	UCircuit	16	// scheme:host:port
				// (not provided by parse_uniform)
#define	USize		16

#endif
