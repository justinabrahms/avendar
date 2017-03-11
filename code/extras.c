#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "merc.h"
#include "extras.h"

/*
 * Aeolis' chess program.
 *
 * Disclaimer: Anybody touches this code, they die a horrible and painful death.
 *
 */

/*
 * Mobvalue usage:
 *
 * 0: id of player 1 (white)
 * 1: id of player 2 (black)
 * 2: current turn (0 = white, 1 = black)
 * 3: keep track of destination of last movement
 * 4: keep track of origin of last movement
 * 5: a number of toggle flags
 * 6: keeps track of the previous character at movement destination
 *
 */

char get_chess_char		args( ( char *cstr, int x, int y ) );
char *chess_char_to_name	args( ( char inchar ) );
void set_chess_char		args( ( char *cstr, int x, int y, char tchar) );
int  chess_can_any_reach	args( ( char *cstr, OBJ_DATA *board, int x, 
					int y, bool white, bool incl_king,
					bool checkvalid ) );
bool chess_is_valid_move	args( ( char *cstr, OBJ_DATA *board, int x1,
					int y1, int x2, int y2,
					CHAR_DATA *ch, bool do_out,
					bool checkvalid ) );

void do_chess(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *board = NULL, *obj;
    AFFECT_DATA *paf, *sdata = NULL;
    CHAR_DATA *p1 = NULL, *p2 = NULL;
    CHAR_DATA *vch;
    DESCRIPTOR_DATA *d;
    char *cstr;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_STRING_LENGTH];
    int i, j;

    for (obj = ch->in_room->contents; obj; obj = obj->next_content)
	if ((obj->item_type == ITEM_SPECIAL) && (obj->value[0] == SOBJ_CHESS))
	{
	    board = obj;
	    break;
	}
	
    if (!board)
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Usage: chess <command>\n\r\n\r", ch);
        send_to_char("Available commands:\n\r", ch);
        send_to_char("showboard move play leave reset\n\r", ch);
	send_to_char("autoshow colour takeback\n\r", ch);
	if (ch->trust > LEVEL_HERO)
	{
            send_to_char("\n\rImmortal commands:\n\r", ch);
	    send_to_char("set turnswitch\n\r", ch);
	}
	return;
    }

    for (paf = board->affected; paf; paf = paf->next)
	if (paf->point && (paf->type == gsn_stringdata))
	{
	    sdata = paf;
	    break;
	}

    if (!sdata)
    {
	bug("Error: No stringdata located for chess board.", 0);
	return;
    }

    cstr = (char *) sdata->point;

    for (d = descriptor_list; d; d = d->next)
    {
	if (d->connected == CON_PLAYING)
	{
	    if (d->character->id == board->objvalue[0])
		p1 = d->character;

	    if (d->character->id == board->objvalue[1])
		p2 = d->character;
	}
    }

    if (!p1 || !p2)
	for (vch = char_list; vch; vch = vch->next)
	{
	    if (vch->id == board->objvalue[0])
		p1 = vch;
	    else if (vch->id == board->objvalue[1])
		p2 = vch;

	    if (p1 && p2)
		break;
	}

    if (!str_prefix(arg, "play"))
    {
	if ((board->objvalue[0] == ch->id) || (board->objvalue[1] == ch->id))
	{
	    send_to_char("You are already playing in this game of chess.\n\r", ch);
	    return;
	}

	if (!p1 || (p2 && (p1->in_room != ch->in_room)))
	{
	    send_to_char("You sit down in front of the white pieces, and prepare to play.\n\r", ch);
	    act("$n sits down in front of the white pieces, and prepares to play.", ch, NULL, NULL, TO_ROOM);
	    board->objvalue[0] = ch->id;
	    return;
	}

	if (!p2 || (p2->in_room != ch->in_room))
	{
	    send_to_char("You sit down in front of the black pieces, and prepare to play.\n\r", ch);
	    act("$n sits down in front of the black pieces, and prepares to play.", ch, NULL, NULL, TO_ROOM);
	    board->objvalue[1] = ch->id;
	    return;
	}

	sprintf(buf, "%s and %s are already using this chessboard.\n\r", PERS(p1, ch), PERS(p2, ch));
	send_to_char(buf, ch);
	return;
    }	

    if (!str_prefix(arg, "move"))
    {
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
        char ptype, tchar;
	int x1, x2, y1, y2;
	int ploc;
	int kingloc = -1;
	bool en_passant = FALSE;
	bool checkmate = TRUE;
	bool incl_king;
	bool dblchk = FALSE;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if ((arg1[0] == '\0') || (arg1[1] == '\0') || (arg2[0] == '\0') || (arg2[1] == '\0'))
	{
	    send_to_char("Usage: chess move <co-ord 1> <co-ord 2>\n\r", ch);
	    return;
	}

	if ((board->objvalue[0] != ch->id) && (board->objvalue[1] != ch->id))
	{
	    send_to_char("You are not playing in this game of chess.\n\r", ch);
	    return;
	}

	if (board->objvalue[board->objvalue[2]] != ch->id)
	{
	    send_to_char("It is not your turn to move.\n\r", ch);
	    return;
	}

	x1 = (LOWER(arg1[0]) - 'a');
	x2 = (LOWER(arg2[0]) - 'a');
	y1 = arg1[1] - '0' - 1;
	y2 = arg2[1] - '0' - 1;

	if ((x1 < 0) || (x1 > 7) || (x2 < 0) || (x2 > 7)
	 || (y1 < 0) || (y1 > 7) || (y2 < 0) || (y2 > 7))
	{
	    send_to_char("Improper square reference.\n\r", ch);
	    return;
	}

	ptype = get_chess_char(cstr, x1, y1);

        if ((ptype == ' ') || ((LOWER(ptype) == ptype) && (board->objvalue[0] == ch->id))
         || ((UPPER(ptype) == ptype) && (board->objvalue[1] == ch->id)))
        {
            send_to_char("Invalid first reference.\n\r", ch);
            return;
        }
	
	if (!chess_is_valid_move(cstr, board, x1, y1, x2, y2, ch, TRUE, TRUE))
	    return;

	tchar = get_chess_char(cstr, x2, y2);

	if ((UPPER(ptype) == 'P') && (tchar == ' ') && (abs(x2 - x1) == 1)) /* en passant */
	    en_passant = TRUE;

	set_chess_char(cstr, x1, y1, ' ');
	set_chess_char(cstr, x2, y2, ptype);
	if (en_passant)
	{
	    if (ptype == 'p')
	        set_chess_char(cstr, x2, y2 + 1, ' ');
	    else
		set_chess_char(cstr, x2, y2 - 1, ' ');
	}

	if (tchar != ' ')
	{

	    sprintf(buf, "You move your %s from %c%d to %c%d, capturing %s's %s.",
		chess_char_to_name(ptype),
		(x1 + 'a'), y1 + 1, (x2 + 'a'), y2 + 1,
		((ch == p2) ? (p1 ? p1->name : "{Wwhite{x") : (p2 ? p2->name : "{Dblack{x")),
		chess_char_to_name(tchar));
	    act(buf, ch, NULL, (ch == p2) ? p1 : p2, TO_CHAR);
	    sprintf(buf, "$n moves $s %s from %c%d to %c%d, capturing %s's %s.",
		chess_char_to_name(ptype),
		(x1 + 'a'), y1 + 1, (x2 + 'a'), y2 + 1,
		((ch == p2) ? (p1 ? p1->name : "{Wwhite{x") : (p2 ? p2->name : "{Dblack{x")),
		chess_char_to_name(tchar));
	    act(buf, ch, NULL, (ch == p2) ? p1 : p2, TO_NOTVICT);
	    if ((ch == p2) ? p1 : p2)
	    {
	        sprintf(buf, "$n moves $s %s from %c%d to %c%d, capturing your %s.",
		    chess_char_to_name(ptype),
		    (x1 + 'a'), y1 + 1, (x2 + 'a'), y2 + 1,
		    chess_char_to_name(tchar));
	        act(buf, ch, NULL, (ch == p2) ? p1 : p2, TO_VICT);
	    }

	    if (UPPER(tchar) == 'K')
	    {
		act("You have won the game!", ch, NULL, NULL, TO_CHAR);
		act("$n has won the game!", ch, NULL, NULL, TO_ROOM);
	    }

	}
	else if ((UPPER(ptype) == 'K') && (abs(x1 - x2) == 2))
	{
	    set_chess_char(cstr, (x1 > x2) ? 2 : 4, y2, (char) ((ptype == 'k') ? 'r' : 'R'));
	    set_chess_char(cstr, (x1 > x2) ? 0 : 7, y2, ' ');

	    sprintf(buf, "You castle to the %s, switching your king and rook.\n\r",
		(x1 > x2) ? "left" : "right");
	    send_to_char(buf, ch);
	    sprintf(buf, "$n castles to the %s, switching $s king and rook.",
		(x1 > x2) ? "left" : "right");
	    act(buf, ch, NULL, NULL, TO_ROOM);

	}
	else if ((UPPER(ptype) == 'P') && (tchar == ' ') && (abs(x2 - x1) == 1)) /* en passant */
	{
	    sprintf(buf, "You move your pawn from %c%d to %c%d, capturing %s's pawn en passant.",
		(x1 + 'a'), y1 + 1, (x2 + 'a'), y2 + 1,
		((ch == p2) ? (p1 ? p1->name : "{Wwhite{x") : (p2 ? p2->name : "{Dblack{x")));
	    act(buf, ch, NULL, (ch == p2) ? p1 : p2, TO_CHAR);
	    sprintf(buf, "$n moves $s pawn from %c%d to %c%d, capturing %s's pawn en passant.",
		(x1 + 'a'), y1 + 1, (x2 + 'a'), y2 + 1,
		((ch == p2) ? (p1 ? p1->name : "{Wwhite{x") : (p2 ? p2->name : "{Dblack{x")));
	    act(buf, ch, NULL, (ch == p2) ? p1 : p2, TO_NOTVICT);
	    if ((ch == p2) ? p1 : p2)
	    {
	        sprintf(buf, "$n moves $s pawn from %c%d to %c%d, capturing your pawn en passant.",
		    (x1 + 'a'), y1 + 1, (x2 + 'a'), y2 + 1);
	        act(buf, ch, NULL, (ch == p2) ? p1 : p2, TO_VICT);
	    }
	}
	else
	{
	    sprintf(buf, "You move your %s from %c%d to %c%d.",
		chess_char_to_name(ptype),
		(x1 + 'a'), y1 + 1, (x2 + 'a'), y2 + 1);
	    act(buf, ch, NULL, (ch == p2) ? p1 : p2, TO_CHAR);
	    sprintf(buf, "$n moves $s %s from %c%d to %c%d.",
		chess_char_to_name(ptype),
		(x1 + 'a'), y1 + 1, (x2 + 'a'), y2 + 1);
	    act(buf, ch, NULL, (ch == p2) ? p1 : p2, TO_ROOM);
	}

	if ((ptype == 'p') && (y2 == 0))
	{
	    send_to_char("You remove your pawn from the board, and replace it with a queen.\n\r", ch);
	    act("$n removes $s pawn from the board, replacing it with a queen.", ch, NULL, NULL, TO_ROOM);
	    set_chess_char(cstr, x2, y2, 'q');
	}
	else if ((ptype == 'P') && (y2 == 7))
	{
	    send_to_char("You remove your pawn from the board, and replace it with a queen.\n\r", ch);
	    act("$n removes $s pawn from the board, replacing it with a queen.", ch, NULL, NULL, TO_ROOM);
	    set_chess_char(cstr, x2, y2, 'Q');
	}

	board->objvalue[2] = abs(board->objvalue[2] - 1);
	board->objvalue[3] = (y2 * 8) + x2;
	board->objvalue[4] = (y1 * 8) + x1;
	REMOVE_BIT(board->objvalue[5], CHESS_NOTAKEBACK);
	board->objvalue[6] = tchar;

	if (ptype == 'k')
	    board->objvalue[5] |= CHESS_NOBLACKLEFT|CHESS_NOBLACKRIGHT;
	else if (ptype == 'K')
	    board->objvalue[5] |= CHESS_NOWHITELEFT|CHESS_NOWHITERIGHT;
	else if (ptype == 'r')
	{
	    if (x1 == 0)
		board->objvalue[5] |= CHESS_NOBLACKLEFT;
	    else if (x1 == 7)
		board->objvalue[5] |= CHESS_NOBLACKRIGHT;
	}
	else if (ptype == 'R')
	{
	    if (x1 == 0)
		board->objvalue[5] |= CHESS_NOWHITELEFT;
	    else if (x1 == 7)
		board->objvalue[5] |= CHESS_NOWHITERIGHT;
	}

        REMOVE_BIT(board->objvalue[5], CHESS_WHITECHECK|CHESS_BLACKCHECK);
	kingloc = -1;

	for (i = 0; i < 64; i++)
	    if ((UPPER(cstr[i]) == 'K') && ((UPPER(ptype) == ptype) != (UPPER(cstr[i]) == cstr[i])))
	    {
		kingloc = i;
		break;
	    }

        if (kingloc != -1)
	    if ((ploc = chess_can_any_reach(cstr, board, kingloc % 8, kingloc / 8, (bool) ((cstr[kingloc] == 'k') ? TRUE : FALSE), TRUE, FALSE)) >= 0)
	    {
		for (i = (ploc + 1); i < 64; i++)
        	    if ((cstr[i] != ' ') && ((UPPER(cstr[i]) == cstr[i]) == (cstr[kingloc] == 'k')))
	    		if (chess_is_valid_move(cstr, board, (i % 8), (i / 8), (kingloc % 8), (kingloc / 8), ch, FALSE, FALSE))
			{
			    dblchk = TRUE;
			    break;
			}

		x1 = ploc % 8;
		y1 = ploc / 8;
		x2 = kingloc % 8;
		y2 = kingloc / 8;


		if (!dblchk)
		{
		    if (chess_can_any_reach(cstr, board, x1, y1, (bool) ((cstr[kingloc] == 'k') ? TRUE : FALSE), TRUE, FALSE) >= 0)
		        incl_king = FALSE;
		    else
		        incl_king = TRUE;

		    switch (UPPER(cstr[ploc]))
		    {
		        case 'R':
			    if (chess_can_any_reach(cstr, board, i, y1, (bool) ((cstr[ploc] == 'r') ? TRUE : FALSE), incl_king, TRUE) >= 0)
			        checkmate = FALSE;
			    if (x1 < x2)
			    {
			        for (i = (x1 + 1); i != x2; i++)
				    if (chess_can_any_reach(cstr, board, i, y1, (bool) ((cstr[ploc] == 'r') ? TRUE : FALSE), FALSE, TRUE) >= 0)
				    {
					checkmate = FALSE;
				        break;
				    }
			    }
			    else if (x2 < x1)
			    {
			        for (i = (x1 - 1); i != x2; i--)
				    if (chess_can_any_reach(cstr, board, i, y1, (bool) ((cstr[ploc] == 'r') ? TRUE : FALSE), FALSE, TRUE) >= 0)
				    {
				        checkmate = FALSE;
				        break;
				    }
			    }
			    else if (y1 < y2)
			    {
			        for (i = (y1 + 1); i != y2; i++)
				    if (chess_can_any_reach(cstr, board, x1, i, (bool) ((cstr[ploc] == 'r') ? TRUE : FALSE), FALSE, TRUE) >= 0)
				    {
				        checkmate = FALSE;
				        break;
				    }
			    }
			    else if (y2 < y1)
			    {
			        for (i = (y1 - 1); i != y2; i--)
				    if (chess_can_any_reach(cstr, board, x1, i, (bool) ((cstr[ploc] == 'r') ? TRUE : FALSE), FALSE, TRUE) >= 0)
				    {
				        checkmate = FALSE;
				        break;
				    }
			    }
			    break;

		        case 'N':
			    if (chess_can_any_reach(cstr, board, x1, y2, (bool) ((cstr[ploc] == 'n') ? TRUE : FALSE), incl_king, TRUE) >= 0)
			        checkmate = FALSE;
			    break;

		        case 'B':
			    if (chess_can_any_reach(cstr, board, x1, y1, (bool) ((cstr[ploc] == 'b') ? TRUE : FALSE), incl_king, TRUE) >= 0)
			        checkmate = FALSE;
			    else
			    {
			        for (i = 1; abs(x1 - i) != 0; i++)
			            if (chess_can_any_reach(cstr, board, (x1 < x2) ? (x1 + i) : (x1 - i), (y1 < y2) ? (y1 + i) : (y1 - i), (bool) ((cstr[ploc] == 'b') ? TRUE : FALSE), FALSE, TRUE) >= 0)
			            {
				        checkmate = FALSE;
				        break;
			            }
			    }
			    break;

		        case 'Q':
			    if (chess_can_any_reach(cstr, board, x1, y1, (bool) ((cstr[ploc] == 'q') ? TRUE : FALSE), incl_king, TRUE) >= 0)
			        checkmate = FALSE;
			    else if ((abs(x2 - x1) == 0) || (abs(y2 - y1) == 0))
			    {
			        if (x1 < x2)
			        {
			            for (i = (x1 + 1); i != x2; i++)
				        if (chess_can_any_reach(cstr, board, i, y1, (bool) ((cstr[ploc] == 'q') ? TRUE : FALSE), FALSE, TRUE) >= 0)
				        {
				            checkmate = FALSE; 
				            break;
				        }
			        }
			        else if (x2 < x1)
			        {
			            for (i = (x1 - 1); i != x2; i--)
				        if (chess_can_any_reach(cstr, board, i, y1, (bool) ((cstr[ploc] == 'q') ? TRUE : FALSE), FALSE, TRUE) >= 0)
				        {
				            checkmate = FALSE;
				            break;
				        }
			        }
			        else if (y1 < y2)
			        {
			            for (i = (y1 + 1); i != y2; i++)
				        if (chess_can_any_reach(cstr, board, x1, i, (bool) ((cstr[ploc] == 'q') ? TRUE : FALSE), FALSE, TRUE) >= 0)
				        {
				            checkmate = FALSE;
				            break;
				        }
			        }
			    	else if (y2 < y1)
			    	{
			            for (i = (y1 - 1); i != y2; i--)
				    	if (chess_can_any_reach(cstr, board, x1, i, (bool) ((cstr[ploc] == 'q') ? TRUE : FALSE), FALSE, TRUE) >= 0)
				    	{
				            checkmate = FALSE;
				            break;
				    	}
			        }
			    }
			    else
			    {
			    	for (i = 1; abs(x1 - i) != 0; i++)
			            if (chess_can_any_reach(cstr, board, (x1 < x2) ? (x1 + i) : (x1 - i), (y1 < y2) ? (y1 + i) : (y1 - i), (bool) ((cstr[ploc] == 'q') ? TRUE : FALSE), FALSE, TRUE) >= 0)
			            {
				    	checkmate = FALSE;
				    	break;
			            }
			    }
			    break;

		    	case 'P':
			    if (chess_can_any_reach(cstr, board, x1, y1, (bool) ((cstr[ploc] == 'p') ? TRUE : FALSE), incl_king, TRUE) >= 0)
			        checkmate = FALSE;
			    break;
		    }
		}
		    
		if (checkmate)   /* now try checking king movements */
		{
		    ptype = get_chess_char(cstr, x2, y2);
		    for (x1 = UMAX(x2 - 1, 0); x1 <= UMIN(x2 + 1, 7); x1++)
		    {
			for (y1 = UMAX(y2 - 1, 0); y1 <= UMIN(y2 + 1, 7); y1++)
			{
			    tchar = get_chess_char(cstr, x1, y1);
			    if ((tchar == ' ') || ((UPPER(tchar) == tchar) != (UPPER(ptype) == ptype)))
			    {
			        set_chess_char(cstr, x1, y1, ptype);
			        set_chess_char(cstr, x2, y2, ' ');
			        if (chess_can_any_reach(cstr, board, x1, y1, (bool) ((ptype == 'k') ? TRUE : FALSE), TRUE, FALSE) == -1)
				    checkmate = FALSE;
			        set_chess_char(cstr, x2, y2, get_chess_char(cstr, x1, y1));
			        set_chess_char(cstr, x1, y1, tchar);
			    }
			    if (!checkmate)
				break;
			}
			if (!checkmate)
			    break;
		    }
		}

		if (checkmate)
		{
		    send_to_char("CHECKMATE!  You have won the match!\n\r", ch);
		    sprintf(buf, "CHECKMATE!  $n ({%s{x) has won the match!",
			(ch == p1) ? "Wwhite" : "Dblack");
		    act(buf, ch, NULL, NULL, TO_ROOM);
		}
		else
		{
		    sprintf(buf, "{%s{x king is in CHECK!", (cstr[kingloc] == 'k') ? "DBlack" : "WWhite");
		    act(buf, ch, NULL, NULL, TO_CHAR);
		    act(buf, ch, NULL, NULL, TO_ROOM);
		    if (cstr[kingloc] == 'k')
		        board->objvalue[5] |= CHESS_BLACKCHECK;
		    else
		        board->objvalue[5] |= CHESS_WHITECHECK;
		}
	    }

        if (p1 && p2 && (p1->in_room == p2->in_room) &&
	 IS_SET((ch == p1) ? p2->extra_flags : p1->extra_flags, CHESS_AUTOSHOW))
	    do_chess((ch == p1) ? p2 : p1, "showboard");

	return;
    }

    if (!str_prefix(arg, "reset"))
    {
	if ((ch != p1) && (ch != p2))
	{
	    send_to_char("You cannot reset a chess game you're not participating in.\n\r", ch);
	    return;
	}

	send_to_char("You return all the pieces on the chess board to their original locations.\n\r", ch);
	act("$n returns all the pieces on the chess board to their original locations.", ch, NULL, NULL, TO_ROOM);

	board->objvalue[2] = 0;
	board->objvalue[3] = 0;
        board->objvalue[4] = 0;
	board->objvalue[5] = 0;
	
	if (sdata->point)
	{
		char * temp(static_cast<char*>(sdata->point));
	    free_string(temp);
		sdata->point = 0;
	}

	sdata->point = (void *) str_dup("RNBKQBNRPPPPPPPP                                pppppppprnbkqbnr");
	return;
    }

    if (!str_prefix(arg, "takeback"))
    {
	if ((board->objvalue[0] != ch->id) && (board->objvalue[1] != ch->id))
	{
	    send_to_char("You are not playing in this game of chess.\n\r", ch);
	    return;
	}

	if (board->objvalue[board->objvalue[2]] != ch->id)
	{
	    send_to_char("It is not your turn to move.\n\r", ch);
	    return;
	}

	if (IS_SET(board->objvalue[5], CHESS_NOTAKEBACK))
	{
	    send_to_char("Only one move may be taken back.\n\r", ch);
	    return;
	}

	if ((UPPER(cstr[board->objvalue[3]]) == 'K') && (abs(board->objvalue[4] - board->objvalue[3]) == 2))
	{
	    cstr[board->objvalue[4]] = cstr[board->objvalue[3]];
	    cstr[(board->objvalue[3] > board->objvalue[4]) ? (board->objvalue[3] + 1) : (board->objvalue[3] - 2)] = (cstr[board->objvalue[3]] == 'k') ? 'r' : 'R';
	    cstr[board->objvalue[3]] = ' ';
	    cstr[(board->objvalue[3] > board->objvalue[4]) ? (board->objvalue[3] - 1) : (board->objvalue[3] + 1)] = ' ';

	    if (cstr[board->objvalue[4]] == 'k')
	    {
		if (board->objvalue[3] > board->objvalue[4])
		    REMOVE_BIT(board->objvalue[5], CHESS_NOBLACKRIGHT);
		else
		    REMOVE_BIT(board->objvalue[5], CHESS_NOBLACKLEFT);
	    }
	    else
	    {
		if (board->objvalue[3] > board->objvalue[4])
		    REMOVE_BIT(board->objvalue[5], CHESS_NOWHITERIGHT);
		else
		    REMOVE_BIT(board->objvalue[5], CHESS_NOWHITELEFT);
	    }		
	}
	else if ((UPPER(cstr[board->objvalue[3]]) == 'P') && (board->objvalue[6] == ' ') && ((abs(board->objvalue[3] - board->objvalue[4]) == 7) || (abs(board->objvalue[3] - board->objvalue[4]) == 9)))
	{
	    cstr[board->objvalue[4]] = cstr[board->objvalue[3]];
	    cstr[board->objvalue[3]] = ' ';
	    if (cstr[board->objvalue[4]] == 'p')
	    {
		if ((board->objvalue[4] - board->objvalue[3]) == 9)
	       {
		    cstr[board->objvalue[4] - 1] = 'P';
		    board->objvalue[3] = (board->objvalue[4] - 1);
		    board->objvalue[4] = (board->objvalue[4] - 17);
		}
		else
		{
		    cstr[board->objvalue[4] + 1] = 'P';
		    board->objvalue[3] = (board->objvalue[4] + 1);
		    board->objvalue[4] = (board->objvalue[4] - 15);
		}
	    }
	    else
	    {
		if ((board->objvalue[3] - board->objvalue[4]) == 9)
		{
		    cstr[board->objvalue[3] + 1] = 'p';
		    board->objvalue[3] = (board->objvalue[4] + 1);
		    board->objvalue[4] = (board->objvalue[4] + 17);
		}
		else
		{
		    cstr[board->objvalue[3] - 1] = 'p';
		    board->objvalue[3] = (board->objvalue[4] - 1);
		    board->objvalue[4] = (board->objvalue[4] + 15);
		}
	   }
	}
	else
	{
	    cstr[board->objvalue[4]] = cstr[board->objvalue[3]];
	    cstr[board->objvalue[3]] = board->objvalue[6];
	}

	send_to_char("You take back your opponent's previous move.\n\r", ch);
	act("$n takes back $s opponent's previous move.", ch, NULL, NULL, TO_NOTVICT);
	if ((ch == p2) ? p1 : p2)
	    act("$n takes back your previous move.", ch, NULL, (ch == p2) ? p1 : p2, TO_VICT);

	board->objvalue[5] |= CHESS_NOTAKEBACK;
	board->objvalue[2] = abs(board->objvalue[2] - 1);

       if (p1 && p2 && (p1->in_room == p2->in_room) &&
	 IS_SET((ch == p1) ? p2->extra_flags : p1->extra_flags, CHESS_AUTOSHOW))
	    do_chess((ch == p1) ? p2 : p1, "showboard");

	return;
    }

    if (!str_prefix(arg, "leave"))
    {
	if ((ch != p1) && (ch != p2))
	{
	    send_to_char("You are not participating in a game of chess.\n\r", ch);
	    return;
	}

	if (ch == p1)
	    board->objvalue[0] = 0;
	else
	    board->objvalue[1] = 0;

	send_to_char("You stop playing in the current game of chess.\n\r", ch);
	act("$n stands up and stops playing the current game of chess.", ch, NULL, NULL, TO_ROOM);

	return;
    }

    if (!str_prefix(arg, "showboard"))
    {
	char *cmark = cstr;
        
	send_to_char("    a   b   c   d   e   f   g   h\n\r", ch);
        for (i = 0; i < 8; i++)
        {
	    send_to_char("  ---------------------------------", ch);
	    if (i == 1)
		send_to_char("    {WWhite:{x\n\r", ch);
	    else if (i == 3)
	    {
		sprintf(buf, "    %s%s\n\r",
		    p2 ? p2->name : "Empty",
		    IS_SET(board->objvalue[5], CHESS_BLACKCHECK) ? " (in check)" : "");
		send_to_char(buf, ch);
	    }
	    else
		send_to_char("\n\r", ch);
	    sprintf(buf, "%d ", i+1);
	    send_to_char(buf, ch);
	    for (j = 0; j < 8; j++)
	    {
		if (IS_SET(ch->extra_flags, CHESS_NOCOLOUR))
		    sprintf(buf, "| %c ", *cmark);
		else
		    sprintf(buf, "| {%s%c{x ", (*cmark < 'a' ? "W" : "D"), UPPER(*cmark));
		send_to_char(buf, ch);
		cmark++;
	    }
	    if (i == 1)
	    {
		sprintf(buf, "|    %s%s\n\r",
		    p1 ? p1->name : "Empty",
		    IS_SET(board->objvalue[5], CHESS_WHITECHECK) ? " (in check)" : "");
		send_to_char(buf, ch);
	    }
	    else if (i == 2)
	 	send_to_char("|    {DBlack:{x\n\r", ch);
	    else
		send_to_char("|\n\r", ch);
	}
	send_to_char("  ---------------------------------\n\r\n\r", ch);
        if (((board->objvalue[2] == 1) && !p2)
         || ((board->objvalue[2] == 0) && !p1))
            sprintf(buf, "It is currently {%s{x's turn to move.\n\r",
                (board->objvalue[2] ? "Dblack" : "Wwhite"));
        else
            sprintf(buf, "It is currently %s's ({%s{x) turn.\n\r",
                (board->objvalue[2] ? p2->name : p1->name),
                (board->objvalue[2] ? "Dblack" : "Wwhite"));
        send_to_char(buf, ch);
	return;
    }

    if (!str_prefix(arg, "color") || !str_prefix(arg, "colour"))
    {
	ch->extra_flags ^= CHESS_NOCOLOUR;
	
	if (IS_SET(ch->extra_flags, CHESS_NOCOLOUR))
	    send_to_char("You will no longer see chess in colour.\n\r", ch);
	else
	    send_to_char("You will now see chess in colour.\n\r", ch);
	
	return;
    }

    if (!str_prefix(arg, "autoshow"))
    {
	ch->extra_flags ^= CHESS_AUTOSHOW;

	if (IS_SET(ch->extra_flags, CHESS_AUTOSHOW))
	    send_to_char("You will now see a board update when your opponent moves.\n\r", ch);
	else
	    send_to_char("You will no longer see a board update when your opponent moves.\n\r", ch);

	return;
    }

    if (IS_NPC(ch))
    {
	if (!str_prefix(arg, "set"))
        {
	    char arg1[MAX_INPUT_LENGTH];

	    argument = one_argument(argument, arg1, true);

	    for (i = 0; i < 64; i++)
	        if (arg1[i] == '\0')
		    return;

	    if (arg1[64] != '\0')
	        return;

	    if (sdata->point)
		{
			char * temp(static_cast<char*>(sdata->point));
	        free_string(temp);
			sdata->point = 0;
		}

	    sdata->point = (void *) str_dup(arg1);

 	    return;
	}
    }

    if (ch->trust > LEVEL_HERO)
    {
	if (!str_prefix(arg, "set"))
        {
	    char arg1[MAX_INPUT_LENGTH];

	    argument = one_argument(argument, arg1, true);

	    for (i = 0; i < 64; i++)
	        if (arg1[i] == '\0')
	        {
		    send_to_char("Invalid set string.\n\r", ch);
		    return;
	        }  

	    if (arg1[64] != '\0')
	    {
	        send_to_char("Invalid set string.\n\r", ch);
	        return;
	    }

	    if (sdata->point)
		{
			char * temp(static_cast<char*>(sdata->point));
	        free_string(temp);
			sdata->point = 0;
		}

	    sdata->point = (void *) str_dup(arg1);
	    send_to_char("Chess board rearranged.\n\r", ch);

 	    return;
	}

	if (!str_prefix(arg, "turnswitch"))
	{
	    board->objvalue[2] = abs(board->objvalue[2] - 1);
	    send_to_char("You switch the current turn.\n\r", ch);
	    sprintf(buf, "It is now {%s{x's turn.",
		(board->objvalue[2] ? "Dblack" : "Wwhite"));
	    act(buf, ch, NULL, NULL, TO_ROOM);
	    return;
	}
    }

    send_to_char("Usage: chess <command>\n\r\n\r", ch);
    send_to_char("Available commands:\n\r", ch);
    send_to_char("showboard move play leave reset\n\r", ch);
    send_to_char("autoshow colour takeback\n\r", ch);
    if (ch->trust > LEVEL_HERO)
    {
        send_to_char("\n\rImmortal commands:\n\r", ch);
	send_to_char("set turnswitch\n\r", ch);
    }

    return;
}

