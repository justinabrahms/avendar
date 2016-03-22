#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <time.h>
#include "merc.h"
#include "magic.h"
#include "spells_spirit.h"
#include "NameMaps.h"

/* External declarations */
DECLARE_DO_FUN(do_autoyell);
DECLARE_DO_FUN(do_scan);

extern	void		free_affect	args( ( AFFECT_DATA *af ) );
extern	AFFECT_DATA *	new_affect	args( ( void ) );
extern 	bool		hands_free	args((CHAR_DATA * ch));
extern  EXIT_DATA *     new_exit        args( ( void ) );
extern	void 		do_look		args( (CHAR_DATA *ch, char *argument ) );
extern	EXTRA_DESCR_DATA *	new_extra_descr	args( ( void ) );

bool can_transmogrify(CHAR_DATA *, int);
bool do_generaltransmogrify(int, int, CHAR_DATA *, int, int, int, bool);
void do_generalaffect(CHAR_DATA *, int, int, int, int, int);
void do_transmogrify_messages(int, CHAR_DATA *);

/* Following are spells available in potion form.
 * These were implemented for alchemists.
 */
void do_generalaffect(CHAR_DATA * ch, int sn, int mod, int loc, int dur, int bitv)
{
	AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = ch->level;
	af.duration  = dur;
	af.modifier  = mod;
	af.location  = loc;
	af.bitvector = bitv;
	affect_to_char( ch, &af );
	
	return;
}

bool spell_gills( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
	
	if (is_affected(ch, gsn_gills))
	{
		send_to_char("You already have gills.\n\r", ch);
		return FALSE;
	}
	do_generalaffect(ch, sn, 0, APPLY_NONE, level / 2, 0);

	af.where     = TO_PAFFECTS;
	af.type      = sn;
	af.level     = level;
	af.duration  = level/2;
	af.modifier  = 0;
	af.location  = 0;
	af.bitvector = AFF_AIRLESS;
	affect_to_char( ch, &af );
	
	act("You suddenly develop gills to replace your lungs!", ch, NULL, NULL, TO_CHAR);
	act("$n's neck bulges and flexes oddly, and $s chest deflates!", ch, NULL, NULL, TO_ROOM);
	
	return TRUE;
}

bool spell_drunkenness( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	gain_condition( ch, COND_DRUNK,  100);	
	act("You are absolutely wasted.", ch, NULL, NULL, TO_CHAR);
	act("$n suddenly looks very inebriated.", ch, NULL, NULL, TO_ROOM);

	return TRUE;
}
bool spell_lovepotion( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	if (is_affected(ch, gsn_lovepotion))
	{
		send_to_char("You are already vulnerable to someone's affections.\n\r", ch);
		return FALSE;
	}
	do_generalaffect(ch, sn, 0, APPLY_NONE, level / 2, 0);

	act("You feel vulnerable to anyone's affection.", ch, NULL, NULL, TO_CHAR);
	act("$n suddenly blushes.", ch, NULL, NULL, TO_ROOM);

	return TRUE;
}
bool spell_susceptibility( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	if (is_affected(ch, gsn_susceptibility))
	{
		send_to_char("You are already susceptible.\n\r", ch);
		return FALSE;
	}

	do_generalaffect(ch, sn, -33, number_range(APPLY_RESIST_MAGIC, APPLY_RESIST_IRON), level / 2, 0);

	act("You feel more susceptible.", ch, NULL, NULL, TO_CHAR);

	return TRUE;
}
bool spell_age( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	int increase = number_range(1, 4);
	ch->pcdata->last_age += increase;
	ch->id -= (increase * ((PULSE_TICK * 2) / PULSE_PER_SECOND * NUM_HOURS_DAY * NUM_DAYS_YEAR));
	act("You suddenly feel older.", ch, NULL, NULL, TO_CHAR);
	act("$n suddenly looks older.", ch, NULL, NULL, TO_ROOM);

	return TRUE;
}
bool spell_polyglot( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	if (is_affected(ch, gsn_polyglot))
	{
		send_to_char("You already understand everyone!\n\r", ch);
		return FALSE;
	}
	do_generalaffect(ch, sn, 0, APPLY_NONE, level / 2, 0);

	act("You feel able to speak any tongue.", ch, NULL, NULL, TO_CHAR);

	return TRUE;
}
bool spell_teleportcurse(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	if (is_affected(ch, gsn_teleportcurse))
	{
		send_to_char("You are already afflicted with the teleport curse.\n\r", ch);
		return FALSE;
	}
	do_generalaffect(ch, sn, 0, APPLY_NONE, level, 0);

	act("You feel gripped by the forces of chaos!", ch, NULL, NULL, TO_CHAR);
	
	return TRUE;
}
bool spell_resistance( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	if (is_affected(ch, gsn_resistance))
	{
		send_to_char("You are already resistant.\n\r", ch);
		return FALSE;
	}
	do_generalaffect(ch, sn, 33, number_range(APPLY_RESIST_MAGIC, APPLY_RESIST_IRON), level / 2, 0);

	act("You feel more resistant.", ch, NULL, NULL, TO_CHAR);

	return TRUE;
}
bool spell_heroism( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	if (is_affected(ch, gsn_heroism))
	{
		send_to_char("You already feel heroic.\n\r", ch);
		return FALSE;
	}
	do_generalaffect(ch, sn, 100, APPLY_HIT, level / 2, 0);
	do_generalaffect(ch, sn, 15, APPLY_DAMROLL, level / 2, 0);
	do_generalaffect(ch, sn, 15, APPLY_HITROLL, level / 2, 0);
	do_generalaffect(ch, sn, 3, APPLY_MAXSTR, level / 2, 0);
	do_generalaffect(ch, sn, 3, APPLY_MAXDEX, level / 2, 0);
	do_generalaffect(ch, sn, 3, APPLY_STR, level / 2, 0);
	do_generalaffect(ch, sn, 3, APPLY_DEX, level / 2, 0);

	act("You suddenly feel very heroic!", ch, NULL, NULL, TO_CHAR);
	act("$n's frame seems to grow slightly, and $s muscles bulge!", ch, NULL, NULL, TO_ROOM);

	return TRUE;
}
bool spell_divinesight( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	if (is_affected(ch, gsn_divinesight))
	{
		send_to_char("You are already imbued with divine vision.\n\r", ch);
		return FALSE;
	}
	do_generalaffect(ch, sn, 0, APPLY_NONE, level, 
			AFF_DETECT_INVIS|AFF_DETECT_HIDDEN|AFF_DETECT_GOOD|AFF_DETECT_EVIL|AFF_INFRARED|AFF_SHARP_VISION);

	act("Your vision clears as you gain divine sight.", ch, NULL, NULL, TO_CHAR);

	return TRUE;
}
bool spell_youth( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	int decrease = number_range(1, 4);
	if (IS_NPC(ch))
	    return FALSE;
        ch->pcdata->last_age -= decrease;
	ch->id += (decrease * ((PULSE_TICK * 2) / PULSE_PER_SECOND * NUM_HOURS_DAY * NUM_DAYS_YEAR));
		
	act("You suddenly feel younger.", ch, NULL, NULL, TO_CHAR);
	act("$n suddenly looks younger.", ch, NULL, NULL, TO_ROOM);

	return TRUE;
}
bool spell_invulnerability( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
	
	if (is_affected(ch, gsn_invulnerability))
	{
		send_to_char("You are already invulnerable.\n\r", ch);
		return FALSE;
	}
	do_generalaffect(ch, sn, -300, APPLY_AC, 4, 0);
	do_generalaffect(ch, sn, 75, APPLY_RESIST_MAGIC, 4, 0);

        af.where     = TO_IMMUNE;
	af.type      = sn;
	af.level     = ch->level;
	af.duration  = 4;
	af.modifier  = 0;
	af.location  = 0;
	af.bitvector = IMM_WEAPON;
	affect_to_char( ch, &af );
	
	act("You suddenly feel invulnerable!", ch, NULL, NULL, TO_CHAR);

	return TRUE;
}
bool spell_perfection( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	if (is_affected(ch, gsn_perfection))
	{
		send_to_char("You are already perfect.\n\r", ch);
		return FALSE;
	}
	do_generalaffect(ch, sn, 15, APPLY_DAMROLL, level / 2, 0);
	do_generalaffect(ch, sn, 15, APPLY_HITROLL, level / 2, 0);
	do_generalaffect(ch, sn, 5, APPLY_MAXSTR, level / 2, 0);
	do_generalaffect(ch, sn, 5, APPLY_MAXDEX, level / 2, 0);
	do_generalaffect(ch, sn, 5, APPLY_STR, level / 2, 0);
	do_generalaffect(ch, sn, 5, APPLY_DEX, level / 2, 0);
							
	act("You suddenly feel perfect.", ch, NULL, NULL, TO_CHAR);

	return TRUE;
}
bool spell_improvement( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	int stat = number_range(STAT_STR, STAT_CHR);
	set_perm_stat(ch, stat, UMIN(ch->perm_stat[stat] + 1, get_max_train(ch,stat)));
	ch->max_hit += 5;
	ch->hit += 5;
	act("You feel improved!", ch, NULL, NULL, TO_CHAR);

	return TRUE;
}


bool can_transmogrify(CHAR_DATA * ch, int delay)
{	
	if (is_affected(ch, gsn_transmogrification))
	{
		send_to_char("You don't yet feel able to transmogrify yourself again.\n\r", ch);
		return FALSE;
	}
	do_generalaffect(ch, gsn_transmogrification, 0, 0, delay + number_range(1,6), 0);
	return TRUE;	
}

bool do_generaltransmogrify(int sn, int level, CHAR_DATA * ch, int dam, int hit, int hp, bool drop_weapon)
{
	OBJ_DATA * obj;
	int delay = level / 4, i, item;

	if (!can_transmogrify(ch, delay)) return FALSE;
				        
	do_generalaffect(ch, sn, dam, (dam == 0 ? APPLY_NONE : sn == skill_lookup("greater carapace") ? 
			APPLY_RESIST_WEAPON : APPLY_DAMROLL), delay, (sn == skill_lookup("cat eyes") ? AFF_INFRARED : 0));
	if (hit != 0) do_generalaffect(ch, sn, hit, APPLY_HITROLL, delay, 0);
	if (hp != 0) do_generalaffect(ch, sn, hp, APPLY_HIT, delay, 0);
		
	do_transmogrify_messages(sn, ch);

	if (drop_weapon)
	{
	       send_to_char("With your hands suddenly mutated, you lose grip on anything you were holding!\n\r", ch);
	       act("With $s hands suddenly mutated, $n loses grip on anything $e was holding!\n\r", ch, NULL, NULL, TO_ROOM);
		for (i = 0; i < 4; i++)
		{
			switch(i)
			{
				case 0: item = WEAR_HOLD; break;
				case 1: item = WEAR_WIELD; break;
				case 2: item = WEAR_SHIELD; break;
				case 3: item = WEAR_DUAL_WIELD; break;
				default: item = WEAR_HOLD; break;
			}
			if ((obj = get_eq_char(ch, item)) != NULL)
			{
				act("$p falls to the floor!", ch, obj, NULL, TO_CHAR);
				act("$p falls to the floor!", ch, obj, NULL, TO_ROOM);
				obj_from_char(obj);
				obj_to_room(obj, ch->in_room);
			}
		}
	}
																     return TRUE;
}

void do_transmogrify_messages(int sn, CHAR_DATA * ch)
{
	if (sn == gsn_aviancurse)
	{
		act("Your body bends into itself as you adopt the form of a hen!", ch, NULL, NULL, TO_CHAR);
		act("$n's body warps and twists, gradually forming into that of a hen!", ch, NULL, NULL, TO_ROOM);
		return;
	}
	if (sn == gsn_lesserhandwarp || sn == gsn_handwarp)
	{
		act("You suddenly grow long, sharp claws!", ch, NULL, NULL, TO_CHAR);
		act("$n suddenly grows long, sharp claws!", ch, NULL, NULL, TO_ROOM);
		return;
	}
	if (sn == gsn_greaterhandwarp)
	{
		act("You suddenly grow powerful claws!", ch, NULL, NULL, TO_CHAR);
		act("$n suddenly grows powerful claws!", ch, NULL, NULL, TO_ROOM);
		return;
	}
	if (sn == gsn_lessertentaclegrowth)
	{
		act("Several small, snaky tentacles suddenly sprout from you!", ch, NULL, NULL, TO_CHAR);
		act("Several small, snaky tentacles suddenly sprout from $n!", ch, NULL, NULL, TO_ROOM);
		return;
	}
	if (sn == gsn_greatertentaclegrowth)
	{
		act("Long, powerful tentacles suddenly snake out of your body!", ch, NULL, NULL, TO_CHAR);
		act("Long, powerful tentacles suddenly snake out of $n's body!", ch, NULL, NULL, TO_ROOM);
		return;
	}
	if (sn == gsn_lessercarapace || sn == gsn_greatercarapace)
	{
		act("You become covered in a hardened carapace.", ch, NULL, NULL, TO_CHAR);
		act("$n becomes covered in a hardened carapace.", ch, NULL, NULL, TO_ROOM);
		return;
	}
	if (sn == gsn_lycanthropy)
	{
	  act("Your body seems to twist into itself, and when next you view the world, it is through the eyes of a wolf.", 
		ch, NULL, NULL, TO_CHAR);
	  act("$n's form stretches and shifts, rapidly transforming into that of a powerful wolf!", ch, NULL, NULL, TO_ROOM);
		return;
	}
	if (sn == gsn_thirdeye)
	{
		act("Your vision blurs, then clears as a third eye forms on your forehead.", ch, NULL, NULL, TO_CHAR);
		act("$n's forehead twists momentarily, then forms a third eye!", ch, NULL, NULL, TO_ROOM);
		return;
	}
	if (sn == gsn_cateyes)
	{
		act("Your eyes focus on the darkness as they become more feline.", ch, NULL, NULL, TO_CHAR);
		act("$n's eyes swiftly become slitted and catlike.", ch, NULL, NULL, TO_ROOM);
		return;
	}
	if (sn == gsn_eagleeyes)
	{
		act("Your eyes obtain a heightened focus!", ch, NULL, NULL, TO_CHAR);
		act("$n's eyes seem to gain new clarity.", ch, NULL, NULL, TO_ROOM);
		return;
	}
	
	send_to_char("Your body warps and twists!\n\r", ch);
	act("$n's body warps and twists!", ch, NULL, NULL, TO_ROOM);
	return;
}

