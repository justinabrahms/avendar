/* Name: house_powers.c							*/
/* 									*/
/* This file contains all of the house powers, sorted by a) house, and  */
/* b) skill/spell name.		-Erinos					*/

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
#include "NameMaps.h"

/* External declarations */
DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_autoyell);

extern	int	clan_lookup	args( ( const char *name ) );

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* CHAMPION - House spells and skills.				       */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

bool spell_brotherhood( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
CHAR_DATA *victim = (CHAR_DATA *) vo;
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
int rnum;


        if (!stone_check(ch, gsn_brotherhood))
        {
        send_to_char("Your magic fails to tap enough power from the stones.\n\r", ch);
        return FALSE;
        }

        if (victim == ch)
        {
        send_to_char("You can't tie into yourself.\n\r", ch);
        return FALSE;
        }

        if (is_affected(victim, gsn_brotherhood))
        {
        send_to_char("They are already linked with brotherhood.\n\r", ch);
        return FALSE;
        }

        if (is_affected(ch, gsn_brotherhood))
        {
        send_to_char("You are already linked with brotherhood.\n\r", ch);
        return FALSE;
        }

        if (victim->clan != ch->clan)
        {
        send_to_char("You can only link a champion into brotherhood.\n\r", ch);
        return FALSE;
        }

	if (IS_NPC(victim))
	{
	send_to_char("You can only form a brotherhood with other players.\n\r", ch);
	return FALSE;
	}

        rnum = number_range(1, 10000);

        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = level;
        af.duration  = level/5;
        af.location  = APPLY_NONE;
        af.modifier  = rnum;
        af.bitvector = 0;
        affect_to_char( victim, &af );
        affect_to_char( ch, &af );

        act("You feel yourself tie into $N.", ch, NULL, victim, TO_CHAR);
        act("You feel $n tie into you.", ch, NULL, victim, TO_VICT);

    return TRUE;
}


bool spell_holyavenger( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
OBJ_DATA *weapon;
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (!stone_check(ch, gsn_holyavenger))
    {
        send_to_char("Your magic fails to tap enough power from the stones.\n\r", ch);
        return FALSE;
    }

    weapon = (OBJ_DATA *) vo;

    if (obj_is_affected(weapon, gsn_holyavenger))
    {
        send_to_char("That weapon already shines with a holy light.\n\r", ch);
        return FALSE;
    }

    if (obj_is_affected(weapon, gsn_soulreaver))
    {
	act("Fell runes of binding and destruction cover $p.",ch,weapon,NULL,TO_CHAR);
	return FALSE;
    }

    if (weapon->item_type != ITEM_WEAPON)
    {
        send_to_char("You may only enchant a weapon with this spell.\n\r", ch);
        return FALSE;
    }
  
    int mod = aura_grade(ch);
    if (mod < 0)
        mod = (mod - 1) * -5;
    else
        mod = 0;

    if (mod == 0)
    {
        send_to_char("You can't yet infuse a weapon with your purity.\n\r",ch);
	return FALSE;
    }

    af.where     = TO_OBJECT;
    af.type      = sn;
    af.level     = level;
    af.duration  = 50;
    af.location  = 0;
    af.modifier  = mod;
    af.bitvector = ITEM_GLOW;
    affect_to_obj(weapon, &af);
    af.bitvector = ITEM_BLESS;
    affect_to_obj(weapon, &af);

    act("As you finish the incantation, $p radiates holy light.", ch, weapon, NULL, TO_CHAR);
    act("As $n finishes $s incantation, $p radiates holy light.", ch, weapon, NULL, TO_ROOM);

    return TRUE;
}

bool spell_reveal( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *vch;
    DESCRIPTOR_DATA *d;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int mod;

    if (!stone_check(ch, gsn_reveal))
    {
        send_to_char("Your magic fails to tap enough power from the stones.\n\r", ch);
        return FALSE;
    }

    if (!ch || !ch->in_room || !ch->in_room->area)
	return FALSE;
    
    send_to_char("You send a wave of heavenly light into the shadows.\n\r",ch);

    for (d = descriptor_list; d->next != NULL; d = d->next)
    {
        if ((vch = d->character) 
	 && vch->in_room
	 && (ch->in_room->area == vch->in_room->area))
        {
            send_to_char("A wave of heavenly light sweeps through the area.\n\r", vch);
            mod = aura_grade(vch)-1;
	    if ((vch != ch) && !is_affected(vch, gsn_reveal) && !is_same_group(ch, vch)
	     && !IS_IMMORTAL(vch) && vch->clan != clan_lookup("CHAMPION")
	     && mod > 0)
            {
                af.where     = TO_AFFECTS;
                af.type      = sn;
                af.level     = level;
                af.duration  = mod;
                af.location  = 0;
                af.modifier  = 0;
                af.bitvector = AFF_FAERIE_FIRE;
                affect_to_char(vch, &af);
                send_to_char("The air around you begins to glow.\n\r", vch);
		act("The air around $n begins to glow.", vch, NULL, NULL, TO_ROOM);

                uncamo_char(vch);
                unhide_char(vch);

                affect_strip(vch, gsn_invis);
                affect_strip(vch, gsn_mass_invis);
                affect_strip(vch, gsn_wildmove);
                affect_strip(vch, gsn_sneak);
                if (!(race_table[vch->race].aff & AFF_SNEAK))
                        REMOVE_BIT(vch->affected_by, AFF_SNEAK);
                if (!(race_table[vch->race].aff & AFF_HIDE))
                        REMOVE_BIT(vch->affected_by, AFF_HIDE);
                if (!(race_table[vch->race].aff & AFF_INVISIBLE))
                        REMOVE_BIT(vch->affected_by, AFF_INVISIBLE);
            }
        }
    }

    return TRUE;
}

bool spell_scourgeofdarkness( int sn, int level,CHAR_DATA *ch,void *vo,int target)
{
    char buf[MAX_INPUT_LENGTH];
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    static const int dam_each[] =
    {
          0,
          0,   0,   0,   0,   0,          0,   0,   0,   0,   0,
          0,   0,   0,   0,  30,         35,  40,  45,  50,  55,
         60,  65,  70,  75,  80,         82,  84,  86,  88,  90,
         92,  94,  96,  98, 100,        102, 104, 106, 108, 110,
        112, 114, 116, 118, 120,        122, 124, 126, 128, 130
    };
    int dam;
    int mod;

    if (!stone_check(ch, gsn_scourgeofdarkness))
    {
        send_to_char("Your magic fails to tap enough power from the stones.\n\r", ch);
        return FALSE;
    }

    level = UMIN(level, static_cast<int>(sizeof(dam_each)/sizeof(dam_each[0])) - 1);
    level = UMAX(0, level);
    mod = aura_grade(ch);

    send_to_char( "You raise your arms, and a storm of holy wrath blazes around you!\n\r", ch );
    act( "$n raises $s arms, and a storm of holy wrath blazes around $m!",
        ch, NULL, NULL, TO_ROOM );

    for ( vch = char_list; vch != NULL; vch = vch_next )
    {
        vch_next  = vch->next;
        if ( vch->in_room == NULL )
            continue;
        if ((vch->in_room == ch->in_room) && (ch!=vch) &&
                !(is_same_group(ch, vch)) && !is_safe_spell(ch,vch,TRUE))
        {
            if (((vch!=ch) && (mod<=0)) && IS_TRUE_EVIL(vch) && (!IS_IMMORTAL(vch) ||
               (IS_IMMORTAL(vch) && can_see(ch,vch))))
            {
                if ((ch->fighting != vch) && !IS_NPC(vch) && vch->fighting!=ch)
                {
                    if (can_see(vch, ch))
                    {
                        sprintf( buf, "Help!  %s is attacking me!", PERS(ch, vch));
                        do_autoyell( vch, buf );
                    }
                    else
                    {
                        sprintf( buf, "Help!  I'm being attacked!");
                        do_autoyell( vch, buf );
                    }
                }

           	dam = number_range( dam_each[level] / 2, dam_each[level] * 2 );
		
		if (mod == -4)
		    dam = dam * 3 / 2;
		else if (mod == -3)
		    dam = dam * 125 / 100;
		else if (mod == -1)
		    dam = dam * 3 / 4;
		if ( saves_spell( level, ch, vch, DAM_HOLY) )
            dam /= 2;

                damage_old( ch, vch, saves_spell( level,ch, vch,DAM_HOLY) ? dam / 2 : dam, sn,DAM_HOLY,TRUE);
            }
	    continue;
        }
    }
    return TRUE;
}


