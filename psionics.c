#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cmath>
#include "merc.h"
#include "magic.h"
#include "psionics.h"
#include "recycle.h"
#include "spells_fire_air.h"
#include "spells_spirit.h"
#include "ShadeControl.h"

/** external declarations **/
DECLARE_DO_FUN( do_autoyell   	);
DECLARE_DO_FUN( do_visible 	);
DECLARE_DO_FUN( do_wake		);

extern	void	unassume_form	args( ( CHAR_DATA *ch ) );
extern	char *	target_name;

const   struct  focus_type      focus_table     []      =
{
    { abil_concealremains,	&gsn_concealremains,	2,	1	},
    { abil_celerity,		&gsn_celerity,		1,	1	},
    { abil_acuity,		&gsn_acuity,		1,	1	},
    { abil_shadowfist,		&gsn_shadowfist,	1,	1	},
    { abil_cloak,		&gsn_cloak,		2,	1	},
    { abil_block_vision,	&gsn_block_vision,	1,	1	},
    { abil_mindshield,		&gsn_mindshield,	1,	1	},
    { abil_enhance_reflexes,	&gsn_enhance_reflexes,	1,	1	},
    { abil_paranoia,		&gsn_paranoia,		1,	1	},
    { abil_forget,		&gsn_forget,		1,	1	},
    { abil_enhance_pain,	&gsn_enhance_pain,	1,	1	},
    { abil_reduce_pain,		&gsn_reduce_pain,	1,	1	},
    { abil_slow_reflexes,	&gsn_slow_reflexes,	1,	1	},
    { abil_symbiont,		&gsn_symbiont,		2,	1	},
    { abil_leech,		&gsn_leech,		1,	1	},
    { abil_deflection,		&gsn_deflection,	1,	1	},
    { abil_levitation,		&gsn_levitation,	1,	1	},
    { abil_esp,			&gsn_esp,		1,	1	},
    { abil_read_thoughts,	&gsn_read_thoughts,	1,	1	},
    { abil_ignore_pain,		&gsn_ignore_pain,	1,	1	},
    { abil_detect_life,		&gsn_detect_life,	1,	1	},
    { abil_psychic_block,	&gsn_psychic_block,	1,	1	},
    { abil_mind_thrust,		&gsn_mind_thrust,	0,	0	},
    { abil_psionic_blast,	&gsn_psionic_blast,	0,	0	},
    { abil_suggestion,		&gsn_suggestion,	0,	1	},
    { abil_sensory_vision,	&gsn_sensory_vision,	1,	1	},
    { abil_vertigo,		&gsn_vertigo,		1,	1	},
    { abil_sense_danger,	&gsn_sense_danger,	1,	1	},
    { abil_foresight,		&gsn_foresight,		1,	1	},
    { abil_dominance,		&gsn_dominance,		4,	1	},
    { abil_confusion,		&gsn_confusion,		1,	1	},
    { abil_prowess,		&gsn_prowess,		1,	1	},
    { abil_adrenaline_rush,	&gsn_adrenaline_rush,	2,	2	},
    { abil_overwhelm,		&gsn_overwhelm,		4,	1	},
    { abil_conceal_thoughts,	&gsn_conceal_thoughts,	2,	1	},
    { abil_ordered_mind,	&gsn_ordered_mind,	1,	1	},
    { abil_shove,		&gsn_shove,		0,	0	},
    { abil_accelerated_healing,	&gsn_accelerated_healing,1,	1	},
    { abil_borrow_knowledge,	&gsn_borrow_knowledge,	2,	1	},
    { NULL, 			NULL, 			0 	}
};

/* check hack variable */
CHAR_DATA *	psi_ignore_target;


