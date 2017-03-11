/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
*	ROM 2.4 is copyright 1993-1996 Russ Taylor			   *
*	ROM has been brought to you by the ROM consortium		   *
*	    Russ Taylor (rtaylor@efn.org)				   *
*	    Gabrielle Taylor						   *
*	    Brian Moore (zump@rom.org)					   *
*	By using this code, you have agreed to follow the terms of the	   *
*	ROM license, in the file Rom24/doc/rom.license			   *
***************************************************************************/

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#if defined(unix)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <limits.h>
#include <mysql/mysql.h>
#include <sstream>
#include <vector>
#include "merc.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "magic.h"
#include "olc.h"
#include "languages.h"
#include "dictionary.h"
#include "alchemy.h"
#include "Titles.h"
#include "PyreInfo.h"
#include "RoomPath.h"
#include "spells_spirit.h"
#include "ShadeControl.h"
#include "StatEmitter.h"
#include "Faction.h"
#include "NameMaps.h"
#include "Encumbrance.h"
#include "Luck.h"

/* command procedures needed */
DECLARE_DO_FUN(do_rstat		);
DECLARE_DO_FUN(do_mstat		);
DECLARE_DO_FUN(do_ostat		);
DECLARE_DO_FUN(do_rset		);
DECLARE_DO_FUN(do_mset		);
DECLARE_DO_FUN(do_oset		);
DECLARE_DO_FUN(do_sset		);
DECLARE_DO_FUN(do_mfind		);
DECLARE_DO_FUN(do_ofind		);
DECLARE_DO_FUN(do_slookup	);
DECLARE_DO_FUN(do_mload		);
DECLARE_DO_FUN(do_oload		);
DECLARE_DO_FUN(do_quit		);
DECLARE_DO_FUN(do_look		);
DECLARE_DO_FUN(do_stand		);
DECLARE_DO_FUN(do_mpkill	);
DECLARE_DO_FUN(do_conceal	);

DECLARE_SPELL_FUN( spell_null	);
DECLARE_SPELL_FUN( spell_form	);

extern	bool	global_bool_ranged_attack;
extern 	MYSQL	mysql;

void	smite			args( ( CHAR_DATA *ch ) );
bool    check_parse_name        args( ( char *name, bool long_name) );

void	snoop	args((CHAR_DATA *ch, char *argument, short stype));

extern	void	save_notes	args( ( int type ) );

extern	void	mob_obj_refresh	args( ( ROOM_INDEX_DATA *pRoom, CHAR_DATA *target ) );
extern	void	unbind_shunned_demon args( ( CHAR_DATA *ch ) );
extern	void	char_from_acct	args(( CHAR_DATA *ch, char *reason ));

extern	void	erase_line	args((DESCRIPTOR_DATA *d));
extern	char *	read_elemental_biases	args((struct elemental_bias_type * elem));
extern	char *	read_platonic_biases	args((struct platonic_bias_type * plat));
extern	double	evaluate_item args((OBJ_DATA * obj, struct elemental_bias_type * elem, struct platonic_bias_type * plat, int * vtot));
extern	void    sum_elements args((struct elemental_bias_type * elem1, const struct elemental_bias_type * elem2));
extern	void	sum_platonics args((struct platonic_bias_type * plat1, const struct platonic_bias_type * plat2));
extern	void	fwrite_rumors();
/*
 * Local functions.
 */
ROOM_INDEX_DATA *	find_location	args( ( CHAR_DATA *ch, OBJ_DATA *obj, char *arg ) );

void do_wiznet( CHAR_DATA *ch, char *argument )
{
    int flag;
    char buf[MAX_STRING_LENGTH];

    if ( argument[0] == '\0' )
    {
      	if (IS_SET(ch->wiznet,WIZ_ON))
      	{
            send_to_char("Signing off of Wiznet.\n\r",ch);
            REMOVE_BIT(ch->wiznet,WIZ_ON);
      	}
      	else
      	{
            send_to_char("Welcome to Wiznet!\n\r",ch);
            SET_BIT(ch->wiznet,WIZ_ON);
      	}
      	return;
    }

    if (!str_prefix(argument,"on"))
    {
	send_to_char("Welcome to Wiznet!\n\r",ch);
	SET_BIT(ch->wiznet,WIZ_ON);
	return;
    }

    if (!str_prefix(argument,"off"))
    {
	send_to_char("Signing off of Wiznet.\n\r",ch);
	REMOVE_BIT(ch->wiznet,WIZ_ON);
	return;
    }

    /* show wiznet status */
    if (!str_prefix(argument,"status")) 
    {
	buf[0] = '\0';

	if (!IS_SET(ch->wiznet,WIZ_ON))
	    strcat(buf,"off ");

	for (flag = 0; wiznet_table[flag].name != NULL; flag++)
	    if (IS_SET(ch->wiznet,wiznet_table[flag].flag))
	    {
		strcat(buf,wiznet_table[flag].name);
		strcat(buf," ");
	    }

	strcat(buf,"\n\r");

	send_to_char("Wiznet status:\n\r",ch);
	send_to_char(buf,ch);
	return;
    }

    if (!str_prefix(argument,"show"))
    /* list of all wiznet options */
    {
	buf[0] = '\0';

	for (flag = 0; wiznet_table[flag].name != NULL; flag++)
	{
	    if (wiznet_table[flag].level <= get_trust(ch))
	    {
	    	strcat(buf,wiznet_table[flag].name);
	    	strcat(buf," ");
	    }
	}

	strcat(buf,"\n\r");

	send_to_char("Wiznet options available to you are:\n\r",ch);
	send_to_char(buf,ch);
	return;
    }
   
    flag = wiznet_lookup(argument);

    if (flag == -1 || get_trust(ch) < wiznet_table[flag].level)
    {
	send_to_char("No such option.\n\r",ch);
	return;
    }
   
    if (IS_SET(ch->wiznet,wiznet_table[flag].flag))
    {
	sprintf(buf,"You will no longer see %s on wiznet.\n\r",
	        wiznet_table[flag].name);
	send_to_char(buf,ch);
	REMOVE_BIT(ch->wiznet,wiznet_table[flag].flag);
    	return;
    }
    else
    {
    	sprintf(buf,"You will now see %s on wiznet.\n\r",
		wiznet_table[flag].name);
	send_to_char(buf,ch);
    	SET_BIT(ch->wiznet,wiznet_table[flag].flag);
	return;
    }

}

void wiznet(char *string, CHAR_DATA *ch, void *arg1, long flag, long flag_skip, int min_level) 
{
    DESCRIPTOR_DATA *d;
    char buf[MAX_STRING_LENGTH];
    struct tm *loctime;

    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        if (d && d->connected == CON_PLAYING
        && d->character
        &&  IS_IMMORTAL(d->character) 
        &&  IS_SET(d->character->wiznet,WIZ_ON) 
        &&  (!flag || IS_SET(d->character->wiznet,flag))
        &&  get_trust(d->character) >= min_level
        &&  d->character != ch)
        {
            if (IS_SET(d->character->wiznet,WIZ_TIMESTAMP))
            {
                loctime = localtime(&current_time);
                strftime(buf, 128, "%H:%M:%S> ", loctime);
                send_to_char(buf, d->character);	
            }	   
            else if (IS_SET(d->character->wiznet,WIZ_PREFIX))
                send_to_char("--> ",d->character);

            if (ch != NULL && ch->in_room != NULL)
            {
                sprintf(buf, "[Room: %d] ", ch->in_room->vnum);
                send_to_char(buf, d->character);
            }

            act_nnew(string,d->character,arg1,ch,TO_CHAR,POS_DEAD);
        }
    }
}

DO_FUNC( do_fsave )
{
    CHAR_DATA *vch;

    if (argument[0] == '\0')
    {
	send_to_char("Who do you wish to force to save?\n\r", ch);
	return;
    }

    if (!str_cmp(argument, "all"))
    {
	DESCRIPTOR_DATA *d;

	for (d = descriptor_list; d; d = d->next)
	{
	    if (d->connected == CON_PLAYING)
	    {
	        if (d->original)
		    save_char_obj(d->original);
	        else if (d->character)
		    save_char_obj(d->character);
	    }
	}

	return;
    }

    if ((vch = get_char_world(ch, argument)) == NULL)
    {
	send_to_char("Target not found.\n\r", ch);
	return;
    }

    save_char_obj(vch);

    return;
}