bool spell_soulblade( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;
    int mod;

    if ( !IS_NPC(ch) && IS_TRUE_EVIL(ch) )
        victim = ch;

    if (!stone_check(ch, gsn_soulblade))
    {
        send_to_char("Your magic fails to tap enough power from the stones.\n\r", ch);
        return FALSE;
    }

    if ( IS_TRUE_GOOD(victim) )
    {
        act( "$N is unharmed by your holy wrath.", ch, NULL, victim, TO_CHAR );
        return TRUE;
    }
    if ( IS_TRUE_NEUTRAL(victim) )
    {
        act( "$N is unharmed by your holy wrath.", ch, NULL, victim, TO_CHAR );
        return TRUE;
    }

    act("$n calls the wrath of the pure soul onto the evil of $N!", ch, NULL, victim, TO_NOTVICT);
    act("$n calls the wrath of the pure soul onto your evil!", ch, NULL, victim, TO_VICT);
    act("You call the wrath of the pure soul onto the evil of $N!", ch, NULL, victim, TO_CHAR);

    mod = aura_grade(ch);
    if (mod == -4)
	dam = dice(level, 7);
    else if (mod == -3)
	dam = dice(level, 6);
    else if (mod == -2)
	dam = dice(level, 5);
    else if (mod == -1)
	dam = dice(level, 4);
    else 
        dam = 0;
    
    if (!IS_NPC(victim))
    {
        int karma(effective_karma(*victim));
        if (karma >= FAINTREDAURA)
            dam += karma / 100;
    }
    else
        dam += 50;
    
    if ( saves_spell( level, ch, victim,DAM_HOLY) )
        dam /= 2;
    else if (!IS_SET(victim->act, ACT_NOSUBDUE) && number_percent() < 5)
    {
        act("$N screams in agony as the holy wrath tears $S soul asunder!", ch, NULL, victim, TO_NOTVICT);
        act("$N screams in agony as the holy wrath tears $S soul asunder!", ch, NULL, victim, TO_CHAR);
        act("You scream in agony as the holy wrath tears your soul asunder!", ch, NULL, victim, TO_VICT);
	kill_char(victim, ch);
        return TRUE;
    }
    damage_old( ch, victim, dam, sn, DAM_HOLY ,TRUE);

    return TRUE;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* CONQUEST - House spells and skills.				       */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


bool spell_dedication( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
OBJ_DATA *weapon;
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

        if (!stone_check(ch, gsn_dedication))
        {
        send_to_char("Your magic fails to tap enough power from the stones.\n\r", ch);
        return FALSE;
        }

        weapon = (OBJ_DATA *) vo;

        if (obj_is_affected(weapon, gsn_dedication))
        {
        send_to_char("That weapon already has a conquering might.\n\r", ch);
        return FALSE;
        }

        if (weapon->item_type != ITEM_WEAPON)
        {
        send_to_char("You can only give conquering might to weapons.\n\r", ch);
        return FALSE;
        }

        af.where     = TO_OBJECT;
        af.type      = sn;
        af.level     = level;
        af.duration  = 50;
        af.location  = APPLY_HITROLL;
        af.modifier  = level/10;
        af.bitvector = ITEM_GLOW;
        affect_to_obj(weapon, &af);

        af.location  = APPLY_DAMROLL;
        af.bitvector = ITEM_NOREMOVE;
        affect_to_obj(weapon, &af);

        act("As you finish chanting, $p is imbued with a conquering might.", ch, weapon, NULL, TO_CHAR);
        act("As $n finishes chanting, $p is imbued with a conquering might.", ch, weapon, NULL, TO_ROOM);

    return TRUE;

}

bool spell_division( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

        if (!stone_check(ch, gsn_division))
        {
        send_to_char("Your magic fails to tap enough power from the stones.\n\r", ch);
        return FALSE;
        }

        if (is_affected(ch, gsn_division))
        {
        send_to_char("You are already prepared to divide and conquer.\n\r", ch);
        return FALSE;
        }

        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = level;
        af.duration  = ch->level/6;
        af.location  = 0;
        af.modifier  = 0;
        af.bitvector = 0;
        affect_to_char(ch, &af);

        act("$n taps a flow of power to divide and Conquer.", ch, NULL, NULL, TO_ROOM);
        act("You tap a flow of power to divide and Conquer.", ch, NULL, NULL, TO_CHAR);

    return TRUE;
}

bool spell_matrix( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (!stone_check(ch, gsn_matrix))
    {
        send_to_char("Your magic fails to tap enough power from the stones.\n\r", ch);
        return FALSE;
    }

    if (( victim = get_char_world(ch, target_name )) == NULL)
    {
        send_to_char("You can't find them.\n\r", ch);
        return FALSE;
    }


    if (IS_NPC(victim))
    {
        send_to_char("The magic fails to take hold of such an insignifigant target.\n\r", ch);
        return FALSE;
    }

    if (!IS_PK(victim, ch))
    {
        act("The gods protect $N from $n.", ch, NULL, victim, TO_ROOM);
        act("The gods protect $N from you.", ch, NULL, victim, TO_CHAR);
        return FALSE;
    }

    if (is_affected(victim, gsn_matrix))
    {
        send_to_char("They are already entrapped by a matrix.\n\r", ch);
        return FALSE;
    }

    if (!ch->in_room || !victim->in_room || ch->in_room->area != victim->in_room->area)
    {
        send_to_char("They are not within a close enough range to entrap.\n\r", ch);
        return FALSE;
    }

    if (saves_spell(level+2, ch, victim, DAM_CHARM))
    {
        act("$N resists your attempt to summon a matrix to hold them.", ch, NULL, victim, TO_CHAR);
        return TRUE;
    }

    if (check_spirit_of_freedom(victim))
    {
	send_to_char("You feel the spirit of freedom surge within you.\n\r", victim);
	act("$N resists your attempt to hold them.", ch, NULL, victim, TO_CHAR);
	act("$N glows brightly for a moment.", ch, NULL, victim, TO_NOTVICT);
	return TRUE;
    }

    act("You feel the power of the matrix take hold.", ch, NULL, NULL, TO_CHAR);
    act("A tidal rush of sound echoes in your ears, and you begin to feel trapped.", victim, NULL, NULL, TO_CHAR);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 2;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    return TRUE;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* GUARDIAN - House spells and skills.				       */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

bool spell_aegisoflaw( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (!stone_check(ch, gsn_aegisoflaw))
    {
        send_to_char("Your magic fails to tap enough power from the stones.\n\r", ch);
        return FALSE;
    }

    if (is_affected(ch, gsn_aegisoflaw))
    {
        send_to_char("You are already wrapped in the Aegis of Law.\n\r", ch);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = ch->level;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    act("$n is wrapped in the Aegis of Law.", ch, NULL, NULL, TO_ROOM);
    act("You are wrapped in the Aegis of Law.", ch, NULL, NULL, TO_CHAR);

    return TRUE;
}



bool spell_callguards( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *victim;
    CHAR_DATA *guard1;
    CHAR_DATA *guard2;
    CHAR_DATA *guard3;
    CHAR_DATA *guard4;
    int number;
    ROOM_INDEX_DATA *troom;

    if (!stone_check(ch, gsn_callguards))
    {
        send_to_char("Your magic fails to tap enough power from the stones.\n\r", ch);
        return FALSE; 
    }

    if (is_affected(ch, gsn_callguards))
    {
        send_to_char("You can't call guards again yet.\n\r", ch);
        return FALSE;
    }

    if (( victim = get_char_world( ch, target_name ) ) == NULL)
    {
        send_to_char("You can't find any such person.\n\r", ch);
        return FALSE;
    }

    if ((victim->in_room != NULL)
     && (victim->in_room->area != ch->in_room->area))
    {
        send_to_char("You can't find any such person.\n\r", ch);
        return FALSE;
    }

    if (!IS_CRIMINAL(victim))
    {
        send_to_char("You can only call guards to go after criminals.\n\r", ch);
        return FALSE;
    }

    if (IS_SET(victim->in_room->room_flags,ROOM_LAW))
        troom = victim->in_room;
    else
        troom = ch->in_room;

    number = level/13;

    if (number > 0)
    {
        if (number > 1)
        {
	    act("You dispatch guards after $N.", ch, NULL, victim, TO_CHAR);                act("$n dispatches guards after $N.", ch, NULL, victim, TO_ROOM);
            if (troom == victim->in_room)
	    {
	        act("Guards have come to bring you to justice!", victim, NULL, NULL, TO_CHAR);
                act("Guards have arrived to bring $n to justice!", victim, NULL, NULL, TO_ROOM);
 	    }
	    else
	    {
	        act("Guards have arrived to bring $N to justice!", ch, NULL, victim, TO_ROOM);
		act("You hear the shouts of angry guards nearby!", victim, NULL, NULL, TO_CHAR);
 	    }
        }
        else
        {
            act("You dispatch a guard after $N.", ch, NULL, victim, TO_CHAR);
            act("$n dispatches a guard after $N.", ch, NULL, victim, TO_ROOM);
            if (troom == victim->in_room)
	    {
	        act("A guard has come to bring you to justice!", victim, NULL, NULL, TO_CHAR);
                act("A guard has arrived to bring $n to justice!", victim, NULL, NULL, TO_ROOM);
	    }
	    else
	    {
	        act("A guard has arrived to bring $N to justice!", ch, NULL, NULL, TO_ROOM);
		act("You hear the shout of an angry guard nearby!", victim, NULL, NULL, TO_CHAR);
	    }
        }
        guard1 = create_mobile(get_mob_index(MOB_VNUM_GUARD));
        guard1->max_hit = ch->max_hit;
        guard1->hit = ch->hit;
        guard1->level = (level*3)/4;
        guard1->hitroll = level/5;
        guard1->damroll = level/5;
        guard1->damage[0] = 5;
        guard1->damage[1] = level/5;
        guard1->damage[2] = level/8;
        guard1->armor[0] = 0 - (level / 2);
        guard1->armor[1] = 0 - (level / 2);
        guard1->armor[2] = 0 - (level / 2);
        guard1->armor[3] = 0 - (level / 2);
        guard1->tracking = victim;
        char_to_room(guard1, troom);
    }

    if (number > 1)
    {
        guard2 = create_mobile(get_mob_index(MOB_VNUM_GUARD));
        guard2->hit = ch->hit;
        guard2->max_hit = ch->max_hit;
        guard2->level = (level*3)/4;
        guard2->hitroll = level/5;
        guard2->damroll = level/5;
        guard2->damage[0] = 5;
        guard2->damage[1] = level/5;
        guard2->damage[2] = level/8;
        guard2->armor[0] = 0 - (level / 2);
        guard2->armor[1] = 0 - (level / 2);
        guard2->armor[2] = 0 - (level / 2);
        guard2->armor[3] = 0 - (level / 2);
        guard2->tracking = victim;
        char_to_room(guard2, troom);
    }

    if (number > 2)
    {
        guard3 = create_mobile(get_mob_index(MOB_VNUM_GUARD));
        guard3->hit = ch->hit;
        guard3->max_hit = ch->max_hit;
        guard3->level = (level*3)/4;
        guard3->hitroll = level/5;
        guard3->damroll = level/5;
        guard3->damage[0] = 5;
        guard3->damage[1] = level/5;
        guard3->damage[2] = level/8;
        guard3->armor[0] = 0 - (level / 2);
        guard3->armor[1] = 0 - (level / 2);
        guard3->armor[2] = 0 - (level / 2);
        guard3->armor[3] = 0 - (level / 2);
        guard3->tracking = victim;
        char_to_room(guard3, troom);
    }

    if (number > 3)
    {
        guard4 = create_mobile(get_mob_index(MOB_VNUM_GUARD));
        guard4->hit = ch->hit;
        guard4->max_hit = ch->max_hit;
        guard4->level = (level*3)/4;
        guard4->hitroll = level/5;
        guard4->damroll = level/5;
        guard4->damage[0] = 5;
        guard4->damage[1] = level/5;
        guard4->damage[2] = level/8;
        guard4->armor[0] = 0 - (level / 2);
        guard4->armor[1] = 0 - (level / 2);
        guard4->armor[2] = 0 - (level / 2);
        guard4->armor[3] = 0 - (level / 2);
        guard4->tracking = victim;
        char_to_room(guard4, troom);
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 12;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);
    return TRUE;
}

/* Mostly rewritten by Aeolis.  This code hurt my head. */

void do_criminal( CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    char buf[MAX_INPUT_LENGTH];

    if (IS_NPC(ch))
	return;

    if ((ch->pcdata->learned[gsn_criminal] < 1) || (ch->clan != clan_lookup(skill_table[gsn_criminal].house)))
        if (ch->level < 52)
	{
		send_to_char("Huh?\n\r", ch);
	        return;
	}

    argument = one_argument(argument, arg);

    if ((arg[0] == '\0') || (argument[0] == '\0')
     || ((LOWER(argument[0]) != 'y') && (LOWER(argument[0]) != 'n')))
    {
	send_to_char("Syntax: criminal <char> y|n\n\r", ch);
	return;
    }

    if ((victim = get_char_world(ch, arg)) == NULL)
    {
	send_to_char("You don't see any such person.\n\r", ch);
	return;
    }

    if ((ch == victim) && (ch->level < 52))
    {
	send_to_char("You can't do that to yourself.\n\r", ch);
	return;
    }

    if (IS_NPC(victim))
    {
	send_to_char("They cannot be marked as a criminal.\n\r", ch);
	return;
    }

    if ((victim->act & PLR_REWARD) && (LOWER(argument[0]) == 'y'))
    {
	send_to_char("They are already a criminal.\n\r", ch);
	return;
    }

    if (!IS_SET(victim->act, PLR_REWARD) && (LOWER(argument[0]) == 'n'))
    {
	send_to_char("But they aren't a criminal!\n\r", ch);
	return;
    }

    if (IS_NPC(ch))
	return;

/* Ok, they're passing a legal command, let's rock and roll */

    if (LOWER(argument[0]) == 'y')
    {
	SET_BIT(victim->act, PLR_REWARD);
	victim->pcdata->times_wanted++;
	sprintf(buf, "REWARD: %s marked %s.", ch->name, victim->name);
	log_string(buf);
	wiznet(buf, ch, NULL, WIZ_FLAGS, 0, 0);
	send_to_char("They are now a criminal.\n\r", ch);
	send_to_char("You are now a CRIMINAL!!!\n\r", victim);
	act("$N is now a CRIMINAL!!!", ch, NULL, victim, TO_NOTVICT);
    }
    else
    {
	REMOVE_BIT(victim->act, PLR_REWARD);
	victim->pcdata->times_wanted--;
	send_to_char("They are no longer a criminal.\n\r", ch);
	send_to_char("You are no longer a criminal.\n\r", victim);
	act("$N is no longer a criminal.", ch, NULL, victim, TO_ROOM);
    }

    return;
}

bool spell_locatecriminal( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];

    if (!stone_check(ch, gsn_locatecriminal))
    {
        send_to_char("Your magic fails to tap enough power from the stones.\n\r", ch);
        return FALSE;
    }

    if (( victim = get_char_world(ch, target_name)) == NULL)
    {
        send_to_char("You couldn't find any such person.\n\r", ch);
        return FALSE;
    }

    if (!IS_CRIMINAL(victim))
    {
        send_to_char("That person is no criminal!\n\r", ch);
        return FALSE;
    }

    act("$n calls on mighty power to find $N.", ch, NULL, victim, TO_ROOM);
    act("You call on mighty powers to find $N.", ch, NULL, victim, TO_CHAR);
    send_to_char("\n\r", ch);
    sprintf(buf, "%s", victim->in_room->name);
    send_to_char(buf, ch);
    send_to_char("\n\r", ch);
    sprintf(buf, "%s", victim->in_room->description);
    send_to_char(buf, ch);
    send_to_char("\n\r", ch);

    return TRUE;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* RAIDER - House spells and skills.				       */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void do_escape( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    AFFECT_DATA af, *paf;
af.valid = TRUE;
af.point = NULL;
    int chance;

    one_argument(argument,arg);
 
    if ( (chance = get_skill(ch,gsn_escape)) == 0)
    {	
	send_to_char("Huh?\n\r",ch);
	return;
    }

    if (!stone_check(ch, gsn_escape))
    {
        send_to_char("You cannot draw enough power from the stones.\n\r", ch);
        return;
    }

    if ((victim = ch->fighting) == NULL)
    {
	send_to_char("But you're not fighting anyone!\n\r", ch);
	return;
    }

    if (!IS_NPC(victim))
	for (paf = victim->affected; paf; paf = paf->next)
	    if ((paf->type == gsn_escape) && (paf->modifier == victim->id))
	    {
		send_to_char("They won't fall for that again so soon.\n\r", ch);
		return;
	    }

    if (chance > number_percent())
    {
	do_flee(ch, "");
	if (ch->fighting != NULL)
	    do_flee(ch, "");

	if (ch->in_room != victim->in_room)
	{
	    act("$n plants a swift kick in $N, and escapes with fluid ease.", ch, NULL, victim, TO_NOTVICT);
	    act("You plant a swift kick in $N, and escape swiftly.", ch, NULL, victim, TO_CHAR);
	    act("$n plants a swift kick in your chest, and escapes with fluid ease.", ch, NULL, victim, TO_VICT);
	}
	else
	{
	    act("$n plants a swift kick in $N, but fails to escape!", ch, NULL, victim, TO_NOTVICT);
	    act("You plant a swift kick in $N, but fail to escape!", ch, NULL, victim, TO_CHAR);
	    act("$n plants a swift kick in your chest, but fails to escape!", ch, NULL, victim, TO_VICT);
	}

	if (!IS_NPC(victim))
	{
	    af.where     = TO_AFFECTS;
	    af.type	 = gsn_escape;
	    af.level     = ch->level;
	    af.location  = APPLY_HIDE;
	    af.modifier  = victim->id;
	    af.duration  = 2;
	    af.bitvector = 0;
	    affect_to_char(victim, &af);
	}
		
	if (ch->fighting == NULL)
	{
	    WAIT_STATE(victim, 2*PULSE_VIOLENCE);
	    act("You are stunned by the sharp blow!", ch, NULL, victim, TO_VICT);
	}

	check_improve(ch,victim,gsn_escape,TRUE,1);
    }
    else
    {
	act("$n tries to kick $N, but $N deftly avoids it!", ch, NULL, victim, TO_NOTVICT);
	act("$n tries to kick you, but you deftly avoid it!", ch, NULL, victim, TO_VICT);
	act("You try to kick $N, but $E deftly avoids it!", ch, NULL, victim, TO_CHAR);
	WAIT_STATE(ch, ((PULSE_VIOLENCE*3)/2));
	check_improve(ch,victim,gsn_escape,FALSE,1);
    }
}

bool spell_beltoflooters( int sn, int level,CHAR_DATA *ch,void *vo,int target )
{
    OBJ_DATA *belt;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (!stone_check(ch, gsn_beltoflooters))
    {
        send_to_char("Your magic fails to tap enough power from the stones.\n\r", ch);
        return FALSE;
    }

    belt = create_object(get_obj_index(OBJ_VNUM_RAIDER_BELT), 0);
    belt->value[0]      = ch->level * 2; /* 10 pounds per level capacity */
    belt->value[3]      = ch->level; /* 5 pounds per level max per item */

    af.where     = TO_OBJECT;
    af.type       = sn;
    af.level      = level * 3;
    af.duration  = level * 3;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_obj(belt, &af);

    act("$n has created a $p.",ch,belt,NULL,TO_ROOM);
    send_to_char("You create a looter's belt.\n\r",ch);
    obj_to_char(belt,ch);
    return TRUE;
}

bool spell_birdofprey( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *bird;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (!stone_check(ch, gsn_birdofprey))
    {
        send_to_char("Your magic fails to tap enough power from the stones.\n\r", ch);
        return FALSE;
    }

    if (is_affected(ch, gsn_birdofprey))
    {
        send_to_char("You don't feel you can summon another bird of prey yet.\n\r", ch);
        return FALSE;
    }

    if (ch->pet != NULL)
    {
        send_to_char("You already have a loyal follower, and cannot bind more loyalties.", ch);
        return FALSE;
    }

    bird = create_mobile(get_mob_index(MOB_VNUM_BIRD_PREY));

    bird->level = ch->level;
    bird->damroll = ch->level/2;
    bird->hitroll = (ch->level *2)/3;
    bird->damage[0] = ch->level/2;
    bird->damage[1] = 4;
    bird->damage[2] = ch->level/10;
    bird->hit       = (ch->hit * 2)/3 + number_range(1,ch->hit/3);
    bird->max_hit   = (ch->hit * 2)/3 + number_range(1,ch->hit/3);

    char_to_room(bird, ch->in_room);
    ch->pet = bird;
    bird->master = ch;
    bird->leader = ch;

    act("$n summons a Bird of Prey to serve $m!", ch, NULL, NULL, TO_ROOM);
    act("You summon a Bird of Prey to serve you!", ch, NULL, NULL, TO_CHAR);
    act("$n swoops down out of the skies in response!", bird, NULL, NULL, TO_ROOM);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 48;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    return TRUE;
}

bool spell_pillage( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    DESCRIPTOR_DATA *d, *d_next;

    if (!stone_check(ch, gsn_pillage))
    {
        send_to_char("Your magic fails to tap enough power from the stones.\n\r", ch);
        return FALSE;
    }

    if (!ch->in_room)
        return FALSE;

    if (is_affected(ch, sn))
    {
        send_to_char("You've called the forces of chaos and confusion out too recently to do so again.\n\r", ch);
        return FALSE;
    }

    if (!IS_SET(ch->in_room->room_flags, ROOM_LAW))
    {
        send_to_char("You can only pillage areas governed by the forces of order.\n\r", ch);
        return FALSE;
    }

    if (ch->in_room->clan == clan_lookup("GUARDIAN"))
    {
    	send_to_char("You cannot call the forces of chaos and confusion here!\n\r", ch);
	return FALSE;
    }

    af.where     = TO_AREA;
    af.type      = sn;
    af.level     = level;
    af.duration  = 6;
    af.location  = 0;
    af.modifier  = ch->race;
    af.bitvector = 0;
    affect_to_area(ch->in_room->area, &af);

    af.where     = TO_AREA;
    af.type      = sn;
    af.level     = level;
    af.duration  = 96;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    for (d = descriptor_list; d ; d = d_next)
    {
        d_next = d->next;
        if (!d || !d->character)
            continue;

        if (d->connected == CON_PLAYING
         && d->character->in_room->area == ch->in_room->area)
            send_to_char("With a terrible howl, the winds of chaos tear past your ears, and screams of terror echo as raiders set upon the area!\n\r", d->character);
    }

    return TRUE;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* SHUNNED - House spells and skills.				       */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

bool spell_consumption(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *vch, *lead = NULL;
    bool nums_used[13];
    int casters = 0, x, dnum;

    if (silver_state == 4)
    {
	send_to_char("The consumption of the silver veil has already taken place.\n\r", ch);
	return FALSE;
    }

    if (silver_state == 3)
    {
	send_to_char("The ritual has already been completed.\n\r", ch);
	return FALSE;
    }

    if (silver_state == 2)
    {
	send_to_char("The ritual has already commenced.\n\r", ch);
	return FALSE;
    }

    if (!ch->in_room)
	return FALSE;

    if (silver_state == 1)
    {
	for (x = 0; x < 13; x++)
	    nums_used[x] = FALSE;

	for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
	    if (IS_OAFFECTED(vch, AFF_CONSUMPTION))
	    {
		casters++;
		nums_used[(x = get_modifier(vch->affected, gsn_consumption))] = TRUE;
		if (x == 0)
		   lead = vch; 
	    }

	if (casters == 0)
	{
	    send_to_char("The ritual of the silver veil is already being performed elsewhere.\n\r", ch);
	    return FALSE;
	}

        if (casters >= 13)
        {
	    send_to_char("The ritual already consists of a full regiment of thirteen.\n\r", ch);
	    return FALSE;
	}

	dnum = number_range(1, (13 - casters));

	for (x = 0; x < 13; x++)
	{
	    if (nums_used[x])
	        continue;

	    dnum -= 1;
	    if (dnum == 0)
	    {
	        af.modifier = x;
	        break;
	    }
	}

	af.where	 = TO_OAFFECTS;
	af.type	 = sn;
	af.level	 = level;
	af.duration  = -1;
	af.location  = APPLY_HIDE;
	af.bitvector = AFF_CONSUMPTION;
	affect_to_char(ch, &af);

	act("You join the ritual, tracing a rune on your forehead with $N's blood.", ch, NULL, lead, TO_CHAR);
	act("$n joins the ritual, tracing a rune on $s forehead with $N's blood.", ch, NULL, lead, TO_NOTVICT);
	act("$n joins the ritual, tracing a rune on $s forehead with your blood.", ch, NULL, lead, TO_VICT);
	return TRUE;
    }

    if (silver_state == 0)
    {
	if (!IS_SET(ch->in_room->room_flags, ROOM_POWER_NEXUS))
	{
	    send_to_char("You must be standing in an area of great power to invoke the ritual.\n\r", ch);
	    return FALSE;
	}

	if (time_info.hour < season_table[time_info.season].sun_up)
	{
	    send_to_char("It is too late in the night to begin the ritual.\n\r", ch);
	    return FALSE;
	}

	if (time_info.hour < season_table[time_info.season].sun_down)
	{
	    send_to_char("The ritual of the silver veil can only be performed at night.\n\r", ch);
	    return FALSE;
	}

	af.where     = TO_OAFFECTS;
	af.type      = sn;
	af.level     = level;
	af.duration  = 1;
	af.modifier  = 0;
	af.location  = APPLY_HIDE;
	af.bitvector = AFF_CONSUMPTION;
	affect_to_char(ch, &af);

	act("$n initiates $mself to lead the ritual by scoring a bloody rune into $s own forehead.", ch, NULL, NULL, TO_ROOM);
	act("You initiate yourself to lead the ritual by scoring a bloody rune into your own forehead.", ch, NULL, NULL, TO_CHAR);
	silver_state = 1;
    }

    return TRUE;
}


bool spell_coven(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA *paf;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (!stone_check(ch, gsn_coven))
    {
        send_to_char("Your magic fails to tap enough power from the stones.\n\r", ch);
        return FALSE;
    }

    if (victim == ch)
    {
	send_to_char("You cannot coven with yourself.\n\r", ch);
	return FALSE;
    }

    if (IS_NPC(victim) || (victim->clan != clan_lookup("SHUNNED")))
    {
	send_to_char("They are not a member of the Shunned.\n\r", ch);
	return FALSE;
    }

    for (paf = victim->affected; paf; paf = paf->next)
	if ((paf->type == gsn_coven) && paf->point)
	{
	    send_to_char("They are already lending their power to another.\n\r", ch);
	    return FALSE;
	}

    af.where	 = TO_OAFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.point     = (void *) victim;
    af.duration  = -1;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = AFF_COVEN;
    affect_to_char(ch, &af);

    act("You begin to chant, channeling your mystical powers into $N.",
	ch, NULL, victim, TO_CHAR);
    act("$n begins to chant, channeling $s mystical powers into you.",
	ch, NULL, victim, TO_VICT);
    act("$n begins to chant, channeling $s mystical powers into $N.",
	ch, NULL, victim, TO_NOTVICT);

    return TRUE;
}

bool spell_coverofdarkness( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *sch;
    char buf[MAX_STRING_LENGTH];

    if (!stone_check(ch, gsn_coverofdarkness))
    {
        send_to_char("Your magic fails to tap enough power from the stones.\n\r", ch);
        return FALSE;
    }

    if (is_affected(ch, gsn_coverofdarkness))
        if (IS_PAFFECTED(ch, AFF_SHROUD_OF_NYOGTHUA))
        {
	    send_to_char("You are already shrouded from sight.\n\r", ch);
	    return FALSE;
        }
	else
	{
	    send_to_char("You have too recently been shrouded from sight.\n\r",ch);
	    return FALSE;
	}
    
    int mod = aura_grade(ch);
    if (mod < 1)
    {
        send_to_char("Nyogthua's mocking laughter rings in your ears.\n\r",ch);
	return FALSE;
    }
    
    send_to_char("Violet lights dance briefly around you, concealing your location.\n\r",ch);
    mod *= 4;

    af.where	 = TO_PAFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = mod;
    af.location  = 0;
    af.modifier  = mod;
    af.bitvector = AFF_SHROUD_OF_NYOGTHUA;
    affect_to_char(ch,&af);
  
    af.modifier  = 0;
    char * his_her [] = {"its", "his", "her"};
    for (sch = ch->in_room->people; sch; sch=sch->next)
// If sch is a vanguardsman in the same group
        if (sch != ch && is_same_group(sch,ch) 
	  && sch->clan == clan_lookup("SHUNNED") 
	  && get_skill(sch,gsn_demonic_might) > 0
	  && !is_affected(sch,gsn_coverofdarkness))
	{
	    mod = aura_grade(sch);
	    if (mod < 1)
	        send_to_char("Nyogthua's mocking laughter rings in your ears.\n\r",sch);
	    else
	    {
	        send_to_char("Violet lights dance briefly around you, concealing your location.\n\r",sch);
	        sprintf(buf,"Violet lights dance briefly around %s, concealing %s location.\n\r",sch->name,his_her[sch->sex]);
                send_to_char(buf,ch);
	        
	        af.duration = mod * 2;
	        affect_to_char(sch,&af);
	    }
	}
    
    return TRUE;
}

bool spell_demonsummon( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *vch;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    bool found = FALSE;

    if (!stone_check(ch, gsn_demonsummon))
    {
        send_to_char("Your magic fails to tap enough power from the stones.\n\r", ch);
        return FALSE;
    }

    if (IS_NPC(ch) || !ch->in_room)
	return FALSE;

    if (is_affected(ch, gsn_demonsummon))
    {
	if (get_modifier(ch->affected, gsn_demonsummon) > 0)
	    send_to_char("You are already preparing the tide of Bahhaoth.\n\r", ch);
	else
	    send_to_char("You are not prepared call upon the tide of Bahhaoth again so soon.\n\r", ch);

	return FALSE;
    }

    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
	if (is_affected(vch, gsn_demonsummon) && (get_modifier(vch->affected, gsn_demonsummon) == 1))
	    found = TRUE;

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.location  = APPLY_HIDE;
    af.bitvector = 0;

    if (found)
    {
	send_to_char("Your body tenses as you feel a darkness growing inside of you.\n\r", ch);
	act("$n grows suddenly rigid, spasms flickering across $s face.", ch, NULL, NULL, TO_ROOM);
	af.modifier = af.duration = 2;
    }
    else
    {
	if ((time_info.hour >= 1) && (time_info.hour < season_table[time_info.season].sun_down))
	{
	    send_to_char("The tide of Bahhaoth cannot begin at this time.\n\r", ch);
	    return FALSE;
	}

	send_to_char("Your body tenses as you feel a darkness growing inside of you.\n\r", ch);
	act("$n grows suddenly rigid, spasms flickering across $s face.", ch, NULL, NULL, TO_ROOM);
	af.modifier = af.duration = 1;
    }

    affect_to_char(ch, &af);

    return TRUE;
} 

bool spell_enslave( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    CHAR_DATA *vch;
    int slaves;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (!stone_check(ch, gsn_enslave))
    {
        send_to_char("Your magic fails to tap enough power from the stones.\n\r", ch);
        return FALSE;
    }

    if (IS_NPC(ch))
    {
	send_to_char("You lack enough presence to bind the wills of others.\n\r", ch);
	return FALSE;
    }
   
    if (!IS_NPC(victim)
     || IS_SET(victim->act, ACT_NOSUBDUE)
     || IS_SET(victim->imm_flags, IMM_CHARM)
     || IS_SET(victim->imm_flags, IMM_TAME)
     || (victim->level > ch->level))
    {
	send_to_char("They are too willful to be subjected to your enslavement.\n\r", ch);
	return FALSE;
    }

    if (IS_AFFECTED(victim, AFF_CHARM))
    {
	send_to_char("They are already being subjected to the will of another.\n\r", ch);
	return FALSE;
    }

    if (is_affected(victim, gsn_enslave))
    {
	send_to_char("Another is already attempting to bend their will.\n\r", ch);
	return FALSE;
    }

    slaves = 0;
    for (vch = char_list; vch; vch = vch->next)
        if (is_affected(vch, gsn_enslave) && IS_AFFECTED(vch, AFF_CHARM)
	 && (get_modifier(vch->affected, gsn_enslave) == ch->id))
	    slaves += vch->level;

    if ((slaves + victim->level) > (ch->level * 2))
    {
	send_to_char("You lack sufficient strength of presence to bind them to your will.\n\r", ch);
	return FALSE;
    } 

    act("You concentrate, attempting to bend $N to your will.", ch, NULL, victim, TO_CHAR);
    act("A darkness passes over you as $n attempts to bend you to $s will.", ch, NULL, victim, TO_VICT);
    act("A dim shadow emanates from $n, engulfing $N.", ch, NULL, victim, TO_NOTVICT);

    stop_fighting_all(victim);

    if (victim->hit > ch->hit)
    {
	act("$N resists your attempts to enslave it, and viciously attacks!", ch, NULL, victim, TO_CHAR);
	act("You resist $n's attempts to enslave you, and viciously attack!", ch, NULL, victim, TO_VICT);
	act("$N leaps at $n in fury!", ch, NULL, victim, TO_NOTVICT);
	multi_hit(victim, ch, TYPE_UNDEFINED);
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = 2;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    af.modifier  = ch->id;
    affect_to_char(victim, &af);

    return TRUE;
}
    
bool spell_mantleoffear( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *vch;
    int chance;

    if (!ch->in_room)
	return FALSE;

    if (!stone_check(ch, gsn_mantleoffear))
    {
        send_to_char("Your magic fails to tap enough power from the stones.\n\r", ch);
        return FALSE;
    }

    if (IS_OAFFECTED(ch, AFF_MANTLEOFFEAR))
    {
	send_to_char("You are already cloaked in a mantle of fear.\n\r", ch);
	return FALSE;
    }

    int mod = aura_grade(ch);
    if (mod < 2)
    {
        send_to_char("Your force of presence is not enough to impose fear on others.\n\r",ch);
	return FALSE;
    }
    if (mod == 2) 
        chance = 10;
    else if (mod == 3)
        chance = 20;
    else if (mod == 4)
        chance = 25;
    else
        chance = 0;
   
    af.where	 = TO_OAFFECTS;
    af.type	 = gsn_mantleoffear;
    af.level	 = level;
    af.duration  = level / 5;
    af.modifier  = chance;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_MANTLEOFFEAR;
    affect_to_char(ch, &af);

    send_to_char("You feel a mantle of fear settle about you.\n\r", ch);
    
    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
	if (!IS_NPC(vch) && (vch->clan != clan_lookup("SHUNNED")) && !is_same_group(ch, vch))
	    if (chance == 10)
	        send_to_char("A sense of uneasiness washes over you.\n\r",vch);
	    else if (chance == 20)
	        send_to_char("A sense of trepidation washes over you.\n\r",vch);
	    else if (chance == 25)
	        send_to_char("A sense of fear washes over you.\n\r", vch);

    return TRUE;
}

bool spell_runeofeyes(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (!ch->in_room)
	return FALSE;

    if (!stone_check(ch, gsn_runeofeyes))
    {
        send_to_char("Your magic fails to tap enough power from the stones.\n\r", ch);
        return FALSE;
    }

    if (is_affected(ch, gsn_runeofeyes))
    {
	send_to_char("You have already placed a rune of eyes in this world.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_ROOM;
    af.type	 = sn;
    af.level	 = level;
    af.duration	 = level/2;
    af.location	 = APPLY_NONE;
    af.modifier	 = ch->in_room->vnum;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    send_to_char("Chanting softly, you mark this place with a rune of eyes.\n\r", ch);
    return TRUE;
}

bool spell_soulreaver(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *reaver = (OBJ_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (!stone_check(ch, gsn_soulreaver))
    {
        send_to_char("Your magic fails to tap enough power from the stones.\n\r", ch);
        return FALSE;
    }

    if (is_affected(ch, gsn_soulreaver))
    {
	send_to_char("You are not yet prepared to create another soul reaver.\n\r", ch);
	return FALSE;
    }

    if (!CAN_WEAR(reaver, ITEM_WIELD))
    {
        send_to_char("Only weapons may be desecrated by the magics of the Eternal.\n\r",ch);
	return FALSE;
    }

    if (IS_OBJ_STAT(reaver, ITEM_ANTI_EVIL) || IS_OBJ_STAT(reaver, ITEM_BLESS) || CAN_WEAR(reaver, ITEM_NO_SAC))
    {
        act("$p cannot be desecrated by such magic.",ch,reaver,NULL,TO_CHAR);
	return FALSE;
    }

    if (obj_is_affected(reaver, sn))
    {
	send_to_char("That weapon has already been bound as a soul reaver.\n\r", ch);
	return FALSE;
    }

    act("As you score fell runes of binding and destruction upon $p, you prepare it to reave the souls of the fallen.", ch, reaver, NULL, TO_CHAR);
    act("As $n scores fell runes of binding and destruction upon $p, $e prepares it to reave the souls of the fallen.", ch, reaver, NULL, TO_ROOM);

    if (reaver->pIndexData->vnum == OBJ_VNUM_SOUL_REAVER)
    {
        if (reaver->objfocus[0] != ch)
        {
	    send_to_char("You carry no attunement towards that weapon.\n\r", ch);
	    return FALSE;
        }
        if (reaver->short_descr)
	    free_string(reaver->short_descr);
        if (reaver->description)
	    free_string(reaver->description);
        if (reaver->obj_str)
            free_string(reaver->obj_str);
    
        reaver->obj_str = str_dup("life drain");
        reaver->value[3] = DAM_NEGATIVE;

        if (reaver->value[0] == WEAPON_SWORD)
        {
            setName(*reaver, "reaver rune-scored sword black iron");
            reaver->short_descr = str_dup("a rune-scored sword of black iron");
            reaver->description = str_dup("The black-iron blade of this sword is scored with fell runes.");
        }
        else if (reaver->value[0] == WEAPON_AXE)
        {
	    setName(*reaver, "reaver rune-scored axe black iron");
	    reaver->short_descr = str_dup("a rune-scored axe of black iron");
	    reaver->description = str_dup("The crescent blades of this black-iron war-axe are scored with fell runes.");
        }
        else if (reaver->value[0] == WEAPON_SPEAR)
        {
	    setName(*reaver, "reaver rune-scored spear black iron");
	    reaver->short_descr = str_dup("a runed-scored spear of black iron");
	    reaver->description = str_dup("A tall spear of dark wood is tipped with a black, rune-scored blade.");
        }
        else if (reaver->value[0] == WEAPON_MACE)
        {
	    setName(*reaver, "reaver rune-scored mace black iron");
	    reaver->short_descr = str_dup("a rune-scored mace of black iron");
	    reaver->description = str_dup("A heavy, spiked mace of black iron is scored with fell runes.");
        }
        else if (reaver->value[0] == WEAPON_POLEARM)
        {
	    setName(*reaver, "reaver rune-scored glaive black iron");
	    reaver->short_descr = str_dup("a rune-scored glaive of black iron");
	    reaver->description = str_dup("A tall glaive of black iron is tipped with a dark, rune-scored blade");
        }
        else if (reaver->value[0] == WEAPON_FLAIL)
        {
	    setName(*reaver, "reaver rune-scored flail black iron");
	    reaver->short_descr = str_dup("a rune-scored flail of black iron");
	    reaver->description = str_dup("A black flail bears seven cruelly spiked, rune-scored iron balls.");
        }
        else if (reaver->value[0] == WEAPON_WHIP)
        {
	    setName(*reaver, "reaver three-tailed whip runed");
	    reaver->short_descr = str_dup("a three-tailed whip with runed barbs");
	    reaver->description = str_dup("The three tails of this long, black whip are woven with rune-scored barbs.");
        }
        else if (reaver->value[0] == WEAPON_STAFF)
        {
	    setName(*reaver, "reaver rune-scored quarterstaff");
	    reaver->short_descr = str_dup("a rune-scored quarterstaff");
	    reaver->description = str_dup("A heavy, iron-shod quarterstaff is bound with serrated, rune-scored bands.");
        }
        else if (reaver->value[0] == WEAPON_DAGGER)
        {
	    setName(*reaver, "reaver rune-scored dagger black iron");
	    reaver->short_descr = str_dup("a rune-scored dagger of black iron");
	    reaver->description = str_dup("The black-iron blade of this dagger is scored with fell runes.");
        }
        else if (reaver->value[0] == WEAPON_EXOTIC)
        {
	    setName(*reaver, "reaver rune-scored scythe black iron");
	    reaver->short_descr = str_dup("a rune-scored scythe of black iron");
	    reaver->description = str_dup("A scythe stands here, its black-iron blade scored with fell runes.");
        }
        else
	    return FALSE;
    }

    af.where	 = TO_OBJECT;
    af.type	 = sn;
    af.level	 = level;
    af.duration	 = -1;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = ITEM_ROT_DEATH;
    affect_to_obj(reaver, &af);
    af.bitvector = ITEM_EVIL;
    affect_to_obj(reaver, &af);
    af.type = gsn_reaver_bind;
    af.modifier  = ch->id;
    af.bitvector = 0;
    affect_to_obj(reaver, &af);

    reaver->timer = -1;

    af.where	 = TO_AFFECTS;
    af.duration  = -1;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    return TRUE;
}

void unbind_shunned_demon(CHAR_DATA *victim)
{
    DESCRIPTOR_DATA *td;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    af.where     = TO_AFFECTS;
    af.type      = gsn_demonsummon;
    af.level     = victim->level;
    af.duration  = 12;
    af.location  = APPLY_HIT;
    af.bitvector = 0;

    for (td = descriptor_list; td; td = td->next)
    {
        if (td->character && td->character->shunned_demon
         && (td->character->shunned_demon == victim))
        {
            if (victim->desc)
                write_to_buffer(td, victim->desc->outbuf, 0);
            write_to_buffer(td, "Your binding to the demon is severed, and you return to your own senses, dazed.\n\r", 0);
            if (victim->in_room)
            {
                char_to_room(td->character, victim->in_room);
                    act("$n's form falls to the ground, dazed.", td->character,
NULL, NULL, TO_ROOM);
            }
            else
                char_to_room(td->character, get_room_index(ROOM_VNUM_TEMPLE));
            td->character->hit = UMIN(td->character->hit, td->character->max_hit / 4);
            af.modifier = (td->character->max_hit / 2) * -1;
            affect_to_char(td->character, &af);
            WAIT_STATE(td->character, UMAX(td->character->wait, PULSE_VIOLENCE*5));
            td->character->shunned_demon = NULL;
        } 
    }

    if (victim->desc && victim->desc->original)
    {
        send_to_char("Your binding to the demon is severed, and you return to your own senses, dazed.\n\r", victim);
        if (victim->in_room)
        {
            char_to_room(victim->desc->original, victim->in_room);
            act("$N's form falls to the ground, dazed.", victim, NULL, victim->desc->original, TO_ROOM);
        }
        else
            char_to_room(victim->desc->original, get_room_index(ROOM_VNUM_TEMPLE));
        victim->desc->original->hit = UMIN(victim->desc->original->hit, victim->desc->original->max_hit / 4);
        af.modifier = (victim->desc->original->max_hit / 2) * -1;
        affect_to_char(victim->desc->original, &af);
        victim->desc->original->wait = UMAX(victim->desc->original->wait, PULSE_VIOLENCE*5);
        victim->desc->character     = victim->desc->original;
        victim->desc->original      = NULL;
        victim->desc->character->desc = victim->desc;
        victim->desc                = NULL;
    }
}

void do_dark_insight(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;

    int chance;
 

    if ((chance = get_skill(ch, gsn_dark_insight)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
	return;
    }

    if (!stone_check(ch,gsn_dark_insight))
    {
    	send_to_char("You cannot draw enough power from the Stones.\n\r",ch);
	return;
    }

    if (is_affected(ch, gsn_dark_insight))
    {
	send_to_char("You have already achieved unnatural insight.\n\r",ch);
	return;
    }

    if (ch->mana < skill_table[gsn_dark_insight].min_mana)
    {
        send_to_char("You are too exhausted to heighten your spellpower.\n\r",ch);
	return;
    }

    if (chance < number_percent())
    {
        send_to_char("You search your thoughts, but find no new insight.\n\r",ch);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_dark_insight].beats));
	check_improve(ch,NULL,gsn_dark_insight,FALSE,1);
	return;
    }
    int mod = aura_grade(ch) - 1;
    
    if (mod < 0)
    {
        send_to_char("Your will is not strong enough to claim deeper mystic insight.\n\r",ch);
	return;
    }

    send_to_char("The indomitable Will of the Knowing reinforces your magic.\n\r",ch);
    af.where = TO_AFFECTS;
    af.type = gsn_dark_insight;
    af.level = ch->level;
    af.duration = 24;
    af.location = 0;
    af.modifier = mod;
    af.bitvector = 0;
    affect_to_char(ch, &af);
    
    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_dark_insight].beats));
    expend_mana(ch, skill_table[gsn_dark_insight].min_mana);
    check_improve(ch,NULL,gsn_dark_insight,TRUE,2);
}

void do_demonic_might(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;

    int chance;

    if ((chance = get_skill(ch,gsn_demonic_might)) == 0)
    {
        send_to_char("Huh?\n\r",ch);
	return;
    }

    if (!stone_check(ch,gsn_demonic_might))
    {
        send_to_char("You cannot draw enough power from the Stones.\n\r",ch);
	return;
    }

    if (is_affected(ch,gsn_demonic_might))
    {
        send_to_char("Your body is already reinforced by the strength of the Eternal.\n\r",ch);
	return;
    }

    if (ch->mana < skill_table[gsn_demonic_might].min_mana)
    {
        send_to_char("You are too drained to reinforce your body by will alone.\n\r",ch);
	return;
    }

    if (chance < number_percent())
    {
        send_to_char("You wait anxiously, but fail to receive the strength you seek.\n\r",ch);
	check_improve(ch,NULL,gsn_demonic_might,FALSE,1);
	WAIT_STATE(ch, UMAX(ch->wait,skill_table[gsn_demonic_might].beats));
	return;
    }

    int mod = aura_grade(ch);
    if (mod >= 1)
        mod += 1;
    else
        mod = 0;
    
    if (mod < 0)
    {
        send_to_char("The weight of time is too great for your will to overcome.\n\r",ch);
	return;
    }

    send_to_char("The unrelenting strength of the Eternal burns through you.\n\r",ch);
    af.where = TO_AFFECTS;
    af.type = gsn_demonic_might;
    af.duration = 24;
    af.location = APPLY_RESIST_HOLY;
    af.modifier = -10;
    af.bitvector = 0;
    af.level = ch->level;
    affect_to_char(ch,&af);
    af.modifier = mod;
    af.location = APPLY_STR;
    affect_to_char(ch,&af);
    af.location = APPLY_DEX;
    affect_to_char(ch,&af);
    if ((mod - 2) > 0)
    {
        af.location = APPLY_CON;
	af.modifier = mod - 2;
	affect_to_char(ch,&af);
    }
   
    check_improve(ch,NULL,gsn_demonic_might,TRUE,2);
    expend_mana(ch, skill_table[gsn_demonic_might].min_mana);
    WAIT_STATE(ch, UMAX(ch->wait,skill_table[gsn_demonic_might].beats));
}

bool spell_compact_of_Logor(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;
    int chance = 0;

    if ((chance = get_skill(ch,sn)) == 0)
    {
        send_to_char("Huh?\n\r",ch);
	return FALSE;
    }

    if (!stone_check(ch,gsn_compact_of_Logor))
    {
        send_to_char("You cannot draw enough power from the Stones.\n\r",ch);
	return FALSE;
    }
    
    if (is_affected(ch, sn))
    {
        send_to_char("You have already renewed the Compact.\n\r", ch);
        return FALSE;
    }

    int mod = aura_grade(ch);
    if (mod > 1)
        mod = (mod - 1) * 25;
    else
        mod = 0;
    if (mod == 0)
    {
        send_to_char("You complete the invocation, but remain unnoticed.\n\r",ch);
	return FALSE;
    }
    send_to_char("The dread power of the Iron Throne suffuses you.\n\r",ch);
    af.type = gsn_compact_of_Logor;
    af.modifier = mod;
    af.duration = 24;	    
    af.level = level;
    af.location = APPLY_NONE;
    af.bitvector = 0;
    affect_to_char(ch,&af);
    return TRUE;
}

void do_demonic_focus(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;
    int chance = 0;

    if ((chance = get_skill(ch,gsn_demonic_focus)) == 0)
    {
        send_to_char("Huh?\n\r",ch);
	return;
    }

    if (!stone_check(ch,gsn_demonic_focus))
    {
        send_to_char("You cannot draw enough power from the Stones.\n\r",ch);
	return;
    }
    
    if (is_affected(ch, gsn_demonic_focus))
    {
        send_to_char("Your mind is already focused to its limit.\n\r", ch);
        return;
    }

    if (ch->mana < skill_table[gsn_demonic_focus].min_mana)
    {
        send_to_char("You are too drained to reinforce your mind by will alone.\n\r",ch);
	return;
    }

    if (chance < number_percent())
    {
        send_to_char("You concentrate, but feel no different than before.\n\r",ch);
        check_improve(ch,NULL,gsn_demonic_focus,FALSE,1);
	return;
    }
    
    int mod = aura_grade(ch);
    if (mod > 0)
        mod *= 25;
    else
        mod = 0;

    if (mod == 0)
    {
        send_to_char("The limitations of your mind are too great for your will to overcome.\n\r",ch);
	return;
    }    

    send_to_char("You grow unnaturally calm, your intense focus granting you newfound clarity.\n\r",ch);
    af.type = gsn_demonic_focus;
    af.modifier = mod;
    af.duration = 24;	    
    af.level = ch->level;
    af.location = APPLY_NONE;
    af.bitvector = 0;
    affect_to_char(ch,&af);
    af.location = APPLY_MAXINT;
    af.modifier = 1;
    affect_to_char(ch,&af);
    af.location = APPLY_INT;
    af.modifier = 1;
    affect_to_char(ch,&af);
    check_improve(ch,NULL,gsn_demonic_focus,TRUE,2);
    expend_mana(ch, skill_table[gsn_demonic_focus].min_mana);
    WAIT_STATE(ch, UMAX(ch->wait,skill_table[gsn_demonic_focus].beats));
    return;
}

bool spell_aura_of_corruption(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
   
    if (!stone_check(ch,gsn_aura_of_corruption))
    {
        send_to_char("You cannot draw enough power from the Stones.\n\r",ch);
	return FALSE;
    }

    if (is_affected(ch, gsn_aura_of_corruption))
    {
        send_to_char("You are already projecting your sins onto your enemies.\n\r", ch);
        return FALSE;
    }

    int mod = aura_grade(ch);

    mod += 1;
    if (mod < 3)
    {
        send_to_char("The burden of your sins is not heavy enough to project onto others.\n\r",ch);
	return FALSE;
    }

    send_to_char("You prepare to project your sins onto your foes.\n\r",ch);
    af.type = gsn_aura_of_corruption;
    af.where = TO_PAFFECTS;
    af.modifier = mod;
    af.location = APPLY_NONE;
    af.bitvector = AFF_AURA_OF_CORRUPTION;
    af.duration = 24;
    af.level = level;
    affect_to_char(ch,&af);
    return TRUE;
}
