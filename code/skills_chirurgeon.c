#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sstream>
#include "merc.h"
#include "recycle.h"
#include "skills_chirurgeon.h"
#include "NameMaps.h"

void do_cauterize(CHAR_DATA *ch, char *argument)
{
	OBJ_DATA * obj = NULL;
	AFFECT_DATA *paf = NULL, *paf_next = NULL;

	int chance = 0;

	if ((chance = get_skill(ch, gsn_cauterize)) <= 0)
	{
		send_to_char("Huh?\n\r", ch);
		return;
	}

	if (!IS_OAFFECTED(ch, AFF_BLEEDING))
	{
		send_to_char("You are not bleeding from any grevious wounds.\n\r", ch);
		return;
	}

	obj = get_eq_char(ch, WEAR_WIELD);

// brazen: Fire templars can heat up their own metal weapons innately, whereas
// chirurgeons must heat them without magic.
	
	if (ch->class_num == global_int_class_firetemplar)
	{
	    if (obj == NULL || !material_table[obj->material].metal)
	    {
	        obj = get_eq_char(ch, WEAR_DUAL_WIELD);
	        if (obj == NULL || !material_table[obj->material].metal)
	        {
	            send_to_char("You must be wielding a metal weapon to cauterize with.\n\r",ch);
	            return;
	        }
	    }
	}
	else
	{
	    if (obj == NULL || obj->value[3] != DAM_FIRE)
 	    {
		obj = get_eq_char(ch, WEAR_DUAL_WIELD);
		if (obj == NULL || obj->value[3] != DAM_FIRE)
		{
	    	    send_to_char("You must wield a hot weapon to cauterize your wounds.\n\r", ch);
		    return;
		}
	    }
	}  
	
	/* Past the possibility checks */
	expend_mana(ch, skill_table[gsn_cauterize].min_mana);
	WAIT_STATE(ch, skill_table[gsn_cauterize].beats);

	damage_old(ch, ch, number_range(20, 30), gsn_cauterize, DAM_FIRE, TRUE);
	
	if (number_percent() > chance)
	{
		act("You try to cauterize your wounds with $p, but fail to seal them properly.", ch, obj, NULL, TO_CHAR);
		act("$n tries to cauterize $s wounds with $p, but fails to seal them properly.", ch, obj, NULL, TO_ROOM);
		check_improve(ch, NULL, gsn_cauterize, FALSE, 1);
		return;
	}
	
	for (paf = ch->affected; paf; paf = paf_next)
	{
		paf_next = paf->next;

		if ((paf->where == TO_OAFFECTS) && (paf->bitvector == AFF_BLEEDING))
			affect_remove(ch, paf);
	}
	
	act("Your flesh sizzles slightly as you press $p against your wounds, cauterizing them.", ch, obj, NULL, TO_CHAR);
	act("$n's flesh sizzles slightly as $e presses $p against $s wounds, cauterizing them.", ch, obj, NULL, TO_ROOM);
	check_improve(ch, NULL, gsn_cauterize, TRUE, 1);
}

