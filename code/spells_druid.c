#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <time.h>
#include "merc.h"
#include "magic.h"
#include "tables.h"
#include "string.h"
#include "spells_spirit.h"
#include "NameMaps.h"

/* External declarations */
DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_autoyell);

extern	int		get_moon_state	args( ( int moon_num, 
						bool get_size ) );
extern	EXIT_DATA *	new_exit	args( ( void ) );
extern	bool	leading_vowel(char *);
extern bool remove_obj( CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace );

bool spell_animaleyes( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
CHAR_DATA *victim;
CHAR_DATA *animal;
char buf[MAX_STRING_LENGTH];
DESCRIPTOR_DATA *d;
bool found;

    if (( animal = get_char_world(ch, target_name)) == NULL)
        {
        send_to_char("You could not sense any such creature.\n\r", ch);
        return FALSE;
        }

        if (!IS_NPC(animal) || !IS_SET(animal->act, ACT_ANIMAL))
        {
        send_to_char("You can only see through the eyes of animals.\n\r", ch);
        return FALSE;
        }

        act("$n's eyes glaze over for a moment.", ch, NULL, NULL, TO_ROOM);
        act("You reach out to see through the eyes of $N.", ch, NULL, animal, TO_CHAR);

        sprintf(buf, "Through the eyes of %s you see:\n\r", animal->short_descr);
        send_to_char( buf, ch );
        found = FALSE;
        for ( d = descriptor_list; d; d = d->next )
        {
            if ( d->connected == CON_PLAYING
            && ( victim = d->character ) != NULL
            &&   !IS_NPC(victim)
            &&   victim->in_room != NULL
            &&   !IS_SET(victim->in_room->room_flags,ROOM_NOWHERE)
            &&   (is_room_owner(animal,victim->in_room)
            ||    !room_is_private(victim->in_room))
            &&   victim->in_room->area == animal->in_room->area
            &&   can_see( animal, victim ) )
            {
                found = TRUE;
                sprintf( buf, "%-28s %s\n\r", victim->name, victim->in_room->name);
                send_to_char( buf, ch );
            }
        }
        send_to_char("\n\r", ch);
        sprintf(buf, "%s", animal->in_room->description);
        send_to_char(buf, ch);
        send_to_char("\n\r", ch);
    return TRUE;
}

bool spell_animalswarm( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
        CHAR_DATA *animal;
        AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
        int count, i;
        int vnum;


        vnum = 10;
        if (is_affected(ch, sn))
        {
        send_to_char("You are unable to tap enough energy to call forth more wildlife.\n\r", ch);
        return FALSE;
        }

        if (ch->in_room->sector_type != SECT_FOREST)
        {
        send_to_char("You need to be in the forest to call for its aid that way.\n\r", ch);
        return FALSE;
        }

        act("You call on the forest!", ch, NULL, NULL, TO_CHAR);
        act("$n calls of the forest for its creatures!", ch, NULL, NULL, TO_ROOM);

        count = number_range(2, 20);

        for (i = 0; i<count; i++)
        {
        switch (number_bits(2))
                {
                case 0: i--; break;
                case 1: vnum = MOB_VNUM_DRUID_RABBIT; break;
                case 2: vnum = MOB_VNUM_DRUID_OWL; break;
                case 3: vnum = MOB_VNUM_DRUID_FOX; break;
                default: bug("Broken druid call animal -- numberbits fucked", 0);return FALSE;
                }

        animal = create_mobile (get_mob_index(vnum));
        act("$N answers $n's call!", ch, NULL, animal, TO_ROOM);
        act("$N answers your call!", ch, NULL, animal, TO_CHAR);
        char_to_room(animal, ch->in_room);
        }

        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = level;
        af.duration  = 30;
        af.location  = 0;
        af.modifier  = 0;
        af.bitvector = 0;
        affect_to_char(ch, &af);
    return TRUE;
}


bool spell_animaltongues( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *victim,*aggressor;
    char arg[MAX_STRING_LENGTH];

    target_name = one_argument( target_name, arg );
    //target_name now contains aggro victim name

    if ((aggressor = get_char_room(ch, arg)) == NULL)
    {
        send_to_char("Your spell fails to find a target.\n\r", ch);
        return FALSE;
    }

    if ((victim = get_char_world(ch,target_name)) == NULL)
    {
        send_to_char("Your spell fails to find a target.\n\r", ch);
        return FALSE;
    }

    //This used to be &&... - Seb
    if (aggressor->level > 52 || ch->level < aggressor->level)
    {
        send_to_char("You cannot convince this creature.\n\r", ch);
        return FALSE;
    }
 
    if (is_safe_spell(ch,victim,FALSE))
    {
	send_to_char("You cannot incite this animal against that person.\n\r",ch);
	return FALSE;
    }

    if (is_affected(ch, gsn_animaltongues))
    {
	send_to_char("You cannot speak to another animal so soon.\n\r", ch);
	return FALSE;
    }

    if (IS_NPC(aggressor) && (IS_SET(aggressor->act,ACT_AGGRESSIVE) || IS_SET(aggressor->nact, ACT_PSYCHO)))
    {
        send_to_char("That creature is too aggressive to convince.\n\r",ch);
        return FALSE;
    }
    
    if (is_safe_spell(ch, aggressor, FALSE))
    {
        act("The gods protect $N.", ch, NULL, aggressor, TO_NOTVICT);
        act("The gods protect $N.", ch, NULL, aggressor, TO_CHAR);
        act("The gods protect you.", ch, NULL, aggressor, TO_VICT);
        return TRUE;
    }

    act("You attempt to compel $N to do your bidding.", ch, NULL, aggressor, TO_CHAR);

    if (saves_spell(level, ch, aggressor, DAM_CHARM) 
      || (IS_NPC(aggressor) && IS_SET(victim->act, ACT_NOSUBDUE)) 
      || IS_SET(aggressor->imm_flags, IMM_CHARM) 
      || !IS_SET(aggressor->act, ACT_ANIMAL) || ((aggressor->level - 5) > ch->level))
    {
        act("$N looks at you in confusion.", ch, NULL, aggressor, TO_CHAR);
        return TRUE;
    }

    act("$n compels you to attack $p.",ch,victim,aggressor,TO_VICT);

    // Stop the fucker from casting it again - Seb
    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration	 = (level / 4) + number_range(0, level  / 4); 
    af.location	 = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char( ch, &af );

    if(is_affected(aggressor, gsn_animaltongues))
	affect_strip(aggressor, gsn_animaltongues);

    // OK, now see that the follower wanders off after awhile - Seb
    af.level	 = level;
    af.duration	 = (level / 3) + number_range(0, level / 3); 
    affect_to_char( aggressor, &af );
    aggressor->tracking = victim;
 
    return TRUE;
}


bool spell_barkskin( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
        send_to_char("You skin already has the rough feel of bark.\n\r", ch);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.location  = APPLY_AC;
    af.modifier  = (-10 - level);
    af.duration  = level/2;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    af.location  = APPLY_RESIST_WEAPON;
    af.modifier  = level/5;
    affect_to_char(ch, &af);

    send_to_char("Your skin becomes rough and barklike.\n\r", ch);
    act("$n's skin becomes rough and barklike.", ch, NULL, NULL, TO_ROOM);

    return TRUE;
}

bool spell_bearform( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

        if (is_affected(ch, gsn_bearform))
        {
        send_to_char("You are already affected with the aspect of the bear.\n\r", ch);
        return FALSE;
        }

        if (is_affected(ch, gsn_wolfform))
        {
        act("You discard the aspect of the wolf as you seek the aspect of the bear.", ch, NULL, NULL, TO_CHAR);
        affect_strip(ch, gsn_wolfform);
        }

        if (is_affected(ch, gsn_hawkform))
        {
        act("You discard the aspect of the hawk as you seek the aspect of the bear.", ch, NULL, NULL, TO_CHAR);
        affect_strip(ch, gsn_hawkform);
        }

        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = level;
        af.duration  = level/2;
        af.location  = APPLY_HIT;
        af.modifier  = level * 4;
        af.bitvector = 0;
        affect_to_char(ch, &af);

	// Also modify the actual HP
	ch->hit = UMAX(1, ch->hit + af.modifier);

        af.bitvector = 0;
        af.location  = APPLY_DEX;
        af.modifier  = -level/13;
        affect_to_char(ch, &af);

        af.location  = APPLY_DAMROLL;
        af.modifier  = level/12;
        affect_to_char(ch, &af);

        af.location  = APPLY_RESIST_WEAPON;
        af.modifier  = level/3 + 3;
        affect_to_char(ch, &af);

        act("You growl in fury as you assume the aspect of the bear.", ch, NULL, NULL, TO_CHAR);
        act("$n growls in fury as $e assumes the aspect of the bear.", ch, NULL, NULL, TO_ROOM);
    return TRUE;
}

bool spell_calmanimals( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *vch, *vch_next;

    act("You summon the peace of nature!", ch, NULL, NULL, TO_CHAR);
    act("$n calls upon nature for its serene peace!", ch, NULL, NULL, TO_ROOM);

    for (vch = ch->in_room->people ; vch != NULL ; vch = vch_next)
    {
        vch_next = vch->next_in_room;
        if (IS_NPC(vch) && IS_SET(vch->act, ACT_ANIMAL))
        {
            if (vch->fighting != NULL && number_bits(4) != 0)
		stop_fighting_all(vch);
            if (IS_SET(vch->act, ACT_AGGRESSIVE))
                REMOVE_BIT(vch->act, ACT_AGGRESSIVE);
	    if (IS_SET(vch->nact, ACT_PSYCHO))
		REMOVE_BIT(vch->nact, ACT_PSYCHO);
            if (is_affected(vch, gsn_stampede))
                affect_strip(vch, gsn_stampede);
            vch->tracking = NULL;
        }
    }

    return TRUE;
}

bool spell_chameleon( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    char buf[MAX_STRING_LENGTH];

    if (is_affected(ch, sn))
    {
        affect_strip (ch, sn);
        send_to_char("You shift from your previous animal form.\n\r", ch);
        act("$n returns to $s natural form.", ch, NULL, NULL, TO_ROOM);
    }

    if (IS_OAFFECTED(ch, AFF_DISGUISE))
    {
	send_to_char("Your true appearance is already magically concealed.\n\r", ch);
	return FALSE;
    }

    if (!IS_NPC(victim) || !IS_SET(victim->act, ACT_ANIMAL))
    {
        send_to_char("You can only chameleon animals.\n\r", ch);
        return FALSE;
    }

    act("You shift into the form of $N.", ch, NULL, victim, TO_CHAR);
    act("$n shifts into the form of $N.", ch, NULL, victim, TO_ROOM);

    /* if any string is NULL, dup an empty one instead */

    if (ch->orig_long[0] == '\0')
    {
	free_string(ch->orig_long);
	ch->orig_long = str_dup(ch->long_descr);
    }
    free_string(ch->long_descr);
    ch->long_descr = victim->long_descr ? str_dup(victim->long_descr)
					: str_dup("");

    setFakeName(*ch, victim->name);
    free_string(ch->short_descr);
    ch->short_descr = str_dup(victim->short_descr);

    one_argument(victim->name, buf);
    sprintf(buf, "%s%ld", buf, ch->id);
    setUniqueName(*ch, buf);

    if (ch->orig_description[0] == '\0')
    {
	free_string(ch->orig_description);
	ch->orig_description = str_dup(ch->description);
    }
    free_string(ch->description);
    ch->description = str_dup(victim->description);

    af.where     = TO_OAFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/2;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = AFF_DISGUISE;
    affect_to_char(ch, &af);

    return TRUE;
}


/* Spell: Circle of Stones.  - Belikan + Erinos         */
bool spell_circle_of_stones( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    AFFECT_DATA *paf;

    if (!ch->in_room)
        return FALSE;

    if (ch->in_room->sector_type != SECT_FOREST)
    {
        send_to_char("You may only construct circles of stones in the forest.\n\r", ch);
        return FALSE;
    }

    if (ch->recall_to)
        for (paf = ch->in_room->affected; paf; paf = paf->next)
            if ((paf->type == gsn_circleofstones) && (paf->modifier == ch->id))
            {
                send_to_char("You have already constructed a circle of stones.\n\r", ch);
                return FALSE;
            }

    if (IS_SET(ch->in_room->room_flags, ROOM_NOGATE))
    {
        send_to_char("You concentrate on the forest, but fail to raise a circle of stones.\n\r", ch);
        return FALSE;
    }

    send_to_char("You concentrate on the forest, and raise a circle of stones about you!\n\r", ch);
    act("$n concentrates on the forest, and a circle of stones raise about $m!", ch, NULL, NULL, TO_ROOM);

    af.where     = TO_ROOM;
    af.type      = sn;
    af.level     = level;
    af.duration  = 100;
    af.location  = 0;
    af.modifier  = ch->id;
    af.bitvector = 0;
    affect_to_room(ch->in_room, &af);

    if (!ch->recall_old)
        ch->recall_old = ch->recall_to;
    ch->recall_to = ch->in_room;

    return TRUE;
}

bool spell_command_weather(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    char arg[MAX_STRING_LENGTH];

    if (!ch->in_room)
        return FALSE;

    if (!IS_OUTSIDE(ch))
    {
        send_to_char("You must be outdoors to command the weather.\n\r", ch);
        return FALSE;
    }

    if (is_affected(ch, gsn_commandweather))
    {
        send_to_char("You are not yet ready to command the weather again.\n\r",
ch);
        return FALSE;
    }

    target_name = one_argument(target_name, arg);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/5;
    af.location  = 0;
    af.bitvector = 0;

    if (!str_cmp(arg, "drier"))
    {
        af.modifier = WCOM_DRIER;
        affect_to_area(ch->in_room->area, &af);
        send_to_char("The air begins to become drier.\n\r", ch);
    }
    else if (!str_cmp(arg, "wetter"))
    {
        af.modifier = WCOM_WETTER;
        affect_to_area(ch->in_room->area, &af);
        send_to_char("The air begins to become heavier with precipitation.\n\r", ch);
    }
    else if (!str_cmp(arg, "calmer"))
    {
        af.modifier = WCOM_CALMER;
        affect_to_area(ch->in_room->area, &af);
        send_to_char("The wind begins to calm down.\n\r", ch);
    }
    else if (!str_cmp(arg, "windier"))
    {
        af.modifier = WCOM_WINDIER;
        affect_to_area(ch->in_room->area, &af);
        send_to_char("The wind begins to become more fierce about you.\n\r", ch);
    }
    else if (!str_cmp(arg, "cooler"))
    {
        af.modifier = WCOM_COOLER;
        affect_to_area(ch->in_room->area, &af);
        send_to_char("The air around you begins to become cooler.\n\r", ch);
    }
    else if (!str_cmp(arg, "warmer"))
    {
        af.modifier = WCOM_WARMER;
        affect_to_area(ch->in_room->area, &af);
        send_to_char("The air around you begins to become warmer.\n\r", ch);
    }
    else
    {
        send_to_char("Valid options are: drier wetter calmer windier cooler warmer\n\r", ch);
        return FALSE;
    }

    af.duration = level/4;
    af.modifier = 0;
    affect_to_char(ch, &af);

    return TRUE;
}

bool spell_commune_nature( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, gsn_commune_nature))
    {
        send_to_char("You are already in touch with the voices of natures.\n\r", ch);
        return FALSE;
    }

    send_to_char("You open your mind to the voices of nature.\n\r", ch);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    return TRUE;
}