bool chess_is_valid_move(char *cstr, OBJ_DATA *board, int x1, int y1, int x2, int y2, CHAR_DATA *ch, bool do_out, bool checkvalid)
{
    char buf[MAX_STRING_LENGTH];
    char ptype = get_chess_char(cstr, x1, y1);
    char tchar;
    int i;

    switch (UPPER(ptype))
    {
        case 'R':
     	    if ((x1 - x2) && (y1 - y2))
	    {
		if (do_out)
	            send_to_char("Invalid rook move.\n\r", ch);
	        return FALSE;
	    }

	    if (x1 > x2)
	        for (i = (x1 - 1); i != x2; i--)
		    if ((tchar = get_chess_char(cstr, i, y1)) != ' ')
		    {
			if (do_out)
			{
		            sprintf(buf, "Invalid move.  Blocked by %s at %c%d.\n\r",
		    	    chess_char_to_name(tchar), (i + 'a'), y1 + 1);
			    send_to_char(buf, ch);
			}
		        return FALSE;
		    }

	    if (x1 < x2)
	        for (i = (x1 + 1); i != x2; i++)
		    if ((tchar = get_chess_char(cstr, i, y1)) != ' ')
		    {
			if (do_out)
			{
			    sprintf(buf, "Invalid move.  Blocked by %s at %c%d.\n\r",
				chess_char_to_name(tchar), (i + 'a'), y1 + 1);
			    send_to_char(buf, ch);
			}
			return FALSE;
		    }

	    if (y1 > y2)
		for (i = (y1 - 1); i != y2; i--)
		    if ((tchar = get_chess_char(cstr, x1, i)) != ' ')
		    {
			if (do_out)
			{
			    sprintf(buf, "Invalid move.  Blocked by %s at %c%d.\n\r",
				chess_char_to_name(tchar), (x1 + 'a'), i + 1);
			    send_to_char(buf, ch);
			}
			return FALSE;
		    }

	    if (y1 < y2)
	        for (i = (y1 + 1); i != y2; i++)
		    if ((tchar = get_chess_char(cstr, x1, i)) != ' ')
		    {
			if (do_out)
			{
		            sprintf(buf, "Invalid move.  Blocked by %s at %c%d.\n\r",
				chess_char_to_name(tchar), (x1 + 'a'), i + 1);
			    send_to_char(buf, ch);
			}
			return FALSE;
		    }
	    break;

	case 'N':
	    if (!(((abs(x1 - x2) == 2) && (abs(y1 - y2) == 1))
	     || ((abs(y1 - y2) == 2) && (abs(x1 - x2) == 1))))
	    {
		if (do_out)
	            send_to_char("Invalid knight move.\n\r", ch);
	        return FALSE;
	    }
	    break;

	case 'B':
	    {
	    bool xup = TRUE, yup = TRUE;

	    if (abs(x1 - x2) != abs(y1 - y2))
	    {
	        if (do_out)
		    send_to_char("Invalid bishop move.\n\r", ch);
	        return FALSE;
	    }

	    if (x1 < x2)
		xup = TRUE;
	    else
		xup = FALSE;

	    if (y1 < y2)
		yup = TRUE;
	    else
		yup = FALSE;

	    for (i = 1; i != abs(x1 - x2); i++)
		if ((tchar = get_chess_char(cstr, xup ? (x1 + i) : (x1 - i), yup ? (y1 + i) : (y1 - i))) != ' ')
		{
		    if (do_out)
		    {
			sprintf(buf, "Invalid move.  Blocked by %s at %c%d.\n\r",
			    chess_char_to_name(tchar),
			    ((xup ? (x1 + i) : (x1 - i)) + 'a'),
			    (yup ? (y1 + i + 1) : (y1 - i + 1)));
			send_to_char(buf, ch);
		    }
		    return FALSE;
		}
	    }
	    break;			    			

	case 'Q':
	    if ((x1 - x2) && (y1 - y2) && (abs(x1 - x2) != abs(y1 - y2)))
	    {
		if (do_out)
		    send_to_char("Invalid queen move.\n\r", ch);
		return FALSE;
	    }

	    if (abs(x1 - x2) == abs(y1 - y2))  /* bishop move */
	    {
		bool xup = TRUE, yup = TRUE;

		if (x1 < x2)
		    xup = TRUE;
		else
		    xup = FALSE;

		if (y1 < y2)
		    yup = TRUE;
		else
		    yup = FALSE;

		for (i = 1; i != abs(x1 - x2); i++)
		    if ((tchar = get_chess_char(cstr, xup ? (x1 + i) : (x1 - i), yup ? (y1 + i) : (y1 - i))) != ' ')
		    {
			if (do_out)
			{
			    sprintf(buf, "Invalid move.  Blocked by %s at %c%d.\n\r",
			        chess_char_to_name(tchar),
			        ((xup ? (x1 + i) : (x1 - i)) + 'a'),
			        (yup ? (y1 + i + 1) : (y1 - i + 1)));
			    send_to_char(buf, ch);
			}
			return FALSE;
		    }
	    }
	    else /* rook move */
	    {
		if (x1 > x2)
		    for (i = (x1 - 1); i != x2; i--)
			if ((tchar = get_chess_char(cstr, i, y1)) != ' ')
			{
			    if (do_out)
			    {
			        sprintf(buf, "Invalid move.  Blocked by %s at %c%d.\n\r",
				    chess_char_to_name(tchar), (i + 'a'), y1 + 1);
			        send_to_char(buf, ch);
			    }
			    return FALSE;
			}

		if (x1 < x2)
		    for (i = (x1 + 1); i != x2; i++)
			if ((tchar = get_chess_char(cstr, i, y1)) != ' ')
			{
			    if (do_out)
			    {
			        sprintf(buf, "Invalid move.  Blocked by %s at %c%d.\n\r",
				    chess_char_to_name(tchar), (i + 'a'), y1 + 1);
			        send_to_char(buf, ch);
			    }
		            return FALSE;
			}

		if (y1 > y2)
		    for (i = (y1 - 1); i != y2; i--)
			if ((tchar = get_chess_char(cstr, x1, i)) != ' ')
			{
			    if (do_out)
			    {
			        sprintf(buf, "Invalid move.  Blocked by %s at %c%d.\n\r",
				    chess_char_to_name(tchar), (x1 + 'a'), i + 1);
			        send_to_char(buf, ch);
			    }
			    return FALSE;
			}

		if (y1 < y2)
		    for (i = (y1 + 1); i != y2; i++)
			if ((tchar = get_chess_char(cstr, x1, i)) != ' ')
			{  
			    if (do_out)
			    {
			        sprintf(buf, "Invalid move.  Blocked by %s at %c%d.\n\r",
				    chess_char_to_name(tchar), (x1 + 'a'), i + 1);
			        send_to_char(buf, ch);
			    }
			    return FALSE;
		        } 
	    }

	    break;
	
	case 'K':
	    if ((abs(x1 - x2) > 2) || (abs(y1 - y2) > 1)
             || ((abs(x1 - x2) == 2) 
              && ((x1 != 3)
               || (((x1 > x2) && (((ptype == 'k') && (IS_SET(board->objvalue[5], CHESS_NOBLACKLEFT) || (get_chess_char(cstr, 0, 7) != 'r') || (y1 != 7))) || ((ptype == 'K') && (IS_SET(board->objvalue[5], CHESS_NOWHITELEFT) || (get_chess_char(cstr, 0, 0) != 'R') || (y1 != 0)))))
                || ((x1 < x2) && (((ptype == 'k') && (IS_SET(board->objvalue[5],CHESS_NOBLACKRIGHT) || (get_chess_char(cstr, 7, 7) != 'r') || (y1 != 7))) || ((ptype == 'K') && (IS_SET(board->objvalue[5],CHESS_NOWHITERIGHT) || (get_chess_char(cstr, 7, 0) != 'R') || (y1 != 0)))))))))
	    {
		if (do_out)
		    send_to_char("Invalid king move.\n\r", ch);
		return FALSE;
	    }

	    if (abs(x1 - x2) == 2)
	    {
		if ((ptype == 'k') ? IS_SET(board->objvalue[5], CHESS_BLACKCHECK) : IS_SET(board->objvalue[5], CHESS_WHITECHECK))
		{
		    if (do_out)
			send_to_char("Invalid move.  Cannot castle while king is in check.\n\r", ch);
		    return FALSE;
		}

	        if (x1 > x2)
		{
		    for (i = 2; i > 0; i--)
			if ((tchar = get_chess_char(cstr, i, y1)) != ' ')
			{
			    if (do_out)
			    {
			        sprintf(buf, "Invalid move.  Blocked by %s at %c%d.\n\r",
				    chess_char_to_name(tchar),
				    (i + 'a'), y1 + 1);
			        send_to_char(buf, ch);
			    }
			    return FALSE;
			}
		}
		else
		{
		    for (i = 4; i < 7; i++)
			if ((tchar = get_chess_char(cstr, i, y1)) != ' ')
			{
			    if (do_out)
			    {
				sprintf(buf, "Invalid move.  Blocked by %s at %c%d.\n\r",
				    chess_char_to_name(tchar),
				    (i + 'a'), y1 + 1);
				send_to_char(buf, ch);
			    }
			    return FALSE;
			}
		}
	    }
				    
	    break;

	case 'P':
	    tchar = get_chess_char(cstr, x2, y2);
	    if ((abs(x1 - x2) > 1)
             || ((ptype == 'P') && (y2 < y1))
	     || ((ptype == 'p') && (y2 > y1))
	     || ((abs(x2 - x1) == 1) && ((abs(y2 - y1) != 1)|| ((tchar == ' ')
              && (((ptype == 'p') && ((board->objvalue[3] != ((y2 * 8) + x2 + 8)) || (cstr[board->objvalue[3]] != 'P') || ((board->objvalue[3] - board->objvalue[4]) != 16)))
               || ((ptype == 'P') && ((board->objvalue[3] != ((y2 * 8) + x2 - 8)) || (cstr[board->objvalue[3]] != 'p') || ((board->objvalue[4] - board->objvalue[3]) != 16)))))
              || ((tchar != ' ') && ((UPPER(tchar) == tchar) == (UPPER(ptype) == ptype)))))
             || (abs(y1 - y2) > 2)
	     || ((abs(y1 - y2) == 2) && (((ptype == 'p') && (y1 != 6)) || ((ptype == 'P') && (y1 != 1)))))
	    {
		if (do_out)
		    send_to_char("Invalid pawn move.\n\r", ch);
		return FALSE;
	    }

	    if (abs(x1 - x2) == 0)
	        if (y1 > y2)
		{
		    for (i = 1; i <= abs(y1 - y2); i++)
			if ((tchar = get_chess_char(cstr, x1, y1 - i)) != ' ')
			{
			    if (do_out)
			    {
			        sprintf(buf, "Invalid move.  Blocked by %s at %c%d.\n\r",
				    chess_char_to_name(tchar),
				    (x1 + 'a'), (y1 - i + 1));
			        send_to_char(buf, ch);
			    }
			    return FALSE;
		        }
		}
		else
		{
		    for (i = 1; i <= abs(y1 - y2); i++)
			if ((tchar = get_chess_char(cstr, x1, y1 + i)) != ' ')
			{
			    if (do_out)
			    {
			        sprintf(buf, "Invalid move.  Blocked by %s at %c%d.\n\r",
				    chess_char_to_name(tchar),
				    (x1 + 'a'), (y1 + i + 1));
			        send_to_char(buf, ch);
			    }
			    return FALSE;
		        }
		}
			
	    break;		

    }

    tchar = get_chess_char(cstr, x2, y2);

    if ((tchar != ' ') && ((UPPER(ptype) == ptype) == (UPPER(tchar) == tchar)))
    {
	if (do_out)
	    send_to_char("Invalid move.  Cannot capture own piece.\n\r", ch);
	return FALSE;
    }

    if (checkvalid)
    {
	int en_passant = FALSE;
	int kingloc = -1;

	if ((UPPER(ptype) == 'P') && (tchar == ' ') && (abs(x2 - x1) == 1)) /* en passant */
	    en_passant = TRUE;

	/* Step 0.5: Special castling check */

	if ((UPPER(ptype) == 'K') && (abs(x1 - x2) == 2))
	{
	    if (x1 > x2)
		set_chess_char(cstr, x1 - 1, y1, ptype);
	    else
		set_chess_char(cstr, x1 + 1, y1, ptype);
	    set_chess_char(cstr, x1, y1, ' ');

	    if (chess_can_any_reach(cstr, board, (x1 > x2) ? (x1 - 1) : (x1 + 1), y1, (bool) ((ptype == 'k') ? TRUE : FALSE), TRUE, FALSE) >= 0)
	    {
		if (do_out)
		    send_to_char("Invalid movement.  King cannot move through square which would place it in check.\n\r", ch);
	        set_chess_char(cstr, x1, y1, ptype);
	        set_chess_char(cstr, (x1 > x2) ? (x1 - 1) : (x1 + 1), y2, ' ');
	        return FALSE; 
	    }

	    if (x1 > x2)
		set_chess_char(cstr, x1 - 1, y1, ' ');
	    else
		set_chess_char(cstr, x1 + 1, y1, ' ');
	    set_chess_char(cstr, x1, y1, ptype);
	}

	set_chess_char(cstr, x1, y1, ' ');
	set_chess_char(cstr, x2, y2, ptype);
	if (en_passant)
	{
	    if (ptype == 'p')
	        set_chess_char(cstr, x2, y2 + 1, ' ');
	    else
		set_chess_char(cstr, x2, y2 - 1, ' ');
	}

	/* Step 2: Check all opposing pieces for access to king */

	for (i = 0; i < 64; i++)
	    if ((UPPER(cstr[i]) == 'K') && ((UPPER(ptype) == ptype) == (UPPER(cstr[i]) == cstr[i])))
	    {
		kingloc = i;
		break;
	    }

        if (kingloc != -1)
	{	    
	    if (chess_can_any_reach(cstr, board, kingloc % 8, kingloc / 8, (bool) ((cstr[kingloc] == 'k') ? TRUE : FALSE), TRUE, FALSE) >= 0)
	    {
		if (do_out)
		    send_to_char("Invalid movement.  Would place king in check position.\n\r", ch);
	        set_chess_char(cstr, x1, y1, ptype);
	        set_chess_char(cstr, x2, y2, tchar);
	        if  (en_passant)
	        {
		    if (ptype == 'p')
		        set_chess_char(cstr, x2, y2 + 1, 'P');
		    else
		        set_chess_char(cstr, x2, y2 - 1, 'P');
	        }
	        return FALSE;
	    }
	}
        set_chess_char(cstr, x1, y1, ptype);
        set_chess_char(cstr, x2, y2, tchar);
        if  (en_passant)
        {
	    if (ptype == 'p')
	        set_chess_char(cstr, x2, y2 + 1, 'P');
	    else
	        set_chess_char(cstr, x2, y2 - 1, 'P');
        }
    }

    return TRUE;
}

int chess_can_any_reach(char *cstr, OBJ_DATA *board, int x, int y, bool white, bool incl_king, bool checkvalid)
{
    int i;
    int loc = (y * 8) + x;

    for (i = 0; i < 64; i++)
        if ((cstr[i] != ' ') && ((UPPER(cstr[i]) == cstr[i]) == white) && (incl_king || (UPPER(cstr[i]) != 'K')))
	    if (chess_is_valid_move(cstr, board, (i % 8), (i / 8), (loc % 8), (loc / 8), NULL, FALSE, checkvalid))
		return i;

    return -1;
}


char get_chess_char(char *cstr, int x, int y)
{
    return cstr[(y * 8) + x];
};

void set_chess_char(char *cstr, int x, int y, char tchar)
{
    cstr[(y * 8) + x] = tchar;
    return;
};

char *chess_char_to_name(char inchar)
{
    switch (UPPER(inchar))
    {
	case 'P':
	    return "pawn";
	case 'R':
	    return "rook";
	case 'N':
	    return "knight";
	case 'B':
	    return "bishop";
	case 'Q':
	    return "queen";
	case 'K':
	    return "king";
    }

    return "(null)";
}
