// vim:syntax=lpc
//
// Function name:   update_file
// Description:     reads in a file, ignoring lines that begin with '#'
// Arguements:      file: a string that shows what file to read in.
// Return:          Array of nonblank lines that don't begin with '#'
// Note:            must be declared static (else a security hole)

static string *
update_file(string file)
{
	string *arr;
	string str;
	int i;

	str = read_file(file);
	if (!str) {
		return ({});
	}
	arr = explode(str, "\n");
	for (i = 0; i < sizeof(arr); i++) {
		if (arr[i][0] == '#') {
			arr[i] = 0;
		}
	}
	return arr;
}

// Function name:       epilog
// Return:              List of files to preload

string *
epilog(int load_empty)
{
	string *items;

	items = update_file(INIT_FILE);
	return items;
}

// preload an object

void
preload(string file)
{
	int t1;
	string err;

	if (file_size(file + ".c") == -1)
		return;

	t1 = time();
	write("Preloading : " + file + "...");
	err = catch(call_other(file, "??"));
	if (err != 0) {
		write("\nError " + err + " when loading " + file + "\n");
	} else {
		t1 = time() - t1;
		write("(" + t1/60 + "." + t1 % 60 + ")\n");
	}
}

