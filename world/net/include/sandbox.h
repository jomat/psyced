// vim:syntax=lpc
#ifndef _SANDBOX_H
# define _SANDBOX_H
# ifdef SANDBOX

#  define MASK(name, bname)	nomask mixed name (varargs array(mixed) args) {\
    if (extern_call()) {\
	if (!geteuid(previous_object()) || stringp(geteuid(previous_object()))\
	    && geteuid(previous_object())[0] != '/') {\
	    raise_error(sprintf("INVALID " bname " by %O(%O)\n",\
				previous_object(),\
				geteuid(previous_object())));\
	}\
	set_this_object(previous_object());\
    }\
    return efun::name (args...) ;\
}

#  define MASK2(type, name, args, args2, bname)	nomask type name args {\
    if (extern_call()) {\
	if (!geteuid(previous_object()) || stringp(geteuid(previous_object()))\
	    && geteuid(previous_object())[0] != '/') {\
	    raise_error(sprintf("INVALID " bname " by %O(%O)\n",\
				previous_object(),\
				geteuid(previous_object())));\
	}\
	set_this_object(previous_object());\
    }\
    return efun::name args2 ;\
}

#  define PROTECT(name)	if (extern_call()) {\
    if (!geteuid(previous_object()) || stringp(geteuid(previous_object()))\
	&& geteuid(previous_object())[0] != '/') {\
	raise_error(sprintf("INVALID " name " by %O(%O)\n", previous_object(),\
			    geteuid(previous_object())));\
    }\
}

# else

#  define PROTECT(name)
#  define MASK(name, bname)
#  define MASK2(type, name, args, args2, bname)

# endif
#endif
