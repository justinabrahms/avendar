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
*       ROM license, in the file Rom24/doc/rom.license                     *
***************************************************************************/
/* This code is LOFT derivative of diku/merc/ROM. See LICENSE.loft for     *
 * information, or ftp to ftp.ender.com:/pub/loft for a complete version   */

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <time.h>
#include <cmath>
#include <set>
#include "merc.h"
#include "magic.h"
#include "lookup.h"
#include "tables.h"
#include "RoomPath.h"
#include "interp.h"
#include "spells_spirit.h"
#include "spells_earth.h"
#include "Direction.h"
#include "Player.h"

void offhanddisarm( CHAR_DATA *ch, CHAR_DATA *victim );

DECLARE_DO_FUN(do_yell);
DECLARE_DO_FUN(do_autoyell);
DECLARE_DO_FUN(do_visible);

extern	bool	check_aura	args( ( CHAR_DATA *ch, OBJ_DATA *obj,
					bool damage ) );

extern	bool 	damage_from_obj args( ( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj,
                     			int dam, int dt, int dam_type, bool show) );

/* test a*/
extern bool global_bool_ranged_attack;
extern bool global_bool_check_avoid;
extern char *   global_damage_from;

void mob_class_aggr(CHAR_DATA *ch, CHAR_DATA *victim);

void do_surpriseattack(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af, *paf;
af.valid = TRUE;
af.point = NULL;
    ROOM_INDEX_DATA *room, *troom;
    int chance, direction;
    char arg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *wield;
    int thac0, thac0_00, thac0_32;
    int victim_ac;
    int dam, diceroll;
    int sn, skill;
    int dam_type;
    int dt;

    if ((chance = get_skill(ch, gsn_surpriseattack)) == 0)
    {
      send_to_char("Huh?\n\r", ch);
      return;
    }

    if (argument[0] == '\0')
    {
      send_to_char("Surprise whom?  And in which direction?\n\r", ch);
      return;
    }

    argument = one_argument(argument, arg);

    if (argument[0] == '\0')
    {
        send_to_char("Surprise attack in which direction?\n\r",ch);
        return;
    }

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

    if (ch->in_room->exit[direction] == NULL)
    {
	send_to_char("Invalid direction.\n\r",ch);
	return;
    }

    if ((room = ch->in_room) == NULL)
        return;

    troom = ch->in_room->exit[direction]->u1.to_room; 
    if (room == troom)    
    {
        send_to_char("You can't surprise someone in this room!\n\r", ch);
	return;
    }

    move_char(ch, direction, FALSE);
    
    /* check if room successfully entered */
    if (room != troom)
    {
	sprintf(buf, "%s charges into the room from %s.", IS_NPC(ch) ? ch->short_descr : ch->name,
	OPPOSITE(direction) == 0 ? "the north" :
	(OPPOSITE(direction) == 1 ? "the east" :
	(OPPOSITE(direction) == 2 ? "the south" :
	(OPPOSITE(direction) == 3 ? "the west" :
	(OPPOSITE(direction) == 4 ? "above" :
	"below")))));
	act(buf, ch, NULL, NULL, TO_ROOM);

   	if (ch->wait > 1)
	{
	    send_to_char("You stumble as you charge into the room, and are unable to continue your attack.\n\r", ch);
            act("$n stumbles as he charges into the room, and stops.", ch, NULL, NULL, TO_ROOM);
  	    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_surpriseattack].beats));
	    return;
        }		
   
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_surpriseattack].beats));
 
	if ((victim = get_char_room(ch, arg)) == NULL)
	{
	  send_to_char("You don't see your target here.\n\r", ch);
	  return;
	}

        if (ch == victim)
	{
	    send_to_char("You can't surprise yourself.\n\r", ch);
	    return;
	}
        
        if (!IS_NPC(victim) && (!IS_PK(ch, victim)))
        {
	    send_to_char("The gods protect your victim.\n\r", ch);
	    return;
        }

	if (is_safe(ch, victim))
	{
	    send_to_char("You can't attack them.\n\r", ch);
	    return;
	}
 
    if (is_affected(victim,gsn_protective_shield) && (number_percent() < 50))
    {
        act("You charge into $N, but your attack slides around $S!", ch, NULL, victim, TO_CHAR);
        act("$n charge seems to slide around $N!", ch, NULL, victim, TO_NOTVICT);
        act("$n charges into you, but is deflected by your protective shield!", ch, NULL, victim, TO_VICT);
        if (victim->fighting == NULL)
        {
        victim->fighting = ch;
        switch_position(victim, POS_FIGHTING);
        }
    }
    else if (is_affected(victim,gsn_flameshield) || is_affected(victim, gsn_lambentaura))
    {
        act("$n is scorched by $N's shield of flame and knocked away!", ch, NULL, victim, TO_NOTVICT);
        act("You are scorched by $N's shield of flame and knocked away!", ch, NULL, victim, TO_CHAR);
        act("$n is scorched by your shield of flame and knocked away!", ch, NULL, victim, TO_VICT);
        WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE+1));
        damage(victim,ch,2*number_range(victim->level, 2*victim->level),gsn_flameshield,DAM_FIRE,TRUE);
        if (victim->fighting == NULL)
        {
            victim->fighting = ch;
            switch_position(victim, POS_FIGHTING);
        }
    }
	else if (victim->in_room->sector_type != SECT_UNDERWATER && (is_affected(victim,gsn_windwall) && number_bits(1) == 0))
	{
	    act("You charge into $N's wall of wind, and are knocked away.", ch, NULL, victim, TO_CHAR);
	    act("$n charges into $N's wall of wind, and is knocked away.", ch, NULL, victim, TO_NOTVICT);
	    act("$n charges into your wall of wind, and is knocked away.", ch, NULL, victim, TO_VICT);
	    WAIT_STATE(ch, UMAX(ch->wait, (2*PULSE_VIOLENCE)+2));
	    if (victim->fighting == NULL)
	    {
		victim->fighting = ch;
		switch_position(victim, POS_FIGHTING);
	    }
	}
	else if (is_affected(victim, gsn_mistralward) && victim->in_room->sector_type != SECT_UNDERWATER && number_bits(2) == 0)
	{
	    act("$N's mistral ward throws $n aside, disrupting the charge!", ch, NULL, victim, TO_NOTVICT);
	    act("$N's mistral ward throws you aside, disrupting the charge!", ch, NULL, victim, TO_CHAR);
	    act("Your mistral ward throws $n aside, disrupting the charge!", ch, NULL, victim, TO_VICT);
	    WAIT_STATE(ch, UMAX(ch->wait, (2*PULSE_VIOLENCE)));
        if (victim->fighting == NULL)
        {
            victim->fighting = ch;
            switch_position(victim, POS_FIGHTING);
        }
    }
	else if (is_affected(victim,gsn_wariness) && (number_percent() < (get_skill(victim,gsn_wariness)/2)))
	{
	    act("$N senses your charge and quickly sidesteps your attack.", ch, NULL, victim, TO_CHAR);
	    act("$N quickly sidesteps $n's charge.", ch, NULL, victim, TO_NOTVICT);
	    act("You sense $N charging at you, and quickly step out of the way.", ch, NULL, victim, TO_VICT);
	    if (victim->fighting == NULL)
	    {
		victim->fighting = ch;
		switch_position(victim, POS_FIGHTING);
	    }
        }
	else if (is_affected(victim,gsn_scouting) && (number_percent() < (get_skill(victim,gsn_scouting)/2)))
	{
	    act("$N notices your charge and quickly sidesteps your attack.", ch, NULL, victim, TO_CHAR);
	    act("$N notices $n's charge and quickly sidesteps the attack.", ch, NULL, victim, TO_NOTVICT);
	    act("You notice $n charging at you, and quickly step out of the way.", ch, NULL, victim, TO_VICT);
	    if (victim->fighting == NULL)
	    {
		victim->fighting = ch;
		switch_position(victim, POS_FIGHTING);
	    }
	}
        else if (is_affected(victim,gsn_surpriseattack))
	{
	    act("$N is still wary of you, and easily sidesteps your attack.", ch, NULL, victim, TO_CHAR);
	    act("$n charges at $N, but misses.", ch, NULL, victim, TO_NOTVICT);
	    act("You notice $n charging at you again, and you quickly step out of the way.", ch, NULL, victim, TO_VICT);
	    if (victim->fighting == NULL)
	    {
		victim->fighting = ch;
		switch_position(victim, POS_FIGHTING);
	    }
	}
    else
	{
	    if (is_stabilized(*victim))
	    {
		act("You charge into $N, but $E is rooted to the earth and fails to be knocked down.", ch, NULL, victim, TO_CHAR);
		act("$n charges into $N, but fails to knock $M down.", ch, NULL, victim, TO_NOTVICT);
		act("$n charges into you, but rooted to the earth, you stand firm.", ch, NULL, victim, TO_VICT);
	    }
	    else
	    {
		act("You charge into $N and send $M sprawling!", ch, NULL, victim, TO_CHAR);
		act("$n charges into $N and sends $M sprawling!", ch, NULL, victim, TO_NOTVICT);
		act("$n charges into you and sends you sprawling!", ch, NULL, victim, TO_VICT);
		DAZE_STATE(victim, UMAX(victim->daze, (2*PULSE_VIOLENCE)+number_bits(2)));
		WAIT_STATE(victim, UMAX(victim->wait, (2*PULSE_VIOLENCE)+number_bits(2)));
	    }
	    
	    wield = get_eq_char(ch, WEAR_WIELD);
	    
	    if ((ch->fighting == NULL) && !IS_NPC(victim))
	    {
		sprintf(buf, "Help!  %s is attacking me!", PERS(ch, victim));
		do_autoyell(victim ,buf);
	    }

	    dt = TYPE_HIT;
            if ( wield != NULL && wield->item_type == ITEM_WEAPON )
	    {
                dt += wield->value[3];
		dam_type = wield->value[3];
	    }
            else
	    {
                dt += ch->dam_type;
  	    	dam_type = dt - TYPE_HIT;
	    }
	    
            if (dam_type == -1)
                dam_type = DAM_BASH;

            if (wield && obj_is_affected(wield, gsn_heatsink))
                damage_old(ch,ch,number_range(4,8),gsn_heatsink,DAM_COLD,TRUE);
            
	    wrathkyana_combat_effect(ch, victim);

    	    if (is_an_avatar(ch))
    	    {
        	dt = 20;
        	dam_type = TYPE_HIT+DAM_HOLY;
    	    }
 
            /* get the weapon skill */
    	    sn = get_weapon_sn(ch);
            skill = 20 + ((get_weapon_skill(ch,sn,TRUE)/2) + (get_skill(ch, gsn_surpriseattack)/2));

	    /*
	     * Calculate to-hit-armor-class_num-0 versus armor.
	     */
	    if ( IS_NPC(ch) )
	    {
	        thac0_00 = 20;
	        thac0_32 = -4;   /* as good as a thief */
	        if (IS_SET(ch->act,ACT_WARRIOR))
	            thac0_32 = -10;
	        else if (IS_SET(ch->act,ACT_THIEF))
	            thac0_32 = -4;
	    }
	    else
	    {
	        thac0_00 = class_table[ch->class_num].thac0_00;
	        thac0_32 = class_table[ch->class_num].thac0_32;
	    }
	    thac0  = interpolate( ch->level, thac0_00, thac0_32 );
	
	    if (thac0 < 0)
	        thac0 = thac0/2;
	
	    if (thac0 < -5)
	        thac0 = -5 + (thac0 + 5) / 2;
	
	    thac0 -= GET_HITROLL(ch) * skill/100;
	    thac0 += 5 * (100 - skill) / 100;
	
	
	    switch(dam_type)
	    {
	        case(DAM_PIERCE):victim_ac = GET_AC(victim,AC_PIERCE)/10;       break;
	        case(DAM_BASH):  victim_ac = GET_AC(victim,AC_BASH)/10;         break;
	        case(DAM_SLASH): victim_ac = GET_AC(victim,AC_SLASH)/10;        break;
	        default:         victim_ac = GET_AC(victim,AC_EXOTIC)/10;       break;
	    };
	
	    if (victim_ac < -15)
	        victim_ac = (victim_ac + 15) / 5 - 15;
	
	    if (!can_see(ch, victim) && !is_blindfighter(ch,FALSE))
	        victim_ac -= 4;
	
	    if ( victim->position < POS_FIGHTING)
	        victim_ac += 4;
	
	    if (victim->position < POS_RESTING)
	        victim_ac += 6;
	
	    while ( ( diceroll = number_bits( 5 ) ) >= 20 )
	        ;

	    for (paf = victim->affected; paf; paf = paf->next)
	        if (paf->type == gsn_wariness)
	        {
	            if (number_percent() <= paf->modifier)
	            {
		        act("You sense danger, and narrowly evade $N's surprise attack!", victim, NULL, ch, TO_CHAR);
		        diceroll = 0;
	            }
	            break;
	        }

	
	    if (( diceroll == 0
	    || ( diceroll != 19 && diceroll < thac0 - victim_ac ) )
	    && !is_affected(ch, gsn_form_of_the_serpent))
	    {
	        /* Miss. */
	        damage( ch, victim, 0, gsn_surpriseattack, dam_type, TRUE );
	        check_improve(ch,victim,gsn_surpriseattack,FALSE,1);
	        return;
	    }    
	
	    /*
	     * Hit.
	     * Calc damage.
	     */
	    if ( IS_NPC(ch) && (!ch->pIndexData->new_format || wield == NULL))
	        if (!ch->pIndexData->new_format)
	        {
	            dam = number_range( ch->level / 2, ch->level * 3 / 2 );
	            if ( wield != NULL )
	                dam += dam / 2;
	        }
	        else
	            dam = dice(ch->damage[DICE_NUMBER],ch->damage[DICE_TYPE]);
	    else
	    {
	        if (sn != -1)
	            check_improve(ch,victim,sn,TRUE,1);
	        if ( wield != NULL )
	        {
	            if (wield->pIndexData->new_format)
	                dam = dice(wield->value[1],wield->value[2]) * skill/100;
	            else
	                dam = number_range( wield->value[1] * skill/100,
	                                wield->value[2] * skill/100);
	
	                dam = dam * 11/10;
	        }
	        else
	            dam = number_range( 1 + 4 * skill/100, 2 * ch->level/3 * skill/100);    
	    }
	
	    dam += check_extra_damage(ch,dam,wield);
	
	    if ( !IS_AWAKE(victim) )
	        dam *= 2;
	    else if (victim->position < POS_FIGHTING)
	        dam = dam * 5 / 4;
	
	    dam += GET_DAMROLL(ch) * UMIN(100,skill) /100;
	
	    if ( dam <= 0 )
	        dam = 1;
	
	        dam = dam * 6 / 5;
	
	   damage( ch, victim, dam, gsn_surpriseattack, dam_type, TRUE );
           check_improve(ch,victim,gsn_surpriseattack,TRUE,1);

	   af.where	 = TO_AFFECTS;
	   af.type       = gsn_surpriseattack;
	   af.level 	 = ch->level;
	   af.duration  = 2;
	   af.location  = 0;
	   af.modifier  = 0;
	   af.bitvector = 0;
	   affect_to_char( victim, &af );

        }
    }
}

void do_ignite(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj, *arrow;
    char buf[MAX_STRING_LENGTH];

    if (!get_skill(ch, gsn_archery))
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    obj = get_eq_char(ch, WEAR_HOLD);
    if ((obj == NULL) || (obj->item_type != ITEM_BOW))
    {
	send_to_char("You need to be using a bow before you an ignite an arrow.\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("Which type of arrow do you wish to ignite?\n\r", ch);
	return;
    }

    if (ch->nocked)
    {
	send_to_char("You already have an arrow nocked and ready to fire.\n\r", ch);
	return;
    }

    if ((arrow = get_obj_carry(ch, argument, ch)) == NULL)
    {
	send_to_char("You are not carrying any of that arrow type.\n\r", ch);
	return;
    }

    arrow->value[0] = 29;
    arrow->value[4] = gsn_ignite;
    arrow->timer = 2;
    ch->nocked = arrow;
    sprintf(buf, "You ignite and nock %s.", ch->nocked->short_descr);
    act(buf, ch, NULL, NULL, TO_CHAR);
    sprintf(buf, "$n ignites and nocks %s.", ch->nocked->short_descr);
    act(buf, ch, NULL, NULL, TO_ROOM);
    WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));
    return;
}


void do_nock(IPlayer *player, char *argument)
{
    OBJ_DATA *obj;
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *ch = player->get_ch();

    if (!player->get_skill(gsn_archery))
    {
	player->send_message("Huh?\n\r");
	return;
    }
    obj = get_eq_char(ch, WEAR_HOLD);
    if ((obj == NULL) || (obj->item_type != ITEM_BOW))
    {
	send_to_char("You need to be using a bow to nock an arrow.\n\r", ch);
	return;
    }
    if (!str_cmp(argument,"none"))
    {
	if (ch->nocked)
	{
	    act("You relax your draw on $p and put it away.",ch,ch->nocked,NULL,TO_CHAR);
	    act("$n relaxes $s draw on $p and puts it away.",ch,ch->nocked,NULL,TO_ROOM);
	    ch->nocked = NULL;
	    return;
	}
	else
	{
	    send_to_char("You have no arrow nocked.\n\r",ch);
	    return; 
	}
    }
    if (argument[0] == '\0')
    {
	if (ch->nocked)
	{
	    sprintf(buf, "You are no longer prepared to fire %s.", ch->nocked->short_descr);
            REMOVE_BIT(ch->nocked->extra_flags[0], ITEM_WIZI);
	    ch->nocked = NULL;
	    send_to_char(buf, ch);
	    return;
	}
	else
	{
	    send_to_char("What arrow do you wish to nock?\n\r", ch);
	    return;
	}
    }

    if ((obj = get_obj_carry(ch, argument, ch)) == NULL)
    {
	send_to_char("You are not carrying that arrow.\n\r", ch);
	return;
    }  
    
    if (obj->item_type != ITEM_ARROW)
    {
	send_to_char("You may only nock arrows.\n\r", ch);
	return;
    }
    
    if (ch->nocked)
    {
	act("You relax your draw on $p and put it away.",ch,ch->nocked,NULL,TO_CHAR);
	act("$n relaxes $s draw on $p and puts it away.",ch,ch->nocked,NULL,TO_ROOM);
    }
    ch->nocked = obj;
    sprintf(buf, "You nock and prepare to fire %s.\n\r", obj->short_descr);
    send_to_char(buf, ch);
    sprintf(buf, "$n nocks and prepares to fire %s.",obj->short_descr);
    act(buf,ch,NULL,NULL,TO_ROOM);
    return;
}

void old_do_nock(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj;
    char buf[MAX_STRING_LENGTH];

    if (!get_skill(ch, gsn_archery))
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }
    obj = get_eq_char(ch, WEAR_HOLD);
    if ((obj == NULL) || (obj->item_type != ITEM_BOW))
    {
	send_to_char("You need to be using a bow to nock an arrow.\n\r", ch);
	return;
    }
    if (!str_cmp(argument,"none"))
    {
	if (ch->nocked)
	{
	    act("You relax your draw on $p and put it away.",ch,ch->nocked,NULL,TO_CHAR);
	    act("$n relaxes $s draw on $p and puts it away.",ch,ch->nocked,NULL,TO_ROOM);
	    ch->nocked = NULL;
	    return;
	}
	else
	{
	    send_to_char("You have no arrow nocked.\n\r",ch);
	    return; 
	}
    }
    if (argument[0] == '\0')
    {
	if (ch->nocked)
	{
	    sprintf(buf, "You are no longer prepared to fire %s.", ch->nocked->short_descr);
            REMOVE_BIT(ch->nocked->extra_flags[0], ITEM_WIZI);
	    ch->nocked = NULL;
	    send_to_char(buf, ch);
	    return;
	}
	else
	{
	    send_to_char("What arrow do you wish to nock?\n\r", ch);
	    return;
	}
    }

    if ((obj = get_obj_carry(ch, argument, ch)) == NULL)
    {
	send_to_char("You are not carrying that arrow.\n\r", ch);
	return;
    }  
    
    if (obj->item_type != ITEM_ARROW)
    {
	send_to_char("You may only nock arrows.\n\r", ch);
	return;
    }
    
    if (ch->nocked)
    {
	act("You relax your draw on $p and put it away.",ch,ch->nocked,NULL,TO_CHAR);
	act("$n relaxes $s draw on $p and puts it away.",ch,ch->nocked,NULL,TO_ROOM);
    }
    ch->nocked = obj;
    sprintf(buf, "You nock and prepare to fire %s.\n\r", obj->short_descr);
    send_to_char(buf, ch);
    sprintf(buf, "$n nocks and prepares to fire %s.",obj->short_descr);
    act(buf,ch,NULL,NULL,TO_ROOM);
    return;
}

static bool checkToHitDiceroll(CHAR_DATA * ch, CHAR_DATA * victim, int dam_type, int hitmod, int skill)
{
    int thac0_00;
    int thac0_32;
    
    // Generate base thac
    if (IS_NPC(ch))
    {
        thac0_00 = 20;
        thac0_32 = -4;   /* as good as a thief */
        if (IS_SET(ch->act,ACT_WARRIOR)) thac0_32 = -10;
        else if (IS_SET(ch->act,ACT_THIEF)) thac0_32 = -4;
    }
    else
    {
        thac0_00 = class_table[ch->class_num].thac0_00;
        thac0_32 = class_table[ch->class_num].thac0_32;
    }

    int thac0 = interpolate(ch->level, thac0_00, thac0_32);
    if (thac0 < 0)
        thac0 = thac0 / 2;

    if (thac0 < -5)
        thac0 = -5 + (thac0 + 5) / 2;

    thac0 -= ((GET_HITROLL(ch) + hitmod) * skill) / 100;
    thac0 += (5 * (100 - skill)) / 100;

    // Calculate victim AC
    int victim_ac;
    switch(dam_type)
    {
        case(DAM_PIERCE):victim_ac = GET_AC(victim,AC_PIERCE)/10;       break;
        case(DAM_BASH):  victim_ac = GET_AC(victim,AC_BASH)/10;         break;
        case(DAM_SLASH): victim_ac = GET_AC(victim,AC_SLASH)/10;        break;
        default:         victim_ac = GET_AC(victim,AC_EXOTIC)/10;       break;
    };

    if (victim_ac < -15)
        victim_ac = (victim_ac + 15) / 5 - 15;

    if (victim->position < POS_FIGHTING)
        victim_ac += 4;

    if (victim->position < POS_RESTING)
        victim_ac += 6;

    // Perform diceroll
    int diceroll(number_range(0, 19));
    if (diceroll == 0) return false;
    if (diceroll == 19) return true;
    return (diceroll >= (thac0 - victim_ac));
}

static const int ArrowBlockFlags(EX_CLOSED|EX_WALLED|EX_ICEWALL|EX_WALLOFFIRE);

static CHAR_DATA * find_archery_target_in_direction(CHAR_DATA & ch, const char * victimName, int range, Direction::Value direction)
{
    // Direction was specified, so run down the rooms in that direction, skipping this one
    ROOM_INDEX_DATA * room(ch.in_room);
    for (int i(0); i < range; ++i)
    {
        // Advance to the next room
        room = Direction::Adjacent(*room, direction, ArrowBlockFlags | EX_SECRET);
        if (room == NULL || room_is_affected(room, gsn_smoke))
            break;

        // Look for the target here
        CHAR_DATA * victim(get_char_room(&ch, room, const_cast<char*>(victimName)));
        if (victim != NULL)
            return victim;
    }

    // Did not find target
    return NULL;
}

static CHAR_DATA * find_archery_target(CHAR_DATA & ch, const char * victimName, int range, Direction::Value direction, bool canArcshot, bool & didArcshot)
{
    // First try without arcshot
    if (direction == Direction::Max)
    {
        // No direction specified, so check this room
        CHAR_DATA * victim(get_char_room(&ch, const_cast<char*>(victimName)));
        if (victim != NULL)
            return victim;

        // Check each cardinal direction unless arcshot will come into play soon enough anyway
        if (!canArcshot)
        {
            for (unsigned int i(0); i < Direction::Max; ++i)
            {
                victim = find_archery_target_in_direction(ch, victimName, range, static_cast<Direction::Value>(i));
                if (victim != NULL)
                    return victim;
            }
        }
    }
    else
    {
        // Check the specified direction
        CHAR_DATA * victim(find_archery_target_in_direction(ch, victimName, range, direction));
        if (victim != NULL)
            return victim;
    }

    // Did not find victim; check for arc shot
    if (!canArcshot)
        return NULL;

    // Now scan everywhere in the radius of the range
    typedef std::set<ROOM_INDEX_DATA*> RoomSet;
    RoomSet checkedRooms;
    RoomSet rooms0;
    RoomSet rooms1;
    RoomSet * currRooms(&rooms0);
    RoomSet * nextRooms(&rooms1);
    currRooms->insert(ch.in_room);
    
    for (int i(0); i <= range; ++i)
    {
        // Check all the current rooms
        for (RoomSet::iterator iter(currRooms->begin()); iter != currRooms->end(); ++iter)
        {
            // Make sure this room has not already been checked
            if (checkedRooms.find(*iter) != checkedRooms.end())
                continue;

            // Check this room
            CHAR_DATA * victim(get_char_room(&ch, *iter, const_cast<char*>(victimName)));
            if (victim != NULL)
            {
                didArcshot = true;
                return victim;
            }

            // Prepare the next round of iteration
            checkedRooms.insert(*iter);
            for (unsigned int j(0); j < Direction::Max; ++j)
            {
                // Check for the next room; note that visibility effects like secret and smoke don't matter to a magical arc shot
                ROOM_INDEX_DATA * nextRoom(Direction::Adjacent(**iter, static_cast<Direction::Value>(j), ArrowBlockFlags));
                if (nextRoom != NULL)
                    nextRooms->insert(nextRoom);
            }
        }

        // Set up for the next step in the radius
        std::swap(currRooms, nextRooms);
        nextRooms->clear();
    }

    // Did not find a valid target
    return NULL;
}

