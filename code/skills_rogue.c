#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <time.h>
#include <cmath>
#include "merc.h"
#include "tables.h"
#include "Encumbrance.h"

/* External declarations */
DECLARE_DO_FUN(do_autoyell);
DECLARE_DO_FUN(do_visible);
DECLARE_DO_FUN(do_wake);

extern	bool	global_bool_ranged_attack;

extern	bool	check_aura	args( ( CHAR_DATA *ch, OBJ_DATA *obj,
					bool damage));
extern	int	hands_free	args( ( CHAR_DATA *ch) );
int	get_cur_capacity	args( ( CHAR_DATA *ch ) );
int	get_max_capacity	args( ( CHAR_DATA *ch ) );

bool check_detectstealth(CHAR_DATA *ch, CHAR_DATA *victim);

extern	int	item_svalue[6];

void do_backstab( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    AFFECT_DATA *paf;
    int chance;

    one_argument( argument, arg );

    if ((chance = get_skill(ch,gsn_backstab)) == 0)
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    if (arg[0] == '\0')
    {
        send_to_char("Backstab whom?\n\r",ch);
        return;
    }

    if (ch->fighting != NULL)
    {
	send_to_char("You're facing the wrong end.\n\r",ch);
	return;
    }
 
    else if ((victim = get_char_room(ch,arg)) == NULL)
    {
        send_to_char("They aren't here.\n\r",ch);
        return;
    }

    if ( victim == ch )
    {
	send_to_char( "How can you sneak up on yourself?\n\r", ch );
	return;
    }

    if ( is_safe( ch, victim ) )
      return;

    if (victim->fighting != NULL)
    {
      send_to_char("You cannot get around the combatants.\n\r",ch);
      return;
    }

    if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL)
    {
	send_to_char( "You need to wield a weapon to backstab.\n\r", ch );
	return;
    }

    if (get_weapon_sn(ch) != gsn_dagger)
    {
        send_to_char("You can only backstab with daggers.\n\r",ch);
        return;
    }

    if (is_affected(victim, gsn_thirdeye))
    {
	    send_to_char("They are far too wary for that.\n\r", ch);
	    return;
    }

    if ((IS_NPC(victim) && (victim->hit < victim->max_hit))
     ||  victim->hit < ((victim->max_hit * 3) / 4))
    {
	act( "$N is hurt and suspicious ... you can't sneak up.",
	    ch, NULL, victim, TO_CHAR );
	return;
    }

    check_killer( ch, victim );
 
    WAIT_STATE( ch, skill_table[gsn_backstab].beats );

    if (victim->position > POS_SLEEPING)
        for (paf = victim->affected; paf; paf = paf->next)
	    if (paf->type == gsn_wariness)
	    {
	        if (number_percent() <= paf->modifier)
	        {
		    act("You sense someone behind you, and narrowly evade $N's backstab!", victim, NULL, ch, TO_CHAR);
		    chance = 0;
	        }
	        break;
	    }

    if (is_affected(victim, gsn_sense_danger))
    {
	send_to_char("You sense incoming danger.\n\r", victim);
	chance /= 2;
    }

    if ( number_percent() < chance || (chance >= 2 && !IS_AWAKE(victim)))
    {
        if (!IS_NPC(victim))
        {
          sprintf( buf, "Help! %s tried to backstab me!", PERS(ch, victim));
          do_autoyell( victim, buf );
        }	
        check_improve(ch,victim,gsn_backstab,TRUE,1);
	multi_hit( ch, victim, gsn_backstab );
    
	if (((obj = get_eq_char(ch, WEAR_DUAL_WIELD)) == NULL)
	 || (obj->value[0] != WEAPON_DAGGER))
	    return;

	chance = get_skill(ch, gsn_dualbackstab);
	chance /= 2;

	if (chance > 0)
	    if (number_percent() < chance)
	    {
		check_improve(ch,victim,gsn_dualbackstab,TRUE,1);
		multi_hit( ch, victim, gsn_dualbackstab );
	    }
	    else
	    {
        	check_improve(ch,victim,gsn_dualbackstab,FALSE,1);
		damage( ch, victim, 0, gsn_dualbackstab,DAM_NONE,TRUE);
	    }
    }
    else
    {
        if (!IS_NPC(victim))
        {
          sprintf( buf, "Help!  %s tried to backstab me!", PERS(ch, victim) );
          do_autoyell( victim, buf );	
        }
        check_improve(ch,victim,gsn_backstab,FALSE,1);
	damage( ch, victim, 0, gsn_backstab,DAM_NONE,TRUE);

	if (((obj = get_eq_char(ch, WEAR_DUAL_WIELD)) == NULL)
	 || (obj->value[0] != WEAPON_DAGGER))
	    return;
    
	chance = get_skill(ch, gsn_dualbackstab);
	chance /= 2;

	if (chance > 0)
	    if (number_percent() < chance)
	    {
		check_improve(ch,victim,gsn_dualbackstab,TRUE,1);
		multi_hit( ch, victim, gsn_dualbackstab );
	    }
	    else
	    {
        	check_improve(ch,victim,gsn_dualbackstab,FALSE,1);
		damage( ch, victim, 0, gsn_dualbackstab,DAM_NONE,TRUE);
	    }
    }

    return;
}

void do_bind( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int chance;

    if ((chance = get_skill(ch,gsn_bind)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("Bind whom?\n\r", ch);
	return;
    }

    if ((victim = get_char_room(ch, argument)) == NULL)
    {
	send_to_char("You don't see them here.\n\r", ch);
	return;
    }

    if (is_safe(ch, victim))
    {
	send_to_char("The gods protect your victim.\n\r", ch);
        return;
    }

    if (is_affected(victim, gsn_bind))
    {
	send_to_char("They are already bound.\n\r", ch);
	return;
    }

    if (IS_AWAKE(victim))
    {
	send_to_char("You can't bind them while they're awake!\n\r", ch);
	return;
    }

    if (number_percent() > chance)
    {
        act("You fail to get the hood over $N's head.", ch, NULL, victim, TO_CHAR);
        act("$n attempts to pull a hood over $N's head, but fails.", ch, NULL, victim, TO_NOTVICT);
        check_improve(ch,victim,gsn_bind,FALSE,2);
    }
    else
    {    
        af.where      = TO_AFFECTS;
        af.type       = gsn_bind;
        af.level      = ch->level;
        af.duration   = ch->level/10;
        af.location   = APPLY_DEX;
        af.modifier   = -4;
        af.bitvector  = AFF_BLIND;
	affect_to_char(victim, &af);

	act("$n swiftly pulls a hood over $N.", ch, NULL, victim, TO_NOTVICT);
	act("You swiftly pull a hood over $N.", ch, NULL, victim, TO_CHAR);
	check_improve(ch,victim,gsn_bind,TRUE,2);
    }
    do_wake(victim, "");
    if (IS_AWAKE(victim))
    {
	act("Sensing $n pulling something over your head, you jolt upright!", ch, NULL, victim, TO_VICT);
	act("Sensing the hood being pulled over $s head, $n jolts upright!", victim, NULL, NULL, TO_ROOM);
	WAIT_STATE(victim, 2*PULSE_VIOLENCE);
    }
    else
    {
	if (number_percent() > chance)
        {
            act("With $N unable to wake, $n attempts to bind $S legs, but fails.", ch, NULL, victim, TO_NOTVICT);
	    act("With $N unable to wake, you attempt to bind $S legs, but fail.", ch, NULL, victim, TO_CHAR);
            check_improve(ch,victim,gsn_bind,FALSE,2);
        }
        else
        {
            act("With $N unable to wake, $n quickly follows up by trussing $S legs.", ch, NULL, victim, TO_NOTVICT);
	    act("With $N unable to wake, you quickly follow up by trussing $S legs.", ch, NULL, victim, TO_CHAR); 
	    check_improve(ch,victim,gsn_bind,TRUE,2);
            af.where      = TO_AFFECTS;
            af.type       = gsn_bind;
            af.level      = ch->level;
            af.duration   = ch->level/10;
            af.location   = APPLY_DEX;
            af.modifier   = -4;
            af.bitvector  = 0;
    	    affect_to_char(victim, &af);

	    victim->move /= 3;
	}
    }
}

void do_bolo(CHAR_DATA *ch, char *argument)
{
    ROOM_INDEX_DATA *troom, *room;
    CHAR_DATA *victim;
    AFFECT_DATA af, *paf;
af.valid = TRUE;
af.point = NULL;
    char arg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int chance, direction;

    if ((chance = get_skill(ch, gsn_bolo)) == 0)
    {
	send_to_char("Huh?\n\r", ch);
        return;
    }

    if (IS_NAFFECTED(ch, AFF_BOLO))
    {
        send_to_char("You're too tangled to do that!\n\r", ch);
        return;
    }
    
    if (is_affected(ch, gsn_bolo))
    {
        send_to_char("You're not ready to throw another bolo.\n\r", ch);
        return;
    }

    if (argument[0] == '\0')
    {
        if (ch->fighting)
	    victim = ch->fighting;
	else
	{
	    send_to_char("Throw the bolo at who? In which direction?\n\r", ch);
            return;
	}
    }

    argument = one_argument(argument, arg);

    if (argument[0] == '\0')
	direction = -1;
    else
	if (!str_prefix(argument, "north")) direction = 0;
	else if (!str_prefix(argument, "east")) direction = 1;
    	else if (!str_prefix(argument, "south")) direction = 2;
    	else if (!str_prefix(argument, "west")) direction = 3;
    	else if (!str_prefix(argument, "up")) direction = 4;
    	else if (!str_prefix(argument, "down")) direction = 5;
    	else direction = -1;

    if (direction == -1) /* boloing person in room */
    {
	if (arg[0] == '\0')
	{
	    if ((victim = ch->fighting) == NULL)
	    {
		send_to_char("Who did you want to entangle with a bolo?",ch);
		return;
	    }
	}
	else if ((victim = get_char_room(ch, arg)) == NULL)
	{
	    send_to_char("You don't see them here.\n\r", ch);
	    return;
	}

	if (ch->fighting && ch->fighting != victim)
	{
	    send_to_char("You can't bolo someone you're not already directing attacks to.\n\r",ch);
	    return;
	}

	if (ch == victim)
        {
	    send_to_char("You can't properly throw a bolo at yourself.\n\r", ch);
	    return;
	}

	if (is_safe(ch, victim))
	    return;

	if (victim->position <= POS_SLEEPING)
	{
	    send_to_char("You can't bolo someone who's on the ground.\n\r", ch);
	    return;
	}

	if (is_affected(victim, gsn_reflectiveaura))
	    if (number_percent() < get_skill(victim,gsn_reflectiveaura) / 2)
	    {
	    	act("The bolo wraps around $N, and flies back towards you!",ch, NULL, victim, TO_CHAR);
	    	act("Flows of air redirect the bolo, sending it back at $n!",ch, NULL, victim, TO_VICT);
	    	act("The bolo wraps around $N, flying back towards $n!",ch,NULL,victim,TO_VICTROOM);
	    	victim = ch;
	    }
	    else
	    {
	        act("The bolo wraps around $N, and flies harmlessly to the ground.",ch,NULL,victim,TO_CHAR);
	        act("Flows of air redirect the bolo, sending it harmlessly to the ground.", ch, NULL, victim, TO_VICT);
	        act("The bolo wraps around $N, and flies harmlessly to the ground.",ch,NULL,victim,TO_CHAR);
	        return;
	    }

	chance = chance * 3 / 4;

	if (is_affected(victim, gsn_sense_danger))
	{
	    send_to_char("You sense incoming danger.\n\r", victim);
	    chance /= 2;
	}

	chance += (get_curr_stat(ch, STAT_DEX) - 18);
	chance -= ((get_curr_stat(victim, STAT_DEX) - 18) * 2);
	chance += ch->level - victim->level;

	if (!can_see(ch, victim))
	  chance += 10;

        for (paf = victim->affected; paf; paf = paf->next)
	    if (paf->type == gsn_wariness)
	    {
	        if (number_percent() <= paf->modifier)
	        {
		    act("You sense danger, and narrowly evade an incoming bolo!", victim, NULL, NULL, TO_CHAR);
		    chance = 0;
	        }
	        break;
	    }

	af.where     = TO_NAFFECTS;
	af.type      = gsn_bolo;
	af.level     = ch->level;
	af.duration  = 3;
	af.location  = 0;
	af.modifier  = 0;
	af.bitvector = 0;
	affect_to_char( ch, &af );

	act("You throw a bolo at $N!", ch, NULL, victim, TO_CHAR);
	act("$n spins off a bolo toward $N!", ch, NULL, victim, TO_NOTVICT);
	act("$n spins off a bolo toward you!", ch, NULL, victim, TO_VICT);
        WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_bolo].beats));
	if (number_percent() > chance)
	{
	    act("Your bolo misses.", ch, NULL, victim, TO_CHAR);
	    sprintf(buf, "Help! %s is attacking me!", PERS(ch, victim));
	    if (!IS_NPC(victim))
	        do_autoyell(victim, buf);
	    check_killer(ch, victim);
	    one_hit(victim, ch, TYPE_UNDEFINED, HIT_PRIMARY, false);
        check_improve(ch,victim,gsn_bolo,FALSE,2);
	    return;
	}
	else
	{
        damage( ch, victim, number_range(10,20), gsn_bolo, DAM_BASH, TRUE );
	    act("$N is entangled by the bolo!",ch,NULL,victim,TO_CHAR);

	    sprintf(buf, "Help! %s is attacking me!", PERS(ch, victim));
	    if (IS_AWAKE(victim) && !IS_NPC(victim))
	        do_autoyell(victim, buf);
	    
	    af.where     = TO_NAFFECTS;
	    af.type      = gsn_bolo;
	    af.level     = ch->level;
	    af.duration  = 1;
	    af.location  = APPLY_DEX;
	    af.modifier  = -3;
	    af.bitvector = AFF_BOLO;
	    affect_to_char( victim, &af );
	}
    }
    else  /* boloing person in adjacent room */
    {
	if (ch->fighting)
	{
	    send_to_char("You can't properly aim so far away while already fighting.\n\r",ch);
	    return;
	}
	if ((room = ch->in_room) == NULL)
	    return;

	if (!room->exit[direction]
	|| ((troom = ch->in_room->exit[direction]->u1.to_room) == NULL)
	|| !troom->exit[OPPOSITE(direction)]
	|| (troom->exit[OPPOSITE(direction)]->u1.to_room != ch->in_room))
	{
	    send_to_char("You don't think you can hit them from here.\n\r", ch);
	    return;
	}

	if (room->exit[direction]->exit_info & EX_CLOSED
	|| room->exit[direction]->exit_info & EX_WALLED
	|| room->exit[direction]->exit_info & EX_ICEWALL
	|| room->exit[direction]->exit_info & EX_WALLOFFIRE
	|| room->exit[direction]->exit_info & EX_FAKE)
	{
	    send_to_char("You can't throw a bolo there.\n\r", ch);
	    return;
	}

	if ((victim = get_char_room(ch, troom, arg)) == NULL)
	{
	    send_to_char("You don't see them there.\n\r", ch);
	    return;
	}

	if (victim == ch)
	{
	    send_to_char("You can't properly throw a bolo at yourself.\n\r", ch);
	    return;
	}

	if (is_safe(ch, victim))
	    return;

    if (victim->position <= POS_SLEEPING)
	{
	    send_to_char("You can't bolo someone who's on the ground.\n\r", ch);
	    return;
	}

	af.where     = TO_NAFFECTS;
	af.type      = gsn_bolo;
	af.level     = ch->level;
	af.duration  = 3;
	af.location  = 0;
	af.modifier  = 0;
	af.bitvector = 0;
	affect_to_char( ch, &af );

	sprintf(buf, "$n spins off a bolo, throwing it %s.",
	direction == 0 ? "to the north" :
	(direction == 1 ? "to the east" :
	(direction == 2 ? "to the south" :
	(direction == 3 ? "to the west" :
	(direction == 4 ? "up" :
	"down")))));
	act(buf, ch, NULL, NULL, TO_ROOM);

	sprintf(buf, "A bolo flies in from %s.", 
	OPPOSITE(direction) == 0 ? "the north" :
	(OPPOSITE(direction) == 1 ? "the east" :
	(OPPOSITE(direction) == 2 ? "the south" :
	(OPPOSITE(direction) == 3 ? "the west" :
	(OPPOSITE(direction) == 4 ? "above" :
	"below")))));
	act(buf, ch, NULL, victim, TO_VICTROOM);
	act(buf, ch, NULL, victim, TO_VICT);

	chance = chance * 2 / 5;

	if (is_affected(victim, gsn_sense_danger))
	{
	    send_to_char("You sense incoming danger.\n\r", victim);
	    chance /= 2;
	}

	chance += (get_curr_stat(ch, STAT_DEX) - 18);
	chance -= ((get_curr_stat(victim, STAT_DEX) - 18) * 2);
	chance += ch->level - victim->level;

	if (!can_see(ch, victim))
	    chance += 10;

        for (paf = victim->affected; paf; paf = paf->next)
	    if (paf->type == gsn_wariness)
	    {
	        if (number_percent() <= paf->modifier)
	        {
		    act("You sense danger, and narrowly evade an incoming bolo!", victim, NULL, NULL, TO_CHAR);
		    chance = 0;
	        }
	        break;
	    }

        WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_bolo].beats));
	if (chance < number_percent())
	{
	    act("Your bolo misses.", ch, NULL, victim, TO_CHAR);
	    act("$n's bolo misses you by a thread.", ch, NULL, victim, TO_VICT);
	    sprintf(buf, "Help! %s is attacking me!", PERS(ch, victim));
	    if (!IS_NPC(victim))
	        do_autoyell(victim, buf);
            damage( ch, victim,0, gsn_bolo, DAM_BASH, TRUE );
            check_improve(ch,victim,gsn_bolo,FALSE,2);
	}
	else
	{
	    act("$N is entangled by the bolo!",ch,NULL,victim,TO_CHAR);
	    sprintf(buf, "Help! %s is attacking me!", PERS(ch, victim));
	    if (IS_AWAKE(victim) && !IS_NPC(victim))
	        do_autoyell(victim, buf);
	    
	    af.where     = TO_NAFFECTS;
	    af.type      = gsn_bolo;
	    af.level     = ch->level;
	    af.duration  = 1;
	    af.location  = APPLY_DEX;
	    af.modifier  = -3;
	    af.bitvector = AFF_BOLO;
	    affect_to_char( victim, &af );
            damage( ch, victim, number_range(10,20), gsn_bolo, DAM_BASH, TRUE );
	}
    }
}

