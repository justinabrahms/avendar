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
#include <unistd.h>
#else
#include <time.h>
#endif
#include <sstream>
#include <iomanip>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <mysql/mysql.h>
#include "merc.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "languages.h"
#include "itoa_header.h"
#include "DisplayPanel.h"
#include "PyreInfo.h"
#include "Weave.h"
#include "spells_spirit.h"
#include "spells_spirit_air.h"
#include "spells_water.h"
#include "spells_air_void.h"
#include "spells_void.h"
#include "PhantasmInfo.h"
#include "Faction.h"
#include "NameMaps.h"
#include "Drakes.h"
#include "Encumbrance.h"
#include "Runes.h"

const char *visions_room = "Visions of Truth\n\r\n\r  All around you you see the sins of your past. Each evil committed,\n\reach wrong done. You close your eyes to shut out the visions, but even with your\n\reyes closed, you see the suffering caused when you strayed from the path\n\rof the pure.\n\r\n\r";

const char * brume_room = "A Dense, Heavy Fog\n\n  A thick brume has settled here, filling this place with a dusky haze. The\ndensity of the fog makes it difficult to see more than a foot or two in any direction\nbefore the glistening droplets crowd out any detail, offering only\namorphous shapes and indistinct lines in its place.\n\n";

const char *icy_prison_desc = "An Icy Prison\n\r  Thick walls of ice surround you on every side, their encompassing mass preventing your escape.  The interior of this place is extremely cold and enclosed, sapping away at your strength.\n\r\n\r[Exits: none]\n\r";

/* command procedures needed */
DECLARE_DO_FUN(	do_exits	);
DECLARE_DO_FUN( do_look		);
DECLARE_DO_FUN( do_help		);
DECLARE_DO_FUN( do_affects	);
DECLARE_DO_FUN( do_play		);

extern MYSQL mysql;

bool    write_to_descriptor     args( ( int desc, const char *txt, int length ) );
extern int global_int_race_shuddeni;

// extern const	struct	language_type	lang_data[MAX_LANGUAGE];

char *	const	where_name	[] =
{
    "used as light",
    "worn on finger",
    "worn on finger",
    "worn around neck",
    "worn around neck",
    "worn on torso",
    "worn on head",
    "worn on legs",
    "worn on feet",
    "worn on hands",
    "worn on arms",
    "worn as shield",
    "worn about body",
    "worn about waist",
    "worn around wrist",
    "worn around wrist",
    "wielded",
    "held",
    "floating nearby",
    "dual wielded",
    "branded",
    "concealed",
    "concealed",
    "progged",
    "quiver"
};

static std::string show_eq_to_char(CHAR_DATA & ch, CHAR_DATA & viewer, bool showAbsent, unsigned int & count);

bool is_flying(const CHAR_DATA *ch)
{
    if (is_affected(ch,gsn_pounce))	return FALSE;
    if (IS_AFFECTED(ch,AFF_FLYING))	return TRUE;
    if (IS_AFFECTED(ch,AFF_FLY_NATURAL)) return TRUE;
    return FALSE;
}

bool flight_blocked(const CHAR_DATA & chIn)
{
    CHAR_DATA * ch(const_cast<CHAR_DATA *>(&chIn));

    // Disqualifiers
    if (is_affected(ch, gsn_earthbind)) return true;
    if (ch->in_room != NULL)
    {
         if (area_is_affected(ch->in_room->area, gsn_gravitywell)) return true;
         if (ch->in_room->sector_type == SECT_UNDERWATER) return true;
         if (room_is_affected(ch->in_room, gsn_stonehaven)) return true;
    }
    return false;
}

bool can_fly_natural(const CHAR_DATA & ch)
{
    if (IS_SET(race_table[ch.race].aff, AFF_FLY_NATURAL)) return true;
    if (Drakes::IsMinimumAge(ch, Drakes::Wingborne)) return true;
    return false;
}

bool can_fly_magical(const CHAR_DATA & chIn)
{
    CHAR_DATA * ch(const_cast<CHAR_DATA *>(&chIn));
    if (is_affected(ch, gsn_fly)) return true;
    if (is_affected(ch, gsn_massflying)) return true;
    if (is_affected(ch, gsn_wingsofflame)) return true;
    if (is_affected(ch, gsn_seraphicwings)) return true;
    if (IS_OAFFECTED(ch, AFF_GHOST)) return true;
    if (is_affected(ch, gsn_levitation)) return true;
    if (is_affected(ch, gsn_hawkform)) return true;
    return false;
}

bool can_fly(const CHAR_DATA & ch)
{
    if (flight_blocked(ch)) return false;
    if (can_fly_natural(ch)) return true;
    if (can_fly_magical(ch)) return true;
    return false;
}

void stop_flying(CHAR_DATA & ch)
{
    REMOVE_BIT(ch.affected_by, AFF_FLYING);
    REMOVE_BIT(ch.affected_by, AFF_FLY_NATURAL);
}

/* for do_count */
int max_on = 0;

/*
 * Local functions.
 */
int	is_lawful		args( (CHAR_DATA *ch) );
int	is_balanced		args( (CHAR_DATA *ch) );
int	is_chaotic		args( (CHAR_DATA *ch) );
char *	format_obj_to_char	args( ( OBJ_DATA *obj, CHAR_DATA *ch,
				    bool fShort ) );
void	show_list_to_char	args( ( OBJ_DATA *list, CHAR_DATA *ch,
				    bool fShort, bool fShowNothing ) );
void	show_char_to_char_1	args( ( CHAR_DATA *victim, CHAR_DATA *ch ) );
void	show_char_to_char	args( ( CHAR_DATA *list, CHAR_DATA *ch ) );
bool	check_blind		args( ( CHAR_DATA *ch ) );
char *  format_who		args( ( CHAR_DATA *wch, char *buf, CHAR_DATA *ch ) );

extern int get_moon_state args( ( int moon_num, bool get_size ) );
extern CHAR_DATA *load_offline_char(char *name);


int is_lawful(CHAR_DATA *ch)
{
if (IS_NPC(ch))
	return 0;
else if (ch->pcdata->ethos == ETH_LAWFUL)
	return 1;
return 0;
}

int is_balanced(CHAR_DATA *ch)
{
if (IS_NPC(ch))
	return 0;
else if (ch->pcdata->ethos == ETH_BALANCED)
	return 1;
return 0;
}

int is_chaotic(CHAR_DATA *ch)
{
    if (IS_NPC(ch)) return 0;
    return (ch->pcdata->ethos == ETH_CHAOTIC);
}

void do_factions(CHAR_DATA * ch, char * argument)
{
    // Determine all the factions for this character
    DisplayPanel::Box allies;
    DisplayPanel::Box enemies;
    for (size_t i(0); i < FactionTable::Instance().Count(); ++i)
    {
        switch (FactionTable::CurrentStanding(*ch, i))
        {
            case Rating_Friend: allies.AddLine("{W" + FactionTable::Instance()[i].Name() + "{x");  break;
            case Rating_Enemy:  enemies.AddLine("{W" + FactionTable::Instance()[i].Name() + "{x"); break;
            default: break;
        }
    }

    // Handle the case of no status to show
    if (allies.LineCount() == 0 && enemies.LineCount() == 0)
    {
        send_to_char("You are neither friend nor foe to any faction.\n", ch);
        return;
    }

    // Build the vertical splits
    DisplayPanel::VerticalSplit allyColumn(DisplayPanel::Box("Allied Factions"), allies);
    DisplayPanel::VerticalSplit enemyColumn(DisplayPanel::Box("Enemy Factions"), enemies);

    // Build the main panel
    DisplayPanel::HorizontalSplit mainPanel(allyColumn, enemyColumn);
    send_to_char(DisplayPanel::Render(mainPanel).c_str(), ch);
}

void do_acctdata(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH],dbuf[MAX_STRING_LENGTH];
    char goat[2*MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);

    if (argument[0] == '\0')
    {
	send_to_char("Usage: acctdata <target> <data>\n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL)
    {
	send_to_char("You cannot find that person in the world.\n\r", ch);
	return;
    }

    if (IS_NPC(victim))
    {
	send_to_char("Not on NPCs.\n\r", ch);
	return;
    }

    if (!victim->desc || !victim->desc->acct)
    {
	send_to_char("Target player is not on an account.\n\r", ch);
	return;
    }

    if (victim->desc->acct->immrecord && ((strlen(victim->desc->acct->immrecord) + strlen(argument)) > (MAX_RECORD_STRING - 50)))
    {
	send_to_char("Record space full!\n\r", ch);
	return;
    }

    strftime(dbuf, 13, "%b %d, %Y", localtime(&current_time));

    if (victim->desc->acct->immrecord)
    {
	strcpy(buf, victim->desc->acct->immrecord);
        sprintf(goat, "[%s] %s recorded: %s\n", dbuf, ch->name, argument);
        strcat(buf, goat);
        free_string(victim->desc->acct->immrecord);
        victim->desc->acct->immrecord = str_dup(buf);
    }
    else
    {
        sprintf(buf, "[%s] %s recorded: %s\n", dbuf, ch->name, argument);
        victim->desc->acct->immrecord = str_dup(buf);
    }

    send_to_char("You recorded the information.\n\r", ch);
    sprintf(buf, "$N recorded the following data on %s's account:\n\r%s", victim->name,argument);
    wiznet(buf, ch, NULL, WIZ_ACTIVITY, 0, 0);
    return;
}


void do_data(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char dbuf[13];
    char goat[2*MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg);

    if (argument[0] == '\0')
    {
	send_to_char("Record information on whom? Record what?\n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL)
    {
	send_to_char("You can find no such person to record information about.\n\r", ch);
	return;
    }

    if (IS_NPC(victim))
    {
        send_to_char("Not on NPCS.\n\r", ch);
        return;
    }

    if (victim->pcdata->immrecord
     && (strlen(victim->pcdata->immrecord) + strlen(argument)) > (MAX_RECORD_STRING - 50))
    {
	send_to_char("Record space full!\n\r", ch);
	return;
    }

    strftime(dbuf, 13, "%b %d, %Y", localtime(&current_time));

    if (victim->pcdata->immrecord)
    {
	strcpy(buf, victim->pcdata->immrecord);
	sprintf(goat, "[%s] %s: %s\n", dbuf, ch->desc->original ? ch->desc->original->name : ch->name, argument);
	strcat(buf, goat);
	free_string(victim->pcdata->immrecord);
	victim->pcdata->immrecord = str_dup(buf);
    }
    else
    {
	sprintf(buf, "[%s] %s: %s\n", dbuf, ch->desc->original ? ch->desc->original->name : ch->name, argument);
	victim->pcdata->immrecord = str_dup(buf);
    }

    send_to_char("You recorded the information.\n\r", ch);
    sprintf(buf, "$N recorded the following data on %s:\n\r%s", victim->name,argument);
    wiznet(buf, ch, NULL, WIZ_ACTIVITY, 0, 0);

    return;
}

void do_record(CHAR_DATA *ch, char *argument)
{
CHAR_DATA *victim;
char arg[MAX_INPUT_LENGTH];
char buf[MAX_STRING_LENGTH];
char goat[2*MAX_INPUT_LENGTH];

    if (ch->class_num != global_int_class_watcher && ch->level < 20)
    {
        send_to_char( "Huh?\n\r", ch );
        return;
    }

    argument = one_argument(argument, arg);

    if (argument[0] == '\0')
	{
	  send_to_char("Record information on whom? Record what?\n\r", ch);
	  return;
	}

    if ((victim = get_char_world(ch, arg)) == NULL)
	{
	  send_to_char("You can find no such person to record information about.\n\r", ch);
	  return;
	}

    if (IS_NPC(victim))
    {
      send_to_char("They aren't unique enough to record information about.\n\r", ch);
      return;
    }

  if (victim->pcdata->record)
    if ((strlen(victim->pcdata->record) + strlen(argument)) > (MAX_RECORD_STRING - 50))
	{
	  send_to_char("You don't feel you can record anything regarding them. The gods would no doubt be interested in that.\n\r", ch);
	  return;
	}

    smash_punc(argument);

    if (victim->pcdata->record)
    {
      strcpy(buf, victim->pcdata->record);
      sprintf(goat, "%s recorded: %s\n", ch->name, argument);
      strcat(buf, goat);
      free_string(victim->pcdata->record);
      victim->pcdata->record = str_dup(buf);
    }
    else
    {
      sprintf(buf, "%s recorded: %s\n", ch->name, argument);
      victim->pcdata->record = str_dup(buf);
    }

    send_to_char("You recorded the information.\n\r", ch);

    return;
}

void do_hooded(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    char buf[MAX_STRING_LENGTH];
    int chance;

    if ((chance = get_skill(ch,gsn_hooded)) == 0)
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    if (!stone_check(ch,gsn_hooded))
    {
	send_to_char("You cannot draw enough power from the stones.\n\r", ch);
	return;
    }
 
    if (is_affected(ch,gsn_hooded))
    {
        affect_strip(ch, gsn_hooded);
        act("You remove your hood.", ch, NULL, NULL, TO_CHAR);
        act("$n removes $s hood.", ch, NULL, NULL, TO_ROOM);
        return;
    }

    if (IS_OAFFECTED(ch, AFF_DISGUISE))
    {
	send_to_char("Your true appearance is already concealed.\n\r", ch);
	return;
    }

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_hooded].beats));

    if (number_percent() > chance)
    {
	send_to_char("You fail to properly hood yourself.\n\r", ch);
	check_improve(ch, NULL,gsn_hooded, FALSE, 1);
    }
    else
    {
	send_to_char("You don a dark hood, concealing your true appearance.\n\r", ch);
	act("$n dons a dark hood, concealing $s true appearance.", ch, NULL, NULL, TO_ROOM);

	check_improve(ch,NULL, gsn_hooded, TRUE, 1);
	if (ch->orig_long[0] == '\0' && ch->long_descr[0] != '\0')
	{
	    free_string(ch->orig_long);
	    ch->orig_long = str_dup(ch->long_descr);
	}
	free_string(ch->long_descr);
    ch->long_descr = str_dup("A hooded figure stands here.\n\r");
    setFakeName(*ch, "hooded figure rogue");
	free_string(ch->short_descr);
    ch->short_descr = str_dup("a hooded rogue");
	if (ch->orig_description[0] == '\0')
	{
	    free_string(ch->orig_description);
            ch->orig_description = str_dup(ch->description);
	}
        free_string(ch->description);
        sprintf(buf, "A hooded %s is here, its identity cunningly hidden.\n\r",
	    race_table[ch->race].name);
        ch->description = str_dup(buf);

	sprintf(buf, "hooded%ld", ch->id);
    setUniqueName(*ch, buf);

        af.where     = TO_OAFFECTS;
        af.type      = gsn_hooded;
        af.level     = ch->level;
        af.duration  = ch->level/2;
	af.point = NULL;
        af.location  = 0;
        af.modifier  = 0;
        af.bitvector = AFF_DISGUISE;
        affect_to_char(ch, &af);
    }
}

void do_disguise(CHAR_DATA *ch, char *argument)
{
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
CHAR_DATA *victim;
char buf[MAX_STRING_LENGTH];
int chance;

    if ((chance = get_skill(ch, gsn_disguise)) == 0)
    {
        send_to_char( "Huh?\n\r", ch );
        return;
    }

    if (is_affected(ch, gsn_disguise) && argument[0] != '\0')
    {
	send_to_char("You already have a disguise!\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
      if (is_affected(ch, gsn_disguise))
      {
        affect_strip(ch, gsn_disguise);
        act("You tear off your disguise.", ch, NULL, NULL, TO_CHAR);
        act("$n tears off $s disguise.", ch, NULL, NULL, TO_ROOM);
        return;
      }
      else
      {
        send_to_char("Disguise yourself as whom?\n\r", ch);
	return;
      }
    }

    if (IS_OAFFECTED(ch, AFF_DISGUISE))
    {
	send_to_char("Your true appearance is already concealed.\n\r", ch);
	return;
    }

	if ((victim = get_char_room(ch, argument)) == NULL)
	{
	  send_to_char("You don't see them here.\n\r", ch);
	  return;
	}

	if (IS_OAFFECTED(victim, AFF_DOPPEL) && victim->memfocus[DOPPEL_SLOT])
	    victim = victim->memfocus[DOPPEL_SLOT];

	if (!IS_NPC(victim)
	 || (victim->pIndexData->vnum == MOB_VNUM_MIRROR_IMAGE)) 
	{
	  send_to_char("They're too noticable for a disguise.\n\r", ch);
	  return;
	}

	if (IS_SET(victim->act, ACT_ANIMAL))
	{
	  send_to_char("You wouldn't be very convincing.\n\r", ch);
	  return;
	}

	if (victim->race != global_int_race_human && victim->race != ch->race)
	{
	  send_to_char("You wouldn't be very convincing.\n\r", ch);
	  return;
	}


        if (number_percent() > chance)
        {
          check_improve(ch,victim,gsn_disguise,FALSE,4);
	  send_to_char("You failed to appropriately disguise yourself.\n\r", ch);
	  return;
	}
	else
	{
	  sprintf(buf, "You disguise yourself to look like %s.\n\r", victim->short_descr);
	  send_to_char(buf, ch);
          check_improve(ch,victim,gsn_disguise,TRUE,4);
	}

	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_disguise].beats));

	if (ch->orig_long[0] == '\0' && ch->long_descr[0] != '\0')
	{
	    free_string(ch->orig_long);
	    ch->orig_long = str_dup(ch->long_descr);
	}
	free_string(ch->long_descr);
    ch->long_descr = str_dup(victim->long_descr);
    setFakeName(*ch, victim->name);
	if (ch->orig_short[0] == '\0' && ch->short_descr[0] != '\0')
	{
	    free_string(ch->orig_short);
	    ch->orig_short = str_dup(ch->short_descr);
	}	
	free_string(ch->short_descr);
        ch->short_descr = str_dup(victim->short_descr);
	if (ch->orig_description[0] == '\0')
	{
	    free_string(ch->orig_description);
            ch->orig_description = str_dup(ch->description);
	}
        free_string(ch->description);
        ch->description = str_dup(victim->description);

	one_argument(victim->name, buf);
	sprintf(buf, "%s%ld", buf, ch->id);
    setUniqueName(*ch, buf);

	ch->fake_race = victim->race;

        af.where     = TO_OAFFECTS;
        af.type      = gsn_disguise;
        af.level     = ch->level;
        af.duration  = ch->level/2;
	af.point = NULL;
        af.location  = 0;
        af.modifier  = 0;
        af.bitvector = AFF_DISGUISE;
        affect_to_char(ch, &af);
}

void do_disbelieve(CHAR_DATA *ch, char *argument)
{
    // Check for target
    CHAR_DATA *victim(get_char_room(ch, argument));
    if (victim == NULL)
	{
    	send_to_char("Disbelieve what?\n\r", ch);
	    return;
	}

    // Basic checks
    if (!IS_NPC(victim) || !IS_SET(victim->act, ACT_ILLUSION) || (victim->master != NULL && !IS_PK(ch, victim->master)))
    {
    	act("You wish and wish, but $N doesn't go away.", ch, NULL, victim, TO_CHAR);
	    return;
    }

    // Check mana
    int manaCost(victim->level);
    if (ch->mana < manaCost)
    {
        act("You are too exhausted to decide whether $N is real right now.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (tryDisbelieve(ch, victim))
        expend_mana(ch, manaCost);
}

bool tryDisbelieve(CHAR_DATA * ch, CHAR_DATA * victim)
{
    // Paranoia is automatic failure
    if (is_affected(ch, gsn_paranoia))
    {
        send_to_char("You are so paranoid right now you don't know what to believe!\n", ch);
        return false;
    }

    // Apply lag from the attempt, then begin to prepare odds of success
    WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));
    int chance(20);

    // If the phantasm lacks the Plausible trait, check for out of area 
    if (PhantasmInfo::traitCount(*victim, PhantasmTrait::Plausible) == 0)
    {
        // No plausible trait, so if out of area add 10% to chance
        const MOB_INDEX_DATA * mobIndex(PhantasmInfo::baseMobIndex(*victim));
        if (victim->in_room == NULL || mobIndex == NULL || victim->in_room->area != mobIndex->area)
            chance += 10;
    }

    // Swarming check
    if (PhantasmInfo::traitCount(*victim, PhantasmTrait::Swarming) > 0) 
        chance -= 5;

    if (victim->in_room != NULL)
    {
        // Count total number of phantasms controlled by this caster in this room
        for (CHAR_DATA * phantasm(victim->in_room->people); phantasm != NULL; phantasm = phantasm->next_in_room)
        {
            if (victim != phantasm && IS_NPC(phantasm) && IS_SET(phantasm->act, ACT_ILLUSION) && phantasm->master == victim->master)
            {
                // Found a phantasm controlled by the same caster, check for the swarming trait
                if (PhantasmInfo::traitCount(*phantasm, PhantasmTrait::Swarming) == 0)
                    chance += 10;
                else
                    chance += 5;
            }
        }

        // Check for natureborn and cityborn
        switch (victim->in_room->sector_type)
        {
            // Civilization sectors
            case SECT_CITY:
            case SECT_INSIDE:
                if (PhantasmInfo::traitCount(*victim, PhantasmTrait::Natureborn)) chance += 10;
                if (PhantasmInfo::traitCount(*victim, PhantasmTrait::Cityborn)) chance -= 10;
                break;
            
            // Neither nature nor civilization
            case SECT_UNUSED:
            case SECT_ROAD: 
                break;

            // Nature sectors
            default:
                if (PhantasmInfo::traitCount(*victim, PhantasmTrait::Natureborn)) chance -= 10;
                if (PhantasmInfo::traitCount(*victim, PhantasmTrait::Cityborn)) chance += 10;
                break;
        }
    }

    // Check for mismatched race and the worldly trait
    if (PhantasmInfo::traitCount(*victim, PhantasmTrait::Worldly) > 0) chance -= 5;
    else if (victim->master != NULL && victim->race != victim->master->race) chance += 5;

    // Check for matched/mismatched auras
    if (victim->master != NULL)
    {
        int ch_mod(aura_grade(victim->master));
        int victim_mod(aura_grade(victim));
        if (ch_mod >= 2)
        {
            if (victim_mod <= -2) chance += 5;
            else if (victim_mod >= 2) chance -= 5;
        }
        else if (ch_mod <= -2)
        {
            if (victim_mod <= -2) chance -= 5;
            else if (victim_mod >= 2) chance += 5;
        }
    }

    // Check for save vs illusion; failure halves the chance of disbelief
    if (!saves_spell(victim->level, victim, ch, DAM_ILLUSION))
        chance /= 2;

    // Always at least a 5% chance
    chance = UMAX(chance, 5);
    if (number_percent() <= chance)
    {
	    act("$n realizes $N is an illusion, and banishes $M with intense concentration and disbelief!", ch, NULL, victim, TO_ROOM);
    	act("You realize $N is an illusion, and banish $M with intense concentration and disbelief!", ch, NULL, victim, TO_CHAR);
    	extract_char(victim, TRUE);
        return true;
    }

    // Failed
    act("You stare uncertainly at $N for a long moment, but $E seems real enough.", ch, NULL, victim, TO_CHAR);
    act("$n stares uncertainly at $N for a long moment.", ch, NULL, victim, TO_ROOM);

    // Check for paranoia and substantial trait
    chance = 10;
    if (PhantasmInfo::traitCount(*victim, PhantasmTrait::Substantial) > 0)
        chance = 25;

    if (saves_spell(victim->level, victim, ch, DAM_MENTAL))
        chance /= 2;

    if (number_percent() <= chance && !IS_OAFFECTED(ch, AFF_PARANOIA))
    {
        // Paranoia
        AFFECT_DATA af = {0};
        af.where     = TO_OAFFECTS;
        af.type      = gsn_paranoia;
        af.level     = victim->level;
        af.duration  = 10;
        af.bitvector = AFF_PARANOIA;
        affect_to_char(ch, &af);

        send_to_char("Your disbelief overruns you, turning rapidly into paranoia!\n", ch);
    }

    return true;
}
	
void do_showdata(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    char *string;
    bool offline=FALSE;

    string = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Syntax:\n\r",ch);
	send_to_char("  showdata <name>\n\r",ch);
        send_to_char("  showdata offline <name>\n\r",ch);
        return;
    }

    /* Added this code for showdata'ing offline characters.
     * - transmitt 10/09/05
     */
    if (!str_cmp(arg, "offline"))
    {
        if (victim = load_offline_char(string)) 
	    offline=TRUE;
	else 
	{	
	    send_to_char("Character already online or does not exist!\n\r", ch);
	    return;
	}    
    }
    
    else if ((victim = get_char_world(ch, argument)) == NULL)
    {
	send_to_char("You cannot find any such person.\n\r", ch);
	return;
    }

    if (IS_NPC(victim))
    {
	send_to_char("Not on NPCs.\n\r", ch);
	return;
    }

    if (!victim->pcdata->immrecord)
        send_to_char("No player data.\n\r", ch);
    else
        page_to_char(victim->pcdata->immrecord, ch);

    if (victim->desc && victim->desc->acct && victim->desc->acct->immrecord)
    {   
	send_to_char("\n\rAccount record:\n\r", ch);
	page_to_char(victim->desc->acct->immrecord, ch);
    }
    if (offline) 
	extract_char(victim,TRUE);
}

void do_inquire(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int chance;
    CHAR_DATA *victim;

    if ((chance = get_skill(ch, gsn_inquire)) == 0)
    {
        send_to_char( "Huh?\n\r", ch );
        return;
    }

    if (is_affected(ch, gsn_inquire))
    {
	send_to_char("You already have sent for information on someone.\n\r",ch);
        return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("Who do you wish to inquire about?\n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, argument)) == NULL)
    {
	send_to_char("Inquiries can only be made upon those currently in the realm.\n\r", ch);
	return;
    }

    if (IS_NPC(victim))
    {
	send_to_char("You doubt the records will hold any information on them.\n\r", ch);
	return;
    }

    if (coins_to_value(ch->bank) < 1000)
	if (coins_to_value(ch->coins) < 1000)
    	{
	    send_to_char("You don't have enough money to pay for that information.\n\r", ch);
	    return;
    	}

    WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));

    af.where     = TO_AFFECTS;
    af.type      = gsn_inquire;
    af.level     = ch->level;
    af.duration  = 3;
    af.point     = NULL;
    af.location  = APPLY_HIDE;
    af.bitvector = 0;

    if (number_percent() > chance)
	af.modifier  = -1;
    else
	af.modifier  = victim->id;

    affect_to_char(ch, &af);

    act("You request information on $N from the Office of Records.", ch, NULL, victim,TO_CHAR);

    return;
}

void do_contact_agents(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int chance;
    CHAR_DATA *victim;

    if ((chance = get_skill(ch, gsn_contact_agents)) == 0)
    {
        send_to_char( "Huh?\n\r", ch );
        return;
    }

    if (is_affected(ch, gsn_contact_agents))
    {
	send_to_char("You already have agents searching for someone.\n\r",ch);
        return;
    }
    if (argument[0] == '\0')
    {
	send_to_char("Who did you want the agents to search for?\n\r",ch);
	return;
    }

    if ((victim = get_char_world(ch, argument)) == NULL)
    {
	send_to_char("Your agents don't seem to think they can find any such person.\n\r", ch);
	return;
    }

    if (coins_to_value(ch->bank) < 4000)
	if (coins_to_value(ch->coins) < 4000)
    	{
	    send_to_char("You don't have enough money to get your agents searching.\n\r", ch);
	    return;
    	}

    if (IS_NPC(victim))
    {
	send_to_char("You contact your agents, but they don't think the person you ask about is notorious enough to find.\n\r", ch);
	return;
    }
    
    act("You call upon the Office of Records to locate $N's whereabouts.", ch,NULL,victim,TO_CHAR );

    if (number_percent() > get_skill(ch, gsn_contact_agents))
    {
	af.where     = TO_AFFECTS;
	af.type      = gsn_contact_agents;
	af.level     = ch->level;
	af.duration  = 2 + dice(1,3);
	af.point = NULL;
	af.location  = APPLY_HIDE;
	af.modifier  = -1;
	af.bitvector = 0;
	affect_to_char( ch, &af );
	return;
    }

    WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));

    af.where     = TO_AFFECTS;
    af.type      = gsn_contact_agents;
    af.level     = ch->level;
    af.duration  = 2 + dice(1,3);
    af.point     = (void *) victim;
    af.location  = APPLY_HIDE;
    if (victim->in_room->clan != 0
      || !strcmp(victim->in_room->area->name,"The Temple of the Pegasus")
      || !strcmp(victim->in_room->area->name,"The Tempestuous Temple")
      || !strcmp(victim->in_room->area->name,"The Demesnes of Gods")
      || !strcmp(victim->in_room->area->name,"Limbo")
      || !strcmp(victim->in_room->area->name,"Generic Quest Area")
      || !strcmp(victim->in_room->area->name,"The Shrine of Xiganath")
      || !strcmp(victim->in_room->area->name,"Temple of the Crown")
      || !strcmp(victim->in_room->area->name,"The Shrine of the Rose")
      || !strcmp(victim->in_room->area->name,"The Caliphate of Dullaek Pak")
      || !strcmp(victim->in_room->area->name,"Shrine of Rveyelhi")
      || !strcmp(victim->in_room->area->name,"Mines of Gogoth")
      || !strcmp(victim->in_room->area->name,"Dream Lands"))
        af.modifier = -1;
    else
        af.modifier = victim->in_room->vnum;
    af.bitvector = 0;
    affect_to_char( ch, &af );

    return;
}

bool can_charm_another(CHAR_DATA *ch)
{
int count;
CHAR_DATA *vch;

count = 0;

for (vch = char_list; vch != NULL; vch = vch->next)
	{
	if ((vch->master == ch) && (IS_AFFECTED(vch, AFF_CHARM)))
		count++;
	}

return ch->level/15 > count ? TRUE : FALSE;
}


void do_nightvision(CHAR_DATA *ch, char *argument)
{
    int chance;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ((chance = get_skill(ch, gsn_nightvision)) == 0)
    {
        send_to_char( "Huh?\n\r", ch );
        return;
    }

    if ( IS_AFFECTED(ch, AFF_INFRARED) )
    {
        send_to_char("You can already see in the darkness. \n\r",ch);
        return;
    }


    if (ch->mana < skill_table[gsn_nightvision].min_mana)
    {
	send_to_char("You're too tired for that.\n\r", ch);
	return;
    }

    expend_mana(ch, skill_table[gsn_nightvision].min_mana);

    if (number_percent() < chance)
    {
        af.where     = TO_AFFECTS;
        af.type      = gsn_nightvision;
    	af.level     = ch->level;
    	af.duration  = ch->level;
	af.point = NULL;
    	af.location  = APPLY_NONE;
    	af.modifier  = 0;
    	af.bitvector = AFF_INFRARED;
    	affect_to_char( ch, &af );
    	check_improve(ch,NULL,gsn_nightvision,TRUE,4);
    	send_to_char( "You close your eyes, letting them adapt to the darkness.\n\r", ch );
    }
    else
    {
	send_to_char("You try to focus on the darkness, but fail.\n\r", ch);
	check_improve(ch,NULL,gsn_nightvision,FALSE,4);
    }

    return;
}

void do_sharp_vision(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int chance;

    if ((chance = get_skill(ch, gsn_sharp_vision)) == 0)
    {
        send_to_char( "Huh?\n\r", ch );
        return;
    }

	if (ch->mana < skill_table[gsn_sharp_vision].min_mana)
	{
	send_to_char("You're too tired for that.\n\r", ch);
	return;
	}

    if (IS_PAFFECTED(ch, AFF_SHARP_VISION))
    {
    send_to_char("You are already as alert as you can be. \n\r",ch);
        return;
    }

	expend_mana(ch, skill_table[gsn_sharp_vision].min_mana);

    if (number_percent() < chance)
 	{
    af.where     = TO_PAFFECTS;
    af.type      = gsn_sharp_vision;
    af.level     = ch->level;
    af.duration  = ch->level;
    af.point 	 = NULL;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_SHARP_VISION;
    affect_to_char( ch, &af );
    if (!IS_AFFECTED(ch, AFF_INFRARED))
    {
        af.where     = TO_AFFECTS;
	af.bitvector = AFF_INFRARED;
	affect_to_char( ch, &af);
    }
    check_improve(ch,NULL,gsn_sharp_vision,TRUE,4);
    send_to_char( "You attune yourself to watching the foliage.\n\r", ch );
    return;
	}
	else
	{
	send_to_char("You glance around suspiciously, but fail to attune yourself to the foliage.\n\r", ch);
	check_improve(ch,NULL,gsn_sharp_vision,FALSE,4);
	return;
	}
}

void do_perception(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int chance;

    if ((chance = get_skill(ch, gsn_perception)) == 0)
    {
        send_to_char( "Huh?\n\r", ch );
        return;
    }

	if (!stone_check(ch, gsn_perception))
	{
	send_to_char("You cannot draw enough power from the stones.\n\r", ch);
	return;
	}

	if (ch->mana < skill_table[gsn_perception].min_mana)
	{
	send_to_char("You're too tired for that.\n\r", ch);
	return;
	}

	chance = get_skill(ch, gsn_perception);


    if ( IS_AFFECTED(ch, AFF_DETECT_HIDDEN) && IS_AFFECTED(ch, AFF_DETECT_INVIS) )
    {
    send_to_char("You are already alert.\n\r",ch);
        return;
    }

    if (number_percent() < chance)
 	{
	expend_mana(ch, skill_table[gsn_perception].min_mana);
    af.where     = TO_AFFECTS;
    af.type      = gsn_perception;
    af.level     = ch->level;
    af.duration  = ch->level/2;
	af.point = NULL;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_DETECT_INVIS;
    affect_to_char( ch, &af );

    af.bitvector = AFF_DETECT_HIDDEN;
    affect_to_char( ch, &af );

    send_to_char( "You draw in power, and feel yourself become more alert.\n\r", ch );
	act("$n looks more aware.", ch, NULL, NULL, TO_ROOM);
	check_improve(ch,NULL,gsn_perception,TRUE,4);
    return;
	}
	else
      {
	check_improve(ch,NULL,gsn_perception,TRUE,4);
	expend_mana(ch, skill_table[gsn_perception].min_mana);
	send_to_char("You draw in power, but fail to use it correctly to heighten your perception.\n\r", ch);
	return;
      }
}

void do_sixthsense(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int chance;

    if ((chance = get_skill(ch, gsn_sixthsense)) == 0)
    {
        send_to_char( "Huh?\n\r", ch );
        return;
    }


    if (ch->mana < skill_table[gsn_sixthsense].min_mana)
    {
	send_to_char("You're too tired for that.\n\r", ch);
	return;
    }

    if ( IS_AFFECTED(ch, AFF_DETECT_INVIS) 
      || IS_AFFECTED(ch,AFF_DETECT_HIDDEN)
      || IS_PAFFECTED(ch,AFF_SHARP_VISION))
    {
	send_to_char("You are already as alert as you can be. \n\r",ch);
        return;
    }

    if (number_percent() < chance)
    {
	expend_mana(ch, skill_table[gsn_sixthsense].min_mana);
    	af.where     = TO_AFFECTS;
    	af.type      = gsn_sixthsense;
    	af.level     = ch->level;
    	af.duration  = 5 + ch->level/2;
    	af.location  = APPLY_NONE;
    	af.modifier  = 0;
    	af.bitvector = AFF_DETECT_INVIS;
    	affect_to_char( ch, &af );
	af.bitvector = AFF_DETECT_HIDDEN;
	affect_to_char(ch, &af);
	if (!IS_AFFECTED(ch,AFF_INFRARED))
	{
	    af.bitvector = AFF_INFRARED;
	    affect_to_char(ch, &af);
	}
	af.bitvector = AFF_SHARP_VISION;
	af.where     = TO_PAFFECTS;
	affect_to_char(ch, &af);
    	send_to_char( "You focus, stretching your senses to heightened alertness.\n\r", ch );
	check_improve(ch,NULL,gsn_sixthsense,TRUE,4);
    	return;
    }
    else
    {
	check_improve(ch,NULL,gsn_sixthsense,TRUE,4);
	expend_mana(ch, skill_table[gsn_sixthsense].min_mana);
	send_to_char("You try to heighten your senses, but are unable to focus.\n\r", ch);
	return;
    }
}


void do_detecthidden(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int chance;


    if ((chance = get_skill(ch, gsn_detecthidden)) == 0)
    {
        send_to_char( "Huh?\n\r", ch );
        return;
    }


    if (ch->mana < skill_table[gsn_detecthidden].min_mana)
    {
	send_to_char("You're too tired for that.\n\r", ch);
	return;
    }

    if ( IS_AFFECTED(ch, AFF_DETECT_HIDDEN) )
    {
        send_to_char("You are already as alert as you can be. \n\r",ch);
        return;
    }

    if (number_percent() < chance)
    {
	expend_mana(ch, skill_table[gsn_detecthidden].min_mana);
        af.where     = TO_AFFECTS;
        af.type      = gsn_detecthidden;
        af.level     = ch->level;
        af.duration  = ch->level;
	af.point = NULL;
        af.location  = APPLY_NONE;
        af.modifier  = 0;
        af.bitvector = AFF_DETECT_HIDDEN;
        affect_to_char( ch, &af );
	if (!IS_AFFECTED(ch, AFF_INFRARED))
	{
	    af.bitvector = AFF_INFRARED;
	    affect_to_char( ch, &af);
	}
        send_to_char( "Your awareness improves.\n\r", ch );
	check_improve(ch,NULL,gsn_detecthidden,TRUE,4);
    }
    else
    {
	expend_mana(ch, skill_table[gsn_detecthidden].min_mana);
	send_to_char("You strain your vision, but don't really feel more aware.\n\r", ch);
	check_improve(ch,NULL,gsn_detecthidden,TRUE,4);
    }

    return;
}

