#include <net.h>

/* a room that plays 17+4
 *
 * output messages are not multilingual/textdb compliant.  :(
 * they are also all missing trailing periods..  ;)
 */

#define ON_COMMAND	if (mycmd(command, args, source)) return 1;
#define NAME   "BLACKJACK"

#include <place.gen>

#define DECK_SIZE 52
#define GAME_RUNNING sizeof(players)
volatile array(int) deck;
volatile int deck_top; // oberste karte
volatile array(mixed) cards;
volatile array(int) deny; 
volatile array(object) players;
volatile int currentplayer;

volatile mapping colormap = ([ 0 : "Herz", 1 : "Karo", 2: "Pik", 3: "Kreuz" ]);
volatile mapping facemap = ([0:"As", 1:"2", 2:"3", 3:"4", 4:"5", 5:"6", 6:"7", 7:"8", 8:"9", 9:"10", 10:"Bube", 11:"Dame", 12:"Koenig" ]);
volatile mapping valuemap = ([0:11, 1:2, 2:3, 3:4, 4:5, 5:6, 6:7, 7:8, 8:9, 9:10, 10:10, 11:10, 12:10 ]);

#define card_name(i) colormap[i / 13] + " " + facemap[i % 13]
#define card_value(i) valuemap[i % 13]
#define PLAYER(o) (objectp(o) ? o->qName() : o)
void shuffle() {
    deck = allocate(DECK_SIZE);
    for (int i = 0; i < DECK_SIZE; i++)
	deck[i] = i;
    for (int i=0; i<DECK_SIZE-1; i++) {
	int r = i + random(DECK_SIZE-i);
	{ // swap
	    int t = deck[i]; 
	    deck[i] = deck[r]; 
	    deck[r] = t; 
	}
    }
    deck_top = 0;
}

int calculate_value() {
    int sum = 0;
    for (int i = 0; i < sizeof(cards[currentplayer]); i++) 
	sum += card_value(cards[currentplayer][i]);
    return sum;
}

int next_player() {
    for (int next = (currentplayer+1) % sizeof(players); 
	    next != currentplayer; 
	    next = (next + 1) % sizeof(players)) 
	if (!deny[next]) 
	    return next;
    if (!deny[currentplayer])
	return currentplayer;
    return -1;
}

end_game() {
    int winner_sum = 0;
    int winner = -1;
    int draw = 0;
    // evaluate statistics for each player
    for (currentplayer = 0; currentplayer < sizeof(players); currentplayer++) {
	int val = calculate_value();
	if (val > 21) 
	    continue;
	if (val > winner_sum) {
	    winner_sum = val;
	    winner = currentplayer;
	    draw = 0;
	} else if (val == winner_sum) {
	    if (sizeof(cards[currentplayer]) < sizeof(cards[winner])) {
		winner = currentplayer;
	    } else if (sizeof(cards[currentplayer]) == sizeof(cards[winner])) {
		draw = 1;
	    }
	}
    }
    if (winner == -1 || draw) {
	castmsg(ME, "_notice_place_game_end_draw", "Game over. It's a draw.", ([ ]));
    } else {
	castmsg(ME, "_notice_place_game_end", 
		"Game over. " + PLAYER(players[winner]) + " gewinnt mit " + winner_sum + " Punkten.",
		([ ]));
    }

    // reset globals
    currentplayer = 0;
    players = ({ });
}

// deals a card (and shows everyone (public_card=1) or just the player
void deal_card(int public_card) {
    int current_card = deck[deck_top];
    deck_top++;
    cards[currentplayer] += ({ current_card });
    if (public_card) {
	castmsg(ME, "_notice_place_game_card", 
		PLAYER(players[currentplayer]) + " zieht " + card_name(current_card)+". " + calculate_value() + " Punkte.", 
		([ ]));
    } else {
	sendmsg(players[currentplayer], "_notice_place_game_card", 
		"Du ziehst " + card_name(current_card) + ". " + calculate_value() + " Punkte.", 
		([ "_nick_place" : MYNICK ]));
    }
    if (current_card == DECK_SIZE) {
	end_game();
    }
}

start_game() {
    shuffle();
    players = m_indices(_u);
    deny = allocate(sizeof(players), 0);
    cards = allocate(sizeof(players), ({ }));

    castmsg(ME, "_notice_place_game_start", "Das Spiel beginnt.", ([ ]));
    // two rounds for everyone, first one public
    for (currentplayer = 0; currentplayer < sizeof(players); currentplayer++) {
	deal_card(1);
    }
    for (currentplayer = 0; currentplayer < sizeof(players); currentplayer++) {
	deal_card(0);
    }
    // no one can lose currently
    currentplayer = 0;
    castmsg(ME, "_notice_place_game_player_next", PLAYER(players[currentplayer]) + " ist am Zug", ([ ]));
}

take_card(object theplayer) {
    if (theplayer == players[currentplayer]) {
	deal_card(0); 
	if (calculate_value() > 21) { // verloren
	    // potentiell offenlegen
	    castmsg(ME, "_notice_place_game_player_lose", 
		    PLAYER(players[currentplayer]) + " verliert mit mehr als 21 Punkten", ([ ]));
	    deny[currentplayer] = 1;
	}
	if (players) {
	    currentplayer = next_player();
	    if (currentplayer != -1) 
		castmsg(ME, "_notice_place_game_player_next", PLAYER(players[currentplayer]) + " ist am Zug", ([ ]));
	    else
		end_game();
	}
	return 1;
    } else {
	P0(("%O ist nicht dran, %O ist am zug\n", theplayer, players[currentplayer]))
	// FIXME: du bist nicht dran
    }
}

deny_card(object theplayer) {
    if (theplayer == players[currentplayer]) {
	deny[currentplayer] = 1;
	castmsg(ME, "_notice_place_game_deny", PLAYER(players[currentplayer]) + " zieht keine Karte.", ([ ]));
	currentplayer = next_player();
	if (currentplayer != -1) 
	    castmsg(ME, "_notice_place_game_player_next", PLAYER(players[currentplayer]) + " ist am Zug", ([ ]));
	else
	    end_game();
    } else {
	// hm... not taking a card may be decided in advance?
    }
}


mycmd(a, args, source) {
    switch (a) {
    case "start":
	if (GAME_RUNNING) {
	    // FIXME: game is running, error
	    start_game();
	} else {
	    start_game();
	}
	return 1;
    case "take":
	if (GAME_RUNNING) {
	    take_card(source);
	} else {
	    P0(("take card went wrong, no game is running\n"))
	    // FIXME: no game running
	}
	return 1;
    case "deny":
	if (GAME_RUNNING) {
	    deny_card(source);
	} else {
	    P0(("deny card went wrong, no game is running\n"))
	    // FIXME: no gaming running
	}
	return 1;
    }
}
