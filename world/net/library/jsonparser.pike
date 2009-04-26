// vim:syntax=lpc
// $Id: jsonparser.pike,v 1.5 2008/08/05 12:21:34 lynx Exp $
// 
// I really hate those comments.
//
// Copyright History:
// 1. Public Domain 2002 JSON.org
// 2. Ported to C# by Are Bjolseth, teleplan.no
// 3. Ported to pike by Bill Welliver
//    "Last night, I downloaded the JSON-C# code, and converted it to pike
//    (with relatively little effort, it was mostly a tedious reformatting
//    job)."
// 4. Adopted by Tobias 'tobij' Josefowitz, now uses Pike datastructures,
//    and can only be run in LDMud.
//
// As far as I am concerned, this still is Public Domain.
// I will probably once find out whether Are and Bill think the same wwy
// about it.

#ifdef __PIKE__
/// <summary>
/// <para>
///  A JSONTokener takes a source string and extracts characters and tokens from
///  it. It is used by the JSONObject and JSONArray constructors to parse
///  JSON source strings.
///  </para>
///  <para>
///  Public Domain 2002 JSON.org
///  @author JSON.org
///  @version 0.1
///  </para>
///  <para>Ported to C# by Are Bjolseth, teleplan.no</para>
///  <para>
///  <list type="bullet">
///  <item><description>Implement Custom exceptions</description></item>
///  <item><description>Add unit testing</description></item>
///  <item><description>Add log4net</description></item>
///  </list>
///  </para>
/// </summary>
/// <summary>The index of the next character.</summary>
int myIndex;
/// <summary>The source string being tokenized.</summary>
string mySource;

# define PROTECTED	public

# define THROW(x)	throw(Error.Generic(x))
# define SBGET(x)	(x)->get()

# define int2char(x)	String.int2char(x)
# define trim_whites(x)	String.trim_whites(x)

program objectbuilder, arraybuilder;
#else
# define PROTECTED	protected

PROTECTED int myIndex;
PROTECTED string mySource;

# define arrayp(x)	pointerp(x)
# define THROW(x)	raise_error(x)
# define UNDEFINED	0
# define SBGET(x)	(x)

# define int2char(x)	_int2char(x)
# define trim_whites(x)	trim(x)
# define search	strstr

PROTECTED mixed nextObject();

PROTECTED string _int2char(int c) {
    string s = " ";

    s[0] = c;
    return s;
}
#endif

/// <summary>
/// Construct a JSONTokener from a string.
/// </summary>
/// <param name="s">A source string.</param>
#ifdef __PIKE__
void create(string s, program|void objectb, program|void arrayb)
#else
mixed parse_json(string s)
#endif
{
	mySource = s;
	myIndex = 0;
#ifdef __PIKE__
	objectbuilder = objectb;
	arraybuilder = arrayb;
#else
	return nextObject();
#endif
}

/// <summary>
/// Back up one character. This provides a sort of lookahead capability,
/// so that you can test for a digit or letter before attempting to parse
/// the next number or identifier.
/// </summary>
PROTECTED void back() {
	if (myIndex > 0)
		myIndex -= 1;
}

/// <summary>
/// Determine if the source string still contains characters that next() can consume.
/// </summary>
/// <returns>true if not yet at the end of the source.</returns>
PROTECTED int more() {
	return myIndex < sizeof(mySource);
}

/// <summary>
/// Get the next character in the source string.
/// </summary>
/// <returns>The next character, or 0 if past the end of the source string.</returns>
#ifdef __PIKE__
PROTECTED int next(int|void x)
#else
varargs PROTECTED int next(int x)
#endif
{
   if(x) {
	int n = next();

	if (n != x) {
		THROW("Expected '" + x + "' and instead saw '" + n + "'.\n");
	}

	return n;
    } else {
	int c = more() ? mySource[myIndex] : 0;

	myIndex += 1;
	return c;
    }
}


/// <summary>
/// Get the next n characters.
/// </summary>
/// <param name="n">The number of characters to take.</param>
/// <returns>A string of n characters.</returns>
PROTECTED string nextn(int n) {
	int i = myIndex;
	int j = i + n;

	if (j >= sizeof(mySource)) {
		THROW("Substring bounds error\n");
	}
	myIndex += n;
	return mySource[i..j];
}