void do_caltraps(CHAR_DATA *ch, char *argument)
{
    int direction;
    int chance;
    ROOM_INDEX_DATA *room;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *victim;
    EXIT_DATA *pexit;

    if ((chance = get_skill(ch, gsn_caltraps)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if ((room = ch->in_room) == NULL)
	return;

    if (argument[0] == '\0')
    {
      /* not fleeing */
      if (ch->fighting != NULL)
      {
	send_to_char("You can't spread caltraps around the room while you're fighting.\n\r", ch);
	return;
      }

      if (room_is_affected(ch->in_room, gsn_caltraps))
      {
	send_to_char("There are already caltraps spread around the room.\n\r", ch);
	return;
      }

      if (!ON_GROUND(ch))
      {
	send_to_char("You need to be on solid ground for caltraps to be useful.\n\r", ch);
	return;
      }

        af.where        = TO_ROOM_AFF;
        af.type 	= gsn_caltraps;
        af.level        = ch->level;
        af.duration     = 0;
        af.location     = 0;
        af.modifier     = ch->level;
        af.bitvector    = 0;
        affect_to_room(room, &af);

	act("$n tosses caltraps around the room.", ch, NULL, NULL, TO_ROOM);
	act("You toss caltraps around the room.", ch, NULL, NULL, TO_CHAR);
        WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_caltraps].beats));
	return;
    }
    else
    {
      if ((victim = ch->fighting) == NULL)
      {
	send_to_char("But you're not fighting anyone!\n\r", ch);
	return;
      }


	if (is_affected(ch, gsn_entangle))
        {
	    send_to_char("You're too entangled to escape like that!\n\r", ch);
	    return;
	}


      if (!str_prefix(argument, "north")) direction = 0;
      else if (!str_prefix(argument, "east")) direction = 1;
      else if (!str_prefix(argument, "south"))direction = 2;
      else if (!str_prefix(argument, "west")) direction = 3;
      else if (!str_prefix(argument, "up")) direction = 4;
      else if (!str_prefix(argument, "down")) direction = 5;
      else
	{
	  send_to_char("That's not a direction!\n\r", ch);
	  return;
	}

        if ( ( pexit = room->exit[direction] ) == 0
        ||   pexit->u1.to_room == NULL
        ||   (IS_SET(pexit->exit_info, EX_CLOSED)
        &&   !IS_AFFECTED(ch, AFF_PASS_DOOR))
        ||   number_range(0,ch->daze) != 0
	||   IS_SET(pexit->exit_info, EX_NOFLEE)
        ||   is_affected(ch, gsn_whirlpool)
        || ( IS_NPC(ch)
        &&   IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB) ) )
            {
	      send_to_char("You can't escape that way!\n\r", ch);
	      return;
	    }

      if (is_safe(ch, victim))
	return;

      act("$n tosses caltraps down as $e attempts to escape!", ch, NULL, NULL, TO_ROOM);
      act("You toss caltraps down as you attempt to escape!", ch, NULL, NULL, TO_CHAR);


        WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_caltraps].beats));
	chance += (ch->level - victim->level);
	chance -= (get_curr_stat(victim, STAT_DEX) - 18);

	if (number_percent() > chance)
	{
	  act("The caltraps miss!", ch, NULL, NULL, TO_ROOM);
	  act("The caltraps miss!", ch, NULL, NULL, TO_CHAR);
          check_improve(ch,victim,gsn_caltraps,FALSE,1);
	  return;
	}

        check_improve(ch,victim,gsn_caltraps,TRUE,1);

	if (!is_flying(victim))
	{
	  act("$N winces in agony as $E missteps into the caltraps!", ch, NULL, victim, TO_NOTVICT); 
	  act("$N winces in agony as $E missteps into the caltraps!", ch, NULL, victim, TO_CHAR); 
	  act("You wince in agony as you misstep into the caltraps!", ch, NULL, victim, TO_VICT); 
          af.where        = TO_AFFECTS;
          af.type         = gsn_caltraps;
          af.level        = ch->level;
          af.duration     = 5;
          af.location     = APPLY_DEX;
          af.modifier     = -2;
          af.bitvector    = 0;
          affect_to_char(victim, &af);
          WAIT_STATE(victim, UMAX(victim->wait, skill_table[gsn_caltraps].beats));
          damage( ch, victim, number_range(10,20), gsn_caltraps, DAM_PIERCE, TRUE );
	}
	else
	{
	  act("$n drifts harmlessly above the caltraps.", victim, NULL, NULL, TO_ROOM);
	  act("You drift harmlessly above the caltraps.", victim, NULL, NULL, TO_CHAR);
	}

	stop_fighting_all(ch);
	move_char(ch, direction, FALSE);
     }

}


void do_blindingcloud( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim, *vch_next;
    int chance,vchance;

    if (ch->in_room && ch->in_room->sector_type == SECT_UNDERWATER)
    {
	send_to_char("You can't throw dust underwater.\n\r",ch);
	return;
    }
    if ( (chance = get_skill(ch,gsn_blindingcloud)) == 0)
    {
	send_to_char("Huh?\n\r",ch);
	return;
    }

    if (number_percent() > chance)
    {
	send_to_char("You throw your dust into the air, but it settles harmlessly.\n\r", ch);
	act("$n throws dust into the air, but it settles harmlessly.", ch, NULL, NULL, TO_ROOM);
	check_improve(ch,NULL,gsn_blindingcloud,TRUE,1);
	WAIT_STATE(ch,skill_table[gsn_blindingcloud].beats);
	return;
    }
    else
    {
	check_improve(ch,NULL,gsn_blindingcloud,TRUE,1);
	WAIT_STATE(ch,skill_table[gsn_blindingcloud].beats);
	send_to_char("You throw a cloud of dust into the air!\n\r", ch);
	act("$n throws a cloud of dust into the air!", ch, NULL, NULL, TO_ROOM);
    }

    chance /= 2;
    chance += ch->level;
    if (IS_AFFECTED(ch,AFF_HASTE) || IS_SET(ch->off_flags,OFF_FAST))
	chance += 10;

    if (ch->in_room->sector_type == SECT_INSIDE
      || IS_SET(ch->in_room->room_flags, ROOM_INDOORS))
	chance += 25;

/******* ok, loop and see who gets blinded ****/
    for (victim = ch->in_room->people; victim != NULL; victim = vch_next)
    {
	vch_next = victim->next_in_room;

        if (IS_IMMORTAL(victim))
            continue;

	if (is_same_group(ch, victim))
	    continue;

        if (is_safe(ch,victim))
            continue;

	if (is_safe_spell(ch, victim, TRUE))
	    continue;

	if (IS_SET(victim->imm_flags, IMM_BLIND))
	    continue;

	if (IS_AFFECTED(victim, AFF_BLIND))
	    continue;
	
	check_killer(ch, victim);

	vchance = chance - victim->level;
	if (IS_AFFECTED(victim,AFF_HASTE) || IS_SET(victim->off_flags,OFF_FAST))
	    vchance -= 30;
	if (number_percent() < vchance)
        {
	    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
            if ((ch->fighting != victim) && (!IS_NPC(victim)))
            {
          	sprintf( buf, "I can't see! Someone threw dust in my eyes!");
          	do_autoyell( victim, buf );
            }	

            act("$n is blinded by the dust in $s eyes!",victim,NULL,NULL,TO_ROOM);
            damage(ch,victim,number_range(2,5),gsn_blindingcloud,DAM_NONE,FALSE);
	    send_to_char("You can't see a thing!\n\r",victim);

	    af.where	= TO_AFFECTS;
	    af.type 	= gsn_blindingcloud;
	    af.level 	= ch->level;
  	    af.duration	= 2; 
	    af.location	= APPLY_HITROLL;
	    af.modifier	= -5;
	    af.bitvector = AFF_BLIND;

	    affect_to_char(victim,&af);
        }
        else
        {
            if ((ch->fighting != victim) && (!IS_NPC(victim)))
            {
                sprintf( buf, "Help!  %s threw dust in my eyes!", PERS(ch, victim) );
          	do_autoyell( victim, buf );
            }	

	    if (victim->fighting == NULL)
		one_hit(victim, ch, TYPE_UNDEFINED, HIT_PRIMARY, false);
        }

    } /* end of the people in room loop */
}

void do_blindingdust( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;

    one_argument(argument,arg);

    if (ch->in_room && ch->in_room->sector_type == SECT_UNDERWATER)
    {
	send_to_char("You can't throw dust underwater.\n\r",ch);
	return;
    }
    if ( (chance = get_skill(ch,gsn_blindingdust)) == 0)
    {
	send_to_char("Huh?\n\r",ch);
	return;
    }

    if (arg[0] == '\0')
    {
	victim = ch->fighting;
	if (victim == NULL)
	{
	    send_to_char("But you aren't in combat!\n\r",ch);
	    return;
	}
    }

    else if ((victim = get_char_room(ch,arg)) == NULL)
    {
	send_to_char("They aren't here.\n\r",ch);
	return;
    }

    if (IS_SET(victim->imm_flags, IMM_BLIND))
    {
	send_to_char("You failed.\n\r", ch);
	return;
    }

    if (IS_AFFECTED(victim,AFF_BLIND))
    {
	act("$E's already been blinded.",ch,NULL,victim,TO_CHAR);
	return;
    }

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;

    check_killer(ch, victim);

    if (victim == ch)
    {
	send_to_char("You throw the dust into your eyes! You are blinded!\n\r",ch);
	af.where	= TO_AFFECTS;
	af.type 	= gsn_blindingdust;
	af.level 	= ch->level;
	af.duration	= 3;
	af.location	= APPLY_HITROLL;
	af.modifier	= -5;
	af.bitvector 	= AFF_BLIND;
	affect_to_char(ch, &af);
	return;
    }

    if (victim->position <= POS_SLEEPING)
    {
	send_to_char("You can't throw dust into closed eyes.\n\r", ch);
	return;
    }

    if (is_safe(ch,victim))
	return;

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
	act("But $N is such a good friend!",ch,NULL,victim,TO_CHAR);
	return;
    }

    if (IS_AFFECTED(ch,AFF_HASTE) || IS_SET(ch->off_flags,OFF_FAST))
	chance += 10;
    if (IS_AFFECTED(victim,AFF_HASTE) || IS_SET(victim->off_flags,OFF_FAST))
	chance -= 30;
    chance += ch->level - victim->level;
    
    chance = (chance * 3/4);

    if (IS_SET(victim->imm_flags, IMM_BLIND))
	chance = 0;

    /* now the attack */
    if (number_percent() < chance)
    {
        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
          sprintf( buf, "I can't see! Someone threw dust in my eyes!");
          do_autoyell( victim, buf );
        }	
        act("$n is blinded by the dust in $s eyes!",victim,NULL,NULL,TO_ROOM);
	act("$n throws dust in your eyes!",ch,NULL,victim,TO_VICT);
	send_to_char("You can't see a thing!\n\r",victim);
	check_improve(ch,victim,gsn_blindingdust,TRUE,2);
	WAIT_STATE(ch,skill_table[gsn_blindingdust].beats);
        damage(ch,victim,0,gsn_blindingdust,DAM_NONE,FALSE);
	
	af.where	= TO_AFFECTS;
	af.type 	= gsn_blindingdust;
	af.level 	= ch->level;
	af.duration	= number_bits(1) == 0 ? 3 : 2;
	af.location	= APPLY_HITROLL;
	af.modifier	= -4;
	af.bitvector 	= AFF_BLIND;

	affect_to_char(victim,&af);
    }
    else
    {
        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
          sprintf( buf, "Help!  %s tried to throw dust in my eyes!", PERS(ch, victim) );
          do_autoyell( victim, buf );
        }	
	act("$N avoids your dust!",ch,NULL,victim,TO_CHAR);
        act("$n avoids $N's dust!",victim,NULL,ch,TO_NOTVICT);
	act("$n throws dust at you, but you slip out of the way!",ch,NULL,victim,TO_VICT);
        damage(ch,victim,0,gsn_blindingdust,DAM_NONE,TRUE);
	check_improve(ch,victim,gsn_blindingdust,FALSE,2);
	WAIT_STATE(ch,skill_table[gsn_blindingdust].beats);
    }
}

void do_cutpurse( CHAR_DATA *ch, char *argument )
{
    char buf  [MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *vch;
    int percent, wskill, chance;

    if ( (chance = get_skill(ch,gsn_cutpurse)) == 0)
    {
	send_to_char("Huh?\n\r",ch);
	return;
    }

    if (argument[0] == '\0')
    {
	send_to_char( "Cut what on whom?\n\r", ch );
	return;
    }

    if ((victim = get_char_room(ch, argument)) == NULL)
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if (victim == ch)
    {
	send_to_char( "You think the better of it, and decide not to.\n\r", ch );
	return;
    }

    if (is_safe(ch,victim))
	return;

    if ( IS_NPC(victim) 
	  && victim->position == POS_FIGHTING)
    {
	send_to_char("You can't do that in the middle of a fight.\n\r",ch);
	return;
    }

    WAIT_STATE( ch, skill_table[gsn_cutpurse].beats );
    percent = number_percent();

    if (!IS_AWAKE(victim))
    	percent -= 10;
    else if (!can_see(victim,ch))
    	percent += 25;
    else 
	percent += 50;

    if (!IS_NPC(victim) && BIT_GET(victim->pcdata->traits, TRAIT_STREETWISE))
	chance -= 10;

    check_killer(ch, victim);

    if (( percent > chance) && (!IS_IMMORTAL(ch)))
    {
	/*
	 * Failure.
	 */
	send_to_char( "You wince as you miss.\n\r", ch );
	affect_strip(ch,gsn_sneak);
	REMOVE_BIT(ch->affected_by,AFF_SNEAK);
	REMOVE_BIT(ch->affected_by,AFF_HIDE);

	act( "$n was going for your purse with a knife.", ch, NULL, victim, TO_VICT    );
	act( "$n was going for $N's purse with a knife.",  ch, NULL, victim, TO_NOTVICT );
	switch(number_range(0,3))
	{
	case 0 :
	   sprintf( buf, "%s is a lousy thief!", PERS(ch, victim));
	   break;
        case 1 :
	   sprintf( buf, "%s couldn't rob %s way out of a paper bag!",
		    PERS(ch,victim),(ch->sex == 2) ? "her" : "his");
	   break;
	case 2 :
	    sprintf( buf,"%s tried to rob me!", PERS(ch, victim) );
	    break;
	case 3 :
	    sprintf(buf,"Keep your hands out of there, %s!",PERS(ch, victim));
	    break;
        }
        if (!IS_AWAKE(victim))
            do_wake(victim,"");
	if (IS_AWAKE(victim))
          if (!IS_NPC(victim) && !IS_NPC(ch))
	    do_autoyell( victim, buf );
	    if ( IS_NPC(victim) )
	    {
	        check_improve(ch,victim,gsn_cutpurse,FALSE,2);
		multi_hit( victim, ch, TYPE_UNDEFINED );
	    }
	    else
	    {
		sprintf(buf,"$N tried to cutpurse %s.",victim->name);
		wiznet(buf,ch,NULL,WIZ_FLAGS,0,0);
	    }

	return;
    }

    int i;
    long coins[MAX_COIN];
    bool cFound = FALSE;

    for (i = 0; i < MAX_COIN; i++)
    {
	coins[i] = round(victim->coins[i] * number_range(1, ch->level) / 60.0);

	if (coins[i] > 0)
	    cFound = TRUE;
    }

    if (cFound)
    {
	dec_player_coins(victim,coins);
	inc_player_coins(ch,coins);
	sprintf(buf, "You slit open their purse, and get %s.\n\r", coins_to_str(coins));
	send_to_char(buf, ch);
    }
    else
	send_to_char("You slit open their purse, but nothing falls out.\n\r", ch);

    check_improve(ch,victim,gsn_cutpurse,TRUE,2);

    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
	if (ch == vch)
	    continue;
	if (((wskill = get_skill(vch, gsn_detecttheft)) > 0)
	  && (wskill > number_percent()))
	{
    	    check_improve(vch,ch,gsn_detecttheft,TRUE,2);
	    if (vch == victim)
	    {
		sprintf(buf, "You look down just in time to see %s cut your purse and get %s!\n\r", PERS(ch, vch), coins_to_str(coins));
		send_to_char(buf, vch);
	    }
	    else
	    {
		sprintf(buf, "You notice %s cut %s's purse and get %s.\n\r", PERS(ch, vch), PERS(victim, vch), coins_to_str(coins));
		send_to_char(buf, vch);
	    }
	}
    }

    return;
}