bool spell_creepingcurse( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
DESCRIPTOR_DATA *d;


    if (is_affected(ch, gsn_creepingcurse))
    {
	send_to_char("You cannot summon the power to call the creeping curse again yet.\n\r", ch);
	return FALSE;
    }

    if (area_is_affected(ch->in_room->area, gsn_creepingcurse))
    {
        send_to_char("This place already crawls with the morbid life of the creeping curse.\n\r", ch);
        return FALSE;
    }

    af.where        = TO_AREA;
    af.type = sn;
    af.level        = level;
    af.duration     = 4;
    af.location     = 0;
    af.modifier     = IS_NPC(ch) ? SPH_LUNAR : ch->pcdata->minor_sphere;
    af.bitvector   = 0;
    affect_to_area(ch->in_room->area, &af);

    af.duration     = 120;
    affect_to_char(ch, &af);

    for (d = descriptor_list; d != NULL; d = d->next)
    {
        if (d->connected == CON_PLAYING && d->character && d->character->in_room)
            if (d->character->in_room->area == ch->in_room->area)
                send_to_char("The ground springs forth with new life, as the creeping curse comes from all around!\n\r", d->character);
    }
    return TRUE;
}


bool spell_elementalprotection( int sn, int level, CHAR_DATA *ch, void *vo, int
target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
        send_to_char("You are already protected from the elements.\n\r", ch);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.location  = APPLY_RESIST_FIRE;
    af.modifier  = level/3;
    af.duration  = level/5;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    af.location  = APPLY_RESIST_COLD;
    af.modifier  = level/3;
    affect_to_char(ch, &af);

    af.location  = APPLY_RESIST_LIGHTNING;
    af.modifier  = level/3;
    affect_to_char(ch, &af);

    send_to_char("You feel protected from the elements.\n\r", ch);
    act("$n is protected from the elements.", ch, NULL, NULL, TO_ROOM);

    return TRUE;
}

bool spell_forestwalk( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, gsn_forestwalk))
    {
        send_to_char("The powers of nature already guide your way.\n\r", ch);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.duration  = (level / 3);
    af.bitvector = 0;
    affect_to_char(ch, &af);

    send_to_char("You call upon the powers of nature to ease your passage through the forest.\n\r", ch);

    return TRUE;
}

bool spell_giantgrowth( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(victim, gsn_giantgrowth))
    {
        if (victim == ch)
            send_to_char("You are already affected by giant growth.\n\r", ch);
        else
            act("$N is already affected by giant growth.", ch, NULL, victim, TO_CHAR);
        return FALSE;
    }

    if (is_affected(victim, gsn_shrink))
    {
	if (check_dispel(ch->level, victim, gsn_shrink))
	{
	    if (victim != ch)
		act("$n returns to normal size.",victim,NULL,NULL,TO_ROOM);
	}	
	else
	    if (victim == ch)
		send_to_char("You remain shrunken.\n\r",ch);
	    else
	    {
		act("$n remains shrunken.",victim,NULL,NULL,TO_ROOM);
		act("You remain shrunken.",victim,NULL,NULL,TO_CHAR);
	    }
	
	return TRUE;
    }
	
    if (victim->size == SIZE_LARGE)
    {
        if (victim == ch)
            send_to_char("You cannot grow any larger.\n\r", ch);
        else
            act("$N cannot grow any larger.", ch, NULL, victim, TO_CHAR);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.modifier  = 1;
    af.location  = APPLY_SIZE;
    af.duration  = (level / 5);
    af.bitvector = 0;
    affect_to_char(victim, &af);
    af.location  = APPLY_STR;
    af.modifier  = +2;
    affect_to_char(victim, &af);

    act("You watch as $n's frame enlarges.", victim, NULL, NULL, TO_ROOM);
    send_to_char("You feel the world grow smaller.\n\r", victim);

    return TRUE;
}


bool spell_hawkform( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, gsn_hawkform))
    {
        send_to_char("You are already affected with the aspect of the hawk.\n\r", ch);
        return FALSE;
    }

        if (is_affected(ch, gsn_wolfform))
        {
        act("You discard the aspect of the wolf as you seek the aspect of the hawk.", ch, NULL, NULL, TO_CHAR);
        affect_strip(ch, gsn_wolfform);
        }

        if (is_affected(ch, gsn_bearform))
        {
        act("You discard the aspect of the bear as you seek the aspect of the hawk.", ch, NULL, NULL, TO_CHAR);
        affect_strip(ch, gsn_bearform);
        }

        if (!ch->in_room)
                return FALSE;

        if (area_is_affected(ch->in_room->area, gsn_gravitywell))
        {
            send_to_char("The intense pull towards the earth keeps you from transforming!\n\r", ch);
            return FALSE;
        }

        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = level;
        af.duration  = level/2;
        af.location  = APPLY_HIT;
        af.modifier  = -level;
        af.bitvector = AFF_FLYING;
        affect_to_char(ch, &af);

		// Also modify the hp
		ch->hit = UMAX(1, ch->hit + af.modifier);

        af.bitvector = 0;
        af.location  = APPLY_DAMROLL;
        af.modifier  = -level/5;
        affect_to_char(ch, &af);

        af.bitvector = 0;
        af.location  = APPLY_HITROLL;
        af.modifier  = level/5;
        affect_to_char(ch, &af);

        act("You rise off the ground as you assume the aspect of the hawk.", ch, NULL, NULL, TO_CHAR);
        act("$n rises off the ground as $e assumes the aspect of the hawk.", ch, NULL, NULL, TO_ROOM);
    return TRUE;
}

