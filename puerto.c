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

// #define CBUF_LEN 512
// #define CMD_LEN 256

// #define NAME_LEN    25

// #define CMD_GREETING	3

// #define CMD_DATA_PLIST	5
#define PR_CMD_CREATE_GAME 6
#define PR_CMD_LIST_GAMES	7
#define PR_CMD_DATA_GLIST	8
#define PR_CMD_JOIN_GAME	9
#define	PR_CMD_VIEW_SELF	10
#define PR_CMD_SELF_VIEW	11
#define PR_CMD_VIEW_OTHER	12
#define PR_CMD_OTHER_VIEW	13
#define PR_CMD_VIEW_BUILDS	14
#define PR_CMD_BUILDS_VIEW	15
#define PR_CMD_VIEW_GAME	16
#define PR_CMD_GAME_VIEW	17
#define PR_CMD_CHOOSE_ROLE	18
#define PR_CMD_SELECT_PLANT 19
#define PR_CMD_PASS	20
#define PR_CMD_MOVE_COL	21
#define PR_CMD_REMOVE_COL	22
#define PR_CMD_BUILD	23
#define PR_CMD_CRAFT	24
#define PR_CMD_TRADE	25
#define PR_CMD_SHIP	26
#define PR_CMD_DISCARD	27
#define PR_CMD_VIEW_PLIST  28
#define PR_CMD_PLIST_VIEW	29
#define PR_CMD_SPECTATE	30

#define PR_MSG_GAME_CREATED    1
#define PR_MSG_HAS_JOINED	    2
#define	PR_MSG_JOIN	    3
#define PR_MSG_GAME_START	    4
#define PR_MSG_ROLE_CHOSEN	    5
#define PR_MSG_PLANT_CHOSEN    6
#define PR_MSG_NEXT_PHASE	    7
#define PR_MSG_NEXT_TURN	    8
#define PR_MSG_PASS	    9
#define	PR_MSG_NEXT_GOV	    10
#define PR_MSG_GAMEEND_COL	    11
#define PR_MSG_GAMEEND_VPS	    12
#define PR_MSG_GAMEEND_BUILD   13
#define PR_MSG_GET_COLS	    14
#define PR_MSG_MOVE	    15
#define PR_MSG_REMOVE	    16
#define PR_MSG_BUILD	    17
#define PR_MSG_CRAFTS	    18
#define PR_MSG_CRAFT_ADD	    19
#define PR_MSG_TRADE	    20
#define PR_MSG_TRADE_CLEAR	    21
#define PR_MSG_SHIP	    22
#define PR_MSG_DISCARD	    23
#define PR_MSG_FACTORY	    24
#define PR_MSG_PROSPECT	    25
#define PR_MSG_PHASE_DISCARD   26
#define PR_MSG_GAME_OVER       27
#define PR_MSG_POINTS          28
#define PR_MSG_GAME_WINNER     29
#define PR_MSG_GAME_TIED       30
#define PR_MSG_SPECTATE	    31
#define PR_MSG_NOSPEC	    32


#define PR_ERR_JOIN		    1
#define PR_ERR_VIEW		    2
#define PR_ERR_GAME_NOT_FOUND	    3
#define PR_ERR_NOT_YOUR_TURN	    4
#define PR_ERR_INVALID_ROLE	    5
#define PR_ERR_ROLE_PICKED	    6
#define PR_ERR_NOT_PLAYER	    7
#define PR_ERR_WRONG_PHASE	    8
#define PR_ERR_PLANTS_FULL	    9
#define PR_ERR_PLANT_UNAVAIL	    10
#define PR_ERR_CANT_QUARRY	    11
#define PR_ERR_NO_QUARRY	    12
#define PR_ERR_INVALID_MOVE	    13
#define PR_ERR_FREE_SPACES	    14
#define PR_ERR_BUILD_GONE	    15
#define PR_ERR_BUILD_NOSPACE	    16
#define PR_ERR_BUILD_COST	    17
#define PR_ERR_CRAFT_INVALID	    18
#define PR_ERR_CRAFT_NONE	    19
#define PR_ERR_TRADE_NONE	    20
#define PR_ERR_TRADE_SAME	    21
#define PR_ERR_SHIP_DISCARD	    22
#define PR_ERR_SHIP_NONE	    23
#define PR_ERR_SHIP_FULL	    24
#define PR_ERR_NO_WHARF		    25
#define PR_ERR_CANT_WHARF	    26
#define PR_ERR_BOAT_TAKEN	    27
#define PR_ERR_MUST_SHIP	    28
#define PR_ERR_MUST_DISCARD	    29
#define PR_ERR_SHIP_MAIN	    30
#define PR_ERR_DISCARD_NONE	    31
#define PR_ERR_CHOOSE_ROLE	    32
#define PR_ERR_CANT_SPEC	    33
#define PR_ERR_BUILD_DUPLICATE	    34


#define ROLE_SETTLER	    0
#define ROLE_MAYOR	    1
#define ROLE_BUILDER	    2
#define ROLE_CRAFTSMAN	    3
#define ROLE_TRADER	    4
#define ROLE_CAPTAIN	    5
#define ROLE_PROSPECTOR1    6
#define ROLE_PRESPECTOR2    7

const char * role_names[8] = { "Settler", "Mayor", "Builder", "Craftsman",
			       "Trader", "Captain", "Prospector", "Prospector" };

#define PLANT_CORN	    0
#define PLANT_INDIGO	    1
#define PLANT_SUGAR	    2
#define PLANT_TOBACCO	    3
#define PLANT_COFFEE	    4
#define PLANT_QUARRY	    5

const char * good_names[5] = { "corn", "indigo", "sugar", "tobacco", "coffee" };

const char * cgood_names[5] = { "{Ycorn{w", "{Bindigo{w", "{Wsugar{w", "{Rtobacco{w", "{ycoffee{w" };

struct	player_data
{
    char	    pname[NAME_LEN];
    char	    buildings[12];
    unsigned char   plants[6];
    unsigned char   doubloons;
    unsigned char   goods[5];
    unsigned char   col_buildings[12];
    unsigned char   col_plants[6];
    unsigned char   col_SanJuan;
    unsigned char   col_unassigned;
    unsigned char   vps;
};

struct	game_data
{
    unsigned short  gid;
    char	    num_players;
    char	    turn;
    char	    num_turns;
    unsigned char   vps;
    unsigned char   quarries;
    char	    col_supply;
    unsigned char   col_ship;
    char	    roles[8];
    char	    role_taken[8][NAME_LEN];
    unsigned char   goods[5];
    char	    boat_goods[3];
    char	    boat_types[3];
    char	    thouse[4];
    char	    plants_display[6];
    char	    action;
};
    
  
struct building_data
{
    char *	    name;
    char *	    short_form;
    char	    spaces;
    char	    cost;
};