void do_focus( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    int x, chance, sn;
    void *vo;
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    int target;

    target_name = one_argument( argument, arg1 );
    one_argument( target_name, arg2 );

    if (IS_NPC(ch))
	return;

    if (is_affected(ch, gsn_astralprojection))
    {
	send_to_char("You cannot focus your thoughts properly in the astral plane.\n\r", ch);
	return;
    }

    if (is_affected(ch, gsn_bludgeon) && (number_bits(2) != 0))
    {
	send_to_char("You're too dazed to concentrate.\n\r", ch);
	WAIT_STATE(ch, PULSE_VIOLENCE);
	return;
    }

    if ( arg1[0] == '\0' )
    {
	if (!ch->pcdata->focus_on[0])
	    send_to_char("You are not focusing any psionic abilities.\n\r", ch);
	else
	{
	    int maxslots = focus_slots(ch);

	    for (x = 0; x < maxslots; x++)
	    {
		if (ch->pcdata->focus_sn[x] > 0)
		    if (ch->pcdata->focus_on[x])
			sprintf(buf, "%d: %s, focused on %s.\n\r", x + 1, skill_table[ch->pcdata->focus_sn[x]].name, ch->pcdata->focus_ch[x] ? ((ch->pcdata->focus_ch[x] == ch) ? "yourself" : APERS(ch->pcdata->focus_ch[x], ch)) : "no one");
		    else
			sprintf(buf, "%d:   (%s)\n\r", x + 1, skill_table[ch->pcdata->focus_sn[x]].name);
		else
		    sprintf(buf, "%d: - empty -\n\r", x + 1);
		send_to_char(buf, ch);
	    }
	}

        return;
    }

    if ( ch->position < POS_FIGHTING )
    {
        switch( ch->position )
        {
        case POS_DEAD:
            send_to_char( "Lie still; you are DEAD.\n\r", ch );
            break;

        case POS_MORTAL:
        case POS_INCAP:
            send_to_char( "You are hurt far too bad for that.\n\r", ch );
            break;

        case POS_STUNNED:
            send_to_char( "You are too stunned to do that.\n\r", ch );
            break;

        case POS_SLEEPING:
            send_to_char( "In your dreams, or what?\n\r", ch );
            break;

        case POS_RESTING:
            send_to_char( "Nah... You feel too relaxed...\n\r", ch);
            break;

        case POS_SITTING:
            send_to_char( "Better stand up first.\n\r",ch);
            break;
	}
    }

    if ((sn = find_spell(ch,arg1, spell_focus)) < 1 || (get_skill(ch, sn) == 0))
    {
        send_to_char("You do not possess any abilities by that name.\n\r", ch);
        return;
    }

    if ( ch->position < skill_table[sn].minimum_position )
    {
        send_to_char( "You can't concentrate enough.\n\r", ch );
        return;
    }

    victim      = NULL;
    obj         = NULL;
    vo          = NULL;
    target      = TARGET_NONE;

    switch ( skill_table[sn].target )
    {
        default:
            bug( "Do_focus: bad target for sn %d.", sn );
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
                    send_to_char("Focus that ability on whom?\n\r", ch);
                    return;
                }
            }
            else
            {
                if ((victim = get_char_room(ch, target_name)) == NULL)
                {
		    if (ch->mindlink
		     && is_name(target_name, ch->mindlink->name))
		    {
			if (ch->mindlink->in_room && IS_SET(ch->mindlink->in_room->room_flags, ROOM_SAFE))
			{
			    send_to_char("A powerful force protects your victim.\n\r", ch);
			    return;
			}
			/* checks if trying to do psionic blast over mindlink -- transmitt */
			else if (*focus_table[18].sn == sn)
			{
                            send_to_char("You cannot properly focus a psionic blast over a mindlink.\n\r", ch);
                            return;
			}	
			else 
		            victim = ch->mindlink;
		    }
		    else
		    {
                        send_to_char("You don't see them here.\n\r", ch);
                        return;
		    }
                }
            }

            if (IS_OAFFECTED(victim, AFF_GHOST))
            {
                send_to_char("But they're a ghost!\n\r", ch);
                return;
            }

	    if ((sn == gsn_psychic_block || sn == gsn_mind_thrust)
	      && (IS_NPC(victim)
		|| (victim->class_num != global_int_class_psionicist
		  && victim->class_num != global_int_class_assassin)))
	    {
		send_to_char("They have no psychic ability to manipulate.\n\r",ch);
		return;
	    }

	    if (!can_be_affected(victim,sn))
	    {
		act("$N has no mind to affect.",ch,NULL,victim,TO_CHAR);
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

	    if (!can_be_affected(victim,sn))
	    {
		act("$N has no mind to affect.",ch,NULL,victim,TO_CHAR);
		return;
	    }

            vo = (void *) victim;
            target = TARGET_CHAR;
            break;
        
	case TAR_CHAR_DEF_GLOBAL:
            if ( arg2[0] == '\0' )
            {
		send_to_char("Who did you want to focus on?\n\r",ch);
		return;
	    }
            else
            {
                if ((victim = get_char_world(ch, target_name)) == NULL)
                {
                    send_to_char("You don't see them.\n\r", ch);
                    return;
                }
            }

	    if (!can_be_affected(victim,sn))
	    {
		act("$N has no mind to affect.",ch,NULL,victim,TO_CHAR);
		return;
	    }

            vo = (void *) victim;
            target = TARGET_CHAR;
            break;

        case TAR_CHAR_SELF:
            if ((arg2[0] != '\0') && !is_name(target_name, ch->name))
            {
                send_to_char("You cannot focus that ability on another.\n\r", ch);
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

    act("$n concentrates for a moment.", ch, NULL, NULL, TO_ROOM);

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
        send_to_char("You fail to focus properly.\n\r", ch );
        check_improve(ch,victim,sn,FALSE,2);
	    expend_mana(ch, skill_table[sn].min_mana);
    	WAIT_STATE( ch, skill_table[sn].beats );
    }
    else
    {
        if (victim && (target == TARGET_CHAR) && IS_NPC(victim)
         && (victim->pIndexData->vnum == MOB_VNUM_MIRROR_IMAGE))
        {
	    WAIT_STATE( ch, skill_table[sn].beats );
            act("$n turns translucent and disappears.",
		victim, NULL, NULL, TO_ROOM);
            extract_char(victim, TRUE);
            return;
        }

        if (victim && target == TARGET_CHAR && (checkPyrokineticMirrorAttacked(ch, victim) || ShadeControl::CheckShadeAttacked(*ch, *victim)))
        {
            WAIT_STATE(ch, skill_table[sn].beats);
            return;
        }

	if (focus(sn, (!victim || (ch->in_room == victim->in_room)) ? ch->level : round(ch->level * 0.8), ch, vo, target))
	{
	    if (sn == gsn_symbiont || sn == gsn_psychic_block)
		check_improve(ch,victim, sn, TRUE, 0);
	    else
		check_improve(ch,victim, sn, TRUE, 2);
	    expend_mana(ch, skill_table[sn].min_mana * 2);
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
				&& victim->mercy_from == NULL
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

bool focus(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    int i, j, k;
    int numslots;
    
    if (IS_NPC(ch))
	return FALSE;

    numslots = focus_slots(ch); 

    for (i = 0; focus_table[i].focus_fun; i++)
    {
        if (*focus_table[i].sn == sn)
        {
            if (focus_table[i].slots == 0)
                return ((*focus_table[i].focus_fun) (sn, level, ch, vo));
            
            for (j = 0; j < MAX_FOCUS; j++)
            {
                if (ch->pcdata->focus_sn[j] == 0)
                {
                    if ((j + focus_table[i].slots) > numslots)
                    {
                        send_to_char("You are already spreading your focus too thin to employ that ability.\n\r", ch);
                        return FALSE;
                    }

                    psi_ignore_target = NULL;
                    if ((*focus_table[i].focus_fun) (sn, level, ch, vo))
                    {
                        for (k = 0; k < focus_table[i].slots; k++)
                        {
                            ch->pcdata->focus_sn[(j+k)] = sn;
                            ch->pcdata->focus_ch[(j+k)] = psi_ignore_target ? psi_ignore_target : (CHAR_DATA*)vo;
                            ch->pcdata->focus_on[(j+k)] = (k == 0) ? TRUE : FALSE;
                        }
                        psi_ignore_target = NULL;
                        return TRUE;
                    }
                    return FALSE;
                }
            }

            send_to_char("You are already spreading your focus too thin to employ that ability.\n\r", ch);
            return FALSE;
        }
    }
	
    bug("Focus value for sn %d not found.", sn);
    return FALSE;
}

int focus_slots( CHAR_DATA *ch )
{
    int slots=0;
    if (ch->class_num == global_int_class_psionicist)
    {
	slots = URANGE(3, 3 + ch->level / 10, MAX_FOCUS);

	if (is_affected(ch, gsn_psychic_block))
	    slots -= 2;
    }
    else if (ch->class_num == global_int_class_assassin)
    {
	slots = URANGE(0, (ch->level > 9 && ch->level < 21) ? 1 : 1+(ch->level-10)/11, MAX_FOCUS/2);
	if (is_affected(ch, gsn_psychic_block))	
	    slots -= 2;
    }

    return slots;
}

void do_unfocus( CHAR_DATA *ch, char *argument )
{
    int slot = -1, x;
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
	return;

    if (argument[0] == '\0')
    {
	send_to_char("Unfocus which slot?\n\r", ch);
	return;
    }

    if (!is_number(argument))
    {
	argument = one_argument(argument, buf);
	for (x = 0; x < MAX_FOCUS; x++)
	{
	    if (ch->pcdata->focus_on[x])
	        if (!str_prefix(buf, skill_table[ch->pcdata->focus_sn[x]].name) && ((argument[0] == '\0') || ((ch->pcdata->focus_ch[x] == ch) && !str_cmp(argument, "self")) || is_name(argument, ch->pcdata->focus_ch[x]->name))) 
		{
		    slot = x;
		    break;
		}
	}

	if (slot == -1)
	{			    
	    send_to_char("Invalid slot reference.\n\r", ch);
	    return;
	}
    }
    else
    {
	slot = atoi(argument) - 1;
	if ((slot < 0) || (slot >= MAX_FOCUS))
	{
	    send_to_char("Invalid slot number.\n\r", ch);
	    return;
	}
    }

    if (!ch->pcdata->focus_on[slot])
    {
	send_to_char("Slot not currently in use.\n\r", ch);
	return;
    }

    sprintf(buf, "You stop focusing on %s.\n\r", skill_table[ch->pcdata->focus_sn[slot]].name);
    send_to_char(buf, ch);

    unfocus(ch, slot, TRUE);
}

void unfocus(CHAR_DATA *ch, int slot, bool out_mess)
{
    int numslots, x, i = 0, fnum = -1;
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA *paf, *paf_next, *faf, af;
     af.valid = TRUE;
     af.point = NULL;
    CHAR_DATA *victim = ch->pcdata->focus_ch[slot];

    numslots = 1;
    if (ch->pcdata->focus_sn[slot] == gsn_cloak)
    {
        for (paf=ch->affected;paf;paf=paf_next)
        {
            paf_next=paf->next;
            if (paf->type == gsn_cloak)
            {
                if (ch->pcdata->focus_ch[slot] && paf->modifier == ch->pcdata->focus_ch[slot]->id)
                    affect_remove(ch,paf);
            }
        }
    }
    //else
    //    affect_strip(ch, ch->pcdata->focus_sn[slot]);

    if (victim)
    {
        for (x = 0; focus_table[x].focus_fun; x++)
        {
            if (*focus_table[x].sn == ch->pcdata->focus_sn[slot])
            {
                fnum = x;
                break;
            }
        }

        if (fnum > -1)   /* sanity check */ 
        {
            for (paf = victim->affected; paf && (i < focus_table[fnum].afslot); paf = paf_next)
            { 
                paf_next = paf->next;
                if (paf->type != ch->pcdata->focus_sn[slot])
                    continue;
                
                if (paf->type == gsn_borrow_knowledge)
                {
                    if (skill_table[paf->modifier].spell_fun == spell_form)
                    {
                        for (faf = victim->affected; faf; faf = faf->next)
                        {
                            if (faf->type == paf->modifier)
                            {
                                unassume_form(victim);
                                break;
                            }
                        }
                    }

                    af.where     = TO_AFFECTS;
                    af.type	     = gsn_borrow_knowledge;
                    af.level     = victim->level;
                    af.duration  = 6;
                    af.location  = 0;
                    af.modifier  = -1;
                    af.bitvector = 0;
                    af.point     = NULL;
                    affect_to_char(victim, &af); 

                    send_to_char("You feel your stolen knowledge slip away.\n\r", ch);
                }

                affect_remove(victim, paf);
                i++;
            }
        }
		
    	if (out_mess && skill_table[ch->pcdata->focus_sn[slot]].msg_off[0] != '!')
	    {
	        sprintf(buf, "%s\n\r", skill_table[ch->pcdata->focus_sn[slot]].msg_off);
    	    send_to_char(buf, victim);
	    }
    }

    if (ch->pcdata->focus_sn[slot] == gsn_symbiont)
        REMOVE_BIT(ch->oaffected_by, AFF_SYMBIONT);

    if ((ch->pcdata->focus_sn[slot] == gsn_overwhelm)
     && IS_NPC(ch->pcdata->focus_ch[slot]))
	do_wake(ch->pcdata->focus_ch[slot], "");

    if (ch->pcdata->focus_sn[slot] == gsn_dominance)
    {
        ch->pcdata->focus_ch[slot]->master = NULL;
        ch->pcdata->focus_ch[slot]->leader = NULL;
    }

    ch->pcdata->focus_sn[slot] = 0;
    ch->pcdata->focus_ch[slot] = NULL;
    ch->pcdata->focus_on[slot] = FALSE;

    for (x = (slot + 1); ((x < MAX_FOCUS) && (ch->pcdata->focus_sn[x] > 0) && !ch->pcdata->focus_on[x]); x++)
    {
	numslots++;
	ch->pcdata->focus_sn[x] = 0;
	ch->pcdata->focus_ch[x] = NULL;
    }
 	
    if ((slot + numslots) < MAX_FOCUS)
    {
	for (x = (slot + numslots); x < MAX_FOCUS; x++)
	{
	    ch->pcdata->focus_sn[x - numslots] = ch->pcdata->focus_sn[x];
	    ch->pcdata->focus_ch[x - numslots] = ch->pcdata->focus_ch[x];
	    ch->pcdata->focus_on[x - numslots] = ch->pcdata->focus_on[x];
	}

	for (x = (MAX_FOCUS - numslots); x < MAX_FOCUS; x++)
	{
	    ch->pcdata->focus_sn[x] = 0;
	    ch->pcdata->focus_ch[x] = NULL;
	    ch->pcdata->focus_on[x] = FALSE;
	}
    }
    return;
}

bool saves_focus(int level, CHAR_DATA *ch, CHAR_DATA *victim, bool newfocus)
{
    int save = 45;

    save -= get_curr_stat(ch, STAT_INT);
    save -= get_curr_stat(ch, STAT_WIS);
    save += get_curr_stat(victim, STAT_INT);
    save += get_curr_stat(victim, STAT_WIS);

    save += 2 * (victim->level - level);
 
    if (IS_AFFECTED(victim,AFF_BERSERK))
        save += victim->level/2;

    save += get_resist(victim, RESIST_MENTAL);

    if (!newfocus)
	save /= 2;
        
    save = URANGE(5, save, 95);
    return number_percent() < save;
}

void do_abilities(CHAR_DATA *ch, char *argument)
{
    BUFFER *buffer;
    char skill_list[LEVEL_HERO + 1][MAX_STRING_LENGTH];
    int sn, level;
    bool found = FALSE;
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
      return;

    /* initialize data */
    for (level = 0; level < LEVEL_HERO + 1; level++)
        skill_list[level][0] = '\0';

    for (sn = 0; sn < MAX_SKILL; sn++)
    {
        if (skill_table[sn].name == NULL )
            break;

        if ((level = skill_table[sn].skill_level[ch->class_num]) < LEVEL_HERO + 1
        &&  skill_table[sn].spell_fun == spell_focus
        &&  ch->pcdata->learned[sn] > 0)
        {
            found = TRUE;
            level = skill_table[sn].skill_level[ch->class_num];
            if (ch->level < level)
                sprintf(buf,"n/a    %-40s", skill_table[sn].name);
	    else
        	sprintf(buf,"%3d%%    %3d mana    %-40s",get_skill(ch,sn),skill_table[sn].min_mana,skill_table[sn].name);
	    if (skill_list[level][0] == '\0')
                sprintf(skill_list[level],"\n\rLevel %2d: %s",level,buf);
            else 
            {
                strcat(skill_list[level],"\n\r          ");
                strcat(skill_list[level],buf);
            }
        }
    }

    if (!found)
    {
        send_to_char("No psionic abilities found.\n\r",ch);
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

bool abil_accelerated_healing(int sn, int level, CHAR_DATA *ch, void *vo)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (victim != ch && IS_OAFFECTED(ch,AFF_GHOST))
    {
        send_to_char("You can't accelerate another's healing as a ghost.\n\r",ch);
	return FALSE;
    }

    if (is_an_avatar(victim))
    {
        send_to_char("You cannot focus an avatar's healing.\n\r",ch);
	    return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = -1;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    if (victim != ch)
        act("You focus an increase in $N's metabolism, accelerating $S natural healing.", ch, NULL, victim, TO_CHAR);
    send_to_char("Your metabolism increases, accelerating your natural healing.\n\r", victim);

    return TRUE;
}

bool abil_adrenaline_rush(int sn, int level, CHAR_DATA *ch, void *vo)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
	send_to_char("Your adrenaline is already coursing through your body.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = -1;
    af.location  = APPLY_DAMROLL;
    af.modifier  = level/2;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    af.location  = APPLY_HIT;
    af.modifier  = level;
    affect_to_char(ch, &af);

	ch->hit += af.modifier;

    send_to_char("You manipulate your mind, forcing extreme amounts of adrenaline to begin coursing through your body.\n\r", ch);
    act("$n begins to glance around anxiously, an intense look in $s face.",
	ch, NULL, NULL, TO_ROOM);
  
    return TRUE;
}
 
bool abil_block_vision(int sn, int level, CHAR_DATA *ch, void *vo)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (IS_AFFECTED(victim, AFF_BLIND))
    {
	send_to_char("They are already blinded.\n\r", ch);
	return FALSE;
    }

    if ((ch != victim) && (IS_SET(victim->imm_flags, IMM_BLIND) || saves_focus(level, ch, victim, TRUE)))
    {
	act("$E resists your attempt to block $S vision.", ch, NULL, victim, TO_CHAR);
	send_to_char("You feel a probing in your mind, but you willfully force it away.\n\r", victim);
	WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE * 2));
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = -1;
    af.location  = APPLY_HITROLL;
    af.modifier	 = -4;
    af.bitvector = AFF_BLIND;
    affect_to_char(victim, &af);

    if (victim != ch)
        act("You mentally block $N's ocular senses, blinding $M!", ch, NULL, victim, TO_CHAR);
    send_to_char("You are blinded!\n\r", victim);
    act("$n appears to be blinded.", victim, NULL, NULL, TO_ROOM);

    return TRUE;
}

bool abil_borrow_knowledge(int sn, int level, CHAR_DATA *ch, void *vo)
{
    CHAR_DATA *victim;
    char arg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA af, *paf;
af.valid = TRUE;
af.point = NULL;
    int slot;

    target_name = one_argument(target_name, arg);

    if (((victim = get_char_world(ch, arg)) == NULL)
     || ((ch->in_room != victim->in_room) && (victim != ch->mindlink)))
    {
	send_to_char("You don't see them here.\n\r", ch);
	return FALSE;
    }

    if (paf = affect_find(ch->affected, gsn_borrow_knowledge))
    {
	if (paf->modifier == -1)
	    send_to_char("You are not recovered enough to mindtap again so soon.\n\r", ch);
	else
	    send_to_char("You may only have one mindtap active at a time.\n\r", ch);

	return FALSE;
    }

    if (ch->in_room != victim->in_room)
	level = round(level * 0.8);
 
    if (is_safe_spell(ch, victim, FALSE))
	return FALSE;

    if (victim == ch)
    {
	send_to_char("You have no need to tap your own knowledge.\n\r", ch);
	return FALSE;
    }

    if (IS_NPC(victim))
    {
	send_to_char("You do not understand their mind well enough to tap.\n\r", ch);
	return FALSE;
    }

    if (target_name[0] == '\0')
    {
	send_to_char("Which skill or spell do you wish to borrow?\n\r", ch);
	return FALSE;
    }

    if (target_name[0] == '\'' || target_name[0] == '"')
	target_name = one_argument(target_name, arg);
    else
	strcpy(arg, target_name);

    if ((slot = skill_lookup_full(arg)) == -1)
    {
	send_to_char("Invalid spell or skill.\n\r", ch);
	return FALSE;
    }

    if (!is_same_group(ch, victim) && saves_focus(level, ch, victim, TRUE))
    {
	act("$N resists your attempt to invade $S mind.", ch, NULL, victim, TO_CHAR);
	send_to_char("You feel a probing in your mind, but you willfully force it away.\n\r", victim);
        if (ch->in_room && victim->in_room && (ch->in_room == victim->in_room)
         && !victim->fighting && can_see(victim, ch))
        {
	    sprintf(buf, "Help!  %s is invading my mind!", PERS(ch, victim));
	    do_autoyell(victim, buf);
	    check_killer(ch, victim);
	    multi_hit(victim, ch, TYPE_UNDEFINED);
	}
	return FALSE;
    }

    sprintf(buf, "You delve into $N's mind, gleaning $S knowledge of %s!", skill_table[slot].name);
    act(buf, ch, NULL, victim, TO_CHAR);

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = -1;
    af.location  = APPLY_HIDE;
    af.modifier  = slot;
    af.bitvector = 0;
    af.point	 = malloc(sizeof(int));
    *((int *) af.point)  = (int) get_skill(victim, slot);
    affect_to_char(ch, &af);

    if (!is_same_group(ch, victim) && ch->in_room && victim->in_room
     && (ch->in_room == victim->in_room) && !victim->fighting
     && can_see(victim, ch))
    {
	sprintf(buf, "Help!  %s is invading my mind!", PERS(ch, victim));
	do_autoyell(victim, buf);
	check_killer(ch, victim);
	multi_hit(victim, ch, TYPE_UNDEFINED);
    }

    psi_ignore_target = ch;
    return TRUE;
}

bool abil_conceal_thoughts(int sn, int level, CHAR_DATA *ch, void *vo)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(victim, sn))
    {
	if (victim == ch)
	    send_to_char("Your thoughts are already concealed.\n\r", ch);
	else
	    send_to_char("Their thoughts are already concealed.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = -1;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    if (victim != ch)
	act("You conceal $N's thoughts from external detection.",
	    ch, NULL, victim, TO_CHAR);
    send_to_char("Your thoughts are concealed, and obfuscated from external detection.\n\r", victim);

    return TRUE;
}

bool abil_confusion(int sn, int level, CHAR_DATA *ch, void *vo)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(victim, sn))
    {
	send_to_char("Their mind is already in a confused state.\n\r", ch);
	return FALSE;
    }

    if (saves_focus(level, ch, victim, TRUE))
    {
	act("$N resists your attempts to confuse $M.",
	    ch, NULL, victim, TO_CHAR);
	send_to_char("You feel somewhat confused for a moment, but you focus it away.\n\r", victim);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = -1;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    act("You twist $N's perceptions, confusing $M.",
	ch, NULL, victim, TO_CHAR);
    send_to_char("Your perceptions twist, and confusion overtakes you.\n\r", victim);
    act("$N gets a confused look in $S eyes.", ch, NULL, victim, TO_NOTVICT);

    return TRUE;
}

bool abil_deflection(int sn, int level, CHAR_DATA *ch, void *vo)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
	send_to_char("You are already prepared to deflect incoming blows.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = -1;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    send_to_char("You prepare yourself to deflect incoming blows with your mind.\n\r", ch);

    return TRUE;
}

bool abil_detect_life(int sn, int level, CHAR_DATA *ch, void *vo)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
	send_to_char("You are already reaching out with your psychic senses.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.location  = APPLY_NONE;
    af.duration  = -1;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    send_to_char("You reach out with your mind, feeling for life beyond your physical senses.\n\r", ch);

    return TRUE;
}

bool abil_dominance(int sn, int level, CHAR_DATA *ch, void *vo)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
 
    if (victim == ch)
    {
	send_to_char("You cannot dominate yourself.\n\r", ch);
	return FALSE;
    }

    if (IS_AFFECTED(victim, AFF_CHARM) || is_affected(victim, gsn_dominance))
    {
	send_to_char("Their mind is already being dominated.\n\r", ch);
	return FALSE;
    }

    if (IS_NPC(victim) && (victim->level > ch->level))
    {
	send_to_char("They are too powerful to dominate.\n\r", ch);
	return FALSE;
    }

    if (saves_focus(level, ch, victim, TRUE) || (IS_SET(victim->imm_flags,IMM_CHARM)))
    {
	act("$N resists your attempt to dominate $S mind.", ch, NULL, victim, TO_CHAR);
	send_to_char("You feel a probing against you mind, but you willfully force it away.\n\r", victim);
	WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE * 2));
	return FALSE;
    }

    if (check_spirit_of_freedom(victim))
    {
	send_to_char("You feel a probing against your mind, but the spirit of freedom within you forces it away.\n\r", victim);
	act("You feel an unnatural power within $N force your psionic attack away.", ch, NULL, victim, TO_CHAR);
	WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE * 2));
	return FALSE;
    }

    AFFECT_DATA af = {0};
    af.where	= TO_AFFECTS;
    af.type	    = sn;
    af.level	= level;
    af.duration	= 2;
    af.modifier	= ch->id;
    affect_to_char(victim, &af);

    send_to_char("You feel a presence in your mind, attempting to dominate your spirit.\n\r", victim);
    act("You delve into $N's mind, attempting to dominate $S spirit.", ch, NULL, victim, TO_CHAR);

    return TRUE;
}