void do_shoot(CHAR_DATA *ch, char *argument)
{
    // Check for skill
    int baseSkill(get_skill(ch, gsn_archery));
    if (baseSkill <= 0) 
    {
        send_to_char("Huh?\n", ch);
        return;
    }

    // Check for bow
    OBJ_DATA * bow(get_eq_char(ch, WEAR_HOLD));
    if (bow == NULL || bow->item_type != ITEM_BOW)
    {   
        send_to_char("You must be holding a bow to shoot.\n", ch);
    	return;
    }

    // Get the target
    char arg[MAX_INPUT_LENGTH];
    argument = one_argument(argument, arg);
    if (arg[0] == '\0')
    {
    	send_to_char("Shoot whom?\n", ch);
	    return;
    }

    // Check for fighting
    if (ch->fighting != NULL)
    {
    	send_to_char("You can't shoot your bow while fighting.\n", ch);
	    return;
    }

    // Check for nocked
    if (ch->nocked == NULL)
    {
    	send_to_char("You must have an arrow nocked to shoot someone.\n",ch);
	    return;
    }

    // Check for (optional) direction
    Direction::Value direction(Direction::ValueFor(argument));
    if (argument[0] != '\0' && direction == Direction::Max)
    {
        send_to_char("That's not a valid direction.\n", ch);
        return;
    }

    // Check for room effects
    if (ch->in_room == NULL)
    {
        send_to_char("You cannot shoot here.\n", ch);
        return;
    }

    if (area_is_affected(ch->in_room->area, gsn_icestorm))
    {
	    send_to_char("You can't shoot through the raging ice storm.\n", ch);
    	return;
    }

    if (room_is_affected(ch->in_room, gsn_smoke))
    {
    	send_to_char("The smoke in the room prevents you from aiming properly.\n", ch);
	    return;
    }
    
    if (ch->in_room->sector_type == SECT_UNDERWATER && !is_affected(ch,gsn_aquamove))
    {
        if (number_percent() <= get_skill(ch, gsn_waveborne))
            check_improve(ch, NULL, gsn_waveborne, true, 4);
        else
        {
            check_improve(ch, NULL, gsn_waveborne, false, 4);
        	send_to_char("You can't shoot a bow underwater.\n",ch);
	        return;
        }
    }

    // Initialize modifiers
    int hitMod(0);
    int damMod(0);
    int range(0);

    // Adjust for the nocked arrow, including its index
    for (AFFECT_DATA * paf(ch->nocked->affected); paf != NULL; paf = paf->next)
    {
        switch (paf->location)
        {
            case APPLY_HITROLL: hitMod += paf->modifier; break;
            case APPLY_DAMROLL: damMod += paf->modifier; break;
            case APPLY_RANGE:   range += paf->modifier; break;
        }
    }

    for (AFFECT_DATA * paf(ch->nocked->pIndexData->affected); paf != NULL; paf = paf->next)
    {
        switch (paf->location)
        {
            case APPLY_HITROLL: hitMod += paf->modifier; break;
            case APPLY_DAMROLL: damMod += paf->modifier; break;
            case APPLY_RANGE:   range += paf->modifier; break;
        }
    }

    // Adjust for the bow's range
    for (AFFECT_DATA * paf(bow->pIndexData->affected); paf != NULL; paf = paf->next)
    {
	    if (paf->location == APPLY_RANGE)
	        range += paf->modifier;
    }

    // Now find the target
    bool canArcshot(number_percent() <= get_skill(ch, gsn_arcshot));
    bool didArcshot(false);
    CHAR_DATA * victim(find_archery_target(*ch, arg, range, direction, canArcshot, didArcshot));
    if (victim == NULL)
    {
        send_to_char("You cannot find such a target in range.\n", ch);
        return;
    }

    // Verify the target
    if (is_safe(ch, victim))
    	return;

    // Check for sufficient mana
    int manaCost(skill_table[(didArcshot ? gsn_arcshot : gsn_archery)].min_mana);
    if (ch->mana < manaCost)
    {
        send_to_char("You are too tired to aim properly.\n", ch);
        return;
    }

    // Handle shooting echoes
    if (ch->in_room == victim->in_room)
    {
        // Same room, simple echo
        act("You fire an arrow at $N.", ch, NULL, victim, TO_CHAR);
        act("$n fires an arrow at $N.", ch, NULL, victim, TO_NOTVICT);
        act("$n fires an arrow at you.", ch, NULL, victim, TO_VICT);
    }
    else
    {
        // Not the same room, so there must be a valid path
        RoomPath path(*ch->in_room, *victim->in_room, ch, range, ArrowBlockFlags);
        if (!path.Exists() || path.StepCount() == 0)
        {
            bug("Path does not exist for arrow tracking despite found target not being in same room as shooter.", 0);
            send_to_char("An error has occurred, please contact the gods.\n", ch);
            return;
        }
    
        // Initial echo for the current room
        std::ostringstream mess;
        mess << "$n fires an arrow " << Direction::NameFor(static_cast<Direction::Value>(path.StepDirection(0))) << ".";
        act(mess.str().c_str(), ch, NULL, NULL, TO_ROOM);

        mess.str("");
        mess << "You fire an arrow " << Direction::NameFor(static_cast<Direction::Value>(path.StepDirection(0))) << ".\n";
        send_to_char(mess.str().c_str(), ch);

        // Now echo for each intermediate room, but not the final one
        ROOM_INDEX_DATA * room(ch->in_room);
        for (unsigned int i(1); i + 1 < path.StepCount(); ++i)
        {
            // Generate the echo
            Direction::Value prevDirection(static_cast<Direction::Value>(path.StepDirection(i - 1)));
            const char * targetDirection(Direction::NameFor(static_cast<Direction::Value>(path.StepDirection(i))));
            std::ostringstream mess;
            if (path.StepDirection(i) == path.StepDirection(i - 1))
                mess << "An arrow streaks in, heading " << targetDirection << "wards.";
            else
            {
                mess << "An arrow streaks in " << Direction::SourceNameFor(Direction::ReverseOf(prevDirection));
                mess << ", then turns sharply " << targetDirection << "wards!";
            }

            // Load up the room in question
            ROOM_INDEX_DATA * nextRoom(Direction::Adjacent(*room, prevDirection, ArrowBlockFlags));
            if (nextRoom == NULL)
            {
                bug("NULL room returned during arrow targeting after securing path.", 0);
                send_to_char("An error has occurred, please contact the gods.\n", ch);
                return;
            }
        
            // Add tracks and echos
            add_tracks(nextRoom, ch, Direction::ReverseOf(prevDirection));
            room = nextRoom;
            act(mess.str().c_str(), room->people, NULL, NULL, TO_ALL);
        }

        // Now prepare echo and tracks for the final room
        Direction::Value prevDirection(static_cast<Direction::Value>(path.StepDirection(path.StepCount() - 1)));
        add_tracks(victim->in_room, ch, Direction::ReverseOf(prevDirection));

        mess.str("");
        mess << "An arrow flies in " << Direction::SourceNameFor(Direction::ReverseOf(prevDirection)) << ".";
        act(mess.str().c_str(), victim->in_room->people, NULL, NULL, TO_ALL);
    }

    // Handle arcshot improvement
    if (didArcshot)
        check_improve(ch, victim, gsn_arcshot, true, 2);

    // Check for reflective aura
    if (is_affected(victim, gsn_reflectiveaura))
    {
        act("The arrow wraps around $N, and flies back towards you!", ch, NULL, victim, TO_CHAR);
        act("Flows of air redirect the arrow, sending it back at $n!", ch, NULL, victim, TO_VICT);
        act("The arrow wraps around $N, flying back towards $n!", ch, NULL, victim, TO_VICTROOM);
        victim = ch;
    }

	// Check for missile attraction
    if (!is_affected(victim, gsn_missileattraction))
    {
        std::vector<CHAR_DATA*> targets;
        for (CHAR_DATA * vch(victim->in_room->people); vch != NULL; vch = vch->next_in_room)
        {
            if (is_affected(vch, gsn_missileattraction))
                targets.push_back(vch);
        }

        if (!targets.empty())
        {
            victim = targets[number_range(0, targets.size() - 1)];
            act("The arrow veers off its course and flies towards $N!", ch, NULL, victim, TO_CHAR);
            act("The arrow veers off its course and flies towards $N!", ch, NULL, victim, TO_NOTVICT);
            act("The arrow veers off its course and flies towards you!", ch, NULL, victim, TO_VICT);
        }
    }

    // Save off the arrow object for later use
    OBJ_DATA * arrow(ch->nocked);
    ch->nocked = NULL;
    int skill(baseSkill + 20);

    // Apply lag to the shooter, reducing it for trueshot
    int lagBeats(PULSE_VIOLENCE * 2);
    int trueshotSkill(get_skill(ch, gsn_trueshot));
    if (number_percent() <= trueshotSkill)
    {
        lagBeats -= (PULSE_VIOLENCE * trueshotSkill) / 100;
        check_improve(ch, victim, gsn_trueshot, 1, true);
    }
	WAIT_STATE(ch, UMAX(ch->wait, lagBeats));

    // Charge mana as well
    expend_mana(ch, manaCost);

    // Check for missing
    int dam_type(arrow->value[0]);
    if (!checkToHitDiceroll(ch, victim, dam_type, hitMod, skill))
    {
        // Miss; check for hitting another victim instead
        bool newTargetFound(false);
        for (CHAR_DATA * vch(victim->in_room->people); vch != NULL; vch = vch->next_in_room)
        {
            if ((!IS_NPC(vch) || !IS_AFFECTED(vch, AFF_WIZI)) && number_percent() > (vch->size * 4))
            {
                victim = vch;
                newTargetFound = true;

                act("The arrow misses, heading straight for $N instead!", ch, NULL, victim, TO_CHAR);
                act("The arrow misses, heading straight for $N instead!", ch, NULL, victim, TO_NOTVICT);
                act("The arrow misses, heading straight for you instead!", ch, NULL, victim, TO_VICT);
                break;
            }
        }

    	if (!newTargetFound)
        {
            // Missed the original target and did not hit anyone else by accident
            global_bool_check_avoid = false;
            global_damage_from = "The arrow";
            damage_from_obj(ch, victim, arrow, 0, (arrow->value[0] + TYPE_HIT), dam_type, TRUE);
            global_damage_from = NULL;
            global_bool_check_avoid = true;
            obj_from_char(arrow);
            obj_to_room(arrow, victim->in_room);
            check_improve(ch, victim, gsn_archery, false, 1);
            return;
        }
    }

    // Check for automatic dodging
    if (IS_NPC(victim)
    && ((IS_SET(victim->act, ACT_SENTINEL)) || (IS_SET(victim->act, ACT_NOTRACK))
    || (IS_SET(victim->act, ACT_STAY_AREA) && (victim->in_room->area != ch->in_room->area)))
    && (victim->hit < victim->max_hit))
    {
        std::ostringstream mess;
        mess << "$N alertly dodges " << arrow->short_descr << ".";
        act(mess.str().c_str(), ch, NULL, victim, TO_NOTVICT);
        act(mess.str().c_str(), ch, NULL, victim, TO_CHAR);

        mess.str("");
        mess << "You alertly dodge " << arrow->short_descr << ".";
        act(mess.str().c_str(), ch, NULL, victim, TO_VICT);

        obj_from_char(arrow);
        obj_to_room(arrow, victim->in_room);
        return;
    }
    
    // Did not dodge, did not miss, check for defensive effects
    check_improve(ch, victim, gsn_archery, true, 1);
    if (is_affected(victim, gsn_diamondskin) && IS_SET(victim->imm_flags, IMM_WEAPON) && dam_type <= DAM_SLASH)
    {
        act("Your arrow shatters against $N's diamond skin.", ch, NULL, victim, TO_CHAR);
        act("The arrow shatters against your diamond skin.", ch, NULL, victim, TO_VICT);
        act("The arrow shatters against $n's diamond skin.", victim, NULL, NULL, TO_ROOM);
        extract_obj(arrow);
        return;
    }
    
    if (is_affected(victim, gsn_protnormalmissiles) && !IS_SET(arrow->extra_flags[0], ITEM_MAGIC) && number_percent() < victim->level)
    {
        act("Your arrow deflects harmlessly off $N.", ch, NULL, victim, TO_CHAR);
        act("The arrow deflects harmlessly off you.", ch, NULL, victim, TO_VICT);
        act("The arrow bounces harmlesly off $n.", victim, NULL, NULL, TO_ROOM);
        obj_from_char(arrow);
        obj_to_room(arrow, victim->in_room);
        return;
    }

    // Handle screaming arrow
    AFFECT_DATA * screamingArrow(get_obj_affect(arrow, gsn_screamingarrow));
    if (screamingArrow != NULL)
    {
        act("$p disintegrates in flight, emitting an ear-splitting scream!", victim, arrow, NULL, TO_ALL);
        CHAR_DATA * vch_next;
        for (CHAR_DATA * vch(victim->in_room->people); vch != NULL; vch = vch_next)
        {
            // Disqualify safe targets
            vch_next = vch->next_in_room;
            if (is_safe_spell(ch, victim, false))
                continue;

            // Damage target
            int dam(number_range(screamingArrow->level * 3, screamingArrow->level * 9) / 2);
            if (saves_spell(screamingArrow->level, ch, vch, DAM_SOUND))
                dam /= 2;
        
            damage_from_obj(ch, vch, arrow, dam, gsn_screamingarrow, DAM_SOUND, true);
        }

        // Destroy the arrow
        extract_obj(arrow);
        return;
    }

    // Calculate base damage
    int dam;
    if (IS_NPC(ch))
    {
        victim->tracking = ch;
        dam = dice(ch->damage[DICE_NUMBER], ch->damage[DICE_TYPE]);
    }
    else
        dam = (dice(bow->value[1], bow->value[2]) * skill) / 100;
    
    dam += ((GET_DAMROLL(ch) + damMod) * UMIN(100, skill)) /100;
    dam = (dam * 11) / 10;
    dam += check_extra_damage(ch, dam, bow);

    // Check for precision
    if (number_percent() <= get_skill(ch, gsn_precision))
    {
        check_improve(ch, victim, gsn_precision, true, 1);
        dam = (dam * 6) / 5;
    }
    else
        check_improve(ch, victim, gsn_precision, false, 1);

    // Adjust for position
    if (!IS_AWAKE(victim)) dam *= 2;
    else if (victim->position < POS_FIGHTING) dam = (dam * 5) / 4;

    // Adjust for arrow modifier
    dam = dam * arrow->value[1] / 100;
    dam = UMAX(dam, 1);

    // Do the damage
    global_bool_check_avoid = false;
    global_damage_from = "The arrow";
    obj_from_char(arrow);
    int result(damage_from_obj(ch, victim, arrow, dam, (arrow->value[0] + TYPE_HIT), dam_type, true));
    global_damage_from = NULL;
    global_bool_check_avoid = true;

    // Handle casting of spells on the arrow, but only if some damage occurred
    if (result)
    {
        for (int j(2); j < 5; ++j)
        {
            if (arrow->value[j] > 0)
                obj_cast_spell(arrow->value[j], arrow->pIndexData->level, ch, victim, arrow);
        }
    }

    // Check for disjunction
    if (obj_is_affected(arrow, gsn_disjunction))
    {
        if (victim->in_room
        && !IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
        && (!IS_SET(victim->in_room->room_flags, ROOM_NOGATE)
        || (IS_SET(victim->in_room->room_flags, ROOM_NOGATE)
        && victim->clan != victim->in_room->clan))
        && !is_affected(victim, gsn_matrix)
        && !(victim->in_room->area->area_flags & AREA_UNCOMPLETE)
        && (!IS_NPC(victim) || (!IS_SET(victim->act, ACT_NOSUBDUE) 
        && !IS_SET(victim->act, ACT_GUILDGUARD)))
        && !(victim != ch && IS_SET(victim->imm_flags,IMM_SUMMON)))
        {
            // On a save, just move the target around a bit
            stop_fighting_all(victim);
            bool moved(false);
            if (saves_spell(ch->level, ch, victim, DAM_OTHER))
            {
                std::vector<Direction::Value> directions(Direction::ValidDirectionsFrom(*victim->in_room, Direction::Max, EX_CLOSED|EX_WALLED|EX_ICEWALL|EX_WEAVEWALL));
                if (!directions.empty())
                {
                    send_to_char("The arrow's power of disjunction moves you!\n", victim);
                    act("The arrow's power of disjunction moves $N!", ch, NULL, victim, TO_CHAR);
                    {
                        moved = true;
                        move_char(victim, directions[number_range(0, directions.size() - 1)], false);
                        do_look(victim, "auto");
                    }
                }	
            }

            // If not moved, try to teleport
            if (!moved)
            {
                send_to_char("The arrow's power of disjunction disperses you!\n", victim);
                act("The arrow's power of disjunction disperses $N!", ch, NULL, victim, TO_CHAR);
                obj_cast_spell(gsn_teleport, arrow->pIndexData->level, victim, victim, arrow);
            }
        }
    }

    // Check for trueshot to save the arrow in the victim
    if (number_percent() <= trueshotSkill / 2)
    {
        check_improve(ch, victim, gsn_trueshot, 1, true);
        obj_to_char(arrow, victim);
    }
    else
        extract_obj(arrow);

    // Check for trueshot to shoot the target through the leg
    if (number_percent() <= trueshotSkill /2)
    {
        act("You shoot $N through the leg, hampering $S movements!", ch, NULL, victim, TO_CHAR);
        act("$n shoots you through the leg, hampering your movements!", ch, NULL, victim, TO_VICT);
        act("$n shoots $N through the leg, hampering $S movements!", ch, NULL, victim, TO_NOTVICT);

        AFFECT_DATA af = {0};
        af.where = TO_PAFFECTS;
        af.type = gsn_trueshot;
        af.level = ch->level;
        af.duration = 4;
        af.bitvector = AFF_TRUESHOT;
        affect_to_char(victim, &af);
        check_improve(ch, victim, gsn_trueshot, 1, true);
    }
}

void do_submissionhold(CHAR_DATA *ch, char *argument)
{
CHAR_DATA *victim;
ROOM_INDEX_DATA *room;
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
char buf[MAX_STRING_LENGTH];
int chance;
    OBJ_DATA *net = NULL;

    if ((chance = get_skill(ch, gsn_submissionhold)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if ((room = ch->in_room) == NULL)
	return;

    if (is_affected(ch, gsn_submissionhold))
    {
        send_to_char("You're still recovering from your last submission hold.\n\r", ch);
        return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("Put whom in a submission hold?\n\r", ch);
	return;
    }

    if ((victim = get_char_room(ch, argument)) == NULL)
    {
      	send_to_char("You don't see them here.\n\r", ch);
      	return;
    }

    if (ch == victim)
    {
	send_to_char("That would look odd.\n\r", ch);
	return;
    }

    if (is_safe(ch, victim))
	return;

    if (IS_NPC(victim))
    {
	send_to_char("You don't think they will submit.\n\r",ch);
	return;
    }

    if (victim->fighting)
    {
	send_to_char("They are moving too wildly for you to pin down.\n\r",ch);
	return;
    }

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_submissionhold].beats));

    if (number_percent() > chance)
    {
      	if (!IS_AWAKE(victim))
	    switch_position(victim, POS_STANDING);
      	act("$n tries to twist $N's into a submission hold, but $N escapes!", ch, NULL, victim, TO_NOTVICT);
      	act("$n tries to twist you into a submission hold, but you manage to elude it!", ch, NULL, victim, TO_VICT);
      	act("You try to twist $N into a submission hold, but $E eludes you!", ch, NULL, victim, TO_CHAR);
      	sprintf(buf, "Help! %s is trying to pin me to the ground!", PERS(ch, victim));
      	if (!IS_NPC(victim))
            do_autoyell(victim, buf);
      	one_hit(victim,ch,TYPE_UNDEFINED, HIT_PRIMARY, false); 
      	check_improve(ch,victim,gsn_submissionhold,FALSE,1);
      	return;
    }

    if ((net = check_sidestep(ch,victim)) != NULL)
    {
	act("$N sweeps $p into your path, slowing you down!",ch,net,victim,TO_CHAR);
	act("$N sweeps $p into $n's path, slowing $m down!", ch, net, victim, TO_NOTVICT);
	act("You sweep $p into $n's path, slowing $m down!",ch, net, victim, TO_VICT);
	WAIT_STATE(ch, UMAX(ch->wait, 3*PULSE_VIOLENCE));
	extract_obj(net);
	set_fighting(victim, ch);
	return;
    }

    if (IS_AWAKE(victim) && (number_percent() < (get_curr_stat(victim, STAT_DEX) + get_curr_stat(victim, STAT_STR) + 10)))
    {
      	if (!IS_AWAKE(victim))
	    switch_position(victim, POS_STANDING);
      	act("$n tries to twist $N's into a submission hold, but $N escapes!", ch, NULL, victim, TO_NOTVICT);
      	act("$n tries to twist you into a submission hold, but you manage to elude it!", ch, NULL, victim, TO_VICT);
      	act("You try to twist $N into a submission hold, but $E eludes you!", ch, NULL, victim, TO_CHAR);
      	sprintf(buf, "Help! %s is attacking me!", PERS(ch, victim));
      	if (!IS_NPC(victim))
            do_autoyell(victim, buf);
      	one_hit(victim,ch,TYPE_UNDEFINED,HIT_PRIMARY, false);
      	check_improve(ch,victim,gsn_submissionhold,FALSE,1);
      	return;
    }

    switch_position(victim, POS_STANDING);

    act("You seize $N, twisting $S arm into a painful submission hold!", ch, NULL, victim, TO_CHAR);
    act("$n seizes $N, twisting $S arm into a painful submission hold!", ch, NULL, victim, TO_NOTVICT);
    act("$n grabs you, and twists your arm deftly into a painful submission hold!", ch, NULL, victim, TO_VICT);

    sprintf(buf,"holding %s in a submission hold!",victim->name);
    copy_string(ch->pose, buf);
    sprintf(buf,"immobilized by %s's submission hold!",ch->name);
    copy_string(victim->pose, buf);

    af.where        = TO_NAFFECTS;
    af.type 	= gsn_submissionhold;
    af.level        = ch->level;
    af.duration     = 4;
    af.location     = 0;
    af.modifier     = 0;
    af.bitvector    = AFF_SUBMISSION_VICT;
    affect_to_char(victim, &af);

    af.where        = TO_NAFFECTS;
    af.type 	= gsn_submissionhold;
    af.level        = ch->level;
    af.duration     = 6;
    af.location     = 0;
    af.modifier     = victim->id;
    af.bitvector    = AFF_SUBMISSION_CH;
    affect_to_char(ch, &af);
}

void do_circle(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    OBJ_DATA *wield;
    int dam;
    int victim_ac;
    int thac0;
    int thac0_00;
    int thac0_32;
    int diceroll;
    int sn,skill;
    int dam_type;
    bool result;
    int dt;
    CHAR_DATA *vch; 
    CHAR_DATA *vch_next;

    sn = -1;
 
    if (!get_skill(ch, gsn_circle))
    {
        send_to_char("Huh?\n\r", ch );
        return;
    }

    if (argument[0] != '\0')
    {
        if ((victim = get_char_room(ch, argument))==NULL)
        {
	    send_to_char("You don't see them here.\n\r", ch);
            return;
        }
    }
    else
    {
	if (ch->fighting == NULL) 
        { 
            send_to_char("Who did you want to circle behind?\n\r", ch);
            return;
        }	 
        else
            victim = ch->fighting; 
    }

/* If a thief is tanking, he shouldn't be able to circle. -- Jol
 */ 

    for ( vch = ch->in_room->people; vch != NULL; vch = vch_next )
    {
        vch_next        = vch->next_in_room;
        if ( vch->in_room == NULL )
            continue;
        
        if (vch->fighting == ch)
        {   
            send_to_char("You can't circle when someone is fighting you!.\n\r", ch); 
            return; 
        }
    }

    if (victim->fighting == NULL)
    { 
        send_to_char("They're paying too much attention to their surroundings.\n\r",ch);
        return;
    } 

    if (victim->fighting == ch)
    {
	send_to_char("You can't circle around someone who's fighting you.\n\r", ch);
	return;
    }

    if (victim == ch) 
    {
        send_to_char("You can't circle behind yourself.\n\r",ch);
	return;
    }

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;
        
    /*
     * Figure out the type of damage message.
     */
    if (((wield = get_eq_char(ch, WEAR_WIELD)) == NULL) || (wield->value[0] != WEAPON_DAGGER))
	if (((wield = get_eq_char(ch, WEAR_DUAL_WIELD)) == NULL) || (wield->value[0] != WEAPON_DAGGER))
	{
	    send_to_char("You need a dagger to circle stab someone!\n\r", ch);
	    return;
	}

    if (is_safe(ch,victim))
        return;
 
    if ( victim->position == POS_DEAD || ch->in_room != victim->in_room )
        return;
 
    WAIT_STATE( ch, skill_table[gsn_circle].beats );

    if (is_affected(victim,gsn_scouting) && number_bits(1) == 0)
    {
	act("$n tries to circle around $N, but can't escape $N's notice.", ch, NULL, victim, TO_NOTVICT);
	act("You try to circle around $N, but can't escape $N's notice.", ch, NULL, victim, TO_CHAR);
	act("$n tries to circle around you, but can't escape your notice.", ch, NULL, victim, TO_VICT);
	return;
    }
    else
    { 
        act("$n circles around $N.", ch, NULL, victim, TO_NOTVICT);
        act("You circle around $N.", ch, NULL, victim, TO_CHAR);
        act("$n circles around you.", ch, NULL, victim, TO_VICT);
    }
    dt = TYPE_HIT + wield->value[3];
    dam_type = wield->value[3];
 
    if (dam_type == -1)
        dam_type = DAM_PIERCE;
 
    if (wield && obj_is_affected(wield, gsn_heatsink))
        damage_old(ch,ch,number_range(4,8),gsn_heatsink,DAM_COLD,TRUE);

    wrathkyana_combat_effect(ch, victim);

    /* get the weapon skill */
    sn = get_weapon_sn(ch);
    skill = ((get_weapon_skill(ch,sn,TRUE)/2) * get_skill(ch, gsn_circle)/100);
 
    /*
     * Calculate to-hit-armor-class_num-0 versus armor.
     */
    if ( IS_NPC(ch) )
    {
        thac0_00 = 20;
        thac0_32 = -4;   /* as good as a thief */
        if (IS_SET(ch->act,ACT_WARRIOR))
            thac0_32 = -10;
        else if (IS_SET(ch->act,ACT_THIEF))
            thac0_32 = -4;
    }
    else
    {
        thac0_00 = class_table[ch->class_num].thac0_00;
        thac0_32 = class_table[ch->class_num].thac0_32;
    }
    thac0  = interpolate( ch->level, thac0_00, thac0_32 );
 
    if (thac0 < 0)
        thac0 = thac0/2;
 
    if (thac0 < -5)
        thac0 = -5 + (thac0 + 5) / 2;
 
    thac0 -= GET_HITROLL(ch) * skill/100;
    thac0 += 5 * (100 - skill) / 100;
 
    thac0 -= 8; /* hey, its circle */
 
    switch(dam_type)
    {
        case(DAM_PIERCE):victim_ac = GET_AC(victim,AC_PIERCE)/10;       break;
        case(DAM_BASH):  victim_ac = GET_AC(victim,AC_BASH)/10;         break;
        case(DAM_SLASH): victim_ac = GET_AC(victim,AC_SLASH)/10;        break;
        default:         victim_ac = GET_AC(victim,AC_EXOTIC)/10;       break;
    };
 
    if (victim_ac < -15)
        victim_ac = (victim_ac + 15) / 5 - 15;
 
    if ( !can_see( ch, victim ) )
        victim_ac -= 4;
 
    if ( victim->position < POS_FIGHTING)
        victim_ac += 4;
 
    if (victim->position < POS_RESTING)
        victim_ac += 6;
  
    /*
     * The moment of excitement!
     */
    while ( ( diceroll = number_bits( 5 ) ) >= 20 )
        ;
 
     if (( diceroll == 0
    || ( diceroll != 19 && diceroll < thac0 - victim_ac ) )
    && !is_affected(ch, gsn_form_of_the_serpent))
    {
        /* Miss. */
        damage( ch, victim, 0, gsn_circle, dam_type, TRUE );
 
            check_improve(ch,victim,gsn_circle,FALSE,1);
 
        return;
    }
 
    /*
     * Hit.
     * Calc damage.
     */
    if ( IS_NPC(ch) && (!ch->pIndexData->new_format || wield == NULL))
        if (!ch->pIndexData->new_format)
        {
            dam = number_range( ch->level / 2, ch->level * 3 / 2 );
            if ( wield != NULL )
                dam += dam / 2;
        }
        else
            dam = dice(ch->damage[DICE_NUMBER],ch->damage[DICE_TYPE]);
 
    else
    {
        if (sn != -1)
            check_improve(ch,victim,sn,TRUE,5);

        if ( wield != NULL )
        {

	    if (wield->pIndexData->new_format)
                dam = dice(wield->value[1],wield->value[2]) * skill/100*3/2;
            else    
                dam = number_range( wield->value[1] * skill/100,
                                wield->value[2] * skill/100)*3/2;

                dam = dam * 11/10;
        }
        else
            dam = number_range( 1 + 4 * skill/100, 2 * ch->level/3 * skill/100);
    }
 
    /*
     * Bonuses.
     */
    dam += check_extra_damage(ch,dam,wield);

    if ( !IS_AWAKE(victim) )
        dam *= 2;
    else if (victim->position < POS_FIGHTING)
        dam = (dam * 5) / 4;
 
    dam += GET_DAMROLL(ch);

    dam = ch->level > 30 ? round(dam * 3.2) : round(dam * 2.5);
 
    if ( dam <= 0 )
        dam = 1;
 
    result = damage_from_obj( ch, victim, wield, dam, gsn_circle, dam_type, TRUE );

    return;
}