void do_lore(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj = get_obj_carry(ch, argument, ch);
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA *paf, *poisoned;
    int i, masked_gsn, skill = get_skill(ch, gsn_lore);
    bool tan = FALSE;

    if (argument[0] != '\0' && obj
     && (obj->pIndexData->vnum == OBJ_VNUM_HIDE_CAP
      || obj->pIndexData->vnum == OBJ_VNUM_HIDE_VEST
      || obj->pIndexData->vnum == OBJ_VNUM_HIDE_GLOVES
      || obj->pIndexData->vnum == OBJ_VNUM_HIDE_BOOTS
      || obj->pIndexData->vnum == OBJ_VNUM_HIDE_SLEEVES
      || obj->pIndexData->vnum == OBJ_VNUM_HIDE_LEGGINGS
      || obj->pIndexData->vnum == OBJ_VNUM_HIDE_BRACER
      || obj->pIndexData->vnum == OBJ_VNUM_HIDE_COLLAR
      || obj->pIndexData->vnum == OBJ_VNUM_HIDE_BELT
      || obj->pIndexData->vnum == OBJ_VNUM_HIDE_COAT
      || obj->pIndexData->vnum == OBJ_VNUM_RANGER_BOW
      || obj->pIndexData->vnum == OBJ_VNUM_RANGER_SPEAR
      || obj->pIndexData->vnum == OBJ_VNUM_RANGER_MACE
      || obj->pIndexData->vnum == OBJ_VNUM_RANGER_NET
      || obj->pIndexData->vnum == OBJ_VNUM_RANGER_ROPE
      || obj->pIndexData->vnum == OBJ_VNUM_RANGER_ARROW))
	tan = TRUE;

    if (ch->class_num == global_int_class_ranger && tan && skill > 1)
	skill = 100;

    if (skill == 0)
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    if (ch->fighting)
    {
	send_to_char("You cannot recall lore while fighting.\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("What do you wish to recall lore on?\n\r", ch);
	return;
    }

    if (!obj)
    {
	send_to_char("You are not carrying that.\n\r", ch);
	return;
    }

    act("You hold $p in your hands, and study it carefully.", ch, obj, NULL, TO_CHAR);
    act("$n holds $p in $s hands, and studies it carefully.", ch, obj, NULL, TO_ROOM);

    WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE * 2));

    if (obj->level > ch->level)
	skill = URANGE(5, skill - ((obj->level - ch->level) * 2), 95);
    else if (ch->level > obj->level)
	skill = URANGE(5, skill + ch->level - obj->level, 95);
    else
	skill = URANGE(5, skill, 95);

    sprintf(buf, "Object: '%s' is type %s.\n\rWeight is %d.%d, value is %d, level is %d\n\rMaterial is %s\n\r",
	obj->short_descr,
	item_name(obj->item_type),
	obj->weight / 10,
	obj->weight % 10,
	(number_percent() <= skill) ? obj->cost : number_range(obj->cost / 2, obj->cost * 3 / 2),
	(number_percent() <= skill) ? obj->level : number_range(obj->level / 2, obj->cost * 3 / 2),
	material_table[obj->material].name);

    send_to_char(buf, ch);

/** copied from spell_identify **/
    switch ( obj->item_type )
    {
    case ITEM_SCROLL:
    case ITEM_POTION:
    case ITEM_PILL:
    case ITEM_OIL:
     
	masked_gsn = -1;
	if ((poisoned = get_obj_affect(obj, gsn_subtlepoison)) != NULL)
		masked_gsn = poisoned->modifier;

	sprintf( buf, "Level %d spells of:", obj->value[0] );
	send_to_char( buf, ch );

	if ( obj->value[1] >= 0 && obj->value[1] < MAX_SKILL )
	{
		send_to_char( " '", ch );
		send_to_char( masked_gsn > 0 ? skill_table[masked_gsn].name : skill_table[obj->value[1]].name, ch );
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
        sprintf( buf, "Has %d charges of level %d",
            obj->value[2], obj->value[0] );
        send_to_char( buf, ch );

        if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
        {
            send_to_char( " '", ch );
            send_to_char( skill_table[obj->value[3]].name, ch );
            send_to_char( "'", ch );
        }

        send_to_char( ".\n\r", ch );
        break;

    case ITEM_ARROW:
        buf[0] = '\0';
        for (i = 2; i < 5; i++)
        {
            if (obj->value[i] > 0)
            {
                if (buf[0] != '\0')
                    sprintf(buf, "%s, %s", buf, skill_table[obj->value[i]].name);
                else
                    sprintf(buf, "Imbued with %s", skill_table[obj->value[i]].name);
            }
        }

        if (buf[0] != '\0')
        {
            sprintf(buf, "%s.\n\r", buf);
            send_to_char(buf, ch);
        }
        break;

    case ITEM_DRINK_CON:
        sprintf(buf,"It holds %s-colored %s.\n\r",
            liq_table[obj->value[2]].liq_color,
            liq_table[obj->value[2]].liq_name);
        send_to_char(buf,ch);
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

    case ITEM_WEAPON:
        send_to_char("Weapon type is ",ch);
        switch (obj->value[0])
        {
            case(WEAPON_KNIFE)  : send_to_char("knife.\n\r",ch);        break;
            case(WEAPON_STAFF)  : send_to_char("staff.\n\r",ch);        break;
            case(WEAPON_EXOTIC) : send_to_char("exotic.\n\r",ch);       break;
            case(WEAPON_SWORD)  : send_to_char("sword.\n\r",ch);        break;
            case(WEAPON_DAGGER) : send_to_char("dagger.\n\r",ch);       break;
            case(WEAPON_SPEAR)  : send_to_char("spear.\n\r",ch);	break;
            case(WEAPON_MACE)   : send_to_char("mace/club.\n\r",ch);    break;
            case(WEAPON_AXE)    : send_to_char("axe.\n\r",ch);          break;
            case(WEAPON_FLAIL)  : send_to_char("flail.\n\r",ch);        break;
            case(WEAPON_WHIP)   : send_to_char("whip.\n\r",ch);         break;
            case(WEAPON_POLEARM): send_to_char("polearm.\n\r",ch);      break;
            default             : send_to_char("unknown.\n\r",ch);      break;
        }
        if (obj->pIndexData->new_format)
            sprintf(buf,"Damage is %dd%d (average %d).\n\r",
                obj->value[1],obj->value[2],
                (1 + obj->value[2]) * obj->value[1] / 2);
        else
            sprintf( buf, "Damage is %d to %d (average %d).\n\r",
                obj->value[1], obj->value[2],
                ( obj->value[1] + obj->value[2] ) / 2 );
        send_to_char( buf, ch );
        if (obj->value[4])  /* weapon flags */
        {
            sprintf(buf,"Weapons flags: %s\n\r",weapon_bit_name(obj->value[4]));            send_to_char(buf,ch);
        }
        break;

    case ITEM_BOW:
        if (obj->pIndexData->new_format)
            sprintf(buf,"Damage is %dd%d (average %d).\n\r",
                obj->value[1],obj->value[2],
                (1 + obj->value[2]) * obj->value[1] / 2);
        else
            sprintf( buf, "Damage is %d to %d (average %d).\n\r",
                obj->value[1], obj->value[2],
                ( obj->value[1] + obj->value[2] ) / 2 );
        send_to_char(buf, ch);
        break;

    case ITEM_ARMOR:
        sprintf( buf,
        "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic.\n\r",
            obj->value[0], obj->value[1], obj->value[2], obj->value[3] );
        send_to_char( buf, ch );
        break;
    }

    if (!obj->enchanted)
    {
// brazen: Rangers can see the affects on their tans, but not on anything else.
// Bards can see base item affects, but not further enchantments (since those
// wouldn't be in recorded history)
	if (ch->class_num == global_int_class_ranger)
	    if (tan)
	        paf = obj->affected;
	    else
	        paf = NULL;
	else
	    paf = obj->pIndexData->affected;

        for ( ; paf != NULL; paf = paf->next )
        {
            if ( paf->location != APPLY_NONE && paf->location != APPLY_HIDE && paf->modifier != 0 )
            {
                sprintf( buf, "%s wearer's %s.\n\r",
                  (paf->modifier > 0) ? "Increases" :
	          (paf->modifier < 0) ? "Decreases" : "Does not affect",
		  affect_loc_name( paf->location ));
                send_to_char(buf,ch);
                if (paf->bitvector)
                {
                    switch(paf->where)
                    {
                    case TO_AFFECTS:
                        sprintf(buf,"Adds %s affect.\n",
                            affect_bit_name(paf));
                        break;
                    case TO_OBJECT: sprintf(buf,"Adds %s object flag.\n", extra_bit_name(paf->bitvector, 0)); break;
                    case TO_OBJECT1: sprintf(buf,"Adds %s object flag.\n", extra_bit_name(paf->bitvector, 1)); break;
                    case TO_OBJECT2: sprintf(buf,"Adds %s object flag.\n", extra_bit_name(paf->bitvector, 2)); break;
					
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
                send_to_char( buf, ch );
                }
            }
        }
    }

    if (obj->lore && (obj->lore[0] != '\0'))
    {
	if (tan)
	{
	    if (ch->class_num == global_int_class_ranger)
	    {
		sprintf(buf, "%s\n\r", obj->lore);
		send_to_char(buf, ch);
	    }
	}
	else
	{
	    if (ch->class_num != global_int_class_ranger)
	    {
		sprintf(buf, "Lore:\n\r%s\n\r", obj->lore);
	        send_to_char(buf, ch);
	    }
	}
    }
    else if (obj->pIndexData->lore && (obj->pIndexData->lore[0] != '\0')
      && ch->class_num != global_int_class_ranger)
    {
	sprintf(buf, "Lore:\n\r%s\n\r", obj->pIndexData->lore);
	send_to_char(buf, ch);
    }

    check_improve(ch, NULL, gsn_lore, TRUE, 2);

    return;
}

char *format_obj_to_char( OBJ_DATA *obj, CHAR_DATA *ch, bool fShort )
{
    static char buf[MAX_STRING_LENGTH];

    buf[0] = '\0';

    if ((fShort && (obj->short_descr == NULL || obj->short_descr[0] == '\0'))
    ||  (obj->description == NULL || obj->description[0] == '\0'))
    	return buf;

    // Show any runes on the object
    Runes::ShowObjectRunes(*ch, *obj, buf);
 
    if ( IS_OBJ_STAT(obj, ITEM_NOLONG)    )   strcat( buf, "(nolong) "    );
    if ( IS_OBJ_STAT(obj, ITEM_WIZI)      )   strcat( buf, "(wizi) "      );
    if ( IS_OBJ_STAT(obj, ITEM_HIDDEN)    )   strcat( buf, "(Concealed) " );
    if ( IS_OBJ_STAT(obj, ITEM_STASHED)   )   strcat( buf, "(stashed) "   );
    if ( IS_OBJ_STAT(obj, ITEM_INVIS)     )   strcat( buf, "(Invis) "     );

    if (IS_OBJ_STAT(obj, ITEM_EVIL))
    {
        bool showEvil(IS_AFFECTED(ch, AFF_DETECT_EVIL));
        if (number_percent() <= get_skill(ch, gsn_discerniniquity))
        {
            showEvil = true;
            check_improve(ch, NULL, gsn_discerniniquity, true, 12);
        }
        
        if (showEvil)
            strcat( buf, "(Red Aura) "  );
    }

    if (IS_AFFECTED(ch, AFF_DETECT_GOOD)
    &&  IS_OBJ_STAT(obj,ITEM_BLESS))	      strcat( buf, "(Blue Aura) " );

    if (IS_OBJ_STAT(obj, ITEM_MAGIC))
    {
        bool showMagic(is_affected(ch, gsn_detect_magic) || is_an_avatar(ch));
        if (number_percent() <= get_modifier(ch->affected, gsn_weavesense))
        {
            showMagic = true;
            check_improve(ch, NULL, gsn_weavesense, TRUE, 4);
        }
        if (showMagic) strcat( buf, "(Magical) "   );
    }

    // Shroudsight shows possessed objects
    if (obj_is_affected(obj, gsn_demon_bind) && number_percent() <= get_skill(ch, gsn_shroudsight))
    {
        check_improve(ch, NULL, gsn_shroudsight, true, 8);
        strcat(buf, "(Bound) ");
    }

    // Disillusionment shows illusory objects
    if (number_percent() <= get_skill(ch, gsn_disillusionment))
    {
        bool isIllusion(false);
        bool isObscured(false);
        if (obj->pIndexData->vnum == OBJ_VNUM_CRAFTILLUSION) 
            isIllusion = true;
        else
        {
            AFFECT_DATA * illusion(get_obj_affect(obj, gsn_illusion));
            if (illusion != NULL)
            {
                isIllusion = true;
                isObscured = (illusion->point != NULL);
            }
        }

        if (isIllusion)
        {
            check_improve(ch, NULL, gsn_disillusionment, true, 16);
            if (isObscured) strcat(buf, "(Obscured) ");
            else strcat(buf, "(Illusory) ");
        }
    }

    if ( IS_OBJ_STAT(obj, ITEM_GLOW)      )   strcat( buf, "(Glowing) "   );
    if ( IS_OBJ_STAT(obj, ITEM_HUM)       )   strcat( buf, "(Humming) "   );

    if (fShort)
    {
        if (obj->short_descr != NULL)
            strcat(buf, obj->short_descr);
    }
    else if (obj_is_affected(obj, gsn_dispatchlodestone) && obj->short_descr != NULL)
    {
        std::string lodeDesc;
        lodeDesc += obj->short_descr;
        lodeDesc += " is here, hovering in the air as it gently spins.";
        lodeDesc[0] = UPPER(lodeDesc[0]);
        strcat(buf, lodeDesc.c_str());
    }
    else
    {
        if (obj->description != NULL)
            strcat(buf, obj->description);
    }

    return buf;
}

/*
 * Show a list to a character.
 * Can coalesce duplicated items.
 */
void show_list_to_char( OBJ_DATA *list, CHAR_DATA *ch, bool fShort, bool fShowNothing )
{
    char buf[MAX_STRING_LENGTH];
    BUFFER *output;
    char **prgpstrShow;
    int *prgnShow;
    char *pstrShow;
    OBJ_DATA *obj;
    int nShow;
    int iShow;
    int count;
    bool fCombine;

    if ( ch->desc == NULL )
	return;

    /*
     * Alloc space for output lines.
     */
    output = new_buf();

    count = 0;
    for ( obj = list; obj != NULL; obj = obj->next_content )
		count++;
    prgpstrShow	= (char**) malloc( count * sizeof(char *) );
    prgnShow    = (int*) malloc( count * sizeof(int)    );
    nShow	= 0;

    /*
     * Format the list of objects.
     */
    for ( obj = list; obj != NULL; obj = obj->next_content )
    { 
//	if ( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj ) && (!IS_OBJ_STAT(obj, ITEM_NOLONG) || (!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT)))) 

	if ( !obj->worn_on && can_see_obj( ch, obj ) && (!IS_OBJ_STAT(obj, ITEM_NOLONG) || (!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT)))) 
	{
	    pstrShow = format_obj_to_char( obj, ch, fShort );

	    fCombine = FALSE;

	    if ( IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE) )
	    {
		/*
		 * Look for duplicates, case sensitive.
		 * Matches tend to be near end so run loop backwords.
		 */
		for ( iShow = nShow - 1; iShow >= 0; iShow-- )
		{
		    if ( !strcmp( prgpstrShow[iShow], pstrShow ) )
		    {
			prgnShow[iShow]++;
			fCombine = TRUE;
			break;
		    }
		}
	    }

	    /*
	     * Couldn't combine, or didn't want to.
	     */
	    if ( !fCombine )
	    {
		prgpstrShow [nShow] = str_dup( pstrShow );
		prgnShow    [nShow] = 1;
		nShow++;
	    }
	}
    }

    /*
     * Output the formatted list.
     */
    for ( iShow = 0; iShow < nShow; iShow++ )
    {
	if (prgpstrShow[iShow][0] == '\0')
	{
	    free_string(prgpstrShow[iShow]);
	    continue;
	}

	if ( IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE) )
	{
	    if ( prgnShow[iShow] != 1 )
	    {
		sprintf( buf, "(%2d) ", prgnShow[iShow] );
		add_buf(output,buf);
	    }
	    else
	    {
		add_buf(output,"     ");
	    }
	}
	add_buf(output,prgpstrShow[iShow]);
	add_buf(output,"\n\r");
	free_string( prgpstrShow[iShow] );
    }

    if ( fShowNothing && nShow == 0 )
    {
	if ( IS_NPC(ch) || IS_SET(ch->comm, COMM_COMBINE) )
	    send_to_char( "     ", ch );
	send_to_char( "Nothing.\n\r", ch );
    }
    page_to_char(buf_string(output),ch);

    /*
     * Clean up.
     */
    free_buf(output);
    free(prgpstrShow);
    free(prgnShow);

    return;
}


void get_char_to_char_long(char *buf, CHAR_DATA *victim, CHAR_DATA *ch)
{
    CHAR_DATA *avict; /* apparent victim -- for doppel */
    char message[MAX_STRING_LENGTH];

    if (IS_OAFFECTED(victim, AFF_DOPPEL) && victim->memfocus[DOPPEL_SLOT])
	avict = victim->memfocus[DOPPEL_SLOT];
    else
	avict = victim;

    if (is_affected(avict, gsn_bearform))
    {
        sprintf(buf, "%sThe ursine form of %s is here.\n\r", buf, PERS(avict, ch));
        return;
    }

    if (is_affected(avict, gsn_greatertentaclegrowth))
    {
	    sprintf(buf, "%sThe tentacled form of %s is here.\n\r", buf, PERS(avict, ch));
	    return;
    }

    if (is_affected(avict, gsn_aviancurse))
    {
	    sprintf(buf, "%sA large hen is here.\n\r", buf);
	    return;
    }
    
    if (is_affected(avict, gsn_wolfform))
    {
        sprintf(buf, "%sThe lupine form of %s is here.\n\r", buf, PERS(avict, ch));
        return;
    }

    if (is_affected(avict, gsn_hawkform))
    {
	sprintf(buf, "%sThe avian form of %s is here.\n\r", buf, PERS(avict, ch));
	return;
    }

    if (is_affected(avict, gsn_lycanthropy))
    {
	sprintf(buf, "%sA large, ferocious wolf is here.\n\r", buf);
	return;
    }

    if (is_affected(avict, gsn_beastform))
    {
	sprintf(buf,"%sThe beastly form of %s is here.\n\r",buf, PERS(avict, ch));
	return;
    }

    if (is_affected(avict, gsn_gaseousform))
    {
        sprintf(buf, "%sThe hazy, diffuse form of %s flows about here.\n", buf, PERS(avict, ch));
        return;
    }

    // Check for masquerade
    if (avict->in_room != NULL)
    {
        AFFECT_DATA * masquerade(get_area_affect(avict->in_room->area, gsn_masquerade));
        if (masquerade != NULL)
        {
            if (masquerade->modifier == ch->id || number_percent() <= get_skill(ch, gsn_disillusionment))
            {
                sprintf(buf, "%sThe hazy image of a skull is superimposed on %s here.\n", buf, PERS(avict, ch));
                return;
            }

            sprintf(buf, "%sA grinning skull wreathed in crackling lightning looms here.\n", buf);
            return;
        }
    }

    // Check for gravebeat
    AFFECT_DATA * gravebeat(get_affect(avict, gsn_gravebeat));
    if (gravebeat != NULL && gravebeat->modifier != 0)
    {
        sprintf(buf, "%sShuffling steadily, %s shambles along here.\n", buf, PERS(avict, ch));
        return;
    }

    if ((victim->position == avict->start_pos) && avict->long_descr[0] != '\0')
    {
        strcat(buf, avict->long_descr);
        return;
    }

    strcat(buf, PERS(avict, ch));
    
    if (!IS_NPC(avict) || is_affected(avict, gsn_bilocation))
    {
        strcat(buf, " the ");
        strcat(buf, race_table[(IS_OAFFECTED(avict, AFF_DISGUISE) && avict->fake_race >= 0) ? avict->fake_race : avict->race].name);
    }

    switch ( avict->position )
    {
    case POS_DEAD:     strcat( buf, " is DEAD!!" );              break;
    case POS_MORTAL:   strcat( buf, " is mortally wounded." );   break;
    case POS_INCAP:    strcat( buf, " is incapacitated." );      break;
    case POS_STUNNED:  strcat( buf, " is lying here stunned." ); break;
    case POS_SLEEPING: 
	if (avict->on != NULL)
	{
	    if (IS_SET(avict->on->value[2],SLEEP_AT))
  	    {
		sprintf(message," is sleeping at %s.",
		    avict->on->short_descr);
		strcat(buf,message);
	    }
	    else if (IS_SET(avict->on->value[2],SLEEP_ON))
	    {
		sprintf(message," is sleeping on %s.",
		    avict->on->short_descr); 
		strcat(buf,message);
	    }
	    else
	    {
		sprintf(message, " is sleeping in %s.",
		    avict->on->short_descr);
		strcat(buf,message);
	    }
	}
	else 
	    strcat(buf," is sleeping here.");
	break;
    case POS_RESTING:  
        if (avict->on != NULL)
	{
            if (IS_SET(avict->on->value[2],REST_AT))
            {
                sprintf(message," is resting at %s.",
                    avict->on->short_descr);
                strcat(buf,message);
            }
            else if (IS_SET(avict->on->value[2],REST_ON))
            {
                sprintf(message," is resting on %s.",
                    avict->on->short_descr);
                strcat(buf,message);
            }
            else 
            {
                sprintf(message, " is resting in %s.",
                    avict->on->short_descr);
                strcat(buf,message);
            }
	}
        else
	    strcat( buf, " is resting here." );       
	break;
    case POS_SITTING:  
        if (avict->on != NULL)
        {
            if (IS_SET(avict->on->value[2],SIT_AT))
            {
                sprintf(message," is sitting at %s.",
                    avict->on->short_descr);
                strcat(buf,message);
            }
            else if (IS_SET(avict->on->value[2],SIT_ON))
            {
                sprintf(message," is sitting on %s.",
                    avict->on->short_descr);
                strcat(buf,message);
            }
            else
            {
                sprintf(message, " is sitting in %s.",
                    avict->on->short_descr);
                strcat(buf,message);
            }
        }
        else
	    strcat(buf, " is sitting here.");
	break;
    case POS_STANDING: 
	if (avict->on != NULL)
	{
	    if (IS_SET(avict->on->value[2],STAND_AT))
	    {
		sprintf(message," is standing at %s.",
		    avict->on->short_descr);
		strcat(buf,message);
	    }
	    else if (IS_SET(avict->on->value[2],STAND_ON))
	    {
		sprintf(message," is standing on %s.",
		   avict->on->short_descr);
		strcat(buf,message);
	    }
	    else
	    {
		sprintf(message," is standing in %s.",
		    avict->on->short_descr);
		strcat(buf,message);
	    }
	}
	else if (IS_OAFFECTED(avict, AFF_COVEN))
	{
	    if (is_flying(avict))
		strcat(buf, " is flying here, chanting softly.");
	    else
		strcat(buf, " is here, chanting softly." );
	}
	else
	{
	    if (is_flying(avict))
		strcat(buf, " is flying here.");
	    else
		strcat(buf, " is here." );
	}
	break;
    case POS_FIGHTING:
	if (is_flying(avict))
	    strcat( buf, " is flying here, fighting " );
	else
	    strcat( buf, " is here, fighting " );
	if ( avict->fighting == NULL )
	    strcat( buf, "thin air??" );
	else if ( avict->fighting == ch )
	    strcat( buf, "YOU!" );
	else if ( avict->in_room == avict->fighting->in_room )
	{
	    strcat( buf, PERS( avict->fighting, ch ) );
	    strcat( buf, "." );
	}
	else
	    strcat( buf, "someone who left?" );
	break;
    }

    if (avict->pose != NULL || is_affected(avict,gsn_entangle))
    {
        unsigned int p = 0;
        for (p=0;p < strlen(buf);p++)
        {
            if((buf[p] == '.') || (buf[p] == '?') || (buf[p] == '!'))
                break;
        }

        buf[p] = '\0';
        strcat(buf,", ");
        if (is_affected(avict,gsn_entangle))
            strcat(buf,"entangled in a net!");
        else
            strcat(buf,avict->pose);
    }

    strcat( buf, "\n\r" );
    buf[0] = UPPER(buf[0]);

    return;
}



void show_char_to_char_0( CHAR_DATA *victim, CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH];
    char nbuf[MAX_STRING_LENGTH];
    CHAR_DATA *sch;

    if (IS_NPC(victim)
      && (victim->pIndexData->vnum == MOB_VNUM_MIRROR_IMAGE)
      && victim->leader)
	sch = victim->leader;
    else if (IS_OAFFECTED(victim, AFF_DOPPEL)
      && victim->memfocus[DOPPEL_SLOT])
	sch = victim->memfocus[DOPPEL_SLOT];
    else 
	sch = victim;
    if (!can_see(ch,sch))
	return;

    buf[0] = '\0';

    if (IS_OAFFECTED(sch, AFF_GHOST)       ) strcat( buf, "(Ghost) "	);
    if ( IS_SET(sch->comm,COMM_AFK	  )   ) strcat( buf, "[AFK] "	     );
    if ( IS_AFFECTED(sch, AFF_INVISIBLE)   ) strcat( buf, "(Invis) "      );
    if ( sch->invis_level >= LEVEL_HERO    ) strcat( buf, "(Wizi) "	     );
#ifdef HERODAY
    if(!IS_NPC(sch) && IS_SET(sch->in_room->room_flags,ROOM_ARENA))
    if (!BIT_GET(sch->pcdata->bitptr,18653))
        strcat(buf, "(Spectator) ");
    else
        strcat(buf, "(Participant) ");
#endif
    if ( IS_AFFECTED(sch, AFF_WIZI) && IS_NPC(sch)) strcat( buf, "(Wizi) ");
    if (is_an_avatar(sch) && sch->position != POS_STANDING)	strcat( buf, "(Avatar) ");

    if ( IS_AFFECTED(sch, AFF_HIDE)        ) strcat( buf, "(Hide) "       );
    if ( is_affected(sch, gsn_camouflage)  ) strcat( buf, "(Camouflaged) ");
    if ( IS_PAFFECTED(sch, AFF_VOIDWALK)   ) strcat( buf, "(Void) ");
    if ( IS_OAFFECTED(sch, AFF_SHADOWMASTERY) ) strcat(buf, "(Shadowed) ");

    // Shroudsight shows possessions
    if (((is_affected(sch, gsn_possession) && sch->desc) || IS_OAFFECTED(sch, AFF_DEMONPOS)) && number_percent() <= get_skill(ch, gsn_shroudsight))
    {
        check_improve(ch, sch, gsn_shroudsight, true, 8);
        strcat(buf, "(Possessed) ");
    }

    if ( IS_AFFECTED(sch, AFF_CHARM))
    {
	if (is_affected(sch, gsn_demoniccontrol))
	    strcat(buf, "(Bound) ");
	else
	    strcat(buf, "(Charmed) ");
    }

    if ( IS_AFFECTED(sch, AFF_PASS_DOOR)   ) strcat( buf, "(Translucent) ");

    if (IS_AFFECTED(sch, AFF_FAERIE_FIRE)) 
    {
        if (is_affected(sch, gsn_starglamour))
	        strcat(buf, "(Pale Glow) ");
    	else if (is_affected(sch, gsn_faerie_fog))
 	        strcat(buf, "(Purple Glow) ");
        else if (is_affected(sch, gsn_clingingfog))
            strcat(buf, "(Sparkling) ");
        else
            strcat(buf, "(Orange Aura) "  );
    }

    int mod = aura_grade(sch);
	if (mod > 0)
    {
        bool show(IS_AFFECTED(ch, AFF_DETECT_EVIL));
        if (number_percent() <= get_skill(ch, gsn_discerniniquity))
        {
            show = true;
            check_improve(ch, sch, gsn_discerniniquity, true, 12);
        }

        if (show)
        {
	        switch(mod)
    	    {
	            case 1: strcat(buf, "(Faint Red Aura) "); break;
        		case 2: strcat(buf, "(Red Aura) "); break;
	            case 3: strcat(buf, "(Dark Red Aura) "); break;
        		case 4: strcat(buf, "(Black Aura) "); break;
	        }
        }
    }
    
    if (IS_AFFECTED(ch, AFF_DETECT_GOOD))
	if (mod < 0)
	    switch(mod)
	    {
	        case -1: strcat(buf, "(Pale Gold Aura) "); break;
	        case -2: strcat(buf, "(Gold Aura) "); break;
	        case -3: strcat(buf, "(Bright Gold Aura) "); break;
	        case -4: strcat(buf, "(Silver Aura) "); break;
            }

    if (is_affected(sch, gsn_flameshield)) strcat(buf, "(Flaming) ");
    if (is_affected(sch, gsn_blur)) strcat(buf, "(Blur) ");
    if (is_affected(sch, gsn_conduitoftheskies)) strcat(buf, "(Sparking) ");
    if (is_affected(sch, gsn_shadowfiend)) strcat(buf, "(Wraith) ");
    if (is_affected(sch, gsn_fadeshroud)) strcat(buf, "(Faded) ");
    if (has_miasmaofwaning(sch) >= 0) strcat(buf, "(Miasma) ");
	
    if (IS_NAFFECTED(sch, AFF_FLESHTOSTONE))	strcat(buf, "(Stone) "      );
    if (IS_OAFFECTED(sch, AFF_ENCASE))	strcat(buf, "(Encased) "    );
    if (IS_AFFECTED(sch, AFF_SANCTUARY))	strcat(buf, "(White Aura) " );

    AFFECT_DATA * aspectAff(get_affect(sch, gsn_aspectoftheinferno));
    if (aspectAff != NULL && aspectAff->modifier == 0) strcat(buf, "(Blazing) ");
    else if (number_percent() < (get_skill(sch, gsn_flameheart) * 2 - 100)) strcat(buf, "(Fiery Aura) ");

    // Disillusionment shows illusory mobs
    bool illusory(false);
    if (IS_NPC(sch) && ((sch->pIndexData->vnum == MOB_VNUM_WEAVEILLUSION && is_affected(sch, gsn_illusion)) || sch->pIndexData->vnum == MOB_VNUM_PYROKINETICMIRROR))
        illusory  = true;
    if (IS_NPC(victim) && victim->pIndexData->vnum == MOB_VNUM_MIRROR_IMAGE) illusory = true;
    
    if (illusory && number_percent() <= get_skill(ch, gsn_disillusionment))
    {
        check_improve(ch, NULL, gsn_disillusionment, true, 16);
        strcat(buf, "(Illusion) ");
    }

    if (IS_NPC(sch) && IS_SET(sch->nact, ACT_SHADE)) strcat(buf, "(Shade) ");
    strcat(buf, checkDiagnoseTags(ch, sch).c_str());

    if (is_affected(ch,gsn_shurangaze))
	if (sch->hit*100/sch->max_hit < 34)
	    strcat(buf,"(Near Death) ");
	else if (sch->hit*100/sch->max_hit < 76)
	    strcat(buf,"(Injured) ");
    if (!IS_NPC(sch) 
      && IS_SET(sch->act, PLR_REWARD))	strcat(buf, "(REWARD) "     );

    bool showDisguise(false);
    if ( !IS_NPC(sch) && ch->trust > 52
     && (is_affected(sch, gsn_chameleon)
      || is_affected(sch, gsn_alterself)
      || is_affected(sch, gsn_rearrange)
      || is_affected(sch, gsn_disguise)
      || is_affected(sch, gsn_hooded)))
        showDisguise = true;

    if (is_affected(sch, gsn_englamour) && (ch->trust > 52 || number_percent() <= get_skill(ch, gsn_disillusionment)))
        showDisguise = true;

    if (showDisguise)
    {
        sprintf(nbuf, "<%s>", (IS_NPC(sch) ? sch->orig_short : sch->name));
        strcat(buf, nbuf);
    }

    get_char_to_char_long(buf, sch, ch);
    send_to_char( buf, ch );
}

void show_char_status( CHAR_DATA *victim, CHAR_DATA *ch)
{
    int percent;
    char buf[MAX_STRING_LENGTH];

    if ( victim->max_hit > 0 )
	percent = ( 100 * victim->hit ) / victim->max_hit;
    else
	percent = -1;

    strcpy( buf, PERS(victim, ch) );

    if (percent >= 100) 
	strcat( buf, " is in excellent condition.\n\r");
    else if (percent >= 90) 
	strcat( buf, " has a few scratches.\n\r");
    else if (percent >= 75) 
	strcat( buf," has some small wounds and bruises.\n\r");
    else if (percent >=  50) 
	strcat( buf, " has quite a few wounds.\n\r");
    else if (percent >= 30)
	strcat( buf, " has some big nasty wounds and scratches.\n\r");
    else if (percent >= 15)
	strcat ( buf, " looks pretty hurt.\n\r");
    else if (percent >= 0 )
	strcat (buf, " is in awful condition.\n\r");
    else
	strcat(buf, " is bleeding to death.\n\r");

    buf[0] = UPPER(buf[0]);
    send_to_char( buf, ch );

    return;
}

void show_char_to_char_1( CHAR_DATA *victim, CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH];
    bool found;
    CHAR_DATA *sch, *vch;

    bool scrying(is_affected(ch, gsn_scry));
    if (!scrying && victim && victim->in_room)
    {
        for (vch = victim->in_room->people; vch; vch = vch->next_in_room)
	    if ((vch != ch) && can_see(vch, ch))
        {
            if (vch == victim && victim != ch)
                act("$n looks at you.", ch, NULL, victim, TO_VICT);
            else if (vch != victim)
            {
                if (ch == victim)
                    act("$N looks at $Mself.", vch, NULL, ch, TO_CHAR);
                else
                {
                    sprintf(buf, "%s looks at $N.", PERS(ch, vch));
                    act(buf, vch, NULL, victim, TO_CHAR);
                }
            }
        }
    }

    if (scrying && victim->in_room != NULL)
    {
        const char * verb("");
        switch (victim->position)
        {
            case POS_DEAD:      verb = "lying dead";                break;
            case POS_MORTAL:    verb = "lying mortally wounded";    break;
            case POS_INCAP:     verb = "lying incapacitated";       break;
            case POS_STUNNED:   verb = "lying stunned";             break;
            case POS_SLEEPING:  verb = "asleep";                    break;
            case POS_RESTING:   verb = "resting";                   break;
            case POS_SITTING:   verb = "sitting";                   break;
            case POS_FIGHTING:  verb = "fighting";                  break;
            case POS_STANDING:  verb = "standing";                  break;
        }

        const char * location;
        switch (victim->in_room->sector_type)
        {
            case SECT_HILLS:        location = "upon the hills";        break;
            case SECT_INSIDE:       location = "within a building";     break;
            case SECT_CITY:         location = "in a city";             break;
            case SECT_FIELD:        location = "in the fields";         break;
            case SECT_FOREST:       location = "in a forest";           break;
            case SECT_MOUNTAIN:     location = "in the mountains";      break;
            case SECT_WATER_SWIM:
            case SECT_WATER_NOSWIM: 
            case SECT_UNDERWATER:   location = NULL;                    break;
            case SECT_AIR:          location = "in midair";             break;
            case SECT_DESERT:       location = "in a desert";           break;
            case SECT_UNDERGROUND:  location = "underground";           break;
            case SECT_ROAD:         location = "on a road";             break;
            case SECT_SWAMP:        location = "in a swamp";            break;
            default:                location = "somewhere";             break;
        }

        std::ostringstream mess;
        mess << "You see $N " << verb << " ";
        if (location == NULL) 
            mess << "in " << victim->in_room->name;
        else 
            mess << location;    
        
        mess << "...";
        act(mess.str().c_str(), ch, NULL, victim, TO_CHAR);
    }

    // Check for masquerade
    if (victim->in_room != NULL)
    {
        AFFECT_DATA * masquerade(get_area_affect(victim->in_room->area, gsn_masquerade));
        if (masquerade != NULL && masquerade->modifier != ch->id && number_percent() > get_skill(ch, gsn_disillusionment))
        {
            send_to_char("The grinning skull stares back at you, crackling in the air.\n", ch);
            return;
        }
    }

    if (victim->description[0] != '\0') send_to_char(victim->description, ch);
    else act("You see nothing special about $M.", ch, NULL, victim, TO_CHAR);

    show_char_status(victim, ch);

    if (!IS_NPC(victim) && BIT_GET(victim->pcdata->traits, TRAIT_MARKED))
        act(god_table[victim->religion].marking, ch, NULL, victim, TO_CHAR);

    if (!is_affected(victim, gsn_chameleon) || (victim == ch))
    {
    	found = FALSE;
        if (IS_NPC(victim)
        && (victim->pIndexData->vnum == MOB_VNUM_MIRROR_IMAGE)
        && victim->leader)
            sch = victim->leader;
        else if (IS_OAFFECTED(victim, AFF_DOPPEL)
        && victim->memfocus[DOPPEL_SLOT])
            sch = victim->memfocus[DOPPEL_SLOT];
        else 
            sch = victim;

        // Build the eq list
        unsigned int count(0);
        std::string eqList(show_eq_to_char(*sch, *ch, false, count));
        if (count > 0)
        {
            act("\n$N is using:", ch, NULL, victim, TO_CHAR);
            send_to_char(eqList.c_str(), ch);
        }
    }

    if ( victim != ch
    &&   !is_affected(victim, gsn_chameleon)
    &&   !IS_NPC(ch)
    &&   (!IS_NPC(victim) || !victim->pIndexData->pShop)
    &&	 (IS_NPC(victim) || !BIT_GET(victim->pcdata->traits, TRAIT_STREETWISE) || number_percent() >= 10)
    &&   IS_SET(ch->nact,PLR_AUTOPEEK)
    &&   number_percent( ) < get_skill(ch,gsn_peek)
    &&   (IS_IMMORTAL(ch) || !IS_IMMORTAL(victim)))
    {
	send_to_char( "\n\rYou peek at the inventory:\n\r", ch );
	check_improve(ch,victim,gsn_peek,TRUE,4);
	show_list_to_char( victim->carrying, ch, TRUE, TRUE );

        /* Assassins don't get to see gold -cackle- */
	if (ch->class_num != global_int_class_assassin)
	{
	    sprintf(buf, "     %s.\n\r", coins_to_str(victim->coins));
	    send_to_char(buf, ch);
        }
    }
}

