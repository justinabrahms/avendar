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
#include <sstream>
#include <cmath>
#include "merc.h"
#include "magic.h"
#include "recycle.h"
#include "languages.h"
#include "Luck.h"
#include "Oaths.h"
#include "spells_spirit_air.h"

/* command procedures needed */
DECLARE_DO_FUN(do_groups	);
DECLARE_DO_FUN(do_help		);
DECLARE_DO_FUN(do_say		);

unsigned int total_skill_spheres(int sn)
{
    if (sn >= MAX_SKILL)
        return 0;

    unsigned int total(0);
    for (unsigned int i(0); i < MAX_SKILL_SPHERE; ++i)
    {
        if (skill_table[sn].spheres[i] != SPH_NONE)
            ++total;
    }

    return total;
}
bool is_skill_sphere_any(int sn, int sphere)
{
    // Sanity-check
    if (sn < 0 || sn >= MAX_SKILL)
        return false;

    // Check whether any of the spheres for this skill match the argument
    for (unsigned int i(0); i < MAX_SKILL_SPHERE; ++i)
    {
        if (skill_table[sn].spheres[i] == sphere)
            return true;
    }

    return false;
}

bool is_skill_sphere_only(int sn, int sphere)
{
    // Sanity-check
    if (sn < 0 || sn >= MAX_SKILL)
        return false;

    // Check whether all of the spheres for this skill match the argument
    bool any(false);
    for (unsigned int i(0); i < MAX_SKILL_SPHERE; ++i)
    {
        // There must be at least one sphere matching the argument, and NONE doesn't count against
        if (skill_table[sn].spheres[i] == sphere)
            any = true;
        else if (skill_table[sn].spheres[i] != SPH_NONE)
            return false;
    }

    return any;
}

/* used to get new skills */
void do_gain(CHAR_DATA *ch, char *argument)
{
/*    char buf[MAX_STRING_LENGTH];
  */  char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *trainer;
/*    int gn = 0, sn = 0;
*/
    if (IS_NPC(ch))
	return;

    /* find a trainer */
    for ( trainer = ch->in_room->people; 
	  trainer != NULL; 
	  trainer = trainer->next_in_room)
	if (IS_NPC(trainer) && IS_SET(trainer->act,ACT_GAIN))
	    break;

    if (trainer == NULL || !can_see(ch,trainer))
    {
	send_to_char("You can't do that here.\n\r",ch);
	return;
    }

    one_argument(argument,arg);

    if (arg[0] == '\0')
    {
	do_say(trainer,"Pardon me?");
	return;
    }

    if (!str_prefix(arg,"revert"))
    {
	if (ch->train < 1)
	{
	    act("$N tells you 'You have no trains to learn from.'",
		ch,NULL,trainer,TO_CHAR);
	    return;
	}

	act("$N helps you apply your training to practical matters.",
		ch,NULL,trainer,TO_CHAR);
	ch->practice += 10;
	ch->train -=1 ;
	return;
    }

    if (!str_prefix(arg,"convert"))
    {
	if (ch->practice < 10)
	{
	    act("$N tells you 'You are not yet ready.'",
		ch,NULL,trainer,TO_CHAR);
	    return;
	}

	act("$N helps you apply your practice to training.",
		ch,NULL,trainer,TO_CHAR);
	ch->practice -= 10;
	ch->train +=1 ;
	return;
    }

    act("$N tells you 'I do not understand...'",ch,NULL,trainer,TO_CHAR);
    return;
}
    



/* RT spells and skills show the players spells (or skills) */