const struct building_data buildings[23] =
{
    {	"Small Indigo Plant",	"{BSml Indigo{x",	1,  1	},
    {	"Small Sugar Mill",	"{WSml Sugar{x",	1,  2	},
    {	"Small Market",		"{mSml Market{x",	1,  2	},
    {	"Hacienda",		"{mHacienda{x",		1,  2	},
    {	"Construction Hut",	"{mConstr Hut{x",	1,  2	},
    {	"Small Warehouse",	"{mSml Wareh{x",	1,  3	},
    {	"Indigo Plant",		"{BIndigo Pla{x",	3,  3	},
    {	"Sugar Mill",		"{WSugar Mill{x",	3,  4	},
    {	"Hospice",		"{mHospice{x",		1,  4	},
    {	"Office",		"{mOffice{x",		1,  5	},
    {	"Large Market",		"{mLrg Market{x",	1,  5	},
    {	"Large Warehouse",	"{mLrg Wareh{x",	1,  6	},
    {	"Tobacco Storage",	"{RTobacco St{x",	3,  5	},
    {	"Coffee Roaster",	"{yCoffee Rst{x",	2,  6	},
    {	"Factory",		"{mFactory{x",		1,  7	},
    {	"University",		"{mUniversity{x",	1,  8	},
    {	"Harbor",		"{mHarbor{x",		1,  8	},
    {	"Wharf",		"{mWharf{x",		1,  9	},
    {	"Guild Hall",		"{MGuild Hall{x",	1,  10	},
    {	"Residence",		"{MResidence{x",	1,  10	},
    {	"Fortress",		"{MFortress{x",		1,  10	},
    {	"Customs House",	"{MCstms Hous{x",	1,  10	},
    {	"City Hall",		"{MCity Hall{x",	1,  10	}
};


void puerto_viewp(CHAR_DATA *ch, struct player_data pdata, bool show_vps)
{
    char buf[CMD_LEN];
    char out_buf[MAX_STRING_LENGTH];
  int i, j, max_plant = 0, ptypes = 0;

    sprintf(out_buf, "{wPlayer name: {C%s{w\n\r\n\r", pdata.pname);
    send_to_char(out_buf, ch);

	    send_to_char("Bldgs:", ch);
	   
	    if (pdata.buildings[0] == -1)
	    {
	        send_to_char(" {D-none-{x\n\r", ch);
	    }
	    else
	    {
	        for (i = 0; i < 12; i++)
		{
		    if (pdata.buildings[i] == -1)
		    {
			send_to_char("\n\r", ch);
			break;
		    }
	
		    sprintf(buf, "%2d.%-14s %s%s%s",
			i + 1, buildings[pdata.buildings[i]].short_form,
			buildings[pdata.buildings[i]].spaces >= 1 ? pdata.col_buildings[i] >= 1 ? "({y*{x)" : "( )" : "   ",
			buildings[pdata.buildings[i]].spaces >= 2 ? pdata.col_buildings[i] >= 2 ? "({y*{x)" : "( )" : "   ",
			buildings[pdata.buildings[i]].spaces >= 3 ? pdata.col_buildings[i] >= 3 ? "({y*{x)" : "( )" : "   ");

		    send_to_char(buf, ch);
		    
		    if (i % 3 == 2)
		    {
			send_to_char("\n\r", ch);

			if (i != 11)
			    send_to_char("      ", ch);
		    }
		    else
			send_to_char(" ", ch);

//		    send_to_char(out_buf, ch);
		}
	    }

	    send_to_char("\n\r{wPlnts:{x ", ch);

	    for (i = 0; i < 6; i++)
		if (pdata.plants[i] > max_plant)
		    max_plant = pdata.plants[i];

	    for (i = 0; i < 6; i++)
	    {
		max_plant = max_plant <= 5 ? 5 : max_plant <= 9 ? 9 : 12;

		if (pdata.plants[i] <= 0)
		    continue;

		if (((max_plant == 5) && ((ptypes % 3) == 2))
		 || ((max_plant == 9) && ((ptypes % 2) == 1))
		 || ((max_plant == 12) && (ptypes > 0)))
		{
		    send_to_char("\n\r       ", ch);
/*
		    if (i < 5)
			send_to_char("       ", ch);
*/
		}

		switch (i)
		{
		    case 0:
			send_to_char("{YCorn{w   ", ch);
			break;
    
		    case 1:
			send_to_char("{BIndigo{w ", ch);
			break;

		    case 2:
			send_to_char("{WSugar{w  ", ch);
			break;

		    case 3:
			send_to_char("{RTobacc{w ", ch);
			break;

		    case 4:
			send_to_char("{yCoffee{w ", ch);
			break;

		    case 5:
			send_to_char("{wQuarry ", ch);
			break;
		}

		for (j = 1; j <= max_plant; j++)
		    if (pdata.plants[i] >= j)
		    {
			if (pdata.col_plants[i] >= j)
			    send_to_char("({y*{w)", ch);
			else
			    send_to_char("( )", ch);
		    }
		    else
			send_to_char("   " ,ch);

		ptypes++;

	    }

	    if (ptypes == 0)
		send_to_char("{D-none-{x", ch);
    
	    send_to_char("\n\r\n\r", ch);

	    sprintf(out_buf, "{yColonists partying in San Juan: %d.  {YDoubloons: %d",
		    pdata.col_SanJuan, pdata.doubloons);

	    send_to_char(out_buf, ch);

	    if (show_vps)
	    {
		sprintf(out_buf, ", Victory Points: %d\n\r\n\r", pdata.vps);
		send_to_char(out_buf, ch);
	    }
	    else
		send_to_char("\n\r\n\r", ch);

	    sprintf(out_buf, "{xGoods {D- {YCorn: %d, {BIndigo: %d, {WSugar: %d, {RTobacco: %d, {yCoffee: %d{x\n\r",
		    pdata.goods[0], pdata.goods[1], pdata.goods[2],
		    pdata.goods[3], pdata.goods[4]);

	    send_to_char(out_buf, ch);
}

void puerto_message(unsigned short gnum, const char *txt, CHAR_DATA *ch)
{
    char buf[MAX_STRING_LENGTH];

    sprintf(buf, "{rPR:{R%d{r>> {w%s{x", gnum, txt);
    send_to_char(buf, ch);

    return;
}

     