void show_char_to_char( CHAR_DATA *list, CHAR_DATA *ch )
{
    for (CHAR_DATA * rch = list; rch != NULL; rch = rch->next_in_room)
    {
        if (rch == ch || (ch->level < AVATAR && IS_AFFECTED(rch, AFF_WIZI)) || get_trust(ch) < rch->invis_level || !can_see(ch, rch))
            continue;

        show_char_to_char_0(rch, ch);
    }

    // Check for unreal incursion to possibly show a random other character
    if (ch->in_room == NULL || !is_affected(ch, gsn_unrealincursion) || g_num_char_data < 2)
        return;
    
    srand(ch->id + ch->in_room->vnum);
    if ((rand() % 100) <= 3)
    {
        // Find a random character in the world
        unsigned int count(rand() % g_num_char_data);
        CHAR_DATA * rch(char_list);
        for (unsigned int i(0); i < count; ++i)
            rch = rch->next;

        // Make sure they can see the chosen character; include the possibility of seeing themselves
        if (!((ch->level < AVATAR && IS_AFFECTED(rch, AFF_WIZI)) || get_trust(ch) < rch->invis_level || !can_see(ch, rch)))
            show_char_to_char_0(rch, ch);
    }
    srand(time(0));
} 

bool check_blind( CHAR_DATA *ch )
{

    if (!IS_NPC(ch) && IS_SET(ch->act,PLR_HOLYLIGHT))
	return FALSE;
/* Minor change for the sake of speed */
//    if (is_affected(ch,gsn_truesight))
//        return FALSE;

    if (IS_SET(ch->imm_flags, IMM_BLIND))
	return FALSE;

    if (is_affected(ch, gsn_sensory_vision))
	return FALSE;

    if ( IS_AFFECTED(ch, AFF_BLIND) && ch->race != global_int_race_shuddeni) 
	return TRUE; 

    return FALSE;
}

/* changes your scroll */
void do_scroll(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[100];
    int lines;

    one_argument(argument,arg);
    
    if (arg[0] == '\0')
    {
	if (ch->lines == 0)
	    send_to_char("You do not page long messages.\n\r",ch);
	else
	{
	    sprintf(buf,"You currently display %d lines per page.\n\r",
		    ch->lines + 2);
	    send_to_char(buf,ch);
	}
	return;
    }

    if (!is_number(arg))
    {
	send_to_char("You must provide a number.\n\r",ch);
	return;
    }

    lines = atoi(arg);

    if (lines == 0)
    {
        send_to_char("Paging disabled.\n\r",ch);
        ch->lines = 0;
        return;
    }

    if (lines < 10 || lines > 100)
    {
	send_to_char("You must provide a reasonable number.\n\r",ch);
	return;
    }

    sprintf(buf,"Scroll set to %d lines.\n\r",lines);
    send_to_char(buf,ch);
    ch->lines = lines - 2;
}

/* RT does socials */
void do_socials(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    int iSocial;
    int col;
     
    col = 0;
   
    for (iSocial = 0; social_table[iSocial].name[0] != '\0'; iSocial++)
    {
	sprintf(buf,"%-12s",social_table[iSocial].name);
	send_to_char(buf,ch);
	if (++col % 6 == 0)
	    send_to_char("\n\r",ch);
    }

    if ( col % 6 != 0)
	send_to_char("\n\r",ch);
    return;
}


 
/* RT Commands to replace news, motd, imotd, etc from ROM */

void do_motd(CHAR_DATA *ch, char *argument)
{
    do_help(ch,"motd");
}

void do_imotd(CHAR_DATA *ch, char *argument)
{  
    do_help(ch,"imotd");
}

void do_rules(CHAR_DATA *ch, char *argument)
{
    do_help(ch,"rules");
}

void do_story(CHAR_DATA *ch, char *argument)
{
    do_help(ch,"story");
}

void do_wizlist(CHAR_DATA *ch, char *argument)
{
    do_help(ch,"wizlist");
}

/* RT this following section holds all the auto commands from ROM, as well as
   replacements for config */

void show_one_bit(CHAR_DATA * ch, const char * name, long bit, long bitvector)
{
	send_to_char(name, ch);
	if (IS_SET(bitvector, bit))
		send_to_char("ON\n\r", ch);
	else
		send_to_char("OFF\n\r", ch);
}

void do_autolist(CHAR_DATA *ch, char *argument)
{
    /* lists most player flags */
    if (IS_NPC(ch))
      return;

    send_to_char("   action     status\n\r",ch);
    send_to_char("---------------------\n\r",ch);
 
    show_one_bit(ch, "autoattack     ", PLR_AUTOATTACK, ch->act);
    show_one_bit(ch, "autodefend     ", PLR_AUTODEFEND, ch->act);
    show_one_bit(ch, "autoassist     ", PLR_AUTOASSIST, ch->act);
    show_one_bit(ch, "autoexit       ", PLR_AUTOEXIT, ch->act);
    show_one_bit(ch, "autogold       ", PLR_AUTOGOLD, ch->act);
    show_one_bit(ch, "autoloot       ", PLR_AUTOLOOT, ch->act);
    show_one_bit(ch, "autodestroy    ", PLR_AUTODES, ch->act);
    show_one_bit(ch, "autosplit      ", PLR_AUTOSPLIT, ch->act);
    show_one_bit(ch, "autodate       ", PLR_AUTODATE, ch->act);
    show_one_bit(ch, "autooath       ", PLR_AUTOOATH, ch->nact);
    if (get_skill(ch,gsn_peek))
	show_one_bit(ch, "autopeek       ", PLR_AUTOPEEK, ch->nact);
    if (get_skill(ch,gsn_trackless_step) || get_skill(ch,gsn_covertracks))
	show_one_bit(ch, "autotracks     ", PLR_AUTOTRACKS, ch->nact);
    show_one_bit(ch, "show ep        ", PLR_EPNOTE, ch->act);
    show_one_bit(ch, "show lines     ", PLR_SHOWLINES, ch->act);
    show_one_bit(ch, "prompt         ", COMM_PROMPT, ch->comm);
    show_one_bit(ch, "compact mode   ", COMM_COMPACT, ch->comm);
    show_one_bit(ch, "combine items  ", COMM_COMBINE, ch->comm);

    if (ch->level == MAX_LEVEL)
	show_one_bit(ch,"(IMM)extraspam  ", PLR_EXTRASPAM, ch->nact);

    if (IS_SET(ch->act,PLR_NOSUMMON))
	send_to_char("You cannot be summoned outside your PK.\n\r",ch);
    else
	send_to_char("You can be summoned outside your PK.\n\r",ch);
   
    if (IS_SET(ch->act,PLR_NOFOLLOW))
	send_to_char("You do not welcome followers.\n\r",ch);
    else
	send_to_char("You accept followers.\n\r",ch);

    if (IS_SET(ch->comm, COMM_TRANSLATE))
		send_to_char("You are viewing all translated text.\n\r", ch);

    if (IS_SET(ch->nact, PLR_NOYELL))
		send_to_char("You will not yell when being attacked.\n\r", ch);

    if (IS_SET(ch->nact, PLR_NOACCEPT))
		send_to_char("You will not accept items offered to you.\n\r", ch);
}

void do_epshow(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
    
    if (IS_SET(ch->act,PLR_EPNOTE))
    {
      send_to_char("EP display removed.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_EPNOTE);
    }
    else
    {
      send_to_char("EP display on.\n\r",ch);
      SET_BIT(ch->act,PLR_EPNOTE);
    }
}

void do_showlines(CHAR_DATA * ch, char * argument)
{
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->act, PLR_SHOWLINES))
	{
		send_to_char("You will no longer see line numbers in the text editor.\n\r", ch);
		REMOVE_BIT(ch->act, PLR_SHOWLINES);
		return;
	}

	send_to_char("You will now see line numbers in the text editor.\n\r", ch);
	SET_BIT(ch->act, PLR_SHOWLINES);
}

void do_autoattack(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
	return;

    if (IS_AFFECTED(ch, AFF_BERSERK))
    {
	send_to_char("Anger is coursing through your veins!  You must attack!\n\r", ch);
	return;
    }
    if (IS_NAFFECTED(ch, AFF_ASHURMADNESS))
    {
	send_to_char("You must eat!\n\r",ch);
	return;
    }

    if (IS_SET(ch->act,PLR_AUTOATTACK))
    {
	send_to_char("Autoattack removed.\n\r",ch);
	REMOVE_BIT(ch->act,PLR_AUTOATTACK);
    }
    else
    {
	send_to_char("You will now attack in combat.\n\r",ch);
	SET_BIT(ch->act,PLR_AUTOATTACK);
    }
}

void do_autodefend(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
	return;
    if (IS_SET(ch->act,PLR_AUTODEFEND))
    {
	send_to_char("Autodefend removed.\n\r",ch);
	REMOVE_BIT(ch->act,PLR_AUTODEFEND);
    }
    else
    {
	send_to_char("You will now defend in combat.\n\r",ch);
	SET_BIT(ch->act,PLR_AUTODEFEND);
    }
}

void do_autoassist(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
        return;
    
    if (IS_SET(ch->act,PLR_AUTOASSIST))
    {
        send_to_char("Autoassist removed.\n\r",ch);
        REMOVE_BIT(ch->act,PLR_AUTOASSIST);
    }
    else
    {
        send_to_char("You will now assist when needed.\n\r",ch);
        SET_BIT(ch->act,PLR_AUTOASSIST);
    }
}