bool abil_enhance_pain( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (victim == ch)
    {
	send_to_char("You cannot enhance your own feelings of pain.\n\r", ch);
	return FALSE;
    }

    if (is_affected(victim, sn))
    {
	send_to_char("They are already feeling excessive pain.\n\r", ch);
	return FALSE;
    }

    if (!IS_NPC(victim) && BIT_GET(victim->pcdata->traits, TRAIT_CYNIC)
     && saves_focus(level, ch, victim, TRUE))
    {
	act("$N's mind refuses to accept your attempts to make $M feel more pain.", ch, NULL, victim, TO_CHAR);
	act("$n attempts to make you feel more pain, but you fail to accept it.", ch, NULL, victim, TO_VICT);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = -1;
    af.modifier  = 0;
    af.location  = APPLY_HIDE;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    act("You delve into $N's mind, forcing $M to feel excessive amounts of pain.", ch, NULL, victim, TO_CHAR);

    return TRUE;
}

bool abil_enhance_reflexes( int sn, int level, CHAR_DATA *ch, void *vo )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (IS_AFFECTED(ch, AFF_HASTE))
    {
	send_to_char("Your reflexes are already enhanced as much as possible.\n\r", ch);
	return FALSE;
    }

    if (IS_AFFECTED(ch, AFF_SLOW))
    {
	send_to_char("A magical force prevents you from enhancing your reflexes.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = -1;
    af.location  = APPLY_DEX;
    af.modifier  = 1 + (level >= 18) + (level >= 25) + (level >= 32);
    af.bitvector = AFF_HASTE;
    affect_to_char(ch, &af);

    send_to_char("You enhance your mental reflexes, allowing you to act more swiftly.\n\r", ch);
    act("$n appears to begin to move more quickly.", ch, NULL, NULL, TO_ROOM);

    return TRUE;
}

bool abil_esp(int sn, int level, CHAR_DATA *ch, void *vo)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
	send_to_char("You are already projecting your thoughts.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = -1;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    send_to_char("You prepare to project your thoughts when needed.\n\r", ch);

    return TRUE;
}

bool abil_foresight(int sn, int level, CHAR_DATA *ch, void *vo)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
	send_to_char("You are already able to foresee incoming attacks.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = -1;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    send_to_char("You raise your combat alertness, preparing to dodge incoming attacks.\n\r", ch);

    return TRUE;
}

bool abil_forget(int sn, int level, CHAR_DATA *ch, void *vo)
{
    CHAR_DATA *victim;
    char arg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int slot;

    target_name = one_argument(target_name, arg);

    if (((victim = get_char_world(ch, arg)) == NULL)
     || ((ch->in_room != victim->in_room) && (victim != ch->mindlink)))
    {
	send_to_char("You don't see them here.\n\r", ch);
	return FALSE;
    }

    if (ch->in_room != victim->in_room)
	level = round(level * 0.8);
 
    if (is_safe_spell(ch, victim, FALSE))
	return FALSE;

    if (victim == ch)
    {
	send_to_char("You cannot cause yourself to forget.\n\r", ch);
	return FALSE;
    }

    if (!can_be_affected(victim,gsn_forget))
    {
	act("$N has no memory.",ch,NULL,victim,TO_CHAR);
	return FALSE;
    }

    if (target_name[0] == '\0')
    {
	send_to_char("Which skill or spell do you wish to make them forget?\n\r", ch);
	return FALSE;
    }

    if (target_name[0] == '\'' || target_name[0] == '"')
	target_name = one_argument(target_name, arg);
    else
	strcpy(arg, target_name);

    if ((slot = skill_lookup_full(arg)) == -1)
    {
	send_to_char("Invalid spell or skill.\n\r", ch);
	return FALSE;
    }

    if (saves_focus(level, ch, victim, TRUE))
    {
	act("$N resists your attempt to invade $S mind.", ch, NULL, victim, TO_CHAR);
	send_to_char("You feel a probing in your mind, but you willfully force it away.\n\r", victim);
        if (ch->in_room && victim->in_room && (ch->in_room == victim->in_room)
         && !victim->fighting && can_see(victim, ch))
        {
	    sprintf(buf, "Help!  %s is invading my mind!", PERS(ch, victim));
	    do_autoyell(victim, buf);
	    check_killer(ch, victim);
	    multi_hit(victim, ch, TYPE_UNDEFINED);
	}
	WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE * 2));
	return FALSE;
    }

    sprintf(buf, "You delve into $N's mind, causing $M to lose knowledge of %s!", skill_table[slot].name);
    act(buf, ch, NULL, victim, TO_CHAR);
    sprintf(buf, "You feel your knowledge of %s slip away.\n\r", skill_table[slot].name);
    send_to_char(buf, victim);

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = -1;
    af.location  = APPLY_HIDE;
    af.modifier  = slot;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    if (ch->in_room && victim->in_room && (ch->in_room == victim->in_room)
     && !victim->fighting && can_see(victim, ch))
    {
	sprintf(buf, "Help!  %s is invading my mind!", PERS(ch, victim));
	do_autoyell(victim, buf);
	check_killer(ch, victim);
	multi_hit(victim, ch, TYPE_UNDEFINED);
    }

    psi_ignore_target = victim;
    return TRUE;
}