void do_boneshatter( CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *wield, *dual;
    bool arm;  /* if its true, they want an arm, if not, a leg */
    int dam, chance, result;
    int dt;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ((chance = get_skill(ch, gsn_boneshatter)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    chance /= 2;

    argument = one_argument(argument, arg1);

    if (arg1[0] == '\0' || !strcmp(arg1, "arm") || !strcmp(arg1, "leg"))
    {
        if ((victim = ch->fighting) == NULL)
	{
	    send_to_char("Shatter whose bones?\n\r", ch);
	    return;
	}
    }
    else if ((victim = get_char_room(ch, arg1)) == NULL)
    {
	send_to_char("Boneshatter whom?\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	if (!strcmp(arg1, "leg"))
	    arm = FALSE;
	else
	    arm = TRUE;
    }
    else
    {
	if (!strcmp(argument, "leg"))
	    arm = FALSE;
	else
	    arm = TRUE;
    }

    if (is_safe(ch,victim))
        return;
 
    if ((wield = get_eq_char(ch, WEAR_WIELD)) == NULL)
    {
	send_to_char("You must wield a mace or flail to boneshatter.\n\r", ch);
	return;
    }

    if (wield->value[0] != WEAPON_MACE && wield->value[0] != WEAPON_FLAIL)
	if ((wield = get_eq_char(ch, WEAR_DUAL_WIELD)) == NULL 
	  || (wield->value[0] != WEAPON_MACE && wield->value[0] != WEAPON_FLAIL))
	{
	    send_to_char("You must wield a mace or flail to boneshatter.\n\r", ch);
	    return;
	}

    if (!check_reach(ch, victim, REACH_HARD, PULSE_VIOLENCE))
	return;

    if (IS_AFFECTED(victim, AFF_HASTE) || (IS_NPC(victim) && IS_SET(victim->off_flags, OFF_FAST)))
	chance = chance * 3 / 4;

    chance += (ch->level - victim->level);

    if (get_eq_char(victim, WEAR_SHIELD) != NULL)
	chance = round(chance * 0.9);
    else
	chance = round(chance * 1.3);

    chance = URANGE(5, chance, 95);

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_boneshatter].beats));

    if (!IS_NPC(victim))
	if (ch->fighting == NULL || victim->fighting != ch)
	{
	    sprintf(buf, "Help! %s is smashing me!", PERS(ch, victim));
	    do_autoyell(victim, buf);
	}

    check_killer(ch, victim);
    if ((result = number_percent()) < chance)
    {
	act("$n's crushing blow shatters $N's limb!", ch, wield, victim, TO_NOTVICT);
	act("You smash $N with $p with colossal strength!", ch, wield, victim, TO_CHAR);
	act("$n's crushing blow shatters your limb!", ch, wield, victim, TO_VICT);
        check_improve(ch,victim,gsn_boneshatter,TRUE,1);
    }
    else
    {
	act("$n tries to crush $N with $p, but $N avoids the blow!", ch, wield, victim, TO_NOTVICT);
	act("You try to crush $N with $p, but $N avoids the blow!", ch, wield, victim, TO_CHAR);
	act("$n tries to crush you with $p, but you avoid the blow!", ch, wield, victim, TO_VICT);
        check_improve(ch,victim,gsn_boneshatter,FALSE,1);
	return;
    }

    if ( wield != NULL && wield->item_type == ITEM_WEAPON )
	dt = wield->value[3];
    else
	dt = ch->dam_type;

    dam = dice(wield->value[1], wield->value[2]) + GET_DAMROLL(ch)/2;
    if (!IS_PAFFECTED(ch,AFF_TWOHAND))
	dam = round((dam*4)/5);

    damage_from_obj( ch, victim,wield, dam, gsn_boneshatter, dt, TRUE );

    if (arm)
    {
	if (IS_NAFFECTED(victim, AFF_ARMSHATTER))
	    return;
	act("$n's arm is badly crushed by the blow!", victim, NULL, NULL, TO_ROOM);
	act("Your arm is badly crushed by the blow!", victim, NULL, NULL, TO_CHAR);

        af.where        = TO_NAFFECTS;
        af.type         = gsn_boneshatter;
        af.level        = ch->level;
        af.duration     = 8;
        af.location     = APPLY_STR;
        af.modifier     = -2;
        af.bitvector    = AFF_ARMSHATTER;
        affect_to_char(victim, &af);

        af.location     = APPLY_DEX;
        af.modifier     = -1;
        affect_to_char(victim, &af);

	if ((dual = get_eq_char(victim, WEAR_DUAL_WIELD)) != NULL)
	{
	    act("$n drops $p from $s mangled arm.", victim, dual, NULL, TO_ROOM);
	    act("You drop $p from your mangled arm.", victim, dual, NULL, TO_CHAR);
	    if (!IS_OBJ_STAT(dual,ITEM_NOREMOVE))
	    {
	        unequip_char(victim, dual);
	        oprog_remove_trigger(dual);
	    }

	    if (!IS_OBJ_STAT(dual,ITEM_NODROP)
	      && !IS_OBJ_STAT(dual,ITEM_INVENTORY))
	    {
		obj_from_char(dual);
		obj_to_room(dual, victim->in_room);
	    }
	}

	return;
    }
    else
    {
	if (IS_NAFFECTED(victim, AFF_LEGSHATTER))
	   return;
	act("$n crumbles as the blow smashes $s leg!", victim, NULL, NULL, TO_ROOM);
	act("You crumble as the blow smashes your leg!", victim, NULL, NULL, TO_CHAR);

        af.where        = TO_NAFFECTS;
        af.type         = gsn_boneshatter;
        af.level        = ch->level;
        af.duration     = 8;
        af.location     = APPLY_DEX;
        af.bitvector    = AFF_LEGSHATTER;
        af.modifier     = -4;
        affect_to_char(victim, &af);
	return;
    }
	
    return;
}




void do_cover(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    int chance;

    if ((chance = get_skill(ch, gsn_cover)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (ch->guarding != NULL)
    {
	act("$n stops protecting $N.", ch, NULL, ch->guarding, TO_NOTVICT);
	act("You stop protecting $N.", ch, NULL, ch->guarding, TO_CHAR);
	if (ch->guarding->valid && ch->guarding->in_room && ch->guarding->in_room == ch->in_room)
	  act("$n stops protecting you.", ch, NULL, ch->guarding, TO_VICT);
	ch->guarding = NULL;
    }

    if (argument[0] == '\0')
	return;

    if ((victim = get_char_room(ch, argument)) == NULL)
	{
	send_to_char("You don't see them here.\n\r", ch);
	return;
	}

    if (victim == ch)
	{
	send_to_char("Protect yourself? You already try to do that.\n", ch);
	return;
	}

    if (chance < number_percent())
    {
	act("$n tries to protect $N, but can't concentrate enough.", ch, NULL, victim, TO_NOTVICT);
	act("$n tries to protect you, but can't concentrate enough.", ch, NULL, victim, TO_VICT);
	act("You try to protect $N, but you can't concentrate enough.", ch, NULL, victim, TO_CHAR);
        check_improve(ch,victim,gsn_cover,FALSE,2);
	return;
    }

	act("$n begins protecting $N.", ch, NULL, victim, TO_NOTVICT);
	act("$n begins protecting you.", ch, NULL, victim, TO_VICT);
	act("You begin protecting $N.", ch, NULL, victim, TO_CHAR);
        check_improve(ch,victim,gsn_cover,TRUE,2);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_cover].beats));

	ch->guarding = victim;

return;
}


void do_fury(CHAR_DATA *ch, char *argument)
{
    int chance;
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;

    if ((chance = get_skill(ch, gsn_fury)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (ch->fighting == NULL)
    {
	send_to_char("You have no one to be furious with.\n\r", ch);
	return;
    }

    if (is_affected(ch,gsn_fury))
    {
	if (IS_NAFFECTED(ch,AFF_FURY))
	{
	    send_to_char("You're already being furious.\n\r",ch);
	    return;
	}
	else
	{
	    send_to_char("You're too exhausted from your last bout of furiousness.\n\r",ch);
	    return;
	}
    }

    if (ch->move < skill_table[gsn_fury].min_mana)
    {
	  send_to_char("You're too tired to summon up your fury.\n\r", ch);
	  return;
    }

    if (chance < number_percent())
    {
	act("$n tries to work $mself into a fury, but fails.",
		ch, NULL, NULL, TO_ROOM);
	act("You try to work yourself into a fury, but fail.",
		ch, NULL, NULL, TO_CHAR);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_fury].beats));
        check_improve(ch,NULL,gsn_fury,FALSE,1);
	return;
    }

    act("$n snarls in anger, and lashes out with focused fury!",
	ch, NULL, NULL, TO_ROOM);
    act("You snarl in anger, and lash out with focused fury!",
	ch, NULL, NULL, TO_CHAR);

    af.where        = TO_NAFFECTS;
    af.type         = gsn_fury;
    af.level        = ch->level;
    af.duration     = 8;
    af.location     = 0;
    af.modifier     = ch->level/5;
    af.bitvector    = AFF_FURY;
    affect_to_char(ch, &af);

    ch->move -= skill_table[gsn_fury].min_mana;
    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_fury].beats));
    check_improve(ch,NULL,gsn_fury,TRUE,2);
}


void do_rage(CHAR_DATA *ch, char *argument)
{
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
int chance;

    if ((chance = get_skill(ch, gsn_rage)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (ch->fighting == NULL)
    {
	send_to_char("You cannot be consumed with rage if you are not fighting.\n\r", ch);
	return;
    }

    if (is_affected(ch, gsn_rage))
    {
	send_to_char("You are already enraged.\n\r", ch);
	return;
    }

    if (ch->move < skill_table[gsn_rage].min_mana)
    {
	send_to_char("You are too tired to become enraged.\n\r",ch);
	return;
    }

    if (chance < number_percent())
    {
	act("$n is overtaken with rage, but cannot put aside $s fear.",
		ch, NULL, NULL, TO_ROOM);
	act("You are overtaken with rage, but cannot put aside your fear.",
		ch, NULL, NULL, TO_CHAR);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_rage].beats));
        check_improve(ch,NULL,gsn_rage,FALSE,1);
	return;
    }

    ch->move -= skill_table[gsn_rage].min_mana;
    act("$n is overtaken with rage, and abandons caution for anger!",
	ch, NULL, NULL, TO_ROOM);
    act("You are overtaken with rage, and abandon caution for anger!",
	ch, NULL, NULL, TO_CHAR);

    af.where        = TO_NAFFECTS;
    af.type         = gsn_rage;
    af.level        = ch->level;
    af.duration     = 1;
    af.location     = 0;
    af.modifier     = 0;
    af.bitvector    = AFF_RAGE;
    affect_to_char(ch, &af);
    af.duration     = 8;
    af.bitvector    = 0;
    affect_to_char(ch, &af);

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_rage].beats));
    check_improve(ch,NULL,gsn_rage,TRUE,2);	
}

void do_warcry(CHAR_DATA *ch, char *argument)
{
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
CHAR_DATA *vch;
int chance;

    if ((chance = get_skill(ch, gsn_warcry)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (ch->fighting == NULL)
    {
	send_to_char("The best time to yell your warcry is while fighting.\n\r", ch);
	return;
    }

    if (is_affected(ch, gsn_warcry))
    {
	send_to_char("You've already yelled your warcry.\n\r", ch);
	return;
    }

    if (chance < number_percent())
    {
	act("$n tries to cry out $s warcry, but is distracted and fails.", ch, NULL, NULL, TO_ROOM);
	act("You try to cry out your warcry, but you are distracted and fail.", ch, NULL, NULL, TO_CHAR);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_warcry].beats));
        check_improve(ch,NULL,gsn_warcry,FALSE,1);
	return;
    }

    act("$n cries out $s warcry, and attacks more furiously!",
	ch, NULL, NULL, TO_ROOM);
    act("You cry out your warcry, and attack more furiously!",
	ch, NULL, NULL, TO_CHAR);
    if (ch->battlecry != NULL)
	do_yell(ch, ch->battlecry);
    else
	do_yell(ch, "Blood and victory!");

    ch->hit += ch->level * 2;
    ch->hit = UMIN(ch->hit,ch->max_hit);
    one_hit(ch, ch->fighting, TYPE_UNDEFINED,HIT_PRIMARY);
    
    af.where        = TO_NAFFECTS;
    af.type         = gsn_warcry;
    af.level        = ch->level;
    af.duration     = 6;
    af.location     = APPLY_HITROLL;
    af.bitvector    = 0;
    af.modifier     = ch->level/8;
    affect_to_char(ch, &af);

    for (vch = ch->in_room->people;vch != NULL;vch = vch->next_in_room)
	if (vch->fighting == ch)
	{
	    act("$N looks stunned by your warcry!",ch,NULL,vch,TO_CHAR);
	    act("$N looks stunned by $n's warcry!",ch,NULL,vch,TO_NOTVICT);
	    act("You are stunned by $n's warcry!",ch,NULL,vch,TO_VICT);
	    vch->lost_att+=4;
	    WAIT_STATE(vch, UMAX(vch->wait, PULSE_VIOLENCE));
	}

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_warcry].beats));
    check_improve(ch,NULL,gsn_warcry,TRUE,2);
}


void do_rally(CHAR_DATA *ch, char *argument)
{
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
CHAR_DATA *vch;
int chance;

    if ((chance = get_skill(ch, gsn_rally)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (ch->fighting == NULL)
    {
	send_to_char("You have no enemy to rally against.\n\r", ch);
	return;
    }

    if (is_affected(ch, gsn_rally))
    {
	send_to_char("You do not feel ready to start a rally again yet.\n\r", ch);
	return;
    }

    if (chance < number_percent())
    {
	act("$n tries to cry out $s battlecry and rally, but is distracted and fails.",	ch, NULL, NULL, TO_ROOM);
	act("You try to cry out your battlecry and rally, but you are distracted and fail.", ch, NULL, NULL, TO_CHAR);
        check_improve(ch,NULL,gsn_rally,FALSE,1);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_rally].beats));
	return;
    }

    act("$n cries out $s battlecry, and urges everyone into renewed vigor!",
	ch, NULL, NULL, TO_ROOM);
    act("You cry out your battlecry, urging everyone into renewed vigor!",
	ch, NULL, NULL, TO_CHAR);
    if (ch->battlecry != NULL)
	do_yell(ch, ch->battlecry);
    else
	do_yell(ch, "Rally to me! Fight as one!");
    
    af.where        = TO_NAFFECTS;
    af.type         = gsn_rally;
    af.level        = ch->level;
    af.duration     = 6;
    af.location     = 0;
    af.bitvector    = AFF_RALLY;
    af.modifier     = 0;
    affect_to_char(ch, &af);

    for (vch = ch->in_room->people;vch != NULL;vch = vch->next_in_room)
    {
        if (is_same_group(ch, vch))
        {
            one_hit(vch, ch->fighting, TYPE_UNDEFINED, HIT_PRIMARY);
            if(is_an_avatar(vch))
                continue;
            vch->hit = UMIN(vch->max_hit, vch->hit+vch->level);
        }
    }

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_rally].beats));
    check_improve(ch,NULL,gsn_rally,TRUE,1);
}



void do_agility(CHAR_DATA *ch, char *argument)
{
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
int chance;

    if ((chance = get_skill(ch, gsn_agility)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (is_affected(ch, gsn_agility))
    {
	send_to_char("You are already agile and aware.\n\r", ch);
	return;
    }

    if (ch->mana < skill_table[ch->class_num].min_mana)
    {
	send_to_char("You are too tired to enhance your agility.\n\r", ch);
	return;
    }

     expend_mana(ch, skill_table[ch->class_num].min_mana);

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_agility].beats));

    if (chance < number_percent())
    {
	act("$n stretches and flexes, but seems unsatisfied.", ch, NULL, NULL, TO_ROOM);
	act("You stretch and flex, but feel unsatisfied.", ch, NULL, NULL, TO_CHAR);
        check_improve(ch,NULL,gsn_agility,FALSE,1);
	return;
    }

	act("$n stretches and flexes.", ch, NULL, NULL, TO_ROOM);
	act("You stretch and prepare for strenuous situations.", ch, NULL, NULL, TO_CHAR);
        check_improve(ch,NULL,gsn_agility,TRUE,1);


        af.where        = TO_NAFFECTS;
        af.type         = gsn_agility;
        af.level        = 0;
        af.duration     = ch->level/5;
        af.location     = 0;
        af.modifier     = 0;
        af.bitvector    = AFF_AGILITY;
        affect_to_char(ch, &af);

return;
}





void do_readiness(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;
    int chance;

    if ((chance = get_skill(ch, gsn_readiness)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (is_affected(ch, gsn_readiness))
    {
	send_to_char("You are not yet ready to heighten your awareness again.\n\r", ch);
	return;
    }

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_readiness].beats));
    if (chance < number_percent())
    {
	act("$n tries to prepare $mself for combat, but cannot concentrate.", ch, NULL, NULL, TO_ROOM);
	act("You try to prepare yourself for combat, but cannot concentrate.", ch, NULL, NULL, TO_CHAR);
        check_improve(ch,NULL,gsn_readiness,FALSE,1);
	return;
    }

    act("$n prepares $mself for combat.", ch, NULL, NULL, TO_ROOM);
    act("You prepare yourself for combat.", ch, NULL, NULL, TO_CHAR);
    check_improve(ch,NULL,gsn_readiness,TRUE,1);


    af.where        = TO_AFFECTS;
    af.type         = gsn_readiness;
    af.level        = ch->level;
    af.duration     = 8;
    af.location     = APPLY_DAMROLL;
    af.bitvector    = 0;
    af.modifier     = ch->level/8;
    affect_to_char(ch, &af);

    af.location     = APPLY_HITROLL;
    af.modifier     = ch->level/8;
    affect_to_char(ch, &af);

    return;
}

void do_bandage(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *victim;
    int chance;

    if ((chance = get_skill(ch, gsn_bandage)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }
	
    if (is_affected(ch, gsn_ghost))
    {
    	send_to_char("With what, ghost-bandages?\n\r",ch);
	return;
    }
    if (argument[0] != '\0')
    {
	if ((victim = get_char_room(ch, argument)) == NULL)
	{
	    send_to_char("You don't see them here.\n\r", ch);
	    return;
	}
    }
    else
	victim = ch;

    if (is_affected(ch, gsn_bandage))
    {
	send_to_char("You aren't ready to bandage wounds again yet.\n\r", ch);
	return;
    }
    
    if (is_an_avatar(victim))
    {
        act("You cannot apply bandages to $N.",ch, NULL, victim, TO_CHAR);
	    return;
    }

    if (chance < number_percent())
    {
	if (ch == victim)
	{
	    act("You try to bandage your wounds, but fail.", ch, NULL, NULL, TO_CHAR);
	    act("$n tries to bandage $s wounds, but fails.", ch, NULL, NULL, TO_ROOM);
	}
	else
	{
	    act("You try to bandage $N's wounds, but fail.", ch, NULL, victim, TO_CHAR);
	    act("$n tries to bandage your wounds, but fails.", ch, NULL, victim, TO_VICT);
	    act("$n tries to bandage $N's wounds, but fails.", ch, NULL, victim, TO_NOTVICT);
	}
        check_improve(ch,NULL,gsn_bandage,FALSE,1);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_bandage].beats));
	return;
    }

    if (ch == victim)
    {
	act("You quickly apply bandages to your wounds.", ch, NULL, NULL, TO_CHAR);
	act("$n quickly bandages $s wounds.", ch, NULL, NULL, TO_ROOM);
    }
    else
    {
	act("$n quickly bandages your wounds.", ch, NULL, victim, TO_VICT);
	act("You quickly apply bandages to $N's wounds.", ch, NULL, victim, TO_CHAR);
	act("$n quickly applies bandages to $N's wounds.", ch, NULL, victim, TO_NOTVICT);
    }

    victim->hit = UMIN(victim->max_hit, victim->hit + UMAX(number_fuzzy(ch->level * 2),40));

    int duration(10);
    if (number_percent() <= get_skill(ch, gsn_fieldmedicine))
    {
        duration = 5;
        check_improve(ch, NULL, gsn_fieldmedicine, true, 4);
    }
    else
        check_improve(ch, NULL, gsn_fieldmedicine, false, 4);
	
    af.where        = TO_AFFECTS;
    af.type         = gsn_bandage;
    af.level        = ch->level;
    af.duration     = duration;
    af.location     = 0;
    af.bitvector    = 0;
    af.modifier     = 0;
    affect_to_char(ch, &af);
	
    if (IS_AFFECTED(victim, AFF_POISON) && is_affected(victim, gsn_poison) && saves_spell(victim->level, NULL, victim, DAM_POISON))
    {
        act("You feel better!", victim, NULL, NULL, TO_CHAR);
        act("$n looks better!", victim, NULL, NULL, TO_ROOM);
        affect_strip(victim, gsn_poison);
    }

    check_improve(ch,NULL,gsn_bandage,TRUE,1);
    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_bandage].beats));
}

void do_tripwire(CHAR_DATA *ch, char *argument)
{
char buf[MAX_STRING_LENGTH];
int direction;
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
int chance;

    if ((chance = get_skill(ch, gsn_tripwire)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

  if (!ch->in_room)
	return;

  if (is_affected(ch, gsn_tripwire))
	{
	  send_to_char("You have set a tripwire too recently to set another.\n\r", ch);
	  return;
	}

  if (argument[0] == '\0')
	{
	  send_to_char("Guard which direction?\n\r", ch);
	  return;
	}

    if (!str_prefix(argument, "north")) direction = 0;
    else if (!str_prefix(argument, "east")) direction = 1;
    else if (!str_prefix(argument, "south")) direction = 2;
    else if (!str_prefix(argument, "west")) direction = 3;
    else if (!str_prefix(argument, "up")) direction = 4;
    else if (!str_prefix(argument, "down")) direction = 5;
    else {
	   send_to_char("That's not a direction.\n\r", ch);
	   return;
	 }


	if (ch->in_room->exit[direction] == NULL
	|| ch->in_room->exit[direction]->u1.to_room == NULL
	|| ch->in_room->exit[direction]->exit_info & EX_FAKE)
	{
	  send_to_char("You can't set up a tripwire in that direction.\n\r", ch);
	  return;
	}

	if (chance < number_percent())
	{
	  send_to_char("You fail to properly set up your tripwire.\n\r", ch);
          check_improve(ch,NULL,gsn_tripwire,FALSE,1);
	  WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_tripwire].beats));
	  return;
	}
    
        af.where        = TO_AFFECTS;
        af.type         = gsn_tripwire;
        af.level        = ch->level;
        af.duration     = 12;
        af.location     = 0;
        af.modifier     = 0;
        af.bitvector    = 0;
        affect_to_char(ch, &af);

        check_improve(ch,NULL,gsn_tripwire,TRUE,1);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_tripwire].beats));

	switch (direction)
	{
	case 0:
	sprintf(buf, "$n sets up a fine wire on the path to the north.");
	break;
	case 1:
	sprintf(buf, "$n sets up a fine wire on the path to the east.");
	break;
	case 2:
	sprintf(buf, "$n sets up a fine wire on the path to the south.");
	break;
	case 3:
	sprintf(buf, "$n sets up a fine wire on the path to the west.");
	break;
	case 4:
	sprintf(buf, "$n sets up a fine wire on the path up.");
	break;
	case 5:
	sprintf(buf, "$n sets up a fine wire on the path down.");
	break;
	}

	SET_BIT(ch->in_room->exit[direction]->exit_info, EX_TRIPWIRE);
	if (ch->in_room->exit[direction]->u1.to_room->exit[OPPOSITE(direction)] &&
	ch->in_room->exit[direction]->u1.to_room->exit[OPPOSITE(direction)]->u1.to_room == ch->in_room)
	  SET_BIT(ch->in_room->exit[direction]->u1.to_room->exit[OPPOSITE(direction)]->exit_info, EX_TRIPWIRE);

	act("You set up a tripwire to catch the unwary.", ch, NULL, NULL, TO_CHAR);
	act(buf, ch, NULL, NULL, TO_ROOM);

return;
}


void do_guard(CHAR_DATA *ch, char *argument)
{
char buf[MAX_STRING_LENGTH];
int direction;
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
int chance;

    if ((chance = get_skill(ch, gsn_guard)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (argument[0] == '\0')
	{
	if (is_affected(ch, gsn_guard))
		{
		act("$n stops guarding vigilantly.", ch, NULL, NULL, TO_ROOM);
		act("You stop guarding vigilantly.", ch, NULL, NULL, TO_CHAR);
		affect_strip(ch, gsn_guard);
		return;
		}

	send_to_char("Guard which direction?\n\r", ch);
	return;
	}

    if (!str_prefix(argument, "north")) direction = 0;
    else if (!str_prefix(argument, "east")) direction = 1;
    else if (!str_prefix(argument, "south")) direction = 2;
    else if (!str_prefix(argument, "west")) direction = 3;
    else if (!str_prefix(argument, "up")) direction = 4;
    else if (!str_prefix(argument, "down")) direction = 5;
    else {
	   send_to_char("That's not a direction.\n\r", ch);
	   return;
	 }

	if (chance < number_percent())
	{
	send_to_char("You fail to properly set up your guard.\n\r", ch);
        check_improve(ch,NULL,gsn_guard,FALSE,1);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_guard].beats));
	return;
	}
    
        af.where        = TO_NAFFECTS;
        af.type         = gsn_guard;
        af.level        = ch->level;
        af.duration     = -1;
        af.location     = APPLY_HIDE;
        af.bitvector    = AFF_GUARDING;
        af.modifier     = direction;
        affect_to_char(ch, &af);

        check_improve(ch,NULL,gsn_guard,TRUE,1);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_guard].beats));

	switch (direction)
	{
	case 0:
	sprintf(buf, "$n begins to vigilantly guard the path to the north.");
	break;
	case 1:
	sprintf(buf, "$n begins to vigilantly guard the path to the east.");
	break;
	case 2:
	sprintf(buf, "$n begins to vigilantly guard the path to the south.");
	break;
	case 3:
	sprintf(buf, "$n begins to vigilantly guard the path to the west.");
	break;
	case 4:
	sprintf(buf, "$n begins to vigilantly guard the path up.");
	break;
	case 5:
	sprintf(buf, "$n begins to vigilantly guard the path down.");
	break;
	}

	act("You set up guard and remain vigilant.", ch, NULL, NULL, TO_CHAR);
	act(buf, ch, NULL, NULL, TO_ROOM);

return;
}


void do_drive(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    EXIT_DATA *pexit;
    int direction, chance;

    if ((chance = get_skill(ch, gsn_drive)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("Drive who which direction?\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg);
 
    if (argument[0] == '\0')
    {
        if ((victim = ch->fighting) == NULL)
	{
	    send_to_char("Drive who which direction?\n\r", ch);
	    return;
	}
        else
	{
    	    if (!str_prefix(arg, "north")) direction = 0;
    	    else if (!str_prefix(arg, "east")) direction = 1;
    	    else if (!str_prefix(arg, "south")) direction = 2;
    	    else if (!str_prefix(arg, "west")) direction = 3;
    	    else if (!str_prefix(arg, "up")) direction = 4;
    	    else if (!str_prefix(arg, "down")) direction = 5;
	    else
	    {
	        send_to_char("That's not a direction.\n\r", ch);
	        return;
	    }
	}
    }
    else
    {
	if ((victim = get_char_room(ch, arg)) == NULL)
	{
	    send_to_char("You don't see them here.\n\r", ch);
	    return;
	}
	
	if (victim == ch)
	{
	    send_to_char("You can only drive yourself insane.\n\r",ch);
	    return;
	}

    	if (!str_prefix(argument, "north")) direction = 0;
    	else if (!str_prefix(argument, "east")) direction = 1;
    	else if (!str_prefix(argument, "south")) direction = 2;
    	else if (!str_prefix(argument, "west")) direction = 3;
    	else if (!str_prefix(argument, "up")) direction = 4;
    	else if (!str_prefix(argument, "down")) direction = 5;
	else
	{
	    send_to_char("That's not a direction.\n\r", ch);
	    return;
	}
    }

	/* ok, we have a character and a direction in the
	 * format "drive <char> <direction>" or "drive <direction>"
	 * Time to play.
	 */

    if (is_safe(ch, victim))
	return;

    if (ch->fighting != NULL && ch->fighting != victim)
    {
	send_to_char("You can't drive someone back while fighting another person!\n\r", ch);
	return;
    }

    chance /= 2;
    chance += ch->level - victim->level;

    if (!ch->in_room)
	return;

    if ((pexit = ch->in_room->exit[direction]) == NULL)
    {
	send_to_char("There's nothing in the direction.\n\r", ch);
	return;
    }

    if (!IS_AWAKE(victim))
    {
	send_to_char("You can attack them while they're asleep, but you can't drive them back.\n\r", ch);
	return;
    }

    if ((ch->fighting == NULL || victim->fighting == NULL || victim->fighting != ch) && !IS_NPC(victim))
    {
	sprintf(buf, "Help! %s is attacking me!", PERS(ch, victim));
	do_autoyell(victim, buf);
    }

    if (IS_NPC(victim) && (IS_SET(victim->act, ACT_NOSUBDUE) || IS_SET(victim->act, ACT_SENTINEL)))
	chance = 0;

    if (chance < number_percent())
    {
	act("$n attempts to push $N around with a flurry of moves, but fails.", ch, NULL, victim, TO_NOTVICT);
	act("You attempt to push $N around with a flurry of moves, but fail.", ch, NULL, victim, TO_CHAR);
	act("$n attempts to push you around with a flurry of moves, but fails.", ch, NULL, victim, TO_VICT);
        check_improve(ch,victim,gsn_drive,FALSE,1);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_drive].beats));
	if (victim->fighting == NULL)
	{
	    check_killer(ch, victim);
	    one_hit(victim, ch, TYPE_UNDEFINED,HIT_PRIMARY, false);
	}
	return;
    }

    act("$n drives $N back with a flurry of moves!", ch, NULL, victim, TO_NOTVICT);
    act("$n drives you back with a flurry of moves!", ch, NULL, victim, TO_VICT);
    act("You drive $N back with a flurry of moves!", ch, NULL, victim, TO_CHAR);

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_drive].beats));
    move_char(victim, direction, FALSE);
    if (ch->in_room != victim->in_room)
	move_char(ch, direction, FALSE);
    check_improve(ch,victim,gsn_drive,TRUE,1);

    if (victim->fighting == NULL)
    {
	check_killer(ch, victim);
	one_hit(victim, ch, TYPE_UNDEFINED,HIT_PRIMARY, false);
    }
}