bool spell_insectswarm( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (victim == ch)
    {
        send_to_char("You cannot summon insects upon yourself.\n\r", ch);
        return FALSE;
    }

    if (is_affected(victim, gsn_insectswarm))
    {
        act("$N is already surrounded by insects.", ch, NULL, victim, TO_CHAR);
        return FALSE;
    }

    if (is_safe_spell(ch, victim, FALSE))
        return FALSE;

    if (saves_spell(level, ch, victim, DAM_OTHER))
    {
        act("A swarm of insects surrounds $n, but quickly disperses.", victim, NULL, NULL, TO_ROOM);
        act("A swarm of insects surrounds you, but quickly disperses.", victim, NULL, NULL, TO_CHAR);
        return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.location  = 0;
    af.modifier  = 0;
    af.duration  = (ch->level / 15);
    af.bitvector = 0;
    affect_to_char(victim, &af);

    act("You summon a swarm of insects to surround $N!", ch, NULL, victim, TO_CHAR);
    act("$n summons a swarm of insects to surround $N!", ch, NULL, victim, TO_NOTVICT);
    act("$n summons a swarm of insects to surround you!", ch, NULL, victim, TO_VICT);

    return TRUE;
}

bool spell_nettles( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (victim == ch)
    {
        send_to_char("You shouldn't throw nettles upon yourself.\n\r", ch);
        return FALSE;
    }

    if (is_affected(victim, gsn_nettles))
    {
        act("$N is already covered with nettles.", ch, NULL, victim, TO_CHAR);
        return FALSE;
    }

    if (is_safe_spell(ch, victim, FALSE))
        return FALSE;

    if (saves_spell(level, ch, victim, DAM_OTHER))
    {
        act("Nettles fall all around $n.", victim, NULL, NULL, TO_ROOM);
	act("Nettles fall all around you.", victim, NULL, NULL, TO_CHAR);
	return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.location  = 0;
    af.modifier  = 0;
    af.duration  = (ch->level / 15);
    af.bitvector = 0;
    affect_to_char(victim, &af);

    act("You shower $N with nettles!", ch, NULL, victim, TO_CHAR);
    act("$n showers $N with nettles!", ch, NULL, victim, TO_NOTVICT);
    act("$n showers you with nettles, and your skin begins itching!", ch, NULL, victim, TO_VICT);

    return TRUE;
}

bool spell_moonray( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *vch;
  CHAR_DATA *vch_next;
  AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
  int dam, moonstate, moonmod;

    if (ch->in_room->sector_type == SECT_INSIDE)
    {
        send_to_char("You fail to tap the moon's powers from indoors.\n\r", ch);
        return FALSE;
    }

    if ((time_info.hour >= season_table[time_info.season].sun_up)
     && (time_info.hour < season_table[time_info.season].sun_down))
    {
        send_to_char("You can only tap the moon's powers that way at night.\n\r", ch);
        return FALSE;
    }

    moonstate = get_moon_state(1, FALSE);

    moonmod = get_moon_state(2, FALSE);

    if (moonmod == MOON_FULL)
	level = level * 6 / 5;
    else if ((moonmod == MOON_WAXING_GIBBOUS) || (moonmod == MOON_WANING_GIBBOUS))
	level = level * 11 / 10;
    else if ((moonmod == MOON_WAXING_CRESCENT) || (moonmod == MOON_WANING_CRESCENT))
	level = level * 9 / 10;
    else if (moonmod == MOON_NEW)
	level = level * 4 / 5;

    if (moonstate == MOON_FULL)
    {
        act("You call on the full moon!", ch, NULL, NULL, TO_CHAR);
        act("$n calls on the full moon!", ch, NULL, NULL, TO_ROOM);
        for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
        {
            vch_next = vch->next_in_room;
            if ( !is_safe_spell(ch, vch, TRUE)
            && (IS_PK(ch, vch) || IS_NPC(ch) || IS_NPC(vch))
            && !is_same_group(ch, vch)
            && ch->in_room == vch->in_room)
            {
                if ((ch->fighting == NULL || vch->fighting == NULL) && !IS_NPC(vch))
                {
                    if (can_see(vch, ch))
                    {
                        sprintf( buf, "Help!  %s is attacking me!!", PERS(ch, vch));
                        do_autoyell( vch, buf );
                    }
                    else
                    {
                        sprintf( buf, "Help! Someone is attacking me!");
                        do_autoyell( vch, buf );
                    }
                }

                if (is_affected(vch, gsn_moonray))
                {
                    act("$N is already confused by the moon.", ch, NULL, vch, TO_CHAR);
                    if (vch->fighting == NULL && IS_AWAKE(vch))
                        vch->fighting = ch;
                    continue;
                }

                if (saves_spell(level, ch, vch, DAM_OTHER))
                {
                    act("$n looks confused a moment, but shakes it off.", vch, NULL, NULL, TO_ROOM);
                    act("You feel momentarily confused, but you shake it off.", vch, NULL, NULL, TO_CHAR);
                    continue;
                }

                af.where     = TO_AFFECTS;
                af.type      = gsn_moonray;
                af.level     = level;
                af.duration  = number_fuzzy(level / 5);
                af.location  = APPLY_NONE;
                af.modifier  = 0;
                af.bitvector = 0;
                affect_to_char(vch, &af);

                if (vch->fighting == NULL && IS_AWAKE(vch))
                    vch->fighting = ch;

                act("$n looks confused as the moonray bathes $m in soft light.", vch, NULL, NULL, TO_ROOM);
                act("You feel confused as the light of the moon bathes you in soft light.", vch, NULL, NULL, TO_CHAR);

		add_event((void *) vch, 1, &event_moonray);
            }
        }
    }
    else if ((moonstate == MOON_WAXING_GIBBOUS)
    || (moonstate == MOON_WAXING_CRESCENT)
    || (moonstate == MOON_WAXING_HALF))
    {
        send_to_char("You call on the moon for healing.\n\r", ch);
        act("$n calls on the waxing moon!", ch, NULL, NULL, TO_ROOM);
        for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
        {
            vch_next = vch->next_in_room;
            if (is_same_group(ch, vch))
            {
                if(is_an_avatar(vch))
		            continue;
		send_to_char("You feel better!\n\r", vch);
                vch->hit = UMIN(vch->max_hit, vch->hit + number_fuzzy(level + 5));
            }
        }
    }
    else if ((moonstate == MOON_WANING_GIBBOUS)
     || (moonstate == MOON_WANING_HALF)
     || (moonstate == MOON_WANING_CRESCENT))
    {
        act("You call on the waning moon!", ch, NULL, NULL, TO_CHAR);
        act("$n calls on the waning moon!", ch, NULL, NULL, TO_ROOM);

        for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
        {
            vch_next = vch->next_in_room;
            if ( !is_safe_spell(ch, vch, TRUE)
            && (IS_PK(ch, vch) || IS_NPC(ch) || IS_NPC(vch))
            && !is_same_group(ch, vch)
            && ch->in_room == vch->in_room)
            {
                if ((ch->fighting == NULL || vch->fighting == NULL) && !IS_NPC(vch))
                {
                    if (can_see(vch, ch))
                    {
                        sprintf( buf, "Help!  %s is attacking me!", PERS(ch, vch));
                        do_autoyell( vch, buf );
                    }
                    else
                    {
                        sprintf( buf, "Help! Someone is attacking me!");
                        do_autoyell( vch, buf );
                    }
                }

                dam = (dice(level, 5));
                if (saves_spell(level, ch, vch, DAM_OTHER))
                    dam /= 2;
                damage_old( ch, vch, dam, gsn_moonray, DAM_OTHER ,TRUE);
            }
        }
    }
    else if (moonstate == MOON_NEW)
    {
        act("You call on the new moon!", ch, NULL, NULL, TO_CHAR);
        act("$n calls on the new moon!", ch, NULL, NULL, TO_ROOM);

        for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
        {
            vch_next = vch->next_in_room;
            if ( !is_safe_spell(ch, vch, TRUE)
            && (IS_PK(ch, vch) || IS_NPC(ch) || IS_NPC(vch))
            && !is_same_group(ch, vch)
            && ch->in_room == vch->in_room)
            {
                if ((ch->fighting == NULL || vch->fighting == NULL) && !IS_NPC(vch))
                {
                    if (can_see(vch, ch))
                    {
                        sprintf( buf, "Help!  %s is attacking me!", PERS(ch, vch));
                        do_autoyell( vch, buf );
                    }
                    else
                    {
                        sprintf( buf, "Help! Someone is attacking me!");
                        do_autoyell( vch, buf );
                    }
                }

                dam = number_fuzzy(level * 2);
                if (saves_spell(level, ch, vch, DAM_OTHER))
                    dam /= 2;
                expend_mana(vch, dam);
                vch->mana = UMAX(0, vch->mana);
                send_to_char("You feel the moon pull at your mental reserves.\n\r", vch);
                if (vch->fighting == NULL && IS_AWAKE(vch))
                    vch->fighting = ch;

            }
        }
    }
    else
    {
	send_to_char("You fail to tap power from the moon.\n\r", ch);
	return TRUE;
    }

    return TRUE;
}

bool spell_mushroomcircle( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    char arg[MAX_STRING_LENGTH];

    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to create another mushroom circle.\n\r", ch);
        return FALSE;
    }

    if (room_is_affected(ch->in_room, gsn_mushroomcircle))
    {
        send_to_char("There is already a mushroom circle in the room.\n\r", ch);
	return FALSE;
    }

    if (!ch->in_room)
        return FALSE;

    if ((ch->in_room->sector_type == SECT_INSIDE)
     || (ch->in_room->sector_type == SECT_CITY)
     || (ch->in_room->sector_type == SECT_WATER_SWIM)
     || (ch->in_room->sector_type == SECT_WATER_NOSWIM)
     || (ch->in_room->sector_type == SECT_AIR)
     || (ch->in_room->sector_type == SECT_UNDERWATER))
    {
        send_to_char("You cannot create a mushroom circle here!\n\r", ch);
        return FALSE;
    }

    target_name = one_argument(target_name, arg);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = (level / 5);
    af.location  = 0;
    af.bitvector = 0;

    af.modifier = -1;

    if (arg[0] != '\0')
        if (!str_prefix(arg, "sleep"))
            af.modifier = 0;
        else if (!str_prefix(arg, "hallucinatory"))
            af.modifier = 1;
        else if (!str_prefix(arg, "slow"))
            af.modifier = 2;

    if (af.modifier == -1)
    {
        send_to_char("Valid circle types are: sleep hallucinatory slow.\n\r", ch);
        return FALSE;
    }

    send_to_char("A circle of mushrooms rises out of the ground.\n\r", ch);
    act("A circle of mushrooms rises out of the ground.", ch, NULL, NULL, TO_ROOM);
    affect_to_room(ch->in_room, &af);

    af.duration = 6;
    af.modifier = 0;
    affect_to_char(ch, &af);

    return TRUE;
}


bool spell_naturegate( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ((ch->in_room->vnum >= MIN_VNUM_NATUREGATE 
      && ch->in_room->vnum <= MAX_VNUM_NATUREGATE)
      || (ch->in_room->vnum >= MIN_VNUM_G_NATUREGATE
      && ch->in_room->vnum <= MAX_VNUM_G_NATUREGATE))
    {
        send_to_char("You're already inside the naturegate.\n\r", ch);
        return FALSE;
    }

    if ((ch->in_room->sector_type != SECT_FOREST) 
      && (ch->in_room->sector_type != SECT_SWAMP))
    {
        send_to_char("You can't sense the power of nature enough here to enter the gate.\n\r", ch);
        return FALSE;
    }

    if (ch->in_room->room_flags & ROOM_NO_RECALL ||
      ch->in_room->room_flags & ROOM_NOGATE || is_affected(ch, gsn_matrix))
    {
        send_to_char("You cannot enter the naturegate from here.\n\r", ch);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 3;
    af.location  = APPLY_HIDE;
    af.modifier  = ch->in_room->vnum;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    if (ch->pcdata->minor_sphere == SPH_GAMALOTH
    || (ch->pcdata->minor_sphere == SPH_LUNAR	&& (ch->pcdata->karma < 0 || (ch->pcdata->karma == 0 && number_bits(1) == 0))))
    {
	act("You step through the foliage, finding a path to the base of Gamaloth.", ch, NULL, NULL, TO_CHAR);
	act("$n steps through the foliage, finding a path to the base of Gamaloth.", ch, NULL, NULL, TO_ROOM);

	char_from_room(ch);
	char_to_room(ch, get_room_index(ROOM_VNUM_G_NATUREGATE));
    }
    else
    {
	act("You step through the foliage, finding a path to the heart of nature.", ch, NULL, NULL, TO_CHAR);
	act("$n steps through the foliage, finding a path to the heart of nature.", ch, NULL, NULL, TO_ROOM);

	char_from_room(ch);
	char_to_room(ch, get_room_index(ROOM_VNUM_NATUREGATE));
    }
    do_look(ch, "auto");

    return TRUE;
}

void do_forage(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA *vObj;
	int chance = get_skill(ch,gsn_forage);
	int vnum = OBJ_VNUM_BOUNTYGRAPE;

	if (chance == 0)
	{
	    send_to_char("You aren't skilled enough to forage in the wilds.\n\r",ch);
	    return;
	}
	if (!ch->in_room)
	{
	    send_to_char("You can't find anything here.\n\r",ch);
	    return;
	}
	if (ch->move < 5)
	{
	    send_to_char("You are too tired to forage for food.\n\r",ch);
	    return;
	}

	if (number_percent() < chance)
	{
	    switch(ch->in_room->sector_type)
	    {
	        case SECT_FIELD:
	        case SECT_ROAD:
		{
		    if (ch->in_room->area->vnum == AREA_VNUM_SHARGOB
		      || ch->in_room->area->vnum == AREA_VNUM_MINES)
		    {
			send_to_char("All things here are unfit for consumption.\n\r",ch);
			return;
		    }
		    if (number_bits(1) == 0)
		    {
		        vnum = OBJ_VNUM_BOUNTYDEWBERRIES;
		        act("You pick a handful of wild dewberries from a nearby bush.",ch,NULL,NULL,TO_CHAR);
		        act("$n picks a handful of wild dewberries from a nearby bush.",ch,NULL,NULL,TO_ROOM);
		    }
		    else
		    {
			vnum = OBJ_VNUM_BOUNTYDANDELION;
			act("You pick a few dandelion greens.",ch,NULL,NULL,TO_CHAR);
			act("$n picks a few dandelion greens.",ch,NULL,NULL,TO_ROOM);
		    }	
		    break;
		}
	        case SECT_FOREST:
		{
		    if (number_bits(1) == 0)
		    {
			vnum = OBJ_VNUM_BOUNTYACORNS;
		    	act("You gather a handful of acorns from the ground.",ch,NULL,NULL,TO_CHAR);
		    	act("$n gathers a handful of acorns from the ground.",ch,NULL,NULL,TO_ROOM);
		    }
		    else
		    {
			vnum = OBJ_VNUM_BOUNTYFIDDLEHEADS;
			act("You pick a few fiddleheads from their ferns.",ch,NULL,NULL,TO_CHAR);
			act("$n picks a few fiddleheads from their ferns.",ch,NULL,NULL,TO_ROOM);
		    }
		    break;
		}
	        case SECT_HILLS:
		case SECT_MOUNTAIN:
		{
		    if (number_bits(1) == 0)
		    {
			vnum = OBJ_VNUM_BOUNTYCHANTERELLE; 
		    	act("You find and gather some chanterelle mushrooms.",ch,NULL,NULL,TO_CHAR);
		    	act("$n gathers some chanterelle mushrooms.",ch,NULL,NULL,TO_ROOM);
		    }
		    else
		    {
			vnum = OBJ_VNUM_BOUNTYBLACKBERRIES;
			act("You pick a handful of blackberries from a nearby bush.",ch,NULL,NULL,TO_CHAR);
			act("$n picks a handful of blackberries from a nearby bush.",ch,NULL,NULL,TO_ROOM);
		    }
		    break;	   
		}
	        case SECT_SWAMP:
		{
		    if (number_bits(1) == 0)
		    {
			vnum = OBJ_VNUM_BOUNTYCATTAIL; 
		    	act("You cut a few cattails from their stalks.",ch,NULL,NULL,TO_CHAR);
		    	act("$n cuts a cattail from its stalk.",ch,NULL,NULL,TO_ROOM);
		    }
		    else
		    {
			vnum = OBJ_VNUM_BOUNTYLOTUS;
			act("You pick a lotus, keeping the root.",ch,NULL,NULL,TO_CHAR);
			act("$n picks a lotus, keeping the root.",ch,NULL,NULL,TO_ROOM);
		    }
		    break;
		}
	        case SECT_WATER_SWIM:
	        case SECT_WATER_NOSWIM:
		{
		    if (number_bits(1) == 0)
		    {
			vnum = OBJ_VNUM_BOUNTYTUBER;
			act("You pull a few arrowhead tubers free.",ch,NULL,NULL,TO_CHAR);
			act("$n pulls a few arrowhead tubers free.",ch,NULL,NULL,TO_ROOM);
		    }
		    else
		    {	
			vnum = OBJ_VNUM_BOUNTYWATERCRESS;
			act("You pick a few watercress leaves from the water.",ch,NULL,NULL,TO_CHAR);
			act("$n picks a few watercress leaves from the water.",ch,NULL,NULL,TO_ROOM);

		    }
		    break;
		}
	        case SECT_UNDERWATER:
		{
		    if (number_bits(1) == 0)
		    {
		        act("You pluck a few red porphyra blossoms from a floating strand.",ch,NULL,NULL,TO_CHAR);
		        act("$n plucks a few red porphyra blossoms from a floating strand.",ch,NULL,NULL,TO_ROOM);
		        vnum = OBJ_VNUM_BOUNTYPORPH;
		    }
		    else
		    {
			vnum = OBJ_VNUM_BOUNTYKELP;
			act("You gather a frond of edible kelp from the water.",ch,NULL,NULL,TO_CHAR);
			act("$n gathers a frond of edible kelp from the water.",ch,NULL,NULL,TO_ROOM);
		    }
		    break;
		}
	        case SECT_DESERT:
		{
		    if (number_bits(1) == 0)
		    {
		        act("You cut a pad from a nearby prickly pear cactus.",ch,NULL,NULL,TO_CHAR);
		        act("$n cuts a pad from a nearby prickly pear cactus.",ch,NULL,NULL,TO_ROOM);
		        vnum = OBJ_VNUM_BOUNTYPRICKLY; 
		    }
		    else
		    {
			act("You cut a few yucca fruit from a nearby plant.",ch,NULL,NULL,TO_CHAR);
			act("$n cuts a few yucca fruit from a nearby plant.",ch,NULL,NULL,TO_ROOM);
			vnum = OBJ_VNUM_BOUNTYYUCCA;
		    }
		    break;
		}
	        case SECT_INSIDE:
	        case SECT_AIR:
	        case SECT_UNDERGROUND:
	        default: 
	        {
		    send_to_char("You can't find anything here.\n\r",ch);
		    return;
	        }
	    }
	    ch->move -= 5;
	    vObj = create_object(get_obj_index(vnum), ch->level);
	    vObj->value[0] = (ch->level / 2);
	    vObj->value[1] = chance * 3/4;
	    obj_to_char(vObj, ch);
	    check_improve(ch,NULL,gsn_forage,TRUE,1);
	}
	else
	{
	    act("You forage for food, but are unable to find anything.",ch, NULL, NULL, TO_CHAR);
	    act("$n forages for food, but is unable to find anything.",ch,NULL,NULL,TO_ROOM);
	    check_improve(ch,NULL,gsn_forage,FALSE,1);
	}
	WAIT_STATE(ch,UMAX(ch->wait,skill_table[gsn_forage].beats));
	
	return;    
}

bool spell_plantentangle( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, gsn_plantentangle))
    {
        send_to_char("They have already been entangled.\n\r", ch);
        return FALSE;
    }

    if (victim == ch)
    {
        send_to_char("You cannot call upon to the plants to entwine yourself.\n\r", ch);
        return FALSE;
    }

    if (saves_spell(level + 22 - get_curr_stat(victim, STAT_DEX), ch, victim, DAM_OTHER))
    {
        act("$N avoids the entangling plants.", ch, NULL, victim, TO_CHAR);
        act("$n summons plants to entangle $N, but $E avoids them.", ch, NULL, victim, TO_NOTVICT);
        act("You cleverly avoid the entangling plants.", ch, NULL, victim, TO_VICT);
        return TRUE;
    }

    if (IS_NPC(ch) || ch->pcdata->minor_sphere == SPH_FIRIEL)
    {
    	act("Plants spring up around $N, entangling $M!", victim, NULL, NULL, TO_ROOM);
	    act("Plants spring up from the ground suddenly, entangling you!", victim, NULL, NULL, TO_CHAR);
    }
    else if (ch->pcdata->minor_sphere == SPH_GAMALOTH)
    {
    	act("Pale roots burst forth from the ground, entangling $N!",victim,NULL,NULL,TO_ROOM);
	    act("Pale roots burst forth from the ground, entangling you!",victim,NULL,NULL,TO_CHAR);
    }
    else if (ch->pcdata->minor_sphere == SPH_LUNAR)
    {
    	act("$n begins moving more slowly as $s blood thickens!",victim,NULL,NULL,TO_ROOM);
	    act("Your blood gels within your veins, slowing your movement!",victim,NULL,NULL,TO_CHAR);
    }
    
    if (check_spirit_of_freedom(victim))
    {
    	send_to_char("The spirit of freedom surges within you, and the entangling plants quickly fall away.\n\r", victim);
	    act("$n begins to glow brightly, and the plants entangling $m fall away.", victim, NULL, NULL, TO_ROOM);
    	return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = gsn_plantentangle;
    af.level     = level;
    af.duration  = 8;
    af.modifier  = -1;
    af.location  = APPLY_DEX;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    return TRUE;
}


bool spell_plantgrowth( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.modifier  = 0;
    af.location  = 0;
    af.duration  = 10;
    af.bitvector = 0;

    if (is_affected(ch, gsn_plantgrowth))
    {
        send_to_char("You don't feel ready to summon plants again.\n\r", ch);
        return FALSE;
    }

    if (!ch->in_room)
        return FALSE;

    if (ch->in_room->sector_type == SECT_FOREST)
    {
        send_to_char("This area is already filled with wildlife.\n\r", ch);
        return FALSE;
    }

    if ((ch->in_room->sector_type == SECT_INSIDE)
     || (ch->in_room->sector_type == SECT_CITY)
     || (ch->in_room->sector_type == SECT_WATER_SWIM)
     || (ch->in_room->sector_type == SECT_WATER_NOSWIM)
     || (ch->in_room->sector_type == SECT_AIR)
     || (ch->in_room->sector_type == SECT_UNDERWATER))
    {
        send_to_char("You cannot summon plants here.\n\r", ch);
        return FALSE;
    }


    affect_to_char(ch, &af);

    if (number_percent() > get_skill(ch, gsn_plantgrowth))
    {
        send_to_char("You call upon the power of nature to consume the area, but fail.\n\r", ch);
        return TRUE;
    }

    af.duration = (ch->level / 5);
    af.modifier = ch->in_room->sector_type;
    ch->in_room->sector_type = SECT_FOREST;

    affect_to_room(ch->in_room, &af);

    send_to_char("Wild growth springs up around you, filling the area with trees and plants.\n\r", ch);
    act("The area is quickly filled with trees and plants.\n\r", ch, NULL, NULL, TO_ROOM);

    return TRUE;
}

bool spell_regeneration( int sn, int level, CHAR_DATA *ch, void *vo,
        int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (victim != ch)
    {
        send_to_char("You may not cast this spell upon others.\n\r", ch);
        return FALSE;
    }

    if ( IS_AFFECTED(victim, AFF_REGENERATION) )
    {
        send_to_char("You are already regenerating quickly.\n\r",ch);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level / 4;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = AFF_REGENERATION;
    affect_to_char( victim, &af );
    send_to_char( "You feel healthier.\n\r", victim );
    act( "$n looks healthier.", victim, NULL, NULL, TO_ROOM );
    return TRUE;
}


/* Spell: Rite of Dawn - Belikan and Erinos                   */
/* Design modification by Jolinn.  Die, Jolinn, Die. -E       */
/* Further design modification by Kronos. Yay, Kronos, Yay -K */
/* Brazen here - Now, with extra Firielness 		      */
bool spell_riteofthesun( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch,gsn_riteofthesun))
    {
	send_to_char("You are already infused with the power of the sun.\n\r",ch);
	return FALSE;
    }

    if (!ch->in_room)
        return FALSE;

    if (!IS_OUTSIDE(ch) || IS_SET(ch->in_room->room_flags, ROOM_DARK)
    ||  IS_SET(ch->in_room->room_flags, ROOM_UBERDARK) || IS_SET(ch->in_room->room_flags, ROOM_NOWEATHER))
    {
        send_to_char("You cannot commune the rite from your current location.\n\r", ch);
        return FALSE;
    }

    if (time_info.hour < season_table[time_info.season].sun_up
      || time_info.hour >= season_table[time_info.season].sun_down)
    {
        send_to_char("You cannot commune the rite at night.\n\r", ch);
        return FALSE;
    }

    if (time_info.hour + 1 >= season_table[time_info.season].sun_down)
    {
	send_to_char("The sun will have set by the time the rite is complete.\n\r",ch);
	return FALSE;
    }
    af.where     = TO_OAFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 48;
    af.location  = 0;
    af.modifier  = current_time;
    af.bitvector = AFF_RITEOFTHESUN;
    affect_to_char(ch, &af);

    send_to_char("You begin harnessing the sun's power.\n\r", ch);
    act("$n kneels down, and begins to glow faintly.", ch, NULL, NULL, TO_ROOM);

    return TRUE;
}

