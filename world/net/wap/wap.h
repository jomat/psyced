// $Id: wap.h,v 1.4 2006/09/29 08:51:40 lynx Exp $ // vim:syntax=lpc
//
#define	HTOK htok3(prot, "text/vnd.wap.wml", "Cache-Control: no-cache\n")

// wml.textdb is the correct way to do this these days...

#define	WML_START "<?xml version=\"1.0\"?>\
<!DOCTYPE wml PUBLIC \"-//WAPFORUM//DTD WML 1.1//EN\"\
 \"http://www.wapforum.org/DTD/wml_1.1.xml\"><wml>\
<template><do type='prev' label='Back'><prev/></do></template>"

#define	WML_END "</wml>"

#define LOGIN_WML write("User:<br/><input type='text' name='u' emptyok='false' /><br/>"); \
                  write("Pass:<br/><input type='password' name='p' emptyok='false' /><br/>"); \
                  write("<a href='login?u=$(u)&amp;p=$(p)'>[login]</a>")

#define HEADER_WML(x) write(WML_START+"<card title='"+CHATNAME+" "+(x)+"'><p align='center'>")

#define NAV_WML write("<a href='inbox?u="+username+"&amp;p="+password+"'>[inbox]</a><a href='send?u="+username+"&amp;p="+password+"'>[send msg]</a><br/>");

#define SEND_PERSON(x) ("<a href='send?u="+username+"&amp;p="+password+"&amp;r="+(x)+"'>"+(x)+"</a>")

#define FOOTER_WML write("</p></card>\n" + WML_END)