void do_entangle( CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    OBJ_DATA *wield;
    CHAR_DATA *victim;
    int chance;

    if ((chance = get_skill(ch, gsn_entangle)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if ((wield = get_eq_char(ch, WEAR_HOLD)) == NULL)
    {
	send_to_char("You must wield a net to entangle.\n\r", ch);
	return;
    }

    if (wield->item_type != ITEM_NET)
    {
	send_to_char("You must wield a net to entangle.\n\r", ch);
	return;
    }

    if (is_affected(ch, gsn_entangle))
    {
	send_to_char("You're too tangled up to throw your own net!\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	if ((victim = ch->fighting) == NULL)
	{
	    send_to_char("Entangle whom?\n\r", ch);
	    return;
	}
    }
    else if ((victim = get_char_room(ch, argument)) == NULL)
    {
	send_to_char("You don't see them here.\n\r", ch);
	return;
    }

    if (is_affected(victim,gsn_entangle))
    {
	send_to_char("You don't think another net will tangle them up much more.\n\r",ch);
	return;
    }
 
    if (!check_reach(ch, victim, REACH_EASY, PULSE_VIOLENCE))
	return;

    if (victim->fighting != ch && !IS_NPC(victim))
    {
	sprintf(buf, "Help! %s is entangling me!", PERS(ch, victim));
	do_autoyell(victim, buf);
    }

    act("$n casts $p out to entangle $N!", ch, wield, victim, TO_NOTVICT);
    act("You cast $p out to entangle $N!", ch, wield, victim, TO_CHAR);
    act("$n casts $p out to entangle you!", ch, wield, victim, TO_VICT);

    if (IS_AFFECTED(victim, AFF_PASS_DOOR) && number_bits(1) == 0)
    {
	act("Your net passes through $N, and lands on the ground.",ch,wield,victim,TO_CHAR);
	act("$n's net passes through $N, and lands on the ground.",ch,wield,victim,TO_NOTVICT);
	act("$n's net passes through you, and lands on the ground.",ch,wield,victim,TO_VICT);
	obj_from_char(wield);
	obj_to_room(wield,ch->in_room);
	return;
    }
    
    chance /= 2;
    chance += (ch->level - victim->level);
    chance -= (get_curr_stat(victim, STAT_DEX) - 18);

    if (IS_AFFECTED(victim, AFF_HASTE)
     || (IS_NPC(victim) && IS_SET(victim->off_flags, OFF_FAST)))
	chance -= 20;

    if (IS_AFFECTED(ch, AFF_HASTE))
	chance += 10;

    if (IS_NAFFECTED(victim, AFF_AGILITY) 
      && (number_percent() < get_skill(victim,gsn_agility)/2+((SIZE_MEDIUM - victim->size)*10)))
	chance /= 2;
    
    if (IS_AFFECTED(ch, AFF_BLIND))
	chance *= 1.5;

    chance -= get_skill(victim,gsn_evade)/5;
    chance -= get_skill(victim,gsn_tumbling)/5;

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_entangle].beats));

    check_killer(ch, victim);

    if (number_percent() > chance)
    {
        act("$N avoids $p!", ch, wield, victim, TO_NOTVICT);
        act("You avoid $p!", ch, wield, victim, TO_VICT);
        act("$N avoids $p!", ch, wield, victim, TO_CHAR);
        if (!victim->fighting)
            one_hit(victim, ch, TYPE_UNDEFINED,HIT_PRIMARY, false);
        check_improve(ch,victim,gsn_entangle,FALSE,1);
        obj_from_char(wield);
        obj_to_room(wield,ch->in_room);
        return;
    }
    
    af.where        = TO_AFFECTS;
    af.type         = gsn_entangle;
    af.level        = ch->level;
    af.duration     = 1;
    af.location     = 0;
    af.bitvector    = 0;
    af.modifier     = 1;
    affect_to_char(victim, &af);

    WAIT_STATE(victim, UMAX(victim->wait, skill_table[gsn_entangle].beats));

    act("$N is entangled in $p!", ch, wield, victim, TO_NOTVICT);
    act("You are entangled in $p!", ch, wield, victim, TO_VICT);
    act("$N is entangled in $p!", ch, wield, victim, TO_CHAR);
    check_improve(ch,victim,gsn_entangle,TRUE,1);

    if (victim->fighting == NULL)
	one_hit(victim, ch, TYPE_UNDEFINED,HIT_PRIMARY, false);
    extract_obj(wield);
}

void do_grip( CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    OBJ_DATA *wield;
    int chance;

    if ((chance = get_skill(ch, gsn_grip)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (is_affected(ch, gsn_grip))
    {
	send_to_char("You already have a clenched grip.\n\r", ch);
	return;
    }

    if ((wield = get_eq_char(ch, WEAR_WIELD)) == NULL)
    {
	send_to_char("You need a primary weapon in hand before you can grip it.\n\r", ch);
	return;
    }

    if (ch->mana < skill_table[gsn_grip].min_mana)
    {
	send_to_char("You don't have the strength to lock your grip on your weapon.\n\r", ch);
	return;
    }

    expend_mana(ch, skill_table[gsn_grip].min_mana);

    if (number_percent() > chance)
    {
	send_to_char("You try to tighten your grip, but your hand aches and you have to relax.\n\r", ch);
        check_improve(ch,NULL,gsn_grip,FALSE,1);
	return;
    }

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_grip].beats));
    check_improve(ch,NULL,gsn_grip,TRUE,1);

    af.where        = TO_AFFECTS;
    af.type         = gsn_grip;
    af.level        = ch->level;
    af.duration     = UMAX(4,ch->level/5);
    af.modifier     = 0;
    af.location     = 0;
    af.bitvector    = 0;
    affect_to_char(ch, &af);

    act("You tighten your grip, in preparation for battle.", ch, NULL, NULL, TO_CHAR);

}



void do_cross( CHAR_DATA *ch, char *argument)
{
CHAR_DATA *victim;
OBJ_DATA *wield;
int chance, result;


    if ((chance = get_skill(ch, gsn_cross)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if ((victim = ch->fighting) == NULL)
    {
	send_to_char("You aren't fighting anyone.\n\r", ch);
	return;
    }

    if ((wield = get_eq_char(ch, WEAR_WIELD)) == NULL ||
	wield->value[0] != WEAPON_SWORD)
    {
	send_to_char("You can't cross without a sword in your main hand.\n\r", ch);
	return;
    }

    if (is_safe(ch,victim))
        return;

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;

    act("$n feints with $s sword, using a stroke to close the distance with $N.", ch, NULL, victim, TO_NOTVICT);
    act("You feint with your sword, using a stroke to close the distance with $N.", ch, NULL, victim, TO_CHAR);
    act("$n feints with $s sword, using a stroke to close the distance with you.", ch, NULL, victim, TO_VICT);

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_cross].beats));
 
   chance += ((ch->level - victim->level)*2);

    if ((result = number_percent()) < chance*.8)
    {
        check_improve(ch,victim,gsn_cross,TRUE,1);
	act("$n uses $s position to knee $N sharply!", ch, NULL, victim, TO_NOTVICT);
	act("$n uses $s position to knee you sharply!", ch, NULL, victim, TO_VICT);
	act("You use your position to knee $N sharply!", ch, NULL, victim, TO_CHAR);
        damage_old(ch,victim,number_range(ch->level/2+10, ch->level+10),gsn_cross,DAM_BASH,TRUE);

        if (victim != NULL && !IS_OAFFECTED(victim,AFF_GHOST) && result < chance/2)
	{
	act("$n uses $s leverage to knock $N to the ground!", ch, NULL, victim, TO_NOTVICT);
	act("$n uses $s leverage to knock you to the ground!", ch, NULL, victim, TO_VICT);
	act("You use your leverage to knock $N to the ground!", ch, NULL, victim, TO_CHAR);
	WAIT_STATE(victim, UMAX(victim->wait, 2*PULSE_VIOLENCE+1));
	}
        check_improve(ch,victim,gsn_cross,TRUE,1);

    }
    else
	{
	act("$n tries to knee $N, but $N pushes away and avoids it!", ch, NULL, victim, TO_NOTVICT);
	act("You try to knee $N, but $N pushes away and avoids it!", ch, NULL, victim, TO_CHAR);
	act("$n tries to knee you, but you push away and avoids it!", ch, NULL, victim, TO_VICT);
        check_improve(ch,victim,gsn_cross,FALSE,1);
	return;
	}


}

void do_strip( CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *wield;
    OBJ_DATA *vwield;
    AFFECT_DATA *paf;
    int chance, result;


    if ((chance = get_skill(ch, gsn_strip)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    chance = chance * 3 / 4;

    if (!ch->in_room)
	 return;

    if (argument[0] == '\0')
    {
	if ((victim = ch->fighting) == NULL)
	{
	    send_to_char("Strip whom?\n\r", ch);
	    return;
	}
    }
    else if ((victim = get_char_room(ch, argument)) == NULL)
    {
	send_to_char("Strip whom?\n\r", ch);
	return;
    }

    if (is_safe(ch,victim))
        return;
 
    if ((wield = get_eq_char(ch, WEAR_WIELD)) == NULL)
    {
	send_to_char("You must wield a whip to strip.\n\r", ch);
	return;
    }

    if (wield->value[0] != WEAPON_WHIP)
	if ((wield = get_eq_char(ch, WEAR_DUAL_WIELD)) == NULL
	 || (wield->value[0] != WEAPON_WHIP))
	{
	    send_to_char("You must wield an whip to strip.\n\r", ch);
	    return;
	}


    if ((vwield = get_eq_char(victim, WEAR_WIELD)) == NULL)
	if ((vwield = get_eq_char(victim, WEAR_DUAL_WIELD)) == NULL)
	{
	    send_to_char("Your opponent is not wielding a weapon!\n\r", ch);
	    return;
	}

    if (!can_see_obj(ch, vwield))
    {
	send_to_char("Your opponent is not wielding a weapon.\n\r", ch);
	return;
    }

    if (ch == victim)
    {
        send_to_char("You cannot strip yourself.\n", ch);
        return;
    }

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;

    if (IS_AFFECTED(victim, AFF_HASTE) || (IS_NPC(victim) && IS_SET(victim->off_flags, OFF_FAST)))
	chance = chance * 3 / 4;

    chance += (ch->level - victim->level);

    if (vwield->value[0] == WEAPON_DAGGER)
        chance /= 2;

	//With the addition of grip to all warriors, glads' grips will be way better than
	//the rest of the warriors. -Kestrel
    if (is_affected(victim, gsn_grip))
    {
	if (victim->class_num == global_int_class_gladiator)
	    chance /= 7;
	else
	    chance /= 5;
    }
    
    if (obj_is_affected(vwield,gsn_bindweapon))
    {
	chance /= 5;
    }

    if (IS_OBJ_STAT(vwield, WEAPON_TWO_HANDS))
	chance /= 2;

    if ((paf = affect_find(victim->affected, gsn_grease))
     && paf->modifier >= 3)
	chance *= 2;

    chance = URANGE(5, chance, 95);

    if (IS_NPC(victim) && IS_SET(victim->nact, ACT_NODISARM))
	chance = 0;

    if (!IS_NPC(victim) && ((ch->fighting == NULL) || (victim->fighting != ch)))
    {
	sprintf(buf, "Help! %s is attacking me!", PERS(ch, victim));
	do_autoyell(victim, buf);
    }

    check_killer(ch, victim);

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_strip].beats));

    if ((vwield->extra_flags[0] & ITEM_NOREMOVE)
      || (vwield->extra_flags[0] & ITEM_NODISARM))
    {
	act("You try to strip $N's weapon, but it won't budge!", ch, NULL, victim, TO_CHAR);
	act("$n tries to strip $N's weapon, but it won't budge!", ch, NULL, victim, TO_ROOM);
	act("$n tries to strip your weapon, but fails.", ch, NULL, victim, TO_VICT);
    }
    else if ((result = number_percent()) < chance)
    {
	act("$n lashes out at $N with $p, disarming $M!", ch, wield, victim, TO_NOTVICT);
	act("You lash out at $N with $p, disarming $M!", ch, wield, victim, TO_CHAR);
	act("$n lashes out at you with $p, disarming you!", ch, wield, victim, TO_VICT);
        check_improve(ch,victim,gsn_strip,TRUE,1);

	oprog_remove_trigger(vwield);

	if (!IS_OBJ_STAT(vwield, ITEM_NODROP)
	  && !IS_OBJ_STAT(vwield, ITEM_INVENTORY))
	    if (number_percent() < 24)
	    {
	    	act("$n deftly snaps $p back to $mself!", ch, vwield, NULL, TO_ROOM); 
	    	act("You deftly snap $p back to yourself!", ch, vwield, NULL, TO_CHAR); 
	    	obj_from_char(vwield);
	    	obj_to_char(vwield, ch);
	    	check_aura(ch, vwield, TRUE);
	    }
	    else
	    {
	    	obj_from_char(vwield);
	    	obj_to_room(vwield, ch->in_room);
	    }
    }
    else
    {
	act("$n tries to strip $N with $p, but misses!", ch, wield, victim, TO_NOTVICT);
	act("You try to strip $N with $p, but miss!", ch, wield, victim, TO_CHAR);
	act("$n tries to strip you with $p, but misses!", ch, wield, victim, TO_VICT);
        check_improve(ch,victim,gsn_strip,FALSE,1);
    }

    if (!victim->fighting)
        one_hit(victim, ch, TYPE_UNDEFINED,HIT_PRIMARY, false);
}

void do_lash( CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *wield;
    int dam, chance, result;

    if ((chance = get_skill(ch, gsn_lash)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    chance = chance * 3 / 4;

    if (argument[0] == '\0')
    {
	if ((victim = ch->fighting) == NULL)
	{
	    send_to_char("Lash whom?\n\r", ch);
	    return;
	}
    }
    else if ((victim = get_char_room(ch, argument)) == NULL)
    {
	send_to_char("Lash whom?\n\r", ch);
	return;
    }

    if (is_safe(ch,victim))
        return;
 
    if ((wield = get_eq_char(ch, WEAR_WIELD)) == NULL)
    {
	send_to_char("You must wield a whip or flail to lash.\n\r", ch);
	return;
    }

    if (wield->value[0] != WEAPON_WHIP && wield->value[0] != WEAPON_FLAIL)
    	if ((wield = get_eq_char(ch, WEAR_DUAL_WIELD)) == NULL 
	  || (wield->value[0] != WEAPON_WHIP && wield->value[0] != WEAPON_FLAIL))
	{
	    send_to_char("You must wield an whip or flail to lash.\n\r", ch);
	    return;
	}

    if (!check_reach(ch, victim, REACH_EASY, PULSE_VIOLENCE))
	return;

    if (IS_AFFECTED(victim, AFF_HASTE) 
      || (IS_NPC(victim) && IS_SET(victim->off_flags, OFF_FAST)))
	chance = chance * 3 / 4;

    chance += (ch->level - victim->level);

    chance = URANGE(5, chance, 95);

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_lash].beats));

    if ((result = number_percent()) < chance)
    {
	act("$n lashes $N with $p!", ch, wield, victim, TO_NOTVICT);
	act("You lash $N with $p!", ch, wield, victim, TO_CHAR);
	act("$n lashes you with $p!", ch, wield, victim, TO_VICT);
        check_improve(ch,victim,gsn_lash,TRUE,1);
    }
    else
    {
	act("$n tries to lash $N with $p, but $N avoids the stroke!", ch, wield, victim, TO_NOTVICT);
	act("You try to lash $N with $p, but $N avoids the stroke!", ch, wield, victim, TO_CHAR);
	act("$n tries to lash you with $p, but you avoid the stroke!", ch, wield, victim, TO_VICT);
        check_improve(ch,victim,gsn_lash,FALSE,1);
	damage(ch, victim, 0, gsn_lash, DAM_NONE, FALSE);
	return;
    }

    if (!IS_NPC(victim))
	if (ch->fighting == NULL || victim->fighting != ch)
	{
	    sprintf(buf, "Help! %s is lashing me!", PERS(ch, victim));
	    do_autoyell(victim, buf);
	}

    dam = dice(wield->value[1], wield->value[2]) + GET_DAMROLL(ch)/2;

    damage( ch, victim, dam, gsn_lash, wield->value[3], TRUE );

    if (number_percent() < 46)
    {
	act("$N is entangled by $p and tumbles to the ground!", ch, wield, victim, TO_NOTVICT); 
	act("$N is entangled by $p and tumbles to the ground!", ch, wield, victim, TO_CHAR); 
	act("You are entangled by $p and tumble to the ground!", ch, wield, victim, TO_VICT); 
	WAIT_STATE(victim, UMAX(victim->wait, number_fuzzy(2*PULSE_VIOLENCE + 1)));
	DAZE_STATE(victim, UMAX(victim->daze, number_fuzzy(2*PULSE_VIOLENCE + 1)));
    }

    return;
}

void do_lunge( CHAR_DATA *ch, char *argument)
{
char buf[MAX_STRING_LENGTH];
CHAR_DATA *victim;
OBJ_DATA *wield;
int dam, chance;
AFFECT_DATA af;

    if ((chance = get_skill(ch, gsn_lunge)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (argument[0] == '\0')
    {
        if ((victim = ch->fighting) == NULL)
	{
  	    send_to_char("Lunge at whom?\n\r", ch);
	    return;
	}
    }
    else if ((victim = get_char_room(ch, argument)) == NULL)
    {
	send_to_char("Lunge at whom?\n\r", ch);
	return;
    }
    if (ch == victim)
    {
	send_to_char("You lunge viciously at yourself, but miss.\n\r",ch);
	return;
    }
    if (is_safe(ch,victim))
        return;

    wield = get_eq_char(ch,WEAR_WIELD);
 
    if (!check_reach(ch, victim, REACH_EASY, PULSE_VIOLENCE))
	return;
 
    chance += (ch->level - victim->level);

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_lunge].beats));

    if (!IS_NPC(victim))
	if (ch->fighting == NULL || victim->fighting != ch)
	{
   	    sprintf(buf, "Help! %s is attacking me!", PERS(ch, victim));
	    do_autoyell(victim, buf);
	}

    if (number_percent() < chance)
    {
	if (wield)
	{
	    act("$n lunges at $N with $p!", ch, wield, victim, TO_NOTVICT);
	    act("You lunge at $N with $p!", ch, wield, victim, TO_CHAR);
	    act("$n lunges at you with $p!", ch, wield, victim, TO_VICT);
	}
	else
	{
	    act("$n lunges at $N!",ch,NULL,victim,TO_NOTVICT);
	    act("You lunge at $N!",ch,NULL,victim,TO_CHAR);
	    act("$n lunges at you!",ch,NULL,victim,TO_VICT);
	}
        check_improve(ch,victim,gsn_lunge,TRUE,1);
    }
    else
    {
	if (wield)
	{
	    act("$n tries to lunge at $N with $p, but $N avoids the attack!", ch, wield, victim, TO_NOTVICT);
	    act("You try to lunge at $N with $p, but $N avoids the attack!", ch, wield, victim, TO_CHAR);
	    act("$n tries to lunge at you with $p, but you avoid the attack!", ch, wield, victim, TO_VICT);
	}
	else
	{
	    act("$n tries to lunge at $N, but $N avoids the attack!",ch,NULL,victim,TO_NOTVICT);
	    act("You try to lunge at $N, but $N avoids the attack!",ch,NULL,victim,TO_CHAR);
	    act("$n tries to lunge at you, but you avoid the attack!",ch,NULL,victim,TO_VICT);
	}
        check_improve(ch,victim,gsn_lunge,FALSE,1);
        damage( ch, victim, 0, gsn_lunge, 0, TRUE );
	return;
    }

    if (wield)
	dam = dice(wield->value[1], wield->value[2]) + GET_DAMROLL(ch);
    else
	dam = dice(ch->damage[DICE_NUMBER],ch->damage[DICE_TYPE])+GET_DAMROLL(ch);
    dam += check_extra_damage(ch,dam,wield);

    if (wield && IS_WEAPON_STAT(wield,WEAPON_TWO_HANDS))
	dam *= 5/4;
    
    if (wield)
	damage_from_obj( ch, victim, wield, dam, gsn_lunge, wield->value[3], TRUE );
    else
	damage(ch,victim,dam,gsn_lunge,ch->dam_type,TRUE);
    
    if (!is_affected(ch,gsn_lunge))
    {
	af.valid = TRUE;
	af.point = NULL;
	af.where = TO_AFFECTS;
	af.location = APPLY_AC;
	af.type = gsn_lunge;
	af.modifier = ch->level;
	af.duration = 0;
	af.bitvector = 0;
	affect_to_char(ch,&af);
    }	    
    return;
}



void do_cleave( CHAR_DATA *ch, char *argument)
{
char buf[MAX_STRING_LENGTH];
CHAR_DATA *victim;
OBJ_DATA *wield, *shield;
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
int dam, chance;


    if ((chance = get_skill(ch, gsn_cleave)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    chance = chance * 85 / 100;

    if (argument[0] == '\0')
    {
	if ((victim = ch->fighting) == NULL)
	{
	    send_to_char("Cleave whom?\n\r", ch);
	    return;
	}
    }
    else if ((victim = get_char_room(ch, argument)) == NULL)
    {
	send_to_char("Cleave whom?\n\r", ch);
	return;
    }

    if (is_safe(ch,victim))
        return;
 
   if ((wield = get_eq_char(ch, WEAR_WIELD)) == NULL)
   {
	send_to_char("You must wield an axe or sword in your main hand to cleave.\n\r", ch);
	return;
   }

   if (wield->value[0] != WEAPON_AXE && wield->value[0] != WEAPON_SWORD
     && !(wield->value[0] == WEAPON_EXOTIC && wield->value[3] == DAM_SLASH))
	{
	send_to_char("You must wield an axe or sword to cleave.\n\r", ch);
	return;
	}

   if (!check_reach(ch, victim, REACH_HARD, PULSE_VIOLENCE))
	return;

   if (IS_AFFECTED(victim, AFF_HASTE) || (IS_NPC(victim) && IS_SET(victim->off_flags, OFF_FAST)))
	chance = chance * 3 / 4;

   if (get_eq_char(victim, WEAR_SHIELD) != NULL)
	chance /= 2;

   chance += (ch->level - victim->level);

   WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_cleave].beats));

       if (!IS_NPC(victim))
	if (ch->fighting == NULL || victim->fighting != ch)
	{
	sprintf(buf, "Help! %s is cleaving me!", PERS(ch, victim));
	do_autoyell(victim, buf);
	}

   if (number_percent() < chance)
	{
	act("$n cleaves $N with $p!", ch, wield, victim, TO_NOTVICT);
	act("You cleave $N with $p!", ch, wield, victim, TO_CHAR);
	act("$n cleaves you with $p!", ch, wield, victim, TO_VICT);
        check_improve(ch,victim,gsn_cleave,TRUE,1);
	}
   else
	{
	act("$n tries to cleave $N with $p, but $N avoids the swing!", ch, wield, victim, TO_NOTVICT);
	act("You try to cleave $N with $p, but $N avoids the swing!", ch, wield, victim, TO_CHAR);
	act("$n tries to cleave you with $p, but you avoid the swing!", ch, wield, victim, TO_VICT);
        check_improve(ch,victim,gsn_cleave,FALSE,1);
        damage( ch, victim, 0, gsn_cleave, 0, TRUE );
	return;
	}

    if ((shield = get_eq_char(victim, WEAR_ABOUT))
     && (shield->pIndexData->vnum == OBJ_VNUM_MANTLE_EARTH)
     && (number_bits(2) != 0))
    {
	act("$N's mantle of earth stops your cleave!", ch, NULL, victim, TO_CHAR);
	act("Your mantle of earth stops $n's cleave!", ch, NULL, victim, TO_VICT);
	act("$N's mantle of earth stops $n's cleave!", ch, NULL, victim, TO_NOTVICT);
	return;
    }

    dam = dice(wield->value[1], wield->value[2]) + GET_DAMROLL(ch);
    dam *= 2;

    damage_from_obj( ch, victim, wield, dam, gsn_cleave, wield->value[3], TRUE );
    if (victim && !IS_OAFFECTED(victim,AFF_GHOST))
    {
        af.where        = TO_AFFECTS;
        af.type         = gsn_cleave;
        af.level        = ch->level;
        af.duration     = dam/8;
        af.modifier     = -1 * dam/3;
        af.location     = APPLY_HIT;
        af.bitvector    = 0;
        affect_to_char(victim, &af);
    }

	return;
    
}

void do_vitalstrike( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *victim;
    OBJ_DATA *wield;
    int dam, mod, lala;
    int victim_ac;
    int thac0;
    int thac0_00;
    int thac0_32;
    int diceroll;
    int sn,skill;
    int dam_type;
    bool result;
    int dt;
 
    sn = -1;
 
    if (!get_skill(ch, gsn_vitalstrike))
    {
        send_to_char(
            "Huh?\n\r", ch );
        return;
    }

   if (argument[0] != '\0')
        {
        if ((victim = get_char_room(ch, argument))==NULL)
                {
                send_to_char("You don't see them here.\n\r", ch);
                return;
                }
        }
   else if ((victim = ch->fighting) == NULL)
   {
        send_to_char("Who did you want to vitalstrike?\n\r", ch);
        return;
   }
    if (is_affected(victim, gsn_diamondskin) && IS_SET(victim->imm_flags, IMM_WEAPON))
    {
        act("You attempt a strike into $N's vital regions, but are blocked by their diamond skin!", ch, NULL, victim,TO_CHAR);
        act("$n attempts a strike into your vital regions, but your diamond skin blocks it!", ch, NULL, victim,TO_VICT);
        WAIT_STATE(ch,skill_table[gsn_vitalstrike].beats);
        return;
    }


   if (ch == victim)
   {
 	send_to_char("You don't really want to vital strike yourself.\n", ch);
	return;
   }
 
    /*
     * Figure out the type of damage message.
     */
    wield = get_eq_char( ch, WEAR_WIELD );
    if (wield == NULL ||wield->value[0] != WEAPON_DAGGER)
        {
	  wield = get_eq_char(ch, WEAR_DUAL_WIELD);
          if (wield == NULL ||wield->value[0] != WEAPON_DAGGER)
	  {
            send_to_char("You need a dagger to vital strike someone!\n\r", ch);
            return;
	  }
        }
 
    if (is_safe(ch,victim))
        return;
 
    if ( victim->position == POS_DEAD || ch->in_room != victim->in_room )
        return;

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;
 
    WAIT_STATE( ch, skill_table[gsn_vitalstrike].beats );
 
    if (!IS_NPC(victim) && (ch->fighting == NULL || victim->fighting != ch))
    {
      sprintf(buf, "Help! %s is trying to stab me!", PERS(ch, victim));
      do_autoyell(victim, buf);
    }

    if (is_affected(victim,gsn_scouting) && number_bits(1) == 0)
    {
	act("You try to stab $N, but aren't able to escape $S notice.",ch,NULL,victim,TO_CHAR);
	act("$n tries to stab you, but isn't able to escape your notice.",ch,NULL,victim,TO_VICT);
	act("$n tries to stab $N, but isn't able to escape $S notice.",ch,NULL,victim,TO_NOTVICT);
	return;
    }
 
    dt = TYPE_HIT + wield->value[3];
    dam_type = wield->value[3];
 
    if (dam_type == -1)
        dam_type = DAM_BASH;
 
     if (wield && obj_is_affected(wield, gsn_heatsink))
            damage_old(ch,ch,number_range(4,8),gsn_heatsink,DAM_COLD,TRUE);

    wrathkyana_combat_effect(ch, victim);

    /* get the weapon skill */
    sn = get_weapon_sn(ch);
    skill = get_skill(ch, gsn_vitalstrike);
 
    /*
     * Calculate to-hit-armor-class_num-0 versus armor.
     */
    if ( IS_NPC(ch) )
    {
        thac0_00 = 20;
        thac0_32 = -4;   /* as good as a thief */
        if (IS_SET(ch->act,ACT_WARRIOR))
            thac0_32 = -10;
        else if (IS_SET(ch->act,ACT_THIEF))
            thac0_32 = -4;
    }
    else
    {
        thac0_00 = class_table[ch->class_num].thac0_00;
        thac0_32 = class_table[ch->class_num].thac0_32;
    }
    thac0  = interpolate( ch->level, thac0_00, thac0_32 );
 
    if (thac0 < 0)
        thac0 = thac0/2;
 
    if (thac0 < -5)
        thac0 = -5 + (thac0 + 5) / 2;
 
    thac0 -= GET_HITROLL(ch) * skill/100;
    thac0 += 5 * (100 - skill) / 100;
 
 
    switch(dam_type)
    {
        case(DAM_PIERCE):victim_ac = GET_AC(victim,AC_PIERCE)/10;       break;
        case(DAM_BASH):  victim_ac = GET_AC(victim,AC_BASH)/10;         break;
        case(DAM_SLASH): victim_ac = GET_AC(victim,AC_SLASH)/10;        break;
        default:         victim_ac = GET_AC(victim,AC_EXOTIC)/10;       break;
    };
 
    if (victim_ac < -15)
        victim_ac = (victim_ac + 15) / 5 - 15;
 
    if ( !can_see( ch, victim ) )
        victim_ac -= 4;
 
    if ( victim->position < POS_FIGHTING)
        victim_ac += 4;
 
    if (victim->position < POS_RESTING)
        victim_ac += 6;
 
 
    /*
     * The moment of excitement!
     */
    while ( ( diceroll = number_bits( 5 ) ) >= 20 )
        ;
 
 
    if ((lala = (get_curr_stat(victim, STAT_DEX) - 18)) > 0)
    diceroll -= lala;
 
    if (( diceroll == 0
    || ( diceroll != 19 && diceroll < thac0 - victim_ac ) )
    && !is_affected(ch, gsn_form_of_the_serpent))
    {
        /* Miss. */
        damage( ch, victim, 0, gsn_vitalstrike, dam_type, TRUE );
        check_improve(ch,victim,gsn_vitalstrike,FALSE,1);
        return;
    }
 
    /*
     * Hit.
     * Calc damage.
     */
    check_improve(ch,victim,gsn_vitalstrike,TRUE,2);
    if ( IS_NPC(ch) && (!ch->pIndexData->new_format || wield == NULL))
        if (!ch->pIndexData->new_format)
        {
            dam = number_range( ch->level / 2, ch->level * 3 / 2 );
            if ( wield != NULL )
                dam += dam / 2;
        }
        else
            dam = dice(ch->damage[DICE_NUMBER],ch->damage[DICE_TYPE]);
 
    else
    {
        if ( wield != NULL )
        {
            if (wield->pIndexData->new_format)
                dam = (dice(wield->value[1],wield->value[2]) * skill)/100;
            else
                dam = ((number_range( wield->value[1],
                                wield->value[2] )* skill)/100);
 
                dam = dam * 11/10;
 
        }
        else
	  return;
    }
 
    /*
     * Bonuses.
     */
    dam += check_extra_damage(ch,dam,wield);

    if ( !IS_AWAKE(victim) )
        dam *= 2;
     else if (victim->position < POS_FIGHTING)
        dam = dam * 5 / 4;
 
    dam += GET_DAMROLL(ch) * UMIN(100,skill) /100;

    dam /= 3;
 
    if ( dam <= 0 )
        dam = 1;
 
    result = damage( ch, victim, dam, gsn_vitalstrike, dam_type, TRUE );

    if (is_affected(victim, gsn_vitalstrike))
      mod = get_modifier(victim->affected, gsn_vitalstrike);
    else
      mod = 0;

    mod = UMIN(2,mod);
 
    af.where        = TO_OAFFECTS;
    af.type         = gsn_vitalstrike;
    af.level        = ch->level;
    af.duration     = 5;
    af.modifier     = mod+1;
    af.location     = 0;
    af.bitvector    = AFF_BLEEDING;
    affect_strip(victim, gsn_vitalstrike);
    affect_to_char(victim, &af);
}

