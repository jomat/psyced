// converts files given as argument into utf8 if they contain
// non iso-8859-15 characters. used to migrate '.o' files from
// the old SYSTEM_CHARSET to the newer one.
//
// written by el, debugged and styled by lynX

int main(int argc, array(string) args) {
    int rc = 0;
    //write("args: %O\n", args);
    object decoder = Locale.Charset.decoder("iso-8859-15");	

    if (sizeof(args) > 1) foreach (args[1..];;string filename) {
	if (Stdio.is_file(filename)) {
	    Stdio.File in;
	    string ret;

	    if (mixed err = catch {
		in = Stdio.File(filename, "r");
		ret = in->read();
		in->close();
		}) {
		
		write("failed reading %s: %O\n", filename, err);
		rc++;
		continue;
	    }

	    if (mixed err = catch {
		ret = utf8_to_string(ret);
		}) {
		    if (mixed err = catch {
		    decoder->clear();
		    decoder->feed(ret);
		    ret = decoder->drain();
		    }) {
			write("%s neither UTF-8 nor iso-8859-15!?\n", filename);
		    } else {
			if (mixed err = catch {
			    in = Stdio.File(filename, "w");
			    in->write(string_to_utf8(ret));
			    in->close();
			    }) {
				write("writing to %O failed: %O\n", filename, err);
				rc++;
			} else {
			    //write("wrote utf8 data to file.\n");
			    write("%s\n", filename);
			}
		    }
	    } else { // is utf8 alread!!
		write(" ");
	    }
	} else {
	    //write("ignoring non-file: %O.\n", filename);
	}
    } else {
	write("no files. finishing.\n");
    }
    return rc;
}

