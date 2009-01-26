// attic:

#if 0

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