void do_stab( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *wield;
    int dam;
    int victim_ac;
    int thac0;
    int thac0_00;
    int thac0_32;
    int diceroll;
    int sn,skill;
    int dam_type;
    bool result;
    int dt;
 
    sn = -1;
 
    if (!get_skill(ch, gsn_stab))
    {
        send_to_char(
            "Huh?\n\r", ch );
        return;
    }

   if (ch->fighting != NULL)
   {
	send_to_char("But you're already fighting!\n\r", ch);
	return;
   }
 
   if (argument[0] != '\0')
        {
        if ((victim = get_char_room(ch, argument))==NULL)
                {
                send_to_char("You don't see them here.\n\r", ch);
                return;
                }
        }
   else
   {
        send_to_char("Who did you want to stab?\n\r", ch);
        return;
   }

   if (ch == victim)
   {
 	send_to_char("You don't really want to stab yourself.\n", ch);
	return;
   }
 
    /*
     * Figure out the type of damage message.
     */
    wield = get_eq_char( ch, WEAR_WIELD );
 
    if (!wield || (wield->value[0] != WEAPON_DAGGER))
    {
	wield = get_eq_char(ch, WEAR_DUAL_WIELD);
	if (!wield || (wield->value[0] != WEAPON_DAGGER))
	{
            send_to_char("You need a dagger to stab someone!\n\r", ch);
            return;
	}
    }
 
    if (is_safe(ch,victim))
        return;
 
    if ( victim->position == POS_DEAD || ch->in_room != victim->in_room )
        return;

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;
 
    WAIT_STATE( ch, skill_table[gsn_stab].beats );
 
    if (!IS_NPC(victim) && (ch->fighting == NULL || victim->fighting != ch))
    {
    sprintf(buf, "Help! %s is trying to stab me!", PERS(ch, victim));
    do_autoyell(victim, buf);
    }
 
    dt = TYPE_HIT + wield->value[3];
    dam_type = wield->value[3];
 
    if (dam_type == -1)
        dam_type = DAM_BASH;
 
     if (wield && obj_is_affected(wield, gsn_heatsink))
            damage_old(ch,ch,number_range(4,8),gsn_heatsink,DAM_COLD,TRUE);

    wrathkyana_combat_effect(ch, victim);

    /* get the weapon skill */

    if (get_eq_char(ch, WEAR_WIELD) == wield)
        sn = get_weapon_sn(ch);
    else
	sn = get_dual_sn(ch);
    skill = get_skill(ch, gsn_stab);
 
    /*
     * Calculate to-hit-armor-class_num-0 versus armor.
     */
    if ( IS_NPC(ch) )
    {
        thac0_00 = 20;
        thac0_32 = -4;   /* as good as a thief */
        if (IS_SET(ch->act,ACT_WARRIOR))
            thac0_32 = -10;
        else if (IS_SET(ch->act,ACT_THIEF))
            thac0_32 = -4;
    }
    else
    {
        thac0_00 = class_table[ch->class_num].thac0_00;
        thac0_32 = class_table[ch->class_num].thac0_32;
    }
    thac0  = interpolate( ch->level, thac0_00, thac0_32 );
 
    if (thac0 < 0)
        thac0 = thac0/2;
 
    if (thac0 < -5)
        thac0 = -5 + (thac0 + 5) / 2;
 
    thac0 -= GET_HITROLL(ch) * skill/100;
    thac0 += 5 * (100 - skill) / 100;
 
 
    switch(dam_type)
    {
        case(DAM_PIERCE):victim_ac = GET_AC(victim,AC_PIERCE)/10;       break;
        case(DAM_BASH):  victim_ac = GET_AC(victim,AC_BASH)/10;         break;
        case(DAM_SLASH): victim_ac = GET_AC(victim,AC_SLASH)/10;        break;
        default:         victim_ac = GET_AC(victim,AC_EXOTIC)/10;       break;
    };
 
    if (victim_ac < -15)
        victim_ac = (victim_ac + 15) / 5 - 15;
 
    if ( !can_see( ch, victim ) )
        victim_ac -= 4;
 
    if ( victim->position < POS_FIGHTING)
        victim_ac += 4;
 
    if (victim->position < POS_RESTING)
        victim_ac += 6;
 
 
    /*
     * The moment of excitement!
     */
    while ( ( diceroll = number_bits( 5 ) ) >= 20 )
        ;
 
 
 
    if (( diceroll == 0
    || ( diceroll != 19 && diceroll < thac0 - victim_ac ) )
    && !is_affected(ch, gsn_form_of_the_serpent))
    {
        /* Miss. */
        damage( ch, victim, 0, gsn_stab, dam_type, TRUE );
 
        check_improve(ch,victim,gsn_stab,FALSE,2);
 
        return;
    }
 
    /*
     * Hit.
     * Calc damage.
     */
    if ( IS_NPC(ch) && (!ch->pIndexData->new_format || wield == NULL))
        if (!ch->pIndexData->new_format)
        {
            dam = number_range( ch->level / 2, ch->level * 3 / 2 );
            if ( wield != NULL )
                dam += dam / 2;
        }
        else
            dam = dice(ch->damage[DICE_NUMBER],ch->damage[DICE_TYPE]);
 
    else
    {
        check_improve(ch,victim,gsn_stab,TRUE,1);
        if ( wield != NULL )
        {
            if (wield->pIndexData->new_format)
                dam = (dice(wield->value[1],wield->value[2]) * skill)/100;
            else
                dam = ((number_range( wield->value[1],
                                wield->value[2] )* skill)/100);
 
                dam = dam * 11/10;
 
        }
        else
	  return;
    }
 
    /*
     * Bonuses.
     */
    dam += check_extra_damage(ch,dam,wield);

    if ( !IS_AWAKE(victim) )
        dam *= 2;
     else if (victim->position < POS_FIGHTING)
        dam = dam * 5 / 4;
 
    dam += GET_DAMROLL(ch) * UMIN(100,skill) /100;

    dam = round(dam * 1.5);
 
    if ( dam <= 0 )
        dam = 1;
 
    result = damage_from_obj( ch, victim, wield, dam, gsn_stab, dam_type, TRUE );
 
}

void do_batter( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *wield;
    int dam;
    int victim_ac;
    int thac0;
    int thac0_00;
    int thac0_32;
    int diceroll;
    int sn,skill;
    int dam_type;
    bool result;
    int dt;
 
    sn = -1;
 
    if (!get_skill(ch, gsn_batter))
    {
        send_to_char("Huh?\n\r", ch );
        return;
    }
 
    if (argument[0] != '\0')
    { 
        if ((victim = get_char_room(ch, argument))==NULL)
        {
            send_to_char("You don't see them here.\n\r", ch);
            return;
        }
    }
    else if ((victim = ch->fighting) == NULL)
    {
        send_to_char("Who did you want to batter?\n\r", ch);
        return;
    }
 
    /*
     * Figure out the type of damage message.
     */
    if (((wield = get_eq_char(ch, WEAR_WIELD)) == NULL) || ((wield->value[0] != WEAPON_MACE) && (wield->value[0] != WEAPON_FLAIL)))
    {
	if (((wield = get_eq_char(ch, WEAR_DUAL_WIELD)) == NULL) || ((wield->value[0] != WEAPON_MACE) && (wield->value[0] != WEAPON_FLAIL)))
	{
	    send_to_char("You need a mace or flail to batter someone!\n\r", ch);
	    return;
	}
    }
 
    if (is_safe(ch,victim))
        return;
 
    if ( victim->position == POS_DEAD || ch->in_room != victim->in_room )
        return;

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;
 
    WAIT_STATE( ch, skill_table[gsn_batter].beats );
 
    if (!IS_NPC(victim) && (ch->fighting == NULL || victim->fighting != ch))
    {
    sprintf(buf, "Help! %s is trying to batter me!", PERS(ch, victim));
    do_autoyell(victim, buf);
    }
 
    dt = TYPE_HIT + wield->value[3];
    dam_type = wield->value[3];
 
    if (dam_type == -1)
        dam_type = DAM_BASH;
 
     if (wield && obj_is_affected(wield, gsn_heatsink))
            damage_old(ch,ch,number_range(4,8),gsn_heatsink,DAM_COLD,TRUE);

    wrathkyana_combat_effect(ch, victim);

    /* get the weapon skill */
    sn = get_weapon_sn(ch);
    skill = ((get_weapon_skill(ch,sn,TRUE)/2) * get_skill(ch, gsn_batter)/100);
 
    /*
     * Calculate to-hit-armor-class_num-0 versus armor.
     */
    if ( IS_NPC(ch) )
    {
        thac0_00 = 20;
        thac0_32 = -4;   /* as good as a thief */
        if (IS_SET(ch->act,ACT_WARRIOR))
            thac0_32 = -10;
        else if (IS_SET(ch->act,ACT_THIEF))
            thac0_32 = -4;
    }
    else
    {
        thac0_00 = class_table[ch->class_num].thac0_00;
        thac0_32 = class_table[ch->class_num].thac0_32;
    }
    thac0  = interpolate( ch->level, thac0_00, thac0_32 );
 
    if (thac0 < 0)
        thac0 = thac0/2;
 
    if (thac0 < -5)
        thac0 = -5 + (thac0 + 5) / 2;
 
    thac0 -= GET_HITROLL(ch) * skill/100;
    thac0 += 5 * (100 - skill) / 100;
 
 
    switch(dam_type)
    {
        case(DAM_PIERCE):victim_ac = GET_AC(victim,AC_PIERCE)/10;       break;
        case(DAM_BASH):  victim_ac = GET_AC(victim,AC_BASH)/10;         break;
        case(DAM_SLASH): victim_ac = GET_AC(victim,AC_SLASH)/10;        break;
        default:         victim_ac = GET_AC(victim,AC_EXOTIC)/10;       break;
    };
 
    if (victim_ac < -15)
        victim_ac = (victim_ac + 15) / 5 - 15;
 
    if ( !can_see( ch, victim ) )
        victim_ac -= 4;
 
    if ( victim->position < POS_FIGHTING)
        victim_ac += 4;
 
    if (victim->position < POS_RESTING)
        victim_ac += 6;
 
 
    /*
     * The moment of excitement!
     */
    while ( ( diceroll = number_bits( 5 ) ) >= 20 )
        ;
 
 
 
    if (( diceroll == 0
    || ( diceroll != 19 && diceroll < thac0 - victim_ac ) )
    && !is_affected(ch, gsn_form_of_the_serpent))
    {
        /* Miss. */
        damage( ch, victim, 0, gsn_batter, dam_type, TRUE );
 
            check_improve(ch,victim,gsn_batter,FALSE,1);
 
        return;
    }
 
    /*
     * Hit.
     * Calc damage.
     */
    if ( IS_NPC(ch) && (!ch->pIndexData->new_format || wield == NULL))
        if (!ch->pIndexData->new_format)
        {
            dam = number_range( ch->level / 2, ch->level * 3 / 2 );
            if ( wield != NULL )
                dam += dam / 2;
        }
        else
            dam = dice(ch->damage[DICE_NUMBER],ch->damage[DICE_TYPE]);
 
    else
    {
        if (sn != -1)
            check_improve(ch,victim,sn,TRUE,5);
        if ( wield != NULL )
        {
            if (wield->pIndexData->new_format)
                dam = dice(wield->value[1],wield->value[2]) * skill/100;
            else
                dam = number_range( wield->value[1] * skill/100,
                                wield->value[2] * skill/100);
 
                dam = dam * 11/10;
 
        }
        else
            dam = number_range( 1 + 4 * skill/100, 2 * ch->level/3 * skill/100);
    }
 
    /*
     * Bonuses.
     */
    dam += check_extra_damage(ch,dam,wield);

    if ( !IS_AWAKE(victim) )
        dam *= 2;
     else if (victim->position < POS_FIGHTING)
        dam = dam * 5 / 4;
 
    dam += GET_DAMROLL(ch) * UMIN(100,skill) /100;

    dam = round(dam * 3.5);
 
    if ( dam <= 0 )
        dam = 1;
 
    result = damage_from_obj( ch, victim, wield, dam, gsn_batter, dam_type, TRUE );
    victim->move = UMAX(victim->move - number_range(dam/5, dam/2), 0);
 
}

void do_setambush( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *victim;
    int chance;

    if ((chance = get_skill(ch,gsn_setambush)) == 0)
    {
	send_to_char("Huh?\n\r",ch);
        return;
    }

    if (argument[0] == '\0' && !is_affected(ch, gsn_setambush))
    {
        send_to_char("Prepare to ambush whom?\n\r", ch);
        return;
    }
    
    if (is_affected(ch,gsn_setambush))
    {
        send_to_char("You're already set to ambush another.\n\r", ch);
        return;
    }

    if ((victim = get_char_world(ch, argument)) == NULL)
    {
        send_to_char("You can't find them.\n\r", ch);
        return;
    }

    if (IS_NPC(victim) || (!IS_NPC(victim) && !IS_PK(ch,victim)))
    {
        send_to_char("You can't ambush them.\n\r",ch);
        return;
    }

    if (victim == ch)
    {
	send_to_char("You attempt to surprise yourself, but fail miserably.\n\r", ch);
	return;
    }

    if (!IS_AFFECTED(ch,AFF_HIDE) && !is_affected(ch,gsn_camouflage))
    {
        send_to_char("You must be hiding or camouflaged to prepare an ambush.\n\r",ch);
        return;
    }

    if (number_percent() > chance)
    {
        send_to_char("You fail to find a suitable place to ambush from.\n\r", ch);
        WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_setambush].beats));
        check_improve(ch,victim,gsn_setambush,FALSE,1);
        return;
    }

    send_to_char("You quickly find a suitable place to lie in wait.\n\r", ch);
    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_setambush].beats));
    check_improve(ch,victim,gsn_setambush,TRUE,1);

    af.where     = TO_AFFECTS;
    af.type      = gsn_setambush;
    af.level     = ch->level;
    af.duration  = -1;
    af.location  = APPLY_HIDE;
    af.modifier  = victim->id;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    add_event((void *) ch, 1, &event_setambush);
    return;
}

void do_choke( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    OBJ_DATA *wield, *dual;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance, used;
 
    one_argument(argument,arg);
    wield = get_eq_char( ch, WEAR_WIELD );
    dual = get_eq_char( ch, WEAR_DUAL_WIELD );
 
    if ( (chance = get_skill(ch,gsn_choke)) == 0)
    {   
        send_to_char("Huh?\n\r",ch);
        return;
    }
 
   if (arg[0] != '\0')
   {
    if ((victim = get_char_room(ch,arg)) == NULL)
    {
        send_to_char("They aren't here.\n\r",ch);
        return;
    }
   }
   else if ((victim = ch->fighting) == NULL)
   {   
        send_to_char("Choke whom?\n\r", ch);
        return;
   }
 
    if (is_affected(victim, gsn_choke))
    {
        send_to_char("They're vigilant against a choking attack.\n\r", ch);
        return;
    }
 
    if (victim == ch)
    {   
        send_to_char("You can't quite manage that.\n\r",ch);
        return;
    }
 
    if (is_safe(ch,victim))
        return;
 
    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {   
        act("But $N is your friend!",ch,NULL,victim,TO_CHAR);
        return;
    }

    if ((!wield || wield->value[0] != WEAPON_WHIP) && (!dual || dual->value[0] != WEAPON_WHIP) )
    {
	send_to_char("You can't choke someone without a whip!\n\r", ch);

	return;
    }

    if ((wield && wield->value[0] == WEAPON_WHIP))
	used = 1;
    else if ((dual && dual->value[0] == WEAPON_WHIP) )
	used = 0;
    else
	return;

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;
 
    if (!can_see(ch, victim))
	chance -= 10;
 
    /* stats */
    chance += get_curr_stat(ch,STAT_DEX)/3;
    chance -= get_curr_stat(victim,STAT_DEX)/3;
    /* speed */
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
        chance -= 25;
 
    /* level */
    chance += (ch->level - victim->level);
 
    /* modifiers */
    chance /= 3;

    if (IS_NPC(victim) && (victim->act & ACT_NOSUBDUE))
	chance = 0;

    check_killer(ch,victim);

    /* now the attack */
    if (number_percent() < chance )
    {   
        act("$n lashes out for your neck, catching it with $p, and the world fades to black!",
                ch, used ? wield : dual,victim,TO_VICT);
        act("You lash out at $N's neck, catching it with $p, and $E falls unconscious as you choke $M!",
                ch,used ? wield : dual,victim,TO_CHAR);
        act("$n lashes out at $N's neck, catching it with $p, and $E falls unconscious as $n chokes $M!",
                ch,used ? wield : dual,victim,TO_NOTVICT);
        check_improve(ch,victim,gsn_choke,TRUE,1);
        WAIT_STATE(ch,skill_table[gsn_choke].beats);
 
        af.where        = TO_AFFECTS;
        af.type         = gsn_choke;
        af.level        = ch->level;
        af.duration     = 3;
        af.modifier     = 0;
        af.location     = 0;
        af.bitvector    = 0;
        affect_to_char(victim, &af);
 
        af.where        = TO_AFFECTS;
        af.type         = gsn_choke;
        af.level        = 10;
        af.duration     = 1;
        af.modifier     = 0;
        af.location     = APPLY_NONE;
        af.bitvector    = AFF_SLEEP;
        affect_to_char (victim, &af);
	stop_fighting_all(victim);
        if (IS_AWAKE(victim))
            switch_position(victim, POS_SLEEPING);
    }
    else
    {   
        do_visible(ch, "FALSE");
        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        { 
          sprintf( buf, "Help!  %s tried to choke me!", PERS(ch, victim));
          do_autoyell( victim, buf );
        }
        damage(ch,victim,number_range(1,5),gsn_choke,DAM_BASH,FALSE);
        act("Your choke misses!",
            ch,NULL,victim,TO_CHAR);
        act("$n lashes out at $N with $p, but misses.",
            ch,used ? wield : dual,victim,TO_NOTVICT);
        act("You skillfully dodge $n's choke attack.",
            ch,NULL,victim,TO_VICT);
        check_improve(ch,victim,gsn_choke,FALSE,1);
        WAIT_STATE(ch,skill_table[gsn_choke].beats);
    }   
        
}

/*  Shield cover.  A Rannock production.  Modified by Erinos.  */
void do_shieldcover( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *sch;
    int chance;

    if (!ch->in_room)
	return;

    if ( (chance = get_skill(ch, gsn_shieldcover) ) == 0)
    {
        send_to_char("Huh?\n\r",ch);
        return;
    }

    if ( is_affected(ch, gsn_shieldcover) )
    {
        send_to_char("You are already guarding your group.\n\r",ch);
        return;
    }

    if ( get_eq_char(ch, WEAR_SHIELD) == NULL )
    {
        send_to_char("You need a shield to cover with.\n\r", ch);
        return;
    }

    if (ch->mana < skill_table[gsn_shieldcover].min_mana)
    {
	send_to_char("You are too tired to cover your group with your shield.\n\r", ch);
	return;
    }

    expend_mana(ch, skill_table[gsn_shieldcover].min_mana);

    for ( sch = ch->in_room->people; sch; sch = sch->next_in_room )
        if (is_same_group(ch, sch) && (ch != sch))
            break;

    if ( sch == NULL )
    {
        send_to_char("You aren't in a group.\n\r", ch);
        return;
    }

    if ( number_percent() < chance )
    {
        act( "You prepare to guard your group.", ch, NULL, NULL, TO_CHAR );
        act( "$n prepares to guard $s group.", ch, NULL, NULL, TO_ROOM );
        af.where =      TO_AFFECTS;
        af.type =       gsn_shieldcover;
        af.level =      ch->level;
        af.duration =   ch->level/2;
        af.modifier =   0;
        af.location =   0;
        af.bitvector =  0;
        affect_to_char(ch, &af);
    }
    else
    {
        act( "You try to prepare to guard your group, but are distracted.", ch,
NULL, NULL, TO_CHAR );
        act( "$n tries to prepare to guard $s group, but is distracted.", ch, NULL, NULL, TO_ROOM );
    }

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_shieldcover].beats));

    return;
}

void do_shockwave(CHAR_DATA *ch, char *argument)
{
    ROOM_INDEX_DATA *pRoom;
    OBJ_DATA *wield;
    CHAR_DATA *vch;
    int dir, chance;

    if (!ch->in_room)
	return;

    if ( (chance = get_skill(ch, gsn_shockwave) ) == 0)
    {
        send_to_char("Huh?\n\r",ch);
        return;
    }

    wield = get_eq_char(ch, WEAR_WIELD);

    if (!wield
     || ((wield->value[0] != WEAPON_MACE) && (wield->value[0] != WEAPON_STAFF)))
    {
	send_to_char("You must be wielding a staff or mace to create a shockwave.\n\r", ch);
	return;
    }

    if ((ch->in_room->sector_type == SECT_WATER_SWIM)
     || (ch->in_room->sector_type == SECT_WATER_NOSWIM)
     || (ch->in_room->sector_type == SECT_AIR)
     || (ch->in_room->sector_type == SECT_UNDERWATER))
    {
	send_to_char("You must be on land to create a shockwave.\n\r", ch);
	return;
    }

    if (ch->mana < skill_table[gsn_shockwave].min_mana)
    {
	send_to_char("You are too tired to create a shockwave.\n\r", ch);
	return;
    }

    expend_mana(ch, skill_table[gsn_shockwave].min_mana);

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_shockwave].beats));

    if (number_percent() >= chance)
    {
	act("$n strikes the ground with $p, but nothing happens.", ch, wield, NULL, TO_ROOM);
	act("You strike the ground with $p, but nothing happens.", ch, wield, NULL, TO_CHAR);
	check_improve(ch,NULL, gsn_shockwave, FALSE, 1);
	return;
    }
    else
    {
        act("$n strikes the ground with $p, causing a powerful shockwave!", ch, wield, NULL, TO_ROOM);
        act("You strike the ground with $p, causing a powerful shockwave!", ch, wield, NULL, TO_CHAR);
	check_improve(ch,NULL, gsn_shockwave, TRUE, 1);
    }

    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
    {
	if (!is_same_group(ch, vch)
	 && !is_safe(ch, vch) 
	 && !IS_IMMORTAL(vch)
	 && (!IS_NPC(vch) || !IS_AFFECTED(vch, AFF_WIZI))
	 && !saves_spell(ch->level, ch, vch, DAM_OTHER))
	{
	    send_to_char("You stumble and fall to the ground!\n\r", vch);
	    act("$n stumbles and falls to the ground!", vch, NULL, NULL, TO_ROOM);
	    switch_position(vch,  POS_RESTING);
	    WAIT_STATE(vch, UMAX(vch->wait, PULSE_VIOLENCE * 2));
	}
    }

    for (dir = 0; dir < 6; dir++)
	if (ch->in_room->exit[dir] && (pRoom = ch->in_room->exit[dir]->u1.to_room))
	{
	    act("A powerful shockwave rips through the area!", pRoom->people, NULL, NULL, TO_CHAR);
	    act("A powerful shockwave rips through the area!", pRoom->people, NULL, NULL, TO_ROOM);

    	    for (vch = pRoom->people; vch; vch = vch->next_in_room)
    	    {
		if (!is_same_group(ch, vch)
	         && !is_safe(ch, vch) 
	 	 && !IS_IMMORTAL(vch)
	 	 && (!IS_NPC(vch) || !IS_AFFECTED(vch, AFF_WIZI))
	 	 && !saves_spell(ch->level, ch, vch, DAM_OTHER))
		{
	    	    send_to_char("You stumble and fall to the ground!\n\r", vch);
	    	    act("$n stumbles and falls to the ground!", vch, NULL, NULL, TO_ROOM);
	    	    switch_position(vch,  POS_RESTING);
	    	    WAIT_STATE(vch, UMAX(vch->wait, PULSE_VIOLENCE * 2));
		}
	    }
    }
}

void do_inertialstrike(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    OBJ_DATA *wield, *mantle;
    int chance, dam;
    char buf[MAX_STRING_LENGTH];
    bool fake = FALSE;

    if ((chance = get_skill(ch, gsn_inertial_strike)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }
    
    wield = get_eq_char(ch, WEAR_WIELD);

    if (!wield)
    {
	send_to_char("You must be wielding a weapon to deal an inertial strike.\n\r", ch);
	return;
    }

    if (!IS_WEAPON_STAT(wield,WEAPON_TWO_HANDS))
	if (IS_PAFFECTED(ch,AFF_TWOHAND))
	    fake = TRUE;
	else
	{
	    send_to_char("You must be wielding a two-handed weapon to deal an inertial strike.\n\r", ch);
	    return;
	}

    if (argument[0] == '\0')
    {
	if (ch->fighting)
	    victim = ch->fighting;
	else
	{
	    send_to_char("Whom do you wish to strike?\n\r", ch);
	    return;
	}
    }
    else
    {
	if ((victim = get_char_room(ch, argument)) == NULL)
	{
	    send_to_char("You don't see them here.\n\r", ch);
	    return;
	}
    }

    if (victim == ch)
    {
	send_to_char("You can't gather inertia to strike yourself.\n\r",ch);
	return;
    }

    if (ch->mana < skill_table[gsn_inertial_strike].min_mana)
    {
	send_to_char("You lack the energy to gather inertia.\n\r", ch);
	return;
    }

    expend_mana(ch, skill_table[gsn_inertial_strike].min_mana);

    if (is_safe(ch, victim))
	return;

    if (!check_reach(ch, victim, REACH_EASY, PULSE_VIOLENCE))
	return;

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_inertial_strike].beats));

    if (!IS_NPC(victim))
	if (ch->fighting == NULL || victim->fighting != ch)
	{
	    sprintf(buf, "Help! %s is attacking me!", PERS(ch, victim));
	    do_autoyell(victim, buf);
	}

    chance -= (get_skill(victim,gsn_dodge)/10);
    if (IS_AFFECTED(ch,AFF_HASTE) || IS_SET(ch->off_flags,OFF_FAST))
	chance += 10;
    if (IS_AFFECTED(victim,AFF_HASTE) || IS_SET(victim->off_flags,OFF_FAST))
	chance -= 10;
    if (IS_AFFECTED(ch,AFF_SLOW))
	chance -= 10;
    if (IS_AFFECTED(victim,AFF_SLOW))
	chance += 10;
    act("You heft $p, gathering inertia for a devastating blow!",ch,wield,NULL,TO_CHAR);
    act("$n hefts $p, gathering inertia for a devastating blow!",ch,wield,NULL,TO_ROOM);

    if (number_percent() > chance)
    {
	act("You swing violently at $N, but miss!", ch, NULL, victim, TO_CHAR);
	act("$n swings at you violently, but you avoid the blow!", ch, NULL, victim, TO_VICT);
	act("$n swings violently at $N, but $E avoids the blow!", ch, NULL, victim, TO_NOTVICT);
	damage(ch, victim, 0, gsn_inertial_strike, DAM_BASH, FALSE);
	check_improve(ch,victim, gsn_inertial_strike, FALSE, 1);
	return;
    }

    check_improve(ch,victim, gsn_inertial_strike, TRUE, 1);

    if ((mantle = get_eq_char(victim, WEAR_ABOUT))
     && (mantle->pIndexData->vnum == OBJ_VNUM_MANTLE_EARTH)
     && (number_bits(2) != 0))
    {
	act("$N's mantle of earth stops your inertial strike!", ch, NULL, victim, TO_CHAR);
	act("Your mantle of earth stops $n's inertial strike!", ch, NULL, victim, TO_VICT);
	act("$N's mantle of earth stops $n's inertial strike!", ch, NULL, victim, TO_NOTVICT);
	return;
    }

    dam = 0;
    dam += dice(ch->level/14 * wield->value[1], wield->value[2]) + GET_DAMROLL(ch);
    dam += (get_obj_weight(wield)/10 * 1.5);
    dam = round(dam * UMIN(get_weapon_skill_weapon(ch,wield),100)/100);

    if (fake)
	dam = round(dam * 3/4);

    damage_from_obj( ch, victim, wield, dam, gsn_inertial_strike, wield->value[3], TRUE );

    return;
}	    

