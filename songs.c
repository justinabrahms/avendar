#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "songs.h"
#include "magic.h"
#include "tables.h"
#include "olc.h"
#include "psionics.h"
#include "spells_fire_air.h"
#include "spells_spirit.h"
#include "ShadeControl.h"
#include "Direction.h"

/** external declarations **/
DECLARE_DO_FUN( do_autoyell   	);
DECLARE_DO_FUN( do_visible 	);
DECLARE_DO_FUN( do_play		);

/** local declarations **/
AFFECT_DATA	*new_bard_song	args(( CHAR_DATA *ch, int sn, int level ));

/** local variables **/
bool	harmony = FALSE;

void do_harmonize(CHAR_DATA *ch, char *argument)
{
    int skill;

    if ((skill = get_skill(ch, gsn_harmony)) == 0)
    {
	send_to_char("You have no skill at harmonizing.\n\r", ch);
	return;
    }

    if (!ch->song)
    {
	send_to_char("You are not currently playing a primary song to harmonize with.\n\r", ch);
	return;
    }

    skill /= 2;

    skill += get_curr_stat(ch, STAT_CHR);
    skill += get_curr_stat(ch, STAT_DEX);

    if (skill < number_percent())
    {
	send_to_char("You fail to properly harmonize the new song, and your music comes to a halt.\n\r", ch);
	act("$n fails to properly harmonize a new song, and $s music comes to a halt!", ch, NULL, NULL, TO_ROOM);
	stop_playing_song(ch, NULL);
	check_improve(ch, NULL, gsn_harmony, FALSE, 2);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_harmony].beats));
	return;
    }

    check_improve(ch, NULL, gsn_harmony, TRUE, 2);

    harmony = TRUE;
    do_play(ch, argument);
    harmony = FALSE;

    return;
}
	

void do_play( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    int x, chance, sn;
    void *vo;
    char *txt;
    CHAR_DATA *victim;
    OBJ_DATA *obj, *instrument;
    int target, level;
    bool success = FALSE, found = FALSE;

    txt = one_argument( argument, arg1 );
    one_argument( txt, arg2 );

    if (IS_NPC(ch) && (IS_AFFECTED(ch, AFF_CHARM) || is_affected(ch, gsn_commandword)))
	return;

    if ((arg1[0] != '\0') && !str_cmp(arg1, "none"))
    {
	if (ch->song)
	{
	    if (ch->harmony)
	    {
		sprintf(buf, "You stop harmonizing '%s'.\n\r", skill_table[ch->harmony->type].name);
		send_to_char(buf, ch);
	    }

	    sprintf(buf, "You stop performing '%s'.\n\r", skill_table[ch->song->type].name);
	    send_to_char(buf, ch);

	    stop_playing_song(ch, NULL);
	    return;
	}
	else
	{
	    send_to_char("You are not currently performing a song.\n\r", ch);
	    return;
	}
    }

    if (IS_NPC(ch) && (ch->pIndexData->vnum == MOB_VNUM_SHUNNED_DEMON))
    {
	send_to_char("You cannot play songs in your demonic form.\n\r", ch);
	return;
    }

    if (ch->in_room && room_is_affected(ch->in_room, gsn_earthmaw))
    {
	send_to_char("The suffocating dirt and soil prevents you from playing.\n\r", ch);
	return;
    }

    int avatarType(type_of_avatar(ch));
    if (avatarType != -1 && avatarType != gsn_avatarofthelodestar)
    {
    	send_to_char("You can't play songs in your current form.\n\r", ch);
	    return;
    }

    if (is_affected(ch, gsn_astralprojection))
    {
	send_to_char("You cannot focus your thoughts properly in the astral plane.\n\r", ch);
	return;
    }

    if (IS_NAFFECTED(ch, AFF_GRAPPLE) && (number_bits(2) != 0))
    {
	send_to_char("You're too dazed to concentrate.\n\r", ch);
	WAIT_STATE(ch, PULSE_VIOLENCE);
	return;
    }

    if ( arg1[0] == '\0' )
    {
	send_to_char("What do you wish to play?\n\r", ch);
        return;
    }

    if ( ch->position < POS_RESTING )
    {
        switch( ch->position )
        {
        case POS_DEAD:
            send_to_char( "Lie still; you are DEAD.\n\r", ch );
            break;

        case POS_MORTAL:
        case POS_INCAP:
            send_to_char( "You are hurt far too badly for that.\n\r", ch );
            break;

        case POS_STUNNED:
            send_to_char( "You are too stunned to do that.\n\r", ch );
            break;

        case POS_SLEEPING:
            send_to_char( "In your dreams, or what?\n\r", ch );
            break;
	}
    }

    if (IS_NPC(ch))
    {
        sn = skill_lookup(arg1);
        if (get_skill(ch,sn) == -1)
        {
            send_to_char("You don't know any songs by that name.\n\r",ch);
            return;
        }
        else
            found = TRUE;
    }
    else
    {
        bool is_npre, check_the = str_prefix("The ", arg1);

        for ( sn = 0; sn < MAX_SKILL; sn++ )
        {
            if (skill_table[sn].name == NULL)
                break;

            if (check_the && !str_prefix("The ", skill_table[sn].name))
            {
                if (LOWER(arg1[0]) == LOWER(skill_table[sn].name[4]))
                        is_npre = str_prefix(arg1, skill_table[sn].name + 4);
                else
                    is_npre = TRUE;
            }
            else
            {
                if (LOWER(arg1[0]) == LOWER(skill_table[sn].name[0]))
                        is_npre = str_prefix(arg1, skill_table[sn].name);
                else
                    is_npre = TRUE;
            }

            if (!is_npre && (get_skill(ch, sn) > 0))
            {
                found = TRUE;
                break;
            }
        }
    }

    if (!found)
    {
        send_to_char("You do not know any songs by that name.\n\r", ch);
        return;
    }

    // Avatar of the lodestar can only play roar of the exalted
    if (avatarType == gsn_avatarofthelodestar && sn != gsn_roaroftheexalted)
    {
        send_to_char("You can manage little more music than a roar in your current form.\n", ch);
        return;
    }

    if (harmony && ch->song && sn == ch->song->type)
    {
	sprintf(buf, "You are already playing '%s'.\n\r", skill_table[sn].name);
	send_to_char(buf, ch);
	return;
    }

    victim      = NULL;
    obj         = NULL;
    vo          = NULL;
    target      = TARGET_NONE;

    switch ( skill_table[sn].target )
    {
        default:
            bug( "do_play: bad target for sn %d.", sn );
            return;

    	case TAR_IGNORE:
            argument = one_argument(argument, arg2);
            vo = (void *) argument;
            break;

        case TAR_CHAR_OFFENSIVE:
            if ( arg2[0] == '\0' )
            {
                if ((victim = ch->fighting) == NULL)
                {
                    send_to_char("Direct that song at whom?\n\r", ch);
                    return;
                }
            }
            else
            {
                if ((victim = get_char_room(ch, target_name)) == NULL)
                {
                    send_to_char("You don't see them here.\n\r", ch);
                    return;
                }
            }

            if (IS_OAFFECTED(victim, AFF_GHOST))
            {
                send_to_char("But they're a ghost!\n\r", ch);
                return;
            }

            if ( !IS_NPC(ch) )
            {
                if (is_safe_spell(ch,victim,FALSE) && victim != ch)
                    return;
                check_killer(ch,victim);
            }

            vo = (void *) victim;
            target = TARGET_CHAR;
            break;

        case TAR_CHAR_DEFENSIVE:
            if ( arg2[0] == '\0' )
                victim = ch;
            else
            {
                if ((victim = get_char_room(ch, target_name)) == NULL)
                {
                    send_to_char("You don't see them here.\n\r", ch);
                    return;
                }
            }

            vo = (void *) victim;
            target = TARGET_CHAR;
            break;

        case TAR_CHAR_SELF:
            if ((arg2[0] != '\0') && !is_name(target_name, ch->name))
            {
                send_to_char("You cannot play that song for another.\n\r", ch);
                return;
            }

            vo = (void *) ch;
            target = TARGET_CHAR;
            break;

        case TAR_OBJ_INV:
            if (arg2[0] == '\0')
            {
                send_to_char("What object do you wish to focus upon?\n\r", ch );
                return;
            }

            if ((obj = get_obj_carry(ch, target_name, ch)) == NULL)
            {
                send_to_char( "You are not carrying that.\n\r", ch );
                return;
            }

            vo = (void *) obj;
            target = TARGET_OBJ;
            break;
    }

    if (!IS_NPC(ch) && ch->mana < skill_table[sn].min_mana)
    {
	send_to_char("You lack the mental energy.\n\r", ch);
	return;
    }

    chance = get_skill(ch,sn);

    if (victim != NULL)
	if ((ch->fighting != victim) && (!IS_NPC(victim))
         && victim->fighting != ch
         && (skill_table[sn].target == TAR_CHAR_OFFENSIVE)
	 && (victim->in_room == ch->in_room)
         && victim != ch)
            if (can_see(victim, ch))
            {
                sprintf( buf, "Help!  %s is attacking me!", PERS(ch, victim) );
    		do_autoyell( victim, buf );
            }
            else
            {
                sprintf( buf, "Help! I'm being attacked!" );
                do_autoyell( victim, buf );
                do_visible(ch, "FALSE");
	    }

    if ( number_percent( ) > chance )
    {
	act("$n begins to play a song, but misses a note.", ch, NULL, NULL, TO_ROOM);

	if (ch->song)
	{
	    if (ch->harmony)
	    {
		sprintf(buf, "You stop harmonizing '%s'.\n\r", skill_table[ch->harmony->type].name);
		send_to_char(buf, ch);
	    }
		
	    sprintf(buf, "You stop performing '%s'.\n\r", skill_table[ch->song->type].name);
	    send_to_char(buf, ch);

	    stop_playing_song(ch, NULL);
	}

        send_to_char("You fail to play the song properly.\n\r", ch );
        check_improve(ch,victim,sn,FALSE,1);
	    expend_mana(ch, skill_table[sn].min_mana);
	WAIT_STATE( ch, skill_table[sn].beats );
    }
    else
    {
        if (victim && (target == TARGET_CHAR) && IS_NPC(victim)
         && (victim->pIndexData->vnum == MOB_VNUM_MIRROR_IMAGE))
        {
            act("$n turns translucent and disappears.",
		victim, NULL, NULL, TO_ROOM);
            extract_char(victim, TRUE);
            return;
        }

        if (victim != NULL && target == TARGET_CHAR && (checkPyrokineticMirrorAttacked(ch, victim) || ShadeControl::CheckShadeAttacked(*ch, *victim)))
            return;

	for (x = 0; song_table[x].song_fun; x++)
	    if (*song_table[x].sn == sn)
	    {
		if (song_table[x].inst_type != INST_TYPE_VOICE)
		{
		    instrument = get_eq_char(ch, WEAR_HOLD);

		    if (!instrument || (instrument->item_type != ITEM_INSTRUMENT))
		    {
			send_to_char("You must be holding an instrument to perform that song.\n\r", ch);
			return;
		    }

		    if (!IS_SET(song_table[x].inst_type, instrument->value[0]))
		    {
			send_to_char("You are not using the appropriate instrument type to perform that song.\n\r", ch);
			return;
		    }

		    level = ((ch->level * 3) + instrument->level) / 4;
		}
		else
		    level = ch->level;

		level += (get_curr_stat(ch, STAT_CHR) - 14) / 2;

		if (harmony)
		    level -= 5;

		success = (*song_table[x].song_fun) (sn, level, ch, vo, txt);
	    }

	if (success)
	{
	    check_improve(ch,victim,sn,TRUE,1);
	    expend_mana(ch, skill_table[sn].min_mana);
	    WAIT_STATE( ch, skill_table[sn].beats );
	}
    }

    if (skill_table[sn].target == TAR_CHAR_OFFENSIVE
     && victim != ch
     && victim->master != ch)
    {
        CHAR_DATA *vch;
        CHAR_DATA *vch_next;

        for ( vch = ch->in_room->people; vch; vch = vch_next )
        {
            vch_next = vch->next_in_room;
            if ( victim == vch && victim->fighting == NULL
                && sn != gsn_subdue)
            {   check_killer(victim,ch);
                victim->fighting = ch;
                multi_hit( victim, ch, TYPE_UNDEFINED );
                break;
            }
        }
    }
    
    return;
}

