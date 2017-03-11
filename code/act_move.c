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
*       ROM 2.4 is copyright 1993-1996 Russ Taylor                         *
*       ROM has been brought to you by the ROM consortium                  *
*           Russ Taylor (rtaylor@efn.org)                                  *
*           Gabrielle Taylor                                               *
*           Brian Moore (zump@rom.org)                                     *
*       By using this code, you have agreed to follow the terms of the     *
*           Brian Moore (zump@rom.org)                                     *
*       By using this code, you have agreed to follow the terms of the     *
*       ROM license, in the file Rom24/doc/rom.license                     *
***************************************************************************/

#if defined(macintosh)
#include <types.h>
#include <time.h>
#else
#include <sys/types.h>
#endif
#if defined(unix)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "magic.h"
#include "songs.h"
#include "lookup.h"
#include "psionics.h"
#include "tables.h"
#include "spells_fire.h"
#include "spells_spirit.h"
#include "spells_water.h"
#include "spells_air.h"
#include "spells_earth.h"
#include "spells_void.h"
#include "Direction.h"
#include "Encumbrance.h"

/* command procedures needed */
DECLARE_DO_FUN(do_look          );
DECLARE_DO_FUN(do_autoyell      );
DECLARE_DO_FUN(do_recall        );
DECLARE_DO_FUN(do_stand         );
DECLARE_DO_FUN(do_say           );


static bool check_handle_forced_sleep(CHAR_DATA * ch, CHAR_DATA * waker = NULL);
void send_move_to_wizi(CHAR_DATA *ch, char *buf);
void send_move_to_ds(CHAR_DATA *ch, char *buf);

char *  const   dir_name        []              =
{
    "north", "east", "south", "west", "up", "down"
};

char *  const   dir_name_nice   []              =
{
    "the north", "the east", "the south", "the west", "above", "blow"
};

char *  const   rdir_name       []              =
{
    "south", "west", "north", "east", "down", "up"    	
};

char *  const   rdir_nice_name  []		=
{
    "the south", "the west", "the north", "the east", "below", "above"
};


const   int  rev_dir         []              =
{
    2, 3, 0, 1, 5, 4
};

// brazen: Ticket #235: City rooms (the second value here) used to cost 2 mv
// Also added a value for swamp, since it was uninitiated
const   int  movement_loss   [SECT_MAX]      =
{
    1, 1, 2, 3, 4, 6, 4, 1, 6, 10, 6, 3, 5, 1, 6
};


CHAR_DATA *get_scouter(CHAR_DATA *ch, ROOM_INDEX_DATA *room, CHAR_DATA *last_scout);
/*
 * Local functions.
 */
bool	has_boat	args( ( CHAR_DATA *ch ) );
bool    has_key         args( ( CHAR_DATA *ch, int key ) );
void    add_tracks      args( ( ROOM_INDEX_DATA *room, CHAR_DATA *ch, int door)
);


extern bool global_bool_final_valid;
extern bool global_bool_check_avoid;

void move_char( CHAR_DATA *ch, int door, bool follow, bool charmCanMove)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    CHAR_DATA *fch, *scouter = NULL;
    CHAR_DATA *fch_next;
    CHAR_DATA *vch, *vch_next;
    CHAR_DATA *linked = ch->mount ? ch->mount : (ch->rider ? ch->rider : NULL);
    CHAR_DATA *tch;
    ROOM_INDEX_DATA *in_room;
    ROOM_INDEX_DATA *to_room;
    EXIT_DATA *pexit;
    OBJ_DATA *obj;
    AFFECT_DATA af, *paf;