void do_dive( CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    int chance;
    char buf[MAX_STRING_LENGTH];

    if ( (chance = get_skill(ch,gsn_dive)) == 0)
    {
        send_to_char("Huh?\n\r",ch);
        return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("Who do you wish to dive at?\n\r", ch);
	return;
    }

    if ((victim = get_char_room(ch, argument)) == NULL)
    {
	send_to_char("You don't see them here.\n\r", ch);
	return;
    }

    if (!is_flying(ch))
    {
	send_to_char("You must be flying to initiate a diving attack.\n\r", ch);
	return;
    }

    if (victim == ch)
    {
	send_to_char("How do you propose to dive at yourself?\n\r", ch);
	return;
    }

    if (ch->in_room->sector_type == SECT_UNDERWATER)
    {
	send_to_char("You cannot dive underwater.\n\r", ch);
	return;
    }

    if (is_safe(ch, victim))
	return;

    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
        chance -= 30;

    chance += (ch->level - victim->level);

    if (IS_NAFFECTED(victim, AFF_AGILITY) 
      && (number_percent() < get_skill(victim,gsn_agility)/2+((SIZE_MEDIUM - victim->size)*10)))
        chance /= 2;

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_dive].beats));

    chance = (chance * 17) / 20;

    if (!IS_NPC(victim))
    {
        sprintf( buf, "Help! %s is diving at me!", PERS(ch, victim));
        do_autoyell( victim, buf );
    }

    if (number_percent() < chance)
    {
	act("You dive at $N, knocking $M off balance!", ch, NULL, victim, TO_CHAR);
	act("$n dives down at you, knocking you off balance!", ch, NULL, victim, TO_VICT);
	act("$n dives down at $N, knocking $M off balance!", ch, NULL, victim, TO_NOTVICT);

	check_improve(ch,victim, gsn_dive, TRUE, 1);
	WAIT_STATE(victim, UMAX(victim->wait, (2*PULSE_VIOLENCE)+number_bits(2)-1));
	damage(ch, victim, 25 + (ch->level / 2) + GET_DAMROLL(ch), gsn_dive, DAM_BASH, TRUE);
    }
    else
    {
	act("$N evades your dive!", ch, NULL, victim, TO_CHAR);
	act("You avoid $n's dive!", ch, NULL, victim, TO_VICT);
	act("$N evades $n's dive!", ch, NULL, victim, TO_NOTVICT);
	check_improve(ch, victim,gsn_dive, FALSE, 1);
        damage(ch, victim, 0, gsn_dive, DAM_BASH, TRUE);
    }

    check_killer(ch, victim);

    return;
}	

void do_flay( CHAR_DATA *ch, char *argument)
{

    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *wield;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int dam, chance, result;

    if ((chance = get_skill(ch, gsn_flay)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }


    chance = chance * 4 / 5;

    if (argument[0] == '\0')
    {
        if ((victim = ch->fighting) == NULL)
	{
            send_to_char("Flay whom?\n\r", ch);
            return;
        }
    }
    else if ((victim = get_char_room(ch, argument)) == NULL)
    {
         send_to_char("Flay whom?\n\r", ch);
         return;
    }
    if (victim == ch)
    {
        send_to_char("You can't flay yourself.\n\r",ch);
        return;
    }


    if (is_safe(ch,victim))
        return;
    if ((wield = get_eq_char(ch, WEAR_WIELD)) == NULL)
    {
        send_to_char("You must wield a whip to flay someone.\n\r", ch);
        return;
    }

    if (wield->value[0] != WEAPON_WHIP)
        if ((wield = get_eq_char(ch, WEAR_DUAL_WIELD)) == NULL
        || (wield->value[0] != WEAPON_WHIP))
        {
            send_to_char("You must wield a whip to flay someone.\n\r", ch);
            return;
        }

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;

    if (IS_AFFECTED(victim, AFF_HASTE)
    || (IS_NPC(victim) && IS_SET(victim->off_flags, OFF_FAST)))
        chance = chance * 3 / 4;
    
    chance += (ch->level - victim->level);
    chance = URANGE(5, chance, 95);
    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_flay].beats));
 
    if ((result = number_percent()) < chance)
    {
        act("$n flays $N with $p!", ch, wield, victim, TO_NOTVICT);
        act("You flay $N with $p!", ch, wield, victim, TO_CHAR);
        act("$n flays you with $p!", ch, wield, victim, TO_VICT);
        check_improve(ch,victim,gsn_flay,TRUE,1);
        af.where     = TO_AFFECTS;
        af.type      = gsn_flay;
        af.level     = ch->level;
        af.duration  = ch->level/8;
        af.location  = APPLY_STR;
        af.modifier  = number_bits(2) ? -1 : -2;
        af.bitvector = 0;
        affect_to_char( victim, &af );
     }
     else
     {
        act("$n tries to flay $N with $p, but $N avoids the stroke!", ch, wield, victim, TO_NOTVICT);
        act("You try to flay $N with $p, but $N avoids the stroke!", ch, wield,victim, TO_CHAR);
        act("$n tries to flay you with $p, but you avoid the stroke!", ch, wield, victim, TO_VICT);
        check_improve(ch,victim,gsn_flay,FALSE,1);
        damage(ch, victim, 0, gsn_flay, DAM_NONE, FALSE);
        return;
     }

     if (!IS_NPC(victim))
        if (ch->fighting == NULL || victim->fighting != ch)
        {
            sprintf(buf, "Help! %s is flaying me!", PERS(ch, victim));
            do_autoyell(victim, buf);
        }

     dam = dice(wield->value[1], wield->value[2]) + GET_DAMROLL(ch);
     damage_from_obj(ch, victim, wield, dam, gsn_flay, wield-> value[3], TRUE );
     return;
}

void do_sortie( CHAR_DATA *ch, char *argument)
{

    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *vch, *vch_next;
    int chance;

    if ((chance = get_skill(ch, gsn_sortie)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }
    if (is_affected(ch,gsn_sortie))
    {
        send_to_char("You aren't ready to lead another sortie.\n\r",ch);
        return;
    }
    if (!ch->fighting)
    {
        send_to_char("You need to be in battle to lead a sortie.\n\r",ch);
        return;
    }

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_sortie].beats));
    chance = get_skill(ch,gsn_sortie);
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
        chance += 15;
    if (IS_AFFECTED(ch,AFF_SLOW))
        chance -= 15;
    if (number_percent() < chance)
    {
        check_improve(ch,NULL,gsn_sortie,TRUE,1);
    
        af.where        = TO_NAFFECTS;
        af.type         = gsn_sortie;
        af.level        = ch->level;
        af.duration     = 6;
        af.location     = 0;
        af.bitvector    = AFF_RALLY;
        af.modifier     = 0;
        affect_to_char(ch, &af);

        send_to_char("You dart into the midst of combat, yelling your battlecry!\n\r",ch);
        act("$n darts into the midst of combat, leading a sortie and yelling $s battlecry!",ch,NULL,NULL,TO_ROOM);

        if (ch->battlecry != NULL)
            do_yell(ch, ch->battlecry);
        else
            do_yell(ch, "For the light!");
        for (vch = ch->in_room->people ; vch != NULL; vch = vch_next)
        {
            vch_next = vch->next_in_room;
            if (vch->fighting == ch)
            {
                act("$n rushes toward you, making a sortie into your defenses!",ch, NULL,vch,TO_VICT);
                one_hit( ch, vch, TYPE_UNDEFINED, HIT_PRIMARY );
            }
        }
    }    
    else 
    {
        send_to_char("You try to break through your enemy's defenses, but fail.\n\r",ch);
        check_improve(ch,NULL,gsn_sortie,FALSE,1);
        return;
    }
}

void do_gash(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    OBJ_DATA *weapon;
    int chance,dam,skill;
    AFFECT_DATA naf;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];

    one_argument(argument,arg);

    if ((skill = chance = get_skill(ch,gsn_gash)) == 0)
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }	
    
    if (arg[0] == '\0')
    {
	victim = ch->fighting;
	if (victim == NULL)
	{
	    send_to_char("But you aren't fighting anyone!\n\r",ch);
	    return;
	}
    }
    else if ((victim = get_char_room(ch,arg)) == NULL)
    {
	send_to_char("They aren't here.\n\r",ch);
	return;
    }
    if (is_safe(ch,victim))
	return;

    if (victim == ch)
    {
	send_to_char("If you want to gash yourself, try 'suicide yes'.\n\r",ch);
	return;
    }

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
	act("$N is your beloved master.",ch,NULL,victim,TO_CHAR);
	return;
    }

    if ((weapon = get_eq_char(ch,WEAR_WIELD)) == NULL)
    {
	send_to_char("You must use a sword to gash.\n\r",ch);
	return;	
    }
    
    if (weapon->value[0] != WEAPON_SWORD)
    {
	send_to_char("You must use a sword to gash.\n\r",ch);
	return;
    }

    if (!check_reach(ch, victim, REACH_EASY, PULSE_VIOLENCE))
	return;
    
    chance += get_curr_stat(ch,STAT_STR)/2;
    chance += get_curr_stat(ch,STAT_DEX)/2;
    
    chance -= 2*get_curr_stat(victim,STAT_DEX);
    chance += GET_AC(victim,AC_SLASH)/25;
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
	chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
	chance -= 30;
    chance += (ch->level - victim->level);
    
    chance = URANGE(5,chance,95);
    if (number_percent() < chance)
    {
	if ((ch->fighting != victim) && (!IS_NPC(victim)))
	{
	    sprintf(buf,"Help!  %s tried to gash me!", PERS(ch,victim));
	    do_autoyell(victim,buf);
        }
	act("$n gashes you with $p!",ch,weapon,victim,TO_VICT);
	act("You gash $N with $p!",ch,weapon,victim,TO_CHAR);
	act("$n gashes $N with $p.", ch,weapon,victim,TO_NOTVICT);
	check_improve(ch,victim,gsn_gash,TRUE,1);
	WAIT_STATE(ch,skill_table[gsn_gash].beats);
        if (!is_affected(victim,gsn_gash))
        {
	    naf.where = TO_OAFFECTS;
	    naf.type = gsn_gash;
	    naf.level = ch->level;
	    naf.duration = ch->level/10;
	    naf.modifier = 0;
	    naf.location = APPLY_NONE;
	    naf.bitvector = AFF_BLEEDING;
	    affect_to_char(victim,&naf);
        }
    
        if (weapon->pIndexData->new_format)
  	    dam = dice(weapon->value[1],weapon->value[2]) * skill/100*3/2;
        else
	    dam = number_range(weapon->value[1] * skill/100, weapon->value[2] * skill/100)*3/2;
    
        dam += check_extra_damage(ch,dam,weapon);
    
        if (!IS_AWAKE(victim))
	    dam *= 2;
    
        dam += GET_DAMROLL(ch) * UMIN(100,skill)/100;

        if (dam <= 0)
	    dam = 1;

        damage_from_obj(ch,victim,weapon,dam,gsn_gash,weapon->value[3],TRUE);
    }
    else
    {
	if ((ch->fighting != victim) && (!IS_NPC(victim)))
	{
	    sprintf(buf,"Help!  %s tried to gash me!", PERS(ch,victim));
	    do_autoyell(victim,buf);
	}
	damage(ch,victim,0,gsn_gash,weapon->value[3],FALSE);
	act("You swing at $N and miss.", ch,NULL,victim,TO_CHAR);
	act("$n swings at $N and misses.",ch,NULL,victim,TO_NOTVICT);
	act("$n swings at you and misses.",ch,NULL,victim,TO_VICT);
	check_improve(ch,victim,gsn_gash,FALSE,1);
	WAIT_STATE(ch,skill_table[gsn_gash].beats);
   }
    check_killer(ch,victim);
}

void do_cower(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim, *vch = NULL, *fch;
    int chance = 0;

    /* The cower social must live on!! */
    /* Fuck the cower social! -Kestrel */

    if ((chance = get_skill(ch, gsn_cower)) == 0)
    {    
	send_to_char("Huh?\n\r", ch);
	return;	
    }

    if (!ch->fighting)
    {
        send_to_char("You are not fighting.\n\r", ch);
	return;
    }

    if (ch->fighting->fighting == ch->fighting)
	vch = ch->fighting;
    else
    	{
		for (fch = ch->in_room->people; fch; fch = fch->next_in_room)
		   if (fch->fighting == ch)
		   {
			vch = fch;
			break;
		   }
		if (!vch)
			{
		    send_to_char("But you are not under attack!\n\r", ch);
		    return;
			}
    	}

    if (IS_AFFECTED(ch, AFF_BERSERK)
     || is_affected(ch, gsn_zeal)
     || is_affected(ch, gsn_frenzy)
     || is_affected(ch, gsn_adrenaline_rush)
     || is_affected(ch, gsn_rage)
     || is_affected(ch, gsn_savagery))
    {
	send_to_char("You are too frenzied to cower from this foe!\n\r", ch);
	return;
    }
    if (IS_NAFFECTED(ch, AFF_ASHURMADNESS))
    {
	send_to_char("You must eat!\n\r",ch);
	return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("Who do you wish to cower behind?\n\r", ch);
	return;
    }

    if ((victim = get_char_room(ch, argument)) == NULL)
    {
	send_to_char("You don't see them here.\n\r", ch);
	return;
    }

    if (!is_same_group(ch, victim))
    {
	act("But $E isn't part of your group!", ch, NULL, victim, TO_CHAR);
	return;
    }

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_cower].beats));

    chance = (chance * 4) / 5;

    chance += ((get_curr_stat(ch, STAT_DEX) - 18) * 2);

    if (!IS_NPC(ch) && BIT_GET(ch->pcdata->traits, TRAIT_COWARD))
	chance += 10;

    if (number_percent() > chance)
    {
	act("You attempt to cower behind $N, but fail to elude your foe.", ch, NULL, victim, TO_CHAR);
	act("$n attempts to cower behind $N, but fails to elude $s foe.", ch, NULL, victim, TO_NOTVICT);
	act("$n attempts to cower behind you, but fails to elude $s foe.", ch, NULL, victim, TO_VICT);
	check_improve(ch, victim,gsn_cower, FALSE, 1);
	return;
    }

    if (number_percent() < get_skill(ch->fighting,gsn_ferocity))
    {
	act("$N tries to cower from you, but you ignore $S feeble attempt.",ch->fighting,NULL,ch,TO_CHAR);
	act("You can't get away from $n's ferocious assault!",ch->fighting,NULL,ch,TO_VICT);
	act("$N can't get away from $n's ferocious assault!",ch->fighting,NULL,ch,TO_NOTVICT);
	check_improve(ch->fighting,ch,gsn_ferocity,TRUE,1);
	return;
    }
    else
	check_improve(ch->fighting,ch,gsn_ferocity,FALSE,1);

    act("You cower behind $N, and your foe shifts its attention to $M.", ch, NULL, victim, TO_CHAR);
    act("$n runs behind $N, shifting $s foe's attention.", ch, NULL, victim, TO_NOTVICT);
    act("$n cowers behind you, and $s foe begins attacking you!", ch, NULL, victim, TO_VICT);
    check_improve(ch,victim, gsn_cower, TRUE, 1);

    stop_fighting(vch);
    stop_fighting(ch);
    multi_hit(vch, victim, TYPE_UNDEFINED);

   return;
}

void do_feint(CHAR_DATA *ch, char *argument)
{
    int chance;
    bool ctf=FALSE;
    CHAR_DATA *vch;

    if ((chance = get_skill(ch, gsn_feint)) == 0)
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }
    
    if (!ch->fighting)
    {
	send_to_char("You must be fighting to feint.\n\r", ch);
	return;
    }

    if (ch->mana - 5 < 0)
    {
	send_to_char("You're too tired to distract your opponent.\n\r",ch);
	return;
    }
 
    for (vch = ch->in_room->people; vch != NULL; vch=vch->next_in_room)
    {
	if (vch->fighting == ch)
	    ctf = TRUE;
    } 

    if (!ctf)
    {
	send_to_char("You can only distract those fighting you.\n\r",ch);
	return;
    }
    ctf=FALSE;

    chance = chance * 3 / 4;

    chance += get_curr_stat(ch, STAT_DEX) + get_curr_stat(ch, STAT_CHR)  - 30;

    chance -= get_skill(ch->fighting, gsn_feint) / 2;

    if (IS_AFFECTED(ch, AFF_HASTE))
	chance += 10;
   
    if (IS_AFFECTED(ch, AFF_SLOW))
	chance -= 20;
    if (!IS_NPC(ch->fighting) 
      && BIT_GET(ch->fighting->pcdata->traits, TRAIT_CYNIC))
	ctf = TRUE;
    expend_mana(ch, 5);
    if (number_percent() > chance - ctf ? 20 : 0)
    {
	act("You attempt to distract $N with a quick feint, but fail.", ch, NULL, ch->fighting, TO_CHAR);
	act("$n attempts to distract $N with a quick feint, but fails.", ch, NULL, ch->fighting, TO_NOTVICT);
	act("$n attempts to distract you with a quick feint, but you see through the ploy.", ch, NULL, ch->fighting, TO_VICT);
	check_improve(ch, ch->fighting, gsn_feint, FALSE, 2);
    }
    else
    {
	act("You distract $N with a quick feint.", ch, NULL, ch->fighting, TO_CHAR);
	act("$n distracts $N with a quick feint.", ch, NULL, ch->fighting, TO_NOTVICT);
	act("$n catches you off-guard with a quick feint.", ch, NULL, ch->fighting, TO_VICT);
	check_improve(ch, ch->fighting, gsn_feint, TRUE, 2);
	ch->fighting->lost_att++;
    }
    chance += get_skill(ch->fighting, gsn_feint) / 2;
    
    chance /= 2;
    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
    {
	ctf = FALSE;
	if (vch && ch != vch && vch->fighting == ch && ch->fighting != vch)
	{
	    if (!IS_NPC(vch) && BIT_GET(vch->pcdata->traits, TRAIT_CYNIC))
		ctf=TRUE;
	    if (number_percent() < chance - (get_skill(vch,gsn_feint)/2 - ctf ? 20 : 0))
	    {
		if (ch->mana - 5 < 0)
		{
		    act("You're too tired to distract any more opponents.",ch,NULL,vch,TO_CHAR);
		    break;
		}
		else
		{
		    act("$N is also distracted by your feint.", ch, NULL, vch, TO_CHAR);
		    act("$n distracts $N with a quick feint.", ch, NULL, vch, TO_NOTVICT);
		    act("$n catches you off-guard with a quick feint.", ch, NULL, vch, TO_VICT);
		    expend_mana(ch, 5);
		    vch->lost_att++;
		}
	    }
	}
    }
    
    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_feint].beats));

    return;
}

void do_beg(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *vch;
    bool can_mercy = TRUE;

    if (IS_NPC(ch) && !ch->desc)
	return;

    if (!ch->fighting)
    {
	send_to_char("You must be fighting to beg for mercy.\n\r", ch);
	return;
    }

    if (ch->fighting->fighting != ch)
    {
	send_to_char("You must be attacking one of your attackers to beg mercy.\n\r", ch);
	return;
    }

    act("You quickly drop to your knees, begging for mercy.", ch, NULL, NULL, TO_CHAR);
    act("$n drops to $s knees, begging for mercy.", ch, NULL, NULL, TO_ROOM);

    switch_position(ch, POS_RESTING);
    WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));


    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
	if ((vch->fighting == ch) && ((!IS_NPC(vch) && !IS_SET(vch->nact, PLR_MERCY_BEG)) || IS_NPC(vch)))
	{
	    can_mercy = FALSE;
	    break;
	}

    if (can_mercy)
    {
	ch->mercy_from = ch->fighting;
	ch->fighting->mercy_to = ch;
	stop_fighting_all(ch);

	act("$N stands in front of you, weapon poised, granting you mercy.", ch, NULL, ch->mercy_from, TO_CHAR);
	act("$N stands in front of $n, weapon poised, granting $m mercy.", ch, NULL, ch->mercy_from, TO_NOTVICT);
	act("You stand in front of $n, weapon poised, granting $m mercy.", ch, NULL, ch->mercy_from, TO_VICT);
    }

    return;
}

void do_grantmercy(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
	return;

    if (!ch->mercy_to)
    {
	send_to_char("You are not currently granting anyone mercy.\n\r", ch);
	return;
    }

    act("You step back, lowering your weapon away from $N.", ch, NULL, ch->mercy_to, TO_CHAR);
    act("$n steps back, lowering $s weapon away from $N.", ch, NULL, ch->mercy_to, TO_NOTVICT);
    act("$n steps back, lowering $s weapon away from you.", ch, NULL, ch->mercy_to, TO_VICT);

    free_string(ch->mercy_to->pose);
    free_string(ch->pose);
    ch->mercy_to->mercy_from = NULL;
    ch->mercy_to = NULL;
}

void do_mercy(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
	return;

    if (argument[0] == '\0')
    {
	sprintf(buf, "You currently %s granting mercy to defeated opponents.\n\r", IS_SET(ch->nact, PLR_MERCY_DEATH) ? "{WARE{x" : "are {WNOT{x");
	send_to_char(buf, ch);

	sprintf(buf, "You currently %s allowing opponents to beg for mercy.\n\r", IS_SET(ch->nact, PLR_MERCY_BEG) ? "{WARE{x" : "are {WNOT{x");
	send_to_char(buf, ch);

	sprintf(buf, "You currently %s slay a mercied opponent if you are interrupted.\n\r", IS_SET(ch->nact, PLR_MERCY_FORCE) ? "{WWILL{x" : "will {WNOT{x");
	send_to_char(buf, ch);
    }
    else if (!str_cmp(argument, "death"))
    {
	if (IS_SET(ch->nact, PLR_MERCY_DEATH))
	{
	    send_to_char("You will no longer grant mercy to a defeated opponent.\n\r", ch);
	    REMOVE_BIT(ch->nact, PLR_MERCY_DEATH);
	}
	else
	{
	    send_to_char("You will now grant mercy to a defeated opponent.\n\r", ch);
	    SET_BIT(ch->nact, PLR_MERCY_DEATH);
	}
    }
    else if (!str_cmp(argument, "beg"))
    {
	if (IS_SET(ch->nact, PLR_MERCY_BEG))
	{
	    send_to_char("You will no longer grant mercy to a begging opponent.\n\r", ch);
	    REMOVE_BIT(ch->nact, PLR_MERCY_BEG);
	}
	else
	{
	    send_to_char("You will now grant mercy to a begging opponent.\n\r", ch);
	    SET_BIT(ch->nact, PLR_MERCY_BEG);
	}
    }
    else if (!str_cmp(argument, "force"))
    {
	if (IS_SET(ch->nact, PLR_MERCY_FORCE))
	{
	    send_to_char("You will no longer kill an opponent under mercy when interrupted.\n\r", ch);
	    REMOVE_BIT(ch->nact, PLR_MERCY_FORCE);
	}
	else
	{
	    send_to_char("You will now kill an opponent under mercy if interrupted.\n\r", ch);
	    SET_BIT(ch->nact, PLR_MERCY_FORCE);
	}
    }
    else
    {
	send_to_char("Invalid argument.\n\r", ch);
	send_to_char("Valid arguments are: <none>, force, beg, death\n\r", ch);
    }

    return;
}