void puerto_interp(CHAR_DATA *ch, char *cmd)
{
    char buf[CMD_LEN];
    char *p = cmd; // + sizeof(pmsg_len);
    char out_buf[MAX_STRING_LENGTH];
    char *c;
    char name[NAME_LEN];
    unsigned short gnum;

// used in CMD_GAME_VIEW
    char gov[NAME_LEN], cur[NAME_LEN];
    struct game_data gdata;
    int i;
    bool first_found = FALSE;


#if defined(PRDEBUG)
    bug("Processing %d.", *p);
#endif

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
/*
	        case MSG_LOGIN_SUCCESS:
		    send_to_char("Puerto Rico connection successful!\n\r", ch);
		    break;
*/
		case PR_MSG_GAME_CREATED:
		{
		    parse_command(c, "%h", &gnum);
		    sprintf(out_buf, "New game (#{W%d{w) created.\n\r", gnum);
		    send_to_char(out_buf, ch);
		}
		break;

		case PR_MSG_JOIN:
		{
		    parse_command(c, "%h", &gnum);
		    sprintf(out_buf, "You have joined Puerto Rico game #{W%d{w!\n\r", gnum);
		    send_to_char(out_buf, ch);
		}
		break;

		case PR_MSG_HAS_JOINED:
		{
		    parse_command(c, "%h%s", &gnum, name);
		    sprintf(out_buf, "%s has joined Puerto Rico game #{W%d{w!\n\r", name, gnum);
		    send_to_char(out_buf, ch);
		}
		break;

		case PR_MSG_GAME_START:
		{
		    parse_command(c, "%h%s", &gnum, name);
		    sprintf(out_buf, "Puerto Rico game #{W%d{w has started!  {W%s{w is governor, and goes first.\n\r", gnum, name);
		    send_to_char(out_buf, ch);
		}
		break;

		case PR_MSG_ROLE_CHOSEN:
		{
		    char role, gold;

		    parse_command(c, "%h%s%c%c", &gnum, name, &role, &gold);
		    sprintf(out_buf, "%s has chosen the role of {W%s{w (%d gold).\n\r", name, role_names[role], gold);
		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_MSG_PLANT_CHOSEN:
		{
		    char ptype;

		    parse_command(c, "%h%s%c", &gnum, name, &ptype);
		    if (ptype == PLANT_QUARRY)
		        sprintf(out_buf, "%s takes a quarry.\n\r", name);
		    else
			sprintf(out_buf, "%s takes a%s %s plantation.\n\r",
				name, ptype == PLANT_INDIGO ? "n" : "", cgood_names[ptype]);

		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_MSG_NEXT_PHASE:
		{
		    char action;

		    parse_command(c, "%h%s%c", &gnum, name, &action);
		    
		    if (!str_cmp(name, ch->name))
			sprintf(out_buf, "It is now {Gyour{w turn to act ({W%s{w).\n\r", role_names[action]);
		    else
			sprintf(out_buf, "It is now %s'%s turn to act ({W%s{w).\n\r", name, LOWER(name[strlen(name) - 1]) == 's' ? "" : "s", role_names[action]);

		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_MSG_NEXT_TURN:
		{
		    char action;
		    char new_cols;
	    
		    c = parse_command(c, "%h%s%c", &gnum, name, &action);

		    if (action == ROLE_MAYOR)
		    {
			parse_command(c, "%c", &new_cols);

			sprintf(out_buf, "The %s round is over, and the ship is repopulated with %d colonists.\n\r", role_names[action], new_cols);
			puerto_message(gnum, out_buf, ch);

		    }
		    else
		    {
			sprintf(out_buf, "The %s round is over%s.\n\r", role_names[action],
			    (action == ROLE_SETTLER) ? ", and new plantations are revealed" : "");
			puerto_message(gnum, out_buf, ch);

		    }

		    if (!str_cmp(name, ch->name))
		        puerto_message(gnum, "It is now {Gyour{w turn to select a role.\n\r", ch);
		    else
		    {
			sprintf(out_buf, "It is %s'%s turn to select a role.\n\r", name, LOWER(name[strlen(name) - 1]) == 's' ? "" : "s");
			puerto_message(gnum, out_buf, ch);
		    }
		}
		break;

		case PR_MSG_PASS:
		{
		    parse_command(c, "%h%s", &gnum, name);

		    if (!str_cmp(name, ch->name))
			puerto_message(gnum, "You pass.\n\r", ch);
		    else
		    {
			sprintf(out_buf, "%s passes.\n\r", name);
			puerto_message(gnum, out_buf, ch);
		    }
		}
		break;

		case PR_MSG_NEXT_GOV:
		{
		    char turn;

		    parse_command(c, "%h%s%c", &gnum, name, &turn);

		    if (!str_cmp(name, ch->name))
			sprintf(out_buf, "It is turn #%d!  You are now the governor.\n\r", turn);
		    else
			sprintf(out_buf, "It is turn #%d!  %s is now the governor.\n\r", turn, name);

		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_MSG_GAMEEND_COL:
		{
		    parse_command(c, "%h", &gnum);
		    puerto_message(gnum, "The colonist supply has been exhausted; the game will end on this turn.\n\r", ch);
		}
		break;

		case PR_MSG_GAMEEND_VPS:
		{
		    parse_command(c, "%h", &gnum);
		    puerto_message(gnum, "The victory point supply has been exhausted; the game will end on this turn.\n\r", ch);
		}
		break;

		case PR_MSG_GAMEEND_BUILD:
		{
		    parse_command(c, "%h%s", &gnum, name);

		    if (!str_cmp(name, ch->name))
			strcpy(out_buf, "You have filled your building spaces; the game will end on this turn.\n\r");
		    else
			sprintf(out_buf, "%s has filled their building spaces; the game will end on this turn.\n\r", name);

		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_MSG_GET_COLS:
		{
		    char cols;

		    parse_command(c, "%h%s%c", &gnum, name, &cols);

		    if (!str_cmp(name, ch->name))
			sprintf(out_buf, "You get %d colonist%s.\n\r", cols, cols == 1 ? "" : "s");
		    else
			sprintf(out_buf, "%s gets %d colonist%s.\n\r", name, cols, cols == 1 ? "" : "s");

		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_MSG_MOVE:
		{
		    char move;

		    parse_command(c, "%h%c", &gnum, &move);

		    sprintf(out_buf, "You move a colonist from San Juan to %s %s%s.\n\r",
			(move == (PLANT_INDIGO * -1)) ? "an" : (move <= 0) ? "a" : "your",
			(move == (PLANT_QUARRY * -1)) ? "quarry" : (move <= 0) ? cgood_names[move * -1] : buildings[move - 1].name,
			((move > (PLANT_QUARRY * -1)) && (move <= 0)) ? " plantation" : "");
		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_MSG_REMOVE:
		{
		    char move;

		    parse_command(c, "%h%c", &gnum, &move);

		    sprintf(out_buf, "You move a colonist from %s %s%s to San Juan.\n\r",
			(move == (PLANT_INDIGO * -1)) ? "an" : (move <= 0) ? "a" : "your",
			(move == (PLANT_QUARRY * -1)) ? "quarry" : (move <= 0) ? cgood_names[move * -1] : buildings[move - 1].name,
			((move > (PLANT_QUARRY * -1)) && (move <= 0)) ? " plantation" : "");
		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_MSG_BUILD:
		{
		    char build_num, cost;

		    parse_command(c, "%h%s%c%c", &gnum, name, &build_num, &cost);

		    if (!str_cmp(name, ch->name))
			sprintf(out_buf, "You build {W%s{w for %d doubloons.\n\r", buildings[build_num].name, cost);
		    else
			sprintf(out_buf, "%s builds {W%s{w for %d doubloons.\n\r", name, buildings[build_num].name, cost);

		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_MSG_CRAFTS:
		{
		    char g;
		    bool first = TRUE;
		    int i;

		    c = parse_command(c, "%h%s", &gnum, name);

		    if (!str_cmp(name, ch->name))
			strcpy(out_buf, "You craft");
		    else
			sprintf(out_buf, "%s crafts", name);

		    for (i = 0; i < 5; i++)
		    {
			c = parse_command(c, "%c", &g);
			
			if (g > 0)
			{
			    sprintf(buf, "%s {%c%d %s{w", first ? "" : ",",
				i == PLANT_CORN ? 'Y' :
				i == PLANT_INDIGO ? 'B' :
				i == PLANT_SUGAR ? 'W' :
				i == PLANT_TOBACCO ? 'R' : 'y',
				g, good_names[i]);
			    strcat(out_buf, buf);
			    first = FALSE;
			}
		    }

		    if (first)
			strcat(out_buf, " nothing");

		    strcat(out_buf, ".\n\r");
		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_MSG_CRAFT_ADD:
		{
		    char good_num;

		    parse_command(c, "%h%s%c", &gnum, name, &good_num);
		
		    if (!str_cmp(name, ch->name))
		        sprintf(out_buf, "You craft an additional %s.\n\r", cgood_names[good_num]);
		    else
			sprintf(out_buf, "%s crafts an additional %s.\n\r", name, cgood_names[good_num]);

		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_MSG_TRADE:
		{
		    char good_num, value;

		    parse_command(c, "%h%s%c%c", &gnum, name, &good_num, &value);

		    if (!str_cmp(name, ch->name))
		        sprintf(out_buf, "You trade %s for %d doubloons.\n\r",
				cgood_names[good_num], value);
		    else
			sprintf(out_buf, "%s trades %s for %d doubloons.\n\r", name,
				cgood_names[good_num], value);
				
		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_MSG_TRADE_CLEAR:
		{
		    parse_command(c, "%h", &gnum);
		    puerto_message(gnum, "The trading house is cleared of goods.\n\r", ch);
		}
		break;

		case PR_MSG_SHIP:
		{
		    char good_num, quantity, points;

		    parse_command(c, "%h%s%c%c%c", &gnum, name, &quantity, &good_num, &points);

		    if (!str_cmp(name, ch->name))
			sprintf(out_buf, "You ship {%c%d %s{w for %d points.\n\r",
				good_num == PLANT_CORN ? 'Y' :
				good_num == PLANT_INDIGO ? 'B' :
				good_num == PLANT_SUGAR ? 'W' :
				good_num == PLANT_TOBACCO ? 'R' : 'y',
				quantity, good_names[good_num], points);
		    else
			sprintf(out_buf, "%s ships {%c%d %s{w for %d points.\n\r", name,
				good_num == PLANT_CORN ? 'Y' :
				good_num == PLANT_INDIGO ? 'B' :
				good_num == PLANT_SUGAR ? 'W' :
				good_num == PLANT_TOBACCO ? 'R' : 'y',
				quantity, good_names[good_num], points);

		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_MSG_DISCARD:
		{
		    char good_num, quantity;

		    parse_command(c, "%h%s%c%c", &gnum, name, &quantity, &good_num);
	    
		    if (!str_cmp(name, ch->name))
		        sprintf(out_buf, "You discard {%c%d %s{w.\n\r",
				good_num == PLANT_CORN ? 'Y' :
				good_num == PLANT_INDIGO ? 'B' :
				good_num == PLANT_SUGAR ? 'W' :
				good_num == PLANT_TOBACCO ? 'R' : 'y',
				quantity, good_names[good_num]);
		    else
			sprintf(out_buf, "%s discards {%c%d %s{w.\n\r", name,
				good_num == PLANT_CORN ? 'Y' :
				good_num == PLANT_INDIGO ? 'B' :
				good_num == PLANT_SUGAR ? 'W' :
				good_num == PLANT_TOBACCO ? 'R' : 'y',
				quantity, good_names[good_num]);

		    puerto_message(gnum, out_buf, ch);
		}
	        break;

		case PR_MSG_FACTORY:
		{
		    char earn;

		    parse_command(c, "%h%s%c", &gnum, name, &earn);

		    if (!str_cmp(name, ch->name))
		        sprintf(out_buf, "Your factory produces %d doubloons.\n\r", earn);
		    else
		        sprintf(out_buf, "%s'%s factory produces %d doubloons.\n\r", name, LOWER(name[strlen(name) - 1]) == 's' ? "" : "s", earn);

		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_MSG_PROSPECT:
		{
		    parse_command(c, "%h%s", &gnum, name);

		    if (!str_cmp(name, ch->name))
			strcpy(out_buf, "You prospect for 1 doubloon.\n\r");
		    else
			sprintf(out_buf, "%s prospects for 1 doubloon.\n\r", name);

		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_MSG_PHASE_DISCARD:
		{
		    parse_command(c, "%h", &gnum);
		    puerto_message(gnum, "Shipping is done, it is time to discard excess goods.\n\r", ch);
		}
		break;

		case PR_MSG_GAME_OVER:
		{
		    parse_command(c, "%h", &gnum);
		    sprintf(out_buf, "Puerto Rico game #{W%d{w is over!\n\r", gnum);
		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_MSG_POINTS:
		{
		    unsigned char points;

		    parse_command(c, "%h%s%c", &gnum, name, &points);

		    if (!str_cmp(name, ch->name))
			sprintf(out_buf, "You finish the game with {W%d{w points.\n\r", points);
		    else
			sprintf(out_buf, "%s finishes the game with {W%d{w points.\n\r", name, points);

		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_MSG_GAME_WINNER:
		{
		    unsigned char points;

		    parse_command(c, "%h%s%c", &gnum, name, &points);

		    if (!str_cmp(name, ch->name))
			strcpy(out_buf, "{YYou have won the game!!\n\r");
		    else
			sprintf(out_buf, "{Y%s has won Puerto Rico game #%d!\n\r", name, gnum);

		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_MSG_GAME_TIED:
		{
		    unsigned char points;

		    parse_command(c, "%h%c", &gnum, &points);

		    sprintf(out_buf, "{YPuerto Rico game #%d has ended in a tie at %d points!\n\r", gnum, points);
		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_MSG_SPECTATE:
		{
		    parse_command(c, "%h%s", &gnum, name);

		    if (!str_cmp(name, ch->name))
			sprintf(out_buf, "You begin to spectate Puerto Rico game #{W%d{w.\n\r", gnum);
		    else
			sprintf(out_buf, "%s begins to spectate this game.\n\r", name);

		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_MSG_NOSPEC:
		{
		    parse_command(c, "%h%s", &gnum, name);

		    if (!str_cmp(name, ch->name))
			sprintf(out_buf, "You stop spectating Puerto Rico game #{W%d{w.\n\r", gnum);
		    else
			sprintf(out_buf, "%s stops spectating this game.\n\r", name);

		    puerto_message(gnum, out_buf, ch);
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
		case PR_ERR_JOIN:
		{
		    parse_command(c, "%h", &gnum);
		    sprintf(out_buf, "Error joining Puerto Rico game #%d.\n\r", gnum);
		    send_to_char(out_buf, ch);
		}
		break;

		case PR_ERR_VIEW:
		{
		    parse_command(c, "%h%s", &gnum, name);
		    sprintf(out_buf, "'%s' is a not a player in this game.\n\r", name);
		    puerto_message(gnum, out_buf, ch);
		}   
		break;

		case PR_ERR_GAME_NOT_FOUND:
		{
		    parse_command(c, "%h", &gnum);
		    sprintf(out_buf, "Puerto Rico game #%d not found.\n\r", gnum);
		    send_to_char(out_buf, ch);
		}
		break;

		case PR_ERR_NOT_YOUR_TURN:
		{
		    parse_command(c, "%h", &gnum);
		    strcpy(out_buf, "It is not your turn in this game.\n\r");
		    puerto_message(gnum, out_buf, ch);
		}
		break;

//		case PR_ERR_INVALID_ROLE:

		case PR_ERR_ROLE_PICKED:
		{
		    parse_command(c, "%h", &gnum);
		    puerto_message(gnum, "That role has already been taken.  Please choose again.\n\r", ch);
		}
		break;

		case PR_ERR_NOT_PLAYER:
		{
		    parse_command(c, "%h", &gnum);
		    strcpy(out_buf, "You are not a player in this game.\n\r");
		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_ERR_WRONG_PHASE:
		{
		    char phase;

		    parse_command(c, "%h%c", &gnum, &phase);
		    if (phase == -1)
			puerto_message(gnum, "It is not time to pick a role.\n\r", ch);
		    else
		    {
			sprintf(out_buf, "It is not currently the %s phase.\n\r", role_names[phase]);
			puerto_message(gnum, out_buf, ch);
		    }

		    return;
		}
		break;

		case PR_ERR_PLANT_UNAVAIL:
		{
		    parse_command(c, "%h", &gnum);
		    puerto_message(gnum, "That type of plantation is not available.\n\r", ch);
		}
		break;

		case PR_ERR_PLANTS_FULL:
		{
		    parse_command(c, "%h", &gnum);
		    puerto_message(gnum, "Your plantation lot is full.\n\r", ch);
		}
		break;

		case PR_ERR_CANT_QUARRY:
		{
		    parse_command(c, "%h", &gnum);
		    puerto_message(gnum, "You are unable to select a quarry at this time.\n\r", ch);
		}
		break;

		case PR_ERR_NO_QUARRY:
		{
		    parse_command(c, "%h", &gnum);
		    puerto_message(gnum, "There are no quarries left available!  Please select again.\n\r", ch);
		}
		break;

		case PR_ERR_INVALID_MOVE:
		{
		    parse_command(c, "%h", &gnum);
		    puerto_message(gnum, "Invalid colonist move attempted.\n\r", ch);
		}
		break;

		case PR_ERR_FREE_SPACES:
		{
		    parse_command(c, "%h", &gnum);
		    puerto_message(gnum, "You cannot end the mayor phase with free spaces remaining and colonists in San Juan.\n\r", ch);
		}
		break;

		case PR_ERR_BUILD_GONE:
		{
		    char build_num;

		    parse_command(c, "%h%c", &gnum, &build_num);
		    sprintf(out_buf, "There are no %s remaining.\n\r", buildings[build_num].name);
		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_ERR_BUILD_NOSPACE:
		{
		    char build_num;

		    parse_command(c, "%h%c", &gnum, &build_num);
		    sprintf(out_buf, "You don't have enough space to build %s.\n\r", buildings[build_num].name);
		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_ERR_BUILD_COST:
		{
		    char build_num, cost;

		    parse_command(c, "%h%c%c", &gnum, &build_num, &cost);
		    sprintf(out_buf, "You cannot afford to build %s (cost = %d).\n\r", buildings[build_num].name, cost);
		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_ERR_BUILD_DUPLICATE:
		{
		    char build_num;

		    parse_command(c, "%h%c", &gnum, &build_num);
		    sprintf(out_buf, "You already have %s.\n\r", buildings[build_num].name);
		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_ERR_CRAFT_INVALID:
		{
		    char good_num;

		    parse_command(c, "%h%c", &gnum, &good_num);
		    sprintf(out_buf, "You are not allowed to take an additional %s.\n\r", good_names[good_num]);
		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_ERR_CRAFT_NONE:
		{
		    char good_num;

		    parse_command(c, "%h%c", &gnum, &good_num);
		    sprintf(out_buf, "There are no %s available for crafting.\n\r", good_names[good_num]);
		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_ERR_TRADE_NONE:
		{
		    char good_num;

		    parse_command(c, "%h%c", &gnum, &good_num);
		    sprintf(out_buf, "You have no %s to trade.\n\r", good_names[good_num]);
		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_ERR_TRADE_SAME:
		{
		    char good_num;

		    parse_command(c, "%h%c", &gnum, &good_num);
		    sprintf(out_buf, "The trading house already contains %s.\n\r", good_names[good_num]);
		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_ERR_SHIP_DISCARD:
		{
		    parse_command(c, "%h", &gnum);
		    puerto_message(gnum, "It is currently time to discard goods.\n\r", ch);
		}
		break;

		case PR_ERR_SHIP_NONE:
		{
		    char good_num;

		    parse_command(c, "%h%c", &gnum, &good_num);
		    sprintf(out_buf, "You don't have any %s to ship.\n\r", good_names[good_num]);
		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_ERR_SHIP_FULL:
		{
		    parse_command(c, "%h", &gnum);
		    puerto_message(gnum, "That ship is already full.\n\r", ch);
		}
		break;

		case PR_ERR_NO_WHARF:
		{
		    parse_command(c, "%h", &gnum);
		    puerto_message(gnum, "You do not own a manned wharf.\n\r", ch);
		}
		break;

		case PR_ERR_CANT_WHARF:
		{
		    parse_command(c, "%h", &gnum);
		    puerto_message(gnum, "You are not able to use your wharf at this time.\n\r", ch);
		}
		break;

		case PR_ERR_BOAT_TAKEN:
		{
		    parse_command(c, "%h", &gnum);
		    puerto_message(gnum, "That boat already contains another type of good.\n\r", ch);
		}
		break;

		case PR_ERR_MUST_SHIP:
		{
		    parse_command(c, "%h", &gnum);
		    puerto_message(gnum, "You cannot pass at this time, you have goods to ship.\n\r", ch);
		}
		break;

		case PR_ERR_MUST_DISCARD:
		{
		    parse_command(c, "%h", &gnum);
		    puerto_message(gnum, "You cannot pass at this time, you have goods to discard.\n\r", ch);
		}
		break;

		case PR_ERR_SHIP_MAIN:
		{
		    parse_command(c, "%h", &gnum);
		    puerto_message(gnum, "It is currently time to ship goods.\n\r", ch);
		}
		break;

		case PR_ERR_DISCARD_NONE:
		{
		    char good_num;

		    parse_command(c, "%h%c", &gnum, &good_num);
		    sprintf(out_buf, "You have no %s to discard.\n\r", good_names[good_num]);
		    puerto_message(gnum, out_buf, ch);
		}
		break;

		case PR_ERR_CHOOSE_ROLE:
		{
		    parse_command(c, "%h", &gnum);
		    puerto_message(gnum, "You must choose a role at this time.\n\r", ch);
		}
		break;

		case PR_ERR_CANT_SPEC:
		{
		    parse_command(c, "%h", &gnum);
		    puerto_message(gnum, "You cannot spectate a game you're already involved in.\n\r", ch);
		}
		break;
	    }
	}
	break;
/*
	case CMD_DATA_PLIST:
	{
	    long list_size;
	    unsigned short i;

	    c = parse_command(p, "%l", &list_size);

	    sprintf(out_buf, "{BPlayers currently connected ({C%ld{B):\n\r", list_size);
	    send_to_char(out_buf, ch);

	    for (i = 0; i < list_size; i++)
	    {
		c = parse_command(c, "%s", buf);
		sprintf(out_buf, "{c%s{x\n\r", buf);
		send_to_char(out_buf, ch);
	    }
	}
	break;
*/
	case PR_CMD_DATA_GLIST:
	{
	    unsigned short list_size, i, j, gid;
	    char status, numplay, points;
  
	    c = parse_command(p, "%h", &list_size);

	    send_to_char("{wPuerto Rico --- Games Listing\n\r{c[{CGID{c] [{CStatus{c]      [{CPlayers{c]\n\r=============================\n\r", ch);

	    for (i = 0; i < list_size; i++)
	    {
		c = parse_command(c, "%h%c%c", &gid, &status, &numplay);
		sprintf(out_buf, "{W%5d %-16s  ", gid, status == -2 ? "{DCompleted{c" :
						    status == -1 ? "{WNeed players{c" :
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

	case PR_CMD_SELF_VIEW:
	{
	    struct player_data pdata;

	    memcpy(&pdata, p, sizeof(pdata));
	    puerto_viewp(ch, pdata, TRUE);

	}
	break;

	case PR_CMD_OTHER_VIEW:
	{
	    struct player_data pdata;

	    memcpy(&pdata, p, sizeof(pdata) - 1);
	    puerto_viewp(ch, pdata, FALSE);
	}
	break;

	case PR_CMD_BUILDS_VIEW:
	{
	    char bdata[23];
	    int i, j, k, cols = 4;

	    memcpy(bdata, p, sizeof(bdata));
	    
	    for (i = 0; i < 6; i++)
	    {
		if (i == 5)
		    cols = 3;

		for (j = 0; j < cols; j++)
		{
		    if (j == 0)
			strcpy(out_buf, "");
		    else
			strcat(out_buf, "   ");

		    sprintf(buf, "%2d.%-14s x%d", (i + 1) + (j * 6), buildings[i + (j * 6)].short_form, bdata[i + (j * 6)]);
		    strcat(out_buf, buf);
		}

		strcat(out_buf, "\n\r");
		send_to_char(out_buf, ch);
		strcpy(out_buf, "   ");

		for (j = 0; j < cols; j++)
		{
		    if (j != 0)
		        strcat(out_buf, "      ");
		
		    for (k = 0; k < 3; k++)
		        strcat(out_buf, buildings[i + (j * 6)].spaces >= (k + 1) ? "( )" : "   ");

		    sprintf(buf, " {Y%2dg{x", buildings[i + (j * 6)].cost);
		    strcat(out_buf, buf);
		}

	        strcat(out_buf, "\n\r");
		send_to_char(out_buf, ch);
	    }
	}
	break;

	case PR_CMD_GAME_VIEW:
	{
	    c = parse_command(p, "%s%s", gov, cur);

	    memcpy(&gdata, c, 39 + (8 * NAME_LEN));

	    sprintf(out_buf, "Puerto Rico Game #%d -- Status   Turn: %d   Governor: %s\n\r\n\r", gdata.gid, gdata.num_turns + 1, gov);
	    send_to_char(out_buf, ch);

	    for (i = 0; i < (3 + gdata.num_players); i++)
	    {
		if (gdata.roles[i] >= 0)
		    sprintf(out_buf, "%-10s %2d gold\n\r", role_names[i], gdata.roles[i]);
		else
		    sprintf(out_buf, "%-10s {Dunavail{w  -taken by %s-\n\r", role_names[i], gdata.role_taken[i]);

		send_to_char(out_buf, ch);
	    }

	    send_to_char("\n\rGoods Remaining -- ", ch);

	    for (i = 0; i < 5; i++)
	    {
		sprintf(out_buf, "%s%s: %d", (i > 0) ? ", " : "", good_names[i], gdata.goods[i]);
		send_to_char(out_buf, ch);
	    }

	    send_to_char("\n\r\n\rShip #1 carrying ", ch);

	    sprintf(out_buf, "%-11s (%d, %d free)     Victory points remaining: %d\n\r",
		gdata.boat_types[0] == -1 ? "{wnothing{w" : cgood_names[gdata.boat_types[0]],
		gdata.boat_goods[0], 1 + gdata.num_players - gdata.boat_goods[0], gdata.vps);
	    send_to_char(out_buf, ch);

	    sprintf(out_buf, "Ship #2 carrying %-11s (%d, %d free)     Quarries remaining: %d\n\r",
		gdata.boat_types[1] == -1 ? "{wnothing{w" : cgood_names[gdata.boat_types[1]],
		gdata.boat_goods[1], 2 + gdata.num_players - gdata.boat_goods[1], gdata.quarries);
	    send_to_char(out_buf, ch);

	    sprintf(out_buf, "Ship #3 carrying %-11s (%d, %d free)     Colonists -- supply: %d, ship: %d\n\r\n\rTrading House contains: ",
		gdata.boat_types[2] == -1 ? "{wnothing{w" : cgood_names[gdata.boat_types[2]],
		gdata.boat_goods[2], 3 + gdata.num_players - gdata.boat_goods[2], gdata.col_supply, gdata.col_ship);
	    send_to_char(out_buf, ch);

	    for (i = 0; i < 4; i++)
	    {
		if (gdata.thouse[i] >= 0)
		{
		    sprintf(out_buf, "%s%s", (i > 0) ? ", " : "", cgood_names[gdata.thouse[i]]);
		    send_to_char(out_buf, ch);
		}
		else
		{
		    sprintf(out_buf, "  (%d empty)", 4 - i);
		    send_to_char(out_buf, ch);
		    break;
		}
	    }

	    send_to_char("\n\rPlantations visible: ", ch);

	    if (gdata.turn == -1)
		send_to_char("none yet!", ch);
	    else
	    {
	        for (i = 0; i < gdata.num_players + 1; i++)
		    if (gdata.plants_display[i] >= 0)
		    {
		        sprintf(out_buf, "%s%s", first_found ? ", " : "", cgood_names[gdata.plants_display[i]]);
		        send_to_char(out_buf, ch);
			first_found = TRUE;
		    }
	    }

	    send_to_char("\n\r\n\r", ch);

	    if (gdata.turn == -1)
		strcpy(out_buf, "This game has not yet started.\n\r");
	    else if (gdata.action == -1)
		sprintf(out_buf, "Currently waiting for {W%s{w to choose a role.\n\r", cur);
	    else
		sprintf(out_buf, "Currently waiting for {W%s{w to participate in the %s phase.\n\r", cur, role_names[gdata.action]);

	    send_to_char(out_buf, ch);
	}
	break;

	case PR_CMD_PLIST_VIEW:
	{
	    char num_play, turn, curgov, i, j, g, dubs;

	    c = parse_command(p, "%h%c%c%c", &gnum, &num_play, &turn, &curgov);

	    send_to_char("{CT G {b[{CName{b]                     {CDub  {b[{COn Hand{b]            [{CProduction{b]\n\r", ch);
	    send_to_char("{c===========================================================================\n\r", ch);
	
	    for (i = 0; i < num_play; i++)
	    {
		if (turn == i)
		    send_to_char("{MT ", ch);
		else
		    send_to_char("  ", ch);

		if (curgov == i)
		    send_to_char("{RG ", ch);
		else
		    send_to_char("  ", ch);

		c = parse_command(c, "%s%c", name, &dubs);

		sprintf(out_buf, "{W%-25s  {w%2dg", name, dubs);
		send_to_char(out_buf, ch);

		for (j = 0; j < 5; j++)
		{
		    c = parse_command(c, "%c", &g);

		    sprintf(out_buf, " {%c%2d%c",
			j == 0 ? 'Y' : j == 1 ? 'B' : j == 2 ? 'W' : j == 3 ? 'R' : 'y', g,
			j == 0 ? 'c' : j == 1 ? 'i' : j == 2 ? 's' : j == 3 ? 't' : 'c');

		    send_to_char(out_buf, ch);
		}

		send_to_char(" ", ch);

		for (j = 0; j < 5; j++)
		{
		    c = parse_command(c, "%c", &g);

		    sprintf(out_buf, " {%c%2d%c",
			j == 0 ? 'Y' : j == 1 ? 'B' : j == 2 ? 'W' : j == 3 ? 'R' : 'y', g,
			j == 0 ? 'c' : j == 1 ? 'i' : j == 2 ? 's' : j == 3 ? 't' : 'c');

		    send_to_char(out_buf, ch);
		}

		send_to_char("\n\r", ch);
	    }
	}
	break;

    }
}
		    

/*
DO_FUNC(do_citadels)
{
    char arg[MAX_STRING_LENGTH];

#if defined(WIN32)
    if (!check_windows_socket())
    {
	bug("do_puerto: Error during WSAStartup.", 0);
	return;
    }
#endif

    if (IS_NPC(ch))
        return;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Citadels -- Command List\n\r\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "connect"))
    {
	if (ch->pcdata->gamesock != INVALID_SOCKET)
	    gameserv_close(ch);

	ch->pcdata->citdef = -1;
	gameserv_socket(ch);
        return;
    }

    if (ch->pcdata->gamesock == INVALID_SOCKET)
    {
	ch->pcdata->citdef = -1;
	gameserv_socket(ch);
    }

    if (!str_prefix(arg, "players"))
    {
        gameserv_command(ch, SYS_COMMAND, CMD_REQ_PLIST, "");
        return;
    }

    send_to_char("Invalid option.  Type 'citadels' by itself for command list.\n\r", ch);
    
    return;
}
*/    

DO_FUNC (do_puerto)
{
    char arg[MAX_STRING_LENGTH];
    long tgame;

#if defined(WIN32)
    if (!check_windows_socket())
    {
	bug("do_puerto: Error during WSAStartup.", 0);
	return;
    }
#endif

    if (IS_NPC(ch))
        return;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Puerto Rico -- Command List\n\r\n\r", ch);
	send_to_char("puerto players          -- generate a list of people connected to server\n\r", ch);
	send_to_char("puerto create <#>       -- create a new game for <#> players\n\r", ch);
	send_to_char("puerto list/games       -- generate a list of available games\n\r", ch);
	send_to_char("puerto join <#>         -- join game <#>\n\r\n\r", ch);

	send_to_char("puerto default              -- set a default game to replace the first <#>\n\r", ch);
	send_to_char("                               in all of the following commands\n\r", ch);
	send_to_char("puerto <#> spec(tate)       -- spectate game\n\r", ch);
	send_to_char("puerto <#> view (null)/self -- view your personal board\n\r", ch);
	send_to_char("   \"    \"   \"   buildings   -- view available buildings\n\r", ch);
	send_to_char("   \"    \"   \"   game        -- view game status\n\r", ch);
	send_to_char("   \"    \"   \"   play(ers)   -- quick goods summary of players\n\r", ch);
	send_to_char("   \"    \"   \"   <name>      -- view player <name>, case sensitive\n\r", ch);
	send_to_char("puerto <#> game             -- same as \"view game\"\n\r", ch);
	send_to_char("puerto <#> role <role>      -- choose role <role>\n\r", ch);
	send_to_char("puerto <#> choose <plant>   -- choose a plant during the settler phase.\n\r", ch);
	send_to_char("                               <plant> can also be \"random\" or \"quarry\"\n\r", ch);
	send_to_char("puerto <#> select/plant <p> -- same as puerto choose\n\r", ch);
	send_to_char("puerto <#> pass             -- pass your action\n\r", ch);
	send_to_char("puerto <#> move/remove <lo> -- move colonists to and from San Juan.\n\r", ch);
	send_to_char("                               <lo> is building number or plant type.\n\r", ch);
	send_to_char("puerto <#> build <build #>  -- build during the builder phase\n\r", ch);
	send_to_char("puerto <#> craft <good>     -- select extra good in craftsman phase\n\r", ch);
	send_to_char("puerto <#> trade <good>     -- select a type of good to trade, trader phase\n\r", ch);
	send_to_char("puerto <#> ship <good> <#>  -- ship <good> on boat <#>, can also be \"wharf\"\n\r", ch);
	send_to_char("puerto <#> discard <good>   -- select a good type to discard at captain end\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "connect"))
    {
	if (ch->pcdata->gamesock != INVALID_SOCKET)
	    gameserv_close(ch);

	ch->pcdata->prdef = -1;
	gameserv_socket(ch);
        return;
    }

    if (ch->pcdata->gamesock == INVALID_SOCKET)
    {
	ch->pcdata->prdef = -1;
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
	    send_to_char("puerto create -- creates a new game of Puerto Rico\n\r\n\r", ch);
	    send_to_char("usage: puerto create <number of players>\n\r\n\r", ch);
	    send_to_char("<number of players> must be a value from 3 to 5\n\r", ch);
	    return;
	}

	i = atoi(argument);

	if ((i < 3) || (i > 5))
	{
	    send_to_char("puerto create -- creates a new game of Puerto Rico\n\r\n\r", ch);
	    send_to_char("usage: puerto create <number of players>\n\r\n\r", ch);
	    send_to_char("<number of players> must be a value from 3 to 5\n\r", ch);
	    return;
	}

	gameserv_command(ch, GNUM_PR, PR_CMD_CREATE_GAME, "%c", (char) i);
	return;
    }

    if (!str_cmp(arg, "games") || !str_cmp(arg, "list"))
    {
	gameserv_command(ch, GNUM_PR, PR_CMD_LIST_GAMES, "");
        return;
    }

    if (!str_cmp(arg, "join"))
    {
	unsigned short i;

        if (!is_number(argument))
        {
	    send_to_char("puerto join -- joins a game of Puerto Rico\n\r\n\r", ch);
	    send_to_char("usage: puerto join <game id>\n\r", ch);
	    return;
	}

	i = atoi(argument);

	gameserv_command(ch, GNUM_PR, PR_CMD_JOIN_GAME, "%h", i);
        return;
    }

    if (!str_prefix(arg, "default"))
    {
        if (!is_number(argument))
	{
	    send_to_char("puerto default -- sets a default game number for playing Puerto Rico\n\r\n\r", ch);
	    send_to_char("sage: puerto default <game number>\n\r\n\r", ch);
	    send_to_char("Valid game numbers are between 0 and 65535.\n\r", ch);
	    return;
	}

	ch->pcdata->prdef = atoi(argument);

	send_to_char("Default game set.\n\r", ch);
	return;
    }

    if (is_number(arg))
    {
	tgame = atoi(arg);
	argument = one_argument(argument, arg);
    }
    else
    {
	if (ch->pcdata->prdef == -1)
	{
	    send_to_char("You must enter the game number as the first parameter, or select a default game.\n\r", ch);
	    return;
	}
	else
	    tgame = ch->pcdata->prdef;
    }

    if (!str_cmp(arg, "spec") || !str_cmp(arg, "spectate"))
    {
	gameserv_command(ch, GNUM_PR, PR_CMD_SPECTATE, "%h", tgame);
	return;
    }

    if (!str_prefix(arg, "view"))
    {
	if (argument[0] == '\0' || !str_cmp(argument, "self"))
	{
	    gameserv_command(ch, GNUM_PR, PR_CMD_VIEW_SELF, "%h", tgame);
	    return;
	}

	if (!str_cmp(argument, "buildings"))
	{
	    gameserv_command(ch, GNUM_PR, PR_CMD_VIEW_BUILDS, "%h", tgame);
	    return;
	}

	if (!str_cmp(argument, "game"))
	{
	    gameserv_command(ch, GNUM_PR, PR_CMD_VIEW_GAME, "%h", tgame);
	    return;
	}

	if (!str_cmp(argument, "play") || !str_cmp(argument, "players"))
	{
	    gameserv_command(ch, GNUM_PR, PR_CMD_VIEW_PLIST, "%h", tgame);
	    return;
	}

	gameserv_command(ch, GNUM_PR, PR_CMD_VIEW_OTHER, "%h%s", tgame, argument);
	return;
    }

    if (!str_cmp(arg, "game"))
    {
	gameserv_command(ch, GNUM_PR, PR_CMD_VIEW_GAME, "%h", tgame);
	return;
    }


    if (!str_cmp(arg, "role"))
    {
	int i;

	argument = one_argument(argument, arg);

	for (i = 0; i < 7; i++)
	    if (!str_prefix(arg, role_names[i]))
	    {
		gameserv_command(ch, GNUM_PR, PR_CMD_CHOOSE_ROLE, "%h%c", tgame, i);
		return;
	    }

	send_to_char("puerto role -- choose a role to perform\n\r\n\rusage: puerto role <role>\n\r\n\rValid roles are: Settler, Mayor, Builder, Craftsman, Trader, Captain, Prospector\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "choose") || !str_cmp(arg, "select") || !str_cmp(arg, "plant"))
    {
	int i;

	argument = one_argument(argument, arg);

	if (!str_cmp(arg, "random"))
	{
	    gameserv_command(ch, GNUM_PR, PR_CMD_SELECT_PLANT, "%h%c", tgame, -1);
	    return;
	}    

	if (!str_cmp(arg, "quarry"))
	{
	    gameserv_command(ch, GNUM_PR, PR_CMD_SELECT_PLANT, "%h%c", tgame, PLANT_QUARRY);
	    return;
	}

	for (i = 0; i < 5; i++)
	    if (!str_prefix(arg, good_names[i]))
	    {
		gameserv_command(ch, GNUM_PR, PR_CMD_SELECT_PLANT, "%h%c", tgame, i);
		return;
	    }

	send_to_char("puerto select: invalid selection\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "pass"))
    {
	gameserv_command(ch, GNUM_PR, PR_CMD_PASS, "%h", tgame);
	return;
    }

    if (!str_prefix(arg, "move") || !str_prefix(arg, "remove"))
    {
	char i;
	char cnum;

	if (!str_cmp(arg, "move"))
	    cnum = PR_CMD_MOVE_COL;
	else
	    cnum = PR_CMD_REMOVE_COL;

	argument = one_argument(argument, arg);

	if (is_number(arg))
	{
	    i = atoi(arg);

	    if ((i < 1) || (i > 12))
	    {
		send_to_char("Invalid building number.  Valid building numbers are between 1 and 12.\n\r", ch);
		return;
	    }

	    gameserv_command(ch, GNUM_PR, cnum, "%h%c", tgame, i);
	    return;
	}

	if (!str_cmp(arg, "quarry"))
	{
	    gameserv_command(ch, GNUM_PR, cnum, "%h%c", tgame, PLANT_QUARRY * -1);
	    return;
	}

	for (i = 0; i < 5; i++)
	    if (!str_cmp(arg, good_names[i]))
	    {
		gameserv_command(ch, GNUM_PR, cnum, "%h%c", tgame, i * -1);
		return;
	    }

	send_to_char("puerto move/remove -- move colonists during the mayor phase\n\r\n\r", ch);
	send_to_char("usage: puerto (re)move <building #/plant type>\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "build") || !str_cmp(arg, "buy"))
    {
	if (is_number(argument))
	{
	    int i = atoi(argument);

	    if ((i >= 1) && (i <= 23))
	    {
		gameserv_command(ch, GNUM_PR, PR_CMD_BUILD, "%h%c", tgame, i - 1);
		return;
	    }
	}

	send_to_char("puerto build -- build a new building during the builder phase\n\r\n\r", ch);
	send_to_char("usage: puerto build <building #>\n\r\n\r", ch);
	send_to_char("Valid building numbers are from 1 to 23.\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "craft"))
    {
	int i;

	argument = one_argument(argument, arg);

	for (i = 0; i < 5; i++)
	    if (!str_prefix(arg, good_names[i]))
	    {
		gameserv_command(ch, GNUM_PR, PR_CMD_CRAFT, "%h%c", tgame, i);
		return;
	    }

	send_to_char("puerto craft: invalid selection\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "trade"))
    {
	int i;

	argument = one_argument(argument, arg);

	for (i = 0; i < 5; i++)
	    if (!str_prefix(arg, good_names[i]))
	    {
		gameserv_command(ch, GNUM_PR, PR_CMD_TRADE, "%h%c", tgame, i);
		return;
	    }

	send_to_char("puerto trade: invalid selection\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "ship"))
    {
	int i, ship;

	argument = one_argument(argument, arg);

	if (!str_prefix(argument, "wharf"))
	    ship = 3;
	else
	{
	    if (!is_number(argument))
	    {
		send_to_char("puerto ship -- ship goods during the Captain phase\n\r\n\r", ch);
		send_to_char("usage: puerto ship <good type> <boat # or 'wharf'>\n\r", ch);
		return;
	    }

	    ship = atoi(argument);

	    if ((ship < 1) || (ship > 3))
	    {
		send_to_char("puerto ship -- ship goods during the Captain phase\n\r\n\r", ch);
		send_to_char("usage: puerto ship <good type> <boat # or 'wharf'>\n\r", ch);
		return;
	    }

	    ship--;
	}
	
	for (i = 0; i < 5; i++)
	    if (!str_prefix(arg, good_names[i]))
	    {
		gameserv_command(ch, GNUM_PR, PR_CMD_SHIP, "%h%c%c", tgame, i, ship);
		return;
	    }

	send_to_char("puerto ship: invalid selection\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "discard"))
    {
	int i;

	argument = one_argument(argument, arg);

	for (i = 0; i < 5; i++)
	    if (!str_prefix(arg, good_names[i]))
	    {
		gameserv_command(ch, GNUM_PR, PR_CMD_DISCARD, "%h%c", tgame, i);
		return;
	    }

	send_to_char("puerto discard: invalid selection\n\r", ch);
	return;
    }

    send_to_char("Invalid option.  See 'help puerto' for options.\n\r", ch);
    
    return;
}
