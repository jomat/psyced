// $Id: classic.i,v 1.4 2007/09/05 11:04:25 lynx Exp $ // vim:syntax=lpc:ts=8
/*
 * This is a mudlib file. Copy it to /obj/simul_efun.c, or
 * wherever the get_simul_efun() in master.c says.
 * The functions defined in this file should only be replacements of efuns
 * no longer supported. Don't use these functions any longer, use the
 * replacement instead.
 */

#include DRIVER_PATH "sys/erq.h"

#pragma strong_types
#pragma save_types

int file_time(string path)
{
    mixed *vec;

    PROTECT("FILE_TIME")

    set_this_object(previous_object());
    if (sizeof(vec=get_dir(path,4))) return vec[0];
}

#ifdef __EFUN_DEFINED(snoop)__
mixed snoop(mixed snoopee) {
# if 0
    int result;

    if (snoopee && query_snoop(snoopee)) {
        write("Busy.\n");
        return 0;
    }
    result = snoopee ? efun::snoop(this_player(), snoopee)
                     : efun::snoop(this_player());
    switch (result) {
	case -1:
	    write("Busy.\n");
	case  0:
	    write("Failed.\n");
	case  1:
	    write("Ok.\n");
    }
    if (result > 0) return snoopee;
# else
    return 0;
# endif
}
#endif

string query_host_name() { return __HOST_NAME__; }

#if 0 // ndef NO_MAPPINGS
mapping m_delete(mapping m, mixed key) {
    return efun::m_delete(copy_mapping(m), key);
}

nomask void set_this_player() {}

void shout(string s) {
    filter_array(users(), lambda(({'u}),({#'&&,
      ({#'environment, 'u}),
      ({#'!=, 'u, ({#'this_player})}),
      ({#'tell_object, 'u, to_string(s)})
    })));
}

/*
 * Function name: all_environment
 * Description:   Gives an array of all containers which an object is in, i.e.
 *		  match in matchbox in bigbox in chest in room, would for the
 *		  match give: matchbox, bigbox, chest, room 
 * Arguments:     ob: The object
 * Returns:       The array of containers.
 */
public object *
all_environment(object ob)
{
  object *r;
  
  if (!ob || !environment(ob)) return 0;
  if (!environment(environment(ob)))
      return ({ environment(ob) });
  r = ({ ob = environment(ob) });
  while (environment(ob))
      r = r + ({ ob = environment(ob) });
  return r;
}

#endif

string version() { return __VERSION__; }

