// stack implementation donated by psyc://psyced.org/~allo
//
#include <net.h>
#define ON_COMMAND	if (mycmd(command, args, source)) return 1;

#include <place.gen>

mycmd(a, args, source) {
    unless (source) source = previous_object();	// really needed???
	string *stack=allocate(0);
	if (v("stack")){
		stack=v("stack");
	}
	switch (a){
	case "push":
		stack=stack + ({ARGS(1)});
		vSet("stack", stack);
		castmsg(ME, "_notice_public_stack_add", "Eintrag #[_num] hinzugefuegt: "+stack[sizeof(stack)-1], (["_nick": "stack", "_num": sizeof(stack)]));
		break;
	case "get":
		if(sizeof(stack)>0){
			if(sizeof(args)==1){
				castmsg(ME, "_notice_public_stack", "Eintrag #[_num]: "+stack[sizeof(stack)-1], (["_nick": "stack", "_num": sizeof(stack)]));
			}else{
				args[1]=to_int(args[1]);
				if(sizeof(stack)>=args[1]){
					castmsg(ME, "_notice_public_stack", "Eintrag #[_num]: "+stack[(args[1])-1], (["_nick": "stack", "_num": args[1]]));
				}
			}
		}
		break;
	case "pop":
		if(sizeof(args)==1){
			if(sizeof(stack)>0){
				castmsg(ME, "_notice_public_stack_delete", "Eintrag #[_num] entfernt: "+stack[sizeof(stack)-1], (["_nick": "stack", "_num": sizeof(stack)]));
				stack=stack - ({stack[sizeof(stack)-1]});
				vSet("stack", stack);
			}
		}else{
			args[1]=to_int(args[1]);
			if(sizeof(stack)>=to_int(args[1])){
				castmsg(ME, "_notice_public_stack_delete", "Eintrag #[_num] entfernt: "+stack[(args[1])-1], (["_nick": "stack", "_num": args[1]]));
				stack=stack - ({stack[args[1]-1]});
				vSet("stack", stack);
			}
		}
	default:
		return 0;
		break;
	}
	return 1;
}