bool spell_shrink( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_safe_spell(ch, victim, FALSE))
        return FALSE;

    if (is_affected(victim, gsn_shrink))
    {
        if (victim == ch)
            send_to_char("You are already affected by shrink.\n\r", ch);
        else
            act("$N is already affected by shrink.", ch, NULL, victim, TO_CHAR);        return FALSE;
    }

    if (is_affected(victim, gsn_giantgrowth))
    {
	if (check_dispel(ch->level, victim, gsn_giantgrowth))
	{
	    if (victim != ch)
		act("$n returns to normal size.",victim,NULL,NULL,TO_ROOM);
	}	
	else
	    if (victim == ch)
		send_to_char("You remain larger.\n\r",ch);
	    else
	    {
		act("$n remains larger.",victim,NULL,NULL,TO_ROOM);
		act("You remain larger.",victim,NULL,NULL,TO_CHAR);
	    }
	
	return TRUE;
    }
	
    if (victim->size == SIZE_SMALL)
    {
        if (victim == ch)
            send_to_char("You cannot become any smaller.\n\r", ch);
        else
            act("$N cannot become any smaller.", ch, NULL, victim, TO_CHAR);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.modifier  = -1;
    af.location  = APPLY_SIZE;
    af.duration  = (level / 5);
    af.bitvector = 0;
    affect_to_char(victim, &af);
    af.location  = APPLY_STR;
    af.modifier  = -2;
    affect_to_char(victim, &af);

    act("You watch as $n's frame shrinks.", victim, NULL, NULL, TO_ROOM);
    act("You feel the world grow larger.", victim, NULL, NULL, TO_CHAR);

    return TRUE;
}


bool spell_speakwithplants( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];

    act("You reach out to the life of the land to speak with the plants.", ch, NULL, NULL, TO_CHAR);
    act("$n reaches out to the life of the land to speak with the plants.",
ch, NULL, NULL, TO_ROOM);

    if ((victim = get_char_world(ch,target_name)) == NULL)
    {
	send_to_char("You don't sense that person.\n\r",ch);
	return FALSE;
    }

    if (victim->in_room == NULL
      || IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
      || IS_SET(victim->in_room->room_flags, ROOM_NOGATE))
    {
	act("The plants have nothing to say about $N.",ch,NULL,victim,TO_CHAR);
	return FALSE;
    }
	
    if (victim->in_room->sector_type == SECT_FOREST
      || victim->in_room->sector_type == SECT_DESERT
      || victim->in_room->sector_type == SECT_MOUNTAIN
      || victim->in_room->sector_type == SECT_HILLS
      || victim->in_room->sector_type == SECT_FIELD
      || victim->in_room->sector_type == SECT_UNDERWATER
      || victim->in_room->sector_type == SECT_ROAD)
    {
	sprintf(buf,"The plant life of %s acknowledges $N's presence.",
	  victim->in_room->area->name);
	act(buf,ch,NULL,victim,TO_CHAR);
	return TRUE;
    }
    else
    {
	act("The plants have nothing to say about $N.",ch,NULL,victim,TO_CHAR);
	return FALSE;
    }
}


bool spell_stampede( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *vch, *vch_next;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    act("You call upon nature for primal rage!", ch, NULL, NULL, TO_CHAR);
    act("$n calls upon nature for its primal rage!", ch, NULL, NULL, TO_ROOM);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = number_fuzzy(level/10);
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;

    for (vch = ch->in_room->people ; vch != NULL ; vch = vch_next)
    {
        vch_next = vch->next_in_room;
        if (IS_NPC(vch) && IS_SET(vch->act, ACT_ANIMAL) && (number_bits(3) != 0) && !IS_AFFECTED(vch, AFF_CHARM))
        {
            act("$n is infuriated with the rage of nature!", vch, NULL, NULL, TO_ROOM);
            affect_to_char(vch, &af);
	    add_event((void *) vch, 1, &event_stampede);
        }
    }
    return TRUE;
}

bool spell_skycall( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int dam, moonstate, moonmod;

    if (IS_SET(ch->in_room->room_flags, ROOM_INDOORS))
    {
        send_to_char("You must be outdoors to call upon the sky.\n\r", ch);
        return FALSE;
    }
    
    if (IS_SET(ch->in_room->room_flags, ROOM_DARK) ||  IS_SET(ch->in_room->room_flags, ROOM_UBERDARK) || IS_SET(ch->in_room->room_flags, ROOM_NOWEATHER))
    {
        send_to_char("You cannot call on the sky here.\n", ch);
        return FALSE;
    }

    if (ch->in_room->area->w_cur.cloud_cover <= 50)
    {
	if ((time_info.hour >= season_table[time_info.season].sun_down)
	  || (time_info.hour < season_table[time_info.season].sun_up))
	{
	    moonstate = get_moon_state(1, FALSE);
	    moonmod = get_moon_state(2, FALSE);
	    if (moonmod == MOON_FULL)
		level = level * 6 / 5;
	    else if ((moonmod == MOON_WAXING_GIBBOUS) || (moonmod == MOON_WANING_GIBBOUS))
		level = level * 11 / 10;
	    else if ((moonmod == MOON_WAXING_CRESCENT) || (moonmod == MOON_WANING_CRESCENT))
		level = level * 9 / 10;
	    else if (moonmod == MOON_NEW)
		level = level * 4 / 5;

	    if (moonstate == MOON_FULL)
	    {
	        act("You call on the full moon!", ch, NULL, NULL, TO_CHAR);
	        act("$n calls on the full moon!", ch, NULL, NULL, TO_ROOM);
	        for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
	        {
	            vch_next = vch->next_in_room;
	            if ( !is_safe_spell(ch, vch, TRUE)
	            && (IS_PK(ch, vch) || IS_NPC(ch) || IS_NPC(vch))
	            && !is_same_group(ch, vch)
	            && ch->in_room == vch->in_room)
	            {
	                if ((ch->fighting == NULL || vch->fighting == NULL) && !IS_NPC(vch))
	                {
	                    if (can_see(vch, ch))
        	            {
	                        sprintf( buf, "Help!  %s is attacking me!!", PERS(ch, vch));
        	                do_autoyell( vch, buf );
                	    }
	                    else
        	            {
                	        sprintf( buf, "Help! Someone is attacking me!");
                        	do_autoyell( vch, buf );
	                    }
        	        }

        	        if (is_affected(vch, gsn_moonray))
	                {
        	            act("$N is already confused by the moon.", ch, NULL, vch, TO_CHAR);
                	    if (vch->fighting == NULL && IS_AWAKE(vch))
                        	vch->fighting = ch;
	                    continue;
        	        }

                	if (saves_spell(level, ch, vch, DAM_OTHER))
	                {
        	            act("$n looks confused a moment, but shakes it off.", vch, NULL, NULL, TO_ROOM);
                	    act("You feel momentarily confused, but you shake it off.", vch, NULL, NULL, TO_CHAR);
	                    continue;
        	        }

	                af.where     = TO_AFFECTS;
	                af.type      = gsn_moonray;
        	        af.level     = level;
                	af.duration  = number_fuzzy(level / 5);
	                af.location  = APPLY_NONE;
        	        af.modifier  = 0;
                	af.bitvector = 0;
	                affect_to_char(vch, &af);

        	        if (vch->fighting == NULL && IS_AWAKE(vch))
	                    vch->fighting = ch;

	                act("$n looks confused as the moonray bathes $m in soft light.", vch, NULL, NULL, TO_ROOM);
        	        act("You feel confused as the light of the moon bathes you in soft light.", vch, NULL, NULL, TO_CHAR);

			add_event((void *) vch, 1, &event_moonray);
        	    }
	        }
	    }
	    else if ((moonstate == MOON_WAXING_GIBBOUS)
	    || (moonstate == MOON_WAXING_CRESCENT)
	    || (moonstate == MOON_WAXING_HALF))
	    {
	        send_to_char("You call on the moon for healing.\n\r", ch);
	        act("$n calls on the waxing moon!", ch, NULL, NULL, TO_ROOM);
	        for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
	        {
        	    vch_next = vch->next_in_room;
	            if (is_same_group(ch, vch))
        	    {
	                if(is_an_avatar(vch))
			            continue;
        			send_to_char("You feel better!\n\r", vch);
        	        vch->hit = UMIN(vch->max_hit, vch->hit + number_fuzzy(level + 5));
	            }
        	}
	    }
	    else if ((moonstate == MOON_WANING_GIBBOUS)
	     || (moonstate == MOON_WANING_HALF)
	     || (moonstate == MOON_WANING_CRESCENT))
	    {
	        act("You call on the waning moon!", ch, NULL, NULL, TO_CHAR);
	        act("$n calls on the waning moon!", ch, NULL, NULL, TO_ROOM);

	        for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
	        {
	            vch_next = vch->next_in_room;
	            if ( !is_safe_spell(ch, vch, TRUE)
	            && (IS_PK(ch, vch) || IS_NPC(ch) || IS_NPC(vch))
	            && !is_same_group(ch, vch)
	            && ch->in_room == vch->in_room)
        	    {
	                if ((ch->fighting == NULL || vch->fighting == NULL) && !IS_NPC(vch))
	                {
	                    if (can_see(vch, ch))
	                    {
	                        sprintf( buf, "Help!  %s is attacking me!", PERS(ch, vch));
	                        do_autoyell( vch, buf );
        	            }
                	    else
	                    {
        	                sprintf( buf, "Help! Someone is attacking me!");
                	        do_autoyell( vch, buf );
	                    }
        	        }

	                dam = (dice(level, 5));
	                if (saves_spell(level, ch, vch, DAM_OTHER))
	                    dam /= 2;
	                damage_old( ch, vch, dam, gsn_moonray, DAM_OTHER ,TRUE);
	            }
	        }
	    }
	    else if (moonstate == MOON_NEW)
	    {
	        act("You call on the new moon!", ch, NULL, NULL, TO_CHAR);
	        act("$n calls on the new moon!", ch, NULL, NULL, TO_ROOM);

        	for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
	        {
        	    vch_next = vch->next_in_room;
	            if ( !is_safe_spell(ch, vch, TRUE)
        	    && (IS_PK(ch, vch) || IS_NPC(ch) || IS_NPC(vch))
	            && !is_same_group(ch, vch)
        	    && ch->in_room == vch->in_room)
	            {
        	        if ((ch->fighting == NULL || vch->fighting == NULL) && !IS_NPC(vch))
	                {
        	            if (can_see(vch, ch))
                	    {
	                        sprintf( buf, "Help!  %s is attacking me!", PERS(ch, vch));
        	                do_autoyell( vch, buf );
                	    }
	                    else
        	            {
                	        sprintf( buf, "Help! Someone is attacking me!");
	                        do_autoyell( vch, buf );
        	            }
                	}

	                dam = number_fuzzy(level * 2);
        	        if (saves_spell(level, ch, vch, DAM_OTHER))
                	    dam /= 2;
                    expend_mana(vch, dam);
	                vch->mana = UMAX(0, vch->mana);
        	        send_to_char("You feel the moon pull at your mental reserves.\n\r", vch);
	                if (vch->fighting == NULL && IS_AWAKE(vch))
        	            vch->fighting = ch;
	            }
	        }
	    }
	}
	else
	{
	    dam = dice( level, 8 );
	    dam -= (dam * ch->in_room->area->w_cur.cloud_cover / 100);
	    if ( saves_spell( level, ch, victim, DAM_LIGHT ) )
	        dam /= 2;
	    if (victim->in_room != NULL && is_affected(victim,gsn_globedarkness))
	        dam /= 2;

	    act ("$n calls a beam of burning light down on $N!", ch, NULL, victim, TO_NOTVICT);
	    act ("You call down a beam of burning light on $N!", ch, NULL, victim, TO_CHAR);
	    act ("$n calls down a beam of burning light on you!", ch, NULL, victim, TO_VICT);
	    damage_old( ch, victim, dam, sn,DAM_LIGHT,TRUE);

	    if (IS_AFFECTED(victim, AFF_BLIND) || IS_SET(victim->imm_flags, IMM_BLIND) || saves_spell(level - 3, ch, victim, DAM_LIGHT)
	      || IS_OAFFECTED(victim, AFF_GHOST))
	        return TRUE;

	    af.where     = TO_AFFECTS;
	    af.type      = gsn_blindness;
	    af.level     = level - 3;
	    af.location  = APPLY_HITROLL;
	    af.modifier  = -4;
	    af.duration  = (level - 3)/6;
	    af.bitvector = AFF_BLIND;
	    affect_to_char( victim, &af );
	    send_to_char( "You are blinded by the searing sunlight!\n\r", victim );
	    act("$n appears to be blinded.",victim,NULL,NULL,TO_ROOM);
	}
    }
    else
    {
	dam = dice(level,5);
	int damtype=0;
	char *dm;
	if (ch->in_room->area->w_cur.lightning_str > 50)
	{
	    if (area_is_affected(ch->in_room->area,gsn_lightning_storm))
		dam = dam * 3 / 2;
	    damtype = DAM_LIGHTNING;
	    dm = str_dup("lightning strike");
	}
	else if (ch->in_room->area->w_cur.storm_str > 0
	  && ch->in_room->area->w_cur.precip_type == 1)
	{
	    if (area_is_affected(ch->in_room->area,gsn_icestorm))
		dam = dam * 3 / 2;
	    damtype = DAM_COLD;
	    dm = str_dup("pounding hail");
	}
	else if (ch->in_room->area->w_cur.temperature > 45)
	{
	    if (area_is_affected(ch->in_room->area,gsn_rainoffire))
		dam = dam * 3 / 2;
	    damtype = DAM_FIRE;
	    dm = str_dup("scorching heat");
	}
	else if (ch->in_room->area->w_cur.temperature < -10)
	{
	    if (area_is_affected(ch->in_room->area,gsn_icestorm))
		dam = dam * 3 / 2;
	    damtype = DAM_COLD;
	    dm = str_dup("freezing cold");

	}
	if (damtype > 0)
	    damage_new(ch,victim,dam,TYPE_HIT,damtype,TRUE,dm);
	else
	    send_to_char("The sky grants you no aid!\n\r",ch);
    }
    return TRUE;
}

