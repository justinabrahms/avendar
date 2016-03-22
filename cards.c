#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "merc.h"
#include "cards.h"

/*
 * Aeolis' generic card routines.
 *
 * Disclaimer: Anybody touches this code, they die a horrible and painful death.
 *
 */

/*
 * Hearts mobvalue usage:
 *
 * 1: Current turn, 0 to wait for players.
 * 2: id of player 2.
 * 3: id of player 3.
 * 4: id of player 4.
 * 5: points of player 1.
 * 6: points of player 2.
 * 7: points of player 3.
 * 8: points of player 4.
 * 9: Current pass.
 *
 */


POKER_DATA *	new_poker_data();
OBJ_DATA *	find_cards(CHAR_DATA *ch);
char *		get_card_str(OBJ_DATA *cards);

OBJ_DATA *find_cards(CHAR_DATA *ch)
{
    OBJ_DATA *obj, *cards = NULL;

    for (obj = ch->carrying; obj; obj = obj->next_content)
    {
	if ((obj->item_type == ITEM_SPECIAL)
	 && (obj->value[0] == SOBJ_CARDS))
	{
	    cards = obj;
	    break;
	}
    }

    return cards;
}

char *get_card_str(OBJ_DATA *cards)
{
    AFFECT_DATA *paf;

    for (paf = cards->affected; paf; paf = paf->next)
	if (paf->type == gsn_stringdata)
	    return (char *) paf->point;

    return NULL;
}

void show_cards_to_char(CHAR_DATA *ch, char *cstr, char c, bool by_suit)
{
    int x, y;
    char buf[80];
    int cards[52];
    int numshown = 0;

    for (x = 0; x < 52; x++)
    {
	if (by_suit)
	    y = ((x % 13) * 4) + (x / 13);
	else
	    y = x;

	if (cstr[y] == c)
	{
	    cards[numshown] = y;
	    numshown++;
	}
    }

    if (numshown == 0)
        return;

    for (x = 0; x <= ((numshown - 1) / CPL); x++)
    {
	for (y = 0; y < CPL; y++)
	{
	    if (((x * CPL) + y) >= numshown)
	    {
		send_to_char("\n\r", ch);
		break;
	    }

	    send_to_char("+-----+ ", ch);

/*
	    sprintf(buf, "{W%3d{w+-----+{x", (x * CPL) + y + 1);
	    send_to_char(buf, ch);
*/

	    if (y == (CPL - 1))
		send_to_char("\n\r", ch);
	}

	for (y = 0; y < CPL; y++)
	{
	    if (((x * CPL) + y) >= numshown)
	    {
		send_to_char("\n\r", ch);
		break;
	    }

	    sprintf(buf, "{w|{%c%c%c   {w|{x ",
		(((cards[x * CPL + y] % 4) == 0) || ((cards[x * CPL + y] % 4) == 3)) ? 'D' : 'R',
		((cards[x * CPL + y] / 4) == 8 ) ? '1' :
		((cards[x * CPL + y] / 4) == 9 ) ? 'J' :
		((cards[x * CPL + y] / 4) == 10) ? 'Q' :
		((cards[x * CPL + y] / 4) == 11) ? 'K' :
		((cards[x * CPL + y] / 4) == 12) ? 'A' : (cards[x * CPL + y] / 4) + '2',
		((cards[x * CPL + y] / 4) == 8) ? '0' : ' ');

	    send_to_char(buf, ch);

	    if (y == (CPL - 1))
		send_to_char("\n\r", ch);
	}

	for (y = 0; y < CPL; y++)
	{
	    if (((x * CPL) + y) >= numshown)
	    {
		send_to_char("\n\r", ch);
		break;
	    }

	    sprintf(buf, "|  {%c%c  {w|{x ",
		(((cards[x * CPL + y] % 4) == 0) || ((cards[x * CPL + y] % 4) == 3)) ? 'D' : 'R',
		((cards[x * CPL + y] % 4) == 0) ? 'C' :
		((cards[x * CPL + y] % 4) == 1) ? 'D' :
		((cards[x * CPL + y] % 4) == 2) ? 'H' :
		((cards[x * CPL + y] % 4) == 3) ? 'S' : '?');

	    send_to_char(buf, ch);

	    if (y == (CPL - 1))
		send_to_char("\n\r", ch);
	}

	for (y = 0; y < CPL; y++)
	{
	    if (((x * CPL) + y) >= numshown)
	    {
		send_to_char("\n\r", ch);
		break;
	    }

	    sprintf(buf, "{w|   {%c%c%c{w|{x ",
		(((cards[x * CPL + y] % 4) == 0) || ((cards[x * CPL + y] % 4) == 3)) ? 'D' : 'R',
		((cards[x * CPL + y] / 4) == 8) ? '1' : ' ',
		((cards[x * CPL + y] / 4) == 8 ) ? '0' :
		((cards[x * CPL + y] / 4) == 9 ) ? 'J' :
		((cards[x * CPL + y] / 4) == 10) ? 'Q' :
		((cards[x * CPL + y] / 4) == 11) ? 'K' :
		((cards[x * CPL + y] / 4) == 12) ? 'A' : (cards[x * CPL + y] / 4 ) + '2');

	    send_to_char(buf, ch);

	    if (y == (CPL - 1))
		send_to_char("\n\r", ch);
	}

	for (y = 0; y < CPL; y++)
	{
	    if (((x * CPL) + y) >= numshown)
	    {
		send_to_char("\n\r", ch);
		break;
	    }

	    send_to_char("{w+-----+{x ", ch);

	    if (y == (CPL - 1))
		send_to_char("\n\r", ch);
	}

    }

    send_to_char("\n\r", ch);

    return;
	
}