void do_dart(CHAR_DATA *ch, char *argument)
{
    ROOM_INDEX_DATA *troom, *room;
    CHAR_DATA *victim;
    OBJ_DATA *dart;
    AFFECT_DATA *paf;
    char arg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int chance, direction, type;

    if ((chance = get_skill(ch, gsn_dart)) == 0)
    {
      send_to_char("Huh?\n\r", ch);
      return;
    }

    if ((dart = get_obj_carry(ch, "dart", ch)) == NULL)
    {
      send_to_char("You aren't carrying a dart!\n\r", ch);
      return;
    }

    if (argument[0] == '\0')
    {
      send_to_char("Dart whom? in which direction?\n\r", ch);
      return;
    }

    argument = one_argument(argument, arg);

    if (ch->fighting != NULL)
    {
	send_to_char("You can't throw a dart properly while fighting!\n\r", ch);
	return;
    }

    if (argument[0] == '\0') /* darting person in room */
    {
	if ((victim = get_char_room(ch, arg)) == NULL)
	{
	    send_to_char("You don't see them here.\n\r", ch);
	    return;
	}

	if (is_safe(ch, victim))
	    return;

	if (is_affected(victim, gsn_flameshield) || is_affected(victim, gsn_lambentaura))
        {
            act("Your dart collides with $N's flame shield, and is incinerated!", ch, NULL, victim, TO_CHAR);
            act("A small dart strikes your flame shield, and is incinerated.", ch, NULL, victim, TO_VICT);
            extract_obj(dart);
            return;
        }
	
	if (is_affected(victim, gsn_reflectiveaura))
	    if (number_percent() < get_skill(victim,gsn_reflectiveaura) / 2)
	    {
		act("The dart wraps around $N, and flies back towards you!",ch, NULL, victim, TO_CHAR);
		act("Flows of air redirect the dart, sending it back at $n!",ch, NULL, victim, TO_VICT);
		act("The dart wraps around $N, flying back towards $n!",ch,NULL,victim,TO_VICTROOM);
		victim = ch;
	    }
	    else
	    {
		act("The dart wraps around $N, and flies harmlessly to the ground.",ch,NULL,victim,TO_CHAR);
		act("Flows of air redirect the dart, sending it harmlessly to the ground.", ch, NULL, victim, TO_VICT);
		act("The dart wraps around $N, and flies harmlessly to the ground.",ch,NULL,victim,TO_CHAR);
		extract_obj(dart);
		return;
	    }

	if (is_affected(victim, gsn_protnormalmissiles) && (number_percent() < victim->level))
	{
	    send_to_char("A dart strikes you, but deflects harmlessly away.\n\r" , victim);
	    act("Your dart strikes $N, but deflects harmlessly away.", ch, NULL, victim, TO_CHAR);
	    extract_obj(dart);
	    return;
	}
        
	if (is_affected(victim, gsn_sense_danger))
	{
	    send_to_char("You sense incoming danger.\n\r", victim);
	    chance /= 2;
	}

	chance += ch->level - victim->level;
        
	if (IS_AFFECTED(victim, AFF_HASTE))
	    chance -= 20;	
        if (can_see(victim, ch))
	    chance /= 2;
	
	if (victim->position > POS_SLEEPING)
            for (paf = victim->affected; paf; paf = paf->next)
	        if (paf->type == gsn_wariness)
	        {
	            if (number_percent() <= paf->modifier)
	            {
		        act("You sense danger, and narrowly evade an incoming dart!", victim, NULL, NULL, TO_CHAR);
		        chance = 0;
	            }
	            break;
	        }

        WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_dart].beats));
	act("You throw a dart at $N.", ch, NULL, victim, TO_CHAR);
	if (chance < number_percent())
	{
	    act("Your dart misses.", ch, NULL, victim, TO_CHAR);
	    if (IS_AWAKE(victim))
	    {
	        act("$n's dart misses you by a thread.", ch, NULL, victim, TO_VICT);
	        sprintf(buf, "Help! %s is attacking me!", PERS(ch, victim));
	        if (!IS_NPC(victim))
	            do_autoyell(victim, buf);
            check_killer(ch, victim);
	        one_hit(victim, ch, TYPE_UNDEFINED, HIT_PRIMARY, false);
	    }
            check_improve(ch,victim,gsn_dart,FALSE,2);
	    extract_obj(dart);
	    return;
	}
	else
	{
	    act("Your dart plunges into $N.", ch, NULL, victim, TO_CHAR);
	    act("A sharp pain erupts in your side!", ch, NULL, victim, TO_VICT);
	    check_improve(ch, victim, gsn_dart, TRUE, 2);

            damage( ch, victim, number_range(10,20), gsn_dart, DAM_PIERCE, TRUE );
	    if (victim && !IS_NAFFECTED(victim,AFF_GHOST))
	    {
		for (type=0;poison_table[type].spell_fun;type++)
		    if (obj_is_affected(dart,*(poison_table[type].sn)))
		        break;
	        if (poison_table[type].spell_fun)
		    if ((paf = affect_find(dart->affected,*(poison_table[type].sn))) != NULL)
		        poison_table[type].spell_fun(ch,victim,paf->level,gsn_dart);
		    else
		        poison_table[type].spell_fun(ch,victim,ch->level,gsn_dart);
	    }
	    extract_obj(dart);
	    sprintf(buf, "Help! %s is attacking me!", PERS(ch, victim));
	    if (IS_AWAKE(victim) && !IS_NPC(victim))
	        do_autoyell(victim, buf);
	}
    }
    else
    {

	if (!str_prefix(argument, "north")) direction = 0;
	else if (!str_prefix(argument, "east")) direction = 1;
	else if (!str_prefix(argument, "south")) direction = 2;
	else if (!str_prefix(argument, "west")) direction = 3;
	else if (!str_prefix(argument, "up")) direction = 4;
	else if (!str_prefix(argument, "down")) direction = 5;
        else
	{
	  send_to_char("That's not a valid direction.\n\r", ch);
	  return;
	}

	if ((room = ch->in_room) == NULL)
	  return;

	if ((ch->in_room->exit[direction] == NULL) ||
        ((troom = ch->in_room->exit[direction]->u1.to_room) == room) || 
	((troom = ch->in_room->exit[direction]->u1.to_room) == NULL) ||
	(troom->exit[OPPOSITE(direction)]->u1.to_room != ch->in_room))
	{
	  send_to_char("You don't think you can hit them from here.\n\r", ch);
	  return;
	}

	if (ch->in_room->exit[direction]->exit_info & EX_CLOSED
	||ch->in_room->exit[direction]->exit_info & EX_WALLED
	||ch->in_room->exit[direction]->exit_info & EX_ICEWALL
	||ch->in_room->exit[direction]->exit_info & EX_WALLOFFIRE
	||ch->in_room->exit[direction]->exit_info & EX_FAKE)
	{
	  send_to_char("You can't throw a dart there.\n\r", ch);
	  return;
	}

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_dart].beats));

	if ((victim = get_char_room(ch, troom, arg)) == NULL)
	{
        send_to_char("You don't see them there.\n\r", ch);
        return;
	}

    sprintf(buf, "A dart flies in from %s.", 
	OPPOSITE(direction) == 0 ? "the north" :
	(OPPOSITE(direction) == 1 ? "the east" :
	(OPPOSITE(direction) == 2 ? "the south" :
	(OPPOSITE(direction) == 3 ? "the west" :
	(OPPOSITE(direction) == 4 ? "above" :
	"below")))));
	act(buf, ch, NULL, NULL, TO_ROOM);


	if (is_safe(ch, victim))
	    return;

	if (is_affected(victim, gsn_protnormalmissiles) && (number_percent() < victim->level))
	{
	    send_to_char("A dart strikes you, but deflects harmlessly away.\n\r" , victim);
	    act("Your dart strikes $N, but deflects harmlessly away.", ch, NULL, victim, TO_CHAR);
	    extract_obj(dart);
	    return;
	}

	chance = (chance * 4 / 5);
	chance += ch->level - victim->level;

	if (IS_AFFECTED(victim, AFF_HASTE))
	    chance -= 20;	

	if (can_see(victim, ch))
	    chance /= 2;

	if (IS_NPC(victim) && IS_SET(victim->act, ACT_SENTINEL))
	    chance = 0;

	if (victim->position > POS_SLEEPING)
            for (paf = victim->affected; paf; paf = paf->next)
	        if (paf->type == gsn_wariness)
	        {
	            if (number_percent() <= paf->modifier)
	            {
		        act("You sense danger, and narrowly evade an incoming dart!", victim, NULL, NULL, TO_CHAR);
		        chance = 0;
	            }
	            break;
	        }

	if (chance < number_percent())
	{
	    act("Your dart misses.", ch, NULL, victim, TO_CHAR);
	    if (IS_AWAKE(victim))
	    {
	        act("$n's dart misses you by a thread.", ch, NULL, victim, TO_VICT);
	        sprintf(buf, "Help! %s is attacking me!", PERS(ch, victim));
	        if (!IS_NPC(victim))
	            do_autoyell(victim, buf);
                damage( ch, victim,0, gsn_dart, DAM_PIERCE, TRUE );
	    }
            check_improve(ch,victim,gsn_dart,FALSE,2);
	    extract_obj(dart);
	}
	else
	{
	    act("Your dart plunges into $N.", ch, NULL, victim, TO_CHAR);
	    act("A sharp pain erupts in your side!", ch, NULL, victim, TO_VICT);
	    check_improve(ch, victim, gsn_dart, TRUE, 2);
        damage( ch, victim, number_range(10,20), gsn_dart, DAM_PIERCE, TRUE );
	    if (victim && !IS_NAFFECTED(victim,AFF_GHOST))
	    {
            for (type=0;poison_table[type].spell_fun;type++)
            {
		        if (obj_is_affected(dart,*(poison_table[type].sn)))
		            break;
            }
	        if (poison_table[type].spell_fun)
            {
    		    if ((paf = affect_find(dart->affected,*(poison_table[type].sn))) != NULL)
	    	        poison_table[type].spell_fun(ch,victim,paf->level,gsn_dart);
		        else
		            poison_table[type].spell_fun(ch,victim,ch->level,gsn_dart);
            }
	    }
	    extract_obj(dart);
	    if (IS_NPC(victim) && IS_AWAKE(victim) 
	      && !IS_SET(victim->act,ACT_SENTINEL) 
	      && !IS_SET(victim->act,ACT_NOTRACK))
	        move_char(victim, OPPOSITE(direction), FALSE);
	}
    }
}

/* for poisoning weapons and food/drink */
void do_envenom(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *weapon = NULL, *poison = NULL;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int percent,skill,type;
    char arg[MAX_STRING_LENGTH];

    if ((skill = get_skill(ch, gsn_envenom)) == 0)
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }
    if (ch->fighting)
    {
	send_to_char("You can't do that while fighting.\n\r",ch);
	return;
    }

    /* find out what */
    argument = one_argument(argument, arg);
    if (arg[0] == '\0' || argument[0] == '\0')
    {
	send_to_char("Envenom what item with what poison?\n\r",ch);
	return;
    }

    weapon = get_obj_list(ch,arg,ch->carrying);
    poison = get_obj_list(ch,argument,ch->carrying);

    if (!weapon)
    {
	send_to_char("You don't have that weapon.\n\r",ch);
	return;
    }
    
    if (!poison)
    {
	send_to_char("You don't have that poison.\n\r",ch);
	return;
    }

    if (weapon->item_type == ITEM_WEAPON)
    {
        if (IS_WEAPON_STAT(weapon,WEAPON_FLAMING)
        ||  IS_WEAPON_STAT(weapon,WEAPON_FROST)
        ||  IS_WEAPON_STAT(weapon,WEAPON_VAMPIRIC)
        ||  IS_WEAPON_STAT(weapon,WEAPON_VORPAL)
        ||  IS_WEAPON_STAT(weapon,WEAPON_SHOCKING)
	||  IS_OBJ_STAT(weapon,ITEM_BURN_PROOF))
        {
            act("You can't seem to envenom $p.",ch,weapon,NULL,TO_CHAR);
            return;
        }

	if (weapon->value[3] < 0 
	||  weapon->value[3] == DAM_BASH)
	{
	    send_to_char("You can't envenom blunt weapons.\n\r",ch);
	    return;
	}

        if (IS_WEAPON_STAT(weapon,WEAPON_POISON))
        {
            act("$p is already envenomed.",ch,weapon,NULL,TO_CHAR);
            return;
        }
    }
    else
    {
	send_to_char("That's not a weapon.\n\r",ch);
	return;
    }
    for (type = 0;poison_table[type].spell_fun;type++)
	if (obj_is_affected(weapon,*(poison_table[type].sn)))
	    break;
    if (poison_table[type].spell_fun)
    {
	act("$p is already envenomed.",ch,weapon,NULL,TO_CHAR);
	return;
    }
    if (poison->pIndexData->vnum == OBJ_VNUM_POISON_VIAL)
	for (type=0;poison_table[type].spell_fun != NULL;type++)
	    if (obj_is_affected(poison,*(poison_table[type].sn)))
		break;

    if (poison_table[type].spell_fun == NULL)
    {
	send_to_char("That vial contains no poison.\n\r",ch);
	return;
    }

    if (*(poison_table[type].sn) == gsn_sleeppoison)
    {
	send_to_char("You can't envenom a weapon with that poison.\n\r",ch);
	return;
    }

    percent = number_percent();
    if (percent < skill)
    {
	af.where     = TO_WEAPON;
        af.type      = *(poison_table[type].sn);
        af.level     = ch->level * percent / 100;
        af.duration  = ch->level/2 * percent / 100 + 1;
        af.location  = 0;
        af.modifier  = 0;
        af.bitvector = 0;
        affect_to_obj(weapon,&af);
        act("$n coats $p with deadly venom.",ch,weapon,NULL,TO_ROOM);
	act("You coat $p with venom.",ch,weapon,NULL,TO_CHAR);
	check_improve(ch,NULL,gsn_envenom,TRUE,1);
    }
    else
    {
	act("You fail to envenom $p.",ch,weapon,NULL,TO_CHAR);
	check_improve(ch,NULL,gsn_envenom,FALSE,1);
    }

    extract_obj(poison);
    WAIT_STATE(ch,skill_table[gsn_envenom].beats);
    return;
}

