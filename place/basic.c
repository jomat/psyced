/* Last Change SNAKE    at DM0TUI1S on Fri, 27 Jul 1990 15:32:10 CES */

/* C64 version by CURBOIS Software, Holland.
 * Amiga/UNIX/VM version by LYNX, Teldor, 1990.
 * PSYC LPC version by symlynX, Berlin, 2003.
 *
 * $Id: basic.c,v 1.12 2007/08/15 23:58:06 lynx Exp $
 */
sys42336();
sys64738();
sys64764();

#define NAME "BASIC"
#define PLACE_HISTORY_EXPORT
#define ON_ENTER	sys64738(source);
#define ON_CONVERSE	sys42336(source, mc, data, vars);
#define CREATE		sys64764();
#include <place.gen>

static int listflag = 0;
static int chanzcount = 0;

static mapping tab;
static string *chanz;

sys64764() {
	tab = ([
"RUN": "RUN, RUN, RUN, DOESN'T ANYONE THINK OF MY LEGS?",
"?": "YOU AND YOUR ABBREVIATIONS...\nIF YOU MEAN 'PRINT' THEN WHY NOT JUST TYPE 'PRINT' ?",
"PRINT": "PRINT? DO YOU THINK I HAVE NOTHING BETTER TO DO\nTHAN TO CATER TO YOUR EVERY WHIM? LEAVE ME ALONE!",
"LOAD": "IF I THOUGHT YOU HAD ANYTHING WORTHWHILE, I WOULD",
"SAVE": "PRESS PLAY AND RECORD ON NEAREST STEREO",
"GOTO": "GO THERE YOURSELF, BUDDY!\nSEE HOW YOU LIKE IT",
"VERIFY": "NO NEED TO START THE TAPE\nI'LL VOUCH FOR IT",
"NEW": "BOY, AM I GLAD YOU GOT RID OF THAT GARBAGE\nWHY DON'T YOU LET ME REST NOW?",
"POKE": "OUCH!!!!! YOU LOOKING FOR A FAT LIP?",
"SYS": "SYS? WHAT SYS?",
"CALL": "YAAAAOOO!!! NO ANSWER",
"OPEN": "HAVE YOU GOT A CORKSCREW?",
"CLOSE": "I THINK I'LL JUST LEAVE IT OPEN, YOU'LL NEED IT LATER",
"DIM": "YEA?  WELL YOU'RE NOT SO BRIGHT EITHER",
"CLR": "HMM. WOULD YOU MIND IF I HELP MYSELF TO A GUMMY BEAR?",
"FOR": "?  FOR WITHOUT TO  ERROR",
"GOSUB": "IF I GOSUB I MIGHT NOT RETURN!!",
"RETURN": "?  RETURN WITHOUT NEXT  ERROR IN 4404",
"STOP": "BREAK IN 536.5^2",
"INPUT": "YEAH, I'D EAT ANYTHING *HUNGRY*",
"TRON": "SAW THE MOVIE, EH?",
"TROFF": "WILL YOU MAKE THE MOVIE FOR THIS ONE?",
"REST": "I FORGOT HOW TO DO THAT ONE",
"DELETE": "OK, I'LL DELETE THAT GARBAGE AND ALL OTHER...",
"DATA": "23 57 89 23 60 58 30 69 94 76 48 90 47 21 83 23.",
"KILL": "NO, PLEASE DONT DO THAT, IT'S JUST A BABY!",
"LET": "YEAH, LET THERE BE ROCK",
"LLIST": "SEND ME A PRINTER IN AN EMAIL AND I'LL DO IT.",
"PEEK": "KEEP YOUR HANDS OFF, YOU MACHO!",
"END": "EVERYTHING HAS AN END, EXCEPT ME.",
"WHILE": "MEANWHILE I'LL FORMAT THE HARDDISK",
"READ": "I ALREADY ERASED IT, SORRY, I THOUGHT YOU WERE NOT GOING TO NEED IT.",
"REM": "REMARKS ARE NO USE, YOU WON'T UNDERSTAND THE PROGRAM ANYWAY.",
"WAIT": "WHAT DO YOU THINK I'M DOING?? *YAWN*",
"HELP": "NO HUMAN CAN HELP YOU NOW.",
"ERASE": "IS THERE ANYTHING LEFT TO ERASE?"
	]);
/* edit erase exit get if lprint merge next
 restore troff wait ...  help */
 
	chanz = ({
"? PERMISSION DENIED",
"REDO FROM HALFWAY",
"? WIND WITHOUT WELL  ERROR",
"? STRING TOO MEANINGLESS  ERROR",
"? DEVICE NOT PRESENTABLE  ERROR",
"? TOO MANY FINGERS ON KEYBOARD  ERROR",
"? ILLEGAL MOTION IN BACKFIELD  ERROR",
"I THINK I HEARD A MEMORY CHIP EXPLODE!",
"I'M WRITING ON YOUR DISK !!!",
"? UNDEFINED PARACHUTE  ERROR",
"? STRANGE OUT OF RANGE  ERROR",
"SIT BACK A WAYS, YOU'LL RUIN YOUR EYES"
	});
}
 
#define puts(string) castmsg(ME, "_notice_application_basic", string, ([]))
#define tell(user,string) sendmsg(user,"_notice_application_basic",string,([]))

sys64738(source) {
	tell(source, "**** CBM BASIC V2 ****");
	tell(source, "49152 BASIC BYTES FREE");
	tell(source, "READY.");
}

sys42336(source, mc, data, mapping vars) {
	string t;

	if (stringp(data)) {
		sscanf(data, "%s %s", data, t);
		data = upper_case(data);
		if (tab[data]) puts(tab[data]);
		else if (data == "LIST") {
		    listflag = !listflag;
		    if (listflag) {
			 puts ("DIDN'T YOU WRITE IT DOWN SOMEWHERE?");
			 puts ("YOU KNOW HOW UNRELIABLE COMPUTERS CAN BE");
			 puts ("MAYBE TRY AGAIN");
		    } else {
			 puts ("10 DIMA(-5):FRY=1TO10:NEXTWEEK:POKE99,OUCH!:WAITFORIT:OPENFILEORRASP");
			 puts ("20 IF 1 HEN CAN LAY 3 EGGS IN ONE DAY");
			 puts ("30 HOW LONG WOULD IT TAKE A ROOSTER TO");
			 puts ("40 LAY A GOLDEN DOORKNOB?");
			 puts ("41.5 IFPEEKABOO(53280)=EGG THEN HALT AND CATCH FIRE");
			 puts ("50 END OF THE BEGINNING");
		    }
		} else if (data == "DIR") {
		    puts("DIRECTORY OF BERLIN, IOWA:");
		    puts(" HELLO.BAS                            GOTOSLEEP.BAS");
		    puts(" WHOLEPILACRAP.BAS                    IRC.BAS");
		    puts("8734123445782 BYTES OF PIZZA IN 4 BOXES.");
		} else if (random(4) < 1) {
		    if (!chanzcount) chanzcount = sizeof(chanz);
		    puts (chanz[--chanzcount]);
		}
		else return;
		puts ("READY.");
	}
}