void do_spells(CHAR_DATA *ch, char *argument)
{
    BUFFER *buffer;
    char spell_list[LEVEL_HERO + 1][MAX_STRING_LENGTH];
    int sn, level, mana;
    bool fAll = FALSE, found = FALSE;
    char buf[MAX_STRING_LENGTH];

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

    fAll = TRUE;

    /* initialize data */
    for (level = 0; level < LEVEL_HERO + 1; level++)
        spell_list[level][0] = '\0';
 
    for (sn = 0; sn < MAX_SKILL; sn++)
    {
        if (skill_table[sn].name == NULL )
	    break;

	if ((level = skill_table[sn].skill_level[ch->class_num]) < LEVEL_HERO + 1
	&&  (fAll || level <= ch->level)
	&&  skill_table[sn].spell_fun != spell_null
	&&  skill_table[sn].spell_fun != spell_form
	&&  skill_table[sn].spell_fun != spell_focus
	&&  skill_table[sn].spell_fun != spell_lang
	&&  skill_table[sn].spell_fun != spell_song
	&&  pcdata->learned[sn] > 0)
        {
	    found = TRUE;
	    level = skill_table[sn].skill_level[ch->class_num];
	    if (ch->level < level)
	    	sprintf(buf,"n/a    %-40s", skill_table[sn].name);
	    else
	    {
		if ((sn == gsn_sanctuary) 
		 && (ch->class_num == global_int_class_watertemplar))
		{
		    if (BIT_GET(pcdata->traits, TRAIT_MAGAPT))
			mana = 100;
		    else
		        mana = UMAX(100, 100 / (2 + ch->level - level));
		}
		else
		{
		    if (BIT_GET(pcdata->traits, TRAIT_MAGAPT))
			mana = skill_table[sn].min_mana;
		    else
			mana = UMAX(skill_table[sn].min_mana, 100/(2 + ch->level - level));
		}
	        sprintf(buf,"%3d%%    %3d mana    %-40s",get_skill(ch,sn, true),mana,skill_table[sn].name);
	    }
 
	    if (spell_list[level][0] == '\0')
          	sprintf(spell_list[level],"\n\rLevel %2d:   %s",level,buf);
	    else
	    {
		strcat(spell_list[level],"\n\r            ");
          	strcat(spell_list[level],buf);
	    }
	}
    }
 
    /* return results */
 
    if (!found)
    {
      	send_to_char("No spells found.\n\r",ch);
      	return;
    }

    buffer = new_buf();
    for (level = 0; level < LEVEL_HERO + 1; level++)
      	if (spell_list[level][0] != '\0')
	    add_buf(buffer,spell_list[level]);
    add_buf(buffer,"\n\r");
    page_to_char(buf_string(buffer),ch);
    free_buf(buffer);
}

void do_forms(CHAR_DATA *ch, char *argument)
{
    BUFFER *buffer;
    char spell_list[LEVEL_HERO + 1][MAX_STRING_LENGTH];
    int sn, level;
    bool fAll = FALSE, found = FALSE;
    char buf[MAX_STRING_LENGTH];
 
    if (IS_NPC(ch))
      return;

    fAll = TRUE;

    /* initialize data */
    for (level = 0; level < LEVEL_HERO + 1; level++)
        spell_list[level][0] = '\0';
 
    for (sn = 0; sn < MAX_SKILL; sn++)
    {
        if (skill_table[sn].name == NULL )
	    break;

	if ((level = skill_table[sn].skill_level[ch->class_num]) < LEVEL_HERO + 1
	&&  (fAll || level <= ch->level)
	&&  skill_table[sn].spell_fun == spell_form
	&&  ch->pcdata->learned[sn] > 0)
        {
	    found = TRUE;
	    level = skill_table[sn].skill_level[ch->class_num];
	    if (ch->level < level)
	    	sprintf(buf,"n/a    %-40s", skill_table[sn].name);
	    else
	        sprintf(buf,"%3d%%    %3d mana    %-40s",get_skill(ch,sn, true),
		  skill_table[sn].min_mana,skill_table[sn].name);
 
	    if (spell_list[level][0] == '\0')
          	sprintf(spell_list[level],"\n\rLevel %2d:   %s",level,buf);
	    else
	    {
//         
		strcat(spell_list[level],"\n\r            ");
          	strcat(spell_list[level],buf);
	    }
	}
    }
 
    /* return results */
 
    if (!found)
    {
      	send_to_char("No forms found.\n\r",ch);
      	return;
    }

    buffer = new_buf();
    for (level = 0; level < LEVEL_HERO + 1; level++)
      	if (spell_list[level][0] != '\0')
	    add_buf(buffer,spell_list[level]);
    add_buf(buffer,"\n\r");
    page_to_char(buf_string(buffer),ch);
    free_buf(buffer);
}