void stop_playing_song(CHAR_DATA *ch, AFFECT_DATA *song)
{
    if ((!song && ch->song) || ch->song == song)
    {
        if (is_affected(ch,gsn_cloakofshadows))
	    unhide_char(ch);
        remove_room_song(ch->in_room, ch->song);
        free_affect(ch->song);
        ch->song = NULL;
    }

    if ((!song && ch->harmony) || ch->harmony == song)
    {
        if (is_affected(ch,gsn_cloakofshadows))
	    unhide_char(ch);
        remove_room_song(ch->in_room, ch->harmony);
        free_affect(ch->harmony);
        ch->harmony = NULL;
    }
}

void stop_playing_sn(CHAR_DATA *ch, int sn)
{
    if (ch->song && ch->song->type == sn)
	stop_playing_song(ch, ch->song);
    else if (ch->harmony && ch->harmony->type == sn)
	stop_playing_song(ch, ch->harmony);
}

void start_playing_song(CHAR_DATA *ch, AFFECT_DATA *song)
{
    char buf[MAX_STRING_LENGTH];

    if (ch->harmony)
    {
	sprintf(buf, "You stop harmonizing '%s'.\n\r", skill_table[ch->harmony->type].name);
	send_to_char(buf, ch);
	stop_playing_song(ch, ch->harmony);
    }	

    if (!harmony && ch->song)
    {
	sprintf(buf, "You stop playing '%s'.\n\r", skill_table[ch->song->type].name);
	send_to_char(buf, ch);
	stop_playing_song(ch, ch->song);
    }

    if (harmony)
	ch->harmony = song;
    else
	ch->song = song;

    add_room_song(ch->in_room, song);

    return;
}

AFFECT_DATA *new_bard_song(CHAR_DATA *ch, int sn, int level)
{
    AFFECT_DATA *paf = new_affect();

    paf->level = level;
    paf->type = sn;
    paf->point = (void *) ch;

    return paf;
}

void add_room_song(ROOM_INDEX_DATA *pRoom, AFFECT_DATA *song)
{
    if (!pRoom)
	return;

    song->next = pRoom->songs;
    pRoom->songs = song;
    return;
}

void add_room_songs(ROOM_INDEX_DATA *pRoom, CHAR_DATA *ch)
{
    if (ch->harmony)
	add_room_song(pRoom, ch->harmony);

    if (ch->song)
	add_room_song(pRoom, ch->song);

    return;
}

void remove_room_song(ROOM_INDEX_DATA *pRoom, AFFECT_DATA *song)
{
    AFFECT_DATA *paf, *last = NULL;

    if (!pRoom)
	return;

    for (paf = pRoom->songs; paf; paf = paf->next)
    {
	if (paf == song)
	{
	    if (last)
		last->next = paf->next;
	    else
		pRoom->songs = paf->next;
	    return;
	}
	last = paf;
    }

    return;
}

void remove_room_songs(ROOM_INDEX_DATA *pRoom, CHAR_DATA *ch)
{
    if (ch->harmony)
	remove_room_song(pRoom, ch->harmony);

    if (ch->song)
	remove_room_song(pRoom, ch->song);

    return;
}

AFFECT_DATA *get_group_song(CHAR_DATA *ch, int sn)
{
    AFFECT_DATA *paf;

    if (!ch->in_room)
	return NULL;

    for (paf = ch->in_room->songs; paf; paf = paf->next)
	if ((paf->type == sn) && is_same_group((CHAR_DATA *) paf->point, ch))
	    return paf;

    return NULL;
}