af.valid = TRUE;
af.point = NULL;
    int  skill, spines = 0;
    bool rune_saved, sleeping = FALSE;

    if (is_affected(ch,gsn_charge))
	follow = FALSE;

    if ( door < 0 || door > 5 )
    {
        bug( "Do_move: bad door. door: %d",door);
        return;
    }

    if (follow && (is_affected(ch, gsn_confusion) || (ch->mount && is_affected(ch->mount, gsn_confusion))))
	door = number_range(0, 5);

    in_room = ch->in_room;
    if ( ( pexit   = in_room->exit[door] ) == NULL
    ||   ( to_room = pexit->u1.to_room   ) == NULL
    ||   !can_see_room(ch,pexit->u1.to_room))
    {
        send_to_char( "Alas, you cannot go that way.\n\r", ch );
        return;
    }

    if (ch->mount && !can_see_room(ch->mount, pexit->u1.to_room))
    {
	send_to_char("Alas, your mount cannot go that way.\n\r", ch);
	return;
    }

    if (IS_SET(pexit->exit_info, EX_FAKE) && ch->level < 52)
    {
        send_to_char( "Alas, you cannot go that way.\n\r", ch );
        return;
    }

    // Check for mirage
    if (!IS_OAFFECTED(ch, AFF_GHOST) && area_is_affected(in_room->area, gsn_mirage))
    {
        // Check for disillusionment
        if (number_percent() <= get_skill(ch, gsn_disillusionment))
            check_improve(ch, NULL, gsn_disillusionment, true, 8);
        else
        {
            check_improve(ch, NULL, gsn_disillusionment, false, 8);

            // Check whether this exit will be rearranged
            srand(in_room->vnum + door + 10101);
            if (rand() % 100 <= 25)
            {
                // Exit will be rearranged, so find a new exit
                std::vector<std::pair<Direction::Value, ROOM_INDEX_DATA*> > directions;
                for (unsigned int i(0); i < Direction::Max; ++i)
                {
                    Direction::Value direction(static_cast<Direction::Value>(i));
                    ROOM_INDEX_DATA * nextRoom(Direction::Adjacent(*in_room, direction));
                    if (nextRoom != NULL && can_see_room(ch, nextRoom))
                        directions.push_back(std::make_pair(direction, nextRoom));
                }

                if (!directions.empty())
                {
                    // Choose a new random direction
                    std::pair<Direction::Value, ROOM_INDEX_DATA*> exitInfo(directions[number_range(0, directions.size() - 1)]);
                    door = exitInfo.first;
                    pexit = in_room->exit[door];
                    to_room = exitInfo.second;
                }
            }
            srand(time(0));
        }
    }

    if (!IS_IMMORTAL(ch) && !IS_AFFECTED(ch, AFF_WIZI) && !IS_OAFFECTED(ch, AFF_GHOST))
    {
        if (check_webofoame_caught(*ch))
        {
            send_to_char("You are too tangled in the web to move!\n", ch);
            return;
        }

        if (!IS_AFFECTED(ch, AFF_PASS_DOOR) && !IS_PAFFECTED(ch, AFF_VOIDWALK))
        {
            if (IS_SET(pexit->exit_info, EX_WALLED))
            {
                act( "The $d is blocked by a wall of stone.", ch, NULL, pexit->keyword, TO_CHAR );
                return;
            }

            if (IS_SET(pexit->exit_info, EX_ICEWALL))
            {
                act("That way is blocked by an impenetrable sheet of ice.", ch, NULL, NULL, TO_CHAR);
                return;
            }
        }

        if (IS_SET(pexit->exit_info, EX_WEAVEWALL) && !is_affected(ch, gsn_gaseousform))
        {
            act("That way is blocked by a shimmering golden mesh of energy and light.", ch, NULL, NULL, TO_CHAR);
            return;
        }

        if (IS_SET(pexit->exit_info, EX_CLOSED)
        &&  ((!IS_AFFECTED(ch, AFF_PASS_DOOR) && !IS_PAFFECTED(ch, AFF_VOIDWALK))
            || IS_SET(pexit->exit_info,EX_NOPASS)
            || IS_SET(pexit->exit_info, EX_RUNEOFEARTH)))
        {
            if (IS_SET(pexit->exit_info, EX_SECRET))
                act( "Alas, you cannot go that way.\n\r", ch, NULL, NULL, TO_CHAR );
            else
                act( "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR );

            return;
        }

        // Check for figment's cage
        AFFECT_DATA * figment(get_room_affect(in_room, gsn_figmentscage));
        if (figment != NULL)
        {
            // Disillusionment will negate the effects
            if (number_percent() <= get_skill(ch, gsn_disillusionment))
                check_improve(ch, NULL, gsn_disillusionment, true, 6);
            else
            {
                check_improve(ch, NULL, gsn_disillusionment, false, 6);

                // Check for a save
                if (!saves_spell(figment->level, NULL, ch, DAM_ILLUSION))
                {
                    send_to_char("That way is blocked by thick walls of shimmering energy.\n", ch);
                    WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));
                    return;
                }
            }

            send_to_char("You realize the walls of shimmering energy are illusory, and pass right through them!\n", ch);
            act("$n passes through the walls of shimmering energy, shattering the illusion of confinement!", ch, NULL, NULL, TO_ROOM);
            room_affect_strip(in_room, gsn_figmentscage);
        }
    }

    if (!charmCanMove && IS_AFFECTED(ch, AFF_CHARM) && !IS_AFFECTED(ch, AFF_CHARM_FREEAGENT) && ch->master != NULL && in_room == ch->master->in_room)
    {
        send_to_char( "What?  And leave your beloved master?\n\r", ch );
        return;
    }

    if ( !is_room_owner(ch,to_room) && room_is_private( to_room ) )
    {
        send_to_char( "That room is private right now.\n\r", ch );
        return;
    }

    if (IS_NAFFECTED(ch, AFF_GARROTE_VICT))
    {
        send_to_char( "You can't move anywhere with your throat caught.\n\r", ch);
        return;
    }

    if (room_is_affected(ch->in_room, gsn_earthmaw) && !is_flying(ch))
    {
	send_to_char("You are trapped within the earth.\n\r", ch);
	return;
    }

    AFFECT_DATA * boilAff(get_affect(ch, gsn_boilblood));
    if (boilAff != NULL && number_bits(5 - boilAff->modifier) == 0)
    {
        send_to_char("Your boiling blood pains you too much to move!\n", ch);
        WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));
        return;
    }
    
    /// End of "can you move" checks, start of "actions happening upon move" checks
    if (is_affected(ch, gsn_airrune) || is_affected(ch, gsn_runeofair))
    {
        affect_strip(ch, gsn_airrune);
        affect_strip(ch, gsn_runeofair);
        send_to_char("You disturb the air rune, and shimmer back into view.\n\r", ch);
        act("$n shimmers into view.", ch, NULL, NULL, TO_ROOM);
    }

    if (is_affected(ch, gsn_meldwithstone))
    {
        affect_strip(ch, gsn_meldwithstone);
        act("You grow restless in the earth, and rise up from the ground.", ch, NULL, NULL, TO_CHAR);
        act("With a low rumble, the ground seems to part as $n flows up from it and reforms.", ch, NULL, NULL, TO_ROOM);
    }

    if (is_affected(ch, gsn_flameunity))
    {
        affect_strip(ch, gsn_flameunity);
        act("You stir, recorporealizing from the flames.", ch, NULL, NULL, TO_CHAR);
        act("$n emerges from the flames, unharmed despite the ash which swirls about $m.", ch, NULL, NULL, TO_ROOM);
    }

    if (IS_OAFFECTED(ch, AFF_SHADOWMASTERY))
	REMOVE_BIT(ch->oaffected_by, AFF_SHADOWMASTERY);	

    if (IS_SET(pexit->exit_info, EX_WALLOFFIRE)
     && ch->level < 52
     && !IS_OAFFECTED(ch, AFF_GHOST)
     && !IS_PAFFECTED(ch, AFF_VOIDWALK))
    {
        act("$n is burned by the wall of fire!", ch, NULL, NULL, TO_ROOM);
        act("You are burned by the wall of fire!", ch, NULL, NULL, TO_CHAR);
        damage( ch, ch, number_range(ch->level * 2, ch->level * 4), gsn_walloffire,DAM_FIRE,TRUE);

        if (!ch || !ch->valid || IS_OAFFECTED(ch, AFF_GHOST))
       	    return;

	if (linked)
	{
            act("$n is burned by the wall of fire!", linked, NULL, NULL, TO_ROOM);
            act("You are burned by the wall of fire!", linked, NULL, NULL, TO_CHAR);
            damage( ch, ch, number_range(linked->level, linked->level*3), gsn_walloffire,DAM_FIRE,TRUE);

            if (!linked || !linked->valid || IS_OAFFECTED(linked, AFF_GHOST))
		linked = NULL;
	}
    }

    for (vch = ch->in_room->people; vch != NULL;vch = vch_next)
    {
        vch_next = vch->next_in_room;

        if (vch == ch)
          continue;

	tch = ch->rider ? ch->rider : ch;

// brazen: guard should not trigger for groupmates
        if (IS_NAFFECTED(vch, AFF_GUARDING) && is_affected(vch, gsn_guard)
        && IS_AWAKE(vch)
        && get_modifier(vch->affected, gsn_guard) == door
        && !IS_NPC(tch) && IS_PK(tch, vch) && can_see(vch, tch)
        && !is_safe(vch, tch)
	&& !is_same_group(ch,vch))
        {
            sprintf(buf, "Help! %s is attacking me!", PERS(vch, tch));
            do_autoyell(tch, buf);
            act("$N leaps at $n, cutting $m off as $e tries to leave.", tch, NULL, vch, TO_NOTVICT);
            act("$N leaps at you, cutting you off as you try to leave.", tch, NULL, vch, TO_CHAR);
            act("You leap at $n, cutting $m off as $e tries to leave.", tch, NULL, vch, TO_VICT);
            multi_hit(vch, tch, TYPE_UNDEFINED);
            return;
        }
    }


    if (ch->fighting != NULL && global_bool_final_valid )
    {
        for (vch = ch->in_room->people;vch != NULL; vch = vch_next)
        {
            vch_next = vch->next_in_room;
            if (vch->fighting != ch || IS_OAFFECTED(ch, AFF_GHOST) || get_eq_char(vch, WEAR_WIELD) == NULL)
                continue;

            bool shouldStrike(number_percent() <= get_skill(vch, gsn_finalstrike));
            bool alreadyEchoed(false);

            // Check for ethereal brethren
            AFFECT_DATA * brethren(get_affect(vch, gsn_etherealbrethren));
            if (brethren != NULL && number_percent() <= 15 && brethren->duration >= 1)
            {
                brethren->duration -= 1;
                act("Acting on an instinct you never formed, you strike at $N as $E tries to flee!", vch, NULL, ch, TO_CHAR);
                shouldStrike = true;
                alreadyEchoed = true;
            }

            if (shouldStrike)
            {
                if (!alreadyEchoed)
                    act("You deliver a final strike at $N as $E tries to flee!", vch, NULL, ch, TO_CHAR);

                act("$n delivers a final strike at $N as $E tries to flee!", vch, NULL, ch, TO_NOTVICT);
                act("$n delivers a final strike at you as you try to flee!", vch, NULL, ch, TO_VICT);
                check_improve(vch,ch,gsn_finalstrike,TRUE,2);
                global_bool_check_avoid = FALSE;
                one_hit(vch, ch, TYPE_UNDEFINED, HIT_PRIMARY, false);
                global_bool_check_avoid = TRUE;
                global_bool_final_valid = FALSE;
                if (!ch || !ch->valid || IS_OAFFECTED(ch, AFF_GHOST))
                    return;
            }
            else
                check_improve(vch,ch,gsn_finalstrike,FALSE,2);
        }
    }

    if (linked && linked->fighting && global_bool_final_valid )
        for (vch = linked->in_room->people;vch != NULL; vch = vch_next)
	{
            vch_next = vch->next_in_room;
            if ((vch->fighting == linked) && !IS_NPC(vch) 
	     && ((skill = get_skill(vch, gsn_finalstrike)) > 0))
	    {
                if (skill > number_percent() && get_eq_char(vch, WEAR_WIELD) != NULL)
                {
                    act("$n delivers a final strike at $N as $E tries to flee!", vch, NULL, linked, TO_NOTVICT);
                    act("You deliver a final strike at $N as $E tries to flee!", vch, NULL, linked, TO_CHAR);
                    act("$n delivers a final strike at you as you try to flee!", vch, NULL, linked, TO_VICT);
                    check_improve(vch,linked,gsn_finalstrike,TRUE,2);
                    global_bool_check_avoid = FALSE;
                    one_hit(vch, linked, TYPE_UNDEFINED, HIT_PRIMARY, false);
                    global_bool_check_avoid = TRUE;
                    global_bool_final_valid = FALSE;
                    if (!linked || !linked->valid || IS_OAFFECTED(linked, AFF_GHOST))
                  	return;
                }
                else
                    check_improve(vch,ch,gsn_finalstrike,FALSE,2);
	    }
	}


    if (!IS_NPC(ch) && (ch->level < 52))
    {
        int move;
        bool found=FALSE;
        CHAR_DATA *pMob;
        int i;

	if (to_room->room_flags & ROOM_GUILD)
        for (pMob = ch->in_room->people; pMob != NULL ; pMob = pMob->next_in_room)
        {
            if (IS_NPC(pMob) && (pMob->act & ACT_GUILDGUARD))
            {
                for (i = 0; class_table[ch->class_num].guildrooms[i] ; i++)
                {
                    if (to_room->vnum == class_table[ch->class_num].guildrooms[i])
                    {
                    	found = TRUE;
                        break;
                    }
                }
                if (!found)
                {
                    do_say(pMob, "You cannot violate the sanctity of this guild.");
                    return;
                }
                if (ch->pcdata->adrenaline > 0)
                {
		    do_say(pMob, "You cannot enter this place of study before you calm down.");
                    return;
                }

		if (ch->mount)
		{
		    act("$n gestures towards $N.", pMob, NULL, ch->mount, TO_ROOM);
		    do_say(pMob, "You cannot come in here with that.");
		    return;
		}
            }
        }


        if (room_is_affected(in_room, gsn_stonetomud) && number_bits(1) == 0 && !is_flying(ch))
        {
            if (!ch->mount && is_affected(ch, gsn_stabilize))
            {
                act("In tune with the earth, $n is able to slide easily through the mud.", ch, NULL, NULL, TO_ROOM);
                act("In tune with the earth, you are able to slide easily through the mud.", ch, NULL, NULL, TO_CHAR);
            }
            else
            {
                act("You try to move, but you are stuck in the mud!", ch, NULL, NULL, TO_CHAR);
                act("$n tries to move, but is stuck in the mud!", ch, NULL, NULL, TO_ROOM);
                WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
                return;
            }
        }

        if (is_affected(ch, gsn_quicksand) && (number_bits(1) == 0) && !is_flying(ch) && (ch->mount || !is_affected(ch, gsn_stabilize)))
        {
            act("You try to move, but remain mired in the quicksand!", ch, NULL, NULL, TO_CHAR);
            act("$n tries to move, but remains mired in quicksand!", ch, NULL, NULL, TO_ROOM);
            WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
            return;
        }

        if (is_affected(ch, gsn_setsnare) && (ch->class_num != global_int_class_ranger))
        {
            send_to_char("Caught in the snare, you are unable to leave!\n\r", ch);
            return;
        }

	if (ch->mount && is_affected(ch->mount, gsn_setsnare))
	{
	    send_to_char("Your mount is caught in the snare, and you are unable to leave!\n\r", ch);
	    return;
	}

	if (!IS_IMMORTAL(ch) && !IS_OAFFECTED(ch, AFF_GHOST))
    {
	    for (paf = ch->affected; paf; paf = paf->next)
        {
	        if ((paf->type == gsn_abominablerune) && (paf->modifier == (to_room->vnum * -1)))
	        {
		        send_to_char("Your mind balks, unwilling to face the horror of the abominable rune again so soon.\n\r", ch);
		        return;
	        }
        }
    }

	if (!IS_OAFFECTED(ch, AFF_GHOST) && room_is_affected(to_room, gsn_wallofair) && (ch->mount || ((ch->class_num != global_int_class_airscholar) && (ch->class_num != global_int_class_airtemplar))) && (number_bits(1) == 0))
	{
	    send_to_char("You are driven back by a fiercely blowing wall of air!\n\r", ch);
	    WAIT_STATE(ch, UMAX(ch->wait, 10));
	    return;
	}

        if ((paf = affect_find(ch->in_room->area->affected,gsn_creepingcurse)) != NULL
          && (ch->in_room->sector_type == SECT_FOREST
	    || ch->in_room->sector_type == SECT_SWAMP
	    || ch->in_room->sector_type == SECT_FIELD
	    || ch->in_room->sector_type == SECT_ROAD
	    || ch->in_room->sector_type == SECT_HILLS
	    || ch->in_room->sector_type == SECT_MOUNTAIN
	    || ch->in_room->sector_type == SECT_WATER_SWIM
	    || ch->in_room->sector_type == SECT_WATER_NOSWIM
	    || ch->in_room->sector_type == SECT_UNDERWATER)
          && (IS_NPC(ch) ? ch->class_num != global_int_class_druid 
		: ch->pcdata->minor_sphere != paf->modifier)
          && !IS_PAFFECTED(ch, AFF_VOIDWALK))
        {
	    if (number_bits(2) != 0)
	    {
		switch(paf->modifier)
		{
		    case SPH_FIRIEL:
			send_to_char("The plants tear at your clothing, looking for purchase.\n\r", ch);
			damage( ch, ch, number_range(4, ch->level < 6 ? 6 : ch->level), gsn_creepingcurse,DAM_PIERCE,TRUE);
			WAIT_STATE(ch, UMAX(ch->wait, 12));
			break;

		    case SPH_LUNAR:
			send_to_char("The plants swarming around you tear at your flesh!\n\r", ch);
			damage( ch, ch, number_range(4, ch->level < 6 ? 6 : ch->level), gsn_creepingcurse,DAM_SLASH,TRUE);
			break;

		    case SPH_GAMALOTH:
			send_to_char("Pale roots and sickly branches grasp your body!\n\r", ch);
			damage( ch, ch, number_range(4, ch->level < 6 ? 6 : ch->level), gsn_creepingcurse,DAM_POISON,TRUE);
			WAIT_STATE(ch, UMAX(ch->wait, 6));
                	break;

			send_to_char("You avoid the spiny thorns of the creeping curse.\n\r", ch);
			break;
            	}
	    }
	    else
		send_to_char("You avoid the spiny thorns of the creeping curse.\n\r", ch);
        }

// need to add a mount check here

        if (to_room->sector_type == SECT_UNDERWATER && in_room->sector_type != SECT_UNDERWATER && !IS_NPC(ch))
        {
            if (!IS_PAFFECTED(ch, AFF_AIRLESS) && !IS_OAFFECTED(ch, AFF_GHOST) && (ch->level < 53))
            {
                if (get_skill(ch,gsn_swim) < 50)
                {
                            send_to_char( "You can't breathe underwater.\n\r", ch );
                            return;
                }
            }
        }

        if (is_affected(ch, gsn_matrix) && in_room->area != to_room->area)
        {
            send_to_char("The strange glow of the matrix prevents you from going that way.\n\r", ch);
            return;
        }

// need to add a mount check here

        if (in_room->sector_type == SECT_AIR
        ||   to_room->sector_type == SECT_AIR)
        {
            if (!is_flying(ch) && !IS_IMMORTAL(ch))
            {
                send_to_char( "You aren't flying.\n", ch );
                return;
            }
        }


// need to add a mount check here

        if (( in_room->sector_type == SECT_WATER_NOSWIM
        ||    to_room->sector_type == SECT_WATER_NOSWIM )
    	&& !room_is_affected(to_room, gsn_freeze)
        && !is_affected(ch, gsn_waterwalk)
        && !is_affected(ch, gsn_waterbreathing)
	    && !is_affected(ch, gsn_aquamove)
        && !is_flying(ch))
        {
           if ( !has_boat(ch) && get_skill(ch, gsn_swim) <= 0 && get_skill(ch, gsn_waveborne) <= 0 && !is_water_room(*in_room))
            {
                send_to_char( "You need a boat to go there.\n\r", ch );
                return;
            }
        }

        if (room_is_affected(to_room,gsn_sanctify))
	{
            if (IS_OAFFECTED(ch, AFF_DEMONPOS)
             && number_bits(2) != 0) 
            {
                send_to_char("The demon within you snarls angrily!\n\r",ch);
                send_to_char("{gThe demon tells you, 'We will not go that way.'\n\r{x",ch);
                send_to_char("{gThe demon tells you, 'It... burns.'\n\r{x",ch);
	        WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));
                return;
	    }

	    if (ch->mount && (ch->mount->race == race_lookup("demon")))
	    {
		send_to_char("Your mount balks and snarls angrily, unwilling to go that way.\n\r", ch);
		return;
	    }
        }
        
        if (room_is_affected(to_room,gsn_sanctify)
        && ch->desc
        && (ch->desc->original != NULL)
        && (number_bits(2) != 0))
        {
            send_to_char("A holy aura repels you from your destination.\n\r",ch); 
	    WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));
            return;
        }

        if (room_is_affected(to_room,gsn_sanctify) 
        && IS_OAFFECTED(ch, AFF_UNCLEANSPIRIT))
        {
	    send_to_char("The unclean spirit inside of you prevents you from going there.\n\r", ch);
            return;
        }

       if (IS_SET(pexit->exit_info, EX_WALLOFVINES))
            if (ch->class_num == global_int_class_druid)
	    {
	    	act("The wall of vines parts as you approach.", ch, NULL, NULL, TO_CHAR);
	    	act("The wall of vines parts as $n approaches.", ch, NULL, NULL, TO_ROOM);
	    }
	    else if (IS_AFFECTED(ch,AFF_PASS_DOOR))
	    {
		act("The wall of vines slows your passage slightly.",ch,NULL,NULL,TO_CHAR);
		act("The wall of vines slows $n's passage slightly.",ch,NULL,NULL,TO_ROOM);
		WAIT_STATE(ch, UMAX(ch->wait, 4));
	    }
	    else
	    {
		act("The wall of vines slows your movement to a crawl, but you pass through.", ch, NULL, NULL, TO_CHAR);
		act("The wall of vines slows $n's movement to a crawl, but $e passes through.", ch, NULL, NULL, TO_ROOM);
		WAIT_STATE(ch, UMAX(ch->wait, 24));
	     }
	    
        move = movement_loss[UMIN(SECT_MAX-1, in_room->sector_type)]
             + movement_loss[UMIN(SECT_MAX-1, to_room->sector_type)];

        move /= 2;  /* i.e. the average */

	if (IS_SET(ch->in_room->room_flags, ROOM_ROUGH) && !is_flying(ch))
	    move += 2;

        if (is_affected(ch, gsn_smoothterrain) && ON_GROUND(ch))
            move = UMIN(1, move);

        if (((ch->master && is_affected(ch->master, gsn_forestwalk)) 
	    || is_affected(ch, gsn_forestwalk)) 
	  && ((ch->in_room->sector_type == SECT_FOREST) 
	    || (ch->in_room->sector_type == SECT_SWAMP)))
	    move = UMIN(1, move);

        if (is_affected(ch, gsn_mudfootcurse)) ++move;
        if (is_affected(ch, gsn_anchor)) ++move;
        if (is_affected(ch, gsn_density)) move = (move * 5)/2;
        if (is_affected(ch, gsn_plantentangle)) move *= 2;

        if (area_is_affected(ch->in_room->area, gsn_gravitywell) && !IS_NPC(ch) && !is_affected(ch, gsn_stabilize))
            move *= 3;

        if (IS_NAFFECTED(ch, AFF_LEGSHATTER) && !is_flying(ch)) move *= 2;
        if (is_affected(ch, gsn_kneeshatter) && !is_flying(ch)) move *= 3;
        if (area_is_affected(in_room->area, gsn_icestorm)) move *= 3;
        if (is_affected(ch, gsn_stoneshell)) move *= 3;
        if (is_affected(ch, gsn_encase)) move *= 2;

        // Encumbrance penalities
        Encumbrance::Level encumbranceLevel(Encumbrance::LevelFor(*ch));
        switch (encumbranceLevel)
        {
            case Encumbrance::None:     break;
            case Encumbrance::Light:    break;
            case Encumbrance::Medium:   move *= 2; break;
            case Encumbrance::Heavy:    move *= 3; break;
            case Encumbrance::Max:      move *= 6; break;
        }

        // Grimseep calculation; fliers are unaffected here
        GrimseepAffiliation grimseep(check_grimseep_affiliation(*ch, *in_room));
        if (is_flying(ch)) grimseep = Grimseep_None;
        
        switch (grimseep)
        {
            case Grimseep_None: break;
            case Grimseep_Aided: move /= 2; break;
            case Grimseep_Hindered: move *= 2; break;
        }        

        /* conditional effects */
        if ((is_flying(ch) && !IS_AFFECTED(ch,AFF_FLY_NATURAL)) || IS_AFFECTED(ch, AFF_HASTE))
        {
            if (number_percent() <= get_skill(ch, gsn_controlledflight))
            {
                check_improve(ch, NULL, gsn_controlledflight, true, 12);
                move /= 2;
            }
            else
            {
                move = ((move * 3) / 4);
                check_improve(ch, NULL, gsn_controlledflight, false, 12);
            }
        }

        if (is_affected(ch, gsn_hamstring))
            move *= 2;

        if (IS_AFFECTED(ch,AFF_SLOW))
            move *= 2;

	if (!IS_NPC(ch) && ch->pcdata->condition[COND_HUNGER] > get_max_hunger(ch))
	    move *= 2;
	
	if (is_affected(ch,gsn_exhaustionpoison))
	    if (get_modifier(ch->affected,gsn_exhaustionpoison) == -4)
		move *= 4;
	    else
		move *= 2;

	if (get_group_song(ch, gsn_marchingtune)
	  && can_be_affected(ch,gsn_marchingtune))
	    move = (move - 1) / 2;

	if (IS_AFFECTED(ch, AFF_FLY_NATURAL))
	    move -= 1;

	if (is_affected(ch, gsn_celerity))
	    move = 1;	
    
    AFFECT_DATA * calcify(get_affect(ch, gsn_calcify));
    if (calcify != NULL) move += (calcify->modifier / 10);

    if (move < 1)
	    move = 1;

    if (IS_OAFFECTED(ch, AFF_GHOST))
        move = 0;

	// Should be last movement check to make sure swim doesn't improve when
	// you wouldn't be able to move regardless of its success...
        if ((in_room->sector_type == SECT_WATER_NOSWIM) && !found)
	{
	    if (IS_NAFFECTED(ch, AFF_FORCEDMARCH) || ch->move >= move)
	    {
    		if (number_percent() <= get_skill(ch, gsn_swim)) check_improve(ch,NULL,gsn_swim,TRUE,3);
	    	else check_improve(ch,NULL,gsn_swim,FALSE,3);

            bool waveborne(false);
            if (number_percent() <= get_skill(ch, gsn_waveborne)) {check_improve(ch, NULL, gsn_waveborne, true, 8); waveborne = true;}
            else check_improve(ch, NULL, gsn_waveborne, false, 8);
		    
            if (!has_boat(ch) && !is_flying(ch) && !is_affected(ch,gsn_waterwalk) 
            && !is_affected(ch,gsn_aquamove) && !waveborne)
    		    move *= (11-get_skill(ch,gsn_swim)/10);
	    }
	}

        if ( ch->move < move && !IS_NAFFECTED(ch, AFF_FORCEDMARCH))
        {
	    send_to_char( "You are too exhausted.\n\r", ch );
            return;
        }

        if (IS_AFFECTED(ch, AFF_HIDE) && to_room->sector_type != SECT_INSIDE
	 && to_room->sector_type != SECT_CITY && to_room->sector_type != SECT_UNDERGROUND)
            unhide_char(ch);

        if (!IS_AFFECTED(ch, AFF_SNEAK))
            unhide_char(ch);

        if (is_affected(ch, gsn_wildmove)
	  && (to_room->sector_type != SECT_SWAMP)
	  && (to_room->sector_type != SECT_FOREST)
	  && (to_room->sector_type != SECT_MOUNTAIN)
 	  && (to_room->sector_type != SECT_HILLS)
	  && (to_room->sector_type != SECT_FIELD))
	{
	    send_to_char("Out of your domain, you can no longer move cautiously.\n\r", ch);
	    affect_strip(ch, gsn_wildmove);
	}

        if (is_affected(ch, gsn_shadowmastery))
	{
	    send_to_char("You move out of the concealing shadows.\n\r", ch);
	    affect_strip(ch, gsn_shadowmastery);
	    REMOVE_BIT(ch->oaffected_by, AFF_SHADOWMASTERY);
	}

	if (!is_affected(ch, gsn_wildmove))
            uncamo_char(ch);

    // Check for normal movement lag
    bool takesLag(true);
	if (IS_PAFFECTED(ch,AFF_DASH) || is_affected(ch,gsn_moonbornspeed)) 
        takesLag = false;
	
    if (is_flying(ch))
    {
        if (number_percent() <= (get_skill(ch, gsn_windrider) / 2))
        {
            check_improve(ch, NULL, gsn_windrider, true, 12);
            takesLag = false;
        }
        else
            check_improve(ch, NULL, gsn_windrider, false, 12);
    }

    // Apply normal movement lag unless cancelled by one of the effects above
    if (takesLag)
	    WAIT_STATE(ch,UMAX(ch->wait,1));

	ch->move -= move;

    // Additional movement lag
    if (encumbranceLevel >= Encumbrance::Heavy)
        WAIT_STATE(ch, 2 + encumbranceLevel - Encumbrance::Heavy);

    if (grimseep == Grimseep_Hindered)
        WAIT_STATE(ch, 3);
         
	    if (IS_NAFFECTED(ch,AFF_BOLO))
            WAIT_STATE(ch, UMAX(ch->wait,  1));

        if (is_affected(ch, gsn_kneeshatter) && !is_flying(ch))
                WAIT_STATE(ch, UMAX(ch->wait, 3));

        if (is_affected(ch, gsn_stoneshell))
            WAIT_STATE(ch, UMAX(ch->wait, 3));

        if (calcify != NULL) 
            WAIT_STATE(ch, calcify->modifier / 20);

        if (is_affected(ch, gsn_encase))
            WAIT_STATE(ch, UMAX(ch->wait, 3));

	if (is_affected(ch, gsn_plantentangle))
		WAIT_STATE(ch, UMAX(ch->wait, 2));
    }

    sentrynotify(ch,door);

    if (IS_OAFFECTED(ch, AFF_MANTLEOFFEAR) || (linked && IS_OAFFECTED(linked, AFF_MANTLEOFFEAR)))
    {
        bool sFound;

        for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
        {
   	    sFound = FALSE;
		
	    if (vch->clan == clan_lookup("SHUNNED"))
		    continue;

	    for (fch = ch->in_room->people; fch; fch = fch->next_in_room)
	        if (((fch != ch) && (fch != linked))
	         && IS_OAFFECTED(fch, AFF_MANTLEOFFEAR)
	         && !is_same_group(fch, vch))
	        {
	    	    sFound = TRUE;
		    break;
		}

	    if (!sFound)
	    {
	        paf = affect_find(ch->affected,gsn_mantleoffear);
		if (!paf)
		    paf = affect_find(linked->affected,gsn_mantleoffear);
		if (paf)
	            if (paf->modifier == 10)
	                send_to_char("The sense of uneasiness fades away.\n\r", vch);
	            else if (paf->modifier == 20)
	                send_to_char("The sense of trepidation fades away.\n\r",vch);
	            else if (paf->modifier == 25)
	                send_to_char("The sense of fear fades away.\n\r",vch);
 	    }
	}

	for (vch = to_room->people; vch; vch = vch->next_in_room)
	{
	    sFound = FALSE;

	    if (vch->clan == clan_lookup("SHUNNED"))
	        continue;

	    for (fch = to_room->people; fch; fch = fch->next_in_room)
	        if (IS_OAFFECTED(fch, AFF_MANTLEOFFEAR)
	         && !is_same_group(fch, vch))
	        {
	  	    sFound = TRUE;
		    break;
		}

	    if (!sFound)
	    {
	        paf = affect_find(ch->affected,gsn_mantleoffear);
		if (!paf)
		    paf = affect_find(linked->affected,gsn_mantleoffear);
		if (paf)
	            if (paf->modifier == 10)
	                send_to_char("A sense of uneasiness washes over you.\n\r", vch);
	            else if (paf->modifier == 20)
	                send_to_char("A sense of trepidation washes over you.\n\r",vch);
	            else if (paf->modifier == 25)
	                send_to_char("A sense of fear washes over you.\n\r",vch);
 	    }
	}
    }

    mprog_exit_trigger( ch );
    if (linked)
	mprog_exit_trigger(linked);
    for ( obj = ch->in_room->contents; obj != NULL; obj = obj->next_content )
    {
      oprog_exit_trigger( obj );
    }
    for ( fch = ch->in_room->people; fch != NULL; fch = fch_next )
    {
        fch_next = fch->next_in_room;
        for ( obj = fch->carrying; obj != NULL; obj = obj->next_content )
        {
          oprog_exit_trigger( obj );
        }
    }

    if (!IS_NPC(ch) || !IS_AFFECTED(ch, AFF_WIZI))
    {
	if (to_room->sector_type == SECT_UNDERWATER  || ch->in_room->sector_type == SECT_UNDERWATER)
	{
	    if (linked)
	    {
		sprintf(buf, "$n swims %s, riding atop $N.", dir_name[door]);
		act(buf, ch->rider ? ch->rider : ch, NULL, ch->mount ? ch->mount : ch, TO_ROOM);
	    }
	    else if ( ch->invis_level < LEVEL_HERO)
		act( "$n swims $T.", ch, NULL, dir_name[door], TO_ROOM );
	}
	else
	{
	    if (linked)
	    {
		sprintf(buf, "$n leaves %s, riding atop $N.", dir_name[door]);
		act(buf, ch->rider ? ch->rider : ch, NULL, ch->mount ? ch->mount : ch, TO_ROOM);
	    }
	    else if (IS_AFFECTED(ch, AFF_SNEAK))
	    {
            sprintf(buf, "$n sneaks %s.", dir_name[door]);
    		send_move_to_wizi(ch, buf);
	    	send_move_to_ds(ch,buf);
	    }
        else if (is_affected(ch, gsn_gaseousform) || (IS_NPC(ch) && IS_SET(ch->nact, ACT_SHADE)) || is_affected(ch, gsn_astralprojection))
        {
            sprintf(buf, "$n glides %s.", dir_name[door]);
            for (CHAR_DATA * echoChar(ch->in_room->people); echoChar != NULL; echoChar = echoChar->next_in_room)
            {
                if (can_see(echoChar, ch))
                    act(buf, ch, NULL, echoChar, TO_VICT);
            }
        }
	    else if ((is_affected(ch, gsn_wildmove) || is_affected(ch, gsn_forestwalk)) && 
	    (ch->in_room->sector_type == SECT_FOREST || ch->in_room->sector_type == SECT_SWAMP))
	    {
                sprintf(buf, "$n quietly moves %s through the wilderness.", dir_name[door]);
	        send_move_to_wizi(ch, buf);
	    }
	    else if (ch->in_room && ch->invis_level < LEVEL_HERO && (area_is_affected(ch->in_room->area, gsn_icestorm) 
	    && number_percent() < 34))
	    {
                sprintf(buf, "$n moves %s through the icestorm.", dir_name[door]);
	        send_move_to_wizi(ch, buf);
	    }
	    else if (ch->race == global_int_race_ethron 
	      && ch->in_room->sector_type != SECT_CITY 
	      && ch->in_room->sector_type != SECT_INSIDE)
	    {
                sprintf(buf, "$n uses $s racial powers to sneak %s.", dir_name[door]);
	        send_move_to_wizi(ch, buf);
	    }
	    else if (ch->invis_level > LEVEL_HERO || IS_AFFECTED(ch,AFF_WIZI))
            {
		sprintf(buf, "$n leaves %s.", dir_name[door]);
		send_move_to_wizi(ch, buf);
	    }
	    else if (!is_affected(ch,gsn_charge))
		act("$n leaves $T.", ch, NULL, dir_name[door], TO_ROOM);
	}    
    }

    if (room_is_affected(in_room, gsn_desecration))
    {
	if (ch->class_num == global_int_class_voidscholar)
	    send_to_char("The connection you sensed to the Void fades as you leave.\n\r", ch);
	else
	    send_to_char("The brooding heaviness of this place recedes as you leave.\n\r", ch);

	if (ch->rider && ch->rider->class_num == global_int_class_voidscholar)
	    send_to_char("The connection you sensed to the Void fades as you leave.\n\r", ch->rider);
	else
	    send_to_char("The brooding heaviness of this place recedes as you leave.\n\r", ch->rider);
    }

    if (!IS_NPC(ch) && !IS_IMMORTAL(ch) && ch->in_room == in_room
      && IS_SET(ch->in_room->exit[door]->exit_info, EX_TRIPWIRE)
      && !is_flying(ch) && (ch->class_num != global_int_class_watcher))
    {
        act("As $n walks, $e trips on a tight wire, stumbling.", ch, NULL, NULL, TO_ROOM);
        act("As you walk, you trip on a tight wire, stumbling.", ch, NULL, NULL, TO_CHAR);
        switch_position(ch, POS_RESTING);
        WAIT_STATE(ch, UMAX(ch->wait, 3*PULSE_VIOLENCE));
        REMOVE_BIT(ch->in_room->exit[door]->exit_info, EX_TRIPWIRE);
        if (ch->in_room->exit[door]->u1.to_room->exit[OPPOSITE(door)] &&
            ch->in_room->exit[door]->u1.to_room->exit[OPPOSITE(door)]->u1.to_room == ch->in_room)
        REMOVE_BIT(ch->in_room->exit[door]->u1.to_room->exit[OPPOSITE(door)]->exit_info, EX_TRIPWIRE);
        char_from_room( ch );
        char_to_room( ch, to_room );
        act("$n stumbles as $e arrives.", ch, NULL, NULL, TO_ROOM);
    }
    else
    {
        global_linked_move = TRUE;
        char_from_room( ch );
        char_to_room( ch, to_room );
    }
    
    // Show the character the new room
    do_look( ch, "auto" );
    if (linked)
        do_look(linked, "auto");

    // Check glyph of entombment
    check_glyphofentombment(*in_room, static_cast<Direction::Value>(door));

    if (is_affected(ch,gsn_warpath))
    {
	if (ch->mana == 0)
	{
	    send_to_char("You tire from your warpath.\n\r",ch);
	    affect_strip(ch,gsn_warpath);
	}
	else
        expend_mana(ch, 1);
    }
    sentrynotify(ch,6);
    if (!is_flying(ch) && room_is_affected(to_room, gsn_stonetomud))
    {
        if (ch->rider)
            send_to_char("You quickly find your mount sloshing into mud!\n\r", ch);
        else
            send_to_char("You quickly find yourself sloshing into mud!\n\r", ch);
    }

    if (room_is_affected(to_room, gsn_desecration))
    {
	if (ch->class_num == global_int_class_voidscholar)
	    send_to_char("You sense that the Void is somehow... closer in this place.\n\r", ch);
	else
	    send_to_char("You repress an inexplicable shiver as you enter this place.\n\r", ch);

	if (ch->rider && ch->rider->class_num == global_int_class_voidscholar)
	    send_to_char("You sense that the Void is somehow... closer in this place.\n\r", ch->rider);
	else
	    send_to_char("You repress an inexplicable shiver as you enter this place.\n\r", ch->rider);
    }

    bool maketracks = FALSE;

    if (!IS_NPC(ch) && ch->invis_level <= LEVEL_HERO 
      && !IS_AFFECTED(ch,AFF_WIZI))
	if (!(ch->in_room->sector_type == SECT_CITY 
	  || ch->in_room->sector_type == SECT_INSIDE
	  || ch->in_room->sector_type == SECT_ROAD))
	    if (!IS_SET(ch->nact,PLR_AUTOTRACKS))
	        if (number_percent() < get_skill(ch,gsn_trackless_step))
		    check_improve(ch,NULL,gsn_trackless_step,TRUE,4);
		else 
		{
		    check_improve(ch,NULL,gsn_trackless_step,FALSE,4);
		    maketracks = TRUE;
		}
	    else 
		maketracks = TRUE;
	else if (ch->in_room->sector_type == SECT_CITY
	  || ch->in_room->sector_type == SECT_INSIDE
	  || ch->in_room->sector_type == SECT_ROAD)
	    if (!IS_SET(ch->nact,PLR_AUTOTRACKS))
		if (number_percent() < get_skill(ch,gsn_covertracks))
		    check_improve(ch,NULL,gsn_covertracks,TRUE,4);
	    	else 
		{
		    check_improve(ch,NULL,gsn_covertracks,FALSE,4);
		    maketracks=TRUE;
		}
	    else 
	    {
		maketracks = TRUE;
	    }

    if (maketracks)
	add_tracks(in_room,ch,door);

    if (room_is_affected(to_room, gsn_sanctify))
    {
	if (ch->class_num == global_int_class_spiritscholar)
	    send_to_char("You feel the energies of the Weave are closer here.\n\r",ch);
	else
	    send_to_char("You sense a holy aura permeating this place.\n\r",ch);
    }
// need to add a mount check here