void do_songs(CHAR_DATA *ch, char *argument)
{
    BUFFER *buffer;
    char skill_list[LEVEL_HERO + 1][MAX_STRING_LENGTH];
    char skill_columns[LEVEL_HERO + 1];
    int sn, level;
    bool fAll = FALSE, found = FALSE;
    char buf[MAX_STRING_LENGTH];
 
    if (IS_NPC(ch))
      return;

    fAll = TRUE;

    /* initialize data */
    for (level = 0; level < LEVEL_HERO + 1; level++)
    {
        skill_columns[level] = 0;
        skill_list[level][0] = '\0';
    }
 
    for (sn = 0; sn < MAX_SKILL; sn++)
    {
        if (skill_table[sn].name == NULL )
	    break;

	if ((level = skill_table[sn].skill_level[ch->class_num]) < LEVEL_HERO + 1
	&&  (fAll || level <= ch->level)
	/*&&  level >= min_lev && level <= max_lev */
	&&  skill_table[sn].spell_fun == spell_song
	&&  ch->pcdata->learned[sn] > 0)
        {
	    found = TRUE;
	    level = skill_table[sn].skill_level[ch->class_num];
	    if (ch->level < level)
	    	sprintf(buf,"%-24s   n/a     ", skill_table[sn].name);
	    else
	    	sprintf(buf,"%-24s   %3d%%    ",skill_table[sn].name,
		    ch->pcdata->learned[sn]);
 
	    if (skill_list[level][0] == '\0')
          	sprintf(skill_list[level],"\n\rLevel %2d: %s",level,buf);
	    else /* append */
	    {
          	if ( ++skill_columns[level] % 2 == 0)
		    strcat(skill_list[level],"\n\r          ");
          	strcat(skill_list[level],buf);
	    }
	}
    }
 
    /* return results */
 
    if (!found)
    {
      	send_to_char("No forms found.\n\r",ch);
      	return;
    }

    buffer = new_buf();
    for (level = 0; level < LEVEL_HERO + 1; level++)
      	if (skill_list[level][0] != '\0')
	    add_buf(buffer,skill_list[level]);
    add_buf(buffer,"\n\r");
    page_to_char(buf_string(buffer),ch);
    free_buf(buffer);
}

void do_skills(CHAR_DATA *ch, char *argument)
{
    BUFFER *buffer;
    char skill_list[LEVEL_HERO + 1][MAX_STRING_LENGTH];
    char skill_columns[LEVEL_HERO + 1];
    int sn, level, skill;
    bool fAll = FALSE, found = FALSE;
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj = get_eq_char(ch,WEAR_WIELD); 
   
    PC_DATA * pcdata(ch->pcdata);
    if (pcdata == NULL)
    {
        CHAR_DATA * original(find_bilocated_body(ch));
        if (original != NULL)
            pcdata = original->pcdata;
    }
    
    if (pcdata == NULL)
    {
        send_to_char("NPCs cannot get a skill listing.\n", ch);
        return;
    }

	fAll = TRUE;

    /* initialize data */
    for (level = 0; level < LEVEL_HERO + 1; level++)
    {
        skill_columns[level] = 0;
        skill_list[level][0] = '\0';
    }
 
    for (sn = 0; sn < MAX_SKILL; sn++)
    {
        if (skill_table[sn].name == NULL )
	        break;

	    if ((level = skill_table[sn].skill_level[ch->class_num]) <= LEVEL_HERO &&  (fAll || level <= ch->level)
    	&&  skill_table[sn].spell_fun == spell_null	&& pcdata->learned[sn] > 0)
        {
	        found = TRUE;
	        if (obj != NULL && get_weapon_sn_from_obj(obj) == sn)
    	    	skill = get_weapon_skill_weapon(ch, obj, true);
	        else
	            skill = get_skill(ch, sn, true);
    	    
            // If you know the skill only at 1% and aren't high enough level, you get an n/a
            if (pcdata->learned[sn] == 1 && ch->level < level)
	            sprintf(buf,"%-18s n/a      ", skill_table[sn].name);
            // If you have a skill modifier for some reason, you get both displayed
	        else if (skill != pcdata->learned[sn])
    	        sprintf(buf,"%-18s %3d%%(%3d%%)  ", skill_table[sn].name, pcdata->learned[sn], skill);
	        else
    	    	sprintf(buf,"%-18s %3d%%        ",skill_table[sn].name, pcdata->learned[sn]);
 
	        if (skill_list[level][0] == '\0')
              	sprintf(skill_list[level],"\n\rLevel %2d: %s",level,buf);
    	    else /* append */
	        {
              	if ( ++skill_columns[level] % 2 == 0)
    		    strcat(skill_list[level],"\n\r          ");
          	    strcat(skill_list[level],buf);
	        }
	    }
    }
 
    /* return results */
 
    if (!found)
    {
      	send_to_char("No skills found.\n\r",ch);
      	return;
    }

    buffer = new_buf();
    for (level = 0; level < LEVEL_HERO + 1; level++)
      	if (skill_list[level][0] != '\0')
	    add_buf(buffer,skill_list[level]);
    add_buf(buffer,"\n\r");
    page_to_char(buf_string(buffer),ch);
    free_buf(buffer);
}