void do_autoexit(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (IS_SET(ch->act,PLR_AUTOEXIT))
    {
      send_to_char("Exits will no longer be displayed.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_AUTOEXIT);
    }
    else
    {
      send_to_char("Exits will now be displayed.\n\r",ch);
      SET_BIT(ch->act,PLR_AUTOEXIT);
    }
}

void do_autogold(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (IS_SET(ch->act,PLR_AUTOGOLD))
    {
      send_to_char("Autogold removed.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_AUTOGOLD);
    }
    else
    {
      send_to_char("Automatic gold looting set.\n\r",ch);
      SET_BIT(ch->act,PLR_AUTOGOLD);
    }
}

void do_autoloot(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (IS_SET(ch->act,PLR_AUTOLOOT))
    {
      send_to_char("Autolooting removed.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_AUTOLOOT);
    }
    else
    {
      send_to_char("Automatic corpse looting set.\n\r",ch);
      SET_BIT(ch->act,PLR_AUTOLOOT);
    }
}

void do_autooath(CHAR_DATA * ch, char * argument)
{
    if (IS_NPC(ch))
        return;

    if (IS_SET(ch->nact, PLR_AUTOOATH))
    {
        send_to_char("You will no longer accept oaths of fealty.\n", ch);
        REMOVE_BIT(ch->nact, PLR_AUTOOATH);
        return;
    }

    send_to_char("You will now accept oaths of fealty.\n", ch);
    SET_BIT(ch->nact, PLR_AUTOOATH);
}

void do_autopeek(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
    	return;
 
    if (get_skill(ch,gsn_peek)==0)
    {
	send_to_char("You aren't skilled enough to peek into people's inventories.\n\r",ch);
	return;
    }

    if (IS_SET(ch->nact,PLR_AUTOPEEK))
    {
      	send_to_char("You will no longer look into people's inventories.\n\r",ch);
      	REMOVE_BIT(ch->nact,PLR_AUTOPEEK);
    }
    else
    {
      	send_to_char("You will now look into people's inventories.\n\r",ch);
      	SET_BIT(ch->nact,PLR_AUTOPEEK);
    }
}

void do_autotracks(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
        return;
 
    if (get_skill(ch,gsn_trackless_step) == 0 
      && get_skill(ch,gsn_covertracks) == 0)
    {
	send_to_char("You aren't skilled enough to cover your tracks.\n\r",ch);
	return;
    }

    if (IS_SET(ch->nact,PLR_AUTOTRACKS))
    {
      	send_to_char("You will no longer attempt to cover your tracks.\n\r",ch);
      	REMOVE_BIT(ch->nact,PLR_AUTOTRACKS);
    }
    else
    {
      	send_to_char("You will now attempt to cover your tracks.\n\r",ch);
      	SET_BIT(ch->nact,PLR_AUTOTRACKS);
    }
}

void do_autodate(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
	return;

    if (IS_SET(ch->act, PLR_AUTODATE))
    {
	send_to_char("You will no longer automatically see new dates.\n\r", ch);
	REMOVE_BIT(ch->act, PLR_AUTODATE);
    }
    else
    {
	send_to_char("You will now automatically be notified when the in-game date changes.\n\r", ch);
	SET_BIT(ch->act, PLR_AUTODATE);
    }
}

void do_autodestroy(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (IS_SET(ch->act,PLR_AUTODES))
    {
      send_to_char("Autodestroying removed.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_AUTODES);
    }
    else
    {
      send_to_char("Automatic corpse destroying set.\n\r",ch);
      SET_BIT(ch->act,PLR_AUTODES);
    }
}

void do_autosplit(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (IS_SET(ch->act,PLR_AUTOSPLIT))
    {
      send_to_char("Autosplitting removed.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_AUTOSPLIT);
    }
    else
    {
      send_to_char("Automatic coin splitting set.\n\r",ch);
      SET_BIT(ch->act,PLR_AUTOSPLIT);
    }
}

void do_extraspam(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
      return;

    if (ch->level < MAX_LEVEL)
    {
	send_to_char("Huh?\n\r",ch);
	return;
    }
 
    if (IS_SET(ch->nact,PLR_EXTRASPAM))
    {
	send_to_char("Extra combat spam will be hidden.\n\r",ch);
	REMOVE_BIT(ch->nact,PLR_EXTRASPAM);
    }
    else
    {
	send_to_char("Extra combat spam will be visible.\n\r",ch);
	SET_BIT(ch->nact,PLR_EXTRASPAM);
    }
}

void do_brief(CHAR_DATA *ch, char *argument)
{
    if (IS_SET(ch->comm,COMM_BRIEF))
    {
      send_to_char("Full descriptions activated.\n\r",ch);
      REMOVE_BIT(ch->comm,COMM_BRIEF);
    }
    else
    {
      send_to_char("Short descriptions activated.\n\r",ch);
      SET_BIT(ch->comm,COMM_BRIEF);
    }
}

void do_compact(CHAR_DATA *ch, char *argument)
{
    if (IS_SET(ch->comm,COMM_COMPACT))
    {
      send_to_char("Compact mode removed.\n\r",ch);
      REMOVE_BIT(ch->comm,COMM_COMPACT);
    }
    else
    {
      send_to_char("Compact mode set.\n\r",ch);
      SET_BIT(ch->comm,COMM_COMPACT);
    }
}

void do_show(CHAR_DATA *ch, char *argument)
{
    if (IS_SET(ch->comm,COMM_SHOW_AFFECTS))
    {
      send_to_char("Affects will no longer be shown in score.\n\r",ch);
      REMOVE_BIT(ch->comm,COMM_SHOW_AFFECTS);
    }
    else
    {
      send_to_char("Affects will now be shown in score.\n\r",ch);
      SET_BIT(ch->comm,COMM_SHOW_AFFECTS);
    }
}

void do_battlecry(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    if ((get_skill(ch, gsn_warcry) == 0)
     && (get_skill(ch, gsn_sortie) == 0)
     && (get_skill(ch, gsn_rally) == 0)
     && (get_skill(ch, gsn_taunt) == 0)
     && (get_skill(ch, gsn_healingward) == 0))
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	if (ch->battlecry != NULL && ch->battlecry[0] != '\0')
	{
	    sprintf(buf,"Your battlecry is: %s\n\r",ch->battlecry);
	    send_to_char(buf,ch);
	}
	else
	    send_to_char("Set your battlecry to what?\n\r", ch);
	return;
    }

    if (!str_cmp(argument,"clear"))
    {
	if (ch->battlecry)
	{
	    send_to_char("Battlecry cleared.\n\r",ch);
	    free_string(ch->battlecry);
	}
	else
	    send_to_char("You don't have a battlecry set.\n\r",ch);
	return;
    }

    if (ch->battlecry)
	free_string(ch->battlecry);

    ch->battlecry = str_dup(argument);
    send_to_char("You are prepared with your new battlecry.\n\r", ch);

    return;
}

void do_prompt(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];
 
   if ( argument[0] == '\0' )
   {
	if (IS_SET(ch->comm,COMM_PROMPT))
   	{
      	    send_to_char("You will no longer see prompts.\n\r",ch);
      	    REMOVE_BIT(ch->comm,COMM_PROMPT);
    	}
    	else
    	{
      	    send_to_char("You will now see prompts.\n\r",ch);
      	    SET_BIT(ch->comm,COMM_PROMPT);
    	}
       return;
   }
 
   if( !strcmp( argument, "all" ) )
      strcpy( buf, "{c<%hhp %mm %vmv>{x " );
   else
   {
      if ( strlen(argument) > MAX_PROMPT_LENGTH )
         argument[MAX_PROMPT_LENGTH] = '\0';
      strcpy( buf, argument );
      smash_tilde( buf );
      if (str_suffix("%c",buf))
	strcat(buf," ");
	
   }
 
   free_string( ch->prompt );
   ch->prompt = str_dup( buf );
   sprintf(buf,"Prompt set to %s\n\r",ch->prompt );
   send_to_char(buf,ch);


   return;
}

void do_defaultprompt(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];

   if (!(ch->desc->acct)) {
       send_to_char("This character is not associated with an account.\n\r",ch);
       return;
   }
  
   if ( argument[0] == '\0' )
   {
       if (!(ch->desc->acct->def_prompt))
           send_to_char("Your account has no default prompt set.\n\r",ch);
       else {
	   sprintf(buf, "Your default prompt is: %s", ch->desc->acct->def_prompt);
	   send_to_char(buf, ch);
       }
       return;
   }

   free_string( ch->desc->acct->def_prompt );
   
   if (!strcmp(argument, "clear")) {
       strcpy(buf, "");
       ch->desc->acct->def_prompt = str_dup( buf );
       send_to_char("Default prompt cleared\n\r", ch);
   }
   else {
       if ( strlen(argument) > MAX_PROMPT_LENGTH )
           argument[MAX_PROMPT_LENGTH] = '\0';
       strcpy( buf, argument );
       smash_tilde( buf );
       if (str_suffix("%c",buf))
	   strcat(buf," ");
       ch->desc->acct->def_prompt = str_dup( buf );
       sprintf(buf, "Default prompt set to %s\n\r", ch->desc->acct->def_prompt);
       send_to_char(buf, ch);
   }

   return;
}
	   

void do_combine(CHAR_DATA *ch, char *argument)
{
    if (IS_SET(ch->comm,COMM_COMBINE))
    {
      send_to_char("Long inventory selected.\n\r",ch);
      REMOVE_BIT(ch->comm,COMM_COMBINE);
    }
    else
    {
      send_to_char("Combined inventory selected.\n\r",ch);
      SET_BIT(ch->comm,COMM_COMBINE);
    }
}

void do_noloot(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
      return;
 
    if (!IS_SET(ch->act,PLR_CANLOOT))
/*
    {
      send_to_char("Your corpse is now safe from thieves.\n\r",ch);
      REMOVE_BIT(ch->act,PLR_CANLOOT);
    }
    else
*/
    {
      send_to_char("Your corpse may now be looted.\n\r",ch);
      SET_BIT(ch->act,PLR_CANLOOT);
    }
}

void do_nofollow(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *vch;

    if (IS_NPC(ch))
	return;
 
    if (IS_SET(ch->act,PLR_NOFOLLOW))
    {
        send_to_char("You now accept followers.\n\r",ch);
        REMOVE_BIT(ch->act,PLR_NOFOLLOW);
    }
    else
    {
        if (IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL)
        {
            act("You would rather follow $N!", ch, NULL, ch->master, TO_CHAR);
            return;
        }
        send_to_char("You no longer accept followers.\n\r",ch);
        SET_BIT(ch->act,PLR_NOFOLLOW);

        for (vch = char_list; vch; vch = vch->next)
        {
            if (IS_NPC(vch) && is_affected(vch, gsn_demoniccontrol) && vch->leader == ch)
            {
                act("You feel your control over $N fade.", ch, NULL, vch, TO_CHAR);

                stop_follower(vch);
                vch->tracking = ch;
                vch->demontrack = ch;

                if (vch->in_room == ch->in_room)
                {
                    set_fighting(vch, ch);
                    act("$n screams in fury and attacks!", vch, NULL, NULL, TO_ROOM);
                    multi_hit(vch, ch, TYPE_UNDEFINED);
                }
            }
        }
        die_follower(ch, false, true);
    }
}

void do_nosummon(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
    {
      if (IS_SET(ch->imm_flags,IMM_SUMMON))
      {
	send_to_char("You are no longer immune to summon.\n\r",ch);
	REMOVE_BIT(ch->imm_flags,IMM_SUMMON);
      }
      else
      {
	send_to_char("You are now immune to summoning.\n\r",ch);
	SET_BIT(ch->imm_flags,IMM_SUMMON);
      }
    }
    else
    {
      if (IS_SET(ch->act,PLR_NOSUMMON))
      {
        send_to_char("You are no longer immune to summon.\n\r",ch);
        REMOVE_BIT(ch->act,PLR_NOSUMMON);
      }
      else
      {
        send_to_char("You are now immune to summoning.\n\r",ch);
        SET_BIT(ch->act,PLR_NOSUMMON);
      }
    }
}

void do_inspect( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim, *vch;
    char buf[MAX_STRING_LENGTH];

    if (argument[0] == '\0')
    {
	send_to_char("Inspect whom?\n\r", ch);
	return;
    }

    if ((victim = get_char_room(ch, argument)) == NULL)
    {
	send_to_char("You don't see them here.\n\r", ch);
	return;
    }
/* brazen: Inspect can be used to determine character health while in berserk, which defeats the purpose of showing 0 health */

    if (!is_affected(victim, gsn_berserk))
    {
      show_char_status(victim, ch);
    } else {
      if (ch == victim)
      {
        sprintf(buf, "You are too enraged to care about your own condition.\n\r");
        send_to_char(buf, ch);
      } else {
        sprintf(buf, "%s is moving too wildly for you to accurately gauge their condition.\n\r", victim->name);
        send_to_char(buf, ch);
      }
    }
    if (IS_NAFFECTED(victim, AFF_ASHURMADNESS))
	if (ch == victim)
	    send_to_char("You are STARVING!\n\r",ch);
	else
	    act("$N is drooling uncontrollably.",ch,NULL,victim,TO_CHAR);

    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
    {
	if (can_see(vch, ch))
	{
	    if (vch == victim)
		act("$n quickly inspects you.", ch, NULL, victim, TO_VICT);
	    else if (vch != ch)
	    {
		if (ch == victim)
		    act("$N quickly inspects $Mself.", vch, NULL, ch, TO_CHAR);
		else
		{
		    sprintf(buf, "%s quickly inspects $N.", PERS(ch, vch));
		    act(buf, vch, NULL, victim, TO_CHAR);
		}
	    }
	}
    }
  
    return;
}
void do_rangermenu( CHAR_DATA *ch, char *argument)
{
	int i = 0;
	int option_number = 0;
	const int MAX_I = 35;
	char tempstring[MAX_STRING_LENGTH];
	char sample_line[MAX_STRING_LENGTH];
	tempstring[0] = '\0';
	sample_line[0] = '\0';
	if (ch->class_num != global_int_class_ranger)
	{
		   send_to_char("Huh?\n\r",ch);
		   return;
	}
	else 
	{
		//======================ANIMALS=========================
		if (argument[0] == '\0')
		{
			act("      {y== Animal Menu Options =={n", ch, NULL, NULL, TO_CHAR);
			act("      {ynames{D - {nLists the animals you may call.", ch, NULL, NULL, TO_CHAR);
			act("  {yabilities{D - {nLists the commands you may give.",ch, NULL, NULL, TO_CHAR);
			act("    {yoptions{D - {nLists the instinctive actions your animals take when you execute combat moves.",ch,NULL,NULL,TO_CHAR);
			act("        {yall{D - {nDisplays all of the above.",ch,NULL,NULL,TO_CHAR);
			act("     {ytoggle{D - {n(animals toggle ##) Toggles the {yoption{n number for a particular automatic abilitiy.",ch,NULL,NULL,TO_CHAR);
			return;
		}
		//========================ANIMALS NAMES=====================
		else if (argument[0] == 'n')
		{
			if (BIT_GET(ch->pcdata->bitptr,rangermenu_array[i].hasanimal_bit))	
			{
				act("You have learned the ways of the following animals:",ch,NULL,NULL,TO_CHAR);
				do
				{
					if (BIT_GET(ch->pcdata->bitptr,rangermenu_array[i].hasanimal_bit))
	                        	{
						sprintf(tempstring, " {g");
		                                strcat(tempstring,rangermenu_array[i].animal);
						strcat(tempstring,"{n");
					}
	        	               	act(tempstring, ch, NULL, NULL, TO_CHAR);
				   	i++;
				}while(i < MAX_I && i < MAX_RANGER_ABILITIES && rangermenu_array[i].animal && rangermenu_array[i].ability);
			}
		}
		//=====================ANIMALS ABILITIES=================
		else if (argument[0] == 'a')
		{
			act("You have learned to will your animal companions to execute the following moves:",ch,NULL,NULL,TO_CHAR);
			do
			{
				if (BIT_GET(ch->pcdata->bitptr,rangermenu_array[i].has_bit) && rangermenu_array[i].data_type==2 && BIT_GET(ch->pcdata->bitptr,rangermenu_array[i].hasanimal_bit))
				{
                                        sprintf(tempstring, " {y");
					strcat(tempstring,rangermenu_array[i].ability);
					strcat(tempstring,"{n");
				}
                           act(tempstring, ch, NULL, NULL, TO_CHAR);
			   i++;
			}while(i < MAX_I && i < MAX_RANGER_ABILITIES && rangermenu_array[i].animal && rangermenu_array[i].ability);
		}
		//====================ANIMALS OPTIONS====================
		else if (argument[0] == 'o')
		{
			act("Your animal companions are currently executing the following blah blah",ch,NULL,NULL,TO_CHAR);
			do
			{
				if (rangermenu_array[i].data_type==3 && BIT_GET(ch->pcdata->bitptr,rangermenu_array[i].hasanimal_bit))
				{
					if((i+1)<10)
        	        	        {
                	        	       strcat(tempstring,"0");
	                        	       strcat(tempstring,itoa(option_number+1,sample_line));
		                        }
        		                else
                		               sprintf(tempstring,itoa(option_number+1,sample_line));
                        		option_number ++;
					strcat(tempstring,rangermenu_array[i].trigger);
					strcat(tempstring," ");
					if (BIT_GET(ch->pcdata->bitptr,rangermenu_array[i].state_bit))
						strcat(tempstring, "{G ON{n");
					else
						strcat(tempstring, "{r OFF{n");
				}
				act(tempstring, ch, NULL, NULL, TO_CHAR);
				i++;
			}while(i < MAX_I && i < MAX_RANGER_ABILITIES && rangermenu_array[i].animal && rangermenu_array[i].ability);
		}
		
		//============================ANIMALS TOGGLE=======================
		else if(argument[0] == 't')
		{
			act("TOGGLE STUFF GOES HERE******",ch,NULL,NULL,TO_CHAR);
			return;
		}
		
		//============================ANIMALS ALL===========================
		else if(argument[0] == 'a')
		{
			act("You have the following automatic animal commands set.", ch, NULL, NULL, TO_CHAR);
			act(" {gAnimal        {W##   {yAbility            {cTrigger         {rStatus{n", ch, NULL, NULL, TO_CHAR);
			act(" {D============================================================={n", ch, NULL, NULL, TO_CHAR);
			do
			{
				   //the below only proceeds if and they have unlocked the item
				   //(using the has_bit, which is the same for all abilities
				   //of any given animal)
				   if (BIT_GET(ch->pcdata->bitptr,rangermenu_array[i].hasanimal_bit))
				   {
				  	   sprintf(tempstring, " {g"); 
					   strcat(tempstring,rangermenu_array[i].animal);
					   strcat(tempstring,"{W");
					   if (rangermenu_array[i].data_type==3)
					   {  
					   	if((i+1)<10) 
						{
						         strcat(tempstring,"0");
					  	   	 strcat(tempstring,itoa(option_number+1,sample_line));
			        	   	}
						else 
					        	sprintf(tempstring,itoa(option_number+1,sample_line));
						option_number ++;
					   }
					   else if (rangermenu_array[i].data_type==2)
						   strcat(tempstring,"{D--{n");
					   strcat(tempstring," {y");
					   strcat(tempstring,rangermenu_array[i].ability);
					   strcat(tempstring," {c");
					   strcat(tempstring,rangermenu_array[i].trigger);
					   strcat(tempstring," ");			      

					   //only displays the "Status" if the item is a
					   //toggle-type 'option' (ie not command or animal)
					   if (rangermenu_array[i].data_type==3)
					   {
						   if (BIT_GET(ch->pcdata->bitptr,rangermenu_array[i].state_bit))
						        strcat(tempstring, "{G ON{n");
						   else
						        strcat(tempstring, "{r OFF{n");
					   }
				   }
     
				   act(tempstring, ch, NULL, NULL, TO_CHAR);
				   i++;
			} while (i < MAX_I && i < MAX_RANGER_ABILITIES && rangermenu_array[i].animal && rangermenu_array[i].ability);
		}
		else
			act("Type {Wanimals{n for the animal menu options.",ch,NULL,NULL,TO_CHAR);
	}
}

void do_diagnose( CHAR_DATA *ch, char *argument )
{
	CHAR_DATA *victim, *vch;
	char buf[MAX_STRING_LENGTH];
	int diagnoseskill;

	if ((diagnoseskill = get_skill(ch, gsn_diagnose)) == 0)
	{
		send_to_char("Huh?\n\r", ch );
		return;
	}
	if (argument[0] == '\0')
	{
		send_to_char("Diagnose whom?\n\r", ch);
		return;
	}
	if ((victim = get_char_room(ch, argument)) == NULL)
	{
		send_to_char("You don't see them here.\n\r", ch);
		return;
	}
	if(victim==ch)
		act("You examine yourself briskly for outward signs of possible ailments.",ch,NULL,victim,TO_CHAR);
	else
		act("You run your gaze over $N briskly, examining them for telltale signs of possible ailments.",ch,NULL,victim,TO_CHAR);
	//checks skill % vs. success - if fails, gets a blanket echo for any of the 'subtle' mals, still sees the 'obvious' mals regardless of skill
	if (diagnoseskill < number_percent())
	{
		if (is_affected(victim, gsn_plague)
		||(is_affected(victim, gsn_fever))
		||(is_affected(victim, gsn_vitalstrike))
		||(is_affected(victim, gsn_impale))
		||(is_affected(victim, gsn_pox))
		||(is_affected(victim, gsn_poison))
		||(is_affected(victim, gsn_blindness))
		||(is_affected(victim, gsn_curse))
		||(is_affected(victim, gsn_grapple))
		||(is_affected(victim, gsn_hamstring))
		||(is_affected(victim, gsn_slice))
		||(is_affected(victim, gsn_cleave))
		||(is_affected(victim, gsn_enfeeblement))
		||(is_affected(victim, gsn_frostbite))
		||(is_affected(victim, gsn_gash))
		||(is_affected(victim, gsn_pommel))
		|| (victim->pcdata != NULL && victim->pcdata->condition[COND_THIRST] <= 0))
		act("Several things seem amiss with $N, but you do not feel as though you have made a proper examination.",ch,NULL,victim,TO_CHAR);
	}
	else
	{
	        if (is_affected(victim,gsn_enfeeblement))
	                act("$N seems remarkably frail. Telltale pockmarks beneath the skin and general maceration indicate recent enfeeblement.",ch,NULL,victim,TO_CHAR);
	        if (is_affected(victim, gsn_frostbite))
	                act("$N's lips and skin is pale and waxy, and $M seems to be shivering as one afflicted by frostbite.",ch,NULL,victim,TO_CHAR);
	        if ((victim->move) < ((victim->max_move) / 10))
	                act("$N exhibits the signs of complete exhaustion.",ch,NULL,victim,TO_CHAR);	
		if (is_affected(victim, gsn_plague)||is_affected(victim, gsn_fever))
			act("$N appears feverish and weak, and is likely suffering from wasting illness.", ch, NULL, victim, TO_CHAR);
		if (is_affected(victim, gsn_vitalstrike)||is_affected(victim, gsn_impale) )
			act("$N is pale and bleeding heavily from the gut, a sign of deep internal wounds.", ch, NULL, victim, TO_CHAR);
		if (is_affected(victim, gsn_gash))
			act("$N is bleeding from a vicious gash.", ch, NULL, victim, TO_CHAR);
		if (is_affected(victim, gsn_pommel))
			act("$N seems slightly disoriented from a blow to the head.", ch, NULL, victim, TO_CHAR);
		if (is_affected(victim, gsn_pox))
			act("$N is afflicted by the open, weeping sores of the consumptive pox.", ch, NULL, victim, TO_CHAR);
		if (is_affected(victim, gsn_poison))
			act("Bilious spittle dabs the corner of $N's mouth, the body's attempt to purge itself of poison.", ch, NULL, victim, TO_CHAR);
		if (is_affected(victim, gsn_blindness))
			act("$N's eyes are obscured by a hazy film of inky blackness.", ch, NULL, victim, TO_CHAR);
		if (is_affected(victim, gsn_curse))
		{
			if (victim->race != global_int_race_shuddeni)
				act("$N's eyes are dull and rimmed with unnatural, blackened skin, a sure sign of cursing.",ch,NULL,victim,TO_CHAR);
			else
				act("The skin of $N's eyesockets has blackened unnaturally, a sure sign of cursing.",ch,NULL,victim,TO_CHAR);
		}
		if (is_affected(victim, gsn_grapple))
			act("$N is moving stiffly and painfully, and bears the scrapes of someone forcibly wrestled to the ground.",ch,NULL,victim,TO_CHAR);
		if (is_affected(victim, gsn_hamstring))
			act("Blood seeps from the back of $N's leg, where someone has sliced deep into the tendon.",ch,NULL,victim,TO_CHAR);
		if (is_affected(victim, gsn_slice))
			act("The backs of $N's calves have been sliced at deeply.",ch,NULL,victim,TO_CHAR);
		if (is_affected(victim, gsn_consume))
		{
			if (victim->race == global_int_race_kankoran)
				act("$N's fur is charred and blackened, and faint traces of smoke wisp from $M smouldering coat.",ch,NULL,victim,TO_CHAR);
			else if (victim->race == global_int_race_srryn)
				act("$N's scales are tinged with traces of angry, red inflammation.",ch,NULL,victim,TO_CHAR);
			else
				act("$N's skin is inflamed to a deep, angry red, and patches seem to be smouldering.",ch,NULL,victim,TO_CHAR);
		}
		if (!IS_NPC(victim))
		{
			if (victim->pcdata->condition[COND_DRUNK] > 10)
				act("$N is decidedly inebriated.",ch,NULL,victim,TO_CHAR);
			if (victim->pcdata->condition[COND_THIRST] <= 0)
				act("$N seems to be getting thirsty.",ch,NULL,victim,TO_CHAR);
			if (victim->pcdata->condition[COND_HUNGER] <= 0)
				act("$N seems to be getting hungry.",ch,NULL,victim,TO_CHAR);
		}
	}
		//Now outside the else of the skill check -- these are the obvious mals which no one could miss
	if (is_affected(victim, gsn_gouge))
	        act("$N's face has been torn at, blinding $M with torn skin and tissue and clotting blood.", ch, NULL, victim, TO_CHAR);
	if (is_affected(victim, gsn_agony))
		 act("$N is clearly in excruciating pain, likely magically-induced.",ch,NULL,victim,TO_CHAR);
	if (is_affected(victim, gsn_cleave))
		act("Deep gashes, the tissue torn and bruised, show where $N has been recently cleaved with an axe.",ch,NULL,victim,TO_CHAR);
	if (is_affected(victim, gsn_boneshatter))
	{
		 if (IS_NAFFECTED(victim, AFF_ARMSHATTER))
			 act("$N's arm hangs at an odd angle, clearly broken.",ch,NULL,victim,TO_CHAR);
		 if (IS_NAFFECTED(victim, AFF_LEGSHATTER))
			 act("$N's leg is twisted and bent at a terrible angle, clearly broken.",ch,NULL,victim,TO_CHAR);
	}
	if (IS_SET(victim->oaffected_by, AFF_ONEHANDED))
		act("One of $N's hands has been removed entirely, a bare stump showing.",ch,NULL,victim,TO_CHAR);
	if (!IS_NPC(victim))
	{
		if (victim->pcdata->condition[COND_HUNGER] <= -300)
			act("$N is thin, weak, and displaying other signs of advanced starvation.",ch,NULL,victim,TO_CHAR);
		if (victim->pcdata->condition[COND_THIRST] <= -15)
			act("$N's lips are dry and cracked, and they display other signs of advanced dehydration.",ch,NULL,victim,TO_CHAR);
	}
	if(!is_affected(victim, gsn_berserk))
            show_char_status(victim, ch);
	if (diagnoseskill > 0)
		if (!IS_NPC(victim) && (victim!=ch))
		{
			if (number_percent() < diagnoseskill)
				check_improve(ch,victim,gsn_diagnose,TRUE,2);
			else
				check_improve(ch,victim,gsn_diagnose,FALSE,1);
		}
			
	for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
	{
		if (can_see(vch, ch))
		{
			if (vch == victim)
				act("$n briskly examines you with a meticulous glance.", ch, NULL, victim, TO_VICT);
			else if (vch != ch)
			{
				if (ch == victim)
					act("$N examines $Mself thoroughly.", vch, NULL, ch, TO_CHAR);
				else
				{
					sprintf(buf, "%s examines $N with a brisk but meticulous glance.", PERS(ch, vch));
					act(buf, vch, NULL, victim, TO_CHAR);
				}
			}
		}
	}
	return;
}

void do_look( CHAR_DATA *ch, char *argument )
{
    char buf  [MAX_STRING_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char arg3 [MAX_INPUT_LENGTH];
    EXIT_DATA *pexit;
    CHAR_DATA *victim, *charmer;
//    CHAR_DATA *vch;
    OBJ_DATA *obj;
    char *pdesc;
    int door;
    int number,count;
//    bool foo = FALSE;
    ROOM_INDEX_DATA *troom = NULL;

    if ( ch->desc == NULL )
        return;

    if (ch->in_room)
        troom = ch->in_room;

    if (is_affected(ch, gsn_paradise))
    {
        send_to_char("You can see nothing but your visions of paradise.\n\r", ch);
        act("$n glances about, but doesn't appear to see anything.", ch, NULL, NULL, TO_ROOM);
        return;
    }

    if ( ch->position < POS_SLEEPING )
    {
        send_to_char( "You're too hurt to see anything !\n\r", ch );
        return;
    }

    if ( ch->position == POS_SLEEPING )
    {
        send_to_char( "You can't see anything, you're sleeping!\n\r", ch );
        return;
    }

    if ( check_blind( ch ) )
    {
        send_to_char( "You can't see a thing!\n\r", ch);
        return;
    }

    // Check for unreal incursion
    if (troom != NULL && is_affected(ch, gsn_unrealincursion))
    {
        srand(ch->id + troom->vnum);
        if (rand() % 8 == 0)
        {
            // Pick an adjacent room at random
            std::vector<Direction::Value> directions(Direction::ValidDirectionsFrom(*troom));
            if (!directions.empty())
            {
                ROOM_INDEX_DATA * newRoom(Direction::Adjacent(*troom, directions[number_range(0, directions.size() - 1)], EX_CLOSED));
                if (newRoom != NULL)
                    troom = newRoom;
            }
        }
        srand(time(0));
    }

    if (!can_see_in_room(ch, troom))
    {
        send_to_char( "It is pitch black ... \n\r", ch );
        show_char_to_char( troom->people, ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    number = number_argument(arg1,arg3);
    count = 0;

    if ( arg1[0] == '\0' || !str_cmp( arg1, "auto" ) )
    {
	/* 'look' or 'look auto' */
	if (!IS_NPC(ch) && IS_NAFFECTED(ch, AFF_DELUSION))
	{
	    int c;
	    for (c = 0; c < 100;c++)
	    {
		if ((troom = get_room_index(number_range(400,30000))) == NULL)
		    continue;
		if (!BIT_GET(ch->pcdata->travelptr, troom->vnum))
		    continue;
	    }
	    if (troom == NULL)
		troom = ch->in_room;
	}
	if (is_affected(ch, gsn_visions))
	    send_to_char(visions_room, ch);
    else if (area_is_affected(troom->area, gsn_brume) && number_percent() > get_skill(ch, gsn_stormmastery))
        send_to_char(brume_room, ch);
	else
 	{
	    send_to_char(troom->name, ch );

	    if (IS_IMMORTAL(ch) && (IS_NPC(ch) || IS_SET(ch->act,PLR_HOLYLIGHT)))
	    {
	        sprintf(buf," [Room %d]",ch->in_room->vnum);
	    	send_to_char(buf,ch);
	    }

	    send_to_char( "\n\r", ch );

	    if (arg1[0] == '\0' || ((!IS_NPC(ch) || ch->desc != NULL) && !IS_SET(ch->comm, COMM_BRIEF)))
	    {
	        global_bool_sending_brief = TRUE;
	    	send_to_char( "  ",ch);
	    	send_to_char( troom->description, ch );
	    	global_bool_sending_brief = FALSE;
	    	send_to_char("\n\r",ch);
	    }
	    else
	        send_to_char("\n\r",ch);

	if (room_is_affected(ch->in_room, gsn_smokescreen))
	{
	    send_to_char("A thick screen of smoke partially obscures this place.\n\r", ch);
	    count = 1;
	}
	
	if ( room_is_affected(ch->in_room, gsn_blazinginferno))
	{
	    send_to_char("A blazing inferno is tearing through here.\n\r", ch);
	    count = 1;
	}

	if ( room_is_affected(ch->in_room, gsn_blaze))
	{
	    send_to_char("Blazing flames lick at the surroundings.\n\r", ch);
	    count = 1;
	}
	
	if (room_is_affected(ch->in_room, gsn_freeze))
	{
	    if (ch->in_room->sector_type == SECT_WATER_SWIM
	      || ch->in_room->sector_type == SECT_WATER_NOSWIM)
		send_to_char("The water below is frozen solid.\n\r",ch);
	    else
		send_to_char("The area is coated by a thin layer of ice.\n\r",ch);
	}

        if (room_is_affected(ch->in_room, gsn_plantgrowth))
	{
	    count = 1;
	    send_to_char("An unusual amount of wild growth fills the area.\n\r", ch);
	}

	if (room_is_affected(ch->in_room, gsn_mushroomcircle))
	{
            send_to_char("A large circle of mushrooms occupies the centre of this area.\n\r", ch);
	    count = 1;
	}

	if (room_is_affected(ch->in_room, gsn_abominablerune))
	{
	    send_to_char("A black, twisted thing floats here, shaped in the form of some abominable rune.\n\r", ch);
	    count = 1;
	}

    if (room_is_affected(ch->in_room, gsn_heatmine))
    {
        send_to_char("A smoldering ring of flame burns quietly here, reeking faintly of brimstone.\n", ch);
        count = 1;
    }

    AFFECT_DATA * oriflamme(get_room_affect(ch->in_room, gsn_oriflamme));
    if (oriflamme != NULL)
    {
        send_to_char("A burning standard blazes here, the fire dazzling in its brightness.\n", ch);
        count = 1;

        // Check for same group
        CHAR_DATA * caster(static_cast<CHAR_DATA*>(oriflamme->point));
        if (verify_char_room(caster, ch->in_room) && is_same_group(caster, ch))
            send_to_char("Your heart swells with inspiration as the oriflamme spurs you on to greatness!\n", ch);
    }

    if (room_is_affected(ch->in_room, gsn_canticleofthelightbringer))
    {
        send_to_char("A brilliant golden light suffuses this place, dispersing all shadows.\n", ch);
        count = 1;
    }

    if (room_is_affected(ch->in_room, gsn_spectrallantern))
    {
        send_to_char("An eerie silver sphere hovers here, casting a ghostly light on your surroundings.\n", ch);
        count = 1;
    }

    if (room_is_affected(ch->in_room, gsn_steam))
    {
        send_to_char("The air here is filled with scalding hot steam.\n", ch);
        count = 1;
    }

    if (room_is_affected(ch->in_room, gsn_hoarfrost))
    {
        send_to_char("The ground has been coated in a thin layer of glittering white frost.\n", ch);
        count = 1;
    }

    if (room_is_affected(ch->in_room, gsn_sauna))
    {
        send_to_char("The air is filled with a light, warm mist.\n", ch);
        count = 1;
    }

	if (room_is_affected(ch->in_room, gsn_wallofwater))
	{
	    send_to_char("A massive wall of water fills the area.\n\r", ch);
	    count = 1;
	}

    if (room_is_affected(ch->in_room, gsn_flood))
    {
        if (ch->in_room->sector_type == SECT_UNDERWATER) send_to_char("A flood of water engulfs this place completely!\n", ch);
        else send_to_char("Several feet of water flood this place, obscuring the ground.\n", ch);
        count = 1;
    }

    if (room_is_affected(ch->in_room, gsn_contaminate))
    {
        send_to_char("The water here is streaked with a murky black substance.\n", ch);
        count = 1;
    }

    if (room_is_affected(ch->in_room, gsn_maelstrom))
    {
        send_to_char("The water here churns angrily with the power of a maelstrom!\n", ch);
        count = 1;
    }

    if (room_is_affected(ch->in_room, gsn_webofoame))
    {
        send_to_char("Gelatinous threads of dark energy have been spun into a sticky web here.\n", ch);
        count = 1;
    }

    if (room_is_affected(ch->in_room, gsn_grimseep))
    {
        send_to_char("A viscous black slime coats the ground thickly here.\n", ch);
        count = 1;
    }

    AFFECT_DATA * rift(get_room_affect(ch->in_room, gsn_stasisrift));
    if (rift != NULL && rift->modifier == 0)
    {
        send_to_char("A swirling portal of inky night looms ominously here.\n", ch);
        count = 1;
    }

    if (room_is_affected(ch->in_room, gsn_bierofunmaking))
    {
        send_to_char("A dark bier wrought of pure shadow drinks in the light here.\n", ch);
        count = 1;
    }

    if (room_is_affected(ch->in_room, gsn_glyphofentombment))
    {
        send_to_char("A complex glyph full of interlocking lines has been scribed onto the ground.\n", ch);
        count = 1;
    }

    if (room_is_affected(ch->in_room, gsn_latticeofstone))
    {
        send_to_char("A stone lattice has been inscribed on the ground, humming faintly with power.\n", ch);
        count = 1;
    }

    if (room_is_affected(ch->in_room, gsn_stonetomud))
    {
        send_to_char("A thick layer of mud coats the ground here.\n", ch);
        count = 1;
    }
    
    if (room_is_affected(ch->in_room, gsn_lavaforge))
    {
        send_to_char("A steady flow of lava runs from a rift in the earth through a channel, its heat powering a makeshift forge.\n", ch);
        count = 1;
    }

    if (room_is_affected(ch->in_room, gsn_pillarofsparks))
    {
        send_to_char("A solid metal pillar stands here, charged with crackling energy.\n", ch);
        count = 1;
    }

    if (room_is_affected(ch->in_room, gsn_kaagnsplaguestone) && number_percent() <= get_skill(ch, gsn_stonecraft))
    {
        send_to_char("The ground has been tainted, its stone rife with disease.\n", ch);
        count = 1;
    }

    if (room_is_affected(ch->in_room, gsn_stonehaven))
    {
        send_to_char("This place seems to almost hum with a slow, heavy power.\n", ch);
        count = 1;
    }

    if (room_is_affected(ch->in_room, gsn_clingingfog))
    {
        send_to_char("A sparkling, clinging fog hangs in the air.\n", ch);
        count = 1;
    }

    if (room_is_affected(ch->in_room, gsn_figmentscage))
    {
        if ((!IS_NPC(ch) && BIT_GET(ch->pcdata->traits, TRAIT_CYNIC)) ||  number_percent() <= get_skill(ch, gsn_disillusionment))
            send_to_char("(Illusion) Thick walls of shimmering, translucent energy seal off all the exits.\n", ch);
        else
            send_to_char("Thick walls of shimmering, translucent energy seal off all the exits.\n", ch);

        count = 1;
    }

    if (room_is_affected(ch->in_room, gsn_sandstorm))
    {
        send_to_char("A swirling, stinging torrent of wind and sand rages through this place!\n", ch);
        count = 1;
    }

    for (AFFECT_DATA * twister(get_room_affect(ch->in_room, gsn_unleashtwisters)); twister != NULL; twister = get_room_affect(ch->in_room, gsn_unleashtwisters, twister))
    {
        send_to_char("A spinning cyclone tears through here, leaving behind a trail of destruction!\n", ch);
        count = 1;
    }

    if (area_is_affected(ch->in_room->area, gsn_mirage))
    {
        send_to_char("The paths from this place shimmer and twist.\n", ch);
        count = 1;
    }

    if (room_is_affected(ch->in_room, gsn_sparkingcloud))
    {
        send_to_char("A thick, dark cloud hums here, sparking with energy!\n", ch);
        count = 1;
    }

    if (room_is_affected(ch->in_room, gsn_mistsofarcing))
    {
        send_to_char("A fine, sparkling mist hangs over this place.\n", ch);
        count = 1;
    }

    AFFECT_DATA * channelWind(get_room_affect(ch->in_room, gsn_channelwind));
    if (channelWind != NULL)
    {
        std::ostringstream mess;
        mess << "A rush of wind is surging " << Direction::NameFor(static_cast<Direction::Value>(channelWind->modifier)) << " here, focused by a channel of stone.\n";
        send_to_char(mess.str().c_str(), ch);
        count = 1;
    }

    if (room_is_affected(ch->in_room, gsn_boilseas))
    {
        send_to_char("The water here is boiling hot!\n", ch);
        count = 1;
    }

    if (room_is_affected(ch->in_room, gsn_glyphofulyon))
    {
        send_to_char("A simple glyph has been traced on the ground here, pulsing a vibrant blue.\n", ch);
        count = 1;
    }

    if (room_is_affected(ch->in_room, gsn_markofthekaceajka))
    {
        send_to_char("An icy rune on the ground glows a chill blue.\n", ch);
        count = 1;
    }
	
	if (room_is_affected(ch->in_room, gsn_etherealblaze))
	{
	    send_to_char("Eerie flames lick about the surroundings.\n\r", ch);
	    count = 1;
	}

	if (room_is_affected(ch->in_room, gsn_circleofstones) && IS_PAFFECTED(ch, AFF_SHARP_VISION))
	{
	    send_to_char("A circle of stones is carefully concealed among the foliage.\n\r", ch);
	    count = 1;
	}

	if ((is_affected(ch,gsn_eyesoftheforest)
	  || is_affected(ch,gsn_shurangaze)
	  || is_affected(ch,gsn_moonsight)) && ch->in_room->herb_type > 0)
	{
	    send_to_char("You spot a useful herb in the foliage.",ch);
	    count = 1;
	}

        if (room_is_affected(ch->in_room, gsn_encamp) && IS_PAFFECTED(ch,AFF_SHARP_VISION))
	{
	    send_to_char("(Camouflaged) This area looks prepared for camping.\n\r",ch);
	    count = 1;
	}
	if (room_is_affected(ch->in_room, gsn_globedarkness) 
	  && (IS_PAFFECTED(ch, AFF_SENSORY_VISION)
	    || ch->race == global_int_race_shuddeni
	    || get_modifier(ch->in_room->affected,gsn_globedarkness) == ch->id))
	{
	    send_to_char("Unnatural darkness shrouds this place.\n\r",ch);
	    count = 1;
	}
	
	if (room_is_affected(ch->in_room, gsn_cloudkill))
	{
	    send_to_char("Poison gas fills the air.\n\r",ch);
	    count = 1;
	}

    if (area_is_affected(ch->in_room->area, gsn_barrowmist))
    {
        send_to_char("Dark mists swirl about you, reeking of decay.\n", ch);
        count = 1;
    }

    // Show weave lines (the called function will determine whether to actually show them)
    Weave::ShowWeave(*ch, *ch->in_room);

// where my bitches at?
	AFFECT_DATA *paf;
	TRACK_DATA *pTrack;
	if ((paf = affect_find(ch->affected,gsn_huntersense)) != NULL)
	{
	    if (!ch->in_room || !(pTrack = ch->in_room->tracks))
		send_to_char("You see no tracks here.\n\r",ch);
	    else
	      if (!(ch->in_room->sector_type == SECT_CITY 
	      || ch->in_room->sector_type == SECT_INSIDE
	      || ch->in_room->sector_type == SECT_WATER_NOSWIM
	      || ch->in_room->sector_type == SECT_WATER_SWIM
	      || ch->in_room->sector_type == SECT_UNDERWATER))
		if ((char *)(paf->point)!=NULL)
		{
		    for (;pTrack;pTrack = pTrack->next)
		    {
			if (!pTrack->valid)
			    continue;
			if (!pTrack->ch)
			    continue;
		    	if (!str_prefix((char *)(paf->point),pTrack->ch->name))
			    break;
		    }
		
		    if (pTrack)
			if (number_percent() < get_skill(ch,gsn_huntersense))
			{
			    check_improve(ch,NULL,gsn_huntersense,TRUE,1);
				    
			    switch(pTrack->direction)
		 	    {
			        case 0:
				    act("$N left to the north.",ch,NULL,pTrack->ch,TO_CHAR); count=1;break;
			        case 1:
				    act("$N left to the east.",ch,NULL,pTrack->ch,TO_CHAR); count=1;break;
			        case 2:
				    act("$N left to the south.",ch,NULL,pTrack->ch,TO_CHAR); count=1;break;
			        case 3:
				    act("$N left to the west.",ch,NULL,pTrack->ch,TO_CHAR); count=1;break;
			        case 4:
				    act("$N left heading up.",ch,NULL,pTrack->ch,TO_CHAR); count=1;break;
			        case 5:
				    act("$N left heading down.",ch,NULL,pTrack->ch,TO_CHAR); count=1;break;
			    }
			}
		}
		else
		{
		    for (;pTrack;pTrack=pTrack->next)
		    {
			if (!pTrack->valid)
			    continue;
			if (!pTrack->ch)
			    continue;
			if (IS_NPC(pTrack->ch))
			    continue;
			if (pTrack && number_percent() < get_skill(ch,gsn_huntersense))
			    check_improve(ch,pTrack->ch,gsn_huntersense,TRUE,1);
			else
			    continue;	
			switch(pTrack->direction)
			{
			    case 0:
				act("$N left to the north.",ch,NULL,pTrack->ch,TO_CHAR); count=1;break;
			    case 1:
				act("$N left to the east.",ch,NULL,pTrack->ch,TO_CHAR); count=1;break;
			    case 2:
				act("$N left to the south.",ch,NULL,pTrack->ch,TO_CHAR); count=1;break;
			    case 3:
				act("$N left to the west.",ch,NULL,pTrack->ch,TO_CHAR); count=1;break;
			    case 4:
				act("$N left heading up.",ch,NULL,pTrack->ch,TO_CHAR); count=1;break;
			    case 5:
				act("$N left heading down.",ch,NULL,pTrack->ch,TO_CHAR); count=1;break;
			}
		    }
		}
	}
	
	if ((paf = affect_find(ch->affected,gsn_pursuit)) != NULL)
	{
	    if (!ch->in_room || !(pTrack = ch->in_room->tracks))
		send_to_char("You see no tracks here.\n\r",ch);
	    else
	      if (ch->in_room->sector_type == SECT_CITY 
	      || ch->in_room->sector_type == SECT_INSIDE
	      || ch->in_room->sector_type == SECT_ROAD)
	      {
		if ((char *)(paf->point)!=NULL)
		{
		    for (;pTrack;pTrack = pTrack->next)
		    {
			if (!pTrack->valid)
			    continue;
			if (!pTrack->ch)
			    continue;
		    	if (!str_prefix((char *)(paf->point),pTrack->ch->name))
			    break;
		    }
		
		    if (pTrack)
			if (number_percent() < get_skill(ch,gsn_pursuit))
			{
			    check_improve(ch,NULL,gsn_pursuit,TRUE,1);
				    
			    switch(pTrack->direction)
		 	    {
			        case 0:
				    act("$N left to the north.",ch,NULL,pTrack->ch,TO_CHAR); count=1;break;
			        case 1:
				    act("$N left to the east.",ch,NULL,pTrack->ch,TO_CHAR); count=1;break;
			        case 2:
				    act("$N left to the south.",ch,NULL,pTrack->ch,TO_CHAR); count=1;break;
			        case 3:
				    act("$N left to the west.",ch,NULL,pTrack->ch,TO_CHAR); count=1;break;
			        case 4:
				    act("$N left heading up.",ch,NULL,pTrack->ch,TO_CHAR); count=1;break;
			        case 5:
				    act("$N left heading down.",ch,NULL,pTrack->ch,TO_CHAR); count=1;break;
			    }
			}
		}
		else
		{
		    for (;pTrack;pTrack=pTrack->next)
		    {
			if (!pTrack->valid)
			    continue;
			if (!pTrack->ch)
			    continue;
			if (IS_NPC(pTrack->ch))
			    continue;
			if (pTrack && number_percent() < get_skill(ch,gsn_pursuit))
			    check_improve(ch,pTrack->ch,gsn_pursuit,TRUE,1);
			else
			    continue;	
			switch(pTrack->direction)
			{
			    case 0:
				act("$N left to the north.",ch,NULL,pTrack->ch,TO_CHAR); count=1;break;
			    case 1:
				act("$N left to the east.",ch,NULL,pTrack->ch,TO_CHAR); count=1;break;
			    case 2:
				act("$N left to the south.",ch,NULL,pTrack->ch,TO_CHAR); count=1;break;
			    case 3:
				act("$N left to the west.",ch,NULL,pTrack->ch,TO_CHAR); count=1;break;
			    case 4:
				act("$N left heading up.",ch,NULL,pTrack->ch,TO_CHAR); count=1;break;
			    case 5:
				act("$N left heading down.",ch,NULL,pTrack->ch,TO_CHAR); count=1;break;
			}
		    }
		}
	      }
	      else if (ch->in_room->sector_type == SECT_FIELD
		|| ch->in_room->sector_type == SECT_FOREST
		|| ch->in_room->sector_type == SECT_HILLS
		|| ch->in_room->sector_type == SECT_MOUNTAIN
		|| ch->in_room->sector_type == SECT_DESERT
		|| ch->in_room->sector_type == SECT_UNDERGROUND
		|| ch->in_room->sector_type == SECT_SWAMP)
	      {
		for (;pTrack;pTrack = pTrack->next)
		{
		    if (!pTrack->valid)
			continue;
		    if (!pTrack->ch)
			continue;
		    send_to_char("You notice tracks here.\n\r",ch);
		    break;
		}		
	      }
		
	}
        if ( (!IS_NPC(ch) && IS_SET(ch->act, PLR_AUTOEXIT))
	|| (IS_NPC(ch) && ch->desc->original &&
	IS_SET(ch->desc->original->act, PLR_AUTOEXIT)) )
	{
	    if (count==1) send_to_char("\n\r",ch);
            do_exits( ch, "auto" );
	}

      }
	
	show_list_to_char( ch->in_room->contents, ch, FALSE, FALSE );
	show_char_to_char( ch->in_room->people,   ch );
	if (is_affected(ch, gsn_lovepotion))
	{
		for (charmer = ch->in_room->people; charmer != NULL; charmer = charmer->next_in_room)
		{
			if (charmer == NULL) break;
			if (IS_NPC(charmer) || ABS(ch->level - charmer->level) > 8 || !can_see(ch, charmer)) continue;
			if ((IS_MALE(ch) && IS_FEMALE(charmer)) || (IS_FEMALE(ch) && IS_MALE(charmer)))
			{
				spell_charm_person(gsn_charm_person, 70, charmer, ch, 0);
				affect_strip(ch, gsn_lovepotion);
			}
		}
	}
	    
	return;
    }

    if ( !str_cmp( arg1, "i" ) || !str_cmp(arg1, "in")  || !str_cmp(arg1,"on"))
    {
	/* 'look in' */
	if ( arg2[0] == '\0' )
	{
	    send_to_char( "Look in what?\n\r", ch );
	    return;
	}

	if ( ( obj = get_obj_here( ch, arg2 ) ) == NULL )
	{
	    send_to_char( "You do not see that here.\n\r", ch );
	    return;
	}

	switch ( obj->item_type )
	{
	default:
	    send_to_char( "That is not a container.\n\r", ch );
	    break;

	case ITEM_DRINK_CON:
	    if ( obj->value[1] <= 0 )
	    {
		send_to_char( "It is empty.\n\r", ch );
		break;
	    }

	    sprintf( buf, "It's %sfilled with a %s liquid.\n\r",
		obj->value[1] <     obj->value[0] / 4
		    ? "less than half-" :
		obj->value[1] < 3 * obj->value[0] / 4
		    ? "about half-"     : "more than half-",
		liq_table[obj->value[2]].liq_color
		);

	    send_to_char( buf, ch );
	    break;

	case ITEM_CONTAINER:
	    if ( IS_SET(obj->value[1], CONT_CLOSED) )
	    {
		send_to_char( "It is closed.\n\r", ch );
		break;
	    }

	case ITEM_CORPSE_NPC:
	case ITEM_CORPSE_PC:
	    act( "$p holds:", ch, obj, NULL, TO_CHAR );
	    show_list_to_char( obj->contains, ch, TRUE, TRUE );
	    break;
	}
	return;
    }

    if ( ( victim = get_char_room( ch, arg1 ) ) != NULL )
    {
	if ((arg2[0] != '\0') && !IS_NPC(victim))
	{
	    EXTRA_DESCR_DATA *ed;

	    for (ed = victim->pcdata->extra_descr; ed; ed = ed->next)
		if (is_name(arg2, ed->keyword))
		{
    		    if ( can_see( victim, ch ) )
    		    {
			if (ch == victim)
	    		    act( "$n looks at $mself.",ch,NULL,NULL,TO_ROOM);
			else
			{
	    		    act( "$n looks at you.", ch, NULL, victim, TO_VICT    );
	    		    act( "$n looks at $N.",  ch, NULL, victim, TO_NOTVICT );
			}
    		    }

		    send_to_char(ed->description, ch);
		    return;
		}

	    sprintf(buf, "You see no such distinguishing feature upon %s.\n\r", PERS(victim, ch));
	    send_to_char(buf, ch);
	    return;
	}

	show_char_to_char_1( victim, ch );
	return;
    }
    count = 0;
    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
	
	if ( can_see_obj(ch, obj))
	{  /* player can see object */
	    pdesc = get_extra_descr( arg3, obj->extra_descr );
	    if ( pdesc != NULL )
	    	if (++count == number)
	    	{
		    if (obj->item_type == ITEM_WRITING)
		    {
			if (obj->value[0] == 1)
			{
			    one_argument(obj->name, buf);
			    strcat(buf, "1");
			    if (!str_cmp(arg3, buf) || (get_extra_descr(buf, obj->extra_descr) != NULL))
				send_to_char("Writing has been scribed upon the page.\n\r", ch);
			    else
				send_to_char(pdesc, ch);
			}
			else
			{
			    sprintf(buf, "It contains %d pages.\n\r", obj->value[0]);
			    send_to_char(buf, ch);
			}
		    }
		    else
			send_to_char( pdesc, ch );
			
		    return;
	    	}
	    	else continue;

 	    pdesc = get_extra_descr( arg3, obj->pIndexData->extra_descr );
 	    if ( pdesc != NULL )
 	    	if (++count == number)
 	    	{	
		    if (obj->item_type == ITEM_WRITING)
		    {
			if (obj->value[0] == 1)
			{
			    one_argument(obj->name, buf);
			    strcat(buf, "1");
			    if (!str_cmp(arg3, buf) || (get_extra_descr(buf, obj->extra_descr) != NULL))
				send_to_char("Writing has been scribed upon the page.\n\r", ch);
			    else
				send_to_char(pdesc, ch);
			}
			else
			{
			    sprintf(buf, "It contains %d pages.\n\r", obj->value[0]);
			    send_to_char(buf, ch);
			}
		    }
		    else
			send_to_char(pdesc, ch);

		    return;
	     	}
		else continue;

	    if ( is_name(arg3, obj->name) )
	    	if (++count == number)
	    	{
	    	    send_to_char( obj->description, ch );
	    	    send_to_char( "\n\r",ch);

		    if (obj->item_type == ITEM_WRITING)
			if (obj->value[0] == 1)
			{
			    sprintf(buf, "%s1", obj->name);
			    if ((pdesc = get_extra_descr(buf, obj->extra_descr)) != NULL)
				send_to_char("Writing has been scribed upon the page.\n\r", ch);
			}
			else
			{
			    sprintf(buf, "It contains %d pages.\n\r", obj->value[0]);
			    send_to_char(buf, ch);
			}

		    return;
		}
	}
    }

    for ( obj = ch->in_room->contents; obj != NULL; obj = obj->next_content )
    {
	if (obj->pIndexData->vnum == OBJ_VNUM_RUNIC_MESSAGE
	&& ch->class_num != global_int_class_watcher)
	{
	  send_to_char("You can't decipher it.\n\r", ch);
	  continue;
	}

	if (can_see_obj( ch, obj ))
	{
	    pdesc = get_extra_descr( arg3, obj->extra_descr );
	    if ( pdesc != NULL )
	    	if (++count == number)
	    	{
		    send_to_char( pdesc, ch );
		    return;
	    	}

	    pdesc = get_extra_descr( arg3, obj->pIndexData->extra_descr );
	    if ( pdesc != NULL )
	    	if (++count == number)
	    	{
		    send_to_char( pdesc, ch );
		    return;
	    	}

	    if (is_name(arg3, obj->name))
		if (++count == number)
		{
		    send_to_char( obj->description, ch );
		    send_to_char("\n\r",ch);
		    return;
		}
	}
    }

    pdesc = get_extra_descr(arg3,ch->in_room->extra_descr);
    if (pdesc != NULL)
    {
	if (++count == number)
	{
	    send_to_char(pdesc,ch);
	    return;
	}
    }
    
    if (count > 0 && count != number)
    {
    	if (count == 1)
    	    sprintf(buf,"You only see one %s here.\n\r",arg3);
    	else
    	    sprintf(buf,"You only see %d of those here.\n\r",count);
    	
    	send_to_char(buf,ch);
    	return;
    }

         if ( !str_cmp( arg1, "n" ) || !str_cmp( arg1, "north" ) ) door = 0;
    else if ( !str_cmp( arg1, "e" ) || !str_cmp( arg1, "east"  ) ) door = 1;
    else if ( !str_cmp( arg1, "s" ) || !str_cmp( arg1, "south" ) ) door = 2;
    else if ( !str_cmp( arg1, "w" ) || !str_cmp( arg1, "west"  ) ) door = 3;
    else if ( !str_cmp( arg1, "u" ) || !str_cmp( arg1, "up"    ) ) door = 4;
    else if ( !str_cmp( arg1, "d" ) || !str_cmp( arg1, "down"  ) ) door = 5;
    else
    {
	send_to_char( "You do not see that here.\n\r", ch );
	return;
    }

    /* 'look direction' */

    if ( ( pexit = ch->in_room->exit[door] ) == NULL )
    {
	send_to_char( "Nothing special there.\n\r", ch );
	return;
    }

    if ( pexit->description != NULL && pexit->description[0] != '\0' )
	send_to_char( pexit->description, ch );
    else
	send_to_char( "Nothing special there.\n\r", ch );

    if (pexit->exit_info & EX_SECRET)
	return;

    if (pexit->exit_info & EX_TRIPWIRE)
	send_to_char("You see a thin wire stretching along in that direction.\n\r", ch);

    if ( pexit->keyword    != NULL
    &&   pexit->keyword[0] != '\0'
    &&   pexit->keyword[0] != ' ' )
    {
	if ( IS_SET(pexit->exit_info, EX_CLOSED) )
	{
	    act( "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR );
	}
	else if ( IS_SET(pexit->exit_info, EX_ISDOOR) )
	{
	    act( "The $d is open.",   ch, NULL, pexit->keyword, TO_CHAR );
	}
    }

    return;
}

/* RT added back for the hell of it */
void do_read (CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_INPUT_LENGTH * 2];
    char one_name[MAX_INPUT_LENGTH];
    LANG_STRING *trans_new = NULL;
    char *translated;
    OBJ_DATA *obj;
    EXTRA_DESCR_DATA *ed;
    int page = 1, dlen, i;
    int newlang = 0;
    int curblock = 0;
    int orig_lang = ch->speaking;
    unsigned int offset;

    argument = one_argument(argument, arg);

    if ((obj = get_obj_carry(ch, arg, ch)) != NULL)
    {
	if (obj->item_type != ITEM_WRITING)
	{
	    do_look(ch, arg);
	    return;
	}

	if (obj->value[0] > 1)
	{
	    if (argument[0] == '\0' || !is_number(argument))
	    {
		sprintf(buf, "You must specify the page number.  That writing contains %d pages.\n\r", obj->value[0]);
		send_to_char(buf, ch);
		return;
	    }

	    page = atoi(argument);

	    if ((page < 1) || (page > obj->value[0]))
	    {
		sprintf(buf, "Invalid page number.  That writing contains %d pages.\n\r", obj->value[0]);
		send_to_char(buf, ch);
		return;
	    }
	}

	one_argument(obj->name, one_name);

	sprintf(buf, "%s%d", one_name, page);

	for (ed = obj->extra_descr; ed; ed = ed->next)
	    if (!str_cmp(buf, ed->keyword))
		break;

	if (!ed)
	{
	    for (ed = obj->pIndexData->extra_descr; ed; ed = ed->next)
	        if (!str_cmp(buf, ed->keyword))
		    break;
	}

	if (!ed)
	{
	    if (obj->value[0] > 1)
		send_to_char("There is no writing upon that page.\n\r", ch);
	    else
		act("$p has not been written upon.", ch, obj, NULL, TO_CHAR);

	    return;
	}

	dlen = strlen(ed->description);

	for (i = 0; i < dlen; i++)
	{
	    if ((ed->description[i] == '{') && (ed->description[i+1] == 'l'))
	    {
		if (i > curblock)
		{
		   memset((void *) buf, 0, MAX_STRING_LENGTH);
		   strncpy(buf, &ed->description[curblock], UMIN(i - curblock, MAX_STRING_LENGTH - 1));
		   buf[MAX_STRING_LENGTH - 1] = '\0';
		   ch->speaking = newlang;
		   offset = 0;
		   while (offset < strlen(buf)) {
		       memset((void *) buf2, 0, MAX_INPUT_LENGTH * 2);
	               strncpy(buf2, &buf[offset], UMIN(strlen(buf) - offset, MAX_INPUT_LENGTH * 2 - 1));
		       buf2[MAX_INPUT_LENGTH * 2 - 1] = '\0';
		       trans_new = translate_spoken_new(ch, buf2);
		       trans_new->orig_string = buf2;
		       translated = translate_out_new(ch, trans_new);
		       send_to_char(translated, ch);
		       offset += MAX_INPUT_LENGTH * 2 - 1;
		   }
		}

		i += 2;

		newlang = 0;

		while (ed->description[i] != '}')
		{
		    if ((ed->description[i] >= '0') && (ed->description[i] <= '9'))
		    {
			newlang *= 10;
			newlang += ed->description[i] - '0';
		    }

		    i++;

		    if (ed->description[i] == '\0')
		    {
			if (trans_new)
			    free(trans_new);

			return;
		    }
		}

		i++;
		curblock = i;

		sprintf(buf, "[written in %s]\n\r", lang_data[newlang].name);
		send_to_char(buf, ch);
	    }
	}

	ch->speaking = newlang;
        offset = 0;
        while (offset < strlen(&ed->description[curblock])) {
	    memset((void *) buf2, 0, MAX_INPUT_LENGTH * 2);
            strncpy(buf2, &ed->description[curblock + offset], UMIN(strlen(&ed->description[curblock]) - offset, MAX_INPUT_LENGTH * 2 - 1));
            buf2[MAX_INPUT_LENGTH * 2 - 1] = '\0';
            trans_new = translate_spoken_new(ch, buf2);
            trans_new->orig_string = buf2;
            translated = translate_out_new(ch, trans_new);
            send_to_char(translated, ch);
            offset += MAX_INPUT_LENGTH * 2 - 1;
        }

	ch->speaking = orig_lang;

	if (trans_new)
	    free(trans_new);

	return;
    }

    do_look(ch,arg);
}

static void addAffectLines(DisplayPanel::Box & box, AFFECT_DATA * paf)
{
    std::ostringstream out;
    while (paf != NULL)
    {
        // Show a selected subset of affects
        switch (paf->location)
        {
            case APPLY_AC:              case APPLY_HITROLL:             case APPLY_DAMROLL:
            case APPLY_SAVES:           case APPLY_SAVING_ROD:          case APPLY_SIZE:
            case APPLY_SAVING_PETRI:    case APPLY_SAVING_BREATH:       case APPLY_SAVING_SPELL:
            case APPLY_STR:             case APPLY_DEX:                 case APPLY_INT:
            case APPLY_WIS:             case APPLY_CON:                 case APPLY_MANA:
            case APPLY_HIT:             case APPLY_MOVE:                case APPLY_CHR:
            case APPLY_MAXSTR:          case APPLY_MAXDEX:              case APPLY_MAXINT:
            case APPLY_MAXWIS:          case APPLY_MAXCON:              case APPLY_MAXCHR:
            case APPLY_LUCK:
            {
                out.str(""); 
                out << "Affects " << affect_loc_name(paf->location) << " by " << paf->modifier; 
                box.AddLine(out.str());
            }
            break;

            default: break;
        }

        paf = paf->next;
    }
}

void do_examine( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    if (is_affected(ch, gsn_paradise))
    {
	send_to_char("Visions of paradise block your examination.\n\r", ch);
	act("$n tries to blink away $s vision of paradise.", ch, NULL, NULL, TO_ROOM);
	return;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Examine what?\n\r", ch );
	return;
    }

    do_look( ch, arg );

    // Check for whether they are carrying the object
    if ((obj = get_obj_carry( ch, arg, ch )) != NULL)
    {
        // Since they are carrying it, inform them of basic properties normally ascertainable via score (it's ok to also show these for nodrop/noremove items)
        DisplayPanel::Box box;
        std::ostringstream out;
        out << "Weight: " << (obj->weight / 10) << "." << (obj->weight % 10);
        box.AddLine(out.str());

        switch (obj->item_type)
        {
            case ITEM_ARMOR: 
                {out.str(""); out << "AC vs piercing: " << obj->value[0]; box.AddLine(out.str());}
                {out.str(""); out << "AC vs bashing:  " << obj->value[1]; box.AddLine(out.str());}
                {out.str(""); out << "AC vs slashing: " << obj->value[2]; box.AddLine(out.str());}
                {out.str(""); out << "AC vs magic:    " << obj->value[3]; box.AddLine(out.str());}
                break;
        }

        // Write in the effects
        addAffectLines(box, obj->pIndexData->affected);
        addAffectLines(box, obj->affected);
        send_to_char("\nYou determine the following:\n", ch);
        send_to_char(DisplayPanel::Render(box).c_str(), ch);
        send_to_char("\n", ch);
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
	

	switch ( obj->item_type )
	{
	default:
	    break;
	
	case ITEM_JUKEBOX:
	    do_play(ch,"list");
	    break;

	case ITEM_MONEY:
	    if (obj->value[0] == 1)
		sprintf(buf, "Wow.  One %s coin.\n\r", coin_table[obj->value[1]].name);
	    else
		sprintf(buf, "There are %d %s coins in the pile.\n\r", obj->value[0], coin_table[obj->value[1]].name);
	    send_to_char(buf,ch);
	    break;

	case ITEM_WEAPON:
	    if (obj->condition >= 100)
		act("$p is in perfect condition.", ch, obj, NULL, TO_CHAR);
	    else if (obj->condition > 90)
		act("$p is in no need of repair.", ch, obj, NULL, TO_CHAR);
	    else if (obj->condition > 80)
		act("$p is looking slightly worn.", ch, obj, NULL, TO_CHAR);
	    else if (obj->condition > 60)
		act("$p is in some need of repair.", ch, obj, NULL, TO_CHAR);
	    else if (obj->condition > 35)
		act("$p needs to be repaired.", ch, obj, NULL, TO_CHAR);
	    else if (obj->condition > 10)
		act("$p is in desperate need of repair.", ch, obj, NULL, TO_CHAR);
	    else
		act("$p will break if not repaired soon.", ch, obj, NULL, TO_CHAR);
	    break;
		
	case ITEM_DRINK_CON:
	case ITEM_CONTAINER:
	case ITEM_CORPSE_NPC:
	case ITEM_CORPSE_PC:
	    sprintf(buf,"in %s",argument);
	    do_look( ch, buf );
	    break;
	case ITEM_POTIONCONTAINER:
	    int vials = obj->value[0] + obj->value[1] + obj->value[2] + obj->value[3] + obj->value[4];
	    if (vials == 0)
	    {
		sprintf(buf,"$p can hold %i vials, and contains none.",obj->weight);
	        act(buf,ch,obj,NULL,TO_CHAR);
	    }
	    else
	    {
		sprintf(buf,"$p can hold %i vials, and contains %i:",obj->weight,vials);
		act(buf,ch,obj,NULL,TO_CHAR);
		buf[0] = '\0';
		if (obj->value[0] > 0)
		    sprintf(buf,"%i explosive",obj->value[0]);
		if (obj->value[1] > 0)
		    if (buf[0] == '\0')
			sprintf(buf,"%i adhesive",obj->value[1]);
		    else
			sprintf(buf,"%s, %i adhesive",buf,obj->value[1]);
		if (obj->value[2] > 0)
		    if (buf[0] == '\0')
			sprintf(buf,"%i anesthetic",obj->value[2]);
		    else
			sprintf(buf,"%s, %i anesthetic",buf,obj->value[2]);
		if (obj->value[3] > 0)
		    if (buf[0] == '\0')
			sprintf(buf,"%i toxin",obj->value[3]);
		    else
			sprintf(buf,"%s, %i toxin",buf,obj->value[3]);
		if (obj->value[4] > 0)
		    if (buf[0] == '\0')
			sprintf(buf,"%i suppresive",obj->value[4]);
		    else
			sprintf(buf,"%s, %i suppressive",buf,obj->value[4]);
		strcat(buf,".\n\r");
		send_to_char(buf,ch);
	    }
	    break;    	    
	}
    }

    return;
}

/*
 * Thanks to Zrin for auto-exit part.
 */
void do_exits( CHAR_DATA *ch, char *argument )
{
    extern char *const dir_name[];
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    EXIT_DATA *pexit;
    bool found;
    bool fAuto;
    bool radiance = FALSE;
    int door, vertigo;

    fAuto = !str_cmp( argument, "auto" );

    if (check_blind(ch))
	return;
    else if (fAuto)
	sprintf(buf,"[Exits:");
    else if (get_trust(ch) > 51)
	sprintf(buf,"Obvious exits from room %d:\n\r",ch->in_room->vnum);
    else
	sprintf(buf,"Obvious exits:\n\r");

    if (!ch->in_room)
	return;

// brazen: Ticket #264: Visions now nullifies the exit command
    if (is_affected(ch, gsn_visions))
    {
        send_to_char("You see no escape from the horrors of your actions.\n\r",ch);
    	return;
    }

    if (area_is_affected(ch->in_room->area, gsn_brume) && number_percent() > get_skill(ch, gsn_stormmastery))
    {
        send_to_char("You cannot make out the way through the thick fog.\n", ch);
        return;
    }
    
    if (!can_see_in_room(ch, ch->in_room))
    {
	    if (!fAuto)
	        send_to_char("It is pitch black ...\n\r", ch);
    	return;
    }  

    vertigo = is_affected(ch, gsn_vertigo);
    AFFECT_DATA * fugue(get_affect(ch, gsn_fugue));
    if (fugue != NULL && fugue->location != APPLY_NONE && number_bits(6) == 0)
        vertigo = true;

    if (IS_OAFFECTED(ch, AFF_RADIANCE))
        radiance = TRUE;

    found = FALSE;
    for ( door = 0; door <= 5; door++ )
    {
        if ( ( pexit = ch->in_room->exit[door] ) != NULL
        &&   pexit->u1.to_room != NULL
        &&   can_see_room(ch,pexit->u1.to_room) 
            &&   !IS_SET(pexit->exit_info, EX_SECRET) )
        {
            found = TRUE;
            if ( fAuto )
            {
            strcat( buf, " " );
            if (IS_SET(pexit->exit_info, EX_CLOSED))
                strcat(buf,"(");
            if (vertigo && number_bits(2) == 0) 
                strcat( buf, dir_name[number_range(0, 5)] );
            else
                strcat( buf, dir_name[door] );
            if (IS_SET(pexit->exit_info, EX_CLOSED))
                strcat(buf,")");
            }
            else
            {
                sprintf( buf + strlen(buf), "%-5s - %s",
                    capitalize(dir_name[door]),
                    IS_SET(pexit->exit_info,EX_CLOSED) 
                    ? "A Closed Door"
                    : (!can_see_in_room(ch, pexit->u1.to_room) && !radiance)
                        ?  "Too dark to tell"
                        : pexit->u1.to_room->name
                    );
                if (IS_IMMORTAL(ch))
                    sprintf(buf + strlen(buf), 
                    " (room %d)\n\r",pexit->u1.to_room->vnum);
                else
                    sprintf(buf + strlen(buf), "\n\r");
            }
        }
        
        if ( pexit != NULL)
        {
            if (IS_SET(pexit->exit_info, EX_WALLOFFIRE))
            {
                sprintf(buf2, "A wall of fire impedes the %s exit.\n\n", dir_name[door]);
                send_to_char(buf2, ch);
            }

            if (IS_SET(pexit->exit_info, EX_WALLOFVINES))
            {
                sprintf(buf2, "A wall of vines covers the %s exit.\n\n", dir_name[door]);
                send_to_char(buf2, ch);
            }

            if (IS_SET(pexit->exit_info, EX_WALLED))
            {
                sprintf(buf2, "A wall of stone seals off the %s exit.\n\n", dir_name[door]);
                send_to_char(buf2, ch);
            }

            if (IS_SET(pexit->exit_info, EX_WEAVEWALL))
            {
                sprintf(buf2, "A shimmering golden mesh seals off the %s exit.\n\n", dir_name[door]);
                send_to_char(buf2, ch);
            }

            if (IS_SET(pexit->exit_info, EX_ICEWALL))
            {
                sprintf(buf2, "A thick pane of ice seals off the %s exit.\n\n", dir_name[door]);
                send_to_char(buf2, ch);
            }
        }
    }

    if ( !found )
        strcat( buf, fAuto ? " none" : "None.\n\r" );

    if ( fAuto )
        strcat( buf, "]\n\r" );
        
    send_to_char( buf, ch );
}

void do_worth( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
    {
        sprintf(buf,"You have %s.\n\r", coins_to_str(ch->coins));
        send_to_char(buf,ch);
        if (ch->nocked)
        {
            sprintf(buf, "You currently have %s nocked.\n\r", ch->nocked->short_descr);
            send_to_char(buf, ch);
        }
        return;
    }

    sprintf(buf, 
    "You have %s, and %d experience (%d exp to level).\n\r",
	coins_to_str(ch->coins),ch->exp, 
        exp_on_level(ch,ch->level+1)-ch->exp);
    send_to_char(buf,ch);
    if (ch->nocked)
    {
	sprintf(buf, "You currently have %s nocked.\n\r", ch->nocked->short_descr);
	send_to_char(buf, ch);
    }
    sprintf(buf, 
    "You have %d exploration points (%d ep to level).\n\r",
	ch->ep, ep_table[ch->level+1] - ch->ep > 0 ? ep_table[ch->level+1] - ch->ep : 0);
    send_to_char(buf,ch);

    sprintf(buf, "You are %s by your equipment.\n", Encumbrance::DescriptorFor(Encumbrance::LevelFor(*ch)));
    send_to_char(buf, ch);
}

void do_oldscore( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    int i;
    char *suf;
    int day_month = 1, day_year, mnum, total = 0;
    int day = (ch->id / ((PULSE_TICK * 2) / PULSE_PER_SECOND * NUM_HOURS_DAY));
    int bhour = (ch->id / ((PULSE_TICK * 2) / PULSE_PER_SECOND) % NUM_HOURS_DAY);

    if (IS_NPC(ch))
	return;
    sprintf( buf,
	"You are %s%s%s%s, level %d  (%d hours).\n\r",
	ch->name,
	IS_NPC(ch) ? "" : ch->pcdata->title,
        IS_NPC(ch) ? "" : ch->pcdata->extitle[0] == '\0' ? "" : ", ",
        IS_NPC(ch) ? "" : ch->pcdata->extitle[0] == '\0' ? "" : ch->pcdata->extitle,
	ch->level,
        ( ch->played + (int) (current_time - ch->logon) ) / 3600);
    send_to_char( buf, ch );

    if ( get_trust( ch ) != ch->level )
    {
        sprintf( buf, "You are trusted at level %d.\n\r", get_trust( ch ) );
        send_to_char( buf, ch );
    }

    /* must be run first to update age_group */
    get_age(ch);

    sprintf(buf, "You are %d years old, %s %s %s %s.\n\r",
	get_age(ch),
	ch->pcdata->age_group == AGE_NONE ? "youthful" : age_table[ch->pcdata->age_group].name,
	ch->sex == 0 ? "sexless" : ch->sex == 1 ? "male" : "female",
	race_table[ch->race].name,
	IS_NPC(ch) ? "mobile" : class_table[ch->class_num].name);
    send_to_char(buf, ch);

    day_year = (day % NUM_DAYS_YEAR) + 1;

    for (mnum = 0; month_table[mnum].name; mnum++)
    {
	total += month_table[mnum].num_days;
	if (day_year <= total)
	    break;
    }

    day_month = (day_year - (total - month_table[mnum].num_days));

    if ( day_month > 4 && day_month <  20 ) suf = "th";
    else if ( day_month % 10 ==  1        ) suf = "st";
    else if ( day_month % 10 ==  2        ) suf = "nd";
    else if ( day_month % 10 ==  3        ) suf = "rd";
    else                             	    suf = "th";

    sprintf( buf, "You were born at %d o'clock %s, %s, ",
	(bhour % (NUM_HOURS_DAY / 2) == 0) ? (NUM_HOURS_DAY / 2) : (bhour % (NUM_HOURS_DAY / 2)),
	(bhour >= (NUM_HOURS_DAY / 2)) ? "pm" : "am",
	 day_name[day % 7]);

    if (month_table[mnum].num_days > 1)
	sprintf(buf, "%s%d%s day of %s.\n\r", buf, day_month, suf,
	    month_table[mnum].name);
    else
	sprintf(buf, "%s%s.\n\r", buf, month_table[mnum].name);

    send_to_char(buf,ch);

    sprintf( buf,
	"You have %d/%d hit, %d/%d mana, %d/%d movement.\n\r",
	(IS_NAFFECTED(ch, AFF_ASHURMADNESS) || IS_AFFECTED(ch, AFF_BERSERK) || is_loc_not_affected(ch, gsn_anesthetize, APPLY_NONE))? 0 : ch->hit,  ch->max_hit,
	ch->mana, ch->max_mana,
	ch->move, ch->max_move);
    send_to_char( buf, ch );

    sprintf( buf,
	"You have %d practices and %d training sessions.\n\r",
	ch->practice, ch->train);
    send_to_char( buf, ch );

    sprintf( buf,
	"You are carrying %d/%d items with weight %ld/%d pounds.\n\r",
	ch->carry_number, Encumbrance::CarryCountCapacity(*ch),
	get_carry_weight(*ch) / 10L, Encumbrance::CarryWeightCapacity(*ch) /10 );
    send_to_char( buf, ch );

    sprintf( buf,
	"Str: %d(%d)  Int: %d(%d)  Wis: %d(%d)  Dex: %d(%d)  Con: %d(%d)   Chr: %d(%d)\n\r",
	ch->perm_stat[STAT_STR],
	get_curr_stat(ch,STAT_STR),
	ch->perm_stat[STAT_INT],
	get_curr_stat(ch,STAT_INT),
	ch->perm_stat[STAT_WIS],
	get_curr_stat(ch,STAT_WIS),
	ch->perm_stat[STAT_DEX],
	get_curr_stat(ch,STAT_DEX),
	ch->perm_stat[STAT_CON],
	get_curr_stat(ch,STAT_CON),
	ch->perm_stat[STAT_CHR],
	get_curr_stat(ch,STAT_CHR) );
    send_to_char( buf, ch );

    sprintf( buf,
	"You have scored %d exp, %d ep, and have %s.\n\r",
	ch->exp,  ch->ep, coins_to_str(ch->coins) );
    send_to_char( buf, ch );

    /* RT shows exp to level */
    if (!IS_NPC(ch) && ch->level < LEVEL_HERO)
    {
      sprintf (buf, 
	"You need %d exp and %d ep to level.\n\r",
        (exp_on_level(ch,ch->level+1)-ch->exp), ep_table[ch->level+1] - ch->ep > 0 ?
	ep_table[ch->level+1] - ch->ep : 0);
      send_to_char( buf, ch );
     }

    sprintf( buf, "Wimpy set to %d hit points.\n\r", ch->wimpy );
    send_to_char( buf, ch );

    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK]   > 10 )
	send_to_char( "You are drunk.\n\r",   ch );
    if (( !IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] <=  0 ) &&
        (ch->pcdata->condition[COND_THIRST] > -101))
	send_to_char( "You are thirsty.\n\r", ch );
    if (( !IS_NPC(ch) && ch->pcdata->condition[COND_HUNGER]   <=  0 ) &&
	(ch->pcdata->condition[COND_HUNGER] > -101)
	&& !is_affected(ch,gsn_sustenance))
        send_to_char( "You are hungry.\n\r",  ch );

    switch ( ch->position )
    {
    case POS_DEAD:     
	send_to_char( "You are DEAD!!\n\r",		ch );
	break;
    case POS_MORTAL:
	send_to_char( "You are mortally wounded.\n\r",	ch );
	break;
    case POS_INCAP:
	send_to_char( "You are incapacitated.\n\r",	ch );
	break;
    case POS_STUNNED:
	send_to_char( "You are stunned.\n\r",		ch );
	break;
    case POS_SLEEPING:
	send_to_char( "You are sleeping.\n\r",		ch );
	break;
    case POS_RESTING:
	send_to_char( "You are resting.\n\r",		ch );
	break;
    case POS_SITTING:
	send_to_char( "You are sitting.\n\r",		ch );
	break;
    case POS_STANDING:
	send_to_char( "You are standing.\n\r",		ch );
	break;
    case POS_FIGHTING:
	send_to_char( "You are fighting.\n\r",		ch );
	break;
    }


    /* print AC values */
    if (ch->level >= 25)
    {	
	sprintf( buf,"Armor: pierce: %d  bash: %d  slash: %d  magic: %d\n\r",
		 GET_AC(ch,AC_PIERCE),
		 GET_AC(ch,AC_BASH),
		 GET_AC(ch,AC_SLASH),
		 GET_AC(ch,AC_EXOTIC));
    	send_to_char(buf,ch);
    }

    for (i = 0; i < 4; i++)
    {
	char * temp;

	switch(i)
	{
	    case(AC_PIERCE):	temp = "piercing";	break;
	    case(AC_BASH):	temp = "bashing";	break;
	    case(AC_SLASH):	temp = "slashing";	break;
	    case(AC_EXOTIC):	temp = "magic";		break;
	    default:		temp = "error";		break;
	}
	
	send_to_char("You are ", ch);

	if      (GET_AC(ch,i) >  250 ) 
	    sprintf(buf,"hopelessly vulnerable to %s.\n\r",temp);
	else if (GET_AC(ch,i) > 200) 
	    sprintf(buf,"defenseless against %s.\n\r",temp);
	else if (GET_AC(ch,i) > 150)
	    sprintf(buf,"barely protected from %s.\n\r",temp);
	else if (GET_AC(ch,i) > 100)
	    sprintf(buf,"slightly armored against %s.\n\r",temp);
	else if (GET_AC(ch,i) > 50)
	    sprintf(buf,"somewhat armored against %s.\n\r",temp);
	else if (GET_AC(ch,i) > 0)
	    sprintf(buf,"armored against %s.\n\r",temp);
	else if (GET_AC(ch,i) > -50)
	    sprintf(buf,"well-armored against %s.\n\r",temp);
	else if (GET_AC(ch,i) > -100)
	    sprintf(buf,"very well-armored against %s.\n\r",temp);
	else if (GET_AC(ch,i) > -150)
	    sprintf(buf,"heavily armored against %s.\n\r",temp);
	else if (GET_AC(ch,i) > -200)
	    sprintf(buf,"superbly armored against %s.\n\r",temp);
	else if (GET_AC(ch,i) > -250)
	    sprintf(buf,"almost invulnerable to %s.\n\r",temp);
	else
	    sprintf(buf,"divinely armored against %s.\n\r",temp);

	send_to_char(buf,ch);
    }


    /* RT wizinvis and holy light */
    if ( IS_IMMORTAL(ch))
    {
      send_to_char("Holy Light: ",ch);
      if (IS_SET(ch->act,PLR_HOLYLIGHT))
        send_to_char("on",ch);
      else
        send_to_char("off",ch);
 
      if (ch->invis_level)
      {
        sprintf( buf, "  Invisible: level %d",ch->invis_level);
        send_to_char(buf,ch);
      }

      if (ch->incog_level)
      {
	sprintf(buf,"  Incognito: level %d",ch->incog_level);
	send_to_char(buf,ch);
      }
      send_to_char("\n\r",ch);
    }

    if ( ch->level >= 15 )
    {
	sprintf( buf, "Hitroll: %d  Damroll: %d.\n\r",
	    GET_HITROLL(ch), GET_DAMROLL(ch) );
	send_to_char( buf, ch );
    }
    
/*    if ( ch->level >= 10 )
    {
	sprintf( buf, "Alignment: %d.  ", ch->alignment );
	send_to_char( buf, ch );
    }*/

    send_to_char( "You are ", ch );
/*         if ( ch->alignment >  900 ) send_to_char( "angelic.\n\r", ch );
    else if ( ch->alignment >  700 ) send_to_char( "saintly.\n\r", ch );
    else if ( ch->alignment >  350 ) send_to_char( "good.\n\r",    ch );
    else if ( ch->alignment >  100 ) send_to_char( "kind.\n\r",    ch );
    else if ( ch->alignment > -100 ) send_to_char( "neutral.\n\r", ch );
    else if ( ch->alignment > -350 ) send_to_char( "mean.\n\r",    ch );
    else if ( ch->alignment > -700 ) send_to_char( "evil.\n\r",    ch );
    else if ( ch->alignment > -900 ) send_to_char( "demonic.\n\r", ch );
    else                             send_to_char( "satanic.\n\r", ch );*/


    if ( ch->alignment > 100 ) send_to_char( "pure of heart!", ch );
    else if ( ch->alignment > -100 ) send_to_char( "balanced.", ch );
    else send_to_char( "the embodiment of pure evil!", ch );

    if (is_affected(ch, gsn_obscurealign))
    {
	int talign = get_modifier(ch->affected, gsn_obscurealign);

	if (talign > 100) send_to_char("  (Your true alignment is good.)", ch);
	else if (talign > -100) send_to_char("  (Your true alignment is neutral.)", ch);
	else send_to_char("  (Your true alignment is evil.)", ch);
    }
  
    send_to_char( "\n\rYour ethos is ", ch);
	if (( ch->pcdata->ethos == ETH_LAWFUL ))
		send_to_char("lawful.\n\r",	ch );
	else if (( ch->pcdata->ethos == ETH_NEUTRAL ))
		send_to_char("neutral.\n\r",	ch );
	else if (( ch->pcdata->ethos == ETH_CHAOTIC ))
		send_to_char("chaotic.\n\r",	ch );

    if (ch->class_num < 12)
	{
   	 send_to_char( "You have chosen to follow the sphere of ", ch);
		if ((ch->pcdata->major_sphere == SPH_WATER ))
			send_to_char("water.\n\r",	ch );
		else if ((ch->pcdata->major_sphere == SPH_EARTH ))
			send_to_char("earth.\n\r",	ch );
		else if ((ch->pcdata->major_sphere == SPH_VOID ))
			send_to_char("void.\n\r",	ch );
		else if ((ch->pcdata->major_sphere == SPH_SPIRIT ))
			send_to_char("spirit.\n\r",	ch );
		else if ((ch->pcdata->major_sphere == SPH_AIR ))
			send_to_char("air.\n\r",	ch );
		else if ((ch->pcdata->major_sphere == SPH_FIRE ))
			send_to_char("fire.\n\r",	ch );
	if ((ch->class_num < 1))
		{
		send_to_char( "You have minored in the sphere of ", ch);
		if ((ch->pcdata->minor_sphere == SPH_WATER ))
			send_to_char("water.\n\r",	ch );
		else if ((ch->pcdata->minor_sphere == SPH_EARTH ))
			send_to_char("earth.\n\r",	ch );
		else if ((ch->pcdata->minor_sphere == SPH_VOID ))
			send_to_char("void.\n\r",	ch );
		else if ((ch->pcdata->minor_sphere == SPH_SPIRIT ))
			send_to_char("spirit.\n\r",	ch );
		else if ((ch->pcdata->minor_sphere == SPH_AIR ))
			send_to_char("air.\n\r",	ch );
		else if ((ch->pcdata->minor_sphere == SPH_FIRE ))
			send_to_char("fire.\n\r",	ch );
		}
	}
    if (ch->class_num == global_int_class_druid)
    {
	send_to_char("You draw power from ",ch);
	switch(ch->pcdata->minor_sphere)
	{
	    case SPH_FIRIEL: send_to_char("Firiel.\n\r",ch); break;
	    case SPH_LUNAR: send_to_char("the moons.\n\r",ch); break;
	    case SPH_GAMALOTH: send_to_char("Gamaloth.\n\r",ch); break;
	}
    }

    if (IS_SET(ch->comm,COMM_SHOW_AFFECTS))
	do_affects(ch,"");
}

void show_affects(CHAR_DATA *ch, CHAR_DATA * viewer, bool new_affect)
{
    static const int MinLevel = 0;
    AFFECT_DATA *paf, *paf_last = NULL;
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    bool aFound = FALSE;
   
    if ( ch->affected )
    {
        for ( paf = ch->affected; paf != NULL; paf = paf->next )
        {
            if (paf->type < 0 || paf->location == APPLY_HIDE)
                continue;

            if (!aFound)
            {
                if (new_affect)
                {
                    send_to_char("/--------------------------------------------------------------------------\\\n\r", viewer);
                    send_to_char("| You are affected by the following:                                       |\n\r", viewer);
                }
                else if (ch == viewer) send_to_char( "You are affected by the following:\n\r", viewer);
                else act("$N is affected by the following:", viewer, NULL, ch, TO_CHAR);

                aFound = TRUE;
            }

            if (paf_last != NULL && paf->type == paf_last->type && ((paf->type != gsn_generic && paf->type != gsn_sacrifice) || !str_cmp((char *) paf->point, (char *) paf_last->point)))
            {
                if (viewer->level >= MinLevel) sprintf( buf, "                      ");
                else continue;
            }
            else
            {
              if (skill_table[paf->type].spell_fun == spell_form)
            sprintf( buf, "Form : %-15s",
                ((paf->type == gsn_generic || paf->type == gsn_sacrifice)
                  && paf->point && ((char *) paf->point)[0] != '\0') ? (char *) paf->point : skill_table[paf->type].name );
                  else if (skill_table[paf->type].spell_fun == spell_null)
                    sprintf( buf, "Skill: %-15s", 
                ((paf->type == gsn_generic || paf->type == gsn_sacrifice)
                  && paf->point && ((char *) paf->point)[0] != '\0') ? (char *) paf->point : skill_table[paf->type].name );
                  else if (skill_table[paf->type].spell_fun == spell_focus)
                    sprintf( buf, "Focus: %-15s", 
                ((paf->type == gsn_generic || paf->type == gsn_sacrifice)
                  && paf->point && ((char *) paf->point)[0] != '\0') ? (char *) paf->point : skill_table[paf->type].name );
              else if (skill_table[paf->type].minimum_position == POS_RESTING)
                sprintf( buf, "Song : %-15s",
                ((paf->type == gsn_generic || paf->type == gsn_sacrifice)
                  && paf->point && ((char *) paf->point)[0] != '\0') ? (char *) paf->point : skill_table[paf->type].name );
              else 
                sprintf( buf, "Spell: %-15s",
                ((paf->type == gsn_generic || paf->type == gsn_sacrifice)
                  && paf->point && ((char *) paf->point)[0] != '\0') ? (char *) paf->point :  skill_table[paf->type].name );
            }

            if (viewer->level >= MinLevel)
            {
                if (paf->location == APPLY_NONE)
                    sprintf(buf, "%s: modifies something ", buf);
                else
                    sprintf( buf, "%s: modifies %s by %d ", buf, affect_loc_name( paf->location ), paf->modifier);

                if ( paf->duration == -1 )
                    sprintf( buf, "%spermanently", buf );
                else
                    sprintf( buf, "%sfor %d%s hours", buf, (paf->duration / 2), ((paf->duration % 2) == 0) ? "" : ".5");
            }

            if (new_affect)
            {
                sprintf(buf2, "| %-73s|\n\r", buf);
                send_to_char(buf2, viewer);
            }
            else
            {
                send_to_char( buf, viewer);
                send_to_char( "\n\r", viewer);
            }

            // Handle extra info for bloodpyre persistent effects
            if (paf->type == gsn_bloodpyre && paf->point != NULL)
            {
                PyreInfo * pyre(static_cast<PyreInfo*>(paf->point));
                if (pyre->effect() != PyreInfo::Effect_None)
                {
                    // There is a persistent effect, so show it
                    sprintf(buf, "       %-15s: %s (%s)\n", "", PyreInfo::effectName(pyre->effect()), (pyre->isEffectGreater() ? "greater" : "lesser"));
                    send_to_char(buf, viewer);
                }
            }

            // Handle empower phantasm
            if (paf->type == gsn_empowerphantasm)
            {
                MOB_INDEX_DATA * mobIndex(get_mob_index(paf->modifier));
                if (mobIndex != NULL && mobIndex->short_descr != NULL)
                {
                    sprintf(buf, "       %-15s: %s\n", "", mobIndex->short_descr);
                    send_to_char(buf, viewer);
                }
            }

            // Handle radiate aura
            if (paf->type == gsn_radiateaura)
            {
                sprintf(buf, "       %-15s: aura of %s\n", "", auraBaseName(static_cast<SpiritAura>(paf->modifier)));
                send_to_char(buf, viewer);
            }

            // Handle chant litany
            if (paf->type == gsn_chantlitany)
            {
                sprintf(buf, "       %-15s: litany of %s\n", "", litanyName(static_cast<LitanyType>(paf->modifier)));
                send_to_char(buf, viewer);
            }

            paf_last = paf;
        }

        if (new_affect)
            send_to_char("\\--------------------------------------------------------------------------/\n\r", viewer);
    }

    if (!aFound)
    {
    	if (ch == viewer) send_to_char("You are not affected by any spells.\n\r",viewer);
        else act("$N is not affected by any spells.", viewer, NULL, ch, TO_CHAR);
    }
}

void do_affects(CHAR_DATA *ch, char *argument)
{
    bool new_affect = FALSE;
    if ((argument[0] != '\0') && !str_cmp(argument, "newscore"))
        new_affect = TRUE;

    show_affects(ch, ch, new_affect); 
}

static const char * ac_level_identifier(int ac)
{
	if (ac > 300)  return "helpless";
	if (ac > 250)  return "defenseless";
	if (ac > 200)  return "very exposed";
	if (ac > 150)  return "exposed";
	if (ac > 100)  return "very vulnerable";
	if (ac > 50)   return "vulnerable";
	if (ac > 0)    return "unprotected";
	if (ac > -50)  return "barely protected";
	if (ac > -100) return "protected";
	if (ac > -150) return "very protected";
	if (ac > -180) return "slightly armored";
	if (ac > -210) return "armored";
	if (ac > -240) return "well-armored";
	if (ac > -270) return "very well-armored";
	if (ac > -300) return "highly armored";
	if (ac > -330) return "heavily armored";
	if (ac > -360) return "superbly armored";
	if (ac > -390) return "extremely armored";
	if (ac > -420) return "ironclad";
	if (ac > -450) return "steelclad";
	if (ac > -480) return "stoneclad";
	if (ac > -510) return "diamondclad";
	return "divinely armored";
}

void do_score(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char *suf;
    int day_month = 1, day_year, mnum, total = 0, i;
    int day = (ch->id / ((PULSE_TICK * 2) / PULSE_PER_SECOND * NUM_HOURS_DAY));
    int bhour = (ch->id / ((PULSE_TICK * 2) / PULSE_PER_SECOND) % NUM_HOURS_DAY);
    int acvalue;

    PC_DATA * pcdata(ch->pcdata);
    if (pcdata == NULL)
    {
        CHAR_DATA * original(find_bilocated_body(ch));
        if (original != NULL)
            pcdata = original->pcdata;
    }

    if (pcdata == NULL)
    {
    	send_to_char("NPCs cannot get a score listing.\n\r", ch);
	    return;
    }

    send_to_char("/--------------------------------------------------------------------------\\\n\r", ch);
    sprintf(buf2, "%s%s%s%s%s%s%s%s", 
	pcdata->pretitle[0] == '\0' ? "" : pcdata->pretitle,
	pcdata->pretitle[0] == '\0' ? "" : " ",
	ch->name,
	pcdata->surname[0] == '\0' ? "" : " ",
	pcdata->surname[0] == '\0' ? "" : pcdata->surname,
	pcdata->title,
	pcdata->extitle[0] == '\0' ? "" : ", ",
	pcdata->extitle[0] == '\0' ? "" : pcdata->extitle);

    sprintf(buf, "| %-72s |\n\r", buf2);

    send_to_char(buf, ch);
    send_to_char("\\--------------------------------------------------------------------------/\n\r", ch);
    send_to_char("/-------------\\/-----------------------------------------------------------\\\n\r", ch);

    if (get_trust(ch) == ch->level)
	sprintf(buf2, "%2d    ", ch->level);
    else
	sprintf(buf2, "%2d(%2d)", ch->level, get_trust(ch));
    sprintf(buf, "| Str: %2d(%2d) || Race: %-13sClass: %-18sLevel: %s |\n\r",
	get_curr_stat(ch, STAT_STR),
	ch->perm_stat[STAT_STR],
	race_table[ch->race].name,
	class_table[ch->class_num].name,
        buf2);
    send_to_char(buf, ch);

    sprintf(buf2, "%d (%s)", get_age(ch), 
	pcdata->age_group == AGE_NONE ? "youthful" : age_table[pcdata->age_group].name);
    sprintf(buf, "| Int: %2d(%2d) || Sex : %-13sAge  : %-18sHours: %-7d|\n\r",
	get_curr_stat(ch, STAT_INT),
	ch->perm_stat[STAT_INT],
	ch->sex == 0 ? "sexless" : ch->sex == 1 ? "male" : "female",
	buf2,
	((ch->played + (int) (current_time - ch->logon)) / 3600));
    send_to_char(buf, ch);
  

    sprintf(buf, "| Wis: %2d(%2d) ||                                                           |\n\r",
	get_curr_stat(ch, STAT_WIS),
	ch->perm_stat[STAT_WIS]);
    send_to_char(buf, ch);

    sprintf(buf2, "%d/%d", (IS_NAFFECTED(ch, AFF_ASHURMADNESS) || IS_AFFECTED(ch, AFF_BERSERK) || is_loc_not_affected(ch, gsn_anesthetize, APPLY_NONE)) ? 0 : ch->hit, ch->max_hit);
    sprintf(buf, "| Dex: %2d(%2d) || Hit : %-13sExperience : %-12dHitroll: %-5d|\n\r",
	get_curr_stat(ch, STAT_DEX),
	ch->perm_stat[STAT_DEX],
	buf2,
	ch->exp,
	GET_HITROLL(ch));
    send_to_char(buf, ch);

    sprintf(buf2, "%d/%d", ch->mana, ch->max_mana);
    sprintf(buf, "| Con: %2d(%2d) || Mana: %-13sExper/level: %-12dDamroll: %-5d|\n\r",
	get_curr_stat(ch, STAT_CON),
	ch->perm_stat[STAT_CON],
	buf2,
	exp_on_level(ch,ch->level+1)-ch->exp,
	GET_DAMROLL(ch));
    send_to_char(buf, ch);

    sprintf(buf2, "%d/%d", ch->move, ch->max_move);
    sprintf(buf, "| Chr: %2d(%2d) || Move: %-13sExploration: %-12dSaves  : %-5d|\n\r",
	get_curr_stat(ch, STAT_CHR),
	ch->perm_stat[STAT_CHR],
	buf2,
	ch->ep,
	get_save(ch));
    send_to_char(buf, ch);

    send_to_char("\\-------------/\\-----------------------------------------------------------/\n\r", ch);

    send_to_char("/-------------------------------------\\/-----------------------------------\\\n\r", ch);

    sprintf(buf, "| Armor                               || Pracs : %-6dTrains    : %-8d|\n\r",
	ch->practice,
	ch->train);
    send_to_char(buf, ch);

    acvalue = GET_AC(ch, AC_PIERCE);
    sprintf(buf2, "(%s)", ac_level_identifier(acvalue));
    sprintf(buf, "| Piercing: %-6d%-20s|| Weight: %-6ldMax Weight: %-8d|\n\r",
	acvalue,
	buf2,
	(get_carry_weight(*ch) / 10L),
	(Encumbrance::CarryWeightCapacity(*ch) / 10));
    send_to_char(buf, ch);

    acvalue = GET_AC(ch, AC_BASH);
    sprintf(buf2, "(%s)", ac_level_identifier(acvalue));
    sprintf(buf, "| Bashing : %-6d%-20s|| Items : %-6dMax Items : %-8d|\n\r",
	acvalue,
	buf2,
	ch->carry_number,
	Encumbrance::CarryCountCapacity(*ch));
    send_to_char(buf, ch);

    acvalue = GET_AC(ch, AC_SLASH);
    sprintf(buf2, "(%s)", ac_level_identifier(acvalue));
    sprintf(buf, "| Slashing: %-6d%-20s|| Wealth: %-26s|\n\r",
	acvalue,
	buf2,
	coins_to_sstr(ch->coins));
    send_to_char(buf, ch);
	
    acvalue = GET_AC(ch, AC_EXOTIC);
    sprintf(buf2, "(%s)", ac_level_identifier(acvalue));
    sprintf(buf, "| Magical : %-6d%-20s|| Align.: %-26s|\n\r",
	acvalue,
	buf2,
	(ch->alignment >= 100)  ? (pcdata->ethos == ETH_LAWFUL  ? "Lawful Good" :
				   pcdata->ethos == ETH_NEUTRAL ? "Neutral Good" :
						 		      "Chaotic Good") :
	(ch->alignment <= -100) ? (pcdata->ethos == ETH_LAWFUL  ? "Lawful Evil" :
				   pcdata->ethos == ETH_NEUTRAL ? "Neutral Evil" :
				                                      "Chaotic Evil") :
				  (pcdata->ethos == ETH_LAWFUL  ? "Lawful Neutral" :
				   pcdata->ethos == ETH_NEUTRAL ? "True Neutral" :
				                                      "Chaotic Neutral"));
    send_to_char(buf, ch);

    send_to_char("\\-------------------------------------/\\-----------------------------------/\n\r", ch);
    send_to_char("/--------------------------------------------------------------------------\\\n\r", ch);

    day_year = (day % NUM_DAYS_YEAR) + 1;

    for (mnum = 0; month_table[mnum].name; mnum++)
    {
        total += month_table[mnum].num_days;
        if (day_year <= total)
            break;
    }

    day_month = (day_year - (total - month_table[mnum].num_days));

    if ( day_month > 4 && day_month <  20 ) suf = "th";
    else if ( day_month % 10 ==  1        ) suf = "st";
    else if ( day_month % 10 ==  2        ) suf = "nd";
    else if ( day_month % 10 ==  3        ) suf = "rd";
    else                                    suf = "th";

    sprintf( buf2, "You were born at %d o'clock %s, %s, ",
        (bhour % (NUM_HOURS_DAY / 2) == 0) ? (NUM_HOURS_DAY / 2) : (bhour % (NUM_HOURS_DAY / 2)),
        (bhour >= (NUM_HOURS_DAY / 2)) ? "pm" : "am",
         day_name[day % 7]);

    if (month_table[mnum].num_days > 1)
        sprintf(buf2, "%s%d%s day of %s.", buf2, day_month, suf,
            month_table[mnum].name);
    else
        sprintf(buf2, "%s%s.", buf2, month_table[mnum].name);

    sprintf(buf, "| %-72s |\n\r", buf2);
    send_to_char(buf, ch);

    if (ch->wimpy)
    {
	sprintf(buf2, "Your wimpy value is set to %d hit points.", ch->wimpy);
	sprintf(buf, "| %-72s |\n\r", buf2);
	send_to_char(buf, ch);
    }

    if (pcdata->condition[COND_DRUNK] > 10)
	send_to_char("| You are drunk.                                                           |\n\r", ch);

    if ((pcdata->condition[COND_THIRST] <= 0) && (pcdata->condition[COND_THIRST] >= -100))
	send_to_char("| You are thirsty.                                                         |\n\r", ch);

    if (!is_affected(ch, gsn_sustenance) && (pcdata->condition[COND_HUNGER] != -2000))
    {
	if (pcdata->condition[COND_HUNGER] > get_max_hunger(ch, pcdata))
	    send_to_char("| You have gorged yourself.                                                |\n\r", ch);
	else if (pcdata->condition[COND_HUNGER] >= (get_max_hunger(ch, pcdata) * 19 / 20))
	    send_to_char("| You are full.                                                            |\n\r", ch);
	else if (pcdata->condition[COND_HUNGER] >= (get_max_hunger(ch, pcdata) * 9 / 10))
	    send_to_char("| You are nearly full.                                                     |\n\r", ch);
	else if (pcdata->condition[COND_HUNGER] <= -300)
	    send_to_char("| You are starving!                                                        |\n\r", ch);
	else if (pcdata->condition[COND_HUNGER] <= 0)
	    send_to_char("| You are hungry.                                                          |\n\r", ch);
	else if (pcdata->condition[COND_HUNGER] <= (get_max_hunger(ch, pcdata) * 1 / 10))
	    send_to_char("| You are getting hungry.                                                  |\n\r", ch);
    }

    // Encumbrance level
    sprintf(buf2, "You are %s by your equipment.", Encumbrance::DescriptorFor(Encumbrance::LevelFor(*ch)));
    sprintf(buf, "| %-72s |\n", buf2);
    send_to_char(buf, ch);

    if (IS_SET(ch->act, PLR_HOLYLIGHT))
	send_to_char("| You are currently in Holy Light mode.                                    |\n\r", ch);

    if (ch->invis_level)
    {
	sprintf(buf2, "You are currently WizInvis at level %d.", ch->invis_level);
	sprintf(buf, "| %-72s |\n\r", buf2);
	send_to_char(buf, ch);
    }

    if (ch->incog_level)
    {
	sprintf(buf2, "You are currently Incognito at level %d.", ch->incog_level);
	sprintf(buf, "| %-72s |\n\r", buf2);
	send_to_char(buf, ch);
    }
  
    if (ch->song)
    {
	sprintf(buf2, "You are currently performing '%s'.", skill_table[ch->song->type].name);
	sprintf(buf, "| %-72s |\n\r", buf2);
	send_to_char(buf, ch);
    }

    if (ch->harmony)
    {
	sprintf(buf2, "You are currently harmonizing '%s'.", skill_table[ch->harmony->type].name);
	sprintf(buf, "| %-72s |\n\r", buf2);
	send_to_char(buf, ch);
    }

    if (is_flying(ch))
    {
	sprintf(buf2, "You are flying (%s).", IS_AFFECTED(ch, AFF_FLYING) ? "magically" : "naturally");
	sprintf(buf, "| %-72s |\n\r", buf2);
	send_to_char(buf, ch);
    }

    if (is_affected(ch, gsn_obscurealign))
    {
        int talign = get_modifier(ch->affected, gsn_obscurealign);

        if (talign > 100)
	    send_to_char("| Your true alignment is good.                                              |\n\r", ch);
	else if (talign > -100)
	    send_to_char("| Your true alignment is neutral.                                           |\n\r", ch);
        else
	    send_to_char("| Your true alignment is evil.                                              |\n\r", ch);
    }

    for (i = 0; trait_table[i].name; i++)
	if (BIT_GET(pcdata->traits, trait_table[i].bit))
	{
	    sprintf(buf, "| %-72s |\n\r", trait_table[i].score_desc);
	    send_to_char(buf, ch);
	}

    if (ch->class_num == global_int_class_watcher)
	if (ch->level >= 40)
	{
	    sprintf(buf, "| %-72s |\n\r", "You can record information and place bounties on people.");
	    send_to_char(buf, ch);
	}
	else if (ch->level >= 20)
	{
	    sprintf(buf, "| %-72s |\n\r", "You can record information about people.");
	    send_to_char(buf, ch);
	}

    if (IS_SET(ch->act, PLR_NOPK))
        send_to_char("| You are currently not in the PK range of mortals.                        |\n\r", ch);

    send_to_char("\\--------------------------------------------------------------------------/\n\r", ch);

    if (IS_SET(ch->comm,COMM_SHOW_AFFECTS))
	do_affects(ch,"newscore");
}


char *	const	month_name	[] =
{
    "Winter", "the Winter Wolf", "the Frost Giant", "the Old Forces",
    "the Grand Struggle", "the Spring", "Nature", "Futility", "the Dragon",
    "the Sun", "the Heat", "the Battle", "the Dark Shades", "the Shadows",
    "the Long Shadows", "the Ancient Darkness", "the Great Evil"
};

/* if someone logs in as "webgettime", this function sends the
 * output of a do_time to the socket connected. There's a cgi
 * program that connects and logs in as that strictly for this
 * purpose. of course, it would be gentler on the stomach to just
 * do that with a separate program, but hi, I'm lazy!
 */
void do_descriptortime( DESCRIPTOR_DATA *d )
{
    extern char str_boot_time[];
    char buf[MAX_STRING_LENGTH];
    char *suf;
    int day_month = 1, day_year, mnum, total = 0;
    int day = (current_time / ((PULSE_TICK * 2) / PULSE_PER_SECOND * NUM_HOURS_DAY));

    	sprintf(buf,"Avendar started up at %s<br>The system time is %s<br>\n\n",
	    str_boot_time,
	    (char *) ctime( &current_time ));
       write_to_descriptor( d->descriptor, buf, 0 );

    day_year = (day % NUM_DAYS_YEAR) + 1;

    for (mnum = 0; month_table[mnum].name; mnum++)
    {
	total += month_table[mnum].num_days;
	if (day_year <= total)
	    break;
    }

    day_month = (day_year - (total - month_table[mnum].num_days));

    if ( day_month > 4 && day_month <  20 ) suf = "th";
    else if ( day_month % 10 ==  1        ) suf = "st";
    else if ( day_month % 10 ==  2        ) suf = "nd";
    else if ( day_month % 10 ==  3        ) suf = "rd";
    else                             	    suf = "th";

    sprintf( buf, "It is %d o'clock %s, %s, ",
	(time_info.hour % (NUM_HOURS_DAY / 2) == 0) ? (NUM_HOURS_DAY / 2) : (time_info.hour % (NUM_HOURS_DAY / 2)),
	(time_info.hour >= (NUM_HOURS_DAY / 2)) ? "pm" : "am",
	 day_name[day % 7]);

    if (month_table[mnum].num_days > 1)
	sprintf(buf, "%s%d%s day of %s.<br>\n\r", buf, day_month, suf,
	    month_table[mnum].name);
    else
	sprintf(buf, "%s%s.<br>\n\r", buf, month_table[mnum].name);

    write_to_descriptor( d->descriptor, buf, 0 );

    sprintf(buf, "The current season is %s.<br>\n\r", season_table[time_info.season].name);
    write_to_descriptor( d->descriptor, buf, 0 );

    if ((time_info.hour >= season_table[time_info.season].sun_down)
     || (time_info.hour < season_table[time_info.season].sun_up))
    {
	int size = get_moon_state(2, TRUE);

	if (get_moon_state(1, FALSE) >= 0)
	    sprintf(buf, "The bright moon of Lunus is %s<br>\n\r", moon_info[get_moon_state(1, FALSE)]);
	else
	    sprintf(buf, "Lunus cannot be seen in the sky.<br>\n\r"); 

	if (get_moon_state(2, FALSE) >= 0)
            sprintf(buf, "%sThe %s red form of Rhos is %s\n\r", buf,
	    	((size == MSIZE_TINY  ) ? "tiny"  :
	     	 (size == MSIZE_SMALL ) ? "small" :
	     	 (size == MSIZE_MEDIUM) ? "medium" :
	     	 (size == MSIZE_LARGE ) ? "large" :
	     	 (size == MSIZE_HUGE  ) ? "huge"  : "medium"), moon_info[get_moon_state(2, FALSE)]);
	else
	    strcat(buf, "Rhos cannot be seen in the sky.<br>\n\r");
    }
    else
	sprintf(buf, "The daylight prevents the distant moons from being seen.<br>\n\r");

    write_to_descriptor( d->descriptor, buf, 0 );

    return;
}

size_t calculate_total_days()
{
	return (current_time / ((PULSE_TICK * 2) / PULSE_PER_SECOND * NUM_HOURS_DAY));
}

size_t calculate_day_of_week(size_t totalDays)
{
	return (totalDays % 7);
}

size_t calculate_day_of_week()
{
	return calculate_day_of_week(calculate_total_days());
}

void do_immtime( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    sprintf(buf,"Hour: %d Half: %s Day: %d Week: %d\n\r",
      time_info.hour,time_info.half ? "30" : "00",time_info.day,time_info.week);
    send_to_char(buf,ch);  
    sprintf(buf,"Month: %d Season: %d Day of Year: %d Year: %d\n\r",
      time_info.month,time_info.season,time_info.day_year,time_info.year);
    send_to_char(buf,ch);
    sprintf(buf,"Lunus is %d, %d\n\r",time_info.phase_lunus,MSIZE_MEDIUM);
    send_to_char(buf,ch);
    sprintf(buf,"Rhos is %d, %d\n\r",time_info.phase_rhos,time_info.size_rhos);
    send_to_char(buf,ch);
}

void do_time( CHAR_DATA *ch, char *argument )
{
    extern char str_boot_time[];
    char buf[MAX_STRING_LENGTH];

    if (!str_cmp(argument, "system"))
    {
    	sprintf(buf,"Avendar started up at %sThe system time is %s",
	    str_boot_time,
	    (char *) ctime( &current_time ));

        send_to_char( buf, ch );
	return;
    }

    sprintf (buf, "It is %d:%d0%s, ",
	(time_info.hour == 0 ? 12 : (time_info.hour > 12 ? time_info.hour - 12 : time_info.hour)),
	(time_info.half ? 3 : 0),
	time_info.hour >= (NUM_HOURS_DAY / 2) ? "pm" : "am");
    strcpy(buf, display_date(buf));
    send_to_char(buf,ch);

    sprintf(buf, "The current season is %s.\n\r",
        season_table[time_info.season].name);
    send_to_char(buf,ch);

    if ((time_info.hour >= season_table[time_info.season].sun_down)
      || (time_info.hour < season_table[time_info.season].sun_up))
    {
        int size = get_moon_state(2, TRUE);
	
	if (get_moon_state(1, FALSE) >= 0)
	    sprintf(buf, "The bright moon of Lunus is %s\n\r",
	        moon_info[get_moon_state(1, FALSE)]);
	else
	    sprintf(buf, "Lunus cannot be seen in the sky.\n\r");
	
	if (get_moon_state(2, FALSE) >= 0)
	    sprintf(buf, "%sThe %s red form of Rhos is %s\n\r", buf,
	    ((size == MSIZE_TINY) ? "tiny" :
	    (size == MSIZE_SMALL) ? "small" :
	    (size == MSIZE_MEDIUM) ? "medium" :
	    (size == MSIZE_LARGE) ? "large" :
	    (size == MSIZE_HUGE) ? "huge" : "medium"),
            moon_info[get_moon_state(2, FALSE)]);
        else
	    strcat(buf, "Rhos cannot be seen in the sky.\n\r");

    }
    else
        sprintf(buf,"The daylight prevents you from seeing the distant moons.\n\r");
    send_to_char(buf,ch);

    return;
}

char * display_date (char *string)
{
    int day = time_info.day + 1;
    char * suf;

    if (day > 4 && day < 20 ) suf = "th";
    else if (day % 10 == 1) suf = "st";
    else if (day % 10 == 2) suf = "nd";
    else if (day % 10 == 3) suf = "rd";
    else suf = "th";

    sprintf(string, "%s%s, ", string, day_name[time_info.week]);

    if (month_table[time_info.month].num_days > 1)
        sprintf(string, "%s%d%s day of %s.\n\r", string, day, suf,
	  month_table[time_info.month].name);
    else
        sprintf(string, "%s%s.\n\r", string, 
	  month_table[time_info.month].name);

    return string;
}


void do_noweather( CHAR_DATA *ch, char *argument )
{
    if (IS_NPC(ch))
	return;

    if (IS_SET(ch->nact, PLR_NOWEATHER))
    {
	REMOVE_BIT(ch->nact, PLR_NOWEATHER);
	send_to_char("You will now receive weather updates.\n\r", ch);
    }
    else
    {
	SET_BIT(ch->nact, PLR_NOWEATHER);
	send_to_char("You will no longer receive weather updates.\n\r", ch);
    }

    return;
}
	

void do_weather( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

    if ( !IS_OUTSIDE(ch) )
    {
	send_to_char( "You can't see the weather indoors.\n\r", ch );
	return;
    }

    if (!ch->in_room || !ch->in_room->area)
	return;

    if (IS_SET(ch->in_room->room_flags,ROOM_NOWEATHER))
    {
	send_to_char("You can't determine the weather here.\n\r",ch);
	return;
    }

    if (ch->in_room->area->w_cur.temperature >= 32)
    sprintf(buf, "The temperature is unbearably hot!\n");
    else if (ch->in_room->area->w_cur.temperature >= 25)
	sprintf(buf, "The temperature is blisteringly hot.\n\r");
    else if (ch->in_room->area->w_cur.temperature >= 18)
	sprintf(buf, "The temperature is very hot.\n\r");
    else if (ch->in_room->area->w_cur.temperature >= 10)
	sprintf(buf, "The temperature is hot.\n\r");
    else if (ch->in_room->area->w_cur.temperature >= 5)
	sprintf(buf, "The temperature is warm.\n\r");
    else if (ch->in_room->area->w_cur.temperature > -5)
	sprintf(buf, "The temperature is mild.\n\r");
    else if (ch->in_room->area->w_cur.temperature > -10)
	sprintf(buf, "The temperature is cool.\n\r");
    else if (ch->in_room->area->w_cur.temperature > -18)
	sprintf(buf, "The temperature is cold.\n\r");
    else if (ch->in_room->area->w_cur.temperature > -25)
	sprintf(buf, "The temperature is bitingly cold.\n\r");
    else if (ch->in_room->area->w_cur.temperature > -32)
	sprintf(buf, "The temperature is bitterly frigid.\n\r");
    else
    sprintf(buf, "The temperature is unbearably cold!\n");

    send_to_char(buf, ch);

    if (silver_state == SILVER_FINAL)
	sprintf(buf, "Dark clouds dominate the sky, obscuring the heavens.\n\r");
    else if (ch->in_room->area->w_cur.cloud_cover == 0)
        sprintf(buf, "The sky is clear.\n\r");
    else if (ch->in_room->area->w_cur.cloud_cover < 20)
	sprintf(buf, "The sky is mostly clear.\n\r");
    else if (ch->in_room->area->w_cur.cloud_cover < 60)
	sprintf(buf, "The sky is partially covered by clouds.\n\r");
    else if (ch->in_room->area->w_cur.cloud_cover < 100)
	sprintf(buf, "The sky is mostly covered by clouds.\n\r");
    else
	sprintf(buf, "The sky is completely obscured by thick clouds.\n\r");

    const char * windDirName(Direction::SourceNameFor(Direction::ReverseOf(static_cast<Direction::Value>(ch->in_room->area->w_cur.wind_dir))));
    if (ch->in_room->area->w_cur.wind_mag == 0) strcat(buf, "The air is still as no wind blows.\n\r");
    else if (ch->in_room->area->w_cur.wind_mag < 15) sprintf(buf, "%sA gentle breeze blows %s.\n\r", buf, windDirName);
    else if (ch->in_room->area->w_cur.wind_mag < 40) sprintf(buf, "%sThe wind blows %s.\n\r", buf, windDirName);
    else if (ch->in_room->area->w_cur.wind_mag < 70) sprintf(buf, "%sA stiff wind blows %s.\n\r", buf, windDirName);
    else if (ch->in_room->area->w_cur.wind_mag < 90) sprintf(buf, "%sA roaring gale rushes %s.\n\r", buf, windDirName);
    else sprintf(buf, "%sA rushing wall of wind blows %s.\n\r", buf, windDirName);

    if (ch->in_room->area->w_cur.storm_str > 0)
	if (ch->in_room->area->w_cur.precip_type == 0)		/* Snow */
	    if (ch->in_room->area->w_cur.storm_str < 10)
	        strcat(buf, "An occasional snowflake falls here and there.\n\r");
	    else if (ch->in_room->area->w_cur.storm_str < 35)
		strcat(buf, "A light fall of snow comes from above.\n\r");
	    else if (ch->in_room->area->w_cur.storm_str < 65)
		strcat(buf, "Snow falls steadily from above.\n\r");
	    else if (ch->in_room->area->w_cur.storm_str < 90)
		strcat(buf, "A thick snowfall comes from above.\n\r");
	    else
		strcat(buf, "A blizzard of snow falls from above, obscuring vision.\n\r");
	else if (ch->in_room->area->w_cur.precip_type == 1)	/* Hail */
	    if (ch->in_room->area->w_cur.storm_str < 25)
		strcat(buf, "Little bits of ice, about the size of peas, fall from the sky.\n\r");
	    else if (ch->in_room->area->w_cur.storm_str < 50)
		strcat(buf, "Small pieces of ice, near the size of marbles, fall from above.\n\r");
	    else if (ch->in_room->area->w_cur.storm_str < 75)
		strcat(buf, "Large chunks of ice clatter to the ground, falling from the sky.\n\r");
	    else
		strcat(buf, "Huge chunks of ice hurtle to the ground, falling from the sky.\n\r");
	else						/* Rain */
	    if (ch->in_room->area->w_cur.storm_str < 10)
		strcat(buf, "A faint mist of rain falls from the sky.\n\r");
	    else if (ch->in_room->area->w_cur.storm_str < 35)
		strcat(buf, "Drizzling rain falls from above.\n\r");
	    else if (ch->in_room->area->w_cur.storm_str < 65)
		strcat(buf, "A steady flow of rain falls from the sky.\n\r");
	    else if (ch->in_room->area->w_cur.storm_str < 90)
		strcat(buf, "Rain falls in a heavy downpour.\n\r");
	    else
		strcat(buf, "Torrential sheets of rain fall from the sky.\n\r");

    if (ch->in_room->area->w_cur.lightning_str > 0)
	if (ch->in_room->area->w_cur.lightning_str < 10)
	    strcat(buf, "An occasional flash of lightning can be seen in the distance.\n\r");
	else if (ch->in_room->area->w_cur.lightning_str < 35)
	    strcat(buf, "Lightning crackles from the nearby storm.\n\r");
	else if (ch->in_room->area->w_cur.lightning_str < 70)
	    strcat(buf, "Lightning arcs from cloud to cloud above.\n\r");
	else
	    strcat(buf, "Violent flashes of lightning strike all about the nearby landscape.\n\r");

    if (area_is_affected(ch->in_room->area,gsn_icestorm))
	strcat(buf,"Ice and snow swirl around you.\n\r");
    if (area_is_affected(ch->in_room->area,gsn_lightning_storm))
	strcat(buf,"Grey clouds darken the sky, and lightning bolts strike all around you.\n\r");
    if (area_is_affected(ch->in_room->area,gsn_rainoffire))
	strcat(buf,"The red sky rains liquid fire. \n\r");
    if (area_is_affected(ch->in_room->area, gsn_brume))
        strcat(buf, "A thick fog swirls all about you.\n");
    
    send_to_char( buf, ch );
}

void show_helpfile( CHAR_DATA *ch, DESCRIPTOR_DATA *d, char *argument, int hflags )
{
    MYSQL_RES *result;
    MYSQL_ROW row;
    BUFFER *output;
    char *htext;
    bool found = FALSE;
    char argall[MAX_INPUT_LENGTH],argone[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    std::string originalArgument(argument);

    if (!ch && !d)
	return;

    if (global_option_nomysql)
    {
	sprintf(buf, "Help '%s' not found. Helpfiles not available.\n\r", argument);

	if (ch)
	    send_to_char(buf, ch);
	else if (d)
	    write_to_buffer(d, buf, 0);

	return;
    }

    output = new_buf();

    if ( argument[0] == '\0' )
	argument = "summary";

    /* this parts handles help a b so that it returns help 'a b' */
    argall[0] = '\0';
    while (argument[0] != '\0' )
    {
	argument = one_argument(argument,argone);
	if (argall[0] != '\0')
	    strcat(argall," ");
	strcat(argall,argone);
    }

    sprintf(buf, "SELECT * from help_data WHERE keywords LIKE \"%%%s%%\"", argall);

    if (mysql_query(&mysql, buf))
    {
	log_mysql_error();
	free_buf(output);
	return;
    }

    if ((result = mysql_store_result(&mysql)) != NULL)
    {
	while ((row = mysql_fetch_row(result)) != NULL)
	{
	    if ( ch && (atoi(row[0]) > get_trust(ch)))
		continue;

	    if ( is_name(argall, row[2]) )
	    {
		if (found)
		    add_buf(output, "\n\r=============================================================================\n\r\n\r");

		if (row[3] != NULL && !IS_SET(hflags, HELP_NOTITLE))
		{
		    add_buf(output, "/---------------------------------------------------------------------------\\\n\r");
		    sprintf(buf, "|          %-65.65s|\n\r", row[3]);
		    add_buf(output, buf);
		    add_buf(output, "\\---------------------------------------------------------------------------/\n\r\n\r");
		}

		htext = row[1];

		if (htext)
		{
		    if ( htext[0] == '.' )
		        add_buf(output,htext+1);
	            else
		        add_buf(output,htext);
		}

	        found = TRUE;
	        /* small hack :) */
	        if (d && d->connected != CON_PLAYING) 
		    break;
	    }
	}

	mysql_free_result(result);
    }

    if (!found)
    {
        // Strip out apostrophes
        std::string strippedArgument;
        bool foundQuote(false);
        for (size_t i(0); i < originalArgument.size(); ++i)
        {
            if (originalArgument[i] == '\'')
                foundQuote = true;
            else
                strippedArgument += originalArgument[i];
        }

        // If the argument contained apostrophes, try again without them
        if (foundQuote)
            show_helpfile(ch, d, const_cast<char*>(strippedArgument.c_str()), hflags);
        else
        {
    	    if (ch)
            {
                send_to_char( "No help on that word.\n\r", ch );
    	        sprintf(buf, "(%s) Help not found: %s", (ch->name ? ch->name : "(null)"), argall);
            }
            else
                sprintf(buf, "(null) Help not found: %s", argall);

        	bug(buf,0);
        }
    }
    else if (!ch || d->connected != CON_PLAYING)
    	write_to_descriptor(d->descriptor, buf_string(output), 0);
    else
    	page_to_char(buf_string(output),ch);

    free_buf(output);
}

void do_help( CHAR_DATA *ch, char *argument )
{
    show_helpfile(ch, ch->desc, argument, 0);
    return;
}

void do_newwhois (CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    BUFFER *output;
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    bool found = FALSE;
    int i;

    one_argument(argument,arg);
  
    if (arg[0] == '\0')
    {
	send_to_char("You must provide a name.\n\r",ch);
	return;
    }

    output = new_buf();

    for (i = 0; i < 2; i++)
    {
	for (d = descriptor_list; d != NULL; d = d->next)
        {
	    CHAR_DATA *wch;

 	    if (d->connected != CON_PLAYING || !can_see(ch,d->character))
	        continue;

	    wch = ( d->original != NULL ) ? d->original : d->character;

	    if (((i == 0) && IS_IMMORTAL(wch))
	     || ((i == 1) && !IS_IMMORTAL(wch)))
	        continue;

 	    if (!can_see_comm(ch,wch))
	        continue;

	    if (!str_prefix(arg,wch->name))
	    {
	        add_buf(output, format_who(wch, buf, ch));
	        found = TRUE;
	    }
	}
    }

    if (!found)
    {
	send_to_char("No one of that name is playing.\n\r",ch);
	return;
    }

    page_to_char(buf_string(output),ch);
    free_buf(output);
}



/* whois command */
void do_whois (CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    BUFFER *output;
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    bool found = FALSE;
    
    one_argument(argument,arg);
  
    if (arg[0] == '\0')
    {
	send_to_char("You must provide a name.\n\r",ch);
	return;
    }

    output = new_buf();

    for (d = descriptor_list; d != NULL; d = d->next)
    {
	CHAR_DATA *wch;
	char const *class_num;

 	if (d->connected != CON_PLAYING || !can_see(ch,d->character))
	    continue;

	wch = ( d->original != NULL ) ? d->original : d->character;

	if (IS_IMMORTAL(wch))
	    continue;

 	if (!can_see_comm(ch,wch))
	    continue;

	if (!str_prefix(arg,wch->name))
	{
	    found = TRUE;
	    
	    /* work out the printing */
	    class_num = class_table[wch->class_num].who_name;
	    switch(wch->level)
	    {
		case MAX_LEVEL - 0 : class_num = "IMP"; 	break;
		case MAX_LEVEL - 1 : class_num = "CRE";	break;
		case MAX_LEVEL - 2 : class_num = "SUP";	break;
		case MAX_LEVEL - 3 : class_num = "DEI";	break;
		case MAX_LEVEL - 4 : class_num = "GOD";	break;
		case MAX_LEVEL - 5 : class_num = "IMM";	break;
		case MAX_LEVEL - 6 : class_num = "DEM";	break;
		case MAX_LEVEL - 7 : class_num = "ANG";	break;
		case MAX_LEVEL - 8 : class_num = "AVA";	break;
	    }
    
	    /* a little formatting */
            if (!(IS_IMMORTAL(ch) || IS_IMMORTAL(wch) || (wch == ch)))
            sprintf(buf, "[   %6s %s] %s%s%s%s%s%s%s%s%s%s%s\n\r",
                wch->race < MAX_PC_RACE ? pc_race_table[wch->race].who_name
                                        : "     ",
                class_num,
		(IS_IMMORTAL(ch) && IS_SET(ch->nact, PLR_SHOWDATA) && !IS_NPC(wch) && (wch->pcdata->immrecord || (wch->desc && wch->desc->acct && wch->desc->acct->immrecord && wch->desc->acct->immrecord[0] != '\0'))) ? ":D: " : "",
		IS_PK(ch, wch) ? "(PK) ":"",
             (IS_IMMORTAL(ch) && wch->incog_level >= LEVEL_HERO) ? "(Incog) ": "",
             wch->invis_level >= LEVEL_HERO ? "(Wizi) " : "",
	(is_same_clan(ch, wch)|| wch->clan == clan_lookup("GUARDIAN") || wch->level > 51 || ch->level > 51) ? clan_table[wch->clan].display_name : "",
             IS_SET(wch->comm, COMM_AFK) ? "[AFK] " : "",
             IS_SET(wch->act,PLR_REWARD) ? "(REWARD) " : "",
                wch->name, IS_NPC(wch) ? "" : wch->pcdata->title,
        IS_NPC(wch) ? "" : wch->pcdata->extitle[0] == '\0' ? "" : ", ",
        IS_NPC(wch) ? "" : wch->pcdata->extitle[0] == '\0' ? "" : wch->pcdata->extitle);
             else
 	    sprintf(buf, "[%2d %6s %s] %s%s%s%s%s%s%s%s%s%s%s\n\r",
		wch->level,
		wch->race < MAX_PC_RACE ? pc_race_table[wch->race].who_name
					: "     ",
		class_num,
		(IS_IMMORTAL(ch) && IS_SET(ch->nact, PLR_SHOWDATA) && !IS_NPC(wch) && (wch->pcdata->immrecord || (wch->desc && wch->desc->acct && wch->desc->acct->immrecord && wch->desc->acct->immrecord[0] != '\0'))) ? ":D: " : "",
		IS_PK(ch, wch) ? "(PK) ":"",
             (IS_IMMORTAL(ch) && wch->incog_level >= LEVEL_HERO) ? "(Incog) ": "",
 	     wch->invis_level >= LEVEL_HERO ? "(Wizi) " : "",
	(is_same_clan(ch, wch)|| wch->clan == clan_lookup("GUARDIAN") || wch->level > 51 || ch->level > 51) ? clan_table[wch->clan].display_name : "",
	     IS_SET(wch->comm, COMM_AFK) ? "[AFK] " : "",
             IS_SET(wch->act,PLR_REWARD) ? "(REWARD) " : "",
		wch->name, IS_NPC(wch) ? "" : wch->pcdata->title,
        IS_NPC(wch) ? "" : wch->pcdata->extitle[0] == '\0' ? "" : ", ",
        IS_NPC(wch) ? "" : wch->pcdata->extitle[0] == '\0' ? "" : wch->pcdata->extitle);
	    add_buf(output,buf);
	}
    }

    for (d = descriptor_list; d != NULL; d = d->next)
    {
	CHAR_DATA *wch;
	char const *class_num;

 	if (d->connected != CON_PLAYING || !can_see(ch,d->character))
	    continue;

	wch = ( d->original != NULL ) ? d->original : d->character;

	if (!IS_IMMORTAL(wch))
	    continue;
	
 	if (!can_see_comm(ch,wch))
	    continue;

	if (!str_prefix(arg,wch->name))
	{
	    found = TRUE;
	    
	    /* work out the printing */
	    class_num = class_table[wch->class_num].who_name;
	    switch(wch->level)
	    {
		case MAX_LEVEL - 0 : class_num = "IMP"; 	break;
		case MAX_LEVEL - 1 : class_num = "CRE";	break;
		case MAX_LEVEL - 2 : class_num = "SUP";	break;
		case MAX_LEVEL - 3 : class_num = "DEI";	break;
		case MAX_LEVEL - 4 : class_num = "GOD";	break;
		case MAX_LEVEL - 5 : class_num = "IMM";	break;
		case MAX_LEVEL - 6 : class_num = "DEM";	break;
		case MAX_LEVEL - 7 : class_num = "ANG";	break;
		case MAX_LEVEL - 8 : class_num = "AVA";	break;
	    }
    
	    /* a little formatting */
            if (!(IS_IMMORTAL(ch) || IS_IMMORTAL(wch) || (wch == ch)))
            sprintf(buf, "[   %6s %s] %s%s%s%s%s%s%s%s%s%s%s\n\r",
                wch->race < MAX_PC_RACE ? pc_race_table[wch->race].who_name
                                        : "     ",
                class_num,
		(IS_IMMORTAL(ch) && IS_SET(ch->nact, PLR_SHOWDATA) && !IS_NPC(wch) && (wch->pcdata->immrecord || (wch->desc && wch->desc->acct && wch->desc->acct->immrecord && wch->desc->acct->immrecord[0] != '\0'))) ? ":D: " : "",
		IS_PK(ch, wch) ? "(PK) ":"",
             (IS_IMMORTAL(ch) && wch->incog_level >= LEVEL_HERO) ? "(Incog) ": "",
             wch->invis_level >= LEVEL_HERO ? "(Wizi) " : "",
	(is_same_clan(ch, wch)|| wch->clan == clan_lookup("GUARDIAN") || wch->level > 51 || ch->level > 51) ? clan_table[wch->clan].display_name : "",
             IS_SET(wch->comm, COMM_AFK) ? "[AFK] " : "",
             IS_SET(wch->act,PLR_REWARD) ? "(REWARD) " : "",
                wch->name, IS_NPC(wch) ? "" : wch->pcdata->title,
        IS_NPC(wch) ? "" : wch->pcdata->extitle[0] == '\0' ? "" : ", ",
        IS_NPC(wch) ? "" : wch->pcdata->extitle[0] == '\0' ? "" : wch->pcdata->extitle);
             else
 	    sprintf(buf, "[%2d %6s %s] %s%s%s%s%s%s%s%s%s%s%s\n\r",
		wch->level,
		wch->race < MAX_PC_RACE ? pc_race_table[wch->race].who_name
					: "     ",
		class_num,
		(IS_IMMORTAL(ch) && IS_SET(ch->nact, PLR_SHOWDATA) && !IS_NPC(wch) && (wch->pcdata->immrecord || (wch->desc && wch->desc->acct && wch->desc->acct->immrecord && wch->desc->acct->immrecord[0] != '\0'))) ? ":D: " : "",
		IS_PK(ch, wch) ? "(PK) ":"",
             (IS_IMMORTAL(ch) && wch->incog_level >= LEVEL_HERO) ? "(Incog) ": "",
 	     wch->invis_level >= LEVEL_HERO ? "(Wizi) " : "",
	(is_same_clan(ch, wch)|| wch->clan == clan_lookup("GUARDIAN") || wch->level > 51 || ch->level > 51) ? clan_table[wch->clan].display_name : "",
	     IS_SET(wch->comm, COMM_AFK) ? "[AFK] " : "",
             IS_SET(wch->act,PLR_REWARD) ? "(REWARD) " : "",
		wch->name, IS_NPC(wch) ? "" : wch->pcdata->title,
        IS_NPC(wch) ? "" : wch->pcdata->extitle[0] == '\0' ? "" : ", ",
        IS_NPC(wch) ? "" : wch->pcdata->extitle[0] == '\0' ? "" : wch->pcdata->extitle);
	    add_buf(output,buf);
	}
    }


    if (!found)
    {
	send_to_char("No one of that name is playing.\n\r",ch);
	return;
    }

    page_to_char(buf_string(output),ch);
    free_buf(output);
}


/*
 * New 'who' command originally by Alander of Rivers of Mud.
 */
void do_who( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    BUFFER *output;
    DESCRIPTOR_DATA *d;
    int iClass;
    int iRace;
    int iClan;
    int iLevelLower;
    int iLevelUpper;
    int nNumber;
    int nMatch;
    int nImmcount=0;
    bool rgfClass[MAX_CLASS];
    bool rgfRace[MAX_PC_RACE];
    bool rgfClan[MAX_CLAN];
    bool fClassRestrict = FALSE;
    bool fClanRestrict = FALSE;
    bool fClan = FALSE;
    bool fRaceRestrict = FALSE;
    bool fImmortalOnly = FALSE;
    bool fPKonly = FALSE;
    bool fNewbie = FALSE;
    bool fPose = FALSE;
    
    /*
     * Set default arguments.
     */
    iLevelLower    = 0;
    iLevelUpper    = MAX_LEVEL;
    for ( iClass = 0; iClass < MAX_CLASS; iClass++ )
        rgfClass[iClass] = FALSE;
    for ( iRace = 0; iRace < MAX_PC_RACE; iRace++ )
        rgfRace[iRace] = FALSE;
    for (iClan = 0; iClan < MAX_CLAN; iClan++)
	rgfClan[iClan] = FALSE;
 
    /*
     * Parse arguments.
     */
    nNumber = 0;
    for ( ;; )
    {
        char arg[MAX_STRING_LENGTH];
 
        argument = one_argument( argument, arg );
        if ( arg[0] == '\0' )
            break;
 
        if ( is_number( arg ) )
        {
            switch ( ++nNumber )
            {
            case 1: iLevelLower = atoi( arg ); break;
            case 2: iLevelUpper = atoi( arg ); break;
            default:
                send_to_char( "Only two level numbers allowed.\n\r", ch );
                return;
            }
        }
        else
        {
 
            /*
             * Look for classes to turn on.
             */
            if (!str_prefix(arg,"immortals"))
            {
                fImmortalOnly = TRUE;
            }
            if (!str_prefix(arg, "newbies"))
	    {
	        fNewbie = TRUE;
	    }
	    if (!str_prefix(arg, "pose"))
	        fPose = TRUE;
	    else
            {
                iClass = class_lookup(arg);
                if (iClass == -1)
                {
                    iRace = race_lookup(arg);
 
                    if (iRace == 0 || iRace >= MAX_PC_RACE)
		    {
			if (!str_prefix(arg,"house") && IS_IMMORTAL(ch))
			    fClan = TRUE;
			else
		        {
			    iClan = clan_lookup(arg);
			    if (iClan)
			    {
				if ((ch->clan == iClan) || ch->level > 51)
				{
				fClanRestrict = TRUE;
			   	rgfClan[iClan] = TRUE;
				}
				else
				{
				send_to_char("You're not in that house.\n\r", ch);
				return;
				}
			    }
			    else
			    {
                            if (!str_prefix(arg,"pk"))
                              {
                                 fPKonly = TRUE;
                              } 
                              else
                              {
                        	send_to_char(
                        "That's not a valid race, class, or house.\n\r",
				   ch);
                            	return;
                              }
			    }
                        }
		    }
                    else
                    {
                        fRaceRestrict = TRUE;
                        rgfRace[iRace] = TRUE;
                    }
                }
                else
                {
                    fClassRestrict = TRUE;
                    rgfClass[iClass] = TRUE;
                }
            }
        }
    }
 
    /*
     * Now show matching chars.
     */
    nMatch = 0;
    buf[0] = '\0';
    output = new_buf();
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        CHAR_DATA *wch;
        char const *class_num;
 
        /*
         * Check for match against restrictions.
         * Don't use trust as that exposes trusted mortals.
         */
        if ( d->connected != CON_PLAYING || !can_see_comm( ch, d->character ) )
            continue;

        wch   = ( d->original != NULL ) ? d->original : d->character;

	if (IS_IMMORTAL(wch))
	    continue;
	
	if (!can_see_comm(ch,wch))
	    continue;

        if ( wch->level < iLevelLower
        ||   wch->level > iLevelUpper
        || ( fImmortalOnly  && wch->level < LEVEL_IMMORTAL )
        || ( fClassRestrict && (!rgfClass[wch->class_num] || IS_IMMORTAL(wch)))
        || ( fRaceRestrict && !rgfRace[wch->race])
 	|| ( fClan && !is_clan(wch))
        || ( fNewbie && (!IS_NEWBIE(ch) || !IS_NEWBIE(wch)) )
	|| ( fClanRestrict && !rgfClan[wch->clan])
	|| ( fPose && wch->pose == NULL))
            continue;
	
	if (!IS_PK(ch, wch) &&
          (fPKonly == TRUE)) continue;

        nMatch++;
 
        /*
         * Figure out what to print for class_num.
	 */
	class_num = class_table[wch->class_num].who_name;
	switch ( wch->level )
	{
	default: break;
            {
                case MAX_LEVEL - 0 : class_num = "IMP";     break;
                case MAX_LEVEL - 1 : class_num = "IMM";     break;
                case MAX_LEVEL - 2 : class_num = "IMM";     break;
                case MAX_LEVEL - 3 : class_num = "IMM";     break;
                case MAX_LEVEL - 4 : class_num = "IMM";     break;
                case MAX_LEVEL - 5 : class_num = "IMM";     break;
                case MAX_LEVEL - 6 : class_num = "IMM";     break;
                case MAX_LEVEL - 7 : class_num = "IMM";     break;
                case MAX_LEVEL - 8 : class_num = "IMM";     break;
            }
	}

	/*
	 * Format it up.
	 */
            if (!(IS_IMMORTAL(ch) || IS_IMMORTAL(wch) || (wch == ch)))
            sprintf(buf, "[   %6s %s] %s%s%s%s%s%s%s%s%s%s%s\n\r",
                wch->race < MAX_PC_RACE ? pc_race_table[wch->race].who_name
                                        : "     ",
                class_num,
		(IS_IMMORTAL(ch) && IS_SET(ch->nact, PLR_SHOWDATA) && !IS_NPC(wch) && (wch->pcdata->immrecord || (wch->desc && wch->desc->acct && wch->desc->acct->immrecord && wch->desc->acct->immrecord[0] != '\0'))) ? ":D: " : "",
		IS_PK(ch, wch) ? "(PK) ":"",
             (IS_IMMORTAL(ch) && wch->incog_level >= LEVEL_HERO) ? "(Incog) ": "",
             wch->invis_level >= LEVEL_HERO ? "(Wizi) " : "",
	(is_same_clan(ch, wch) || wch->clan == clan_lookup("GUARDIAN") || wch->level > 51 || ch->level > 51) ? clan_table[wch->clan].display_name : "",
             IS_SET(wch->comm, COMM_AFK) ? "[AFK] " : "",
             IS_SET(wch->act,PLR_REWARD) ? "(REWARD) " : "",
                wch->name, IS_NPC(wch) ? "" : wch->pcdata->title,
        IS_NPC(wch) ? "" : wch->pcdata->extitle[0] == '\0' ? "" : ", ",
        IS_NPC(wch) ? "" : wch->pcdata->extitle[0] == '\0' ? "" : wch->pcdata->extitle);
             else
	sprintf( buf, "[%2d %6s %s] %s%s%s%s%s%s%s%s%s%s%s\n\r",
	    wch->level,
	    wch->race < MAX_PC_RACE ? pc_race_table[wch->race].who_name 
				    : "     ",
	    class_num,
		(IS_IMMORTAL(ch) && IS_SET(ch->nact, PLR_SHOWDATA) && !IS_NPC(wch) && (wch->pcdata->immrecord || (wch->desc && wch->desc->acct && wch->desc->acct->immrecord && wch->desc->acct->immrecord[0] != '\0'))) ? ":D: " : "",
		IS_PK(ch, wch) ? "(PK) ":"",
             (IS_IMMORTAL(ch) && wch->incog_level >= LEVEL_HERO) ? "(Incog) ": "",
	    wch->invis_level >= LEVEL_HERO ? "(Wizi) " : "",
	(is_same_clan(ch, wch)|| wch->clan == clan_lookup("GUARDIAN") || wch->level > 51 || ch->level > 51) ? clan_table[wch->clan].display_name : "",
	    IS_SET(wch->comm, COMM_AFK) ? "[AFK] " : "",
            IS_SET(wch->act, PLR_REWARD) ? "(REWARD) " : "",
	    wch->name,
	    IS_NPC(wch) ? "" : wch->pcdata->title,
        IS_NPC(wch) ? "" : wch->pcdata->extitle[0] == '\0' ? "" : ", ",
        IS_NPC(wch) ? "" : wch->pcdata->extitle[0] == '\0' ? "" : wch->pcdata->extitle);
	add_buf(output,buf);
    }

    for ( d = descriptor_list; d != NULL; d = d->next )
    {
        CHAR_DATA *wch;
        char const *class_num;
 
        /*
         * Check for match against restrictions.
         * Don't use trust as that exposes trusted mortals.
         */
        if ( d->connected != CON_PLAYING || !can_see_comm( ch, d->character ) )
            continue;

        wch   = ( d->original != NULL ) ? d->original : d->character;

	if (!IS_IMMORTAL(wch))
	    continue;

	if (!can_see_comm(ch,wch))
	    continue;

        if ( wch->level < iLevelLower
        ||   wch->level > iLevelUpper
        || ( fImmortalOnly  && wch->level < LEVEL_IMMORTAL )
        || ( fClassRestrict && (!rgfClass[wch->class_num] || IS_IMMORTAL(wch)))
        || ( fRaceRestrict && !rgfRace[wch->race])
 	|| ( fClan && !is_clan(wch))
        || ( fNewbie && (!IS_NEWBIE(ch) || !IS_NEWBIE(wch)) )
	|| ( fClanRestrict && !rgfClan[wch->clan]))
            continue;
	
	if (!IS_PK(ch, wch) &&
          (fPKonly == TRUE)) continue;

        nMatch++;
 	if (IS_IMMORTAL(wch))
	    nImmcount++;
        /*
         * Figure out what to print for class_num.
	 */
	class_num = class_table[wch->class_num].who_name;
	switch ( wch->level )
	{
	default: break;
            {
                case MAX_LEVEL - 0 : class_num = "IMP";     break;
                case MAX_LEVEL - 1 : class_num = "IMM";     break;
                case MAX_LEVEL - 2 : class_num = "IMM";     break;
                case MAX_LEVEL - 3 : class_num = "IMM";     break;
                case MAX_LEVEL - 4 : class_num = "IMM";     break;
                case MAX_LEVEL - 5 : class_num = "IMM";     break;
                case MAX_LEVEL - 6 : class_num = "IMM";     break;
                case MAX_LEVEL - 7 : class_num = "IMM";     break;
                case MAX_LEVEL - 8 : class_num = "IMM";     break;
            }
	}

	/*
	 * Format it up.
	 */
            if (!(IS_IMMORTAL(ch) || IS_IMMORTAL(wch) || (wch == ch)))
            sprintf(buf, "[   %6s %s] %s%s%s%s%s%s%s%s%s%s%s\n\r",
                wch->race < MAX_PC_RACE ? pc_race_table[wch->race].who_name
                                        : "     ",
                class_num,
		(IS_IMMORTAL(ch) && IS_SET(ch->nact, PLR_SHOWDATA) && !IS_NPC(wch) && (wch->pcdata->immrecord || (wch->desc && wch->desc->acct && wch->desc->acct->immrecord && wch->desc->acct->immrecord[0] != '\0'))) ? ":D: " : "",
		IS_PK(ch, wch) ? "(PK) ":"",
             (IS_IMMORTAL(ch) && wch->incog_level) >= LEVEL_HERO ? "(Incog) ": "",
             wch->invis_level >= LEVEL_HERO ? "(Wizi) " : "",
	(is_same_clan(ch, wch) || wch->clan == clan_lookup("GUARDIAN") || wch->level > 51 || ch->level > 51) ? clan_table[wch->clan].display_name : "",
             IS_SET(wch->comm, COMM_AFK) ? "[AFK] " : "",
             IS_SET(wch->act,PLR_REWARD) ? "(REWARD) " : "",
                wch->name, IS_NPC(wch) ? "" : wch->pcdata->title,
        IS_NPC(wch) ? "" : wch->pcdata->extitle[0] == '\0' ? "" : ", ",
        IS_NPC(wch) ? "" : wch->pcdata->extitle[0] == '\0' ? "" : wch->pcdata->extitle);
             else
	sprintf( buf, "[%2d %6s %s] %s%s%s%s%s%s%s%s%s%s%s\n\r",
	    wch->level,
	    wch->race < MAX_PC_RACE ? pc_race_table[wch->race].who_name 
				    : "     ",
	    class_num,
		(IS_IMMORTAL(ch) && IS_SET(ch->nact, PLR_SHOWDATA) && !IS_NPC(wch) && (wch->pcdata->immrecord || (wch->desc && wch->desc->acct && wch->desc->acct->immrecord && wch->desc->acct->immrecord[0] != '\0'))) ? ":D: " : "",
		IS_PK(ch, wch) ? "(PK) ":"",
             (IS_IMMORTAL(ch) && wch->incog_level >= LEVEL_HERO) ? "(Incog) ": "",
	    wch->invis_level >= LEVEL_HERO ? "(Wizi) " : "",
	(is_same_clan(ch, wch)|| wch->clan == clan_lookup("GUARDIAN") || wch->level > 51 || ch->level > 51) ? clan_table[wch->clan].display_name : "",
	    IS_SET(wch->comm, COMM_AFK) ? "[AFK] " : "",
            IS_SET(wch->act, PLR_REWARD) ? "(REWARD) " : "",
	    wch->name,
	    IS_NPC(wch) ? "" : wch->pcdata->title,
        IS_NPC(wch) ? "" : wch->pcdata->extitle[0] == '\0' ? "" : ", ",
        IS_NPC(wch) ? "" : wch->pcdata->extitle[0] == '\0' ? "" : wch->pcdata->extitle);
	add_buf(output,buf);
    }

    if (IS_IMMORTAL(ch))
	sprintf(buf2, "\n\rPlayers found: %d   Mortals Found: %d\n\r",nMatch,nImmcount);
    else
	sprintf( buf2, "\n\rPlayers found: %d\n\r", nMatch );
    add_buf(output,buf2);
    page_to_char( buf_string(output), ch );
    free_buf(output);
    return;
}


void do_newwho( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char namepre[MAX_INPUT_LENGTH];
//    char lbuf[3];
    BUFFER *output;
    DESCRIPTOR_DATA *d;
    int i;
    int iClass;
    int iClan;
    int iLevelLower;
    int iLevelUpper;
    int nNumber;
    int nMatch;
    int nImms=0;
    bool rgfClass[MAX_CLASS];
    bool rgfClan[MAX_CLAN];
    bool fClassRestrict = FALSE;
    bool fClanRestrict = FALSE;
    bool fClan = FALSE;
    bool fImmortalOnly = FALSE;
    bool fPKonly = FALSE;
    bool fNewbie = FALSE;
 
    /*
     * Set default arguments.
     */
    iLevelLower    = 0;
    iLevelUpper    = MAX_LEVEL;
    for ( iClass = 0; iClass < MAX_CLASS; iClass++ )
        rgfClass[iClass] = FALSE;
    for (iClan = 0; iClan < MAX_CLAN; iClan++)
	rgfClan[iClan] = FALSE;
 
    /*
     * Parse arguments.
     */
    nNumber = 0;
    namepre[0] = '\0';
    for ( ;; )
    {
        char arg[MAX_STRING_LENGTH];
 
        argument = one_argument( argument, arg );
        if ( arg[0] == '\0' )
            break;
 
        if ( is_number( arg ) )
        {
            switch ( ++nNumber )
            {
                case 1: iLevelLower = atoi( arg ); break;
                case 2: iLevelUpper = atoi( arg ); break;
                default:
                    send_to_char( "Only two level numbers allowed.\n\r", ch );
                    return;
            }

	    continue;
        }

        if (!str_prefix(arg,"immortals"))
        {
            fImmortalOnly = TRUE;
	    continue;
        }

        if (!str_prefix(arg, "newbies"))
	{
	    fNewbie = TRUE;
	    continue;
	}
// brazen: Ticket #11: Who group. Good idea, Anghwyr	
	if (!str_prefix(arg, "group"))
	{
	    iLevelLower = URANGE(1, ch->level - 8, LEVEL_HERO);
	    iLevelUpper = URANGE(1, ch->level + 8, LEVEL_HERO);
	    continue;
	}
        
	iClass = class_lookup(arg);
        if (iClass != -1)
        {
            fClassRestrict = TRUE;
            rgfClass[iClass] = TRUE;
	    continue;
	}

		    
	if (!str_prefix(arg,"house") && IS_IMMORTAL(ch))
	{
	    fClan = TRUE;
	    continue;
	}

	iClan = clan_lookup(arg);
	if (iClan)
	{
	    if ((ch->clan == iClan) || ch->level > 51)
	    {
		fClanRestrict = TRUE;
	   	rgfClan[iClan] = TRUE;
	    }
	    else
	    {
		send_to_char("You're not in that house.\n\r", ch);
		return;
	    }
	    continue;
	}

        if (!str_prefix(arg,"pk"))
        {
            fPKonly = TRUE;
	    continue;
        } 

	strcpy(namepre, arg);	
    }
 
    /*
     * Now show matching chars.
     */
    nMatch = 0;
    buf[0] = '\0';
    output = new_buf();

    for (i = 0; i < 2; i++)
    {
	for ( d = descriptor_list; d != NULL; d = d->next )
	{
            CHAR_DATA *wch;
 
            /*
             * Check for match against restrictions.
             * Don't use trust as that exposes trusted mortals.
             */
            if ( d->connected != CON_PLAYING || !can_see_comm( ch, d->character ) )
                continue;

            wch = ( d->original != NULL ) ? d->original : d->character;

	    if (((i == 0) && IS_IMMORTAL(wch)) 
	     || ((i == 1) && !IS_IMMORTAL(wch)))
	        continue;
	
	    if (!can_see_comm(ch,wch))
	        continue;

	    if (IS_NPC(wch))
		continue;

            if ( wch->level < iLevelLower
            ||   wch->level > iLevelUpper
            || ( fImmortalOnly  && wch->level < LEVEL_IMMORTAL )
            || ( fClassRestrict && (!rgfClass[wch->class_num] || IS_IMMORTAL(wch)))
 	    || ( fClan && !is_clan(wch))
            || ( fNewbie && (!IS_NEWBIE(ch) || !IS_NEWBIE(wch)) )
	    || ( fClanRestrict && !rgfClan[wch->clan])
            || ( (namepre[0] != '\0') && str_prefix(namepre, wch->name)))
                continue;
	
	    if (!IS_PK(ch, wch)
	     && (fPKonly == TRUE))
		continue;

            nMatch++;
 
	    if (IS_IMMORTAL(wch))
		nImms++;
	    /*
	     * Format it up.
	     */


	    /* Someone explain to me why ender doesn't seem to have itoa...? */
/*
	    sprintf(lbuf, "%d", ch->level);

	    sprintf(buf, "[%2s] %s%s%s%s%s%s%s%s%s%s%s%s%s\n\r",
		(IS_IMMORTAL(wch) ? "IM" :
		 (ch == wch)      ? lbuf :
		 IS_PK(ch, wch)   ? "PK" : "  "),
		((IS_IMMORTAL(ch) && wch->incog_level) >= LEVEL_HERO) ? "(Incog) " : "",
		(wch->invis_level > LEVEL_HERO) ? "(Wizi) " : "",
                (is_same_clan(ch, wch) || (wch->clan == clan_lookup("GUARDIAN"))
	         || wch->level > 51 || ch->level > 51) ? clan_table[wch->clan].display_name : "",
		(IS_SET(wch->comm, COMM_AFK)) ? "[AFK] " : "",
		(IS_SET(wch->act, PLR_REWARD)) ? "(REWARD) " : "",
		(wch->pcdata->pretitle[0] == '\0') ? "" : wch->pcdata->pretitle,
		(wch->pcdata->pretitle[0] == '\0') ? "" : " ",
		wch->name,
		(wch->pcdata->surname[0] == '\0') ? "" : " ",
		(wch->pcdata->surname[0] == '\0') ? "" : wch->pcdata->surname,
		wch->pcdata->title,
		(wch->pcdata->extitle[0] == '\0') ? "" : ", ",
		(wch->pcdata->extitle[0] == '\0') ? "" : wch->pcdata->extitle);
*/

	    add_buf(output, format_who(wch, buf, ch));
	}
    }

    if (IS_IMMORTAL(ch))
	sprintf(buf2, "\n\rPlayers Found: %d   Mortals Found: %d\n\r",nMatch,nMatch-nImms);
    else
	sprintf( buf2, "\n\rPlayers found: %d\n\r", nMatch );
    add_buf(output,buf2);
    page_to_char( buf_string(output), ch );
    free_buf(output);
    return;
}


char *format_who(CHAR_DATA *wch, char *buf, CHAR_DATA *ch)
{
    std::ostringstream out;
    out << std::setfill(' ');

    if (IS_IMMORTAL(ch))
        out << '[' << std::setw(2) << wch->level << "  " << std::setw(5) << pc_race_table[wch->race].who_name << " " << std::setw(3) << class_table[wch->class_num].who_name << ']';
    else
    {
        if (IS_IMMORTAL(wch))
            out << "[IM]";
        else
        {
            out << '[';
            out << std::setw(2) << wch->level << ' ';
            out << std::setw(3) << class_table[wch->class_num].who_name;
            out << ']';
            
            if (ch != wch && IS_PK(ch, wch)) out << " [PK]";
        }
    }

    sprintf(buf, "%s %s%s%s%s%s%s%s%s%s%s%s%s%s%s\n\r",
	out.str().c_str(),
	(IS_IMMORTAL(ch) && IS_SET(ch->nact, PLR_SHOWDATA)
         && !IS_NPC(wch) && (wch->pcdata->immrecord || (wch->desc && wch->desc->acct && wch->desc->acct->immrecord && wch->desc->acct->immrecord[0] != '\0'))) ? ":D: " : "",
	(IS_IMMORTAL(ch) && wch->incog_level >= LEVEL_HERO) ? "(Incog) " : "",
	(wch->invis_level > LEVEL_HERO) ? "(Wizi) " : "",
        (is_same_clan(ch, wch) || (wch->clan == clan_lookup("GUARDIAN"))
         || wch->level > 51 || ch->level > 51) ? clan_table[wch->clan].display_name : "",
	(IS_SET(wch->comm, COMM_AFK)) ? "[AFK] " : "",
	(IS_SET(wch->act, PLR_REWARD)) ? "(REWARD) " : "",
	(wch->pcdata->pretitle[0] == '\0') ? "" : wch->pcdata->pretitle,
	(wch->pcdata->pretitle[0] == '\0') ? "" : " ",
	wch->name,
	(wch->pcdata->surname[0] == '\0') ? "" : " ",
	(wch->pcdata->surname[0] == '\0') ? "" : wch->pcdata->surname,
	wch->pcdata->title,
	(wch->pcdata->extitle[0] == '\0') ? "" : ", ",
	(wch->pcdata->extitle[0] == '\0') ? "" : wch->pcdata->extitle);

    return buf;
}

void do_upcount()
{
    int count;
    DESCRIPTOR_DATA *d;

    count = 0;

    for ( d = descriptor_list; d != NULL; d = d->next )
        if ( d->connected == CON_PLAYING  )
	    count++;

    max_on = UMAX(count,max_on);
}



void do_count ( CHAR_DATA *ch, char *argument )
{
    int count;
    DESCRIPTOR_DATA *d;
    char buf[MAX_STRING_LENGTH];

    count = 0;

    for ( d = descriptor_list; d != NULL; d = d->next )
        if ( d->connected == CON_PLAYING && can_see_comm( ch, d->character ) )
	    count++;

    max_on = UMAX(count,max_on);

    if (max_on == count)
        sprintf(buf,"There are %d characters on, the most since the last reboot.\n\r",
	    count);
    else
	sprintf(buf,"There are %d characters on, the most since the last reboot was %d.\n\r",
	    count,max_on);

    send_to_char(buf,ch);
}

void do_inventory( CHAR_DATA *ch, char *argument )
{
    send_to_char( "You are carrying:\n\r", ch );
    show_list_to_char( ch->carrying, ch, TRUE, TRUE );
    return;
}

static unsigned int number_of_free_hands(CHAR_DATA * ch)
{
	// Check for a mainhand weapon
	int result = 2;
	if (get_eq_char(ch, WEAR_WIELD) != NULL)
	{
		--result;
		if (IS_PAFFECTED(ch, AFF_TWOHAND)) 
		return 0;
	}

	// Check the other hand slots
	if (get_eq_char(ch, WEAR_DUAL_WIELD) != NULL) --result;
	if (get_eq_char(ch, WEAR_SHIELD) != NULL) --result;
	if (get_eq_char(ch, WEAR_HOLD) != NULL) --result;
	if (result < 0) result = 0;
	return static_cast<unsigned int>(result);
}

static void AddEQKey(DisplayPanel::Box & keys, const std::string & line)
{
    std::ostringstream mess;
    mess << '<' << line << '>';
    keys.AddLine(mess.str());
}

static void AddEQLine(DisplayPanel::Box & keys, DisplayPanel::Box & values, const std::string & key, const std::string & value)
{
    AddEQKey(keys, key);
    values.AddLine(value);
}

static std::string show_eq_to_char(CHAR_DATA & ch, CHAR_DATA & viewer, bool showAbsent, unsigned int & count)
{
    unsigned int handsFree(number_of_free_hands(&ch));
    DisplayPanel::Box keys;
    DisplayPanel::Box values;
    
    // Iterate the wear slots
    for (unsigned int iWear(0); iWear < MAX_WEAR; ++iWear)
    {
        OBJ_DATA * obj(get_eq_char(&ch, iWear));

        // Handle the familiar wear slot specially
        if (iWear == WEAR_FAMILIAR)
        {
            if (obj != NULL)
            {

                AddEQLine(keys, values, (obj->obj_str == NULL ? "worn as familiar" : obj->obj_str), format_obj_to_char(obj, &viewer, true));
                ++count;
            }
            
            continue;
        }

        // Handle the case of no object
        if ((obj == NULL) 
        || (IS_SET(obj->extra_flags[0], ITEM_WIZI) && !IS_NPC(&viewer) && !IS_SET(viewer.act, PLR_HOLYLIGHT)))
        {
            bool shouldShow = false;
            switch (iWear)
            {
                case WEAR_NONE:		case WEAR_LIGHT:	case WEAR_FLOAT:
                case WEAR_SIGIL:	case WEAR_CONCEAL1:	case WEAR_CONCEAL2:
                case WEAR_PROGSLOT: 	case WEAR_HOLD:
                    break;

                case WEAR_WIELD:
                case WEAR_SHIELD: 
                case WEAR_DUAL_WIELD:
                    shouldShow = (handsFree > 0);
                    break;

                default: 
                    shouldShow = true;
                    break;
            }

            if (shouldShow && showAbsent)
            {
                AddEQLine(keys, values, where_name[iWear], "[nothing]");
                ++count;
            }

            continue;
        }

        ++count;

        // Handle two-handed wielding specially
        if (iWear == WEAR_WIELD && IS_PAFFECTED(&ch, AFF_TWOHAND))
            AddEQKey(keys, "two-handed");
        else
            AddEQKey(keys, where_name[iWear]);
        
        // Add in the name of the obj
        if (can_see_obj(&viewer, obj))
            values.AddLine(format_obj_to_char(obj, &viewer, true));
        else
            values.AddLine("something");
       
        // Handle nocked items 
        if (obj->item_type == ITEM_BOW && ch.nocked != NULL)
        {
            ++count;
            AddEQKey(keys, "nocked");
            if (can_see_obj(&viewer, ch.nocked))
                values.AddLine(format_obj_to_char(ch.nocked, &viewer, true));
            else
                values.AddLine("something");
        }
    }

    // Render the resultant text
    DisplayPanel::HorizontalSplit panel(DisplayPanel::Options(DisplayPanel::Style_None));
    panel.Add(keys);
    panel.Add(values);
    return DisplayPanel::Render(panel, DisplayPanel::Options(DisplayPanel::Style_None));
}

void do_equipment( CHAR_DATA *ch, char *argument )
{
    unsigned int count(0);
    send_to_char("You are using:\n", ch);
    send_to_char(show_eq_to_char(*ch, *ch, true, count).c_str(), ch);
}

void do_compare( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    OBJ_DATA *obj1;
    OBJ_DATA *obj2;
    int value1;
    int value2;
    char *msg;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' )
    {
	send_to_char( "Compare what to what?\n\r", ch );
	return;
    }

    if ( ( obj1 = get_obj_carry( ch, arg1, ch ) ) == NULL )
    {
	send_to_char( "You do not have that item.\n\r", ch );
	return;
    }

    if (arg2[0] == '\0')
    {
	for (obj2 = ch->carrying; obj2 != NULL; obj2 = obj2->next_content)
	{
//	    if (obj2->wear_loc != WEAR_NONE

	    if (obj2->worn_on
	    &&  can_see_obj(ch,obj2)
	    &&  obj1->item_type == obj2->item_type
	    &&  (obj1->wear_flags & obj2->wear_flags & ~ITEM_TAKE) != 0 )
		break;
	}

	if (obj2 == NULL)
	{
	    send_to_char("You aren't wearing anything comparable.\n\r",ch);
	    return;
	}
    } 

    else if ( (obj2 = get_obj_carry(ch,arg2,ch) ) == NULL )
    {
	send_to_char("You do not have that item.\n\r",ch);
	return;
    }

    msg		= NULL;
    value1	= 0;
    value2	= 0;

    if ( obj1 == obj2 )
    {
	msg = "You compare $p to itself.  It looks about the same.";
    }
    else if ( obj1->item_type != obj2->item_type )
    {
	msg = "You can't compare $p and $P.";
    }
    else
    {
	switch ( obj1->item_type )
	{
	default:
	    msg = "You can't compare $p and $P.";
	    break;

	case ITEM_ARMOR:
	    value1 = obj1->value[0] + obj1->value[1] + obj1->value[2];
	    value2 = obj2->value[0] + obj2->value[1] + obj2->value[2];
	    break;

	case ITEM_WEAPON:
	    if (obj1->pIndexData->new_format)
		value1 = (1 + obj1->value[2]) * obj1->value[1];
	    else
	    	value1 = obj1->value[1] + obj1->value[2];

	    if (obj2->pIndexData->new_format)
		value2 = (1 + obj2->value[2]) * obj2->value[1];
	    else
	    	value2 = obj2->value[1] + obj2->value[2];
	    break;
	}
    }

    if ( msg == NULL )
    {
	     if ( value1 == value2 ) msg = "$p and $P look about the same.";
	else if ( value1  > value2 ) msg = "$p looks better than $P.";
	else                         msg = "$p looks worse than $P.";
    }

    act( msg, ch, obj1, obj2, TO_CHAR );
    return;
}



void do_credits( CHAR_DATA *ch, char *argument )
{
    do_help( ch, "diku" );
    return;
}

void do_corpsewhere(CHAR_DATA * ch, char * argument)
{
	// Sanity check
	if (ch->pcdata == NULL || ch->pcdata->last_death_location == NULL)
	{
		send_to_char("The location of your last death has been lost in the sands of time.\n\r", ch);
		return;
	}

	// Now tell them what they want to hear
	std::ostringstream mess;
	mess << "You last died in " << ch->pcdata->last_death_location << "\n\r";
	send_to_char(mess.str().c_str(), ch);
}

static bool can_see_on_where(CHAR_DATA & ch, CHAR_DATA & victim, ROOM_INDEX_DATA & room, bool targeted, bool pkOnly, bool ghostsOnly)
{
    // Check basic disqualifiers
    if (&ch == &victim) return false;
    if (victim.in_room == NULL) return false;
    if (IS_SET(room.room_flags, ROOM_NONEFORYOU)) return false;        
    if (IS_SET(victim.in_room->room_flags, ROOM_NOWHERE)) return false;
    if (room_is_private(victim.in_room) && !is_room_owner(&ch, victim.in_room)) return false;
    if (area_is_affected(victim.in_room->area, gsn_pillage) && is_affected(&victim, gsn_hooded)) return false;
    if (!can_see(&ch, &victim)) return false;
    
    // Check for coven powers
    int coven(clan_lookup("SHUNNED"));
    if (victim.in_room->clan == coven && ch.clan != coven && is_affected(&victim, gsn_shadow_ward)) return false;
    if (ch.clan != coven && IS_PAFFECTED(&victim, AFF_SHROUD_OF_NYOGTHUA)) return false;

    // Check chameleon
    if (is_affected(&victim, gsn_chameleon))
    {
        switch (victim.in_room->sector_type)
        {
            case SECT_FOREST:
            case SECT_SWAMP:
            case SECT_HILLS:
            case SECT_MOUNTAIN: return false;
        }
    }

    // Check for targeted/untargeted disqualifiers
    if (targeted)
    {
        if (is_affected(&victim, gsn_conceal_thoughts) && number_bits(2) != 0) return false;
    }
    else
    {
        if (is_affected(&victim, gsn_conceal_thoughts)) return false;
        if (IS_NPC(&victim)) return false;
    }

    // Check for pk filter
    if (pkOnly && !is_pk(&ch, &victim)) return false;

    // Check for ghosts
    if (ghostsOnly && !IS_OAFFECTED(&victim, AFF_GHOST)) return false;
    
    // Check for area
    if (victim.in_room->area != room.area) 
    {
        // Area restrictions only matter in certain circumstances
        if (!ghostsOnly && check_grimseep_affiliation(ch, *victim.in_room) != Grimseep_Aided)
            return false;
    }

    return true; 
}

static void send_where_string(CHAR_DATA & ch, CHAR_DATA & victim)
{
    char buf[MAX_STRING_LENGTH];
    const char * prefix(IS_PK(&ch, &victim) ? "(PK) ": "");
    
    const char * name(victim.name);
    if (IS_NPC(&victim) || (IS_OAFFECTED(&victim, AFF_DISGUISE) && is_affected(&victim, gsn_hooded))) 
        name = victim.short_descr;
    
    sprintf(buf, "%s%-28s %s\n\r", prefix, name, victim.in_room->name);
    send_to_char(buf, &ch);
}

void do_where( CHAR_DATA *ch, char *argument )
{
    // Find the originating room
    ROOM_INDEX_DATA * troom(ch->in_room);
    if (!IS_NPC(ch) && IS_NAFFECTED(ch, AFF_DELUSION))
    {
        int c;
        for (c = 0; c < 100;c++)
        {
            if ((troom = get_room_index(number_range(400,32768))) == NULL) continue;
            if (!BIT_GET(ch->pcdata->travelptr, troom->vnum)) continue;
        }
        if (troom == NULL)
            troom = ch->in_room;
    }
    
    if (troom == NULL || troom->area == NULL || troom->area->name == NULL || troom->area->name[0] == '\0')
    {
        send_to_char( "You cannot sense anything here.\n", ch);
        return;
    }
    
    // Check general where disqualifiers
    if (ch->in_room && area_is_affected(ch->in_room->area, gsn_icestorm))
    {
        send_to_char("You can't see anything with all the ice and snow swirling around!\n\r", ch);
        return;
    }

    if (is_affected(ch, gsn_visions))
    {
        send_to_char("Your vision is too dominated by the horrors of your sins to see around you.\n\r", ch);
        return;
    }

    if (area_is_affected(troom->area, gsn_brume) && number_percent() > get_skill(ch, gsn_stormmastery))
    {
        send_to_char("You cannot see through the thick brume.\n", ch);
        return;
    }

    if (check_blind(ch))
    {
        send_to_char("You can't see anything!\n\r", ch);
        return;
    }
    
    // Get the argument and initialize the filters 
    char arg[MAX_INPUT_LENGTH];
    one_argument(argument, arg);
    bool filterPK(false);
    bool filterGhosts(false);

    // Handle the case of no argument
    if (arg[0] == '\0')
    {
        char buf[MAX_INPUT_LENGTH];
        sprintf(buf, "Others in %s:\n\r", troom->area->name);
        send_to_char(buf, ch);
    }
    // Handle the case of 'where ghost'
    else if (!str_prefix(arg, "ghost"))
    {
        // Check for thanatopsis
        if (!is_affected(ch, gsn_thanatopsis))
        {
            send_to_char("You are not sufficiently in tune with the world to sense the deceased.\n", ch);
            return;
        }

        // Check for mana
        if (ch->mana < 60)
        {
            send_to_char("You are too weary to sense for the deceased.\n", ch);
            return;
        }

        // Echo and expend the mana
        send_to_char("You sense the following ghosts:\n", ch);
        expend_mana(ch, 60);
        filterGhosts = true;
    }
    // Handle the case of 'where pk'
    else if (!str_prefix(arg, "pk"))
    {
        char buf[MAX_INPUT_LENGTH];
        sprintf(buf, "Others in %s:\n\r", troom->area->name);
        send_to_char(buf, ch);
        filterPK = true;
	}
    // Handle the case of a specified target
    else
    {
        bool found(false);
        for (CHAR_DATA * victim(char_list); victim != NULL; victim = victim->next)
        {
            if (is_name(arg, victim->name) && can_see_on_where(*ch, *victim, *troom, true, filterPK, filterGhosts))
            {
                send_where_string(*ch, *victim);
                found = true;
            }
        }

        if (!found)
            act("You didn't find any $T.", ch, NULL, arg, TO_CHAR);

        return;
    }

    // Perform the general where
    bool found(false);
    for (DESCRIPTOR_DATA * d(descriptor_list); d != NULL; d = d->next)
    {
        if (d->connected != CON_PLAYING)
            continue;
       
        // Check the character 
        if (d->character != NULL && can_see_on_where(*ch, *d->character, *troom, false, filterPK, filterGhosts))
        { 
            send_where_string(*ch, *d->character);
            found = true;
        }

        // Check the original character
        if (d->original != NULL && can_see_on_where(*ch, *d->original, *troom, false, filterPK, filterGhosts))
        {
            send_where_string(*ch, *d->original);
            found = true;
        }
    }

    // Notify if no one found
    if (!found)
        send_to_char("None\n", ch);
}

void do_consider( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    char *msg;
    int diff;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Consider killing whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They're not here.\n\r", ch );
	return;
    }

   protect_works = FALSE;
   if (!IS_OAFFECTED(ch, AFF_GHOST) || !IS_NPC(victim))
    if (is_safe(ch,victim))
    {
   protect_works = TRUE;
	send_to_char("Don't even think about it.\n\r",ch);
	return;
    }
   protect_works = TRUE;

    diff = victim->level - ch->level;

         if ( diff <= -10 ) msg = "You can kill $N naked and weaponless.";
    else if ( diff <=  -5 ) msg = "$N is no match for you.";
    else if ( diff <=  -2 ) msg = "$N looks like an easy kill.";
    else if ( diff <=   1 ) msg = "The perfect match!";
    else if ( diff <=   4 ) msg = "$N looks a bit tough.";
    else if ( diff <=   9 ) msg = "$N regards you with open contempt.";
    else                    msg = "Death will thank you for your gift.";


    act( msg, ch, NULL, victim, TO_CHAR );

    if (IS_NPC(victim))
    {
        diff = victim->alignment;

	if (victim->race != global_int_race_shuddeni)
	{
	    if (diff < 0)
  	        msg = "$N has an evil glint in $S eye.";
	    else if (diff > 0)
	        msg = "$N has an aura of goodness.";
	    else
	        msg = "$N seems very disinterested.";
    
            act( msg, ch, NULL, victim, TO_CHAR );
	}
    }
	
    return;
}