bool spell_lesserhandwarp( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	return do_generaltransmogrify(sn, level, ch, 8, 8, 0, TRUE);
}
bool spell_handwarp( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	return do_generaltransmogrify(sn, level, ch, 15, 15, 0, TRUE);
}
bool spell_greaterhandwarp( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	return do_generaltransmogrify(sn, level, ch, 25, 25, 0, TRUE);
}

bool spell_lessertentaclegrowth( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	return do_generaltransmogrify(sn, level, ch, 0, 0, 0, TRUE);
}
bool spell_greatertentaclegrowth( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	return do_generaltransmogrify(sn, level, ch, 0, 0, 0, TRUE);
}
bool spell_aviancurse(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
	char buf[MAX_STRING_LENGTH];
	
	if (do_generaltransmogrify(sn, level, ch, -25, -25, -1 * (ch->max_hit - 25), TRUE))
	{
		if (ch->orig_long[0] == '\0' && ch->long_descr[0] != '\0')
		{
			free_string(ch->orig_long);
			ch->orig_long = str_dup(ch->long_descr);
		}
		free_string(ch->long_descr);
		ch->long_descr = str_dup("A large hen is here.\n\r");
        setFakeName(*ch, "large hen");
		free_string(ch->short_descr);
		ch->short_descr = str_dup("a large hen");
		if (ch->orig_description[0] == '\0')
		{
			free_string(ch->orig_description);
			ch->orig_description = str_dup(ch->description);
		}
		free_string(ch->description);
		ch->description = str_dup("A large hen picks and pecks at the ground.\n\r");
		sprintf(buf, "%s%ld", "hen", ch->id);
        setUniqueName(*ch, buf);
		ch->fake_race = ch->race;

		af.where     = TO_OAFFECTS;
		af.type      = gsn_aviancurse;
		af.level     = ch->level;
		af.duration  = ch->level / 4;
		af.location  = 0;
		af.modifier  = 0;
		af.bitvector = AFF_DISGUISE;
		affect_to_char(ch, &af);
								 
		return TRUE;
	}
	return FALSE;
}

bool spell_lessercarapace( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	return do_generaltransmogrify(sn, level, ch, 0, 0, 0, FALSE);
}
bool spell_greatercarapace( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	return do_generaltransmogrify(sn, level, ch, 15, 0, 0, FALSE);
}

bool spell_lycanthropy( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
	char buf[MAX_STRING_LENGTH];
	
	if (do_generaltransmogrify(sn, level, ch, 20, 20, level, TRUE))
	{
		if (ch->orig_long[0] == '\0' && ch->long_descr[0] != '\0')
		{
			free_string(ch->orig_long);
			ch->orig_long = str_dup(ch->long_descr);
		}
		free_string(ch->long_descr);
		ch->long_descr = str_dup("A large, ferocious wolf is here.\n\r");
        setFakeName(*ch, "large ferocious wolf");
		free_string(ch->short_descr);
		ch->short_descr = str_dup("a large wolf");
		if (ch->orig_description[0] == '\0')
		{
			free_string(ch->orig_description);
			ch->orig_description = str_dup(ch->description);
		}
		free_string(ch->description);
		ch->description = str_dup("This powerfully-built wolf pads along, growling.\n\r");
		sprintf(buf, "%s%ld", "wolf", ch->id);
        setUniqueName(*ch, buf);
		ch->fake_race = ch->race;

		af.where     = TO_OAFFECTS;
		af.type      = gsn_lycanthropy;
		af.level     = ch->level;
		af.duration  = ch->level / 4;
		af.location  = 0;
		af.modifier  = 0;
		af.bitvector = AFF_DISGUISE;
		affect_to_char(ch, &af);

		return TRUE;
	}

	return FALSE;
}

bool spell_thirdeye( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	if (ch->race != global_int_race_shuddeni)
		return do_generaltransmogrify(sn, level, ch, 0, 0, 0, FALSE);
	send_to_char("Being a shuddeni, it hardly makes sense for you to have a third eye.\n\r", ch);
	return FALSE;
}
bool spell_cateyes( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	if (ch->race != global_int_race_shuddeni)
		return do_generaltransmogrify(sn, level, ch, 0, 0, 0, FALSE);
	send_to_char("As you are shuddeni, you don't have eyes to make catlike.\n\r", ch);
	return FALSE;
}
bool spell_eagleeyes( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	if (ch->race != global_int_race_shuddeni)
		return do_generaltransmogrify(sn, level, ch, 0, 2, 0, FALSE);
	send_to_char("Since you are shuddeni, your nonexistent eyes cannot become sharper.\n\r", ch);
	return FALSE;
}

/* End alchemist-potion spells */

bool spell_acid_blast( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = dice( level, 12 );
    if ( saves_spell( level, ch, victim, DAM_ACID ) )
        dam /= 2;
    damage_old( ch, victim, dam, sn,DAM_ACID,TRUE);
    return TRUE;
}

bool do_prismaticbeam(int sn, int level, CHAR_DATA *ch, void *vo, int target, int beam)
{
	CHAR_DATA * victim = (CHAR_DATA *) vo;
	switch(beam)
	{
		case 0: act("A ray of {rred{x light streaks towards $N!", ch, NULL, victim, TO_CHAR);
			act("A ray of {rred{x light streaks towards $N!", ch, NULL, victim, TO_NOTVICT);
			if (ch != victim) act("A ray of {rred{x light streaks towards you!", ch, NULL, victim, TO_VICT);
			spell_beam_of_fire(gsn_beamoffire, level, ch, vo, target);
			break;
		case 1: act("A ray of {bblue{x light streaks towards $N!", ch, NULL, victim, TO_CHAR);
			act("A ray of {bblue{x light streaks towards $N!", ch, NULL, victim, TO_NOTVICT);
			if (ch != victim) act("A ray of {bblue{x light streaks towards you!", ch, NULL, victim, TO_VICT);
			spell_encase(gsn_encase, level, ch, vo, target);
			break;
		case 2: act("A ray of {ybrown{x light streaks towards $N!", ch, NULL, victim, TO_NOTVICT);
			act("A ray of {ybrown{x light streaks towards you!", ch, NULL, victim, TO_VICT);
			if (ch != victim) act("A ray of {ybrown{x light streaks towards $N!", ch, NULL, victim, TO_CHAR);
			spell_petrify(sn, level, ch, vo, target);
			break;
		case 3: act("A ray of {Ygolden{x light streaks towards $N!", ch, NULL, victim, TO_NOTVICT);
			act("A ray of {Ygolden{x light streaks towards you!", ch, NULL, victim, TO_VICT);
			if (ch != victim) act("A ray of {Ygolden{x light streaks towards $N!", ch, NULL, victim, TO_CHAR);
			spell_spiritwrack(sn, level, ch, vo, target);
			break;
		case 4: act("A ray of {Wwhite{x light streaks towards $N!", ch, NULL, victim, TO_NOTVICT);
			act("A ray of {Wwhite{x light streaks towards you!", ch, NULL, victim, TO_VICT);
			if (ch != victim) act("A ray of {Wwhite{x light streaks towards $N!", ch, NULL, victim, TO_CHAR);
			spell_thunderclap(sn, level, ch, vo, target);
			break;
		case 5: act("A ray of {Dblack{x light streaks towards $N!", ch, NULL, victim, TO_NOTVICT);
			act("A ray of {Dblack{x light streaks towards you!", ch, NULL, victim, TO_VICT);
			if (ch != victim) act("A ray of {Dblack{x light streaks towards $N!", ch, NULL, victim, TO_CHAR);
			spell_pox(gsn_pox, level, ch, vo, target);
			break;
		default:break;
	}
	return TRUE;
}

bool spell_prismaticray(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	int rays = number_range(1, 6), i;
	for (i = 0; i < rays; i++) do_prismaticbeam(sn, level, ch, vo, target, number_range(0,5));
	return TRUE;
}

bool spell_prismaticspray(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA * victim = (CHAR_DATA *) vo, *vch = NULL;
	
	for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
	{
		if (vch == NULL) break;
		if ((is_same_group(vch, victim) || vch->fighting == ch)
		&& !is_safe_spell(ch, vch, TRUE)
		&& !(IS_AFFECTED(vch, AFF_WIZI) || (!IS_IMMORTAL(ch) && IS_IMMORTAL(vch)))
		&& (IS_NPC(vch) || IS_PK(vch, ch)))
		{
			do_prismaticbeam(sn, level, ch, vch, target, number_range(0, 5));
		}
	}
	return TRUE;
}

void destroy_maze(ROOM_INDEX_DATA * pRoomIndex, ROOM_INDEX_DATA *tRoom)
{
	CHAR_DATA *vch, *vch_next;
	OBJ_DATA *pObj, *obj_next;
	
	if (!tRoom) tRoom = get_room_index(ROOM_VNUM_TEMPLE);
	for (vch = pRoomIndex->people; vch; vch = vch_next)
	{
		vch_next = vch->next_in_room;
		global_linked_move = TRUE;
		send_to_char("The maze dissolves around you!\n\r", vch);
		send_to_char("You pass through a confusion of space, filled with the dizzying whirls and eddies of existence itself!\n\r", vch);
		spell_prismaticray(gsn_prismaticray, 70, vch, vch, skill_table[gsn_prismaticray].target);
		spell_prismaticray(gsn_prismaticray, 70, vch, vch, skill_table[gsn_prismaticray].target);
		spell_prismaticray(gsn_prismaticray, 70, vch, vch, skill_table[gsn_prismaticray].target);
		vch->hit /= 2;
		send_to_char("As the madness ends, you are left to recover as best you can in the Prime Materium.\n\r", vch);
		
		char_from_room(vch);
		char_to_room(vch, tRoom);
		if (tRoom->vnum != 0) vch->was_in_room = NULL;
		do_look(vch, "auto");
		act("$n suddenly blinks into existence.", vch, NULL, NULL, TO_ROOM);
	}
	
	for (pObj = pRoomIndex->contents; pObj; pObj = obj_next)
	{
		obj_next = pObj->next_content;
		obj_from_room(pObj);
		obj_to_room(pObj, tRoom);
	}
	free_room_area(pRoomIndex);
	return;
}

bool spell_maze(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	char buf[MAX_STRING_LENGTH]; 
	CHAR_DATA *victim = (CHAR_DATA *) vo;
    	AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
        ROOM_INDEX_DATA *maze[MAZE_SIZE];
	EXTRA_DESCR_DATA *ed;
	int good_exit = number_range(0, 5), i, m;

	if (!ch->in_room) return FALSE;

	if (room_is_affected(ch->in_room, gsn_maze))
	{
		send_to_char("You cannot form a maze from within a maze.\n\r", ch);
		return FALSE;
	}

	if (((IS_NPC(victim) && IS_SET(victim->act, ACT_NOSUBDUE)) 
	|| (number_percent() < (get_curr_stat(victim, STAT_DEX) * 1.3))) && (victim != ch))
	{
		act("A portal opens to suck in $n, but $e leaps away swiftly!", victim, NULL, NULL, TO_ROOM);
		send_to_char("A portal opens to suck you in, but you leap away swiftly!\n\r", victim);
		return TRUE;
	}

	act("A portal suddenly appears and engulfs $n before $e can escape!", victim, NULL, NULL, TO_ROOM);
	send_to_char("A portal suddenly gapes open and engulfs you!\n\r", victim);

	if (check_spirit_of_freedom(victim))
	{
		send_to_char("The spirit of freedom surges within you, and you break free!\n\r", victim);
		act("The portal seems to dissipate as $n breaks free!", victim, NULL, NULL, TO_ROOM);
		return TRUE;
	}

	af.where     = TO_ROOM;
	af.type      = sn;
	af.level     = level;
	af.modifier  = (victim->was_in_room ? victim->was_in_room->vnum : victim->in_room->vnum);
	af.duration  = 24;
	af.bitvector = 0;
	af.location  = APPLY_HIDE;
	affect_to_char(ch, &af);
	
	af.location  = APPLY_NONE;
	for (m = 0; m < MAZE_SIZE; m++)
	{
		sprintf(buf, "Within a Maze (%d)", m);
		maze[m] = new_room_area(victim->in_room->area);
		maze[m]->name        = (IS_IMMORTAL(victim) ? str_dup(buf) : str_dup("Within a Maze"));
		maze[m]->description = str_dup("You stand in a hexagonal gallery, carved from immutable crystal.\nTwenty empty, barren shelves sit on all sides of the gallery,\nthemselves separated by low railings. Peering over the sides of the\ngallery reveals an interminable procession of identical chambers, both\nabove and below as far as the eye can see. Light is provided by two\nwhite lamps, shaped like some spherical fruit and transversally\nplaced. Exits proceed in all directions from this chamber through\nnarrow hallways, but it is clear even from a cursory glance that the\nchambers in those directions are replicas of this one. Also through\nhere passes a spiral stairway, which sinks abysmally and soars upwards\nto remote distances.\n\r");
		ed                      =   new_extra_descr();
		ed->keyword             =   str_dup( "shelf shelves" );
		ed->description         =   str_dup( "Each is actually composed of five long shelves on a side, and their\nheight and dimensions are that of a normal bookcase. Lacking, however,\nare any signs of a book -- the shelves are completely barren.\n\r" );
		ed->next                =   maze[m]->extra_descr;
		maze[m]->extra_descr    =   ed;

		ed                      =   new_extra_descr();
		ed->keyword             =   str_dup( "lamp lamps" );
		ed->description         =   str_dup( "The lamps are shaped like some unknown, spherical fruit. The light\nthey emit is both insufficient and incessant.\n\r");
		ed->next                =   maze[m]->extra_descr;
		maze[m]->extra_descr    =   ed;

		ed                      =   new_extra_descr();
		ed->keyword             =   str_dup( "stair stairs stairwell stairway" );
		ed->description         =   str_dup( "The spiral stairway ascends and descends as far as can be seen. They\nmay very well continue onwards infinitely.\n\r" );
		ed->next                =   maze[m]->extra_descr;
		maze[m]->extra_descr    =   ed;

		ed                      =   new_extra_descr();
		ed->keyword             =   str_dup( "railing railings" );
		ed->description         =   str_dup( "Looking over the edge of the railing, it is clear that there are many\nmore chambers above, below, and off to the side which exactly mirror\nthis one. They are separated by vast air shafts, extending off into\nthe gray distance.\n\r" );
		ed->next                =   maze[m]->extra_descr;
		maze[m]->extra_descr    =   ed;
								
		maze[m]->room_flags  = ROOM_NOGATE|ROOM_NOSUM_TO|ROOM_NOSUM_FROM|ROOM_NOWEATHER;
		maze[m]->sector_type = ch->in_room->sector_type;
		SET_BIT(maze[m]->room_flags, ROOM_NO_RECALL);
		affect_to_room(maze[m], &af);
	}
	//Now to apply exits -- couldn't do it til they were all made
	for (m = 0; m < 16; m++)
	{
		for (i= 0; i < 6; i++)
		{
			maze[m]->exit[i] = new_exit();
			if (i == good_exit && m == (MAZE_SIZE - 5)) 
				maze[m]->exit[i]->u1.to_room = get_room_index(victim->in_room->vnum);
			else maze[m]->exit[i]->u1.to_room = maze[number_range(0, MAZE_SIZE - 1)];
		}
	}		

	if (victim->in_room->vnum != 0) victim->was_in_room = victim->in_room;

	char_from_room(victim);
	char_to_room(victim, maze[0]);
	do_look(victim, "auto");

	return TRUE;
}