bool abil_ignore_pain(int sn, int level, CHAR_DATA *ch, void *vo)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
	send_to_char("You are already prepared to ignore the pain of your physical self.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = -1;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    send_to_char("You prepare yourself to ignore the pain of your physical self.\n\r", ch);

    return TRUE;
}

bool abil_leech(int sn, int level, CHAR_DATA *ch, void *vo)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int x;

    if (victim == ch)
    {
	send_to_char("You cannot leech energy from yourself.\n\r", ch);
	return FALSE;
    }

    for (x = 0; x < MAX_FOCUS; x++)
	if ((ch->pcdata->focus_sn[x] == sn) && (ch->pcdata->focus_ch[x] == victim))
	{
	    send_to_char("You are alreay leeching energy from that person.\n\r", ch);
	    return FALSE;
	}

    if (saves_focus(level, ch, victim, TRUE))
    {
	act("$N resists your attempt to leech $S mental energies.",
	    ch, NULL, victim, TO_CHAR);
	send_to_char("You feel a probing in your mind, but you willfully force it away.\n\r", victim);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.point	 = (void *) ch;
    af.location  = APPLY_NONE;
    af.duration  = -1;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    act("You latch into $N's mind, and begin draining away mental energy.",
	ch, NULL, victim, TO_CHAR);
    send_to_char("You feel your mental energy begin to drain away.\n\r", victim);

    return TRUE;
}