void set_title( CHAR_DATA *ch, char *title )
{
    char buf[MAX_STRING_LENGTH];

    if ( IS_NPC(ch) )
    {
	bug( "Set_title: NPC.", 0 );
	return;
    }

    if ( title[0] != '.' && title[0] != ',' && title[0] != '!' && title[0] != '?' )
    {
	buf[0] = ' ';
	strcpy( buf+1, title );
    }
    else
    {
	strcpy( buf, title );
    }

    free_string( ch->pcdata->title );
    ch->pcdata->title = str_dup( buf );
    return;
}

void set_extitle( CHAR_DATA *ch, char *extitle )
{
    char buf[MAX_STRING_LENGTH];

    if ( IS_NPC(ch) )
    {
        bug( "Set_extitle: NPC.", 0 );
        return;
    }

/*    if ( extitle[0] != '.' && extitle[0] != ',' && extitle[0] != '!' &&
extitle[0] != '?' )
    {
        buf[0] = ' ';
        strcpy( buf+1, extitle );
    }
    else
    {
        strcpy( buf, extitle );
    }*/
    
    strcpy( buf, extitle );

    free_string( ch->pcdata->extitle );
    ch->pcdata->extitle = str_dup( buf );
    return;
}


void do_title( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC(ch) )
	return;
    if ( !IS_IMMORTAL(ch) ) {
      send_to_char( "Huh?\n\r", ch );
      return;
    }

    if ( argument[0] == '\0' )
    {
	send_to_char( "Change your title to what?\n\r", ch );
	return;
    }

    if ( strlen(argument) > 45 )
	argument[45] = '\0';

    smash_tilde( argument );
    set_title( ch, argument );
    send_to_char( "Ok.\n\r", ch );
}

