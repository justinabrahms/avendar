#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#if defined(WIN32)
#include <Winsock2.h>
#else
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
static const size_t INVALID_SOCKET = static_cast<size_t> (-1);
static const size_t SOCKET_ERROR = static_cast<size_t> (-1);
#endif
#include "merc.h"
#include "gameserv.h"

#define GNUM_CTTO		3

#define CTTO_CMD_CREATE_GAME	6
#define CTTO_CMD_LIST_GAMES	7
#define CTTO_CMD_DATA_GLIST	8
#define CTTO_CMD_JOIN_GAME	9
#define CTTO_CMD_VIEW_GAME	10
#define CTTO_CMD_GAME_VIEW	11
#define CTTO_CMD_DRAW_CARD	12
#define CTTO_CMD_PLACE_CARD	13
#define CTTO_CMD_TAKE_PILE	14

#define CTTO_MSG_GAME_CREATED	1
#define CTTO_MSG_HAS_JOINED	2
#define CTTO_MSG_GAME_START	3
#define CTTO_MSG_JOIN		4
#define CTTO_MSG_CARD_DRAWN	5
#define CTTO_MSG_CARD_PLACED	6
#define CTTO_MSG_NEXT_TURN	7
#define CTTO_MSG_ROUND_OVER	8
#define CTTO_MSG_POINTS_SCORED	9
#define CTTO_MSG_PILE_TAKEN	10
#define CTTO_MSG_CARD_GIVEN	11

#define CTTO_ERR_JOIN		4
#define CTTO_ERR_GAME_NOT_FOUND	5
#define CTTO_ERR_NOT_PLAYER	6
#define CTTO_ERR_NOT_TURN	7
#define CTTO_ERR_WRONG_PHASE	8
#define CTTO_ERR_PILES_FULL	9
#define CTTO_ERR_INVALID_PILE	10
#define CTTO_ERR_PILE_FULL	11
#define CTTO_ERR_PILE_GONE	12

#define CTTO_CARD_PINK		1
#define CTTO_CARD_BLUE		2
#define CTTO_CARD_GREEN		3
#define CTTO_CARD_ORANGE	4
#define CTTO_CARD_YELLOW	5
#define CTTO_CARD_GREY		6
#define CTTO_CARD_BROWN		7
#define CTTO_CARD_WILD		8
#define CTTO_CARD_TWO		9

#define CTTO_STATE_WAITING	-1
#define CTTO_STATE_TURN		0
#define CTTO_STATE_GAMEOVER	1
#define CTTO_STATE_PLACE_CARD	2

const char *card_names[10] =
{ "{Dnone{x", "{MPink{x", "{BBlue{x", "{gGreen{x", "{ROrange{x", "{YYellow{x", "{wGrey{x", "{yBrown{x", "{mW{Gi{Ml{Yd{x", "{C2point{x" };

void coloretto_message(unsigned short gnum, const char *txt, CHAR_DATA *ch)
{
    char buf[MAX_STRING_LENGTH];

    sprintf(buf, "{B[{YColoretto:{W%d{B] {w%s{x", gnum, txt);
    send_to_char(buf, ch);

    return;
}