void do_gag( CHAR_DATA *ch, char *argument )
{
CHAR_DATA *victim;
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
int chance;

    if ((chance = get_skill(ch,gsn_gag)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("Gag whom?\n\r", ch);
	return;
    }

    if ((victim = get_char_room(ch, argument)) == NULL)
    {
	send_to_char("You don't see them here.\n\r", ch);
	return;
    }
    
    if (is_safe(ch, victim))
    {
        send_to_char("The gods protect your victim.\n\r", ch);
        return;
    }

    if (is_affected(victim, gsn_gag))
    {
	send_to_char("They are already gagged.\n\r", ch);
	return;
    }

    if (IS_AWAKE(victim))
    {
	send_to_char("You can't gag them while they're awake!\n\r", ch);
	return;
    }

    if (IS_AFFECTED(victim,AFF_SLEEP))
        chance = get_skill(ch,gsn_gag);
    else
        chance = (get_skill(ch,gsn_gag)/2);

    if (number_percent() > chance)
    {  
	  /* Failure */
	  send_to_char("You fail to properly secure the gag.\n\r", ch);
	  check_improve(ch, victim,gsn_gag, FALSE, 1);
	  act("$n attempts to gag $N, but fails.", ch, NULL, victim, TO_NOTVICT);
    }
	else
	{
	  act("You quickly tie a gag around $N's mouth.",ch,NULL,victim,TO_CHAR);
          act("$n quickly ties a gag around $N's mouth.",ch,NULL,victim,TO_NOTVICT);
	  check_improve(ch,victim, gsn_gag, TRUE, 1);
	
          af.where      = TO_AFFECTS;
          af.type       = gsn_gag;
          af.level      = ch->level;
	  if (!is_affected(victim,gsn_bind))
            af.duration   = ch->level/15;
          else
            af.duration   = (ch->level/15) + 2;
          af.location   = 0;
          af.modifier   = 0;
          af.bitvector  = 0;
          affect_to_char(victim, &af);
	}

        WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_gag].beats));
	if (!IS_AFFECTED(victim,AFF_SLEEP))
	  do_wake(victim,"");
	if (IS_AWAKE(victim))
	{
	  if (is_affected(victim,gsn_gag))
	    act("You suddenly awaken as $N quickly gags you!",victim,NULL,ch,TO_CHAR);
	  else
	    act("You suddenly awaken as $N tries to gag you!",victim,NULL,ch,TO_CHAR);
	}
}

void do_garrote(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    char buf[MAX_STRING_LENGTH];
    int chance;

    if ((chance = get_skill(ch, gsn_garrote)) == 0)
    {
      send_to_char("Huh?\n\r", ch);
      return;
    }

    if (!ch->in_room)
	return;

    if (is_affected(ch,gsn_garrote) && !IS_NAFFECTED(ch, AFF_GARROTE))
    {
	send_to_char("You don't feel ready to garrote anyone again.\n\r", ch);
	return;
    }

    if ((victim = get_char_room(ch, argument)) == NULL)
    {
	send_to_char("You don't see them here.\n\r", ch);
	return;
    }

    if (IS_NPC(victim) || victim->fighting)
    {
	send_to_char("You don't think you can get them with the wire.\n\r",ch);
	return;
    }
    
    if (can_see(victim, ch))
    {
	send_to_char("But they can see you coming!\n\r", ch);
	return;
    }

    if (is_affected(victim, gsn_thirdeye))
    {
	send_to_char("Their third eye would catch you before you were on them.\n\r", ch);
	return;
    }

    if (is_safe(ch, victim))
	return;

    if (IS_NAFFECTED(victim, AFF_GARROTE_VICT))
    {
	act("$N is already being garroted.", ch, NULL, victim, TO_CHAR);
	return;
    }

    if (is_affected(victim, gsn_diamondskin) && IS_SET(victim->imm_flags, IMM_WEAPON))
    {
        act("Your wire snaps on $N's diamond skin." , ch, NULL, victim, TO_CHAR);
        act("$n attempts to garrote you, but their wire snaps on your diamond skin.",ch, NULL, victim,TO_VICT);
        act("$n tries to garrote $N, but the wire snaps!",ch,NULL,victim,TO_NOTVICT);
        WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_garrote].beats));
        return;
    }

    if (IS_NAFFECTED(victim, AFF_WARINESS))
	chance /= 2; 

    if (is_affected(victim, gsn_sense_danger))
    {
	send_to_char("You sense incoming danger.\n\r", ch);
	chance /= 2;
    }

    if (BIT_GET(victim->pcdata->traits, TRAIT_STREETWISE))
	chance -= 10;

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_garrote].beats));
    do_visible(ch, "");

    af.where	 = TO_NAFFECTS;
    af.type	 = gsn_garrote;
    af.level     = ch->level;
    af.duration  = 10;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);
// brazen: Ticket #192: garrote now sets adrenaline as it should
    ch->pcdata->adrenaline = UMAX(ch->pcdata->adrenaline,20);
    victim->pcdata->adrenaline = UMAX(victim->pcdata->adrenaline,20);

    if ((number_percent() > chance)
     || (IS_AWAKE(victim) && (number_percent() < (get_curr_stat(victim, STAT_DEX) + get_curr_stat(victim, STAT_STR) + 10))))
    {
        if (!IS_AWAKE(victim))
	    switch_position(victim, POS_STANDING);
	act("$n tries to catch $N's neck with a garrote, but $N escapes!", ch, NULL, victim, TO_NOTVICT);
	act("$n tries to catch your neck with a garrote, but you sense it coming and escape!", ch, NULL, victim, TO_VICT);
	act("You try to catch $N's neck with a garrote, but $E pulls away before you can seize $M!", ch, NULL, victim, TO_CHAR);
        sprintf(buf, "Help! %s is trying to strangle me!", PERS(ch, victim));
        if (!IS_NPC(victim))
            do_autoyell(victim, buf);
        check_improve(ch,victim,gsn_garrote,FALSE,1);
        check_killer(ch, victim);
        one_hit(victim, ch, TYPE_UNDEFINED, HIT_PRIMARY, false);
        return;
    }

    switch_position(victim, POS_STANDING);

    act("Sneaking up on $N, you seize $S throat with a garrote!", ch, NULL, victim, TO_CHAR);
    act("Sneaking up on $N, $n seizes $N's throat with a garrote!", ch, NULL, victim, TO_NOTVICT);
    act("A sharp pain explodes around your neck, and you struggle to breathe!", ch, NULL, victim, TO_VICT);

    check_killer(ch, victim);

    af.point	 = (void *) victim;
    af.duration  = 6;
    af.bitvector = AFF_GARROTE_CH;
    affect_to_char(ch, &af);

    af.point	 = (void *) ch;
    af.duration  = 1;
    af.location  = APPLY_MANA;
    af.modifier  = -1;
    af.bitvector = AFF_GARROTE_VICT;
    affect_to_char(victim, &af);

    return;
}

void do_hide( CHAR_DATA *ch, char *argument )
{
    int cost = 0, chance;

    if (!IS_NPC(ch))
	cost = number_bits(0)==0 ? 1 : 2;

    if (ch->mana > cost)
	    expend_mana(ch, cost);
    else
    {
	send_to_char("You're too tired to hide.\n\r", ch);
	return;
    }

    if ( IS_AFFECTED(ch, AFF_HIDE) )
	REMOVE_BIT(ch->affected_by, AFF_HIDE);

    if ( IS_AFFECTED(ch, AFF_FAERIE_FIRE))
    {
	send_to_char("You can't hide while glowing.\n\r", ch);
	return;
    }

    if (ch->in_room && room_is_affected(ch->in_room, gsn_etherealblaze))
    {
	send_to_char("The flames around you prevent you from hiding.\n\r", ch);
	return;
    }

    if (IS_OAFFECTED(ch, AFF_BLEEDING))
    {
	send_to_char("You are trailing too much blood to hide.\n\r", ch);
	return;
    }

    if (is_affected(ch, gsn_ringoffire))
    {
	send_to_char("You cannot hide while surrounded by a ring of flames.\n\r", ch);
	return;
    }

    if (ch->in_room->sector_type != SECT_INSIDE && ch->in_room->sector_type != SECT_CITY && ch->in_room->sector_type != SECT_UNDERGROUND)
    {
	send_to_char("You're too far from your element to skulk around hiding.\n\r", ch);
	return;
    }

    chance = get_skill(ch, gsn_hide);

    if (IS_AFFECTED(ch, AFF_BLIND) && (ch->race != global_int_race_shuddeni))
    {
	send_to_char("You stumble about blindly, attempting to hide.\n\r", ch);
	chance /= 5;
    }
    else
	send_to_char( "You attempt to hide.\n\r", ch );

    if (chance > number_percent())
    {
	SET_BIT(ch->affected_by, AFF_HIDE);
	check_improve(ch,NULL,gsn_hide,TRUE,3);
    }
    else
	check_improve(ch,NULL,gsn_hide,FALSE,3);

    return;
}


/*
 * check to see who killed someone -- watcher
 */
void do_investigate(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj;
    int chance;
    char oname[MAX_STRING_LENGTH];
    char *stype;
    int type=0,cost=0;
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;

    if ((chance = get_skill(ch,gsn_investigate)) == 0)
    {
	send_to_char("Huh?\n\r", ch);
        return;
    }

    stype = one_argument(argument,oname);
    
    if (oname[0] == '\0')
    {
	send_to_char("What did you want to investigate?\n\r",ch);
	return;
    }

    if (stype[0] == '\0')
    {
	cost = 500;
	type = 5;
    }
    else if (!str_cmp(stype,"some"))
    {
	cost = 200;
	type = 1;
    }
    else if (!str_cmp(stype,"detailed"))
    {
	cost = 500;
	type = 2;
    }
    else if (!str_cmp(stype,"all"))
    {
	cost = 1500;
	type = 3;
    }
    else 
    {
	send_to_char("How much information did you want on what?\n\r",ch);
	return;
    }
    
    if (type == 5)
	obj = get_obj_here(ch,oname);
    else
	obj = get_obj_carry(ch, oname, ch);

    if (obj == NULL)
    {
	send_to_char("You don't see that.\n\r",ch);
	return;
    }

    if (type == 5 
      && (obj->item_type != ITEM_CORPSE_PC 
	&& obj->item_type != ITEM_CORPSE_NPC))
    {
	send_to_char("How much information did you want on what?\n\r", ch);
	return;
    }

    if (type != 5 
      && (obj->item_type == ITEM_CORPSE_PC 
	|| obj->item_type == ITEM_CORPSE_NPC))
	type = 5;

    if (number_percent() > chance)
    {
	send_to_char("You find yourself unable to glean any information from the item.\n\r", ch);
	check_improve(ch,NULL,gsn_investigate,FALSE,1);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_investigate].beats));
	return;
    }

    if (!dec_player_bank(ch,value_to_coins(cost,FALSE)))
	if (!dec_player_coins(ch,value_to_coins(cost,FALSE)))
    	{
	    send_to_char("You don't have enough money to pay for an investigation.\n\r",ch);
	    return;
        }
    if (type == 2 || type == 5)
	act("You contact the Office of Records for detailed information on $p.",ch,obj,NULL,TO_CHAR);
    else if (type == 1)
	act("You contact the Office of Records for some information on $p.",ch,obj,NULL,TO_CHAR);
    else
    	act("You contact the Office of Records for all information on $p.",ch,obj,NULL,TO_CHAR);
    investigate_note(ch,obj,type);

    check_improve(ch,NULL,gsn_investigate,TRUE,1);
    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_investigate].beats));
}

void do_listen(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    EXIT_DATA *pexit;
    CHAR_DATA *victim;
    int i, dir = -1, chance;

    if ((chance = get_skill(ch, gsn_listen)) == 0)
    {
        send_to_char( "Huh?\n\r", ch );
        return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("What do you wish to listen to?\n\r", ch);
	return;
    }

    if ((victim = get_char_room(ch, argument)) == NULL)
	for (i = 0; i < 6; i++)
	    if (!str_prefix(argument, dir_name[i]))
		dir = i;

    if (!victim && (dir == -1))
    {
	send_to_char("Invalid direction or person.\n\r", ch);
	return;
    }	

    af.point = NULL;

    if (!victim)
    {
        if ((pexit = ch->in_room->exit[dir]) == NULL)
        {
	    send_to_char( "There's nothing there to listen to.\n\r", ch );
	    return;
        }
    }
    else
       af.point = (void *) victim;

    if (chance < number_percent())
    {
	send_to_char("You fail to find a good position to initiate listening.\n\r", ch);
	WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE * 2));
	return;
    }

    af.where     = TO_AFFECTS;
    af.type      = gsn_listen;
    af.level     = ch->level;
    af.duration  = ch->level;
    af.location  = APPLY_NONE;
    af.modifier  = dir;
    af.bitvector = 0;
    affect_to_char( ch, &af );

    send_to_char( "You calm yourself and begin to quietly listen.\n\r", ch );
    return;
}

void do_muffle(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    char arg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int chance;

    if ((chance = get_skill(ch, gsn_muffle)) == 0)
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
        send_to_char("Muffle whom and do what?\n\r", ch);
        return;
    }

    argument = one_argument(argument, arg);

    if (argument[0] == '\0')
    {
        send_to_char("Muffle whom and do what?\n\r", ch);
        return;
    }

    if ((victim = get_char_room(ch, arg)) == NULL)
    {
        send_to_char("You don't see them here.\n\r", ch);
        return;
    }

    if (ch->fighting)
    {
	send_to_char("You cannot muffle while fighting.\n\r", ch);
	return;
    }

    if (!IS_AWAKE(victim))
	chance *= 2;

    if (is_safe(ch, victim))
	return;

//    WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));

    if ((chance - get_curr_stat(victim, STAT_DEX)) > number_percent())
        check_improve(ch,victim,gsn_muffle,TRUE,2);
    else
    {
        check_improve(ch,victim,gsn_muffle,FALSE,2);
	act("$N avoids your attempt to muffle $M!", ch, NULL, victim, TO_CHAR);
	act("$N avoids $n's attempt to muffle $M!", ch, NULL, victim, TO_NOTVICT);
	act("You avoid $n's attempt to muffle you!", ch, NULL, victim, TO_VICT);

	if (!IS_NPC(victim))
	    if (victim->fighting != ch)
	    {
		sprintf(buf, "Help!  %s is trying to muffle me!", PERS(ch, victim));
		do_autoyell(victim, buf);
	    }
        WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_muffle].beats));

	check_killer(ch, victim);
	multi_hit(victim, ch, TYPE_UNDEFINED);

	return;
    }

    SET_BIT(victim->naffected_by, AFF_MUFFLE);

    interpret(ch, argument);

    REMOVE_BIT(victim->naffected_by, AFF_MUFFLE);

    return;
}


