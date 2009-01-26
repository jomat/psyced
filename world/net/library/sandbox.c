// vim:foldmethod=marker:syntax=lpc:noexpandtab
// $Id: sandbox.c,v 1.16 2008/02/06 12:16:16 lynx Exp $

#include <net.h>
#include <sandbox.h>

#if __EFUN_DEFINED__(shutdown)
MASK(shutdown, "SHUTDOWN")
#endif

#if __EFUN_DEFINED__(add_action)
MASK(add_action, "ADD_ACTION")
#endif

#if __EFUN_DEFINED__(break_point)
MASK(break_point, "BREAK_POINT")
#endif

#if __EFUN_DEFINED__(call_direct)
MASK(call_direct, "CALL_DIRECT")
#endif

#if __EFUN_DEFINED__(call_direct_resolved)
MASK(call_direct_resolved, "CALL_DIRECT_RESOLVED")
#endif

#if __EFUN_DEFINED__(call_resolved)
MASK(call_resolved, "CALL_RESOLVED")
#endif

#if __EFUN_DEFINED__(swap)
MASK(swap, "SWAP")
#endif

#if __EFUN_DEFINED__(debug_info)
MASK(debug_info, "DEBUG_INFO")
#endif

#if __EFUN_DEFINED__(command)
MASK(command, "COMMAND")
#endif

#if __EFUN_DEFINED__(command_stack)
MASK(command_stack, "COMMAND_STACK")
#endif

#if __EFUN_DEFINED__(command_stack_depth)
MASK(command_stack_depth, "COMMAND_STACK_DEPTH")
#endif

#if __EFUN_DEFINED__(convert_charset)
MASK(convert_charset, "CONVERT_CHARSET")
#endif

#if __EFUN_DEFINED__(debug_message) && DEBUG < 1
MASK(debug_message, "DEBUG_MESSAGE")
#endif

#if __EFUN_DEFINED__(disable_commands)
MASK(disable_commands, "DISABLE_COMMANDS")
#endif

#if __EFUN_DEFINED__(enable_commands)
MASK(enable_commands, "ENABLE_COMMANDS")
#endif

#if __EFUN_DEFINED__(function_exists)
MASK(function_exists, "FUNCTION_EXISTS")
#endif

#if __EFUN_DEFINED__(functionlist)
MASK(functionlist, "FUNCTIONLIST")
#endif

#if __EFUN_DEFINED__(garbage_collection)
MASK(garbage_collection, "GARBAGECOLLECTION")
#endif

#if __EFUN_DEFINED__(get_dir) // caught by valid_read?
MASK(get_dir, "GETDIR")
#endif

#if __EFUN_DEFINED__(include_list)
MASK(include_list, "INCLUDE_LIST")
#endif

#if __EFUN_DEFINED__(inherit_list)
MASK(inherit_list, "INHERIT_LIST")
#endif

#if __EFUN_DEFINED__(input_to_info)
MASK(input_to_info, "INPUT_TO_INFO")
#endif

#if __EFUN_DEFINED__(last_instructions)
MASK(last_instructions, "LAST_INSTRUCTIONS")
#endif

#if __EFUN_DEFINED__(load_object)
MASK(load_object, "LOAD_OBJECT")
#endif

#if __EFUN_DEFINED__(mkdir) // caught by valid_write?
MASK(mkdir, "MKDIR")
#endif

#if __EFUN_DEFINED__(move_object)
MASK(move_object, "MOVE_OBJECT")
#endif

#if __EFUN_DEFINED__(object_info)
MASK(object_info, "OBJECT_INFO")
#endif

#if __EFUN_DEFINED__(process_string)
MASK(process_string, "PROCESS_STRING")
#endif

#if __EFUN_DEFINED__(query_ip_name)
MASK(query_ip_name, "QUERY_IP_NAME")
#endif

#if __EFUN_DEFINED__(query_ip_number)
MASK(query_ip_number, "QUERY_IP_NUMBER")
#endif

#if __EFUN_DEFINED__(query_load_average)
MASK(query_load_average, "QUERY_LOAD_AVERAGE")
#endif

#if __EFUN_DEFINED__(query_shadowing)
MASK(query_shadowing, "QUERY_SHADOWING")
#endif

#if __EFUN_DEFINED__(remove_action)
MASK(remove_action, "REMOVE_ACTION")
#endif

#if __EFUN_DEFINED__(remove_input_to)
MASK(remove_input_to, "REMOVE_INPUT_TO")
#endif

#if __EFUN_DEFINED__(remove_interactive)
MASK(remove_interactive, "REMOVE_INTERACTIVE")
#endif

#if __EFUN_DEFINED__(rename) // seems to be caught by valid_(read|write), but
			     // i have to check that
MASK(rename, "RENAME")
#endif

#if __EFUN_DEFINED__(rm) // caught by valid_write?
MASK(rm, "RM")
#endif

#if __EFUN_DEFINED__(rmdir) // caught by valid_write?
MASK(rmdir, "RMDIR")
#endif

#if __EFUN_DEFINED__(rusage)
MASK(rusage, "RUSAGE")
#endif

#if __EFUN_DEFINED__(say)
MASK(say, "SAY")
#endif

#if __EFUN_DEFINED__(set_environment)
MASK(set_environment, "SET_ENVIRONMENT")
#endif

#if __EFUN_DEFINED__(set_is_wizard)
MASK(set_is_wizard, "SET_IS_WIZARD")
#endif