void do_background( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_STRING_LENGTH];
    CHAR_DATA *vch;
    bool offline = FALSE;

    if (IS_NPC(ch))
	return;

    argument = one_argument(argument, arg);

    if ( arg[0] != '\0' )
    {
	buf[0] = '\0';
	smash_tilde( arg );

    	if (arg[0] == '-')
    	{
            int len;
            bool found = FALSE;
 
            if (ch->pcdata->background == NULL || ch->pcdata->background[0] == '\0')
            {
                send_to_char("No lines left to remove.\n\r",ch);
                return;
            }
	
  	    strcpy(buf,ch->pcdata->background);
 
            for (len = strlen(buf); len > 0; len--)
            {
                if (buf[len] == '\r')
                {
                    if (!found)  /* back it up */
                    {
                        if (len > 0)
                            len--;
                        found = TRUE;
                    }
                    else /* found the second one */
                    {
                        buf[len + 1] = '\0';
			free_string(ch->pcdata->background);
			ch->pcdata->background = str_dup(buf);
			send_to_char( "Your background is:\n\r", ch );
			send_to_char( ch->pcdata->background ? ch->pcdata->background : 
			    "(None).\n\r", ch );
                        return;
                    }
                }
            }
            buf[0] = '\0';
	    free_string(ch->pcdata->background);
	    ch->pcdata->background = str_dup(buf);
	    send_to_char("Background cleared.\n\r",ch);
	    return;
        }

	if (!str_cmp(arg, "clear"))
	{
	    buf[0] = '\0';
	    free_string(ch->pcdata->background);
	    ch->pcdata->background = str_dup(buf);
	    send_to_char("Background cleared.\n\r",ch);
	    return;
	}

	if (!str_cmp(arg, "format"))
	{
	    if (ch->pcdata->background)
	        ch->pcdata->background = format_string(ch->pcdata->background);
	    send_to_char("Background formatted.\n\r", ch);
	    return;
	}

	if (!str_cmp(arg, "edit"))
	{
	    string_append(ch, &ch->pcdata->background);
	    return;
	}

	if (!str_cmp(arg, "view"))
	{	
	    if (!IS_TRUSTED(ch, ANGEL))
	    {
		send_to_char("You cannot view other backgrounds.\n\r", ch);
		return;
	    }

	    if (argument[0] == '\0')

	    {
		send_to_char("Syntax: \n\r", ch);
		send_to_char(" bg view <char>\n\r", ch);
		send_to_char(" bg view offline <char>\n\r", ch);
		return;
	    }
	    
	    /* Code added for viewing backgrounds offline.
	     * - transmitt 10/09/05
	     */
	    argument = one_argument(argument, arg);
	    if (!str_cmp(arg, "offline"))
	    {
                if (vch = load_offline_char(argument))
		    offline = TRUE;
		else
		{
                    send_to_char("Character already online or does not exist!\n\r", ch);
	            return;
		}	
	    }	    

	    /* if not offline, do it */
	    else if ((vch = get_char_world(ch, arg)) == NULL)
	    {
		send_to_char("You cannot sense them in the land.\n\r", ch);
		return;
	    }

	    if (IS_NPC(vch) || !vch->pcdata->background || (vch->pcdata->background[0] == '\0'))
		act("$N has no background.", ch, NULL, vch, TO_CHAR);
	    else
	    {
	        act("$N's background is:", ch, NULL, vch, TO_CHAR);
		send_to_char(vch->pcdata->background, ch);
	    }
	    if (offline)
		extract_char(vch,TRUE);

	    return;
	}	    

	if ( arg[0] == '+' )
	{
	    if ( ch->pcdata->background != NULL )
		strcat( buf, ch->pcdata->background );

            if ( strlen(buf) >= MAX_DESCRIPTION_LENGTH)
	    {
	        send_to_char( "Background too long.\n\r", ch );
	        return;
	    }

	    strcat( buf, argument );
	    strcat( buf, "\n\r" );
	    free_string( ch->pcdata->background );
	    ch->pcdata->background = str_dup( buf );
            send_to_char( "Your background is:\n\r", ch );
    	    send_to_char( ch->pcdata->background ? ch->pcdata->background : "(None).\n\r", ch );
    	    return;
	}

	sprintf(buf, "Invalid argument '%s'.\n\rPlease read HELP BACKGROUND for more information.\n\r", arg);
	send_to_char(buf, ch);
	return;
    }

    send_to_char( "Your background is:\n\r", ch );
    send_to_char( ch->pcdata->background ? ch->pcdata->background : "(None).\n\r", ch );
    return;
}