void do_castalign( CHAR_DATA *ch, char *argument )
{
    DESCRIPTOR_DATA *d, *d_next;
    char arg[MAX_INPUT_LENGTH];
    int align;
    int sn, level;

    if (argument[0] == '\0')
    {
	send_to_char("Cast what on which alignment?\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg);

    if (argument[0] == '\0')
    {
	send_to_char("Cast which spell?\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "good"))
	align = 750;
    else if (!str_cmp(arg, "neutral"))
	align = 0;
    else if (!str_cmp(arg, "evil"))
	align = -750;
    else
    {
	send_to_char("Invalid alignment.\n\r", ch);
	send_to_char("Format: castalign <alignment> <spell>\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg);

    if ((sn = find_spell(ch, arg, NULL)) < 1
    ||  skill_table[sn].spell_fun == spell_null
    ||  skill_table[sn].spell_fun == spell_form
    || (!IS_NPC(ch) && !get_skill(ch, sn)))
    {
	send_to_char( "You don't know any spells of that name.\n\r", ch );
	return;
    }

    if ((skill_table[sn].target == TAR_IGNORE)
     || (skill_table[sn].target == TAR_CHAR_SELF)
     || (skill_table[sn].target == TAR_OBJ_INV))
    {
	send_to_char("That spell may not be cast in this manner.\n\r", ch);
	return;
    }

    if (argument[0] != '\0')
    {
	level = atoi(argument);

	if (level <= 0)
	{
	    send_to_char("Invalid level.\n\r", ch);
	    return;
	}
    }
    else
	level = ch->level;


    for (d = descriptor_list; d; d = d_next)
    {
	d_next = d->next;

	if ((d->connected == CON_PLAYING)
	 && d->character
	 && !IS_IMMORTAL(d->character)
	 && d->character->alignment == align)
	    (*skill_table[sn].spell_fun) (sn, level, ch, d->character, TARGET_CHAR);
    }

    return;
}



void do_causesegv( CHAR_DATA *ch, char *argument)
{
char x[10];
    send_to_char("You're causing a segv -- but of course, if it works, this won't show up.", ch);

    sprintf(x, "wjgiiiiii45u4986uj246j804hb62 697y2qv0gg4 37w8gyo4zxy4jgiiiiii45u4986uj246j804hb62 697y2qv0gg4 37w8gyjgiiiiii45u4986uj246j804hb62 697y2qv0gg4 37w8gyjgiiiiii45u4986uj246j804hb62 697y2qv0gg4 37w8gyjgiiiiii45u4986uj246j804hb62 697y2qv0gg4 37w8gyjgiiiiii45u4986uj246j804hb62 697y2qv0gg4 37w8gyjgiiiiii45u4986uj246j804hb62 697y2qv0gg4 37w8gy");
   send_to_char(x, ch);
}

void do_badscan(CHAR_DATA *ch, char *argument)
{
    DESCRIPTOR_DATA *d;
    CHAR_DATA *vch;
    bool check_app = FALSE;
    char buf[MAX_STRING_LENGTH];

    if ((argument[0] != '\0') && !str_cmp(argument, "app"))
	check_app = TRUE;

    for (d = descriptor_list; d; d = d->next)
    {
	vch = d->original ? d->original : d->character;

	if ((d->connected == CON_PLAYING) && !IS_NPC(vch))
	{
	    if (vch->level > 10)
	    {
		if (!vch->description)
		{
		    sprintf(buf, "%s has no description. (Level %d)\n\r", vch->name, vch->level);
		    send_to_char(buf, ch);
		}
		else if (check_app && !IS_SET(vch->pcdata->pcdata_flags, PCD_APP_DESC))
		{
		    sprintf(buf, "%s's description has not been approved.\n\r", vch->name);
		    send_to_char(buf, ch);
		}
	    }

	    if (vch->level > 25)
	    {
		if (!vch->pcdata->background)
		{
		    sprintf(buf, "%s has no background. (Level %d)\n\r", vch->name, vch->level);
		    send_to_char(buf, ch);
		}
		else if (check_app && !IS_SET(vch->pcdata->pcdata_flags, PCD_APP_BG))
		{
		    sprintf(buf, "%s's background has not been approved.\n\r", vch->name);
		    send_to_char(buf, ch);
		}
	    }
	}
    }

    return;
}



void do_approve(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    argument = one_argument(argument, arg);

    if ((argument[0] == '\0') || (arg[0] == '\0'))
    {
	send_to_char("Syntax: approve <bg/desc> <char>\n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, argument)) == NULL)
    {
	send_to_char("Player not found.\n\r", ch);
	return;
    }

    if (IS_NPC(victim))
    {
	send_to_char("You can only approve PCs.\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "bg"))
    {
	if (IS_SET(victim->pcdata->pcdata_flags, PCD_APP_BG))
	{
	    send_to_char("Background no longer approved.\n\r", ch);
	    REMOVE_BIT(victim->pcdata->pcdata_flags, PCD_APP_BG);
	    sprintf(log_buf, "%s unapproved %s's background.", ch->name, victim->name);
	    log_string(log_buf);
	    wiznet(log_buf, ch, NULL, WIZ_ACTIVITY, 0, 0);
	}
	else
	{
	    send_to_char("Background approved.\n\r", ch);
	    SET_BIT(victim->pcdata->pcdata_flags, PCD_APP_BG);
	    sprintf(log_buf, "%s approved %s's background.", ch->name, victim->name);
	    log_string(log_buf);
	    wiznet(log_buf, ch, NULL, WIZ_ACTIVITY, 0, 0);
	}
    }
    else if (!str_cmp(arg, "desc"))
    {
	if (IS_SET(victim->pcdata->pcdata_flags, PCD_APP_DESC))
	{
	    send_to_char("Description no longer approved.\n\r", ch);
	    REMOVE_BIT(victim->pcdata->pcdata_flags, PCD_APP_DESC);
	    sprintf(log_buf, "%s unapproved %s's description.", ch->name, victim->name);
	    log_string(log_buf);
	    wiznet(log_buf, ch, NULL, WIZ_ACTIVITY, 0, 0);
	}
	else
	{
	    send_to_char("Description approved.\n\r", ch);
	    SET_BIT(victim->pcdata->pcdata_flags, PCD_APP_DESC);
	    sprintf(log_buf, "%s approved %s's description.", ch->name, victim->name);
	    log_string(log_buf);
	    wiznet(log_buf, ch, NULL, WIZ_ACTIVITY, 0, 0);
	}
    }
    else
    {
	do_approve(ch, "");
	return;
    }

    return;
}
   



void do_showbit( CHAR_DATA *ch, char *argument)
{
    char arg[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    int bit;

    if (argument[0] == '\0')
    {
	send_to_char("syntax: showbit <char> <bitnumber>\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg);

    if ((victim = get_char_room(ch, arg)) == NULL)
      if ((victim = get_char_world(ch, arg)) == NULL)
      {
	send_to_char("You can't find them.\n\r", ch);
	return;
      }

    if (IS_NPC(victim))
    {
	send_to_char("Not on NPCs!\n\r", ch);
	return;
    }

    if ((bit = atoi(argument)) < 1)
    {
	send_to_char("Invalid bit.\n\r", ch);
	return;
    }

    if (bit > 65535)
    {
	send_to_char("Invalid bit.\n\r", ch);
	return;
    }

    if (BIT_GET(victim->pcdata->bitptr, bit))
	send_to_char("Bit is set to TRUE.\n\r", ch);
    else
	send_to_char("Bit is set to FALSE.\n\r", ch);

    return;
}

void do_flipbit( CHAR_DATA *ch, char *argument)
{
    char arg[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    int bit;

    if (argument[0] == '\0')
    {
	send_to_char("syntax: flipbit <char> <bitnumber>\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg);

    if ((victim = get_char_room(ch, arg)) == NULL)
      if ((victim = get_char_world(ch, arg)) == NULL)
      {
	send_to_char("You can't find them.\n\r", ch);
	return;
      }

    if (IS_NPC(victim))
    {
	send_to_char("Not on NPCs!\n\r", ch);
	return;
    }

    if ((bit = atoi(argument)) < 1)
    {
	send_to_char("Invalid bit.\n\r", ch);
	return;
    }

    if (bit > 65535)
    {
	send_to_char("Invalid bit.\n\r", ch);
	return;
    }

    BIT_FLIP(victim->pcdata->bitptr, bit);

    return;
}




void do_astrip( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    OBJ_DATA *vobj;
    ROOM_INDEX_DATA *vRoom;
    AFFECT_DATA *af;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int sn = 0;

    argument = one_argument( argument, arg1 );

    if ( arg1[0] == '\0' )
    {
	send_to_char("Usage: astrip <mob/char name> [affect name]\n\r", ch);
	send_to_char("       astrip mobile/char <mob/char name> [affect name]\n\r", ch);
	send_to_char("       astrip object <object name> [affect name]\n\r", ch);
	send_to_char("       astrip room [target room] [affect name]\n\r", ch);
	send_to_char("       astrip zone [affect name]\n\r", ch);
	return;
    }

    if (!str_prefix(arg1, "mobile") || !str_cmp(arg1, "char"))
	argument = one_argument(argument, arg1);

    argument = one_argument( argument, arg2 );

    if (!str_prefix(arg1, "object"))
    {
	if ((vobj = get_obj_world(ch, arg2)) == NULL)
	{
	    send_to_char("Object not found.\n\r", ch);
	    return;
	}

	if (argument[0] == '\0')
	{
	    while (af = vobj->affected)
		affect_remove_obj(vobj, af);

	    send_to_char("All affects stripped.\n\r", ch);
	}
	else
	{
	    if ((sn = skill_lookup(argument)) == -1)
	    {
		send_to_char("Invalid skill.\n\r", ch);
		return;
	    }

	    for (af = vobj->affected; af; af = af->next)
		if (af->type == sn)
		{
		    object_affect_strip(vobj, sn);
		    sprintf(buf, "Affect '%s' stripped.\n\r", skill_table[sn].name);
		    send_to_char(buf, ch);
		    break;
		}
	}

	return;
    }

    if (!str_cmp(arg1, "room"))
    {
	if (arg2[0] == '\0')
	    vRoom = ch->in_room;
	else if ((vRoom = find_location(ch, NULL, arg2)) == NULL)
	{
	    if ((sn = skill_lookup(argument)) == -1)
	    {
		send_to_char("Room or skill not found.\n\r", ch);
	        return;
	    }
	}

	if (sn == 0 && argument[0] == '\0')
	{
	    while (af = vRoom->affected)
		affect_remove_room(vRoom, af);

	    sprintf(buf, "All affects stripped from room %d.\n\r", vRoom->vnum);
	    send_to_char(buf, ch);
	}
	else
	{
	    if ((sn == 0) && ((sn = skill_lookup(argument)) == -1))
	    {
		send_to_char("Invalid skill.\n\r", ch);
		return;
	    }

	    for (af = vRoom->affected; af; af = af->next)
		if (af->type == sn)
		{
		    room_affect_strip(vRoom, sn);
		    sprintf(buf, "Affect '%s' stripped from room %d.\n\r", skill_table[sn].name, vRoom->vnum);
		    send_to_char(buf, ch);
		    break;
		}
	}

	return;
    }

    if (!str_cmp(arg1, "zone") || !str_cmp(arg1, "area"))
    {
	if (!ch->in_room || !ch->in_room->area)
	{
	    send_to_char("Not currently in an area, astrip aborted.\n\r", ch);
	    return;
	}

	if (argument[0] == '\0')
	{
	    while (af = ch->in_room->area->affected)
		affect_remove_area(ch->in_room->area, af);

	    send_to_char("All affects stripped from area.\n\r", ch);
	}
	else
	{
	    if ((sn = skill_lookup(argument)) == -1)
	    {
		send_to_char("Invalid skill.\n\r", ch);
		return;
	    }

	    for (af = ch->in_room->area->affected; af; af = af->next)
		if (af->type == sn)
		{
		    area_affect_strip(ch->in_room->area, sn);
		    sprintf(buf, "Affect '%s' stripped from area.\n\r", skill_table[sn].name);
		    send_to_char(buf, ch);
		    break;
		}
	}

	return;
    }

    if ( ( victim = get_char_room( ch, arg1 ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }

    if ( !IS_NPC(victim) && victim->level > get_trust(ch) )
    {
        send_to_char( "You failed.\n\r", ch );
        return;
    }

    if ( arg2[0] == '\0' )
    {

	if (IS_NAFFECTED(victim, AFF_FLESHTOSTONE)
	 || IS_OAFFECTED(victim, AFF_ENCASE))
	    REMOVE_BIT(victim->act, PLR_FREEZE);

        while ((af = victim->affected) != NULL)
            affect_strip(victim,af->type);

       send_to_char( "All affects stripped.\n\r", ch);
    } 
    else if (is_affected(victim, sn=skill_lookup(arg2)))
    {
	affect_strip(victim,sn);
	sprintf(buf, "Affect '%s' stripped.\n\r", skill_table[sn].name);
        send_to_char(buf, ch);
    } 
    else
        send_to_char( "Affect not found.\n\r", ch);

    return;
}

void do_hostlist( CHAR_DATA *ch, char *argument )
{
    HOST_DATA *pHost;
    char buf[MAX_STRING_LENGTH];

    if (!host_list)
    {
	sprintf(buf, "No mortals have disconnected in the past %d seconds.\n\r", HOST_TIME_WARNING);
	send_to_char(buf, ch);
	return;
    }

    send_to_char("Time Character            Host\n\r", ch);

    for (pHost = host_list; pHost; pHost = pHost->next)
    {
	sprintf(buf, "%-4d %-19s %-50s\n\r", (int) (current_time - pHost->lastoff), pHost->char_name, pHost->host_name);
	send_to_char(buf, ch);
    }

    return;
}

void do_guild( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH],arg3[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int clan;
    bool shunned_warrior = TRUE;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    if ((ch->level < 52) && (!(ch->act & PLR_INDUCT)))
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
        send_to_char( "Syntax: induct <char> <house name>\n\r",ch);
        return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        send_to_char( "They aren't playing.\n\r", ch );
        return;
    }

    if (!str_prefix(arg2,"none"))
    {
	if ((ch->level < 52) && (ch->clan != victim->clan))
	{
	    send_to_char("You cannot remove someone from another house!\n\r", ch);
	    return;
	}

	send_to_char("They are now without a house.\n\r",ch);
	send_to_char("You are now without a house.\n\r",victim);

	if (!IS_NPC(victim))
	{
	    if (victim->clan == clan_lookup("SHUNNED"))
	    {
		group_remove(victim, "SHUNNED2");
		BIT_CLEAR(victim->pcdata->bitptr, ROOM_VAULT_SHUNNED);
		if (is_affected(victim, gsn_shadow_ward))
		{
		    affect_strip(victim, gsn_shadow_ward);
		    send_to_char("The wards of the Halls no longer protect you.\n\r",victim);
		}
	    }
	    group_remove(victim, clan_table[victim->clan].who_name);
	}
        
	if (!IS_NPC(victim) && (victim->clan == clan_lookup("CHAMPION")))
    {
        modify_karma(victim, 2500);
	    victim->pcdata->karma = UMIN(victim->pcdata->karma, PALEGOLDENAURA);
	    if (is_affected(victim, gsn_strength_of_Aramril))
	    {
	        affect_strip(victim, gsn_strength_of_Aramril);
		    send_to_char("The strength of Aramril fades as you leave the Tower.\n\r",victim);
	    }
    } 

	victim->clan = 0;
	return;
    }

    if ((clan = clan_lookup(arg2)) == 0)
    {
	send_to_char("No such house exists.\n\r",ch);
	return;
    }

    /* SHUNNED hack.  I hate the world.  Particularly Synrael.  No wait.. */
    /* Jolinn.  Well.  Both of them...    -Erinos (1978-)                 */
    if (clan == clan_lookup("SHUNNED"))
    {
	if (arg3[0] == '\0')
	{
	    act("Which branch of the Coven of the Shunned do you wish to induct $M into?", ch, NULL, victim, TO_CHAR);
	    return;
	}
	else if (!str_cmp(arg3, "warrior"))
	    shunned_warrior = TRUE;
	else if (!str_cmp(arg3, "scholar"))
	    shunned_warrior = FALSE;
	else
	{
	   send_to_char("Valid choices for branch are: warrior scholar\n\r", ch);
	   return;
	}
    }
	
    if ((ch->level < 52) && (ch->clan != clan) )
    {
	send_to_char("You're not part of that house!\n\r",ch);
	return;
    }

    if (clan_table[clan].independent)
    {
	sprintf(buf,"They are now a %s.\n\r",clan_table[clan].name);
	send_to_char(buf,ch);
	sprintf(buf,"You are now a %s.\n\r",clan_table[clan].name);
	send_to_char(buf,victim);
    }
    else
    {
	sprintf(buf,"%s has been inducted into %s.\n\r",
	victim->name, capitalize(clan_table[clan].name));
	send_to_char(buf,ch);
	sprintf(buf,"You have been inducted into %s.\n\r",
	    capitalize(clan_table[clan].name));
	send_to_char(buf,victim);
    }

    if (clan == clan_lookup("SHUNNED"))
    {
        if (!IS_NPC(victim))
	    if (!shunned_warrior)
	    {
	        BIT_SET(victim->pcdata->bitptr, ROOM_VAULT_SHUNNED);
	        group_add(victim, "SHUNNED2", TRUE);
 	    }
	    else
    	        group_add(victim, clan_table[clan].who_name, TRUE);
 	if (victim->in_room->clan == clan_lookup("SHUNNED"))
	{
	    if (is_affected(victim, gsn_shadow_ward))
                affect_strip(victim, gsn_shadow_ward);
	    AFFECT_DATA naf;
	    naf.type = gsn_shadow_ward;
	    naf.duration = -1;
	    naf.valid = TRUE;
	    naf.point = NULL;
	    naf.where = TO_AFFECTS;
	    naf.location = APPLY_NONE;
	    naf.modifier = 0;
	    naf.bitvector = 0;
	    naf.level = 60;
	    affect_to_char(victim, &naf);
	    send_to_char("The wards of the Halls attune themselves to your presence.\n\r",victim);
	}
    }
    else
        group_add(victim,clan_table[clan].who_name, TRUE);

    if (!IS_NPC(victim) && (clan == clan_lookup("CHAMPION")) && (victim->clan != clan_lookup("CHAMPION")))
        modify_karma(victim, -2500);

    victim->clan = clan;
}

/* equips a character */
void do_outfit ( CHAR_DATA *ch, char *argument )
{
/*
    OBJ_DATA *obj;
    int i,sn,vnum;

    if (ch->level > 5 || IS_NPC(ch))
    {
	send_to_char("Find it yourself!\n\r",ch);
	return;
    }

    if ( ( obj = get_eq_char( ch, WEAR_LIGHT ) ) == NULL )
    {
        obj = create_object( get_obj_index(OBJ_VNUM_SCHOOL_BANNER), 0 );
	obj->cost = 0;
        obj_to_char( obj, ch );
        equip_char( ch, obj, WORN_LIGHT );
	oprog_wear_trigger(obj);
    }
 
    if ( ( obj = get_eq_char( ch, WEAR_BODY ) ) == NULL )
    {
	obj = create_object( get_obj_index(OBJ_VNUM_SCHOOL_VEST), 0 );
	obj->cost = 0;
        obj_to_char( obj, ch );
        equip_char( ch, obj, WORN_BODY );
	oprog_wear_trigger(obj);
    }

    if ((obj = get_eq_char(ch,WEAR_WIELD)) == NULL)
    {
    	sn = 0; 
    	vnum = OBJ_VNUM_SCHOOL_DAGGER;


	if (ch->class_num > 5 && ch->class_num < 12)
		vnum = OBJ_VNUM_SCHOOL_SWORD;

	if (ch->class_num > 17 && ch->class_num < 24)
		vnum = OBJ_VNUM_SCHOOL_SWORD;


    	for (i = 0; weapon_table[i].name != NULL; i++)
    	{
	    if (ch->pcdata->learned[sn] < 
		ch->pcdata->learned[*weapon_table[i].gsn])
	    {
	    	sn = *weapon_table[i].gsn;
	    	vnum = weapon_table[i].vnum;
	    }
    	}

    	obj = create_object(get_obj_index(vnum),0);
     	obj_to_char(obj,ch);
    	equip_char(ch,obj,WORN_WIELD);
	oprog_wear_trigger(obj);
    }

    if (((obj = get_eq_char(ch,WEAR_WIELD)) == NULL 
    ||   !IS_WEAPON_STAT(obj,WEAPON_TWO_HANDS)) 
    &&  (obj = get_eq_char( ch, WEAR_SHIELD ) ) == NULL )
    {
        obj = create_object( get_obj_index(OBJ_VNUM_SCHOOL_SHIELD), 0 );
	obj->cost = 0;
        obj_to_char( obj, ch );
    }

    send_to_char("You have been equipped by the gods.\n\r",ch);
*/
}


void do_alinks( CHAR_DATA *ch, char *argument )
{
    ALINK_DATA *pLink;
    char buf[MAX_STRING_LENGTH];

    if (!alink_first)
	return;

    pLink = alink_first;
    while (pLink)
    {
	sprintf(buf, "%30s <- %5s/%5s -> %s\n\r", pLink->a1->name, 
	    ((pLink->dir1 == 0) ? "north" :
             (pLink->dir1 == 1) ? " east" :
             (pLink->dir1 == 2) ? "south" :
	     (pLink->dir1 == 3) ? " west" :
	     (pLink->dir1 == 4) ? "   up" : 
	     (pLink->dir1 == 5) ? " down" : "?????" ),
	    ((pLink->dir2 == 0) ? "north" :
             (pLink->dir2 == 1) ? "east " :
             (pLink->dir2 == 2) ? "south" :
	     (pLink->dir2 == 3) ? "west " :
	     (pLink->dir2 == 4) ? "up   " : 
	     (pLink->dir2 == 5) ? "down " : "?????" ),
	    pLink->a2->name);
	send_to_char(buf, ch);
	pLink = pLink->next;
    }
    return;
}

     
/* RT nochannels command, for those spammers */
void do_nochannels( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
 
    one_argument( argument, arg );
 
    if ( arg[0] == '\0' )
    {
        send_to_char( "Nochannel whom?", ch );
        return;
    }
 
    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }
 
    if ( get_trust( victim ) >= get_trust( ch ) )
    {
        send_to_char( "You failed.\n\r", ch );
        return;
    }
 
    if ( IS_SET(victim->comm, COMM_NOCHANNELS) )
    {
        REMOVE_BIT(victim->comm, COMM_NOCHANNELS);
        send_to_char( "You can speak again.\n\r", victim );
        send_to_char( "NOCHANNELS removed.\n\r", ch );
	sprintf(buf,"$N restores channels to %s",victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }
    else
    {
        SET_BIT(victim->comm, COMM_NOCHANNELS);
        send_to_char( "Your tongue is frozen!\n\r", victim );
        send_to_char( "NOCHANNELS set.\n\r", ch );
	sprintf(buf,"$N revokes %s's channels.",victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }
 
    return;
}

void do_bamfin( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

    if ( !IS_NPC(ch) )
    {
	smash_tilde( argument );

	if (argument[0] == '\0')
	{
	    sprintf(buf,"Your poofin is %s\n\r",ch->pcdata->bamfin);
	    send_to_char(buf,ch);
	    return;
	}

	if ( strstr(argument,ch->name) == NULL)
	{
	    send_to_char("You must include your name.\n\r",ch);
	    return;
	}
	     
	free_string( ch->pcdata->bamfin );
	ch->pcdata->bamfin = str_dup( argument );

        sprintf(buf,"Your poofin is now %s\n\r",ch->pcdata->bamfin);
        send_to_char(buf,ch);
    }
    return;
}



void do_bamfout( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
 
    if ( !IS_NPC(ch) )
    {
        smash_tilde( argument );
 
        if (argument[0] == '\0')
        {
            sprintf(buf,"Your poofout is %s\n\r",ch->pcdata->bamfout);
            send_to_char(buf,ch);
            return;
        }
 
        if ( strstr(argument,ch->name) == NULL)
        {
            send_to_char("You must include your name.\n\r",ch);
            return;
        }
 
        free_string( ch->pcdata->bamfout );
        ch->pcdata->bamfout = str_dup( argument );
 
        sprintf(buf,"Your poofout is now %s\n\r",ch->pcdata->bamfout);
        send_to_char(buf,ch);
    }
    return;
}

extern void move_header(CHAR_DATA *ch, bool to_denied);
extern void remove_header(HEADER_DATA *header);

void delete_char(CHAR_DATA *victim)
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;

    // Correct counts.
    player_levels -= victim->level;

    for (obj = victim->carrying; obj; obj = obj->next_content)
	--obj->pIndexData->current;

    sprintf(buf, "%s%s", PLAYER_DIR, victim->name);
    remove(buf);
    sprintf(buf, "%s%s", TRAVELPATH, victim->name);
    remove(buf);
    sprintf(buf, "%s%s", MEMPATH, victim->name);
    remove(buf);

    if (!victim->pcdata->header)
		victim->pcdata->header = (HEADER_DATA*) dict_lookup(g_char_names, victim->name);

    if (victim->pcdata->header)
	remove_header(victim->pcdata->header);

    extract_char(victim, TRUE);
}

void deny_char( CHAR_DATA *victim )
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;

    if (IS_NPC(victim))
        return;

    SET_BIT(victim->act, PLR_DENY);

    // First, remove the pfile from the player directory.
    sprintf(buf, "%s%s", PLAYER_DIR, victim->name);
    remove(buf);
   
    // Now add the name to the badname list;
    move_header(victim, TRUE);

    // Correct counts
    player_levels -= victim->level;
    for (obj = victim->carrying; obj; obj = obj->next_content)
        --obj->pIndexData->current;

    save_char_obj(victim);

    sprintf(buf, "%s%s", TRAVELPATH, victim->name);
    remove(buf);
    sprintf(buf, "%s%s", MEMPATH, victim->name);
    remove(buf);

    stop_fighting_all(victim);
    extract_char(victim, TRUE);
}

extern HEADER_DATA *add_denied(char *name);
extern void remove_denied(HEADER_DATA *header);

void do_undeny(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    HEADER_DATA *hdp;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Undeny whom?\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "name"))
    {
	if ((hdp = (HEADER_DATA*)dict_lookup(g_denied_names, argument)) == NULL)
	{
	    send_to_char("That name is not currently denied.\n\r", ch);
	    return;
	}

	remove_denied(hdp);
	sprintf(arg, "Name '%s' undenied.\n\r", argument);
	send_to_char(arg, ch);
	return;
    }

    send_to_char("Current usage: undeny name <name>\n\r", ch);
    return;
}

void do_deny( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    HEADER_DATA *hdp;
    bool deny_account = FALSE;

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Deny whom?\n\r", ch );
	return;
    }

    if (!str_cmp(arg, "account") || !str_cmp(arg, "acct"))
    {
	deny_account = TRUE;
	if ((victim = get_char_world(ch, argument)) == NULL)
	{
	    send_to_char("They aren't here.\n\r", ch);
	    return;
	}

	if (!victim->desc || !victim->desc->acct)
	{
	    send_to_char("That character it not associated with an account.\n\r", ch);
	    return;
	}
    }
    else if (!str_cmp(arg, "name"))
    {
	if (hdp = (HEADER_DATA*)dict_lookup(g_denied_names, argument))
	{
	    send_to_char("That name is already denied.\n\r", ch);
	    return;
	}

	add_denied(argument);

	sprintf(buf, "Name '%s' denied.\n\r", argument);
	send_to_char(buf, ch);
	return;
    }
    else if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n\r", ch );
	return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
	send_to_char( "You failed.\n\r", ch );
	return;
    }

    if (deny_account)
    {
	SET_BIT(victim->desc->acct->flags, ACCT_DENY);
	send_to_char("Your account has been denied access!\n\r", victim);
	sprintf(buf, "$N denies access to %s's account.", victim->name);
    }
    else
    {
	send_to_char( "You are denied access!\n\r", victim );
        sprintf(buf,"$N denies access to %s.",victim->name);
    }

    wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);

    sprintf(buf, "denied by %s", ch->name);
    act("$N is now denied.",ch,NULL,victim,TO_CHAR);

    char_from_acct(victim, buf);

    deny_char(victim);

    return;
}



void do_disconnect( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Disconnect whom?\n\r", ch );
	return;
    }

    if (is_number(arg))
    {
	unsigned int desc;

	desc = atoi(arg);
    	for ( d = descriptor_list; d != NULL; d = d->next )
    	{
            if ( d->descriptor == desc )
            {
            	close_socket( d );
            	send_to_char( "Ok.\n\r", ch );
            	return;
            }
	}
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }
 
    if (ch->trust <= victim->trust)
	{
	send_to_char("They are too well trusted for you to disconnect.\n\r", ch);
	return;
	}

    if ( victim->desc == NULL )
    {
	act( "$N doesn't have a descriptor.", ch, NULL, victim, TO_CHAR );
	return;
    }

    for ( d = descriptor_list; d != NULL; d = d->next )
    {
	if ( d == victim->desc )
	{
	    close_socket( d );
	    send_to_char( "Ok.\n\r", ch );
	    return;
	}
    }

    bug( "Do_disconnect: desc not found.", 0 );
    send_to_char( "Descriptor not found!\n\r", ch );
    return;
}



void do_pardon( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
	send_to_char( "Syntax: pardon <character> <killer|thief>.\n\r", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n\r", ch );
	return;
    }

    if ( !str_cmp( arg2, "killer" ) )
    {
	if ( IS_SET(victim->act, PLR_REWARD) )
	{
	    REMOVE_BIT( victim->act, PLR_REWARD );
	    send_to_char( "Reward flag removed.\n\r", ch );
	    send_to_char( "There is no longer a reward on your death.\n\r", victim );
	}
	return;
    }

    send_to_char( "Syntax: pardon <character> <killer|thief>.\n\r", ch );
    return;
}



void do_lgecho( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    char buf[MAX_STRING_LENGTH];
    int low, high;
    
    if ( argument[0] == '\0' )
    {
	send_to_char( "lgecho <low> <high> <message>\n\r", ch );
	return;
    }

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    
    if ( argument[0] == '\0' )
    {
	send_to_char( "lgecho <low> <high> <message>\n\r", ch );
	return;
    }

    if ((low = atoi(arg1)) < 1)
	return;

    if ((high = atoi(arg2)) < 1)
	return;

    for ( d = descriptor_list; d; d = d->next )
    {
	if ( d->connected == CON_PLAYING )
	{
	    if (!IS_IMM_TRUST(d->character) && d->character->level < low && d->character->level > high)
		continue;
	    if (get_trust(d->character) >= get_trust(ch))
		{
		sprintf(buf, "[%d to %d] %s: global> ", low, high, ch->name);
		send_to_char( buf ,d->character);
		}
	    send_to_char( argument, d->character );
	    send_to_char( "\n\r",   d->character );

	    sprintf(buf, "(lgecho) %s\n\r", argument);
	    check_comm(d->character, buf);
	}
    }

    return;
}

void do_agecho( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    char buf[MAX_STRING_LENGTH];
    int align;
    
    if ( argument[0] == '\0' )
    {
	send_to_char( "agecho <align> <message>\n\r", ch );
	return;
    }

    argument = one_argument(argument, arg1);

    if ( argument[0] == '\0' )
    {
	send_to_char( "agecho <align> <message>\n\r", ch );
	return;
    }

    if (!strcmp(arg1, "good")) align = 0;
    else if (!strcmp(arg1, "evil")) align = 1;
    else if (!strcmp(arg1, "neutral")) align = 2;
    else 
	{
	send_to_char( "agecho <align> <message>\n\r", ch );
	return;
        }
    
    for ( d = descriptor_list; d; d = d->next )
    {
	if ( d->connected == CON_PLAYING )
	{
	    if (align == 0 && !IS_GOOD(d->character) && !IS_IMM_TRUST(d->character))
		continue;
	    if (align == 1 && !IS_EVIL(d->character) && !IS_IMM_TRUST(d->character))
		continue;
	    if (align == 2 && !IS_NEUTRAL(d->character) && !IS_IMM_TRUST(d->character))
		continue;
	    if (get_trust(d->character) >= get_trust(ch))
		{
		sprintf(buf, "[%s] %s: global> ", arg1, ch->name);
		send_to_char( buf ,d->character);
		}
	    send_to_char( argument, d->character );
	    send_to_char( "\n\r",   d->character );

	    sprintf(buf, "(agecho) %s\n\r", argument);
	    check_comm(d->character, buf);
	}
    }

    return;
}

void do_echo( CHAR_DATA *ch, char *argument )
{
    DESCRIPTOR_DATA *d;
    char buf[MAX_STRING_LENGTH];
    
    if ( argument[0] == '\0' )
    {
	send_to_char( "Global echo what?\n\r", ch );
	return;
    }
    
    for ( d = descriptor_list; d; d = d->next )
    {
	if ( d->connected == CON_PLAYING )
	{
	    if (get_trust(d->character) >= get_trust(ch) && IS_IMM_TRUST(d->character))
	    {
		sprintf(buf, "%s: global> ", ch->name);
		send_to_char( buf ,d->character);
	    }
	    send_to_char( argument, d->character );
	    send_to_char( "\n\r",   d->character );

	    sprintf(buf, "(gecho) %s\n\r", argument);
	    check_comm(d->character, buf);
	}
    }

    return;
}

void do_materials( CHAR_DATA *ch, char *argument)
{
int x;


for (x = 0; x < MAX_MATERIALS; x++)
	{
	send_to_char(material_table[x].name, ch);
	send_to_char(" ", ch);
	}
send_to_char("\n\r", ch);

}


void do_cgen(CHAR_DATA *ch, char *argument)
{
OBJ_DATA *crystal;
int x;
char buf[MAX_STRING_LENGTH];

	for (x = 0; x < MAX_SKILL; x++)
	{
	if (skill_table[x].name && skill_table[x].target == TAR_CHAR_OFFENSIVE)
		{
		crystal = create_object(get_obj_index(OBJ_VNUM_MAGIC_CRYSTAL), ch->level);
        	sprintf(buf, crystal->short_descr, skill_table[x].name);
        	free_string(crystal->short_descr);
        	crystal->short_descr = str_dup(buf);
        	crystal->value[0] = ch->level;
        	crystal->value[1] = 1;
        	crystal->value[2] = 1;
        	crystal->value[3] = x;
		crystal->level = ch->level;
        	obj_to_char(crystal, ch);
		}
	}
}



void do_rcheck( CHAR_DATA *ch, char *argument)
{
char buf[MAX_STRING_LENGTH];
int count, x, y, z;
int p;
int d;
int l, f;

x = y = z = p = l = f = 0;

d = 50;

	send_to_char("Initializing Jolinn's bitch locator, v1.0.\n\r", ch);
	send_to_char("Generating 10,000 random numbers, 1 to 100: ", ch);

	for (count = 0; count < 10000; count++)
	{
	x += z = number_percent();

	if (z < d && d - z < 5)
	{p++; l++;}
	else if (d < z && z - d < 5)
	{p++; l++;}
	else if (d == z)
	l++;
	else
	l = 0;
	
	f = UMAX(f, l);
	
	if (z>50)
	y += (z - 50);
	else
	y += (50 - z);
	d = z;
	}

	send_to_char("Done.\n\r", ch);

	sprintf(buf, "Total of all numbers: %d\n\r", x);
	send_to_char(buf, ch);
	
	sprintf(buf, "Average: %d\n\r", x/10000);
	send_to_char(buf, ch);

	sprintf(buf, "Total deviation from 50: %d\n\r", y);
	send_to_char(buf, ch);

	sprintf(buf, "Standard Deviation: %d\n\r", y/10000);
	send_to_char(buf, ch);

	sprintf(buf, "Total occurences of numbers falling within 5 of the last: %d\n\r", p);
	send_to_char(buf, ch);

	sprintf(buf, "Largest String of repeated numbers: %d\n\r", f);
	send_to_char(buf, ch);

	sprintf(buf, "Odds of Jolinn scoring: %d in 1000000", number_range(1, 99));
	send_to_char(buf, ch);

	sprintf(buf, "\n\rJoin us next time, as Ashur whoops more ass.\n\r");
	send_to_char(buf, ch);
}

void do_whodata( CHAR_DATA *ch, char *argument )
{
    if (IS_NPC(ch))
	return;

    if (IS_SET(ch->nact, PLR_SHOWDATA))
    {
	send_to_char("You will no longer see data flags in who lists.\n\r", ch);
	REMOVE_BIT(ch->nact, PLR_SHOWDATA);
    }
    else
    {
	send_to_char("You will now see data flags in who lists.\n\r", ch);
	SET_BIT(ch->nact, PLR_SHOWDATA);
    }

    return;
}	

void do_recho( CHAR_DATA *ch, char *argument )
{
    DESCRIPTOR_DATA *d;
    char log_buf[MAX_STRING_LENGTH];
    
    if ( argument[0] == '\0' )
    {
	send_to_char( "Local echo what?\n\r", ch );

	return;
    }

    for ( d = descriptor_list; d; d = d->next )
    {
	if ( d->connected == CON_PLAYING
	&&   d->character->in_room == ch->in_room )
	{
            if (get_trust(d->character) >= get_trust(ch))
                send_to_char( "local> ",d->character);
	    send_to_char( argument, d->character );
	    send_to_char( "\n\r",   d->character );

	    sprintf(log_buf, "(recho) %s\n\r", argument);
	    check_comm(d->character, argument);
	}
    }

    return;
}

void do_zecho(CHAR_DATA *ch, char *argument)
{
    DESCRIPTOR_DATA *d;
    char log_buf[MAX_STRING_LENGTH];
    int t;

    if (argument[0] == '\0')
    {
	send_to_char("Zone echo what?\n\r",ch);
	return;
    }

    for (d = descriptor_list; d; d = d->next)
    {
	if (d->connected == CON_PLAYING
	&&  d->character->in_room != NULL && ch->in_room != NULL
	&&  d->character->in_room->area == ch->in_room->area)
	{
	    if ((t = get_trust(d->character)) >= get_trust(ch) && t > 51)
		send_to_char("zone> ",d->character);
	    send_to_char(argument,d->character);
	    send_to_char("\n\r",d->character);

	    sprintf(log_buf, "(zecho) %s\n\r", argument);
	    check_comm(d->character, log_buf);
	}
    }
}

void do_pecho( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH], log_buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    argument = one_argument(argument, arg);
 
    if ( argument[0] == '\0' || arg[0] == '\0' )
    {
	send_to_char("Personal echo what?\n\r", ch); 
	return;
    }
   
    if  ( (victim = get_char_world(ch, arg) ) == NULL )
    {
	send_to_char("Target not found.\n\r",ch);
	return;
    }

    if (get_trust(victim) >= get_trust(ch) && get_trust(ch) != MAX_LEVEL)
        send_to_char( "personal> ",victim);

    send_to_char(argument,victim);
    send_to_char("\n\r",victim);

    sprintf(log_buf, "I: (pecho) %s\n\r", argument);
    check_comm(victim, log_buf);

    send_to_char( "personal> ",ch);
    send_to_char(argument,ch);
    send_to_char("\n\r",ch);

    sprintf(log_buf, "O: (pecho) %s\n\r", argument);
    check_comm(ch, log_buf);
}


ROOM_INDEX_DATA *find_location( CHAR_DATA *ch, OBJ_DATA *obj, char *arg )
{
    CHAR_DATA *victim;
    OBJ_DATA *pObj;

    if ( is_number(arg) )
	return get_room_index( atoi( arg ) );

    if (ch)
    {
	if ( ( victim = get_char_world( ch, arg ) ) != NULL )
	    return victim->in_room;

        if ( ( pObj = get_obj_world( ch, arg ) ) != NULL )
	    return pObj->in_room;
    }
    else if (obj)
    {
	if ( ( victim = obj_get_char_world(obj, arg ) ) != NULL )
	    return victim->in_room;

        if ( ( pObj = obj_get_obj_world(obj, arg ) ) != NULL )
	    return pObj->in_room;
    }

    return NULL;
}


void do_fwap( CHAR_DATA *ch, char *argument )
{
CHAR_DATA *victim;

if (strcmp(ch->name, "Kestrel") || IS_NPC(ch))
	{
	send_to_char("Don't even try to fwap. You don't have the {Wskillz, fool.\n\r", ch);
	return;
	}

if (argument[0] == '\0')
	{
	send_to_char("Fwap who?\n\r", ch);
	return;
	}

if ((victim = get_char_room(ch, argument)) == NULL)
	{
	send_to_char("They aren't here.\n\r", ch);
	return;
	}

if (strcmp(victim->name, "Chadim") == 0)
{
	send_to_char("Kestrel tries to kill you, but dies instead.\n\r", victim);
	send_to_char("You are, indeed, a code-abuser. Pay for your crimes.\n\r", ch);
	raw_kill(ch);
}
else
{
send_to_char("You have angered Kestrel.  That's bad...\n\r", victim);
send_to_char("{RKestrel crushes you with a single, wrathful thought!{n\n\r", victim);
act("{R$n crushes $N with a dire glare! $N dies.{n", ch, NULL, victim, TO_NOTVICT);
raw_kill(victim);
send_to_char("You code-abuser. It's just so wrong.\n\r", ch);
}

}
void do_transfer( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim, *rch;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' )
    {
	send_to_char( "Transfer whom (and where)?\n\r", ch );
	return;
    }

    if ( !str_cmp( arg1, "all" ) )
    {
	for ( d = descriptor_list; d != NULL; d = d->next )
	{
	    if ( d->connected == CON_PLAYING
	    &&   d->character != ch
	    &&   d->character->in_room != NULL
	    &&   can_see( ch, d->character ) )
	    {
		char buf[MAX_STRING_LENGTH];
		sprintf( buf, "%s %s", d->character->name, arg2 );
		do_transfer( ch, buf );
	    }
	}
	return;
    }

    /*
     * Thanks to Grodyn for the optional location parameter.
     */
    if ( arg2[0] == '\0' )
    {
	location = ch->in_room;
    }
    else
    {
	if ( ( location = find_location( ch, NULL, arg2 ) ) == NULL )
	{
	    send_to_char( "No such location.\n\r", ch );
	    return;
	}

	if ( !is_room_owner(ch,location) && room_is_private( location ) 
	&&  get_trust(ch) < MAX_LEVEL)
	{
	    send_to_char( "That room is private right now.\n\r", ch );
	    return;
	}
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( victim->in_room == NULL )
    {
	send_to_char( "They are in limbo.\n\r", ch );
	return;
    }

    if ( victim->fighting != NULL )
	stop_fighting_all(victim);

    for (rch = victim->in_room->people; rch; rch = rch->next_in_room)
	if ((rch != victim) && can_see(rch, victim))
	    act("$N disappears suddenly.", rch, NULL, victim, TO_CHAR);

    char_from_room( victim );
    char_to_room( victim, location );
    wizact( "$n is returned to the mortal realms by the gods.", victim, NULL, NULL, TO_ROOM );

    if ( ch != victim )
	act( "$n has transported you.", ch, NULL, victim, TO_VICT );
    do_look( victim, "auto" );
    send_to_char( "Ok.\n\r", ch );
}



void do_at( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    ROOM_INDEX_DATA *original;
    OBJ_DATA *on;
    CHAR_DATA *wch;
    
    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
	send_to_char( "At where what?\n\r", ch );
	return;
    }

    if ( ( location = find_location( ch, NULL, arg ) ) == NULL )
    {
	send_to_char( "No such location.\n\r", ch );
	return;
    }

    if (!is_room_owner(ch,location) && room_is_private( location ) 
    &&  get_trust(ch) < MAX_LEVEL)
    {
	send_to_char( "That room is private right now.\n\r", ch );
	return;
    }

    original = ch->in_room;
    on = ch->on;
    global_bool_ranged_attack = TRUE;
    char_from_room( ch );
    char_to_room( ch, location );
    interpret( ch, argument );

    /*
     * See if 'ch' still exists before continuing!
     * Handles 'at XXXX quit' case.
     */
    for ( wch = char_list; wch != NULL; wch = wch->next )
    {
        if ( wch == ch )
        {
            char_from_room( ch );
            char_to_room( ch, original );
            ch->on = on;
            break;
        }
    }

    global_bool_ranged_attack = FALSE;
}


void do_shift( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *rch;

    if (!is_affected(ch, gsn_astralprojection))
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    if (ch->mana < 150)
    {
  	send_to_char("You don't have the energy to shift.\n\r", ch);
	return;
    }

    if ( argument[0] == '\0' )
    {
	send_to_char( "Goto where?\n\r", ch );
	return;
    }

    if ( ( rch = get_char_world( ch, argument ) ) == NULL )
    {
	send_to_char( "You sense no such person to anchor your shift.\n\r", ch );
	return;
    }

    if (rch == ch)
    {
	send_to_char("You cannot use yourself as an anchor!\n\r", ch);
	return;
    }

    if (rch->position > POS_SLEEPING && saves_spell(ch->level - 10, ch, rch, DAM_OTHER))
    {
        send_to_char("They resist your attempt to shift to them.\n", ch);
        return;
    }

    if   (rch->in_room == NULL
    ||   is_affected(ch, gsn_matrix)
    ||   IS_SET(rch->in_room->room_flags, ROOM_IMP_ONLY)
    ||   IS_SET(rch->in_room->room_flags, ROOM_GODS_ONLY)
    ||   IS_SET(rch->in_room->room_flags, ROOM_HEROES_ONLY)
    ||   IS_SET(rch->in_room->room_flags, ROOM_NEWBIES_ONLY)
    ||   IS_SET(rch->in_room->room_flags, ROOM_GUILD)
    ||   IS_SET(rch->in_room->room_flags, ROOM_NOWHERE )
    ||   IS_SET(rch->in_room->room_flags, ROOM_NOGATE)
    ||   IS_SET(rch->in_room->room_flags, ROOM_NONEFORYOU)
    ||   IS_SET(rch->in_room->room_flags, ROOM_VAULT)
    ||   (rch->in_room->area->area_flags & AREA_UNCOMPLETE )
    ||   IS_SET(rch->in_room->room_flags, ROOM_NOMAGIC)
    ||   IS_SET(rch->in_room->room_flags, ROOM_SAFE)
    ||   IS_SET(rch->in_room->room_flags, ROOM_PRIVATE)
    ||   IS_SET(rch->in_room->room_flags, ROOM_SOLITARY)
    ||   (IS_NPC(rch) && IS_SET(rch->imm_flags,IMM_SUMMON)))
	{
    	send_to_char("You failed to shift yourself through the planes.\n\r", ch);
    	WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
        expend_mana(ch, 75);
	    return;
	}

    if ( ch->fighting != NULL )
	    stop_fighting_all(ch);

    for (CHAR_DATA * echoChar(ch->in_room == NULL ? NULL : ch->in_room->people); echoChar != NULL; echoChar = echoChar->next_in_room)
    {
        if (echoChar != ch && can_see(echoChar, ch))
            act("$n ripples briefly, then phases out of existence.", ch, NULL, echoChar, TO_VICT);
    }

    act("You phase yourself through the astral plane, touching a new point on the material plane.", ch, NULL, NULL, TO_CHAR);
    ROOM_INDEX_DATA * targetRoom(rch->in_room);

    // Check for spectral lantern
    if (number_bits(4) == 0)
    {
        std::vector<const ROOM_INDEX_DATA *> lanterns(ShadeControl::SpectralLanternRooms());
        if (!lanterns.empty())
        {
            targetRoom = const_cast<ROOM_INDEX_DATA*>(lanterns[number_range(0, lanterns.size() - 1)]);
            send_to_char("A glowing light catches your attention on the way, and almost by instinct you gravitate towards it!\n", ch);
        }
    }


    char_from_room( ch );
    char_to_room( ch, targetRoom);
    expend_mana(ch, 150);

    for (CHAR_DATA * echoChar(ch->in_room->people); echoChar != NULL; echoChar = echoChar->next_in_room)
    {
        if (echoChar != ch && can_see(echoChar, ch))
            act("$n phases into existence as $e anchors $s astral form in the material world.", ch, NULL, echoChar, TO_VICT);
    }

    WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
    do_look( ch, "auto" );
}

void do_goto( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA *location;
    CHAR_DATA *rch;
    int count = 0;

    if ( argument[0] == '\0' )
    {
	send_to_char( "Goto where?\n\r", ch );
	return;
    }

    if ( ( location = find_location( ch, NULL, argument ) ) == NULL )
    {
	send_to_char( "No such location.\n\r", ch );
	return;
    }

    count = 0;
    for ( rch = location->people; rch != NULL; rch = rch->next_in_room )
        count++;

    if (!is_room_owner(ch,location) && room_is_private(location) 
    &&  (count > 1 || get_trust(ch) < MAX_LEVEL))
    {
	send_to_char( "That room is private right now.\n\r", ch );
	return;
    }

    if ( ch->fighting != NULL )
	stop_fighting_all(ch);

    if (!IS_NPC(ch))
        for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
        {
	    if (get_trust(rch) >= ch->invis_level)
	    {
	        if (ch->pcdata != NULL && ch->pcdata->bamfout[0] != '\0')
		    act("$t",ch,ch->pcdata->bamfout,rch,TO_VICT);
	        else
		    act("$n leaves in a swirling mist.",ch,NULL,rch,TO_VICT);
	    }
        }

    char_from_room( ch );
    char_to_room( ch, location );

    if (!IS_NPC(ch))
        for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
        {
            if (get_trust(rch) >= ch->invis_level)
            {
                if (ch->pcdata != NULL && ch->pcdata->bamfin[0] != '\0')
                    act("$t",ch,ch->pcdata->bamfin,rch,TO_VICT);
                else
                    act("$n appears in a swirling mist.",ch,NULL,rch,TO_VICT);
            }
        }

    do_look( ch, "auto" );
    return;
}

void do_violate( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA *location;
    CHAR_DATA *rch;
 
    if ( argument[0] == '\0' )
    {
        send_to_char( "Goto where?\n\r", ch );
        return;
    }
 
    if ( ( location = find_location( ch, NULL, argument ) ) == NULL )
    {
        send_to_char( "No such location.\n\r", ch );
        return;
    }

    if (!room_is_private( location ))
    {
        send_to_char( "That room isn't private, use goto.\n\r", ch );
        return;
    }
 
    if ( ch->fighting != NULL )
        stop_fighting_all(ch);
 
    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (get_trust(rch) >= ch->invis_level)
        {
            if (ch->pcdata != NULL && ch->pcdata->bamfout[0] != '\0')
                act("$t",ch,ch->pcdata->bamfout,rch,TO_VICT);
            else
                act("$n leaves in a swirling mist.",ch,NULL,rch,TO_VICT);
        }
    }
 
    char_from_room( ch );
    char_to_room( ch, location );
 
 
    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (get_trust(rch) >= ch->invis_level)
        {
            if (ch->pcdata != NULL && ch->pcdata->bamfin[0] != '\0')
                act("$t",ch,ch->pcdata->bamfin,rch,TO_VICT);
            else
                act("$n appears in a swirling mist.",ch,NULL,rch,TO_VICT);
        }
    }
 
    do_look( ch, "auto" );
    return;
}

/* I made this function in four minutes.  Live with it.  - Erinos */
void do_wstat( CHAR_DATA *ch, char *argument )
{
    extern bool crumble_test;
    extern bool rip_test;
    char buf[MAX_STRING_LENGTH];

    sprintf(buf, "Temperature: %d C  (%d F)\n\r", ch->in_room->area->w_cur.temperature, (((ch->in_room->area->w_cur.temperature * 9) / 5) + 32));
    send_to_char(buf, ch);
    sprintf(buf, "Cloud Cover: %d%%\n\r", ch->in_room->area->w_cur.cloud_cover);
    send_to_char(buf, ch);
    sprintf(buf, "Wind Magnitude; %d%%\n\r", ch->in_room->area->w_cur.wind_mag);
    send_to_char(buf, ch);
    sprintf(buf, "Storm Strength: %d%%\n\r", ch->in_room->area->w_cur.storm_str);
    send_to_char(buf, ch);
    sprintf(buf, "Precipitation: %d\n\r", ch->in_room->area->w_cur.precip_index);
    send_to_char(buf, ch);
    sprintf(buf, "Lightning Strength: %d%%\n\r", ch->in_room->area->w_cur.lightning_str);
    send_to_char(buf, ch);

    if (!strcmp(argument, "crumble"))
    	crumble_test = TRUE;
    else if (!strcmp(argument, "recalc"))
	rip_test = TRUE;

    return;
}

void do_rstat( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    AFFECT_DATA *paf;
    OBJ_DATA *obj;
    CHAR_DATA *rch;
    int door, i;

    one_argument( argument, arg );
    location = ( arg[0] == '\0' ) ? ch->in_room : find_location( ch, NULL, arg );
    if ( location == NULL )
    {
	send_to_char( "No such location.\n\r", ch );
	return;
    }

    if (!is_room_owner(ch,location) && ch->in_room != location 
    &&  room_is_private( location ) && !IS_TRUSTED(ch,IMPLEMENTOR))
    {
	send_to_char( "That room is private right now.\n\r", ch );
	return;
    }

    sprintf( buf, "Name: '%s'\n\rArea: '%s'\n\r",
	location->name,
	location->area->name );
    send_to_char( buf, ch );

    sprintf( buf,
	"Vnum: %d  Sector: %d  Light: %d  Healing: %d  Mana: %d\n\r",
	location->vnum,
	location->sector_type,
	location->light,
	location->heal_rate,
	location->mana_rate );
    send_to_char( buf, ch );

    sprintf( buf,
	"Room Danger: %d    Area Danger: %d\n\r",
	location->danger,
	location->area->danger);
    send_to_char( buf, ch );

    sprintf( buf,
	"Room flags: %d.\n\rDescription:\n\r%s",
	location->room_flags,
	location->description );
    send_to_char( buf, ch );

    sprintf(buf, "Herb: %s\n\r", (location->herb_type >= 0) ? herb_table[location->herb_type].name : "none");
    send_to_char( buf, ch );

    if ( location->extra_descr != NULL )
    {
	EXTRA_DESCR_DATA *ed;

	send_to_char( "Extra description keywords: '", ch );
	for ( ed = location->extra_descr; ed; ed = ed->next )
	{
	    send_to_char( ed->keyword, ch );
	    if ( ed->next != NULL )
		send_to_char( " ", ch );
	}
	send_to_char( "'.\n\r", ch );
    }

    send_to_char( "Characters:", ch );
    for ( rch = location->people; rch; rch = rch->next_in_room )
    {
	if (can_see(ch,rch))
        {
	    send_to_char( " ", ch );
	    one_argument( rch->name, buf );
	    send_to_char( buf, ch );
	}
    }

    send_to_char( ".\n\rObjects:   ", ch );
    for ( obj = location->contents; obj; obj = obj->next_content )
    {
	send_to_char( " ", ch );
	one_argument( obj->name, buf );
	send_to_char( buf, ch );
    }
    send_to_char( ".\n\rRoom values:", ch );

    for (i = 0; i < 10; i++)
    {
	sprintf(buf, " %d", location->roomvalue[i]);
	send_to_char(buf, ch);
    }
    
    send_to_char("\n\r", ch);
    for (unsigned int i(0); i < MAX_MOBVALUE; ++i)
    {
        if (location->stringvalue[i] != 0)
        {
            sprintf(buf, "{wString Value #%d: %s\n", i, location->stringvalue[i]);
            send_to_char(buf, ch);
        }
    }

    for ( door = 0; door <= 5; door++ )
    {
	EXIT_DATA *pexit;

	if ( ( pexit = location->exit[door] ) != NULL )
	{
	    sprintf( buf,
		"Door: %d. To: %d. Key: %d. Exit flags: %s.\n\rKeyword: '%s'.  Description: %s",

		door,
		(pexit->u1.to_room == NULL ? -1 : pexit->u1.to_room->vnum),
	    	pexit->key,
	    	flag_string(exit_flags, pexit->exit_info),
	    	pexit->keyword,
	    	pexit->description[0] != '\0'
		    ? pexit->description : "(none).\n\r" );
	    send_to_char( buf, ch );
	}
    }

    if (location->affected != NULL)
	{
	send_to_char("This room is affected by:\n\r", ch);
    		for ( paf = location->affected; paf != NULL; paf = paf->next )
    		{
        		sprintf( buf,
            		"Room Spell: '%s' modifies %s by %d for %d%s hours with bits %s, level %d.\n\r",
            		skill_table[(int) paf->type].name,
            		affect_loc_name( paf->location ),
            		paf->modifier,
            		paf->duration / 2,
			((paf->duration %2) == 0) ? "" : ".5",
            		affect_bit_name( paf ),
            		paf->level
            		);
        		send_to_char( buf, ch );
    		}
	}

    if (location->area->affected != NULL)
	{
	send_to_char("The area this room is in is affected by:\n\r", ch);
    		for ( paf = location->area->affected; paf != NULL; paf = paf->next )
    		{
        		sprintf( buf,
            		"Area Spell: '%s' modifies %s by %d for %d%s hours with bits %s, level %d.\n\r",
            		skill_table[(int) paf->type].name,
            		affect_loc_name( paf->location ),
            		paf->modifier,
            		paf->duration / 2,
			((paf->duration % 2) == 0) ? "" : ".5",
            		affect_bit_name( paf ),
            		paf->level
            		);
        		send_to_char( buf, ch );
    		}
	}
    if (location->herb_type > -1 && herb_table[location->herb_type].name)
    {
	sprintf(buf,"Herb: %s.\n\r",herb_table[location->herb_type].name);
	send_to_char(buf,ch);
    }
    if (location->progtypes)
	send_to_char("This room has progs.\n\r",ch);
    return;
}



void do_ostat( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    AFFECT_DATA *paf;
    OBJ_DATA *obj;

    struct elemental_bias_type elemental_bias;
    struct platonic_bias_type platonic_bias;
    char * temp;
    int total_vnums = 0; //Just to fool the function.
    double power;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Stat what?\n\r", ch );
	return;
    }

    if ( ( obj = get_obj_world( ch, argument ) ) == NULL )
    {
	send_to_char( "Nothing like that in earth or heaven.\n\r", ch );
	return;
    }

    sprintf( buf, "Name(s): %s\n\r",
	obj->name );
    send_to_char( buf, ch );

    sprintf( buf, "Vnum: %d  Format: %s  Type: %s  Level: %d  Resets: %d\n\r",
	obj->pIndexData->vnum, obj->pIndexData->new_format ? "new" : "old",
	item_name(obj->item_type), obj->level, obj->pIndexData->reset_num );
    send_to_char( buf, ch );

    sprintf( buf, "Short description: %s\n\rLong description: %s\n\r",
	obj->short_descr, obj->description );
    send_to_char( buf, ch );

    std::string ex1(extra_bit_name(obj->extra_flags[1], 1));
    std::string ex2(extra_bit_name(obj->extra_flags[2], 2));
    sprintf( buf, "Wear bits: %s\n\rExtra bits: %s %s %s\n\r",
        wear_bit_name(obj->wear_flags), 
        extra_bit_name(obj->extra_flags[0], 0), 
        (ex1 == "none" ? "" : ex1.c_str()),
        (ex2 == "none" ? "" : ex2.c_str()));
    send_to_char( buf, ch );

    sprintf( buf, "Number: %d/%d  Weight: %d/%d/%d (10th pounds)\n\r",
	1,           get_obj_number( obj ),
	obj->weight, get_obj_weight( obj ),get_true_weight(obj) );
    send_to_char( buf, ch );

    sprintf( buf, "Limit: %d  Limit_Factor: %d Current: %d\n\r",
	obj->pIndexData->limit, obj->pIndexData->limit_factor, obj->pIndexData->current );
    send_to_char( buf, ch );

    sprintf( buf, "Object Values: %d %d %d %d %d %d %d %d %d %d\n\r", obj->objvalue[0],
	obj->objvalue[1], obj->objvalue[2], obj->objvalue[3],
	obj->objvalue[4], obj->objvalue[5], obj->objvalue[6],
	obj->objvalue[7], obj->objvalue[8], obj->objvalue[9]);
    send_to_char(buf, ch);

    for (unsigned int i(0); i < MAX_MOBVALUE; ++i)
    {
        if (obj->stringvalue[i] != 0)
        {
            sprintf(buf, "{wString Value #%d: %s\n", i, obj->stringvalue[i]);
            send_to_char(buf, ch);
        }
    }

    sprintf( buf,
	"In room: %d  In object: %s  Carried by: %s  Owner: %s\n\r",
	obj->in_room    == NULL    ?        0 : obj->in_room->vnum,
	obj->in_obj     == NULL    ? "(none)" : obj->in_obj->short_descr,
	obj->carried_by == NULL    ? "(none)" : 
	    can_see(ch,obj->carried_by) ? obj->carried_by->name
				 	: "someone",
	obj->owner);
    send_to_char( buf, ch );
    
    sprintf( buf, "Values: %d %d %d %d %d  Timer: %d  Condition: %d\n\r",
	obj->value[0], obj->value[1], obj->value[2], obj->value[3],
	obj->value[4], obj->timer, obj->condition );
    send_to_char( buf, ch );

    if (obj->lastowner[0][0] != '\0')
    {
	int i;

	sprintf(buf, "Last owners: %s", obj->lastowner[0]);
	for (i = 1; i < MAX_LASTOWNER; i++)
	    if (obj->lastowner[i][0] != '\0')
	        sprintf(buf, "%s, %s", buf, obj->lastowner[i]);

	sprintf(buf, "%s.\n\r", buf);
	send_to_char( buf, ch );
    }

    /* now give out vital statistics as per identify */
    
    switch ( obj->item_type )
    {
    	case ITEM_SCROLL: 
    	case ITEM_POTION:
    	case ITEM_PILL:
	case ITEM_OIL:
	    sprintf( buf, "Level %d spells of:", obj->value[0] );
	    send_to_char( buf, ch );

	    if ( obj->value[1] >= 0 && obj->value[1] < MAX_SKILL )
	    {
	    	send_to_char( " '", ch );
	    	send_to_char( skill_table[obj->value[1]].name, ch );
	    	send_to_char( "'", ch );
	    }

	    if ( obj->value[2] >= 0 && obj->value[2] < MAX_SKILL )
	    {
	    	send_to_char( " '", ch );
	    	send_to_char( skill_table[obj->value[2]].name, ch );
	    	send_to_char( "'", ch );
	    }

	    if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
	    {
	    	send_to_char( " '", ch );
	    	send_to_char( skill_table[obj->value[3]].name, ch );
	    	send_to_char( "'", ch );
	    }

	    if (obj->value[4] >= 0 && obj->value[4] < MAX_SKILL)
	    {
		send_to_char(" '",ch);
		send_to_char(skill_table[obj->value[4]].name,ch);
		send_to_char("'",ch);
	    }

	    send_to_char( ".\n\r", ch );
	break;

    	case ITEM_WAND: 
    	case ITEM_STAFF: 
	    sprintf( buf, "Has %d(%d) charges of level %d",
	    	obj->value[1], obj->value[2], obj->value[0] );
	    send_to_char( buf, ch );
      
	    if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
	    {
	    	send_to_char( " '", ch );
	    	send_to_char( skill_table[obj->value[3]].name, ch );
	    	send_to_char( "'", ch );
	    }

	    send_to_char( ".\n\r", ch );
	break;

	case ITEM_DRINK_CON:
	    sprintf(buf,"It holds %s-colored %s.\n\r",
		liq_table[obj->value[2]].liq_color,
		liq_table[obj->value[2]].liq_name);
	    send_to_char(buf,ch);
	    break;
		
      
    	case ITEM_WEAPON:
 	    send_to_char("Weapon type is ",ch);
	    switch (obj->value[0])
	    {
	    	case(WEAPON_EXOTIC): 
		    send_to_char("exotic\n\r",ch);
		    break;
	    	case(WEAPON_SWORD): 
		    send_to_char("sword\n\r",ch);
		    break;	
	    	case(WEAPON_DAGGER): 
		    send_to_char("dagger\n\r",ch);
		    break;
		case(WEAPON_STAFF):
		    send_to_char("staff\n\r",ch);
		    break;
		case(WEAPON_KNIFE):
		    send_to_char("knife\n\r",ch);
		    break;
	    	case(WEAPON_SPEAR):
		    send_to_char("spear\n\r",ch);
		    break;
	    	case(WEAPON_MACE): 
		    send_to_char("mace/club\n\r",ch);	
		    break;
	   	case(WEAPON_AXE): 
		    send_to_char("axe\n\r",ch);	
		    break;
	    	case(WEAPON_FLAIL): 
		    send_to_char("flail\n\r",ch);
		    break;
	    	case(WEAPON_WHIP): 
		    send_to_char("whip\n\r",ch);
		    break;
	    	case(WEAPON_POLEARM): 
		    send_to_char("polearm\n\r",ch);
		    break;
	    	default: 
		    send_to_char("unknown\n\r",ch);
		    break;
 	    }
	    if (obj->pIndexData->new_format)
        {
            int aveDamTenths(((1 + obj->value[2]) * obj->value[1] * 10) / 2);
            sprintf(buf,"Damage is %dd%d (average %d.%d)\n", obj->value[1],obj->value[2], aveDamTenths / 10, aveDamTenths % 10);
        }
	    else
	    	sprintf( buf, "Damage is %d to %d (average %d)\n\r",
	    	    obj->value[1], obj->value[2],
	    	    ( obj->value[1] + obj->value[2] ) / 2 );
	    send_to_char( buf, ch );

	    sprintf(buf,"Damage noun is %s.\n\r", obj->obj_str);
	    send_to_char(buf,ch);

	    sprintf(buf, "Damage type is %s.\n\r",
		    damtype_table[obj->value[3]].name);
	    send_to_char(buf, ch);
	    
	    if (obj->value[4])  /* weapon flags */
	    {
	        sprintf(buf,"Weapons flags: %s\n\r",
		    weapon_bit_name(obj->value[4]));
	        send_to_char(buf,ch);
            }
	break;

    	case ITEM_ARMOR:
	    sprintf( buf, 
	    "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic\n\r",
	        obj->value[0], obj->value[1], obj->value[2], obj->value[3] );
	    send_to_char( buf, ch );
	break;

        case ITEM_CONTAINER:
            sprintf(buf,"Capacity: %d#  Maximum weight: %d#  flags: %s\n\r",
                obj->value[0], obj->value[3], cont_bit_name(obj->value[1]));
            send_to_char(buf,ch);
            if (obj->value[4] != 100)
            {
                sprintf(buf,"Weight multiplier: %d%%\n\r",
		    obj->value[4]);
                send_to_char(buf,ch);
            }
        break;
    }


    if ( obj->extra_descr != NULL || obj->pIndexData->extra_descr != NULL )
    {
	EXTRA_DESCR_DATA *ed;

	send_to_char( "Extra description keywords: '", ch );

	for ( ed = obj->extra_descr; ed != NULL; ed = ed->next )
	{
	    send_to_char( ed->keyword, ch );
	    if ( ed->next != NULL )
	    	send_to_char( " ", ch );
	}

	for ( ed = obj->pIndexData->extra_descr; ed != NULL; ed = ed->next )
	{
	    send_to_char( ed->keyword, ch );
	    if ( ed->next != NULL )
		send_to_char( " ", ch );
	}

	send_to_char( "'\n\r", ch );
    }

    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
        if (paf->type >= 0)
            sprintf(buf, "Affects %s by %d from %s, level %d", affect_loc_name( paf->location ), paf->modifier, skill_table[paf->type].name, paf->level);
        else
            sprintf(buf, "Affects %s by %d, level %d", affect_loc_name(paf->location), paf->modifier, paf->level);

        send_to_char(buf,ch);
        if ( paf->duration > -1)
            sprintf(buf,", %d%s hours.\n\r",
            paf->duration/2,
            ((paf->duration % 2) == 0) ? "" : ".5");
        else
            sprintf(buf,".\n\r");
        send_to_char( buf, ch );
        if (paf->bitvector)
        {
            switch(paf->where)
            {
            case TO_AFFECTS:
                sprintf(buf,"Adds %s affect.\n",
                affect_bit_name(paf));
                break;
                    case TO_WEAPON:
                        sprintf(buf,"Adds %s weapon flags.\n",
                            weapon_bit_name(paf->bitvector));
                break;
            case TO_OBJECT:
                sprintf(buf,"Adds %s object flag.\n",
                extra_bit_name(paf->bitvector, 0));
                break;
            case TO_OBJECT1:
                sprintf(buf,"Adds %s object flag.\n",
                extra_bit_name(paf->bitvector, 1));
                break;
            case TO_OBJECT2: sprintf(buf, "Adds %s object flag.\n", extra_bit_name(paf->bitvector, 2)); break;
            case TO_IMMUNE:
                sprintf(buf,"Adds immunity to %s.\n",
                imm_bit_name(paf->bitvector));
                break;
            case TO_RESIST:
                sprintf(buf,"Adds resistance to %s.\n\r",
                imm_bit_name(paf->bitvector));
                break;
            case TO_VULN:
                sprintf(buf,"Adds vulnerability to %s.\n\r",
                imm_bit_name(paf->bitvector));
                break;
            default:
                sprintf(buf,"Unknown bit %d: %d\n\r",
                paf->where,paf->bitvector);
                break;
            }
            send_to_char(buf,ch);
        }
    }

    if (!obj->enchanted)
    for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
    {
	sprintf( buf, "Affects %s by %d, level %d.\n\r",
	    affect_loc_name( paf->location ), paf->modifier,paf->level );
	send_to_char( buf, ch );
        if (paf->bitvector)
        {
            switch(paf->where)
            {
                case TO_AFFECTS:
                    sprintf(buf,"Adds %s affect.\n",
                        affect_bit_name(paf));
                    break;
                case TO_OBJECT:
                    sprintf(buf,"Adds %s object flag.\n",
                        extra_bit_name(paf->bitvector, 0));
                    break;
		case TO_OBJECT1:
                    sprintf(buf,"Adds %s object flag.\n",
                        extra_bit_name(paf->bitvector, 1));
                    break;
		case TO_OBJECT2: sprintf(buf, "Adds %s object flag.\n", extra_bit_name(paf->bitvector, 2)); break;
                case TO_IMMUNE:
                    sprintf(buf,"Adds immunity to %s.\n",
                        imm_bit_name(paf->bitvector));
                    break;
                case TO_RESIST:
                    sprintf(buf,"Adds resistance to %s.\n\r",
                        imm_bit_name(paf->bitvector));
                    break;
                case TO_VULN:
                    sprintf(buf,"Adds vulnerability to %s.\n\r",
                        imm_bit_name(paf->bitvector));
                    break;
                default:
                    sprintf(buf,"Unknown bit %d: %d\n\r",
                        paf->where,paf->bitvector);
                    break;
            }
            send_to_char(buf,ch);
        }
    }

	//Show alchemical biases, power
	power = URANGE(0, evaluate_item(obj, &elemental_bias, &platonic_bias, &total_vnums), 10);
	sprintf(buf, "Alchemical power: %d\n\r", (int)power);
	send_to_char(buf, ch);
	temp = read_elemental_biases(&elemental_bias);
	sprintf(buf, temp);
	if (buf[0] != '\0') 
	{
		send_to_char("Elemental biases:\n\r", ch);
		send_to_char(buf, ch);
	}
	free_string(temp);
	temp = read_platonic_biases(&platonic_bias);
	sprintf(buf, temp);
	if (buf[0] != '\0')
	{
		send_to_char("\nPlatonic biases:\n\r", ch);
		send_to_char(buf, ch);
	}
	free_string(temp);
	if (obj->pIndexData->progtypes)
	    send_to_char("\nThis object has progs.\n\r",ch);
}

void do_fstat( CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    int x;

    if ((victim = get_char_world(ch, argument)) == NULL)
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if (IS_NPC(victim))
    {
        send_to_char("Not on NPCs!\n\r", ch);
        return;
    }

    for (x = 0;x < MAX_SWORDS;x++)
    {
	if (victim->pcdata->favored_vnum[x] != 0)
	{
	    sprintf(buf, "Vnum: %d    Skill Bonus: %d\n",
		  victim->pcdata->favored_vnum[x],
		  victim->pcdata->favored_bonus[x]);
	    send_to_char(buf, ch);
	}
    }
}

void stat_npc(CHAR_DATA *ch, CHAR_DATA *victim)
{
    char buf[MAX_STRING_LENGTH];
    int i;

    sprintf(buf, "{wUnique name{D: {W%s {w({Wnpc{w)\n\r", victim->unique_name);
    send_to_char(buf, ch);

    sprintf(buf, "{wName{D: {W%s\n\r", victim->name);
    send_to_char(buf, ch);

    sprintf(buf, "{wVnum{D: {W%-7d{wRace{D: {W%-10s{wCount{D: {W%d  {wKilled{D: {W%d  {wClan{D: {W%s\n\r",
	victim->pIndexData->vnum,
	race_table[victim->race].name,
	victim->pIndexData->count,
	victim->pIndexData->killed,
	clan_table[victim->clan].name);
    send_to_char(buf, ch);

    sprintf(buf, "{wLevl{D: {W%-7d{wSex {D: {W%-10s{wAlign{D: {W%d {w({W%s{w)\n\r\n\r",
	victim->level,
	sex_table[victim->sex].name,
	victim->alignment,
	IS_TRUE_GOOD(victim) ? "good" : IS_TRUE_EVIL(victim) ? "evil" : "neutral");
    send_to_char(buf, ch);

    sprintf(buf, "{wStr{D: {W%2d{w({W%2d{w)  {wInt{D: {W%2d{w({W%2d{w)  {wWis{D: {W%2d{w({W%2d{w)  {wDex{D: {W%2d{w({W%2d{w)  {wCon{D: {W%2d{w({W%2d{w)  {wChr{D: {W%2d{w({W%2d{w)\n\r",
    	get_curr_stat(victim, STAT_STR),
	victim->perm_stat[STAT_STR],
	get_curr_stat(victim, STAT_INT),
	victim->perm_stat[STAT_INT],
	get_curr_stat(victim, STAT_WIS),
	victim->perm_stat[STAT_WIS],
	get_curr_stat(victim, STAT_DEX),
	victim->perm_stat[STAT_DEX],
	get_curr_stat(victim, STAT_CON),
	victim->perm_stat[STAT_CON],
	get_curr_stat(victim, STAT_CHR),
	victim->perm_stat[STAT_CHR]);
    send_to_char(buf, ch);
    
    sprintf(buf, "{wHP{D: {W%d{w/{W%d  {wMana{D: {W%d{w/{W%d  {wMove{D: {W%d{w/{W%d\n\r",
	victim->hit, victim->max_hit,
	victim->mana, victim->max_mana,
	victim->move, victim->max_move);
    send_to_char(buf, ch);

    sprintf(buf, "{wAC{D: {wpierce{D: {W%d  {wbash{D: {W%d  {wslash{D: {W%d  {wmagic{D: {W%d\n\r",
	    GET_AC(victim,AC_PIERCE), GET_AC(victim,AC_BASH),
	    GET_AC(victim,AC_SLASH),  GET_AC(victim,AC_EXOTIC));
    send_to_char(buf, ch);

    for (i = 0; i < MAX_COIN; i++)
    {
	if (i > 0)
	    send_to_char("  ", ch);

	sprintf(buf, "{w%s{D: {W%ld", coin_table[i].name, victim->coins[i]);
	buf[0] = UPPER(buf[0]);
	send_to_char(buf, ch);
    }
    send_to_char("\n\r", ch);

    sprintf(buf, "{wMemory Values{D: %d %d %d %d %d %d %d %d %d %d\n", victim->mobvalue[0],
	victim->mobvalue[1], victim->mobvalue[2], victim->mobvalue[3],
	victim->mobvalue[4],victim->mobvalue[5], victim->mobvalue[6],
	victim->mobvalue[7],victim->mobvalue[8], victim->mobvalue[9]);
    send_to_char( buf, ch );

    for (unsigned int i(0); i < MAX_MOBVALUE; ++i)
    {
        if (victim->stringvalue[i] != 0)
        {
            sprintf(buf, "{wString Value #%d: %s\n", i, victim->stringvalue[i]);
            send_to_char(buf, ch);
        }
    }

    sprintf(buf, "{wTimer:{D: %d\n\n", victim->timer);
    send_to_char(buf, ch);

    sprintf(buf, "{wRm{D: {W%d  {wHit{D: {W%d  {wDam{D: {W%d  {wSave{D: {W%d  {wLuck{D: {W%d  {wSize{D: {W%s  {wPos{D: {W%s  {wWimpy{D: {W%d\n\r",
	victim->in_room ? victim->in_room->vnum : -1,
	GET_HITROLL(victim), GET_DAMROLL(victim), victim->saving_throw, Luck::LuckFor(*victim),
	size_table[victim->size].name, position_table[victim->position].name,
	victim->wimpy );
    send_to_char(buf, ch);

    sprintf(buf, "{wDamage{D: {W%dd%d+%d  {wMessage{D: {W%s {w({W%s{w)  Carry number{D: {W%d  {wCarry weight{D: {W%d\n\r\n\r",
	    victim->damage[DICE_NUMBER],victim->damage[DICE_TYPE],victim->damage[DICE_BONUS],
	    victim->dam_verb, damtype_table[victim->dam_type].name,
	    victim->carry_number, get_carry_weight(*victim) / 10);
    send_to_char(buf, ch);
}

void stat_pc(CHAR_DATA *ch, CHAR_DATA *victim)
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    int i;

    sprintf(buf2, "%s {w(%s{w)", victim->name, (victim == ol_char) ? "{Roff" : "{Wpc");
    sprintf(buf, "{wName{D: {W%-23s  {wClass{D: {W%-16s{wLevel{D: {W%2d {w({W%2d{w)  {wSex{D: {W%s\n\r",
	buf2,
	class_table[victim->class_num].name,
	victim->level,
	victim->trust,
	sex_table[victim->sex].name);

    send_to_char(buf, ch);

    sprintf(buf2, "%s {w({W%d{w)", IS_TRUE_GOOD(victim) ? "good" : IS_TRUE_EVIL(victim) ? "evil" : "neutral", victim->pcdata->karma);
    sprintf(buf, "{wRace{D: {W%-19s{wAlign{D: {W%-22s{wEthos{D: {W%-9s{wPr{D/{wTr{D: {W%d{w/{W%d\n\r",
	race_table[victim->race].name,
	buf2,
	ethos_table[victim->pcdata->ethos].name,
	victim->practice,
	victim->train);

    send_to_char(buf, ch);
    if (victim->clan != 0)
    {
	sprintf(buf, "{wClan{D: {W%-42s{w",clan_table[victim->clan].name);
	send_to_char(buf,ch);
    }
    if (victim->class_num <= 5)
    {
        std::ostringstream mess;
        mess << "{wSpheres{D: {W" << (victim->class_num <= 5 ? sphere_table[victim->pcdata->major_sphere].name : "{wn");
        mess << "{w/{W" << (victim->class_num <= 5 ? sphere_table[victim->pcdata->minor_sphere].name : "{wa");
        if (victim->pcdata->chosen_path != PATH_NONE)
            mess << "{w/{W" << path_table[victim->pcdata->chosen_path].name;
        mess << "\n";
        send_to_char(mess.str().c_str(), ch);
    }
    else if (victim->class_num == global_int_class_druid)
    {
	if (victim->pcdata->minor_sphere == SPH_FIRIEL)
	    send_to_char("{wMinor{D: {Wfiriel\n\r",ch);
	if (victim->pcdata->minor_sphere == SPH_LUNAR)
	    send_to_char("{wMinor{D: {Wlunar\n\r",ch); 
	if (victim->pcdata->minor_sphere == SPH_GAMALOTH)
	    send_to_char("{wMinor{D: {Wgamaloth\n\r",ch);
    }
    else
	send_to_char("\n\r",ch);

    sprintf(buf, "\n\r{wStr{D: {W%2d{w({W%2d{w)  {wInt{D: {W%2d{w({W%2d{w)  {wWis{D: {W%2d{w({W%2d{w)  {wDex{D: {W%2d{w({W%2d{w)  {wCon{D: {W%2d{w({W%2d{w)  {wChr{D: {W%2d{w({W%2d{w)\n\r",
    	get_curr_stat(victim, STAT_STR),
	victim->perm_stat[STAT_STR],
	get_curr_stat(victim, STAT_INT),
	victim->perm_stat[STAT_INT],
	get_curr_stat(victim, STAT_WIS),
	victim->perm_stat[STAT_WIS],
	get_curr_stat(victim, STAT_DEX),
	victim->perm_stat[STAT_DEX],
	get_curr_stat(victim, STAT_CON),
	victim->perm_stat[STAT_CON],
	get_curr_stat(victim, STAT_CHR),
	victim->perm_stat[STAT_CHR]);
    send_to_char(buf, ch);

    sprintf(buf2, "%d{w/{W%d", victim->hit, victim->max_hit);
    sprintf(buf, "{wHP{D: {W%-18s", buf2);
    send_to_char(buf, ch);

    sprintf(buf2, "%d{w/{W%d", victim->mana, victim->max_mana);
    sprintf(buf, "{wMana{D: {W%-17s", buf2);
    send_to_char(buf, ch);

    sprintf(buf2, "%d{w/{W%d", victim->move, victim->max_move);
    sprintf(buf, "{wMove{D: {W%-17s{wXP{D: {W%d{w/{W%d\n\r",
	buf2,
	victim->exp,
	exp_on_level(victim,victim->level+1));
    send_to_char(buf, ch);

    sprintf(buf, "{wAC{D: {wpierce{D: {W%-6d{wbash{D: {W%-6d{wslash{D: {W%-6d{wmagic{D: {W%-6d{wEP{D: {W%d{w/{W%d\n\r\n\r",
	    GET_AC(victim,AC_PIERCE), GET_AC(victim,AC_BASH),
	    GET_AC(victim,AC_SLASH),  GET_AC(victim,AC_EXOTIC),
	    victim->ep, ep_table[victim->level+1]);
    send_to_char(buf, ch);
	
    send_to_char("{wOn hand {D- ", ch);

    for (i = 0; i < MAX_COIN; i++)
    {
	if (i > 0)
	    send_to_char("  ", ch);

	sprintf(buf, "{w%s{D: {W%ld", coin_table[i].name, victim->coins[i]);
	send_to_char(buf, ch);
    }
    send_to_char("\n\r{wIn bank {D- ", ch);

    for (i = 0; i < MAX_COIN; i++)
    {
	if (i > 0)
	    send_to_char("  ", ch);

	sprintf(buf, "{w%s{D: {W%ld", coin_table[i].name, victim->bank[i]);
	send_to_char(buf, ch);
    }
    send_to_char("\n\r\n\r", ch);

    sprintf(buf, "{wRm{D: {W%d  {wHit{D: {W%d  {wDam{D: {W%d  {wSave{D: {W%d  {wLuck{D: {W%d {wSize{D: {W%s  {wPos{D: {W%s  {wWimpy{D: {W%d\n\r",
	victim->in_room ? victim->in_room->vnum : -1,
	GET_HITROLL(victim), GET_DAMROLL(victim), victim->saving_throw, Luck::LuckFor(*victim),
	size_table[victim->size].name, position_table[victim->position].name,
	victim->wimpy );
    send_to_char(buf, ch);

    sprintf(buf, "{wHunger{D: {W%d {w({W%d{w%%)  Thirst{D: {W%d  {wDrunk{D: {W%d  {wAge{D: {W%d {w(Max {W%d{w)  Played{D: {W%d  Altar: %d\n\r",
	    victim->pcdata->condition[COND_HUNGER],
	    victim->pcdata->food_quality,
	    victim->pcdata->condition[COND_THIRST],
	    victim->pcdata->condition[COND_DRUNK],
	    get_age(victim), 
            victim->pcdata->max_age,
	    (int) (victim->played + current_time - victim->logon) / 3600,
	    victim->recall_to->vnum);
    send_to_char(buf, ch);

    sprintf(buf, "{wDeaths{D: {W%d {w(Max{D: {W%d{w)  PKills{D: {W%d  {w(Good{D: {W%d{w, Neutral{D: {W%d{w, Evil{D: {W%d{w)  Wanted{D: {W%d\n\r",
	    victim->pcdata->death_count,
	    victim->pcdata->max_deaths,
	    victim->pcdata->pkills,
	    victim->pcdata->align_kills[ALIGN_GOOD],
	    victim->pcdata->align_kills[ALIGN_NEUTRAL],
	    victim->pcdata->align_kills[ALIGN_EVIL],
	    victim->pcdata->times_wanted);
    send_to_char(buf, ch);

    sprintf(buf, "{wCarry number{D: {W%d  {wCarry weight{D: {W%d  {wLast Level{D: {W%d  {wTimer{D: {W%d  {wSec{D: {W%d\n\r\n\r",
	    victim->carry_number, get_carry_weight(*victim) / 10,
	    victim->pcdata->last_level, 
	    victim->timer,
	    victim->pcdata->security);
    send_to_char(buf, ch);

    return;
}

void mstat(CHAR_DATA *ch, CHAR_DATA *victim)
{
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA *paf;
    bool rFound = FALSE;
    int i;

    if (IS_NPC(victim))
       stat_npc(ch, victim);
    else
       stat_pc(ch, victim);

    if (victim->fighting)
    {
        sprintf( buf, "{wFighting{D: {W%s\n\r", victim->fighting->name);
        send_to_char( buf, ch );
    }

    if (victim->master || victim->leader || victim->pet || victim->familiar)
    {
        sprintf( buf, "{wMaster{D: {W%s  {wLeader{D: {W%s  {wPet{D: {W%s  {wFamiliar{D: {w%s\n\r\n\r",
	    victim->master      ? victim->master->name   : "(none)",
	    victim->leader      ? victim->leader->name   : "(none)",
	    victim->pet		? victim->pet->name	 : "(none)",
	    victim->familiar    ? victim->familiar->name : "(none)");
	send_to_char( buf, ch );
    }
    else if (victim->fighting)
        send_to_char("\n\r", ch);

    sprintf(buf, "{wAct {D: {W%s", IS_NPC(victim) ? flag_string(act_flags, victim->act) : flag_string(plr_flags, victim->act));
    send_to_char(buf,ch);

    if (IS_NPC(victim))
    {
	if (str_cmp(flag_string(nact_flags, victim->nact), "none"))
	{
	    sprintf(buf, " %s\n\r", flag_string(nact_flags, victim->nact));
	    send_to_char(buf, ch);
	}
	else
	    send_to_char("\n\r", ch);
    }
    else
    {
	if (str_cmp(flag_string(nplr_flags, victim->nact), "none"))
	{
	    sprintf(buf, " %s\n\r", flag_string(nplr_flags, victim->nact));
	    send_to_char(buf, ch);
	}
	else
	    send_to_char("\n\r", ch);
    }
    
    if (victim->comm)
    {
    	sprintf(buf,"{wComm{D: {W%s\n\r",comm_bit_name(victim->comm));
    	send_to_char(buf,ch);
    }

    if (IS_NPC(victim) && victim->off_flags)
    {
    	sprintf(buf, "{wOff {D: {W%s\n\r",off_bit_name(victim->off_flags));
	send_to_char(buf,ch);
    }

    if (victim->imm_flags)
    {
	sprintf(buf, "{wImm {D: {W%s\n\r",imm_bit_name(victim->imm_flags));
	send_to_char(buf,ch);
    }
   
    for (i = 0; i < MAX_RESIST; i++)
    {
	if (victim->resist[i] > 0)
	{ 
	    if (rFound)
		sprintf(buf, "%s, {W%s {w({W%d{w)", buf, imm_flags[i].name, victim->resist[i]);
	    else
            {
		sprintf(buf, "{wRes {D: {W%s {w({W%d{w)", imm_flags[i].name, 
                                                     victim->resist[i]);
		rFound = TRUE;
	    }
	}
    }
    if (rFound)
    {
        sprintf(buf, "%s.\n\r", buf);
        send_to_char(buf, ch);
    }

    rFound = FALSE;
    for (i = 0; i < MAX_RESIST; i++)
    {
        if (victim->resist[i] < 0)
        {
            if (rFound)
                sprintf(buf, "%s, {W%s {w({W%d{w)", buf, imm_flags[i].name, victim->resist[i]);
            else
            {
                sprintf(buf, "{wVuln{D: {W%s {w({W%d{w)", imm_flags[i].name,
                                                     victim->resist[i]);
                rFound = TRUE;
            }
        }
    }
    if (rFound)
    {
        sprintf(buf, "%s.\n\r", buf);
        send_to_char(buf, ch);
    }

    sprintf(buf, "{wForm{D: {W%s\n\r{wPart{D: {W%s\n\r{w", 
	form_bit_name(victim->form), part_bit_name(victim->parts));
    send_to_char(buf,ch);

    if (IS_NPC(victim))
    {
	send_to_char( "{wLang{D: ", ch );
	for (i = 0; i < MAX_LANGUAGE; i++)
	{
	    if (IS_SET(victim->lang_flags, (1 << i)))
	    {
		sprintf(buf, "%s ", lang_data[i].name);
		send_to_char(buf, ch);
	    }
	}
	send_to_char("{x\n\r", ch);
    }

    if (victim->affected_by || victim->paffected_by 
      || victim->naffected_by || victim->oaffected_by)
    {
	sprintf(buf, "Affected by %s\n\r", 
	    affect_bit_name(victim->affected));
	send_to_char(buf,ch);
    }

    if ((victim->short_descr && victim->short_descr[0] != '\0')
     || (victim->long_descr && victim->long_descr[0] != '\0'))
        send_to_char("\n\r", ch);

    if (victim->short_descr && victim->short_descr[0] != '\0')
    {
        sprintf( buf, "Short description: %s\n\r", victim->short_descr);
	send_to_char(buf, ch);
    }

    if (victim->long_descr && victim->long_descr[0] != '\0')
    {
        sprintf(buf, "Long  description: %s\n\r", victim->long_descr);
        send_to_char( buf, ch );
    }

    if ( IS_NPC(victim) && victim->spec_fun != 0 )
    {
	sprintf(buf,"Mobile has special procedure %s.\n\r",
		spec_name(victim->spec_fun));
	send_to_char(buf,ch);
    }

    for ( paf = victim->affected; paf != NULL; paf = paf->next )
    {
	sprintf( buf,
	    "Spell: '%s' %smodifies %s by %d for %d%s hours with bits %s, level %d.\n\r",
	    ((paf->type == gsn_generic || paf->type == gsn_sacrifice) 
	      && paf->point != NULL)
	      ? (char *) paf->point
	      : (paf->type >= 0 ? skill_table[(int) paf->type].name : "?"),
	    paf->type == gsn_generic ? "(generic) " 
	      : (paf->type == gsn_sacrifice ? "(sacrifice) ": ""),
	    affect_loc_name( paf->location ),
	    paf->modifier,
	    paf->duration / 2,
	    ((paf->duration % 2) == 0) ? "" : ".5",
	    affect_bit_name( paf ),
	    paf->level
	    );
	send_to_char( buf, ch );

        // Handle extra info for bloodpyre persistent effects
        if (paf->type == gsn_bloodpyre)
        {
            if (paf->point == NULL)
                send_to_char("    (cooldown only; pyre is gone)\n", ch);
            else
            {
                PyreInfo * pyre(static_cast<PyreInfo*>(paf->point));
                sprintf(buf, "     : '%s' (%s)\n", PyreInfo::effectName(pyre->effect()), (pyre->isEffectGreater() ? "greater" : "lesser"));
                send_to_char(buf, ch);
            }
        }

        // Handle radiate aura
        if (paf->type == gsn_radiateaura)
        {
            sprintf(buf, "     : 'aura of %s'\n", auraBaseName(static_cast<SpiritAura>(paf->modifier)));
            send_to_char(buf, ch);
        }

        // Handle chant litany
        if (paf->type == gsn_chantlitany)
        {
            sprintf(buf, "     : 'litany of %s'\n", litanyName(static_cast<LitanyType>(paf->modifier)));
            send_to_char(buf, ch);
        }
    }

    rFound = FALSE;
    if (!IS_NPC(victim))
	for (i = 0; trait_table[i].name; i++)
	    if (BIT_GET(victim->pcdata->traits, trait_table[i].bit))
	    {
		if (rFound)
		    sprintf(buf, ", {W%s{w", trait_table[i].name);
		else
		{
		    rFound = TRUE;
		    sprintf(buf, "{wTrts{D: {W%s{w", trait_table[i].name);
		}
		send_to_char(buf, ch);
	    }

    if (rFound)
	send_to_char("\n\r", ch);

    if (victim->religion >= 0)
    {
        sprintf(buf, "%s is a follower of %s.\n\r", victim->name, god_table[victim->religion].name);
        send_to_char(buf, ch);
    }

    const Faction * faction(FactionTable::LookupFor(*victim));
    if (faction != NULL)
    {
        sprintf(buf, "This mob is a member of %s.\n", faction->Name().c_str());
        send_to_char(buf,ch);
    }

    if (IS_NPC(victim) && victim->pIndexData->progtypes)
        send_to_char("This mob has progs.\n\r",ch);
}


void do_mstat( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int i;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Stat whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if (argument[0] != '\0')
    {
	if (!str_cmp(argument, "sacrifices"))
	{
	    if (IS_NPC(victim))
	    {
		send_to_char("NPC sacrifices are not tracked.\n\r",ch);
		return;
	    }

	    bool found = FALSE;
	    for (i=0;i<MAX_GODS;i++)
	    {
		if (victim->pcdata->sacrifices[i].level > 0
		  || victim->pcdata->sacrifices[i].number > 0)
		{
		    if (!found)
		    {
			sprintf(buf, "%s has made these offerings:\n\r",APERS(victim, ch));
			send_to_char(buf,ch);
			sprintf(buf, "%15s %7s %7s %7s\n\r",
			  "Name","Levels","Number","Average");
			send_to_char(buf,ch);
			sprintf(buf,"%15s %7d %7d %7d\n\r",
			  god_table[i].name,
			  victim->pcdata->sacrifices[i].level,
			  victim->pcdata->sacrifices[i].number,
			  victim->pcdata->sacrifices[i].level/
			    victim->pcdata->sacrifices[i].number);
			send_to_char(buf,ch);
			found=TRUE;
		    }
		    else
		    {
			sprintf(buf,"%15s %7d %7d %7d\n\r",
			  god_table[i].name,
			  victim->pcdata->sacrifices[i].level,
			  victim->pcdata->sacrifices[i].number,
			  victim->pcdata->sacrifices[i].level/
			    victim->pcdata->sacrifices[i].number);
			send_to_char(buf,ch);
		    }
		}
	    }

	    if (!found)
	    {
		sprintf(buf,"%s has made no sacrifices.\n\r",APERS(victim,ch));
		send_to_char(buf,ch);
	    }

	    return;
	}
	else if (!str_cmp(argument, "symbols"))
	{
	    bool found = FALSE;

	    sprintf(buf, "%s has learned the following symbols:\n\r\n\r", APERS(victim, ch));
	    send_to_char(buf, ch);
	    for (i = 0; inscribe_table[i].name; i++)
	        if (IS_IMMORTAL(victim) || IS_SET(victim->symbols_known, inscribe_table[i].bit))
	        {
		    found = TRUE;
		    send_to_char(inscribe_table[i].name, ch);
		    send_to_char("\n\r", ch);
	        }

	    if (!found)
	    {
		sprintf(buf, "%s has no knowledge of any symbols.\n\r", APERS(victim, ch));
		send_to_char(buf, ch);
	    }

	    return;
	}

	send_to_char("Valid extra arguments are: symbols, sacrifices.\n\r", ch);

	return;
    }

    mstat(ch, victim);	    

    return;
}


CHAR_DATA *load_offline_char(char *name)
{
    FILE *fp;
    char arg[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    CHAR_DATA *ch;

    for (d = descriptor_list; d; d = d->next)
    {
	if ((ch = d->original ? d->original : d->character) == NULL)
	    continue;

	if (!str_cmp(ch->name, name))
	    return NULL;
    }

    sprintf(arg, "%s%s", PLAYER_DIR, capitalize(name));

    if (fp = fopen(arg, "r"))
    {
	fclose(fp);
    	ol_char = new_char();
	load_char_obj(name, ol_char);
	ol_char->was_in_room = ol_char->in_room;
	ol_char->in_room = NULL;
    }
    else
	return NULL;

    return ol_char;
}

void do_listmaterialbiases(CHAR_DATA * ch)
{
	char buf[MAX_STRING_LENGTH];
	char disp[MAX_STRING_LENGTH * 10];
	struct elemental_bias_type elemental_bias;
	struct platonic_bias_type platonic_bias;
	char * temp;
	int i;

	disp[0] = '\0';
	
	for (i = 0; i < MAX_MATERIALS; i++)
	{
		sprintf(buf, "{G%s:{x\n", material_table[i].name);
		strcat(disp, buf);
		memset(&elemental_bias, 0, sizeof(struct elemental_bias_type));
		memset(&platonic_bias, 0, sizeof(struct platonic_bias_type));
		sum_elements(&elemental_bias, &material_table[i].elem_bias);
                sum_platonics(&platonic_bias, &material_table[i].plat_bias);
		temp = read_elemental_biases(&elemental_bias);
		sprintf(buf, temp);
		free_string(temp);
		if (buf[0] != '\0')
		{
			strcat(disp, "Elemental biases\n\r");
			strcat(disp, buf);
			strcat(disp, "\n\r");
		}
		temp = read_platonic_biases(&platonic_bias);
		sprintf(buf, temp);
		free_string(temp);
		if (buf[0] != '\0')
		{
			strcat(disp, "Platonic biases\n\r");
			strcat(disp, buf);
			strcat(disp, "\n\r");
		}
	}

	page_to_char(disp, ch);
	
	return;
}

void do_listtypebiases(CHAR_DATA * ch)
{
	char buf[MAX_STRING_LENGTH];
	char disp[MAX_STRING_LENGTH * 10];
	struct elemental_bias_type elemental_bias;
	struct platonic_bias_type platonic_bias;
	char * temp;
	int i;

	disp[0] = '\0';

	i = 0;
	while (item_table[i].name != NULL)
	{
		sprintf(buf, "{Y%s:{x\n", item_table[i].name);
		strcat(disp, buf);
		memset(&elemental_bias, 0, sizeof(struct elemental_bias_type));
		memset(&platonic_bias, 0, sizeof(struct platonic_bias_type));
		sum_elements(&elemental_bias, &item_table[i].elem_bias);
		sum_platonics(&platonic_bias, &item_table[i].plat_bias);
		temp = read_elemental_biases(&elemental_bias);
		sprintf(buf, temp);
		free_string(temp);
		if (buf[0] != '\0')
		{
			strcat(disp, "Elemental biases\n\r");
			strcat(disp, buf);
			strcat(disp, "\n\r");
		}
		temp = read_platonic_biases(&platonic_bias);
		sprintf(buf, temp);
		free_string(temp);
		if (buf[0] != '\0')
		{
			strcat(disp, "Platonic biases\n\r");
			strcat(disp, buf);
			strcat(disp, "\n\r");
		}
		i++;
	}

	page_to_char(disp, ch);
													       
	return;
}

void do_version(CHAR_DATA * ch, char * argument)
{
	std::ostringstream mess;
	mess << "Avendar: The Crucible of Configuration Management" << std::endl;
	mess << "Version:    \"{W" << GetBuildVersion() << "{x\"" << std::endl;
	mess << "Build Time: \"{W" << GetBuildTime() << "{x\"" << std::endl;
	mess << "Build Path: \"{W" << GetBuildPath() << "{x\"" << std::endl;
	send_to_char(mess.str().c_str(), ch);
}

//Pulls up material and item type bias list for alchemical products
void do_biaslist ( CHAR_DATA *ch, char *argument )
{
	char arg[MAX_INPUT_LENGTH];
	argument = one_argument(argument, arg);
	if (UPPER(arg[0]) == 'M')
		do_listmaterialbiases(ch);
	else if (UPPER(arg[0]) == 'T')
		do_listtypebiases(ch);
	else
	{
		do_listmaterialbiases(ch);
		do_listtypebiases(ch);
	}
	
	return;
}

void do_showlimits(CHAR_DATA * ch, char * argument)
{
	char lower[MAX_INPUT_LENGTH];
	char upper[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	BUFFER * displayBuffer = NULL;
	int lowerLimit = 0;
	int upperLimit = INT_MAX;
	int vnum = 0;
	bool found = FALSE;
	OBJ_INDEX_DATA * pObjIndex = NULL;
	
	// Grab the arguments from the string
	argument = one_argument(argument, lower);
	argument = one_argument(argument, upper);

	// Validate the arguments; on failure, display the proper syntax and bail
	if (lower[0] == '\0' || !is_number(lower) || (upper[0] != '\0' && !is_number(upper)))
	{
		send_to_char("Syntax:\n\r",ch);
	    send_to_char("  showlimits <lower_limit>: displays all limit factors at <lower_limit> or higher\n\r",ch);
	    send_to_char("  showlimits <lower_limit> <upper_limit>: displays all limit factors between <lower_limit> and <upper_limit>, inclusive\n\r",ch);
		return;
	}

	// Convert the strings to numbers; no need to check sign, as if they input a negative number,
	// they just won't get results back.  Not a problem.
	lowerLimit = atoi(lower);
	if (upper[0] != '\0')
		upperLimit = atoi(upper);

	// Create (and check) the display buffer
	displayBuffer = new_buf();
	if (displayBuffer == NULL)
	{
		send_to_char("Could not process command; unable to allocate memory\n\r", ch);
		return;
	}

	// Iterate over all the vnums
    int matchedVnums(0);
	for (vnum = 0; matchedVnums < top_obj_index; vnum++)
	{
		if ((pObjIndex = get_obj_index(vnum)) != NULL)
		{
            ++matchedVnums;

			// Check for the limit factor being within bounds
			if (pObjIndex->limit_factor >= lowerLimit && pObjIndex->limit_factor <= upperLimit)
			{
				found = TRUE;
				sprintf(buf, "[%5u] Limit factor = %4u, Limit = %4u (%s)\n\r", 
						pObjIndex->vnum, pObjIndex->limit_factor, pObjIndex->limit, pObjIndex->short_descr);
				add_buf(displayBuffer, buf);
			}
		}
	}

	// Let them know if nothing was found
	if (!found)
		add_buf(displayBuffer, "No objects matching your criteria were found\n\r");

	// Show them the results, then free the display buffer
	page_to_char(buf_string(displayBuffer), ch);
	free_buf(displayBuffer);
}

/* RT to replace the 3 stat commands */
void do_stat ( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   char *string;
   OBJ_DATA *obj;
   ROOM_INDEX_DATA *location;
   CHAR_DATA *victim;

   if (!ch->in_room)
	return;

   string = one_argument(argument, arg);
   if ( arg[0] == '\0')
   {
	send_to_char("Syntax:\n\r",ch);
	send_to_char("  stat <name>\n\r",ch);
	send_to_char("  stat obj <name>\n\r",ch);
	send_to_char("  stat mob <name>\n\r",ch);
 	send_to_char("  stat room <number>\n\r",ch);
	return;
   }

   if (!str_cmp(arg,"room"))
   {
	do_rstat(ch,string);
	return;
   }
  
   if (!str_cmp(arg,"obj"))
   {
	do_ostat(ch,string);
	return;
   }

   if(!str_cmp(arg,"char")  || !str_cmp(arg,"mob"))
   {
	do_mstat(ch,string);
	return;
   }

    if (!str_cmp(arg, "offline"))
    {
	if (load_offline_char(string))
	{
	    mstat(ch, ol_char);
	    extract_char(ol_char, TRUE);
	    ol_char = NULL;
	}
	else
	    send_to_char("Character already online or does not exist!\n\r", ch);

	return;
    }
   
   /* do it the old way */

   if ((victim = get_char_room(ch, arg)) == NULL)
	victim = get_char_world(ch,arg);

    if (victim != NULL)
    {
        do_mstat(ch,argument);
        return;
    }

    if ((obj = get_obj_list(ch, arg, ch->in_room->contents)) == NULL)
	obj = get_obj_world(ch,arg);

    if (obj != NULL)
    {
        do_ostat(ch,arg);
        return;
    }

    location = find_location(ch, NULL, argument);
    if (location != NULL)
    {
        do_rstat(ch,argument);
        return;
    }

    send_to_char("Nothing by that name found anywhere.\n\r",ch);
}


/* ofind and mfind replaced with vnum, vnum skill also added */

void do_vnum(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char *string;

    string = one_argument(argument,arg);
 
    if (arg[0] == '\0')
    {
	send_to_char("Syntax:\n\r",ch);
	send_to_char("  vnum obj <name>\n\r",ch);
	send_to_char("  vnum mob <name>\n\r",ch);
	send_to_char("  vnum skill <skill or spell>\n\r",ch);
	return;
    }

    if (!str_cmp(arg,"obj"))
    {
	do_ofind(ch,string);
 	return;
    }

    if (!str_cmp(arg,"mob") || !str_cmp(arg,"char"))
    { 
	do_mfind(ch,string);
	return;
    }

    if (!str_cmp(arg,"skill") || !str_cmp(arg,"spell"))
    {
	do_slookup(ch,string);
	return;
    }
    /* do both */
    do_mfind(ch,argument);
    do_ofind(ch,argument);
}


void do_mfind( CHAR_DATA *ch, char *argument )
{
    extern int top_mob_index;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    MOB_INDEX_DATA *pMobIndex;
    int vnum;
    int nMatch;
    bool fAll;
    bool found;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Find whom?\n\r", ch );
	return;
    }

    fAll	= FALSE; /* !str_cmp( arg, "all" ); */
    found	= FALSE;
    nMatch	= 0;

    /*
     * Yeah, so iterating over all vnum's takes 10,000 loops.
     * Get_mob_index is fast, and I don't feel like threading another link.
     * Do you?
     * -- Furey
     */
    for ( vnum = 0; nMatch < top_mob_index; vnum++ )
    {
	if ( ( pMobIndex = get_mob_index( vnum ) ) != NULL )
	{
	    nMatch++;
	    if ( fAll || is_name( argument, pMobIndex->player_name ) )
	    {
		found = TRUE;
		sprintf( buf, "[%5d] %s\n\r",
		    pMobIndex->vnum, pMobIndex->short_descr );
		send_to_char( buf, ch );
	    }
	}
    }

    if ( !found )
	send_to_char( "No mobiles by that name.\n\r", ch );

    return;
}

typedef bool search_fun	args( ( void *v_ptr, unsigned int sOffset, void *data, char *opr ) );

bool vsearch_name(void *v_ptr, unsigned int sOffset, void *data, char *opr)
{
    if (is_name((char *) data, *(char **)((char *) v_ptr + sOffset)))
	return TRUE;

    return FALSE;
}

bool vsearch_flag(void *v_ptr, unsigned int sOffset, void *data, char *opr)
{
    if (IS_SET(*(int *)((int)v_ptr + sOffset), *((int *) data)))
	return TRUE;
    else
	return FALSE;
}

bool vsearch_compare_short(void *v_ptr, unsigned int sOffset, void *data, char *opr)
{
    int i;

    if (opr[0] != '\0')
	i = atoi((char *) data);
    else
	i = *((int *) data);

    if ( opr[0] == '\0' || !str_cmp( opr, "==" ) )
	return ( *(int *)((unsigned int)v_ptr + sOffset) == i);
    if ( !str_cmp( opr, "!=" ) )
	return ( *(int *)((unsigned int)v_ptr + sOffset) != i);
    if ( !str_cmp( opr, ">" ) )
	return ( *(int *)((unsigned int)v_ptr + sOffset) > i);
    if ( !str_cmp( opr, "<" ) )
	return ( *(int *)((unsigned int)v_ptr + sOffset) < i);
    if ( !str_cmp( opr, "<=" ) )
	return ( *(int *)((unsigned int)v_ptr + sOffset) <= i);
    if ( !str_cmp( opr, ">=" ) )
	return ( *(int *)((unsigned int)v_ptr + sOffset) >= i);
    if ( !str_cmp( opr, "&" ) )
	return ( *(int *)((unsigned int)v_ptr + sOffset) & i);
    if ( !str_cmp( opr, "|" ) )
	return ( *(int *)((unsigned int)v_ptr + sOffset) | i);

    return FALSE;
}

bool vsearch_compare(void *v_ptr, unsigned int sOffset, void *data, char *opr)
{
    int i;

    if (opr[0] != '\0')
	i = atoi((char *) data);
    else
	i = *((int *) data);

    if ( opr[0] == '\0' || !str_cmp( opr, "==" ) )
	return ( *(int *)((unsigned int)v_ptr + sOffset) == i);
    if ( !str_cmp( opr, "!=" ) )
	return ( *(int *)((unsigned int)v_ptr + sOffset) != i);
    if ( !str_cmp( opr, ">" ) )
	return ( *(int *)((unsigned int)v_ptr + sOffset) > i);
    if ( !str_cmp( opr, "<" ) )
	return ( *(int *)((unsigned int)v_ptr + sOffset) < i);
    if ( !str_cmp( opr, "<=" ) )
	return ( *(int *)((unsigned int)v_ptr + sOffset) <= i);
    if ( !str_cmp( opr, ">=" ) )
	return ( *(int *)((unsigned int)v_ptr + sOffset) >= i);
    if ( !str_cmp( opr, "&" ) )
	return ( *(int *)((unsigned int)v_ptr + sOffset) & i);
    if ( !str_cmp( opr, "|" ) )
	return ( *(int *)((unsigned int)v_ptr + sOffset) | i);

    return FALSE;
}


DO_FUNC (do_vsearch)
{
    char arg[MAX_INPUT_LENGTH];
    char argstr[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    BUFFER *buffer;
    char *argptr;
    unsigned int sOffset;
    search_fun *func_ptr;
    int i, nMatch, rv_int;
    bool found;
    void *rv;
    
    buffer = new_buf();
    argument = one_argument(argument, arg);

    if (!str_cmp(arg, "obj") || !str_cmp(arg, "object"))
    {
	OBJ_INDEX_DATA *obj;

	if (argument[0] == '\0')
	{
	    send_to_char("Valid search fields for objects are:\n\r", ch);
	    send_to_char("  Substring searches:\n\r", ch);
	    send_to_char("      name short desc lore\n\r", ch);
	    send_to_char("  Value searches:\n\r", ch);
	    send_to_char("      vnum resets level weight size limit cost v0 v1 v2 v3 v4\n\r", ch);
	    send_to_char("  Flag searches:\n\r", ch);
	    send_to_char("      material type extra wear\n\r", ch);
	    return;
	}

	for (i = 0; i < MAX_KEY_HASH; i++)
	    for (obj = obj_index_hash[i]; obj; obj = obj->next)
		obj->sFlag = TRUE;

	do
	{
	    argument = one_argument(argument, argstr);
	    argptr = one_argument(argstr, arg);

	    if (!str_cmp(arg, "name"))
	    {
		sOffset = offsetof(struct obj_index_data, name);
		func_ptr = vsearch_name;
		rv = (void *) argptr;
	    }
	    else if (!str_cmp(arg, "short"))
	    {
		sOffset = offsetof(struct obj_index_data, short_descr);
		func_ptr = vsearch_name;
		rv = (void *) argptr;
	    }
	    else if (!str_cmp(arg, "desc"))
	    {
		sOffset = offsetof(struct obj_index_data, description);
		func_ptr = vsearch_name;
		rv = (void *) argptr;
	    }
	    else if (!str_cmp(arg, "lore"))
	    {
		sOffset = offsetof(struct obj_index_data, lore);
		func_ptr = vsearch_name;
		rv = (void *) argptr;
	    }
	    else if (!str_cmp(arg, "vnum"))
	    {
		sOffset = offsetof(struct obj_index_data, vnum);
		func_ptr = vsearch_compare_short;
		rv = argptr = one_argument(argptr, arg);
	    }
	    else if (!str_cmp(arg, "resets"))
	    {
		sOffset = offsetof(struct obj_index_data, reset_num);
		func_ptr = vsearch_compare_short;
		rv = argptr = one_argument(argptr, arg);
	    }
	    else if (!str_cmp(arg, "material"))
	    {
		sOffset = offsetof(struct obj_index_data, material);
		func_ptr = vsearch_compare;
		rv_int = material_lookup(argptr);
		rv = (void *) &rv_int;
		strcpy(arg, "");
	    }
	    else if (!str_cmp(arg, "type"))
	    {
		sOffset = offsetof(struct obj_index_data, item_type);
		func_ptr = vsearch_compare;
		if ((rv_int = flag_value(type_flags, argptr)) == NO_FLAG)
		{
		    sprintf(buf, "Invalid 'type' flag: %s.\n\r", argptr);
		    send_to_char(buf, ch);
		    return;
		}
		rv = (void *) &rv_int;
		strcpy(arg, "");
	    }
	    else if (!str_cmp(arg, "extra"))
        {
            if ((rv_int = flag_value(extra_flags[0], argptr)) != NO_FLAG)
                sOffset = offsetof(struct obj_index_data, extra_flags[0]);
            else if ((rv_int = flag_value(extra_flags[1], argptr)) != NO_FLAG)
                sOffset = offsetof(struct obj_index_data, extra_flags[1]);
            else if ((rv_int = flag_value(extra_flags[2], argptr)) != NO_FLAG)
                sOffset = offsetof(struct obj_index_data, extra_flags[2]);
            else
            {
                sprintf(buf, "Invalid 'extra' flag: %s.\n\r", argptr);
                send_to_char(buf, ch);
                return;
            }

            func_ptr = vsearch_flag;
            rv = (void *) &rv_int;
        }
	    else if (!str_cmp(arg, "wear"))
	    {
		sOffset = offsetof(struct obj_index_data, wear_flags);
		func_ptr = vsearch_flag;
		if ((rv_int = flag_value(wear_flags, argptr)) == NO_FLAG)
		{
		    sprintf(buf, "Invalid 'wear' flag: %s.\n\r", argptr);
		    send_to_char(buf, ch);
		    return;
		}
		rv = (void *) &rv_int;
	    }
	    else if (!str_cmp(arg, "level"))
	    {
		sOffset = offsetof(struct obj_index_data, level);
		func_ptr = vsearch_compare_short;
		rv = argptr = one_argument(argptr, arg);
	    }
	    else if (!str_cmp(arg, "weight"))
	    {
		sOffset = offsetof(struct obj_index_data, weight);
		func_ptr = vsearch_compare_short;
		rv = argptr = one_argument(argptr, arg);
	    }
	    else if (!str_cmp(arg, "size"))
	    {
		sOffset = offsetof(struct obj_index_data, size);
		func_ptr = vsearch_compare_short;
		rv = argptr = one_argument(argptr, arg);
	    }
        else if (!str_cmp(arg, "limit"))
        {
            sOffset = offsetof(struct obj_index_data, limit);
            func_ptr = vsearch_compare_short;
            rv = argptr = one_argument(argptr, arg);
        }
	    else if (!str_cmp(arg, "cost"))
	    {
		sOffset = offsetof(struct obj_index_data, cost);
		func_ptr = vsearch_compare;
		rv = argptr = one_argument(argptr, arg);
	    }
	    else if (!str_cmp(arg, "v0"))
	    {
		sOffset = offsetof(struct obj_index_data, value[0]);
		func_ptr = vsearch_compare;
		rv = argptr = one_argument(argptr, arg);
	    }
	    else if (!str_cmp(arg, "v1"))
	    {
		sOffset = offsetof(struct obj_index_data, value[1]);
		func_ptr = vsearch_compare;
		rv = argptr = one_argument(argptr, arg);
	    }
	    else if (!str_cmp(arg, "v2"))
	    {
		sOffset = offsetof(struct obj_index_data, value[2]);
		func_ptr = vsearch_compare;
		rv = argptr = one_argument(argptr, arg);
	    }
	    else if (!str_cmp(arg, "v3"))
	    {
		sOffset = offsetof(struct obj_index_data, value[3]);
		func_ptr = vsearch_compare;
		rv = argptr = one_argument(argptr, arg);
	    }
	    else if (!str_cmp(arg, "v4"))
	    {
		sOffset = offsetof(struct obj_index_data, value[4]);
		func_ptr = vsearch_compare;
		rv = argptr = one_argument(argptr, arg);
	    }
	    else
	    {
		sprintf(buf, "Bad search keyword: %s\n\r", arg);
		send_to_char(buf, ch);
		return;
	    }

	    for (i = 0; i < MAX_KEY_HASH; i++)
		for (obj = obj_index_hash[i]; obj; obj = obj->next)
		    if (obj->sFlag && !func_ptr(obj, sOffset, rv, arg))
			obj->sFlag = FALSE;
	} while (argument[0] != '\0');

	found = FALSE;
	nMatch = 0;
	for (i = 0; nMatch < top_obj_index; i++)
	    if (obj = get_obj_index(i))
	    {
		nMatch++;

		if (obj->sFlag)
		{
		    found = TRUE;
		    sprintf(buf, "[%5d] %s\n\r", i, obj->short_descr);
		    add_buf(buffer, buf);
		}
	    }

	if (!found)
	    send_to_char("No matching objects found.\n\r", ch);
	else page_to_char(buf_string(buffer), ch);

	return;
    }
    else if (!str_cmp(arg, "mob") || !str_cmp(arg, "mobile"))
    {
	MOB_INDEX_DATA *mob;

	if (argument[0] == '\0')
	{
	    send_to_char("Valid search fields for objects are:\n\r", ch);
	    send_to_char("  Substring searches:\n\r", ch);
	    send_to_char("      name short long desc damverb\n\r", ch);
	    send_to_char("  Value searches:\n\r", ch);
	    send_to_char("      vnum alignment hitroll level sex wealth size\n\r", ch);
	    send_to_char("  Flag searches:\n\r", ch);
	    send_to_char("      act nact aff off imm race form part material\n\r", ch);
	    return;
	}

	for (i = 0; i < MAX_KEY_HASH; i++)
	    for (mob = mob_index_hash[i]; mob; mob = mob->next)
		mob->sFlag = TRUE;

	do
	{
	    argument = one_argument(argument, argstr);
	    argptr = one_argument(argstr, arg);

	    if (!str_cmp(arg, "vnum"))
	    {
		sOffset = offsetof(struct mob_index_data, vnum);
		func_ptr = vsearch_compare_short;
		rv = argptr = one_argument(argptr, arg);
	    }
	    else if (!str_cmp(arg, "name"))
	    {
		sOffset = offsetof(struct mob_index_data, player_name);
		func_ptr = vsearch_name;
		rv = (void *) argptr;
	    }
	    else if (!str_cmp(arg, "short"))
	    {
		sOffset = offsetof(struct mob_index_data, short_descr);
		func_ptr = vsearch_name;
		rv = (void *) argptr;
	    }
	    else if (!str_cmp(arg, "long"))
	    {
		sOffset = offsetof(struct mob_index_data, long_descr);
		func_ptr = vsearch_name;
		rv = (void *) argptr;
	    }
	    else if (!str_cmp(arg, "desc"))
	    {
		sOffset = offsetof(struct obj_index_data, description);
		func_ptr = vsearch_name;
		rv = (void *) argptr;
	    }
	    else if (!str_cmp(arg, "act"))
	    {
		sOffset = offsetof(struct mob_index_data, act);
		func_ptr = vsearch_flag;
		if ((rv_int = flag_value(act_flags, argptr)) == NO_FLAG)
		{
		    sprintf(buf, "Invalid 'act' flag: %s.\n\r", argptr);
		    send_to_char(buf, ch);
		    return;
		}
		rv = (void *) &rv_int;
	    }
	    else if (!str_cmp(arg, "nact"))
	    {
		sOffset = offsetof(struct mob_index_data, nact);
		func_ptr = vsearch_flag;
		if ((rv_int = flag_value(nact_flags, argptr)) == NO_FLAG)
		{
		    sprintf(buf, "Invalid 'nact' flag: %s.\n\r", argptr);
		    send_to_char(buf, ch);
		    return;
		}
		rv = (void *) &rv_int;
	    }
	    else if (!str_cmp(arg, "aff"))
	    {
		sOffset = offsetof(struct mob_index_data, affected_by);
		func_ptr = vsearch_flag;
		if ((rv_int = flag_value(affect_flags, argptr)) == NO_FLAG)
		{
		    sprintf(buf, "Invalid 'aff' flag: %s.\n\r", argptr);
		    send_to_char(buf, ch);
		    return;
		}
		rv = (void *) &rv_int;
	    }
	    else if (!str_cmp(arg, "alignment"))
	    {
		sOffset = offsetof(struct mob_index_data, alignment);
		func_ptr = vsearch_compare_short;
		rv = argptr = one_argument(argptr, arg);
	    }
	    else if (!str_cmp(arg, "level"))
	    {
		sOffset = offsetof(struct mob_index_data, level);
		func_ptr = vsearch_compare_short;
		rv = argptr = one_argument(argptr, arg);
	    }
	    else if (!str_cmp(arg, "hitroll"))
	    {
		sOffset = offsetof(struct mob_index_data, hitroll);
		func_ptr = vsearch_compare_short;
		rv = argptr = one_argument(argptr, arg);
	    }
	    else if (!str_cmp(arg, "damverb"))
	    {
		sOffset = offsetof(struct mob_index_data, dam_verb);
		func_ptr = vsearch_name;
		rv = (void *) argptr;
	    }
	    else if (!str_cmp(arg, "off"))
	    {
		sOffset = offsetof(struct mob_index_data, off_flags);
		func_ptr = vsearch_flag;
		if ((rv_int = flag_value(off_flags, argptr)) == NO_FLAG)
		{
		    sprintf(buf, "Invalid 'off' flag: %s.\n\r", argptr);
		    send_to_char(buf, ch);
		    return;
		}
		rv = (void *) &rv_int;
	    }
	    else if (!str_cmp(arg, "imm"))
	    {
		sOffset = offsetof(struct mob_index_data, imm_flags);
		func_ptr = vsearch_flag;
		if ((rv_int = flag_value(imm_flags, argptr)) == NO_FLAG)
		{
		    sprintf(buf, "Invalid 'imm' flag: %s.\n\r", argptr);
		    send_to_char(buf, ch);
		    return;
		}
		rv = (void *) &rv_int;
	    }
	    else if (!str_cmp(arg, "sex"))
	    {
		sOffset = offsetof(struct mob_index_data, sex);
		func_ptr = vsearch_compare_short;
		rv = argptr = one_argument(argptr, arg);
	    }
	    else if (!str_cmp(arg, "race"))
	    {
		sOffset = offsetof(struct mob_index_data, race);
		func_ptr = vsearch_compare;
		rv_int = race_lookup(argptr);
		rv = (void *) &rv_int;
		strcpy(arg, "");
	    }
	    else if (!str_cmp(arg, "wealth"))
	    {
		sOffset = offsetof(struct mob_index_data, wealth);
		func_ptr = vsearch_compare;
		rv = argptr = one_argument(argptr, arg);
	    }
	    else if (!str_cmp(arg, "form"))
	    {
		sOffset = offsetof(struct mob_index_data, form);
		func_ptr = vsearch_flag;
		if ((rv_int = flag_value(form_flags, argptr)) == NO_FLAG)
		{
		    sprintf(buf, "Invalid 'form' flag: %s.\n\r", argptr);
		    send_to_char(buf, ch);
		    return;
		}
		rv = (void *) &rv_int;
	    }
	    else if (!str_cmp(arg, "part"))
	    {
		sOffset = offsetof(struct mob_index_data, parts);
		func_ptr = vsearch_flag;
		if ((rv_int = flag_value(part_flags, argptr)) == NO_FLAG)
		{
		    sprintf(buf, "Invalid 'part' flag: %s.\n\r", argptr);
		    send_to_char(buf, ch);
		    return;
		}
		rv = (void *) &rv_int;
	    }
	    else if (!str_cmp(arg, "size"))
	    {
		sOffset = offsetof(struct mob_index_data, size);
		func_ptr = vsearch_compare_short;
		rv = argptr = one_argument(argptr, arg);
	    }
	    else if (!str_cmp(arg, "material"))
	    {
		sOffset = offsetof(struct mob_index_data, material);
		func_ptr = vsearch_compare;
		rv_int = material_lookup(argptr);
		rv = (void *) &rv_int;
		strcpy(arg, "");
	    }
	    else
	    {
		sprintf(buf, "Bad search keyword: %s\n\r", arg);
		send_to_char(buf, ch);
		return;
	    }

	    for (i = 0; i < MAX_KEY_HASH; i++)
		for (mob = mob_index_hash[i]; mob; mob = mob->next)
		    if (mob->sFlag && !func_ptr(mob, sOffset, rv, arg))
			mob->sFlag = FALSE;
	} while (argument[0] != '\0');

	found = FALSE;
	nMatch = 0;
	for (i = 0; nMatch < top_mob_index; i++)
	    if (mob = get_mob_index(i))
	    {
		nMatch++;

		if (mob->sFlag)
		{
		    found = TRUE;
		    sprintf(buf, "[%5d] %s\n\r", i, mob->short_descr);
		    add_buf(buffer, buf);
		}
	    }

	if (!found)
	    send_to_char("No matching mobiles found.\n\r", ch);
	else page_to_char(buf_string(buffer), ch);

	return;
    }

    send_to_char("Syntax: vsearch <mob/obj> [search conditions]\n\r", ch);
    return;
}


void do_ofind( CHAR_DATA *ch, char *argument )
{
    extern int top_obj_index;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    OBJ_INDEX_DATA *pObjIndex;
    int vnum;
    int nMatch;
    bool fAll;
    bool found;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Find what?\n\r", ch );
	return;
    }

    fAll	= FALSE; /* !str_cmp( arg, "all" ); */
    found	= FALSE;
    nMatch	= 0;

    /*
     * Yeah, so iterating over all vnum's takes 10,000 loops.
     * Get_obj_index is fast, and I don't feel like threading another link.
     * Do you?
     * -- Furey
     */
    for ( vnum = 0; nMatch < top_obj_index; vnum++ )
    {
	if ( ( pObjIndex = get_obj_index( vnum ) ) != NULL )
	{
	    nMatch++;
	    if ( fAll || is_name( argument, pObjIndex->name ) )
	    {
		found = TRUE;
		sprintf( buf, "[%5d] %s\n\r",
		    pObjIndex->vnum, pObjIndex->short_descr );
		send_to_char( buf, ch );
	    }
	}
    }

    if ( !found )
	send_to_char( "No objects by that name.\n\r", ch );

    return;
}

void do_olimit(CHAR_DATA *ch, char *argument)
{
    int iHash;
    OBJ_INDEX_DATA *objIndex;
    bool oFound = FALSE;
    char buf[MAX_STRING_LENGTH];

    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
    {
	for (objIndex = obj_index_hash[iHash];
	     objIndex;
	     objIndex = objIndex->next)
	{
	    if ((objIndex->limit_factor > 0) && (objIndex->current > objIndex->limit))
	    {
	        sprintf(buf, "%s (Obj #%d) is overlimit.  (Limit: %d, Current: %d)\n\r",
			objIndex->short_descr,
			objIndex->vnum,
			objIndex->limit, 
			objIndex->current);
	        send_to_char(buf, ch);
	        oFound = TRUE;
	    }
	}
    }

    if (!oFound)
	send_to_char("No overlimit objects found.\n\r", ch);

    return;
}


void do_owhere(CHAR_DATA *ch, char *argument )
{
    char buf[MAX_INPUT_LENGTH];
    BUFFER *buffer;
    OBJ_DATA *obj;
    OBJ_DATA *in_obj;
    bool found;
    int number = 0, max_found;

    found = FALSE;
    number = 0;
    max_found = 200;

    buffer = new_buf();

    if (argument[0] == '\0')
    {
	send_to_char("Find what?\n\r",ch);
	return;
    }
 
    for ( obj = object_list; obj != NULL; obj = obj->next )
    {
        if ( !can_see_obj( ch, obj ) || !is_name( argument, obj->name ))
            continue;
 
        found = TRUE;
        number++;
 
        for ( in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj )
            ;
 
        if ( in_obj->carried_by != NULL && can_see(ch,in_obj->carried_by)
	&&   in_obj->carried_by->in_room != NULL)
            sprintf( buf, "%3d) %s is carried by %s [Room %d]\n\r",
                number, obj->short_descr,PERS(in_obj->carried_by, ch),
		in_obj->carried_by->in_room->vnum );
        else if (in_obj->in_room != NULL && can_see_room(ch,in_obj->in_room))
            sprintf( buf, "%3d) %s is in %s [Room %d]\n\r",
                number, obj->short_descr,in_obj->in_room->name, 
	   	in_obj->in_room->vnum);
	else
            sprintf( buf, "%3d) %s is somewhere\n\r",number, obj->short_descr);
 
        buf[0] = UPPER(buf[0]);
        add_buf(buffer,buf);
 
        if (number >= max_found)
            break;
    }
 
    if ( !found )
        send_to_char( "Nothing like that in heaven or earth.\n\r", ch );
    else
        page_to_char(buf_string(buffer),ch);

    free_buf(buffer);
}

void do_mrefresh( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *pMob;

    if (!str_cmp(argument, "room"))
    {
	if (ch->in_room)
	{
	    mob_obj_refresh(ch->in_room, NULL);
	    send_to_char("All mobiles in current room processed.\n\r", ch);
	}
	return;
    }

    if (!str_cmp(argument, "area"))
    {
	if (ch->in_room && ch->in_room->area)
	{
	    int vnum;
	    ROOM_INDEX_DATA *pRoom;
	    VNUM_RANGE *vrange;

	    for (vrange = ch->in_room->area->vnums; vrange; vrange = vrange->next)
	        for (vnum = vrange->min_vnum; vnum <= vrange->max_vnum; vnum++)
		    if ((pRoom = get_room_index(vnum)))
		        mob_obj_refresh(pRoom, NULL);

	    send_to_char("All mobiles in current area processed.\n\r", ch);
	}
	return;
    }

    if (!str_cmp(argument, "world"))
    {
	for (pMob = char_list; pMob; pMob = pMob->next)
	    if (IS_NPC(pMob) && pMob->in_room)
		mob_obj_refresh(pMob->in_room, pMob);
	send_to_char("All mobiles in world processed.\n\r", ch);
	return;
    }

    if ((argument[0] != '\0') && (pMob = get_char_world(ch, argument)))
    {
	if (pMob->in_room)
	{
	    mob_obj_refresh(pMob->in_room, pMob);
	    act("$N processed.", ch, NULL, pMob, TO_CHAR);
	}
	return;
    }

    send_to_char("Usage:\n\r\n\r", ch);
    send_to_char("  mrestore <mob name>\n\r", ch);
    send_to_char("  mrestore room\n\r", ch);
    send_to_char("  mrestore area\n\r", ch);
    send_to_char("  mrestore world\n\r", ch);
    return;
}

void do_mobcount( CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *vch;
    MOB_INDEX_DATA *goat;
    char buf[MAX_STRING_LENGTH*12];
    char lbuf[MAX_STRING_LENGTH];
    int mobs[65535];
    int x, count;

    for (x = 1;x < 65535;x++)
    {
	mobs[x]=0;
    }

    if (argument[0] == '\0' || (count = atoi(argument)) < 3)
    {
	send_to_char("Usage: mobcount <num (3+)>\n\r", ch);
	return;
    }


    buf[0] = 0;
    lbuf[0] = 0;

    for (vch = char_list;vch != NULL;vch = vch->next)
	if (IS_NPC(vch))
	    mobs[vch->pIndexData->vnum]++;

    for (x = 1;x < 65535;x++)
    {
	if (mobs[x] < count)
	  continue;
	if ((goat = get_mob_index(x)) == NULL)
	  continue;
	sprintf(lbuf, "[%5d] [%10.10s] --> %d\n", x, goat->short_descr, mobs[x]);
	strcat(buf, lbuf);
    }

    page_to_char(buf, ch);
	
}

void do_objcount( CHAR_DATA *ch, char *argument)
{
    OBJ_INDEX_DATA *pIndexData;
    char buf[MAX_STRING_LENGTH];
    int x, count;

    if ((argument[0] == '\0') || ((count = atoi(argument)) < 2))
    {
	send_to_char("Syntax: objcount <number>\n\r\n\rNote: <number> must be >= 2.", ch);
	return;
    }

    for (x = 1; x < 65535; x++)
    {
	pIndexData = get_obj_index(x);

	if (pIndexData && (pIndexData->current >= count))
        {
	    sprintf(buf, "[%5d] [%20.20s] --> %d\n\r", x, pIndexData->short_descr, pIndexData->current);
	    send_to_char(buf, ch);
	}
    }

    return;
}

DO_FUNC( do_aclist )
{
    DESCRIPTOR_DATA *d;
    CHAR_DATA *vch;
    char buf[MAX_STRING_LENGTH];
    BUFFER *buffer = new_buf();

    for (d = descriptor_list; d; d = d->next)
    {
	vch = d->original ? d->original : d->character;

	if (vch && (d->connected == CON_PLAYING) && !IS_IMMORTAL(vch) && can_see(ch, vch))
	{
	    if (d->acct)
		sprintf(buf, "%3d {%c%s{x\n\r", d->acct->award_points,
			d->acct->award_points <= -5 ? 'R' :
			d->acct->award_points < 0 ? 'r' :
			d->acct->award_points == 0 ? 'w' :
			d->acct->award_points < 5 ? 'g' :
			d->acct->award_points < 10 ? 'G' : 'W', PERS(vch, ch));
	    else
		sprintf(buf, "n/a {D%s{x\n\r", PERS(vch, ch));

	    add_buf(buffer, buf);
	}
    }

    page_to_char(buf_string(buffer), ch);
    free_buf(buffer);
    return;
}

   

void do_mwhere( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    BUFFER *buffer;
    CHAR_DATA *victim;
    bool found;
    int count = 0;

    if ( argument[0] == '\0' )
    {
	DESCRIPTOR_DATA *d;

	/* show characters logged */

	buffer = new_buf();
	for (d = descriptor_list; d != NULL; d = d->next)
	{
	    if (d->character != NULL && d->connected == CON_PLAYING
	    &&  d->character->in_room != NULL && can_see(ch,d->character)
	    &&  can_see_room(ch,d->character->in_room))
	    {
		victim = d->character;
		count++;
		if (d->original != NULL)
		    sprintf(buf,"%3d) %s (in the body of %s) is in %s [%d]\n\r",
			count, d->original->name,victim->short_descr,
			victim->in_room->name,victim->in_room->vnum);
		else
		    sprintf(buf,"%3d) %s is in %s [%d]\n\r",
			count, victim->name,victim->in_room->name,
			victim->in_room->vnum);
		add_buf(buffer,buf);
	    }
	}

        page_to_char(buf_string(buffer),ch);
	free_buf(buffer);
	return;
    }

    found = FALSE;
    buffer = new_buf();
    for ( victim = char_list; victim != NULL; victim = victim->next )
    {
	if ( victim->in_room != NULL
	&&   is_name( argument, victim->name ) )
	{
	    found = TRUE;
	    count++;
	    sprintf( buf, "%3d) [%5d] %-28s [%5d] %s\n\r", count,
		IS_NPC(victim) ? victim->pIndexData->vnum : 0,
		IS_NPC(victim) ? victim->short_descr : victim->name,
		victim->in_room->vnum,
		victim->in_room->name );
	    add_buf(buffer,buf);
	}
    }

    if ( !found )
	act( "You didn't find any $T.", ch, NULL, argument, TO_CHAR );
    else
    	page_to_char(buf_string(buffer),ch);

    free_buf(buffer);

    return;
}

void do_emitstats(CHAR_DATA * ch, char * argument)
{
    StatEmitter::EmitStats();
    send_to_char("Finished. We hope you enjoy your new stats.\n", ch);
}

void do_reboo( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to REBOOT, spell it out.\n\r", ch );
    return;
}

void do_reboot( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    extern bool merc_down;
    DESCRIPTOR_DATA *d,*d_next;
    CHAR_DATA *vch;

    if (ch->invis_level < LEVEL_HERO)
    {
    	sprintf( buf, "Reboot by %s.", ch->name );
    	do_echo( ch, buf );
    }

    merc_down = TRUE;
    for ( d = descriptor_list; d != NULL; d = d_next )
    {
	d_next = d->next;
	vch = d->original ? d->original : d->character;
	if (vch != NULL)
	    save_char_obj(vch);
    	close_socket(d);
    }
    
    return;
}



void do_shutdow( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to SHUTDOWN, spell it out.\n\r", ch );
    return;
}



void do_shutdown( CHAR_DATA *ch, char *argument )
{
//    char buf[MAX_STRING_LENGTH];
    extern bool merc_down;
    DESCRIPTOR_DATA *d,*d_next;
    CHAR_DATA *vch;
/*
    if (ch->invis_level < LEVEL_HERO)
    sprintf( buf, "Shutdown by %s.", ch->name );
    append_file( ch, SHUTDOWN_FILE, buf );
    strcat( buf, "\n\r" );
    if (ch->invis_level < LEVEL_HERO)
    	do_echo( ch, buf );
*/
    merc_down = TRUE;

    for ( d = descriptor_list; d != NULL; d = d_next)
    {
	d_next = d->next;
	vch = d->original ? d->original : d->character;
	if (vch != NULL)
	    save_char_obj(vch);
	close_socket(d);
    }

        
    return;
}

void do_snoopprotect( CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;

    if (argument[0] == '\0')
    {
	send_to_char("Protect whom from snooping?\n\r",ch);
	return;
    }

    if ((victim = get_char_world(ch,argument)) == NULL)
    {
	send_to_char("You can't find them.\n\r",ch);
	return;
    }

    if (IS_SET(victim->comm,COMM_SNOOP_PROOF))
    {
	act_new("$N is no longer snoop-proof.",ch,NULL,victim,TO_CHAR,POS_DEAD);
	send_to_char("Your snoop-proofing was just removed.\n\r",victim);
	REMOVE_BIT(victim->comm,COMM_SNOOP_PROOF);
    }
    else
    {
	act_new("$N is now snoop-proof.",ch,NULL,victim,TO_CHAR,POS_DEAD);
	send_to_char("You are now immune to snooping.\n\r",victim);
	SET_BIT(victim->comm,COMM_SNOOP_PROOF);
    }
}


void snoop(CHAR_DATA *ch, char *argument, short stype)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    DESCRIPTOR_DATA *d;
    int i;
    char *snoop_names[3] = { "snoop", "comm snoop", "brief snoop" };

    if (!ch->desc)
	return;

    //if (IS_NPC(ch))
    //{
    //	send_to_char("Mobiles cannot initiate snoops.\n\r", ch);
    //	return;
    //}

    if (argument[0] == '\0')
    {
	send_to_char("Snoop who?\n\r", ch);
	return;
    }

    one_argument( argument, arg );    

    if (!(victim = get_char_world(ch, arg)))
    {
	send_to_char("You cannot find them.\n\r", ch);
	return;
    }

    if (!is_room_owner(ch,victim->in_room) && ch->in_room != victim->in_room 
    &&  room_is_private(victim->in_room) && !IS_TRUSTED(ch,IMPLEMENTOR))
    {
        send_to_char("That character is in a private room.\n\r",ch);
        return;
    }

    /* The reasoning for this, is so that lower level immortals can't get a */
    /* csnoop started on higher-level immortals when they're switched.      */
    if (IS_NPC(victim) && (get_trust(ch) < MAX_LEVEL))
    {
	send_to_char("Snoops cannot be initiated on mobiles.\n\r", ch);
	return;
    }

    if (!victim->desc)
    {
	if (!IS_NPC(victim))
	    send_to_char("That character has no descriptor attached.\n\r", ch);
	else
	    send_to_char("That mobile has no descriptor attached.\n\r", ch);
	return;
    }

    if (ch == victim)
    {
	for (d = descriptor_list; d; d = d->next)
	{
	    d->snooped = FALSE;
	    for (i = 0; i < MAX_SNOOP; i++)
		if ((d->snoop_by[i] == ch->desc) && (d->snoop_type[i] == stype)) 
		    d->snoop_by[i] = NULL;
		else if (d->snoop_by[i])
		    d->snooped = TRUE;
	}

	sprintf(buf, "All %ss stopped.\n\r", snoop_names[stype]);
	send_to_char(buf, ch);
	return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) 
    ||   IS_SET(victim->comm,COMM_SNOOP_PROOF))
    {
	send_to_char( "You failed.\n\r", ch );
	return;
    }

    for (i = 0; i < MAX_SNOOP; i++)
    {
	if (ch->desc->snoop_by[i] == victim->desc)
	{
	    send_to_char("They are alreay snooping you.  Loops are not allowed.\n\r", ch);
	    return;
        }

        if (victim->desc->snoop_by[i] == ch->desc)
        {
	    if (victim->desc->snoop_type[i] == stype)
	    {
	        victim->desc->snoop_by[i] = NULL;
		sprintf(buf, "You stop %sing them.\n\r", snoop_names[stype]);
	        send_to_char(buf, ch); 
		return;
	    }
	    //else
	    // 	send_to_char("You are already performing another type of snoop on them.\n\r", ch);
	    //return;
	}
    }

    for (i = 0; i < 5; i++)
	if (!victim->desc->snoop_by[i])
	{
	    victim->desc->snooped = TRUE;
	    victim->desc->snoop_by[i] = ch->desc;
	    victim->desc->snoop_type[i] = stype;
	    sprintf(buf, "You begin %sing them.\n\r", snoop_names[stype]);
	    send_to_char(buf, ch);
	    sprintf(log_buf, "%s begins %sing %s.", ch->name, snoop_names[stype], victim->name);
	    wiznet(log_buf, ch, NULL, WIZ_SNOOPS, WIZ_SECURE, get_trust(ch));
	    return;
	}

    send_to_char("They are already fully snooped.\n\r", ch);
    return;
}

DO_FUNC(do_csnoop)
{
    snoop(ch, argument, SNOOP_COMM);
    return;
}

DO_FUNC(do_bsnoop)
{
    snoop(ch, argument, SNOOP_BRIEF);
    return;
}

DO_FUNC(do_snoop)
{
    snoop(ch, argument, SNOOP_NORMAL);
    return;
}

void do_switch( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );
    
    if ( arg[0] == '\0' )
    {
	send_to_char( "Switch into whom?\n\r", ch );
	return;
    }

    if ( ch->desc == NULL )
	return;
    
    if ( ch->desc->original != NULL )
    {
	send_to_char( "You are already switched.\n\r", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( victim == ch )
    {
	send_to_char( "Ok.\n\r", ch );
	return;
    }

    if (!IS_NPC(victim))
    {
	send_to_char("You can only switch into mobiles.\n\r",ch);
	return;
    }

    if (!is_room_owner(ch,victim->in_room) && ch->in_room != victim->in_room 
    &&  room_is_private(victim->in_room) && !IS_TRUSTED(ch,IMPLEMENTOR))
    {
	send_to_char("That character is in a private room.\n\r",ch);
	return;
    }

    if ( victim->desc != NULL )
    {
	send_to_char( "Character in use.\n\r", ch );
	return;
    }

    sprintf(buf,"$N switches into %s",victim->short_descr);
    wiznet(buf,ch,NULL,WIZ_SWITCHES,WIZ_SECURE,get_trust(ch));

    ch->desc->character = victim;
    ch->desc->original  = ch;
    victim->desc        = ch->desc;
    ch->desc            = NULL;
    /* change communications to match */
    if (ch->prompt != NULL)
        victim->prompt = str_dup(ch->prompt);
    victim->comm = ch->comm;
    victim->lines = ch->lines;
    send_to_char( "Ok.\n\r", victim );
    return;
}

void do_revert( CHAR_DATA *ch, char *argument )
{
    if (!is_affected(ch, gsn_wolfform) && !is_affected(ch, gsn_bearform) 
    && !is_affected(ch, gsn_hawkform) && !is_affected(ch, gsn_shadowfiend))
    {
    	send_to_char("You are already in your natural form.\n\r", ch);
	    return;
    }

    send_to_char("Your body shifts slightly as you return to your true form.\n\r", ch);
    act("$n's appearance shifts as $e returns to $s true form.", ch, NULL, NULL, TO_ROOM);

    affect_strip(ch, gsn_wolfform);
    affect_strip(ch, gsn_hawkform);
    affect_strip(ch, gsn_bearform);
    affect_strip(ch, gsn_shadowfiend);
}
    
void do_relinquish( CHAR_DATA *ch, char *argument )
{
    bool purge = FALSE;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( ch->desc == NULL )
	return;

    if ( ch->desc->original == NULL )
    {
	send_to_char( "You have nothing to relinquish.\n\r", ch );
	return;
    }

    if (IS_NPC(ch) && (ch->pIndexData->vnum == MOB_VNUM_SHUNNED_DEMON))
    {
	unbind_shunned_demon(ch);
	extract_char(ch, TRUE);
	return;
    }

    if (is_affected(ch, gsn_astralprojection))
    {
        for (CHAR_DATA * echoChar(ch->in_room == NULL ? NULL : ch->in_room->people); echoChar != NULL; echoChar = echoChar->next_in_room)
        {
            if (ch != echoChar && can_see(echoChar, ch))
                act("$n follows $s invisible tie back to $s mortal body, disappearing instantly.", ch, NULL, echoChar, TO_VICT);
        }

        purge = TRUE;
    }

    if (str_cmp(argument,"notext"))
        send_to_char("You push off, returning to your mortal body.\n\r", ch);

    if (ch->prompt != NULL)
    {
	free_string(ch->prompt);
	ch->prompt = NULL;
    }

    act("$n shakes $s head clear as $s spirit returns.", ch->desc->original, NULL, ch, TO_NOTVICT);

    WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
    WAIT_STATE(ch->desc->original, UMAX(ch->desc->original->wait, 2*PULSE_VIOLENCE));
    ch->desc->original->hit /= 2;

    if (is_affected(ch, gsn_possession))
    {
        affect_strip(ch, gsn_possession);
	af.where	= TO_AFFECTS;
	af.type		= gsn_possession;
	af.level	= ch->desc->original->level;
	af.duration	= 8;
	af.modifier	= 0;
	af.location	= 0;
	af.bitvector	= 0;
	affect_to_char(ch->desc->original, &af);
    }

    if (ch->desc != NULL)
    {
    ch->desc->character       = ch->desc->original;
    ch->desc->original        = NULL;
    ch->desc->character->desc = ch->desc; 
    ch->desc                  = NULL;
    }

    affect_strip(ch, gsn_astralprojection);

    if (purge)
     extract_char(ch, TRUE);
}

void do_return( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( ch->desc == NULL )
	return;

    if ( ch->desc->original == NULL )
    {
	send_to_char( "You aren't switched.\n\r", ch );
	return;
    }

    if (IS_NPC(ch) && ch->pIndexData->vnum == MOB_VNUM_SHUNNED_DEMON)
    {
	send_to_char("You cannot willfully unbind yourself from the tide of Bahhaoth.\n\r", ch);
	return;
    }

    send_to_char( "You return to your original body.\n\r", ch );

    if (ch->prompt != NULL)
    {
	free_string(ch->prompt);
	ch->prompt = NULL;
    }

    if (is_affected(ch, gsn_astralprojection))
      if (ch->desc->original)
	ch->desc->original->mana = UMIN(ch->mana, ch->desc->original->mana);

    if (is_affected(ch, gsn_possession))
    {
        affect_strip(ch, gsn_possession);
	af.where	= TO_AFFECTS;
	af.type		= gsn_possession;
	af.level	= ch->desc->original->level;
	af.duration	= 8;
	af.modifier	= 0;
	af.location	= 0;
	af.bitvector	= 0;
	affect_to_char(ch->desc->original, &af);
    }


    sprintf(buf,"$N returns from %s.",ch->short_descr);
    wiznet(buf,ch->desc->original,0,WIZ_SWITCHES,WIZ_SECURE,get_trust(ch));
    ch->desc->character       = ch->desc->original;
    ch->desc->original        = NULL;
    ch->desc->character->desc = ch->desc; 
    ch->desc                  = NULL;

    return;
}

/* trust levels for load and clone */
bool obj_check (CHAR_DATA *ch, OBJ_DATA *obj)
{
    if (IS_TRUSTED(ch,GOD)
	|| (IS_TRUSTED(ch,IMMORTAL) && obj->level <= 20 && obj->cost <= 1000)
	|| (IS_TRUSTED(ch,DEMI)	    && obj->level <= 10 && obj->cost <= 500)
	|| (IS_TRUSTED(ch,ANGEL)    && obj->level <=  5 && obj->cost <= 250)
	|| (IS_TRUSTED(ch,AVATAR)   && obj->level ==  0 && obj->cost <= 100))
	return TRUE;
    else
	return FALSE;
}

/* for clone, to insure that cloning goes many levels deep */
void recursive_clone(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *clone)
{
    OBJ_DATA *c_obj, *t_obj;


    for (c_obj = obj->contains; c_obj != NULL; c_obj = c_obj->next_content)
    {
	if (obj_check(ch,c_obj))
	{
	    t_obj = create_object(c_obj->pIndexData,0);
	    clone_object(c_obj,t_obj);
	    obj_to_obj(t_obj,clone);
	    recursive_clone(ch,c_obj,t_obj);
	}
    }
}

/* command that is similar to load */
void do_clone(CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char *rest;
    CHAR_DATA *mob;
    OBJ_DATA  *obj;

    rest = one_argument(argument,arg);

    if (arg[0] == '\0')
    {
	send_to_char("Clone what?\n\r",ch);
	return;
    }

    if (!str_prefix(arg,"object"))
    {
	mob = NULL;
	obj = get_obj_here(ch,rest);
	if (obj == NULL)
	{
	    send_to_char("You don't see that here.\n\r",ch);
	    return;
	}
    }
    else if (!str_prefix(arg,"mobile") || !str_prefix(arg,"character"))
    {
	obj = NULL;
	mob = get_char_room(ch,rest);
	if (mob == NULL)
	{
	    send_to_char("You don't see that here.\n\r",ch);
	    return;
	}
    }
    else /* find both */
    {
	mob = get_char_room(ch,argument);
	obj = get_obj_here(ch,argument);
	if (mob == NULL && obj == NULL)
	{
	    send_to_char("You don't see that here.\n\r",ch);
	    return;
	}
    }

    /* clone an object */
    if (obj != NULL)
    {
	OBJ_DATA *clone;

	if (!obj_check(ch,obj))
	{
	    send_to_char(
		"Your powers are not great enough for such a task.\n\r",ch);
	    return;
	}

	clone = create_object(obj->pIndexData,0); 
	clone_object(obj,clone);
	if (obj->carried_by != NULL)
	    obj_to_char(clone,ch);
	else
	    obj_to_room(clone,ch->in_room);
 	recursive_clone(ch,obj,clone);

	act("You clone $p.",ch,clone,NULL,TO_CHAR);
	wiznet("$N clones $p.",ch,clone,WIZ_LOAD,WIZ_SECURE,get_trust(ch));
	return;
    }
    else if (mob != NULL)
    {
	CHAR_DATA *clone;
	OBJ_DATA *new_obj;
	char buf[MAX_STRING_LENGTH];

	if (!IS_NPC(mob))
	{
	    send_to_char("You can only clone mobiles.\n\r",ch);
	    return;
	}

	if ((mob->level > 20 && !IS_TRUSTED(ch,GOD))
	||  (mob->level > 10 && !IS_TRUSTED(ch,IMMORTAL))
	||  (mob->level >  5 && !IS_TRUSTED(ch,DEMI))
	||  (mob->level >  0 && !IS_TRUSTED(ch,ANGEL))
	||  !IS_TRUSTED(ch,AVATAR))
	{
	    send_to_char(
		"Your powers are not great enough for such a task.\n\r",ch);
	    return;
	}

	clone = create_mobile(mob->pIndexData);
	clone_mobile(mob,clone); 
	
	for (obj = mob->carrying; obj != NULL; obj = obj->next_content)
	{
	    if (obj_check(ch,obj))
	    {
		new_obj = create_object(obj->pIndexData,0);
		clone_object(obj,new_obj);
		recursive_clone(ch,obj,new_obj);
		obj_to_char(new_obj,clone);
//		new_obj->wear_loc = obj->wear_loc;
		new_obj->worn_on = obj->worn_on;
	    }
	}
	char_to_room(clone,ch->in_room);
        act("You clone $N.",ch,NULL,clone,TO_CHAR);
	sprintf(buf,"$N clones %s.",clone->short_descr);
	wiznet(buf,ch,NULL,WIZ_LOAD,WIZ_SECURE,get_trust(ch));
        return;
    }
}

/* RT to replace the two load commands */

void do_load(CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument,arg);

    if (arg[0] == '\0')
    {
	send_to_char("Syntax:\n\r",ch);
	send_to_char("  load mob <vnum>\n\r",ch);
	send_to_char("  load obj <vnum> <level>\n\r",ch);
	return;
    }

    if (!str_cmp(arg,"mob") || !str_cmp(arg,"char"))
    {
	do_mload(ch,argument);
	return;
    }

    if (!str_cmp(arg,"obj"))
    {
	do_oload(ch,argument);
	return;
    }
    /* echo syntax */
    do_load(ch,"");
}


void do_mload( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    MOB_INDEX_DATA *pMobIndex;
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];
    
    one_argument( argument, arg );

    if ( arg[0] == '\0' || !is_number(arg) )
    {
	send_to_char( "Syntax: load mob <vnum>.\n\r", ch );
	return;
    }

    if ( ( pMobIndex = get_mob_index( atoi( arg ) ) ) == NULL )
    {
	send_to_char( "No mob has that vnum.\n\r", ch );
	return;
    }

    victim = create_mobile( pMobIndex );
    char_to_room( victim, ch->in_room );
    sprintf(buf,"$N loads %s.",victim->short_descr);
    wiznet(buf,ch,NULL,WIZ_LOAD,WIZ_SECURE,get_trust(ch));
    send_to_char( "Ok.\n\r", ch );
    return;
}

void brand_char(CHAR_DATA *ch, CHAR_DATA *vch, int sn, bool prog)
{
    OBJ_DATA *obj;

    if (!vch)
	return;

    for (obj = vch->carrying; obj; obj = obj->next_content)
    {
	if (!prog && IS_SET(obj->worn_on, WORN_SIGIL))
	{
	    if (ch)
		send_to_char("They are already branded!\n\r", ch);
	    return;
	}
	else if (prog && IS_SET(obj->worn_on, WORN_PROGSLOT))
	{
	    if (ch)
		send_to_char("They are already wearing a prog object!\n\r", ch);
	    return;
	}
    }

    if ((obj = create_object(get_obj_index(sn), ch ? ch->level : vch->level)) == NULL)
	return;

    if ( CAN_WEAR(obj, ITEM_TAKE) )
	obj_to_char( obj, vch );
    else
    {
	extract_obj(obj);
	bug("Fucked: sigil/prog with no take flag", 0);
	return;
    }

    if (!prog)
        equip_char(vch, obj, WORN_SIGIL);
    else
	equip_char(vch, obj, WORN_PROGSLOT);

    oprog_wear_trigger(obj);

    if (!IS_SET(obj->extra_flags[0], ITEM_WIZI))
    {
	if (ch)
            act("$N brands you with $p.", vch, obj, ch, TO_CHAR);
	else
	    act("You are branded with $p.", vch, obj, NULL, TO_CHAR);

	act("$n is branded with $p.", vch, obj, NULL, TO_ROOM);
    }

    if (ch)
        send_to_char( "You brand them.\n\r", ch );

    return;
}


void do_brand( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH] ,arg2[MAX_INPUT_LENGTH];
    OBJ_INDEX_DATA *pObjIndex;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    CHAR_DATA *vch;
    int level;
    bool prog = FALSE;
    
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0')
    {
	send_to_char( "Syntax: brand <character> <tattoo vnum>.\n\r", ch );
	return;
    }
    
    level = get_trust(ch); /* default */
    if (IS_NPC(ch))
	level = ch->level;
  
    if ( arg2[0] == '\0')  /* load with a level */
    {
	send_to_char( "Syntax: brand <character> <tattoo vnum>.\n\r", ch );
	return;
    }

    if (argument[0] != '\0' && !str_cmp(argument, "prog"))
	prog = TRUE;

    if ((vch = get_char_room(ch, arg1)) == NULL)
	{
	send_to_char("They aren't here.\n\r", ch);
	return;
	}
	
    if ( ( pObjIndex = get_obj_index( atoi( arg2 ) ) ) == NULL )
    {
	send_to_char( "No object has that vnum.\n\r", ch );
	return;
    }

    if (!prog && !IS_SET(pObjIndex->wear_flags, ITEM_WEAR_SIGIL))
    {
	send_to_char( "That item is not a Sigil!\n\r", ch);
	return;
    }
    else if (prog && !IS_SET(pObjIndex->wear_flags, ITEM_PROG))
    {
	send_to_char("That is not a prog item!\n\r", ch);
	return;
    }

    for ( obj = vch->carrying; obj != NULL; obj = obj_next )
    {
        obj_next = obj->next_content;

	if (!prog && IS_SET(obj->worn_on, WORN_SIGIL))
	{
	    send_to_char( "They are already branded!\n\r", ch);
	    return;
	}
	else if (prog && IS_SET(obj->worn_on, WORN_PROGSLOT))
	{
	    send_to_char("They are already wearing a prog object.\n\r", ch);
	    return;
	}
    }

    brand_char(ch, vch, atoi(arg2), prog);

    return;
}

void do_unbrand( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    CHAR_DATA *vch;
    bool prog = FALSE;
    
    argument = one_argument( argument, arg1 );

    if ( arg1[0] == '\0')
    {
	send_to_char( "Syntax: unbrand <character>.\n\r", ch );
	return;
    }
    
    if ((vch = get_char_room(ch, arg1)) == NULL)
    {
	send_to_char("They aren't here.\n\r", ch);
	return;
    }
  
    if (argument[0] != '\0' && !str_cmp(argument, "prog"))
	prog = TRUE;
	
    for ( obj = vch->carrying; obj != NULL; obj = obj_next )
    {
        obj_next = obj->next_content;

        if ((!prog && IS_SET(obj->worn_on, WORN_SIGIL)) || (prog && IS_SET(obj->worn_on, WORN_PROGSLOT)))
        {
            if (!IS_SET(obj->extra_flags[0], ITEM_WIZI))
            {
                act("$N strips you of $p.", vch, obj, ch, TO_CHAR);
                act("$n is stripped of $p.", vch, obj, NULL, TO_ROOM);
            }
            extract_obj(obj);
            return;
        }
    }

    send_to_char( "Not found on that character.\n\r", ch );
    return;
}

void do_eyefocus( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *eyemob;
    ROOM_INDEX_DATA *pRoom;

    if (!get_skill(ch, gsn_runeofeyes))
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }
    
    if (!is_affected(ch, gsn_runeofeyes))
    {
	send_to_char("You have placed no rune of eyes in this world.\n\r", ch);
	return;
    }

    eyemob = create_mobile(get_mob_index(MOB_VNUM_EYE_RUNE));
    if ((pRoom = get_room_index(get_modifier(ch->affected, gsn_runeofeyes))) == NULL)
    {
	send_to_char("Something prevents you from viewing your Rune of Eyes.\n\r", ch);
	return;
    }

    char_to_room(eyemob, pRoom);

    send_to_char("You focus your perceptions on that of the Rune of Eyes.\n\r", ch);
    act("$n's eyes assume a distant look.", ch, NULL, NULL, TO_ROOM);

    SET_BIT(ch->oaffected_by, AFF_EYEFOCUS);

    ch->desc->character		= eyemob;
    ch->desc->original		= ch;
    eyemob->desc		= ch->desc;
    ch->desc			= NULL;
    if (ch->prompt != NULL)
        eyemob->prompt = str_dup(ch->prompt);
    eyemob->comm = ch->comm;
    eyemob->lines = ch->lines;
    eyemob->hit		= 0;
    eyemob->max_hit	= 0;
    eyemob->mana	= 0;
    eyemob->max_mana	= 0;
    eyemob->move	= 0;
    eyemob->max_move	= 0;
 
    return;
}