/// <summary>
/// Get the next char in the string, skipping whitespace
/// and comments (slashslash and slashstar).
/// </summary>
/// <returns>A character, or 0 if there are no more characters.</returns>
PROTECTED int nextClean() {
	while (1) {
		int c = next();
		if (c == '/') {
			switch (next()) {
				case '/':
					do {
						c = next();
					} while (c != '\n' && c != '\r' && c != 0);
					break;
				case '*':
					while (1) {
						c = next();

						if (c == 0) {
							THROW("Unclosed comment.\n");
						}

						if (c == '*') {
							if (next() == '/') {
								break;
							}

							back();
						}
					}

					break;
				default:
					back();
					return '/';
			}
		} 
		else if (c == 0 || c > ' ') {
			return c;
		}
	}

#ifndef __PIKE__
	return 0; // will never be reached, stupid LDMud!
#endif
}

/// <summary>
/// Return the characters up to the next close quote character.
/// Backslash processing is done. The formal JSON format does not
/// allow strings in single quotes, but an implementation is allowed to
/// accept them.
/// </summary>
/// <param name="quote">The quoting character, either " or '</param>
/// <returns>A String.</returns>
PROTECTED string nextString(int quote) {
	int c;
#ifdef __PIKE__
	String.Buffer sb = String.Buffer();
#else
	string sb = "";
#endif

	while (1) {
		c = next();
		if ((c == 0x00) || (c == 0x0A) || (c == 0x0D)) {
			THROW("Unterminated string.\n");
		}
		// CTRL chars
		if (c == '\\') {
			c = next();
			switch (c) {
				case 'b': //Backspace
					sb+="\b";
					break;
				case 't': //Horizontal tab
					sb+="\t";
					break;
				case 'n':  //newline
					sb+="\n";
					break;
				case 'f':  //Form feed
					sb+="\f";
					break;
				case 'r':  // Carriage return
					sb+="\r";
					break;
				case 'u':
#ifdef __PIKE__
					int iascii;
					sscanf(nextn(4), "%4x", iascii);
					sb+=int2char(iascii);
#else
					sb+=int2char(hex2int(nextn(2)));
					sb+=int2char(hex2int(nextn(2)));
#endif
					break;
				default:
					sb+=int2char(c);
					break;
			}
		} else {
			if (c == quote) {
#ifdef __PIKE__
				return SBGET(sb);
#else
				return sb;
#endif
			}
			sb+=int2char(c);
		}
	}//END-while

#ifndef __PIKE__
	return ""; // will never be reached. stupid LDMud
#endif
}

/// <summary>
/// Unescape the source text. Convert %hh sequences to single characters,
/// and convert plus to space. There are Web transport systems that insist on
/// doing unnecessary URL encoding. This provides a way to undo it.
/// </summary>

/// <summary>
/// Convert %hh sequences to single characters, and convert plus to space.
/// </summary>
/// <param name="s">A string that may contain plus and %hh sequences.</param>
/// <returns>The unescaped string.</returns>
#ifdef __PIKE__
PROTECTED string unescape(string|void s)
#else
PROTECTED varargs string unescape(string s)
#endif
{
	if(!s) s = mySource;
	int len = sizeof(s);
#ifdef __PIKE__
	String.Buffer sb = String.Buffer();
#else
	string sb = "";
#endif

	for (int i=0; i < len; i++) {
		int c = s[i];
		if (c == '+') {
			c = ' ';
		} else if (c == '%' && (i + 2 < len)) {
			i += 2;
			c = hex2int(s[i-1 .. i]);
		}
		sb+=int2char(c);
	}
	return SBGET(sb);
}

#ifdef __PIKE__
mapping|object jsonObject()
#else
mapping jsonObject()
#endif
{
#ifdef __PIKE__
	mixed addTo = objectbuilder ? objectbuilder() : ([ ]);
#else
	mapping addTo = ([ ]);
#endif

	if (next() == '%') {
		unescape();
	}

	back();

	if (nextClean() != '{') {
		THROW("A JSONObject must begin with '{'.\n");
	}

	while (1) {
		int c;
		string key;
		mixed obj;

		c = nextClean();
		switch (c) {
			case 0:
				THROW("A JSONObject must end "
						    "with '}'.\n");
			case '}':
#ifdef __PIKE__
				return mappingp(addTo)
					? addTo
					: ([object]addTo)->finish();
#else
				return addTo;
#endif
			case '"':
				back();
				// TODO:: error on != " || '
				key = nextObject();
				break;
			default:
				THROW("Non-String as "
						    "JSONObject-key. That "
						    "is invalid!\n");
		}

		if (nextClean() != ':') {
			PT(("jsonFAIL: '%c' at %O\n", nextClean(), myIndex))
			THROW("Expected a ':' after a key.\n");
		}

		obj = nextObject();

#ifdef __PIKE__
		if (mappingp(addTo)) {
		    ([mapping]addTo)[key] = obj;
		} else {
		    ([object]addTo)->add(key, obj);
		}
#else
		addTo[key] = obj;
#endif

		switch (nextClean()) {
			case ',':
				if (nextClean() == '}') {
#ifdef __PIKE__
					return mappingp(addTo) ? addTo : ([object]addTo)->finish();
#else
					return addTo;
#endif
				}

				back();
				break;
			case '}':
#ifdef __PIKE__
				return mappingp(addTo) ? addTo : ([object]addTo)->finish();
#else
				return addTo;
#endif
			default:
				THROW("Expected a ',' or '}'");
		}
	}

#ifdef __PIKE__
	return mappingp(addTo) ? addTo : ([object]addTo)->finish();
#else
	return addTo;
#endif
}

