#define CAT_MAX_LINES 50

varargs int cat(string file, int start, int num) 
{ 
    mixed *result = funcall(function mixed*() : int ex_call = extern_call() 
    {   
        if (ex_call) 
            set_this_object(previous_object(1)); 
 
        int more; 
 
        if (num < 0 || !this_player()) 
            return 0; 
 
        if (!start) 
            start = 1; 
 
        if (!num || num > CAT_MAX_LINES) { 
            num = CAT_MAX_LINES; 
            more = strlen(read_file(file, start+num, 1)); 
        }   
 
        string txt = read_file(file, start, num); 
        if (!txt) 
            return 0; 
         
  return ({txt, more}); 
    }); 
 
    if (!result) 
        return 0; 
 
    tell_object(this_player(), result[0]); 
 
    if (result[1]) 
        tell_object(this_player(), "***** GEKUERZT ****\n"); 
 
    return strlen(result[0] & "\n"); 
}