void do_whipstitch(CHAR_DATA * ch, char * argument)
{
	CHAR_DATA * victim = ch;

	int chance = 0;

	if (ch->position == POS_FIGHTING)
	{
		send_to_char("You are too preoccupied to perform stitchwork right now!\n\r", ch);
		return;
	}
	
	if ((chance = get_skill(ch, gsn_whipstitch)) <= 0)
	{
		send_to_char("Huh?\n\r", ch);
		return;
	}

	if (argument[0] != '\0')
	{
		if ((victim = get_char_room(ch, argument)) == NULL)
		{
			send_to_char("You don't see them here.\n\r", ch);
			return;
		}
		if (!is_same_group(ch, victim))
		{
			send_to_char("You may only whipstitch groupmates.\n\r", ch);
			return;
		}
	}
	if (victim->max_hit <= victim->hit)
	{
		if (victim == ch)
			send_to_char("You have no wounds to stitch.\n\r", ch);
		else
			send_to_char("You doubt they would appreciate being stitched when they aren't even hurt.\n\r", ch);
		return;
	}
	
	if (is_loc_affected(ch, gsn_whipstitch, APPLY_NONE))
	{
		send_to_char("You aren't ready to whipstitch again yet.\n\r", ch);
		return;
	}
	
	/* Past the possibility checks */
	expend_mana(ch, skill_table[gsn_whipstitch].min_mana);
	WAIT_STATE(ch, skill_table[gsn_whipstitch].beats);

	int location = APPLY_CHR;
	switch (number_range(0, 4))
	{
		case 0: location = APPLY_STR; break;
		case 1: location = APPLY_DEX; break;
		case 2: location = APPLY_CON; break;
	}

	if (victim->position == POS_SLEEPING) 
		chance += 10;
	
	AFFECT_DATA af;
	af.valid = TRUE;
	af.point = NULL;
	af.location  = APPLY_NONE;
	af.where     = TO_AFFECTS;
	af.type      = gsn_whipstitch;
	af.level     = ch->level;
	af.duration  = number_fuzzy(4);
	af.modifier = 0;
	af.bitvector = 0;
	affect_to_char(ch, &af);

	af.duration = number_fuzzy(6);
	af.location  = location;
	af.modifier  = -1;
	af.bitvector = 0;

	// Chance of completely screwing up
	if (number_percent() > chance)
	{
		if (victim == ch)
		{
			act("You try to loosely stitch yourself, but only manage to tear your wounds more.", ch, NULL, NULL, TO_CHAR);
			act("$n tries to loosely stitch $mself, but only manages to tear $s wounds more.", ch, NULL, NULL, TO_ROOM);
		}
		else
		{
			act("You try to loosely stitch $N, but only manage to tear $S wounds more.", ch, NULL, victim, TO_CHAR);
			act("$n tries to loosely stitch $N, but only manages to tear $S wounds more.", ch, NULL, victim, TO_NOTVICT);
			act("$n tries to loosely stitch you, but only manages to tear your wounds more.", ch, NULL, victim, TO_VICT);
		}
		check_improve(ch, NULL, gsn_whipstitch, FALSE, 1);
		damage_old(victim, victim, number_range(20, 30), gsn_whipstitch, DAM_PIERCE, FALSE);
		affect_to_char(victim, &af);
		return;
	}

	// Half chance of partially screwing up
	chance /= 2;
	if (number_percent() > chance)
	{
		if (victim == ch)
		{
			act("You loosely stitch your wounds.", ch, NULL, NULL, TO_CHAR);
			act("$n loosely stitches $s wounds.", ch, NULL, NULL, TO_ROOM);
		}
		else
		{
			act("You loosely stitch $N's wounds.", ch, NULL, victim, TO_CHAR);
			act("$n loosely stitches $N's wounds.", ch, NULL, victim, TO_NOTVICT);
			act("$n loosely stitches your wounds.", ch, NULL, victim, TO_VICT);
		}
		affect_to_char(victim, &af);
	}
	else
	{
		if (victim == ch)
		{
			act("With a series of practiced movements, you tidily stitch yourself up!", ch, NULL, NULL, TO_CHAR);
			act("With a series of practiced movements, $n tidily stitches $mself up!", ch, NULL, NULL, TO_ROOM);
		}
		else
		{
			act("With a series of practiced movements, you tidily stitch $N up!", ch, NULL, victim, TO_CHAR);
			act("With a series of practiced movements, $n tidily stitches $N up!", ch, NULL, victim, TO_NOTVICT);
			act("With a series of practiced movements, $n tidily stitches you up!", ch, NULL, victim, TO_VICT);
		}
	}

	check_improve(ch, NULL, gsn_whipstitch, TRUE, 1);
	victim->hit += number_fuzzy(ch->level * (victim->position == POS_SLEEPING ? 3 : 2));
	victim->hit = UMIN(victim->max_hit, victim->hit);
}