// brazen: Ticket #225: Changed from evil alignment to taint-based detection when someone enters the room
   int mod = aura_grade(ch);
   if (mod > 0 && !IS_AFFECTED(ch, AFF_WIZI) && ch->invis_level <= LEVEL_HERO)
   {
	    for (fch = ch->in_room->people; fch  != NULL; fch = fch_next)
	    {            
	        fch_next = fch->next_in_room;

            if (fch == ch || ch->invis_level > fch->level)
                continue;

            bool show(false);
            if (is_affected(fch, gsn_improveddetectevil)) show = true;
            else if (number_percent() <= get_skill(fch, gsn_discerniniquity))
            {
                check_improve(fch, ch, gsn_discerniniquity, TRUE, 12);
                show = true;
            }

            if (show)
            {
                if (mod < 3) send_to_char("You sense the arrival of a presence of evil.\n\r", fch);
        		else if (mod == 3) send_to_char("You sense the arrival of a force of darkness.\n\r", fch);
		        else if (mod == 4) send_to_char("You sense the arrival of a great force of darkness.\n\r", fch);
            }
        }
   }

    tch = ch->mount ? ch->mount : ch;
    if (IS_NAFFECTED(tch, AFF_LEGSHATTER) && !is_flying(tch))
    {
        act("$n winces in pain as $e walks on $s crushed leg.", tch, NULL, NULL, TO_ROOM);
        act("You wince in pain as you walk on your crushed leg.", tch, NULL, NULL, TO_CHAR);
        damage( tch, tch, number_range(2, 10), gsn_roomdeathwalk,DAM_OTHER,TRUE);
        /* roomdeathwalk's dam msg was stolen for this, since its not
         *  an affect that ever goes on anything but a room */
    }

    // tch set above legshatter, should still be effective here
    for (paf = tch->affected; paf; paf = paf->next)
	if (paf->type == gsn_qwablith)
	    spines += paf->modifier;

    if (spines > 0)
    {
	damage(tch, tch, number_range(spines * 5, spines * 10), gsn_qwablith, DAM_PIERCE, TRUE);
	if (!IS_VALID(tch))
	    return;
    }

// need to add a mount check here

    if (is_affected(ch, gsn_slice) && !IS_AFFECTED(ch, AFF_FLYING))
	if (IS_AFFECTED(ch, AFF_FLY_NATURAL))
	{
	    int mod;

	    act("$n moans in pain as $e flies in with a sliced wing.", ch, NULL, NULL, TO_ROOM);
	    act("You moan in pain as you fly with your sliced wing.", ch, NULL, NULL, TO_CHAR);
	    if ((mod = get_modifier(ch->affected, gsn_slice)) < -1)
	        damage(ch, ch, (mod * -1), gsn_slice, DAM_OTHER, FALSE);
	    WAIT_STATE(ch, UMAX(ch->wait, 2));
	}
	else
	{
	    act("$n moans in pain as $e limps into the room.", ch, NULL, NULL, TO_ROOM);
	    act("You moan in pain as you walk on your injured leg.", ch, NULL, NULL, TO_CHAR);
	    damage(ch, ch, number_range(1,3), gsn_slice, DAM_OTHER, FALSE);
	    WAIT_STATE(ch, UMAX(ch->wait, 2));
	}
   OBJ_DATA *arrow, *bow;
   while ((scouter = get_scouter(ch, ch->in_room, scouter)) != NULL)
   {
	if (can_see(scouter, ch) && IS_AWAKE(scouter))
	{
            if (ch->scoutdir < 4)
                sprintf(buf, "You scout the motion of %s %s the %s", PERS(ch, scouter), ch->scoutrooms > 3 ? "far to" : "to",dir_name[OPPOSITE(ch->scoutdir)]);
            else
                sprintf(buf, "You scout the motion of %s%s%s", PERS(ch, scouter), ch->scoutrooms > 3 ? " far " : " ",dir_name[OPPOSITE(ch->scoutdir)]);
	    if ((paf = affect_find(scouter->affected,gsn_ready)) && scouter->wait == 0)
	    {
		arrow = scouter->nocked;
		bow = get_eq_char(scouter,WEAR_HOLD);
		if (arrow && bow && bow->item_type == ITEM_BOW && paf->point 
		  && is_name((char *)paf->point,ch->name))
		{
		    if (ch->scoutrooms != 0 && paf->modifier >= ch->scoutrooms)
		    {
			send_to_char(buf,scouter);
			sprintf(buf,", and release your readied arrow!\n\r");
			send_to_char(buf,scouter);
			sprintf(buf,"shoot %s %s",(char *)paf->point,rdir_name[ch->scoutdir]);
			interpret(scouter,buf);
		    }
		    else
		    {
			send_to_char(buf,scouter);
			sprintf(buf,", but they are not yet within your range.\n\r");
			send_to_char(buf,scouter);
		    }
		}
		else
		{
		    send_to_char(buf,scouter);
		    sprintf(buf,".\n\r");
		    send_to_char(buf,scouter);
		}
	    }
	    else
	    {
		send_to_char(buf,scouter);
		sprintf(buf,".\n\r");
	    	send_to_char(buf,scouter);
	    }
	}
   }

   if (!ch || !ch->in_room)
	return;
   if (is_affected(ch, gsn_resonance))
   {
	int mod = get_modifier(ch->affected, gsn_resonance);
	int curdir;
	bool found = FALSE;

	for (fch = ch->in_room->people; fch; fch = fch->next_in_room)
	    if (!IS_NPC(fch) && (fch->id == mod))
	    {
		act("You sense the presence of $N in the room.", fch, NULL, ch, TO_CHAR);
		found = TRUE;
		break;
	    }

	if (!found)
	    for (curdir = 0; curdir < 6; curdir++)
	    {
		if (ch->in_room->exit[curdir])
		    for (fch = ch->in_room->exit[curdir]->u1.to_room->people; fch; fch = fch->next_in_room)
			if (!IS_NPC(fch) && (fch->id == mod))
			{
			    sprintf(buf, "You sense the presence of $N%s.", (curdir == 0) ? " to the south" :
    				(curdir == 1) ? " to the west" :
    				(curdir == 2) ? " to the north" :
    				(curdir == 3) ? " to the east" :
    				(curdir == 4) ? " below you" :
    				(curdir == 5) ? " above you" : "");
			    act(buf, fch, NULL, ch, TO_CHAR);
			    found = TRUE;
			    break;
			}
		if (found)
		    break;
	    }
    }	

// brazen: Ticket #225: changed improved detect evil to detect taint rather than evil alignment
    bool show(is_affected(ch, gsn_improveddetectevil));
    bool canImprove(false);
    if (number_percent() <= get_skill(ch, gsn_discerniniquity))
    {
        canImprove = true;
        show = true;
    }
    if (show)
    {
        for (fch = ch->in_room->people; fch; fch = fch->next_in_room)
        {
    	    int mod = aura_grade(fch);
	        if (mod > 0 && fch != ch && ((!IS_NPC(fch) && (fch->invis_level < tch->level)) || (IS_NPC(fch) && !IS_AFFECTED(fch, AFF_WIZI) && fch->invis_level <= LEVEL_HERO)))
	        {
                if (canImprove)
                    check_improve(ch, fch, gsn_discerniniquity, TRUE, 12);

                if (mod < 3) send_to_char("You sense the presence of evil.\n\r", ch);
	        	else if (mod == 3) send_to_char("You sense a force of darkness.\n\r", ch);
    		    else if (mod == 4) send_to_char("You sense a great force of darkness.\n\r", ch);
        		break;
	        }
	    }
    }

    if (is_affected(ch, gsn_detect_life))
	for (fch = ch->in_room->people; fch; fch = fch->next_in_room)
	    if (!can_see(ch, fch) && (fch != ch) && !IS_IMMORTAL(fch) && (!IS_NPC(fch) || !IS_AFFECTED(fch, AFF_WIZI)))
		send_to_char("You detect a hidden presence.\n\r", ch);

    for (fch = ch->in_room->people; fch; fch = fch->next_in_room)
    {
	if (fch == ch)
	    continue;

        if ((is_affected(fch, gsn_wolfform) && !can_see(fch, ch))
	  && !IS_AFFECTED(ch, AFF_WIZI)
	  || (IS_NAFFECTED(fch, AFF_LIGHTSLEEP) && (!IS_AFFECTED(ch, AFF_SNEAK)
	  && !is_affected(ch, gsn_wildmove) && (!is_affected(ch, gsn_forestwalk)
	  || ((ch->in_room->sector_type != SECT_FOREST) 
	  && (ch->in_room->sector_type != SECT_SWAMP))))))
            send_to_char("Someone has arrived.\n\r", fch);

// need to add a mount check here

	if (is_affected(fch, gsn_detect_life) && (IS_AFFECTED(ch, AFF_SNEAK) 
	  && !IS_AFFECTED(ch,AFF_WIZI)
	  || ((is_affected(ch, gsn_wildmove) || is_affected(ch, gsn_forestwalk))
	  && ((ch->in_room->sector_type == SECT_FOREST) 
	    || (ch->in_room->sector_type == SECT_SWAMP))) 
	  || ((ch->race == global_int_race_ethron) 
	    && (ch->in_room->sector_type != SECT_CITY) 
	    && (ch->in_room->sector_type != SECT_INSIDE))))
	    send_to_char("Someone has arrived.\n\r", fch);
    }

    if (ch->in_room->area != in_room->area)
        for ( d = descriptor_list; d != NULL; d = d->next )
        {
            if ( d->connected == CON_PLAYING
            &&   d->character != ch
            &&   d->character->in_room != NULL
            &&   d->character->in_room->area == ch->in_room->area
            &&   d->character->class_num == global_int_class_druid
	    &&   (get_skill(d->character, gsn_forestsense) > 0))
            {
                if (number_percent() < get_skill(d->character, gsn_forestsense)
		 && d->character->in_room->sector_type == SECT_FOREST)
                {
                    send_to_char("You sense a new presence enter the forest.\n\r", d->character);
                    check_improve(ch,NULL,gsn_forestsense,TRUE,2);
                }
            }
        }


// need to add a mount check here

    if (linked)
	act( "$n has arrived, riding atop $N.", ch->rider ? ch->rider : ch, NULL, ch->mount ? ch->mount : ch, TO_ROOM);
    else if (IS_AFFECTED(ch, AFF_SNEAK) && !IS_AFFECTED(ch,AFF_WIZI))
    {
        sprintf(buf, "$n sneaks in from the %s.", rdir_name[door]);
    	send_move_to_wizi(ch, buf);
	    send_move_to_ds(ch, buf);
    }
    else if (is_affected(ch, gsn_gaseousform) || (IS_NPC(ch) && IS_SET(ch->nact, ACT_SHADE)) || is_affected(ch, gsn_astralprojection))
    {
        sprintf(buf, "$n glides in from the %s.", rdir_name[door]);
        for (CHAR_DATA * echoChar(ch->in_room->people); echoChar != NULL; echoChar = echoChar->next_in_room)
        {
            if (can_see(echoChar, ch))
                act(buf, ch, NULL, echoChar, TO_VICT);
        }
    }
    else if ((is_affected(ch, gsn_wildmove) 
      	|| is_affected(ch, gsn_forestwalk)) 
      && (ch->in_room->sector_type == SECT_FOREST 
      	|| ch->in_room->sector_type == SECT_SWAMP))
    {
        sprintf(buf, "$n arrives from the %s through the wilderness.", rdir_name[door]);
	send_move_to_wizi(ch, buf);
    }
    else if (ch->in_room && ch->invis_level < LEVEL_HERO 
      && (area_is_affected(ch->in_room->area, gsn_icestorm) 
      && number_percent() < 34))
    {
        sprintf(buf, "$n arrives from the %s through the icestorm.", rdir_name[door]);
        send_move_to_wizi(ch, buf);
    }
    else if (ch->race == global_int_race_ethron 
      && ch->in_room->sector_type != SECT_CITY 
      && ch->in_room->sector_type != SECT_INSIDE)
    {
        sprintf(buf, "$n uses $s racial powers to sneak in from the %s.", rdir_name[door]);
        send_move_to_wizi(ch, buf);
    }
    else if (ch->invis_level > LEVEL_HERO || IS_AFFECTED(ch,AFF_WIZI))
    {
        sprintf(buf, "$n has arrived from %s.",rdir_nice_name[door]);
        send_move_to_wizi(ch, buf);
    }	    
    else if (!is_affected(ch,gsn_charge))
    {
        sprintf(buf,"$n has arrived from %s.",rdir_nice_name[door]);
	act(buf, ch, NULL, NULL, TO_ROOM );
    }
    
    // Check for glyph of ulyon
    AFFECT_DATA * glyph(get_room_affect(ch->in_room, gsn_glyphofulyon));
    if (!IS_IMMORTAL(ch) && !IS_OAFFECTED(ch, AFF_GHOST) 
    && ((IS_NPC(ch) && IS_SET(ch->act, ACT_UNDEAD)) || is_demon(ch) || number_percent() <= get_skill(ch, gsn_revenant) / 4)
    && glyph != NULL && !saves_spell(glyph->level, NULL, ch, DAM_HOLY))
    {
        // Undead or demon, and failed the save
        send_to_char("The glyph flares up, sending you reeling from the powerful energy of life and vitality!\n", ch);
        act("The glyph flares up, sending $n reeling!", ch, NULL, NULL, TO_ROOM);
        sourcelessDamage(ch, "the glyph of Ulyon", dice(glyph->level, 3), gsn_glyphofulyon, DAM_HOLY);
        WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));

        // Try to send them back whence they came
        if (ch->in_room->exit[OPPOSITE(door)] != NULL)
        {
            move_char(ch, OPPOSITE(door), FALSE);
            return;
        }
    }

    if (!IS_IMMORTAL(ch) && ch->in_room->exit[OPPOSITE(door)]
     && room_is_affected(ch->in_room, gsn_abominablerune) 
     && (IS_NPC(ch) || ((get_modifier(ch->in_room->affected, gsn_abominablerune) != ch->id && !IS_OAFFECTED(ch,AFF_GHOST)))))
    {
	rune_saved = FALSE;
	for (paf = ch->affected; paf; paf = paf->next)
	    if ((paf->type == gsn_abominablerune) &&  (paf->modifier == ch->in_room->vnum))
	    {
		rune_saved = TRUE;
		break;
	    }
	
	if (!rune_saved)
	    for (paf = ch->in_room->affected; paf; paf = paf->next)
		if (paf->type == gsn_abominablerune)
		{
		    af.where     = TO_AFFECTS;
		    af.type      = gsn_abominablerune;
		    af.location  = APPLY_HIDE;
		    af.level	 = paf->level;
		    af.bitvector = 0;
	af.point = NULL;

		    if (saves_spell(paf->level, NULL, ch, DAM_FEAR))
		    {
                af.modifier = ch->in_room->vnum;
                af.duration = paf->duration;
                affect_to_char(ch, &af);
		    }
		    else
            {
                af.duration  = number_bits(1) + 1;
                af.modifier = ch->in_room->vnum * -1;
                affect_to_char(ch, &af);
                send_to_char("You gasp in horror, unable to bear the sight of the abominable rune!\n\r", ch);
                act("$n gapes at the rune, and flees in terror!", ch, NULL, NULL, TO_ROOM);
                move_char(ch, OPPOSITE(door), FALSE);
                return;
            }
		    break;
		}
    }

    // I'm sure if I was awake I could figure out some way to combine this into the above...
    if (linked && !IS_IMMORTAL(linked) && linked->in_room->exit[OPPOSITE(door)]
     && room_is_affected(linked->in_room, gsn_abominablerune)
     && (IS_NPC(linked) || (get_modifier(linked->in_room->affected, gsn_abominablerune) != linked->id)))
    {
	rune_saved = FALSE;
	for (paf = linked->affected; paf; paf = paf->next)
	    if ((paf->type == gsn_abominablerune) &&  (paf->modifier == linked->in_room->vnum))
	    {
		rune_saved = TRUE;
		break;
	    }
	
	if (!rune_saved)
	    for (paf = linked->in_room->affected; paf; paf = paf->next)
		if (paf->type == gsn_abominablerune)
		{
		    af.where     = TO_AFFECTS;
		    af.type      = gsn_abominablerune;
		    af.location  = APPLY_HIDE;
		    af.level	 = paf->level;
		    af.bitvector = 0;
	af.point = NULL;

		    if (saves_spell(paf->level, NULL, linked, DAM_FEAR))
		    {
                af.modifier = linked->in_room->vnum;
                af.duration = paf->duration;
                affect_to_char(linked, &af);
		    }
		    else
		    {
                af.duration  = number_bits(1) + 1;
                af.modifier = linked->in_room->vnum * -1;
                affect_to_char(linked, &af);
                send_to_char("You gasp in horror, unable to bear the sight of the abominable rune!\n\r", linked);
                act("$n gapes at the rune, and flees in terror!", linked, NULL, NULL, TO_ROOM);
                move_char(linked, OPPOSITE(door), FALSE);
                return;
		    }
		    break;
		}
    }

    tch = ch->mount ? ch->mount : ch;
    if (!is_flying(tch) && number_bits(3) == 0)
    {
        for (fch = tch->in_room->people; fch; fch = fch->next_in_room)
	    if (IS_OAFFECTED(fch, AFF_INSCRIBE))
	    {
		send_to_char("Your movement disrupts the inscription upon the ground, destroying it.\n\r", tch);
		act("$n's movement disrupts the inscription upon the ground, destroying it.", tch, NULL, NULL, TO_ROOM);
		affect_strip(fch, gsn_inscribe);
	    }	
    }

    if (in_room == to_room) /* no circular follows */
        return;

    if (is_affected(ch, gsn_plantentangle) && ch->in_room && (ch->in_room->sector_type != SECT_FOREST))
    {
	send_to_char("Out of the forest, the plants cease entwining you.\n\r", ch);
	affect_strip(ch, gsn_plantentangle);
    }

    if (linked && is_affected(linked, gsn_plantentangle) && linked->in_room && (linked->in_room->sector_type != SECT_FOREST))
    {
	send_to_char("Out of the forest, the plants cease entwining you.\n\r", linked);
	affect_strip(linked, gsn_plantentangle);
    }
    if (!IS_IMMORTAL(ch) && !IS_OAFFECTED(ch,AFF_GHOST))
    {
	AFFECT_DATA *trap, *trap_next;
	ROOM_INDEX_DATA *here = ch->in_room;
	CHAR_DATA *trapmob;
	for (trap = here->affected; trap; trap = trap_next)
	{
	    trap_next = trap->next;
	    if (trap->type == gsn_daggertrap && (((CHAR_DATA *) trap->point) != ch) && trap->modifier == OPPOSITE(door))
	    {
		trapmob = create_mobile(get_mob_index(MOB_VNUM_TRAP));
		char_to_room(trapmob, ch->in_room);
		send_to_char("You feel a slight tug against your skin, and feel a wire snap.\n\r",ch);
		global_bool_check_avoid = FALSE;
		damage_new(trapmob,ch,number_range(ch->level,ch->level*2),TYPE_HIT,DAM_PIERCE,TRUE,"barrage of daggers");
		global_bool_check_avoid = TRUE;
		extract_char(trapmob, TRUE);
	        affect_remove_room(here,trap);
	    }
	}
    }
    // tch should still be pointing to a mount right here...
    if (tch->in_room && !IS_IMMORTAL(tch) && !is_flying(tch)) 
/* check for traps */
    {
	AFFECT_DATA *trap, *trap_next;
	CHAR_DATA *trapmob;

	for (trap = tch->in_room->affected; trap; trap = trap_next)
	{
	    trap_next = trap->next;
	    if ((trap->type == gsn_trap) && (((CHAR_DATA *) trap->point) != tch))
	    {
		switch (trap->modifier)
		{
		    case TRAP_DAMAGE:
			send_to_char("You hear a quiet click as you stumble over a tripwire.\n\r", tch);
			act("You hear a quiet click as $n stumbles over a tripwire.", tch, NULL, NULL, TO_ROOM);
			trapmob = create_mobile(get_mob_index(MOB_VNUM_TRAP));
			char_to_room(trapmob, tch->in_room);
			damage_old(trapmob, tch, number_fuzzy(ch->level * 3), gsn_trap, DAM_PIERCE, TRUE);
			extract_char(trapmob, TRUE);
			if (!IS_VALID(tch) || IS_OAFFECTED(tch, AFF_GHOST))
			    return;	
		    break;

		    case TRAP_GREASE:
		    {
			int grease;

			if (number_percent() < get_curr_stat(tch, STAT_WIS))
			    send_to_char("As you're about to step down, you notice a thin layer of grease covering the ground, and carefully avoid it.\n\r", ch);
			else
			{
			    grease = number_range(0,3);

			    switch (grease)
			    {
				case 0:
				    send_to_char("You stumble, but manage to keep your footing as you step onto a concealed coating of grease!\n\r", ch);
				    act("$n stumbles but keeps $s footing, as $e steps onto a concealed coating of grease!", ch, NULL, NULL, TO_ROOM);
				    WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));
				    break;

				case 1:
				    send_to_char("You go sprawling as you step onto a thin layer of grease!\n\r", ch);
				    act("$n goes sprawling as $e steps onto a thin layer of grease!", ch, NULL, NULL, TO_ROOM);
				    WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE * 2));
				    break;

				case 2:
				    send_to_char("You go sprawling as you step onto a thin layer of grease!\n\r", ch);
				    send_to_char("Your clothing becomes covered in the slick substance as you attempt to recover.\n\r", ch);
				    act("$n goes sprawling as $e steps onto a thin layer of grease!", ch, NULL, NULL, TO_ROOM);
				    act("$n's clothing becomes covered in the slick substance as $e tries to recover.", ch, NULL, NULL, TO_ROOM);
				    WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE * 2));
				    break;

				case 3:
				    send_to_char("You go sprawling as you step onto a thin layer of grease!\n\r", ch);
				    send_to_char("Your hands and clothing becomes covered in the slick substance as you attempt to recover.\n\r", ch);
				    act("$n goes sprawling as $e steps onto a thin layer of grease!", ch, NULL, NULL, TO_ROOM);
				    act("$n's hands and clothing becomes covered in the slick substance as $e tries to recover.", ch, NULL, NULL, TO_ROOM);
				    WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE * 2));
				    break;
			    }

			    af.where	= TO_AFFECTS;
			    af.type	= gsn_grease;
			    af.level	= ch->level;
			    af.modifier = grease;
			    af.location = 0;
			    af.duration = grease + number_range(1,3);
			    af.point = NULL;
			    af.bitvector = 0;
			    affect_to_char(ch, &af);
				
		 	}
		    }
		}
		affect_remove_room(tch->in_room, trap);
	    }
	}
    }