void do_oload( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH] ,arg2[MAX_INPUT_LENGTH];
    OBJ_INDEX_DATA *pObjIndex;
    OBJ_DATA *obj;
    int level;
    
    argument = one_argument( argument, arg1 );
    one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || !is_number(arg1))
    {
	send_to_char( "Syntax: load obj <vnum> <level>.\n\r", ch );
	return;
    }
    
    level = get_trust(ch); /* default */
  
    if ( arg2[0] != '\0')  /* load with a level */
    {
	if (!is_number(arg2))
        {
	  send_to_char( "Syntax: oload <vnum> <level>.\n\r", ch );
	  return;
	}
        level = atoi(arg2);
        if (level < 0 || level > get_trust(ch))
	{
	  send_to_char( "Level must be be between 0 and your level.\n\r",ch);
  	  return;
	}
    }

    if ( ( pObjIndex = get_obj_index( atoi( arg1 ) ) ) == NULL )
    {
	send_to_char( "No object has that vnum.\n\r", ch );
	return;
    }

    obj = create_object( pObjIndex, level );
	if ((pObjIndex->current > pObjIndex->limit) && (pObjIndex->limit > 0))
		send_to_char( "_Warning_: The item you have created is over limit\n\r", ch);

    if ( CAN_WEAR(obj, ITEM_TAKE) )
	obj_to_char( obj, ch );
    else
	obj_to_room( obj, ch->in_room );
    wiznet("$N loads $p.",ch,obj,WIZ_LOAD,WIZ_SECURE,get_trust(ch));
    send_to_char( "Ok.\n\r", ch );
    return;
}