void do_nerve(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *wield;
    int chance, result;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    bool disarmed = FALSE;

    if ((chance = get_skill(ch, gsn_nerve)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    chance /= 2;

    if ((victim = ch->fighting) == NULL)
	if (argument[0] == '\0')
	{
            send_to_char("Nerve whom?\n\r", ch);
	    return;
        }

    if (!victim)
        if ((victim = get_char_room(ch, argument)) == NULL)
        {
            send_to_char("Nerve whom?\n\r", ch);
	    return;
        }

    if (is_safe(ch,victim))
        return;

    if (victim == ch)
    {
        send_to_char("You cannot nerve yourself.\n\r",ch);
        return;
    }

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_nerve].beats));

    if (is_affected(victim, gsn_diamondskin) && IS_SET(victim->imm_flags, IMM_WEAPON))
    {
        act("You try to press into $N's nerve points, but are blocked by diamond-hard skin!" , ch, NULL, victim, TO_CHAR)
;
        act("$n attempts to press into your nerves, but your diamond skin blocks the attempt!",ch,NULL,victim,TO_VICT); 
        return;
    }

    if (is_loc_not_affected(victim, gsn_anesthetize, APPLY_NONE))
    {
	act("You press into $N's nerve points, but find $E is immune to pain!", ch, NULL, victim, TO_CHAR);
	act("$n presses into your nerve points, but you are immune to pain!", ch, NULL, victim, TO_VICT);
	return;
    }
 
    if (IS_AFFECTED(victim, AFF_HASTE) || (IS_NPC(victim) && IS_SET(victim->off_flags, OFF_FAST)))
	chance = chance * 3 / 4;

    chance += (ch->level - victim->level);

    chance = URANGE(5, chance, 95);

    if (!IS_NPC(victim))
	if (ch->fighting == NULL || victim->fighting != ch)
	{
	    sprintf(buf, "Help! %s is attacking me!", PERS(ch, victim));
	    do_autoyell(victim, buf);
	}

    check_killer(ch, victim);
    if (victim->fighting == NULL)
	victim->fighting = ch;
    if (ch->fighting == NULL)
	ch->fighting = victim;

    if ((result = number_percent()) < chance)
    {
	act("$n deftly presses into $N's nerve points!", ch, NULL, victim, TO_NOTVICT);
	act("$n deftly presses into your nerve points!", ch, NULL, victim, TO_VICT);
	act("You deftly press into $N's nerve points!", ch, NULL, victim, TO_CHAR);
        check_improve(ch,victim,gsn_nerve,TRUE,1);
    }
    else
    {
	act("You fumble and miss $N's nerve points!", ch, NULL, victim, TO_CHAR);
	act("$n fumbles trying to pressure $N's nerve points!", ch, NULL, victim, TO_NOTVICT);
	act("$n fumbles and misses your nerve points!", ch, NULL, victim, TO_VICT);
        check_improve(ch,victim,gsn_nerve,FALSE,1);
	return;
    }

    af.where        = TO_AFFECTS;
    af.type         = gsn_nerve;
    af.level        = ch->level;
    af.duration     = 6 + number_bits(2);
    af.location     = APPLY_STR;
    af.modifier     = -2;
    af.bitvector    = 0;
    affect_to_char(victim, &af);

    af.location     = APPLY_DEX;
    af.modifier     = -2;
    affect_to_char(victim, &af);

    if ((wield = get_eq_char(victim, WEAR_WIELD)) && (number_bits(2) == 0)
      && !IS_OBJ_STAT(wield, ITEM_NOREMOVE) 
      && !IS_SET(wield->extra_flags[0], ITEM_VIS_DEATH))
    {
	act("$n drops $p, as weakness shoots through $m.", victim, wield, NULL, TO_ROOM);
	act("You drop $p as weakness shoots through you.", victim, wield, NULL, TO_CHAR);
	unequip_char(victim, wield);
	oprog_remove_trigger(wield);
	if (!IS_NPC(victim)
	  && !IS_OBJ_STAT(wield,ITEM_NODROP)
	  && !IS_OBJ_STAT(wield,ITEM_INVENTORY))
	
	{
	    obj_from_char(wield);
	    obj_to_room(wield, victim->in_room);
	}
	disarmed=TRUE;
    }
    
    if (!disarmed && (wield = get_eq_char(victim, WEAR_DUAL_WIELD)) 
      && (number_bits(2) == 0) 
      && !IS_OBJ_STAT(wield, ITEM_NOREMOVE)
      && !IS_SET(wield->extra_flags[0], ITEM_VIS_DEATH))
    {
	act("$n drops $p, as weakness shoots through $m.", victim, wield, NULL, TO_ROOM);
	act("You drop $p as weakness shoots through you.", victim, wield, NULL, TO_CHAR);
	unequip_char(victim, wield);
	oprog_remove_trigger(wield);
	if (!IS_NPC(victim)
	  && !IS_OBJ_STAT(wield,ITEM_NODROP)
	  && !IS_OBJ_STAT(wield,ITEM_INVENTORY))
	{
	    obj_from_char(wield);
	    obj_to_room(wield, victim->in_room);
	}
	disarmed=TRUE;
    }
    if (!disarmed && (wield = get_eq_char(victim, WEAR_HOLD)) 
      && (number_bits(2) == 0) && !IS_OBJ_STAT(wield, ITEM_NOREMOVE))
    {
	act("$n drops $p, as weakness shoots through $m.", victim, wield, NULL, TO_ROOM);
	act("You drop $p as weakness shoots through you.", victim, wield, NULL, TO_CHAR);
	unequip_char(victim, wield);
	oprog_remove_trigger(wield);
	if (!IS_NPC(victim)
	  && !IS_OBJ_STAT(wield,ITEM_NODROP)
	  && !IS_OBJ_STAT(wield,ITEM_INVENTORY))
	{
	    obj_from_char(wield);
	    obj_to_room(wield, victim->in_room);
	}
	disarmed=TRUE;
    }
    return;
}

void do_obscure(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (get_skill(ch, gsn_obscure_evidence) <= 0)
    {
	send_to_char("You do not know how to properly obscure evidence.\n\r", ch);
	return;
    }

    if (is_affected(ch, gsn_obscure_evidence))
    {
	send_to_char("You cease preparing to obscure evidence.\n\r", ch);
	affect_strip(ch, gsn_obscure_evidence);
	return;
    }

    af.where	    = TO_PAFFECTS;
    af.type	    = gsn_obscure_evidence;
    af.level	    = ch->level;
    af.duration	    = -1;
    af.location	    = 0;
    af.modifier	    = 0;
    af.bitvector    = AFF_OBSCURE_EVIDENCE;
    affect_to_char(ch, &af);

    send_to_char("You prepare to obscure the evidence of death.\n\r", ch);
    return;
}

void do_relock( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Relock what?\n\r", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_relock].beats );
    if (IS_AFFECTED(ch, AFF_CHARM) || is_affected(ch, gsn_commandword))
    return;

    if ( !IS_NPC(ch) && number_percent( ) > get_skill(ch,gsn_relock))
    {
        send_to_char( "You failed.\n\r", ch);
        check_improve(ch,NULL,gsn_relock,FALSE,2);
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

            if (obj->value[2] < 1)
            {
                send_to_char("It can't be locked.\n\r",ch);
                return;
            }

            if (IS_SET(obj->value[1],EX_PICKPROOF))
            {
                send_to_char("You failed.\n\r",ch);
                return;
            }

            SET_BIT(obj->value[1],EX_LOCKED);
            act("You deftly click the tumblers into place, locking $p.",ch,obj,NULL,TO_CHAR);
            act("$n deftly manipulates the tumbers into place, locking $p.",ch,obj,NULL,TO_ROOM);
            check_improve(ch,NULL,gsn_relock,TRUE,2);
            return;
        }





        /* 'pick object' */
        if ( obj->item_type != ITEM_CONTAINER )
            { send_to_char( "That's not a container.\n\r", ch ); return; }
        if ( !IS_SET(obj->value[1], CONT_CLOSED) )
            { send_to_char( "It's not closed.\n\r",        ch ); return; }
        if ( obj->value[2] < 1 )
            { send_to_char( "It can't be locked.\n\r",   ch ); return; }
        if ( IS_SET(obj->value[1], CONT_LOCKED) )
            { send_to_char( "It's already locked.\n\r",  ch ); return; }
        if ( IS_SET(obj->value[1], CONT_PICKPROOF) )
            { send_to_char( "You failed.\n\r",             ch ); return; }

        SET_BIT(obj->value[1], CONT_LOCKED);
        act("You deftly click the tumblers into place, locking $p.",ch,obj,NULL,TO_CHAR);
        act("$n deftly manipulates the tumbers into place, locking $p.",ch,obj,NULL,TO_ROOM);
        check_improve(ch,NULL,gsn_relock,TRUE,2);
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
        if ( ((pexit->key < 0) || IS_SET(pexit->exit_info, EX_NOLOCK)) && !IS_IMMORTAL(ch) )
            { send_to_char( "It can't be locked.\n\r",     ch ); return; }
        if ( IS_SET(pexit->exit_info, EX_LOCKED) )
            { send_to_char( "It's already locked.\n\r",  ch ); return; }
        if ( IS_SET(pexit->exit_info, EX_PICKPROOF) && !IS_IMMORTAL(ch))
            { send_to_char( "You failed.\n\r",             ch ); return; }

        SET_BIT(pexit->exit_info, EX_LOCKED);
        act( "You deftly relock the $d.", ch, NULL, pexit->keyword, TO_CHAR );
        act( "$n deftly relocks the $d.", ch, NULL, pexit->keyword, TO_ROOM );
        check_improve(ch,NULL,gsn_relock,TRUE,2);

        /* pick the other side */
        if ( ( to_room   = pexit->u1.to_room            ) != NULL
        &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
        &&   pexit_rev->u1.to_room == ch->in_room )
        {
            SET_BIT( pexit_rev->exit_info, EX_LOCKED );
        }
    }

    return;
}

void do_shadowmastery( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int chance;

    if ((chance = get_skill(ch, gsn_shadowmastery)) == 0)
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    if ( IS_AFFECTED(ch, AFF_FAERIE_FIRE))
    {
	send_to_char("You can't hide while glowing.\n\r", ch);
	return;
    }

    if (ch->in_room && room_is_affected(ch->in_room, gsn_etherealblaze))
    {
	send_to_char("The flames around you prevent you from melding into the shadows.\n\r", ch);
	return;
    }

    if (IS_OAFFECTED(ch, AFF_BLEEDING))
    {
	send_to_char("You are trailing too much blood to conceal yourself.\n\r", ch);
	return;
    }

    if (is_affected(ch, gsn_ringoffire))
    {
	send_to_char("You cannot meld into the shadows while surrounded by a ring of flames.\n\r", ch);
	return;
    }

    if ((ch->in_room->sector_type != SECT_INSIDE) && (ch->in_room->sector_type != SECT_CITY) && (ch->in_room->sector_type != SECT_UNDERGROUND))
    {
	send_to_char("There are not sufficient shadows here to blend into.\n\r", ch);
	return;
    }  

    if (ch->mana < skill_table[gsn_shadowmastery].min_mana)
    {
	send_to_char("You cannot concentrate enough to conceal yourself.\n\r", ch);
	return;
    }
//brazen: blind chance pasted verbatim from hide, with a lower percent chance of occurring.
//Ticket #136
    if (IS_AFFECTED(ch, AFF_BLIND) && (ch->race != global_int_race_shuddeni))
    {
	send_to_char("You stumble about blindly, attempting to hide.\n\r", ch);
	chance /= 5;
    }
    else
    	send_to_char("You move deep into the shadows, concealing yourself completely.\n\r", ch);
    
    expend_mana(ch, skill_table[gsn_shadowmastery].min_mana);
    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_shadowmastery].beats));

    if (number_percent() < chance)
    {
	af.where	= TO_OAFFECTS;
	af.type		= gsn_shadowmastery;
        af.level	= ch->level;
	af.duration 	= ch->level / 2;
	af.location	= APPLY_HIDE;
	af.modifier	= 0;
	af.bitvector    = 0;
        affect_to_char(ch, &af);
	SET_BIT(ch->oaffected_by, AFF_SHADOWMASTERY);
	check_improve(ch, NULL,gsn_shadowmastery, TRUE, 1);
    }
    else
	check_improve(ch,NULL, gsn_shadowmastery, FALSE, 1);

    return;
}