void do_exdesc( CHAR_DATA *ch, char *argument )
{
    EXTRA_DESCR_DATA *ed;
    CHAR_DATA *victim = NULL;
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    if (argument[0] == '\0')
    {
	if (IS_NPC(ch))
	    return;

	if (!ch->pcdata->extra_descr)
	{
	    send_to_char("You have no extended descriptions declared.\n\r", ch);
	    return;
	}

	send_to_char("You have the following extended descriptions declared:\n\r", ch);

	for (ed = ch->pcdata->extra_descr; ed; ed = ed->next)
	{
	    sprintf(buf, "\n\rKeywords: %s\n\r%s", ed->keyword, ed->description);
	    send_to_char(buf, ch);
	}

	return;
    }

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (!str_cmp(arg1, "other"))
    {
	/** could put all of these iffs together, but cleaner this way **/
	if ((!IS_NPC(ch) && !IS_IMMORTAL(ch))
	 || (IS_NPC(ch) && (IS_AFFECTED(ch, AFF_CHARM) || ch->commander)))
	    return;

	if ((arg2[0] == '\0') || ((victim = get_char_world(ch, arg2)) == NULL))
	    return;

	if (IS_NPC(victim) || IS_IMMORTAL(victim))
	    return;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
    }
    else if (!IS_NPC(ch))
	victim = ch;
    else
	return;

    if (!str_cmp(arg1, "add"))
    {
	if (arg2[0] == '\0')
	{
	    send_to_char("Syntax: exdesc add <keyword(s)>\n\r", ch);
	    return;
	}
	
	ed = new_extra_descr();
	ed->next = victim->pcdata->extra_descr;
	victim->pcdata->extra_descr = ed;

	ed->keyword = str_dup(arg2);
	ed->description = str_dup("");

	if (!IS_NPC(ch))
	    string_append(ch, &ed->description);

	return;
    }

    if (!str_cmp(arg1, "edit") && !IS_NPC(ch))
    {
	if (arg2[0] == '\0')
	{
	    send_to_char("Syntax: exdesc edit <keyword(s) of exdesc to edit>\n\r", ch);
	    return;
	}

	for (ed = victim->pcdata->extra_descr; ed; ed = ed->next)
	    if (!str_cmp(arg2, ed->keyword))
	    {
		if (!ed->can_edit && !IS_NPC(ch) && !IS_IMMORTAL(ch))
		{
		    send_to_char("You may not edit that exdesc.\n\r", ch);
		    return;
		}

		string_append(ch, &ed->description);
		break;
	    }

	return;
    }

    if (!str_cmp(arg1, "remove") || !str_cmp(arg1, "delete"))
    {
	EXTRA_DESCR_DATA *ed_next, *ed_last = NULL;

	if (arg2[0] == '\0')
	{
	    send_to_char("Syntax: exdesc remove <keyword(s) of exdesc to remove>\n\r", ch);
	    return;
	}

	for (ed = victim->pcdata->extra_descr; ed; ed = ed_next)
	{
	    ed_next = ed->next;
	    if (!str_cmp(arg2, ed->keyword))
	    {
		if (!ed->can_edit && !IS_NPC(ch) && !IS_IMMORTAL(ch))
		{
		    send_to_char("You may not remove that exdesc.\n\r", ch);
		    return;
		}

		if (ed_last)
		   ed_last->next = ed->next;
		else
		   victim->pcdata->extra_descr = ed->next;
		free_extra_descr(ed);
		break;
	    }
	    ed_last = ed;
	}

	return;
    }

    if (!str_cmp(arg1, "append"))
    {
	char newdesc[MAX_DESCRIPTION_LENGTH];

	if ((arg2[0] == '\0') || (argument[0] == '\0'))
	    return;

	for (ed = victim->pcdata->extra_descr; ed; ed = ed->next)
	{
	    if (!str_cmp(arg2, ed->keyword))
	    {
		if (!ed->can_edit && !IS_NPC(ch) && !IS_IMMORTAL(ch))
		{
		    send_to_char("You may not edit that exdesc.\n\r", ch);
		    return;
		}

		strcpy(newdesc, ed->description);
		if ((strlen(newdesc) + strlen(argument)) >= MAX_DESCRIPTION_LENGTH)
		{
		    send_to_char("Description too long.\n\r", ch);
		    return;
		}

		strcat(newdesc, argument);
		strcat(newdesc, "\n\r");
		free_string(ed->description);
		ed->description = str_dup(newdesc);
		sprintf(buf, "New exdesc '%s' is:\n\r%s", ed->keyword, ed->description);
		send_to_char(buf, ch);
		return;
	    }
	}

	return;
    }	

    send_to_char("Syntax: exdesc <add/edit/remove> <keyword(s)>\n\r", ch);
    return;
}