bool abil_levitation(int sn, int level, CHAR_DATA *ch, void *vo)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (IS_AFFECTED(ch, AFF_FLYING))
    {
	send_to_char("You are already magically imbued with flight.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration	 = -1;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = AFF_FLYING;
    affect_to_char(ch, &af);

    send_to_char("You levitate yourself into the air with telekinetic energy.\n\r", ch);
    act("$n's feet rise off the ground.", ch, NULL, NULL, TO_ROOM);

    return TRUE;
}

bool abil_mind_thrust(int sn, int level, CHAR_DATA *ch, void *vo)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    char buf[MAX_STRING_LENGTH];
    int x;

    if (victim == ch)
    {
	send_to_char("You cannot focus a mind thrust within yourself.\n\r", ch);
	return FALSE;
    }

    act("You focus a mass of disruptive psychic energy into $N's mind!",
	ch, NULL, victim, TO_CHAR);
    act("$N focuses a blast of disruptive energy into your mind!",
	ch, NULL, victim, TO_VICT);

    if (saves_focus(level, ch, victim, FALSE))
    {
	act("$E resists your mind thrust.", ch, NULL, victim, TO_CHAR);
	act("You resist the mind thrust.", ch, NULL, victim, TO_VICT);
	return FALSE;
    }

    for (x = MAX_FOCUS - 1; x >= 0; x--)
	if (victim->pcdata->focus_on[x]
	 && !saves_focus(level, ch, victim, TRUE))
	{
	    sprintf(buf, "Your mind thrust disrupts $N's %s focus.",
		skill_table[victim->pcdata->focus_sn[x]].name);
	    act(buf, ch, NULL, victim, TO_CHAR);
	    sprintf(buf, "$n's mind thrust disrupts your %s focus.",
		skill_table[victim->pcdata->focus_sn[x]].name);
	    act(buf, ch, NULL, victim, TO_VICT);
	    unfocus(victim, x, TRUE);
	}

    return TRUE;
}

bool abil_mindshield(int sn, int level, CHAR_DATA *ch, void *vo)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo, *cch;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (ch->class_num == global_int_class_assassin
      && victim != ch)
    {
	send_to_char("You cannot focus that ability upon another.\n\r",ch);
	return FALSE;
    }
    
    if (is_affected(victim, gsn_mindshield))
    {
	if (victim == ch)
	    send_to_char("Your mind is already shielded from intrusion.\n\r", ch);
	else
	    send_to_char("Their mind is already shielded from intrusion.\n\r", ch);
	return FALSE;
    }

    if (cch=cloak_remove(victim))
    {
	send_to_char("You feel your cloak forced away.\n\r",cch);
	send_to_char("You realize that someone was cloaked from your vision.\n\r",victim);
    }
    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = -1;
    af.location  = APPLY_RESIST_MENTAL;
    af.modifier  = level / 2;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    if (victim != ch)
	act("You shield $N's mind from intrusion.", ch, NULL, victim, TO_CHAR);
    send_to_char("Your mind is shielded from intrusion.\n\r", victim);

    return TRUE;
}