void do_steal( CHAR_DATA *ch, char *argument )
{
    char buf  [MAX_STRING_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *vch;
    OBJ_DATA *obj;
    int percent, wskill, chance;
    bool pilfer = FALSE;
    long coins[MAX_COIN];

    if ((chance = get_skill(ch, gsn_steal)) == 0)
    {
	send_to_char("You lack skill in the art of pilfering.\n\r", ch);
	return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
	send_to_char( "Steal what from whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( victim == ch )
    {
	send_to_char( "That's pointless.\n\r", ch );
	return;
    }

    if (is_safe(ch,victim))
	return;

    if (victim->position == POS_FIGHTING)
    {
	send_to_char("You'd better not -- you might get hit.\n\r",ch);
	return;
    }

    WAIT_STATE( ch, skill_table[gsn_steal].beats );
    percent  = number_percent();

    if (!IS_AWAKE(victim))
    	percent -= 10;
    else if (!can_see(victim,ch))
    	percent += 25;
    else 
	percent += 50;

    if (!IS_NPC(victim) && BIT_GET(victim->pcdata->traits, TRAIT_STREETWISE))
	chance -= 10;

    // Cannot steal from shoppies.
    if (IS_NPC(victim) && victim->pIndexData->pShop)
    {
	act("$N's hand darts down reflexively to prevent the theft.", ch, NULL, victim, TO_CHAR);
	chance = 0;
    }

    if ((percent > chance) && !IS_IMMORTAL(ch))
    {
	/*
	 * Failure.
	 */
	uncamo_char(ch);
        unhide_char(ch);
	send_to_char( "Oops.\n\r", ch );
	affect_strip(ch,gsn_sneak);
	REMOVE_BIT(ch->affected_by,AFF_SNEAK);

	act( "$n tried to steal from you.", ch, NULL, victim, TO_VICT    );
	act( "$n tried to steal from $N.",  ch, NULL, victim, TO_NOTVICT );
	switch(number_range(0,3))
	{
	    case 0 :
	        sprintf( buf, "%s is a lousy thief!", PERS(ch, victim));
	        break;
            case 1 :
	        sprintf( buf, "%s couldn't rob %s way out of a paper bag!",
		    PERS(ch,victim),(ch->sex == 2) ? "her" : "his");
	        break;
	    case 2 :
	        sprintf( buf,"%s tried to rob me!", PERS(ch, victim) );
	        break;
	    case 3 :
	        sprintf(buf,"Keep your hands out of there, %s!",PERS(ch, victim));
	        break;
        }

        if (!IS_AWAKE(victim))
            do_wake(victim,"");

	if (IS_AWAKE(victim) && !IS_NPC(victim) && (!IS_NPC(ch) || IS_SET(ch->act, ACT_GUILDGUARD)))
	    do_autoyell( victim, buf );

	if ( IS_NPC(victim) )
	{
	    check_improve(ch,victim,gsn_steal,FALSE,2);
	    multi_hit( victim, ch, TYPE_UNDEFINED );
	}
	else
	{
	    sprintf(buf,"$N tried to steal from %s.",victim->name);
	    wiznet(buf,ch,NULL,WIZ_FLAGS,0,0);
	}

	check_killer(ch, victim);
	return;
    }
    
    if (IS_AFFECTED(ch, AFF_HIDE) && number_bits(1) == 0)
	unhide_char(ch);
    
    if (!IS_NPC(ch))
	ch->pcdata->adrenaline = IS_NPC(victim) ? UMAX(ch->pcdata->adrenaline, 2) : UMAX(ch->pcdata->adrenaline,20);

    if ( !str_cmp( arg1, "coins" ))
    {
	int i;
	bool cFound = FALSE;

	for (i = 0; i < MAX_COIN; i++)
	{
	    coins[i] = round(victim->coins[i]/3 * number_range(1, ch->level) / 60.0);
	    if (coins[i] > 0)
		cFound = TRUE;
	}

	if (!cFound)
	    send_to_char("You reach their purse undetected, but manage to grab no coins.\n\r", ch);
	else
	{
    	    inc_player_coins(ch, coins);
	    dec_player_coins(victim, coins);

	    sprintf(buf, "Bingo!  You got %s.\n\r", coins_to_str(coins));
	    send_to_char(buf, ch);
	}

	check_improve(ch,victim,gsn_steal,TRUE,2);
	for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
	{
	    if (vch == ch)
		continue;
	    if (can_see(vch, ch)
             && ((wskill = get_skill(vch, gsn_detecttheft)) > 0)
	     && (wskill > number_percent()))
	    {
    		check_improve(vch,ch,gsn_detecttheft,TRUE,2);
		if (vch == victim)
		{
		    sprintf(buf, "%s stole money from you.\n\r", PERS(ch, vch));
		    send_to_char(buf, vch);
		}
		else
		{
		    sprintf(buf, "%s stole money from %s.\n\r", PERS(ch, vch), PERS(victim, vch));
		    send_to_char(buf, vch);
		}
	    }
	    if (check_detectstealth(vch,ch))
		if (vch == victim)
		{
		    sprintf(buf, "%s stole money from you.\n\r", PERS(ch, vch));
		    send_to_char(buf, vch);
		}
		else
		{
		    sprintf(buf, "%s stole money from %s.\n\r", PERS(ch, vch), PERS(victim, vch));
		    send_to_char(buf, vch);
		}
	}
	
	return;
    }

    if ( ( obj = get_obj_carry( victim, arg1, ch ) ) == NULL )
    {
	send_to_char( "You can't find it.\n\r", ch );
	return;
    }

    if (obj == victim->nocked && victim->position < POS_RESTING)
    {
	send_to_char("You're not good enough to steal a nocked arrow.\n\r",ch);
	return;
    }

    if ( !can_drop_obj( ch, obj )
    ||   IS_SET(obj->extra_flags[0], ITEM_INVENTORY))
    {
	send_to_char( "You can't pry it away.\n\r", ch );
	return;
    }

    if (!IS_NPC(ch) && !IS_IMMORTAL(ch)
     && (get_cur_capacity(ch) + item_svalue[obj->size]) > get_max_capacity(ch))
    {
	send_to_char("You lack the room to carry that.\n\r", ch);
	return;
    }

    if ( ch->carry_number + 1 > static_cast<int>(Encumbrance::CarryCountCapacity(*ch)))
    {
	send_to_char( "You have your hands full.\n\r", ch );
	return;
    }

    if ( ch->carry_weight + get_obj_weight( obj ) > static_cast<int>(Encumbrance::CarryWeightCapacity(*ch)))
    {
	send_to_char( "You can't carry that much weight.\n\r", ch );
	return;
    }

    if (obj == victim->on)
    {
	send_to_char("You don't think you can steal that undetected.\n\r",ch);
	return;
    }

    if (obj == ch->nocked)
	ch->nocked = NULL;   
    obj_from_char( obj );
    obj_to_char( obj, ch );
    act("You pocket $p.",ch,obj,NULL,TO_CHAR);
    check_improve(ch,victim,gsn_steal,TRUE,1);
    send_to_char( "Got it!\n\r", ch );

    if (number_percent() < get_skill(ch,gsn_pilfer))
    {
	int i;
	bool cFound = FALSE;

	for (i = 0; i < MAX_COIN; i++)
	{
	    coins[i] = round(victim->coins[i]/5 * number_range(1, ch->level) / 60.0);
	    if (coins[i] > 0)
		cFound = TRUE;
	}

	if (cFound)
	{
    	    inc_player_coins(ch, coins);
	    dec_player_coins(victim, coins);

	    sprintf(buf, "Bingo!  You also pilfered %s.\n\r", coins_to_str(coins));
	    send_to_char(buf, ch);
	    pilfer = TRUE;
	}

	check_improve(ch,victim,gsn_pilfer,TRUE,2);
    }

    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
	if (vch == ch)
	    continue;
	if (can_see(vch, ch) && can_see_obj(vch, obj)
	 && (wskill = get_skill(vch, gsn_detecttheft)) > 0
	 && wskill > number_percent())
	{
    	    check_improve(vch,ch,gsn_detecttheft,TRUE,2);
	    if (pilfer)
		if (vch == victim)
	    	{
		    sprintf(buf, "%s stole %s and money from you.\n\r", PERS(ch, vch), obj->short_descr);
		    send_to_char(buf, vch);
		    log_string(buf);
	    	}
	    	else
	    	{
		    sprintf(buf, "%s stole %s and money from %s\n\r", PERS(ch, vch), obj->short_descr, PERS(victim, vch));
		    send_to_char(buf, vch);
		    log_string(buf);
	    	}
	    else
		if (vch == victim)
	    	{
		    sprintf(buf, "%s stole %s from you.\n\r", PERS(ch, vch), obj->short_descr);
		    send_to_char(buf, vch);
		    log_string(buf);
	    	}
	    	else
	    	{
		    sprintf(buf, "%s stole %s from %s.\n\r", PERS(ch, vch), obj->short_descr, PERS(victim, vch));
		    send_to_char(buf, vch);
		    log_string(buf);
	    	}
	}
	if (check_detectstealth(vch,ch))
	    if (pilfer)
		if (vch == victim)
	    	{
	    	    sprintf(buf, "%s stole %s and money from you.\n\r", 
		      PERS(ch, vch), obj->short_descr);
		    send_to_char(buf, vch);
		    log_string(buf);
	    	}
	    	else
	    	{
	 	    sprintf(buf, "%s stole %s and money from %s.\n\r", 
		      PERS(ch, vch),obj->short_descr, PERS(victim, vch));
		    send_to_char(buf, vch);
		    log_string(buf);
	    	}
	    else
		if (vch == victim)
	    	{
	    	    sprintf(buf, "%s stole %s from you.\n\r", 
		      PERS(ch, vch), obj->short_descr);
		    send_to_char(buf, vch);
		    log_string(buf);
	    	}
	    	else
	    	{
	 	    sprintf(buf, "%s stole %s from %s.\n\r", 
		      PERS(ch, vch),obj->short_descr, PERS(victim, vch));
		    send_to_char(buf, vch);
		    log_string(buf);
	    	}
    }

    check_aura(ch, obj, TRUE);

    return;
}

void do_trap( CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int skill, ttype;

    if ((skill = get_skill(ch,gsn_trap)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (IS_OAFFECTED(ch, AFF_GHOST))
    {
	send_to_char("You cannot set a trap in your current state.\n\r", ch);
	return;
    }
 
    if (ch->mana < skill_table[gsn_trap].min_mana)
    {
	send_to_char("You can't concentrate enough to set up a trap.\n\r", ch);
	return;
    }

    if (is_affected(ch, gsn_trap))
    {
	send_to_char("You can't set up another trap yet.\n\r", ch);
	return;
    }

    if (room_is_affected(ch->in_room, gsn_trap))
    {
	send_to_char("There is already a trap setup here.\n\r", ch);
	return;
    }

    if (argument[0] == '\0' || !str_cmp(argument, "damage"))
    {
	ttype = TRAP_DAMAGE;

        if (ch->in_room->sector_type != SECT_INSIDE && ch->in_room->sector_type != SECT_CITY)
        {
	    send_to_char("There isn't sufficient structure about for a well-set trap.\n\r", ch);
	    return;
        }
    }
    else if (!str_cmp(argument, "grease"))
    {
	if (ch->class_num == global_int_class_watcher)
	{
	    send_to_char("You can't create that kind of trap.\n\r",ch);
	    return;
	}
	ttype = TRAP_GREASE;

        if (ch->in_room->sector_type != SECT_INSIDE && ch->in_room->sector_type != SECT_CITY)
        {
	    send_to_char("There is no proper surface to grease here.\n\r", ch);
	    return;
        }
    }
    else
    {
	send_to_char("Invalid trap type.\n\r", ch);
	return;
    }

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_trap].beats));

    if (number_percent() > skill) 
    {
        check_improve(ch,NULL,gsn_trap,FALSE,1);
	send_to_char("You fail to set the trap up properly.\n\r", ch);
	expend_mana(ch, skill_table[gsn_trap].min_mana/2);
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = gsn_trap;
    af.level     = ch->level;
    af.duration  = 150/ch->level;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch,&af);

    af.point     = (void *) ch;
    af.duration  = 12;
    af.modifier  = ttype;
    affect_to_room(ch->in_room, &af);

    check_improve(ch,NULL,gsn_trap,TRUE,1);
    
    switch (ttype)
    {
	case TRAP_DAMAGE:
    	    send_to_char("You quickly lay out a trap for the next unsuspecting fool to trip through.\n\r", ch);
    	    act("$n lays out a trap for the next person to come through here.", ch, NULL, NULL, TO_ROOM);
	    break;

	case TRAP_GREASE:
	    send_to_char("You carefully smear a barely noticable layer of grease across the floor.\n\r", ch);
	    act("$n smears a barely noticable layer of grease across the floor here.", ch, NULL, NULL, TO_ROOM);
	    break;
    }

    expend_mana(ch, skill_table[gsn_trap].min_mana);
}

void do_twirl( CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *wield;
    int chance, result;

    if ((chance = get_skill(ch, gsn_twirl)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (argument[0] == '\0')
    {
  	if ((victim = ch->fighting) == NULL)
	{
	    send_to_char("You aren't fighting anyone.\n\r", ch);
	    return;
	}
    }
    else if ((victim = get_char_room(ch, argument)) == NULL)
    {
	send_to_char("You don't see them here.\n\r", ch);
	return;
    }

    if (((wield = get_eq_char(ch, WEAR_WIELD)) == NULL || wield->value[0] != WEAPON_STAFF)
      && ((wield = get_eq_char(ch, WEAR_DUAL_WIELD)) == NULL || wield->value[0] != WEAPON_STAFF))
    {
	send_to_char("You can't twirl without a staff.\n\r", ch);
	return;
    }

    if (is_safe(ch,victim))
        return;

    if (!check_reach(ch, victim, REACH_HARD, PULSE_VIOLENCE))
	return;

    act("$n twirls $p, striking swiftly at $N with the gathered force.", ch, wield, victim, TO_NOTVICT);
    act("You twirl $p, striking swiftly at $N with the gathered force.", ch, wield, victim, TO_CHAR);
    act("$n twirls $p, striking swiftly at you with the gathered force.", ch, wield, victim, TO_VICT);

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_twirl].beats));

    if (!IS_NPC(victim) && (ch->fighting == NULL || victim->fighting != ch))
    {
	sprintf(buf, "Help! %s is attacking me!", PERS(ch, victim));
	do_autoyell(victim, buf);
    }
 
    chance += ((ch->level - victim->level)*2);

    if ((result = number_percent()) < chance)
    {
        check_improve(ch,victim,gsn_twirl,TRUE,1);
        damage_from_obj(ch,victim,wield,number_range(ch->level * 3/2, ch->level * 2),gsn_twirl, wield->value[3],TRUE);

        if (IS_VALID(victim) && !IS_OAFFECTED(victim,AFF_GHOST) 
	  && (ch->in_room == victim->in_room) && (result < chance/3))
	{
	    act("$n snaps $p back, using the other end to knock $N to the ground!", ch, wield, victim, TO_NOTVICT);
	    act("$n snaps $p back, using the other end to knock you to the ground!", ch, wield, victim, TO_VICT);
	    act("You snap $p back, using the other end to knock $N to the ground!", ch, wield, victim, TO_CHAR);
	    WAIT_STATE(victim, UMAX(victim->wait, 2*PULSE_VIOLENCE+number_range(1,3)-2));
	}
        check_improve(ch,victim,gsn_twirl,TRUE,1);
    }
    else
    {
        check_improve(ch,victim,gsn_twirl,FALSE,1);
        damage_old(ch,victim,0,gsn_twirl,DAM_OTHER,TRUE);
    }
}

void do_wariness(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int chance;

    if ((chance = get_skill(ch, gsn_wariness)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (is_affected(ch, gsn_wariness))
    {
	expend_mana(ch, skill_table[gsn_wariness].min_mana);
	if (ch->mana < 0)
	    ch->mana = 0;
	send_to_char("You are no longer wary.\n\r", ch);
	affect_strip(ch, gsn_wariness);
	return;
    }

    if (ch->mana < skill_table[ch->class_num].min_mana)
    {
	send_to_char("You are too tired to make yourself wary.\n\r", ch);
	return;
    }

    expend_mana(ch, skill_table[ch->class_num].min_mana);
    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_wariness].beats));

    if (chance < number_percent())
    {
	act("$n tries to make $mself wary, but cannot concentrate.", ch, NULL, NULL, TO_ROOM);
	act("You try to make yourself wary, but cannot concentrate.", ch, NULL, NULL, TO_CHAR);
        check_improve(ch,NULL,gsn_wariness,FALSE,1);
	return;
    }

    act("$n looks more aware.", ch, NULL, NULL, TO_ROOM);
    act("You heighten your senses and grow wary for trouble.", ch, NULL, NULL, TO_CHAR);
    check_improve(ch,NULL,gsn_wariness,TRUE,1);


    af.where        = TO_NAFFECTS;
    af.type         = gsn_wariness;
    af.level        = 0;
    af.duration     = -1;
    af.location     = APPLY_NONE;
    af.modifier     = chance * 3 / 4;
    af.bitvector    = AFF_WARINESS;
    affect_to_char(ch, &af);

    return;
}

void do_waylay( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af, *paf;
af.valid = TRUE;
af.point = NULL;
    OBJ_DATA *wield;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;

    one_argument(argument,arg);
    wield = get_eq_char( ch, WEAR_WIELD );

    if ((chance = get_skill(ch,gsn_waylay)) == 0)
    {	
	send_to_char("Huh?\n\r",ch);
	return;
    }
     
 
    if (arg[0] == '\0')
    {
	send_to_char("Waylay whom?\n\r", ch);
        return;
    }

    else if ((victim = get_char_room(ch,arg)) == NULL)
    {
	send_to_char("They aren't here.\n\r",ch);
	return;
    }

    if (is_affected(victim, gsn_waylay))
	{
	send_to_char("They're too suspicious for you to sneak up on.\n\r", ch);
	return;
	}

    if (victim->fighting)
    {
	send_to_char("They are moving too quickly for you to waylay.\n\r", ch);
	return;
    }

    if (victim == ch)
    {
	send_to_char("You can't seem to reach.\n\r",ch);
	return;
    }

    if (is_safe(ch,victim))
	return;

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
	act("But $N is your friend!",ch,NULL,victim,TO_CHAR);
	return;
    }

    /* modifiers */

    chance = (chance * 2) / 3;

    if (!IS_NPC(ch))
      ch->pcdata->adrenaline =  IS_NPC(victim) ? UMAX(ch->pcdata->adrenaline, 2) : UMAX(ch->pcdata->adrenaline, 20);

    if (!IS_NPC(victim))
	victim->pcdata->adrenaline = IS_NPC(ch) ? UMAX(victim->pcdata->adrenaline, 2) : UMAX(victim->pcdata->adrenaline, 20);

    if (is_affected(victim, gsn_diamondskin) && IS_SET(victim->imm_flags, IMM_WEAPON))
    {
	act("$N is unaffected by your blow!", ch, NULL, victim, TO_CHAR);
	act("You are unaffected by $n's sharp blow!", ch, NULL, victim, TO_VICT);
	act("$N is unaffected by $n's sharp blow!", ch, NULL, victim, TO_NOTVICT);
	WAIT_STATE(ch,skill_table[gsn_waylay].beats);
	return;
    }
    if (IS_SET(victim->act, ACT_NOSUBDUE) || is_affected(victim, gsn_thirdeye))
    {
        act("They're too wary for you to waylay.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (IS_AFFECTED(ch, AFF_SNEAK))
	chance += 10;

    /* stats */
    chance += get_curr_stat(ch,STAT_STR)/4;
    chance += get_curr_stat(ch,STAT_DEX)/3;
    chance -= (get_curr_stat(victim,STAT_DEX) * 4)/3;
    /* speed */
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
        chance -= 20;

    /* level */
    chance += (ch->level - victim->level);

    if (victim->position > POS_SLEEPING)
        for (paf = victim->affected; paf; paf = paf->next)
	    if (paf->type == gsn_wariness)
	    {
	        if (number_percent() <= paf->modifier)
	        {
		    act("You sense someone behind you, and narrowly evade $N's waylay!", victim, NULL, ch, TO_CHAR);
		    chance = 0;
	        }
	        break;
	    }

    if (is_affected(victim, gsn_sense_danger))
    {
	send_to_char("You sense incoming danger.\n\r", victim);
	chance /= 2;
    }

    if (!IS_NPC(victim) && BIT_GET(victim->pcdata->traits, TRAIT_STREETWISE))
	chance -= 10;

    if (can_see(victim, ch))
	chance /= 2;

    /* now the attack */
    if (number_percent() < chance )
    {
        act("$n knocks you on the back of the head.",
		ch,NULL,victim,TO_VICT);
	act("You knock $N unconscious with a well-placed blow.",
		ch,NULL,victim,TO_CHAR);
	act("$n knocks $N unconscious with a well-placed blow.",
		ch,NULL,victim,TO_NOTVICT);
	check_improve(ch,victim,gsn_waylay,TRUE,1);
	stop_fighting_all(victim);
	if (IS_AWAKE(victim))
            switch_position(victim, POS_SLEEPING);
	WAIT_STATE(ch,skill_table[gsn_waylay].beats);

	if (!IS_NPC(ch) && !IS_NPC(victim))
	{
	    sprintf(buf,"%s, you got knocked the FUCK OUT! (by $N in room %d).",victim->name, victim->in_room->vnum);
	    wiznet(buf,ch,NULL,WIZ_FLAGS,0,0);
	}

	af.where	= TO_AFFECTS;
	af.type		= gsn_waylay;
	af.level	= ch->level;
	af.duration	= 3;
	af.modifier	= 0;
	af.location	= 0;
	af.bitvector	= 0;
	affect_to_char(victim, &af);

	af.where 	= TO_AFFECTS;
	af.type		= gsn_waylay;
 	af.level	= 10;
	af.duration	= 2;
	af.modifier	= 0;
	af.location	= APPLY_NONE;
	af.bitvector	= AFF_SLEEP;
	affect_to_char (victim, &af);
    }
    else
    {
	do_visible(ch, "FALSE");
        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
            sprintf( buf, "Help!  %s tried to jump me!", PERS(ch, victim));
            do_autoyell( victim, buf );
        }	
        damage(ch,victim,number_range(1,5),gsn_waylay,DAM_BASH,FALSE);
	act("Your waylay misses!",
	    ch,NULL,victim,TO_CHAR);
	act("$n tried to waylay $N.",
	    ch,NULL,victim,TO_NOTVICT);
	act("You skillfully dodge $n's waylay.",
	    ch,NULL,victim,TO_VICT);
	check_improve(ch,victim,gsn_waylay,FALSE,1);
	WAIT_STATE(ch,skill_table[gsn_waylay].beats); 

	af.where	= TO_AFFECTS;
	af.type		= gsn_waylay;
	af.level	= ch->level;
	af.duration	= 2;
	af.modifier	= 0;
	af.location	= 0;
	af.bitvector	= 0;
	affect_to_char(victim, &af);

    }
	check_killer(ch,victim);
}