void coloretto_interp(CHAR_DATA *ch, char *cmd)
{
    char buf[CMD_LEN];
    char *p = cmd; // + sizeof(pmsg_len);
    char out_buf[MAX_STRING_LENGTH];
    char *c;
    char name[NAME_LEN];
    unsigned short gnum;

    switch (*p++)
    {
	case CMD_MSG:
	{
	    char msg;
	    char *c;

	    if (!(c = parse_command(p, "%c", &msg)))
		break;

	    switch (msg)
	    {
		case CTTO_MSG_GAME_CREATED:
		{
		    parse_command(c, "%h", &gnum);
		    sprintf(out_buf, "New Coloretto game (#{W%d{w) created.\n\r", gnum);
		    send_to_char(out_buf, ch);
		}
		break;

		case CTTO_MSG_JOIN:
		{
		    parse_command(c, "%h", &gnum);
		    sprintf(out_buf, "You have joined Coloretto game #{W%d{w!\n\r", gnum);
		    send_to_char(out_buf, ch);
		}
		break;

		case CTTO_MSG_HAS_JOINED:
		{
		    parse_command(c, "%h%s", &gnum, name);
		    sprintf(out_buf, "%s has joined Coloretto game #{W%d{w!\n\r", name, gnum);
		    send_to_char(out_buf, ch);
		}
		break;

		case CTTO_MSG_GAME_START:
		{
		    parse_command(c, "%h%s", &gnum, name);
		    sprintf(out_buf, "Coloretto game #{W%d{w has started!  {W%s{w goes first.\n\r", gnum, name);
		    send_to_char(out_buf, ch);
		}
		break;

		case CTTO_MSG_CARD_DRAWN:
		{
		    char card;

		    parse_command(c, "%h%s%c", &gnum, name, &card);
		    if (!str_cmp(name, ch->name))
			sprintf(out_buf, "You have drawn a %s card.\n\r", card_names[card]);
		    else
			sprintf(out_buf, "%s has drawn a %s card.\n\r", name, card_names[card]);

		    coloretto_message(gnum, out_buf, ch);
		}
		break;

	 	case CTTO_MSG_CARD_PLACED:
		{
		    char card, pile;

		    parse_command(c, "%h%s%c%c", &gnum, name, &card, &pile);
		    if (!str_cmp(name, ch->name))
			sprintf(out_buf, "You placed a %s card on pile #%d.\n\r", card_names[card], pile + 1);
		    else
			sprintf(out_buf, "%s has a %s card on pile #%d.\n\r", name, card_names[card], pile + 1);

		    coloretto_message(gnum, out_buf, ch);
		}
		break;

		case CTTO_MSG_NEXT_TURN:
		{
		    parse_command(c, "%h%s", &gnum, name);
		    if (!str_cmp(name, ch->name))
			sprintf(out_buf, "It is now your turn.\n\r");
		    else
			sprintf(out_buf, "It is now %s's turn.\n\r", name);

		    coloretto_message(gnum, out_buf, ch);
		}
		break;

		case CTTO_MSG_ROUND_OVER:
		{
		    char round;

		    parse_command(c, "%h%c", &gnum, &round);
		    sprintf(out_buf, "Round #%d is over!\n\r", round);
		    coloretto_message(gnum, out_buf, ch);
		}
		break;

		case CTTO_MSG_POINTS_SCORED:
		{
		    char points;

		    parse_command(c, "%h%s%c", &gnum, name, &points);
		    if (!str_cmp(name, ch->name))
			sprintf(out_buf, "You scored %d points this round!\n\r", points);
		    else
			sprintf(out_buf, "%s scored %d points this round!\n\r", name, points);

		    coloretto_message(gnum, out_buf, ch);
		}
		break;

		case CTTO_MSG_PILE_TAKEN:
		{
		    char cards[3];
		    char pile;

		    parse_command(c, "%h%s%c%c%c%c", &gnum, name, &pile, &cards[0], &cards[1], &cards[2]);
		    if (!str_cmp(name, ch->name))
			sprintf(out_buf, "You took pile #%d (%s, %s, %s).\n\r", pile + 1, card_names[cards[0]], card_names[cards[1]], card_names[cards[2]]);
		    else
			sprintf(out_buf, "%s took pile #%d (%s, %s, %s).\n\r", name, pile + 1, card_names[cards[0]], card_names[cards[1]], card_names[cards[2]]);
		    coloretto_message(gnum, out_buf, ch);
		}
		break;

		case CTTO_MSG_CARD_GIVEN:
		{
		    char card;

		    parse_command(c, "%h%s%c", &gnum, name, &card);
		    if (!str_cmp(name, ch->name))
			sprintf(out_buf, "You have been given a %s card.\n\r", card_names[card]);
		    else
			sprintf(out_buf, "%s has been given a %s card.\n\r", name, card_names[card]);

		    coloretto_message(gnum, out_buf, ch);
		}
		break;
	    }
	}
	break;

	case CMD_ERR:
	{
	    char msg;
	    char *c;

	    if (!(c = parse_command(p, "%c", &msg)))
		break;

	    switch (msg)
	    {
		case CTTO_ERR_JOIN:
		{
		    parse_command(c, "%h", &gnum);
		    sprintf(out_buf, "Error joining Coloretto game #%d.\n\r", gnum);
		    send_to_char(out_buf, ch);
		}
		break;

		case CTTO_ERR_GAME_NOT_FOUND:
		{
		    parse_command(c, "%h", &gnum);
		    sprintf(out_buf, "Coloretto game #%d not found.\n\r", gnum);
		    send_to_char(out_buf, ch);
		}
		break;

		case CTTO_ERR_NOT_PLAYER:
		{
		    parse_command(c, "%h", &gnum);
		    strcpy(out_buf, "You are not a player in this game.\n\r");
		    coloretto_message(gnum, out_buf, ch);
		}
		break;

		case CTTO_ERR_NOT_TURN:
		{
		    parse_command(c, "%h", &gnum);
		    strcpy(out_buf, "It is not your turn in this game.\n\r");
		    coloretto_message(gnum, out_buf, ch);
		}
		break;

		case CTTO_ERR_WRONG_PHASE:
		{
		    parse_command(c, "%h", &gnum);
		    strcpy(out_buf, "It is not time to do that.\n\r");
		    coloretto_message(gnum, out_buf, ch);
		}
		break;

		case CTTO_ERR_PILES_FULL:
		{
		    parse_command(c, "%h", &gnum);
		    strcpy(out_buf, "All of the piles are full, you must take one.\n\r");
		    coloretto_message(gnum, out_buf, ch);
		}
		break;

		case CTTO_ERR_INVALID_PILE:
		{
		    parse_command(c, "%h", &gnum);
		    coloretto_message(gnum, "Invalid pile.\n\r", ch);
		}
		break;

		case CTTO_ERR_PILE_FULL:
		{
		    parse_command(c, "%h", &gnum);
		    coloretto_message(gnum, "That pile is full, you may not place there.\n\r", ch);
		}
		break;

		case CTTO_ERR_PILE_GONE:
		{
		    parse_command(c, "%h", &gnum);
		    coloretto_message(gnum, "That pile has already been taken.\n\r", ch);
		}
		break;
	    }
	}
	break;

	case CTTO_CMD_DATA_GLIST:
	{
	    unsigned short list_size, i, j, gid;
	    char status, numplay, points;
  
	    c = parse_command(p, "%h", &list_size);

	    send_to_char("{wColoretto --- Games Listing\n\r{c[{CGID{c] [{CStatus{c]      [{CPlayers{c]\n\r=============================\n\r", ch);

	    for (i = 0; i < list_size; i++)
	    {
		c = parse_command(c, "%h%c%c", &gid, &status, &numplay);
		sprintf(out_buf, "{W%5d %-16s  ", gid, status == CTTO_STATE_GAMEOVER ? "{DCompleted{c" :
						    status == CTTO_STATE_WAITING ? "{WNeed players{c" :
								   "{CIn progress{c");
		send_to_char(out_buf, ch);

		for (j = 0; j < numplay; j++)
		{
		    c = parse_command(c, "%s", buf);

		    if (status == -2)
		        c = parse_command(c, "%c", &points);

		    if (buf[0] != '\0')
		    {
		        sprintf(out_buf, "%s%s%s", j > 0 ? ", " : "", buf, j == status ? "{M*{c" : "");
			send_to_char(out_buf, ch);

			if (status == -2)
			{
			    sprintf(out_buf, " ({C%d{c)", points);
			    send_to_char(out_buf, ch);
			}
		    }
		}

		if (status == -1)
		{
		    sprintf(out_buf, " (Max = {C%d{c)", numplay);
		    send_to_char(out_buf, ch);
		}

		send_to_char("\n\r", ch);
	    }
	}
	break;

	case CTTO_CMD_GAME_VIEW:
	{
	    short gid;
	    char round, turn, state, piles[3][5], *c, cards[10], points;
	    unsigned char num_players;
	    int i, j, k;
	    char pname[25];
	    bool pile_taken;

	    c = parse_command(p, "%h%c%c%c%c", &gid, &round, &turn, &state, &num_players);
	    memcpy(piles, c, 15);
	    c += 15;

	    sprintf(out_buf, "Coloretto Game #%d - Round %d\n\r\n\r", gid, round);
	    send_to_char(out_buf, ch);
	    for (i = 0; i < num_players; i++)
	    {
		sprintf(out_buf, "%sPile %d", i > 0 ? "   " : "", i + 1);
		send_to_char(out_buf, ch);
	    }

	    send_to_char("\n\r", ch);

	    for (i = 0; i < num_players; i++)
	    {
		sprintf(out_buf, "%s======", i > 0 ? "   " : "");
		send_to_char(out_buf, ch);
	    }
	    send_to_char("\n\r", ch);

	    for (i = 0; i < 3; i++)
	    {
		for (j = 0; j < num_players; j++)
		{
		    if (piles[i][j] == -1)
			strcpy(out_buf, "{D-gone-{x");
		    else
			sprintf(out_buf, "%-10s", card_names[piles[i][j]]);

		    send_to_char(out_buf, ch);

		    if (piles[i][j] == CTTO_CARD_WILD)
			send_to_char("  ", ch);

		    if (j < (num_players - 1))
			send_to_char("   ", ch);
		}
		send_to_char("\n\r", ch);
	    }

	    for (i = 0; i < num_players; i++)
	    {
		c = parse_command(c, "%s", name);

		if (i == turn)
		    strcpy(pname, name);

		sprintf(out_buf, "\n\r%s:", name);
		send_to_char(out_buf, ch);

		memcpy(cards, c, 10);
		c += 10;

		for (j = 1; j < 10; j++)
		{
		    if (cards[j] > 0)
		    {
//			found = TRUE;
			sprintf(out_buf, " %c%c", card_names[j][0], card_names[j][1]);
			send_to_char(out_buf, ch);
			for (k = 0; k < cards[j]; k++)
			{
			    sprintf(out_buf, "%c", card_names[j][2]);
			    send_to_char(out_buf, ch);
			}
		    }
		}

		c = parse_command(c, "%c%c", &points, &pile_taken);

		sprintf(out_buf, " {D(%d points)%s{x", points, pile_taken ? " (pile taken)" : "");
		send_to_char(out_buf, ch);
	    }

	    if (state == CTTO_STATE_WAITING)
		strcpy(out_buf, "\n\r\n\rGame is waiting for more people to start.\n\r");
	    else if (state == CTTO_STATE_GAMEOVER)
		strcpy(out_buf, "\n\r\n\rThis game has ended.\n\r");
	    else
		sprintf(out_buf, "\n\r\n\rIt is currently %s's turn to go.\n\r", pname);

	    send_to_char(out_buf, ch);
	}
    }
}