// need to add a mount check here

   if (!IS_IMMORTAL(ch) && room_is_affected(ch->in_room, gsn_mushroomcircle)
    && (!IS_NPC(ch) || (!IS_SET(ch->act, ACT_NOSUBDUE) && !IS_AFFECTED(ch, AFF_WIZI)))
    && (ch->invis_level == 0) && (ch->class_num != global_int_class_druid)
    && !IS_OAFFECTED(ch, AFF_GHOST))
   {
	send_to_char("A light puff of spores from a nearby mushroom circle surround you.\n\r", ch);
	act("A light puff of spores from the mushroom circle surround $n.", ch, NULL, NULL, TO_ROOM);

	af.where	 = TO_AFFECTS;
	af.level	 = ch->level;
	af.point = NULL;

	/* Yes, this spell just steals effects from three other spells */
	/* and abilities.  Why?  Because the effects are the same.     */
	/* Deal with it.	- Erinos                               */

	switch (get_modifier(ch->in_room->affected, gsn_mushroomcircle))
	{
	    case 0:		/* Sleep */
	        if (!saves_spell(ch->level + lethebane_sleep_level_mod(NULL, ch), NULL, ch, DAM_OTHER))
	        {
	    	    af.type	 = gsn_sleep;
	    	    af.duration  = UMAX(1, ch->level / 8);
	    	    af.location  = APPLY_NONE;
	    	    af.modifier  = 0;
	    	    af.bitvector = AFF_SLEEP;
	            affect_join(ch, &af);
	            send_to_char("You feel very sleepy ..... zzzzzz.\n\r", ch);
	            act("$n goes to sleep.\n\r", ch, NULL, NULL, TO_ROOM);
		    switch_position(ch, POS_SLEEPING);
		    sleeping = TRUE;
		}
	        break;
	     case 1:		/* Hallucinatory */
		if (!saves_spell(ch->level, NULL, ch, DAM_OTHER))
		{
		    af.type	 = gsn_delusionpoison;
		    af.duration  = 3;
		    af.location  = APPLY_HIDE;
		    af.modifier  = 0;
		    af.bitvector = AFF_DELUSION;
		    affect_join(ch, &af);
		    send_to_char("You begin to grow very nervous.\n\r", ch);
		    act("$n twitches a moment, and begins to look around suspiciously.", ch, NULL, NULL, TO_ROOM);
		}
		break;
	    case 2:		/* Slow */
	        spell_slow(gsn_slow, ch->level, ch, ch, TAR_CHAR_OFFENSIVE);
	        break;
	    default:
	        bug("Mushroom circle gone bad!  Modifier %d.", get_modifier(ch->in_room->affected, gsn_mushroomcircle));
	        break;
        }
    }		

// need to add a mount check here

    if (follow)
    {
        for ( fch = in_room->people; fch != NULL; fch = fch_next )
        {
            fch_next = fch->next_in_room;

	    if (!IS_NPC(fch) && IS_SET(fch->act, PLR_FREEZE))
	        continue;

            if (fch->master == ch && IS_AFFECTED(fch,AFF_CHARM)
            &&   fch->position < POS_STANDING)
                do_stand(fch,"");

            // Check for following, including gravebeat
            bool gravebeating(false);
            AFFECT_DATA * gravebeat(get_affect(fch, gsn_gravebeat));
            if (gravebeat != NULL && gravebeat->modifier == ch->id)
                gravebeating = true;

            if ((gravebeating || fch->master == ch) 
            && fch->position == POS_STANDING
            &&   can_see_room(fch,to_room) && fch->in_room
            &&   fch->in_room != ch->in_room)
            {
                if (!gravebeating && IS_SET(ch->in_room->room_flags,ROOM_LAW)
                &&  (IS_NPC(fch) && (IS_SET(fch->act,ACT_AGGRESSIVE) || IS_SET(fch->nact, ACT_PSYCHO))
	            &&   !IS_SET(fch->act, ACT_ILLUSION)))
                {
                    act("You can't bring $N into the city.", ch,NULL,fch,TO_CHAR);
                    act("You aren't allowed in the city.", fch,NULL,NULL,TO_CHAR);
                    continue;
                }

                if (!IS_AFFECTED(fch, AFF_SNEAK))
                    unhide_char(fch);

                act( "You follow $N.", fch, NULL, ch, TO_CHAR );
                move_char( fch, door, TRUE );
            } 
        }
    }

    /* put here in order to prevent the greet_prog related aggro bug */
    if (!sleeping)
        stop_fighting_all(ch);

    mprog_entry_trigger( ch );
    if (linked)
	mprog_entry_trigger( linked );
    mprog_greet_trigger( ch );
    if (linked)
	mprog_greet_trigger( linked );

    if (ch->in_room)
        rprog_greet_trigger( ch->in_room, ch );

    if (linked && linked->in_room)
	rprog_greet_trigger( linked->in_room, linked );

    if (ch->in_room != NULL)
        for ( obj = ch->in_room->contents; obj != NULL; obj = obj->next_content )
        {
            oprog_entry_trigger( obj );
            oprog_greet_trigger( obj, ch );
        }

    if (ch->in_room != NULL)
        for ( fch = ch->in_room->people; fch != NULL; fch = fch_next )
        {
            fch_next = fch->next_in_room;
            if (fch != ch)
                for ( obj = fch->carrying; obj != NULL; obj = obj->next_content )
                    oprog_greet_trigger( obj, ch );
            else
                for ( obj = fch->carrying; obj != NULL; obj = obj->next_content )
                    oprog_entry_trigger( obj );
        }

    if (linked && linked->in_room != NULL)
        for ( fch = linked->in_room->people; fch != NULL; fch = fch_next )
        {
            fch_next = fch->next_in_room;
            if (fch != linked)
                for ( obj = fch->carrying; obj != NULL; obj = obj->next_content )
                    oprog_greet_trigger( obj, linked );
            else
                for ( obj = fch->carrying; obj != NULL; obj = obj->next_content )
                    oprog_entry_trigger( obj );
        }

    tch = ch->mount ? ch->mount : ch;
    if (!is_flying(tch)
     && room_is_affected(tch->in_room, gsn_setsnare)
     && !IS_PAFFECTED(tch, AFF_VOIDWALK)
     && !IS_IMMORTAL(tch)
     && !IS_AFFECTED(tch, AFF_WIZI)
     && !is_affected(tch, gsn_astralprojection))
    {
        if (tch->class_num == global_int_class_ranger)
        {
            act("You carefully avoid a waiting snare.", tch, NULL, NULL, TO_CHAR);
            act("$n carefully avoids the snare.", tch, NULL, NULL, TO_ROOM);
        }
        else if (get_modifier(tch->in_room->affected, gsn_setsnare) == 1)
        {
	    if (IS_NAFFECTED(tch, AFF_WARINESS) && (number_percent() < (get_skill(tch, gsn_wariness)/2)))
	    {
		act("You warily avoid a waiting snare.\n\r", tch, NULL, NULL, TO_CHAR);
		act("$n warily avoids a waiting snare.\n\r", tch, NULL, NULL, TO_ROOM);
	    }
	    else
	    {
                act("You step into a well-placed snare, and are caught!", tch, NULL, NULL, TO_CHAR);
                act("$n steps into a well-placed snare, and is caught!", tch, NULL, NULL, TO_ROOM);
                af.where      = TO_AFFECTS;
                af.type       = gsn_setsnare;
                af.level      = tch->level;
                af.duration   = number_range(1,3);
                af.location   = APPLY_DEX;
                af.modifier   = -3;
                af.bitvector  = 0;
                room_affect_strip(tch->in_room, gsn_setsnare);
                affect_to_char(tch, &af);
                WAIT_STATE(tch, UMAX(tch->wait, PULSE_VIOLENCE*2));
	    }
        }
        else
        {
            act("You step into a hidden snare, but it fails to go off!", tch, NULL, NULL, TO_CHAR);
            act("$n steps into a hidden snare, but it fails to go off!", tch, NULL, NULL, TO_ROOM);
            room_affect_strip(tch->in_room, gsn_setsnare);
        }
    }

    if (room_is_affected(ch->in_room, gsn_etherealblaze) && (IS_AFFECTED(ch, AFF_HIDE) || is_affected(ch, gsn_camouflage)))
    {
	send_to_char("The ethereal blaze forces you out of the shadows.\n\r", ch);
	unhide_char(ch);
	uncamo_char(ch);
    }

    if (linked && room_is_affected(linked->in_room, gsn_etherealblaze) && (IS_AFFECTED(linked, AFF_HIDE) || is_affected(linked, gsn_camouflage)))
    {
	send_to_char("The ethereal blaze forces you out of the shadows.\n\r", linked);
	unhide_char(linked);
	uncamo_char(linked);
    }

    // Check for harvest of souls
    if (check_harvestofsouls(*ch))
        return;

    if (!is_flying(tch)
     && room_is_affected(tch->in_room, gsn_caltraps)
     && tch->class_num != global_int_class_assassin
     && (abs(tch->level - get_modifier(tch->in_room->affected, gsn_caltraps)) < 9))
    {
        act("$n winces in agony as $e missteps into the caltraps!", tch, NULL, NULL, TO_ROOM);
        act("You wince in agony as you misstep into caltraps!", tch, NULL, NULL, TO_CHAR);
        af.where        = TO_AFFECTS;
        af.type         = gsn_caltraps;
        af.level        = tch->level;
        af.duration     = 5;
        af.location     = APPLY_DEX;
        af.modifier     = -3;
        af.bitvector    = 0;
        affect_to_char(tch, &af);
        WAIT_STATE(tch, UMAX(tch->wait, skill_table[gsn_caltraps].beats));
        damage( tch, tch, number_range(10,20), gsn_caltraps, DAM_PIERCE, TRUE );
    }

    tch = ch->rider ? ch->rider : ch;
    if (is_affected(tch, gsn_sense_danger))
    {
	int i;
	bool aFound;

	for (i = 0; i < 6; i++)
	{
	    aFound = FALSE;
	    if (tch->in_room->exit[i])
		for (fch = tch->in_room->exit[i]->u1.to_room->people; fch; fch = fch->next_in_room)
		    if (IS_NPC(fch) && (IS_SET(fch->act, ACT_AGGRESSIVE) || IS_SET(fch->nact, ACT_PSYCHO)))
		    {
			aFound = TRUE;
			break;
		    } 

	    if (aFound)
	    {
		sprintf(buf, "You sense danger %s.\n\r",
			(i == 0) ? "to the north" :
			(i == 1) ? "to the east" :
			(i == 2) ? "to the south" :
			(i == 3) ? "to the west" :
			(i == 4) ? "above you" :
			(i == 5) ? "below you" : "somewhere");
		send_to_char(buf, tch);
	    }
	}
    }
	
    // tch should be rider here
    if (ch != NULL && ch->in_room != NULL)
        for (fch = ch->in_room->people; fch  != NULL; fch = fch->next_in_room)
        {
            if (IS_NAFFECTED(fch, AFF_GUARDING) && is_affected(fch, gsn_guard)
             && IS_AWAKE(fch)
             && get_modifier(fch->affected, gsn_guard) == OPPOSITE(door)
             && !IS_NPC(ch) && IS_PK(ch, fch) && can_see(fch, ch)
             && !is_safe(fch, ch)
// brazen: guard shouldn't attack groupmates
 	     && !is_same_group(fch,ch))
            {
                sprintf(buf, "Help! %s is attacking me!", PERS(fch, ch));
                do_autoyell(ch, buf);
                act("$N leaps at $n, attacking $m as $e enters.", ch, NULL, fch, TO_NOTVICT);
                act("$N leaps at you, attacking you as you enter.", ch, NULL, fch, TO_CHAR);
                act("You leap at $n, attacking $m as $e enters.", ch, NULL, fch, TO_VICT);
                multi_hit(fch, ch, TYPE_UNDEFINED);
            }

        }

    // Update gravebeat
    check_gravebeat_room(*ch);
}

void add_tracks (ROOM_INDEX_DATA *room, CHAR_DATA *ch, int door)
{
    TRACK_DATA *nTrack=NULL;
    TRACK_DATA *pCheck;

    if (room->tracks == NULL)
    {
        bug ("Fuxin EMPTY ROOM TRACKS", 0);
        return;
    }

    if (IS_IMMORTAL(ch))
        return; 

    for ( pCheck = room->tracks ; pCheck->ch != NULL; pCheck = nTrack )
    {
        if (pCheck->ch == ch)
        {
            pCheck->direction = door;
            pCheck->time = 0;
            pCheck->valid = TRUE;
            return;
        }
        else
        {
            if (pCheck->next != NULL)
                nTrack = pCheck->next;
        }
    }

    /* New character in the room */
    nTrack = (TRACK_DATA*)alloc_perm( sizeof (*nTrack));
    g_num_tracks++;

    nTrack->ch = NULL;
    nTrack->next = NULL;
    nTrack->direction = 0;

    pCheck->ch = ch;
    pCheck->direction = door;
    pCheck->next = nTrack;
    pCheck->time = 0;
    pCheck->valid = TRUE;
}

void do_north( CHAR_DATA *ch, char *argument )
{
    move_char( ch, DIR_NORTH, TRUE );
}

void do_east( CHAR_DATA *ch, char *argument )
{
    move_char( ch, DIR_EAST, TRUE );
}

void do_south( CHAR_DATA *ch, char *argument )
{
    move_char( ch, DIR_SOUTH, TRUE );
}

void do_west( CHAR_DATA *ch, char *argument )
{
    move_char( ch, DIR_WEST, TRUE );
}

void do_up( CHAR_DATA *ch, char *argument )
{
    move_char( ch, DIR_UP, TRUE );
}

void do_down( CHAR_DATA *ch, char *argument )
{
    move_char( ch, DIR_DOWN, TRUE );
}

bool has_boat( CHAR_DATA *ch )
{
    OBJ_DATA *obj;
    bool found = FALSE;

    if (IS_IMMORTAL(ch))
        found = TRUE;

    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
        if ( obj->item_type == ITEM_BOAT )
        {
            found = TRUE;
            break;
        }

    return found;
}

int find_door( CHAR_DATA *ch, char *arg )
{
    EXIT_DATA *pexit;
    int door;

         if ( !str_cmp( arg, "n" ) || !str_cmp( arg, "north" ) ) door = 0;
    else if ( !str_cmp( arg, "e" ) || !str_cmp( arg, "east"  ) ) door = 1;
    else if ( !str_cmp( arg, "s" ) || !str_cmp( arg, "south" ) ) door = 2;
    else if ( !str_cmp( arg, "w" ) || !str_cmp( arg, "west"  ) ) door = 3;
    else if ( !str_cmp( arg, "u" ) || !str_cmp( arg, "up"    ) ) door = 4;
    else if ( !str_cmp( arg, "d" ) || !str_cmp( arg, "down"  ) ) door = 5;
    else
    {
        for ( door = 0; door <= 5; door++ )
        {
            if ( ( pexit = ch->in_room->exit[door] ) != NULL
            &&   IS_SET(pexit->exit_info, EX_ISDOOR)
            &&   pexit->keyword != NULL
            &&   is_name( arg, pexit->keyword ) )
                return door;
        }
        act( "I see no $T here.", ch, NULL, arg, TO_CHAR );
        return -1;
    }

    if ( ( pexit = ch->in_room->exit[door] ) == NULL )
    {
        act( "I see no door $T here.", ch, NULL, arg, TO_CHAR );
        return -1;
    }

    if ( !IS_SET(pexit->exit_info, EX_ISDOOR) )
    {
        send_to_char( "You can't do that.\n\r", ch );
        return -1;
    }

    return door;
}

void do_rush(CHAR_DATA *ch, char *argument)
{
    int chance = 0;
    int dirone = 0, dirtwo = 0;
    char arg[MAX_STRING_LENGTH];
    
    if (!ch->in_room)
	return;

    if (ch->fighting)
    {
	send_to_char("You're too busy fighting!\n\r", ch);
	return;
    }
    
    if ((chance = get_skill(ch,gsn_rush)) == 0)
    {
	send_to_char("You aren't swift enough to run through there without being noticed.\n\r",ch);
	return;
    }

    argument = one_argument(argument,arg);

    if ( !str_prefix(arg, "north" ) ) dirone = 0;
    else if (!str_prefix(arg, "east") ) dirone = 1;
    else if (!str_prefix(arg, "south") ) dirone = 2;
    else if (!str_prefix(arg, "west") ) dirone = 3;
    else if (!str_prefix(arg, "up") ) dirone = 4;
    else if (!str_prefix(arg, "down") ) dirone = 5;
    else
    {
	send_to_char("You can't rush in that direction!\n\r",ch);
	return;
    }

    if ( !str_prefix(argument, "north" ) ) dirtwo = 0;
    else if (!str_prefix(argument, "east") ) dirtwo = 1;
    else if (!str_prefix(argument, "south") ) dirtwo = 2;
    else if (!str_prefix(argument, "west") ) dirtwo = 3;
    else if (!str_prefix(argument, "up") ) dirtwo = 4;
    else if (!str_prefix(argument, "down") ) dirtwo = 5;
    else
    {
	send_to_char("You can't rush in that direction!\n\r",ch);
	return;
    }

    unhide_char(ch);
    uncamo_char(ch);
    affect_strip(ch,gsn_sneak);
    REMOVE_BIT(ch->affected_by,AFF_SNEAK);

    WAIT_STATE(ch, UMAX(ch->wait, 5+number_range(1,7)));
    ROOM_INDEX_DATA *proom = ch->in_room;
    move_char(ch,dirone,FALSE);
    if (ch)
	if ((ch->in_room && ch->in_room == proom) 
	  || IS_OAFFECTED(ch,AFF_GHOST))
            send_to_char("Your rush comes to a premature end.\n\r",ch);
	else
	{
	    if (number_percent() > chance)
	    {
		send_to_char("You stumble as you rush in, and try to regain your footing.\n\r",ch);
		check_improve(ch,NULL,gsn_rush,FALSE,1);
		WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
	    }
	    else
	    {
		CHAR_DATA *vch;
		for (vch = ch->in_room->people;vch;vch=vch->next_in_room)
		    if (vch->fighting == ch)
			break;
		if (vch)
		    send_to_char("Your rush comes to a premature end.\n\r",ch);
		else
		{
		    check_improve(ch,NULL,gsn_rush,TRUE,1);
	    	    proom = ch->in_room;
	    	    move_char(ch,dirtwo,FALSE);
	    	    if (ch)
		    	if ((ch->in_room && ch->in_room == proom)
		          || IS_OAFFECTED(ch,AFF_GHOST))
		    	    send_to_char("Your rush comes to a premature end.\n\r",ch);
		        else
			    if (number_percent() > chance)
			    {
			    	send_to_char("You stumble as you rush in, and try to regain your footing.\n\r",ch);
			    	check_improve(ch,NULL,gsn_rush,FALSE,1);
			    	WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
			    }
			    else
			    	check_improve(ch,NULL,gsn_rush,TRUE,1);
		}
	    }
	}
}

void do_retreat(CHAR_DATA *ch, char *argument)
{
    int chance = 0;
    int direction=0;

    if (!ch->in_room)
	return;

    if (!ch->fighting)
    {
	send_to_char("But you're not fighting anyone!\n\r", ch);
	return;
    }

    if (is_affected(ch, gsn_zeal)
     || is_affected(ch, gsn_frenzy)
     || IS_AFFECTED(ch, AFF_BERSERK)
     || is_affected(ch, gsn_rage)
     || is_affected(ch, gsn_adrenaline_rush)
     || is_affected(ch, gsn_consuming_rage)
     || is_affected(ch, gsn_savagery)
     || IS_OAFFECTED(ch, AFF_BURNOUT))
    {
	send_to_char("You cannot break your focus to retreat.\n\r", ch);
	return;
    } 
    
    if (IS_NAFFECTED(ch, AFF_ASHURMADNESS))
    {
	send_to_char("You must eat!\n\r",ch);
	return;
    }

    if ( !str_prefix(argument, "north" ) ) direction = 0;
    else if (!str_prefix(argument, "east") ) direction = 1;
    else if (!str_prefix(argument, "south") ) direction = 2;
    else if (!str_prefix(argument, "west") ) direction = 3;
    else if (!str_prefix(argument, "up") ) direction = 4;
    else if (!str_prefix(argument, "down") ) direction = 5;
    else
    {
	send_to_char("You can't retreat in that direction!\n\r",ch);
	return;
    }
    
    if (ch->in_room->exit[direction] == NULL
      || ch->in_room->exit[direction]->u1.to_room == NULL)
    {
	send_to_char("You can't retreat in that direction!\n\r",ch);
	return;
    }

    WAIT_STATE(ch, UMAX(ch->wait, 5+number_range(1,7)));
    
    chance = get_skill(ch, gsn_retreat) * 3 / 4;

    if (number_percent() > chance)
    {
	send_to_char("You attempt to step out of the battle, but are caught within the melee.\n\r", ch);
	check_improve(ch, ch->fighting, gsn_retreat, FALSE, 1);
	return;
    }

    send_to_char("You retreat, removing yourself from the fight.\n\r", ch);
    act("$n retreats, removing $mself from the fight.",ch,NULL,NULL,TO_ROOM);

    check_improve(ch, ch->fighting, gsn_retreat, TRUE, 1);

    stop_fighting_all(ch);
    move_char(ch,direction,FALSE);
}


void do_vanish(CHAR_DATA *ch, char *argument)
{
    ROOM_INDEX_DATA *pRoom=NULL;
    int chance;
    int x=0;

    if ((chance = get_skill(ch,gsn_vanish)) == 0)
    {
        send_to_char("Huh?\n\r",ch);
        return;
    }

    if (ch->mana < skill_table[gsn_vanish].min_mana)
    {
        send_to_char("You're too tired to disappear that way.\n\r", ch);
        WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE/2));
        return;
    }

    if (ch->mount)
    {
	send_to_char("You cannot vanish while mounted.\n\r", ch);
	return;
    }

    if (ch->in_room->room_flags & ROOM_NO_RECALL)
    {
        send_to_char("You failed.\n\r", ch);
        WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE/2));
        return;
    }

    if (number_percent() > chance || number_bits(3) == 0)
    {
        send_to_char("You throw down a globe, but the smoke drifts away.\n\r", ch);
        expend_mana(ch, skill_table[gsn_vanish].min_mana);
        WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE/2));
        act("$n throws down a globe, but the smoke just drifts away.", ch, NULL, NULL, TO_ROOM);
        check_improve(ch,NULL,gsn_vanish,FALSE,1);
        return;
    }

    expend_mana(ch, skill_table[gsn_vanish].min_mana);
    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_vanish].beats));

    while (x < 100)
    {
        pRoom = get_random_room(ch);

        x++;

        if (ch->in_room->area == pRoom->area)
                break;
    }


    if (x > 98 || pRoom==NULL)
    {
        send_to_char("You failed.\n\r", ch);
        return;
    }

    send_to_char("You throw down a globe, vanishing in the confusion!\n\r", ch);
    act("$n throws down a globe.", ch, NULL, NULL, TO_ROOM);
    act("$n vanishes!", ch, NULL, NULL, TO_ROOM);
    global_linked_move = TRUE;
    char_from_room(ch);
    char_to_room(ch, pRoom);
    check_improve(ch,NULL,gsn_vanish,TRUE,1);
    do_look(ch, "auto");
}