void do_rob( CHAR_DATA *ch, char *argument )
{
    char buf  [MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *vch;
    OBJ_DATA *obj;
    int percent, wskill, chance;

    if ((chance = get_skill(ch, gsn_rob)) == 0)
    {
	send_to_char("You lack skill in the art of robbing.\n\r", ch);
	return;
    }

    if ( argument[0] == '\0')
    {
	if (( victim = ch->fighting) == NULL)
	{
	    send_to_char( "Rob whom?\n\r", ch );
	    return;
	}
    }
    else if (( victim = get_char_room( ch, argument ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( victim == ch )
    {
	send_to_char( "You hit yourself, then move some coins from one pocket to the other.\n\r", ch );
	return;
    }

    if (is_safe(ch,victim))
	return;

    WAIT_STATE( ch, skill_table[gsn_rob].beats );
    percent = number_percent();

    if (!IS_AWAKE(victim))
    	percent -= 10;
    else if (!can_see(victim,ch))
    	percent += 25;
    else 
	percent += 50;

    if (!IS_NPC(victim) && BIT_GET(victim->pcdata->traits, TRAIT_STREETWISE))
	percent += 20;
    // Cannot steal from shoppies.
    
    chance /= 10;
    chance += 20 * hands_free(ch) ;
    
    if (IS_NPC(victim) && victim->pIndexData->pShop)
    {
	act("$N's hand darts down reflexively to prevent the theft.", ch, NULL, victim, TO_CHAR);
	chance = 0;
    }

    if ((percent > chance) && !IS_IMMORTAL(ch))
    {
	/*
	 * Failure.
	 */
	uncamo_char(ch);
        unhide_char(ch);
	send_to_char( "Oops.\n\r", ch );
	affect_strip(ch,gsn_sneak);
	REMOVE_BIT(ch->affected_by,AFF_SNEAK);

	act( "$n tried to rob you.", ch, NULL, victim, TO_VICT    );
	act( "$n tried to rob $N.",  ch, NULL, victim, TO_NOTVICT );
	switch(number_range(0,3))
	{
	    case 0 :
	        sprintf( buf, "%s is a lousy thief!", PERS(ch, victim));
	        break;
            case 1 :
	        sprintf( buf, "%s couldn't rob %s way out of a paper bag!",
		    PERS(ch,victim),(ch->sex == 2) ? "her" : "his");
	        break;
	    case 2 :
	        sprintf( buf,"%s tried to rob me!", PERS(ch, victim) );
	        break;
	    case 3 :
	        sprintf(buf,"Keep your hands out of there, %s!",PERS(ch, victim));
	        break;
	}
        if (!IS_AWAKE(victim))
            do_wake(victim,"");

	if (IS_AWAKE(victim) && !IS_NPC(victim) && (!IS_NPC(ch) || IS_SET(ch->act, ACT_GUILDGUARD)))
	    do_autoyell( victim, buf );

	if ( IS_NPC(victim) )
	{
	    check_improve(ch,victim,gsn_rob,FALSE,2);
	    multi_hit( victim, ch, TYPE_UNDEFINED );
	}
	else
	{
	    sprintf(buf,"$N tried to rob %s.",victim->name);
	    wiznet(buf,ch,NULL,WIZ_FLAGS,0,0);
	}

	check_killer(ch, victim);
	return;
    }
    
    if (IS_AFFECTED(ch, AFF_HIDE) && number_bits(1) == 0)
	unhide_char(ch);
    
    if (!IS_NPC(ch))
	ch->pcdata->adrenaline = IS_NPC(victim) ? UMAX(ch->pcdata->adrenaline, 2) : UMAX(ch->pcdata->adrenaline,20);

    one_hit(ch, victim, TYPE_UNDEFINED, HIT_PRIMARY, false);
    if (number_bits(2) == 1)
    {
	int i;
    	bool cFound = FALSE;
    	long coins[MAX_COIN];
    
    	for (i = 0; i < MAX_COIN; i++)
    	{
       	    coins[i] = round(victim->coins[i] * number_range(1, ch->level) / 60.0);
            if (coins[i] > 0)
    	        cFound = TRUE;
        }

    	inc_player_coins(ch, coins);
    	dec_player_coins(victim, coins);

    	sprintf(buf, "You got %s.\n\r", coins_to_str(coins));
    	send_to_char(buf, ch);

    	check_improve(ch,victim,gsn_steal,TRUE,2);
    	for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    	{
            if (ch == vch)
		continue;
	    if (can_see(vch, ch)
              && ((wskill = get_skill(vch, gsn_detecttheft)) > 0)
	      && (wskill > number_percent()))
	    {
    	        check_improve(vch,ch,gsn_detecttheft,TRUE,2);
	    	if (vch == victim)
	    	{
	            sprintf(buf, "%s stole money from you.\n\r", PERS(ch, vch));
	            send_to_char(buf, vch);
	    	}
	    	else
	    	{
	            sprintf(buf, "%s stole money from %s.\n\r", PERS(ch, vch), PERS(victim, vch));
	            send_to_char(buf, vch);
	    	}
	    }
    	}
    }
    else
    {
	int i=0,j=0,m,c;
	for (obj = victim->carrying;obj;obj=obj->next_content)
	    i++;
	
	while(j < chance/5)
	{
	    j++;
    	    obj = victim->carrying;
	    m = number_range(1,i);
	    for (c=1;c<m;c++)
		if (obj)
		    obj = obj->next_content;
	    if (obj && obj->worn_on)
	        continue;
	    if (obj)
		break;
	}    

	if (!obj || obj->worn_on)
	{
	    send_to_char("You couldn't get anything.\n\r",ch);
	    return;
	}

        if ( !can_drop_obj( ch, obj )
          || IS_SET(obj->extra_flags[0], ITEM_INVENTORY))
        {
	    act("You can't pry $p away.",ch,obj,NULL,TO_CHAR);
	    return;
	}

        if (!IS_NPC(ch) && !IS_IMMORTAL(ch)
         && (get_cur_capacity(ch) + item_svalue[obj->size]) > get_max_capacity(ch))
        {
	    act("You lack the room to carry $p.", ch, obj, NULL, TO_CHAR);
	    return;
        }

        if ( ch->carry_number + 1 > static_cast<int>(Encumbrance::CarryCountCapacity(*ch)))
        {
	    send_to_char( "You have your hands full.\n\r", ch );
	    return;
        }

        if ( ch->carry_weight + get_obj_weight( obj ) > static_cast<int>(Encumbrance::CarryWeightCapacity(*ch)))
        {
	    send_to_char( "You can't carry that much weight.\n\r", ch );
	    return;
        }
        if (obj == ch->nocked)
	    ch->nocked = NULL;
        obj_from_char( obj );
        obj_to_char( obj, ch );
        act("You grab $p from $N.",ch,obj,victim,TO_CHAR);
    	check_improve(ch,victim,gsn_rob,TRUE,1);
        
	for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
        {
	    if (vch == ch)
		continue;
	    if (can_see(vch, ch) && can_see_obj(vch, obj)
	      && (wskill = get_skill(vch, gsn_detecttheft)) > 0
	      && wskill > number_percent())
	    {
    	        check_improve(vch,ch,gsn_detecttheft,TRUE,2);
	        if (vch == victim)
	        {
		    sprintf(buf, "%s stole %s from you.\n\r", PERS(ch, vch), obj->short_descr);
		    send_to_char(buf, vch);
	        }
	        else
	        {
		    sprintf(buf, "%s stole %s from %s.\n\r", PERS(ch, vch), obj->short_descr, PERS(victim, vch));
		    send_to_char(buf, vch);
		    log_string(buf);
	        }
	    }
        }

        check_aura(ch, obj, TRUE);
    }
    return;
}

// If source = 0, poison is from eat
void deathpoison(CHAR_DATA *ch, CHAR_DATA *victim, int level, int source)
{
    AFFECT_DATA af;
    int chance = 0;
    if (victim->class_num == global_int_class_assassin
      || IS_IMMORTAL(victim)
      || (IS_NPC(victim) 
	&& (IS_SET(victim->act,ACT_NOSUBDUE)
	  || IS_SET(victim->act,ACT_BADASS)
	  || IS_SET(victim->act,ACT_UNDEAD)
	  || IS_SET(victim->form, FORM_UNDEAD)))
      || is_affected(victim,gsn_deathpoison))
	return;
    if (is_affected(victim,gsn_protectionfrompoison))
    {
        send_to_char("Poison burns in your veins, but quickly dissipates as it flushes out of your system.\n\r",victim);
        return;
    }

    if (number_percent() <= get_skill(victim, gsn_detoxify))
    {
        check_improve(victim, NULL, gsn_detoxify, true, 4);
        send_to_char("Poison burns in your veins, but quickly dissipates as it flushes out of your system.\n", victim);
        return;
    }
    check_improve(victim, NULL, gsn_detoxify, false, 4);

    if (source == gsn_envenom)
	chance = 0;
    else if (source == gsn_taint)
	chance = 45;

    chance -= get_curr_stat(victim,STAT_CON);
    if (source == gsn_dart)
	chance = 5;

    if (number_percent() < chance)
    {
	if (number_percent() > get_resist(victim, DAM_POISON))
	{
	    act("Your heart seizes, then stops, as the poison in your system quickly takes hold.",victim,NULL,NULL,TO_CHAR);
	    act("$n suddenly convulses, the poison in $s system quickly taking hold.",victim,NULL,NULL,TO_ROOM);
	    raw_kill(victim);
	    return;
	}
    }
    if (number_percent() > (40-get_curr_stat(ch,STAT_CON)))
        return;
    
    act("You clutch a fist at your chest as pain fills your heart.",victim,NULL,NULL,TO_CHAR);
    act("$n involuntarily clutches a fist at $s chest.",victim,NULL,NULL,TO_ROOM);

    af.valid = TRUE;
    af.point = NULL;
    af.where = TO_AFFECTS;
    af.type = gsn_deathpoison;
    af.location = APPLY_CON;
    af.modifier = -2;
    af.duration = 5;
    af.level = level;
    af.bitvector = AFF_POISON;
    affect_to_char(victim,&af);
}

void erosivepoison(CHAR_DATA *ch, CHAR_DATA *victim, int level, int source)
{
    AFFECT_DATA af;
    int chance = 0;
    if (victim->class_num == global_int_class_assassin
      || IS_IMMORTAL(victim)
      || (IS_NPC(victim) 
	&& (IS_SET(victim->act,ACT_NOSUBDUE)
	  || IS_SET(victim->act,ACT_BADASS)
	  || IS_SET(victim->act,ACT_UNDEAD)
	  || IS_SET(victim->form, FORM_UNDEAD))))
	return;
    if (is_affected(victim,gsn_protectionfrompoison))
    {
	send_to_char("Poison burns in your veins, but quickly dissipates as it flushes out of your system.\n\r",victim);
	return;
    }

    if (number_percent() <= get_skill(victim, gsn_detoxify))
    {
        check_improve(victim, NULL, gsn_detoxify, true, 4);
        send_to_char("Poison burns in your veins, but quickly dissipates as it flushes out of your system.\n", victim);
        return;
    }
    check_improve(victim, NULL, gsn_detoxify, false, 4);

    if (source == gsn_envenom)
	chance = 30;
    else if (source == gsn_taint)
	chance = 75;
    else if (source == gsn_dart)
	chance = 60;

    chance -= get_curr_stat(victim,STAT_CON);

    if (number_percent() < chance)
    {
	if (number_percent() > get_resist(victim,DAM_POISON))
	{
	    act("You feel a very strong burning sensation.",victim,NULL,NULL,TO_CHAR);
	    do_autoyell(victim,"Aaauuugghhh!");
	    damage(victim,victim,number_range(level*7/4,level*9/4),gsn_erosivepoison,DAM_ACID,TRUE);
	    if (victim && !IS_NAFFECTED(victim,AFF_GHOST))
	        return;
	    if (!is_affected(victim,gsn_erosivepoison))
	    {
	    	af.valid = TRUE;
	    	af.point = NULL;
	    	af.where = TO_NAFFECTS;
	    	af.type = gsn_erosivepoison;
	    	af.bitvector = 0;
	    	af.level = level;
	    	af.location = APPLY_CHR;
	    	af.duration = 5;
	    	af.modifier = -2;
	    	affect_to_char(victim,&af);
	    }
	    return;
	}
    }
    
    act("You feel a burning sensation.",victim,NULL,NULL,TO_CHAR);
    damage(victim,victim,number_range(level*3/4,level*5/4),gsn_erosivepoison,DAM_ACID,TRUE);
}

void sleeppoison(CHAR_DATA *ch, CHAR_DATA *victim, int level, int source)
{
    AFFECT_DATA af;
    int chance = 0, percent;
    if (victim->class_num == global_int_class_assassin
      || IS_IMMORTAL(victim)
      || (IS_NPC(victim) 
	&& (IS_SET(victim->act,ACT_NOSUBDUE)
	  || IS_SET(victim->act,ACT_BADASS)
	  || IS_SET(victim->act,ACT_UNDEAD)
	  || IS_SET(victim->form, FORM_UNDEAD)))
      || is_affected(victim,gsn_sleeppoison))
	return;
    if (is_affected(victim,gsn_protectionfrompoison))
    {
	send_to_char("Poison burns in your veins, but quickly dissipates as it flushes out of your system.\n\r",victim);
	return;
    }
   
    if (number_percent() <= get_skill(victim, gsn_detoxify))
    {
        check_improve(victim, NULL, gsn_detoxify, true, 4);
        send_to_char("Poison burns in your veins, but quickly dissipates as it flushes out of your system.\n", victim);
        return;
    }
    check_improve(victim, NULL, gsn_detoxify, false, 4);

    if (source == gsn_envenom)
	return;
    else if (source == gsn_taint)
	chance = 65;
    else if (source == gsn_dart)
	chance = 50;

    chance -= get_curr_stat(victim,STAT_CON);

    af.valid = TRUE;
    af.point = NULL;
    af.where = TO_AFFECTS;
    af.type = gsn_sleeppoison;
    af.location = APPLY_NONE;
    af.level = level;
    af.bitvector = AFF_SLEEP;
    af.modifier = 0;

    percent = number_percent();

    if (number_percent() < percent)
    {
	if (number_percent() > get_resist(victim,DAM_POISON))
	{    
	    act("Sudden drowsiness overcomes you, and you collapse to the ground.",victim,NULL,NULL,TO_CHAR);
	    act("$n looks quite tired, then slumps to the ground.",victim,NULL,NULL,TO_ROOM);
	    af.duration = 2;
	    stop_fighting_all(victim);
    	    affect_to_char(victim,&af);
    	    switch_position(victim,POS_SLEEPING);
	    return;
	}
        act("Drowsiness overcomes you, and you collapse to the ground.",victim,NULL,NULL,TO_CHAR);
    	act("$n looks tired, then slumps to the ground.",victim,NULL,NULL,TO_ROOM);
    	af.duration = 0;
    
   	stop_fighting_all(victim);
    	affect_to_char(victim,&af);
    	switch_position(victim,POS_SLEEPING);
    }
}

void exhaustionpoison(CHAR_DATA *ch, CHAR_DATA *victim, int level, int source)
{
    AFFECT_DATA af;
    int chance = 0;
    if (victim->class_num == global_int_class_assassin
      || IS_IMMORTAL(victim)
      || (IS_NPC(victim) 
	&& (IS_SET(victim->act,ACT_NOSUBDUE)
	  || IS_SET(victim->act,ACT_BADASS)
	  || IS_SET(victim->act,ACT_UNDEAD)
	  || IS_SET(victim->form, FORM_UNDEAD)))
      || is_affected(victim,gsn_exhaustionpoison))
	return;
    if (is_affected(victim,gsn_protectionfrompoison))
    {
	send_to_char("Poison burns in your veins, but quickly dissipates as it flushes out of your system.\n\r",victim);
	return;
    }
    
    if (number_percent() <= get_skill(victim, gsn_detoxify))
    {
        check_improve(victim, NULL, gsn_detoxify, true, 4);
        send_to_char("Poison burns in your veins, but quickly dissipates as it flushes out of your system.\n", victim);
        return;
    }
    check_improve(victim, NULL, gsn_detoxify, false, 4);

    if (source == gsn_envenom)
	chance = 30;
    else if (source == gsn_taint)
	chance = 75;
    else if (source == gsn_dart)
	chance = 60;
    
    chance -= get_curr_stat(victim,STAT_CON);

    af.valid = TRUE;
    af.point = NULL;
    af.where = TO_AFFECTS;
    af.type = gsn_exhaustionpoison;
    af.location = APPLY_DEX;
    af.level = level;
    af.bitvector = AFF_POISON;
    
    if (number_percent() < chance)
    {
	if (number_percent() > get_resist(victim,DAM_POISON))
	{
	    act("You feel exhausted.",victim,NULL,NULL,TO_CHAR);
	    act("$n looks tired.",victim,NULL,NULL,TO_ROOM);
	    af.duration = 5;
	    af.modifier = -4;
            affect_to_char(victim,&af);
	    return;
	}
    }

    act("You feel a bit weary.",victim,NULL,NULL,TO_CHAR);
    act("$n looks a bit ragged.",victim,NULL,NULL,TO_ROOM);
    af.modifier = -2;
    af.duration = 3;
    affect_to_char(victim,&af);
}

void delusionpoison(CHAR_DATA *ch, CHAR_DATA *victim, int level, int source)
{
    AFFECT_DATA af;
    int chance = 0;
    if (victim->class_num == global_int_class_assassin
      || IS_IMMORTAL(victim)
      || (IS_NPC(victim) 
	&& (IS_SET(victim->act,ACT_NOSUBDUE)
	  || IS_SET(victim->act,ACT_BADASS)
	  || IS_SET(victim->act,ACT_UNDEAD)
	  || IS_SET(victim->form, FORM_UNDEAD)))
      || is_affected(victim,gsn_delusionpoison))
	return;
    if (is_affected(victim,gsn_protectionfrompoison))
    {
	send_to_char("Poison burns in your veins, but quickly dissipates as it flushes out of your system.\n\r",victim);
	return;
    }

    if (number_percent() <= get_skill(victim, gsn_detoxify))
    {
        check_improve(victim, NULL, gsn_detoxify, true, 4);
        send_to_char("Poison burns in your veins, but quickly dissipates as it flushes out of your system.\n", victim);
        return;
    }
    check_improve(victim, NULL, gsn_detoxify, false, 4);

    if (source == gsn_envenom)
	chance = 30;
    else if (source == gsn_taint)
	chance = 75;
    else if (source == gsn_dart)
	chance = 60;

    chance -= get_curr_stat(victim,STAT_CON);

    af.valid = TRUE;
    af.point = NULL;
    af.where = TO_NAFFECTS;
    af.type = gsn_delusionpoison;
    af.bitvector = AFF_DELUSION;
    af.level = level;
    af.location = APPLY_WIS;
    if (number_percent() < chance)
    {
	if (number_percent() > get_resist(victim,DAM_POISON))
	{
	    act("Your vision fades for a moment, then returns.",victim,NULL,NULL,TO_CHAR);
	    act("$n looks awestruck.",victim,NULL,NULL,TO_ROOM);
	    af.duration = 5;
	    af.modifier = -4;
	    affect_to_char(victim,&af);
	    return;
	}
    }
    act("You are less attentive.",victim,NULL,NULL,TO_CHAR);
    act("$n looks bored.",victim,NULL,NULL,TO_ROOM);
    af.duration = 3;
    af.modifier = -2;
    affect_to_char(victim,&af);
}

void painpoison(CHAR_DATA *ch, CHAR_DATA *victim, int level, int source)
{
    AFFECT_DATA af;
    int chance = 0;
    if (victim->class_num == global_int_class_assassin
      || IS_IMMORTAL(victim)
      || (IS_NPC(victim) 
	&& (IS_SET(victim->act,ACT_NOSUBDUE)
	  || IS_SET(victim->act,ACT_BADASS)
	  || IS_SET(victim->act,ACT_UNDEAD)
	  || IS_SET(victim->form, FORM_UNDEAD)))
      || is_affected(victim,gsn_painpoison))
	return;
    if (is_affected(victim,gsn_protectionfrompoison))
    {
	send_to_char("Poison burns in your veins, but quickly dissipates as it flushes out of your system.\n\r",victim);
	return;
    }

    if (number_percent() <= get_skill(victim, gsn_detoxify))
    {
        check_improve(victim, NULL, gsn_detoxify, true, 4);
        send_to_char("Poison burns in your veins, but quickly dissipates as it flushes out of your system.\n", victim);
        return;
    }
    check_improve(victim, NULL, gsn_detoxify, false, 4);


    if (source == gsn_envenom)
	chance = 30;
    else if (source == gsn_taint)
	chance = 75;
    else if (source == gsn_dart)
	chance = 60;

    chance -= get_curr_stat(victim,STAT_CON);

    af.valid = TRUE;
    af.point = NULL;
    af.where = TO_AFFECTS;
    af.type = gsn_painpoison;
    af.bitvector = AFF_POISON;
    af.level = level;
    if (number_percent() < chance)
    {
	if (number_percent() > get_resist(victim,DAM_POISON))
	{
	    act("Intense pain blooms throughout your body.",victim,NULL,NULL,TO_CHAR);
	    act("$n doubles over in pain.",victim,NULL,NULL,TO_ROOM);
	    af.duration = 5;
	    af.location = APPLY_DAMROLL;
	    af.modifier = -12;
	    affect_to_char(victim,&af);
	    af.location = APPLY_HITROLL;
	    af.modifier = -10;
	    affect_to_char(victim,&af);
	    af.location = APPLY_STR;
	    af.modifier = -5;
	    affect_to_char(victim,&af);
	    return;
	}
    }
    act("Pain blooms throughout your body.",victim,NULL,NULL,TO_CHAR);
    act("$n grimaces in pain.",victim,NULL,NULL,TO_ROOM);
    af.duration = 3;
    af.location = APPLY_DAMROLL;
    af.modifier = -2;
    affect_to_char(victim,&af);
    af.location = APPLY_HITROLL;
    affect_to_char(victim,&af);
    af.location = APPLY_STR;
    affect_to_char(victim,&af);
}

void tremblingpoison(CHAR_DATA *ch, CHAR_DATA *victim, int level, int source)
{
    AFFECT_DATA af;
    int chance = 0;
    if (victim->class_num == global_int_class_assassin
      || IS_IMMORTAL(victim)
      || (IS_NPC(victim) 
	&& (IS_SET(victim->act,ACT_NOSUBDUE)
	  || IS_SET(victim->act,ACT_BADASS)
	  || IS_SET(victim->act,ACT_UNDEAD)
	  || IS_SET(victim->form, FORM_UNDEAD)))
      || is_affected(victim,gsn_tremblingpoison))
	return;
    if (is_affected(victim,gsn_protectionfrompoison))
    {
	send_to_char("Poison burns in your veins, but quickly dissipates as it flushes out of your system.\n\r",victim);
	return;
    }

    if (number_percent() <= get_skill(victim, gsn_detoxify))
    {
        check_improve(victim, NULL, gsn_detoxify, true, 4);
        send_to_char("Poison burns in your veins, but quickly dissipates as it flushes out of your system.\n", victim);
        return;
    }
    check_improve(victim, NULL, gsn_detoxify, false, 4);

    if (source == gsn_envenom)
	chance = 30;
    else if (source == gsn_taint)
	chance = 75;
    else if (source == gsn_dart)
	chance = 60;

    chance -= get_curr_stat(victim,STAT_CON);

    af.valid = TRUE;
    af.point = NULL;
    af.where = TO_NAFFECTS;
    af.type = gsn_tremblingpoison;
    af.bitvector = AFF_TREMBLE;
    af.level = level;
    af.location = APPLY_HIDE;
    if (number_percent() < chance)
    {
	if (number_percent() > get_resist(victim,DAM_POISON))
	{
	    act("Your muscles begin spasming uncontrollably.",victim,NULL,NULL,TO_CHAR);
	    act("$n begins twitching violently.",victim,NULL,NULL,TO_ROOM);
	    af.duration = 4;
	    af.modifier = 20;
	    affect_to_char(victim,&af);
	    return;
	}
    }
    act("Your muscles begin spasming occasionally.",victim,NULL,NULL,TO_CHAR);
    act("$n begins twitching slightly.",victim,NULL,NULL,TO_ROOM);
    af.duration = 2;
    af.modifier = 10;
    affect_to_char(victim,&af);
}

void do_bounty(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    char victname[MAX_INPUT_LENGTH];
    char amount[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    unsigned long bounty;

    if (ch->class_num != global_int_class_watcher
      || ch->level < 40)
    {
	send_to_char("Huh?\n\r",ch);
	return;
    }

    argument = one_argument(argument,victname);

    if (victname[0] == '\0' || argument[0] == '\0')
    {
	send_to_char("Place a bounty on whom for how much?\n\r",ch);
	return;
    }
    
    one_argument(argument,amount);
    if (!is_number(amount))
    {
	send_to_char("Place a bounty on whom for how much?\n\r",ch);
	return;
    }

    if ((victim = get_char_room(ch,victname)) == NULL)
	if ((victim = get_char_world(ch,victname)) == NULL)
	{
	    send_to_char("You can find no such person to place a bounty on.\n\r",ch);
	    return;
	}

    if (IS_NPC(victim))
    {
	send_to_char("You may only call for bounties upon your fellow adventurers.\n\r",ch);
	return;
    }

    bounty = atoi(amount);
    
    if (!dec_player_bank(ch,value_to_coins(bounty*1000,FALSE)))
	if (!dec_player_coins(ch,value_to_coins(bounty*1000,FALSE)))
	{
	    send_to_char("You don't have enough platinum for that bounty.\n\r",ch);
	    return;
	}

    if (victim->pcdata->bounty > 0)
    {
	victim->pcdata->bounty += bounty;
	sprintf(buf,"The bounty on %s's head has increased to %lu platinum coin%s!\n\r",victim->name,victim->pcdata->bounty,victim->pcdata->bounty == 1 ? "" : "s");
	send_to_char(buf,ch);
	sprintf(buf,"The bounty on your head has increased to %lu platinum coin%s!\n\r",victim->pcdata->bounty,victim->pcdata->bounty == 1 ? "" : "s");
	send_to_char(buf,victim);
    }
    else
    {
	victim->pcdata->bounty += bounty;
	sprintf(buf,"There is now a bounty on %s's head of %lu platinum coin%s!\n\r",victim->name,victim->pcdata->bounty,victim->pcdata->bounty == 1 ? "" : "s");
	send_to_char(buf,ch); 
	sprintf(buf,"There is now a bounty on your head of %lu platinum coin%s!\n\r",victim->pcdata->bounty,victim->pcdata->bounty == 1 ? "" : "s");
	send_to_char(buf,victim);
    }
}

void do_pursuit(CHAR_DATA *ch, char *argument)
{
// Start cost 25, maint 25 if all or 15 if targetted
// Syntax: pursuit (no argument):
//	if not affected, turns on pursuit
//	if affected, shows you whether you are hunting all or specific tracks
// pursuit (name):
//	if not affected, turns on pursuit if tracks are in room
//	if affected, turns on pursuit if tracks are in room
// pursuit stop
// 	if not affected, you can't stop. Idiot.
//	if affected, turns off pursuit

    AFFECT_DATA af;
    AFFECT_DATA *paf;
    TRACK_DATA *pTrack;
    int skill = get_skill(ch,gsn_pursuit);
    char buf[MAX_STRING_LENGTH];
        
    if (skill == 0)
    {
	send_to_char("Huh?\n\r",ch);
	return;
    }

    if (ch->in_room
      && !(ch->in_room->sector_type == SECT_INSIDE
	|| ch->in_room->sector_type == SECT_CITY
	|| ch->in_room->sector_type == SECT_ROAD)
      && str_cmp(argument,"stop"))
    {
	send_to_char("You can't look for tracks here.\n\r",ch);
	return;
    }

    if (argument[0]=='\0')
    {
	if ((paf = affect_find(ch->affected,gsn_pursuit)) == NULL)
	{
	    if (ch->mana - skill_table[gsn_pursuit].min_mana < 0)
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
	    af.type = gsn_pursuit;
	    af.bitvector = 0;
	    af.location = APPLY_NONE;
	    affect_to_char(ch,&af);
	    WAIT_STATE(ch,UMAX(ch->wait,skill_table[gsn_pursuit].beats));
	    return;
	}
	sprintf(buf,"You are looking for %s%stracks.\n\r",(char *)(paf->point) ? (char *)(paf->point) : "all",(char *)(paf->point) ? "'s " : " ");
	send_to_char(buf,ch);
	return;
    }
    else if (!str_cmp(argument,"stop"))
    {
	if (is_affected(ch,gsn_pursuit))
	{
	    act("You stop looking for tracks.",ch,NULL,NULL,TO_CHAR);
	    act("$n stops looking for tracks.",ch,NULL,NULL,TO_ROOM);
	    affect_strip(ch,gsn_pursuit);
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

	if ((paf = affect_find(ch->affected,gsn_pursuit)) == NULL)
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
	    WAIT_STATE(ch,UMAX(ch->wait,skill_table[gsn_pursuit].beats));
	    send_to_char(buf,ch);
	    return;
	}
    }
}

bool check_detectstealth(CHAR_DATA *ch, CHAR_DATA *victim)
{
    if (ch == victim)
	return FALSE;
    int chance = get_skill(ch,gsn_detectstealth);
    bool success = FALSE;

    if (chance > 0 && can_see(ch,victim) && !IS_AFFECTED(ch,AFF_WIZI)
      && skill_table[gsn_detectstealth].skill_level[ch->class_num] <= ch->level)
	if (number_percent() < chance)
	{
	    success = TRUE;
	    check_improve(ch,victim,gsn_detectstealth,TRUE,1);
	}
	else
	    check_improve(ch,victim,gsn_detectstealth,FALSE,1);

    return success;
}

void do_daggertrap( CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int skill, direction;
    char buf[MAX_STRING_LENGTH];

    if ((skill = get_skill(ch,gsn_daggertrap)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (IS_OAFFECTED(ch, AFF_GHOST))
    {
	send_to_char("You cannot set a trap in your current state.\n\r", ch);
	return;
    }
 
    if (ch->mana < skill_table[gsn_trap].min_mana)
    {
	send_to_char("You can't concentrate enough to set up a trap.\n\r", ch);
	return;
    }

    if (is_affected(ch, gsn_daggertrap))
    {
	send_to_char("You can't set up another trap yet.\n\r", ch);
	return;
    }

    if (room_is_affected(ch->in_room, gsn_daggertrap))
    {
	send_to_char("There is already a trap setup here.\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("Across which direction did you want to string the wire?\n\r",ch);
	return;
    }
    
    for (direction = 0;direction<6;direction++)
	if (!str_prefix(argument,dir_name[direction]))
	    break;

    if (direction == 6
      || ch->in_room->exit[direction] == NULL)
    {
	send_to_char("Across which direction did you want to string the wire?\n\r",ch);
	return;	
    }

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_daggertrap].beats));

    if (number_percent() > skill) 
    {
        check_improve(ch,NULL,gsn_trap,FALSE,1);
	send_to_char("You fail to set the trap up properly.\n\r", ch);
	expend_mana(ch, skill_table[gsn_trap].min_mana);
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = gsn_daggertrap;
    af.level     = ch->level;
    af.duration  = 150/ch->level;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch,&af);

    af.point     = (void *) ch;
    af.duration  = 12;
    af.modifier  = direction;
    affect_to_room(ch->in_room, &af);

    check_improve(ch,NULL,gsn_daggertrap,TRUE,1);
    
    send_to_char("You quickly lay out a trap for the next unsuspecting fool to trip through.\n\r", ch);
    sprintf(buf,"$n lays out a trap for the next person to leave %s.",dir_name[direction]);
    act(buf, ch, NULL, NULL, TO_ROOM);

    expend_mana(ch, skill_table[gsn_trap].min_mana);
    return;
}