bool spell_transmutation(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	OBJ_DATA * obj = (OBJ_DATA *) vo;
	int mat_chlonin=material_lookup("ch'lonin");
	int mat_meitzec=material_lookup("mei'tzec");
	int mat_elirium=material_lookup("elirium");

	if (obj->material == material_lookup("lead")) 
        obj->material = material_lookup("gold");
	else 
    {
	    do 
        {
	    	obj->material = number_range(0, MAX_MATERIALS - 1);
	    } 
        while (obj->material == mat_chlonin || obj->material == mat_meitzec || obj->material == mat_elirium);
    }
	
	if (obj->material == material_lookup("gold"))
	{
		act("$p shimmers slightly, then transmutes to pure gold!", ch, obj, NULL, TO_CHAR);
		act("$p shimmers slightly, then transmutes to pure gold!", ch, obj, NULL, TO_ROOM);
	}
	else
	{
		act("$p shimmers slightly as it transmutes.", ch, obj, NULL, TO_CHAR);
		act("$p shimmers slightly as it transmutes.", ch, obj, NULL, TO_ROOM);
	}
	return TRUE;
}

bool spell_acid_breath( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam,hp_dam,dice_dam,hpch;

    act("$n spits acid at $N.",ch,NULL,victim,TO_NOTVICT);
    act("$n spits a stream of corrosive acid at you.",ch,NULL,victim,TO_VICT);
    act("You spit acid at $N.",ch,NULL,victim,TO_CHAR);

    hpch = URANGE(12,ch->hit,2200);
    hp_dam = number_range(hpch/11 + 1, hpch/6);
    dice_dam = dice(level,16);

    dam = UMAX(hp_dam + dice_dam/10,dice_dam + hp_dam/10);

    if (check_defensiveroll(victim))
    {
	dam /= 2;
        send_to_char("You narrowly avoid the full blast of lightning.\n\r",victim);
    }

    if (saves_spell(level,ch, victim,DAM_ACID))
    {
        acid_effect(victim,level/2,dam/4,TARGET_CHAR);
        damage_old(ch,victim,dam/2,sn,DAM_ACID,TRUE);
    }
    else
    {
        acid_effect(victim,level,dam,TARGET_CHAR);
        damage_old(ch,victim,dam,sn,DAM_ACID,TRUE);
    }
    return TRUE;
}

bool spell_barbs(int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af = {0};
    af.where     = TO_OAFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 2;
    af.bitvector = AFF_BLEEDING;
    affect_to_char(victim, &af);

    act("The arrow's barbs dig into you, causing a vicious wound.", ch, NULL, victim, TO_VICT);
    act("The arrow's barbs dig into $N, causing a vicious wound.", ch, NULL, victim, TO_VICTROOM);
    act("The arrow's barbs dig into $N, causing a vicious wound.", ch, NULL, victim, TO_CHAR);
    return TRUE;
}

bool spell_brainwash( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( is_affected(victim, gsn_brainwash))
        {
        send_to_char("They are already brainwashed.\n\r",ch);
        return FALSE;
        }
    if (saves_spell(level,ch, victim,DAM_MENTAL))
        {
        send_to_char("Their mind resists your spell.\n\r",ch);
        return TRUE;
        }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.location  = 0;
    af.modifier  = 0;
    af.duration  = level/8;
    af.bitvector = 0;
    affect_to_char( victim, &af );
    send_to_char( "You have been brainwashed!\n\r", victim );
    act("$n appears to be very confused.",victim,NULL,NULL,TO_ROOM);
    return TRUE;
}

bool spell_call_lightning( int sn, int level,CHAR_DATA *ch,void *vo,int target)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int dam;

    if ( !IS_OUTSIDE(ch) )
    {
        send_to_char( "You must be out of doors.\n\r", ch );
        return FALSE;
    }

    if (!ch->in_room)
        return FALSE;

    if ( ch->in_room->area->w_cur.storm_str <= 0)
    {
        send_to_char( "You need bad weather.\n\r", ch );
        return FALSE;
    }


    dam = dice(level/2, 8);

    send_to_char( "Lightning strikes your foes!\n\r", ch );
    act( "$n calls lightning to strike $s foes!",
        ch, NULL, NULL, TO_ROOM );

    for ( vch = char_list; vch != NULL; vch = vch_next )
    {
        vch_next        = vch->next;
        if ( vch->in_room == NULL )
            continue;
        if ((vch->in_room == ch->in_room) && (ch!=vch))
        {
            if (!IS_IMMORTAL(vch))
            {
                if (!IS_NPC(vch))
                 if (can_see(vch, ch))
                {
                  sprintf( buf, "Help!  %s is attacking me!", PERS(ch, vch));
                }
                else
                {
                  sprintf( buf, "Help! I'm being attacked!");
                }
                do_autoyell( vch, buf );
                damage_old( ch, vch, saves_spell( level,ch, vch,DAM_LIGHTNING)
                ? dam / 2 : dam, sn,DAM_LIGHTNING,TRUE);
            }
            continue;
        }

        if ( vch->in_room->area == ch->in_room->area
        &&   IS_OUTSIDE(vch)
        &&   IS_AWAKE(vch) )
            send_to_char( "Lightning flashes in the sky.\n\r", vch );
    }

    return TRUE;
}

bool spell_cause_critical(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
   damage_old( ch, (CHAR_DATA *) vo, dice(3, 8) + level - 6, sn,DAM_NEGATIVE,TRUE);
    return TRUE;
}

bool spell_cause_light( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    damage_old(ch, (CHAR_DATA *) vo, dice(1, 8) + level / 3,sn,DAM_NEGATIVE,TRUE);
    return TRUE;
}

bool spell_cause_serious(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
   damage_old( ch, (CHAR_DATA *) vo, dice(2, 8) + level / 2,sn,DAM_NEGATIVE,TRUE);
    return TRUE;
}

bool spell_change_sex( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( is_affected( victim, sn ))
    {
        if (victim == ch)
          send_to_char("You've already been changed.\n\r",ch);
        else
          act("$N has already had $s(?) sex changed.",ch,NULL,victim,TO_CHAR);
        return FALSE;
    }
    if (saves_spell(level, ch, victim,DAM_OTHER))
        return TRUE;
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 2 * level;
    af.location  = APPLY_SEX;
    af.modifier = victim->sex == SEX_MALE ? 1 : 
	victim->sex == SEX_FEMALE ? -1 : number_range(1,2);
    
    while ( af.modifier == 0 );
    af.bitvector = 0;
    affect_to_char( victim, &af );
    send_to_char( "You feel different.\n\r", victim );
    act("$n doesn't look like $mself anymore...",victim,NULL,NULL,TO_ROOM);
    return TRUE;
}

bool spell_charm_person( int sn, int level, CHAR_DATA *ch, void *vo,int target ){
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_safe_spell(ch,victim,FALSE)) return FALSE;

    if ( victim == ch )
    {
        send_to_char( "You like yourself even better!\n\r", ch );
        return FALSE;
    }


    if (!can_charm_another(ch))
        return TRUE;

    if ( IS_AFFECTED(victim, AFF_CHARM)
    ||   IS_AFFECTED(ch, AFF_CHARM)
    ||   level < victim->level
    ||   IS_SET(victim->imm_flags,IMM_CHARM)
    ||   saves_spell( level, ch, victim,DAM_CHARM) )
        return TRUE;


    if ( victim->master )
        stop_follower( victim );
    add_follower( victim, ch );
    victim->leader = ch;
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = number_fuzzy( level / 4 );
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = AFF_CHARM;
    affect_to_char( victim, &af );
    act( "Isn't $n just so nice?", ch, NULL, victim, TO_VICT );
    if ( ch != victim )
        act("$N looks at you with adoring eyes.",ch,NULL,victim,TO_CHAR);
    return TRUE;
}

bool spell_clumsiness( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( is_affected( victim, sn ) || saves_spell( level, ch, victim,DAM_OTHER) )
        return TRUE;
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level / 2;
    af.location  = APPLY_DEX;
    af.modifier  = -1 * (level / 5);
    af.bitvector = 0;
    affect_to_char( victim, &af );
    send_to_char( "You feel clumsier.\n\r", victim );
    act("$n looks error-prone.",victim,NULL,NULL,TO_ROOM);
    return TRUE;
}

bool spell_colour_spray( int sn, int level, CHAR_DATA *ch, void *vo,int target ){
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    static const int dam_each[] =
    {
         0,
         0,  0,  0,  0,  0,      0,  0,  0,  0,  0,
        30, 35, 40, 45, 50,     55, 55, 55, 56, 57,
        58, 58, 59, 60, 61,     61, 62, 63, 64, 64,
        65, 66, 67, 67, 68,     69, 70, 70, 71, 72,
        73, 73, 74, 75, 76,     76, 77, 78, 79, 79
    };
    int dam;

    level       = UMIN(level, static_cast<int>(sizeof(dam_each)/sizeof(dam_each[0])) - 1);
    level       = UMAX(0, level);
    dam         = number_range( dam_each[level] / 2,  dam_each[level] * 2 );
    if ( saves_spell( level, ch, victim,DAM_LIGHT) )
        dam /= 2;
    else
        spell_blindness(gsn_blindness,
            level/2,ch,(void *) victim,TARGET_CHAR);
    damage_old( ch, victim, dam, sn, DAM_LIGHT,TRUE );
    return TRUE;
}

bool spell_combustion( int sn, int level, CHAR_DATA *ch, void *vo,
        int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = dice( level, 7 );
    if ( saves_spell( level, ch, victim, DAM_FIRE ) )
        dam /= 2;
    act ("$n unleashes a blast of fiery devastation upon $N!",
        ch,NULL,victim,TO_ROOM);
    damage_old( ch, victim, dam, sn,DAM_FIRE,TRUE);
    return TRUE;
}

bool spell_control_weather(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    return TRUE;
}

bool spell_coughing_dust( int sn, int level,CHAR_DATA *ch,void *vo,
        int target)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    static const int dam_each[] =
    {
          5,
          5,   5,   6,   6,   7,          7,   9,   9,  11,  13,
         15,  17,  19,  22,  25,         28,  31,  34,  37,  40,
         43,  46,  49,  52,  55,         58,  61,  64,  67,  70,
         92,  94,  96,  98, 100,        102, 104, 106, 108, 110,
        112, 114, 116, 118, 120,        122, 124, 126, 128, 130
    };
    int dam;

    level       = UMIN(level, static_cast<int>(sizeof(dam_each)/sizeof(dam_each[0])) - 1);
    level       = UMAX(0, level);

    send_to_char( "Your coughing dust swirls through the room!\n\r", ch );
    act( "$n conjures a swirling dust cloud, making it hard to breathe!",
        ch, NULL, NULL, TO_ROOM );

    for ( vch = char_list; vch != NULL; vch = vch_next )
    {
        vch_next        = vch->next;
        if ( vch->in_room == NULL )
            continue;
        if ((vch->in_room == ch->in_room) && (ch!=vch) &&
                !(is_same_group(ch, vch)) && !IS_IMMORTAL(vch))
            {
                if ((ch->fighting != vch) && vch->fighting != ch && !IS_NPC(vch))
                {
                  sprintf( buf, "Help! %s is summoning coughing dust into the air!", PERS(ch, vch));
                  do_autoyell( vch, buf );
                }
           dam          = number_range( dam_each[level] / 2, dam_each[level] * 2 );
                   if ( saves_spell( level, ch, vch, DAM_DROWNING) )
                        dam /= 2;

                damage_old( ch, vch, saves_spell( level,ch, vch,DAM_DROWNING)
                ? dam / 2 : dam, sn,DAM_DROWNING,TRUE);
            }
            continue;
    }

    return TRUE;
}