void do_open( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Open what?\n\r", ch );
        return;
    }

    if (is_affected(ch, gsn_astralprojection))
    {
        send_to_char("You can't do that from the astral plane.\n\r", ch);
        return;
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
    	if (!IS_NPC(ch) && (ch->perm_stat[STAT_CON] < 4) && IS_OAFFECTED(ch, AFF_GHOST))
    	{
	    act("Your ghostly hands pass through $p.", ch, obj, NULL, TO_CHAR);
	    return;
	}

        /* open portal */
        if (obj->item_type == ITEM_PORTAL)
        {
            if (!IS_SET(obj->value[1], EX_ISDOOR))
            {
                send_to_char("You can't do that.\n\r",ch);
                return;
            }

            if (!IS_SET(obj->value[1], EX_CLOSED))
            {
                send_to_char("It's already open.\n\r",ch);
                return;
            }

            if (IS_SET(obj->value[1], EX_LOCKED))
            {
                send_to_char("It's locked.\n\r",ch);
                return;
            }
            REMOVE_BIT(obj->value[1], EX_CLOSED);
            act("You open $p.",ch,obj,NULL,TO_CHAR);
            act("$n opens $p.",ch,obj,NULL,TO_ROOM);
            return;
        }

        /* 'open object' */
        if ( obj->item_type != ITEM_CONTAINER)
            { send_to_char( "That's not a container.\n\r", ch ); return; }
        if ( !IS_SET(obj->value[1], CONT_CLOSED) )
            { send_to_char( "It's already open.\n\r",      ch ); return; }
        if ( !IS_SET(obj->value[1], CONT_CLOSEABLE) )
            { send_to_char( "You can't do that.\n\r",      ch ); return; }
        if ( IS_SET(obj->value[1], CONT_LOCKED) )
            { send_to_char( "It's locked.\n\r",            ch ); return; }

        REMOVE_BIT(obj->value[1], CONT_CLOSED);
        act("You open $p.",ch,obj,NULL,TO_CHAR);
        act( "$n opens $p.", ch, obj, NULL, TO_ROOM );

        if (obj_is_affected(obj, gsn_runeoffire))
        {
            act("With a blast of flame, a rune of fire on $p explodes at $n!", ch, obj, NULL, TO_ROOM);
            act("With a blast of flame, a rune of fire on $p explodes at you!", ch, obj, NULL, TO_CHAR);
            damage(ch, ch, number_range(ch->level, ch->level*4), gsn_runeoffire, DAM_FIRE, true);
            object_affect_strip(obj, gsn_runeoffire);
        }

        return;
    }

    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
        /* 'open door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

    	if (!IS_NPC(ch) && (ch->perm_stat[STAT_CON] < 4) && IS_OAFFECTED(ch, AFF_GHOST))
    	{
	        send_to_char("Your ghostly hands pass through the door.\n\r", ch);
    	    return;
	    }

        if (!checkDoorFireRune(ch, door))
        {
            pexit = ch->in_room->exit[door];
            if ( !IS_SET(pexit->exit_info, EX_CLOSED) )
                { send_to_char( "It's already open.\n\r",      ch ); return; }
            if ( IS_SET(pexit->exit_info, EX_LOCKED) || IS_SET(pexit->exit_info, EX_RUNEOFEARTH))
                { send_to_char( "It's locked.\n\r",            ch ); return; }

            REMOVE_BIT(pexit->exit_info, EX_CLOSED);
            act("$n opens the $d.", ch, NULL, pexit->keyword, TO_ROOM );
	        act("You open the $d.", ch, NULL, pexit->keyword, TO_CHAR );
    
            /* open the other side */
            if ( ( to_room   = pexit->u1.to_room            ) != NULL
            &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
            &&   pexit_rev->u1.to_room == ch->in_room )
            {
                CHAR_DATA *rch;
                REMOVE_BIT( pexit_rev->exit_info, EX_CLOSED );
                for ( rch = to_room->people; rch != NULL; rch = rch->next_in_room )
                    act( "The $d opens.", rch, NULL, pexit_rev->keyword, TO_CHAR );
            }
        }
    }
}

void do_close( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        send_to_char( "Close what?\n\r", ch );
        return;
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
        /* portal stuff */
        if (obj->item_type == ITEM_PORTAL)
        {

            if (!IS_SET(obj->value[1],EX_ISDOOR)
            ||   IS_SET(obj->value[1],EX_NOCLOSE))
            {
                send_to_char("You can't do that.\n\r",ch);
                return;
            }

            if (IS_SET(obj->value[1],EX_CLOSED))
            {
                send_to_char("It's already closed.\n\r",ch);
                return;
            }

            SET_BIT(obj->value[1],EX_CLOSED);
            act("You close $p.",ch,obj,NULL,TO_CHAR);
            act("$n closes $p.",ch,obj,NULL,TO_ROOM);
            return;
        }

        /* 'close object' */
        if ( obj->item_type != ITEM_CONTAINER )
            { send_to_char( "That's not a container.\n\r", ch ); return; }
        if ( IS_SET(obj->value[1], CONT_CLOSED) )
            { send_to_char( "It's already closed.\n\r",    ch ); return; }
        if ( !IS_SET(obj->value[1], CONT_CLOSEABLE) )
            { send_to_char( "You can't do that.\n\r",      ch ); return; }

        SET_BIT(obj->value[1], CONT_CLOSED);
        act("You close $p.",ch,obj,NULL,TO_CHAR);
        act( "$n closes $p.", ch, obj, NULL, TO_ROOM );
        return;
    }

    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
        /* 'close door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

        pexit   = ch->in_room->exit[door];
        if ( IS_SET(pexit->exit_info, EX_CLOSED) )
        {
	    send_to_char( "It's already closed.\n\r",    ch );
	    return;
	}

	if (IS_SET(pexit->exit_info, EX_NOCLOSE))
	{
	    send_to_char("You can't do that.\n\r", ch);
	    return;
	}

        SET_BIT(pexit->exit_info, EX_CLOSED);
        act( "$n closes the $d.", ch, NULL, pexit->keyword, TO_ROOM );
	act( "You close the $d.", ch, NULL, pexit->keyword, TO_CHAR );

        /* close the other side */
        if ( ( to_room   = pexit->u1.to_room            ) != NULL
        &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != 0
        &&   pexit_rev->u1.to_room == ch->in_room )
        {
            CHAR_DATA *rch;

            SET_BIT( pexit_rev->exit_info, EX_CLOSED );
            for ( rch = to_room->people; rch != NULL; rch = rch->next_in_room )
                act( "The $d closes.", rch, NULL, pexit_rev->keyword, TO_CHAR );        }
    }

    return;
}



bool has_key( CHAR_DATA *ch, int key )
{
    OBJ_DATA *obj;

    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
        if ( obj->pIndexData->vnum == key )
            return TRUE;
    }
    return FALSE;
}



void do_lock( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Lock what?\n\r", ch );
        return;
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
        /* portal stuff */
        if (obj->item_type == ITEM_PORTAL)
        {
            if (!IS_SET(obj->value[1],EX_ISDOOR)
            ||  IS_SET(obj->value[1],EX_NOCLOSE))
            {
                send_to_char("You can't do that.\n\r",ch);
                return;
            }
            if (!IS_SET(obj->value[1],EX_CLOSED))
            {
                send_to_char("It's not closed.\n\r",ch);
                return;
            }

            if (obj->value[4] < 0 || IS_SET(obj->value[1],EX_NOLOCK))
            {
                send_to_char("It can't be locked.\n\r",ch);
                return;
            }

            if (!has_key(ch,obj->value[4]))
            {
                send_to_char("You lack the key.\n\r",ch);
                return;
            }

            if (IS_SET(obj->value[1],EX_LOCKED))
            {
                send_to_char("It's already locked.\n\r",ch);
                return;
            }

            SET_BIT(obj->value[1],EX_LOCKED);
            act("You lock $p.",ch,obj,NULL,TO_CHAR);
            act("$n locks $p.",ch,obj,NULL,TO_ROOM);
            return;
        }

        /* 'lock object' */
        if ( obj->item_type != ITEM_CONTAINER )
            { send_to_char( "That's not a container.\n\r", ch ); return; }
        if ( !IS_SET(obj->value[1], CONT_CLOSED) )
            { send_to_char( "It's not closed.\n\r",        ch ); return; }
        if ( obj->value[2] < 0 )
            { send_to_char( "It can't be locked.\n\r",     ch ); return; }
        if ( !has_key( ch, obj->value[2] ) )
            { send_to_char( "You lack the key.\n\r",       ch ); return; }
        if ( IS_SET(obj->value[1], CONT_LOCKED) )
            { send_to_char( "It's already locked.\n\r",    ch ); return; }

        SET_BIT(obj->value[1], CONT_LOCKED);
        act("You lock $p.",ch,obj,NULL,TO_CHAR);
        act( "$n locks $p.", ch, obj, NULL, TO_ROOM );
        return;
    }

    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
        /* 'lock door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

        pexit   = ch->in_room->exit[door];
        if ( !IS_SET(pexit->exit_info, EX_CLOSED) )
            { send_to_char( "It's not closed.\n\r",        ch ); return; }
        if ( pexit->key < 0 )
            { send_to_char( "It can't be locked.\n\r",     ch ); return; }
        if ( !has_key( ch, pexit->key) )
            { send_to_char( "You lack the key.\n\r",       ch ); return; }
        if ( IS_SET(pexit->exit_info, EX_LOCKED) )
            { send_to_char( "It's already locked.\n\r",    ch ); return; }

        SET_BIT(pexit->exit_info, EX_LOCKED);
        act( "*Click*  You lock the $d.", ch, NULL, pexit->keyword, TO_CHAR);
        act( "$n locks the $d.", ch, NULL, pexit->keyword, TO_ROOM );

        /* lock the other side */
        if ( ( to_room   = pexit->u1.to_room            ) != NULL
        &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != 0
        &&   pexit_rev->u1.to_room == ch->in_room )
        {
            SET_BIT( pexit_rev->exit_info, EX_LOCKED );
        }
    }

    return;
}



void do_unlock( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Unlock what?\n\r", ch );
        return;
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
        /* portal stuff */
        if (obj->item_type == ITEM_PORTAL)
        {
            if (!IS_SET(obj->value[1],EX_ISDOOR))
            {
                send_to_char("You can't do that.\n\r",ch);
                return;
            }

            if (!IS_SET(obj->value[1],EX_CLOSED))
            {
                send_to_char("It's not closed.\n\r",ch);
                return;
            }

            if (obj->value[4] < 0)
            {
                send_to_char("It can't be unlocked.\n\r",ch);
                return;
            }

            if (!has_key(ch,obj->value[4]))
            {
                send_to_char("You lack the key.\n\r",ch);
                return;
            }

            if (!IS_SET(obj->value[1],EX_LOCKED))
            {
                send_to_char("It's already unlocked.\n\r",ch);
                return;
            }

            REMOVE_BIT(obj->value[1],EX_LOCKED);
            act("You unlock $p.",ch,obj,NULL,TO_CHAR);
            act("$n unlocks $p.",ch,obj,NULL,TO_ROOM);
            return;
        }

        /* 'unlock object' */
        if ( obj->item_type != ITEM_CONTAINER )
            { send_to_char( "That's not a container.\n\r", ch ); return; }
        if ( !IS_SET(obj->value[1], CONT_CLOSED) )
            { send_to_char( "It's not closed.\n\r",        ch ); return; }
        if ( obj->value[2] < 0 )
            { send_to_char( "It can't be unlocked.\n\r",   ch ); return; }
        if ( !has_key( ch, obj->value[2] ) )
            { send_to_char( "You lack the key.\n\r",       ch ); return; }
        if ( !IS_SET(obj->value[1], CONT_LOCKED) )
            { send_to_char( "It's already unlocked.\n\r",  ch ); return; }

        REMOVE_BIT(obj->value[1], CONT_LOCKED);
        act("You unlock $p.",ch,obj,NULL,TO_CHAR);
        act( "$n unlocks $p.", ch, obj, NULL, TO_ROOM );
        return;
    }

    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
        /* 'unlock door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

        pexit = ch->in_room->exit[door];
        if ( IS_SET(pexit->exit_info, EX_RUNEOFEARTH) )
        {
          act("You approach the $d to unlock it, but the rune of earth flares up, driving you back!", ch, NULL, pexit->keyword, TO_CHAR);
          act("$n approaches the $d to unlock it, but the rune of earth flares up, driving $m back!", ch, NULL, pexit->keyword, TO_ROOM);
          return;
        }
        if ( !IS_SET(pexit->exit_info, EX_CLOSED) )
            { send_to_char( "It's not closed.\n\r",        ch ); return; }
        if ( pexit->key < 0 )
            { send_to_char( "It can't be unlocked.\n\r",   ch ); return; }
        if ( !has_key( ch, pexit->key) )
            { send_to_char( "You lack the key.\n\r",       ch ); return; }
        if ( !IS_SET(pexit->exit_info, EX_LOCKED) )
            { send_to_char( "It's already unlocked.\n\r",  ch ); return; }

        REMOVE_BIT(pexit->exit_info, EX_LOCKED);
        act( "*Click*  You unlock the $d.", ch, NULL, pexit->keyword, TO_CHAR );
        act( "$n unlocks the $d.", ch, NULL, pexit->keyword, TO_ROOM );

        /* unlock the other side */
        if ( ( to_room   = pexit->u1.to_room            ) != NULL
        &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
        &&   pexit_rev->u1.to_room == ch->in_room )
        {
            REMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
        }
    }

    return;
}


void do_morph( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;


        return;

    if ( is_affected( ch, gsn_sneak ) )
    {
       send_to_char("You are too weak to morph for a while.\n\r",ch);
       return;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
       send_to_char("Morph into what?\n\r", ch);
       return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
        { send_to_char( "They aren't here.\n\r",              ch ); return; }

    if (victim->level > ch->level)
    {
       send_to_char("You struggle to attain their form but fail.\n\r", ch);
       return;
    }

    af.where     = TO_AFFECTS;
    af.type      = gsn_morph;
    af.level     = ch->level;
    af.duration  = 24;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char( ch, &af );

    free_string( ch->long_descr );
    ch->long_descr = str_dup(victim->long_descr);

    act( "$n shifts and contorts into a new form!", ch, NULL, NULL, TO_ROOM );
    send_to_char( "Your body shifts and contorts into the new form!\n\r", ch );

    return;
}



void do_ram( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA *to_room;
    EXIT_DATA *pexit;
    EXIT_DATA *pexit_rev;
    char arg[MAX_INPUT_LENGTH];
    int door, chance;

    if ((chance = get_skill(ch, gsn_ram)) == 0)
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Ram what?\n\r", ch );
        return;
    }


    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
        /* 'pick door' */
        pexit = ch->in_room->exit[door];
        if (!checkDoorFireRune(ch, door))
        {
            if ( !IS_SET(pexit->exit_info, EX_CLOSED))
                { send_to_char( "It's not closed.\n\r",        ch ); return; }
            if ( pexit->key < 0 && !IS_IMMORTAL(ch))
                { send_to_char( "It looks too solid for that.\n\r",     ch ); return; }
            if ( IS_SET(pexit->exit_info, EX_PICKPROOF) && !IS_IMMORTAL(ch))
                { send_to_char( "It looks too solid for that.\n\r",             ch ); return; }
            if ( IS_SET(pexit->exit_info, EX_RUNEOFEARTH) )
            {
                act("$n makes as if to ram the door, but the rune of earth flares up, driving $m back.", ch, NULL, NULL, TO_ROOM);
                act("You make as if to ram the door, but the rune of earth flares up, driving you back.", ch, NULL, NULL, TO_CHAR);
                return;
            }

            WAIT_STATE( ch, skill_table[gsn_ram].beats );
            if ( !IS_NPC(ch) && number_percent( ) > get_skill(ch,gsn_ram))
            {
                act("$n lowers $s shoulder and smashes into the $d, and crumples as $e fails to ram it down.", ch, NULL, pexit->keyword, TO_ROOM);
                act("You lower your shoulder and smash into the $d, and crumple as you fail to ram it down.", ch, NULL, pexit->keyword, TO_CHAR);
                damage( ch, ch, number_range(ch->level, ch->level*3), gsn_ram,DAM_OTHER,TRUE);
                check_improve(ch,NULL,gsn_ram,FALSE,2);
                return;
            }
        }

        if (IS_SET(pexit->exit_info, EX_LOCKED))
                REMOVE_BIT(pexit->exit_info, EX_LOCKED);
        if (IS_SET(pexit->exit_info, EX_CLOSED))
                REMOVE_BIT(pexit->exit_info, EX_CLOSED);
        act("$n lowers $s shoulder and smashes into the $d, and it flies open, leaving $m to stumble through!", ch, NULL, pexit->keyword, TO_ROOM);
        act("You lower your shoulder and smash into the $d, and it flies open, leaving you to stumble through.", ch, NULL, pexit->keyword, TO_CHAR);
        damage( ch, ch, number_range(ch->level/2, (ch->level*3)/2), gsn_ram,DAM_OTHER,TRUE);
        check_improve(ch,NULL,gsn_ram,TRUE,2);

        /* pick the other side */
        if ( ( to_room   = pexit->u1.to_room            ) != NULL
        &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
        &&   pexit_rev->u1.to_room == ch->in_room )
        {
        if (IS_SET(pexit_rev->exit_info, EX_LOCKED))
                REMOVE_BIT(pexit_rev->exit_info, EX_LOCKED);
        if (IS_SET(pexit_rev->exit_info, EX_CLOSED))
                REMOVE_BIT(pexit_rev->exit_info, EX_CLOSED);
        move_char(ch, door, FALSE);
        act("The $d crashes open as $n smashes through it!", ch, NULL, pexit_rev->keyword, TO_ROOM);
        }
        else
          move_char(ch, door, FALSE);
    }
}

void do_setsnare( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int chance, dir;
    OBJ_DATA *obj;
    char buf[MAX_STRING_LENGTH];
    if ((chance = get_skill(ch, gsn_setsnare)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("From which direction did you want to catch people with your trap?",ch);
	return;
    }

    dir = Direction::ValueFor(argument);
    if (dir == Direction::Max)
    {
        send_to_char("That's not a valid direction.\n", ch);
        return;
    }

    if (ch->mana < skill_table[gsn_setsnare].min_mana)
    {
        send_to_char("You can't concentrate enough to properly set a snare.\n\r", ch);
        WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_setsnare].beats));
        return;
    }

    if (is_affected(ch, gsn_setsnare))
    {
        send_to_char("You don't feel ready to set another snare yet.\n\r", ch);
        expend_mana(ch, skill_table[gsn_setsnare].min_mana);
        WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_setsnare].beats));
        return;
    }

    if (ch->in_room->sector_type != SECT_FOREST 
      && ch->in_room->sector_type != SECT_MOUNTAIN
      && ch->in_room->sector_type != SECT_HILLS
      && ch->in_room->sector_type != SECT_WATER_SWIM
      && ch->in_room->sector_type != SECT_WATER_NOSWIM)
    {
        send_to_char("You must be in a forest, mountains, hills, or on water to properly set a snare.\n\r", ch);
        return;
    }

    if (room_is_affected(ch->in_room, gsn_setsnare))
    {
        send_to_char("There is already a ready snare here.\n\r", ch);
        return;
    }
    obj = get_eq_char(ch,ITEM_HOLD);
    if (obj)
        switch (ch->in_room->sector_type)
        {
	    case SECT_FOREST:
	    {
	        if (obj->item_type == ITEM_NET)
		{
		    sprintf(buf,"You string $p between two trees to the %s.",dir_name[dir]);
		    act(buf,ch,obj,NULL,TO_CHAR);
		    break;
		}
	    }
        }
    act("$n carefully sets up a snare for the next person that walks in.", ch, NULL, NULL, TO_ROOM);
    act("You carefully set up a snare for the next person that walks in.", ch, NULL, NULL, TO_CHAR);

    af.where     = TO_AFFECTS;
    af.type      = gsn_setsnare;
    af.level     = ch->level;
    af.duration  = ch->level/10;
    af.location  = 0;
    af.modifier  = 2;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    /* set up room affect */
    af.where    = TO_ROOM_AFF;

    if (number_percent() > chance)
    {
        af.modifier = 0;
        check_improve(ch,NULL, gsn_setsnare, FALSE, 1);
        expend_mana(ch, skill_table[gsn_setsnare].min_mana);
        WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_setsnare].beats));
        affect_to_room(ch->in_room, &af);
    }
    else
    {
        af.modifier = 1;
        check_improve(ch,NULL, gsn_setsnare, TRUE, 1);
        expend_mana(ch, skill_table[gsn_setsnare].min_mana);
        WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_setsnare].beats));
        affect_to_room(ch->in_room, &af);
    }
}