void mob_class_update(CHAR_DATA *ch)
{
    OBJ_DATA *obj, *wield;
    char buf[MAX_STRING_LENGTH];

    if (!IS_NPC(ch) || !IS_SET(ch->nact, ACT_CLASSED))
	return;

    if (ch->class_num == CLASS_FIGHTER)
    {
	if (!ch->fighting)
	{
	    if (!is_affected(ch, gsn_readiness)
	     && (skill_table[gsn_readiness].skill_level[CLASS_FIGHTER] <= ch->level))
	        do_readiness(ch, "");
	    else if ((ch->hit < (ch->max_hit - ch->level)) && !is_affected(ch, gsn_bandage)
	     && (skill_table[gsn_bandage].skill_level[CLASS_FIGHTER] <= ch->level))
	        do_bandage(ch, "");
	    else if (ch->tracking && ch->in_room && ch->in_room != ch->tracking->in_room
	     && !IS_SET(ch->act, ACT_SENTINEL))
	    {
            ClearPath(ch);
            RoomPath roomPath(*ch->in_room, *ch->tracking->in_room, ch, 200);
            if (roomPath.Exists())
	    	{
                AssignNewPath(ch, roomPath);

                // Move twice towards the target
                for (unsigned int i(0); i < 2; ++i)
                {
                    StepAlongPath(ch);
        		    if (ch->in_room == ch->tracking->in_room)
	        	    {
		        	    mob_class_aggr(ch, ch->tracking);
    		        	return;
        		    }
                }
		    }
	    }
	    else if (ch->tracking && ch->in_room && ch->in_room == ch->tracking->in_room)
	    {
		mob_class_aggr(ch, ch->tracking);
		return;
	    }
	}
	else
	{
	    if ((ch->hit <= (ch->max_hit - ch->level)) && !is_affected(ch, gsn_rally)
	     && (skill_table[gsn_rally].skill_level[CLASS_FIGHTER] <= ch->level))
		do_rally(ch, "");
	    else if (((obj = get_eq_char(ch->fighting, WEAR_WIELD)) != NULL)
	     && can_see_obj(ch, obj) && !IS_OBJ_STAT(obj, ITEM_NOREMOVE)
	     && (skill_table[gsn_disarm].skill_level[CLASS_FIGHTER] <= ch->level))
		do_disarm(ch, "");
	    else if (!IS_NPC(ch->fighting)
	     && !is_affected(ch->fighting, gsn_protective_shield)
	     && !IS_NAFFECTED(ch->fighting, AFF_AGILITY)
	     && !is_affected(ch->fighting, gsn_astralform)
	     && !is_affected(ch->fighting, gsn_form_of_the_panther)
	     && !is_stabilized(*ch->fighting)
	     && !is_affected(ch->fighting, gsn_lessercarapace)
	     && !is_affected(ch->fighting, gsn_greatercarapace)
	     && !is_affected(ch->fighting, gsn_deflection)
	     && !is_affected(ch->fighting, gsn_barkskin)
	     && !is_affected(ch->fighting, gsn_flameshield)
         && !is_affected(ch->fighting, gsn_lambentaura)
	     && !is_affected(ch->fighting, gsn_windwall)
         && !is_affected(ch->fighting, gsn_mistralward)
	     && (skill_table[gsn_bash].skill_level[CLASS_FIGHTER] <= ch->level))
		do_bash(ch, "");
	    else if (!IS_NPC(ch->fighting) && !is_flying(ch->fighting))
		do_trip(ch, "");
	    else if ((ch->fighting->fighting != ch)
	     && (skill_table[gsn_flank].skill_level[CLASS_FIGHTER] <= ch->level))
		do_flank(ch, "");
	    else if ((wield = get_eq_char(ch, WEAR_WIELD))
	     && (wield->value[0] == WEAPON_STAFF)
	     && (skill_table[gsn_pugil].skill_level[CLASS_FIGHTER] <= ch->level))
		do_pugil(ch, "");
	    else if ((wield = get_eq_char(ch, WEAR_WIELD))
	     && (wield->value[0] == WEAPON_SPEAR)
	     && (skill_table[gsn_lunge].skill_level[CLASS_FIGHTER] <= ch->level))
		do_lunge(ch, "");
	    else if (wield && (wield->value[0] == WEAPON_SWORD)
	     && (skill_table[gsn_pommel].skill_level[CLASS_FIGHTER] <= ch->level))
		do_pommel(ch, "");
	    else
		do_kick(ch, "");
	}
    }
    else if (ch->class_num == CLASS_BARBARIAN)
    {
	if (!ch->fighting)
	{
	    if (!is_affected(ch, gsn_savagery) && !IS_AFFECTED(ch, AFF_CALM)
	     && (ch->mana >= 50)
	     && (skill_table[gsn_savagery].skill_level[ch->class_num] <= ch->level))
	    {
	        do_savagery(ch, "");
		return;
	    }
	    else if (ch->in_room && (ch->mana >= skill_table[gsn_fetish].min_mana)
	     && (skill_table[gsn_fetish].skill_level[ch->class_num] <= ch->level))
	    {
		int i = 0;
		for (obj = ch->in_room->contents; obj; obj = obj->next_content)
		{
		    if ((obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC)
		     || (obj->pIndexData->vnum == OBJ_VNUM_CORPSE_NPC))
			i++;

		    if ((obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC)
		     && !str_cmp(obj->killed_by, ch->short_descr)
		     && !IS_SET(obj->value[1], CORPSE_DESTROYED)
		     && !IS_SET(obj->value[1], CORPSE_MISSING_HEAD)
		     && (!IS_SET(obj->value[1], CORPSE_MISSING_EAR1)
		      || !IS_SET(obj->value[1], CORPSE_MISSING_EAR2))
		     && !IS_SET(obj->value[1], CORPSE_FETISHED))
		    {
			sprintf(buf, "%d.corpse", i);
			do_fetish(ch, buf);

			for (obj = ch->carrying; obj; obj = obj->next_content)
			    if (obj->pIndexData->vnum == OBJ_VNUM_FETISH)
			    {
				if (!obj->worn_on)
				    if (!get_eq_char(ch, WEAR_NECK_1))
				        equip_char(ch, obj, WORN_NECK_1);
				    else if (!get_eq_char(ch, WEAR_NECK_2))
					equip_char(ch, obj, WORN_NECK_2);
				break;
			    }

			return;
		    }
		}
	    }

	    if (ch->tracking && ch->in_room && ch->in_room != ch->tracking->in_room
	     && !IS_SET(ch->act, ACT_SENTINEL))
	    {
            ClearPath(ch);
            RoomPath roomPath(*ch->in_room, *ch->tracking->in_room, ch, 200);
            if (roomPath.Exists())
            {
                AssignNewPath(ch, roomPath);

                // Move twice towards the target
                for (unsigned int i(0); i < 2; ++i)
                {
                    StepAlongPath(ch);
                    if (ch->in_room == ch->tracking->in_room)
                    {
                        mob_class_aggr(ch, ch->tracking);
                        return;
                    }
                }
            }
	    }
	    else if (ch->tracking && ch->in_room && ch->in_room == ch->tracking->in_room)
	    {
		mob_class_aggr(ch, ch->tracking);
		return;
	    }
	}
	else
	{
	    if (!is_affected(ch, gsn_rage)
	     && (skill_table[gsn_rage].skill_level[ch->class_num] <= ch->level))
		do_rage(ch, "");
	    else if (!IS_AFFECTED(ch->fighting, AFF_BLIND) && !IS_SET(ch->fighting->imm_flags, IMM_BLIND)
	     && (skill_table[gsn_dirt].skill_level[ch->class_num] <= ch->level))
		do_dirt(ch, "");
	    else if (!IS_AFFECTED(ch, AFF_BERSERK) && ((100 * ch->hit / ch->max_hit) < 30)
	     && (skill_table[gsn_berserk].skill_level[ch->class_num] <= ch->level))
		do_berserk(ch, "");
	    else if ((wield = get_eq_char(ch, WEAR_WIELD))
	     && (wield->value[0] == WEAPON_AXE)
	     && (skill_table[gsn_hew].skill_level[ch->class_num] <= ch->level))
		do_hew(ch, "");
	    else if (wield && (wield->value[0] == WEAPON_MACE)
	     && ((obj = get_eq_char(ch->fighting, WEAR_DUAL_WIELD)) != NULL)
	     && !IS_NAFFECTED(ch->fighting, AFF_ARMSHATTER)
	     && (skill_table[gsn_boneshatter].skill_level[ch->class_num] <= ch->level))
	        do_boneshatter(ch, "arm");
	    else if (wield && (wield->value[0] == WEAPON_MACE)
	     && (skill_table[gsn_bludgeon].skill_level[ch->class_num] <= ch->level))
		do_bludgeon(ch, "");
	    else if (!is_affected(ch->fighting, gsn_astralform)
	     && !is_affected(ch->fighting, gsn_grapple)
	     && !IS_NAFFECTED(ch->fighting, AFF_AGILITY)
	     && (skill_table[gsn_grapple].skill_level[ch->class_num] <= ch->level))
		do_grapple(ch, "");
	    else if (!is_affected(ch->fighting, gsn_protective_shield)
	     && !is_affected(ch->fighting, gsn_astralform)
	     && !IS_NAFFECTED(ch->fighting, AFF_AGILITY)
	     && !is_affected(ch->fighting, gsn_form_of_the_panther)
	     && !is_stabilized(*ch->fighting)
	     && !is_affected(ch->fighting, gsn_lessercarapace)
	     && !is_affected(ch->fighting, gsn_greatercarapace)
	     && !is_affected(ch->fighting, gsn_deflection)
	     && !is_affected(ch->fighting, gsn_barkskin)
	     && !is_affected(ch->fighting, gsn_flameshield)
         && !is_affected(ch->fighting, gsn_lambentaura)
	     && !is_affected(ch->fighting, gsn_windwall)
         && !is_affected(ch->fighting, gsn_mistralward)
	     && (skill_table[gsn_bash].skill_level[ch->class_num] <= ch->level))
		do_bash(ch, "");
	    else if (!is_affected(ch, gsn_warcry) && !is_affected(ch->fighting, gsn_warcry)
	     && (skill_table[gsn_dirt].skill_level[ch->class_num] <= ch->level))
		do_warcry(ch, "");
	    else
		do_kick(ch, "");
	}
    }
}

void mob_class_aggr(CHAR_DATA *ch, CHAR_DATA *victim)
{
    OBJ_DATA *obj, *wield;
    char arg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int i = 0;

    if (ch->class_num == CLASS_AIR_TEMPLAR)
    {
        bool imm_lightning = (get_resist(victim, DAM_LIGHTNING) == 100);

        if (is_flying(ch) && (ch->in_room->sector_type != SECT_UNDERWATER)
         && (skill_table[gsn_dive].skill_level[ch->class_num] <= ch->level))
            do_dive(ch, victim->unique_name);
        else if (!imm_lightning && (ch->mana >= skill_table[gsn_ball_lightning].min_mana)
         && (skill_table[gsn_ball_lightning].skill_level[ch->class_num] <= ch->level))
        {
            sprintf(buf, "'ball lightning' %s", victim->unique_name);
            do_cast(ch, buf);
        }
        else if (!imm_lightning && (ch->mana >= skill_table[gsn_lightning_bolt].min_mana)
         && (skill_table[gsn_lightning_bolt].skill_level[ch->class_num] <= ch->level))
        {
            sprintf(buf, "'lightning bolt' %s", victim->unique_name);
            do_cast(ch, buf);
        }
        else if (!imm_lightning && (ch->mana >= skill_table[gsn_shocking_grasp].min_mana)
         && (skill_table[gsn_shocking_grasp].skill_level[ch->class_num] <= ch->level))
        {
            sprintf(buf, "'shocking grasp' %s", victim->unique_name);
            do_cast(ch, buf);
        }
        else
            multi_hit(ch, victim, TYPE_UNDEFINED);
    }
    else if (ch->class_num == CLASS_FIGHTER)
    {
        for (obj = ch->carrying; obj; obj = obj->next)
            if (!obj->worn_on)
            {
            i++;
            if ((obj->item_type == ITEM_WEAPON) && (obj->value[0] == WEAPON_SPEAR)
             && (get_obj_weight(obj) <= 190))
            {
                sprintf(arg, "%d. '%s'", i, victim->name);
                do_throw(ch, arg);
                return;
            }
            }

        if (!is_affected(ch->fighting, gsn_protective_shield)
         && !IS_NAFFECTED(ch->fighting, AFF_AGILITY)
         && !is_affected(ch->fighting, gsn_astralform)
         && !is_affected(ch->fighting, gsn_form_of_the_panther)
         && !is_stabilized(*ch->fighting)
         && !is_affected(ch->fighting, gsn_lessercarapace)
         && !is_affected(ch->fighting, gsn_greatercarapace)
         && !is_affected(ch->fighting, gsn_deflection)
         && !is_affected(ch->fighting, gsn_barkskin)
         && !is_affected(ch->fighting, gsn_flameshield)
         && !is_affected(ch->fighting, gsn_lambentaura)
         && !is_affected(ch->fighting, gsn_windwall)
         && !is_affected(ch->fighting, gsn_mistralward)
         && (skill_table[gsn_lunge].skill_level[CLASS_FIGHTER] <= ch->level))
        {
            one_argument(victim->name, arg);
            do_bash(ch, arg);
        }
        else if (!is_flying(ch->fighting))
        {
            one_argument(victim->name, arg);
            do_trip(ch, arg);
        }
        else if ((wield = get_eq_char(ch, WEAR_WIELD))
         && (wield->value[0] == WEAPON_SPEAR)
         && (skill_table[gsn_lunge].skill_level[CLASS_FIGHTER] <= ch->level))
        {
            one_argument(victim->name, arg);
            do_lunge(ch, arg);
        }
        else
            multi_hit(ch, victim, TYPE_UNDEFINED);
    }
    else if (ch->class_num == CLASS_BARBARIAN)
    {
        if (!IS_AFFECTED(victim, AFF_BLIND) && !IS_SET(victim->imm_flags, IMM_BLIND)
         && (skill_table[gsn_dirt].skill_level[ch->class_num] <= ch->level))
        {
            one_argument(victim->name, arg);
            do_dirt(ch, arg);
        }
            else if ((wield = get_eq_char(ch, WEAR_WIELD))
         && (wield->value[0] == WEAPON_AXE)
         && (victim->hit >= victim->max_hit * 75/100)
         && (skill_table[gsn_hew].skill_level[ch->class_num] <= ch->level))
        {
            one_argument(victim->name, arg);
            do_hew(ch, arg);
        }
        else if (wield && (wield->value[0] == WEAPON_MACE)
         && ((obj = get_eq_char(ch->fighting, WEAR_DUAL_WIELD)) != NULL)
         && !IS_NAFFECTED(ch->fighting, AFF_ARMSHATTER)
         && (skill_table[gsn_boneshatter].skill_level[ch->class_num] <= ch->level))
        {
            one_argument(victim->name, arg);
            sprintf(buf, "%s arm", arg);
            do_boneshatter(ch, buf);
        }
        else if (wield && (wield->value[0] == WEAPON_MACE)
         && (skill_table[gsn_bludgeon].skill_level[ch->class_num] <= ch->level))
        {
            one_argument(victim->name, arg);
            do_bludgeon(ch, arg);
        }
        else if (!is_affected(ch->fighting, gsn_protective_shield)
         && !is_affected(ch->fighting, gsn_astralform)
         && !IS_NAFFECTED(ch->fighting, AFF_AGILITY)
         && !is_affected(ch->fighting, gsn_form_of_the_panther)
         && !is_stabilized(*ch->fighting)
         && !is_affected(ch->fighting, gsn_lessercarapace)
         && !is_affected(ch->fighting, gsn_greatercarapace)
         && !is_affected(ch->fighting, gsn_deflection)
         && !is_affected(ch->fighting, gsn_barkskin)
         && !is_affected(ch->fighting, gsn_flameshield)
         && !is_affected(ch->fighting, gsn_lambentaura)
         && !is_affected(ch->fighting, gsn_windwall)
         && !is_affected(ch->fighting, gsn_mistralward)
         && (skill_table[gsn_bash].skill_level[ch->class_num] <= ch->level))
        {
            one_argument(victim->name, arg);
            do_bash(ch, arg);
        }
        else
            multi_hit(ch, victim, TYPE_UNDEFINED);
    }
    else
        multi_hit(ch, victim, TYPE_UNDEFINED);
}		

int effective_karma(CHAR_DATA & ch)
{
    int karma(0);
    if (IS_NPC(&ch))
    {
        if (IS_EVIL(&ch)) karma = REDAURA;
        else if (IS_GOOD(&ch)) karma = GOLDENAURA;
    }
    else
        karma = ch.pcdata->karma;
    
    if (is_affected(&ch, gsn_heartofstone))
        karma /= 3;

    return karma;
}

int aura_grade(CHAR_DATA *ch) 
{
    int karma(effective_karma(*ch));
    if (karma <= SILVERAURA) return -4;
	if (karma > SILVERAURA && karma <= BRIGHTGOLDENAURA) return -3;
	if (karma > BRIGHTGOLDENAURA && karma <= GOLDENAURA) return -2;
	if (karma > GOLDENAURA && karma <= PALEGOLDENAURA) return -1;
	if (karma > PALEGOLDENAURA && karma < FAINTREDAURA) return 0;
	if (karma >= FAINTREDAURA && karma < REDAURA) return 1;
	if (karma >= REDAURA && karma < DARKREDAURA) return 2;
	if (karma >= DARKREDAURA && karma < BLACKAURA) return 3;
    return 4;
}

void do_sweep(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *vch;
    OBJ_DATA *weapon;
    int chance,dam,skill;
    char buf[MAX_STRING_LENGTH];

    if ((skill = chance = get_skill(ch,gsn_sweep)) == 0)
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }	
    
    if ((weapon = get_eq_char(ch,WEAR_WIELD)) == NULL)
    {
	send_to_char("You must use a polearm to knock people down.\n\r",ch);
	return;	
    }
    
    if (weapon->value[0] != WEAPON_POLEARM)
    {
	send_to_char("You must use a polearm to knock people down.\n\r",ch);
	return;
    }
   
    if (ch->fighting == NULL)
    {
	send_to_char("You must be fighting to knock down your opponents.\n\r",ch);
	return;
    }

    for (vch=ch->in_room->people;vch!=NULL;vch=vch->next_in_room)
	if (vch->fighting == ch)
	    break;
    
    if (vch==NULL)
    {
	send_to_char("You can only sweep at the legs of those fighting you.\n\r",ch);
	return;
    }
 
    for (vch=ch->in_room->people;vch!=NULL;vch=vch->next_in_room)
    {
	if (is_same_group(ch,vch)
	|| vch->fighting != ch
	||  is_safe(ch,vch))
	    continue;
	if (!check_reach(ch, vch, REACH_EASY, PULSE_VIOLENCE))
	    continue;
	if (get_curr_stat(ch,STAT_STR)>get_curr_stat(ch,STAT_DEX))
        {
	    chance += get_curr_stat(ch,STAT_STR);
	    chance += get_curr_stat(ch,STAT_DEX)/2;
        }
        else
        {
	    chance += get_curr_stat(ch,STAT_STR)/2;
            chance += get_curr_stat(ch,STAT_DEX);
        }
    
        chance -= get_curr_stat(vch,STAT_DEX);
        chance -= GET_AC(vch,AC_SLASH)/25;
        if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
	    chance += 10;
        if (IS_SET(vch->off_flags,OFF_FAST) || IS_AFFECTED(vch,AFF_HASTE))
	    chance -= 30;
        chance += (ch->level - vch->level);
        if (!IS_NPC(vch) && chance < get_skill(vch,gsn_dodge))
	    chance -= (get_skill(vch,gsn_dodge)/5);
    
	chance = UMIN(chance,92);
	if (is_flying(vch))
	    chance = 0;

        if (number_percent() < chance)
        {
	    if ((vch->fighting != ch) && (!IS_NPC(vch)))
	    {
	        sprintf(buf,"Help!  %s is knocking me down!", PERS(ch,vch));
	        do_autoyell(vch,buf);
            }
	    act("$n {rsweeps{n your legs with $p!",ch,weapon,vch,TO_VICT);
	    act("You {rsweep{n $N's legs with $p!",ch,weapon,vch,TO_CHAR);
	    act("$n {rsweeps{n $N's legs with $p!", ch,weapon,vch,TO_NOTVICT);
	    check_improve(ch,vch,gsn_sweep,TRUE,1);
	    WAIT_STATE(ch,skill_table[gsn_sweep].beats);
	    WAIT_STATE(vch,UMAX(vch->wait,2*PULSE_VIOLENCE + number_range(1,3) - 2));
    	    if (is_affected(vch, gsn_form_of_the_hawk)
	      && get_skill(vch, gsn_form_of_the_hawk) > number_percent())
	    {
		act("As you expose yourself to sweep $N, $S swordwork allows $M to strike at you!", ch, NULL, vch, TO_CHAR);
		act("As $n exposes $mself to sweep $N, $N's swordwork allows $M to strike at $m!", ch, NULL, vch, TO_NOTVICT);
		act("As $n exposes $mself to sweep you, your swordwork allows you to strike at $m!", ch, NULL, vch, TO_VICT);
		multi_hit(vch, ch, TYPE_UNDEFINED);
	    }
  	    dam = dice(1,5);
    
            damage_from_obj(ch,vch,weapon,dam,gsn_sweep,weapon->value[3],FALSE);
        }
        else
        {
  	    if ((vch->fighting != ch) && (!IS_NPC(vch)))
	    {
	        sprintf(buf,"Help!  %s tried to knock me down!", PERS(ch,vch));
	        do_autoyell(vch,buf);
	    }
	    damage(ch,vch,0,gsn_sweep,weapon->value[3],FALSE);
	    act("You try to sweep at $N's legs and miss.", ch,NULL,vch,TO_CHAR);
	    act("$n tries to sweep at $N's legs and misses.",ch,NULL,vch,TO_NOTVICT);
	    act("$n tries to sweep at your legs and misses.",ch,NULL,vch,TO_VICT);
	    check_improve(ch,vch,gsn_sweep,FALSE,1);
	    WAIT_STATE(ch,skill_table[gsn_sweep].beats);
        }
        check_killer(ch,vch);
	chance = get_skill(ch,gsn_sweep);
    }
}

void do_whirl(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *vch;
    OBJ_DATA *weapon;
    int chance,dam,vmod;
    char buf[MAX_STRING_LENGTH];
    bool fake = FALSE;

    if ((chance = get_skill(ch,gsn_whirl)) == 0)
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }	
    
    if ((weapon = get_eq_char(ch,WEAR_WIELD)) == NULL)
    {
	send_to_char("You must use a two-handed weapon to hit everyone.\n\r",ch);
	return;	
    }
    
    if (!IS_WEAPON_STAT(weapon, WEAPON_TWO_HANDS))
	if (IS_PAFFECTED(ch,AFF_TWOHAND))
	    fake = TRUE;
	else
	{
	    send_to_char("You must use a two-handed weapon to hit everyone.\n\r",ch);
	    return;
	}
   
    if (ch->fighting == NULL)
    {
	send_to_char("You must be fighting to hit everyone.\n\r",ch);
	return;
    }

    for (vch=ch->in_room->people;vch!=NULL;vch=vch->next_in_room)
	if (vch->fighting == ch)
	    break;
    
    if (vch==NULL)
    {
	send_to_char("You can only hit everyone fighting you.\n\r",ch);
	return;
    }

    if (ch->mana < skill_table[gsn_whirl].min_mana)
    {
	send_to_char("You are too tired to whirl your weapon around.\n\r",ch);
	return;
    }
    expend_mana(ch, skill_table[gsn_whirl].min_mana);

    act("You whirl $p about, hitting your opponents!",ch,weapon,NULL,TO_CHAR);
    act("$n whirls $p about, hitting $s opponents!",ch,weapon,NULL,TO_ROOM); 
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
	chance += 10;
    for (vch=ch->in_room->people;vch!=NULL;vch=vch->next_in_room)
    {
	    if (is_same_group(ch,vch) || vch->fighting != ch || is_safe(ch,vch))
    	    continue;

    	if (!check_reach(ch, vch, REACH_EASY, PULSE_VIOLENCE))
	        continue;

    	vmod = 0;
    
        if (IS_SET(vch->off_flags,OFF_FAST) || IS_AFFECTED(vch,AFF_HASTE))
	        vmod -= 20;

        vmod += (ch->level - vch->level);
    	if (number_percent() < URANGE(5,chance + vmod,100))
        {
	        if ((vch->fighting != ch) && (!IS_NPC(vch)))
	        {
	            sprintf(buf,"Help!  %s is attacking me!", PERS(ch,vch));
    	        do_autoyell(vch,buf);
            }
	        check_improve(ch,vch,gsn_whirl,TRUE,1);
    	    WAIT_STATE(ch,skill_table[gsn_whirl].beats);
  	        dam = dice(weapon->value[1],weapon->value[2]) + GET_DAMROLL(ch);
    	    dam += check_extra_damage(ch,dam,weapon);
   	        dam += round(dam*3/20);
            dam = round(dam * UMIN(get_weapon_skill_weapon(ch,weapon),100)/100);
    	    if (fake)
	        	dam = round(dam*3/4);
 
            damage_from_obj(ch,vch,weapon,dam,gsn_whirl,weapon->value[3],TRUE);
        }
        else
        {
      	    if ((vch->fighting != ch) && (!IS_NPC(vch)))
	        {
	            sprintf(buf,"Help!  %s tried to attack me!", PERS(ch,vch));
    	        do_autoyell(vch,buf);
	        }
    	    damage(ch,vch,0,gsn_whirl,weapon->value[3],TRUE);
	        check_improve(ch,vch,gsn_whirl,FALSE,1);
	        WAIT_STATE(ch,skill_table[gsn_whirl].beats);
        }
        check_killer(ch,vch);
    }
}

void do_charge(CHAR_DATA *ch, char *argument)
{
    char target_name[MAX_STRING_LENGTH], dir[MAX_STRING_LENGTH],
	 opdir[MAX_STRING_LENGTH],buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim = NULL;
    ROOM_INDEX_DATA *starting_room = NULL, *target_room = NULL;
    int depth, max_depth = 3, chance, door, power;
    bool brief_off = FALSE;
 
    AFFECT_DATA af;
    EXIT_DATA *pExit = NULL;
    
    if (ch->in_room == NULL)
    {
	send_to_char("Huh?\n\r",ch);
	return;
    }
    starting_room = target_room = ch->in_room;

    if ((chance = get_skill(ch,gsn_charge)) == 0)
    {
	send_to_char("Huh?\n\r",ch); return;
    }

    if (ch->fighting)
    {
	send_to_char("Not now, you're busy!\n\r",ch); return;
    }

    argument = one_argument(argument,target_name);
    one_argument(argument,dir);
    
    if (target_name[0] == '\0')
    {
	send_to_char("Who do you want to charge?\n\r",ch); return;
    }

    if (dir[0] == '\0')
    {
	send_to_char("Which direction do you want to charge?\n\r",ch); 	return;
    }
    
    if (ch->in_room->sector_type == SECT_UNDERWATER && !is_affected(ch,gsn_aquamove))
    {
        if (number_percent() <= get_skill(ch, gsn_waveborne))
            check_improve(ch, victim, gsn_waveborne, true, 4);
        else
        {
            check_improve(ch, victim, gsn_waveborne, false, 4);
        	send_to_char("You can't charge underwater.\n\r",ch);
        	return;
        }
    }

    if (!str_prefix(dir,"north")) {door = 0; sprintf(dir,"north");}
    else if (!str_prefix(dir,"east")) {door = 1; strcpy(dir,"east");}
    else if (!str_prefix(dir,"south")) {door = 2; strcpy(dir,"south");}
    else if (!str_prefix(dir,"west")) {door = 3; strcpy(dir,"west");}
    else if (!str_prefix(dir,"up")) {door = 4; strcpy(dir,"up");}
    else if (!str_prefix(dir,"down")) {door = 5; strcpy(dir,"down");}
    else { send_to_char("Which direction do you want to charge?\n\r",ch); return; }
    
    if (OPPOSITE(door) == 0) sprintf(opdir,"the north");
    else if (OPPOSITE(door) == 1) sprintf(opdir,"the east");
    else if (OPPOSITE(door) == 2) sprintf(opdir,"the south");
    else if (OPPOSITE(door) == 3) sprintf(opdir,"the west");
    else if (OPPOSITE(door) == 4) sprintf(opdir,"above");
    else sprintf(opdir,"below");
    
    if (!IS_NPC(ch) && BIT_GET(ch->pcdata->traits, TRAIT_EAGLE_EYED))
	max_depth += 2;
    if (is_affected(ch,gsn_eagleeyes))
	max_depth += 3;

    for (depth = 1; depth <= max_depth;depth++)
    {
	if ((pExit = target_room->exit[door]) != NULL)
	{
	    if (pExit->exit_info & EX_CLOSED || room_is_affected(pExit->u1.to_room,gsn_smoke)
	      || pExit->exit_info & EX_WALLED || pExit->exit_info & EX_ICEWALL
          || pExit->exit_info & EX_WALLOFFIRE || pExit->exit_info & EX_WEAVEWALL
	      || pExit->exit_info & EX_SECRET)
	    {
	        send_to_char("You can't see your target in that direction.\n\r",ch);
		return;
	    }   
	    target_room = pExit->u1.to_room;
	    if ((victim = room_get_char_room(target_room,target_name)) != NULL)
		if (can_see(ch,victim) && !is_safe(ch,victim))
		    break;
	}
	else
	{
	    send_to_char("You can't see your target in that direction.\n\r",ch);
	    return;
	}
    }
    if (victim == NULL)
    {
	send_to_char("You can't see your target in that direction.\n\r",ch);
	return;
    }
// So now we have a valid target and a direction

    power = depth;

    if (ch->in_room == target_room)
    {
	send_to_char("You can't charge someone so close.\n\r",ch);
	return;
    }
// We're charging now!
    af.type = gsn_charge;
    af.duration = 0;
    af.bitvector = 0;
    af.valid = TRUE;
    af.point = NULL;
    af.where = TO_AFFECTS;
    af.modifier = 0;
    af.location = APPLY_NONE;
    affect_to_char(ch,&af);

// Modifiers
    if (IS_AFFECTED(victim,AFF_HASTE))
	chance -= 10;
    if (IS_AFFECTED(ch, AFF_HASTE))
	chance += 10;
    if (is_affected(ch, gsn_sense_danger))
	chance -= 20;
    if (is_affected(ch, gsn_wariness))
	chance -= 20;
    if (is_affected(ch, gsn_scouting))
	chance -= 10*power;
// Eliminate some spam
    if (!IS_SET(ch->comm,COMM_BRIEF))
    {
	brief_off = TRUE;
	SET_BIT(ch->comm,COMM_BRIEF);
    }
    for (depth = 1;depth <= power;depth++)
    {
	if (depth == 1)
	    sprintf(buf,"$n begins to charge %s.",dir);
	else
	    sprintf(buf,"$n continues charging %s.",dir);
	act(buf,ch,NULL,NULL,TO_ROOM);
	if (depth == 1)
	    sprintf(buf,"You begin to charge %s.",dir);
	else
	    sprintf(buf,"You continue charging %s.",dir);
	act(buf,ch,NULL,NULL,TO_CHAR);

    move_char(ch, door, false, true);
	//interpret(ch,dir);
	if (ch->in_room == starting_room)
	{
	    act("You halt your charge.",ch,NULL,NULL,TO_CHAR);
	    act("$n halts $s charge.",ch,NULL,NULL,TO_ROOM);
	    affect_strip(ch,gsn_charge);
	    if (brief_off)
		REMOVE_BIT(ch->comm,COMM_BRIEF);
	    return;
	}
	
	if (ch->in_room->sector_type == SECT_UNDERWATER && !is_affected(ch,gsn_aquamove))
	{
	    act("You halt your charge, your movement slowed by the water.",ch,NULL,NULL,TO_CHAR);
	    act("$n halts $s charge, $s movement slowed by the water.",ch,NULL,NULL,TO_ROOM);
	    affect_strip(ch,gsn_charge);
	    if (brief_off)
		REMOVE_BIT(ch->comm,COMM_BRIEF);
	    return;
	}
	if (ch->in_room == target_room)
	{
	    if (number_percent() < chance)
	    {
	        if (is_affected(victim,gsn_anchor))
		{
		    sprintf(buf,"You charge in from %s, but $N stands firm!",opdir);
		    act(buf,ch,NULL,victim,TO_CHAR);
		    sprintf(buf,"$n charges in from %s, but $N stands firm!",opdir);
		    act(buf,ch,NULL,victim,TO_NOTVICT);
		    sprintf(buf,"$n charges in from %s, but you stand firm!",opdir);
		    act(buf,ch,NULL,victim,TO_VICT);
                    WAIT_STATE(ch,UMAX(ch->wait, 3*skill_table[gsn_charge].beats));
	    	    if (brief_off)
		        REMOVE_BIT(ch->comm,COMM_BRIEF);
		    return;
		}
		if (get_eq_char(victim,WEAR_WIELD)
		  && number_percent() < get_skill(victim,gsn_retaliation)/2)
		{
		    int damage = 0;
		    OBJ_DATA *weapon = get_eq_char(victim,WEAR_WIELD);
		    if (weapon != NULL)
		    {
			act("$n stops $N's charge!",victim,NULL,ch,TO_NOTVICT);
			act("You stop $N's charge!",victim,NULL,ch,TO_CHAR);
			act("$n stops your charge!",victim,NULL,ch,TO_VICT);
			damage = dice(weapon->value[1], weapon->value[2]) + GET_DAMROLL(victim);
			damage += check_extra_damage(victim,damage,weapon);	
			damage_from_obj(victim,ch,weapon,damage,weapon->value[0],weapon->value[3],TRUE);
			wrathkyana_combat_effect(victim,ch);
			check_improve(victim,ch,gsn_retaliation,TRUE,1);
	    		if (brief_off)
			    REMOVE_BIT(ch->comm,COMM_BRIEF);
			return;
		    }		    
		}
		sprintf(buf,"You charge in from %s, attacking $N!",opdir);
	        act(buf,ch,NULL,victim,TO_CHAR);
	        sprintf(buf,"$n charges in from %s, attacking $N!",opdir);
	        act(buf,ch,NULL,victim,TO_NOTVICT);
	        sprintf(buf,"$n charges in from %s, attacking you!",opdir);
	        act(buf,ch,NULL,victim,TO_VICT);
	        damage(ch,victim,power*ch->level,gsn_charge,DAM_BASH,TRUE);
		if (is_affected(victim,gsn_flameshield) || is_affected(victim, gsn_lambentaura))
		{
		    sprintf(buf,"You charge in from %s, and $N's flame shield flares as you strike!",opdir);
		    act(buf,ch,NULL,victim,TO_CHAR);
		    sprintf(buf,"$n charges in from %s, but $N's flame shield flares as $e strikes!",opdir);
		    act(buf,ch,NULL,victim,TO_NOTVICT);
		    sprintf(buf,"$n charges in from %s, but your flame shield flares as $e strikes!",opdir);	
		    act(buf,ch,NULL,victim,TO_ROOM);
		    damage(victim,ch,3*victim->level,gsn_flameshield,DAM_FIRE,TRUE);
		}
		check_improve(ch,victim,gsn_charge,TRUE,2);
  	        WAIT_STATE(ch, UMAX(ch->wait, 2*skill_table[gsn_charge].beats));
		if (victim != NULL)
		{
		    if (IS_NPC(victim) &&
		      (IS_SET(victim->act,ACT_SENTINEL)
		      || IS_SET(victim->act,ACT_NOSUBDUE)
		      || IS_OAFFECTED(victim,AFF_GHOST)))
		    {
	    		if (brief_off)
			    REMOVE_BIT(ch->comm,COMM_BRIEF);
		        return;
		    }
		}
		else
		{
	    	    if (brief_off)
			REMOVE_BIT(ch->comm,COMM_BRIEF);
		    return;
		}
		  
		if (victim != NULL && victim->in_room != NULL)
		{
		    stop_fighting(victim);
		// going to reuse opdir here
		    if ((pExit = victim->in_room->exit[door]) != NULL
		      && power > 1
		      && (!(pExit->exit_info & EX_CLOSED 
	      		|| pExit->exit_info & EX_WALLED 
	      		|| pExit->exit_info & EX_ICEWALL 
                || pExit->exit_info & EX_WEAVEWALL
			|| pExit->exit_info & EX_WALLOFFIRE
		     	|| pExit->exit_info & EX_SECRET)))
		    {
		        if (door == 0) strcpy(opdir,"northward");
		        else if (door == 1) strcpy(opdir,"eastward");
		        else if (door == 2) strcpy(opdir,"southward");
		        else if (door == 3) strcpy(opdir,"westward");
		        else if (door == 4) strcpy(opdir,"upward");
		        else if (door == 5) strcpy(opdir,"downward");

		        sprintf(buf,"$N is thrown %s!",opdir);
		        act(buf,ch,NULL,victim,TO_CHAR);
		        act(buf,ch,NULL,victim,TO_NOTVICT);
		        sprintf(buf,"You are thrown %s!",opdir);
		        act(buf,ch,NULL,victim,TO_VICT);
			affect_to_char(victim,&af);
			int whee = 0;
			while (victim && victim->in_room && whee < (power/2))
			{
		            interpret(victim,dir);
			    whee++;
			}
	    		if (victim && victim->in_room)
			{
			    affect_strip(victim,gsn_charge);
			    victim->tracking = ch;
 		            WAIT_STATE(victim,UMAX(victim->wait, skill_table[gsn_charge].beats));
			}
                        if (brief_off)
		 	    REMOVE_BIT(ch->comm,COMM_BRIEF);
			return;
		    }
		    else
		    {
		    	WAIT_STATE(victim,UMAX(victim->wait, skill_table[gsn_charge].beats));
    			if (brief_off)
			    REMOVE_BIT(ch->comm,COMM_BRIEF);
			return;
		    }
		}
	    }
	    else
	    {
		sprintf(buf,"You charge in from %s, but miss $N!",opdir);
		act(buf,ch,NULL,victim,TO_CHAR);
	        sprintf(buf,"$n charges in from %s, but misses $N!",opdir);
	        act(buf,ch,NULL,victim,TO_NOTVICT);
	        sprintf(buf,"$n charges in from %s, but misses you!",opdir);
	        act(buf,ch,NULL,victim,TO_VICT);
	        damage(ch,victim,0,gsn_charge,DAM_BASH,TRUE);
		WAIT_STATE(ch,UMAX(ch->wait, skill_table[gsn_charge].beats));
		check_improve(ch,victim,gsn_charge,FALSE,2);
                affect_strip(ch,gsn_charge);
                if (brief_off)
	            REMOVE_BIT(ch->comm,COMM_BRIEF);
		return;
	    }
	}
	else
	{
	    sprintf(buf,"$n charges in from %s.",opdir);
	    act(buf,ch,NULL,NULL,TO_ROOM);
	}
    } 
    affect_strip(ch,gsn_charge);
    if (brief_off)
	REMOVE_BIT(ch->comm,COMM_BRIEF);
}

