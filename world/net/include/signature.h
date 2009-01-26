// $Id: signature.h,v 1.1 2007/08/09 22:10:29 lynx Exp $ // vim:syntax=lpc:ts=8

#ifndef _SIGNATURE_H
#define _SIGNATURE_H

#define	SHandler	0
#define	SPreset		1
#define	SKeys		2

#define Signature	array(mixed)
#define new_Signature	({ })
#define cast_Signature

#define signaturep(node) pointerp(node)

#endif // _SIGNATURE_H