void do_pick( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Pick what?\n\r", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_pick_lock].beats );

    if (IS_AFFECTED(ch, AFF_CHARM) || is_affected(ch, gsn_commandword))
    return;

    if ( !IS_NPC(ch) && number_percent( ) > get_skill(ch,gsn_pick_lock))
    {
        send_to_char( "You failed.\n\r", ch);
        check_improve(ch,NULL,gsn_pick_lock,FALSE,2);
        return;
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
        /* portal stuff */
        if (obj->item_type == ITEM_PORTAL)
        {
            if (!IS_SET(obj->value[1],EX_ISDOOR))
            {
                send_to_char("You can't do that.\n\r",ch);
                return;
            }

            if (!IS_SET(obj->value[1],EX_CLOSED))
            {
                send_to_char("It's not closed.\n\r",ch);
                return;
            }

            if (obj->value[4] < 0)
            {
                send_to_char("It can't be unlocked.\n\r",ch);
                return;
            }

            if (IS_SET(obj->value[1],EX_PICKPROOF))
            {
                send_to_char("You failed.\n\r",ch);
                return;
            }

            REMOVE_BIT(obj->value[1],EX_LOCKED);
            act("You pick the lock on $p.",ch,obj,NULL,TO_CHAR);
            act("$n picks the lock on $p.",ch,obj,NULL,TO_ROOM);
            check_improve(ch,NULL,gsn_pick_lock,TRUE,2);
            return;
        }





        /* 'pick object' */
        if ( obj->item_type != ITEM_CONTAINER )
            { send_to_char( "That's not a container.\n\r", ch ); return; }
        if ( !IS_SET(obj->value[1], CONT_CLOSED) )
            { send_to_char( "It's not closed.\n\r",        ch ); return; }
        if ( obj->value[2] < 0 )
            { send_to_char( "It can't be unlocked.\n\r",   ch ); return; }
        if ( !IS_SET(obj->value[1], CONT_LOCKED) )
            { send_to_char( "It's already unlocked.\n\r",  ch ); return; }
        if ( IS_SET(obj->value[1], CONT_PICKPROOF) )
            { send_to_char( "You failed.\n\r",             ch ); return; }

        REMOVE_BIT(obj->value[1], CONT_LOCKED);
        act("You pick the lock on $p.",ch,obj,NULL,TO_CHAR);
        act("$n picks the lock on $p.",ch,obj,NULL,TO_ROOM);
        check_improve(ch,NULL,gsn_pick_lock,TRUE,2);
        return;
    }

    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
        /* 'pick door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

        pexit = ch->in_room->exit[door];
        if ( !IS_SET(pexit->exit_info, EX_CLOSED) && !IS_IMMORTAL(ch))
            { send_to_char( "It's not closed.\n\r",        ch ); return; }
        if ( pexit->key < 0 && !IS_IMMORTAL(ch))
            { send_to_char( "It can't be picked.\n\r",     ch ); return; }
        if ( !IS_SET(pexit->exit_info, EX_LOCKED) )
            { send_to_char( "It's already unlocked.\n\r",  ch ); return; }
        if ( IS_SET(pexit->exit_info, EX_PICKPROOF) && !IS_IMMORTAL(ch))
            { send_to_char( "You failed.\n\r",             ch ); return; }
        if ( IS_SET(pexit->exit_info, EX_RUNEOFEARTH) )
            {
                act("$n tries to pick the door, but the rune of earth flares up, driving $m back.", ch, NULL, NULL, TO_ROOM);
                act("You try to pick the door, but the rune of earth flares up, driving you back.", ch, NULL, NULL, TO_CHAR);
                return;
            }

        REMOVE_BIT(pexit->exit_info, EX_LOCKED);
        act( "*Click*  You pick the $d.", ch, NULL, pexit->keyword, TO_CHAR );
        act( "$n picks the $d.", ch, NULL, pexit->keyword, TO_ROOM );
        check_improve(ch,NULL,gsn_pick_lock,TRUE,2);

        /* pick the other side */
        if ( ( to_room   = pexit->u1.to_room            ) != NULL
        &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
        &&   pexit_rev->u1.to_room == ch->in_room )
        {
            REMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
        }
    }

    return;
}

static bool check_handle_forced_sleep_bit(CHAR_DATA * ch, CHAR_DATA * waker)
{
    // If not affected by the sleep bit or anesthetize, no problem
    if (!IS_AFFECTED(ch, AFF_SLEEP) && !is_loc_not_affected(ch, gsn_anesthetize, APPLY_NONE))
        return true;

    // Check for lethebane on the waker
    if (waker != NULL)
    {
        int skill(get_skill(waker, gsn_lethebane));
        if (number_percent() <= skill)
        {
            send_to_char("A rush of clarity fills your mind, clearing away the haze of dreams.\n", ch);
            act("You send a pulse of shining white energy to clear away the haze of $N's dreams.", waker, NULL, ch, TO_CHAR);
            check_improve(waker, NULL, gsn_lethebane, true, 1);
            return true;
        }
        else if (skill > 0)
        {
            act("You try to clear away the haze of $N's dreams, but $E is too deeply entrenched in them!", waker, NULL, ch, TO_CHAR);
            check_improve(waker, NULL, gsn_lethebane, false, 1);
            WAIT_STATE(waker, UMAX(waker->wait, PULSE_VIOLENCE / 2));
        }
    }

    // Check for for lucid dreaming
    int skill = get_skill(ch, gsn_lucid);
    if (skill > 0)
    {
        // Check whether there an effect which blocks lucid dreaming from firing
        bool blocked(false);
        for (AFFECT_DATA * paf(get_affect(ch, gsn_lucid)); paf != NULL; paf = get_affect(ch, gsn_lucid, paf))
        {
            if (paf->where == TO_AFFECTS && paf->bitvector == AFF_SLEEP)
            {
                blocked = true;
                break;
            }
        }

        if (!blocked)
        {
            // Not blocked, check for lucid dreaming
            if (number_percent() <= skill)
            {
                switch_position(ch, POS_STANDING);
                send_to_char("You awake from an unusually vivid dream.\n\r", ch);
                check_improve(ch, NULL, gsn_lucid, TRUE, 1);
                return true;
            }

            check_improve(ch, NULL, gsn_lucid, FALSE, 1);
        }
    }
 
    // Failed to break past the sleep bit
    send_to_char("You struggle to awaken, but cannot escape the depths of your dreams.\n", ch);
    if (waker == NULL || waker == ch)
        WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE * 2));
    return false;
}

static bool check_handle_forced_sleep(CHAR_DATA * ch, CHAR_DATA * waker)
{
    // If not sleeping, no problem
    if (ch->position != POS_SLEEPING)
        return true;

    // Check for the sleep bit
    if (!check_handle_forced_sleep_bit(ch, waker))
        return false;

    // Check for density
    if (is_affected(ch, gsn_density) && number_percent() < (15 * ch->size - SIZE_SMALL))
    {
        send_to_char("You try to rise, but fall back to the ground!\n\r", ch);
        return false;
    }

    return true;
}


static void do_stand_helper( CHAR_DATA *ch, char *argument, CHAR_DATA * waker)
{
    OBJ_DATA *obj = NULL;

    if (argument[0] != '\0')
    {
	    if (!str_cmp(argument, "none") && (ch->position == POS_STANDING) && ch->on)
    	{
            if (IS_SET(ch->on->value[2],STAND_IN))
            {
                act("You step out of $p.",ch,ch->on,NULL,TO_CHAR);
                act("$n steps out of $p.",ch,ch->on,NULL,TO_ROOM);
            }
            else if (IS_SET(ch->on->value[2],STAND_ON))
            {
                act("You step off of $p.",ch,ch->on,NULL,TO_CHAR);
                act("$n steps off of $p.",ch,ch->on,NULL,TO_ROOM);
            }
            else
            {
                act("You step away from $p.",ch,ch->on,NULL,TO_CHAR);
                act("$n steps away from $p.",ch,ch->on,NULL,TO_ROOM);
            }

	        ch->on = NULL;
    	    return;
	    }

        if (ch->position == POS_FIGHTING)
        {
            send_to_char("Maybe you should finish fighting first?\n\r",ch);
            return;
        }

// brazen: first, check inventory       
        obj = get_obj_list(ch, argument, ch->carrying);
	if (!obj)
// next, check ground
	    obj = get_obj_list(ch,argument,ch->in_room->contents);

	if (obj == NULL)
        {
            send_to_char("You don't see that here.\n\r",ch);
            return;
        }

        if (obj->pIndexData->vnum == OBJ_VNUM_DISC && obj->item_type == ITEM_CONTAINER)
        {
            if (!try_convert_floating_disc_to_furniture(*ch, *obj))
                return;
        }

        if (obj->item_type != ITEM_FURNITURE
        ||  (!IS_SET(obj->value[2],STAND_AT)
        &&   !IS_SET(obj->value[2],STAND_ON)
        &&   !IS_SET(obj->value[2],STAND_IN)))
        {
            send_to_char("You can't seem to find a place to stand.\n\r",ch);
            return;
        }

        if (ch->on != obj && count_users(obj) >= obj->value[0])
        {
            act("There's no room left to stand.", ch, NULL, NULL, TO_CHAR);
            return;
        }

        ch->on = obj;
    }

    switch (ch->position)
    {
        case POS_SLEEPING:
            if (!check_handle_forced_sleep(ch, waker))
                return;
	    
	        switch_position(ch, POS_STANDING);
    	    if (obj)
	        	ch->on = obj;

            if (obj == NULL)
            {
	        	send_to_char( "You wake and stand up.\n\r", ch );
                act( "$n wakes and stands up.", ch, NULL, NULL, TO_ROOM );
                ch->on = NULL;
            }
            else if (IS_SET(obj->value[2],STAND_AT))
            {
		        act_new("You wake and stand at $p.",ch,obj,NULL,TO_CHAR,POS_DEAD);
                act("$n wakes and stands at $p.",ch,obj,NULL,TO_ROOM);
            }
            else if (IS_SET(obj->value[2],STAND_ON))
            {
                act_new("You wake and stand on $p.",ch,obj,NULL,TO_CHAR,POS_DEAD);
                act("$n wakes and stands on $p.",ch,obj,NULL,TO_ROOM);
            }
            else
            {
                act_new("You wake and stand in $p.",ch,obj,NULL,TO_CHAR,POS_DEAD);
                act("$n wakes and stands in $p.",ch,obj,NULL,TO_ROOM);
            }
            break;

        case POS_RESTING: case POS_SITTING:
            switch_position(ch, POS_STANDING);
	        if (obj) 
                ch->on = obj;

            if (obj == NULL)
            {
                send_to_char( "You stand up.\n\r", ch );
                act( "$n stands up.", ch, NULL, NULL, TO_ROOM );
                ch->on = NULL;
            }
            else if (IS_SET(obj->value[2],STAND_AT))
            {
                act("You stand at $p.",ch,obj,NULL,TO_CHAR);
                act("$n stands at $p.",ch,obj,NULL,TO_ROOM);
            }
            else if (IS_SET(obj->value[2],STAND_ON))
            {
                act("You stand on $p.",ch,obj,NULL,TO_CHAR);
                act("$n stands on $p.",ch,obj,NULL,TO_ROOM);
            }
            else
            {
                act("You stand in $p.",ch,obj,NULL,TO_CHAR);
                act("$n stands in $p.",ch,obj,NULL,TO_ROOM);
            }
            break;

        case POS_STANDING:
	    if (obj == NULL)
	        send_to_char( "You are already standing.\n\r", ch );
            else if (IS_SET(obj->value[2],STAND_AT))
            {
                act("You stand at $p.",ch,obj,NULL,TO_CHAR);
                act("$n stands at $p.",ch,obj,NULL,TO_ROOM);
            }
            else if (IS_SET(obj->value[2],STAND_ON))
            {
                act("You stand on $p.",ch,obj,NULL,TO_CHAR);
                act("$n stands on $p.",ch,obj,NULL,TO_ROOM);
            }
            else
            {
                act("You stand in $p.",ch,obj,NULL,TO_CHAR);
                act("$n stands in $p.",ch,obj,NULL,TO_ROOM);
            }
            break;

        case POS_FIGHTING:
            send_to_char( "You are already fighting!\n\r", ch );
            break;
    }
}

void do_stand(CHAR_DATA * ch, char * argument)
{
    do_stand_helper(ch, argument, NULL);
}

void do_encamp( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int chance;

    if ((chance = get_skill(ch, gsn_encamp)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (!ch->in_room 
      || (ch->in_room->sector_type != SECT_FOREST 
	&& ch->in_room->sector_type != SECT_FIELD 
	&& ch->in_room->sector_type != SECT_HILLS
	&& ch->in_room->sector_type != SECT_MOUNTAIN
	&& ch->in_room->sector_type != SECT_DESERT
	&& ch->in_room->sector_type != SECT_SWAMP))
    {
	send_to_char("You must be in the wilds to set up an encampment.\n\r", ch);
	return;
    }

    if (room_is_affected(ch->in_room, gsn_encamp))
    {
	send_to_char("There is already a camp set up here.\n\r", ch);
	return;
    }

    if (ch->mana < skill_table[gsn_encamp].min_mana)
    {
	send_to_char("You are too exhausted to properly set up an encampment.\n\r", ch);
	return;
    }
  
    expend_mana(ch, skill_table[gsn_encamp].min_mana);
    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_encamp].beats));   

    if (number_percent() > chance)
    {
	send_to_char("You fail to find a good place to set up camp.\n\r", ch);
	check_improve(ch,ch,gsn_encamp,FALSE,1);
	return;
    }

    af.where	 = TO_OAFFECTS;
    af.type	 = gsn_encamp;
    af.level	 = ch->level;
    af.duration  = 48;
    af.location  = 0;
    af.modifier  = ch->id;
    af.bitvector = AFF_ENCAMP;
    affect_to_room(ch->in_room, &af);

    act("You find a suitable location and set up camp.", ch, NULL, NULL, TO_CHAR);
    act("$n finds a suitable location and sets up camp.", ch, NULL, NULL, TO_ROOM);
    check_improve(ch,NULL,gsn_encamp,TRUE,1);
    return;
}


void do_rest( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj = NULL;

    if (ch->in_room == NULL)
        return;

    if (ch->position == POS_FIGHTING)
    {
	send_to_char("You are already fighting!\n\r",ch);
	return;
    }

    /* okay, now that we know we can rest, find an object to rest on */
    if (argument[0] != '\0')
    {
// brazen: first, check inventory
	obj = get_obj_list(ch, argument, ch->carrying);
	if(!obj)
// then, check ground
	    obj = get_obj_list(ch,argument,ch->in_room->contents);
	if (obj == NULL)
	{
	    send_to_char("You don't see that here.\n\r",ch);
	    return;
	}
    }
    else obj = ch->on;

    if (obj != NULL)
    {
        if (obj->pIndexData->vnum == OBJ_VNUM_DISC && obj->item_type == ITEM_CONTAINER)
        {
            if (!try_convert_floating_disc_to_furniture(*ch, *obj))
                return;
        }

        if (obj->item_type != ITEM_FURNITURE
    	||  (!IS_SET(obj->value[2],REST_ON)
    	&&   !IS_SET(obj->value[2],REST_IN)
    	&&   !IS_SET(obj->value[2],REST_AT)))
    	{
	    send_to_char("You can't rest on that.\n\r",ch);
	    return;
    	}

        if (obj != NULL && ch->on != obj && count_users(obj) >= obj->value[0])
        {
	    act_new("There's no more room on $p.",ch,obj,NULL,TO_CHAR,POS_DEAD);
	    return;
    	}
    }

    switch ( ch->position )
    {
        case POS_SLEEPING:        
            if (!check_handle_forced_sleep(ch))
                return;

        if (obj == NULL)
        {
            send_to_char( "You wake up and start resting.\n\r", ch );
            act ("$n wakes up and starts resting.",ch,NULL,NULL,TO_ROOM);
        }
        else if (IS_SET(obj->value[2],REST_AT))
        {
            act_new("You wake up and rest at $p.",
                ch,obj,NULL,TO_CHAR,POS_SLEEPING);
            act("$n wakes up and rests at $p.",ch,obj,NULL,TO_ROOM);
        }
        else if (IS_SET(obj->value[2],REST_ON))
        {
            act_new("You wake up and rest on $p.",
                    ch,obj,NULL,TO_CHAR,POS_SLEEPING);
            act("$n wakes up and rests on $p.",ch,obj,NULL,TO_ROOM);
        }
        else
        {
            act_new("You wake up and rest in $p.",
                    ch,obj,NULL,TO_CHAR,POS_SLEEPING);
            act("$n wakes up and rests in $p.",ch,obj,NULL,TO_ROOM);
        }
	switch_position(ch, POS_RESTING);
	break;

    case POS_RESTING:
	send_to_char( "You are already resting.\n\r", ch );
	return;
	break;

    case POS_STANDING:
	if (obj == NULL)
	{
	    send_to_char( "You rest.\n\r", ch );
	    act( "$n sits down and rests.", ch, NULL, NULL, TO_ROOM );
	}
        else if (IS_SET(obj->value[2],REST_AT))
        {
	    act("You sit down at $p and rest.",ch,obj,NULL,TO_CHAR);
	    act("$n sits down at $p and rests.",ch,obj,NULL,TO_ROOM);
        }
        else if (IS_SET(obj->value[2],REST_ON))
        {
	    act("You sit on $p and rest.",ch,obj,NULL,TO_CHAR);
	    act("$n sits on $p and rests.",ch,obj,NULL,TO_ROOM);
        }
        else
        {
	    act("You rest in $p.",ch,obj,NULL,TO_CHAR);
	    act("$n rests in $p.",ch,obj,NULL,TO_ROOM);
        }
	switch_position(ch, POS_RESTING);
	break;

    case POS_SITTING:
	if (obj == NULL)
	{
	    send_to_char("You rest.\n\r",ch);
	    act("$n rests.",ch,NULL,NULL,TO_ROOM);
	}
        else if (IS_SET(obj->value[2],REST_AT))
        {
	    act("You rest at $p.",ch,obj,NULL,TO_CHAR);
	    act("$n rests at $p.",ch,obj,NULL,TO_ROOM);
        }
        else if (IS_SET(obj->value[2],REST_ON))
        {
	    act("You rest on $p.",ch,obj,NULL,TO_CHAR);
	    act("$n rests on $p.",ch,obj,NULL,TO_ROOM);
        }
        else
        {
	    act("You rest in $p.",ch,obj,NULL,TO_CHAR);
	    act("$n rests in $p.",ch,obj,NULL,TO_ROOM);
	}
	switch_position(ch, POS_RESTING);
	break;
    }

    ch->on = obj;

    if (IS_OAFFECTED(ch, AFF_NIGHTFEARS))
	send_to_char("You are haunted by visions torn from your nightmares. True rest seems unlikely at best.\n\r", ch);

    return;
}


void do_sit (CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj = NULL;

    if (ch->position == POS_FIGHTING)
    {
	send_to_char("Maybe you should finish this fight first?\n\r",ch);
	return;
    }

    /* okay, now that we know we can sit, find an object to sit on */
    if (argument[0] != '\0')
    {
// brazen: first, check inventory
	obj = get_obj_list(ch, argument, ch->carrying);
	if (!obj)
// check ground
	    obj = get_obj_list(ch,argument,ch->in_room->contents);

	if (obj == NULL)
	{
	    send_to_char("You don't see that here.\n\r",ch);
	    return;
	}
    }
    else obj = ch->on;

    if (obj != NULL)                                                              
    {
        if (obj->pIndexData->vnum == OBJ_VNUM_DISC && obj->item_type == ITEM_CONTAINER)
        {
            if (!try_convert_floating_disc_to_furniture(*ch, *obj))
                return;
        }

        if (obj->item_type != ITEM_FURNITURE
        ||  (!IS_SET(obj->value[2],SIT_ON)
        &&   !IS_SET(obj->value[2],SIT_IN)
        &&   !IS_SET(obj->value[2],SIT_AT)))
        {
            send_to_char("You can't sit on that.\n\r",ch);
            return;
        }

        if (obj != NULL && ch->on != obj && count_users(obj) >= obj->value[0])
        {
            act_new("There's no more room on $p.",ch,obj,NULL,TO_CHAR,POS_DEAD);
            return;
        }
    }

    switch (ch->position)
    {
    	case POS_SLEEPING:
            if (!check_handle_forced_sleep(ch))
                return;

            if (obj == NULL)
            {
            	send_to_char( "You wake and sit up.\n\r", ch );
            	act( "$n wakes and sits up.", ch, NULL, NULL, TO_ROOM );
            }
            else if (IS_SET(obj->value[2],SIT_AT))
            {
            	act_new("You wake and sit at $p.",ch,obj,NULL,TO_CHAR,POS_DEAD);
            	act("$n wakes and sits at $p.",ch,obj,NULL,TO_ROOM);
            }
            else if (IS_SET(obj->value[2],SIT_ON))
            {
            	act_new("You wake and sit on $p.",ch,obj,NULL,TO_CHAR,POS_DEAD);
            	act("$n wakes and sits at $p.",ch,obj,NULL,TO_ROOM);
            }
            else
            {
            	act_new("You wake and sit in $p.",ch,obj,NULL,TO_CHAR,POS_DEAD);
            	act("$n wakes and sits in $p.",ch,obj,NULL,TO_ROOM);
            }

	    switch_position(ch, POS_SITTING);
	    break;
	case POS_RESTING:
	    if (obj == NULL)
		send_to_char("You stop resting.\n\r",ch);
	    else if (IS_SET(obj->value[2],SIT_AT))
	    {
		act("You sit at $p.",ch,obj,NULL,TO_CHAR);
		act("$n sits at $p.",ch,obj,NULL,TO_ROOM);
	    }

	    else if (IS_SET(obj->value[2],SIT_ON))
	    {
		act("You sit on $p.",ch,obj,NULL,TO_CHAR);
		act("$n sits on $p.",ch,obj,NULL,TO_ROOM);
	    }
	    switch_position(ch, POS_SITTING);
	    break;
	case POS_SITTING:
	    send_to_char("You are already sitting down.\n\r",ch);
	    return;
	    break;
	case POS_STANDING:
	    if (obj == NULL)
    	    {
		send_to_char("You sit down.\n\r",ch);
    	        act("$n sits down on the ground.",ch,NULL,NULL,TO_ROOM);
	    }
	    else if (IS_SET(obj->value[2],SIT_AT))
	    {
		act("You sit down at $p.",ch,obj,NULL,TO_CHAR);
		act("$n sits down at $p.",ch,obj,NULL,TO_ROOM);
	    }
	    else if (IS_SET(obj->value[2],SIT_ON))
	    {
		act("You sit on $p.",ch,obj,NULL,TO_CHAR);
		act("$n sits on $p.",ch,obj,NULL,TO_ROOM);
	    }
	    else
	    {
		act("You sit down in $p.",ch,obj,NULL,TO_CHAR);
		act("$n sits down in $p.",ch,obj,NULL,TO_ROOM);
	    }
    	    switch_position(ch, POS_SITTING);
    	    break;
    }

    ch->on = obj;
}

