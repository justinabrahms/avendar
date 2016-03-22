/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,	   *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *									   *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael	   *
 *  Chastain, Michael Quan, and Mitchell Tse.				   *
 *									   *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc	   *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.						   *
 *									   *
 *  Much time and thought has gone into this software and you are	   *
 *  benefitting.  We hope that you share your changes too.  What goes	   *
 *  around, comes around.						   *
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

DECLARE_DO_FUN( do_tell );

void do_heal(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *mob;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int cost,sn;
    SPELL_FUN *spell;
    char *words;	

    /* check for healer */
    for ( mob = ch->in_room->people; mob; mob = mob->next_in_room )
    {
        if ( IS_NPC(mob) && IS_SET(mob->act, ACT_IS_HEALER) )
            break;
    }
 
    if ( mob == NULL )
    {
        send_to_char( "You can't do that here.\n\r", ch );
        return;
    }

    if (mob->fighting)
    {
	act("$N is too busy fighting to attend you.", ch, NULL, mob, TO_CHAR);
	return;
    }

    if (!can_see(mob, ch))
    {
	act("$N cannot see you.", ch, NULL, mob, TO_CHAR);
	return;
    }

    one_argument(argument,arg);

    if (arg[0] == '\0')
    {
        /* display price list */
	act("$N says, 'I offer the following spells:'",ch,NULL,mob,TO_CHAR);
	send_to_char("  light: cure light wounds       5 gold\n\r",ch);
	send_to_char("  serious: cure serious wounds   7 gold\n\r",ch);
	send_to_char("  critic: cure critical wounds  12 gold\n\r",ch);
	send_to_char("  heal: healing spell	      25 gold\n\r",ch);
	send_to_char("  blind: cure blindness         10 gold\n\r",ch);
	send_to_char("  disease: cure disease          7 gold\n\r",ch);
	send_to_char("  poison:  cure poison	      12 gold\n\r",ch); 
	send_to_char("  uncurse: remove curse	      25 gold\n\r",ch);
	send_to_char("  refresh: restore movement      2 gold\n\r",ch);
	send_to_char("  mana:  restore mana	       5 gold\n\r",ch);
	send_to_char(" Type heal <type> to receive that spell.\n\r\n\r",ch);
	send_to_char(" Type 'heal resurrect' to be returned from ghostly form.\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "resurrect"))
    {
	AFFECT_DATA *paf;

	if (ch->perm_stat[STAT_CON] < 4)
	{
	    sprintf(buf, "%s Your spirit is too far gone from this world, %s.  You are in the hands of the gods now.", ch->unique_name, PERS(ch, mob));
	    do_tell(mob, buf);
	    return;
	}

	if (!IS_OAFFECTED(ch, AFF_GHOST))
	{
	    act("$N raises an eyebrow at you.", ch, NULL, mob, TO_CHAR);
	    act("$N raises an eyebrow at $n.", ch, NULL, mob, TO_ROOM);
	    sprintf(buf, "%s I cannot resurrect those who are already living, %s.", ch->unique_name, PERS(ch, mob));
	    do_tell(mob, buf);
	    return;
 	}

	if ((paf = affect_find(ch->affected, gsn_ghost)) == NULL)
	    return;

	if ((paf->duration >= 30) && (ch->level > 10))
	{
	    sprintf(buf, "%s The taint of death still hangs too heavy upon your soul, %s.  I cannot resurrect you yet.", ch->unique_name, PERS(ch, mob));
	    do_tell(mob, buf);
	    return;
	}

	send_to_char("The healing magic takes hold, and your spirit is bound to a newly resurrected form!\n\r", ch);
	act("$N gestures towards $n, and $n's ghostly form solidifies in this world.", ch, NULL, mob, TO_ROOM);
	affect_strip(ch, gsn_ghost);
	sprintf(buf,"%s resurrected %s in room %d.",mob->short_descr,ch->name,mob->in_room->vnum);
	wiznet(buf,ch,NULL,WIZ_DEATHS,0,0);
	log_string(buf);
	return;
    }

    if (!str_prefix(arg,"light"))
    {
        spell = spell_cure_light;
	sn    = skill_lookup("cure light");
	words = "judicandus dies";
	 cost  = 500;
    }

    else if (!str_prefix(arg,"serious"))
    {
	spell = spell_cure_serious;
	sn    = skill_lookup("cure serious");
	words = "judicandus gzfuajg";
	cost  = 700;
    }

    else if (!str_prefix(arg,"critical"))
    {
	spell = spell_cure_critical;
	sn    = skill_lookup("cure critical");
	words = "judicandus qfuhuqar";
	cost  = 1200;
    }

    else if (!str_prefix(arg,"heal"))
    {
	spell = spell_heal;
	sn = skill_lookup("heal");
	words = "pzar";
	cost  = 2500;
    }

    else if (!str_prefix(arg,"blindness"))
    {
	spell = spell_cure_blindness;
	sn    = skill_lookup("cure blindness");
      	words = "judicandus noselacri";		
        cost  = 1000;
    }

    else if (!str_prefix(arg,"disease"))
    {
	spell = spell_cure_disease;
	sn    = skill_lookup("cure disease");
	words = "judicandus eugzagz";
	cost = 700;
    }

    else if (!str_prefix(arg,"poison"))
    {
	spell = spell_cure_poison;
	sn    = skill_lookup("cure poison");
	words = "judicandus sausabru";
	cost  = 1200;
    }
	
    else if (!str_prefix(arg,"uncurse") || !str_prefix(arg,"curse"))
    {
	spell = spell_remove_curse; 
	sn    = skill_lookup("remove curse");
	words = "candussido judifgz";
	cost  = 2500;
    }

    else if (!str_prefix(arg,"mana") || !str_prefix(arg,"energize"))
    {
        spell = NULL;
        sn = -1;
        words = "energizer";
        cost = 500;
    }

	
    else if (!str_prefix(arg,"refresh") || !str_prefix(arg,"moves"))
    {
	spell =  spell_refresh;
	sn    = skill_lookup("refresh");
	words = "candusima"; 
	cost  = 200;
    }

    else 
    {
	act("$N says, 'Type 'heal' for a list of spells.'",
	    ch,NULL,mob,TO_CHAR);
	return;
    }

    if (cost > coins_to_value(ch->coins))
    {
	act("$N says, 'You do not have enough coin for my services.'",
	    ch,NULL,mob,TO_CHAR);
	return;
    }

    WAIT_STATE(ch,PULSE_VIOLENCE);

    deduct_cost(ch, (float) cost);
    inc_player_coins(mob, value_to_coins((float) cost, FALSE));
    act("$n utters the words, '$T'.",mob,NULL,words,TO_ROOM);
  
    if (spell == NULL)  /* restore mana trap...kinda hackish */
    {
	ch->mana += dice(2,8) + mob->level / 3;
	ch->mana = UMIN(ch->mana,ch->max_mana);
	send_to_char("A warm glow passes through you.\n\r",ch);
	return;
     }

     if (sn == -1)
	return;
    
     spell(sn,mob->level,mob,ch,TARGET_CHAR);
}