void do_permexdesc( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    EXTRA_DESCR_DATA *ed;
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
	return;

    argument = one_argument(argument, arg1);

    if ((argument[0] == '\0') || (arg1[0] == '\0'))
    {
	send_to_char("Syntax: permexdesc <char> <keyword>\n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, arg1)) == NULL)
    {
	send_to_char("Cannot locate target character.\n\r", ch);
	return;
    }

    if (IS_NPC(victim))
    {
	send_to_char("NPCs do not have exdescs.\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg1);

    for (ed = victim->pcdata->extra_descr; ed; ed = ed->next)
    {
	if (!str_cmp(arg1, ed->keyword))
	{
	    if (ed->can_edit)
	    {
		ed->can_edit = FALSE;
		sprintf(buf, "Exdesc '%s' is now permanent.\n\r", ed->keyword);
		send_to_char(buf, ch);
	    }
	    else
	    {
		ed->can_edit = TRUE;
		sprintf(buf, "Exdesc '%s' is no longer permament.\n\r", ed->keyword);
		send_to_char(buf, ch);
	    }	

	    return;
	}
    }

    send_to_char("Keyword not found.\n\r", ch);
    return;
}

void do_description( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *vch;	
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    bool offline = FALSE; 
/*    if (IS_NPC(ch))
	return;*/
    
    argument = one_argument(argument, arg);
    if ( arg[0] != '\0' )
    {
	if (is_affected(ch, gsn_rearrange))
	{
	    send_to_char("You cannot alter your description while under the affects of rearrange.\n\r", ch);
	    return;
	}

	buf[0] = '\0';
	smash_tilde( arg );

    	if (arg[0] == '-')
    	{
            int len;
            bool found = FALSE;
 
            if (ch->description == NULL || ch->description[0] == '\0')
            {
                send_to_char("No lines left to remove.\n\r",ch);
                return;
            }
	
  	    strcpy(buf,ch->description);
 
            for (len = strlen(buf); len > 0; len--)
            {
                if (buf[len] == '\r')
                {
                    if (!found)  /* back it up */
                    {
                        if (len > 0)
                            len--;
                        found = TRUE;
                    }
                    else /* found the second one */
                    {
                        buf[len + 1] = '\0';
			free_string(ch->description);
			ch->description = str_dup(buf);
			send_to_char( "Your description is:\n\r", ch );
			send_to_char( ch->description ? ch->description : 
			    "(None).\n\r", ch );
                        return;
                    }
                }
            }
            buf[0] = '\0';
	    free_string(ch->description);
	    ch->description = str_dup(buf);
	    send_to_char("Description cleared.\n\r",ch);
	    return;
        }
	if ( arg[0] == '+' )
	{
	    if ( ch->description != NULL )
		strcat( buf, ch->description );

            if ( strlen(buf) >= MAX_DESCRIPTION_LENGTH)
	    {
	        send_to_char( "Description too long.\n\r", ch );
	        return;
	    }

	    strcat( buf, argument );
	    strcat( buf, "\n\r" );
	    free_string( ch->description );
	    ch->description = str_dup( buf );
    	    send_to_char( "Your description is:\n\r", ch );
    	    send_to_char( ch->description ? ch->description : "(None).\n\r", ch );
    	    return;
	}
       
	/* Code added for viewing descriptions both on and offline.
	 * - transmitt 10/09/05
	 */
	if (!str_cmp(arg, "view"))
	{
            if (!IS_TRUSTED(ch, ANGEL))
            {		    
                send_to_char("You cannot view other backgrounds.\n\r", ch);
		return;
	    }
	    
	    if (argument[0] == '\0')
	    {
	        send_to_char("Syntax: \n\r", ch);
	        send_to_char(" desc view <char>\n\r", ch);
		send_to_char(" desc view offline <char>\n\r", ch);
		return;
            }
            
	    argument = one_argument(argument, arg);
	    if (!str_cmp(arg, "offline"))
            {
                if (vch = load_offline_char(argument))
		    offline = TRUE; 	
		else
		{
                    send_to_char("Character already online or does not exist!\n\r", ch);
		    return;
		}	
            }
            else if ((vch = get_char_world(ch, arg)) == NULL)
	    {
                send_to_char("You cannot sense them in the land.\n\r", ch);
		return;
	    }	    
	    sprintf(buf, "%s's description is:\n\r", vch->name);
	    send_to_char(buf, ch);
	    send_to_char(vch->description ? vch->description : "(None).\n\r", ch);
	    if (offline)
		extract_char(vch,TRUE);
	    return;	    
	}	
	
	if (!str_cmp(arg, "clear"))
	{
	    buf[0] = '\0';
	    free_string(ch->description);
	    ch->description = str_dup(buf);
	    send_to_char("Description cleared.\n\r", ch);
	    return;
	}

	if (!str_cmp(arg, "format"))
	{
	    if (ch->description)
	        ch->description = format_string(ch->description);
	    send_to_char("Description formatted.\n\r", ch);
	    return;
	}

	if (!str_cmp(arg, "edit"))
	{
	    if (ch->orig_description[0] != '\0')
	    {
		send_to_char("You cannot edit your description at this time.\n\r", ch);
		return;
	    }
	    string_append(ch, &ch->description);
	    return;
	}

	sprintf(buf, "Invalid argument '%s'.\n\rPlease read HELP DESCRIPTION for more information.\n\r", arg);
	send_to_char(buf, ch);
	return;
    }

    send_to_char( "Your description is:\n\r", ch );
    send_to_char( ch->description ? ch->description : "(None).\n\r", ch );
    return;
}

void do_report( CHAR_DATA *ch, char *argument )
{
    send_to_char("That command has been taken out.\n\r", ch);
}

void do_practice( CHAR_DATA *ch, char *argument )
{
    bool found=FALSE;
    GM_DATA *pGm;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_STRING_LENGTH];
    char * originalArgument(argument);
    char sbuf[20];
    int sn;
    BUFFER *output;

   argument = one_argument(argument, arg);

    if ( arg[0] == '\0' || (IS_IMMORTAL(ch) && !str_cmp(arg, "char")))
    {
        int col, i, j, s_size, index;
        CHAR_DATA *victim;
        int sort_array[MAX_SKILL];

        if (arg[0] != '\0')  // second case must be true
            victim = get_char_world(ch, argument);
        else
            victim = ch;

        if (!victim)
        {
            send_to_char("Character not found.\n\r", ch);
            return;
        }

        // Locate the correct PC_DATA
        PC_DATA * pcdata(victim->pcdata);
        if (pcdata == NULL)
        {
            CHAR_DATA * original(find_bilocated_body(victim));
            if (original != NULL)
                pcdata = original->pcdata;
        }

        if (pcdata == NULL)
        {
            send_to_char("NPCs cannot get a practice listing.\n\r", ch);
            return;
        }

        // Alright kids, it's time for an insertion sort!
        // Step one, populate sort_array with skill numbers.
        s_size = 0;
        for ( sn = 0; sn < MAX_SKILL; sn++ )
        {
            if ( skill_table[sn].name == NULL )
                break;

            if ( victim->level >= skill_table[sn].skill_level[victim->class_num] 
              && pcdata->learned[sn] >= 1 /* skill is not known */)
            {
                sort_array[s_size] = sn;
                s_size++;
            }
        }

        // Step two, do the actual sort on sort_array.
        for (i = 1; i < s_size; i++)
        {
            index = sort_array[i];
            j = i;

            while ((j > 0) && strcoll(skill_table[sort_array[j - 1]].name, skill_table[index].name) >= 1)
            {
                sort_array[j] = sort_array[j - 1];
            j = j - 1;
            }

            sort_array[j] = index;
        }
            
        // Step three, output the prac list based on sort_array.
        output = new_buf();
        col    = 0;
        for ( i = 0; i < s_size; i++ )
        {
            memset((void *) sbuf, 0, 20);
            strncpy(sbuf, skill_table[sort_array[i]].name, 19);
            sprintf( buf, "%-19s%3d%%  ", sbuf, pcdata->learned[sort_array[i]] );

            add_buf(output, buf);

            if ( ++col % 3 == 0 )
            add_buf(output, "\n\r");
        }

        if ( col % 3 != 0 )
            add_buf(output, "\n\r");

        if (victim == ch)
            sprintf( buf, "You have %d practice sessions left.\n\r", victim->practice );
        else
            sprintf( buf, "%s has %d practice sessions left.\n\r", victim->name, victim->practice);

        add_buf(output, buf);

        page_to_char(buf_string(output), ch);

    }
    else
    {
        CHAR_DATA *mob;
        int adept;
        int skill_increase = 0;
        int max_skill_increase = 100;
        int num_stats = 0;	

        if (IS_NPC(ch))
            return;

        if ( !IS_AWAKE(ch) )
        {
            send_to_char( "In your dreams, or what?\n\r", ch );
            return;
        }

        for ( mob = ch->in_room->people; mob != NULL; mob = mob->next_in_room )
        {
            if ( IS_NPC(mob) && IS_SET(mob->act, ACT_PRACTICE) )
            break;
        }

        if ( mob == NULL )
        {
            send_to_char( "You can't do that here.\n\r", ch );
            return;
        }

        if ( ch->practice <= 0 )
        {
            send_to_char( "You have no practice sessions left.\n\r", ch );
            return;
        }

        if (((sn = skill_lookup_full(originalArgument)) < 0)
        || (((ch->level < skill_table[sn].skill_level[ch->class_num])
        ||   ((skill_table[sn].spell_fun != spell_lang) && (ch->pcdata->learned[sn] < 1)) /* skill is not known */
        ||   (skill_table[sn].rating[ch->class_num] == 0))))
        {
            send_to_char( "You can't practice that.\n\r", ch );
            return;
        }

        for ( pGm = mob->pIndexData->gm; pGm != NULL ; pGm = pGm->next)
        {
        if (pGm->sn == sn)
            {found = TRUE;break;}
        }

        if (!found)
        {
        sprintf(buf, "Apparently %s is unable or unwilling to teach you that.\n\r", PERS(mob, ch));
        send_to_char(buf, ch);
        return;
        }

        if (skill_table[sn].spell_fun == spell_lang)
        {
            // Common may be fully practiced (in 33% chunks)
            // Other languages stop at 33%
            if (sn == gsn_language_common)
            {
                adept = 100;
                max_skill_increase = 33;
            }
            else
                adept = 33;
        }
        else
            adept = IS_NPC(ch) ? 100 : class_table[ch->class_num].skill_adept;

        if ( ch->pcdata->learned[sn] >= adept )
        {
            sprintf( buf, "You are already learned at %s.\n\r",
            skill_table[sn].name );
            send_to_char( buf, ch );
        }
        else
        {
            ch->practice--;

            if (IS_SET(skill_table[sn].attr, ATTR_STR))
            {
            skill_increase += learn_table[get_curr_stat(ch, STAT_STR)];
            num_stats += 1;
            }

            if (IS_SET(skill_table[sn].attr, ATTR_DEX))
            {
            skill_increase += learn_table[get_curr_stat(ch, STAT_DEX)];
            num_stats += 1;
            }

            if (IS_SET(skill_table[sn].attr, ATTR_CON))
            {
            skill_increase += learn_table[get_curr_stat(ch, STAT_CON)];
            num_stats += 1;
            }

            if (IS_SET(skill_table[sn].attr, ATTR_INT))
            {
            skill_increase += learn_table[get_curr_stat(ch, STAT_INT)];
            num_stats += 1;
            }

            if (IS_SET(skill_table[sn].attr, ATTR_WIS))
            {
            skill_increase += learn_table[get_curr_stat(ch, STAT_WIS)];
            num_stats += 1;
            }

            if (IS_SET(skill_table[sn].attr, ATTR_CHR))
            {
            skill_increase += learn_table[get_curr_stat(ch, STAT_CHR)];
            num_stats += 1;
            }

            skill_increase /= num_stats;

            ch->pcdata->learned[sn] += UMIN(skill_increase, max_skill_increase);
            
            if ( ch->pcdata->learned[sn] < adept )
            {
            act( "You practice $T.",
                ch, NULL, skill_table[sn].name, TO_CHAR );
            act( "$n practices $T.",
                ch, NULL, skill_table[sn].name, TO_ROOM );
            }
            else
            {
            ch->pcdata->learned[sn] = adept;
            act( "You are now learned at $T.",
                ch, NULL, skill_table[sn].name, TO_CHAR );
            act( "$n is now learned at $T.",
                ch, NULL, skill_table[sn].name, TO_ROOM );
            }
        }
    }
}

/*
 * 'Wimpy' originally by Dionysos.
 */
void do_wimpy( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int wimpy;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
	wimpy = ch->max_hit / 5;
    else
	wimpy = atoi( arg );

    if ( wimpy < 0 )
    {
	send_to_char( "Your courage exceeds your wisdom.\n\r", ch );
	return;
    }

    if ( wimpy > ((!IS_NPC(ch) && BIT_GET(ch->pcdata->traits, TRAIT_COWARD)) ? ch->max_hit * 3 / 4 : ch->max_hit/2 ))
    {
	send_to_char( "Such cowardice ill becomes you.\n\r", ch );
	return;
    }

    ch->wimpy	= wimpy;
    sprintf( buf, "Wimpy set to %d hit points.\n\r", wimpy );
    send_to_char( buf, ch );
    return;
}


void do_scouting( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (!get_skill(ch, gsn_scouting))
    {   
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (is_affected(ch, gsn_scouting))
    {
      send_to_char("You're already vigilant for motion.\n\r", ch);
      return;
    }

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_scouting].beats));

    if (number_percent() > get_skill(ch, gsn_scouting))
	{
	  send_to_char("You try to keep a lookout, but fail.\n\r", ch);
	  return;
	}

    check_improve(ch, NULL,gsn_scouting, TRUE, 4);

    af.where     = TO_NAFFECTS;
    af.type      = gsn_scouting;
    af.level     = ch->level;
    af.duration  = 8;
	af.point = NULL;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = AFF_SCOUTING;
    affect_to_char( ch, &af );

    act("You begin to watch vigilantly for motion in all directions.", ch, NULL, NULL, TO_CHAR);

}


void do_password( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char *pArg;
    char *pwdnew;
    char *p;
    char cEnd;

    if ( IS_NPC(ch) )
	return;

    /*
     * Can't use one_argument here because it smashes case.
     * So we just steal all its code.  Bleagh.
     */
    pArg = arg1;
    while ( isspace(*argument) )
	argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
	cEnd = *argument++;

    while ( *argument != '\0' )
    {
	if ( *argument == cEnd )
	{
	    argument++;
	    break;
	}
	*pArg++ = *argument++;
    }
    *pArg = '\0';

    pArg = arg2;
    while ( isspace(*argument) )
	argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
	cEnd = *argument++;

    while ( *argument != '\0' )
    {
	if ( *argument == cEnd )
	{
	    argument++;
	    break;
	}
	*pArg++ = *argument++;
    }
    *pArg = '\0';

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
	send_to_char( "Syntax: password <old> <new>.\n\r", ch );
	return;
    }

    if ( strcmp( crypt( arg1, ch->pcdata->pwd ), ch->pcdata->pwd ) )
    {
	WAIT_STATE( ch, 40 );
	send_to_char( "Wrong password.  Wait 10 seconds.\n\r", ch );
	return;
    }

    if ( strlen(arg2) < 5 )
    {
	send_to_char(
	    "New password must be at least five characters long.\n\r", ch );
	return;
    }

    /*
     * No tilde allowed because of player file format.
     */
    pwdnew = crypt( arg2, ch->name );
    for ( p = pwdnew; *p != '\0'; p++ )
    {
	if ( *p == '~' )
	{
	    send_to_char(
		"New password not acceptable, try again.\n\r", ch );
	    return;
	}
    }

    free_string( ch->pcdata->pwd );
    ch->pcdata->pwd = str_dup( pwdnew );
    save_char_obj( ch );
    send_to_char( "Ok.\n\r", ch );
    return;
}

void do_acctpwd( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char *pArg;
    char *pwdnew;
    char *p;
    char cEnd;

    if ( IS_NPC(ch) )
	return;

    if (!ch->desc || !ch->desc->acct)
    {
	send_to_char("Character not attached to player account.\n\r", ch);
	return;
    }

    /*
     * Can't use one_argument here because it smashes case.
     * So we just steal all its code.  Bleagh.
     */
    pArg = arg1;
    while ( isspace(*argument) )
	argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
	cEnd = *argument++;

    while ( *argument != '\0' )
    {
	if ( *argument == cEnd )
	{
	    argument++;
	    break;
	}
	*pArg++ = *argument++;
    }
    *pArg = '\0';

    pArg = arg2;
    while ( isspace(*argument) )
	argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
	cEnd = *argument++;

    while ( *argument != '\0' )
    {
	if ( *argument == cEnd )
	{
	    argument++;
	    break;
	}
	*pArg++ = *argument++;
    }
    *pArg = '\0';

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
	send_to_char( "Syntax: password <old> <new>.\n\r", ch );
	return;
    }

    if ( strcmp( crypt( arg1, ch->desc->acct->pwd ), ch->desc->acct->pwd ) )
    {
	WAIT_STATE( ch, 40 );
	send_to_char( "Wrong password.  Wait 10 seconds.\n\r", ch );
	return;
    }

    if ( strlen(arg2) < 5 )
    {
	send_to_char(
	    "New password must be at least five characters long.\n\r", ch );
	return;
    }

    /*
     * No tilde allowed because of player file format.
     */
    pwdnew = crypt( arg2, ch->name );
    for ( p = pwdnew; *p != '\0'; p++ )
    {
	if ( *p == '~' )
	{
	    send_to_char(
		"New password not acceptable, try again.\n\r", ch );
	    return;
	}
    }

    free_string( ch->desc->acct->pwd );
    ch->desc->acct->pwd = str_dup( pwdnew );
    save_char_obj( ch );
    send_to_char( "Ok.\n\r", ch );
    return;
}

void do_acctinfo(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];
    BUFFER *output;

    if (argument[0] == '\0')
    {
	send_to_char("Who do you wish to view the account info of?\n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, argument)) == NULL)
    {
	send_to_char("You cannot locate that person in the world.\n\r", ch);
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
   
    output = new_buf();

    sprintf(buf, "Account Name: %s\n\r", victim->desc->acct->name);
    add_buf(output,buf);

    if (victim->desc->acct->flags)
    {
	sprintf(buf, "Account Flags: %s\n\r", flag_string(acct_flags, victim->desc->acct->flags));
	add_buf(output,buf);
    }

    sprintf(buf, "Account Points: %d\n\r", victim->desc->acct->award_points);
    add_buf(output,buf);
    sprintf(buf, "Active Characters: %s\n\r", victim->desc->acct->chars);
    add_buf(output,buf);

    sprintf(buf, "Deleted Characters:\n\r%s\n\r", victim->desc->acct->deleted);
    add_buf(output,buf);

    if (victim->desc->acct->immrecord[0] != '\0')
    {
	sprintf(buf, "Account record:\n\r%s\n\r", victim->desc->acct->immrecord);
	add_buf(output,buf);
    }

    if (victim->desc->acct->socket_info[0] != '\0')
    {
	sprintf(buf, "\n\rSocket record:\n\r%s\n\r", victim->desc->acct->socket_info);
	add_buf(output,buf);
    }
    
    page_to_char(buf_string(output),ch);
    return;
}

DO_FUNC(do_finquiry)
{
    CHAR_DATA *fmob, *victim;
    char buf[MAX_STRING_LENGTH], arg[MAX_INPUT_LENGTH];

    if (argument[0] == '\0')
    {
	send_to_char("With whom do you wish to inquire?\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg);

    if ((fmob = get_char_room(ch, arg)) == NULL)
    {
	send_to_char("You don't see them here.\n\r", ch);
	return;
    }

    if (!IS_NPC(fmob))
    {
	send_to_char("Ask them yourself!\n\r", ch);
	return;
    }

    if (!fmob->pIndexData->lang_flags)
    {
	sprintf(buf, "%s ignores you.\n\r", PERS(fmob, ch));
	*buf = UPPER(*buf);
	send_to_char(buf, ch);
	return;
    }

    const Faction * faction(FactionTable::LookupFor(*fmob));
    if (faction == NULL || faction->HasFlag(Faction::Fake))
    {
        do_tell_target(fmob, ch, "I hold no allegiance to any known faction in this land.");
        return;
    }

    if (argument[0] == '\0')
    {
        switch (FactionTable::CurrentStanding(*ch, *fmob))
        {
            case Rating_Friend: 
                sprintf(buf, "Greetings, %s. You are known as a friend of %s.", PERS(ch, fmob), faction->Name().c_str());
                break;

            case Rating_Enemy:
                sprintf(buf, "Begone, %s! You are an enemy of %s!", PERS(ch, fmob), faction->Name().c_str());
                break;

            default:
                sprintf(buf, "You are relatively unknown to %s!", faction->Name().c_str());
                break;
        }
        
        do_tell_target(fmob, ch, buf);
        return;
    }

    if ((victim = get_char_world(ch, argument)) == NULL || IS_NPC(victim))
    {
        send_to_char("You cannot see the target of your inquiry in the land.\n\r", ch);
        return;
    }

    if (FactionTable::CurrentStanding(*ch, *fmob) != Rating_Friend)
    {
        sprintf(buf, "You are not a friend of %s, I will not answer such things.", faction->Name().c_str());
        do_tell_target(fmob, ch, buf);
        return;
    }

    switch (FactionTable::CurrentStanding(*victim, *fmob))
    {
        case Rating_Friend:
	        sprintf(buf, "%s is a friend of %s.", PERS(victim, ch), faction->Name().c_str());
            break;

        case Rating_Enemy:
	        sprintf(buf, "%s is an enemy of %s.", PERS(victim, ch), faction->Name().c_str());
            break;

        default:
	        sprintf(buf, "%s is relatively unknown to %s.", PERS(victim, ch), faction->Name().c_str());
            break;
    }

    do_tell_target(fmob, ch, buf);
}

void sentrynotify(CHAR_DATA *ch, int enter)
{
    DESCRIPTOR_DATA *d;
    AFFECT_DATA *paf;
    char buf[MAX_STRING_LENGTH];

    if (ch->in_room == NULL || IS_AFFECTED(ch,AFF_WIZI))
	return;

    for (paf=ch->in_room->affected;paf;paf=paf->next)
    {
	if (paf->type != gsn_sentry)
	    continue;
	for (d=descriptor_list;d;d = d->next)
	    if (ch->id != paf->modifier && d->character && d->character->id == paf->modifier)
	    {
		if (enter == 6)
		    sprintf(buf,"{gA sentry tells you, '%s just arrived in %s.'{x\n\r",
		      PERS(ch,d->character),ch->in_room->name);
		else
		    sprintf(buf,"{gA sentry tells you, '%s just left %s, heading %s.'{x\n\r",
		      PERS(ch,d->character),ch->in_room->name,dir_name[enter]);
		send_to_char(buf,d->character);
	    }
    }
}