int exp_per_level(CHAR_DATA *ch, int points)
{
    int expl,inc;

    if (IS_NPC(ch))
	return 1000; 

    expl = 1000;
    inc = 500;

    if (points < 40)
	return 1000 * (pc_race_table[ch->race].class_mult[ch->class_num] ?
		       pc_race_table[ch->race].class_mult[ch->class_num]/100 : 1);

    /* processing */
    points -= 40;

    while (points > 9)
    {
	expl += inc;
        points -= 10;
        if (points > 9)
	{
	    expl += inc;
	    inc *= 2;
	    points -= 10;
	}
    }

    expl += points * inc / 10;  

    return expl * pc_race_table[ch->race].class_mult[ch->class_num]/100;
}

/* Xurinos uses this function to get exp on a level */
int exp_on_level(CHAR_DATA *ch, int level)
{
    int exp/*, lvl*/;
    int i;
    double factor;

    for (factor = 1,i = 1; i < ch->level+2; i++)
    {
	factor *= 1.08;
    }

    exp = round((300 * (level+2) * (level+3))/2 + 1800*factor) - 2000;

  return exp;
}

/* shows all groups, or the sub-members of a group */
void do_groups(CHAR_DATA *ch, char *argument)
{
    char buf[100];
    int gn,sn,col;

    if (IS_NPC(ch))
	return;

    col = 0;

    if (argument[0] == '\0')
    {   /* show all groups */
	
	for (gn = 0; gn < MAX_GROUP; gn++)
        {
	    if (group_table[gn].name == NULL)
		break;
	    if (ch->pcdata->group_known[gn])
	    {
		sprintf(buf,"%-25s ",group_table[gn].name);
		send_to_char(buf,ch);
		if (++col % 3 == 0)
		    send_to_char("\n\r",ch);
	    }
        }
        if ( col % 3 != 0 )
            send_to_char( "\n\r", ch );
        sprintf(buf,"Creation points: %d\n\r",ch->pcdata->points);
	send_to_char(buf,ch);
	return;
     }

     if (!str_cmp(argument,"all"))    /* show all groups */
     {
        for (gn = 0; gn < MAX_GROUP; gn++)
        {
            if (group_table[gn].name == NULL)
                break;
	    sprintf(buf,"%-25s ",group_table[gn].name);
            send_to_char(buf,ch);
	    if (++col % 3 == 0)
            	send_to_char("\n\r",ch);
        }
        if ( col % 3 != 0 )
            send_to_char( "\n\r", ch );
	return;
     }
	
     
     /* show the sub-members of a group */
     gn = group_lookup(argument);
     if (gn == -1)
     {
	send_to_char("No group by that name exists.\n\r",ch);
	send_to_char(
	    "Type 'info all' for a full listing.\n\r",ch);
	return;
     }

     for (sn = 0; sn < MAX_IN_GROUP; sn++)
     {
	if (group_table[gn].spells[sn] == NULL)
	    break;
	sprintf(buf,"%-25s ",group_table[gn].spells[sn]);
	send_to_char(buf,ch);
	if (++col % 3 == 0)
	    send_to_char("\n\r",ch);
     }
    if ( col % 3 != 0 )
        send_to_char( "\n\r", ch );
}