char *cardnum_to_str(char *buf, int cnum)
{
    switch(cnum / 4)
    {
	case 0:		sprintf(buf, "two");	break;
	case 1:		sprintf(buf, "three");	break;
	case 2:		sprintf(buf, "four");	break;
	case 3:		sprintf(buf, "five");	break;
	case 4:		sprintf(buf, "six");	break;
	case 5:		sprintf(buf, "seven");	break;
	case 6:		sprintf(buf, "eight");	break;
	case 7:		sprintf(buf, "nine");	break;
	case 8:		sprintf(buf, "ten");	break;
	case 9:		sprintf(buf, "jack");	break;
	case 10:	sprintf(buf, "queen");	break;
	case 11:	sprintf(buf, "king");	break;
	case 12:	sprintf(buf, "ace");	break;
    }

    strcat(buf, " of ");

    switch(cnum % 4)
    {
	case 0:		strcat(buf, "clubs");	break;
	case 1:		strcat(buf, "diamonds");break;
	case 2:		strcat(buf, "hearts");	break;
	case 3:		strcat(buf, "spades");	break;
    }

    return buf;
}

int cardstr_to_num(char *cstr)
{
    int cnum;

    if ((cstr[0] >= '2')
     && (cstr[0] <= '9'))
	cnum = cstr[0] - 50;
    else
    {
	switch(cstr[0])
	{
	    case '1':
	        if (cstr[1] == '0')
		    cnum = 8;
	        else
		    cnum = -1;
	        break;
	    case 'j':
	    case 'J':
		cnum = 9;
		break;

	    case 'q':
	    case 'Q':
		cnum = 10;
		break;

	    case 'k':
	    case 'K':
		cnum = 11;
		break;

	    case 'a':
	    case 'A':
		cnum = 12;
		break;

	    default:
		cnum = -1;
	}
    }

    if (cnum == -1)
	return -1;

    if (cnum == 8)
	cstr++;

    cstr++;

    cnum *= 4;

    switch(UPPER(*cstr))
    {
	case 'C':
	    break;

	case 'D':
	    cnum += 1;
	    break;

	case 'H':
	    cnum += 2;
	    break;

	case 'S':
	    cnum += 3;
	    break;

	default:
	    return -1;
    }

    return cnum;
}