void do_anesthetize(CHAR_DATA * ch, char * argument)
{
	char arg[MAX_STRING_LENGTH];
	CHAR_DATA * victim = NULL;
	bool heavy = FALSE;

	int chance = 0;

	if ((chance = get_skill(ch, gsn_anesthetize)) <= 0)
	{
		send_to_char("Huh?\n\r", ch);
		return;
	}

	argument = one_argument(argument, arg);

	if (arg[0] == '\0')
	{
		send_to_char("Who do you want to anesthetize?\n\r", ch);
		return;
	}

	if (UPPER(argument[0]) == 'H')
		heavy = TRUE;
	
	if ((victim = get_char_room(ch, arg)) == NULL)
	{
		send_to_char("You don't see them here.\n\r", ch);
		return;
	}

	if (is_loc_affected(ch, gsn_anesthetize, APPLY_NONE))
	{
		send_to_char("Your anesthetic is not yet prepared.\n\r", ch);
		return;
	}

	if (is_loc_not_affected(victim, gsn_anesthetize, APPLY_NONE))
	{
		send_to_char("They are already anesthetized.\n\r", ch);
		return;
	}

	/* Past the possibility checks */
	expend_mana(ch, skill_table[gsn_anesthetize].min_mana * (heavy ? 2 : 1));
	WAIT_STATE(ch, skill_table[gsn_anesthetize].beats);

	AFFECT_DATA af;
	af.valid = TRUE;
	af.point = NULL;
	af.location  = APPLY_NONE;
	af.where     = TO_AFFECTS;
	af.type      = gsn_anesthetize;
	af.level     = ch->level;
	af.duration  = number_fuzzy(heavy ? 4 : 2);
	af.modifier = 0;
	af.bitvector = 0;
	affect_to_char(ch, &af);
	
	// Chance of dodging if not in same group
	if (!is_same_group(ch, victim))
	{
		act("You rush at $N with a rag soaked in anesthetic!", ch, NULL, victim, TO_CHAR);
		act("$n rushes at you with a rag soaked in anesthetic!", ch, NULL, victim, TO_VICT);
		act("$n rushes at $N with a rag soaked in anesthetic!", ch, NULL, victim, TO_NOTVICT);
	
		if (victim->position > POS_SLEEPING)
		{
			if (victim->fighting == NULL) 
				set_fighting(victim, ch);
		
			if (check_dodge(ch, victim, NULL)) 
			{
				check_improve(ch, victim, gsn_anesthetize, FALSE, 1);
				return;
			}
		}
	}
	else
	{
		act("You apply an anesthetic to $N.", ch, NULL, victim, TO_CHAR);
		act("$n applies an anesthetic to you.", ch, NULL, victim, TO_VICT);
		act("$n applies an anesthetic to $N.", ch, NULL, victim, TO_NOTVICT);
	}

	if (number_percent() > chance || saves_spell(ch->level + (heavy ? 4 : 0), ch, victim, DAM_MENTAL))
	{
		act("$N seems unaffected by the anesthetic.", ch, NULL, victim, TO_CHAR);
		act("You resist the effects of the anesthetic.", ch, NULL, victim, TO_VICT);
		act("$N seems unaffected by the anesthetic.", ch, NULL, victim, TO_NOTVICT);
		check_improve(ch, victim, gsn_anesthetize, FALSE, 1);
		return;
	}

	// Success, prepare the affect
	af.location  = APPLY_DEX;
	af.duration  = number_fuzzy(heavy ? 8 : 6);
	af.modifier = (heavy ? -2 : -1);

	// Test for sleep
	if (number_bits(heavy ? 7 : 8) == 0 && !saves_spell(ch->level + (heavy ? 2 : -2), ch, victim, DAM_MENTAL))
	{
		act("$N crumples into a heap!", ch, NULL, victim, TO_CHAR);
		act("You crumple into a heap!", ch, NULL, victim, TO_VICT);
		act("$N crumples into a heap!", ch, NULL, victim, TO_NOTVICT);
		af.bitvector = AFF_SLEEP;
	}
	else
	{
		act("$N staggers from the anesthetic.", ch, NULL, victim, TO_CHAR);
		act("You stagger from the anesthetic.", ch, NULL, victim, TO_VICT);
		act("$N staggers from the anesthetic.", ch, NULL, victim, TO_NOTVICT);
	}
	affect_to_char(victim, &af);
	check_improve(ch, victim, gsn_anesthetize, TRUE, 1);
}
		