DO_FUNC(do_coloretto)
{
    char arg[MAX_STRING_LENGTH];
    long tgame;

#if defined(WIN32)
    if (!check_windows_socket())
    {
	bug("do_coloretto: Error during WSAStartup.", 0);
	return;
    }
#endif

    if (IS_NPC(ch))
        return;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Coloretto -- Command List\n\r\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "connect"))
    {
	if (ch->pcdata->gamesock != INVALID_SOCKET)
	    gameserv_close(ch);

	ch->pcdata->coldef = -1;
	gameserv_socket(ch);
        return;
    }

    if (ch->pcdata->gamesock == INVALID_SOCKET)
    {
	ch->pcdata->coldef = -1;
	gameserv_socket(ch);
    }

    if (!str_prefix(arg, "players"))
    {
        gameserv_command(ch, SYS_COMMAND, CMD_REQ_PLIST, "");
        return;
    }

    if (!str_cmp(arg, "create"))
    {
	int i;

	if (!is_number(argument))
	{
	    send_to_char("coloretto create -- creates a new game of Coloretto\n\r\n\r", ch);
	    send_to_char("usage: coloretto create <number of players>\n\r\n\r", ch);
	    send_to_char("<number of players> must be a value from 3 to 5\n\r", ch);
	    return;
	}

	i = atoi(argument);

	if ((i < 3) || (i > 5))
	{
	    send_to_char("coloretto create -- creates a new game of Coloretto\n\r\n\r", ch);
	    send_to_char("usage: coloretto create <number of players>\n\r\n\r", ch);
	    send_to_char("<number of players> must be a value from 3 to 5\n\r", ch);
	    return;
	}

	gameserv_command(ch, GNUM_CTTO, CTTO_CMD_CREATE_GAME, "%c", (char) i);
	return;
    }

    if (!str_cmp(arg, "games") || !str_cmp(arg, "list"))
    {
	gameserv_command(ch, GNUM_CTTO, CTTO_CMD_LIST_GAMES, "");
        return;
    }

    if (!str_cmp(arg, "join"))
    {
	unsigned short i;

        if (!is_number(argument))
        {
	    send_to_char("coloretto join -- joins a game of Coloretto\n\r\n\r", ch);
	    send_to_char("usage: coloretto join <game id>\n\r", ch);
	    return;
	}

	i = atoi(argument);

	gameserv_command(ch, GNUM_CTTO, CTTO_CMD_JOIN_GAME, "%h", i);
        return;
    }

    if (!str_prefix(arg, "default"))
    {
        if (!is_number(argument))
	{
	    send_to_char("coloretto default -- sets a default game number for playing Coloretto\n\r\n\r", ch);
	    send_to_char("sage: coloretto default <game number>\n\r\n\r", ch);
	    send_to_char("Valid game numbers are between 0 and 65535.\n\r", ch);
	    return;
	}

	ch->pcdata->coldef = atoi(argument);

	send_to_char("Default Coloretto game set.\n\r", ch);
	return;
    }

    if (is_number(arg))
    {
	tgame = atoi(arg);
	argument = one_argument(argument, arg);
    }
    else
    {
	if (ch->pcdata->coldef == -1)
	{
	    send_to_char("You must enter the game number as the first parameter, or select a default game.\n\r", ch);
	    return;
	}
	else
	    tgame = ch->pcdata->coldef;
    }

    if (!str_cmp(arg, "draw"))
    {
	gameserv_command(ch, GNUM_CTTO, CTTO_CMD_DRAW_CARD, "%h", tgame);
	return;
    }

    if (!str_cmp(arg, "place"))
    {
	int i;

	if (!is_number(argument))
	{
	    send_to_char("coloretto place -- place a drawn card onto a pile\n\r\n\r", ch);
	    send_to_char("usage: coloretto place <pile number>\n\r\n\r", ch);
	    return;
	}

	i = atoi(argument);

	if ((i < 1) || (i > 5))
	{
	    send_to_char("coloretto place -- place a drawn card onto a pile\n\r\n\r", ch);
	    send_to_char("usage: coloretto place <pile number>\n\r\n\r", ch);
	    return;
	}
	
	gameserv_command(ch, GNUM_CTTO, CTTO_CMD_PLACE_CARD, "%h%c", tgame, (char) i - 1);
	return;
    }

    if (!str_cmp(arg, "take"))
    {
	int i;

	if (!is_number(argument))
	{
	    send_to_char("coloretto take -- take a pile of cards\n\r\n\r", ch);
	    send_to_char("usage: coloretto take <pile number>\n\r\n\r", ch);
	    return;
	}

	i = atoi(argument);

	if ((i < 1) || (i > 5))
	{
	    send_to_char("coloretto take -- take a pile of cards\n\r\n\r", ch);
	    send_to_char("usage: coloretto take <pile number>\n\r\n\r", ch);
	    return;
	}
	
	gameserv_command(ch, GNUM_CTTO, CTTO_CMD_TAKE_PILE, "%h%c", tgame, (char) i - 1);
	return;
    }

    if (!str_cmp(arg, "view"))
    {
	gameserv_command(ch, GNUM_CTTO, CTTO_CMD_VIEW_GAME, "%h", tgame);
	return;
    }

    send_to_char("Invalid option.  Type 'coloretto' by itself for command list.\n\r", ch);
    
    return;
}