bool abil_ordered_mind(int sn, int level, CHAR_DATA *ch, void *vo)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(victim, sn))
    {
	if (victim == ch)
	    send_to_char("Your mind is already in a state of order.\n\r", ch);
	else
	    send_to_char("Their mind is already in a state of order.\n\r", ch);
	return FALSE;
    }

    if (IS_NPC(victim))
    {
	send_to_char("Their thoughts cannot be ordered.\n\r", ch);
	return FALSE;
    }

    if (IS_CHAOTIC(victim))
    {
	act("$N's thoughts are too chaotic to enforce mental order upon.",
	    ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = -1;
    af.modifier	 = 0;
    af.location  = 0;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    if (victim != ch)
	send_to_char("You enforce order upon their mind, allowing them to think more clearly.\n\r", ch);
    send_to_char("A sense of order fills your mind, and you begin to think more clearly.\n\r", victim);

    return TRUE;
}


bool abil_overwhelm(int sn, int level, CHAR_DATA *ch, void *vo)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (IS_NPC(victim) && IS_SET(victim->act,ACT_UNDEAD))
    {
	send_to_char("Its undead mind cannot be overwhelmed.\n\r", ch);
        return FALSE;
    }

    if (victim->fighting)
    {
	send_to_char("Their mind is too alert to overwhelm at the moment.\n\r", ch);
	return FALSE;
    }
    
    if ((IS_NPC(victim) && IS_SET(victim->act, ACT_NOSUBDUE)) || saves_focus(level, ch, victim, TRUE))
    {
	act("$N resists your attempts to overwhelm $S mind.",
	    ch, NULL, victim, TO_CHAR);
	send_to_char("A barrage of psychic energy assails your mind, but you resist the overwhelming force.\n\r", victim);
	WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE * 2));
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.point	 = (void *) ch;
    af.duration  = -1;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = AFF_SLEEP;
    affect_to_char(victim, &af);

    act("You assail $N with psychic energy, overwhelming $S consciousness.",
	ch, NULL, victim, TO_CHAR);

    if (IS_AWAKE(victim))
    {
        send_to_char("Psychic energy assails you, and you collapse to the ground as your consciousness fades.\n\r", victim);
	act("$n collapses to the ground.", victim, NULL, NULL, TO_ROOM);
	switch_position(victim, POS_SLEEPING);
    }
    
    return TRUE;
}
  

bool abil_paranoia(int sn, int level, CHAR_DATA *ch, void *vo)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (victim == ch)
    {
	send_to_char("You cannot fill yourself with paranoia.\n\r", ch);
	return FALSE;
    }

    if (is_affected(victim, sn))
    {
	act("$N's thoughts are already plagued with paranoia.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }

    if (saves_focus(level, ch, victim, TRUE))
    {
	act("$N resists your attempt to inflict paranoia upon $M.", ch, NULL, victim, TO_CHAR);
	send_to_char("You feel a probing in your mind, but you willfully force it away.\n\r", victim);
	return true;
    }

    af.where	 = TO_OAFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = -1;
    af.modifier  = 0;
    af.location  = APPLY_HIDE;
    af.bitvector = AFF_PARANOIA;
    affect_to_char(victim, &af);

    send_to_char("You fill their mind with delusions of paranoia.\n\r", ch);
    return TRUE;
}

bool abil_psionic_blast(int sn, int level, CHAR_DATA *ch, void *vo)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int dam;

    dam = (3 * level) / 2 + dice(level, 5);

    act("You project a blast of psionic energy at $N!",
	ch, NULL, victim, TO_CHAR);

    if (saves_spell(level, ch, victim, DAM_MENTAL))
        dam /= 2;
    else
    {
        af.where     = TO_AFFECTS;
        af.type	     = sn;
        af.level     = level;
        af.duration  = level/10;
        af.location  = APPLY_INT;
        af.modifier  = -1;
        af.bitvector = 0;
        affect_to_char(victim, &af);

        af.location  = APPLY_WIS;
        affect_to_char(victim, &af);
    }

    damage_old(ch, victim, dam, gsn_psionic_blast, DAM_MENTAL, TRUE);

    return TRUE;
}

bool abil_psychic_block(int sn, int level, CHAR_DATA *ch, void *vo)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int slots, x;

    if (victim == ch)
    {
	send_to_char("You cannot block your own psionic abilities.\n\r", ch);
	return FALSE;
    }

    if (is_affected(victim, gsn_psychic_block))
    {
	act("$N is already hindered with a psychic block.", ch, NULL, victim, TO_CHAR);
	return FALSE;
    }
   
    if (saves_focus(level, ch, victim, TRUE))
    {
	act("$N resists your attempt to block $S psionic abilities.",
	    ch, NULL, victim, TO_CHAR);
	act("You resist $n's attempt to block your psionic abilities.",
	    ch, NULL, victim, TO_VICT);
	return FALSE;
    }

    slots = focus_slots(ch);
    
    for (x = 0; x < slots; x++)
	if (victim->pcdata->focus_on[slots-1-x])
	    unfocus(victim, slots-1-x, TRUE);

    af.where	 = TO_AFFECTS;
    af.type	 = sn; 
    af.level	 = level;
    af.duration  = -1;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    act("You erect a psychic barrier within $N's mind, partially blocking $S psionic abilities.", ch, NULL, victim, TO_CHAR);
    send_to_char("You feel a psychic barrier in your mind.\n\r", victim);

    return TRUE;
}

bool abil_read_thoughts(int sn, int level, CHAR_DATA *ch, void *vo)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (target_name[0] == '\0')
    {
	send_to_char("Who do you wish to read the thoughts of?\n\r", ch);
	return FALSE;
    }

    if (!(victim = get_char_world(ch, target_name)) || ((victim->in_room != ch->in_room) && (ch->mindlink != victim)))
    {
	send_to_char("You don't seem them here.\n\r", ch);
	return FALSE;
    }

    if (ch->in_room != victim->in_room)
	level = round(level * 0.8);

    if (victim == ch)
    {
	send_to_char("You do not require psionics to read your own thoughts.\n\r", ch);
	return FALSE;
    }

    if (saves_focus(level, ch, victim, TRUE))
    {
	act("$N resists your attempts to read $S thoughts.",
	    ch, NULL, victim, TO_CHAR);
	send_to_char("You feel a probing in your mind, but you willfully force it away.\n\r", victim);
        if (ch->in_room && victim->in_room && (ch->in_room == victim->in_room)
         && !victim->fighting && can_see(victim, ch))
        {
	    sprintf(buf, "Help!  %s is invading my mind!", PERS(ch, victim));
	    do_autoyell(victim, buf);
	    check_killer(ch, victim);
	    multi_hit(victim, ch, TYPE_UNDEFINED);
	}
	return FALSE;
    }

    af.where	 = TO_OAFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.point	 = ch;
    af.duration  = -1;
    af.location  = APPLY_HIDE;
    af.modifier  = 0;
    af.bitvector = AFF_READTHOUGHTS;
    affect_to_char(victim, &af);

    act("You prepare yourself to read $N's thoughts.",
	ch, NULL, victim, TO_CHAR);

    psi_ignore_target = victim;
    return TRUE;
}


