// $Id: xml.h,v 1.12 2006/09/23 17:32:36 fippo Exp $ // vim:syntax=lpc:ts=8
//
// this shows how one could have done xml using structs
// and furtherly, how to use structs or arrays depending
// on availability.. but in the end we decided to stick
// to good old tagged arrays ...  :)
//
#ifndef _XML_H
#define _XML_H
#if 0 //def  __LPC_STRUCTS__

struct XMLNode {
        struct XMLNode parent;
        string tag;
        string cdata;
        mapping param;
        mapping child;
};

# define XMLNode	struct XMLNode
# define new_XMLNode	(<XMLNode> 0, "", "", ([]), ([]))
# define cast_XMLNode	(struct XMLNode)

# define parent(n)	n->parent
# define tag(n)		n->tag
# define cdata(n)	n->cdata
# define param(n)	n->param
# define child(n)	n->child

#else
# define	Parent	0
# define	NodeLen 1
# define	Tag	2
# define	Cdata	3

# define XMLNode	mixed
# define new_XMLNode	([ ])
# define cast_XMLNode

# define nodelistp(node) pointerp(node)

#endif
#endif // _XML_H