void do_animate_dead(CHAR_DATA * ch, char * argument)
{
	// Perform initial checks
	int chance = get_skill(ch, gsn_animate_dead);
	if (chance <= 0)
	{
		send_to_char("Huh?\n\r", ch);
		return;
	}

	if (ch->pet != NULL)
	{
		send_to_char("You already have a loyal follower.\n\r", ch);
		return;
	}

	if (ch->mana < skill_table[gsn_animate_dead].min_mana)
	{
		send_to_char("You are too tired to animate a corpse right now.\n\r", ch);
		return;
	}

	if (is_affected(ch, gsn_animate_dead))
	{
		send_to_char("You do not yet feel prepared to animate another corpse again.\n\r", ch);
		return;
	}

	char arg[MAX_INPUT_LENGTH];
	argument = one_argument(argument, arg);
	if (arg[0] == '\0')
	{
		send_to_char("What do you want to animate?\n\r", ch);
		return;
	}

	if (ch->in_room == NULL)
	{
		std::ostringstream mess;
		mess << "(" << (ch->name ? ch->name : "null") << ") Null in_room for char";
		bug(mess.str().c_str(), 0);

		send_to_char("You are not in a room.\n\r", ch);
		return;
	}

	OBJ_DATA * corpse = get_obj_list(ch, arg, ch->in_room->contents);
	if (corpse == NULL)
	{
		send_to_char("You do not see any such thing here.\n\r", ch);
		return;
	}

	if (corpse->pIndexData->vnum != OBJ_VNUM_CORPSE_NPC && corpse->pIndexData->vnum != OBJ_VNUM_CORPSE_PC)
	{
		send_to_char("You can only animate corpses.\n\r", ch);
		return;
	}

	if (IS_SET(corpse->value[1], CORPSE_DESTROYED))
	{
		send_to_char("That corpse is too mangled to animate.\n\r", ch);
		return;
	}

	// Skill is clear to proceed
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_animate_dead].beats));
	expend_mana(ch, skill_table[gsn_animate_dead].min_mana);

	// Check for failure
	if (number_percent() >= chance)
	{
		act("$n screws around with $p, but fails to animate it.", ch, corpse, NULL, TO_ROOM);
		act("You screw around with $p, but fail to animate it.", ch, corpse, NULL, TO_CHAR);
		check_improve(ch, NULL, gsn_animate_dead, FALSE, 1);
		return;
	}

	// Success; animate the corpse
	check_improve(ch, NULL, gsn_animate_dead, TRUE, 1);

	CHAR_DATA * zombie = create_mobile(get_mob_index(MOB_VNUM_CHIRURGEON_ZOMBIE));
	if (zombie == NULL)
	{
		send_to_char("An error occurred.  Please inform an immortal.\n\r", ch);
		bug("Failed to create chirurgeon zombie", 0);
		return;
	}

	SET_BIT(zombie->act, ACT_PET);
	SET_BIT(zombie->affected_by, AFF_CHARM);
	zombie->comm = COMM_NOTELL|COMM_NOSHOUT|COMM_NOCHANNELS;
	
	zombie->level = corpse->level;
	zombie->hit = 30 * zombie->level;
	zombie->max_hit = zombie->hit;
	zombie->hitroll = zombie->level;
	zombie->damroll = zombie->level;
	zombie->damage[0] = zombie->level / 5;
	zombie->damage[1] = 4;
	zombie->damage[2] = zombie->level / 5;
	zombie->race = corpse->value[2];
	
	for (size_t i(0); i < 4; ++i)
		zombie->armor[0] = -1 * zombie->level;

	// Build the strings
	std::string name("zombie ");
	if (corpse->owner != NULL)
		name += corpse->owner;
	else if (corpse->value[0] >= 0)
	{
		MOB_INDEX_DATA * mobIndex = get_mob_index(corpse->value[0]);
		if (mobIndex != NULL && mobIndex->player_name != NULL)
			name += mobIndex->player_name;
	}
	setName(*zombie, name.c_str());

	free_string(zombie->short_descr);
	zombie->short_descr = str_dup("a zombie");

	name = "A zombie animated from ";
	if (corpse->short_descr)
		name += corpse->short_descr;
	name += " shambles along here.\n\r";
	free_string(zombie->long_descr);
	zombie->long_descr = str_dup(name.c_str());

	// Zombie built, bring it to life
	char_to_room(zombie, ch->in_room);
	ch->pet = zombie;
	zombie->master = ch;
	zombie->leader = ch;

	// Let everyone know about it
	act("$n applies some seriously messed up stuff to $p, and gets a zombie in return.", ch, corpse, NULL, TO_ROOM);
	act("You apply some seriously messed up stuff to $p, and get a zombie in return.", ch, corpse, NULL, TO_CHAR);

	// Apply the affect to the PC to prevent animating another yet
	AFFECT_DATA af = {0};
	af.where    = TO_AFFECTS;
	af.type     = gsn_animate_dead;
	af.level    = ch->level;
	af.duration = 50 - (ch->level / 5);
	affect_to_char(ch, &af);

	// Put all of the items that were in the corpse in the zombie's possession
	OBJ_DATA * next_iter(NULL);
	for (OBJ_DATA * iter(corpse->contains); iter != NULL; iter = next_iter)
	{
		next_iter = iter->next_content;
		obj_from_obj(iter);
		obj_to_char(iter, zombie);
	}

	// Get rid of the corpse
	extract_obj(corpse);
}