#ifdef __PIKE__
array|object jsonArray()
#else
mixed *jsonArray()
#endif
{
#ifdef __PIKE__
	mixed addTo = objectbuilder ? objectbuilder() : ({  });
#else
	mixed *addTo = ({ });
#endif

	if (nextClean() != '[') {
		THROW("A JSONArray must start with '['.\n");
	}

	if (nextClean() == ']') {
#ifdef __PIKE__
		return arrayp(addTo) ? addTo : ([object]addTo)->finish();
#else
		return addTo;
#endif
	}

	back();
	while (1) {
		if (arrayp(addTo)) {
		    addTo += ({ nextObject() });
		} else {
		    addTo->add(nextObject());
		}

		switch (nextClean()) 
		{
			case ',':
				if (nextClean() == ']') {
#ifdef __PIKE__
					return arrayp(addTo)
						? addTo
						: ([object]addTo)->finish();
#else
					return addTo;
#endif
				}
				back();
				break;
			case ']':
#ifdef __PIKE__
					return arrayp(addTo)
						? addTo
						: ([object]addTo)->finish();
#else
					return addTo;
#endif
			default:
				THROW("Expected a ',' or ']'.\n");
		}
	}

#ifdef __PIKE__
	return arrayp(addTo) ? addTo : ([object]addTo)->finish();
#else
	return addTo;
#endif
}

/// <summary>
/// Get the next value as object. The value can be a Boolean, Double, Integer,
/// JSONArray, JSONObject, or String, or the JSONObject.NULL object.
/// </summary>
/// <returns>An object.</returns>
PROTECTED mixed nextObject() {
	int c = nextClean();
	string s;

	if (c == '"' || c == '\'') {
		return nextString(c);
	}
	// Object
	if (c == '{') {
		back();
		return jsonObject();
	}

	// JSON Array
	if (c == '[') {
		back();
		return jsonArray();
	}

#ifdef __PIKE__
	String.Buffer sb = String.Buffer();
#else
	string sb = "";
#endif

	int b = c;
	while (c >= ' ' && c != ':' && c != ',' && c != ']' && c != '}' && c != '/') {
		sb+=int2char(c);
		c = next();
	}
	back();

	s = trim_whites(SBGET(sb));
	if (s == "true")
		return 1; 
	if (s == "false")
		return 0;
	if (s == "null")
		return UNDEFINED;

	if ((b >= '0' && b <= '9') || b == '.' || b == '-' || b == '+') {
	   int a; float b_; string c_;
	    sscanf(s, "%d%s", a, c_);
	   if(c_ && sizeof(c_)) {
#ifdef __PIKE__
	     sscanf(s, "%f", b_);
#else
	     b_ = to_float(s);
#endif
	     return b_;
	   }
	   else return a;
	}
	if (s == "") {
		THROW("Missing value.\n");
	}
	return s;
}

/// <summary>
/// Skip characters until the next character is the requested character.
/// If the requested character is not found, no characters are skipped.
/// </summary>
/// <param name="to">A character to skip to.</param>
/// <returns>
/// The requested character, or zero if the requested character is not found.
/// </returns>
PROTECTED int skipTo(int to) {
	int c;
	int i = myIndex;
	do {
		c = next();
		if (c == 0) {
			myIndex = i;
			return c;
		}
	}while (c != to);

	back();
	return c;
}

/// <summary>
/// Skip characters until past the requested string.
/// If it is not found, we are left at the end of the source.
/// </summary>
/// <param name="to">A string to skip past.</param>
PROTECTED void skipPast(string to) {
	myIndex = search(mySource, to, myIndex);
	if (myIndex < 0) {
		myIndex = sizeof(mySource);
	} else {
		myIndex += sizeof(to);
	}
}

// TODO implement exception SyntaxError


/// <summary>
/// Make a printable string of this JSONTokener.
/// </summary>
/// <returns>" at character [myIndex] of [mySource]"</returns>
PROTECTED string ToString() {
	return " at character " + myIndex + " of " + mySource;
}