void do_cards(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char cbuf[20];
    char *cstr;
    OBJ_DATA *cards = NULL;
    CHAR_DATA *victim, *vch;
    int x, y, z, cnum, pnum = 0, lastturn = 0;
    CHAR_DATA *players[4];
    bool found;

    if (IS_NPC(ch))
	return;

    if (argument[0] == '\0')
    {
	send_to_char("Invalid option.  For help with cards, type 'help cards'.\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg);

    if (!str_cmp(arg, "join"))
    {
	if (ch->pcdata->cards_id > 0)
	{
	    send_to_char("You are already playing in a game of cards.\n\rType 'cards leave' to leave your current game.\n\r", ch);
	    return;
	}

	if ((victim = get_char_room(ch, argument)) == NULL)
	{
	    send_to_char("They're not here.\n\r", ch);
	    return;
	}

	if (IS_NPC(victim)
	 || ((cards = find_cards(victim)) == NULL)
	 || (victim->pcdata->cards_id != victim->id))
	{
	    act("$N is not currently dealing a game of cards.", ch, NULL, victim, TO_CHAR);
	    return;
	}

	switch (cards->objvalue[0])
	{
	    case CARDS_GAME_HEARTS:
	    {
		if (cards->objvalue[1] > 0)
		{
		    send_to_char("That game is already underway.\n\r", ch);
		    return;
		}

		for (x = 2; x <= 4; x++)
		{
		    if (cards->objvalue[x] == 0)
		    {
			cards->objvalue[x] = ch->id;
			ch->pcdata->cards_id = victim->id;
			act("You join $N's game of hearts.", ch, NULL, victim, TO_CHAR);
			act("$n joins your game of hearts.", ch, NULL, victim, TO_VICT);
			act("$n joins $N's game of hearts.", ch, NULL, victim, TO_NOTVICT);
			return;
		    }
		}
	    }
	    break;
	}

	send_to_char("There is not currently room in that game.\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "hand"))
    {
	if (ch->pcdata->cards_id == 0)
	{
	    send_to_char("You are not currently playing in a game of cards.\n\r", ch);
	    return;
	}
 
	if (((victim = get_char_by_id(ch->pcdata->cards_id)) == NULL)
	 || ((cards = find_cards(victim)) == NULL))
	{
	    send_to_char("Cards error: cannot locate dealer for game.\n\r", ch);
	    return;
        }

	switch(cards->objvalue[0])
	{
	    case CARDS_GAME_HEARTS:
	    {
		if (ch->pcdata->cards_id == ch->id)
		    pnum = 1;
		else
		{
	    	    for (x = 2; x < 5; x++)
	        	if (cards->objvalue[x] == ch->id)
		            pnum = x;
		}

		if (pnum == 0)
	    	    return;

		if ((cstr = get_card_str(cards)) == NULL)
	    	    return;

		show_cards_to_char(ch, cstr, (char) ('A' + pnum - 1), TRUE);
	    }
	    break;
	}

	return;
    }

    if (!str_cmp(arg, "leave"))
    {
	if (ch->pcdata->cards_id == 0)
	{
	    send_to_char("You are not currently playing in a game of cards.\n\r", ch);
	    return;
	}

	ch->pcdata->cards_id = 0;
    }

    if (!str_cmp(arg, "play"))
    {
	if (ch->pcdata->cards_id == 0)
	{
	    send_to_char("You are not currently playing in a game of cards.\n\r", ch);
	    return;
	}
 
	if (((victim = get_char_by_id(ch->pcdata->cards_id)) == NULL)
	 || ((cards = find_cards(victim)) == NULL))
	{
	    send_to_char("Cards error: cannot locate dealer for game.\n\r", ch);
	    return;
        }

	switch(cards->objvalue[0])
	{
	    case CARDS_GAME_HEARTS:
	    {
		int lead = -1, winner, points[4];

		if (ch->pcdata->cards_id == ch->id)
		    pnum = 1;
		else
		{
	    	    for (x = 2; x < 5; x++)
	        	if (cards->objvalue[x] == ch->id)
		            pnum = x;
		}

		if (pnum == 0)
	    	    return;

		if ((cstr = get_card_str(cards)) == NULL)
	    	    return;

		if ((cards->objvalue[1] < CARDS_HEARTS_TURN_1F)
		 || (cards->objvalue[1] > CARDS_HEARTS_TURN_4B)
		 || ((pnum % 4) != (cards->objvalue[1] % 4)))
		{
		    send_to_char("It is not your turn to play.\n\r", ch);
		    return;
		}

		for (x = 0; x < 52; x++)
		{
		    if (cstr[x] == '1')
			lead = x;

		    if ((cstr[x] >= '1') && (cstr[x] <= '4') && ((cstr[x] - '1' + 1) > lastturn))
			lastturn = cstr[x] - '1' + 1;
		}
		
		if ((cnum = cardstr_to_num(argument)) == -1)
		{
		    send_to_char("Invalid card reference.\n\rRead 'HELP CARDS' for help on card referencing.\n\r", ch);
		    return;
		}

		if (cstr[cnum] != ('A' + pnum - 1))
		{
		    send_to_char("You do not have that card in your hand.\n\r", ch);
		    return;
		}

		if ((IS_HEART(cnum) || (cnum == CARD_QS))
		 && (cards->objvalue[1] <= CARDS_HEARTS_TURN_4F))
		{
		    found = FALSE;
		    for (x = 0; x < 52; x++)
			if (!IS_HEART(x) && (cstr[x] == ('A' + pnum - 1)))
			{
			    found = TRUE;
			    break;
			}

		    if (found)
		    {
		        send_to_char("You may not play points on the first trick.\n\r", ch);
		        return;
		    }
		}

		if (lead == -1)     /* check to see if hearts broken */
		{
		    if (IS_HEART(cnum) && (cards->objvalue[1] <= CARDS_HEARTS_TURN_4A))
		    {
			found = FALSE;
			for (x = 0; x < 52; x++)
			    if (!IS_HEART(x) && (cstr[x] == ('A' + pnum - 1)))
			    {
				found = TRUE;
				break;
			    }

			if (found)
			{
			    send_to_char("Hearts have not yet been broken.\n\r", ch);
			    return;
			}
		    }
		}
		else if ((cnum % 4) != (lead % 4))
		{
		    found = FALSE;
		    for (x = (lead % 4); x < 52; x += 4)
		        if (cstr[x] == ('A' + pnum - 1))
		        {
			    found = TRUE;
			    break;
			}

		    if (found)
		    {
			send_to_char("Invalid play; you must follow suit.\n\r", ch);
			return;
		    }
		}
		  
		sprintf(buf, "You play the %s.\n\r", cardnum_to_str(cbuf, cnum));
		send_to_char(buf, ch);

		sprintf(buf, "$n plays the %s.", cardnum_to_str(cbuf, cnum));
		act(buf, ch, NULL, NULL, TO_ROOM);

		if (IS_HEART(cnum) && (cards->objvalue[1] <= CARDS_HEARTS_TURN_4A))
		    cards->objvalue[1] += 4;

		cstr[cnum] = '1' + lastturn;

		if (lastturn == 3)
		{
		    int wnum;

		    for (x = (lead % 4); x < 52; x += 4)
		    {
			if (((cstr[x] >= '1') && (cstr[x] <= '4'))
			 && (x > lead))
			    lead = x;
		    }

		    winner = cstr[lead] - '1' + 1;

		    wnum = ((pnum + winner - 1) % 4) + 1;

		    /* if (wnum == 4)
			victim = ch;
		    else */ if (wnum == 1)
			victim = get_char_by_id(ch->pcdata->cards_id);
		    else
		        victim = get_char_by_id(cards->objvalue[wnum]);
		    
		    act("You win the trick.", victim, NULL, NULL, TO_CHAR);
		    act("$n wins the trick.", victim, NULL, NULL, TO_ROOM);

		    for (x = 0; x < 52; x++)
			if ((cstr[x] >= '1') && (cstr[x] <= '4'))
			    cstr[x] += 4;

		    if (!strchr(cstr, 'A'))   /* should mean hand is over */
		    {
			for (x = 0; x < 4; x++)
			    points[x] = 0;

			for (x = 0; x < 52; x++)
			{
			    if (IS_HEART(x))
			        points[cstr[x] - '5'] += 1;

			    if (x == CARD_QS)
				points[cstr[x] - '5'] += 13;
			}

			for (x = 0; x < 4; x++)
			{
			    if (points[x] == 26)
			    {
				if (x == 0)
				    victim = get_char_by_id(ch->pcdata->cards_id);
				else
				    victim = get_char_by_id(cards->objvalue[x+1]);

				act("You have shot the moon!", victim, NULL, NULL, TO_CHAR);
				act("$n has shot the moon!", victim, NULL, NULL, TO_ROOM);
		
				for (y = 0; y < 4; y++)
				    if (y == x)
					points[y] = 0;
				    else
					points[y] = 26;
			    }
			}

			found = FALSE;
			for (x = 0; x < 4; x++)
			{
			    cards->objvalue[5 + x] += points[x];
			    if (cards->objvalue[5 + x] >= 100)
				found = TRUE;
			}

			if (found)
			{
			    winner = 1;
			    for (x = 2; x < 5; x++)
				if (cards->objvalue[4 + x] < cards->objvalue[4 + winner])
				    winner = x;

			    if (winner == 1)
				victim = get_char_by_id(ch->pcdata->cards_id);
			    else
				victim = get_char_by_id(cards->objvalue[winner]);

			    act("The game is over!", ch, NULL, NULL, TO_CHAR);
			    act("The game is over!", ch, NULL, NULL, TO_ROOM);

			    act("You are the winner!", ch, NULL, NULL, TO_CHAR);
			    act("$n is the winner!", victim, NULL, NULL, TO_ROOM);

			    cards->objvalue[0] = 0;
			    return;
			}

			/** Dealing out the cards **/
		
			for (x = 0; x < 52; x++)
			{
		    	    cnum = number_range(1, 52-x);
		    	    for (y = 0; y < 52; y++)
				if (cstr[y] == ' ')
				{
			    	    cnum--;
			    	    if (cnum == 0)
			    	    {
					cstr[y] = (x % 4) + 'A';
					break;
			    	    }
				}
			}

			victim = get_char_by_id(ch->pcdata->cards_id);

			act("You deal a new round of cards.", victim, NULL, NULL, TO_CHAR);
			act("$n deals a new round of cards.", victim, NULL, NULL, TO_ROOM);
			show_cards_to_char(victim, cstr, 'A', TRUE);

			for (x = 1; x < 4; x++)
		    	    show_cards_to_char(players[x], cstr, (char) ('A' + x), TRUE);

			cards->objvalue[9] += 1;
			if (cards->objvalue[9] == 4)
			    cards->objvalue[9] = 0;

			if (cards->objvalue[9] == 3)
			{
		    	    cards->objvalue[1] = cstr[0] - 64;
		    	    send_to_char("It is your turn, lead with the 2 of clubs.\n\r", players[cards->objvalue[1]]);
			}
			else
			{
		    	    cards->objvalue[1] = CARDS_HEARTS_PASS;
		    	    sprintf(buf, "Current pass is %s.", 
				(cards->objvalue[9] == 0) ? "to the left" :
				(cards->objvalue[9] == 1) ? "to the right" :
				(cards->objvalue[9] == 2) ? "across" : "none");
		    	    act(buf, ch, NULL, NULL, TO_CHAR);
		    	    act(buf, ch, NULL, NULL, TO_ROOM);
			}

			return;

		    }

		    if (cards->objvalue[1] >= CARDS_HEARTS_TURN_1B)
			cards->objvalue[1] = CARDS_HEARTS_TURN_1B + wnum - 1;
		    else
			cards->objvalue[1] = CARDS_HEARTS_TURN_1A + wnum - 1;

		    if (wnum == 1)
			vch = get_char_by_id(ch->pcdata->cards_id);
		    else
			vch = get_char_by_id(cards->objvalue[wnum]);

		    send_to_char("It is your turn to play.\n\r", vch);
		    do_cards(vch, "hand");
		}
		else
		{
		    if (cards->objvalue[1] == CARDS_HEARTS_TURN_4A)
			cards->objvalue[1] = CARDS_HEARTS_TURN_1A;
		    else if (cards->objvalue[1] == CARDS_HEARTS_TURN_4B)
			cards->objvalue[1] = CARDS_HEARTS_TURN_1B;
		    else if (cards->objvalue[1] == CARDS_HEARTS_TURN_4F)
			cards->objvalue[1] = CARDS_HEARTS_TURN_1F;
		    else
			cards->objvalue[1] += 1;

		    if ((cards->objvalue[1] % 4) == 1)
		        victim = get_char_by_id(ch->pcdata->cards_id);
		    else
			victim = get_char_by_id(cards->objvalue[((cards->objvalue[1] - 1) % 4) + 1]);

		    send_to_char("It is your turn to play.\n\r", victim);
		    do_cards(victim, "hand");
		}

		return;
	    }
	}
    }

    if (!str_cmp(arg, "status"))
    {
	if (ch->pcdata->cards_id == 0)
	{
	    send_to_char("You are not currently playing in a game of cards.\n\r", ch);
	    return;
	}

	if (((victim = get_char_by_id(ch->pcdata->cards_id)) == NULL)
	 || ((cards = find_cards(victim)) == NULL))
	{
	    send_to_char("Cards error: cannot locate dealer for game.\n\r", ch);
	    return;
        }

	switch(cards->objvalue[0])
	{
	    case CARDS_GAME_HEARTS:
	    {
		if (ch->pcdata->cards_id == ch->id)
		    pnum = 1;
		else
		{
	    	    for (x = 2; x < 5; x++)
	        	if (cards->objvalue[x] == ch->id)
		            pnum = x;
		}

		if (pnum == 0)
	    	    return;

		if ((cstr = get_card_str(cards)) == NULL)
	    	    return;

		send_to_char("You are currently playing Hearts.\n\r\n\r", ch);

		send_to_char("Player status:\n\r", ch);

		lastturn = 0;
		for (x = 0; x < 52; x++)
		    if ((cstr[x] >= '1') && (cstr[x] <= '4') && ((cstr[x] - '1' + 1) > lastturn))
			lastturn = cstr[x] - '1' + 1;
	
		for (x = 1; x < 5; x++)
		{
		    if (x == 1)
			victim = get_char_by_id(ch->pcdata->cards_id);
		    else
			victim = get_char_by_id(cards->objvalue[x]);

		    if (!victim)
		    {
			sprintf(buf, "   %d. Waiting for player.\n\r", x);
			send_to_char(buf, ch);
			continue;
		    }

		    sprintf(buf, "   %d. %-12s: ", x, victim->name);
		    send_to_char(buf, ch);
			
		    if (cards->objvalue[1] == CARDS_HEARTS_PASS)
		    {
		        if (strchr(cstr, 'a' + x - 1))
			    send_to_char("Waiting for others.           ", ch);
			else
			    send_to_char("Choosing cards to pass.       ", ch);
		    }
		    else
		    {
			z = (x + 4 - lastturn) % 4;
			z = (x - z + 5) % 4;

			if (z == 0)
			    z = 4;

			found = FALSE;
			for (y = 0; y < 52; y++)
			{
			    if (cstr[y] == '1' + z - 1)
			    {
				sprintf(buf, "Played the %-19s", cardnum_to_str(cbuf, y));
				found = TRUE;
				break;
			    }
			}

			if (!found)
			    send_to_char("{DHas not played.{x               ", ch);
			else
			    send_to_char(buf, ch);
		    }

		    sprintf(buf, "(%d points)\n\r", cards->objvalue[x + 4]);
		    send_to_char(buf, ch);
		}

		if ((cards->objvalue[1] >= CARDS_HEARTS_TURN_1F)
		 && (cards->objvalue[1] <= CARDS_HEARTS_TURN_4B))
		{
		    if ((cards->objvalue[1] % 4) == pnum)
			sprintf(buf, "It is currently your turn to play.\n\r");
		    else if ((cards->objvalue[1] % 4) == 1)
		    {
			victim = get_char_by_id(ch->pcdata->cards_id);
			sprintf(buf, "It is currently %s's turn to play.\n\r", victim->name);
		    }
		    else if ((cards->objvalue[1] % 4) == 0)
		    {
			victim = get_char_by_id(cards->objvalue[4]);
			sprintf(buf, "It is currently %s's turn to play.\n\r", victim->name);
		    }
		    else
		    {
			victim = get_char_by_id(cards->objvalue[cards->objvalue[1] % 4]);
			sprintf(buf, "It is currently %s's turn to play.\n\r", victim->name);
		    }

		    send_to_char(buf, ch);
		}
	    }
	}

	return;
    }

		
		
					
	

    if (!str_cmp(arg, "pass"))
    {
	int pass[3];
	bool found = FALSE;

	if (ch->pcdata->cards_id == 0)
	{
	    send_to_char("You are not currently playing in a game of cards.\n\r", ch);
	    return;
	}
 
	if (((victim = get_char_by_id(ch->pcdata->cards_id)) == NULL)
	 || ((cards = find_cards(victim)) == NULL))
	{
	    send_to_char("Cards error: cannot locate dealer for game.\n\r", ch);
	    return;
        }

	if (cards->objvalue[0] != CARDS_GAME_HEARTS)
	{
	    send_to_char("That command is only used when playing hearts.\n\r", ch);
	    return;
	}

	if (cards->objvalue[1] != CARDS_HEARTS_PASS)
	{
	    send_to_char("It is not time to pass.\n\r", ch);
	    return;
	}

	if (ch->pcdata->cards_id == ch->id)
	    pnum = 1;
	else
	{
	    for (x = 2; x < 5; x++)
	        if (cards->objvalue[x] == ch->id)
		    pnum = x;
	}

	if (pnum == 0)
	    return;

	if ((cstr = get_card_str(cards)) == NULL)
	    return;

	if (strchr(cstr, 96 + pnum))
	{
	    send_to_char("You have already chosen cards to pass.\n\r", ch);
	    return;
	}

	for (x = 0; x < 3; x++)
	{
	    argument = one_argument(argument, arg);

	    if (arg[0] == '\0')
	    {
		send_to_char("Syntax: cards pass <card 1> <card 2> <card 3>\n\r", ch);
		return;
	    }

	    if ((pass[x] = cardstr_to_num(arg)) == -1)
	    {
		send_to_char("Invalid card reference.\n\rRead 'HELP CARDS' for help on card referencing.\n\r", ch);
		return;
	    }

	    if (cstr[pass[x]] != (64 + pnum))
	    {
		send_to_char("You do not have those three cards in your hand.\n\r", ch);
		return;
	    }
	}

	for (x = 0; x < 3; x++)
	    cstr[pass[x]] = 'a' - 1 + pnum;

	switch(cards->objvalue[9])
	{
	    case 0:
		if (pnum != 4)
		    victim = get_char_by_id(cards->objvalue[pnum+1]);
		else
		    victim = get_char_by_id(ch->pcdata->cards_id);
		break;
	    case 1:
		switch (pnum)
		{
		    case 1: victim = get_char_by_id(cards->objvalue[4]); break;
		    case 2: victim = get_char_by_id(ch->pcdata->cards_id); break;
		    case 3: victim = get_char_by_id(cards->objvalue[2]); break;
		    case 4: victim = get_char_by_id(cards->objvalue[3]); break;
		}
		break;

	    case 2:
		switch (pnum)
		{
		    case 1: victim = get_char_by_id(cards->objvalue[3]); break;
		    case 2: victim = get_char_by_id(cards->objvalue[4]); break;
		    case 3: victim = get_char_by_id(ch->pcdata->cards_id); break;
		    case 4: victim = get_char_by_id(cards->objvalue[2]); break;
		}
		break;
	}

	act("You select three cards to pass to $N.", ch, NULL, victim, TO_CHAR);

	for (x = 0; x < 4; x++)
	    if (!strchr(cstr, 'a' + x))
	    {
		found = TRUE;
		break;
	    }

	if (!found)
	{
	    switch(cards->objvalue[9])
	    {
		case 0:
		{
		    for (x = 0; x < 3; x++)
		        *strchr(cstr, 'a') = 'B';
		    for (x = 0; x < 3; x++)
		        *strchr(cstr, 'b') = 'C';
		    for (x = 0; x < 3; x++)
		        *strchr(cstr, 'c') = 'D';
		    for (x = 0; x < 3; x++)
		        *strchr(cstr, 'd') = 'A';

		    vch = get_char_by_id(ch->pcdata->cards_id);
		    victim = get_char_by_id(cards->objvalue[2]);
		    act("You receive three cards from $N.", victim, NULL, vch, TO_CHAR);
		    vch = get_char_by_id(cards->objvalue[3]);
		    act("You receieve three cards from $N.", vch, NULL, victim, TO_CHAR);
		    victim = get_char_by_id(cards->objvalue[4]);
		    act("You receieve three cards from $N.", victim, NULL, vch, TO_CHAR);
		    vch = get_char_by_id(ch->pcdata->cards_id);
		    act("You receieve three cards from $N.", vch, NULL, victim, TO_CHAR);
		}
		break;
		case 1:
		{
		    for (x = 0; x < 3; x++)
		        *strchr(cstr, 'a') = 'D';
		    for (x = 0; x < 3; x++)
		        *strchr(cstr, 'b') = 'A';
		    for (x = 0; x < 3; x++)
		        *strchr(cstr, 'c') = 'B';
		    for (x = 0; x < 3; x++)
		        *strchr(cstr, 'd') = 'C';

		    vch = get_char_by_id(ch->pcdata->cards_id);
		    victim = get_char_by_id(cards->objvalue[4]);
		    act("You receive three cards from $N.", victim, NULL, vch, TO_CHAR);
		    vch = get_char_by_id(cards->objvalue[3]);
		    act("You receieve three cards from $N.", vch, NULL, victim, TO_CHAR);
		    victim = get_char_by_id(cards->objvalue[2]);
		    act("You receieve three cards from $N.", victim, NULL, vch, TO_CHAR);
		    vch = get_char_by_id(ch->pcdata->cards_id);
		    act("You receieve three cards from $N.", vch, NULL, victim, TO_CHAR);
		}
		break;
		case 2:
		{
		    for (x = 0; x < 3; x++)
		        *strchr(cstr, 'a') = 'C';
		    for (x = 0; x < 3; x++)
		        *strchr(cstr, 'b') = 'D';
		    for (x = 0; x < 3; x++)
		        *strchr(cstr, 'c') = 'A';
		    for (x = 0; x < 3; x++)
		        *strchr(cstr, 'd') = 'B';

		    vch = get_char_by_id(cards->objvalue[2]);
		    victim = get_char_by_id(cards->objvalue[4]);
		    act("You receive three cards from $N.", victim, NULL, vch, TO_CHAR);
		    act("You receive three cards from $N.", vch, NULL, victim, TO_CHAR);
		    vch = get_char_by_id(ch->pcdata->cards_id);
		    victim = get_char_by_id(cards->objvalue[3]);
		    act("You receive three cards from $N.", victim, NULL, vch, TO_CHAR);
		    act("You receive three cards from $N.", vch, NULL, victim, TO_CHAR);
		}
		break;
	    }

	    show_cards_to_char(get_char_by_id(ch->pcdata->cards_id), cstr, 'A', TRUE);

	    for (x = 1; x < 4; x++)
		show_cards_to_char(get_char_by_id(cards->objvalue[x+1]), cstr, (char) ('A' + x), TRUE);

	    cards->objvalue[1] = cstr[0] - 64;
	    send_to_char("It is your turn, lead with the 2 of clubs.\n\r",
		(cards->objvalue[1] == 1) ? get_char_by_id(ch->pcdata->cards_id)
					  : get_char_by_id(cards->objvalue[cards->objvalue[1]]));
	}	

	return;
    }
	
    if ((cards = find_cards(ch)) == NULL)
    {
	send_to_char("You are not carrying any cards.\n\r", ch);
	return;
    }
 
    if ((cstr = get_card_str(cards)) == NULL)
    {
	bug("Error: cannot find stringdata on cards.", 0);
	return;
    }

    if (!str_cmp(arg, "game"))
    {
	if (cards->objvalue[0] > 0)
	{
	    send_to_char("You are already playing a game with your cards.\n\r", ch);
	    return;
	}

	if (ch->pcdata->cards_id > 0)
	{
	    send_to_char("You are already playing in a game of cards.\n\r", ch);
	    return;
	}

	if (argument[0] != '\0')
	{
	    if (!str_cmp(argument, "hearts"))
	    {
		send_to_char("You prepare your cards for a game of hearts.\n\r", ch);
		act("Others must type 'cards join $n' to join your game.", ch, NULL, NULL, TO_CHAR);
		send_to_char("Once you have four players, type 'cards start' to begin the game.\n\r", ch);
		ch->pcdata->cards_id = ch->id;
		cards->objvalue[0] = CARDS_GAME_HEARTS;
		for (x = 1; x < 10; x++)
		    cards->objvalue[x] = 0;
		return;
	    }
	}

	send_to_char("Syntax: cards game <game type>\n\rCurrent game types are: hearts\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "start"))
    {
	if (ch->pcdata->cards_id != ch->id)
	{
	    send_to_char("You are not dealing a game of cards.\n\r", ch);
	    return;
	}

	switch(cards->objvalue[0])
	{
	    case CARDS_GAME_HEARTS:
	    {
		for (x = 1; x <= 3; x++)
		{
		    players[x] = get_char_by_id(cards->objvalue[x+1]);
		    if (!players[x])
		    {
			sprintf(buf, "Cannot find player %d, clearing position.\n\r", x+1);
			send_to_char(buf, ch);
			cards->objvalue[x+1] = 0;
		    }
		}

		if (!players[1] || !players[2] || !players[3])
		{
		    send_to_char("Hearts requires four players to start.\n\r", ch);
		    return;
		}

		players[0] = ch;

		send_to_char("You start the game of hearts, dealing yourself 13 cards.\n\r", ch);

		/** Dealing out the cards **/
		
		for (x = 0; x < 52; x++)
		{
		    cnum = number_range(1, 52-x);
		    for (y = 0; y < 52; y++)
			if (cstr[y] == ' ')
			{
			    cnum--;
			    if (cnum == 0)
			    {
				cstr[y] = (x % 4) + 'A';
				break;
			    }
			}
		}

		show_cards_to_char(ch, cstr, 'A', TRUE);

		for (x = 1; x < 4; x++)
		{
		    act("$n begins the game of hearts, dealing you 13 cards.", ch, NULL, players[x], TO_VICT);
		    show_cards_to_char(players[x], cstr, (char) ('A' + x), TRUE);
		}

		if (cards->objvalue[9] == 3)
		{
		    cards->objvalue[1] = cstr[0] - 64;
		    send_to_char("It is your turn, lead with the 2 of clubs.\n\r", players[cards->objvalue[1]]);
		}
		else
		{
		    cards->objvalue[1] = CARDS_HEARTS_PASS;
		    sprintf(buf, "Current pass is %s.", 
			(cards->objvalue[9] == 0) ? "to the left" :
			(cards->objvalue[9] == 1) ? "to the right" :
			(cards->objvalue[9] == 2) ? "across" : "none");
		    act(buf, ch, NULL, NULL, TO_CHAR);
		    act(buf, ch, NULL, NULL, TO_ROOM);
		}

		return;
	    }
	    break;
	}
    }
		

    return;
}

void do_poker(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *dealer = NULL, *vch;
    CHAR_DATA *players[MAX_POKERPLAYER];
    OBJ_DATA *deck;
    char *cstr;
    AFFECT_DATA *sdata, *paf;
    POKER_DATA *pdata = NULL;
    int x;
    DESCRIPTOR_DATA *d;   
    
    if (!ch->in_room)
	return;

    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
	if (IS_SET(vch->nact, ACT_DEALER))
	{
	    dealer = vch;
	    break;
	}

    if (!dealer)
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    for (deck = dealer->carrying; deck; deck = deck->next_content)
	if ((deck->item_type == ITEM_SPECIAL) && (deck->value[0] == SOBJ_CARDS))
	    break;

    if (!deck)
    {
	deck = create_object(get_obj_index(OBJ_VNUM_CARDS_GENERIC), dealer->level);
	obj_to_char(deck, dealer);
    }

    for (sdata = deck->affected; sdata; sdata = sdata->next)
	if (sdata->point && (sdata->type == gsn_stringdata))
	    break;

    if (!sdata)
    {
	bug("Error: no stringdata located for cards.", 0);
	return;
    }

    cstr = (char *) sdata->point;

    for (paf = dealer->affected; paf; paf = paf->next)
	if (paf->type == gsn_stringdata)
	{
	    pdata = (POKER_DATA *) paf->point;
	    break;
	}

    if (!pdata)
    {
	AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

	pdata = new_poker_data();

	af.where     = TO_AFFECTS;
	af.type      = gsn_stringdata;
	af.level     = dealer->level;
	af.location  = APPLY_NONE;
	af.point     = (void *) pdata;
	af.duration  = -1;
	af.modifier  = 0;
	af.bitvector = 0;
	affect_to_char(dealer, &af);
    }

    for (x = 0; x < MAX_POKERPLAYER; x++)
    {
	players[x] = NULL;
	if (dealer->mobvalue[x] > 0)
	    for (d = descriptor_list; d; d = d->next)
		if ((d->connected == CON_PLAYING)
		 && d->character
		 && (dealer->mobvalue[x] == d->character->id))
		    players[x] = d->character;
    }

    return;
}

POKER_DATA *new_poker_data()
{
    POKER_DATA *pdata;

    pdata = (POKER_DATA*)malloc(sizeof(struct poker_data));
    g_num_poker++;

    return pdata;
}

/*
void poker_update(CHAR_DATA *dealer)
{
*/