AFFECT_DATA *get_room_song(ROOM_INDEX_DATA *pRoom, int sn)
{
    AFFECT_DATA *paf;

    /* damn cpu using sanity check garbage */
    if (!pRoom)
	return NULL;

    for (paf = pRoom->songs; paf; paf = paf->next)
	if (paf->type == sn)
	    return paf;

    return NULL;
}

SONG_FUNC( song_wallsofjericho )
{
    char buf[MAX_STRING_LENGTH];
    int dir, chance;

    if (!ch->in_room)
	return FALSE;

    act("You strike a booming note upon your instrument!", ch, NULL, NULL, TO_CHAR);
    act("$n strikes a booming note upon $p!", ch, get_eq_char(ch, WEAR_HOLD), NULL, TO_ROOM);

    for (dir = 0; dir < 6; dir++)
    {
	if (ch->in_room->exit[dir])
	{
	    if (IS_SET(ch->in_room->exit[dir]->exit_info, EX_WALLED))
	    {
		if (number_percent() <= ch->level)
		{
		    sprintf(buf, "The wall of earth %s crumbles to the ground!",
			((dir == 0) ? "to the north" : (dir == 1) ? "to the east" :
			 (dir == 2) ? "to the south" : (dir == 3) ? "to the west" :
			 (dir == 4) ? "above you" : "below you"));
		    REMOVE_BIT(ch->in_room->exit[dir]->exit_info, EX_WALLED);
		    act(buf, ch, NULL, NULL, TO_CHAR);
		    act(buf, ch, NULL, NULL, TO_ROOM);
		    if (ch->in_room->exit[dir]->u1.to_room->exit[OPPOSITE(dir)]
		     && (ch->in_room->exit[dir]->u1.to_room->exit[OPPOSITE(dir)]->u1.to_room == ch->in_room)
		     && IS_SET(ch->in_room->exit[dir]->u1.to_room->exit[OPPOSITE(dir)]->exit_info, EX_WALLED))
		    {
			sprintf(buf, "The wall of earth %s crumbles to the ground!",
			    ((dir == 0) ? "to the south" : (dir == 1) ? "to the west" :
			     (dir == 2) ? "to the north" : (dir == 3) ? "to the east" :
			     (dir == 4) ? "above you" : "below you"));
			REMOVE_BIT(ch->in_room->exit[dir]->u1.to_room->exit[OPPOSITE(dir)]->exit_info, EX_WALLED);
			act(buf, ch->in_room->exit[dir]->u1.to_room->people, NULL, NULL, TO_CHAR);
			act(buf, ch->in_room->exit[dir]->u1.to_room->people, NULL, NULL, TO_ROOM);
		    }
		}
		else
		{
		    sprintf(buf, "The wall of earth %s shakes, but does not fall.",
			((dir == 0) ? "to the north" : (dir == 1) ? "to the east" :
			 (dir == 2) ? "to the south" : (dir == 3) ? "to the west" :
			 (dir == 4) ? "above you" : "below you"));
		    act(buf, ch, NULL, NULL, TO_CHAR);
		    act(buf, ch, NULL, NULL, TO_ROOM);
		    if (ch->in_room->exit[dir]->u1.to_room->exit[OPPOSITE(dir)]
		     && (ch->in_room->exit[dir]->u1.to_room->exit[OPPOSITE(dir)]->u1.to_room == ch->in_room))
		    {
			sprintf(buf, "The wall of earth %s shakes, but does not fall.",
			    ((dir == 0) ? "to the south" : (dir == 1) ? "to the west" :
			     (dir == 2) ? "to the north" : (dir == 3) ? "to the east" :
			     (dir == 4) ? "above you" : "below you"));
			act(buf, ch->in_room->exit[dir]->u1.to_room->people, NULL, NULL, TO_CHAR);
			act(buf, ch->in_room->exit[dir]->u1.to_room->people, NULL, NULL, TO_ROOM);
		    }
		}
	    }

	    if (IS_SET(ch->in_room->exit[dir]->exit_info, EX_CLOSED)
             && !IS_SET(ch->in_room->exit[dir]->exit_info, EX_PICKPROOF)
	     && !IS_SET(ch->in_room->exit[dir]->exit_info, EX_NORAM))
	    {
		chance = ch->level * 2;
		
		if (IS_SET(ch->in_room->exit[dir]->exit_info, EX_RUNEOFEARTH))
		    chance /= 3;

		if (number_percent() < chance)
		{
		    sprintf(buf, "The door %s flies open!",
			((dir == 0) ? "to the north" : (dir == 1) ? "to the east" :
			 (dir == 2) ? "to the south" : (dir == 3) ? "to the west" :
			 (dir == 4) ? "above you" : "below you"));
		    REMOVE_BIT(ch->in_room->exit[dir]->exit_info, EX_CLOSED);
		    REMOVE_BIT(ch->in_room->exit[dir]->exit_info, EX_RUNEOFEARTH);
		    act(buf, ch, NULL, NULL, TO_CHAR);
		    act(buf, ch, NULL, NULL, TO_ROOM);
		    if (ch->in_room->exit[dir]->u1.to_room->exit[OPPOSITE(dir)]
		     && (ch->in_room->exit[dir]->u1.to_room->exit[OPPOSITE(dir)]->u1.to_room == ch->in_room))
		    {
			sprintf(buf, "The door %s flies open!",
			    ((dir == 0) ? "to the south" : (dir == 1) ? "to the west" :
			     (dir == 2) ? "to the north" : (dir == 3) ? "to the east" :
			     (dir == 4) ? "above you" : "below you"));
			REMOVE_BIT(ch->in_room->exit[dir]->u1.to_room->exit[OPPOSITE(dir)]->exit_info, EX_CLOSED);
			REMOVE_BIT(ch->in_room->exit[dir]->u1.to_room->exit[OPPOSITE(dir)]->exit_info, EX_RUNEOFEARTH);
			act(buf, ch->in_room->exit[dir]->u1.to_room->people, NULL, NULL, TO_CHAR);
			act(buf, ch->in_room->exit[dir]->u1.to_room->people, NULL, NULL, TO_ROOM);
		    }
		}
		else
		{
		    sprintf(buf, "The door %s shakes violently, but does not open.",
			((dir == 0) ? "to the north" : (dir == 1) ? "to the east" :
			 (dir == 2) ? "to the south" : (dir == 3) ? "to the west" :
			 (dir == 4) ? "above you" : "below you"));
		    act(buf, ch, NULL, NULL, TO_CHAR);
		    act(buf, ch, NULL, NULL, TO_ROOM);
		    if (ch->in_room->exit[dir]->u1.to_room->exit[OPPOSITE(dir)]
		     && (ch->in_room->exit[dir]->u1.to_room->exit[OPPOSITE(dir)]->u1.to_room == ch->in_room))
		    {
			sprintf(buf, "The door %s shakes violently, but does not open.",
			    ((dir == 0) ? "to the south" : (dir == 1) ? "to the west" :
			     (dir == 2) ? "to the north" : (dir == 3) ? "to the east" :
			     (dir == 4) ? "above you" : "below you"));
			act(buf, ch->in_room->exit[dir]->u1.to_room->people, NULL, NULL, TO_CHAR);
			act(buf, ch->in_room->exit[dir]->u1.to_room->people, NULL, NULL, TO_ROOM);
		    }
		}
	    }
	}
    }

    return TRUE;
}