void do_purge( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char buf[100];
    CHAR_DATA *victim, *vch, *vnext;
    OBJ_DATA *obj;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	/* 'purge' */
	OBJ_DATA  *obj_next;

	for ( victim = ch->in_room->people; victim != NULL; victim = vnext )
	{
	    vnext = victim->next_in_room;
	    if (IS_IMMORTAL(victim))
	        act( "$n purges the room!", ch, NULL, victim, TO_VICT);
	    if (IS_NPC(victim) && !IS_SET(victim->act,ACT_NOPURGE) 
	     && victim != ch /* safety precaution */ )
		extract_char( victim, TRUE );
	}

	for ( obj = ch->in_room->contents; obj != NULL; obj = obj_next )
	{
	    obj_next = obj->next_content;
	    if (!IS_OBJ_STAT(obj,ITEM_NOPURGE))
	      extract_obj( obj );
	}

	send_to_char( "Room purged.\n\r", ch );
	return;
    }

    if ((victim = get_char_world(ch, arg)) != NULL)
    {
	if ( !IS_NPC(victim) )
        {

	    if (ch == victim)
	    {
		send_to_char("Ho ho ho.\n\r",ch);
		return;
	    }

	    if (get_trust(ch) <= get_trust(victim))
	    {
		send_to_char("Maybe that wasn't a good idea...\n\r",ch);
		sprintf(buf,"%s tried to purge you!\n\r",ch->name);
		send_to_char(buf,victim);
		return;
	    }

	    act("$n disintegrates $N.",ch,0,victim,TO_NOTVICT);

    	    if (victim->level > 1)
	        save_char_obj( victim );
    	    extract_char( victim, TRUE );

	    return;
        }

	for (vch = ch->in_room->people; vch; vch = vnext)
	{
	    vnext = vch->next_in_room;
	    if (IS_IMMORTAL(vch))
	    {
		sprintf(buf, "$n purges %s.", PERS(victim, vch));
		act( buf, ch, NULL, vch, TO_VICT );
	    }
	}

        extract_char( victim, TRUE );
    }
    else if ((obj = get_obj_here(ch, arg)) != NULL)
    {
	for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
	    if (IS_IMMORTAL(vch))
		act("$n purges $p.", ch, obj, vch, TO_VICT);
	 
	extract_obj( obj );
    }
    else
	send_to_char("Mobile or object not found.\n\r", ch);

    return;
}