void do_lightsleep( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
af.point = NULL;
    OBJ_DATA *obj = NULL;
    int chance;

    if ((chance = get_skill(ch,gsn_lightsleep)) == 0)
    {
        send_to_char("Huh?\n\r",ch);
        return;
    }

    if (chance < number_percent())
    {
	send_to_char("You try to sleep lightly, but fail to get any rest.\n\r", ch);
	check_improve(ch,NULL,gsn_lightsleep,FALSE,4);
	WAIT_STATE(ch,  UMAX(ch->wait, skill_table[gsn_lightsleep].beats));
	return;
    }
	
    if (is_affected(ch, gsn_berserk))
	{
	send_to_char( "You can't sleep while consumed by rage!", ch);
	return;
	}
    if (IS_NAFFECTED(ch, AFF_ASHURMADNESS))
    {
	send_to_char("You must eat!\n\r",ch);
	return;
    }

    switch ( ch->position )
    {
    case POS_SLEEPING:
	send_to_char( "You are already sleeping.\n\r", ch );
	break;

    case POS_RESTING:
    case POS_SITTING:
    case POS_STANDING: 
	if (argument[0] == '\0' && ch->on == NULL)
	{
	    send_to_char( "You go to sleep.\n\r", ch );
	    act( "$n goes to sleep.", ch, NULL, NULL, TO_ROOM );
	    switch_position(ch, POS_SLEEPING);
	}
	else  /* find an object and sleep on it */
	{
	    if (argument[0] == '\0')
		obj = ch->on;
	    else
	    {
// brazen: first, check inventory
		obj = get_obj_list(ch, argument, ch->carrying);
                if(!obj)
// then, check ground
	       	    obj = get_obj_list( ch, argument,  ch->in_room->contents );
	    }

	    if (obj == NULL)
	    {
		send_to_char("You don't see that here.\n\r",ch);
		return;
	    }
	    if (obj->item_type != ITEM_FURNITURE
	    ||  (!IS_SET(obj->value[2],SLEEP_ON) 
	    &&   !IS_SET(obj->value[2],SLEEP_IN)
	    &&	 !IS_SET(obj->value[2],SLEEP_AT)))
	    {
		send_to_char("You can't sleep on that!\n\r",ch);
		return;
	    }

	    if (ch->on != obj && count_users(obj) >= obj->value[0])
	    {
		act_new("There is no room on $p for you.",
		    ch,obj,NULL,TO_CHAR,POS_DEAD);
		return;
	    }

	    ch->on = obj;
	    if (IS_SET(obj->value[2],SLEEP_AT))
	    {
		act("You go to sleep at $p.",ch,obj,NULL,TO_CHAR);
		act("$n goes to sleep at $p.",ch,obj,NULL,TO_ROOM);
	    }
	    else if (IS_SET(obj->value[2],SLEEP_ON))
	    {
	        act("You go to sleep on $p.",ch,obj,NULL,TO_CHAR);
	        act("$n goes to sleep on $p.",ch,obj,NULL,TO_ROOM);
	    }
	    else
	    {
		act("You go to sleep in $p.",ch,obj,NULL,TO_CHAR);
		act("$n goes to sleep in $p.",ch,obj,NULL,TO_ROOM);
	    }
	    switch_position(ch, POS_SLEEPING);
	    ch->on = obj;
	}
	check_improve(ch,NULL,gsn_lightsleep,TRUE,4);
	af.where     = TO_NAFFECTS;
	af.type      = gsn_lightsleep;
	af.level     = ch->level; 
	af.duration  = ch->level;
	af.location  = 0;
	af.modifier  = 0;
	af.bitvector = AFF_LIGHTSLEEP;
	affect_to_char( ch, &af );
	break;

    case POS_FIGHTING:
	send_to_char( "You are already fighting!\n\r", ch );
	break;
    }

    return;
}


void do_sleep( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj = NULL;
    AFFECT_DATA af;
    if (is_affected(ch, gsn_berserk))
	{
	send_to_char( "You can't sleep while consumed by rage!\n\r", ch);
	return;
	}
    if (IS_NAFFECTED(ch, AFF_ASHURMADNESS))
    {
	send_to_char("You must eat!\n\r",ch);
	return;
    }

    switch ( ch->position )
    {
    case POS_SLEEPING:
	send_to_char( "You are already sleeping.\n\r", ch );
	break;

    case POS_RESTING:
    case POS_SITTING:
    case POS_STANDING: 
	if (argument[0] == '\0' && ch->on == NULL)
	{
	    if (is_affected(ch,gsn_huntersense))
	    {
		send_to_char("You stop looking for tracks.\n\r",ch);	
		affect_strip(ch,gsn_huntersense);
	    }
	    if (is_affected(ch,gsn_pursuit))
	    {
		send_to_char("You stop looking for tracks.\n\r",ch);	
		affect_strip(ch,gsn_pursuit);
	    }
	    send_to_char( "You go to sleep.\n\r", ch );
	    act( "$n goes to sleep.", ch, NULL, NULL, TO_ROOM );
	    switch_position(ch, POS_SLEEPING);
	}
	else  /* find an object and sleep on it */
	{
	    if (argument[0] == '\0')
            obj = ch->on;
	    else 
	    {
            // Check the inventory for the obj, then the ground if not found
	    	obj = get_obj_list( ch, argument,  ch->carrying);
		    if (obj == NULL)
		        obj = get_obj_list( ch, argument, ch->in_room->contents);
	    }

	    if (obj == NULL)
	    {
            send_to_char("You don't see that here.\n\r",ch);
            return;
	    }

        if (obj->pIndexData->vnum == OBJ_VNUM_DISC && obj->item_type == ITEM_CONTAINER)
        {
            if (!try_convert_floating_disc_to_furniture(*ch, *obj))
                return;
        }

	    if (obj->item_type != ITEM_FURNITURE
	    ||  (!IS_SET(obj->value[2],SLEEP_ON) 
	    &&   !IS_SET(obj->value[2],SLEEP_IN)
	    &&	 !IS_SET(obj->value[2],SLEEP_AT)))
	    {
            send_to_char("You can't sleep on that!\n\r",ch);
            return;
	    }

	    if (ch->on != obj && count_users(obj) >= obj->value[0])
	    {
            act_new("There is no room on $p for you.", ch,obj,NULL,TO_CHAR,POS_DEAD);
            return;
	    }

	    if (IS_SET(obj->value[2],SLEEP_AT))
	    {
            act("You go to sleep at $p.",ch,obj,NULL,TO_CHAR);
            act("$n goes to sleep at $p.",ch,obj,NULL,TO_ROOM);
	    }
	    else if (IS_SET(obj->value[2],SLEEP_ON))
	    {
	        act("You go to sleep on $p.",ch,obj,NULL,TO_CHAR);
	        act("$n goes to sleep on $p.",ch,obj,NULL,TO_ROOM);
	    }
	    else
	    {
            act("You go to sleep in $p.",ch,obj,NULL,TO_CHAR);
            act("$n goes to sleep in $p.",ch,obj,NULL,TO_ROOM);
	    }

	    switch_position(ch, POS_SLEEPING);
	    ch->on = obj;
	    if (!IS_NPC(ch) && BIT_GET(ch->pcdata->traits,TRAIT_LIGHTSLEEPER))
	    {
            af.where     = TO_NAFFECTS;
            af.type      = gsn_lightsleep;
            af.level     = ch->level; 
            af.duration  = -1;
            af.location  = 0;
            af.modifier  = 0;
            af.bitvector = AFF_LIGHTSLEEP;
            affect_to_char( ch, &af );
	    }

	    send_to_char("You drift off to sleep almost immediately...\n\r", ch);
	}
	break;

    case POS_FIGHTING:
	send_to_char( "You are already fighting!\n\r", ch );
	break;
    }

    return;
}


void do_wake( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
	{ do_stand( ch, argument ); return; }

    if ( !IS_AWAKE(ch) )
	{ send_to_char( "You are asleep yourself!\n\r",       ch ); return; }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
	{ send_to_char( "They aren't here.\n\r",              ch ); return; }

    if ( IS_AWAKE(victim) )
	{ act( "$N is already awake.", ch, NULL, victim, TO_CHAR ); return; }

    act_new("$n shakes you, trying to wake you.", ch, NULL, victim, TO_VICT,POS_SLEEPING);
    WAIT_STATE(ch, UMAX(ch->wait, 18));
    do_stand_helper(victim, "", ch);
}

void do_wildmovement( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int cost = 0;

    if (!get_skill(ch, gsn_wildmove))
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }
    
    if (is_affected(ch, gsn_wildmove))
    {
	send_to_char("You are already moving cautiously.\n\r", ch);
	return;
    }

    if (ch->pcdata != NULL)
    {
        cost = ch->level > 40 ? 3:(ch->level>20 ? 4:5);
        cost -= ch->pcdata->learned[gsn_wildmove] > 75 ?
               (ch->pcdata->learned[gsn_wildmove] > 85? 2:1):0;
    }

    if (ch->mana < cost)
    {
	send_to_char("You're too tired to move cautiously.\n\r", ch);
	return;
    }

    expend_mana(ch, cost);

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_wildmove].beats));

    if ((ch->in_room->sector_type != SECT_FOREST) && (ch->in_room->sector_type != SECT_MOUNTAIN) && (ch->in_room->sector_type != SECT_HILLS) && (ch->in_room->sector_type != SECT_SWAMP) && (ch->in_room->sector_type != SECT_FIELD))
    {
	send_to_char("You cannot use your wilderness skills here.\n\r", ch);
	return;
    } 

    send_to_char("You attempt to move cautiously through the wilderness.\n\r", ch);

    if ( number_percent( ) < get_skill(ch,gsn_wildmove))
    {
	check_improve(ch,NULL,gsn_wildmove,TRUE,1);
	af.where     = TO_AFFECTS;
	af.type      = gsn_wildmove;
	af.level     = ch->level; 
	af.duration  = ch->level;
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector = 0;
	affect_to_char( ch, &af );
    }
    else
	check_improve(ch,NULL,gsn_wildmove,FALSE,1);

    return;
}

void do_sneak( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int cost = 0;

    affect_strip( ch, gsn_sneak );

    if (ch->pcdata != NULL)
    {
    cost = ch->level > 40 ? 3:(ch->level>20 ? 4:5);
    cost -= ch->pcdata->learned[gsn_sneak] > 85 ?
(ch->pcdata->learned[gsn_sneak] > 95? 2:1):0;
    }

    if (ch->mana < cost)
	{
	send_to_char("You're too tired to sneak around.\n\r", ch);
	return;
	}

    expend_mana(ch, cost);

    if (IS_AFFECTED(ch,AFF_SNEAK))
	return;

    if ( number_percent( ) < get_skill(ch,gsn_sneak))
    {
	send_to_char("You begin moving silently.\n\r",ch);
	check_improve(ch,NULL,gsn_sneak,TRUE,3);
	af.where     = TO_AFFECTS;
	af.type      = gsn_sneak;
	af.level     = ch->level; 
	af.duration  = ch->level;
	af.location  = APPLY_NONE;
	af.modifier  = 0;
	af.bitvector = AFF_SNEAK;
	affect_to_char( ch, &af );
    }
    else
    {
	send_to_char("You attempt to move silently, but quickly realize you aren't focused enough.\n\r",ch);
	check_improve(ch,NULL,gsn_sneak,FALSE,3);
    }

    return;
}

void do_camoublind( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *vch;
    int chance;
    bool ch_in_group = FALSE;

    if ((chance = get_skill(ch,gsn_camoublind)) == 0)
    {
        send_to_char("Huh?\n\r",ch);
        return;
    }

    if ((ch->in_room->sector_type != SECT_HILLS) && (ch->in_room->sector_type != SECT_FOREST) && (ch->in_room->sector_type != SECT_MOUNTAIN) && (ch->in_room->sector_type != SECT_SWAMP))
    {
	send_to_char("You can't find sufficient cover for that here.\n\r", ch);
	return;
    }

    if (ch->mana < skill_table[gsn_camoublind].min_mana)
    {
	send_to_char("You are too tired to arrange cover here.\n\r", ch);
	return;
    }

    af.where     = TO_AFFECTS;
    af.type      = gsn_camoublind;
    af.level     = ch->level;
    af.location  = APPLY_HIDE;
    if (number_percent() > chance)
        af.modifier  = 0;
    else
	af.modifier  = 1;
    af.duration  = -1;
    af.bitvector = 0;

    if (is_affected(ch,gsn_camouflage))
	affect_strip(ch,gsn_camouflage);
    if (is_affected(ch,gsn_camoublind))
	affect_strip(ch,gsn_camoublind);

  
    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
	if (!is_same_group(ch, vch) || (ch == vch))
	    continue;
	ch_in_group = TRUE;
        if (!is_affected(vch, gsn_camoublind))
	{
	    act("$n attempts to conceal you behind a camouflage blind.", ch, NULL, vch, TO_VICT);
	    affect_to_char(vch, &af);
	}
    }

    if (ch_in_group)
    {
	affect_to_char(ch, &af);
	send_to_char("You attempt to conceal yourself and your group behind a camouflage blind.\n\r", ch);
	act("$n attempts to conceal $mself and $s group behind a camouflage blind.", ch, NULL, NULL, TO_ROOM);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_camoublind].beats));
	expend_mana(ch, skill_table[gsn_camoublind].min_mana);
	return;
    }
    else
    {
	send_to_char("But you are not in a group!\n\r", ch);
	return;
    }
}

void do_camouflage( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int cost = 0;
    int chance;

    if (!str_cmp(argument, "blind"))
    {
	do_camoublind(ch, argument);
	return;
    }

    if ((chance = get_skill(ch,gsn_camouflage)) == 0)
    {
        send_to_char("Huh?\n\r",ch);
        return;
    }

    if ( IS_AFFECTED(ch, AFF_FAERIE_FIRE))
    {
	send_to_char("You can't camouflage while glowing.\n\r", ch);
	return;
    }
 
    if ( is_affected(ch, gsn_camouflage))
	affect_strip(ch, gsn_camouflage);

    if (ch->in_room && room_is_affected(ch->in_room, gsn_etherealblaze))
    {
	send_to_char("The flames around you prevent you from finding cover.\n\r", ch);
	return;
    }

    if ((ch->in_room->sector_type != SECT_HILLS)
      && (ch->in_room->sector_type != SECT_FOREST)
      && (ch->in_room->sector_type != SECT_MOUNTAIN)
      && (ch->in_room->sector_type != SECT_SWAMP)
      && (ch->in_room->sector_type != SECT_FIELD))
    {
	send_to_char("You can't find sufficient cover for that here.\n\r", ch);
	return;
    }

    if (IS_AFFECTED(ch, AFF_BLIND) && (ch->race != global_int_race_shuddeni))
    {
	send_to_char("You stumble about blindly, attempting to camouflage yourself.\n\r", ch);
	chance /= 5;
    }
    else
	send_to_char("You attempt to conceal yourself in the surrounding terrain.\n\r", ch );

    if (ch->pcdata != NULL)
    {
        cost = ch->level > 40 ? 3:(ch->level>20 ? 4:5);
        cost -= ch->pcdata->learned[gsn_camouflage] > 75 ? (ch->pcdata->learned[gsn_camouflage] > 85 ? 2 : 1 ) : 0;
        if (ch->mana > cost)
    	    expend_mana(ch, cost);
        else
        {
	    send_to_char("You're too tired to camouflage yourself.\n\r", ch);
	    return;
	}
    }

    if ( number_percent( ) < chance)
    {
        af.where     = TO_AFFECTS;
        af.type      = gsn_camouflage;
        af.level     = ch->level;
        af.duration  = ch->level/2;
        af.location  = APPLY_HIDE;
        af.modifier  = 0;
        af.bitvector = 0;
        affect_to_char( ch, &af );

	check_improve(ch,NULL,gsn_camouflage,TRUE,3);
    }
    else
	check_improve(ch,NULL,gsn_camouflage,FALSE,3);


    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_camouflage].beats));
    return;
}

bool unhide_char( CHAR_DATA *ch )
{
    if (IS_AFFECTED(ch, AFF_HIDE))
    {
        REMOVE_BIT(ch->affected_by, AFF_HIDE);
	affect_strip(ch, gsn_cloakofshadows);
        act( "$n steps out of the shadows.", ch, NULL, NULL, TO_ROOM );
        send_to_char("You step out of the shadows.\n\r", ch);

        if (is_affected(ch,gsn_setambush))
	{
            affect_strip(ch,gsn_setambush);
            send_to_char("You are no longer prepared to ambush.\n\r",ch);
        }

	stop_playing_sn(ch, gsn_cloakofshadows);
/*
	if (ch->song && (ch->song->type == gsn_cloakofshadows))
	    stop_playing_song(ch);
*/

	return TRUE;
    }

    return FALSE;
}   

bool uncamo_char( CHAR_DATA *ch )
{
    if (is_affected(ch,gsn_camouflage))
    {
        affect_strip(ch,gsn_camouflage);
	if (!(ch->race == global_int_race_ethron || is_affected(ch,gsn_forestwalk)))
	{
            act( "$n steps out from the foliage.", ch, NULL, NULL, TO_ROOM );
            send_to_char("You step out from the foliage.\n\r", ch);
	}
	else
	    send_to_char("You quietly slip out from the foliage.\n\r",ch);

        if (is_affected(ch,gsn_setambush))
        {
            affect_strip(ch,gsn_setambush);
            send_to_char("You are no longer prepared to ambush.\n\r",ch);
        }

	return TRUE;
    }

    return FALSE;
}
/*
 * Contributed by Alander.
 */
void do_visible( CHAR_DATA *ch, char *argument )
{
    int i=0;
    
    if (is_affected(ch, gsn_cloak))
	send_to_char("You stop cloaking your presence.\n\r", ch);

    if (IS_AFFECTED(ch, AFF_SNEAK) && !(race_table[ch->race].aff & AFF_SNEAK) && !IS_NPC(ch))
	send_to_char("You stop sneaking quietly.\n\r", ch);

    if (is_affected(ch, gsn_wildmove))
   	send_to_char("You stop moving carefully.\n\r", ch); 

    if (is_affected(ch, gsn_forestwalk))
	send_to_char("You cease forest walking.\n\r", ch);

    if (is_affected(ch, gsn_greaterinvis) && strcmp(argument, "FALSE"))
    {
    	affect_strip (ch, gsn_greaterinvis);
	send_to_char("You fade into existence.\n\r", ch);
	act("$n fades into existence.", ch, NULL, NULL, TO_ROOM);
	REMOVE_BIT   ( ch->affected_by, AFF_INVISIBLE	);
    }

    bool shouldStrip(true);
    if (!strcmp(argument, "FALSE"))
    {
        if (is_affected(ch, gsn_greaterinvis)) 
            shouldStrip = false;
        else
        {
            if (number_percent() <= get_skill(ch, gsn_endlessfacade))
            {
                check_improve(ch, NULL, gsn_endlessfacade, true, 2);
                shouldStrip = false;
            }
            else
                check_improve(ch, NULL, gsn_endlessfacade, false, 2);
        }
    }

    if (IS_AFFECTED(ch, AFF_INVISIBLE) && shouldStrip)
    {
        send_to_char("You fade into existence.\n\r", ch);
        act("$n fades into existence.", ch, NULL, NULL, TO_ROOM);
        REMOVE_BIT   ( ch->affected_by, AFF_INVISIBLE	);
    }
    
    uncamo_char(ch);
    unhide_char(ch);

    if (IS_PAFFECTED(ch, AFF_VOIDWALK))
    {  
        AFFECT_DATA af;
        af.valid = TRUE;
        af.point = NULL;

        act("You shift back into the material world.", ch, NULL, NULL, TO_CHAR);
        act("$n shifts $s essense back into the material world.", ch, NULL, NULL, TO_ROOM);
        ch->hit = UMAX(1, ch->hit / 4);
        affect_strip(ch, gsn_voidwalk);

        af.where     = TO_PAFFECTS;
        af.type      = gsn_voidwalk;
        af.level     = ch->level;
        af.duration  = ch->level / 4;
        af.location  = APPLY_HIT;
        af.modifier  = ch->level * -1;
        af.bitvector = 0;
        affect_to_char(ch, &af);

        WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE * 2));
        send_to_char("You pause for a moment, feeling drained from the experience.\n\r", ch);
    }

    if (shouldStrip)
        affect_strip(ch, gsn_invis);

    affect_strip ( ch, gsn_mass_invis			);
    affect_strip ( ch, gsn_sneak			);
    affect_strip ( ch, gsn_wildmove                     );
    affect_strip ( ch, gsn_forestwalk			);
    affect_strip ( ch, gsn_cloak			);
    if (!IS_NPC(ch))
	for (i=0;i<focus_slots(ch);i++)
	    if(ch->pcdata->focus_sn[i] == gsn_cloak)
	    {
		unfocus(ch,i,TRUE);
		i = UMAX(-1,i-2);
	    }
    if (!race_table[ch->race].aff & AFF_SNEAK)
        REMOVE_BIT   ( ch->affected_by, AFF_SNEAK		);
    return;
}

void do_recall( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    ROOM_INDEX_DATA *location;
    AFFECT_DATA *paf;
    bool cFound;

    if (IS_NPC(ch))
    {
	send_to_char("Only players can recall.\n\r",ch);
	return;
    }
  
    if (ch->recall_to)
	location = ch->recall_to;
    else
    {
	send_to_char( "You have no location to recall to.\n\r", ch );
	return;
    }

    if ( ch->in_room == location )
    {
	send_to_char("You are already at your recall location.\n\r", ch);
	return;
    }

    act( "$n prays for transportation!", ch, 0, 0, TO_ROOM );

    if ( ( victim = ch->fighting ) != NULL )
    {
	send_to_char("You can't recall while fighting!\n\r", ch);
	return;
    }
    
    if ((ch->level > (BIT_GET(ch->pcdata->traits, TRAIT_PIOUS) ? 25 : 11)) && !IS_OAFFECTED(ch, AFF_GHOST))
    {
	/* Check for a circle of stones */
	cFound = FALSE;
	for (paf = location->affected; paf; paf = paf->next)
	    if ((paf->type == gsn_circleofstones) && (paf->modifier == ch->id))
	    {
		cFound = TRUE;
		break;
	    }

	if (!cFound)
	{
	    send_to_char("You're too high ranked to call on the gods for transportation.\n\r", ch);
	    return;
	}
    }
    
    if (!IS_NPC(ch) && (ch->pcdata->adrenaline > 0) && (ch->level > 10))
    {
	send_to_char("You need to calm down before you can recall.\n\r", ch);
	return;
    }
    
    if (ch->in_room)
    {
	if (is_affected(ch, gsn_matrix))
	{
	    send_to_char("You are trapped by the power of the matrix.\n\r", ch);
	    return;
	}

        if (IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
         || IS_AFFECTED(ch, AFF_CURSE)
	 || (area_is_affected(ch->in_room->area, gsn_suppress)
	  && !is_affected(ch, gsn_spirit_of_freedom)))
    	{
	    send_to_char( "The gods have forsaken you.\n\r", ch );
	    return;
    	}
    }

    uncamo_char(ch);

    ch->move = UMIN(ch->move, ch->move /= 2);
    act( "$n disappears.", ch, NULL, NULL, TO_ROOM );
    char_from_room( ch );
    char_to_room( ch, location );
    act( "$n appears in the room.", ch, NULL, NULL, TO_ROOM );
    do_look( ch, "auto" );
    
    if ((ch->familiar != NULL) && (ch->familiar->in_room == ch->in_room))
	do_recall(ch->familiar,"");

    if ((ch->pet != NULL) && (ch->pet->in_room == ch->in_room))
	do_recall(ch->pet,"");

    return;
}