void do_bravado(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;
    int chance;

    if ((chance = get_skill(ch, gsn_bravado)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (is_affected(ch, gsn_bravado))
    {
	send_to_char("You are already filled with self-confidence.\n\r", ch);
	return;
    }

    if (ch->mana - skill_table[gsn_bravado].min_mana < 0)
    {
	act("You don't feel confident when you're tired.",ch,NULL,NULL,TO_CHAR);
	return;
    }

    if (chance < number_percent())
    {
	act("You don't feel any more self-confident.", ch, NULL, NULL, TO_CHAR);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_bravado].beats));
        check_improve(ch,NULL,gsn_bravado,FALSE,1);
	return;
    }
    
    expend_mana(ch, skill_table[gsn_bravado].min_mana);

    act("You feel indomitable.",ch, NULL, NULL, TO_CHAR);

    af.where        = TO_AFFECTS;
    af.type         = gsn_bravado;
    af.level        = ch->level;
    af.duration     = 5+ch->level/5;
    af.location     = APPLY_HITROLL;
    af.bitvector    = 0;
    af.modifier     = ch->level/10 + get_curr_stat(ch, STAT_CHR)/4;
    affect_to_char(ch, &af);

    af.location = APPLY_DAMROLL;
    affect_to_char(ch, &af);

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_bravado].beats));
    check_improve(ch,NULL,gsn_bravado,TRUE,2);
}

void do_aggression(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;
    int chance;

    if ((chance = get_skill(ch, gsn_aggression)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (is_affected(ch, gsn_aggression))
    {
	send_to_char("You are already filled with aggression.\n\r", ch);
	return;
    }

    if (ch->mana - skill_table[gsn_aggression].min_mana < 0)
    {
	act("You don't feel aggressive when you're tired.",ch,NULL,NULL,TO_CHAR);
	return;
    }

    if (chance < number_percent())
    {
	act("You don't feel any more aggressive.", ch, NULL, NULL, TO_CHAR);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_aggression].beats));
        check_improve(ch,NULL,gsn_aggression,FALSE,1);
	return;
    }
    
    expend_mana(ch, skill_table[gsn_aggression].min_mana);

    act("You feel aggressive.",ch, NULL, NULL, TO_CHAR);

    af.where        = TO_AFFECTS;
    af.type         = gsn_aggression;
    af.level        = ch->level;
    af.duration     = 5+ch->level/5;
    af.location     = APPLY_HITROLL;
    af.bitvector    = 0;
    af.modifier     = ch->level/10 + get_curr_stat(ch, STAT_STR)/4;
    affect_to_char(ch, &af);

    af.location = APPLY_DAMROLL;
    affect_to_char(ch, &af);

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_aggression].beats));
    check_improve(ch,NULL,gsn_aggression,TRUE,2);
}

void do_enthusiasm(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;
    int chance;

    if ((chance = get_skill(ch, gsn_enthusiasm)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (is_affected(ch, gsn_enthusiasm))
    {
	send_to_char("You are already filled with enthusiasm.\n\r", ch);
	return;
    }
    
    if (ch->mana - skill_table[gsn_enthusiasm].min_mana < 0)
    {
	act("You're too tired to pump yourself up.",ch,NULL,NULL,TO_CHAR);
	return;
    }

    if (chance < number_percent())
    {
	act("You don't feel any more enthusiastic.", ch, NULL, NULL, TO_CHAR);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_enthusiasm].beats));
        check_improve(ch,NULL,gsn_enthusiasm,FALSE,1);
	return;
    }
    expend_mana(ch, skill_table[gsn_enthusiasm].min_mana);

    act("You prepare to overcome adversity.",ch, NULL, NULL, TO_CHAR);

    af.where        = TO_NAFFECTS;
    af.type         = gsn_enthusiasm;
    af.level        = ch->level;
    af.duration     = ch->level/8;
    af.location     = APPLY_NONE;
    af.bitvector    = 0;
    af.modifier     = 1;
    affect_to_char(ch, &af);

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_enthusiasm].beats));
    check_improve(ch,NULL,gsn_enthusiasm,TRUE,2);
}

void do_endure(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;
    int chance;

    if ((chance = get_skill(ch, gsn_endure)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (is_affected(ch, gsn_endure))
    {
	if (IS_PAFFECTED(ch, AFF_ENDURE))
	{
	    send_to_char("You are enduring blows already.\n\r", ch);
	    return;
	}
	else
	{
	    send_to_char("You were enduring blows too recently.\n\r", ch);
	    return;
	}
    }
    
    if (ch->mana - skill_table[gsn_endure].min_mana < 0)
    {
	act("You are too tired to endure blows.", ch, NULL, NULL, TO_CHAR);
	return;
    }

    if (chance < number_percent())
    {
	act("You don't feel any more ready to endure blows.", ch, NULL, NULL, TO_CHAR);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_endure].beats));
        check_improve(ch,NULL,gsn_endure,FALSE,1);
	return;
    }
    
    expend_mana(ch, skill_table[gsn_endure].min_mana);

    act("$n looks ready to endure blows.",ch, NULL, NULL, TO_ROOM);
    act("You feel ready to endure blows.",ch, NULL, NULL, TO_CHAR);

    af.where        = TO_PAFFECTS;
    af.type         = gsn_endure;
    af.level        = ch->level;
    af.duration     = ch->level/10;
    af.location     = APPLY_HIT;
    af.bitvector    = AFF_ENDURE;
    af.modifier     = ch->level * 2 + get_curr_stat(ch,STAT_CON)*2;
    affect_to_char(ch, &af);
    ch->hit += af.modifier;

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_endure].beats));
    check_improve(ch,NULL,gsn_endure,TRUE,2);
}

void do_taunt(CHAR_DATA *ch, char *argument)
{
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
CHAR_DATA *vch;
int chance;

    if ((chance = get_skill(ch, gsn_taunt)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (ch->fighting == NULL)
    {
	send_to_char("You have no enemy to taunt against.\n\r", ch);
	return;
    }

    if (is_affected(ch, gsn_taunt))
    {
	send_to_char("You do not feel ready to taunt another foe.\n\r", ch);
	return;
    }

    if (chance < number_percent())
    {
	act("$n tries to issue a taunt, but is distracted and fails.",ch, NULL, NULL, TO_ROOM);
	act("You try to issue a taunt, but are distracted and fail.", ch, NULL, NULL, TO_CHAR);
        check_improve(ch,NULL,gsn_taunt,FALSE,1);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_taunt].beats));
	return;
    }

    act("$n viciously taunts $s foes!",	ch, NULL, NULL, TO_ROOM);
    act("You viciously taunt your foes!",ch, NULL, NULL, TO_CHAR);
    if (ch->battlecry != NULL)
	do_yell(ch, ch->battlecry);
    else
	do_yell(ch, "Is that all you've got?");
    
    af.where        = TO_NAFFECTS;
    af.type         = gsn_taunt;
    af.level        = ch->level;
    af.duration     = 6;
    af.location     = 0;
    af.bitvector    = AFF_RALLY;
    af.modifier     = 0;
    affect_to_char(ch, &af);

    af.duration = 1;
    af.location = APPLY_DAMROLL;
    af.modifier = 0 - ch->level/10 - get_curr_stat(ch,STAT_CHR)/4;
    af.bitvector = 0;
    int cchance=2*get_curr_stat(ch,STAT_CHR)+number_percent(), vchance=0;
    for (vch = ch->in_room->people;vch != NULL;vch = vch->next_in_room)
    {
        if (vch->fighting == ch)
	{
	    if (IS_SET(vch->imm_flags, IMM_MENTAL))
		continue;
	    vchance = get_curr_stat(vch,STAT_WIS)+number_percent()+get_resist(vch,DAM_MENTAL);
	    if (cchance >= vchance)
	    {
		act("$n looks unnerved!",vch,NULL,NULL,TO_ROOM);
		act("You feel unnerved!",vch,NULL,NULL,TO_CHAR);
		affect_to_char(vch,&af);
	    }
	    else
	    {
		act("$n appears unfazed.",vch,NULL,NULL,TO_ROOM);
		act("You feel unfazed.",vch,NULL,NULL,TO_CHAR);
	    }
	}
    }
    if (vchance == 0)
	send_to_char("Your taunts have no effect!\n\r",ch);

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_taunt].beats));
    check_improve(ch,NULL,gsn_taunt,TRUE,1);
}

void do_ready(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    OBJ_DATA *wield;
    AFFECT_DATA *paf; 

    if (get_skill(ch, gsn_archery) == 0)
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }
    if (!IS_NAFFECTED(ch,AFF_SCOUTING))
    {
	send_to_char("You must be scouting to prepare a readied shot.\n\r",ch);
	return;
    }
    if (ch->mana - skill_table[gsn_ready].min_mana < 0)
    {
	send_to_char("You are too tired to ready a shot.\n\r",ch);
	return;
    }

    wield = get_eq_char(ch, WEAR_HOLD);

    if ((wield == NULL) || (wield->item_type != ITEM_BOW))
    {   
        send_to_char("You must be holding a bow to shoot.\n\r", ch);
	return;
    }

    if (argument[0] == '\0')
    {
	send_to_char("Shoot whom?\n\r", ch);
	return;
    }

    if (ch->fighting != NULL)
    {
	send_to_char("You can't ready a shot while fighting.\n\r", ch);
	return;
    }

    if (ch->nocked == NULL)
    {
	send_to_char("You must have an arrow nocked to prepare to shoot someone.\n\r",ch);
	return;
    }
    if (is_affected(ch,gsn_ready))
    {
	act("You stop readying your shot.",ch,NULL,NULL,TO_CHAR);
	act("$n relaxes $s draw on $p.",ch,ch->nocked,NULL,TO_ROOM);
	affect_strip(ch,gsn_ready);
	return;
    }
    expend_mana(ch, skill_table[gsn_ready].min_mana);
    af.modifier = 2;
    for (paf = ch->nocked->pIndexData->affected; paf != NULL; paf = paf->next)
        if (paf->location == APPLY_RANGE)
	    af.modifier += paf->modifier;
    for (paf = ch->nocked->affected; paf != NULL; paf = paf->next)
        if (paf->location == APPLY_RANGE)
	    af.modifier += paf->modifier;
    for (paf = wield->pIndexData->affected; paf != NULL; paf = paf->next)
	if (paf->location == APPLY_RANGE)
	    af.modifier += paf->modifier;
        
    af.where = TO_AFFECTS;
    af.type = gsn_ready;
    af.location = APPLY_NONE;
    af.point = (void *)str_dup(argument);
    af.duration = 2;
    af.bitvector = 0;
    affect_to_char(ch,&af);
    act("You draw back $p, readying a shot.",ch,ch->nocked,NULL,TO_CHAR);
    act("$n draws $p, readying a shot.",ch,ch->nocked,NULL,TO_ROOM);
};
    
void do_jab( CHAR_DATA *ch, char *argument)
{
char buf[MAX_STRING_LENGTH];
CHAR_DATA *victim;
OBJ_DATA *wield;
int dam, chance;
AFFECT_DATA af;

    if ((chance = get_skill(ch, gsn_jab)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (argument[0] == '\0')
    {
        if ((victim = ch->fighting) == NULL)
	{
  	    send_to_char("Jab at whom?\n\r", ch);
	    return;
	}
    }
    else if ((victim = get_char_room(ch, argument)) == NULL)
    {
	send_to_char("Jab at whom?\n\r", ch);
	return;
    }
    if (ch == victim)
    {
	send_to_char("You slap yourself around a bit.\n\r",ch);
	return;
    }

    if (is_safe(ch,victim))
        return;
   
    int hands = 0;
 
    if ((wield = get_eq_char(ch, WEAR_WIELD)) == NULL
      || wield->pIndexData->value[0] == WEAPON_MACE)
	hands++;
    if (((wield && !IS_SET(wield->pIndexData->value[4],WEAPON_TWO_HANDS))
      || ((wield && IS_SET(wield->pIndexData->value[4],WEAPON_TWO_HANDS)
	&& (ch->race == global_int_race_chaja 
	  || ch->race == global_int_race_alatharya))))
      && ((wield = get_eq_char(ch, WEAR_DUAL_WIELD)) == NULL
      || wield->pIndexData->value[0] == WEAPON_MACE)
      && get_eq_char(ch, WEAR_LIGHT) == NULL
      && get_eq_char(ch, WEAR_SHIELD) == NULL
      && get_eq_char(ch, WEAR_HOLD) == NULL)
	hands++;

    if (hands < 1)
    {
	send_to_char("You need a free hand or a mace to jab someone.\n\r",ch);
	return;
    }

    if (!check_reach(ch, victim, REACH_EASY, PULSE_VIOLENCE))
	return;
 
    chance += (ch->level - victim->level);

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_jab].beats));

    if (!IS_NPC(victim))
	if (ch->fighting == NULL || victim->fighting != ch)
	{
   	    sprintf(buf, "Help! %s is attacking me!", PERS(ch, victim));
	    do_autoyell(victim, buf);
	}

    if (IS_NAFFECTED(victim,AFF_AGILITY) 
      && number_percent() < (get_skill(victim,gsn_agility)/2+(SIZE_MEDIUM - victim->size)*10))
    {
	act("$N alertly dodges your jab.",ch,NULL,victim,TO_CHAR);
	act("You alertly dodge $n's jab.",ch,NULL,victim,TO_VICT);
	act("$N alertly dodges $n's jab.",ch,NULL,victim,TO_VICTROOM);
	return;
    }

    if (number_percent() < chance)
    {
	if (wield)
	{
	    act("$n jabs at $N with $p!", ch, wield, victim, TO_NOTVICT);
	    act("You jab at $N with $p!", ch, wield, victim, TO_CHAR);
	    act("$n jabs at you with $p!", ch, wield, victim, TO_VICT);
	}
	else
	{
	    act("$n jabs at $N!",ch,NULL,victim,TO_NOTVICT);
	    act("You jab at $N!",ch,NULL,victim,TO_CHAR);
	    act("$n jabs at you!",ch,NULL,victim,TO_VICT);
	}
        check_improve(ch,victim,gsn_jab,TRUE,1);
    }
    else
    {
	if (wield)
	{
	    act("$n tries to jab at $N with $p, but $E avoids the attack!", ch, wield, victim, TO_NOTVICT);
	    act("You try to jab at $N with $p, but $E avoids the attack!", ch, wield, victim, TO_CHAR);
	    act("$n tries to jab at you with $p, but you avoid the attack!", ch, wield, victim, TO_VICT);
	}
	else
	{
	    act("$n tries to jab at $N, but $E avoids the attack!",ch,NULL,victim,TO_NOTVICT);
	    act("You try to jab at $N, but $E avoids the attack!",ch,NULL,victim,TO_CHAR);
	    act("$n tries to jab at you, but you avoid the attack!",ch,NULL,victim,TO_VICT);
	}
        check_improve(ch,victim,gsn_jab,FALSE,1);
        damage( ch, victim, 0, gsn_jab, 0, TRUE );
	return;
    }

    if (wield)
	dam = dice(wield->value[1], wield->value[2]) + GET_DAMROLL(ch)/2;
    else
	dam = dice(ch->damage[DICE_NUMBER],ch->damage[DICE_TYPE])+GET_DAMROLL(ch);
    dam += check_extra_damage(ch,dam,wield);

    if (wield && IS_WEAPON_STAT(wield,WEAPON_TWO_HANDS))
	dam *= 5/4;
    
    if (wield)
	damage_from_obj( ch, victim, wield, dam, gsn_jab, wield->value[3], TRUE );
    else
	damage(ch,victim,dam,gsn_jab,ch->dam_type,TRUE);
    
    if (!is_affected(victim,gsn_jab))
    {
	af.valid = TRUE;
	af.point = NULL;
	af.where = TO_AFFECTS;
	af.location = APPLY_AC;
	af.type = gsn_jab;
	af.modifier = ch->level;
	af.duration = 1;
	af.bitvector = 0;
	affect_to_char(victim,&af);
    }	    
    return;
}

void do_headbutt( CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    int dam, chance;
    AFFECT_DATA af;

    if ((chance = get_skill(ch, gsn_headbutt)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (argument[0] == '\0')
    {
        if ((victim = ch->fighting) == NULL)
	{
  	    send_to_char("Headbutt whom?\n\r", ch);
	    return;
	}
    }
    else if ((victim = get_char_room(ch, argument)) == NULL)
    {
	send_to_char("Headbutt whom?\n\r", ch);
	return;
    }
    if (ch == victim)
    {
	send_to_char("You jerk your head around violently to no avail.\n\r",ch);
	return;
    }

    if (is_safe(ch,victim))
        return;

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;

    chance /= 2;
 
    chance += (ch->level - victim->level);

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_headbutt].beats));

    if (!IS_NPC(victim))
	if (ch->fighting == NULL || victim->fighting != ch)
	{
   	    sprintf(buf, "Help! %s is attacking me!", PERS(ch, victim));
	    do_autoyell(victim, buf);
	}

    if (is_affected(victim,gsn_jab))
	chance = chance * 5 / 4;

    if (number_percent() < chance)
    {
	dam = number_range(ch->level,ch->level*1.5)+GET_DAMROLL(ch)/2;
	dam += check_extra_damage(ch,dam,NULL)/2;
	damage(ch,victim,dam,gsn_headbutt,ch->dam_type,TRUE);
        check_improve(ch,victim,gsn_headbutt,TRUE,1);
    }
    else
    {
	act("$n tries to headbutt $N, but {rmisses{x.",ch,NULL,victim,TO_NOTVICT);
	act("You try to headbutt $N, but {rmiss{x.",ch,NULL,victim,TO_CHAR);
	act("$n tries to headbutt you, but {rmisses{x.",ch,NULL,victim,TO_VICT);
        check_improve(ch,victim,gsn_headbutt,FALSE,1);
        damage( ch, victim, 0, gsn_headbutt, 0, FALSE );
	return;
    }

    if (IS_NAFFECTED(victim,AFF_AGILITY)
      && number_percent() < (get_skill(victim,gsn_agility)/2+(SIZE_MEDIUM - victim->size)*10))
    {
	act("$N alertly dodges your headbutt.",ch,NULL,victim,TO_CHAR);
	act("You alertly dodge $n's headbutt.",ch,NULL,victim,TO_VICT);
	act("$N alertly dodges $n's neadbutt.",ch,NULL,victim,TO_VICTROOM);
	return;
    }

    if (!is_affected(victim,gsn_headbutt))
    {
	af.valid = TRUE;
	af.point = NULL;
	af.where = TO_AFFECTS;
	af.location = APPLY_NONE;
	af.type = gsn_headbutt;
	af.modifier = 1;
	af.duration = 1;
	af.bitvector = 0;
	affect_to_char(victim,&af);
    }

    if (!is_affected(ch,gsn_headbutt) 
      && number_percent() < (100 - get_skill(ch,gsn_headbutt) * 3 / 4))
    {
	af.valid = TRUE;
	af.point = NULL;
	af.where = TO_AFFECTS;
	af.location = APPLY_NONE;
	af.type = gsn_headbutt;
	af.modifier = 0;
	af.duration = 0;
	af.bitvector = 0;
	affect_to_char(ch,&af);
    }

    return;
}

void do_warpath(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int wpsk = 0;

    if ((wpsk = get_skill(ch,gsn_warpath)) == 0)
    {
	send_to_char("Huh?\n\r",ch);
	return;
    }

    if (is_affected(ch, gsn_warpath))
    {
        send_to_char("You're already hunting someone.\n\r", ch);
        return;
    }

    if (ch->mana < skill_table[gsn_warpath].min_mana)
    {
	send_to_char("You're too tired to begin a warpath.\n\r",ch);
	return;
    }

    if (( victim = get_char_world(ch, argument )) == NULL)
    {
        send_to_char("You can't find them.\n\r", ch);
        return;
    }

    if (ch == victim)
    {
	send_to_char("Try as you might, you never see yourself coming.\n\r",ch);
	return;
    }

    if (!IS_NPC(victim) && !IS_PK(ch,victim))
    {
        send_to_char("The gods protect them.\n\r",ch);
        return;
    }

    if (!ch->in_room || !victim->in_room || ch->in_room->area != victim->in_room->area)
    {
        send_to_char("They are not within a close enough range to warpath.\n\r", ch);
        return;
    }

    if (number_percent() > wpsk)
    {
	check_improve(ch,NULL,gsn_warpath,FALSE,0);
	return;
    }
    check_improve(ch,NULL,gsn_warpath,TRUE,0);
    expend_mana(ch, skill_table[gsn_warpath].min_mana);
    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_warpath].beats));
    act("Your pulse speeds up as you begin your warpath toward $N.", ch, NULL, victim, TO_CHAR);

    ch->hunting = victim;

    af.where     = TO_AFFECTS;
    af.type      = gsn_warpath;
    af.level     = ch->level;
    af.duration  = 4;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    add_event((void *) ch, 1, &event_warpath);
}

void do_fortitude(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (get_skill(ch,gsn_fortitude) == 0 )
    {
	send_to_char("Huh?\n\r",ch);
	return;
    }

    if (is_affected(ch,gsn_fortitude))
    {
	send_to_char("You're already ignoring injuries.\n\r",ch);
	return;
    }

    if (ch->move < skill_table[gsn_fortitude].min_mana)
    {
	send_to_char("You're too tired to ignore your injuries.\n\r",ch);
	return;
    } 

    if (number_percent() > get_skill(ch,gsn_fortitude))
    {
	send_to_char("You try to prepare yourself to ignore injuries, but can't get angry enough.\n\r",ch);
	check_improve(ch,NULL,gsn_fortitude,FALSE,1);
  	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_fortitude].beats));
	return;
    }

    send_to_char("You steel yourself against injuries.\n",ch);
    act("$n steels $mself against injuries.",ch,NULL,NULL,TO_ROOM);
    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_fortitude].beats));

    ch->move -= skill_table[gsn_fortitude].min_mana;
    af.where = TO_AFFECTS;
    af.type = gsn_fortitude;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.level = ch->level;
    af.duration = 12;
    af.bitvector = 0;
    affect_to_char( ch, &af );
    check_improve(ch,NULL,gsn_fortitude,TRUE,1);
}


void do_maul(CHAR_DATA *ch, char *argument)
{
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
int chance;

    if ((chance = get_skill(ch, gsn_maul)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (ch->fighting == NULL)
    {
	send_to_char("You have no enemy to maul.\n\r", ch);
	return;
    }

    if (is_affected(ch, gsn_maul))
    {
	send_to_char("You do not feel ready to maul someone yet.\n\r", ch);
	return;
    }

    if (chance < number_percent())
    {
	act("You are distracted from mauling your foe.", ch, NULL, NULL, TO_CHAR);
        check_improve(ch,NULL,gsn_maul,FALSE,1);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_maul].beats));
	return;
    }

    act("$n surges forward to maul you!", ch, NULL, ch->fighting,TO_VICT);
    act("$n surges forward to maul $N!", ch, NULL, ch->fighting, TO_NOTVICT);
    act("You surge forward to maul $N!", ch, NULL, ch->fighting, TO_CHAR);
    
    af.where        = TO_PAFFECTS;
    af.type         = gsn_maul;
    af.level        = ch->level;
    af.duration     = 6;
    af.location     = 0;
    af.bitvector    = AFF_MAUL;
    af.modifier     = 0;
    affect_to_char(ch, &af);

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_maul].beats));
    check_improve(ch,NULL,gsn_maul,TRUE,1);
}