static bool can_learn_skill(const CHAR_DATA & ch, int sn)
{
    if (ch.level < skill_table[sn].skill_level[ch.class_num]) return false;     // Not at the required level yet
    if (skill_table[sn].rating[ch.class_num] == 0) return false;                // Skill learning disabled for this class
    if (ch.pcdata->learned[sn] == 0 && skill_table[sn].spell_fun != spell_lang) return false; // Have not yet learned the skill
    if (ch.pcdata->learned[sn] == 100) return false;                            // Already maxed the skill
  	return true;
}

/* checks for skill improvement */
void check_improve(CHAR_DATA *ch,CHAR_DATA *victim, int sn, bool success, int multiplier)
{
    int chance=0;
    int range=0;
    int langnum = -1;
    int xp = 0;
    char buf[100];
    int rsn = 0;

    if (IS_NPC(ch))
	return;
    
    if (sn == gsn_shield_block && ch->class_num == global_int_class_bandit)
	    rsn = gsn_brawlingblock;
    else
    	rsn = sn;
 
    if (!can_learn_skill(*ch, rsn))
        return;

    if (skill_table[rsn].spell_fun == spell_lang)
    {
	int i;

	for (i = 0; i < MAX_LANGUAGE; i++)
	{
	    if (rsn == *lang_data[i].sn) 
	    {
		if (IS_SET(ch->lang_flags, (1 << i)))
		{
		    return;
		}
		else
		{
		    langnum = i;
		}
		break;
	    }
	}
    }

    /* check to see if the character has a chance to learn */
    chance = 10 * learn_table[get_curr_stat(ch,STAT_INT)];
    if (multiplier != 0)
        chance /= (		multiplier
		*	skill_table[rsn].rating[ch->class_num] 
		*	4);
    chance += ch->level;

    if (IS_SET(ch->nact, PLR_HARDCORE))
        chance = chance * 5 / 4;

    // Luck check
    switch (Luck::Check(*ch))
    {
        case Luck::Lucky: chance = (chance * 3) / 2; break;
        case Luck::Unlucky: chance /= 2; break;
        default: break;
    }

//    if (!success && BIT_GET(ch->pcdata->traits, TRAIT_CRITICAL))
    if (!success && ch->desc && ch->desc->acct)
	chance = chance * 7 / 6;

    if (skill_table[sn].spell_fun == spell_lang && BIT_GET(ch->pcdata->traits, TRAIT_LINGUIST))
	chance *= 2;
    
    /*  Experimental code to modify the way combat skills are learned. */
    if (victim && multiplier != 0)
    {
        range = (victim->level - ch->level);
        chance = (chance * UMAX(0,range + 9)) / 9;
    }

    if (number_range(1, 1000) > chance)
	return;

    /* now that the character has a CHANCE to learn, see if they really have */	

    bool skillImproved(false);
    if (success)
    {
        chance = URANGE(5,100 - ch->pcdata->learned[rsn], 95);
        if (multiplier == 0 ? number_bits(1) == 0 : number_percent() < chance)
        {
            skillImproved = true;
            ch->pcdata->learned[rsn]++;
    // brazen: Adjusting xp for skill gains. For skill gains over 75%, XP
    // awarded is equal to the new skill %
            if(ch->pcdata->learned[rsn] > 75)
            {
                xp = ch->pcdata->learned[rsn];
                gain_exp(ch,xp);
                sprintf(buf,"You have become better at %s, and gain %d experience!\n\r", skill_table[rsn].name,xp);
            }
            else
                sprintf(buf,"You have become better at %s!\n\r",skill_table[rsn].name);

            send_to_char(buf,ch);

            if (langnum > -1)
            {
                AFFECT_DATA af = {0};
                af.where	= TO_LANG;
                af.type		= rsn;
                af.modifier	= 0;
                af.location	= APPLY_HIDE;
                af.duration	= 6;
                af.level	= ch->level;
                af.bitvector	= (1 << langnum);
                affect_to_char(ch, &af);
            }
        }
    }
    else
    {
        chance = URANGE(5,ch->pcdata->learned[rsn]/2,multiplier == 0 ? 95 : 30);
        if (number_percent() < chance)
        {
            skillImproved = true;
            ch->pcdata->learned[rsn] += number_range(1,3);
            ch->pcdata->learned[rsn] = UMIN(ch->pcdata->learned[rsn],100);
    // brazen: Adjusting xp for skill gains. For skill gains over 75%, XP
    // awarded is equal to the new skill %
                if (ch->pcdata->learned[rsn] > 75)
            {
                xp = ch->pcdata->learned[rsn];
                gain_exp(ch,xp);
                sprintf(buf,"Your skill at %s improves, and you gain %d experience points.\n\r",skill_table[rsn].name,xp);
            }
            else
                sprintf(buf,"Your skill at %s improves.\n\r",skill_table[rsn].name);
            send_to_char(buf,ch);

            if (langnum > -1)
            {
            AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;

            af.where	= TO_LANG;
            af.type		= rsn;
            af.modifier	= 0;
            af.location	= APPLY_HIDE;
            af.duration	= 6;
            af.level	= ch->level;
            af.bitvector	= (1 << langnum);
            affect_to_char(ch, &af);
            }

         }
    }

    struct OathImprover
    {
        static void Improve(CHAR_DATA * ch, CHAR_DATA * ch_orig, int sn)
        {
            if (!can_learn_skill(*ch, sn))
                return;
            
            std::ostringstream mess;
            mess << "You watch $N's technique carefully, and your skill at " << skill_table[sn].name << " improves!";
            act(mess.str().c_str(), ch, NULL, ch_orig, TO_CHAR);
            ++ch->pcdata->learned[sn];

            if (ch->pcdata->learned[sn] > 75)
            {
                mess.str("");
                mess << "You gain " << ch->pcdata->learned[sn] << " experience points.\n";
                send_to_char(mess.str().c_str(), ch);
                gain_exp(ch, ch->pcdata->learned[sn]);
            }
        }
    };

    // Check for shared skill improvement
    if (skillImproved && number_bits(4) == 0 && ch->in_room != NULL)
    {
        // Improve anyone in the room who this ch is beholden to
        CHAR_DATA * oathHolder(Oaths::OathHolderFor(*ch));
        if (oathHolder != NULL && oathHolder->in_room == ch->in_room)
            OathImprover::Improve(oathHolder, ch, rsn);

        // Iterate the room, looking for oath bondsman
        for (CHAR_DATA * ch_oath(ch->in_room->people); ch_oath != NULL; ch_oath = ch_oath->next_in_room)
        {
            if (ch_oath != ch && Oaths::OathHolderFor(*ch_oath) == ch)
                OathImprover::Improve(ch_oath, ch, rsn);
        }
    }
}