bool abil_reduce_pain( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (victim == ch)
    {
	send_to_char("You cannot reduce your own feelings of pain.\n\r", ch);
	return FALSE;
    }

    if (is_affected(victim, sn))
    {
	send_to_char("They are already feeling reduced pain.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = -1;
    af.modifier  = 0;
    af.location  = APPLY_HIDE;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    act("You delve into $N's mind, reducing $S feelings of pain.", ch, NULL, victim, TO_CHAR);

    return TRUE;
}

bool abil_sense_danger(int sn, int level, CHAR_DATA *ch, void *vo)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
	send_to_char("You are already alert for danger.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = -1;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    send_to_char("You attune yourself to the surface thoughts of those around you, scanning for indications of danger.\n\r", ch);

    return TRUE;
}

bool abil_sensory_vision(int sn, int level, CHAR_DATA *ch, void *vo)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, gsn_sensory_vision)
     || (ch->race == global_int_race_shuddeni))
    {
	send_to_char("You can already view your surroundings with your senses.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_PAFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = -1;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = AFF_SENSORY_VISION;
    affect_to_char(ch, &af);

    send_to_char("You begin to view your surroundings using your mental senses.\n\r", ch);

    return TRUE;
}

bool abil_slow_reflexes( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (IS_AFFECTED(victim, AFF_SLOW))
    {
	if (victim == ch)
	    send_to_char("Your reflexes are already considerably slowed.\n\r", ch);
	else
	    send_to_char("Their reflexes are already slowed as much as possible.\n\r", ch);
	return FALSE;
    }

    if (IS_AFFECTED(victim, AFF_HASTE))
    {
	if (victim == ch)
	    send_to_char("A magical force prevents you from slowing your reflexes.\n\r", ch);
	else
	    send_to_char("A magical force prevents you from slowing their reflexes.\n\r", ch);
	return FALSE;
    }

    if ((victim != ch) && saves_focus(level, ch, victim, TRUE))
    {
	act("$N resists your attempts to slow $S reflexes.", ch, NULL, victim, TO_CHAR);
	send_to_char("You feel yourself slow down momentarily, but it passes.\n\r", victim);
	WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE * 2));
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = -1;
    af.location  = APPLY_DEX;
    af.modifier  = - 1 - (level >= 18) - (level >= 25) - (level >= 32);
    af.bitvector = AFF_SLOW;
    affect_to_char(victim, &af);

    send_to_char("You begin to move more slowly.\n\r", victim);
    act("$n appears to be moving slower.", victim, NULL, NULL, TO_ROOM);
    if (ch->in_room != victim->in_room)
    act("You feel $N moving slower.",ch, NULL, victim, TO_CHAR);
    return TRUE;
}

bool abil_suggestion(int sn, int level, CHAR_DATA *ch, void *vo)
{
    CHAR_DATA *victim;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    char arg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char *suggest;

    target_name = one_argument(target_name, arg);

    if (arg[0] == '\0')
    {
	send_to_char("Whom do you wish to place a suggestion upon?\n\r", ch);
	return FALSE;
    }

    if (!(victim = get_char_world(ch, arg)) || ((ch->in_room != victim->in_room) && (ch->mindlink != victim)))
    {
	send_to_char("You don't see them here.\n\r", ch);
	return FALSE;
    }

    if (ch->in_room != victim->in_room)
	level = round(level * 0.8);

    if (victim == ch)
    {
	send_to_char("You cannot plant a suggestion in your own mind.\n\r", ch);
	return FALSE;
    }

    if (IS_NPC(victim)
     || ((victim->level >= 52) && (ch->level < victim->level)))
    {
        send_to_char("You cannot plant a suggestion in their mind.\n\r", ch);
        return FALSE;
    }

    if (is_safe_spell(ch, victim, FALSE))
	return FALSE;

    if (is_affected(victim, gsn_suggestion))
    {
	send_to_char("A suggestion has already been planted in their mind.\n\r", ch);
	return FALSE; 
    }

    if (target_name[0] == '\0')
    {
	send_to_char("What suggestion do you wish to plant?\n\r", ch);
	return FALSE;
    }

    if (saves_focus(level, ch, victim, TRUE))
    {
	act("$N resists your attempt to plant a suggestion in $S mind.",
	    ch, NULL, victim, TO_CHAR);
	send_to_char("You feel a probing in your mind, but willfully force it away.\n\r", victim); 
	WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE * 2));
        if (ch->in_room && victim->in_room && (ch->in_room == victim->in_room)
         && !victim->fighting && can_see(victim, ch))
        {
	    sprintf(buf, "Help!  %s is invading my mind!", PERS(ch, victim));
	    do_autoyell(victim, buf);
	    multi_hit(victim, ch, TYPE_UNDEFINED);
	    check_killer(ch, victim);
        }
	return FALSE;
    }

	if (IsOrderBanned(target_name))
    {
        send_to_char("The gods won't allow such an order.\n\r", ch);
        sprintf(buf, "%s tried to force someone to do a naughty with suggestion.\n\r", ch->name);
        bug(buf, 0);
        return FALSE;
    }

    suggest = str_dup(target_name);

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.point     = (void *) suggest;
    af.duration  = 2;
    af.modifier	 = 0;
    af.location  = APPLY_HIDE;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    act("You plant the suggestion deep within $N's mind.",
	ch, NULL, victim, TO_CHAR);

    psi_ignore_target = victim;
    return TRUE;
}

bool abil_symbiont(int sn, int level, CHAR_DATA *ch, void *vo)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];

    if (IS_OAFFECTED(ch, AFF_SYMBIONT))
    {
	send_to_char("You have already formed a symbiotic link.\n\r", ch);
	return FALSE;
    }

    if (target_name[0] == '\0')
    {
	send_to_char("Who do you wish to form a symbiont link with?\n\r", ch);
	return FALSE;
    }

    if (!(victim = get_char_world(ch, target_name)) || ((victim->in_room != ch->in_room) && (ch->mindlink != victim)))
    {
	send_to_char("You don't seem them here.\n\r", ch);
	return FALSE;
    }

    if (victim == ch)
    {
	send_to_char("You cannot form a symbiotic link with yourself.\n\r", ch);
	return FALSE;
    }

    if (!can_be_affected(victim,gsn_symbiont))
    {
	act("You can't form a symbiotic link with $N.",ch,NULL,victim,TO_CHAR);
	return FALSE;
    }

    if (ch->in_room != victim->in_room)
	level = round(level * 0.8);

    if (is_safe_spell(ch, victim, FALSE))
	return FALSE;

    if (is_affected(ch, gsn_symbiont)
     || (!IS_NPC(victim) && IS_SET(victim->comm, COMM_SNOOP_PROOF)))
    {
	send_to_char("Something prevents you from forming a symbiotic relationship with them.\n\r", ch);
	return FALSE;
    }

    if (IS_NPC(victim))
    {
	send_to_char("You cannot form a symbiotic link with them.\n\r", ch);
	return FALSE;
    }

    if (saves_focus(level, ch, victim, TRUE))
    {
	act("$N resists your attempt to invade $S mind.", ch, NULL, victim, TO_CHAR);
	send_to_char("You feel a probing in your mind, but you willfully force it away.\n\r", victim);
        if (ch->in_room && victim->in_room && (ch->in_room == victim->in_room)
         && !victim->fighting && can_see(victim, ch))
        {
	    sprintf(buf, "Help!  %s is invading my mind!", PERS(ch, victim));
	    do_autoyell(victim, buf);
	    multi_hit(victim, ch, TYPE_UNDEFINED);
	    check_killer(ch, victim);
	    WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE * 2));
	}
	return FALSE;
    }

    af.where	 = TO_OAFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.point	 = (void *) ch->desc;
    af.duration  = -1;
    af.modifier  = 0;
    af.location  = APPLY_HIDE;
    af.bitvector = AFF_SYM_TARGET;
    affect_to_char(victim, &af);

    SET_BIT(ch->oaffected_by, AFF_SYMBIONT);

    act("You attach yourself to $N's mind, forming a symbiotic relationship.", ch, NULL, victim, TO_CHAR);

    psi_ignore_target = victim;
    return TRUE;
}