bool spell_starglamour( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (!ch->in_room)
        return FALSE;

    if (((time_info.hour >= season_table[time_info.season].sun_up)
     && (time_info.hour < season_table[time_info.season].sun_down))
     || (ch->in_room->area->w_cur.cloud_cover == 100)
     || IS_SET(ch->in_room->room_flags, ROOM_INDOORS))
    {
        send_to_char("You cannot see the stars.\n\r", ch);
        return FALSE;
    }

    if (IS_AFFECTED(victim, AFF_FAERIE_FIRE))
    {
        act("$N is already glowing.", ch, NULL, victim, TO_CHAR);
        return FALSE;
    }

    if (saves_spell(level, ch, victim, DAM_OTHER))
    {
        act("$n begins to glow, but the light quickly fades.", victim, NULL, NULL, TO_ROOM);
	act("You begin to glow, but the light quickly fades.", victim, NULL, NULL, TO_CHAR);
	return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.modifier  = (level * 2);
    af.location  = APPLY_AC;
    af.bitvector = AFF_FAERIE_FIRE;
    affect_to_char(victim, &af);

    send_to_char("You are bathed in starlight, and begin to glow.\n\r", victim);
    act("$n is bathed in starlight, and begins to glow.", victim, NULL, NULL, TO_ROOM);

    return TRUE;
}

bool spell_stickstosnakes( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *vObj;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *pMob, *victim;
    int sCount, i;
    char arg[MAX_STRING_LENGTH];

    if (is_affected(ch, gsn_stickstosnakes))
    {
        send_to_char("You are not able to create more snakes yet.\n\r", ch);
        return FALSE;
    }

    target_name = one_argument(target_name, arg);

    if (!(vObj = get_obj_carry(ch, arg, ch)) || !vObj->carried_by || (vObj->carried_by != ch))
    {
        send_to_char("You are not carrying that.\n\r", ch);
        return FALSE;
    }

    if (target_name[0] == '\0')
    {
	if (!ch->fighting)
	{
            send_to_char("You are not fighting anyone.\n\r", ch);
            return FALSE;
	}
	else
	    victim = ch->fighting;
    }
    else
    {
	target_name = one_argument(target_name, arg);

	if (!(victim = get_char_room(ch, arg)))
        {
            send_to_char("You do not see them here.\n\r", ch);
            return FALSE;
        }
    }

    if (is_safe_spell(ch, victim, FALSE))
    {
        send_to_char("The gods protect them.\n\r", ch);
        send_to_char("The gods protect you.\n\r", victim);
        return FALSE;
    }

    if (str_cmp(material_table[vObj->material].name, "wood")
     && str_cmp(material_table[vObj->material].name, "maple")
     && str_cmp(material_table[vObj->material].name, "oak"))
    {
        send_to_char("That isn't made out of wood!\n\r", ch);
        return FALSE;
    }

    if (!can_drop_obj(ch, vObj))
    {
        send_to_char("You can't let go of it.\n\r", ch);
        return FALSE;
    }

    sCount = (vObj->weight / 100) + (ch->level / 15) + 1;

    act("You hurl $p to the ground!", ch, vObj, ch->fighting, TO_CHAR);
    act("$n hurls $p to the ground!", ch, vObj, ch->fighting, TO_ROOM);

    obj_from_char(vObj);
    obj_to_room(vObj, ch->in_room);

    if (!vObj)          /* sanity check */
        return FALSE;

    af.where    =  TO_AFFECTS;
    af.type     =  sn;
    af.level    =  ch->level;
    af.duration =  number_range(3,5);
    af.location =  0;
    af.modifier =  0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    af.duration =  ch->level / 5;

    if (sCount > 0)
        for (i = 0; i < sCount; i++)
        {
            pMob = create_mobile(get_mob_index(MOB_VNUM_SNAKE));
            if (pMob)   /* sanity check */
            {
                pMob->level = vObj->level;
                affect_to_char(pMob, &af);
                char_to_room(pMob, ch->in_room);
                act("A snake appears and quickly slithers towards $n!", ch->fighting, NULL, NULL, TO_ROOM);
                act("A snake appears and quickly slithers towards you!", ch->fighting, NULL, NULL, TO_CHAR);
                multi_hit(pMob, victim, TYPE_UNDEFINED);
            }
        }
    if (!IS_NPC(ch))
	ch->pcdata->adrenaline = IS_NPC(victim) ? UMAX(ch->pcdata->adrenaline, 2) : UMAX(ch->pcdata->adrenaline, 20);
    check_killer(ch,victim);

    extract_obj( vObj );
    return TRUE;
}



bool spell_tangle_trail( int sn, int level, CHAR_DATA *ch, void *vo, int target){
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int i, j, k, num_placed = 0;

    if (!ch->in_room)
        return FALSE;

    if (is_affected(ch, gsn_tangletrail))
    {
        send_to_char("You are not yet ready to tangle the forest again.\n\r", ch);
        return FALSE;
    }

    if (room_is_affected(ch->in_room, gsn_tangletrail))
    {
        send_to_char("This forest here is already tangled.\n\r", ch);
        return FALSE;
    }

    if ((ch->in_room->sector_type != SECT_FOREST) && (ch->in_room->sector_type != SECT_SWAMP))
    {
        send_to_char("You must be in a forest to tangle the trail.\n\r", ch);
        return FALSE;
    }

    if (room_is_affected(ch->in_room, gsn_plantgrowth))
    {
        send_to_char("You are not in a natural forest.\n\r", ch);
        return FALSE;
    }

    for (i = 0; i < 6; i++)
        ch->in_room->exit[i] = NULL;

    for (i = 0; i < 6; i++)
        if (ch->in_room->old_exit[i])
        {
            k = 0;
            for (j = 0; j < (6 - num_placed); j++)
            {
                if (ch->in_room->exit[k])
                    k++;

                if (number_range(1, (6 - num_placed - j)) == 1)
                {
                    ch->in_room->exit[k] = ch->in_room->old_exit[i];
                    num_placed++;
                    break;
                }
            }
        }

    for (i = 0; i < 6; i++)
        if (!ch->in_room->exit[i] && (number_percent() < 20))
        {
            ch->in_room->exit[i] = new_exit();
            ch->in_room->exit[i]->u1.to_room = ch->in_room;    /* ROM OLC */
            ch->in_room->exit[i]->orig_door = i;
            ch->in_room->exit[i]->rs_flags = EX_ILLUSION;
            ch->in_room->exit[i]->exit_info = EX_ILLUSION;
        }


    act("You call upon the powers of nature to tangle the surrounding forest.",
ch, NULL, NULL, TO_CHAR);
    act("$n calls upon the powers of nature, and the forest around you twists.", ch, NULL, NULL, TO_ROOM);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = (level / 2);
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = 0;
    affect_to_room(ch->in_room, &af);
    affect_to_char(ch, &af);

    return TRUE;
}


bool spell_thornspray( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;
    OBJ_DATA *sigil = NULL;
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;

    if (is_safe_spell(ch, victim, FALSE))
        return FALSE;
    
    dam = (dice(level, 2) + level/2 + 25);
    if (saves_spell(level, ch, victim, DAM_PIERCE))
        dam /= 2;
    sigil = get_eq_char(ch,WEAR_BODY);
    if (sigil && sigil->pIndexData->vnum == OBJ_VNUM_BLOOD_SIGIL)
    {
	act("You send a spray of thorns to pierce $N!", ch, NULL, victim, TO_CHAR);
	act("$n sends a spray of thorns to pierce $N!", ch, NULL, victim, TO_VICTROOM);
	act("$n sends a spray of thorns to pierce you!", ch, NULL, victim, TO_VICT);
	dam = dam * 6 / 5;
	damage( ch, victim, dam, sn, DAM_PIERCE, TRUE);
	if (victim->in_room && !IS_OAFFECTED(victim,AFF_GHOST) 
	  && !IS_OAFFECTED(victim,AFF_BLEEDING))
	{
	    af.type = sn;
	    af.level = level;
	    af.where = TO_OAFFECTS;
	    af.location = APPLY_NONE;
	    af.modifier = 0;
	    af.bitvector = AFF_BLEEDING;
	    af.duration = level/12;
	    affect_to_char(victim,&af);
	    act("The thorns dig into $N's skin.",ch,NULL,victim,TO_CHAR);
	    act("The thorns dig into your skin.",ch,NULL,victim,TO_VICT);
	}		
    }
    else
    {
	act("You send a spray of thorns at $N!", ch, NULL, victim, TO_CHAR);
	act("$n sends a spray of thorns at $N!", ch, NULL, victim, TO_VICTROOM);
	act("$n sends a spray of thorns at you!", ch, NULL, victim, TO_VICT);
	damage( ch, victim, dam, sn, DAM_PIERCE, TRUE);
    }

    return TRUE;
}


bool spell_wallofvines( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    char buf[MAX_STRING_LENGTH];
    int dir, i;

    if (!ch->in_room)
        return FALSE;

    if (is_affected(ch, gsn_wallofvines))
    {
        send_to_char("You cannot summon another wall of vines yet.\n", ch);
        return FALSE;
    }

    if ((ch->in_room->sector_type == SECT_INSIDE)
     || (ch->in_room->sector_type == SECT_CITY)
     || (ch->in_room->sector_type == SECT_WATER_SWIM)
     || (ch->in_room->sector_type == SECT_WATER_NOSWIM)
     || (ch->in_room->sector_type == SECT_AIR)
     || (ch->in_room->sector_type == SECT_UNDERWATER))
    {
        send_to_char("You cannot summon a wall of vines here.\n\r", ch);
        return FALSE;
    }

    if (target_name[0] == '\0')
    {
	i = 0;
	do
	{
	    i++;
	    dir = number_door();;
	} while(ch->in_room->exit[dir] == NULL && i < 30);
    }
    else
    {
	if      (!str_prefix(target_name, "north")) dir = 0;
	else if (!str_prefix(target_name, "east") ) dir = 1;
	else if (!str_prefix(target_name, "south")) dir = 2;
	else if (!str_prefix(target_name, "west") ) dir = 3;
	else if (!str_prefix(target_name, "up")   ) dir = 4;
	else if (!str_prefix(target_name, "down") ) dir = 5;
	else
	{
            send_to_char("That is not a valid direction.\n\r", ch);
            return FALSE;
	}
    }
    if (!ch->in_room->exit[dir])
    {
        send_to_char("You cannot grow a wall of vines there!\n\r", ch);
        return FALSE;
    }

    if (IS_SET(ch->in_room->exit[dir]->exit_info, EX_WALLOFVINES))
    {
        send_to_char("There is already a wall of vines blocking that direction.\n\r", ch);
        return FALSE;
    }

    SET_BIT(ch->in_room->exit[dir]->exit_info, EX_WALLOFVINES);
    
    if (ch->in_room->exit[dir]->u1.to_room->exit[OPPOSITE(dir)]
     && (ch->in_room->exit[dir]->u1.to_room->exit[OPPOSITE(dir)]->u1.to_room
        == ch->in_room))
    {
        SET_BIT(ch->in_room->exit[dir]->u1.to_room->exit[OPPOSITE(dir)]->exit_info, EX_WALLOFVINES);

	if (ch->in_room->exit[dir]->u1.to_room->people)
	{
	    sprintf(buf, "A wall of vines rises %s.",
		((OPPOSITE(dir) == 0) ? "to the north" : (OPPOSITE(dir) == 1) ? "to the east" :
		 (OPPOSITE(dir) == 2) ? "to the south" : (OPPOSITE(dir) == 3) ? "to the west" :
		 (OPPOSITE(dir) == 4) ? "above you" : "below you"));

	    act(buf, ch->in_room->exit[dir]->u1.to_room->people, NULL, NULL, TO_ROOM);
	    act(buf, ch->in_room->exit[dir]->u1.to_room->people, NULL, NULL, TO_CHAR);
	}
    }

    sprintf(buf, "A wall of vines rises %s.",
	((dir == 0) ? "to the north" : (dir == 1) ? "to the east" :
	 (dir == 2) ? "to the south" : (dir == 3) ? "to the west" :
	 (dir == 4) ? "above you" : "below you"));

    act(buf, ch, NULL, NULL, TO_ROOM);
    act(buf, ch, NULL, NULL, TO_CHAR);

    af.where     = TO_AFFECTS;
    af.type      = gsn_wallofvines;
    af.level     = level;
    af.duration  = 24;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    af.duration  = (level / 6);
    af.modifier  = dir;
    affect_to_room(ch->in_room, &af);

    return TRUE;
}

bool spell_warpwood( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    extern char *const dir_name [];
    extern const int rev_dir [];
    OBJ_DATA *vObj;
    EXIT_DATA *pexit;
    CHAR_DATA *victim = NULL;
    char buf[MAX_STRING_LENGTH];
    int door, x = 0;
    bool oFound = FALSE;

    if ((target_name[0] == '\0') || (victim = get_char_room(ch, target_name)))
    {
	//FIXME: Sort of cheesy (inefficient ifs), oh well -- Seb
        if ((target_name[0] == '\0') && !ch->fighting)
        {
            send_to_char("Who or what do you wish to target?\n\r", ch);
            return FALSE;
        }

	//Fix by Seb
	if(!victim && ch->fighting)
	{
		victim = ch->fighting;
	}

        if (!IS_NPC(victim) && is_safe_spell(victim, ch, FALSE))
        {
            act("The gods protect $n.", victim, NULL, NULL, TO_ROOM);
            act("The gods protect you.", victim, NULL, NULL, TO_CHAR);
            return FALSE;
        }

        sprintf(buf, "Help!  %s is attacking me!", PERS(ch, victim));
        do_autoyell(victim, buf);

        for (vObj = victim->carrying; vObj; vObj = vObj->next_content, x++)
        {
            if (vObj->worn_on && (number_bits(1) == 0) && (!str_cmp(material_table[vObj->material].name, "wood") || !str_cmp(material_table[vObj->material].name, "maple") || !str_cmp(material_table[vObj->material].name, "oak")))
            {
                oFound = TRUE;
                break;
            }

            if (x == 20)
                break;
        }

        if (!oFound)
        {
            send_to_char("You failed.\n\r", ch);
            return TRUE;
        }

        if (saves_spell(level, ch, victim, DAM_OTHER) || IS_SET(vObj->extra_flags[0], ITEM_NODESTROY) || (number_bits(2) == 0))
        {
            act("You attempt to warp $N's gear, but fail.", ch, NULL, victim, TO_CHAR);
            act("You feel $p begin to tremble, but it subsides.", ch, vObj, victim, TO_VICT);
            return TRUE;
        }

        act("$p splinters and breaks into many pieces.", ch, vObj, NULL, TO_CHAR);
        act("$p splinters and breaks into many pieces.", ch, vObj, NULL, TO_ROOM);
        extract_obj(vObj);

        if (!victim->fighting)
            multi_hit(victim, ch, TYPE_UNDEFINED);

    }
    else
    {
        if ((door = find_door(ch, target_name)) < 0)    /* Invalid direction */
        {
            send_to_char("That is not a valid direction.\n\r", ch);
            return FALSE;
        }

        pexit = ch->in_room->exit[door];

        if (!IS_SET(pexit->exit_info, EX_CLOSED))
        {
            send_to_char("It's not closed.\n\r", ch);
            return FALSE;
        }

        if (IS_SET(pexit->exit_info, EX_RUNEOFEARTH))
        {
            act("The rune of earth on the door flares, absorbing $n's magic.", ch, NULL, NULL, TO_ROOM);
            send_to_char("The rune of earth on the door flares, absorbing your magic.\n\r", ch);
            return TRUE;
        }

        if (IS_SET(pexit->exit_info, EX_PICKPROOF) || IS_SET(pexit->exit_info, EX_NORAM))
        {
            send_to_char("You failed.\n\r", ch);
            return TRUE;
        }

        sprintf(buf, "The door to the %s cracks and bends, opening wide.",
            dir_name[door]);
        act(buf, ch, NULL, NULL, TO_CHAR);
        act(buf, ch, NULL, NULL, TO_ROOM);

        REMOVE_BIT(pexit->exit_info, EX_CLOSED);

        if (pexit->u1.to_room
         && pexit->u1.to_room->exit[rev_dir[door]]
         && (pexit->u1.to_room->exit[rev_dir[door]]->u1.to_room == ch->in_room))            REMOVE_BIT(pexit->u1.to_room->exit[rev_dir[door]]->exit_info, EX_CLOSED);

    }

    return TRUE;
}


bool spell_wolfform( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

        if (is_affected(ch, gsn_wolfform))
        {
        send_to_char("You are already affected with the aspect of the wolf.\n\r", ch);
        return FALSE;
        }

        if (is_affected(ch, gsn_hawkform))
        {
        act("You discard the aspect of the hawk as you seek the aspect of the wolf.", ch, NULL, NULL, TO_CHAR);
        affect_strip(ch, gsn_hawkform);
        }

        if (is_affected(ch, gsn_bearform))
        {
        act("You discard the aspect of the bear as you seek the aspect of the wolf.", ch, NULL, NULL, TO_CHAR);
        affect_strip(ch, gsn_bearform);
        }

        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = level;
        af.duration  = level/2;
        af.location  = APPLY_HIT;
        af.modifier  = level;
        af.bitvector = 0;
        affect_to_char(ch, &af);

		// Also modify the hp
		ch->hit = UMAX(1, ch->hit + af.modifier);

        af.bitvector = 0;
        af.location  = APPLY_DAMROLL;
        af.modifier  = level/10;
        affect_to_char(ch, &af);

        af.location  = APPLY_HITROLL;
        af.modifier  = level/10;
        affect_to_char(ch, &af);

        act("You feel more perceptive as you assume the aspect of the wolf.", ch, NULL, NULL, TO_CHAR);
        act("$n looks more perceptive as $e assumes the aspect of the wolf.", ch, NULL, NULL, TO_ROOM);
    return TRUE;
}

void do_applybarbs(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *arrow;
    int skill = get_skill(ch,gsn_arrowcraft);
    int i;
    int chance=0;
    AFFECT_DATA *paf;

    if (skill == 0)
    {
	send_to_char("You don't know how to properly apply barbs to an arrow.\n\r",ch);
	return;
    }

    if (argument[0]=='\0')
    {
	send_to_char("What arrow did you want to apply barbs to?\n\r",ch);
	return;
    }
    
    if ((arrow = get_obj_carry(ch,argument,ch)) == NULL)
    {
	send_to_char("You don't have that arrow.\n\r",ch);
	return;
    }

    if (arrow->item_type != ITEM_ARROW
      || arrow->pIndexData->vnum != OBJ_VNUM_RANGER_ARROW)
    {
	send_to_char("You can't apply barbs to that.\n\r",ch);
	return;
    }

    if (ch->mana - skill_table[gsn_arrowcraft].min_mana < 0)
    {
	send_to_char("You are too tired to apply barbs to an arrow.\n\r",ch);
	return;
    }

// first, multiple thing arrows are harder to do stuff to
    for (i=2;i<5;i++)
	if (arrow->value[i] > 0)
	    if (arrow->value[i] == gsn_barbs)
	    {
		send_to_char("This arrow is already barbed.\n\r",ch);
		return;
	    }
	    else
	        chance -= 5;

    for (paf = arrow->affected; paf != NULL; paf = paf->next)
	if (paf->location == APPLY_RANGE && paf->modifier > 1)
	    chance -= 5;

// next,try to find an empty slot to add the affect to
    for (i=2;i<5;i++)
	if (arrow->value[i] == 0)
	    break;

    if (i==5)
    {
	send_to_char("You can't apply barbs to this arrow.\n\r",ch);
	return;
    }
    expend_mana(ch, skill_table[gsn_arrowcraft].min_mana);

    if (number_percent() < skill+chance)
    {
        act("You skillfully apply barbs to $p.",ch,arrow,NULL,TO_CHAR);
	act("$n skillfully applies barbs to $p.",ch,arrow,NULL,TO_ROOM);
        arrow->value[i] = gsn_barbs;
        check_improve(ch,NULL,gsn_arrowcraft,TRUE,1);
	WAIT_STATE(ch,UMAX(ch->wait,skill_table[gsn_arrowcraft].beats));
        return;    	    
    }
    act("You break $p during your attempt to apply barbs to it.",ch,arrow,NULL,TO_CHAR);
    act("$n breaks $p during $s attempt to apply barbs to it.",ch,arrow,NULL,TO_ROOM);
    damage(ch,ch,number_range(1,10),gsn_barbs,DAM_PIERCE,TRUE);
    extract_obj(arrow);
    check_improve(ch,NULL,gsn_arrowcraft,FALSE,1);
    WAIT_STATE(ch,UMAX(ch->wait,skill_table[gsn_arrowcraft].beats));
    return;
}

void do_fletch(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *arrow;
    int skill = get_skill(ch,gsn_fletch);
    int i;
    int chance=0;
    AFFECT_DATA *paf;

    if (skill == 0)
    {
	send_to_char("You don't know how to properly fletch an arrow.\n\r",ch);
	return;
    }

    if (argument[0]=='\0')
    {
	send_to_char("What arrow did you want to fletch?\n\r",ch);
	return;
    }
    
    if ((arrow = get_obj_carry(ch,argument,ch)) == NULL)
    {
	send_to_char("You don't have that arrow.\n\r",ch);
	return;
    }

    if (arrow->item_type != ITEM_ARROW
      || arrow->pIndexData->vnum != OBJ_VNUM_RANGER_ARROW)
    {
	send_to_char("You can't fletch that.\n\r",ch);
	return;
    }
   
     if (ch->mana - skill_table[gsn_fletch].min_mana < 0)
    {
	send_to_char("You are too tired to fletch that.\n\r",ch);
	return;
    }

// first, multiple thing arrows are harder to do stuff to
    for (i=2;i<5;i++)
	if (arrow->value[i] > 0)
	    chance -= 10;

    for (paf = arrow->affected; paf != NULL; paf = paf->next)
	if (paf->location == APPLY_RANGE)
	    if (paf->modifier > 1)
	    {
	    	send_to_char("This arrow is already fletched.\n\r",ch);
	    	return;
	    }
	    else
		break;

    expend_mana(ch, skill_table[gsn_fletch].min_mana);

    if (number_percent() < skill+chance)
    {
        act("You skillfully fletch $p.",ch,arrow,NULL,TO_CHAR);
	act("$n skillfully fletches $p.",ch,arrow,NULL,TO_ROOM);
	if (paf != NULL)
	    paf->modifier += ch->level/13;
	else
	{
	    AFFECT_DATA af;
	    af.point = NULL;
	    af.valid = TRUE;
	    af.where = TO_OBJECT;
	    af.type = gsn_fletch;
	    af.level = ch->level;
	    af.duration = -1;
	    af.location = APPLY_RANGE;
	    af.modifier = ch->level/13;
	    af.bitvector = 0;
	    obj_affect_join(arrow,&af);
	}	    
        check_improve(ch,NULL,gsn_fletch,TRUE,1);
	WAIT_STATE(ch,UMAX(ch->wait,skill_table[gsn_fletch].beats));
        return;    	    
    }
    act("You break $p during your attempt to fletch it.",ch,arrow,NULL,TO_CHAR);
    act("$n breaks $p during $s attempt to fletch it.",ch,arrow,NULL,TO_ROOM);
    extract_obj(arrow);
    check_improve(ch,NULL,gsn_fletch,FALSE,1);
    WAIT_STATE(ch,UMAX(ch->wait,skill_table[gsn_fletch].beats));
    return;
}

void do_applypoison(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *arrow;
    int skill = get_skill(ch,gsn_arrowcraft);
    int i;
    int chance=0;
    AFFECT_DATA *paf;

    if (skill == 0)
    {
	send_to_char("You don't know how to properly apply poison to an arrow.\n\r",ch);
	return;
    }

    if (argument[0]=='\0')
    {
	send_to_char("What arrow did you want to apply poison to?\n\r",ch);
	return;
    }
    
    if ((arrow = get_obj_carry(ch,argument,ch)) == NULL)
    {
	send_to_char("You don't have that arrow.\n\r",ch);
	return;
    }

    if (arrow->item_type != ITEM_ARROW
      || arrow->pIndexData->vnum != OBJ_VNUM_RANGER_ARROW)
    {
	send_to_char("You can't apply poison to that.\n\r",ch);
	return;
    }

    if (ch->mana - skill_table[gsn_arrowcraft].min_mana < 0)
    {
	send_to_char("You are too tired to apply poison to an arrow.\n\r",ch);
	return;
    }

// first, multiple thing arrows are harder to do stuff to
    for (i=2;i<5;i++)
	if (arrow->value[i] > 0)
	    if (arrow->value[i] == gsn_poison)
	    {
		send_to_char("This arrow is already poisoned.\n\r",ch);
		return;
	    }
	    else
	        chance -= 5;

    for (paf = arrow->affected; paf != NULL; paf = paf->next)
	if (paf->location == APPLY_RANGE && paf->modifier > 1)
	    chance -= 5;

// next,try to find an empty slot to add the affect to
    for (i=2;i<5;i++)
	if (arrow->value[i] == 0)
	    break;

    if (i==5)
    {
	send_to_char("You can't apply poison to this arrow.\n\r",ch);
	return;
    }
    expend_mana(ch, skill_table[gsn_arrowcraft].min_mana);

    if (number_percent() < (skill+chance))
    {
        act("You skillfully apply poison to $p.",ch,arrow,NULL,TO_CHAR);
	act("$n skillfully applies poison to $p.",ch,arrow,NULL,TO_ROOM);
        arrow->value[i] = gsn_poison;
        check_improve(ch,NULL,gsn_arrowcraft,TRUE,1);
	WAIT_STATE(ch,UMAX(ch->wait,skill_table[gsn_arrowcraft].beats));
        return;    	    
    }
    act("You break $p during your attempt to poison it.",ch,arrow,NULL,TO_CHAR);
    act("$n breaks $p during $s attempt to poison it.",ch,arrow,NULL,TO_ROOM);
    if (!saves_spell(ch->level,NULL, ch,DAM_POISON))
    {
	if (is_affected(ch,gsn_protectionfrompoison))
	    send_to_char("You feel strange for a moment, but your protection quickly flushes the poison from your system.\n\r", ch);
    else if (number_percent() <= get_skill(ch, gsn_detoxify))
    {
        check_improve(ch, NULL, gsn_detoxify, true, 4);
	    send_to_char("You feel ill for a moment, but the toxins are quickly flushed from your system.\n\r", ch);
    }
	else
	{
        check_improve(ch, NULL, gsn_detoxify, false, 4);
        send_to_char("You feel terrible.\n\r",ch);
	    act("$n looks sick.", ch, NULL, NULL,TO_ROOM);
	    AFFECT_DATA af;
	    af.valid = TRUE;
	    af.point = NULL;
	    af.where = TO_AFFECTS;
	    af.type = gsn_poison;
	    af.level = ch->level;
	    af.duration = number_bits(2);
	    af.location = APPLY_STR;
	    af.modifier = -2;
	    af.bitvector = AFF_POISON;
	    affect_to_char(ch,&af);
	}
    }
    else
    {
	send_to_char("You wipe the poison off your hands.\n\r",ch);
	act("$n wipes poison off $s hands.",ch,NULL,NULL,TO_ROOM);
    }
    extract_obj(arrow);
    check_improve(ch,NULL,gsn_arrowcraft,FALSE,1);
    WAIT_STATE(ch,UMAX(ch->wait,skill_table[gsn_arrowcraft].beats));
    return;
}

struct color_type
{
    char *name;
};

const struct color_type color_table[] = 
{
	{""},
    	{"red"},
    	{"orange"},
    	{"yellow"},
	{"green"},
	{"blue"},
	{"indigo"},
	{"violet"},
	{"black"},
	{"brown"},
	{"white"},
	{NULL}
};

void do_bandarrow(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *arrow;
    char *color;
    int i=0;
    char buf[MAX_STRING_LENGTH];

    if (ch->class_num != global_int_class_ranger)
    {
	send_to_char("Huh?\n\r",ch);
	return;
    }

    if (argument[0]=='\0')
    {
	send_to_char("What arrow did you want to band?\n\r",ch);
	return;
    }
    
    char target_arrow[MAX_INPUT_LENGTH];
    color = one_argument(argument,target_arrow);
    
    if (target_arrow[0] == '\0')
    {
	send_to_char("What arrow did you want to band?\n\r",ch);
        return;
    }

    if (color[0] == '\0')
    {
	send_to_char("What color did you want to band your arrow?\n\r",ch);
	return;
    }
    if ((arrow = get_obj_carry(ch,target_arrow,ch)) == NULL)
    {
	send_to_char("You don't have that arrow.\n\r",ch);
	return;
    }

    if (arrow->item_type != ITEM_ARROW
      || arrow->pIndexData->vnum != OBJ_VNUM_RANGER_ARROW)
    {
	send_to_char("You can't band that.\n\r",ch);
	return;
    }
    do
    {
	if (!str_cmp(color_table[i].name,color))
	    break;
	i++;
    } while (color_table[i].name);

    if (!color_table[i].name)
    {
	send_to_char("Valid arrow colors:",ch);
	i=0;
	while (color_table[i++].name)
	{
	    send_to_char(" ",ch);
	    send_to_char(color_table[i].name,ch);
	}
	send_to_char("\n\r",ch);
	return;
    }
    
    sprintf(buf,"You paint a%s %s band on $p.",
      leading_vowel(color_table[i].name) ? "n" : "",color_table[i].name);
    act(buf,ch,arrow,NULL,TO_CHAR);
    sprintf(buf,"$n paints a%s %s band on $p.",
      leading_vowel(color_table[i].name) ? "n" : "",color_table[i].name);
    act(buf,ch,arrow,NULL,TO_ROOM);
    sprintf(buf,"%s %s-banded %s banded",arrow->pIndexData->name,color_table[i].name,color_table[i].name);
    setName(*arrow, buf);
    sprintf(buf,"a%s %s-banded handmade arrow",
      leading_vowel(color_table[i].name) ? "n" : "",color_table[i].name);
    free_string(arrow->short_descr);
    arrow->short_descr = str_dup(buf);
    sprintf(buf,"A%s %s-banded handmade arrow lies on the ground.",
      leading_vowel(color_table[i].name) ? "n" : "",color_table[i].name);
    free_string(arrow->description);
    arrow->description = str_dup(buf);
}

void do_huntersense(CHAR_DATA *ch, char *argument)
{
// Start cost 25, maint 25 if all or 15 if targetted
// Syntax: huntersense (no argument):
//	if not affected, turns on huntersense
//	if affected, shows you whether you are hunting all or specific tracks
// huntersense (name):
//	if not affected, turns on huntersense if tracks are in room
//	if affected, turns on huntersense if tracks are in room
// huntersense stop
// 	if not affected, you can't stop. Idiot.
//	if affected, turns off huntersense

    AFFECT_DATA af;
    AFFECT_DATA *paf;
    TRACK_DATA *pTrack;
    int skill = get_skill(ch,gsn_huntersense);
    char buf[MAX_STRING_LENGTH];
        
    if (skill == 0)
    {
	send_to_char("Huh?\n\r",ch);
	return;
    }

    if (ch->in_room
      && (ch->in_room->sector_type == SECT_INSIDE
	|| ch->in_room->sector_type == SECT_CITY)
      && str_cmp(argument,"stop"))
    {
	send_to_char("You can't look for tracks here.\n\r",ch);
	return;
    }

    if (argument[0]=='\0')
    {
	if ((paf = affect_find(ch->affected,gsn_huntersense)) == NULL)
	{
	    if (ch->mana - skill_table[gsn_huntersense].min_mana < 0)
	    {
		send_to_char("You are too tired to hunt for tracks.\n\r",ch);
		return;
	    }
	    act("You begin searching for tracks.",ch,NULL,NULL,TO_CHAR);
	    act("$n begins searching for tracks.",ch,NULL,NULL,TO_ROOM);
	    expend_mana(ch, 25);
	    af.valid = TRUE;
	    af.point = NULL;
	    af.where = TO_AFFECTS;
	    af.modifier = 0;
	    af.duration = -1;
	    af.type = gsn_huntersense;
	    af.bitvector = 0;
	    af.location = APPLY_NONE;
	    affect_to_char(ch,&af);
	    WAIT_STATE(ch,UMAX(ch->wait,skill_table[gsn_huntersense].beats));
	    return;
	}
	sprintf(buf,"You are looking for %s%stracks.\n\r",(char *)(paf->point) ? (char *)(paf->point) : "all",(char *)(paf->point) ? "'s " : " ");
	send_to_char(buf,ch);
	return;
    }
    else if (!str_cmp(argument,"stop"))
    {
	if (is_affected(ch,gsn_huntersense))
	{
	    act("You stop looking for tracks.",ch,NULL,NULL,TO_CHAR);
	    act("$n stops looking for tracks.",ch,NULL,NULL,TO_ROOM);
	    affect_strip(ch,gsn_huntersense);
	    return;
	}
	else
	{
	    send_to_char("You weren't looking for tracks.\n\r",ch);
	    return;
	}
    }
    else // we think we are looking for someone
    {
	if (!ch->in_room || !ch->in_room->tracks)
	{
	    send_to_char("You can't find any tracks here.\n\r",ch);
	    return;
	}

	if ((paf = affect_find(ch->affected,gsn_huntersense)) == NULL)
	{
	    send_to_char("You can't follow tracks you can't see.\n\r",ch);
	    return;
	}
	if (ch->mana - 15 < 0)
	{
	    send_to_char("You are too tired to focus on tracks.\n\r",ch);
	    return;
	}
	for (pTrack = ch->in_room->tracks;pTrack != NULL;pTrack = pTrack->next)
	{
	    if (!pTrack->valid)
		continue;
	    if (!str_prefix(argument,pTrack->ch->name))
		break;
	}
	if (!pTrack)
	{
	    send_to_char("You don't sense their trail here.\n\r",ch);
	    return;
	}
	else
	{
	    expend_mana(ch, 15);
	    paf->point = (void *)str_dup(pTrack->ch->name);
	    sprintf(buf,"You begin focusing on %s's tracks.\n\r",(char*)(paf->point));
	    WAIT_STATE(ch,UMAX(ch->wait,skill_table[gsn_huntersense].beats));
	    send_to_char(buf,ch);
	    return;
	}
    }
}

void do_pounce( CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    int skill = get_skill(ch,gsn_pounce);

    if (skill == 0)
    {
	send_to_char("Huh?\n\r",ch);
	return;
    }
    if (ch->in_room->sector_type == SECT_AIR
      || ch->in_room->sector_type == SECT_WATER_SWIM
      || ch->in_room->sector_type == SECT_WATER_NOSWIM
      || (ch->in_room->sector_type == SECT_UNDERWATER
	&& !is_affected(ch,gsn_aquamove)))
    {
	send_to_char("You can't pounce someone here.\n\r",ch);
	return;
    }

    if (argument[0]=='\0')
    {
        if((victim=ch->fighting) == NULL)
	{
	    send_to_char("Who did you want to pounce at?\n\r",ch);
	    return;
	}
    }
    else if ((victim = get_char_room(ch,argument))==NULL)
    {
    	send_to_char("You don't see them here.\n\r",ch);
	return;
    }
    
    if (victim == ch)
    {
	send_to_char("You chase yourself around a bit, but never catch yourself.\n\r",ch);
	return;
    }
    if (is_safe(ch,victim))
	return;
 
    skill -= get_skill(victim,gsn_dodge)/5;
    if (ch->in_room->sector_type == SECT_MOUNTAIN
      || ch->in_room->sector_type == SECT_HILLS)
    {
	skill+=15;
    }
    if (IS_AFFECTED(victim,AFF_HASTE) || IS_SET(victim->off_flags,OFF_FAST))
	skill -= 30;
    if (IS_AFFECTED(ch,AFF_HASTE) || IS_SET(ch->off_flags,OFF_FAST))
	skill += 10;
    skill += (ch->level - victim->level);
    skill += (ch->size - victim->size) * 10;
    if (number_percent() > skill)
    {
	act("You pounce at $N and miss.",ch,NULL,victim,TO_CHAR);
	act("$n pounces at $N and misses.",ch,NULL,victim,TO_NOTVICT);
	act("$n pounces at you and misses.",ch,NULL,victim,TO_VICT);
	damage(ch,victim,0,gsn_pounce,DAM_OTHER,FALSE);
        WAIT_STATE(ch,UMAX(ch->wait,skill_table[gsn_pounce].beats));
	check_improve(ch,victim,gsn_pounce,FALSE,1);
	return;
    }
    act("You leap forward, pouncing at $N.",ch,NULL,victim,TO_CHAR);
    act("$n leaps forward, pouncing at $N.",ch,NULL,victim,TO_NOTVICT);
    act("$n leaps forward, pouncing at you.",ch,NULL,victim,TO_VICT);
    WAIT_STATE(ch,UMAX(ch->wait,skill_table[gsn_pounce].beats));
    check_improve(ch,victim,gsn_pounce,TRUE,1);
    one_hit(ch,victim,gsn_pounce,HIT_PRIMARY, false);
}

void do_predatoryattack( CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *wield;
    CHAR_DATA *victim;
    int skill = get_skill(ch,gsn_predatoryattack);

    if (skill == 0)
    {
	send_to_char("Huh?\n\r",ch);
	return;
    }

    if (argument[0]=='\0')
    {
        if((victim=ch->fighting) == NULL)
	{
	    send_to_char("Who did you want to attack?\n\r",ch);
	    return;
	}
    }
    else if ((victim = get_char_room(ch,argument))==NULL)
    {
    	send_to_char("You don't see them here.\n\r",ch);
	return;
    }
    
    if (victim == ch)
    {
	send_to_char("You inspect your wounds, but fail to find a way to capitalize on them for the kill.\n\r",ch);
	return;
    }

    wield = get_eq_char(ch,WEAR_WIELD);
    if (is_safe(ch,victim))
	return;
 
    skill -= get_skill(victim,gsn_dodge)/5;
    if (IS_AFFECTED(victim,AFF_HASTE) || IS_SET(victim->off_flags,OFF_FAST))
	skill -= 30;
    if (IS_AFFECTED(ch,AFF_HASTE) || IS_SET(ch->off_flags,OFF_FAST))
	skill += 10;
    skill += (ch->level - victim->level);
    skill += (ch->size - victim->size) * 10;
    if (number_percent() > skill)
    {
	act("You start to circle $N, but fumble and miss your attack.",ch,NULL,victim,TO_CHAR);
	act("$n starts to circle $N, but fumbles and misses $s attack.",ch,NULL,victim,TO_NOTVICT);
	act("$n starts to circle you, but fumbles and misses $s attack.",ch,NULL,victim,TO_VICT);
	damage(ch,victim,0,gsn_predatoryattack,DAM_OTHER,FALSE);
        WAIT_STATE(ch,UMAX(ch->wait,skill_table[gsn_predatoryattack].beats));
	check_improve(ch,victim,gsn_predatoryattack,FALSE,1);
	return;
    }
    act("You start circling $N, quickly closing in for a vicious attack.",ch,NULL,victim,TO_CHAR);
    act("$n starts circling $N, quickly closing in for a vicious attack.",ch,NULL,victim,TO_NOTVICT);
    act("$n starts circling you, quickly closing in for a vicious attack.",ch,NULL,victim,TO_VICT);
    WAIT_STATE(ch,UMAX(ch->wait,skill_table[gsn_predatoryattack].beats));
    check_improve(ch,victim,gsn_predatoryattack,TRUE,1);
    one_hit(ch,victim,gsn_predatoryattack,HIT_PRIMARY, false);
}

struct YUM {
    char *nomnom;
};
struct YUM yum[10] = 
{
    {"burned"}, {"overcooked"}, {"undercooked"}, {"scorched"}, {"cooked"},
    {"roasted"}, {"seared"}, {"sizzling"}, {"roasted"}, {"flame-broiled"}
};

void do_cook( CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *corpse, *food, *fire;
    char buf[MAX_STRING_LENGTH],arg[MAX_STRING_LENGTH];
    char *text;
    int skill = get_skill(ch,gsn_cook),degree;
    int number,i;
    int percent;

    if (skill == 0)
    {
	send_to_char("Huh?\n\r",ch);
	return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("What did you want to cook?\n\r",ch);
	return;
    }
 
    corpse = get_obj_list(ch,argument,ch->in_room->contents);
    if (!corpse)
	corpse = get_obj_list(ch,"pccorpse",ch->in_room->contents);
    if (!corpse)
	corpse = get_obj_list(ch,"npccorpse",ch->in_room->contents);
    if (!corpse)
    {
	send_to_char("There is no corpse here to carve meat from.\n\r",ch);
	return;
    }

    if ( corpse->pIndexData->vnum != OBJ_VNUM_CORPSE_NPC
      && corpse->pIndexData->vnum != OBJ_VNUM_CORPSE_PC)
    {
	send_to_char("There is no corpse here to carve meat from.\n\r",ch);
	return;
    }

    if (IS_SET(corpse->value[1],CORPSE_DESTROYED))
    {
	send_to_char("That corpse is not fit for cooking.\n\r",ch);
	return;
    }

    fire = get_obj_list(ch,"campfire",ch->in_room->contents);

    if (!fire)
    {
	send_to_char("There is no campfire here to cook meat over.\n\r",ch);
	return;
    }

    percent = number_percent();
    if (percent<skill)
    {
	sprintf(buf,"You carve $p into edible chunks, and cook them over %s.",fire->short_descr);
	act(buf,ch,corpse,NULL,TO_CHAR);
	sprintf(buf,"$n carves $p into edible chunks, and cooks them over %s.",fire->short_descr);
	act(buf,ch,corpse,NULL,TO_ROOM);
// delicious meat
	number = 1;
	if (corpse->size > SIZE_SMALL)
	    number += 1;
	if (corpse->size > SIZE_HUGE)
	    number += 1;
        text = one_argument(corpse->short_descr,arg); // corpse
	text = one_argument(text,arg);	// of
	text = one_argument(text,arg);
	for (i=0;i<number;i++)
	{
	    food = create_object(get_obj_index(OBJ_VNUM_MMM_MEAT),corpse->level);
	    degree = number_range(0,7)+skill/25-1;
	    if (degree > 9)
		degree = 9;
	    if (degree < 0)
		degree = 0;
	    sprintf(buf,"%s %s %s",food->name,text,yum[degree].nomnom);
        setName(*food, buf);
	    sprintf(buf,"a chunk of %s meat from %s",yum[degree].nomnom,text);
	    free_string(food->short_descr);
	    food->short_descr = str_dup(buf);
	    sprintf(buf,"A chunk of %s meat from %s lies here.",yum[degree].nomnom,text);
	    free_string(food->description);
	    food->description = str_dup(buf);
	    food->value[0]= degree < 3 ? ch->level/2 : ch->level;
	    food->value[1]=skill;
	    if (IS_SET(corpse->value[1],CORPSE_POISONED))
		food->value[3] = 1;
	    obj_to_char(food,ch);
	}
	
	desiccate_corpse(corpse);
	check_improve(ch,NULL,gsn_cook,TRUE,1);
	WAIT_STATE(ch,UMAX(ch->wait,skill_table[gsn_cook].beats));
	return;
    }
    else
    {
	act("You attempt to carve $p into edible chunks, but end up ruining it.",ch,corpse,NULL,TO_CHAR);
	act("$n attempts to carve $p into edible chunks, but ends up ruining it.",ch,corpse,NULL,TO_ROOM);
	desiccate_corpse(corpse);
	check_improve(ch,NULL,gsn_cook,FALSE,1);
	WAIT_STATE(ch,UMAX(ch->wait,skill_table[gsn_cook].beats));
	return;
    } 
}

bool spell_eyesoftheforest( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    bool found = FALSE;

    if (is_affected(ch, sn))
    {
        send_to_char("Your eyes are already attuned to the outdoors.\n\r", ch);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = 0;
    if (!IS_AFFECTED(ch,AFF_DETECT_INVIS))
    {
	af.bitvector = AFF_DETECT_INVIS;
	affect_to_char(ch, &af );
	found = TRUE;
    }
    if (!IS_AFFECTED(ch,AFF_INFRARED))
    {
	af.bitvector = AFF_INFRARED;
	affect_to_char(ch,&af);
	found = TRUE;
    }
    if (!IS_PAFFECTED(ch,AFF_SHARP_VISION))
    {
	af.bitvector = AFF_SHARP_VISION;
	affect_to_char(ch,&af);
	found = TRUE;
    }
    if (!found)
	affect_to_char(ch,&af);

    send_to_char( "You attune your eyes to the outdoors.\n\r", ch );

    return TRUE;
}

bool spell_moonsight( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    bool found = FALSE;

    if (is_affected(ch, sn))
    {
        send_to_char("The light of the moons is already in your eyes.\n\r", ch);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = 0;
    if (!IS_AFFECTED(ch,AFF_DETECT_INVIS))
    {
	af.bitvector = AFF_DETECT_INVIS;
	affect_to_char(ch, &af );
	found = TRUE;
    }
    if (!IS_AFFECTED(ch,AFF_INFRARED))
    {
	af.bitvector = AFF_INFRARED;
	affect_to_char(ch,&af);
	found = TRUE;
    }
    if (!IS_PAFFECTED(ch,AFF_SHARP_VISION))
    {
	af.bitvector = AFF_SHARP_VISION;
	affect_to_char(ch,&af);
	found = TRUE;
    }
    if (!found)
	affect_to_char(ch,&af);

    send_to_char( "The light of the moons fills your eyes.\n\r", ch );

    return TRUE;
}

bool spell_shurangaze( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    bool found = FALSE;

    if (is_affected(ch, sn))
    {
        send_to_char("Your eyes are already attuned to death.\n\r", ch);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = 0;
    if (!IS_AFFECTED(ch,AFF_DETECT_INVIS))
    {
	af.bitvector = AFF_DETECT_INVIS;
	affect_to_char(ch, &af );
	found = TRUE;
    }
    if (!IS_AFFECTED(ch,AFF_INFRARED))
    {
	af.bitvector = AFF_INFRARED;
	affect_to_char(ch,&af);
	found = TRUE;
    }
    if (!IS_PAFFECTED(ch,AFF_SHARP_VISION))
    {
	af.bitvector = AFF_SHARP_VISION;
	affect_to_char(ch,&af);
	found = TRUE;
    }
    if (!found)
	affect_to_char(ch,&af);

    send_to_char( "You attune your eyes to death.\n\r", ch );

    return TRUE;
}

bool spell_oaklance( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    char *direction;
    ROOM_INDEX_DATA *pRoom = NULL;
    int dir = -1, dam, i;

    if (!ch->in_room)
	return FALSE;

    direction = one_argument(target_name, arg);

    if ((arg[0] == '\0') && !ch->fighting) 
    {
	send_to_char("Who are you trying to lance?\n\r", ch);
	return FALSE;
    }

    if (direction[0] == '\0')
        pRoom = ch->in_room;
    else
    {
	if (ch->fighting)
	{
	    send_to_char("You can't aim an oak lance into another room while fighting!\n\r", ch);
	    return FALSE;
	}

        for (i = 0; i < 6; i++)
            if (!str_prefix(direction, dir_name[i]))
                dir = i;

        if (dir == -1)
        {
	    send_to_char("Invalid direction.\n\r",ch);
            return FALSE;
        }

	if (!ch->in_room->exit[dir]
         || IS_SET(ch->in_room->exit[dir]->exit_info, EX_CLOSED) 
	 || IS_SET(ch->in_room->exit[dir]->exit_info, EX_WALLED) 
	 || IS_SET(ch->in_room->exit[dir]->exit_info, EX_ICEWALL) 
	 || IS_SET(ch->in_room->exit[dir]->exit_info, EX_WALLOFFIRE) 
	 || IS_SET(ch->in_room->exit[dir]->exit_info, EX_WALLOFVINES)
	 || IS_SET(ch->in_room->exit[dir]->exit_info, EX_FAKE))
        {
            send_to_char("You can't aim an oak lance in that direction!\n\r",ch);
            return FALSE;
        }

	pRoom = ch->in_room->exit[dir]->u1.to_room;
    }

    if (pRoom == NULL)
    {
        send_to_char("You cannot aim an oak lance in that direction!\n\r",ch);
        return FALSE;
    }

    if (room_is_affected(ch->in_room, gsn_smoke)  
     || room_is_affected(pRoom, gsn_smoke))
    {
        send_to_char("The thick smoke wafting by blocks you from targetting an oak lance.\n\r",ch);
        return FALSE;
    }

    if (pRoom == ch->in_room)
    {
	if (arg[0] == '\0')
	{
	    if (ch->fighting)
		victim = ch->fighting;
	    else
	    {
		send_to_char("Aim an oak lance at whom?\n\r", ch);
		return FALSE;
	    }
	}    
    	else if ((victim = get_char_room(ch,arg)) == NULL)
        {
            send_to_char("You don't see them here.\n\r",ch);
            return FALSE;
        }
    }
    else
    {
        victim = get_char_room(ch, pRoom, arg);
        if (!victim)
        {
            send_to_char("You can't seem to see that person in that direction.\n\r",ch);
            return FALSE;
        }
    }
    if (is_safe_spell(ch, victim, FALSE))
	return FALSE;

    dam = dice(level, 4) + dice((2 * level) / 3, 1);

    if (saves_spell(level, ch, victim, DAM_COLD))
        dam /= 2;

    if (pRoom != ch->in_room)
        dam /= 2;

    /* 1/2 damage if they save, 1/2 damage again if it's one room away. */

    act("You make a throwing gesture, and a log of oak appears in mid-air!",ch,NULL,NULL,TO_CHAR);

    if (pRoom == ch->in_room)
    {
        act("$n makes a throwing gesture, and a log of oak appears, hurling towards $N.",ch,NULL,victim,TO_NOTVICT);
        act("$n makes a throwing gesture, and a log of oak appears, hurling towards you!",ch,NULL,victim,TO_VICT);
    }
    else
    {
	char buf[MAX_STRING_LENGTH];

	sprintf(buf, "$n makes a throwing gesture, and a log of oak appears, hurling %s!", (dir == 0 ? "northwards" 
			: dir == 1 ? "eastwards"
			: dir == 2 ? "southwards"
			: dir == 3 ? "westwards"
			: dir == 4 ? "upwards"
			: dir == 5 ? "down below you" : "away"));
	act(buf, ch, NULL, NULL, TO_ROOM); 
        act("A log of oak appears, hurling towards $n!", victim, NULL, NULL, TO_ROOM);
	act("A log of oak appears, hurling towards you!", victim, NULL, NULL, TO_CHAR);
    }

    if ((pRoom != ch->in_room) && IS_NPC(victim) && ((IS_SET(victim->act, ACT_SENTINEL) || IS_SET(victim->act, ACT_NOTRACK))) && (victim->hit < victim->max_hit))
	dam = 0;
    if (pRoom != ch->in_room
      && check_defensiveroll(victim))
    {
	send_to_char("You roll as the log of oak strikes you, lessening its affect.\n\r",victim);
	dam /= 2;
    }
    damage_old( ch, victim, dam, sn,DAM_BASH,TRUE);

    return TRUE;
}

bool spell_wither( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA waf, *paf;
    int dam;

    if (is_safe_spell(ch, victim, FALSE))
        return FALSE;

    act("You send debilitating energy towards $N!", ch, NULL, victim, TO_CHAR);
    act("$n sends debilitating energy towards $N!", ch, NULL, victim, TO_VICTROOM);
    act("$n sends debilitating energy towards you!", ch, NULL, victim, TO_VICT);

    dam = (dice(level, 2) + level/2 + 25);
    if (saves_spell(level, ch, victim, DAM_POISON))
        dam /= 2;
    damage( ch, victim, dam, sn, DAM_POISON, TRUE);

    if (ch->in_room == NULL || IS_OAFFECTED(ch,AFF_GHOST))
	return TRUE;

    dam = 0;
    for (paf=victim->affected;paf;paf=paf->next)
	if (paf->type == gsn_wither)
	    dam++;

    if (dam >= 20)
        return TRUE;
    
    if (!saves_spell(level, ch, victim, DAM_POISON))
    {
        waf.valid = TRUE;
        waf.point = NULL;
        waf.where = TO_AFFECTS;
        waf.type = gsn_wither;
        waf.location = APPLY_DEX;
        waf.modifier = -1;
        waf.duration = level/5;
        waf.bitvector = 0;
        affect_to_char(victim,&waf);
        waf.location = APPLY_STR;
        affect_to_char(victim,&waf);
        act("$n looks weaker.",victim,NULL,NULL,TO_ROOM);
        act("You feel weaker.",victim,NULL,NULL,TO_CHAR);
    }

    return TRUE;
}

bool spell_snakebite( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, gsn_snakebite))
    {
        send_to_char("You are already biting like a snake.\n\r",ch);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.modifier  = 20;
    af.location  = APPLY_RESIST_POISON;
    af.duration  = (level / 5);
    af.bitvector = 0;
    affect_to_char(ch, &af);

    send_to_char("Your teeth begin oozing a foul poison.\n\r", ch);

    return TRUE;
}

bool spell_infectiousaura( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
        send_to_char("You're already feverish.\n\r", ch);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.location  = APPLY_RESIST_DISEASE;
    af.modifier  = 20;
    af.duration  = level/8;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    if (ch->race == global_int_race_srryn)
	send_to_char("Your body feels warmer.\n\r",ch);
    else
	send_to_char("You begin sweating as your body temperature rises.\n\r",ch);    
    act("$n's body begins radiating heat.", ch, NULL, NULL, TO_ROOM);

    return TRUE;
}

void do_bloodsigil(CHAR_DATA *ch, char *argument)
{
    int chance = get_skill(ch,gsn_bloodsigil);
    OBJ_DATA *sigil;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    
    if (chance < 1 || ch->level < skill_table[gsn_bloodsigil].skill_level[ch->class_num])
    {
	send_to_char("Huh?\n\r",ch);
	return;
    }

    if ((sigil = get_eq_char(ch, WEAR_BODY)) != NULL)
    {
	if (sigil->pIndexData->vnum == OBJ_VNUM_BLOOD_SIGIL)
	    send_to_char("You're already empowered with a blood sigil.\n\r",ch);
	else
	    act("$p will get in the way.",ch,sigil,NULL,TO_CHAR);
	return;
    }

    if (!(sigil = create_object(get_obj_index(OBJ_VNUM_BLOOD_SIGIL), ch->level)))
    {
        bug("Skill: blood sigil.  Cannot load blood sigil object.", 0);
        send_to_char("Something seems to be amiss...\n\r", ch);
        return;
    }

    if (number_percent() > chance)
    {
	send_to_char("Your hand slips as you begin carving a blood sigil upon your chest.\n\r",ch);
	check_improve(ch,ch,gsn_bloodsigil,1,FALSE);
	damage(ch,ch,50,gsn_bloodsigil,DAM_SLASH,TRUE);
	return;	
    }
    damage(ch,ch,25,gsn_bloodsigil,DAM_SLASH,TRUE);

    if (ch->in_room == NULL || IS_OAFFECTED(ch,AFF_GHOST))
    {
	if (ch != NULL)
	    send_to_char("Mocking laughter fills your mind.\n\r",ch);
	return;
    }

    sigil->level       = ch->level;
    sigil->value[0]    = ch->level/2;
    sigil->value[1]    = ch->level/2;
    sigil->value[2]    = ch->level/2;
    sigil->value[3]    = ch->level/2;

    af.where     = TO_OBJECT;
    af.type      = gsn_bloodsigil;
    af.level     = ch->level;
    af.duration  = 48;
    af.location  = APPLY_MANA;
    af.modifier  = ch->level;
    af.bitvector = 0;
    affect_to_obj(sigil,&af);
    af.location  = APPLY_SAVES;
    af.modifier  = -(ch->level/10);
    affect_to_obj(sigil,&af);
    af.location = APPLY_HIT;
    af.modifier = -(ch->level/2);
    affect_to_obj(sigil,&af);

    send_to_char("You carve a blood sigil upon your chest.\n\r", ch);
    act("$n carves a blood sigil upon $s chest.", ch, NULL, NULL, TO_ROOM);

    obj_to_char(sigil, ch);
    equip_char(ch,sigil,WORN_BODY);
    check_improve(ch,ch,gsn_bloodsigil,1,TRUE);
    return;
}

bool spell_tendrilgrowth( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;
    OBJ_DATA *obj = NULL;

    if (is_affected(ch,gsn_tendrilgrowth))
    {
	send_to_char("Your arms are already tendrils of Gamaloth.\n\r",ch);
	return FALSE;
    }

    if (obj = get_eq_char(ch,WEAR_DUAL_WIELD))
        if (!remove_obj(ch,obj,TRUE))
	    return FALSE;

    if (obj = get_eq_char(ch,WEAR_WIELD))
        if (!remove_obj(ch,obj,TRUE))
	    return FALSE;
    
    if (obj = get_eq_char(ch,WEAR_HOLD))
        if (!remove_obj(ch,obj,TRUE))
	    return FALSE;

    if (obj = get_eq_char(ch,WEAR_LIGHT))
        if (!remove_obj(ch,obj,TRUE))
	    return FALSE;

    if (obj = get_eq_char(ch,WEAR_SHIELD))
        if (!remove_obj(ch,obj,TRUE))
	    return FALSE;

    if (obj = get_eq_char(ch,WEAR_HANDS))
        if (!remove_obj(ch,obj,TRUE))
	    return FALSE;

    af.type = sn;
    af.where = TO_AFFECTS;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.duration = 48;
    af.bitvector = 0;
    affect_to_char(ch,&af);

    act("Your arms twist and elongate into oily, black tendrils.",ch,NULL,NULL,TO_CHAR);
    act("$n's arms twist and elongate into  oily, black tendrils.",ch,NULL,NULL,TO_ROOM);
    return TRUE;
}

bool spell_drawblood( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;

    if (is_affected(ch,gsn_drawblood))
    {
	send_to_char("You're already focused on drawing blood from your opponents.\n\r",ch);
	return FALSE;
    }

    af.where = TO_AFFECTS;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = 0;
    af.duration = 24;
    af.type = sn;
    affect_to_char(ch,&af);
    send_to_char("You focus on cutting deep gashes into your victims.\n\r",ch);

    return TRUE;
}

bool spell_beastform( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;

    if (is_affected(ch,gsn_beastform))
    {
	if (IS_OAFFECTED(ch,AFF_BEASTFORM))
	    send_to_char("You've already adopted the nature of the beast.\n\r",ch);
	else
	    send_to_char("You're still recovering from your last rampage.\n\r",ch);
	
	return FALSE;
    }

    af.where = TO_OAFFECTS;
    af.type = sn;
    af.duration = 16;
    af.bitvector = 0;

    af.modifier = level * 2 / 5;
    af.location = APPLY_DAMROLL;
    affect_to_char(ch,&af);
    af.location = APPLY_HITROLL;
    affect_to_char(ch,&af);

    af.bitvector = AFF_BEASTFORM;
    af.modifier = level * 3;
    af.location = APPLY_HIT;
    affect_to_char(ch,&af);
    send_to_char("Your body contorts into a beastlike form.\n\r",ch);
    act("$n's body contorts into a beastlike form.",ch,NULL,NULL,TO_ROOM);
    
    return TRUE;
}

bool spell_moonbornspeed( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;

    if (is_affected(ch,gsn_moonbornspeed))
    {
	send_to_char("You're already moving with moonborn speed.\n\r",ch);
	return FALSE;
    }

    if (time_info.hour >= season_table[time_info.season].sun_up
      && time_info.hour < season_table[time_info.season].sun_down)
    {
	send_to_char("You can't move with moonborn speed during the day.\n\r",ch);
	return FALSE;
    }
    af.type = sn;
    af.modifier = 0;
    af.location = 0;
    af.where = TO_AFFECTS;
    af.bitvector = 0;
    af.duration = level * 2 / 5;
    affect_to_char(ch,&af);

    send_to_char("You begin moving with moonborn speed.\n\r",ch);
    return TRUE;
}

bool spell_naturalarmor( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_INDEX_DATA *objIndex;
    OBJ_DATA *shield;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
        send_to_char("You don't feel ready to fashion another piece of natural armor yet.\n\r", ch);
        return FALSE;
    }

    if (ch->in_room->sector_type != SECT_FOREST)
    {
	send_to_char("Only the forests will lend you their protection.\n\r",ch);
	return FALSE;
    }
    if (target_name[0] == '\0')
    {
	send_to_char("Did you want to make natural armor for your torso, feet, legs, or arms?\n\r",ch);
	return FALSE;
    }

    if ((objIndex = get_obj_index(OBJ_VNUM_NATURAL_ARMOR)) == NULL)
    {
        bug("Spell: natural armor.  Cannot load armor object.", 0);
        send_to_char("Something seems to be amiss...\n\r", ch);
        return FALSE;
    }

    shield = create_object(objIndex, level);

    if (!str_cmp(target_name,"torso") || !str_cmp(target_name,"vest"))
    {
	SET_BIT(shield->wear_flags,ITEM_WEAR_BODY);
	free_string(shield->short_descr);
	shield->short_descr = str_dup("a vest of woven bark");
	setName(*shield, "vest woven bark natural armor");
	free_string(shield->description);
	shield->description = str_dup("A vest of woven bark is here on the ground.");
    }
    else if (!str_cmp(target_name,"feet") || !str_cmp(target_name,"boots"))
    {
	SET_BIT(shield->wear_flags,ITEM_WEAR_FEET);
	free_string(shield->short_descr);
	shield->short_descr = str_dup("a pair of woven reed boots");
	setName(*shield, "pair woven reed boots natural armor");
	free_string(shield->description);
	shield->description = str_dup("A pair of woven reed boots are here on the ground.");
    }
    else if (!str_cmp(target_name,"legs") || !str_cmp(target_name,"pants"))
    {
	SET_BIT(shield->wear_flags,ITEM_WEAR_LEGS);
	free_string(shield->short_descr);
	shield->short_descr = str_dup("a pair of woven grass pants");
	setName(*shield, "pair woven reed pants natural armor");
	free_string(shield->description);
	shield->description = str_dup("A pair of woven reed pants is here on the ground.");
    }
    else if (!str_cmp(target_name,"arms") || !str_cmp(target_name,"sleeves"))
    {
	SET_BIT(shield->wear_flags,ITEM_WEAR_ARMS);
	free_string(shield->short_descr);
	shield->short_descr = str_dup("a pair of woven grass sleeves");
	setName(*shield, "pair sleeves woven grass natural armor");
	free_string(shield->description);
	shield->description = str_dup("A pair of woven grass sleeves is here on the ground.");
    }
    else
    {
	send_to_char("What did you want to create?\n\r",ch);
	return FALSE;
    }

    shield->timer	= 0;
    shield->level       = level;
    shield->value[0]    = level/4;
    shield->value[1]    = level/4;
    shield->value[2]    = level/4;
    shield->value[3]    = level/4;

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 2;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    af.where     = TO_OBJECT;
    af.duration  = -1;
    af.location  = APPLY_MANA;
    af.modifier  = level/2;
    affect_to_obj(shield, &af);
    af.location = APPLY_HITROLL;
    af.modifier = level/20;
    affect_to_obj(shield,&af);
    af.location = APPLY_DAMROLL;
    af.modifier = level/20;
    affect_to_obj(shield,&af);

    act("You call upon the forest to grant you protection, and $p forms within your hands!", ch, shield, NULL, TO_CHAR);
    act("$n concentrates, and $p forms within $s hands!", ch, shield, NULL, TO_ROOM);

    obj_to_char(shield, ch);

    return TRUE;
}

bool spell_seedofgamaloth( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *corpse = NULL;
    AFFECT_DATA af;

    if (target_name[0] == '\0')
    {
	send_to_char("In what did you want to plant a seed of Gamaloth?\n\r",ch);
	return FALSE;
    }

    if ((corpse = get_obj_here(ch,target_name)) == NULL)
    {
	send_to_char("There is nothing here in which to plant a seed of Gamaloth.\n\r",ch);
	return FALSE;
    }

    if (!(corpse->pIndexData->vnum == ITEM_CORPSE_PC
      || corpse->pIndexData->vnum != ITEM_CORPSE_NPC))
    {
	send_to_char("That is not a suitable vessel in which to plant a seed of Gamaloth.\n\r",ch);
	return FALSE;
    }
 
    af.valid = TRUE;
    af.point = NULL;
    af.where = TO_OBJECT;
    af.type = gsn_seedofgamaloth;
    af.duration = 1;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = 0;
    affect_to_obj(corpse,&af);

    af.duration = 48;
    af.where = TO_AFFECTS;
    affect_to_char(ch,&af);

    act("You plant a seed of Gamaloth in $p, which begins to fester.",ch,corpse,NULL,TO_CHAR);
    act("$n plants a seed of Gamaloth into $p, which begins to fester.",ch,corpse,NULL,TO_ROOM);

    return TRUE;
}