bool group_contains(CHAR_DATA * ch, int skillNum)
{
    int group(group_lookup(class_table[ch->class_num].base_group));
    if (group >= 0 && group_contains(group, skillNum)) return true;
    group = group_lookup(class_table[ch->class_num].default_group);
    if (group >= 0 && group_contains(group, skillNum)) return true;
    return false;
}

/* recursively returns whether a group contains a skill */
bool group_contains(int gn, int skillNum)
{
    // Iterate the entries in this group
    for (unsigned int i(0); i < MAX_IN_GROUP && group_table[gn].spells[i] != NULL; ++i)
    {
        // Check for a skill
        int sn = skill_lookup_full(group_table[gn].spells[i]);
        if (sn >= 0)
        {
            // Found a skill; check for a match
            if (sn == skillNum)
                return true;

            continue;
        }

        // Check for a group
        int group = group_lookup(group_table[gn].spells[i]);
        if (group >= 0)
        {
            // Found a group; check it recursively for a match
            if (group_contains(group, skillNum))
                return true;

            continue;
        }

        // Neither a skill nor a group
        std::ostringstream mess;
        mess << "Invalid skill/group found [Group '" << group_table[gn].spells[i] << "'] [Parent: '" << group_table[gn].name << "']";
        bug(mess.str().c_str(), 0);
    }

    // Never found the target skill
    return false;
}

/* returns a group index number given the name */
int group_lookup( const char *name )
{
    int gn;
 
    for ( gn = 0; gn < MAX_GROUP; gn++ )
    {
        if ( group_table[gn].name == NULL )
            break;
        if ( LOWER(name[0]) == LOWER(group_table[gn].name[0])
        &&   !strcmp( name, group_table[gn].name ) )
            return gn;
    }
 
    return -1;
}