bool spell_create_rose( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    OBJ_DATA *rose;
    rose = create_object(get_obj_index(OBJ_VNUM_ROSE), 0);
    act("$n has created a beautiful red rose.",ch,rose,NULL,TO_ROOM);
    send_to_char("You create a beautiful red rose.\n\r",ch);
    obj_to_char(rose,ch);
    return TRUE;
}

bool spell_dalliance( int sn, int level, CHAR_DATA *ch, void *vo,
        int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (IS_NPC(victim))
        {
        send_to_char("They are too sober-minded to dally.\n\r", ch);
        return FALSE;
        }

    victim->pcdata->condition[COND_DRUNK] += level/5 + dice (level/10, 3);
    if (victim->pcdata->condition[COND_DRUNK] > 100)
        victim->pcdata->condition[COND_DRUNK] = 100;
    send_to_char( "You feel inebriated!\n\r", victim );
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return TRUE;
}

bool spell_demonfire(int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    if ( !IS_NPC(ch) && !IS_TRUE_EVIL(ch) )
    {
        victim = ch;
        send_to_char("The demons turn upon you!\n\r",ch);
    }

    if (victim != ch)
    {
        act("$n calls forth the demons of Hell upon $N!",
            ch,NULL,victim,TO_ROOM);
        act("$n has assailed you with the demons of Hell!",
            ch,NULL,victim,TO_VICT);
        send_to_char("You conjure forth the demons of hell!\n\r",ch);
    }
    dam = dice( level, 10 );
    if ( saves_spell( level, ch, victim,DAM_NEGATIVE) )
        dam /= 2;
    damage_old( ch, victim, dam, sn, DAM_NEGATIVE ,TRUE);
    spell_curse(gsn_curse, 3 * level / 4, ch, (void *) victim,TARGET_CHAR);
    return TRUE;
}

bool spell_detect_evil( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(victim, sn))
    {
        if (victim == ch)
          send_to_char("You can already sense evil.\n\r",ch);
        else
          act("$N can already detect evil.",ch,NULL,victim,TO_CHAR);
        return FALSE;
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_DETECT_EVIL;
    affect_to_char( victim, &af );
    send_to_char( "Your eyes tingle.\n\r", victim );
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return TRUE;
}