bool abil_vertigo(int sn, int level, CHAR_DATA *ch, void *vo)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(victim, sn))
    {
	send_to_char("They are already experiencing vertigo.\n\r", ch);
	return FALSE;
    }

    if (saves_focus(level, ch, victim, TRUE))
    {
	act("$N resists your attempt to instill $M with vertigo.",
	    ch, NULL, victim, TO_CHAR);
	send_to_char("You feel a probing against your mind, but you willfully force it away.\n\r", victim);
	WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE * 2));
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = -1;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    act("You disorient $N's senses, and $E begins to reel.",
	ch, NULL, victim, TO_CHAR);
    send_to_char("You reel as a wave of dizziness overtakes you.\n\r", victim);
    act("$N appears to be very disoriented.", ch, NULL, victim, TO_NOTVICT);

    return TRUE;
}

bool abil_prowess(int sn, int level, CHAR_DATA *ch, void *vo)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (ch->class_num == global_int_class_assassin
      && victim != ch)
    {
	send_to_char("You cannot focus that ability upon another.\n\r",ch);
	return FALSE;
    }
    
    if (is_affected(victim, sn))
    {
	if (victim == ch)
	    send_to_char("You already possess acute combat prowess.\n\r", ch);
	else
	    send_to_char("They already possess acute combat prowess.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = -1;
    af.modifier  = level/10;
    af.location  = APPLY_HITROLL;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    if (victim != ch)
	act("You enhance $N's mind with acute combat prowess.",
	    ch, NULL, victim, TO_CHAR);
    send_to_char("Your feel your combat prowess become more acute.\n\r", victim);

    return TRUE;
}

void do_mindlink(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    int chance, ytype;
    bool failed = FALSE;

    if ((chance = get_skill(ch,gsn_mindlink)) == 0)
    {
        send_to_char("Huh?\n\r",ch);
        return;
    }

    if (argument[0] == '\0')
    {
	if (!ch->mindlink)
	    send_to_char("You do not have an active mindlink.\n\r", ch);
	else
	    act("You currently have a mindlink to $N.", ch, NULL, ch->mindlink, TO_CHAR);
	return;
    }

    if (!str_cmp(argument, "none"))
    {
	if (!ch->mindlink)
	    send_to_char("But you do not have an existing mindlink!\n\r", ch);
	else
	    act("You break your mindlink to $N.", ch, NULL, ch->mindlink, TO_CHAR);
	ch->mindlink = NULL;
	return;
    }

    if ((victim = get_char_room(ch, argument)) == NULL)
    {
	send_to_char("You don't see them here.\n\r", ch);
	return;
    }

    if (IS_NPC(victim))
    {
	send_to_char("They have not the presence of mind to form a link with.\n\r", ch);
	return;
    }

    if (ch == victim)
    {
	send_to_char("You cannot mindlink yourself.\n\r", ch);
	return;
    }

    if (ch->mindlink == victim)
    {
        act("You are already linked to $N's mind.", ch, NULL, victim, TO_CHAR);
	return;
    }

    if (is_safe(ch, victim))
	return;

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_mindlink].beats));

    if (!IS_NPC(ch))
	ch->pcdata->adrenaline = IS_NPC(victim) ? UMAX(ch->pcdata->adrenaline, 2) : UMAX(ch->pcdata->adrenaline, 20);

    if (chance < number_percent())
    {
	send_to_char("You fail to properly focus the energies to mindlink.\n\r", ch);
	send_to_char("You feel a fumbling presence in your mind...\n\r", victim);
	failed = TRUE;
        check_improve(ch, victim, gsn_mindlink, FALSE, 2);
    }
    else
	check_improve(ch, victim, gsn_mindlink, TRUE, 1);

    if (saves_focus(ch->level, ch, victim, TRUE))
    {
	act("$N resists your attempt to mindlink $M.", ch, NULL, victim, TO_CHAR);
	act("You resist $n's attempt to mindlink you.", ch, NULL, victim, TO_VICT);
	failed = TRUE;
    }

    if (failed)
    {
        if (IS_AWAKE(victim) && (victim->fighting != ch))
        {
            ytype = number_range(1, 3);
            switch (ytype)
            {
                case 1:
                sprintf(buf, "Help!  %s is trying to invade my mind!", PERS(ch, victim));
                break;
                case 2:
                sprintf(buf, "Get out of my head, %s!", PERS(ch, victim));
                break;
                case 3:
                sprintf(buf, "Help!  %s is trying to mindlink me!", PERS(ch, victim));
                break;
            }

            do_autoyell(victim, buf);
        }

        if (IS_AWAKE(victim))
        {
            check_killer(ch, victim);
            one_hit(victim, ch, TYPE_UNDEFINED, HIT_PRIMARY, false);
        }
        return;
    }

    ch->mindlink = victim;
    act("You link yourself into $N's mind.", ch, NULL, victim, TO_CHAR);
}

bool abil_shove(int sn, int level, CHAR_DATA *ch, void *vo)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    int odds_success = 25;
	if (victim->fighting) odds_success += 10;
	
	if (number_percent() < odds_success)
    {
		act("You motion your hand towards $N, and $E goes flying!",	ch, NULL, victim, TO_CHAR);
		act("$n motions towards you, and you go flying!", ch, NULL, victim, TO_VICT);
		act("$N goes flying as $n motions towards $M.",	ch, NULL, victim, TO_NOTVICT);
		WAIT_STATE(victim, UMAX(victim->wait, 2 * PULSE_VIOLENCE - 1 + number_bits(2)));
	}
	else
	{
		act("You motion your hand towards $N, but fail to knock $M off balance.", ch, NULL, victim, TO_CHAR);
		act("$n mentally shoves you, but you stand firm against it!", ch, NULL, victim, TO_VICT);
		act("$n motions $s hand towards $N.", ch, NULL, victim, TO_NOTVICT);
    }

    return TRUE;
}

bool abil_acuity(int sn, int level, CHAR_DATA *ch, void *vo)
{
    if (is_affected(ch, sn))
    {
        send_to_char("Your senses are already heightened.\n\r", ch);
        return FALSE;
    }

    AFFECT_DATA af = {0};
    af.where	 = TO_AFFECTS;
    af.type	     = sn;
    af.level	 = level;
    af.location  = APPLY_NONE;
    af.duration  = -1;
    af.bitvector = AFF_DETECT_HIDDEN|AFF_DETECT_INVIS;
    affect_to_char(ch, &af);

    send_to_char("You heighten your senses, revealing the unseen.\n\r", ch);
    return TRUE;
}

bool abil_cloak( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (!IS_NPC(ch) && ch->pcdata->adrenaline > 0)
    {
	send_to_char("You need to calm down before you can focus your mind to cloak.\n\r",ch);
	return FALSE;
    }

    if (is_affected(victim, gsn_mindshell) 
      || is_affected(victim,gsn_mindshield))
    {
	send_to_char("They are shielded against your power.\n\r",ch);
	return FALSE;
    }

    act("You cloak yourself from $N's perception.",ch,NULL,victim,TO_CHAR);

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = -1;
    af.location  = APPLY_NONE;
    af.modifier  = victim->id;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    return TRUE;
}

bool abil_celerity(int sn, int level, CHAR_DATA *ch, void *vo)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
	send_to_char("You already move tirelessly.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = -1;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    send_to_char("You focus, allowing your body to move tirelessly.\n\r", victim);

    return TRUE;
}

bool abil_shadowfist(int sn, int level, CHAR_DATA *ch, void *vo)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
	send_to_char("Your will already permeates your hands and feet.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = -1;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    send_to_char("The force of your will permeates your hands and feet.\n\r", victim);

    return TRUE;
}

bool abil_concealremains(int sn, int level, CHAR_DATA *ch, void *vo)
{
    OBJ_DATA *corpse;
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;

    if (IS_OAFFECTED(ch,AFF_GHOST))
    {
	send_to_char("You can't exert your influence on remains without a body of your own.\n\r",ch);
	return FALSE;
    }

    if ((corpse = get_obj_here(ch,"corpse") ) == NULL)
    {
	send_to_char("There are no corpses here.\n\r",ch);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = -1;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = 0;
    af.point 	 = (void *) ch;

    affect_to_char(ch, &af);

    act("You begin concealing death from view.", ch, NULL, NULL, TO_CHAR);

    return TRUE;
}