SONG_FUNC( song_noteofshattering )
{
    CHAR_DATA *victim = NULL, *vch, *vch_next = NULL;
    OBJ_DATA *obj, *obj_next;
    int chance;
  
    if (!ch->in_room)
	return FALSE;

    if ((txt[0] != '\0') && ((victim = get_char_room(ch, txt)) == NULL))
    {
	send_to_char("You don't see them here.\n\r", ch);
	return FALSE;
    }

    act("You strike a sharp note upon your instrument!", ch, NULL, NULL, TO_CHAR);
    act("$n strikes a sharp note upon $p!", ch, get_eq_char(ch, WEAR_HOLD), NULL, TO_ROOM);

    if (victim)
    {
	if (is_safe_spell(ch, victim, FALSE))
	    return TRUE;

	act("You begin to shake violently as the note vibrates around you!", victim, NULL, NULL, TO_CHAR);
	act("$n begins to shake violently as the note vibrates around $m!", victim, NULL, NULL, TO_ROOM);

	vch = victim;
    }
    else
    {
	vch = ch->in_room->people;
	act("The room is filled with a violent vibration!", ch, NULL, NULL, TO_CHAR);
	act("The room is filled with a violent vibration!", ch, NULL, NULL, TO_ROOM);
    }

    while (vch)
    {
	if (!victim)
	{
	    vch_next = vch->next_in_room;

	    if ((vch == ch) || is_safe_spell(ch, vch, TRUE))
	    {
		vch = vch_next;
		continue;
	    }
	}

	for (obj = vch->carrying; obj; obj = obj_next)
	{
	    obj_next = obj->next_content;

	    if (IS_OBJ_STAT(obj, ITEM_NODESTROY)
	     ||(str_cmp(material_table[obj->material].name, "glass")
	     && str_cmp(material_table[obj->material].name, "crystal")))
		continue;

	    chance = obj->level;

	    if (IS_OBJ_STAT(obj, ITEM_MAGIC))
		chance *= 3;
	    else
		chance *= 2;

	    chance -= ch->level;

	    chance = URANGE(5, chance, 95);

	    if (number_percent() > chance)
	    {
		act("$p upon you shakes violently and shatters!", vch, obj, NULL, TO_CHAR);
	        act("$p on $n shakes violently and shatters!", vch, obj, NULL, TO_ROOM);

		extract_obj(obj);
	    }
	}

	if (!is_same_group(ch, vch))
	    damage(ch, vch, 1, gsn_noteofshattering, DAM_SOUND, FALSE);

	if (!victim)
	    vch = vch_next;
	else
	    vch = NULL;
    }

    return TRUE;
}

SONG_FUNC( song_eleventhhour )
{
    AFFECT_DATA *paf = new_affect();
    CHAR_DATA *vch;

    paf->type	= gsn_eleventhhour;
    paf->point	= (void *) ch;

    act("You begin playing a powerful war march, spurring your companions on!", ch, NULL, NULL, TO_CHAR);
   
    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
    {
	if (vch == ch)
	    continue;
	
	if (is_same_group(ch, vch))
            act("$N begins playing a powerful war march, spurring you on to greatness!", vch, NULL, ch, TO_CHAR);
	else
	    act("$N begins playing a powerful war march!", vch, NULL, ch, TO_CHAR);
    }

    start_playing_song(ch, paf);

    return TRUE;
}


SONG_FUNC( song_soundbubble )
{
    AFFECT_DATA *paf = new_affect();

    paf->type	= gsn_soundbubble;
    paf->point  = (void *) ch;

    act("A shimmer of musical energy surrounds the area as you begin to strum a sombre tune.", ch, NULL, NULL, TO_CHAR);
    act("A shimmer of musical energy surrounds the area as $n begins to strum a sombre tune.", ch, NULL, NULL, TO_ROOM);

    start_playing_song(ch, paf);

    return TRUE;
}