bool spell_detect_good( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( IS_AFFECTED(victim, AFF_DETECT_GOOD) )
    {
        if (victim == ch)
          send_to_char("You can already sense good.\n\r",ch);
        else
          act("$N can already detect good.",ch,NULL,victim,TO_CHAR);
        return TRUE;
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_DETECT_GOOD;
    affect_to_char( victim, &af );
    send_to_char( "Your eyes tingle.\n\r", victim );
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return TRUE;
}

bool spell_detect_magic( int sn, int level, CHAR_DATA *ch, void *vo,int target ){
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( is_affected(victim, sn) )
    {
        if (victim == ch)
          send_to_char("You can already sense magical auras.\n\r",ch);
        else
          act("$N can already detect magic.",ch,NULL,victim,TO_CHAR);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = 0;
    affect_to_char( victim, &af );
    send_to_char( "Your eyes tingle.\n\r", victim );
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return TRUE;
}

bool spell_detect_poison( int sn, int level, CHAR_DATA *ch, void *vo,int target){
    OBJ_DATA *obj = (OBJ_DATA *) vo;

    if ( obj->item_type == ITEM_DRINK_CON || obj->item_type == ITEM_FOOD )
    {
        if ( obj->value[3] != 0 )
            send_to_char( "You smell poisonous fumes.\n\r", ch );
        else
            send_to_char( "It looks delicious.\n\r", ch );
    }
    else
    {
        send_to_char( "It doesn't look poisoned.\n\r", ch );
    }

    return TRUE;
}

bool spell_dispel_evil( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    if ( !IS_NPC(ch) && IS_TRUE_EVIL(ch) )
        victim = ch;

    if ( IS_TRUE_GOOD(victim) )
    {
        act( "The gods protect $N.", ch, NULL, victim, TO_ROOM );
        return TRUE;
    }

    if ( IS_TRUE_NEUTRAL(victim) )
    {
        act( "$N does not seem to be affected.", ch, NULL, victim, TO_CHAR );
        return TRUE;
    }

    if (victim->hit > (ch->level * 4))
      dam = dice( level, 4 );
    else
      dam = UMAX(victim->hit, dice(level,4));
    if ( saves_spell( level, ch, victim,DAM_HOLY) )
        dam /= 2;
    damage_old( ch, victim, dam, sn, DAM_HOLY ,TRUE);
    return TRUE;
}

bool spell_dispel_good( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    if ( !IS_NPC(ch) && IS_TRUE_GOOD(ch) )
        victim = ch;

    if ( IS_TRUE_EVIL(victim) )
    {
        act( "$N is protected by $S evil.", ch, NULL, victim, TO_ROOM );
        return TRUE;
    }

    if ( IS_TRUE_NEUTRAL(victim) )
    {
        act( "$N does not seem to be affected.", ch, NULL, victim, TO_CHAR );
        return TRUE;
    }

    if (victim->hit > (ch->level * 4))
      dam = dice( level, 4 );
    else
      dam = UMAX(victim->hit, dice(level,4));
    if ( saves_spell( level, ch, victim,DAM_NEGATIVE) )
        dam /= 2;
    damage_old( ch, victim, dam, sn, DAM_NEGATIVE ,TRUE);
    return TRUE;
}

bool spell_dispel_magic( int sn, int level, CHAR_DATA *ch, void *vo,int target ){
   CHAR_DATA *victim = (CHAR_DATA *) vo;
    bool found = FALSE;
    int dam;


    if (check_dispel(level +10, victim, skill_lookup("backfire")))
        {
        act("$n's backfire explodes in $N's face!", victim,NULL,ch,TO_ROOM);
        dam = number_range(level-10, level+10 );
        if ( saves_spell( level, NULL, ch,DAM_ENERGY) )
                dam /= 2;
        damage_old( ch,ch, dam, sn, DAM_ENERGY ,TRUE);
        found = TRUE;
        return TRUE;
        }
    if (check_dispel(level, victim, gsn_blight))
    {
        found = TRUE;
        act("$n appears less haggard.",victim,NULL,NULL,TO_ROOM);
    }

    if (check_dispel(level, victim, gsn_infernofury))
    {
	found = TRUE;
	act("$n's fury subsides.", victim, NULL, NULL, TO_ROOM);
    }

    if (check_dispel(level, victim, gsn_protnormalmissiles))
    {
        found = TRUE;
        act("$n is no longer protected from missiles.", victim, NULL, NULL, TO_ROOM);
    }

    if (check_dispel(level, victim, gsn_petrify))
    {
        found = TRUE;
        act("$n appears able to move more freely.", victim, NULL, NULL, TO_CHAR);
    }

    if (check_dispel(level, victim, gsn_windwall))
        {
        found = TRUE;
        act("$n's wind wall disappears.", victim, NULL, NULL, TO_ROOM);
        act("Your wind wall disappears.", victim, NULL, NULL, TO_CHAR);
        REMOVE_BIT(victim->act, PLR_FREEZE);
        }

    if (check_dispel(level, victim, gsn_stabilize))
        {
        found = TRUE;
        act("$n's link to the ground dissolves.", victim, NULL, NULL, TO_ROOM);
        act("Your link to the ground dissolves.", victim, NULL, NULL, TO_CHAR);
        REMOVE_BIT(victim->act, PLR_FREEZE);
        }

    if (check_dispel(level, victim, gsn_fleshtostone))
        {
        found = TRUE;
        act("$n's stone encasing crumbles.", victim, NULL, NULL, TO_ROOM);
        act("Your stone encasing crumbles.", victim, NULL, NULL, TO_CHAR);
        REMOVE_BIT(victim->act, PLR_FREEZE);
        }

    if (check_dispel(level, victim, gsn_encase))
        {
        found = TRUE;
        act("The ice encasing $n melts away.", victim, NULL, NULL, TO_ROOM);
        act("The ice encasing you melts away.", victim, NULL, NULL, TO_CHAR);
        REMOVE_BIT(victim->act, PLR_FREEZE);
    }

    if (check_dispel(level, victim, gsn_stoneshell))
        {
        found = TRUE;
        act("Your stone shell crumbles.", victim, NULL, NULL, TO_CHAR);
        }

    if (check_dispel(level, victim, gsn_nightfears))
        {
        act("$n's fears of the night dissipates.", victim, NULL, NULL, TO_ROOM);        found = TRUE;
        }

    if (check_dispel(level, victim, gsn_powerwordfear))
        {
        act("$n's terror dissipates.", victim, NULL, NULL, TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level, victim, gsn_possession))
        {
        act("$n shakes $s head as $e regains control of $s own body.", victim,
NULL, NULL, TO_ROOM);
        act("Your spell of possession shattered, your spirit retreats to your own body.", victim, NULL, NULL, TO_CHAR);
                    if (ch->prompt != NULL)
                        {
                        free_string(ch->prompt);
                        ch->prompt = NULL;
                        }

                act("$n shakes $s head clear as $s spirit returns.", victim->desc->original, NULL, NULL, TO_ROOM);

                WAIT_STATE(victim->desc->original, UMAX(victim->desc->original->wait, 5*PULSE_VIOLENCE));
                victim->desc->original->hit /= 2;
                if (victim->desc != NULL)
                   {
                        victim->desc->character       = victim->desc->original;
                        victim->desc->original        = NULL;
                        victim->desc->character->desc = victim->desc;
                        victim->desc                  = NULL;
                   }
        found = TRUE;
        }


    if (check_dispel(level, victim, gsn_agony))
        {
        act("$n relaxes as $s agony fades.", victim, NULL, NULL, TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level, victim, gsn_protectionfromfire))
        {
        act("$n's aura of cold disappears.", victim, NULL, NULL, TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level, victim, gsn_cloakofthevoid))
        {
        act("$n shivers as $s protection from the void slips away.", victim, NULL, NULL, TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level, victim,gsn_mindshell))
        {
        act("$n's mental barrier fades away.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level,victim,gsn_subdue))
        {
        act("$n looks more capable of violence.",
                victim,NULL,NULL,TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level,victim,gsn_paradise))
        {
        act("$n looks more aware of their surroundings.",
                victim,NULL,NULL,TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level,victim,gsn_sustenance))
        {
        act("$n looks hungrier.", victim,NULL,NULL,TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level,victim,gsn_manabarbs))
        {
        act("$n looks more comfortable with $s magic.",
                victim,NULL,NULL,TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level,victim,gsn_waterbreathing))
        {
        act("$n becomes unable to breathe underwater.", victim,NULL,NULL,TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level,victim,gsn_shatter))
        {
        act("$n's skin stops glittering.", victim,NULL,NULL,TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level,victim,gsn_earthbind))
        {
        act("$n looks lighter.", victim,NULL,NULL,TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level,victim,gsn_regeneration))
        {
        act("$n stops regenerating so quickly.",
                victim,NULL,NULL,TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level,victim,gsn_armor))
        {
        act("$n looks less protected.", victim,NULL,NULL,TO_ROOM);
        found = TRUE;
        }
    if (check_dispel(level,victim,gsn_bless))
        {
        act("$n looks less blessed.", victim,NULL,NULL,TO_ROOM);
        found = TRUE;
        }
    if (check_dispel(level, victim, gsn_immolation))
    {
        found = TRUE;
        act("$n no longer appears so enflamed...", victim,NULL,NULL,TO_ROOM);
    }

    if (check_dispel(level,victim, gsn_calm))
    {
        found = TRUE;
        act("$n no longer looks so peaceful...",victim,NULL,NULL,TO_ROOM);
    }

    if (check_dispel(level,victim,gsn_change_sex))
    {
        found = TRUE;
        act("$n looks more like $mself again.",victim,NULL,NULL,TO_ROOM);
    }

    if (check_dispel(level,victim,gsn_charm_person))
    {
        found = TRUE;
        act("$n regains $s free will.",victim,NULL,NULL,TO_ROOM);
    }

    if (check_dispel(level,victim,gsn_enervatingray))
    {
        found = TRUE;
        act("$n looks warmer.",victim,NULL,NULL,TO_ROOM);
    }

    if (check_dispel(level,victim,gsn_curse))
        {
        act("$n looks more comfortable.", victim,NULL,NULL,TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level,victim,skill_lookup("detect evil")))
        found = TRUE;

    if (check_dispel(level,victim,skill_lookup("detect good")))
        found = TRUE;

    if (check_dispel(level,victim,gsn_detect_invis))
        found = TRUE;

    if (check_dispel(level,victim,skill_lookup("detect magic")))
        found = TRUE;

    if (check_dispel(level,victim,gsn_coronalglow))
    {
        act("$n's pink outline fades.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }

    if (check_dispel(level,victim,gsn_fly))
    {
        act("$n falls to the ground!",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }
 
    if (check_dispel(level,victim,gsn_frenzy))
    {
        act("$n no longer looks so wild.",victim,NULL,NULL,TO_ROOM);;
        found = TRUE;
    }

    if (check_dispel(level,victim,gsn_giantstrength))
    {
        act("$n no longer looks so mighty.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }

    if (check_dispel(level,victim,gsn_haste))
    {
        act("$n is no longer moving so quickly.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }

    if (check_dispel(level,victim,gsn_infravision))
        found = TRUE;

    if (check_dispel(level,victim,gsn_invis))
    {
        act("$n fades into existence.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }

    if (check_dispel(level,victim,gsn_mass_invis))
    {
        act("$n fades into existence.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }

    if (check_dispel(level,victim,gsn_passdoor))
        {
        act("$n looks more solid.", victim,NULL,NULL,TO_ROOM);
        found = TRUE;
        }

    if (check_dispel(level,victim,skill_lookup("protection evil")))
        found = TRUE;

    if (check_dispel(level,victim,skill_lookup("protection good")))
        found = TRUE;

    if (check_dispel(level,victim,gsn_sanctuary))
    {
        act("The white aura around $n's body vanishes.",
            victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }

    if (IS_AFFECTED(victim,AFF_SANCTUARY)
        && !saves_dispel(level, victim->level,-1)
        && !is_affected(victim,skill_lookup("sanctuary")))
    {
        REMOVE_BIT(victim->affected_by,AFF_SANCTUARY);
        act("The white aura around $n's body vanishes.",
            victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }

    if (check_dispel(level,victim,gsn_shield))
    {
        act("The shield protecting $n vanishes.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }

    if (check_dispel(level,victim,gsn_sleep))
        found = TRUE;

    if (check_dispel(level,victim,gsn_slow))
    {
        act("$n is no longer moving so slowly.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }

    if (check_dispel(level,victim,gsn_stoneskin))
    {
        act("$n's skin regains its normal texture.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }

    if (check_dispel(level,victim,gsn_weaken))
    {
        act("$n looks stronger.",victim,NULL,NULL,TO_ROOM);
        found = TRUE;
    }

    if (found)
        send_to_char("Ok.\n\r",ch);
    else
        send_to_char("Spell failed.\n\r",ch);
        return TRUE;
}

bool spell_enchant_armor( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    AFFECT_DATA *paf;
    int result, fail;
    int ac_bonus, added;
    bool ac_found = FALSE;

    if (obj->item_type != ITEM_ARMOR)
    {
        send_to_char("That isn't an armor.\n\r",ch);
        return FALSE;
    }

//    if (obj->wear_loc != -1)

    if (obj->worn_on)
    {
        send_to_char("The item must be carried to be enchanted.\n\r",ch);
        return TRUE;
    }

    /* this means they have no bonus */
    ac_bonus = 0;
    fail = 25;  /* base 25% chance of failure */

    /* find the bonuses */

    if (!obj->enchanted)
        for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
        {
            if ( paf->location == APPLY_AC )
            {
                ac_bonus = paf->modifier;
                ac_found = TRUE;
                fail += 5 * (ac_bonus * ac_bonus);
            }

            else  /* things get a little harder */
                fail += 20;
        }

    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
        if ( paf->location == APPLY_AC )
        {
            ac_bonus = paf->modifier;
            ac_found = TRUE;
            fail += 5 * (ac_bonus * ac_bonus);
        }

        else /* things get a little harder */
            fail += 20;
    }

    /* apply other modifiers */
    fail -= level;

    if (IS_OBJ_STAT(obj,ITEM_BLESS))
        fail -= 15;
    if (IS_OBJ_STAT(obj,ITEM_GLOW))
        fail -= 5;

    fail = URANGE(5,fail,85);

    result = number_percent();

    /* the moment of truth */
    if (result < (fail / 5))  /* item destroyed */
    {
        act("$p flares blindingly... and evaporates!",ch,obj,NULL,TO_CHAR);
        act("$p flares blindingly... and evaporates!",ch,obj,NULL,TO_ROOM);
        extract_obj(obj);
        return TRUE;
    }

    if (result < (fail / 3)) /* item disenchanted */
    {
        AFFECT_DATA *paf_next;

        act("$p glows brightly, then fades...oops.",ch,obj,NULL,TO_CHAR);
        act("$p glows brightly, then fades.",ch,obj,NULL,TO_ROOM);
        obj->enchanted = TRUE;

        /* remove all affects */
        for (paf = obj->affected; paf != NULL; paf = paf_next)
        {
            paf_next = paf->next;
            free_affect(paf);
        }
        obj->affected = NULL;

        /* clear all flags */
        obj->extra_flags[0] = 0;
	obj->extra_flags[1] = 0;
        return TRUE;
    }

    if ( result <= fail )  /* failed, no bad result */
    {
        send_to_char("Nothing seemed to happen.\n\r",ch);
        return TRUE;
    }

    /* okay, move all the old flags into new vectors if we have to */
    if (!obj->enchanted)
    {
        AFFECT_DATA *af_new;
        obj->enchanted = TRUE;

        for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
        {
            af_new = new_affect();

            af_new->next = obj->affected;
            obj->affected = af_new;

            af_new->where       = paf->where;
            af_new->type        = UMAX(0,paf->type);
            af_new->level       = paf->level;
            af_new->duration    = paf->duration;
            af_new->location    = paf->location;
            af_new->modifier    = paf->modifier;
            af_new->bitvector   = paf->bitvector;
        }
    }

    if (result <= (90 - level/5))  /* success! */
    {
        act("$p shimmers with a gold aura.",ch,obj,NULL,TO_CHAR);
        act("$p shimmers with a gold aura.",ch,obj,NULL,TO_ROOM);
        SET_BIT(obj->extra_flags[0], ITEM_MAGIC);
        added = -1;
    }

    else  /* exceptional enchant */
    {
        act("$p glows a brillant gold!",ch,obj,NULL,TO_CHAR);
        act("$p glows a brillant gold!",ch,obj,NULL,TO_ROOM);
        SET_BIT(obj->extra_flags[0],ITEM_MAGIC);
        SET_BIT(obj->extra_flags[0],ITEM_GLOW);
        added = -2;
    }

    /* now add the enchantments */

    if (obj->level < LEVEL_HERO)
        obj->level = UMIN(LEVEL_HERO - 1,obj->level + 1);

    if (ac_found)
    {
        for ( paf = obj->affected; paf != NULL; paf = paf->next)
        {
            if ( paf->location == APPLY_AC)
            {
                paf->type = sn;
                paf->modifier += added;
                paf->level = UMAX(paf->level,level);
            }
        }
    }
    else /* add a new affect */
    {
        paf = new_affect();

        paf->where      = TO_OBJECT;
        paf->type       = sn;
        paf->level      = level;
        paf->duration   = -1;
        paf->location   = APPLY_AC;
        paf->modifier   =  added;
        paf->bitvector  = 0;
        paf->next       = obj->affected;
        obj->affected   = paf;
    }
    return TRUE;
}


bool spell_enchant_weapon(int sn,int level,CHAR_DATA *ch, void *vo,int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    AFFECT_DATA *paf;
    int result, fail;
    int hit_bonus, dam_bonus, added;
    bool hit_found = FALSE, dam_found = FALSE;

    if (obj->item_type != ITEM_WEAPON)
    {
        send_to_char("That isn't a weapon.\n\r",ch);
        return FALSE;
    }

//    if (obj->wear_loc != -1)

    if (obj->worn_on)
    {
        send_to_char("The item must be carried to be enchanted.\n\r",ch);
        return FALSE;
    }

    /* this means they have no bonus */
    hit_bonus = 0;
    dam_bonus = 0;
    fail = 25;  /* base 25% chance of failure */

    /* find the bonuses */

    if (!obj->enchanted)
        for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
        {
            if ( paf->location == APPLY_HITROLL )
            {
                hit_bonus = paf->modifier;
                hit_found = TRUE;
                fail += 2 * (hit_bonus * hit_bonus);
            }

            else if (paf->location == APPLY_DAMROLL )
            {
                dam_bonus = paf->modifier;
                dam_found = TRUE;
                fail += 2 * (dam_bonus * dam_bonus);
            }

            else  /* things get a little harder */
                fail += 25;
        }

    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
        if ( paf->location == APPLY_HITROLL )
        {
            hit_bonus = paf->modifier;
            hit_found = TRUE;
            fail += 2 * (hit_bonus * hit_bonus);
        }

        else if (paf->location == APPLY_DAMROLL )
        {
            dam_bonus = paf->modifier;
            dam_found = TRUE;
            fail += 2 * (dam_bonus * dam_bonus);
        }

        else /* things get a little harder */
            fail += 25;
    }

    /* apply other modifiers */
    fail -= 3 * level/2;

    if (IS_OBJ_STAT(obj,ITEM_BLESS))
        fail -= 15;
    if (IS_OBJ_STAT(obj,ITEM_GLOW))
        fail -= 5;

    fail = URANGE(5,fail,95);

    result = number_percent();

    /* the moment of truth */
    if (result < (fail / 5))  /* item destroyed */
    {
        act("$p shivers violently and explodes!",ch,obj,NULL,TO_CHAR);
        act("$p shivers violently and explodes!",ch,obj,NULL,TO_ROOM);
        extract_obj(obj);
        return TRUE;
    }

    if (result < (fail / 2)) /* item disenchanted */
    {
        AFFECT_DATA *paf_next;

        act("$p glows brightly, then fades...oops.",ch,obj,NULL,TO_CHAR);
        act("$p glows brightly, then fades.",ch,obj,NULL,TO_ROOM);
        obj->enchanted = TRUE;

        /* remove all affects */
        for (paf = obj->affected; paf != NULL; paf = paf_next)
        {
            paf_next = paf->next;
            free_affect(paf);
        }
        obj->affected = NULL;

        /* clear all flags */
// brazen: Ticket: 126. this clears many flags it shouldn't.
//      obj->extra_flags[0] = 0;
//	obj->extra_flags[1] = 0;
	if(IS_SET(obj->extra_flags[0], ITEM_GLOW))
	    REMOVE_BIT(obj->extra_flags[0], ITEM_GLOW);
	if(IS_SET(obj->extra_flags[0], ITEM_HUM))
	    REMOVE_BIT(obj->extra_flags[0], ITEM_HUM);
	if(IS_SET(obj->extra_flags[0], ITEM_INVIS))
	    REMOVE_BIT(obj->extra_flags[0], ITEM_INVIS);
	if(IS_SET(obj->extra_flags[0], ITEM_MAGIC))
	    REMOVE_BIT(obj->extra_flags[0], ITEM_MAGIC);
	if(!IS_SET(obj->extra_flags[0], ITEM_NOUNCURSE))
	    if(IS_SET(obj->extra_flags[0], ITEM_NODROP))
	        REMOVE_BIT(obj->extra_flags[0], ITEM_NODROP);
	    if(IS_SET(obj->extra_flags[0], ITEM_NOREMOVE))
	        REMOVE_BIT(obj->extra_flags[0], ITEM_NOREMOVE);
	if(IS_SET(obj->extra_flags[0], ITEM_NODISARM))
	    REMOVE_BIT(obj->extra_flags[0], ITEM_NODISARM);
	if(IS_SET(obj->extra_flags[0], ITEM_EVIL))
	    REMOVE_BIT(obj->extra_flags[0], ITEM_EVIL);
	if(IS_SET(obj->extra_flags[0], ITEM_BLESS))
	    REMOVE_BIT(obj->extra_flags[0], ITEM_BLESS);
        if(IS_SET(obj->extra_flags[0], ITEM_DARK))
	    REMOVE_BIT(obj->extra_flags[0], ITEM_DARK);
        return TRUE;
    }

    if ( result <= fail )  /* failed, no bad result */
    {
        send_to_char("Nothing seemed to happen.\n\r",ch);
        return TRUE;
    }

    /* okay, move all the old flags into new vectors if we have to */
    if (!obj->enchanted)
    {
        AFFECT_DATA *af_new;
        obj->enchanted = TRUE;

        for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
        {
            af_new = new_affect();

            af_new->next = obj->affected;
            obj->affected = af_new;

            af_new->where       = paf->where;
            af_new->type        = UMAX(0,paf->type);
            af_new->level       = paf->level;
            af_new->duration    = paf->duration;
            af_new->location    = paf->location;
            af_new->modifier    = paf->modifier;
            af_new->bitvector   = paf->bitvector;
        }
    }

    if (result <= (100 - level/5))  /* success! */
    {
        act("$p glows blue.",ch,obj,NULL,TO_CHAR);
        act("$p glows blue.",ch,obj,NULL,TO_ROOM);
        SET_BIT(obj->extra_flags[0], ITEM_MAGIC);
        added = 1;
    }

    else  /* exceptional enchant */
    {
        act("$p glows a brillant blue!",ch,obj,NULL,TO_CHAR);
        act("$p glows a brillant blue!",ch,obj,NULL,TO_ROOM);
        SET_BIT(obj->extra_flags[0],ITEM_MAGIC);
        SET_BIT(obj->extra_flags[0],ITEM_GLOW);
        added = 2;
    }

    /* now add the enchantments */

    if (obj->level < LEVEL_HERO - 1)
        obj->level = UMIN(LEVEL_HERO - 1,obj->level + 1);

    if (dam_found)
    {
        for ( paf = obj->affected; paf != NULL; paf = paf->next)
        {
            if ( paf->location == APPLY_DAMROLL)
            {
                paf->type = sn;
                paf->modifier += added;
                paf->level = UMAX(paf->level,level);
                if (paf->modifier > 4)
                    SET_BIT(obj->extra_flags[0],ITEM_HUM);
            }
        }
    }
    else /* add a new affect */
    {
        paf = new_affect();

        paf->where      = TO_OBJECT;
        paf->type       = sn;
        paf->level      = level;
        paf->duration   = -1;
        paf->location   = APPLY_DAMROLL;
        paf->modifier   =  added;
        paf->bitvector  = 0;
        paf->next       = obj->affected;
        obj->affected   = paf;
    }

    if (hit_found)
    {
        for ( paf = obj->affected; paf != NULL; paf = paf->next)
        {
            if ( paf->location == APPLY_HITROLL)
            {
                paf->type = sn;
                paf->modifier += added;
                paf->level = UMAX(paf->level,level);
                if (paf->modifier > 4)
                    SET_BIT(obj->extra_flags[0],ITEM_HUM);
            }
        }
    }
    else /* add a new affect */
    {
        paf = new_affect();

        paf->type       = sn;
        paf->level      = level;
        paf->duration   = -1;
        paf->location   = APPLY_HITROLL;
        paf->modifier   =  added;
        paf->bitvector  = 0;
        paf->next       = obj->affected;
        obj->affected   = paf;
    }
    return TRUE;
}

/*
 * Drain XP, MANA, HP.
 * Caster gains HP.
 */
bool spell_energy_drain( int sn, int level, CHAR_DATA *ch, void *vo,int target ){
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;


    if ( saves_spell( level, ch, victim,DAM_NEGATIVE) )
    {
        send_to_char("You feel a momentary chill.\n\r",victim);
        return TRUE;
    }


    if ( victim->level <= 2 )
    {
        dam              = ch->hit + 1;
    }
    else
    {
        gain_exp( victim, 0 - number_range( level/2, 3 * level / 2 ) );
        victim->mana    /= 2;
        victim->move    /= 2;
        dam              = dice(1, level);
        ch->hit         += dam;
    }

    act("Icy tendrils leech strength from $N!", ch, NULL, victim, TO_NOTVICT);
    act("Icy tendrils leech strength from $N, filling you with $S life force!",
ch, NULL, victim, TO_CHAR);
    act("Icy tendrils leech strength from you!", ch, NULL, victim, TO_VICT);
    damage_old( ch, victim, dam, sn, DAM_NEGATIVE ,TRUE);

    return TRUE;
}

bool spell_eyes_hunter(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, gsn_eyes_hunter))
    {
	send_to_char("You already see the world through the eyes of the hunter.\n\r", ch);
	return FALSE;
    }

    af.where     = TO_PAFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level * 2 / 5;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = AFF_SHARP_VISION;
    affect_to_char(ch, &af);

    af.where     = TO_AFFECTS;
    af.bitvector = AFF_DETECT_HIDDEN;
    affect_to_char(ch, &af);

    af.bitvector = AFF_INFRARED;
    affect_to_char(ch, &af);

    send_to_char("Your vision sharpens, as you begin to perceive your surroundings through the eyes of a hunter.\n\r", ch);
    act("$n looks around sharply, a wary look in $s eyes.", ch, NULL, NULL, TO_ROOM);

    return TRUE;
}

bool spell_fever( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(victim, gsn_fever))
        {
        send_to_char ("They are already feverish.\n\r", ch);
        return FALSE;
        }

    if (saves_spell(level,ch, victim,DAM_DISEASE) ||
        (IS_NPC(victim) && IS_SET(victim->act,ACT_UNDEAD)))
    {
        if (ch == victim)
          send_to_char("You feel slightly feverish, but it passes.\n\r",
                ch);
        else
          act("$N seems to be unaffected.",ch,NULL,victim,TO_CHAR);
        return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type       = sn;
    af.level      = level * 3/4;
    af.duration  = level/2;
    af.location  = APPLY_STR;
    af.modifier  = (-1) * level/6;
    af.bitvector = AFF_PLAGUE;
    affect_join(victim,&af);

    send_to_char("You feel feverish and faint.\n\r",victim);
    act("$n looks pale and feverish.", victim,NULL,NULL,TO_ROOM);
    return TRUE;
}

bool spell_fire_breath( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    CHAR_DATA *vch, *vch_next;
    int dam,hp_dam,dice_dam;
    int hpch;

    act("$n breathes forth a cone of fire.",ch,NULL,victim,TO_NOTVICT);
    act("$n breathes a cone of hot fire over you!",ch,NULL,victim,TO_VICT);
    act("You breath forth a cone of fire.",ch,NULL,NULL,TO_CHAR);

    hpch = URANGE(10,ch->hit,1600);
    hp_dam  = number_range( hpch/9+1, hpch/5 );
    dice_dam = dice(level,20);

    dam = UMAX(hp_dam + dice_dam /10, dice_dam + hp_dam / 10);
    fire_effect(victim->in_room,level,dam/2,TARGET_ROOM);
    int odam = dam;
    for (vch = victim->in_room->people; vch != NULL; vch = vch_next)
    {
        vch_next = vch->next_in_room;

        if (is_safe_spell(ch,vch,TRUE)
        ||  (IS_NPC(vch) && IS_NPC(ch)
        &&   (ch->fighting != vch || vch->fighting != ch)))
            continue;
	
	dam = odam;
	if (check_defensiveroll(vch))
	{
	    dam /= 2;
	    send_to_char("You narrowly avoid the full blast of fire.\n\r",vch);
	}
	
        if (vch == victim) /* full damage */
        {
            if (saves_spell(level,ch, vch,DAM_FIRE))
            {
                fire_effect(vch,level/2,dam/4,TARGET_CHAR);
                damage_old(ch,vch,dam/2,sn,DAM_FIRE,TRUE);
            }
            else
            {
                fire_effect(vch,level,dam,TARGET_CHAR);
                damage_old(ch,vch,dam,sn,DAM_FIRE,TRUE);
            }
        }
        else /* partial damage */
        {
            if (saves_spell(level - 2,ch, vch,DAM_FIRE))
            {
                fire_effect(vch,level/4,dam/8,TARGET_CHAR);
                damage_old(ch,vch,dam/4,sn,DAM_FIRE,TRUE);
            }
            else
            {
                fire_effect(vch,level/2,dam/4,TARGET_CHAR);
                damage_old(ch,vch,dam/2,sn,DAM_FIRE,TRUE);
            }
        }
    }
    return TRUE;
}


bool spell_fireproof(int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (IS_OBJ_STAT(obj,ITEM_BURN_PROOF))
    {
        act("$p is already protected from burning.",ch,obj,NULL,TO_CHAR);
        return FALSE;
    }

    af.where     = TO_OBJECT;
    af.type      = sn;
    af.level     = level;
    af.duration  = number_fuzzy(level / 4);
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = ITEM_BURN_PROOF;

    affect_to_obj(obj,&af);

    act("You protect $p from fire.",ch,obj,NULL,TO_CHAR);
    act("$p is surrounded by a protective aura.",ch,obj,NULL,TO_ROOM);
    return TRUE;
}

bool spell_form( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    send_to_char( "That's not a spell!\n\r", ch );
    return FALSE;
}

bool spell_focus( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    send_to_char( "That's not a spell!\n\r", ch);
    return FALSE;
}

bool spell_frost_breath( int sn, int level, CHAR_DATA *ch, void *vo,int target ){
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    CHAR_DATA *vch, *vch_next;
    int dam,hp_dam,dice_dam, hpch;

    act("$n breathes out a freezing cone of frost!",ch,NULL,victim,TO_NOTVICT);
    act("$n breathes a freezing cone of frost over you!",
        ch,NULL,victim,TO_VICT);
    act("You breath out a cone of frost.",ch,NULL,NULL,TO_CHAR);

    hpch = URANGE(12,ch->hit,2200);
    hp_dam = number_range(hpch/11 + 1, hpch/6);
    dice_dam = dice(level,16);

    dam = UMAX(hp_dam + dice_dam/10,dice_dam + hp_dam/10);
    cold_effect(victim->in_room,level,dam/2,TARGET_ROOM);
    int odam = dam;
    for (vch = victim->in_room->people; vch != NULL; vch = vch_next)
    {
        vch_next = vch->next_in_room;
	dam = odam;
        if (is_safe_spell(ch,vch,TRUE)
        ||  (IS_NPC(vch) && IS_NPC(ch)
        &&   (ch->fighting != vch || vch->fighting != ch)))
            continue;
    	if (check_defensiveroll(vch))
    	{
	    dam /= 2;
            send_to_char("You narrowly avoid the full blast of lightning.\n\r",vch);
        }

        if (vch == victim) /* full damage */
        {
            if (saves_spell(level,ch, vch,DAM_COLD))
            {
                cold_effect(vch,level/2,dam/4,TARGET_CHAR);
                damage_old(ch,vch,dam/2,sn,DAM_COLD,TRUE);
            }
            else
            {
                cold_effect(vch,level,dam,TARGET_CHAR);
                damage_old(ch,vch,dam,sn,DAM_COLD,TRUE);
            }
        }
        else
        {
            if (saves_spell(level - 2,ch, vch,DAM_COLD))
            {
                cold_effect(vch,level/4,dam/8,TARGET_CHAR);
                damage_old(ch,vch,dam/4,sn,DAM_COLD,TRUE);
            }
            else
            {
                cold_effect(vch,level/2,dam/4,TARGET_CHAR);
                damage_old(ch,vch,dam/2,sn,DAM_COLD,TRUE);
            }
        }
    }
    return TRUE;
}

bool spell_gas_breath( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int dam,hp_dam,dice_dam,hpch;

    act("$n breathes out a cloud of poisonous gas!",ch,NULL,NULL,TO_ROOM);
    act("You breath out a cloud of poisonous gas.",ch,NULL,NULL,TO_CHAR);

    hpch = URANGE(16,ch->hit,2500);
    hp_dam = number_range(hpch/15+1,8);
    dice_dam = dice(level,12);

    dam = UMAX(hp_dam + dice_dam/10,dice_dam + hp_dam/10);
    poison_effect(ch->in_room,level,dam,TARGET_ROOM);
    int odam = dam;
    for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
    {
        vch_next = vch->next_in_room;
	dam = odam;
        if (is_safe_spell(ch,vch,TRUE)
        ||  (IS_NPC(ch) && IS_NPC(vch)
        &&   (ch->fighting == vch || vch->fighting == ch)))
            continue;

    	if (check_defensiveroll(vch))
    	{
	    dam /= 2;
            send_to_char("You narrowly avoid the full blast of poison gas.\n\r",vch);
    	}
        if (saves_spell(level,ch, vch,DAM_POISON))
        {
            poison_effect(vch,level/2,dam/4,TARGET_CHAR);
            damage_old(ch,vch,dam/2,sn,DAM_POISON,TRUE);
        }
        else
        {
            poison_effect(vch,level,dam,TARGET_CHAR);
            damage_old(ch,vch,dam,sn,DAM_POISON,TRUE);
        }
    }
    return TRUE;
}

bool spell_general_purpose(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = number_range( 25, 100 );
    if ( saves_spell( level, ch, victim, DAM_PIERCE) )
        dam /= 2;
    damage_old( ch, victim, dam, sn, DAM_PIERCE ,TRUE);
    return TRUE;
}


bool spell_haste( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( is_affected( victim, sn ) || IS_AFFECTED(victim,AFF_HASTE)
    ||   IS_SET(victim->off_flags,OFF_FAST))
    {
        if (victim == ch)
          send_to_char("You can't move any faster!\n\r",ch);
        else
          act("$N is already moving as fast as $E can.",
              ch,NULL,victim,TO_CHAR);
        return FALSE;
    }

    if (IS_AFFECTED(victim,AFF_SLOW))
    {
        if (!check_dispel(level,victim,skill_lookup("slow")))
        {
            if (victim != ch)
                send_to_char("Spell failed.\n\r",ch);
            send_to_char("You feel momentarily faster.\n\r",victim);
            return TRUE;
        }
        act("$n is moving less slowly.",victim,NULL,NULL,TO_ROOM);
        return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    if (victim == ch)
      af.duration  = level/2;
    else
      af.duration  = level/4;
    af.location  = APPLY_DEX;
    af.modifier  = 1 + (level >= 18) + (level >= 25) + (level >= 32);
    af.bitvector = AFF_HASTE;
    affect_to_char( victim, &af );
    send_to_char( "You feel yourself moving more quickly.\n\r", victim );
    act("$n is moving more quickly.",victim,NULL,NULL,TO_ROOM);
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return TRUE;
}

bool spell_high_explosive(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = number_range( 30, 120 );
    if ( saves_spell( level, ch, victim, DAM_PIERCE) )
        dam /= 2;
    damage_old( ch, victim, dam, sn, DAM_PIERCE ,TRUE);
    return TRUE;
}

bool spell_holy_word(int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int dam;
    int bless_num, curse_num, frenzy_num;

    bless_num = skill_lookup("bless");
    curse_num = skill_lookup("curse");
    frenzy_num = skill_lookup("frenzy");

    act("$n utters a word of divine power!",ch,NULL,NULL,TO_ROOM);
    send_to_char("You utter a word of divine power.\n\r",ch);

    for ( vch = ch->in_room->people; vch != NULL; vch = vch_next )
    {
        vch_next = vch->next_in_room;

        if ((IS_TRUE_GOOD(ch) && IS_TRUE_GOOD(vch)) ||
            (IS_TRUE_EVIL(ch) && IS_TRUE_EVIL(vch)) ||
            (IS_TRUE_NEUTRAL(ch) && IS_TRUE_NEUTRAL(vch)) )
        {
          send_to_char("You feel full more powerful.\n\r",vch);
          spell_frenzy(frenzy_num,level,ch,(void *) vch,TARGET_CHAR);
          spell_bless(bless_num,level,ch,(void *) vch,TARGET_CHAR);
        }

        else if ((IS_TRUE_GOOD(ch) && IS_TRUE_EVIL(vch)) ||
                 (IS_TRUE_EVIL(ch) && IS_TRUE_GOOD(vch)) )
        {
          if (!is_safe_spell(ch,vch,TRUE))
          {
            spell_curse(curse_num,level,ch,(void *) vch,TARGET_CHAR);
            send_to_char("You are struck down!\n\r",vch);
            dam = dice(level,6);
            damage_old(ch,vch,dam,sn,DAM_ENERGY,TRUE);
          }
        }

        else if (IS_NEUTRAL(ch))
        {
          if (!is_safe_spell(ch,vch,TRUE))
          {
            spell_curse(curse_num,level/2,ch,(void *) vch,TARGET_CHAR);
            send_to_char("You are struck down!\n\r",vch);
            dam = dice(level,4);
            damage_old(ch,vch,dam,sn,DAM_ENERGY,TRUE);
          }
        }
    }

    send_to_char("You feel drained.\n\r",ch);
    ch->move = 0;
    ch->hit /= 2;
    return TRUE;
}

bool spell_ignite(int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

   if (is_affected(victim, gsn_ignite))
   {
        send_to_char("They are already covered in flame.\n\r", ch);
        return FALSE;
   }

   if (is_affected(victim, gsn_protectionfromfire))
   {
        send_to_char("They fail to catch on fire.\n\r", ch);
        return TRUE;
   }

   if (saves_spell(level, ch, victim, DAM_FIRE))
   {
        send_to_char("You douse the flames before they can spread.\n\r",victim);
        send_to_char("They fail to catch on fire.\n\r", ch);
        return TRUE;
   }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 3;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(victim,&af);

    act("Flames quickly spread across $N's body!", ch, NULL, victim, TO_VICTROOM);
    act("Flames quickly spread across your body!", victim, NULL,
NULL, TO_CHAR);
    act("Flames quickly spread across $N's body!", ch, NULL, victim, TO_CHAR);
    return TRUE;
}


bool spell_inferno( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = dice( level, 3 );
    if ( saves_spell( level, ch, victim, DAM_FIRE ) )
        dam /= 2;
    act ("$n unleashes a fiery inferno upon $N!", ch,NULL,victim,TO_ROOM);
    damage_old( ch, victim, dam, sn,DAM_FIRE,TRUE);
    return TRUE;
}


/* This spell was specifically designed to work as a sigil power. */
/* Results may very if used as a normal spell.  - Erinos	  */
bool spell_inspire( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *vch;
    bool gFound = FALSE;
    char buf[MAX_STRING_LENGTH];

    if (!ch->in_room)
        return FALSE;

    if (IS_OAFFECTED(ch, AFF_INSPIRE))
    {
        send_to_char("You already feel inspired.\n\r", ch);
        return FALSE;
    }

    if (ch->master)
    {
        send_to_char("You must be leading a group to inspire them to greatness.\n\r", ch);
        return FALSE;
    }

    if (!ch->fighting)
    {
        send_to_char("You must be in combat to give inspiration.\n\r", ch);
        return FALSE;
    }

    af.where     = TO_OAFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level / 3;
    af.bitvector = AFF_INSPIRE;

    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
        if (is_same_group(ch, vch) && (vch != ch) && can_see(ch, vch))
        {
            if (!gFound)
            {
                act("You gallantly charge $N, inspiring your group to greatness!", ch, NULL, ch->fighting, TO_CHAR);
                gFound = TRUE;

                af.location = APPLY_HITROLL;
                af.modifier = level / 10;
                affect_to_char(ch, &af);

                af.location = APPLY_SAVES;
                af.modifier = ((level / 20) + 1) * -1;
                affect_to_char(ch, &af);

                af.location = APPLY_HIT;
                af.modifier = level;
                affect_to_char(ch, &af);
		ch->hit += level;
            }

            sprintf(buf, "%s gallantly charges $N, inspiring you onwards!", (IS_NPC(ch) ? ch->short_descr : ch->name));
            act(buf, vch, NULL, ch->fighting, TO_CHAR);

            af.location = APPLY_HITROLL;
            af.modifier = level / 10;
            affect_to_char(vch, &af);

            af.location = APPLY_SAVES;
            af.modifier = ((level / 20) + 1) * -1;
            affect_to_char(vch, &af);

            af.location = APPLY_HIT;
            af.modifier = level;
            affect_to_char(vch, &af);
	    vch->hit += level;
        }

    if (gFound)
    {
        WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));
        for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
            if (!is_same_group(ch, vch))
                if (vch == ch->fighting)
                    act("$n gallantly charges at you, inspiring $s groupmates to greatness!", ch, NULL, vch, TO_VICT);
                else
                {
                    sprintf(buf, "%s gallantly charges at $N, inspiring his groupmates to greatness!", (IS_NPC(ch) ? ch->short_descr : ch->name));
                    act(buf, vch, NULL, ch->fighting, TO_CHAR);
                }
    }
    else
        send_to_char("You must be fighting alongside a group to instill inspiration!\n\r", ch);

    return TRUE;
}

bool spell_lang( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    send_to_char( "That's not a spell!\n\r", ch );
    return FALSE;
}


bool spell_lightning_breath(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam,hp_dam,dice_dam,hpch;

    act("$n breathes a bolt of lightning at $N.",ch,NULL,victim,TO_NOTVICT);
    act("$n breathes a bolt of lightning at you!",ch,NULL,victim,TO_VICT);
    act("You breathe a bolt of lightning at $N.",ch,NULL,victim,TO_CHAR);

    hpch = URANGE(10,ch->hit,1600);
    hp_dam = number_range(hpch/9+1,hpch/5);
    dice_dam = dice(level,20);

    dam = UMAX(hp_dam + dice_dam/10,dice_dam + hp_dam/10);

    if (check_defensiveroll(victim))
    {
	dam /= 2;
        send_to_char("You narrowly avoid the full blast of lightning.\n\r",victim);
    }
    if (saves_spell(level,ch, victim,DAM_LIGHTNING))
    {
        shock_effect(victim,level/2,dam/4,TARGET_CHAR);
        damage_old(ch,victim,dam/2,sn,DAM_LIGHTNING,TRUE);
    }
    else
    {
        shock_effect(victim,level,dam,TARGET_CHAR);
        damage_old(ch,victim,dam,sn,DAM_LIGHTNING,TRUE);
    }
    return TRUE;
}



bool spell_mass_healing(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *gch;
    int heal_num, refresh_num;

    heal_num = skill_lookup("heal");
    refresh_num = skill_lookup("refresh");

    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if ((IS_NPC(ch) && IS_NPC(gch)) ||
            (!IS_NPC(ch) && !IS_NPC(gch)))
        {
            spell_heal(heal_num,level,ch,(void *) gch,TARGET_CHAR);
            spell_refresh(refresh_num,level,ch,(void *) gch,TARGET_CHAR);
        }
    }
    return TRUE;
}

bool spell_mindshell( int sn, int level, CHAR_DATA *ch, void *vo,
        int target )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *cch;
    if ( is_affected( ch, sn ) )
        {
        send_to_char("Your thoughts are already protected.\n\r",ch);
        return FALSE;
        }
    if (cch=cloak_remove(ch))
    {
	send_to_char("You feel your cloak forced away.\n\r",cch);
	send_to_char("You realize that someone was obscured from your sight.\n\r",ch);
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/2;
    af.location  = APPLY_RESIST_MENTAL;
    af.modifier  = PERC_RES;
    af.bitvector = 0;
    affect_to_char( ch, &af );
    send_to_char( "Your mind feels protected.\n\r", ch );
    return TRUE;
}

bool spell_null( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    send_to_char( "That's not a spell!\n\r", ch );
    return FALSE;
}

bool spell_numb( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
/*    char buf[MAX_STRING_LENGTH];*/
    static const int dam_each[] =
    {
         0,
         3,  4,  6,  7,  8,      9, 12, 13, 13, 13,
        14, 14, 14, 15, 15,     15, 16, 16, 16, 17,
        17, 17, 18, 18, 18,     19, 19, 19, 20, 20,
        20, 21, 21, 21, 22,     22, 22, 23, 23, 23,
        24, 24, 24, 25, 25,     25, 26, 26, 26, 27
    };
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int dam;

    level       = UMIN(level, static_cast<int>(sizeof(dam_each)/sizeof(dam_each[0])) - 1);
    level       = UMAX(0, level);
    dam         = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    if ( !saves_spell( level, ch, victim,DAM_COLD ) )
    {
        act("$n shivers with numbness.",victim,NULL,NULL,TO_ROOM);
        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = level;
        af.duration  = 6;
        af.location  = APPLY_STR;
        af.modifier  = -1;
        af.bitvector = 0;
        affect_join( victim, &af );
    }
    else
    {
        dam /= 2;
    }

    damage_old( ch, victim, dam, sn, DAM_COLD,TRUE );
    return TRUE;
}


bool spell_obscurealign( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, gsn_obscurealign))
    {
        send_to_char("Your alignment is already magically obscured.\n\r", ch);
        return FALSE;
    }

    if (target_name[0] == '\0')
    {
        send_to_char("What do you wish to disguise your alignment as?\n\r", ch);
        return FALSE;
    }

    af.modifier = ch->alignment;

    switch (UPPER(target_name[0]))
    {
        case 'G':
           ch->alignment = 750;
           break;
        case 'N':
           ch->alignment = 0;
           break;
        case 'E':
           ch->alignment = -750;
           break;
        default:
           send_to_char("Invalid alignment.  Valid options are: good neutral evil.\n\r", ch);
           return FALSE;
           break;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/2;
    af.location  = APPLY_HIDE;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    send_to_char("A magical veil settles over your natural aura, obscuring your intentions.\n\r", ch);
    return TRUE;
}

bool spell_oldtempest( int sn, int level, CHAR_DATA *ch, void *vo,
        int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = dice( level+2, 9 );
    if ( saves_spell( level, ch, victim, DAM_ENERGY ) )
        dam /= 2;
    act ("$n unleashes a tempestuous storm upon $N!",
        ch,NULL,victim,TO_ROOM);
    damage_old( ch, victim, dam, sn,DAM_ENERGY,TRUE);
    return TRUE;
}


bool spell_paradise( int sn, int level, CHAR_DATA *ch, void *vo,
int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( is_affected( victim, sn ) )
    {
        if (victim == ch)
          send_to_char("You are already entranced by paradise.\n\r",ch);
        else
          act("$N already has visions of paradise.",
                ch,NULL,victim,TO_CHAR);
        return FALSE;
    }

    if ( saves_spell( level, ch, victim, DAM_MENTAL) )
        {
        send_to_char("Their mind resists your spell.\n\r", ch);
        return TRUE;
        }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level-5;
    af.duration  = level/10;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = 0;
    affect_to_char( victim, &af );
    send_to_char( "You are entranced by a glimpse of paradise.\n\r",
        victim );
    if ( ch != victim )
        act("$N is entranced by paradise.",ch,NULL,victim,TO_CHAR);
    return TRUE;
}

bool spell_protection_evil(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( IS_AFFECTED(victim, AFF_PROTECT_EVIL)
    ||   IS_AFFECTED(victim, AFF_PROTECT_GOOD))
    {
        if (victim == ch)
          send_to_char("You are already protected.\n\r",ch);
        else
          act("$N is already protected.",ch,NULL,victim,TO_CHAR);
        return FALSE;
    }
    if (IS_EVIL(victim))
    {
        send_to_char("Your own darkness prevents you from protecting yourself from evil.\n\r",ch);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 24;
    af.location  = APPLY_SAVING_SPELL;
    af.modifier  = -1;
    af.bitvector = AFF_PROTECT_EVIL;
    affect_to_char( victim, &af );
    send_to_char( "You feel holy and pure.\n\r", victim );
    if ( ch != victim )
        act("$N is protected from evil.",ch,NULL,victim,TO_CHAR);
    return TRUE;
}

bool spell_protectionfromcold(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
        send_to_char("You are already protected from cold.\n\r", ch);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 24;
    af.location  = APPLY_RESIST_COLD;
    af.modifier  = PERC_RES;
    af.bitvector = 0;
    affect_to_char( ch, &af );
    send_to_char( "You feel a warm aura around you.\n\r", ch );
    act("Warmth begins to emanate from $n.",ch,NULL,NULL,TO_ROOM);
    return TRUE;
}

bool spell_protection_good(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( IS_AFFECTED(victim, AFF_PROTECT_GOOD)
    ||   IS_AFFECTED(victim, AFF_PROTECT_EVIL))
    {
        if (victim == ch)
          send_to_char("You are already protected.\n\r",ch);
        else
          act("$N is already protected.",ch,NULL,victim,TO_CHAR);
        return FALSE;
    }
    if (IS_GOOD(victim))
    {
        send_to_char("Your own purity prevents you from protecting yourself from good.\n\r",ch);
        return FALSE;
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 24;
    af.location  = APPLY_SAVING_SPELL;
    af.modifier  = -1;
    af.bitvector = AFF_PROTECT_GOOD;
    affect_to_char( victim, &af );
    send_to_char( "You feel aligned with darkness.\n\r", victim );
    if ( ch != victim )
        act("$N is protected from good.",ch,NULL,victim,TO_CHAR);
    return TRUE;
}


bool spell_rayofpurity( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    act("You focus a ray of pure light directly at $N!", ch, NULL, victim, TO_CHAR);
    act("$n focuses a ray of pure light directly at $N!", ch, NULL, victim, TO_VICTROOM);
    act("$n focuses a ray of pure light directly at you!", ch, NULL, victim, TO_VICT);

    if (IS_TRUE_GOOD(victim))
    {
        act("$N seems unaffected by your ray of purity!", ch, NULL, victim, TO_CHAR);
        act("$N seems to be unaffected by $n's ray of purity!", ch, NULL, victim, TO_VICTROOM);
        act("You are unaffected by $n's ray of purity!", ch, NULL, victim, TO_VICT);
        return TRUE;
    }

    if (is_affected(victim, gsn_cloakofthevoid))
    {
        act("Your ray of purity sears away $N's cloak of the void!", ch, NULL, victim, TO_CHAR);
        act("$n's ray of purity sears away your cloak of the void!", ch, NULL, victim, TO_VICT);
        affect_strip(victim, gsn_cloakofthevoid);
    }

    dam = dice( level, 3 );
    if (IS_TRUE_EVIL(victim))
        dam = dam * 22 / 10;
    if ( saves_spell( level, ch, victim, DAM_LIGHT ) )
        dam /= 2;
    damage_old(ch, victim, dam, sn, DAM_LIGHT, TRUE);

    return TRUE;
}

bool spell_ray_of_truth (int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam, align;

    if (IS_TRUE_EVIL(ch) )
    {
        victim = ch;
        send_to_char("The energy explodes inside you!\n\r",ch);
    }

    if (victim != ch)
    {
        act("$n raises $s hand, and a blinding ray of light shoots forth!",
            ch,NULL,NULL,TO_ROOM);
        send_to_char(
           "You raise your hand and a blinding ray of light shoots forth!\n\r",
           ch);
    }

    if (IS_TRUE_GOOD(victim))
    {
        act("$n seems unharmed by the light.",victim,NULL,victim,TO_ROOM);
        send_to_char("The light seems powerless to affect you.\n\r",victim);
        return TRUE;
    }

    dam = dice( level, 10 );
    if ( saves_spell( level, ch, victim,DAM_HOLY) )
        dam /= 2;

    align = victim->alignment;
    align -= 350;

    if (align < -1000)
        align = -1000 + (align + 1000) / 3;

    dam = (dam * align * align) / 1000000;

    damage_old( ch, victim, dam, sn, DAM_HOLY ,TRUE);
    spell_soulflare(688, level - 2, ch, (void *) victim,TARGET_CHAR);
    return TRUE;
}

bool spell_recharge( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    int chance, percent;

    if (obj->item_type != ITEM_WAND && obj->item_type != ITEM_STAFF)
    {
        send_to_char("That item does not carry charges.\n\r",ch);
        return FALSE;
    }

    if (obj->value[3] >= 3 * level / 2)
    {
        send_to_char("Your skills are not great enough for that.\n\r",ch);
        return FALSE;
    }

    if (obj->value[1] == 0)
    {
        send_to_char("That item has already been recharged once.\n\r",ch);
        return FALSE;
    }

    chance = 40 + 2 * level;

    chance -= obj->value[3]; /* harder to do high-level spells */
    chance -= (obj->value[1] - obj->value[2]) *
              (obj->value[1] - obj->value[2]);

    chance = UMAX(level/2,chance);

    percent = number_percent();

    if (percent < chance / 2)
    {
        act("$p glows softly.",ch,obj,NULL,TO_CHAR);
        act("$p glows softly.",ch,obj,NULL,TO_ROOM);
        obj->value[2] = UMAX(obj->value[1],obj->value[2]);
        obj->value[1] = 0;
        return TRUE;
    }

    else if (percent <= chance)
    {
        int chargeback,chargemax;

        act("$p glows softly.",ch,obj,NULL,TO_CHAR);
        act("$p glows softly.",ch,obj,NULL,TO_CHAR);

        chargemax = obj->value[1] - obj->value[2];

        if (chargemax > 0)
            chargeback = UMAX(1,chargemax * percent / 100);
        else
            chargeback = 0;

        obj->value[2] += chargeback;
        obj->value[1] = 0;
        return TRUE;
    }

    else if (percent <= UMIN(95, 3 * chance / 2))
    {
        send_to_char("Nothing seems to happen.\n\r",ch);
        if (obj->value[1] > 1)
            obj->value[1]--;
        return TRUE;
    }

    else /* whoops! */
    {
        act("$p glows brightly and explodes!",ch,obj,NULL,TO_CHAR);
        act("$p glows brightly and explodes!",ch,obj,NULL,TO_ROOM);
        extract_obj(obj);
    }
    return TRUE;
}


bool spell_resist_poison(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
        send_to_char("You are already resistant to poison.\n\r", ch);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 24;
    af.location  = APPLY_RESIST_POISON;
    af.modifier  = PERC_RES;
    af.bitvector = 0;
    affect_to_char( ch, &af );
    send_to_char( "You feel resistant to poison.\n\r", ch );
    return TRUE;
}

bool spell_sandstorm_old( int sn, int level,CHAR_DATA *ch,void *vo,int target)
{
    char buf[MAX_STRING_LENGTH];
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

    level       = UMIN(level, static_cast<int>(sizeof(dam_each)/sizeof(dam_each[0])) - 1);
    level       = UMAX(0, level);

    send_to_char( "You summon a raging sandstorm!\n\r", ch );
    act( "$n summons a raging sandstorm!",
        ch, NULL, NULL, TO_ROOM );

    for ( vch = char_list; vch != NULL; vch = vch_next )
    {
        vch_next        = vch->next;
        if ( vch->in_room == NULL )
            continue;
        if ((vch->in_room == ch->in_room) && (ch!=vch) &&
                !(is_same_group(ch, vch)) && !is_safe_spell(ch,vch,TRUE))
        {
            if ((vch!=ch) && (!IS_IMMORTAL(vch) ||
               (IS_IMMORTAL(vch) && can_see(ch,vch))))
            {
                if ((ch->fighting != vch) && !IS_NPC(vch))
                {
                        if (can_see(vch, ch))
                        {
                        sprintf( buf, "Help!  %s is attacking me!",
                                PERS(ch, vch));
                        do_autoyell( vch, buf );
                        }
                        else
                        {
                        sprintf( buf, "Help! Someone is attacking me!");
                        do_autoyell( vch, buf );
                        }

                }
           dam          = number_range( dam_each[level] / 2, dam_each[level] * 2 );
                   if ( saves_spell( level, ch, vch, DAM_SLASH) )
                        dam /= 2;

                damage_old( ch, vch, saves_spell( level,ch, vch,DAM_SLASH)
                ? dam / 2 : dam, sn,DAM_SLASH,TRUE);
            }
            continue;
        }
    }

    return TRUE;
}

bool spell_scare( int sn, int level, CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (is_affected(victim, gsn_courage))
        level -= 5;

    if ( saves_spell( level, ch, victim,DAM_FEAR) )
    {
        send_to_char("They bravely continue fighting.\n\r",ch);
        return TRUE;
    }
    do_flee (victim, "");
    act("$n scares the wits out of $N!", ch, NULL, victim, TO_NOTVICT);
    act("$n scares the wits out of you!", ch, NULL, victim, TO_VICT);
    return TRUE;
}

bool spell_slow( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( is_affected( victim, sn ) || IS_AFFECTED(victim,AFF_SLOW))
    {
        if (victim == ch)
          send_to_char("You can't move any slower!\n\r",ch);
        else
          act("$N can't get any slower than that.",
              ch,NULL,victim,TO_CHAR);
        return FALSE;
    }

    if (saves_spell(level,ch, victim,DAM_OTHER)
    ||  IS_SET(victim->imm_flags,IMM_MAGIC))
    {
        if (victim != ch)
            send_to_char("Nothing seemed to happen.\n\r",ch);
        send_to_char("You feel momentarily lethargic.\n\r",victim);
        return TRUE;
    }

    if (IS_AFFECTED(victim,AFF_HASTE))
    {
        if (!check_dispel(level,victim,skill_lookup("haste")))
        {
            if (victim != ch)
                send_to_char("Spell failed.\n\r",ch);
            send_to_char("You feel momentarily slower.\n\r",victim);
            return TRUE;
        }

        act("$n is moving less quickly.",victim,NULL,NULL,TO_ROOM);
        return TRUE;
    }


    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/2;
    af.location  = APPLY_DEX;
    af.modifier  = -1 - (level >= 18) - (level >= 25) - (level >= 32);
    af.bitvector = AFF_SLOW;
    affect_to_char( victim, &af );
    send_to_char( "You feel yourself slow down.\n\r", victim );
    act("$n starts to move in slow motion.",victim,NULL,NULL,TO_ROOM);
    return TRUE;
}


bool spell_slow_cure_critical( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int heal, amtheal;

    if (is_an_avatar(victim))
    {
        send_to_char("An avatar cannot receive such magic.\n\r",ch);
	    return FALSE;
    }

    if (is_affected(victim,gsn_slow_cure_crit))
    {
	if (victim==ch)
	{
	    send_to_char("Your critical wounds are already slowly healing.\n\r",ch);
	    return FALSE;
	}
	else
	{
	    act("$N's critical wounds are already slowly healing.\n\r",ch,NULL,victim,TO_CHAR);
	    act("Your critical wounds are already slowly healing.\n\r",ch,NULL,victim,TO_VICT);
	}
    }

    heal = dice(3, 8) + level - 6;
    amtheal = heal / 3;
    victim->hit = UMIN( victim->hit + amtheal + (heal % 3), victim->max_hit);
    update_pos( victim );
    send_to_char("You feel better!\n\r", victim);
    if (ch != victim)
        send_to_char("Ok.\n\r", ch);

    af.where     = TO_AFFECTS;
    af.type      = gsn_slow_cure;
    af.level     = level;
    af.modifier  = amtheal;
    af.location  = APPLY_HIDE;
    af.duration  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    af.duration  = 1;
    affect_to_char(ch, &af);

    return TRUE;
}

bool spell_slow_cure_disease( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(victim,gsn_slow_cure_disease))
    {
	if (victim==ch)
	{
	    send_to_char("Your diseases are already being slowly cured.\n\r",ch);
	    return FALSE;
	}
	else
	{
	    act("$N's diseases are already being slowly cured.\n\r",ch,NULL,victim,TO_CHAR);
	    act("Your diseases are already being slowly cured.\n\r",ch,NULL,victim,TO_VICT);
	}
    }

    send_to_char("You feel a cleansing power begin to course through your body.\n\r", victim);
    if (victim != ch)
        send_to_char("Ok.\n\r", ch);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 3;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    return TRUE;
}


bool spell_slow_cure_light( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int heal, amtheal;

    if (is_an_avatar(victim))
    {
        send_to_char("An avatar cannot receive such magic.\n\r",ch);
	    return FALSE;
    }
    if (is_affected(victim,gsn_slow_cure))
    {
	if (victim==ch)
	{
	    send_to_char("Your light wounds are already slowly healing.\n\r",ch);
	    return FALSE;
	}
	else
	{
	    act("$N's light wounds are already slowly healing.\n\r",ch,NULL,victim,TO_CHAR);
	    act("Your light wounds are already slowly healing.\n\r",ch,NULL,victim,TO_VICT);
	}
    }

    heal = dice(1, 8) + level / 3;
    amtheal = heal / 3;
    victim->hit = UMIN( victim->hit + amtheal + (heal % 3), victim->max_hit);
    update_pos( victim );
    send_to_char("You feel better!\n\r", victim);
    if (ch != victim)
        send_to_char("Ok.\n\r", ch);

    af.where     = TO_AFFECTS;
    af.type      = gsn_slow_cure;
    af.level     = level;
    af.modifier  = amtheal;
    af.location  = APPLY_HIDE;
    af.duration  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    af.duration  = 1;
    affect_to_char(ch, &af);

    return TRUE;
}

bool spell_slow_cure_poison( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(victim,gsn_slow_cure_poison))
    {
	if (victim==ch)
	{
	    send_to_char("Your poison is already being slowly cured.\n\r",ch);
	    return FALSE;
	}
	else
	{
	    act("$N's poison is already being slowly cured.\n\r",ch,NULL,victim,TO_CHAR);
	    act("Your poison is already being slowly cured.\n\r",ch,NULL,victim,TO_VICT);
	}
    }

    send_to_char("You feel a cleansing power begin to course through your body.\n\r", victim);
    if (victim != ch)
        send_to_char("Ok.\n\r", ch);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 3;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    return TRUE;
}


bool spell_slow_cure_serious( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int heal, amtheal;

    if (is_an_avatar(victim))
    {
        send_to_char("An avatar cannot receive such magic.\n\r",ch);
	    return FALSE;
    }
    if (is_affected(victim,gsn_slow_cure_ser))
    {
	if (victim==ch)
	{
	    send_to_char("Your serious wounds are already slowly healing.\n\r",ch);
	    return FALSE;
	}
	else
	{
	    act("$N's serious wounds are already slowly healing.\n\r",ch,NULL,victim,TO_CHAR);
	    act("Your serious wounds are already slowly healing.\n\r",ch,NULL,victim,TO_VICT);
	}
    }

    heal = dice(2, 8) + level / 2;
    amtheal = heal / 3;
    victim->hit = UMIN( victim->hit + amtheal + (heal % 3), victim->max_hit);
    update_pos( victim );
    send_to_char("You feel better!\n\r", victim);
    if (ch != victim)
        send_to_char("Ok.\n\r", ch);

    af.where     = TO_AFFECTS;
    af.type      = gsn_slow_cure;
    af.level     = level;
    af.modifier  = amtheal;
    af.location  = APPLY_HIDE;
    af.duration  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    af.duration  = 1;
    affect_to_char(ch, &af);

    return TRUE;
}


bool spell_slow_heal( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_an_avatar(victim))
    {
        send_to_char("An avatar cannot receive such magic.\n\r",ch);
	    return FALSE;
    }

    if (is_affected(victim,gsn_slow_heal))
    {
	if (victim==ch)
	{
	    send_to_char("Your wounds are already healing slowly.\n\r",ch);
	    return FALSE;
	}
	else
	{
	    act("$N's wounds are already healing slowly.\n\r",ch,NULL,victim,TO_CHAR);
	    act("Your wounds are already healing slowly.\n\r",ch,NULL,victim,TO_VICT);
	}
    }

    victim->hit = UMIN( victim->hit + 34, victim->max_hit);
    update_pos( victim );
    send_to_char("You feel better!\n\r", victim);
    if (ch != victim)
        send_to_char("Ok.\n\r", ch);

    af.where     = TO_AFFECTS;
    af.type      = gsn_slow_cure;
    af.level     = level;
    af.modifier  = 33;
    af.location  = APPLY_HIDE;
    af.duration  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    af.duration  = 1;
    affect_to_char(ch, &af);

    return TRUE;
}

bool spell_song( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    send_to_char( "That's not a spell!\n\r", ch );
    return FALSE;
}


bool spell_sustenance( int sn, int level, CHAR_DATA *ch, void *vo,
        int target )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, gsn_sustenance))
        {
        send_to_char("Your magic already sustains you.\n\r", ch);
        return FALSE;
        }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level*3;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char( ch, &af );

    send_to_char("Your body feels sustained.\n\r", ch);

    return TRUE;
}