#if __EFUN_DEFINED__(set_modify_command)
MASK(set_modify_command, "SET_MODIFY_COMMAND")
#endif

#if __EFUN_DEFINED__(set_prompt)
MASK(set_prompt, "SET_PROMPT")
#endif

#if __EFUN_DEFINED__(swap)
MASK(swap, "SWAP")
#endif

#if __EFUN_DEFINED__(tell_object)
MASK(tell_object, "TELL_OBJECT")
#endif

#if __EFUN_DEFINED__(tell_room)
MASK(tell_room, "TELL_ROOM")
#endif

#if __EFUN_DEFINED__(tls_deinit_connection)
MASK(tls_deinit_connection, "TLS_DEINIT_CONNECTION")
#endif

#if __EFUN_DEFINED__(tls_init_connection)
MASK(tls_init_connection, "TLS_INIT_CONNECTION")
#endif

#if __EFUN_DEFINED__(tls_query_connection_info)
MASK(tls_query_connection_info, "TLS_QUERY_CONNECTION_INFO")
#endif

#if __EFUN_DEFINED__(tls_query_connection_state)
MASK(tls_query_connection_state, "TLS_QUERY_CONNECTION_STATE")
#endif

#if __EFUN_DEFINED__(transfer)
//MASK(transfer, "TRANSFER")
// avoid deprecation warning, and as nobody uses it:
nomask void transfer() {}
#endif

#if __EFUN_DEFINED__(unshadow)
MASK(unshadow, "UNSHADOW")
#endif

#if __EFUN_DEFINED__(users)
MASK(users, "USERS")
#endif

#if __EFUN_DEFINED__(variable_list)
MASK(variable_list, "VARIABLE_LIST")
#endif

#if __EFUN_DEFINED__(wizlist_info)
MASK(wizlist_info, "WIZLIST_INFO")
#endif

nomask mixed call_other(mixed obj, string func, varargs mixed * b) {
    P4(("call_other(%O, %O, %O)\n", obj, func, b));
    if (extern_call()) {
	mixed euid = geteuid(previous_object());

	if (stringp(euid) && euid[0] != '/') {
	    string s;
	    object o;

	    s = stringp(obj) ? obj : object_name(obj);
	    o = objectp(obj) ? obj : find_object(obj);

#ifndef __COMPAT_MODE__
	    if (s[0] == '/') s = s[1..];
#endif

	    if (euid[0] == '@') { // yes, this check is overdone, but only for
				  // now. i think we can use exactly this
				  // structure for allowed call_others into
				  // daemons. says tobij.
		  // <lynX> yes, checking the identity of daemons is much
		  // easier. some of them we already have stored in library
		  // vars, others have static object names, so they can be put
		  // into a switch or mapping. it's only doing so for "any"
		  // user object, which gets too messy. it is architecturally
		  // much cleaner to let sendmsg() decide which msg is legal
		  // (which it has to do anyway) so we don't need yet another
		  // security scheme in here with this wild guessing for user
		  // objects. in fact two redundant security approaches also
		  // unnecessarily raise the chances of being flawed. so let's
		  // allow as few -> operations as possible for sandbox mode.
		  //
		  // great idea. library_object()->sendmsg() _is_
		  // a -> operation, in case you forgot we're doing that (in
		  // entity.c).
		  //
		  // if we rename the library-sendmsg to something reachable
		  // without a call_other, anyways, sendmsg() will be an extra
		  // function call if it just redirects the msg to
		  // target->msg().
		  // in fact, it might be same complexity as calling target->msg
		  // directly, as call_other is overloaded and an lfun doing
		  // ... virtually the same thing for the discussed type of
		  // call.
		  //
		  // the if here isn't even more complex than in sendmsg, it'll
		  // be of equal or less complexity, plus, normal objects
		  // (and, if we want to allow ->qName for everybody anywhere,
		  // normal call_others (well, qName)
		  // don't run deep into this check.
		  //
		  // so, after all, the only argument is "evil! code!
		  // 'duplication'!", which i prefer over (for users who do
		  // their own rooms etc) incomprehensible
		  // 	#ifndef SADNBOX
		  // 	target->msg
		  // 	#else
		  // 	sendmsg(target, ...)
		  // 	#endif
		  // mess in files.
		  //
		  // additionally, my sandbox-philosophy is to keep things
		  // maximum equal in usage with and without SANDBOX,
		  // so i'll do it this way.
		unless (o == previous_object() || o == ME) {
		    int i;

		    if ((i = index(s, '#')) == -1) i = strlen(s) - 1;
		    unless (func == "qName"
#ifdef FORK
			    || func == "qOrigin"
#endif
			    ) switch (s[..i]) {
			case "net/person#":
			case "net/user#":
			    //if (func == "qName" || func == "msg") break;
			    if (func == "msg") break;
			default:
			    if (sscanf(s[..i], "net/%~s/user#")
				&& func == "msg") break;
			    raise_error(sprintf("INVALID %O "
						"call_other(%O, %O, %O)\n",
						euid, obj, func, b));
		    }
		}
	    } else {
		raise_error(sprintf("INVALID %O call_other(%O, %O, %O)\n",
				    euid, obj, func, b));
	    }
	} else unless (euid) {
	    raise_error(sprintf("INVALID euid-0 (%O) call_other(%O, %O, %O)\n",
				previous_object(), obj, func, b));
	}
	set_this_object(previous_object());
    }
    return efun::call_other(obj, func, b...);
}