void do_advance( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int level;
    int iLevel;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || !is_number( arg2 ) )
    {
	send_to_char( "Syntax: advance <char> <level>.\n\r", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
	send_to_char( "That player is not here.\n\r", ch);
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n\r", ch );
	return;
    }

    if ( ( level = atoi( arg2 ) ) < 1 || level > 60 )
    {
	send_to_char( "Level must be 1 to 60.\n\r", ch );
	return;
    }

    if ( level > get_trust( ch ) )
    {
	send_to_char( "Limited to your trust level.\n\r", ch );
	return;
    }

    /*
     * Lower level:
     *   Reset to level 1.
     *   Then raise again.
     *   Currently, an imp can lower another imp.
     *   -- Swiftest
     */
    if ( level <= victim->level )
    {
        int temp_prac;

	send_to_char( "Lowering a player's level!\n\r", ch );
	send_to_char( "**** OOOOHHHHHHHHHH  CCCRRAAPP ****\n\r", victim );
	temp_prac = victim->practice;
	player_levels -= (victim->level - 1);
        victim->level    = 1;
	victim->exp      = 0;
	victim->max_hit  = 10;
	victim->max_mana = 100;
	victim->max_move = 100;
	victim->practice = 0;
	victim->hit      = victim->max_hit;
	victim->mana     = victim->max_mana;
	victim->move     = victim->max_move;
	advance_level( victim, TRUE );
	victim->practice = temp_prac;
    }
    else
    {
	send_to_char( "Raising a player's level!\n\r", ch );
	send_to_char( "**** OOOOHHHHHHHHHH  YYYYEEEESSS ****\n\r", victim );
    }

    for ( iLevel = victim->level ; iLevel < level; iLevel++ )
    {
	victim->level += 1;
	player_levels++;
	advance_level( victim,TRUE);
    }
    sprintf(buf,"You are now level %d.\n\r",victim->level);
    send_to_char(buf,victim);
    victim->exp = UMAX(0,exp_on_level(victim,victim->level));
    /* if part added by Xurinos to fix trust */
    if (victim->trust < victim->level) victim->trust = 0;
    save_char_obj(victim);
    return;
}



void do_trust( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int level;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || !is_number( arg2 ) )
    {
	send_to_char( "Syntax: trust <char> <level>.\n\r", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
	send_to_char( "That player is not here.\n\r", ch);
	return;
    }

    if ( ( level = atoi( arg2 ) ) < 0 || level > 60 )
    {
	send_to_char( "Level must be 0 (reset) or 1 to 60.\n\r", ch );
	return;
    }

    if ( level > get_trust( ch ) )
    {
	send_to_char( "Limited to your trust.\n\r", ch );
	return;
    }

    victim->trust = level;
    return;
}