void do_spiritrecall(CHAR_DATA *ch, char *argument)
{
    ROOM_INDEX_DATA *pRoomIndex, *room=NULL;
    CHAR_DATA *vch, *vch_next;
    OBJ_DATA *obj;
    int iHash;

    if (!ch->in_room)
	return;

    if (get_skill(ch, gsn_runeofspirit) <= 0)
    {
	    send_to_char("Huh?\n\r", ch);
    	return;
    }

    // Lookup the modifier
    int mod(-1);
    for (AFFECT_DATA * runePaf(get_affect(ch, gsn_runeofspirit)); runePaf != NULL; runePaf = get_affect(ch, gsn_runeofspirit, runePaf))
    {
        if (runePaf->location == APPLY_HIDE)
        {
            mod = runePaf->modifier;
            break;
        }
    }

    if (mod == -1)
    {
    	send_to_char("You have no sense of a rune of spirit in the world.\n\r", ch);
	    return;
    }

    if (is_an_avatar(ch))
    {
        send_to_char("You cannot find your rune in such an altered state!\n", ch);
        return;
    }

    if (ch->in_room)
    {

        if (is_affected(ch, gsn_matrix))
        {
	    if (check_spirit_of_freedom(ch))
	        send_to_char("You feel the spirit of freedom surge within you, allow you escape from the matrix.\n\r", ch);
	    else
	    {
        	send_to_char("You are trapped by the powers of the matrix.\n\r", ch);
                return;
	    }
        }

        if (IS_SET(ch->in_room->room_flags,ROOM_NO_RECALL)
         || IS_AFFECTED(ch, AFF_CURSE)
        || (area_is_affected(ch->in_room->area, gsn_suppress)
	 && !is_affected(ch, gsn_spirit_of_freedom))
	|| (silver_state == 16))
        {
            send_to_char("The gods have forsaken you.\n\r",ch);
            return;
        }
    }

    if (is_affected(ch, gsn_unrealincursion))
    {
        send_to_char("Your reality is too distorted to find a clear path across the Weave.\n", ch);
        return;
    }
       
    if (mod > 10000)
    {
	for (obj = object_list; obj != NULL; obj = obj->next)
	{
	    if (obj_is_affected(obj, gsn_runeofspirit) && get_modifier(obj->affected, gsn_runeofspirit) == mod)
	    {
	        if ((room = get_room_for_obj(*obj)) == NULL)
            {
                send_to_char("The rune of spirit is beyond your reach.\n\r", ch);
                bug("obj for rune of spirit not in room", 0);
                affect_strip(ch, gsn_runeofspirit);
                object_affect_strip(obj, gsn_runeofspirit);
                return;
            }

 	        act("$n reaches out for a rune of spirit, pulling $mself and $s group across the weave.", ch, NULL, NULL, TO_ROOM);
	        act("You reach out for the rune of spirit, pulling yourself and your group across the weave.", ch, NULL, NULL, TO_CHAR);

	    	if (IS_SET(room->room_flags, ROOM_NOGATE)
    		||   (room->area->area_flags & AREA_UNCOMPLETE )
    	 	||   IS_SET(ch->in_room->room_flags, ROOM_NOMAGIC)
    		||   IS_SET(room->room_flags, ROOM_SAFE)
    		||   IS_SET(room->room_flags, ROOM_PRIVATE)
    		||   IS_SET(room->room_flags, ROOM_SOLITARY)
    		||   IS_SET(room->room_flags, ROOM_NO_RECALL)
    		||   IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL))
		{
		    send_to_char("The rune of spirit is beyond your reach.\n\r", ch);
		    affect_strip(ch, gsn_runeofspirit);
		    object_affect_strip(obj, gsn_runeofspirit);
	 	    return;
		}

		for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
	 	{
		    vch_next = vch->next_in_room;
		    if (is_same_group(ch, vch))
	            {
// brazen: Cursed groupmates do not pass go, do not collect $200
 	                if(IS_AFFECTED(vch, AFF_CURSE) && 
	                !is_affected(ch, gsn_spirit_of_freedom))
			{
	                    act("The gods have forsaken you.", vch, NULL, NULL, TO_CHAR);
	                    act("The gods have forsaken $n.", vch, NULL, ch, TO_VICT);
	                }
			else
			{
	                    act("The world spins around you, leaving you dizzy and gasping for breath!", vch, NULL, NULL, TO_CHAR);
	                    char_from_room(vch);
	                    char_to_room(vch, room);
	                    act("$n fades into view, materializing from nothing.", vch, NULL, NULL, TO_ROOM);
	                    do_look(vch, "auto");
	                }
	            }
        }

	        affect_strip(ch, gsn_runeofspirit);
   	        object_affect_strip(obj, gsn_runeofspirit);
            apply_runeofspirit_cooldown(ch);
	        return;
	    }
        }
// brazen: If this point in the code is reached, we have failed to locate an object with this person's
// rune of spirit on it. It was, however, cast on an object, which means that this object has failed to exist
// (eaten ration cakes, destroyed items). This is comparable to airline travel, where your luggage is removed
// from the object list with no explanation. We don't really want to remove the affect then, because it will 
// spoil the surprise.  This was "working" before, because objects were not properly removed from the object 
// list, but this consumes increasing amounts of memory.  We now need to remove the affect from the PC so 
// they can cast another rune.
        send_to_char("You failed.\n\r", ch);
        affect_strip(ch, gsn_runeofspirit);
	return;
    }
    else /* the runeofspirit is on a room */
    {
        for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    	{   
            for( pRoomIndex = room_index_hash[iHash]; pRoomIndex; pRoomIndex = pRoomIndex->next )
	    {
	        if (room_is_affected(pRoomIndex, gsn_runeofspirit) && get_modifier(pRoomIndex->affected, gsn_runeofspirit) == mod)
	        {
	            room = pRoomIndex;
	            break;
	        }
	    }
	}

	if (room == NULL)
	{
	    send_to_char("You do not sense a rune of spirit is beyond your reach.\n\r", ch);
	    bug("10000+ modifier on rune of spirit, but no room with affect", 0);
	    affect_strip(ch, gsn_runeofspirit);
	    room_affect_strip(room, gsn_runeofspirit);
	    return;
	}

 	act("$n reaches out for a rune of spirit, pulling $mself and $s group across the weave.", ch, NULL, NULL, TO_ROOM);
	act("You reach out for the rune of spirit, pulling yourself and your group across the weave.", ch, NULL, NULL, TO_CHAR);

        if (IS_SET(room->room_flags, ROOM_NOGATE)
    	||   (room->area->area_flags & AREA_UNCOMPLETE )
    	||   IS_SET(ch->in_room->room_flags, ROOM_NOMAGIC)
    	||   IS_SET(room->room_flags, ROOM_SAFE)
    	||   IS_SET(room->room_flags, ROOM_PRIVATE)
    	||   IS_SET(room->room_flags, ROOM_SOLITARY)
    	||   IS_SET(room->room_flags, ROOM_NO_RECALL)
    	||   IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL))
	{
	    send_to_char("The rune of spirit is beyond your reach.\n\r", ch);
	    affect_strip(ch, gsn_runeofspirit);
	    room_affect_strip(room, gsn_runeofspirit);
	    return;
	}

	for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
	{
	    vch_next = vch->next_in_room;
	    if (is_same_group(ch, vch))
	    {
// brazen: Cursed groupmates do not pass go, do not collect $200
                if(IS_AFFECTED(vch, AFF_CURSE) && 
                !is_affected(ch, gsn_spirit_of_freedom))
		{
                    act("The gods have forsaken you.", vch, NULL, NULL, TO_CHAR);
                    act("The gods have forsaken $n.", vch, NULL, ch, TO_VICT);
		}
		else
                {
                    act("The world spins around you, leaving you dizzy and gasping for breath!", vch, NULL, NULL, TO_CHAR);
                    char_from_room(vch);
                    char_to_room(vch, room);
                    act("$n fades into view, materializing from nothing.", vch, NULL, NULL, TO_ROOM);
                    do_look(vch, "auto");
                }
	    }
        }

        affect_strip(ch, gsn_runeofspirit);
        room_affect_strip(room, gsn_runeofspirit);
        apply_runeofspirit_cooldown(ch);
        return;
    }
}

void do_forcedmarch(CHAR_DATA *ch, char *argument)
{
   AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (!get_skill(ch, gsn_forcedmarch))
    {   
        send_to_char("Huh?\n\r", ch);
        return;
    }
    
    if (is_affected(ch, gsn_forcedmarch))
    {
      send_to_char("You don't feel ready to begin a forced march again yet.\n\r", ch);
      return;
    }

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_forcedmarch].beats));

    if (number_percent() > get_skill(ch, gsn_forcedmarch))
	{
	  send_to_char("You try to prepare for a grueling march, but fail.\n\r", ch);
	  return;
	}

    check_improve(ch,NULL, gsn_forcedmarch, TRUE, 3);

    af.where     = TO_NAFFECTS;
    af.type      = gsn_forcedmarch;
    af.level     = ch->level;
    af.duration  = 12;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = AFF_FORCEDMARCH;
    affect_to_char( ch, &af );

    af.duration  = 36;
    af.bitvector = 0;
    affect_to_char( ch, &af);

    act("You prepare for a grueling march, ready to stretch your endurance to keep moving.", ch, NULL, NULL, TO_CHAR);
    act("$n adjusts $mself, and prepares for a grueling but unflagging travel pace.", ch, NULL, NULL, TO_ROOM);
}

void do_dash(CHAR_DATA *ch, char *argument)
{
   AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (!get_skill(ch, gsn_dash))
    {   
        send_to_char("Huh?\n\r", ch);
        return;
    }
    
    if (is_affected(ch, gsn_dash))
    {
	if (IS_PAFFECTED(ch,AFF_DASH))
	    send_to_char("You are already prepared to dash away.\n\r",ch);
	else
	    send_to_char("You don't feel ready to dash again yet.\n\r", ch);
	return;
    }

    if (number_percent() > get_skill(ch, gsn_dash))
    {
	send_to_char("You fail to prepare yourself to make an escape.\n\r", ch);
	check_improve(ch,NULL,gsn_dash,TRUE,2);
	return;
    }

    check_improve(ch,NULL, gsn_dash, TRUE, 1);

    af.where     = TO_PAFFECTS;
    af.type      = gsn_dash;
    af.level     = ch->level;
    af.duration  = 2;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = AFF_DASH;
    affect_to_char( ch, &af );

    af.duration  = 5;
    af.bitvector = 0;
    affect_to_char( ch, &af);

    act("You prepare yourself for a quick getaway.", ch, NULL, NULL, TO_CHAR);
}

void do_drag(CHAR_DATA *ch, char *argument)
{
    int direction;
    bool vict_is_person;
    CHAR_DATA *victim;
    OBJ_DATA *vobj;
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    EXIT_DATA *pexit;
    ROOM_INDEX_DATA *start_room;
    int chance;

/* I commend you on copious comments -- mw */

    if ((chance = get_skill(ch,gsn_drag)) == 0)
    {
        send_to_char("Huh?\n\r",ch);
        return;
    }

    if (IS_OAFFECTED(ch, AFF_GHOST))
    {
      send_to_char("Your ghostly hands pass through the corpse.\n\r",ch);
      return;
    }
		    
    
/* First line: We get the target of drag, which gets labelled arg1 */

    argument = one_argument( argument, arg1 );

/* Second word: Where are they going to drag it? N/s/e/w/up/d/shadows */
 
    argument = one_argument( argument, arg2 );


    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
        send_to_char( "Drag what to where?\n\r", ch );
        return;
    }

/* Tells us if its a player or an object they're moving.
 * Similar to taint skill
 */
 
    if ((victim = get_char_room(ch, arg1)) == NULL)
    {
        if ((vobj = get_obj_list( ch, arg1, ch->in_room->contents)) == NULL)
        {
          send_to_char( "You don't see that person or thing here.\n\r", ch );
          return;
        }
        else
          vict_is_person = FALSE;
    }
    else
        vict_is_person = TRUE;

    start_room = ch->in_room;
    
    if (vict_is_person)
    {   
        if (victim->position == POS_SLEEPING)
        {
 
	    if (!str_prefix(arg2, "north")) direction = 0;
            else if (!str_prefix(arg2, "east")) direction = 1;
            else if (!str_prefix(arg2, "south")) direction = 2;
            else if (!str_prefix(arg2, "west")) direction = 3;
            else if (!str_prefix(arg2, "up")) direction = 4;
            else if (!str_prefix(arg2, "down")) direction = 5;
	    else
	    {
	        send_to_char("That's not a valid direction.\n\r", ch);
	        return;
	    }
        }     
        else 
        {
            send_to_char("You can't drag them if they aren't asleep or dead.\n\r",ch);
            return; 
        }

        if ((pexit = ch->in_room->exit[direction]) == NULL)
        {
            send_to_char("There's nothing in the direction.\n\r", ch);
            return;
        }
	if (pexit->u1.to_room->sector_type == SECT_AIR
	  && !is_flying(victim))
	{
	    send_to_char("How would they stay up there?\n\r",ch);
	    return;
	    
	}
        WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_drag].beats));

	act("$n starts to drag $N out of the room.", ch, NULL, victim, TO_ROOM);
	move_char(ch, direction, TRUE);

	if (number_percent() < chance && ch->in_room != start_room)
	{
	    char_from_room(victim);
	    char_to_room(victim, ch->in_room);
	    act("You drag $N into the room.", ch, NULL, victim, TO_CHAR);
	    act("$n drags $N into the room.", ch, NULL, victim, TO_NOTVICT);
	}
	else
	{
	    act("You fail to drag $N very far.", ch, NULL, victim, TO_CHAR);
	    act("$n tries to drag $N, but doesn't get far.", ch, NULL, victim, TO_NOTVICT);
	}

	if (!IS_AFFECTED(victim,AFF_SLEEP) || number_percent() > (chance*3/4))
	{
	    act("Your dragging jostles $N awake!", ch, NULL, victim, TO_CHAR);
	    act("You are jostled awake by someone dragging you along the ground!", victim, NULL, NULL, TO_CHAR);
	    damage(ch, victim, 1, gsn_drag, DAM_SLASH, TRUE);
	    check_improve(ch,victim, gsn_drag, FALSE, 1);
	}
	else
            check_improve(ch,victim,gsn_drag,TRUE,1);
        return;
    }
    else /* if (vict_is_person == FALSE) */
    {
        vobj = get_obj_list( ch, arg1, ch->in_room->contents); 
        if (!(vobj->item_type == ITEM_CORPSE_PC
	  || vobj->item_type == ITEM_CORPSE_NPC))
        {
            send_to_char("That's not a corpse.\n\r",ch);
            return;
        }
	if (!str_prefix(arg2, "north")) direction = 0;
        else if (!str_prefix(arg2, "east")) direction = 1;
        else if (!str_prefix(arg2, "south")) direction = 2;
        else if (!str_prefix(arg2, "west")) direction = 3;
        else if (!str_prefix(arg2, "up")) direction = 4;
        else if (!str_prefix(arg2, "down")) direction = 5;
        else
        {
            send_to_char("That's not a valid direction.\n\r", ch);
            return;
        }
          
	if ((pexit = ch->in_room->exit[direction]) == NULL)
        {
            send_to_char("There's nothing in the direction.\n\r", ch);
            return;
        }
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_drag].beats));
       
        act("You drag a corpse out of the room.",ch, NULL, NULL, TO_CHAR); 
        act("$n drags a corpse out of the room.",ch, NULL, NULL, TO_ROOM);
        move_char(ch,direction,FALSE);
        obj_from_room(vobj);
        obj_to_room (vobj,ch->in_room);
        check_improve(ch,NULL,gsn_drag,TRUE,1);
        act("$n drags a corpse into the room.",ch, NULL, NULL, TO_ROOM); 
        return;
    }
}

void do_train( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *mob;
    int stat = - 1;
    char *pOutput = NULL;
    int cost;

    if ( IS_NPC(ch) )
	return;

    /*
     * Check for trainer.
     */
    for ( mob = ch->in_room->people; mob; mob = mob->next_in_room )
    {
	if ( IS_NPC(mob) && IS_SET(mob->act, ACT_TRAIN) )
	    break;
    }

    if ( mob == NULL )
    {
	send_to_char( "You can't do that here.\n\r", ch );
	return;
    }

    if ( argument[0] == '\0' )
    {
	sprintf( buf, "You have %d training sessions.\n\r", ch->train );
	send_to_char( buf, ch );
	argument = "foo";
    }

    cost = 1;

    if ( !str_cmp( argument, "str" ) )
    {
	if (ch->attr_prime[STAT_STR] > 0)
	    cost    = 1;
	stat        = STAT_STR;
	pOutput     = "strength";
    }

    else if ( !str_cmp( argument, "int" ) )
    {
	if (ch->attr_prime[STAT_INT] > 0)
	    cost    = 1;
	stat	    = STAT_INT;
	pOutput     = "intelligence";
    }

    else if ( !str_cmp( argument, "wis" ) )
    {
	if (ch->attr_prime[STAT_WIS] > 0)
	    cost    = 1;
	stat	    = STAT_WIS;
	pOutput     = "wisdom";
    }

    else if ( !str_cmp( argument, "dex" ) )
    {
	if (ch->attr_prime[STAT_DEX] > 0)
	    cost    = 1;
	stat  	    = STAT_DEX;
	pOutput     = "dexterity";
    }

    else if ( !str_cmp( argument, "con" ) )
    {
	if (ch->attr_prime[STAT_CON] > 0)
	    cost    = 1;
	stat	    = STAT_CON;
	pOutput     = "constitution";
    }

    else if ( !str_cmp( argument, "chr" ) )
    {
	if (ch->attr_prime[STAT_CHR] > 0)
	    cost    = 1;
	stat	    = STAT_CHR;
	pOutput     = "charisma";
    }

    else if ( !str_cmp(argument, "hp" ) )
	cost = 1;

    else if ( !str_cmp(argument, "mana" ) )
	cost = 1;

    else
    {
	strcpy( buf, "You can train:" );
	if ( ch->perm_stat[STAT_STR] < get_max_train(ch,STAT_STR)) 
	    strcat( buf, " str" );
	if ( ch->perm_stat[STAT_INT] < get_max_train(ch,STAT_INT))  
	    strcat( buf, " int" );
	if ( ch->perm_stat[STAT_WIS] < get_max_train(ch,STAT_WIS)) 
	    strcat( buf, " wis" );
	if ( ch->perm_stat[STAT_DEX] < get_max_train(ch,STAT_DEX))  
	    strcat( buf, " dex" );
	if ( ch->perm_stat[STAT_CON] < get_max_train(ch,STAT_CON))  
	    strcat( buf, " con" );
	if ( ch->perm_stat[STAT_CHR] < get_max_train(ch,STAT_CHR))  
	    strcat( buf, " chr" );
	strcat( buf, " hp mana");

	if ( buf[strlen(buf)-1] != ':' )
	{
	    strcat( buf, ".\n\r" );
	    send_to_char( buf, ch );
	}
	else
	{
	    /*
	     * This message dedicated to Jordan ... you big stud!
	     */
	    act( "You have nothing left to train, you $T!",
		ch, NULL,
		ch->sex == SEX_MALE   ? "big stud" :
		ch->sex == SEX_FEMALE ? "hot babe" :
					"wild thing",
		TO_CHAR );
	}

	return;
    }

    if (!str_cmp("hp",argument))
    {
    	if ( cost > ch->train )
    	{
       	    send_to_char( "You don't have enough training sessions.\n\r", ch );
            return;
        }
 
	ch->train -= cost;
        ch->pcdata->perm_hit += 10;
        ch->max_hit += 10;
        ch->hit +=10;
	ch->pcdata->hp_trains++;
        act( "Your durability increases!",ch,NULL,NULL,TO_CHAR);
        act( "$n's durability increases!",ch,NULL,NULL,TO_ROOM);
        return;
    }
 
    if (!str_cmp("mana",argument))
    {
        if ( cost > ch->train )
        {
            send_to_char( "You don't have enough training sessions.\n\r", ch );
            return;
        }

	ch->train -= cost;
        ch->pcdata->perm_mana += 10;
        ch->max_mana += 10;
        ch->mana += 10;
	ch->pcdata->mana_trains++;
        act( "Your power increases!",ch,NULL,NULL,TO_CHAR);
        act( "$n's power increases!",ch,NULL,NULL,TO_ROOM);
        return;
    }

    if ( ch->perm_stat[stat]  >= get_max_train(ch,stat) )
    {
	act( "Your $T is already at maximum.", ch, NULL, pOutput, TO_CHAR );
	return;
    }

    if ( cost > ch->train )
    {
	send_to_char( "You don't have enough training sessions.\n\r", ch );
	return;
    }

    ch->train		-= cost;

    set_perm_stat(ch, stat, ch->perm_stat[stat] + 1);  
    act( "Your $T increases!", ch, NULL, pOutput, TO_CHAR );
    act( "$n's $T increases!", ch, NULL, pOutput, TO_ROOM );
    return;
}

DO_FUNC(do_fly)
{
    if (is_flying(ch))
    {
        send_to_char("You are already flying.\n", ch);
        return;
    }

    if (flight_blocked(*ch))
    {
        send_to_char("You are unable to fly here.\n", ch);
        return;
    }

    bool magical(can_fly_magical(*ch));
    if (!magical && !can_fly_natural(*ch))
    {
        send_to_char("You possess no means of flight.\n", ch);
        return;
    }

    if (!check_handle_forced_sleep(ch))
        return;

    if (ch->position < POS_FIGHTING)
        do_stand(ch, "");

    if (ch->position < POS_FIGHTING)
    {
        send_to_char("You must be standing up to fly.\n", ch);
        return;
    }
    
    if (magical)
    {
        send_to_char("You rise off the ground and begin to fly.\n", ch);
        act("$n rises off the ground and begins to fly.", ch, NULL, NULL, TO_ROOM);
        SET_BIT(ch->affected_by, AFF_FLYING);
        return;
    }

	send_to_char("You flex your wings and begin to fly.\n", ch);
	act("$n flexes $s wings and begins to fly.", ch, NULL, NULL, TO_ROOM);
	SET_BIT(ch->affected_by, AFF_FLY_NATURAL);
}

DO_FUNC(do_land)
{
    if (!is_flying(ch))
    {
    	send_to_char("You are not flying.\n", ch);
	    return;
    }

    send_to_char("You carefully land and cease flight.\n", ch);
    act("$n carefully lands and ceases flight.", ch, NULL, NULL, TO_ROOM);
    stop_flying(*ch);
}

/* send_move_to_wizi: fixes the problem of immortals not being able to
 * see sneakers, forestwalkers, etc. - transmitt
 */
void send_move_to_wizi(CHAR_DATA *ch, char *buf)
{
    CHAR_DATA *tmp, *tmp2;
   
    for (tmp = ch->in_room->people; tmp != NULL; tmp = tmp2)
    {
        tmp2 = tmp->next_in_room;
	if (tmp->level > LEVEL_HERO) 
	{
	    act(buf, ch, NULL, tmp, TO_VICT);
	}
    }
    return;
} 

void send_move_to_ds(CHAR_DATA *ch, char *buf)
{
    CHAR_DATA *tmp;
   
    for (tmp = ch->in_room->people; tmp != NULL; tmp = tmp->next_in_room)
	if (check_detectstealth(tmp,ch)) 
	    act(buf, ch, NULL, tmp, TO_VICT);

    return;
} 