/* recursively adds a group given its number -- uses group_add */
void gn_add( CHAR_DATA *ch, int gn)
{
    int i;
    
    ch->pcdata->group_known[gn] = TRUE;
    for ( i = 0; i < MAX_IN_GROUP; i++)
    {
        if (group_table[gn].spells[i] == NULL)
            break;
        group_add(ch,group_table[gn].spells[i],FALSE);
    }
}

/* recusively removes a group given its number -- uses group_remove */
void gn_remove( CHAR_DATA *ch, int gn)
{
    int i;

    ch->pcdata->group_known[gn] = FALSE;

    for ( i = 0; i < MAX_IN_GROUP; i ++)
    {
	if (group_table[gn].spells[i] == NULL)
	    break;
	group_remove(ch,group_table[gn].spells[i]);
    }
}

void do_wizgroupadd( CHAR_DATA *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int sn,gn;
    bool deduct = FALSE;
    CHAR_DATA *victim;

    if (argument[0] == '\0')
	{
	send_to_char("Add to whom what group? (syntax: groupadd <ch> <group name>)\n\r", ch);
	return;
	}

    argument = one_argument(argument, arg1);

    if (argument[0] == '\0')
	{
	send_to_char("Add to whom what group? (syntax: groupadd <ch> <group name>)\n\r", ch);
	return;
	}


    deduct = FALSE;
    argument = one_argument(argument, arg2);

    if ((victim = get_char_world(ch, arg1)) == NULL)
	{
	send_to_char("You couldn't find them.\n\r", ch);
	return;
	}

    if (IS_NPC(victim)) /* NPCs do not have skills */
	{
	send_to_char("Not on NPCs.\n\r", ch);
	return;
	}

    sn = skill_lookup_full(arg2);

    if (sn != -1)
    {
	    if (victim->pcdata->learned[sn] == 0) /* i.e. not known */
    	{
	        if (deduct)
    	    	victim->pcdata->learned[sn] = 65;
           	else
	        	victim->pcdata->learned[sn] = 1;

    	    if (skill_table[sn].house != 0)
	        	victim->pcdata->learned[sn] = 65;
    	}
	    return;
    }
	
    /* now check groups */

    gn = group_lookup(arg2);

    if (gn != -1)
    {
	if (victim->pcdata->group_known[gn] == FALSE)  
	{
	    victim->pcdata->group_known[gn] = TRUE;
	}
	gn_add(victim,gn); /* make sure all skills in the group are known */
    }
}

/* use for processing a skill or group for addition  */
void group_add( CHAR_DATA *ch, const char *name, bool deduct)
{
    int sn,gn;

    if (IS_NPC(ch)) /* NPCs do not have skills */
	return;

    sn = skill_lookup_full(name);

    if (sn != -1)
    {
	if (ch->pcdata->learned[sn] == 0) /* i.e. not known */
	{
	    if (deduct)
	    	ch->pcdata->learned[sn] = 65;
       	    else
	    	ch->pcdata->learned[sn] = 1;

	    if (skill_table[sn].house != 0)
		ch->pcdata->learned[sn] = 65;
	}

	return;
    }
	
    /* now check groups */

    gn = group_lookup(name);

    if (gn != -1)
    {
	if (ch->pcdata->group_known[gn] == FALSE)  
	{
	    ch->pcdata->group_known[gn] = TRUE;
	}
	gn_add(ch,gn); /* make sure all skills in the group are known */
    }
}

/* used for processing a skill or group for deletion -- no points back! */

void group_remove(CHAR_DATA *ch, const char *name)
{
    int sn, gn;
    
     sn = skill_lookup(name);

    if (sn != -1)
    {
	ch->pcdata->learned[sn] = 0;
	return;
    }
 
    /* now check groups */
 
    gn = group_lookup(name);
 
    if (gn != -1 && ch->pcdata->group_known[gn] == TRUE)
    {
	ch->pcdata->group_known[gn] = FALSE;
	gn_remove(ch,gn);  /* be sure to call gn_add on all remaining groups */
    }
}