void do_restore( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *vch;
    DESCRIPTOR_DATA *d;

    one_argument( argument, arg );
    if (arg[0] == '\0' || !str_cmp(arg,"room"))
    {
    /* cure room */
    	
        for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
        {
            
            vch->hit 	= vch->max_hit;
            vch->mana	= vch->max_mana;
            vch->move	= vch->max_move;
            update_pos( vch);
            act("$n has restored you.",ch,NULL,vch,TO_VICT);
        }

        sprintf(buf,"$N restored room %d.",ch->in_room->vnum);
        wiznet(buf,ch,NULL,WIZ_RESTORE,WIZ_SECURE,get_trust(ch));
        
        send_to_char("Room restored.\n\r",ch);
        return;

    }
    
    if ( get_trust(ch) >=  MAX_LEVEL - 1 && !str_cmp(arg,"all"))
    {
    /* cure all */
    	
        for (d = descriptor_list; d != NULL; d = d->next)
        {
	    victim = d->character;

	    if (victim == NULL || IS_NPC(victim))
		continue;
                
            victim->hit 	= victim->max_hit;
            victim->mana	= victim->max_mana;
            victim->move	= victim->max_move;
            update_pos( victim);
	    if (victim->in_room != NULL)
                act("$n has restored you.",ch,NULL,victim,TO_VICT);
        }
	send_to_char("All active players restored.\n\r",ch);
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    victim->hit  = victim->max_hit;
    victim->mana = victim->max_mana;
    victim->move = victim->max_move;
    update_pos( victim );
    act( "$n has restored you.", ch, NULL, victim, TO_VICT );
    sprintf(buf,"$N restored %s",
	IS_NPC(victim) ? victim->short_descr : victim->name);
    wiznet(buf,ch,NULL,WIZ_RESTORE,WIZ_SECURE,get_trust(ch));
    send_to_char( "Ok.\n\r", ch );
    return;
}

 	
void do_freeze( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Freeze whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n\r", ch );
	return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
	send_to_char( "You failed.\n\r", ch );
	return;
    }

    if ( IS_SET(victim->act, PLR_FREEZE) )
    {
	REMOVE_BIT(victim->act, PLR_FREEZE);
	send_to_char( "You can play again.\n\r", victim );
	send_to_char( "FREEZE removed.\n\r", ch );
	sprintf(buf,"$N thaws %s.",victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }
    else
    {
	SET_BIT(victim->act, PLR_FREEZE);
	send_to_char( "You can't do ANYthing!\n\r", victim );
	send_to_char( "FREEZE set.\n\r", ch );
	sprintf(buf,"$N puts %s in the deep freeze.",victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }

    save_char_obj( victim );

    return;
}

void do_slog( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    if ( argument[0] == '\0' )
    {
	send_to_char("Log whom?\n\r", ch);
	return;
    }

    one_argument( argument, arg );

    if (!(victim = get_char_world(ch, arg)))
    {
	send_to_char("You cannot sense them in the world.\n\r", ch);
	return;
    }

    if (IS_NPC(victim))
    {
	send_to_char("You cannot log mobiles.\n\r", ch);
	return;
    }

    if (IS_SET(victim->act, PLR_SLOG))
    {
	sprintf(log_buf, "M: Logging ended by %s.", ch->name);
	write_slog(victim, log_buf);
	REMOVE_BIT(victim->act, PLR_SLOG);
	sprintf(log_buf, "%s will no longer be slogged.\n\r", victim->name);
	send_to_char(log_buf, ch);
	return;
    }
    else
    {
	SET_BIT(victim->act, PLR_SLOG);
	sprintf(log_buf, "M: Logging started by %s.", ch->name);
	write_slog(victim, log_buf);
	sprintf(log_buf, "%s will now be slogged.\n\r", victim->name);
	send_to_char(log_buf, ch);
	return;
    }
}


void do_log( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Log whom?\n\r", ch );
	return;
    }

    if ( !str_cmp( arg, "all" ) )
    {
	if ( fLogAll )
	{
	    fLogAll = FALSE;
	    send_to_char( "Log ALL off.\n\r", ch );
	}
	else
	{
	    fLogAll = TRUE;
	    send_to_char( "Log ALL on.\n\r", ch );
	}
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n\r", ch );
	return;
    }

    /*
     * No level check, gods can log anyone.
     */
    if ( IS_SET(victim->act, PLR_LOG) )
    {
	REMOVE_BIT(victim->act, PLR_LOG);
	send_to_char( "LOG removed.\n\r", ch );
    }
    else
    {
	SET_BIT(victim->act, PLR_LOG);
	send_to_char( "LOG set.\n\r", ch );
    }

    return;
}



void do_noemote( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Noemote whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }


    if ( get_trust( victim ) >= get_trust( ch ) )
    {
	send_to_char( "You failed.\n\r", ch );
	return;
    }

    if ( IS_SET(victim->comm, COMM_NOEMOTE) )
    {
	REMOVE_BIT(victim->comm, COMM_NOEMOTE);
	send_to_char( "You can emote again.\n\r", victim );
	send_to_char( "NOEMOTE removed.\n\r", ch );
	sprintf(buf,"$N restores emotes to %s.",victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }
    else
    {
	SET_BIT(victim->comm, COMM_NOEMOTE);
	send_to_char( "You can't emote!\n\r", victim );
	send_to_char( "NOEMOTE set.\n\r", ch );
	sprintf(buf,"$N revokes %s's emotes.",victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }

    return;
}



void do_noshout( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Noshout whom?\n\r",ch);
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n\r", ch );
	return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
	send_to_char( "You failed.\n\r", ch );
	return;
    }

    if ( IS_SET(victim->comm, COMM_NOSHOUT) )
    {
	REMOVE_BIT(victim->comm, COMM_NOSHOUT);
	send_to_char( "You can shout again.\n\r", victim );
	send_to_char( "NOSHOUT removed.\n\r", ch );
	sprintf(buf,"$N restores shouts to %s.",victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }
    else
    {
	SET_BIT(victim->comm, COMM_NOSHOUT);
	send_to_char( "You can't shout!\n\r", victim );
	send_to_char( "NOSHOUT set.\n\r", ch );
	sprintf(buf,"$N revokes %s's shouts.",victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }

    return;
}




void do_noooc( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Noooc whom?", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
	send_to_char( "You failed.\n\r", ch );
	return;
    }

    if ( IS_SET(victim->comm, COMM_NOOOC) )
    {
	REMOVE_BIT(victim->comm, COMM_NOOOC);
	send_to_char( "You can ooc again.\n\r", victim );
	send_to_char( "NOOOC removed.\n\r", ch );
	sprintf(buf,"$N restores ooc to %s.",victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }
    else
    {
	SET_BIT(victim->comm, COMM_NOOOC);
	send_to_char( "You can't ooc any more!\n\r", victim );
	send_to_char( "NOOOC set.\n\r", ch );
	sprintf(buf,"$N revokes %s's ooc.",victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }

    return;
}

void do_notell( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Notell whom?", ch );
	return;
    }

    if ( ( victim = get_char_world( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( get_trust( victim ) >= get_trust( ch ) )
    {
	send_to_char( "You failed.\n\r", ch );
	return;
    }

    if ( IS_SET(victim->comm, COMM_NOTELL) )
    {
	REMOVE_BIT(victim->comm, COMM_NOTELL);
	send_to_char( "You can tell again.\n\r", victim );
	send_to_char( "NOTELL removed.\n\r", ch );
	sprintf(buf,"$N restores tells to %s.",victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }
    else
    {
	SET_BIT(victim->comm, COMM_NOTELL);
	send_to_char( "You can't tell!\n\r", victim );
	send_to_char( "NOTELL set.\n\r", ch );
	sprintf(buf,"$N revokes %s's tells.",victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }

    return;
}

void do_peace( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *rch;

    for ( rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room )
    {
	if ( rch->fighting != NULL )
	    stop_fighting_all(rch);
	if (IS_NPC(rch) && IS_SET(rch->act,ACT_AGGRESSIVE))
	    REMOVE_BIT(rch->act,ACT_AGGRESSIVE);
	if (IS_NPC(rch) && IS_SET(rch->nact,ACT_PSYCHO))
	    REMOVE_BIT(rch->nact, ACT_PSYCHO);
	if (IS_NPC(rch))
	    rch->tracking = NULL;
	rch->fighting = NULL;
    }

    send_to_char( "Ok.\n\r", ch );
    return;
}

void do_wizlock( CHAR_DATA *ch, char *argument )
{
    extern char * g_wizlock;
    
    if ( !g_wizlock )
    {
	if (argument[0] == '\0')
	    g_wizlock = str_dup("unknown");
	else
	    g_wizlock = str_dup(argument);

	sprintf(log_buf, "$N has wizlocked the game (Reason: %s).", g_wizlock);
	wiznet(log_buf,ch,NULL,0,0,0);
	send_to_char( "Game wizlocked.\n\r", ch );
    }
    else
    {
	free_string(g_wizlock);
	g_wizlock = NULL;
	wiznet("$N removes wizlock.",ch,NULL,0,0,0);
	send_to_char( "Game un-wizlocked.\n\r", ch );
    }

    return;
}

/* RT anti-newbie code */

void do_newlock( CHAR_DATA *ch, char *argument )
{
    extern bool newlock;
    newlock = !newlock;
 
    if ( newlock )
    {
	wiznet("$N locks out new characters.",ch,NULL,0,0,0);
        send_to_char( "New characters have been locked out.\n\r", ch );
    }
    else
    {
	wiznet("$N allows new characters back in.",ch,NULL,0,0,0);
        send_to_char( "Newlock removed.\n\r", ch );
    }
 
    return;
}


void do_slookup( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char buf1[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    BUFFER *buffer;
    int sn, sn2;

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Lookup which skill or spell?\n\r", ch );
	return;
    }

    if ( !str_cmp( arg, "all" ) )
    {
    	buffer = new_buf();
    	for (sn = 0; sn < MAX_SKILL && skill_table[sn].name != NULL; sn += 2)
	    {
	        sprintf( buf1, "Sn: %3d  Name: '%s'", sn, skill_table[sn].name);
    	    if (sn + 1 < MAX_SKILL && skill_table[sn+1].name != NULL)
    	    {
	        	sprintf( buf2, "Sn: %3d  Name: '%s'", sn+1, skill_table[sn+1].name);
        		sprintf( buf, "%-37s %-37s\n\r", buf1, buf2);
		        add_buf(buffer, buf);
	        }
	        else
	        {
		        sprintf(buf, "%s\n\r", buf1);
                add_buf(buffer, buf);
		        break;
            }
	    }

    	page_to_char(buf_string(buffer),ch);
    	free_buf(buffer);
    }
    else
    {
	if (is_number(arg))
	{
	    sn = atoi(arg);
	    if ((sn < 0) || (sn >= MAX_SKILL) || !skill_table[sn].name)
	    {
		send_to_char("Invalid skill number.\n\r", ch);
		return;
	    }

	    argument = one_argument(argument, arg);
	    if ((arg[0] != '\0') && is_number(arg))
	    {
		sn2 = atoi(arg);
		if ((sn2 < 0) || (sn2 >= MAX_SKILL) || (sn2 <= sn)
		 || !skill_table[sn2].name)
		{
		    send_to_char("Invalid skill range number.\n\r", ch);
		    return;
		}

		for ( ; sn <= sn2; sn++)
		{
		    sprintf( buf, "Sn: %3d  Skill/spell: '%s'\n\r",
	    	    	     sn, skill_table[sn].name );
		    send_to_char( buf, ch );
		}

		return;
	    }
	}
	else if ( ( sn = skill_lookup_full( arg ) ) < 0 )
	{
	    send_to_char( "No such skill or spell.\n\r", ch );
	    return;
	}

	sprintf( buf, "Sn: %3d  Skill/spell: '%s'\n\r",
	    sn, skill_table[sn].name );
	send_to_char( buf, ch );
    }
	
    return;
}

/* RT set replaces sset, mset, oset, and rset */

void do_set( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];

    argument = one_argument(argument,arg);

    if (arg[0] == '\0')
    {
	send_to_char("Syntax:\n\r",ch);
	send_to_char("  set mob   <name> <field> <value>\n\r",ch);
	send_to_char("  set obj   <name> <field> <value>\n\r",ch);
	send_to_char("  set room  <room> <field> <value>\n\r",ch);
        send_to_char("  set skill <name> <spell or skill> <value>\n\r",ch);
	return;
    }

    if (!str_prefix(arg,"mobile") || !str_prefix(arg,"character"))
    {
	do_mset(ch,argument);
	return;
    }

    if (!str_prefix(arg,"skill") || !str_prefix(arg,"spell"))
    {
	do_sset(ch,argument);
	return;
    }

    if (!str_prefix(arg,"object"))
    {
	do_oset(ch,argument);
	return;
    }

    if (!str_prefix(arg,"room"))
    {
	do_rset(ch,argument);
	return;
    }
    /* echo syntax */
    do_set(ch,"");
}

void sget(CHAR_DATA *victim, char *argument, int *memval)
{
    char arg[MAX_INPUT_LENGTH];
    int sn, slot;

    if (argument[0] == '\0')
	return;

    argument = one_argument(argument, arg);

    if (((sn = skill_lookup(arg)) < 0) || !is_number(argument))
	return;

    slot = atoi(argument);

    if ((slot < 0) || (slot > 9))
	return;

    memval[slot] = get_skill(victim, sn);

    return;
}

void do_sset( CHAR_DATA *ch, char *argument )
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int value;
    int sn, gn = -1, i;
    bool fAll, fCurrent;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
	send_to_char( "Syntax:\n\r",ch);
	send_to_char( "  set skill <name> <spell or skill> <value>\n\r", ch);
	send_to_char( "  set skill <name> all <value>\n\r",ch);  
	send_to_char( "  set skill <name> current <value>\n\r",ch);  
	send_to_char( "  set skill <name> <group> <value>\n\r", ch);
	send_to_char("   (use the name of the skill, not the number)\n\r",ch);
	return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( IS_NPC(victim) )
    {
	send_to_char( "Not on NPC's.\n\r", ch );
	return;
    }

    fAll = !str_cmp( arg2, "all" );
    fCurrent = !str_cmp(arg2, "current");
    sn   = 0;
    gn = group_lookup(arg2);
    
    if ( gn < 0 && !fAll && !fCurrent 
      && ( sn = skill_lookup_full( arg2 ) ) < 0 )
    {
		send_to_char( "No such skill or spell.\n\r", ch );
		return;
    }

    /*
     * Snarf the value.
     */
    if ( !is_number( arg3 ) )
    {
	send_to_char( "Value must be numeric.\n\r", ch );
	return;
    }

    value = atoi( arg3 );
    if ( value < 0 || value > 100 )
    {
	send_to_char( "Value range is 0 to 100.\n\r", ch );
	return;
    }

    if ( fAll || fCurrent)
    {
	    for ( sn = 0; sn < MAX_SKILL; sn++ )
    	{
	        if ( skill_table[sn].name == NULL ) continue;
		    if (fCurrent && victim->pcdata->learned[sn] <= 0) continue;

    		victim->pcdata->learned[sn]	= value;
	    }
    }
    else if (gn >= 0)
    {
    	for (i = 0; i < MAX_IN_GROUP; i++)
	    {
    		if (group_table[gn].spells[i] == NULL) break;
	    	sn = skill_lookup_full(group_table[gn].spells[i]);
		    victim->pcdata->learned[sn] = value;
    	}
    }
    else
    	victim->pcdata->learned[sn] = value;
}

static int countLines(const char * text, const char * stopPoint = NULL)
{
    int result(0);
    bool hadNewLine(true);
    while (*text != '\0' && text != stopPoint)
    {
        if (hadNewLine)
        {
            if (*text != '\r')
            {
                hadNewLine = (*text == '\n');
                ++result;
            }
        }
        else if (*text == '\n')
            hadNewLine = true;

        ++text;
    }
    return result;
}

void rget(ROOM_INDEX_DATA * room, char * arg2, char * argument, int * memval)
{
    smash_tilde(argument);

    /* Once upon a time there was arg1, but it was removed */
    if (arg2[0] == '\0' || argument[0] == '\0' )
	    return;
    
    if (!is_number(argument))
        return;

	int value = atoi(argument);

    // Begin field checks
    if (!str_cmp(arg2, "itemcount"))
    {
        int count(0);
        for (const OBJ_DATA * obj(room->contents); obj != NULL; obj = obj->next_content)
            ++count;

        memval[value] = count;
        return;
    }

    if (!str_cmp(arg2, "charcount"))
    {
        int count(0);
        for (const CHAR_DATA * ch(room->people); ch != NULL; ch = ch->next_in_room)
            ++count;

        memval[value] = count;
        return;
    }

    if (!str_cmp(arg2, "stone"))
    {
        memval[value] = room->stone_type;
        if (memval[value] < 0) memval[value] = room->area->stone_type;
        if (memval[value] < 0 || memval[value] >= MAX_MATERIALS || !material_table[memval[value]].stone) memval[value] = -1;
        return;
    }

    if (!str_cmp(arg2, "vnum"))
    {
        memval[value] = room->vnum;
        return;
    }


    if (!str_cmp(arg2, "dataproglinecount"))
    {
        if (room->prog_state.data_prog != NULL && room->prog_state.data_prog->comlist != NULL)
            memval[value] = countLines(room->prog_state.data_prog->comlist);

        return;
    }

    if (!str_cmp(arg2, "dataprogline"))
    {
        if (room->prog_state.data_prog != NULL && room->prog_state.data_prog->comlist != NULL)
            memval[value] = countLines(room->prog_state.data_prog->comlist, room->prog_state.data_marker);

        return;
    }

    // No field found
    bug("Unknown field in mprget", 0);
}

void oget(OBJ_DATA * obj, char * argument, int * memval)
{
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];

    smash_tilde(argument);
    argument = one_argument( argument, arg2 );
    strcpy( arg3, argument );

    /* Once upon a time there was arg1, but it was removed */
    if (arg2[0] == '\0' || arg3[0] == '\0' )
	    return;
    
    if (!is_number(arg3))
        return;

	int value = atoi(arg3);

    if (!str_cmp(arg2, "timer"))
    {
        memval[value] = obj->timer;
        return;
    }

    if (!str_prefix(arg2, "level"))
    {
    	memval[value] = obj->level;
	    return;
    }

    if (!str_cmp(arg2, "material"))
    {
        memval[value] = obj->material;
        return;
    }

    if (!str_cmp(arg2, "type"))
    {
        memval[value] = obj->item_type;
        return;
    }

    if (!str_cmp(arg2, "vnum"))
    {
        memval[value] = obj->pIndexData->vnum;
        return;
    }

    if (!str_prefix("val", arg2))
    {
        if (arg2[3] >= '0' && arg2[3] <= '9')
            memval[value] = obj->objvalue[arg2[3] - '0'];
        return;
    }

    if (!str_cmp(arg2, "dataproglinecount"))
    {
        if (obj->prog_state.data_prog != NULL && obj->prog_state.data_prog->comlist != NULL)
            memval[value] = countLines(obj->prog_state.data_prog->comlist);

        return;
    }

    if (!str_cmp(arg2, "dataprogline"))
    {
        if (obj->prog_state.data_prog != NULL && obj->prog_state.data_prog->comlist != NULL)
            memval[value] = countLines(obj->prog_state.data_prog->comlist, obj->prog_state.data_marker);

        return;
    }

    bug("Unknown field in mpoget", 0);
}

void mget(CHAR_DATA *victim, char *argument, int *memval)
{
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    int value, ctype;

    smash_tilde(argument);
    argument = one_argument( argument, arg2 );
    strcpy( arg3, argument );

    /* Once upon a time there was arg1, but it was removed */

    if (arg2[0] == '\0' || arg3[0] == '\0' )
	return;

    if ( !str_cmp(arg2, "clore") )
    {
        char exarg[MAX_INPUT_LENGTH];
        bool setvnum;
        int slotnum;	

        argument = one_argument(argument, exarg);
        if (!str_cmp(exarg, "vnum"))
            setvnum = TRUE;
        else if (!str_cmp(exarg, "percent"))
            setvnum = FALSE;
        else
            return;
        
        argument = one_argument(argument, exarg);
        if (is_number(exarg))
        {
            slotnum = atoi(exarg);
            if ((slotnum < 0) || (slotnum >= MAX_CLORE))
            return;
        }
        else
            return;

        if (is_number(argument))
        {
            if (setvnum)
                memval[atoi(argument)] = victim->lcreature[slotnum];
            else
            memval[atoi(argument)] = victim->lpercent[slotnum];
                return;
        }

        return;
    }	

    if (!str_cmp(arg2, "somaticarts"))
    {
        // Find the race specified by arg3
        int raceIndex(-1);
        char raceValue[MAX_STRING_LENGTH];
        argument = one_argument(argument, raceValue);
        if (is_number(raceValue)) raceIndex = atoi(raceValue);
        else raceIndex = race_lookup(raceValue);

        if (!is_number(argument))
            return;
        
        // Check for the data
        value = atoi(argument);
        if (IS_NPC(victim) || victim->pcdata->somatic_arts_info == NULL) memval[value] = SomaticArtsInfo::Unknown;
        else memval[value] = victim->pcdata->somatic_arts_info->SkillFor(raceIndex);
        return;
    }

    if (!is_number(arg3))
        return;
 
 	value = atoi(arg3);
   
    if ( !str_cmp(arg2, "recall") )
    {
	if (victim->recall_old)
	    memval[value] = victim->recall_old->vnum;
	else if (victim->recall_to)
	    memval[value] = victim->recall_to->vnum;
	else
	    memval[value] = -1;

	return;
    }

    if ( !str_cmp(arg2, "ep") )
	memval[value] = victim->ep;

    if ( !str_cmp (arg2, "maxage" ) )
    {
    	if (IS_NPC(victim))
	    {
	        memval[value] = -1;
    	    return;
	    }
    	memval[value] = victim->pcdata->max_age;
    	return;
    }

    if ( !str_prefix( arg2, "karma" ) )
    {
    	if (IS_NPC(victim))
        {
            if (IS_GOOD(victim)) memval[value] = GOLDENAURA;
            else if (IS_EVIL(victim)) memval[value] = REDAURA;
            else memval[value] = 0;
	        return;
	    }

        memval[value] = victim->pcdata->karma;
        return;
    }

    if (!str_cmp(arg2, "dataproglinecount"))
    {
        if (victim->prog_state.data_prog != NULL && victim->prog_state.data_prog->comlist != NULL)
            memval[value] = countLines(victim->prog_state.data_prog->comlist);

        return;
    }

    if (!str_cmp(arg2, "dataprogline"))
    {
        if (victim->prog_state.data_prog != NULL && victim->prog_state.data_prog->comlist != NULL)
            memval[value] = countLines(victim->prog_state.data_prog->comlist, victim->prog_state.data_marker);

        return;
    }

    if ( !str_cmp (arg2, "age" ) )
    {
	if (IS_NPC(victim))
	{
	    memval[value] = -1;
	    return;
	}

	memval[value] = get_age(victim);
	return;
    }
 
    if ( !str_cmp( arg2, "deaths" ) )
    {
	if (IS_NPC(victim))
	    memval[value] = -1;
	else
	    memval[value] = victim->pcdata->death_count;

	return;	    
    }

    if ( !str_cmp( arg2, "adrenaline" ) )
    {
        if ( IS_NPC( victim ) )
	    memval[value] = -1;
	else
	    memval[value] = victim->pcdata->adrenaline;
        return;
    }

    if (!str_cmp(arg2, "timer"))
    {
        memval[value] = victim->timer;
        return;
    }

    if (!str_cmp(arg2, "vnum"))
    {
        if (IS_NPC(victim))
            memval[value] = victim->pIndexData->vnum;
        else
            memval[value] = -1;
        return;
    }

    if ( !str_cmp( arg2, "str" ) )
    {
	memval[value] = get_curr_stat(victim, STAT_STR);
	return;
    }

    if ( !str_cmp( arg2, "int" ) )
    {
	memval[value] = get_curr_stat(victim, STAT_INT);
	return;
    }

    if ( !str_cmp( arg2, "wis" ) )
    {
	memval[value] = get_curr_stat(victim, STAT_WIS);
	return;
    }

    if ( !str_cmp( arg2, "dex" ) )
    {
	memval[value] = get_curr_stat(victim, STAT_DEX);
	return;
    }

    if ( !str_cmp( arg2, "con" ) )
    {
	memval[value] = get_curr_stat(victim, STAT_CON);
	return;
    }

    if ( !str_cmp( arg2, "chr" ) )
    {
	memval[value] = get_curr_stat(victim, STAT_CHR);
	return;
    }

    if ( !str_cmp( arg2, "permstr" ) )
    {
	memval[value] = victim->perm_stat[STAT_CHR];
	return;
    }

    if ( !str_cmp( arg2, "permint" ) )
    {
	memval[value] = victim->perm_stat[STAT_CHR];
	return;
    }

    if ( !str_cmp( arg2, "permwis" ) )
    {
	memval[value] = victim->perm_stat[STAT_CHR];
	return;
    }

    if ( !str_cmp( arg2, "permdex" ) )
    {
	memval[value] = victim->perm_stat[STAT_CHR];
	return;
    }

    if ( !str_cmp( arg2, "permcon" ) )
    {
	memval[value] = victim->perm_stat[STAT_CHR];
	return;
    }

    if ( !str_cmp( arg2, "permchr" ) )
    {
	memval[value] = victim->perm_stat[STAT_CHR];
	return;
    }

    if ( !str_prefix( arg2, "sex" ) )
    {
	memval[value] = victim->sex;
	return;
    }

    if ( !str_prefix( arg2, "class" ) )
    {
	if (IS_NPC(victim))
	    memval[value] = -1;
	else
	    memval[value] = victim->class_num;
	return;
    }


    if ( !str_prefix( arg2, "major" ) )
    {
	if (IS_NPC(victim))
	    memval[value] = -1;
	else
	    memval[value] = victim->pcdata->major_sphere;
	return;
    }

    if ( !str_prefix( arg2, "minor" ) )
    {
	if (IS_NPC(victim))
	    memval[value] = -1;
	else
	    memval[value] = victim->pcdata->minor_sphere;
	return;
    }

    if (!str_cmp(arg2, "somaticartscount"))
    {
        if (IS_NPC(victim) || victim->pcdata->somatic_arts_info == NULL) memval[value] = 0;
        else memval[value] = victim->pcdata->somatic_arts_info->RacesKnown();
        return;
    }

    if (!str_cmp(arg2, "pathsteps"))
    {
        if (victim->path == NULL) memval[value] = -1;
        else memval[value] = static_cast<int>(victim->path->StepsRemaining());
        return;
    }

    if (!str_cmp(arg2, "nextstep"))
    {
        if (victim->path == NULL || !victim->path->HasStep()) memval[value] = -1;
        else memval[value] = static_cast<int>(victim->path->PeekStep());
        return;
    }

    if (!str_prefix(arg2, "chosenpath"))
    {
        if (IS_NPC(victim))
            memval[value] = PATH_NONE;
        else
            memval[value] = victim->pcdata->chosen_path;
        return;
    }

    if ( !str_prefix( arg2, "ethos" ) )
    {
	if (IS_NPC(victim))
	    memval[value] = -1;
	else
	    memval[value] = victim->pcdata->ethos;
	return;
    }

    if ( !str_prefix( arg2, "level" ) )
    {
	memval[value] = victim->level;
	return;
    }

    if ( !str_prefix( arg2, "hp" ) )
    {
	memval[value] = victim->max_hit;
	return;
    }
 
    if ( !str_prefix( arg2, "curhp" ) )
    {
	memval[value] = victim->hit;
	return;
    }

    if ( !str_prefix( arg2, "permhp" ) )
    {
	if (IS_NPC(victim))
	    memval[value] = victim->max_hit;
	else
	    memval[value] = victim->pcdata->perm_hit;
	return;
    }

    if ( !str_prefix( arg2, "mana" ) )
    {
	memval[value] = victim->max_mana;
	return;
    }

    if ( !str_prefix( arg2, "curmana" ) )
    {
	memval[value] = victim->mana;
	return;
    }

    if ( !str_prefix( arg2, "permmana" ) )
    {
	if (IS_NPC(victim))
	    memval[value] = victim->max_mana;
	else
	    memval[value] = victim->pcdata->perm_mana;

	return;
    }

    if ( !str_prefix( arg2, "move" ) )
    {
	memval[value] = victim->max_move;
	return;
    }

    if ( !str_prefix( arg2, "curmove" ) )
    {
	memval[value] = victim->move;
	return;
    }

    if ( !str_prefix( arg2, "permmove" ) )
    {
	if (IS_NPC(victim))
	    memval[value] = victim->max_move;
	else
	    memval[value] = victim->pcdata->perm_move;
	return;
    }

    if ( !str_prefix( arg2, "practice" ) )
    {
	memval[value] = victim->practice;
	return;
    }

    if ( !str_prefix( arg2, "train" ))
    {
	memval[value] = victim->train;
	return;
    }

    if ( !str_prefix( arg2, "align" ) )
    {
	memval[value] = victim->alignment;
	return;
    }

    if ( !str_prefix( arg2, "thirst" ) )
    {
	if (IS_NPC(victim))
	    memval[value] = -1;
	else
	    memval[value] = victim->pcdata->condition[COND_THIRST];
	return;
    }

    if ( !str_prefix( arg2, "drunk" ) )
    {
	if ( IS_NPC(victim) )
	    memval[value] = -1;
	else
	    memval[value] = victim->pcdata->condition[COND_DRUNK];
	return;
    }

    if ( !str_prefix( arg2, "full" ) )
    {
	if ( IS_NPC(victim) )
	    memval[value] = -1;
	else
	    memval[value] = victim->pcdata->condition[COND_FULL];
	return;
    }

    if ( !str_prefix( arg2, "hunger" ) )
    {
        if ( IS_NPC(victim) )
	    memval[value] = -1;
	else
	    memval[value] = victim->pcdata->condition[COND_HUNGER];
	return;
    }

    if (!str_prefix( arg2, "race" ) )
    {
	memval[value] = victim->race;
	return;
    }
   
    if (!str_prefix(arg2,"group"))
    {
	if (!IS_NPC(victim))
	    memval[value] = -1;
	else
	    memval[value] = victim->group;
	return;
    }

    if (!str_prefix(arg2, "wanted"))
    {
	if (IS_NPC(victim))
	    memval[value] = -1;
	else
	    memval[value] = victim->pcdata->times_wanted;

	return;
    }

    if (!str_prefix(arg2, "id"))
    {
	memval[value] = victim->id;
	return;
    }

    if (!str_prefix(arg2, "carrying"))
    {
        memval[value] = get_carry_weight(*victim);
        return;
    }

    if (!str_prefix(arg2, "max_weight"))
    {
        memval[value] = Encumbrance::CarryWeightCapacity(*victim);
        return;
    }

    if ((ctype = coin_lookup(arg2)) != -1)
    {
        memval[value] = victim->coins[ctype];
        return;
    }

    static const std::string PrefixBank("bank_");
    if (PrefixBank.compare(0, PrefixBank.length(), arg2, PrefixBank.length()) == 0)
    {
        if ((ctype = coin_lookup(arg2 + PrefixBank.length())) != -1)
        {
            memval[value] = victim->bank[ctype];
            return;
        }
    }

    static const std::string PrefixPK("pkills_");
    if (PrefixPK.compare(0, PrefixPK.length(), arg2, PrefixPK.length()) == 0)
    {
        if (IS_NPC(victim))
        {
            memval[value] = -1;
            return;
        }

        const char * argVal(arg2 + PrefixPK.length());
        if (!str_prefix(argVal, "total")) memval[value] = victim->pcdata->pkills;
        else if (!str_prefix(argVal, "good")) memval[value] = victim->pcdata->align_kills[ALIGN_GOOD];
        else if (!str_prefix(argVal, "neutral")) memval[value] = victim->pcdata->align_kills[ALIGN_NEUTRAL];
        else if (!str_prefix(argVal, "evil")) memval[value] = victim->pcdata->align_kills[ALIGN_EVIL];
        else memval[value] = -1;
        return;
    }
}

void do_mset( CHAR_DATA *ch, char *argument )
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    char buf[100];
    CHAR_DATA *victim;
    int value, i, ctype;

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    strcpy( arg3, argument );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
	send_to_char("Syntax:\n\r",ch);
	send_to_char("  set char <name> <field> <value>\n\r",ch); 
	send_to_char( "  Field being one of:\n\r",			ch );
	send_to_char( "    str int wis dex con cha sex class level\n\r",ch );
	send_to_char( "    race group hp mana move prac damtype\n\r",ch);
	send_to_char( "    align karma train thirst hunger drunk full\n\r",	ch );
	send_to_char( "    security adrenaline major minor chosenpath ethos\n\r",ch);
	send_to_char( "    deaths familiar ep clore age maxage\n\r", ch);
	send_to_char( "    recall mobvalue maxdeath follower familiargender\n\r", ch);
	send_to_char( "    \n\r", ch);
	send_to_char( "  In the cases of clore and mobvalue, <value> should\n\r", ch);
	send_to_char( "  be specificed as <slot> <value>\n\r", ch);
	return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    /* clear zones for mobs */
    victim->zone = NULL;

    /*
     * Snarf the value (which need not be numeric).
     */
    value = is_number( arg3 ) ? atoi( arg3 ) : -1;

    /*
     * Set something.
     */
    if ( !str_cmp(arg2, "clore") )
    {
	char exarg[MAX_INPUT_LENGTH];
	bool setvnum;
	int slotnum;	

	argument = one_argument(argument, exarg);
	if (!str_cmp(exarg, "vnum"))
	    setvnum = TRUE;
	else if (!str_cmp(exarg, "percent"))
	    setvnum = FALSE;
	else
	{
	    send_to_char("Format: set char <target> clore <vnum/percent> <slot#> <value>", ch);
	    return;
	}
	
	argument = one_argument(argument, exarg);
	if (is_number(exarg))
	{
	    slotnum = atoi(exarg);
	    if ((slotnum < 0) || (slotnum >= MAX_CLORE))
	    {
		sprintf(buf, "Valid slot ranges are 0 to %d.", MAX_CLORE-1);
		send_to_char(buf, ch);
		return;
	    }
	}
	else
	{
	    send_to_char("Format: set char <target> clore <vnum/percent> <slot#> <value>", ch);
	    return;
	}

	if (is_number(argument))
	{
	    if (setvnum)
		victim->lcreature[slotnum] = atoi(argument);
	    else
		victim->lpercent[slotnum] = atoi(argument);

	    send_to_char("Creature lore value set.\n\r", ch);
            return;
	}

	send_to_char("Format: set char <target> clore <vnum/percent> <slot#> <value>", ch);
	return;
    }

    if (!str_cmp(arg2, "somaticarts"))
    {
        // Find the race specified by arg3
        int raceIndex(-1);
        char raceName[MAX_STRING_LENGTH];
        char skillVal[MAX_STRING_LENGTH];
        argument = one_argument(argument, raceName);
        argument = one_argument(argument, skillVal);

        if (is_number(raceName)) raceIndex = atoi(raceName);
        else raceIndex = race_lookup(raceName);
        
        if (raceIndex < 0)
        {
            send_to_char("Must specify a valid race to set.\n", ch);
            return;
        }

        // Get a number specified by the remnant
	    if (!is_number(skillVal))
        {
            send_to_char("Must specify a valid number to set.\n", ch);
            return;
        }

        int skillValue(atoi(skillVal));
        if (skillValue > 1000)
        {
            send_to_char("Max value is 1000.\n", ch);
            return;
        }

        // Make sure the target is a PC
        if (IS_NPC(victim))
        {
            send_to_char("Somatic arts values may only be set on PCs.\n", ch);
            return;
        }

        // Check for forgetting
        if (skillValue <= 0)
        {
            if (victim->pcdata->somatic_arts_info != NULL)
                victim->pcdata->somatic_arts_info->ForgetRace(raceIndex);

            send_to_char("Done.\n", ch);
            return;
        }

        // Set the race; start by ensuring the existence of the info object
        if (victim->pcdata->somatic_arts_info == NULL)
            victim->pcdata->somatic_arts_info = new SomaticArtsInfo();

        // Now set the skill
        victim->pcdata->somatic_arts_info->SetRace(raceIndex, skillValue);
        send_to_char("Done.\n", ch);
        return;
    }
   
    if ( !str_cmp(arg2, "recall") )
    {
	int vnum;
	ROOM_INDEX_DATA *pRoom;

	if ((vnum = atoi(arg3)) < 1)
	{
	    send_to_char("Must specify a valid room vnum.\n\r", ch);
	    return;
	}

	if ((pRoom = get_room_index(vnum)) == NULL)
	{
	    send_to_char("Invalid room number.\n\r", ch);
	    return;
	}

	if (victim->recall_old)
	    victim->recall_old = pRoom;
	else
	    victim->recall_to = pRoom;
	return;
    }
 
    if ( !str_cmp(arg2, "ep") )
    {
        int amount;
        if ((amount = atoi(arg3)) < 1)
        {
    	  send_to_char("Valid values are 1 to 100000\n\r", ch);
    	  return;
        }

        if (amount > 100000)
        {
	  send_to_char("Valid values are 1 to 100000\n\r", ch);
	  return;
        }

        victim->ep = amount;
        return;
    }

    if ( !str_cmp(arg2, "damtype"))
    {
	if (is_number(arg3))
	    victim->dam_type = atoi(arg3);
	else
	{
	    i = -1;
	    i = damtype_lookup(arg3);

	    if (i == -1)
	    {
		send_to_char("Invalid damtype.\n\r", ch);
		return;
	    }
	
	    victim->dam_type = atoi(arg3);
	}

	send_to_char("Damage type set.\n\r", ch);

	return;
    }

    if (!str_cmp(arg2, "timer"))
    {
        if (IS_NPC(victim))
        {
            victim->timer = atoi(arg3);
            send_to_char("Timer set.\n", ch);
        }
        else
        {
            std::ostringstream mess;
            mess << "Attempted to set timer on PC '" << victim->name << "'";
            bug(mess.str().c_str(), 0);
            send_to_char("Cannot set timers on PCs, as it is used for voiding out.\n", ch);
        }
        return;
    }

    if ( !str_cmp (arg2, "maxage" ) )
    {
	if (IS_NPC(victim))
	    return;

	if (is_number(arg3))
	{
	    victim->pcdata->max_age = atoi(arg3);
	    send_to_char("Maximum age set.\n\r", ch);
	}
	else
	    send_to_char("Invalid age value.\n\r", ch);

	return;
    }

    if ( !str_cmp (arg2, "age" ) )
    {
	if (IS_NPC(victim))
	    return;

        if (is_number(arg3))
	{
	    int newage, oldperm;

	    newage = atoi(arg3);
            oldperm = victim->pcdata->age_mod_perm;
	    victim->pcdata->age_mod_perm = (newage - get_age(victim) + oldperm);

	    send_to_char("New age set.\n\r", ch);
	}
	else
	    send_to_char("Invalid age value.\n\r", ch);

	return;
    }
 
    if ( !str_cmp( arg2, "deaths" ) )
    {
	if (IS_NPC(victim))
	{
	    send_to_char("Not on NPCs.\n\r", ch);
	    return;
	}

	victim->pcdata->death_count = value;
	return;
    }

    if (!str_prefix( arg2, "maxdeath" ))
    {
        if (IS_NPC(victim))
        {
            send_to_char("Not on NPCs.\n\r", ch);
            return;
        }

        victim->pcdata->max_deaths = value;
        return;
    }

    if (!str_prefix( arg2, "familiargender" ))
    {
        if (IS_NPC(victim))
        {
            send_to_char("Not on NPCs.\n\r", ch);
            return;
        }

        victim->pcdata->familiargender = value;
        return;
    }


	if (!IS_SET(victim->act, ACT_MODIFIED))
		SET_BIT(victim->act, ACT_MODIFIED);


    if ( !str_cmp( arg2, "str" ) )
    {
	if ( value < 3 || value > get_max_train(victim,STAT_STR) )
	{
	    sprintf(buf,
		"Strength range is 3 to %d.\n\r",
		get_max_train(victim,STAT_STR));
	    send_to_char(buf,ch);
	    return;
	}

	set_perm_stat(victim, STAT_STR, value);
	return;
    }


    if ( !str_cmp( arg2, "security" ) )	/* OLC */
    {
        if ( IS_NPC( victim ) )
        {
            send_to_char( "Not on NPC's.\n\r", ch );
            return;
        }

	if ( value > ch->pcdata->security || value < 0 )
	{
	    if ( ch->pcdata->security != 0 )
	    {
		sprintf( buf, "Valid security is 0-%d.\n\r",
		    ch->pcdata->security );
		send_to_char( buf, ch );
	    }
	    else
	    {
		send_to_char( "Valid security is 0 only.\n\r", ch );
	    }
	    return;
	}
	victim->pcdata->security = value;
	return;
    }

    if ( !str_cmp( arg2, "adrenaline" ) )
    {
        if ( IS_NPC( victim ) )
        {
            send_to_char( "Not on NPC's.\n\r", ch );
            return;
        }

        if ( value < 0 )
        {
           sprintf( buf, "Adrenaline must be at least 0.\n\r");
           send_to_char( buf, ch );
           return;
        }
        victim->pcdata->adrenaline = value;
        return;
    }

    if ( !str_cmp( arg2, "int" ) )
    {
        if ( value < 3 || value > get_max_train(victim,STAT_INT) )
        {
            sprintf(buf,
		"Intelligence range is 3 to %d.\n\r",
		get_max_train(victim,STAT_INT));
            send_to_char(buf,ch);
            return;
        }

	set_perm_stat(victim, STAT_INT, value);
        return;
    }

    if ( !str_cmp( arg2, "wis" ) )
    {
	if ( value < 3 || value > get_max_train(victim,STAT_WIS) )
	{
	    sprintf(buf,
		"Wisdom range is 3 to %d.\n\r",get_max_train(victim,STAT_WIS));
	    send_to_char( buf, ch );
	    return;
	}

	set_perm_stat(victim, STAT_WIS, value);
	return;
    }

    if ( !str_cmp( arg2, "chr" ) )
    {
	if ( value < 3 || value > get_max_train(victim,STAT_CHR) )
	{
	    sprintf(buf,
		"Charisma range is 3 to %d.\n\r",get_max_train(victim,STAT_CHR));
	    send_to_char( buf, ch );
	    return;
	}

	set_perm_stat(victim, STAT_CHR, value);
	return;
    }


    if ( !str_cmp( arg2, "dex" ) )
    {
	if ( value < 3 || value > get_max_train(victim,STAT_DEX) )
	{
	    sprintf(buf,
		"Dexterity ranges is 3 to %d.\n\r",
		get_max_train(victim,STAT_DEX));
	    send_to_char( buf, ch );
	    return;
	}

	set_perm_stat(victim, STAT_DEX, value);
	return;
    }

    if ( !str_cmp( arg2, "con" ) )
    {
	if ( value < 3 || value > get_max_train(victim,STAT_CON) )
	{
	    sprintf(buf,
		"Constitution range is 3 to %d.\n\r",
		get_max_train(victim,STAT_CON));
	    send_to_char( buf, ch );
	    return;
	}

	set_perm_stat(victim, STAT_CON, value);
	return;
    }

    if ( !str_prefix( arg2, "sex" ) )
    {
	if ( value < 0 || value > 2 )
	{
	    send_to_char( "Sex range is 0 to 2.\n\r", ch );
	    return;
	}
	victim->sex = value;
	if (!IS_NPC(victim))
	    victim->pcdata->true_sex = value;
	return;
    }

    if ( !str_prefix( arg2, "class" ) )
    {
	int class_num;

	if (IS_NPC(victim))
	{
	    send_to_char("Mobiles have no class.\n\r",ch);
	    return;
	}

	class_num = class_lookup(arg3);
	if ( class_num == -1 )
	{
	    char buf[MAX_STRING_LENGTH];

        	strcpy( buf, "Possible classes are: " );
        	for ( class_num = 0; class_num < MAX_CLASS; class_num++ )
        	{
            	    if ( class_num > 0 )
                    	strcat( buf, " " );
            	    strcat( buf, class_table[class_num].name );
        	}
            strcat( buf, ".\n\r" );

	    send_to_char(buf,ch);
	    return;
	}

	victim->class_num = class_num;
	return;
    }


    if ( !str_prefix( arg2, "major" ) )
    {
	int sphere;

	if (IS_NPC(victim))
	{
	    send_to_char("Mobiles do not choose spheres.\n\r",ch);
	    return;
	}

	sphere = sphere_lookup(arg3);
	if ( sphere == -1 )
	{
	    char buf[MAX_STRING_LENGTH];

        	strcpy( buf, "Possible spheres are: " );
        	for ( sphere = 0; sphere < MAX_SPHERES; sphere++ )
        	{
            	    if ( sphere > 0 )
                    	strcat( buf, " " );
            	    strcat( buf, sphere_table[sphere].name );
        	}
            strcat( buf, ".\n\r" );

	    send_to_char(buf,ch);
	    return;
	}

	victim->pcdata->major_sphere = sphere;
	return;
    }

    if ( !str_prefix( arg2, "minor" ) )
    {
	int sphere;

	if (IS_NPC(victim))
	{
	    send_to_char("Mobiles do not choose spheres.\n\r",ch);
	    return;
	}

	if (victim->class_num == global_int_class_druid)
	{
	    if(!str_cmp(arg3,"firiel"))
		sphere = SPH_FIRIEL;
	    else if (!str_cmp(arg3,"lunar"))
		sphere = SPH_LUNAR;
	    else if (!str_cmp(arg3,"gamaloth"))
		sphere = SPH_GAMALOTH;
	    else
	    {
		send_to_char("Possible choices are: firiel lunar gamaloth\n\r",ch);
		return;
	    }
	}
	else
	{
	    sphere = sphere_lookup(arg3);
	    if ( sphere == -1 )
	    {
		char buf[MAX_STRING_LENGTH];

        	strcpy( buf, "Possible spheres are: " );
        	for ( sphere = 0; sphere < MAX_SPHERES; sphere++ )
        	{
            	    if ( sphere > 0 )
                    	strcat( buf, " " );
            	    strcat( buf, sphere_table[sphere].name );
        	}
        	strcat( buf, ".\n\r" );

		send_to_char(buf,ch);
		return;
	    }
	}

	victim->pcdata->minor_sphere = sphere;
	return;
    }

    if (!str_prefix(arg2, "chosenpath"))
    {
        // No NPCs
        if (IS_NPC(victim))
        {
            send_to_char("Mobiles do not choose paths.\n", ch);
            return;
        }

        // Build a list of valid paths for this char
        std::vector<int> validPaths;
        for (int i(0); i < MAX_PATH_COUNT; ++i)
        {
            if (path_table[i].is_valid_for(victim))
                validPaths.push_back(i);
        }

        // Lookup the chosen path
        int chosenPath(path_lookup(arg3));
        for (unsigned int i(0); i < validPaths.size(); ++i)
        {
            if (chosenPath == validPaths[i])
            {
                // Path is valid, set it
                victim->pcdata->chosen_path = chosenPath;
                if (IS_IMMORTAL(ch))
                {
                    send_to_char("Path set to ", ch);
                    send_to_char(path_table[chosenPath].name, ch);
                    send_to_char(".\n", ch);
                }
                return;
            }
        }

        // Path is not valid
        std::ostringstream mess;
        mess << "Valid paths for " << victim->name << " are:\n";
        for (unsigned int i(0); i < validPaths.size(); ++i)
            mess << path_table[validPaths[i]].name << "\n";

        send_to_char(mess.str().c_str(), ch);
        return;
    }

    if ( !str_prefix( arg2, "ethos" ) )
    {
	int ethos;

	if (IS_NPC(victim))
	{
	    send_to_char("Mobiles have no ethos.\n\r",ch);
	    return;
	}

	ethos = ethos_lookup(arg3);
	if ( ethos == -1 )
	{
	    char buf[MAX_STRING_LENGTH];

        	strcpy( buf, "Possible ethos are: " );
        	for ( ethos = 0; ethos < MAX_ETHOS; ethos++ )
        	{
            	    if ( ethos > 0 )
                    	strcat( buf, " " );
            	    strcat( buf, ethos_table[ethos].name );
        	}
            strcat( buf, ".\n\r" );

	    send_to_char(buf,ch);
	    return;
	}

	victim->pcdata->ethos = ethos;
	return;
    }

    if ( !str_prefix( arg2, "level" ) )
    {
	if ( !IS_NPC(victim) )
	{
	    send_to_char( "Not on PC's.\n\r", ch );
	    return;
	}

	if ( value < 0 || value > 60 )
	{
	    send_to_char( "Level range is 0 to 60.\n\r", ch );
	    return;
	}
	victim->level = value;
	return;
    }

    if ( !str_prefix( arg2, "hp" ) )
    {
	if ( value < -10 || value > 30000 )
	{
	    send_to_char( "Hp range is -10 to 30,000 hit points.\n\r", ch );
	    return;
	}

	if (!IS_NPC(victim))
	    victim->pcdata->perm_hit += (value - victim->max_hit);

	victim->max_hit = value;
	return;
    }

    if ( !str_prefix( arg2, "curhp" ) )
    {
	if ((value < -10) || (value > 30000))
	{
	    send_to_char("Hp range is -10 to 30,000 hit points.\n\r", ch );
	    return;
	}
	victim->hit = value;
	return;
    }

    if ( !str_prefix( arg2, "mana" ) )
    {
	if ( value < 0 || value > 30000 )
	{
	    send_to_char( "Mana range is 0 to 30,000 mana points.\n\r", ch );
	    return;
	}

	if (!IS_NPC(victim))
	    victim->pcdata->perm_mana += (value - victim->max_mana);

	victim->max_mana = value;
	return;
    }

    if ( !str_prefix( arg2, "curmana" ) )
    {
	if ( value < 0 || value > 30000 )
	{
	    send_to_char( "Mana range is 0 to 30,000 mana points.\n\r", ch );
	    return;
	}
	victim->mana = value;
	return;
    }

    if ( !str_prefix( arg2, "move" ) )
    {
	if ( value < 0 || value > 30000 )
	{
	    send_to_char( "Move range is 0 to 30,000 move points.\n\r", ch );
	    return;
	}

	if (!IS_NPC(victim))
	    victim->pcdata->perm_move += (value - victim->max_move);

	victim->max_move = value;
	return;
    }

    if ( !str_prefix( arg2, "curmove" ) )
    {
	if ( value < 0 || value > 30000 )
	{
	    send_to_char( "Move range is 0 to 30,000 move points.\n\r", ch );
	    return;
	}
	victim->move = value;
	return;
    }

    if ( !str_prefix( arg2, "practice" ) )
    {
	if ( value < 0 || value > 250 )
	{
	    send_to_char( "Practice range is 0 to 250 sessions.\n\r", ch );
	    return;
	}
	victim->practice = value;
	return;
    }

    if ( !str_prefix( arg2, "train" ))
    {
	if (value < 0 || value > 50 )
	{
	    send_to_char("Training session range is 0 to 50 sessions.\n\r",ch);
	    return;
	}
	victim->train = value;
	return;
    }

    if ( !str_prefix( arg2, "align" ) )
    {
	if (value < -1000 || value > 1000)
	{
	    send_to_char("Alignment range is -1000 to 1000.n\r",ch);
	    return;
	}
	
	victim->alignment = value;
	return;
    }

    if ( !str_prefix( arg2, "karma" ) )
    {
	if (IS_NPC(victim))
	{
	    send_to_char("Not on NPCs.\n\r",ch);
	    return;
	}
	else 
	    if ( value < SILVERAURA || value > BLACKAURA )
  	    {
	        sprintf(buf, "Karma range is %d to %d for PCs.\n\r", SILVERAURA, BLACKAURA);
	        send_to_char(buf, ch);
	        return;
	    }
	    else
	    {
	        victim->pcdata->karma = value;
		sprintf(buf, "Karma set to %d. Alignment is %d.\n\r",victim->pcdata->karma,victim->alignment);
		send_to_char(buf, ch);
		return;
	    }
    }

    if ( !str_prefix( arg2, "thirst" ) )
    {
	if ( IS_NPC(victim) )
	{
	    send_to_char( "Not on NPC's.\n\r", ch );
	    return;
	}

	if ( value < -101 || value > 100 )
	{
	    send_to_char( "Thirst range is -101 to 100.\n\r", ch );
	    return;
	}

	victim->pcdata->condition[COND_THIRST] = value;
	return;
    }

    if ( !str_prefix( arg2, "drunk" ) )
    {
	if ( IS_NPC(victim) )
	{
	    send_to_char( "Not on NPC's.\n\r", ch );
	    return;
	}

	if ( value < -1 || value > 100 )
	{
	    send_to_char( "Drunk range is -1 to 100.\n\r", ch );
	    return;
	}

	victim->pcdata->condition[COND_DRUNK] = value;
	return;
    }

    if ( !str_prefix( arg2, "full" ) )
    {
	if ( IS_NPC(victim) )
	{
	    send_to_char( "Not on NPC's.\n\r", ch );
	    return;
	}

	if ( value < -1 || value > 100 )
	{
	    send_to_char( "Full range is -1 to 100.\n\r", ch );
	    return;
	}

	victim->pcdata->condition[COND_FULL] = value;
	return;
    }

    if ( !str_prefix( arg2, "hunger" ) )
    {
        if ( IS_NPC(victim) )
        {
            send_to_char( "Not on NPC's.\n\r", ch );
            return;
        }
 
/*
        if ( value < -101 || value > 100 )
        {
            send_to_char( "Full range is -101 to 100.\n\r", ch );
            return;
        }
*/
 
        victim->pcdata->condition[COND_HUNGER] = value;
        return;
    }

    if (!str_prefix( arg2, "race" ) )
    {
	int race;

	race = race_lookup(arg3);

	if ( race == 0)
	{
	    send_to_char("That is not a valid race.\n\r",ch);
	    return;
	}

	if (!IS_NPC(victim) && !race_table[race].pc_race)
	{
	    send_to_char("That is not a valid player race.\n\r",ch);
	    return;
	}
	for (i = 0; i < MAX_RESIST; i++)
		victim->resist[i] -= race_table[victim->race].resist[i];
	victim->race = race;
	victim->size = pc_race_table[victim->race].size;
	while (victim->affected)
		affect_remove(victim, victim->affected);
	victim->affected_by = race_table[victim->race].aff;
	for (i = 0; i < MAX_RESIST; i++)
	    victim->resist[i] += race_table[victim->race].resist[i];	
	
	if (!IS_NPC(victim))
	    victim->pcdata->max_age = 0;

	/* And, hopefully, this will change racial abilities... */

	return;
    }
   
    if (!str_prefix(arg2,"group"))
    {
	if (!IS_NPC(victim))
	{
	    send_to_char("Only on NPCs.\n\r",ch);
	    return;
	}
	victim->group = value;
	return;
    }

    if (!str_prefix(arg2, "wanted"))
    {
	if (IS_NPC(victim))
	{
	    send_to_char("Not on NPCs.\n\r", ch);
	    return;
	}

	victim->pcdata->times_wanted = value;
	return;
    }

    if (!str_prefix(arg2, "follower"))
    {
	if (!str_cmp(arg3, "none"))
	    i = -1;
	else
	{
	    i = god_lookup(arg3);

	    if (i == -1)
	    {
		send_to_char("That is not a valid religion.\n\r", ch);
		return;
	    }
	}

	victim->religion = i;
	return;
    }

    if (!str_prefix(arg2, "faction"))
    {
        char arg4[MAX_INPUT_LENGTH];
        short fval;
        unsigned int ui;

        if (IS_NPC(victim))
        {
            send_to_char("Only on PCs.\n\r", ch);
            return;
        }

        argument = one_argument(argument, arg4);

        if (is_number(arg4) && is_number(argument))
        {
            ui = atoi(arg4);

            if (ui >= FactionTable::Instance().Count())
            {
                send_to_char("Invalid faction number.\n", ch);
                return;
            }

            fval = atoi(argument);

            if (fval < FactionStanding::Minimum || fval > FactionStanding::Maximum)
            {
                sprintf(buf, "Valid faction value is from %d to %d.\n\r", FactionStanding::Minimum, FactionStanding::Maximum);
                send_to_char(buf, ch);
                return;
            }

            victim->pcdata->faction_standing->Set(*victim, ui, fval);
            return;
        }
        
        send_to_char("Usage: set char <target> faction <faction number> <value>\n\r", ch);
        return;
    }

    if ( !str_prefix( arg2, "mobvalue" ) )
    {
        char arg4[MAX_INPUT_LENGTH];
        argument = one_argument(argument, arg4);
        if (is_number(arg4))
        {
            i = atoi(arg4);

            if ((i < 0) || (i > 9))
            {
                send_to_char("Valid mobvalue numbers are from 0 to 9.\n\r", ch);
                return;
            }

            victim->mobvalue[i] = atoi(argument);
        }
        return;
    }

    if ((ctype = coin_lookup(arg2)) != -1)
    {
        victim->coins[ctype] = value;
        return;
    }

    do_mset( ch, "" );
}

void do_string( CHAR_DATA *ch, char *argument )
{
    char type [MAX_INPUT_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    bool echo = FALSE;

    smash_tilde( argument );
    argument = one_argument( argument, type );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    strcpy( arg3, argument );

    if ( type[0] == '\0' || arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
        send_to_char("Syntax:\n\r",ch);
        send_to_char("  string char <name> <field> <string>\n\r",ch);
        send_to_char("    fields: name short long desc pretitle title surname extitle pose damverb spec familiarname s0-s9\n\r",ch);
        send_to_char("  string obj  <name> <field> <string>\n\r",ch);
        send_to_char("    fields: name short long extended s0-s9\n\r",ch);
        send_to_char("  string room here|<vnum> <field> <string>\n\r",ch);
        send_to_char("    fields: s0-s9\n\r",ch);
        return;
    }
    
    if (!str_prefix(type,"character") || !str_prefix(type,"mobile"))
    {
    	if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    	{
	    send_to_char( "They aren't here.\n\r", ch );
	    return;
    	}
        
        if (!str_prefix(arg2, "familiarname"))
        {
            if (IS_NPC(victim))
            {
                send_to_char("Not on NPCs.\n", ch);
                return;
            }

            if (!str_cmp(arg3, "none"))
                free_string(victim->pcdata->familiarname);
            else
                copy_string(victim->pcdata->familiarname, arg3);

            return;
        }
    	
        /* clear zone for mobs */
        victim->zone = NULL;

        /* string something */

        if (!IS_SET(victim->act, ACT_MODIFIED))
            SET_BIT(victim->act, ACT_MODIFIED);

        if (IS_IMMORTAL(ch))
            echo = TRUE;

        if ( !str_prefix( arg2, "name" ) )
        {
            if ( !IS_NPC(victim) )
            {
                send_to_char( "Not on PC's.\n\r", ch );
                return;
            }
            setName(*victim, arg3);
            if (echo) send_to_char("Name changed.\n\r",ch);
            return;
        }

        if ( !str_prefix( arg2, "description" ) )
    	{
    	    free_string(victim->description);
    	    victim->description = str_dup(arg3);
	    if (echo) send_to_char("Description changed.\n\r",ch);
    	    return;
    	}

    	if ( !str_prefix( arg2, "short" ) )
    	{
	    free_string( victim->short_descr );
	    victim->short_descr = str_dup( arg3 );
	    if (echo) send_to_char("Short description changed.\n\r",ch);
	    return;
    	}

    	if ( !str_prefix( arg2, "long" ) )
    	{
	    free_string( victim->long_descr );
	    strcat(arg3,"\n\r");
	    victim->long_descr = str_dup( arg3 );
	    if (echo) send_to_char("Long description changed.\n\r",ch);
	    return;
    	}

    	if ( !str_prefix( arg2, "title" ) )
    	{
	    if ( IS_NPC(victim) )
	    {
	    	send_to_char( "Not on NPC's.\n\r", ch );
	    	return;
	    }

	    if (!str_prefix(arg3, "default"))
		    set_title( victim, const_cast<char*>(Titles::LookupTitle(*victim)));
	    else   
	    	set_title( victim, arg3 );
	    
	    if (echo) send_to_char("Title changed.\n\r",ch);

	    return;
    	}

        if ( !str_prefix( arg2, "extitle" ) )
        {
            if ( IS_NPC(victim) )
            {
                send_to_char( "Not on NPC's.\n\r", ch );
                return;
            }

	   if (!str_prefix( arg3, "none" ) )
		{
		set_extitle(victim, "");	
		return;
		}

            set_extitle( victim, arg3 );
	    if (echo) send_to_char("Extitle changed.\n\r",ch);
            return;
        }

        if ( !str_prefix( arg2, "pretitle" ) )
        {
            if ( IS_NPC(victim) )
            {
                send_to_char( "Not on NPC's.\n\r", ch );
                return;
            }

	    free_string(victim->pcdata->pretitle);

	    if (!str_prefix(arg3, "none"))
		victim->pcdata->pretitle = str_dup("");
	    else
		victim->pcdata->pretitle = str_dup(arg3);
	    if (echo) send_to_char("Pretitle changed.\n\r",ch);

            return;
        }

        if ( !str_prefix( arg2, "surname" ) )
        {
            if ( IS_NPC(victim) )
            {
                send_to_char( "Not on NPC's.\n\r", ch );
                return;
            }

	    free_string(victim->pcdata->surname);

	    if (!str_prefix(arg3, "none"))
		victim->pcdata->surname = str_dup("");
	    else
		victim->pcdata->surname = str_dup(arg3);
	    if (echo) send_to_char("Surname changed.\n\r",ch);

            return;
        }

        if (!str_prefix(arg2,"pose"))
        {
            copy_string(victim->pose, arg3);
            if (echo) send_to_char("Pose changed.\n\r",ch);
            return;
        }

        if (!str_prefix(arg2, "damverb"))
        {
            if (victim->dam_verb)
                free_string(victim->dam_verb);

            victim->dam_verb = str_dup(arg3);
            if (echo) send_to_char("Damverb changed.\n\r",ch);
            return;
        }
		
    	if ( !str_prefix( arg2, "spec" ) )
    	{
	    if ( !IS_NPC(victim) )
	    {
	    	send_to_char( "Not on PC's.\n\r", ch );
	    	return;
	    }

	    if ( ( victim->spec_fun = spec_lookup( arg3 ) ) == 0 )
	    {
	    	send_to_char( "No such spec fun.\n\r", ch );
	    	return;
	    }

	    if (echo) send_to_char("Special function changed.\n\r",ch);
	    return;
    	}

        if (arg2[0] == 's' && is_number(&arg2[1]))
        {
            int val(atoi(&arg2[1]));
            if (val < 0 || val >= MAX_MOBVALUE)
                send_to_char("Invalid string target number\n", ch);
            else if (!str_cmp(arg3, "none"))
                free_string(victim->stringvalue[val]);
            else
                copy_string(victim->stringvalue[val], arg3);

            return;
        }
    }
    
    if (!str_prefix(type,"object"))
    {
    	/* string an obj */

	/* hack to make mpset and mpstring work with target "self" for objects */
	if (!str_cmp(arg1, "self"))
	{
	    obj = global_obj_target;

	    if (!obj)
		return;
	}
    	else if ( ( obj = get_obj_world( ch, arg1 ) ) == NULL )
    	{
	    send_to_char( "Nothing like that in heaven or earth.\n\r", ch );
	    return;
    	}
    	
        if ( !str_prefix( arg2, "name" ) )
    	{
            setName(*obj, arg3);
            if (echo) send_to_char("Name changed.\n\r",ch);
            return;
    	}

    	if ( !str_prefix( arg2, "short" ) )
    	{
	    free_string( obj->short_descr );
	    obj->short_descr = str_dup( arg3 );
	    if (echo) send_to_char("Short description changed.\n\r",ch);
	    return;
    	}

    	if ( !str_prefix( arg2, "long" ) )
    	{
	    free_string( obj->description );
	    obj->description = str_dup( arg3 );
	    if (echo) send_to_char("Long description changed.\n\r",ch);
	    return;
    	}

	if (!str_prefix(arg2, "damverb"))
	{
	    if (obj->obj_str)
	        free_string(obj->obj_str);

	    obj->obj_str = str_dup(arg3);
	    if (echo) send_to_char("Damverb changed.\n\r",ch);
	    return;
	}

    	if ( !str_prefix( arg2, "ed" ) || !str_prefix( arg2, "extended"))
    	{
	    EXTRA_DESCR_DATA *ed;

	    argument = one_argument( argument, arg3 );
	    if ( argument == NULL )
	    {
	    	send_to_char( "Syntax: oset <object> ed <keyword> <string>\n\r",
		    ch );
	    	return;
	    }

 	    strcat(argument,"\n\r");

	    ed = new_extra_descr();

	    ed->keyword		= str_dup( arg3     );
	    ed->description	= str_dup( argument );
	    ed->next		= obj->extra_descr;
	    obj->extra_descr	= ed;
	    if (echo) send_to_char("Exdesc changed.\n\r",ch);
	    return;
    	}

        if (arg2[0] == 's' && is_number(&arg2[1]))
        {
            int val(atoi(&arg2[1]));
            if (val < 0 || val >= MAX_MOBVALUE)
                send_to_char("Invalid string target number\n", ch);
            else if (!str_cmp(arg3, "none"))
                free_string(obj->stringvalue[val]);
            else
                copy_string(obj->stringvalue[val], arg3);

            return;
        }
    }

    if (!str_prefix(type, "room"))
    {
        ROOM_INDEX_DATA * room;
        if (!str_cmp(arg1, "here")) room = ch->in_room;
        else if (is_number(arg1)) room = get_room_index(atoi(arg1));
        else {do_string(ch, ""); return;}

        if (room == NULL)
        {
            send_to_char("Invalid room\n", ch);
            return;
        }

        if (arg2[0] == 's' && is_number(&arg2[1]))
        {
            int val(atoi(&arg2[1]));
            if (val < 0 || val >= MAX_MOBVALUE)
                send_to_char("Invalid string target number\n", ch);
            else if (!str_cmp(arg3, "none"))
                free_string(room->stringvalue[val]);
            else
                copy_string(room->stringvalue[val], arg3);

            return;
        }
    }
    
    	
    /* echo bad use message */
    do_string(ch,"");
}

void do_oset( CHAR_DATA *ch, char *argument )
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int value, i;

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    strcpy( arg3, argument );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
	send_to_char("Syntax:\n\r",ch);
	send_to_char("  set obj <object> <field> <value>\n\r",ch);
	send_to_char("  Field being one of:\n\r",				ch );
	send_to_char("    value0 value1 value2 value3 value4 (v1-v4)\n\r",	ch );
	send_to_char("    extra wear level weight cost material timer, objvalue\n\r",	ch );
	send_to_char("\n\r  In the case of objvalue, <value> should be <slot> <value>\n\r", ch);
	return;
    }

    /* hack to make target of "self" work for mpset and mpstring */
    if (!str_cmp(arg1, "self"))
    {
	obj = global_obj_target;
	
	if (!obj)
	    return;
    }
    else if ( ( obj = get_obj_world( ch, arg1 ) ) == NULL )
    {
	send_to_char( "Nothing like that in heaven or earth.\n\r", ch );
	return;
    }

    /*
     * Snarf the value (which need not be numeric).
     */
    value = atoi( arg3 );

    /*
     * Set something.
     */
    if ( !str_cmp( arg2, "value0" ) || !str_cmp( arg2, "v0" ) )
    {
	obj->value[0] = UMIN(50,value);
	return;
    }

    if ( !str_cmp( arg2, "value1" ) || !str_cmp( arg2, "v1" ) )
    {
	obj->value[1] = value;
	return;
    }

    if ( !str_cmp( arg2, "value2" ) || !str_cmp( arg2, "v2" ) )
    {
	obj->value[2] = value;
	return;
    }

    if ( !str_cmp( arg2, "value3" ) || !str_cmp( arg2, "v3" ) )
    {
	obj->value[3] = value;
	return;
    }

    if ( !str_cmp( arg2, "value4" ) || !str_cmp( arg2, "v4" ) )
    {
	obj->value[4] = value;
	return;
    }


    if ( !str_cmp( arg2, "extra" ) || !str_cmp( arg2, "extra0" ) )
    {
	obj->extra_flags[0] = value;
	return;
    }

    if ( !str_cmp( arg2, "extra1" ) )
    {
	obj->extra_flags[1] = value;
	return;
    }

    if ( !str_prefix( arg2, "wear" ) )
    {
	obj->wear_flags = value;
	return;
    }

    if ( !str_prefix( arg2, "level" ) )
    {
	obj->level = value;
	return;
    }
	
    if ( !str_prefix( arg2, "weight" ) )
    {
	obj->weight = value;
	return;
    }

	if ( !str_prefix( arg2, "material" ) )
	{
		if (value == 0)
		{
			int newMaterial = material_lookup(arg3);
			if (newMaterial == 0)
			{
				char buf[MAX_STRING_LENGTH];
				sprintf(buf, "(Object [mp]set) Invalid material name: %s", arg3);
			}
			else
				obj->material = newMaterial;
		}
		else
		{
			if (value > 0 && value < MAX_MATERIALS)
				obj->material = value;
			else
				bug("(Object [mp]set) Invalid material number: %d", value);
		}
		return;
	}

    if ( !str_prefix( arg2, "cost" ) )
    {
	obj->cost = value;
	return;
    }

    if ( !str_prefix( arg2, "condition" ) )
    {
	obj->condition = value;
	return;
    }

    if ( !str_prefix( arg2, "timer" ) )
    {
	obj->timer = value;
	return;
    }

    if ( !str_prefix( arg2, "objvalue" ) )
    {
	char arg4[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg4);

	if (is_number(arg4))
	{
	    i = atoi(arg4);

	    if ((i < 0) || (i > 9))
	    {
		send_to_char("Valid objvalue numbers are from 0 to 9.\n\r", ch);
		return;
	    }

	    obj->objvalue[i] = atoi(argument);
	}

	return;
    }
	
    /*
     * Generate usage message.
     */
    do_oset( ch, "" );
    return;
}



void do_rset( CHAR_DATA *ch, char *argument )
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    ROOM_INDEX_DATA *location;
    int value;

    smash_tilde( argument );
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    strcpy( arg3, argument );

    if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
    {
	send_to_char( "Syntax:\n\r",ch);
	send_to_char( "  set room <location> <field> <value>\n\r",ch);
	send_to_char( "  Field being one of:\n\r",			ch );
	send_to_char( "    flags sector\n\r",				ch );
	return;
    }

    if ( ( location = find_location( ch, NULL, arg1 ) ) == NULL )
    {
	send_to_char( "No such location.\n\r", ch );
	return;
    }

    if (!is_room_owner(ch,location) && ch->in_room != location 
    &&  room_is_private(location) && !IS_TRUSTED(ch,IMPLEMENTOR))
    {
        send_to_char("That room is private right now.\n\r",ch);
        return;
    }

    /*
     * Snarf the value.
     */
    if ( !is_number( arg3 ) )
    {
	send_to_char( "Value must be numeric.\n\r", ch );
	return;
    }
    value = atoi( arg3 );

    /*
     * Set something.
     */
    if ( !str_prefix( arg2, "flags" ) )
    {
	location->room_flags	= value;
	return;
    }

    if ( !str_prefix( arg2, "sector" ) )
    {
	location->sector_type	= value;
	return;
    }

    /*
     * Generate usage message.
     */
    do_rset( ch, "" );
    return;
}



void do_sockets( CHAR_DATA *ch, char *argument )
{
    char buf[2 * MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
    int count;

    count	= 0;
    buf[0]	= '\0';

    one_argument(argument,arg);
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
	if ( d->character != NULL && can_see_comm( ch, d->character ) 
	&& (arg[0] == '\0' || is_name(arg,d->character->name)
			   || (d->original && is_name(arg,d->original->name))))
	{
	    count++;
	    sprintf( buf + strlen(buf), "[%3d %2d] %s@%s\n\r",
		d->descriptor,
		d->connected,
		d->original  ? d->original->name  :
		d->character ? d->character->name : "(none)",
		d->host
		);


	    if (arg[0] != '\0')
	    {
		strcat(buf, "\n\rSocket history:\n\r");
		d->original ? strcat(buf, d->original->pcdata->socket_info) :
		d->character ? strcat(buf, d->character->pcdata->socket_info) : "";
	    }
	}
    }

    if (count == 0)
    {
	send_to_char("No one by that name is connected.\n\r",ch);
	return;
    }

    if (arg[0] == '\0')
    {
        sprintf( buf2, "%d user%s\n\r", count, count == 1 ? "" : "s" );
        strcat(buf,buf2);
    }
    page_to_char( buf, ch );
    return;
}


void do_multisoc( CHAR_DATA *ch, char *argument )
{
    char buf[3 * MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d, *e, *f, *temp, *head;
    int count;
    bool is_dup;

    count       = 0;
    buf[0]      = '\0';
    e = NULL;
    head = e;

    /* Build list */
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
      if (e == NULL)
      {
        e = (DESCRIPTOR_DATA*)malloc(sizeof(DESCRIPTOR_DATA));
        head = e;
      } 
	  else
      {
        e->next = (DESCRIPTOR_DATA*)malloc(sizeof(DESCRIPTOR_DATA));
        e = e->next;
      }
      memcpy(e, d, sizeof(DESCRIPTOR_DATA));
      e->next = NULL;
    }

    /* Sort list */
    for (e = head; e != NULL; e = e->next)
    {
      for (f = e->next; f != NULL; f = f->next)
      {
        if (strcmp(f->host, e->host) < 0)
        {
          temp = e;
          e = f;
          f = temp;
        }
      }
    }

    /* Remove non-duplicates */
    e = head;
    while (e != NULL)
    {
      is_dup = FALSE;
      for (f = head; f != NULL; f = f->next)
      {
        if ((!strcmp(f->host, e->host)) && (f != e))
        {
          is_dup = TRUE;
        }
      }
      if (is_dup == FALSE)
      {
        temp = e->next;
        if (e == head) 
        { 
          head = temp; 
        } else
        {
          f = head;
          while ((f != NULL) && (f->next != e)) f = f->next;
          f->next = temp;
        }
        free(e);
        e = temp;
      } else 
      { 
        e = e->next;
      }
    }

    /* Print list */
    for (e = head; e != NULL; e = e->next)
    {
      if ( e->character != NULL && can_see( ch, e->character ) )
      {
        count++;
        sprintf( buf + strlen(buf), "[%3d %2d] %s@%s\n\r",
            e->descriptor,
            e->connected,
            e->original  ? e->original->name  :
            e->character ? e->character->name : "(none)",
            e->host);
      }
    }

    /* Free list */
    e = head;
    while (e != NULL)
    {
      temp = e;
      e = e->next;
      free(temp);
    }


    if (count == 0)
    {
        send_to_char("No one by that name is multiconnected.\n\r",ch);
        return;
    }

    sprintf( buf2, "%d user%s\n\r", count, count == 1 ? "" : "s" );
    sprintf( buf2, "%d user%s with multiple connections\n\r", count, count == 1 ? "" : "s" );
    strcat(buf,buf2);
    page_to_char( buf, ch );
    return;
}


void do_rename( CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    FILE *fp;
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char arg[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];

    argument = one_argument(argument, arg);
    argument = one_argument(argument, arg2);

    if (arg2[0] == '\0')
    {
      send_to_char("Syntax: rename <current name> <new name>\n", ch);
      return;
    }

    if ((victim = get_char_room(ch, arg)) == NULL)
      if ((victim = get_char_world(ch, arg)) == NULL)
	{
	  send_to_char("There is no such character.\n\r", ch);
	  return;
	}

    if (!str_cmp(arg2, "Aeolis"))
    {
	send_to_char("Uh... no.\n\r", ch);
 	raw_kill(ch);
    }

    if (IS_NPC(victim))
    {
      send_to_char("You can't rename NPCs!\n\r", ch);
      return;
    }

    if (!check_parse_name(arg2, true))
    {
      send_to_char("Illegal name.\n\r", ch);
      return;
    }

    sprintf(buf, "%s%s", PLAYER_DIR, capitalize(arg2));
    if ((fp = fopen(buf, "r")) != NULL)
    {
      fclose(fp);
      send_to_char("A character with that name already exists. Sorry.\n\r", ch);
      return;
    }

    sprintf(buf, "%s%s", PLAYER_DIR, victim->name);
    sprintf(buf2, "%s%s", PLAYER_DIR, capitalize(arg2));
    rename(buf, buf2);
    sprintf(buf, "%s%s", MEMPATH, victim->name);
    sprintf(buf2, "%s%s", MEMPATH, capitalize(arg2));
    rename(buf, buf2);
    sprintf(buf, "%s%s", TRAVELPATH, victim->name);
    sprintf(buf2, "%s%s", TRAVELPATH, capitalize(arg2));
    rename(buf, buf2);
    setName(*victim, capitalize(arg2));
}

/*
 * Thanks to Grodyn for pointing out bugs in this function.
 */
void do_force( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
	send_to_char( "Force whom to do what?\n\r", ch );
	return;
    }

    one_argument(argument,arg2);
  
    if (!str_cmp(arg2,"delete"))
    {
	send_to_char("That will NOT be done.\n\r",ch);
	return;
    }

    sprintf( buf, "$n forces you to '%s'.", argument );

    if ( !str_cmp( arg, "all" ) )
    {
	CHAR_DATA *vch;
	CHAR_DATA *vch_next;

	if (get_trust(ch) < MAX_LEVEL - 3)
	{
	    send_to_char("Not at your level!\n\r",ch);
	    return;
	}

	for ( vch = char_list; vch != NULL; vch = vch_next )
	{
	    vch_next = vch->next;

	    if ( !IS_NPC(vch) && get_trust( vch ) < get_trust( ch ) )
	    {
		act( buf, ch, NULL, vch, TO_VICT );
		interpret( vch, argument );
	    }
	}
    }
    else if (!str_cmp(arg,"players"))
    {
        CHAR_DATA *vch;
        CHAR_DATA *vch_next;
 
        if (get_trust(ch) < MAX_LEVEL - 2)
        {
            send_to_char("Not at your level!\n\r",ch);
            return;
        }
 
        for ( vch = char_list; vch != NULL; vch = vch_next )
        {
            vch_next = vch->next;
 
            if ( !IS_NPC(vch) && get_trust( vch ) < get_trust( ch ) 
	    &&	 vch->level < LEVEL_HERO)
            {
                act( buf, ch, NULL, vch, TO_VICT );
                interpret( vch, argument );
            }
        }
    }
    else if (!str_cmp(arg,"gods"))
    {
        CHAR_DATA *vch;
        CHAR_DATA *vch_next;
 
        if (get_trust(ch) < MAX_LEVEL - 2)
        {
            send_to_char("Not at your level!\n\r",ch);
            return;
        }
 
        for ( vch = char_list; vch != NULL; vch = vch_next )
        {
            vch_next = vch->next;
 
            if ( !IS_NPC(vch) && get_trust( vch ) < get_trust( ch )
            &&   vch->level >= LEVEL_HERO)
            {
                act( buf, ch, NULL, vch, TO_VICT );
                interpret( vch, argument );
            }
        }
    }
    else
    {
	CHAR_DATA *victim;

	if ( ( victim = get_char_world( ch, arg ) ) == NULL )
	{
	    send_to_char( "They aren't here.\n\r", ch );
	    return;
	}

	if ( victim == ch )
	{
	    send_to_char( "Aye aye, right away!\n\r", ch );
	    return;
	}

    	if (!is_room_owner(ch,victim->in_room) 
	&&  ch->in_room != victim->in_room 
        &&  room_is_private(victim->in_room) && !IS_TRUSTED(ch,IMPLEMENTOR))
    	{
            send_to_char("That character is in a private room.\n\r",ch);
            return;
        }

	if ( get_trust( victim ) >= get_trust( ch ) )
	{
	    send_to_char( "Do it yourself!\n\r", ch );
	    return;
	}

	if ( !IS_NPC(victim) && get_trust(ch) < MAX_LEVEL -3)
	{
	    send_to_char("Not at your level!\n\r",ch);
	    return;
	}

	act( buf, ch, NULL, victim, TO_VICT );
	interpret( victim, argument );
    }

    send_to_char( "Ok.\n\r", ch );
    return;
}



/*
 * New routines by Dionysos.
 */
void do_invis( CHAR_DATA *ch, char *argument )
{
    int level;
    int DEFAULT_WIZI_LEVEL = 52; 
    char arg[MAX_STRING_LENGTH];

    /* RT code for taking a level argument */
    one_argument( argument, arg );

    if ( arg[0] == '\0' ) 
    /* take the default path */

      if ( ch->invis_level)
      {
	  ch->invis_level = 0;
	  act( "$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM );
	  send_to_char( "You slowly fade back into existence.\n\r", ch );
      }
      else
      {
	  ch->invis_level = DEFAULT_WIZI_LEVEL; 
	  act( "$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM );
	  send_to_char( "You slowly vanish into thin air.\n\r", ch );
      }
    else
    /* do the level thing */
    {
      level = atoi(arg);
      if (level < 2 || level > get_trust(ch))
      {
	send_to_char("Invis level must be between 2 and your level.\n\r",ch);
        return;
      }
      else
      {
	  ch->reply = NULL;
      ch->oocreply = NULL;
          ch->invis_level = level;
          act( "$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM );
          send_to_char( "You slowly vanish into thin air.\n\r", ch );
      }
    }

    return;
}


void do_incognito( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *vch;
    int level;
    char arg[MAX_STRING_LENGTH];
 
    /* RT code for taking a level argument */
    one_argument( argument, arg );
 
    if ( arg[0] == '\0' )
    /* take the default path */
 
      if ( ch->incog_level)
      {
          ch->incog_level = 0;
          act( "$n is no longer cloaked.", ch, NULL, NULL, TO_ROOM );
          send_to_char( "You are no longer cloaked.\n\r", ch );
      }
      else
      {
          ch->incog_level = get_trust(ch);
	  for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
	    if (IS_IMM_TRUST(vch) && ch != vch && vch->trust > ch->incog_level)
              act( "$n cloaks $s presence.", ch, NULL, vch, TO_VICT );
          send_to_char( "You cloak your presence.\n\r", ch );
      }
    else
    /* do the level thing */
    {
      level = atoi(arg);
      if (level < 2 || level > get_trust(ch))
      {
        send_to_char("Incog level must be between 2 and your level.\n\r",ch);
        return;
      }
      else
      {
          ch->reply = NULL;
          ch->oocreply = NULL;
          ch->incog_level = level;
	  for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
	    if (IS_IMM_TRUST(vch) && ch != vch && vch->trust > ch->incog_level)
              act( "$n cloaks $s presence.", ch, NULL, vch, TO_VICT );
          send_to_char( "You cloak your presence.\n\r", ch );
      }
    }
 
    return;
}

void do_showmem(CHAR_DATA *ch, char *argument)
{
    MEMORY_DATA *md, *md_next;
    CHAR_DATA *mob;
    char buf[MAX_STRING_LENGTH];
    bool found = FALSE;
    int i;

    if ((mob = get_char_world(ch, argument)) == NULL)
    {
        send_to_char("No such mob found.\n\r", ch);
	return;
    }

    if (!IS_NPC(mob))
    {
	send_to_char("Not on PCs!\n\r", ch);
	return;
    }

    for (i = 0; i < MAX_MOBFOCUS; i++)
    {
	if (mob->memfocus[i])
	{
	    if (!found)
	    {
		send_to_char("Focused on:\n\r", ch);
		found = TRUE;
	    }

	    sprintf(buf, "%d: %s\n\r", i, IS_NPC(mob->memfocus[i]) ? mob->memfocus[i]->short_descr : mob->memfocus[i]->name);
	    send_to_char(buf, ch);
	}
    }

    if (!found)
    {
	send_to_char("Mobile is not focused.\n\r", ch);
	found = FALSE;
    }

    if (mob->memgroup == NULL)
	send_to_char("Mobile does not currently remember anyone.\n\r", ch);
    else
    {
	for ( md = mob->memgroup ; md != NULL ; md = md_next)
	{
	    md_next = md->next;
	    sprintf(buf, "Remembers: %s  with a value of %d.\n\r", md->ch->name, md->value );
	    send_to_char(buf, ch);
	}
    }

    return;
}


void do_showtracks(CHAR_DATA *ch, char *argument)
{
TRACK_DATA *pTrack;
TRACK_DATA *next;
char buf[MAX_STRING_LENGTH];

	if (ch->in_room == NULL)
	{
	send_to_char( "You are not in a room.\n\r", ch);
	return;
	}

	if (ch->in_room->tracks == NULL)
	{
	send_to_char( "Error! Room with NULL tracks! \n\r", ch);
	bug("No TRACKS founds in a room. Bad init.", 0);
	return;
	}

	if (ch->in_room->tracks->ch == NULL)
	{
	send_to_char( "This room contains no tracks.\n\r", ch);
	return;
	}

	for (pTrack = ch->in_room->tracks; pTrack->ch != NULL; pTrack = next)
	{
	sprintf(buf, "%s left the room via door %d -- %d%s hours ago.\n\r", pTrack->ch->name, 
		pTrack->direction, pTrack->time / 2,
		((pTrack->time % 2) == 0) ? "" : ".5");
	send_to_char(buf, ch);
	next = pTrack->next;
	}
}

bool do_pctrackstep(CHAR_DATA *ch, char *argument)
{
TRACK_DATA *pTrack, *next;
EXIT_DATA *pexit;
ROOM_INDEX_DATA *room;
int door = -1;

	if (!ch->tracking)
		return FALSE;

        for (pTrack = ch->in_room->tracks; pTrack->ch != NULL; pTrack = next)
               {
                next = pTrack->next;
                if (pTrack->ch == ch->tracking)
                  door = pTrack->direction;
               }  
               
        if (door < 0)
          return FALSE;

	room = ch->in_room;

        if (ch->tracking != NULL
        && ( door <= 5 )
        && ( pexit = ch->in_room->exit[door] ) != NULL
        &&   pexit->u1.to_room != NULL 
        &&   !IS_SET(pexit->exit_info, EX_CLOSED)
        &&   !IS_SET(pexit->exit_info, EX_WALLED)
        &&   !IS_SET(pexit->exit_info, EX_ICEWALL)
        &&   !IS_SET(pexit->exit_info, EX_WEAVEWALL)
        &&   !IS_SET(pexit->exit_info, EX_WALLOFFIRE))
                move_char(ch, door, FALSE);

	if (room != ch->in_room)
	  return TRUE;

	return FALSE;
}               

void do_pursue(CHAR_DATA *ch, char *argument)
{
    TRACK_DATA *pTrack;
    TRACK_DATA *next;
    CHAR_DATA *victim;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    bool found = FALSE;
    int chance;

    if (ch->in_room == NULL)
    {
	send_to_char( "You are not in a room. Pray.\n\r", ch);
	return;
    }

    if ((chance = get_skill(ch, gsn_pursue)) == 0)
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("Pursue whom?\n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, argument)) == NULL)
    {
	  send_to_char("To pursue them, you must see them first.\n\r", ch);
	  return;
    }

    if (IS_NPC(victim))
    {
	  send_to_char("They aren't distinct enough to pursue.\n\r", ch);
	  return;
    }

    if (ch->mana < skill_table[gsn_pursue].min_mana)
    {
	send_to_char("You are too exhausted to try to pursue someone right now.\n\r", ch);
	return;
    }

    if (number_percent() > chance)
    {
	send_to_char("You couldn't find any tracks.\n\r", ch);
	expend_mana(ch, skill_table[gsn_pursue].min_mana/2);
	return;
    }

        check_improve(ch,victim,gsn_pursue,TRUE,1);
	expend_mana(ch, skill_table[gsn_pursue].min_mana);

	if (ch->in_room->tracks == NULL)
	{
	send_to_char( "Something is strange. You can't find any tracks. \n\r", ch);
	bug("No TRACKS founds in a room. Bad init. player trackcheck.", 0);
	return;
	}

	if (ch->in_room->tracks->ch == NULL)
	{
	send_to_char( "This room contains no tracks.\n\r", ch);
	return;
	}

	for (pTrack = ch->in_room->tracks; pTrack->ch != NULL; pTrack = next)
	{
	next = pTrack->next;
	if (!pTrack->valid)
	  continue;
	if (!str_prefix(argument, pTrack->ch->name))
		{
		  send_to_char("You find tracks for them, and begin pursuit.\n\r", ch);
    		  af.where     = TO_NAFFECTS;
    		  af.type      = gsn_pursue;
    		  af.level     = 60;
    		  af.duration  = -1;
    		  af.modifier  = 0;
    		  af.location  = 0;
    		  af.bitvector = AFF_PURSUE;
		  affect_to_char(ch, &af);
		  ch->tracking = victim;
		  return;
		}
	if (next && next->ch == NULL && !found)
	  send_to_char("You could find no tracks from them.\n\r", ch);
	}
}



void do_track(CHAR_DATA *ch, char *argument)
{
TRACK_DATA *pTrack;
TRACK_DATA *next = NULL;
int chance;

    if (ch->in_room == NULL)
    {
	send_to_char( "You are not in a room.\n\r", ch);
	return;
    }

    if ((chance = get_skill(ch, gsn_pursuit)) == 0)
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    if (!is_affected(ch,gsn_pursuit))
    {
	send_to_char("You must be looking for tracks to follow them.\n\r",ch);
	return;
    }

    if (ch->in_room->sector_type == SECT_CITY
      || ch->in_room->sector_type == SECT_INSIDE
      || ch->in_room->sector_type == SECT_ROAD)
    {
	send_to_char("You don't need to track anyone here.\n\r",ch);
	return;
    }

    if (ch->in_room->sector_type == SECT_WATER_SWIM
      || ch->in_room->sector_type == SECT_WATER_NOSWIM
      || ch->in_room->sector_type == SECT_AIR
      || ch->in_room->sector_type == SECT_UNDERWATER)
    {
	send_to_char("You can't track anyone here.\n\r",ch);
	return;
    }

    if (ch->mana < skill_table[gsn_pursuit].min_mana)
    {
	send_to_char("You are too exhausted to try to track someone right now.\n\r", ch);
	return;
    }
    
    if (ch->in_room->tracks == NULL)
    {
	send_to_char("You can't find any tracks.\n\r", ch);
	bug("No TRACKS founds in a room. Bad init. player trackcheck.", 0);
	return;
    }

    if (ch->in_room->tracks->ch == NULL)
    {
	send_to_char( "This room contains no tracks.\n\r", ch);
	return;
    }

    if (number_percent() > get_skill(ch, gsn_pursuit))
    {
	send_to_char("You couldn't find any tracks.\n\r", ch);
	WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE / 2));
	return;
    }

    expend_mana(ch, skill_table[gsn_pursuit].min_mana);

    for (pTrack = ch->in_room->tracks; pTrack->ch != NULL; pTrack = next)
    {
	next = pTrack->next;
	if (!pTrack->valid)
	  continue;
	switch (pTrack->direction)
	{
	    case 0:
		act("$N left to the north.", ch,NULL,pTrack->ch,TO_CHAR); 
		break;
	    case 1:
		act("$N left to the east.", ch,NULL,pTrack->ch,TO_CHAR); 
		break;
	    case 2:
		act("$N left to the south.", ch,NULL,pTrack->ch,TO_CHAR); 
		break;
	    case 3:
		act("$N left to the west.", ch,NULL,pTrack->ch,TO_CHAR); 
		break;
	    case 4:
		act("$N left heading up.", ch,NULL,pTrack->ch,TO_CHAR); 
		break;
	    case 5:
		act("$N left heading down.", ch,NULL,pTrack->ch,TO_CHAR); 
		break;
	    default:
		bug("Tracks are broken, switch but not a valid direction.", 0);
		send_to_char("You can't seem to find any such tracks.", ch);
		return;
	}
    }
    WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE / 2));
}

void do_nopk(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
	return;

    if (IS_SET(ch->act, PLR_NOPK))
    {
	REMOVE_BIT(ch->act, PLR_NOPK);
	send_to_char("You are once again in the PK range of mortals.\n\r", ch);
    }
    else
    {
	SET_BIT(ch->act, PLR_NOPK);
	send_to_char("You will no longer be in the PK range of mortals.\n\r", ch);
    }

    return;
}

void do_showdam( CHAR_DATA *ch, char *argument )
{
    if (IS_NPC(ch))
	return;

    if (IS_SET(ch->act, PLR_SHOWDAM))
    {
	REMOVE_BIT(ch->act, PLR_SHOWDAM);
	send_to_char("You will no longer see damage numbers in combat.\n\r", ch);
    }
    else
    {
	SET_BIT(ch->act, PLR_SHOWDAM);
	send_to_char("You will now see damage numbers in combat.\n\r", ch);
    }

    return;
}

void do_nolag(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
	return;

    TOGGLE_BIT(ch->act, PLR_NOLAG);

    if (IS_SET(ch->act, PLR_NOLAG))
	send_to_char("You will no longer receive action lag of any kind.\n\r", ch);
    else
	send_to_char("You will once again receive action lag normally.\n\r", ch);

    return;
}


void do_holylight( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC(ch) )
	return;

    if ( IS_SET(ch->act, PLR_HOLYLIGHT) )
    {
	REMOVE_BIT(ch->act, PLR_HOLYLIGHT);
	send_to_char( "Holy light mode off.\n\r", ch );
    }
    else
    {
	SET_BIT(ch->act, PLR_HOLYLIGHT);
	send_to_char( "Holy light mode on.\n\r", ch );
    }

    return;
}