SONG_FUNC( song_cloakofshadows )
{
    AFFECT_DATA *paf;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (IS_AFFECTED(ch, AFF_FAERIE_FIRE))
    {
	send_to_char("You can't hide while glowing.\n\r", ch);
	return FALSE;
    }

    if (ch->in_room && room_is_affected(ch->in_room, gsn_etherealblaze))
    {
	send_to_char("The flames around you prevent you from hiding.\n\r", ch);
	return FALSE;
    }

    if (IS_OAFFECTED(ch, AFF_BLEEDING))
    {
	send_to_char("You are trailing too much blood to hide.\n\r", ch);
	return FALSE;
    }

    if (is_affected(ch, gsn_ringoffire))
    {
	send_to_char("You cannot hide while surrounded by a ring of flames.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = gsn_cloakofshadows;
    af.level	 = ch->level;
    af.duration  = -1;
    af.modifier  = 0;
    af.location  = APPLY_HIDE;
    af.bitvector = AFF_HIDE;
    affect_to_char(ch, &af);

    paf = new_affect();

    paf->type   = gsn_cloakofshadows;
    paf->point	= (void *) ch;

    act("$n disappears into the shadows, amidst a haunting dirge.", ch, NULL, NULL, TO_ROOM);
    act("You disappear into the shadows, leaving only a haunting dirge in your wake.", ch, NULL, NULL, TO_CHAR);

    start_playing_song(ch, paf);
 
    return TRUE;
}


SONG_FUNC( song_marchofwar )
{
    AFFECT_DATA *paf = new_bard_song(ch, gsn_marchofwar, level); 
    CHAR_DATA *vch;

    act("You begin playing a rapid song of battle, throwing combatants into a fighting frenzy!", ch, NULL, NULL, TO_CHAR);
    act("$n begins playing a rapid song of battle!", ch, NULL, NULL, TO_ROOM);
    
    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
    {
	if (!vch->fighting || (vch == ch) || is_safe_spell(ch, vch, TRUE))
	    continue;

	send_to_char("You are thrown into a fighting frenzy!\n\r", vch);
    }

    paf->modifier = (55 + (level / 2)) * get_skill(ch, gsn_marchofwar);

    start_playing_song(ch, paf);

    return TRUE;
}

SONG_FUNC( song_sonicwave )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *victim = NULL, *vch = NULL, *vch_next = NULL;
    OBJ_DATA *instrument;
    char buf[MAX_STRING_LENGTH];
    int dam;
    bool grouped;

    if (!ch->in_room)
	return FALSE;

    if ((txt[0] != '\0') && ((victim = get_char_room(ch, txt)) == NULL))
    {
	send_to_char("You don't see them here.\n\r", ch);
	return FALSE;
    }

    instrument = get_eq_char(ch, WEAR_HOLD);

    act("You strike a powerful note upon $p!", ch, instrument, NULL, TO_CHAR);
    act("$n strikes a powerful note upon $p!", ch, instrument, NULL, TO_ROOM);

    if (victim)
    {
	if (is_safe_spell(ch, victim, FALSE))
	    return TRUE;

	if (!can_be_affected(victim,gsn_sonicwave))
	{
	    act("$N is immune to your music.",ch,NULL,victim,TO_CHAR);
	    return FALSE;
	}
	act("A powerful wave of sonic energy washes over you!", victim, NULL, NULL, TO_CHAR);
	act("A sonic boom erupts from around $n!", victim, NULL, NULL, TO_ROOM);

	vch = victim;
    }
    else
    {
	vch = ch->in_room->people;
	act("The room erupts in a torrent of sonic energy!", ch, NULL, NULL, TO_CHAR);
	act("The room erupts in a torrent of sonic energy!", ch, NULL, NULL, TO_ROOM);
    }

    while (vch)
    {
	if (!victim)
	{
	    vch_next = vch->next_in_room;

	    if ((vch == ch) || is_safe_spell(ch, vch, TRUE) 
	      || !can_be_affected(vch,gsn_sonicwave))
	    {
		vch = vch_next;
		continue;
	    }
	}

        if ((ch->fighting != vch) && (vch->fighting != ch) && !IS_NPC(vch))
	{
	    if (can_see(vch, ch))
	    {
		sprintf(buf, "Help!  %s is attacking me!", PERS(ch, vch));
		do_autoyell(vch, buf);
	    }
	    else
		do_autoyell(vch, "Help!  Someone is attacking me!");
	}

	dam = 50 + dice(level, 3);
	
	if ((grouped = is_same_group(ch, vch)) == TRUE)
	    dam /= 2;

	if (saves_spell(level, ch, vch, DAM_SOUND))
	    damage(ch, vch, dam / 2, gsn_sonicwave, DAM_SOUND, TRUE);
	else
	{
	    damage(ch, vch, dam, gsn_sonicwave, DAM_SOUND, TRUE);

	    if (!grouped && !IS_OAFFECTED(vch, AFF_DEAFEN)
	     && (!IS_NPC(vch) || !IS_SET(vch->act, ACT_NOSUBDUE))
 	     && (number_bits(2) != 0))
	    {
		send_to_char("You are defeaned by the power of the sonic wave!\n\r", vch);
		act("$n appears to be deafened.", vch, NULL, NULL, TO_ROOM);

		af.where     = TO_OAFFECTS;
		af.type      = gsn_sonicwave;
		af.level     = ch->level;
		af.duration  = number_bits(2) + 1;
		af.location  = APPLY_NONE;
		af.modifier  = 0;
		af.bitvector = AFF_DEAFEN;
		affect_to_char(vch, &af);
	    }
	}

	if (!victim)
	    vch = vch_next;
	else
	    vch = NULL;

    }

    return TRUE;
}

SONG_FUNC( song_auraoflight )
{
    AFFECT_DATA *paf = new_bard_song(ch, gsn_auraoflight, level);
    
    act("An aura of light begins to cascade around you.", ch, NULL, NULL, TO_CHAR);
    act("An aura of light begins to cascade around $n as $e begins a symphonic hymn.", ch, NULL, NULL, TO_ROOM);

/*  Did I do it like this for a reason?
    stop_playing_song(ch);
    ch->song = paf;
*/

    start_playing_song(ch, paf);

    return TRUE;
}

SONG_FUNC( song_manasong )
{
    AFFECT_DATA *paf = new_bard_song(ch, gsn_manasong, level);
    OBJ_DATA *instrument = get_eq_char(ch, WEAR_HOLD);

    act("You begin playing the mystical chords of ancient times.", ch, NULL, NULL, TO_CHAR);

    if (instrument)
        act("Your skin tingles as $n begins playing mystical chords upon $p, bathing the area in a field of energy.", ch, instrument, NULL, TO_ROOM);
    else
	act("Your skin tingles as $n begins playing an ancient tune.", ch, NULL, NULL, TO_ROOM);

    start_playing_song(ch, paf);

    return TRUE;
}


SONG_FUNC( song_psalmofhealing )
{
    AFFECT_DATA *paf = new_bard_song(ch, gsn_psalmofhealing, level);

    act("You begin singing a quiet psalm of healing.", ch, NULL, NULL, TO_CHAR);
    act("$n begins singing a quiet psalm of healing.", ch, NULL, NULL, TO_ROOM);

    start_playing_song(ch, paf);

    return TRUE;
}

SONG_FUNC( song_serenadeoflife )
{
    AFFECT_DATA *paf = new_bard_song(ch, gsn_serenadeoflife, level);

    act("You begin playing a harmonious serenade of life.", ch, NULL, NULL, TO_CHAR);
    act("$n begins playing a harmonious serenade of life.", ch, NULL, NULL, TO_ROOM);

    start_playing_song(ch, paf);

    return TRUE;
}

SONG_FUNC( song_disruption )
{
    CHAR_DATA *victim = NULL, *vch, *vch_next = NULL;
    OBJ_DATA *instrument;
    int chance, maxslots, x;
    bool disruption;
  
    if (!ch->in_room)
	return FALSE;

    if ((txt[0] != '\0') && ((victim = get_char_room(ch, txt)) == NULL))
    {
	send_to_char("You don't see them here.\n\r", ch);
	return FALSE;
    }

    instrument = get_eq_char(ch, WEAR_HOLD);

    act("You play a disrupting chord upon $p!", ch, instrument, NULL, TO_CHAR);
    act("$n plays a disrupting chord upon $p!", ch, instrument, NULL, TO_ROOM);

    if (victim)
    {
	if (is_safe_spell(ch, victim, FALSE))
	    return TRUE;

	act("You begin to shake violently as the note vibrates around you!", victim, NULL, NULL, TO_CHAR);
	act("$n begins to shake violently as the note vibrates around $m!", victim, NULL, NULL, TO_ROOM);

	vch = victim;
    }
    else
    {
	vch = ch->in_room->people;
	act("The room is filled with a violent vibration!", ch, NULL, NULL, TO_CHAR);
	act("The room is filled with a violent vibration!", ch, NULL, NULL, TO_ROOM);
    }

    while (vch)
    {
	if (!victim)
	{
	    vch_next = vch->next_in_room;

	    if (IS_NPC(vch) || (vch == ch) || is_safe_spell(ch, vch, TRUE))
	    {
		vch = vch_next;
		continue;
	    }
	}

// brazen: Only PCs have focus slots, so this song simply doesn't work on mobs.
// Fixed to not work on mobs; if mobs gain focus slots at some point, then this
// should be allowed. Ticket #160, and thanks to ninjadyne for the diagnosis and fix
	
	if (!IS_NPC(vch))
	{
	    maxslots = focus_slots(vch);
    	    chance = 50 + (ch->level * 2) - (vch->level * 2);
	    disruption = FALSE;

	    for (x = 0; x < maxslots; x++)
  	    {
	        if ((vch->pcdata->focus_sn[x] > 0) && vch->pcdata->focus_on[x] && (number_percent() <= chance))
	        {
		    if (!disruption)
		    {
		        act("You clutch your head in pain, as the disrupting chord echoes in your mind!", vch, NULL, NULL, TO_CHAR);
		        act("$n clutches $s head in pain, as the disrupting chord echoes in $s mind!", vch, NULL, NULL, TO_ROOM);
		        disruption = TRUE;
		    }   

		    unfocus(vch, x, TRUE);
	        }    
 	    }
	}

	if (!is_same_group(ch, vch))
	    damage(ch, vch, 1, gsn_noteofshattering, DAM_SOUND, FALSE);

	if (!victim)
	    vch = vch_next;
	else
	    vch = NULL;
    }

    return TRUE;
}
    

SONG_FUNC( song_invokesympathy )
{
    AFFECT_DATA *paf = new_bard_song(ch, gsn_invokesympathy, level);

    act("You begin a song of woe, seeking the sympathy of those around you.", ch, NULL, NULL, TO_CHAR);
    act("$n begins to sing a song of woe, seeking the sympathy of those around $m.", ch, NULL, NULL, TO_ROOM);
/*
    stop_playing_song(ch);
    ch->song = paf;
*/

    start_playing_song(ch, paf);

    return TRUE;
}

SONG_FUNC( song_marchingtune )
{
    AFFECT_DATA *paf = new_bard_song(ch, gsn_marchingtune, level);

    act("You begin to sing a light-hearted tune, easing the burden of travel.", ch, NULL, NULL, TO_CHAR);
    act("$n begins to sing a light-hearted tune, easing the burden of travel.", ch, NULL, NULL, TO_ROOM);

    start_playing_song(ch, paf);

    return TRUE;
}

SONG_FUNC( song_songofsoothing )
{
    AFFECT_DATA *paf = new_bard_song(ch, gsn_songofsoothing, level);

    act("You begin playing a soothing tune, calming those around you.", ch, NULL, NULL, TO_CHAR);
    act("$n begins playing a soothing tune, calming those around $m.", ch, NULL, NULL, TO_ROOM);

    start_playing_song(ch, paf);

    return TRUE;
}

SONG_FUNC( song_stuttering )
{
    AFFECT_DATA *paf = new_bard_song(ch, gsn_stuttering, level);

    act("You break into a silly, lively tune about a stuttering sage.", ch, NULL, NULL, TO_CHAR);
    act("$n breaks into a silly, lively tune about a stuttering sage.", ch, NULL, NULL, TO_ROOM);

    start_playing_song(ch, paf);

    return TRUE;
}

SONG_FUNC( song_aegisofmusic )
{
    AFFECT_DATA *paf = new_bard_song(ch, gsn_aegisofmusic, level);
    CHAR_DATA *vch;

    act("The air around you shimmers as you begin to form an aegis of music.", ch, NULL, NULL, TO_CHAR);
    act("The air around $n begins to shimmer as $e forms an aegis of music.", ch, NULL, NULL, TO_ROOM);

    for (vch = ch->in_room->people; vch; vch = vch->next);
    {
	if ((vch != ch) && is_same_group(ch, vch))
	    send_to_char("The air around you begins to shimmer.\n\r", vch);
    }

    start_playing_song(ch, paf);

    return TRUE;
}

SONG_FUNC( song_noteofstriking )
{
    CHAR_DATA *victim = NULL, *vch, *vch_next = NULL;
  
    if (!ch->in_room)
	return FALSE;

    if ((txt[0] != '\0') && ((victim = get_char_room(ch, txt)) == NULL))
    {
	send_to_char("You don't see them here.\n\r", ch);
	return FALSE;
    }

    if (victim && !can_be_affected(victim,gsn_noteofstriking))
    {
	act("$N is immune to your music.",ch,NULL,victim,TO_CHAR);
	return FALSE;
    }

    act("You strike a loud note upon your instrument!", ch, NULL, NULL, TO_CHAR);
    act("$n strikes a loud note upon $p!", ch, get_eq_char(ch, WEAR_HOLD), NULL, TO_ROOM);

    if (victim)
    {
	if (is_safe_spell(ch, victim, FALSE))
	    return TRUE;

	if (!can_be_affected(victim,gsn_noteofstriking))
	{
	    act("$N is immune to your music.",ch,NULL,victim,TO_CHAR);
	    return FALSE;
	}
	act("You are knocked back as the power of the note strikes you!", victim, NULL, NULL, TO_CHAR);
	act("$n is knocked back as the power of the note strikes $m!", victim, NULL, NULL, TO_ROOM);

	vch = victim;
    }
    else
    {
	vch = ch->in_room->people;
	act("The room is filled with a violent vibration!", ch, NULL, NULL, TO_CHAR);
	act("The room is filled with a violent vibration!", ch, NULL, NULL, TO_ROOM);
    }

    while (vch)
    {
	if (!victim)
	{
	    vch_next = vch->next_in_room;

	    if ((vch == ch) || is_safe_spell(ch, vch, TRUE)
	      || !can_be_affected(vch,gsn_noteofstriking))
	    {
		vch = vch_next;
		continue;
	    }
	}

	if (!saves_spell(ch->level, ch, vch, DAM_SOUND))
	{
	    act("You are stunned!", vch, NULL, NULL, TO_CHAR);
	    act("$n is stunned by the force!", vch, NULL, NULL, TO_ROOM);
	    WAIT_STATE(vch, UMAX(vch->wait, (2 * PULSE_VIOLENCE) - 2 + number_bits(2)));
	}

	if (!is_same_group(ch, vch))
	    damage(ch, vch, 1, gsn_noteofshattering, DAM_SOUND, FALSE);

	if (!victim)
	    vch = vch_next;
	else
	    vch = NULL;
    }

    return TRUE;
}

SONG_FUNC( song_echoesoffear )
{
    AFFECT_DATA *paf = new_bard_song(ch, gsn_echoesoffear, level);
    CHAR_DATA *vch;

    act("You begin playing a haunting tune, instilling fear in your foes.", ch, NULL, NULL, TO_CHAR);
    act("$n starts to play a haunting tune.", ch, NULL, NULL, TO_ROOM);

    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
	if (!is_same_group(vch, ch))
	    send_to_char("You begin to glance around fearfully.\n\r", vch);
 
    start_playing_song(ch, paf);

    return TRUE;
}

SONG_FUNC( song_discord )
{
    CHAR_DATA *vch, *vch_next;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (!ch->in_room)
	return FALSE;

    act("You play a quick series of disjunct chords, sending a jarring wave of energy through the room.", ch, NULL, NULL, TO_CHAR);
    act("$n plays a quick series of disjunct chords, and a jarring wave of energy courses through the area.", ch, NULL, NULL, TO_ROOM);

    for (vch = ch->in_room->people; vch; vch = vch_next)
    {
	vch_next = vch->next_in_room;

	if (vch == ch || !can_be_affected(vch,gsn_discord))
	    continue;

	if (!vch->master && !IS_NPC(vch)
         && ((is_same_group(ch, vch) && (number_bits(6) == 0))
	 || (!is_same_group(ch, vch) && !saves_spell(ch->level + 3, ch, vch, DAM_SOUND))))
	{
	    act("You are filled with an intense feeling of distrust, and your bonds of unity fade.", vch, NULL, NULL, TO_CHAR);
	    act("$n glances around suspiciously, and withdraws slightly.", vch, NULL, NULL, TO_ROOM);
	    stop_follower(vch);

	    af.where	 = TO_AFFECTS;
	    af.type	 = gsn_discord;
	    af.level	 = ch->level;
	    af.duration  = 1;
	    af.location  = APPLY_NONE;
	    af.modifier  = 0;
	    af.bitvector = 0;
	    affect_to_char(ch, &af);
	}
    }

    return TRUE;

}

SONG_FUNC(song_roaroftheexalted)
{
    // Sanity-check
    if (ch->in_room == NULL)
	    return false;

    act("You roar exultantly, heralding the triumph of the celestial hosts!", ch, NULL, NULL, TO_CHAR);
    act("$n roars exultantly, heralding the triumph of the celestial hosts!", ch, NULL, NULL, TO_ROOM);

    // Iterate everyone in the room as possible targets
    CHAR_DATA * victim_next;
    for (CHAR_DATA * victim(ch->in_room->people); victim != NULL; victim = victim_next)
    {
        victim_next = victim->next_in_room;

        // Demons are always affected; non-evils, undead, and nosubdue folk are never affected (unless demons)
        // Note that groupmates _can_ be affected
        bool isDemon(is_demon(victim));
        if (!isDemon && (aura_grade(victim) <= 0 
        || (IS_NPC(victim) && (IS_SET(victim->act, ACT_UNDEAD) || IS_SET(victim->act, ACT_NOSUBDUE)))))
            continue;

        // Check for some basic sanity things
        if (is_safe_spell(ch, victim, true) || !can_be_affected(victim, gsn_roaroftheexalted))
            continue;

        // Check for a save; demons have a much harder time saving
        if (saves_spell(ch->level + (isDemon ? 5 : 0), ch, victim, DAM_FEAR))
            continue;

        act("Your heart palpitates as fear-inspired panic takes over!", victim, NULL, NULL, TO_CHAR);
        act("$n cowers away from the holy roar, fear clouding $s face!", victim, NULL, NULL, TO_ROOM); 

        // Check whether the victim already has the effect
        AFFECT_DATA * paf(get_affect(victim, gsn_roaroftheexalted));
        if (paf == NULL)
        {
            // Apply the effect to the victim
            AFFECT_DATA af = {0};
            af.where    = TO_AFFECTS;
            af.type     = gsn_roaroftheexalted;
            af.duration = 3;
            af.modifier = (isDemon ? 2 : 1);
            affect_to_char(victim, &af);
        }
        else
        {
            // Reset the duration and upgrade the modifier, but only to a point
            paf->duration = 3;
            paf->modifier = UMIN(paf->modifier + (isDemon ? 2 : 1), (isDemon ? 20 : 10));
        }

        // Check for fleeing
        if (number_percent() <= (isDemon ? 10 : 5))
        {
            ++victim->lost_att;
            send_to_char("Raw terror takes over, and you turn and try to run!\n", victim);
            do_flee(victim, "");
        }
    }

    return true;
}

void expand_rystaias_light(ROOM_INDEX_DATA & startRoom, int level)
{
    static const unsigned int MaxRadius(5);

    // Initialize
    std::vector<ROOM_INDEX_DATA *> roomSet1;
    std::vector<ROOM_INDEX_DATA *> roomSet2;
    std::vector<ROOM_INDEX_DATA *> * currRooms(&roomSet1);
    std::vector<ROOM_INDEX_DATA *> * nextRooms(&roomSet2);
    currRooms->push_back(&startRoom);
    time_t startTime(current_time);
    int duration(level / 7);

    // Loop until the radius is reached or there are no more rooms to process
    for (unsigned int radius(0); radius < MaxRadius && !currRooms->empty(); ++radius)
    {
        // Iterate the set of current rooms
        for (size_t i(0); i < currRooms->size(); ++i)
        {
            ROOM_INDEX_DATA * room((*currRooms)[i]);

            // Check whether this room has the light
            AFFECT_DATA * paf(get_room_affect(room, gsn_canticleofthelightbringer));
            if (paf == NULL)
            {
                // Apply the light
                AFFECT_DATA af = {0};
                af.where    = TO_ROOM;
                af.type     = gsn_canticleofthelightbringer;
                af.level    = level;
                af.duration = duration;
                af.modifier = startTime;
                affect_to_room(room, &af);

                if (room->people != NULL)
                    act("A brilliant golden light fills the air, dispersing all shadows!", room->people, NULL, NULL, TO_ALL);

                continue;
            }
            
            // Light already present, check whether this room has already been traversed in this iteration
            if (paf->modifier == startTime)
                continue;

            // Haven't traversed this room, so update the effect
            paf->level = UMAX(paf->level, level);
            paf->duration = UMAX(paf->duration, duration);
            paf->modifier = startTime;
            
            if (room->people != NULL)
                act("The golden light here pulses brightly as a wave of radiant energy passes through.", room->people, NULL, NULL, TO_ALL);

            // Build a list of adjacent rooms
            for (unsigned int j(0); j < Direction::Max; ++j)
            {
                ROOM_INDEX_DATA * nextRoom(Direction::Adjacent(*room, static_cast<Direction::Value>(j)));
                if (nextRoom != NULL)
                    nextRooms->push_back(nextRoom);
            }
        }

        // Reset for the next iteration
        std::swap(currRooms, nextRooms);
        nextRooms->clear();
    }
}

SONG_FUNC(song_canticleofthelightbringer)
{
    act("You begin singing a joyous hymn of praise.", ch, NULL, NULL, TO_CHAR);
    act("$n begins singing a joyous hymn of praise.", ch, NULL, NULL, TO_ROOM);
    start_playing_song(ch, new_bard_song(ch, gsn_canticleofthelightbringer, level));
    return true;
}

SONG_FUNC(song_requiemofthemartyr)
{
    act("You begin playing a soft, sad melody, the gentle tune calling to mind the sacrifice of those who have gone before.", ch, NULL, NULL, TO_CHAR);
    act("$n begins playing a soft, sad melody, the gentle tune calling to mind the sacrifice of those who have gone before.", ch, NULL, NULL, TO_ROOM);
    start_playing_song(ch, new_bard_song(ch, gsn_requiemofthemartyr, level));
    return true;
}

AFFECT_DATA * get_char_song(const CHAR_DATA & ch, int sn)
{
    if (ch.song != NULL && ch.song->type == sn) return ch.song;
    if (ch.harmony != NULL && ch.harmony->type == sn) return ch.harmony;
    return NULL;
}

int gravebeat_total_levels(const CHAR_DATA & ch)
{
    if (ch.in_room == NULL || get_char_song(ch, gsn_gravebeat) == NULL) 
        return 0;

    int result(0);
    for (const CHAR_DATA * undead(ch.in_room->people); undead != NULL; undead = undead->next_in_room)
    {
        AFFECT_DATA * paf(get_affect(undead, gsn_gravebeat));
        if (paf != NULL && paf->modifier == ch.id)
            result += undead->level;
    }

    return result;
}

static int gravebeat_level(int level)
{
    int result(level * level);
    return UMAX(result, 200);
}

void check_gravebeat_room(CHAR_DATA & ch)
{
    // Ensure there is a room and that ch is playing gravebeat
    AFFECT_DATA * song(get_char_song(ch, gsn_gravebeat));
    if (song == NULL || ch.in_room == NULL)
        return;

    // Determine how many levels of control ch may have
    int currCHR(get_curr_stat(&ch, STAT_CHR));
    int maxLevels(ch.level * ch.level + (currCHR * currCHR * 2) + 800);

    // Check for undead in the room
    std::vector<std::pair<CHAR_DATA*, void*> > candidates;
    int totalLevels(0);
    for (CHAR_DATA * undead(ch.in_room->people); undead != NULL; undead = undead->next_in_room)
    {
        // Skip certain chars
        if (!IS_NPC(undead) || !IS_SET(undead->act, ACT_UNDEAD) || undead->level >= ch.level 
        || IS_SET(undead->act, ACT_NOSUBDUE) || IS_SET(undead->act, ACT_SENTINEL))
            continue;

        // Determine effective gravebeat level
        int gravebeatLevel(gravebeat_level(undead->level));

        // Check whether already enthralled
        AFFECT_DATA * paf(get_affect(undead, gsn_gravebeat));
        if (paf != NULL && paf->modifier != 0)
        {
            // Already enthralled by someone; check whether enthralled by ch
            if (paf->modifier != ch.id)
                continue;
            
            // Enthralled by ch; add to the total levels so long as it won't cause an overage
            if (totalLevels + gravebeatLevel <= maxLevels)
            {
                totalLevels += gravebeatLevel;
                continue;
            }

            // This undead should no longer be under the thrall of ch
            act("$n's shamblings become more erratic, no longer in tune to the beat.", undead, NULL, NULL, TO_ROOM);
            paf->modifier = 0; 
            continue;
        }

        // Check whether there are enough levels for this undead
        if (totalLevels + gravebeatLevel <= maxLevels)
            candidates.push_back(std::make_pair(undead, (paf == NULL ? undead->in_room : paf->point)));
    }

    // Now enthrall all undead which can fit in the level gap
    for (unsigned int i(0); i < candidates.size(); ++i)
    {
        int gravebeatLevel(gravebeat_level(candidates[i].first->level));
        if (totalLevels + gravebeatLevel > maxLevels)
            continue;

        // Echo, and begin enthrallment
        act("$n begins shuffling steadily, lurching and shambling to the beat.", candidates[i].first, NULL, NULL, TO_ROOM);
        totalLevels += gravebeatLevel;

        affect_strip(candidates[i].first, gsn_gravebeat);
        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.type     = gsn_gravebeat;
        af.level    = ch.level;
        af.duration = -1;
        af.modifier = ch.id;
        af.point    = candidates[i].second;
        affect_to_char(candidates[i].first, &af);
    }
}

SONG_FUNC(song_gravebeat)
{
    act("You begin beating out a slow, steady rhythm, the sound low and foreboding.", ch, NULL, NULL, TO_CHAR);
    act("$n begins beating out a slow, steady rhythm, the sound low and foreboding.", ch, NULL, NULL, TO_ROOM);
    start_playing_song(ch, new_bard_song(ch, gsn_gravebeat, level));
    check_gravebeat_room(*ch);
    return true;
}

void check_embraceofthedeeps_cauterize(CHAR_DATA & ch, int sn)
{
    // Check for the song
    AFFECT_DATA * song(get_group_song(&ch, gsn_theembraceofthedeeps));
    if (song == NULL)
        return;

    // Check for basic qualifiers
    if (!can_be_affected(&ch, gsn_theembraceofthedeeps) || is_an_avatar(&ch))
        return;

    // Close the wounds
    act("A thick sludge forms over your bleeding wounds, knitting them together!", &ch, NULL, NULL, TO_CHAR);
    act("A thick sludge forms over $n's bleeding wounds, knitting them together!", &ch, NULL, NULL, TO_ROOM);
    affect_strip(&ch, sn);
}

SONG_FUNC(song_theembraceofthedeeps)
{
    act("You begin singing a deep, sonorous ballad.", ch, NULL, NULL, TO_CHAR);
    act("$n begins singing a deep, sonorous ballad.", ch, NULL, NULL, TO_ROOM);
    start_playing_song(ch, new_bard_song(ch, gsn_theembraceofthedeeps, level));
    return true;
}

void do_theilalslastsailing(CHAR_DATA & ch)
{
    // Calculate base damage
    int baseDam(5 + (ch.level / 2) + get_curr_stat(&ch, STAT_CHR));

    // Iterate the people in the room
    CHAR_DATA * vch_next;
    for (CHAR_DATA * vch(ch.in_room->people); vch != NULL; vch = vch_next)
    {
        // Ignore those in the same group, or who are safe from the singer
        vch_next = vch->next_in_room;
        if (is_same_group(&ch, vch) || is_safe_spell(&ch, vch, true))
            continue;

        // Hit the rest with drowning damage
        damage_old(&ch, vch, number_range(baseDam, baseDam * 3) / 2, gsn_theilalslastsailing, DAM_DROWNING, true);
    }
}

SONG_FUNC(song_theilalslastsailing)
{
    act("You begin singing a gurgling, throaty chanty.", ch, NULL, NULL, TO_CHAR);
    act("$n begins singing a gurgling, throaty chanty.", ch, NULL, NULL, TO_ROOM);
    start_playing_song(ch, new_bard_song(ch, gsn_theilalslastsailing, level));
    return true;
}

SONG_FUNC( song_hymntotourach )
{
    CHAR_DATA *victim = NULL, *vch, *vch_next;

    if (!ch->in_room || IS_NPC(ch))
	return FALSE;

    if (!str_cmp(ch->name, "Aeolis"))
    {
	if ((txt[0] != '\0') && ((victim = get_char_room(ch, txt)) == NULL))
	{
	    send_to_char("You don't see them here.\n\r", ch);
	    return FALSE;
        }

	act("You play the Hymn to Tourach, eliminating those who would oppose you.", ch, NULL, NULL, TO_CHAR);
	act("$n begins to play a terrifing dirge, summoning the power of Tourach!", ch, NULL, NULL, TO_ROOM);
	act("Horrible, writhing tentacles emerge from the ground, answering $n's call!", ch, NULL, NULL, TO_ROOM);

	if (victim)
	{
	    act("The tentacles streak towards $n, entwining $m in a deathly embrace!", victim, NULL, NULL, TO_ROOM);
	    act("The tentacles streak towards you, entwining you in a deathly embrace!", victim, NULL, NULL, TO_CHAR);
	    act("The tentacles begin to glow with an maleficent light, as $n struggles to escape!", victim, NULL, NULL, TO_ROOM);
	    act("You struggle to escape, as you feel your life force draining away!", victim, NULL, NULL, TO_CHAR);
	    act("Unholy laughter erupts from the depths of the earth, as $n falls to the ground... dead.", victim, NULL, NULL, TO_ROOM);
	    act("Unholy laughter erupts from the depths of the earth, as you fall to the ground.... dead.", victim, NULL, NULL, TO_CHAR);
	    raw_kill(victim);
	    return TRUE;
	}

	for (vch = ch->in_room->people; vch; vch = vch_next)
	{
	    vch_next = vch->next_in_room;

	    if (vch == ch)
		continue;

	    victim = vch;

	    act("The tentacles streak towards $n, entwining $m in a deathly embrace!", victim, NULL, NULL, TO_ROOM);
	    act("The tentacles streak towards you, entwining you in a deathly embrace!", victim, NULL, NULL, TO_CHAR);
	    act("The tentacles begin to glow with an maleficent light, as $n struggles to escape!", victim, NULL, NULL, TO_ROOM);
	    act("You struggle to escape, as you feel your life force draining away!", victim, NULL, NULL, TO_CHAR);
	    act("Unholy laughter erupts from the depths of the earth, as $n falls to the ground... dead.", victim, NULL, NULL, TO_ROOM);
	    act("Unholy laughter erupts from the depths of the earth, as you fall to the ground.... dead.", victim, NULL, NULL, TO_CHAR);
	    raw_kill(victim);
	}

	return TRUE;
    }
    else
    {
	act("As you begin to chant the hymn to Tourach, you feel as though something is very wrong...", ch, NULL, NULL, TO_CHAR);
	act("A dark shadow gathers around $n as $e begins to chant the hynm to Tourach.", ch, NULL, NULL, TO_ROOM);
	act("The shadows converge, manifesting into inconceivable horrors, and $n screams in pure terror.", ch, NULL, NULL, TO_ROOM);
	act("Dark shadows converge around you, tearing into your soul, and you begin to scream in terror.", ch, NULL, NULL, TO_CHAR);
	act("As the shadows disperse, $n lays in a pool of $s own blood.... dead.", ch, NULL, NULL, TO_ROOM);
	act("The shadows recede, and you lie destroyed... one should not play with powers they don't understand.", ch, NULL, NULL, TO_CHAR);
	raw_kill(ch);
    }

    return TRUE;
}
	
void do_songbook(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    bool found = FALSE;
    int x, entry;

    if (ch->class_num != global_int_class_bard)
    {
	send_to_char("You do not have a songbook.\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	if (IS_NPC(ch))
	    return;

	for (x = 0; x < MAX_SONGBOOK; x++)
	    if (ch->pcdata->songbook[x])
	    {
		sprintf(buf, "Entry #%d:\n\r========\n\r%s\n\r", (x + 1), ch->pcdata->songbook[x]);
		send_to_char(buf, ch);
	        found = TRUE;
	    }

	if (!found)
	    send_to_char("You have nothing recorded in your songbook.\n\r", ch);

	return;
    }

    argument = one_argument(argument, arg);

    if (!is_number(arg))
    {
	send_to_char("Syntax: songbook <entry> <edit/delete>\n\r", ch);
	return;
    }

    entry = atoi(arg);

    if ((entry < 1) || (entry > MAX_SONGBOOK))
    {
	sprintf(buf, "Appropriate entry numbers are between 1 and %d.\n\r", MAX_SONGBOOK);
	send_to_char(buf, ch);
	return;
    }

    if (ch->pcdata->performing)
    {
	send_to_char("You cannot edit your songbook while performing.\n\r", ch);
	return;
    }

    entry--;

    if (!str_cmp(argument, "edit"))
    {
	if (!ch->pcdata->songbook[entry])
	    ch->pcdata->songbook[entry] = str_dup("");

	string_append(ch, &ch->pcdata->songbook[entry]);
	return;
    }

    if (!str_cmp(argument, "delete"))
    {
	if (!ch->pcdata->songbook[entry])
	    send_to_char("That songbook entry does not currently exist.\n\r", ch);
	else
	{
	    free_string(ch->pcdata->songbook[entry]);
	    ch->pcdata->songbook[entry] = NULL;
	}

	return;
    }

    send_to_char("Syntax: songbook <entry> <edit/delete>\n\r", ch);
    return;
}

void do_perform(CHAR_DATA *ch, char *argument)
{
    int entry;
    char buf[45];

    if (IS_NPC(ch))
	return;

    if (ch->song)
    {
	send_to_char("You may not perform while playing a song.\n\r", ch);
	return;
    }

    if (ch->class_num != global_int_class_bard)
    {
	send_to_char("You do not possess a songbook to perform from.\n\r", ch);
	return;
    }

    if ((argument[0] == '\0'))
    {
	if (ch->pcdata->performing)
	{
	    send_to_char("You end your performance.\n\r", ch);
	    ch->pcdata->performing = NULL;
	    return;
	}

	send_to_char("Which entry from your songbook do you wish to perform?\n\r", ch);
	return;
    }

    if (!is_number(argument))
    {
	send_to_char("Syntax: perform <entry #>       to begin performing\n\r", ch);
	send_to_char("        perform                 to cease performing\n\r", ch);
	return;
    }

    entry = atoi(argument);

    if ((entry < 1) || (entry > MAX_SONGBOOK))
    {
	sprintf(buf, "Appropriate entry ranges are from 1 to %d.\n\r", MAX_SONGBOOK);
	send_to_char(buf, ch);
	return;
    }

    ch->pcdata->performing = NULL;
    send_to_char("You begin to perform the song.\n\r", ch);
    ch->pcdata->performing = ch->pcdata->songbook[entry-1];

    return;
}