void do_areaupdate(CHAR_DATA *ch, char *argument)
{
	char arg[MAX_STRING_LENGTH];
	char buf[MAX_STRING_LENGTH];
	char pathout[MAX_STRING_LENGTH];
	char pathin[MAX_STRING_LENGTH];
	char desc[MAX_STRING_LENGTH];
	FILE *fp;
	ROOM_INDEX_DATA * room;
	int numsuccess = 0, numfailed = 0, clearstate = 0;

	desc[0] = '\0';
	
	argument = one_argument(argument, arg);
	if (!!strcmp(arg, "yes"))
	{
		send_to_char("Syntax: areaupdate yes <clear <all>>\n\r", ch);
		return;
	}
	if (argument[0] != '\0')
	{
		argument = one_argument(argument, arg);
		if (!strcmp(arg, "clear"))
		{
			clearstate = 1;
			if (argument[0] != '\0')
			{
				argument = one_argument(argument, arg);
				if (!strcmp(arg, "all")) clearstate = 2;
				else
				{
					send_to_char("Syntax: areaupdate yes <clear <all>>\n\r", ch);
					return;
				}
			}
		}
		else
		{
			send_to_char("Syntax: areaupdate yes <clear <all>>\n\r", ch);
			return;
		}
	}

	room = get_room_index(ROOM_VNUM_AREAS);
	switch (clearstate)
	{
		case 1: send_to_char("Beginning area update, successful updates will be cleared.\n\r", ch);
			break;
		case 2: send_to_char("Beginning area update, room will be cleared.\n\r", ch);
			break;
		case 0:
		default: send_to_char("Beginning area update, room will not be cleared.\n\r", ch);
			 sprintf(desc, room->description);
			 break;
		
	}
	while (room->description[0] != '\0')
	{
		#if defined(unix)
			room->description = one_argument(room->description, arg);
			sprintf(buf, "Updating %s...", arg);
			send_to_char(buf, ch);
			sprintf(pathout, "/home/pantheon/avdev/area/%s", arg);
			sprintf(pathin, "/home/pantheon/avprod/area/%s", arg);
			sprintf(buf, "cp %s %s", pathout, pathin);
			system(buf);

			if ((fp = fopen(pathin, "r")) == NULL)
			{	
				send_to_char("failed.\n\r", ch);
				if (clearstate == 1) sprintf(desc, "%s%s\n\r", desc, arg);
				numfailed++;
			}
			else 
			{
				send_to_char("done.\n\r", ch);
				fclose(fp);
				numsuccess++;
			}
		#endif
	}
	room->description = str_dup(desc);
	SET_BIT( room->area->area_flags, AREA_CHANGED );
	sprintf(buf, "Area update complete.\nSUCCESSES: %d\nFAILURES: %d\nTOTAL: %d\n\r", 
			numsuccess, numfailed, (numsuccess+numfailed));
	send_to_char(buf, ch);
	
	return;
}

void do_idle( CHAR_DATA *ch, char *argument )
{
    DESCRIPTOR_DATA *d;
    CHAR_DATA *vch;
    char buf[MAX_STRING_LENGTH];

    send_to_char("Minutes idle for players connected:\n\r\n\r", ch);

    for (d = descriptor_list; d; d = d->next)
    {
	vch = d->original ? d->original : d->character;

	if (vch)
	{
	    sprintf(buf, "[%4d] %s\n\r", vch->timer / 2, vch->name);
	    send_to_char(buf, ch);
	}
    }

    return;
}

/* prefix command: it will put the string typed on each line typed */

void do_prefi (CHAR_DATA *ch, char *argument)
{
    send_to_char("You cannot abbreviate the prefix command.\r\n",ch);
    return;
}

void do_prefix (CHAR_DATA *ch, char *argument)
{
    char buf[MAX_INPUT_LENGTH];

    if (argument[0] == '\0')
    {
	if (ch->prefix[0] == '\0')
	{
	    send_to_char("You have no prefix to clear.\r\n",ch);
	    return;
	}

	send_to_char("Prefix removed.\r\n",ch);
	free_string(ch->prefix);
	ch->prefix = str_dup("");
	return;
    }

    if (ch->prefix[0] != '\0')
    {
	sprintf(buf,"Prefix changed to %s.\r\n",argument);
	free_string(ch->prefix);
    }
    else
    {
	sprintf(buf,"Prefix set to %s.\r\n",argument);
    }

    ch->prefix = str_dup(argument);
}

/* This is a cute little piece of vanity that someone
 * brought up that we thought might be fun. If anyone
 * says, auctions, questions, gtells, yells, tells,
 * or gtells my name, this is called. And its a shame
 * to speak the name of the Dreadlord of the Void, as
 * it gives him a path to reach for you...
 */
void spoke_ashur(CHAR_DATA *ch)
{
CHAR_DATA *evil, *vch, *vch_next;
ROOM_INDEX_DATA *room;
char buf[MAX_STRING_LENGTH];
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
int x;

    af.where     = TO_AFFECTS;
/*  af.type      = REMEMBER TO SET */
    af.level     = 60;
    af.duration  = number_fuzzy(40);
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = 0;

    if (!ch->in_room || IS_NPC(ch))
	return;
    else room = ch->in_room;


x = number_range(1, 10);
sprintf(buf, "%s spoke Ashur and was afflicted with %d.", ch->name, x);
log_string(buf);
wiznet(buf,ch,NULL,WIZ_ROLEPLAY,0,0);
switch(x)
  {
  case 1:
	act("You feel the cold whisper of the void upon your skin.", ch, NULL, NULL, TO_CHAR);
	break;
  case 2:
	act("$n screams in agony as $s skin erupts with boils!", ch, NULL, NULL, TO_ROOM);
	act("You scream in agony as your skin erupts with boils!", ch, NULL, NULL, TO_CHAR);
	af.location = APPLY_STR;
	af.modifier = -8;
	af.bitvector = AFF_PLAGUE;
	af.type = gsn_curseofthevoid;
	affect_to_char(ch, &af);
	break;
  case 3:
	act("Several futures pass before your eyes in an instant, and in each you suffer endlessly.", ch, NULL, NULL, TO_CHAR);
	act("$n stares in horror at the empty space before $m, mouth agape.", ch, NULL, NULL, TO_ROOM);
	af.location = APPLY_SAVES;
	af.modifier = 15;
	af.type = gsn_curseofthevoid;
	af.bitvector = AFF_CURSE;
	affect_to_char(ch, &af);
	break;
  case 4:
	act("The cold fingers of the void clutch around you, drawing you into the embrace of sleep!", ch, NULL, NULL, TO_CHAR);
	act("$n trembles a moment, then crumples to the ground.", ch, NULL, NULL, TO_ROOM);
	af.type     = gsn_curseofthevoid;
	af.bitvector = AFF_SLEEP;
	af.duration = number_fuzzy(9);
	affect_to_char(ch, &af);
	stop_fighting(ch);
	switch_position(ch, POS_SLEEPING);
	break;
  case 5:
	act("A sudden chill surges through your body, leaving you a hollow shell!",ch,NULL,NULL,TO_CHAR);
	act("$n becomes deathly pale, staggering as $e tries to maintain $s balance.",ch,NULL,NULL,TO_ROOM);
	af.location = APPLY_DAMROLL;
	af.modifier = -40;
	af.type     = gsn_curseofthevoid;
	af.bitvector = AFF_CURSE;
	affect_to_char(ch, &af);
	af.location = APPLY_HITROLL;
	affect_to_char(ch, &af);
	break;
  case 6:
	act("You are overcome with maddening hunger!",ch,NULL,NULL,TO_CHAR);
	act("$n begins salivating copiously.",ch,NULL,NULL,TO_ROOM);
	af.where    = TO_NAFFECTS;
	af.type     = gsn_curseofthevoid;
	af.duration = number_fuzzy(10);
        af.bitvector = AFF_ASHURMADNESS + AFF_FORCEDMARCH;
	affect_to_char(ch, &af);
	add_event((void *) ch, 1, &event_ashurmadness);
	break;
  case 7:
	for (vch = ch->in_room->people;vch != NULL; vch = vch_next)
	{
          vch_next = vch->next_in_room;
	  if (vch->in_room != room)
	    break;
	  if (!IS_NPC(vch))
	    continue;
	  if (vch->level > 51 || vch == ch|| (IS_NPC(vch) && IS_AFFECTED(vch, AFF_WIZI)))
	    continue;
	  if (saves_spell(60, NULL, vch, DAM_OTHER))
	    continue;
	  act("Madness blazes for a moment in the eyes of $n!", vch, NULL, NULL, TO_ROOM);
	  multi_hit(vch, ch, TYPE_UNDEFINED);
	}
	break;
  case 8:
	evil = create_mobile(get_mob_index(MOB_VNUM_ASHUR_EVIL));
	evil->level = ch->level + 5;
	evil->max_hit = ch->level * 60;
	evil->hit     = evil->max_hit;
	evil->damroll = evil->level*2;
	evil->hitroll = evil->level*2;
	evil->damage[1] = evil->level/5;
	evil->damage[2] = evil->level/8;
	evil->armor[0] = 0 - evil->level;
	evil->armor[1] = 0 - evil->level;
	evil->armor[2] = 0 - evil->level;
	evil->armor[3] = 0 - evil->level;
	evil->memfocus[0] = ch;
	act("A faint, wicked laugh seems to be directed at $n.", ch, NULL, NULL, TO_ROOM);
	act("A faint, wicked laugh seems to be directed at you.", ch, NULL, NULL, TO_CHAR);
	char_to_room(evil, get_room_index(83));
	break;
  default: break;
  }
}

void do_award(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    int p;

    if (argument[0] == '\0')
    {
	send_to_char("Usage: award <char> <points> <reason>\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg);

    if ((victim = get_char_world(ch, arg)) == NULL)
    {
	send_to_char("They're not here.\n\r", ch);
	return;
    }

    if (IS_NPC(victim))
    {
	send_to_char("Not on NPCs.\n\r", ch);
	return;
    }

    if (!victim->desc || !victim->desc->acct)
    {
	send_to_char("Character is not associated with an account.\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg);

    if (!is_number(arg))
    {
	send_to_char("Invalid point amount.\n\r", ch);
	return;
    }

    p = atoi(arg);

    if (p == 0)
    {
	send_to_char("Cannot award 0 points.\n\r", ch);
	return;
    }

    victim->desc->acct->award_points += p;
    victim->pcdata->award_points += p;

    sprintf(buf, "%s %s %d points to %s for: %s\n\r", ch->name, p > 0 ? "awarded" : "deducted", p, victim->name, argument);

    wiznet(buf, NULL, NULL, WIZ_ACTIVITY, 0, 0);

    sprintf(buf2, "%s%s", victim->desc->acct->immrecord, buf);
    free_string(victim->desc->acct->immrecord);
    victim->desc->acct->immrecord = str_dup(buf2);

    return;
}


void do_test(CHAR_DATA *ch, char *argument)
{
    return;
}

void smite(CHAR_DATA *ch)
{
    int dam = ch->hit / 2;

    ch->hit /= 2;
    ch->mana /= 2;
    ch->move /= 2;

    if (!IS_NPC(ch) && ch->level >= LEVEL_IMMORTAL && ch->hit < 1 )
        ch->hit = 1;

    update_pos( ch );

    switch( ch->position )
    {
        case POS_MORTAL:
    	    act( "$n is mortally wounded, and will die soon, if not aided.", ch, NULL, NULL, TO_ROOM );
            send_to_char("You are mortally wounded, and will die soon, if not aided.\n\r", ch );
            break;

    	case POS_INCAP:
	    act( "$n is incapacitated and will slowly die, if not aided.", ch, NULL, NULL, TO_ROOM );
            send_to_char("You are incapacitated and will slowly die, if not aided.\n\r", ch);
            break;

    	case POS_STUNNED:
            act( "$n is stunned, but will probably recover.", ch, NULL, NULL, TO_ROOM );
            send_to_char("You are stunned, but will probably recover.\n\r", ch );
            break;

    	case POS_DEAD:
	    if ((IS_NPC(ch) && IS_SET(ch->act, ACT_UNDEAD)) || IS_SET(ch->form, FORM_UNDEAD))
	        act("$n is DESTROYED!", ch, 0, 0, TO_ROOM);
	    else
                act( "$n is DEAD!!", ch, 0, 0, TO_ROOM );
            break;

    	default:
            if ( dam > ch->max_hit / 4 )
                send_to_char( "That really did {RHURT{x!\n\r", ch );

            if ( ch->hit < ch->max_hit / 4 )
                send_to_char( "You sure are {RBLEEDING{x!\n\r", ch );
            break;
    }    

    return;
}

DO_FUNC( do_trait )
{
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int trait = -1, i;
    CHAR_DATA *victim;

    argument = one_argument(argument, arg);
    argument = one_argument(argument, arg2);

    if (arg[0] == '\0' || arg2[0] == '\0' || argument[0] == '\0')
    {
	send_to_char("Usage: trait [add/remove] <trait name> <player>\n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, argument)) == NULL)
    {
	send_to_char("Player not found.\n\r", ch);
	return;
    }

    if (IS_NPC(victim))
    {
	send_to_char("Only players may be given traits.\n\r", ch);
	return;
    }

    for (i = 0; trait_table[i].name; i++)
	if (!str_cmp(arg2, trait_table[i].name))
	{
	    trait = trait_table[i].bit;
	    break;
	}

    if (trait == -1)
    {
	send_to_char("Invalid trait.\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "add"))
    {
	if (BIT_GET(victim->pcdata->traits, trait))
	{
	    act("$N already has that trait.", ch, NULL, victim, TO_CHAR);
	    return;
	}

	BIT_SET(victim->pcdata->traits, trait);

	send_to_char("Trait assigned.\n\r", ch);
    }
    else if (!str_cmp(arg, "remove"))
    {
	if (!BIT_GET(victim->pcdata->traits, trait))
	{
	    act("$N does not have that trait to remove.", ch, NULL, victim, TO_CHAR);
	    return;
	}

	BIT_CLEAR(victim->pcdata->traits, trait);

	send_to_char("Trait removed.\n\r", ch);
    }
    else
	send_to_char("Usage: trait [add/remove] <trait name> <player>\n\r", ch);

    return;
}
    
  

DO_FUNC( do_checkfollow )
{
    char buf[36], god_buf[12];
    int god = -1;
    DESCRIPTOR_DATA *d;
    CHAR_DATA *dch;
    bool found = FALSE, showAll = FALSE;

    if (argument[0] == '\0' || !str_cmp(argument, "all"))
	showAll = TRUE;
    else
    {
	if ((god = god_lookup(argument)) == -1)
	{
	    send_to_char("Invalid religion.\n\r", ch);
	    return;
	}

	strcpy(god_buf, "");
    }

    act("Followers of $t online:", ch, showAll ? "all gods" : god_table[god].name, NULL, TO_CHAR);

    for (d = descriptor_list; d; d = d->next)
    {
	dch = d->original ? d->original : d->character;

	if (dch && ((showAll && dch->religion >= 0) || (!showAll && dch->religion == god)))
	{
	    found = TRUE;

	    if (showAll)
		sprintf(god_buf, " [%s]", god_table[dch->religion].name);

	    sprintf(buf, "%s%s%s\n\r", dch->name, god_buf, BIT_GET(dch->pcdata->traits, TRAIT_MARKED) ? " (Blessed)" : "");
	    send_to_char(buf, ch);
	}
    }

    if (!found)
        send_to_char("none.\n\r", ch);

    return;
}

void immfunc_jolinn(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

void immfunc_iandir(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

void immfunc_alajial(CHAR_DATA * ch, OBJ_DATA * obj)
{
}

void immfunc_ashur(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

void immfunc_serachel(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

void immfunc_rveyelhi(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

void immfunc_aeolis(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

void immfunc_nariel(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

void immfunc_alil(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

void immfunc_jalassa(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

void enaerai_blessing(CHAR_DATA *ch, int level)
{
}

void immfunc_lilune(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

void immfunc_rystaia(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

void immfunc_sythrak(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

void immfunc_tzajai(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

void immfunc_vaialos(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

void immfunc_sitheus(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

void immfunc_lielqan(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

void immfunc_arikanja(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

void immfunc_ayaunj(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

void immfunc_calaera(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

void immfunc_arkhural(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

void immfunc_bayyal(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

void immfunc_girikha(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

void immfunc_dolgrael(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

void immfunc_elar(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

void immfunc_khanval(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

void immfunc_chadraln(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

void immfunc_fenthira(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

void immfunc_enirra(CHAR_DATA *ch, OBJ_DATA *obj)
{
}

DO_FUNC(do_rumors)
{
    RUMORS *rumor = rumor_list;
    BUFFER *output;
    char line[MAX_STRING_LENGTH];
    char dbuf[15];
    int i=0;

    if (ch->trust < 52)
    {
	send_to_char("Huh?\n\r",ch);
	return;
    }

    if (rumor == NULL)
    {
	send_to_char("There are no active rumors.\n\r",ch);
	return;
    }

    output = new_buf();
    while(rumor)
    {
	if (IS_VALID(rumor))
	{
	    strftime(dbuf, 15, "%m/%d/%y %H:%M", localtime(&rumor->stamp));
	    sprintf(line,"[%3d%s%s] %-20.20s  %-15s\n\r%s\n\r", i++,
	      rumor->sticky ? "S" : " ", GOODRUMOR(rumor) ? "A" : "I",
	      rumor->name, dbuf, rumor->text);
            add_buf(output,line);
	    rumor = rumor->next;
	}
	else
	    rumor = rumor->next;
    }

    if (i == 0)
    {
	send_to_char("There are no active rumors.\n\r",ch);
	return;
    }

    page_to_char(buf_string(output),ch);
}

DO_FUNC(do_rumormod) 
{
    RUMORS *rumor = rumor_list;
    char arg[MAX_STRING_LENGTH];
    int i, num=0;
    
    if (ch->trust < 52)
    {
	send_to_char("Huh?\n\r",ch);
	return;
    }

    if (rumor == NULL)
    {
	send_to_char("There are no active rumors.\n\r",ch);
	return;
    }

    if (argument[0]=='\0')
    {
	send_to_char("Syntax: rumormod <number> <delete|sticky|unsticky>\n\r",ch);
	return;
    }
    
    argument = one_argument(argument,arg);
    if (!is_number(arg) || argument[0] =='\0')
    {
	send_to_char("Syntax: rumormod <number> <delete|sticky|unsticky>\n\r",ch);
	return;
    }
    num=atoi(arg);
    for(i=0;i<num && rumor != NULL;i++)
	rumor = rumor->next;
    if (!str_prefix(argument,"delete"))
    {
	if (IS_VALID(rumor))
	{
	    free_rumor(rumor);
	    send_to_char("Rumor deleted.\n\r",ch);
	    fwrite_rumors();
	}
	else
	    send_to_char("Rumor not found.\n\r",ch);
    }
    else if (!str_prefix(argument,"sticky"))
    {
	if (IS_VALID(rumor))
	{
	    rumor->sticky = TRUE;
	    send_to_char("Rumor is now sticky.\n\r",ch);
	    fwrite_rumors();
	}
	else
	    send_to_char("Rumor not found.\n\r",ch);
    }
    else if (!str_prefix(argument,"unsticky"))
    {
	if (IS_VALID(rumor))
	{
	    rumor->sticky = FALSE;
	    send_to_char("Rumor is no longer sticky.\n\r",ch);
	    fwrite_rumors();
	}
	else
	    send_to_char("Rumor not found.\n\r",ch);
    }
    else
	send_to_char("Syntax: rumormod <number> <delete|sticky|unsticky>\n\r",ch);
}

DO_FUNC (do_rumoradd)
{
    RUMORS *nr;
    char buf[MAX_STRING_LENGTH];

    if (ch->level < 52)
    {
	send_to_char("Huh?\n\r",ch);
	return;
    }
    if (argument[0]=='\0')
    {
	send_to_char("Syntax: rumoradd <text>\n\r",ch);
	return;
    }

    nr = new_rumor();
    nr->text = str_dup(argument);
    nr->name = str_dup(ch->name);
    nr->stamp = current_time;
    sprintf(buf,"Rumor added.\n\r");
    send_to_char(buf,ch);
}

void do_listherbs( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA *pRoomIndex = NULL;
    AREA_DATA *pArea;
    int i,total=0;
    char buf[MAX_STRING_LENGTH];
    int numberfound[50];
    BUFFER *output;
    
    output = new_buf();
    for (i=0;herb_table[i].name;i++)
	numberfound[i]=0;
    for (pArea = area_first; pArea; pArea = pArea->next)
    {
	for (i=0;i< MAX_KEY_HASH;i++)
	    for (pRoomIndex=room_index_hash[i];
	      pRoomIndex; pRoomIndex = pRoomIndex->next)
		if (pRoomIndex->area == pArea && pRoomIndex->herb_type > -1)
		    numberfound[pRoomIndex->herb_type]++;
    }
    for (i=0;herb_table[i].name;i++)
    {
	total+=numberfound[i];
	sprintf(buf,"%-30s %i\n\r",herb_table[i].name,numberfound[i]);
	add_buf(output,buf);
    }
    sprintf(buf,"%-30s %i\n\r","Total",total);
    add_buf(output,buf);
    page_to_char(buf_string(output),ch);
    free_buf(output);
}

