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
*	ROM 2.4 is copyright 1993-1996 Russ Taylor			   *
*	ROM has been brought to you by the ROM consortium		   *
*	    Russ Taylor (rtaylor@efn.org)				   *
	    Gabrielle Taylor						   *
*	    Brian Moore (zump@rom.org)					   *
*	By using this code, you have agreed to follow the terms of the	   *
*	ROM license, in the file Rom24/doc/rom.license			   *
***************************************************************************/

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <string.h>
#if defined(unix)
#include <termios.h>
#include <unistd.h>
#endif
#include <string>
#include <time.h>
#include <cmath>
#include <sstream>
#include "merc.h"
#include "magic.h"
#include "lookup.h"
#include "demons.h"
#include "skills_chirurgeon.h"
#include "tables.h"
#include "recycle.h"
#include "spells_air.h"
#include "spells_fire.h"
#include "spells_fire_air.h"
#include "spells_spirit.h"
#include "spells_spirit_air.h"
#include "spells_earth.h"
#include "spells_earth_void.h"
#include "spells_void.h"
#include "PyreInfo.h"
#include "PhantasmInfo.h"
#include "ShadeControl.h"
#include "Weave.h"
#include "fight.h"
#include "interp.h"
#include "Luck.h"
#include "Faction.h"
#include "NameMaps.h"
#include "Runes.h"
#include "Drakes.h"
#include "Forge.h"
#include "ExperienceList.h"
#include "Oaths.h"

extern        void    append_note     args( ( NOTE_DATA *pnote ) );

/* command procedures needed */
DECLARE_DO_FUN(do_visible	);
DECLARE_DO_FUN(do_backstab	);
DECLARE_DO_FUN(do_emote		);
DECLARE_DO_FUN(do_berserk	);
DECLARE_DO_FUN(do_bash		);
DECLARE_DO_FUN(do_trip		);
DECLARE_DO_FUN(do_dirt		);
DECLARE_DO_FUN(do_tail		);
DECLARE_DO_FUN(do_bite		);
DECLARE_DO_FUN(do_bludgeon	);
DECLARE_DO_FUN(do_impale	);
DECLARE_DO_FUN(do_hamstring	);
DECLARE_DO_FUN(do_throw		);
DECLARE_DO_FUN(do_waylay	);
DECLARE_DO_FUN(do_murder	);
DECLARE_DO_FUN(do_druidtalk	);

DECLARE_DO_FUN(do_flee		);
DECLARE_DO_FUN(do_kick		);
DECLARE_DO_FUN(do_disarm	);
DECLARE_DO_FUN(do_get		);
DECLARE_DO_FUN(do_yell		);
DECLARE_DO_FUN(do_autoyell	);
DECLARE_DO_FUN(do_destroy	);
DECLARE_DO_FUN(do_shoot		);
DECLARE_DO_FUN(do_look		);

DECLARE_SPEC_FUN(spec_executioner);
extern CHAR_DATA *get_affinity_owner(OBJ_DATA *obj);
extern int hands_free(CHAR_DATA *ch);
extern void unbind_shunned_demon(CHAR_DATA *victim);
extern void desiccate_corpse args( (OBJ_DATA *corpse) );
extern void lunar_update_char args ((CHAR_DATA *ch) );
extern bool can_use_dual(CHAR_DATA *ch, OBJ_DATA *weapon, OBJ_DATA *dual);
extern void do_alchemy( CHAR_DATA *ch, char *argument, int alc_sn, int delay, double added_power);

static void do_rescue_helper(CHAR_DATA * ch, CHAR_DATA * victim);

void whinyassbitch(CHAR_DATA *ch, CHAR_DATA *victim, char *defname,int chance);
/*
 * Local functions.
 */
int 	mod_by_absorb(CHAR_DATA *ch);
int	mod_by_enthusiasm args( ( CHAR_DATA *ch));
void	check_enthusiasm args( ( CHAR_DATA *ch, CHAR_DATA *victim) );
bool	check_shamanicward args( ( CHAR_DATA *ch, CHAR_DATA *victim) );
OBJ_DATA *check_sidestep args( ( CHAR_DATA *ch, CHAR_DATA *victim) );
void	do_assume	args( ( CHAR_DATA *ch, char *argument ) );
void	do_savagery	args( ( CHAR_DATA *ch, char *argument ) );
void	do_eagle_jab	args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void	do_crab_disarm	args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool	check_lastword(CHAR_DATA *victim,CHAR_DATA *ch);
void	check_assist	args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool    check_creaturelore args((CHAR_DATA *ch, CHAR_DATA *victim) );
bool	check_dodge	args( ( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj ) );
bool    check_brawlingblock  args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool	check_evade	args( ( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj ) );
void	check_killer	args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool	check_divineshield args((CHAR_DATA *ch,CHAR_DATA *victim ) );
bool	check_parry	args((CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj));
bool	check_offhandparry	args((CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj ) );
bool    check_shield_block     args( ( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj, int dam ) );
void	check_shieldslam args( ( CHAR_DATA *ch,CHAR_DATA *victim) );
void	check_snakebite args( ( CHAR_DATA *ch,CHAR_DATA *victim) );
bool    check_fend      args( ( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj ) );
bool	check_stonephalanx args (( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj));
bool    check_deflect   args( ( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj ) );
bool	check_showmanship args( ( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj));
bool	check_savagery args( ( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj, int dam_type));
void    dam_message 	args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dam,
                            int dt, char *attack, bool immune ) );
void	death_cry	args( ( CHAR_DATA *ch, bool decorporealize ) );
bool	is_safe		args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void	make_corpse	args( ( CHAR_DATA *ch, CHAR_DATA *killer, bool decorporealize ) );
void    mob_hit		args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dt ) );
void	kill_char	args( ( CHAR_DATA *victim, CHAR_DATA *ch ) );
void	raw_kill	args( ( CHAR_DATA *victim ) );
void	set_fighting	args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool	disarm		args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );

int	num_engaged(CHAR_DATA *ch, bool ignoreCharmed);

bool 	damage_from_obj args( (CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj,
                     		int dam, int dt, int dam_type, bool show));

extern	void	mob_class_update	args((CHAR_DATA *ch));
void check_decapitate( CHAR_DATA *ch, CHAR_DATA *victim);
void shielddisarm( CHAR_DATA *ch, CHAR_DATA *victim );
bool check_defensiveroll(CHAR_DATA *victim);

bool	protect_works = TRUE;  /* damn that is_safe function is getting nuts */
bool	counter_works;  /* dittooo */
bool	global_counter_works = TRUE;

int	dual_damage = FALSE;

extern bool global_bool_check_avoid;
extern bool global_bool_final_valid;
extern bool global_bool_ranged_attack;

/* used to replace normal damage from */
extern char *   global_damage_from;
char *		global_damage_noun = NULL;
 
int* form_table[25] =
{ 
  &gsn_form_of_the_whirlwind,
  &gsn_form_of_the_hawk,
  &gsn_form_of_the_eagle,
  &gsn_form_of_the_mongoose,
  &gsn_form_of_the_panther,
  &gsn_form_of_the_viper,
  &gsn_form_of_the_monkey,
  &gsn_form_of_the_mockingbird,
  &gsn_form_of_the_cat,
  &gsn_form_of_the_bear,
  &gsn_form_of_the_wasp,
  &gsn_form_of_the_serpent,
  &gsn_form_of_the_dragon,
  &gsn_form_of_the_spider,
  &gsn_form_of_the_reed,
  &gsn_form_of_the_crab,
  &gsn_form_of_the_bull,
  &gsn_form_of_the_rose,
  &gsn_form_of_the_whirlwind,
  &gsn_form_of_the_asp,
  &gsn_form_of_the_winter_wind,
  &gsn_form_of_the_wraith,
  &gsn_form_of_the_living_seas,
  &gsn_form_of_the_asp,
  NULL
};

const	char	*obj_spell_dam[5] = { "freezing bite", "shock", "flaming strike", "divine power", "defilement" };


    

bool unassume_form(CHAR_DATA *ch)
{
    AFFECT_DATA *paf;
    char buf[MAX_STRING_LENGTH];

    for (paf = ch->affected; paf; paf = paf->next)
    {
	if (paf->location == APPLY_FORM)
	{
	    sprintf(buf, "You step out of the %s.\n\r", skill_table[paf->type].name);
	    send_to_char(buf, ch);
	    sprintf(buf, "$n steps out of the %s.\n\r", skill_table[paf->type].name);
	    act(buf, ch, NULL, NULL, TO_ROOM);
	    
	    affect_strip(ch, paf->type);
	    return TRUE;
	}
    }

    return FALSE;
}

/*
 * Assume (to assume a swordmaster form)
 *
 */

void do_assume(CHAR_DATA *ch, char *argument)
{
    int level;
    char abuf[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH+32];
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    AFFECT_DATA newaf1;
newaf1.valid = TRUE;
newaf1.point = NULL;
    AFFECT_DATA newaf2;
newaf2.valid = TRUE;
newaf2.point = NULL;
    AFFECT_DATA newaf3;
newaf3.valid = TRUE;
newaf3.point = NULL;
    OBJ_DATA *weapon, *dual;
    int mana, sn;
    bool vers = FALSE;

    if (argument[0] == '\0')
    {
	send_to_char("What form did you wish to assume?\n\r", ch);
	return;
    }

    if (!str_prefix(argument,"none"))
    {
	if (!unassume_form(ch))
	    send_to_char("You're already not in a form.\n\r", ch);

	return;
    }

    dual = NULL;

    if ((weapon = get_eq_char(ch, WEAR_WIELD))==NULL)
	if ((dual = get_eq_char(ch, WEAR_DUAL_WIELD)) == NULL)
	{
	    send_to_char("You must be using a sword to assume a form.\n\r", ch);
	    return;
	}
	else
	    vers = TRUE; /* they need to be versatile, sword is in the offhand */


    if (!vers)
    {
        if (weapon->value[0] != WEAPON_SWORD)
	{
	    send_to_char("You must be using a sword to assume a form.\n\r", ch);
	    return;
	}
    }
    else
    {
  	if (dual->value[0] != WEAPON_SWORD)
	{
	    send_to_char("You must be using a sword to assume a form.\n\r", ch);
	    return;
	}
    }

    if (!strncmp(argument, "form", 4))
	strcpy(abuf, argument);
    else
 	sprintf(abuf, "form of the %s", argument);
	  

    if ((sn = find_spell(ch, abuf, spell_form)) < 1 || !get_skill(ch, sn))
    {
        send_to_char( "You don't know any forms of that name.\n\r", ch );
        return;
    }

    if ((sn == GSN_FORM_OF_THE_BEAR) && (get_eq_char(ch, WEAR_SHIELD) != NULL))
    {
	send_to_char( "You can't step into the form of the bear while using a shield!\n\r", ch);
	return;
    }

    if (ch->level + 2 == skill_table[sn].skill_level[ch->class_num])
        mana = 50;
    else
        mana = UMAX(
            skill_table[sn].min_mana,
            100 / ( 2 + ch->level - skill_table[sn].skill_level[ch->class_num] ) );

    if (ch->mana < mana)
    {
 	send_to_char("You're too exhausted to assume that form.\n\r", ch);
	WAIT_STATE(ch, PULSE_VIOLENCE);
	return;
    }
    else
	    expend_mana(ch, mana/2);

    if (number_percent() > get_skill(ch, sn))
    {
	send_to_char("You try to assume the form, but you can't quite get it right.\n\r", ch);
	WAIT_STATE(ch, 2*PULSE_VIOLENCE);
	return;
    }

    unassume_form(ch);

    level = ch->level;

	/*
	 * Good god, they're actually getting into a form. Set up the affect.
	 */

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = -1;
    af.modifier  = 0;
    af.location  = APPLY_FORM;
    af.bitvector = 0;
    affect_to_char( ch, &af );

    sprintf(buf, "You step into the %s.\n", skill_table[sn].name);
    send_to_char(buf, ch);
    sprintf(buf, "$n steps into the %s.", skill_table[sn].name);
    act(buf, ch, NULL, NULL, TO_ROOM);

	/* back when I did this, I didn't expect to keep sns
	 * constant. Then I realized I really needed to for
	 * the sake of people writing progs. Live and learn	
	 */
    switch(sn)
    {
	default:
		break;

	case GSN_FORM_OF_THE_CYCLONE:

	newaf1.where	 = TO_AFFECTS;
	newaf1.type	 = sn;
	newaf1.level	 = level;
	newaf1.duration  = -1;
	newaf1.modifier	 = (level / 3 * -1) - 3;
	newaf1.location  = APPLY_AC;
	newaf1.bitvector = 0;
	affect_to_char( ch, &newaf1 );
	break;

	case GSN_FORM_OF_THE_BULL:

    	newaf1.where     = TO_AFFECTS;
    	newaf1.type      = sn;
    	newaf1.level     = level;
    	newaf1.duration  = -1;
    	newaf1.modifier  = URANGE(0,level,15);
    	newaf1.location  = APPLY_DAMROLL;
    	newaf1.bitvector = 0;
    	affect_to_char( ch, &newaf1 );

    	newaf2.where     = TO_AFFECTS;
    	newaf2.type      = sn;
    	newaf2.level     = level;
    	newaf2.duration  = -1;
    	newaf2.modifier  = URANGE(-15,0-level,0);
    	newaf2.location  = APPLY_HITROLL;
    	newaf2.bitvector = 0;
    	affect_to_char( ch, &newaf2 );
   	break;

	case GSN_FORM_OF_THE_BEAR:

    	newaf1.where     = TO_AFFECTS;
    	newaf1.type      = sn;
    	newaf1.level     = level;
    	newaf1.duration  = -1;
    	newaf1.modifier  = level/2;
    	newaf1.location  = APPLY_DAMROLL;
    	newaf1.bitvector = 0;
    	affect_to_char( ch, &newaf1 );

    	newaf2.where     = TO_AFFECTS;
    	newaf2.type      = sn;
    	newaf2.level     = level;
    	newaf2.duration  = -1;
    	newaf2.modifier  = level/2;
    	newaf2.location  = APPLY_HITROLL;
    	newaf2.bitvector = 0;
    	affect_to_char( ch, &newaf2 );
   	break;

	case GSN_FORM_OF_THE_VIPER:

    	newaf1.where     = TO_AFFECTS;
    	newaf1.type      = sn;
    	newaf1.level     = level;
    	newaf1.duration  = -1;
    	newaf1.modifier  = 0;
    	newaf1.location  = 0;
    	newaf1.bitvector = 0;
    	affect_to_char( ch, &newaf1 );
	break;

	case GSN_FORM_OF_THE_WASP:

    	newaf1.where     = TO_AFFECTS;
    	newaf1.type      = sn;
    	newaf1.level     = level;
    	newaf1.duration  = -1;
    	newaf1.modifier  = 0;
    	newaf1.location  = 0;
    	newaf1.bitvector = 0;
    	affect_to_char( ch, &newaf1 );
	break;

	case GSN_FORM_OF_THE_WHIRLWIND:

    	newaf1.where     = TO_AFFECTS;
    	newaf1.type      = sn;
    	newaf1.level     = level;
    	newaf1.duration  = -1;
    	newaf1.modifier  = 200;
    	newaf1.location  = APPLY_AC;
    	newaf1.bitvector = 0;
    	affect_to_char( ch, &newaf1 );
	break;

	case GSN_FORM_OF_THE_BOAR:

    	newaf1.where     = TO_AFFECTS;
    	newaf1.type      = sn;
    	newaf1.level     = level;
    	newaf1.duration  = -1;
    	newaf1.modifier  = (level/10)+5;
    	newaf1.location  = APPLY_DAMROLL;
    	newaf1.bitvector = 0;
    	affect_to_char( ch, &newaf1 );

    	newaf2.where     = TO_AFFECTS;
    	newaf2.type      = sn;
    	newaf2.level     = level;
    	newaf2.duration  = -1;
    	newaf2.modifier  = (level/10)+5;
    	newaf2.location  = APPLY_HITROLL;
    	newaf2.bitvector = 0;
    	affect_to_char( ch, &newaf2 );

    	newaf3.where     = TO_AFFECTS;
    	newaf3.type      = sn;
    	newaf3.level     = level;
    	newaf3.duration  = -1;
    	newaf3.modifier  = level - 10;
    	newaf3.location  = APPLY_AC;
    	newaf3.bitvector = 0;
    	affect_to_char( ch, &newaf3 );
   	break;
	
	case GSN_FORM_OF_THE_SERPENT:

    	newaf1.where     = TO_AFFECTS;
    	newaf1.type      = sn;
    	newaf1.level     = level;
    	newaf1.duration  = -1;
    	newaf1.modifier  = -15 + level/5;
    	newaf1.location  = APPLY_DAMROLL;
    	newaf1.bitvector = 0;
    	affect_to_char( ch, &newaf1 );
	break;

	case GSN_FORM_OF_THE_HAWK:

    	newaf1.where     = TO_AFFECTS;
    	newaf1.type      = sn;
    	newaf1.level     = level;
    	newaf1.duration  = -1;
    	newaf1.modifier  = 3;
    	newaf1.location  = APPLY_DEX;
    	newaf1.bitvector = 0;
    	affect_to_char( ch, &newaf1 );

    	newaf2.where     = TO_AFFECTS;
    	newaf2.type      = sn;
    	newaf2.level     = level;
    	newaf2.duration  = -1;
    	newaf2.modifier  = -100 + (100-level);
    	newaf2.location  = APPLY_AC;
    	newaf2.bitvector = 0;
    	affect_to_char( ch, &newaf2 );
	break;

    }

}

/*
 * Control the fights going on.
 * Called periodically by update_handler.
 */
void violence_update( void )
{
    ROOM_INDEX_DATA *room;
    bool checked = FALSE, meep = FALSE, vers = FALSE, found = FALSE;
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    AFFECT_DATA *paf, *song;
    CHAR_DATA *vch;
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    int tcount, chance;
    /* used for learning creatures */
    int i;

    for ( ch = char_list; ch != NULL; ch = ch->next )
    {
	ch_next	= ch->next;

	checked = FALSE;

	/* decrement the wait */
	if (ch->desc == NULL)
	{
	    ch->wait = UMAX(0,ch->wait - PULSE_VIOLENCE);
	    ch->daze = UMAX(0,ch->daze - PULSE_VIOLENCE); 
        }

	if (IS_NPC(ch) && IS_SET(ch->nact, ACT_CLASSED) && !ch->wait)
	{
	    mob_class_update(ch);
	    if (!IS_VALID(ch))
		continue;
	}

    if (ch->in_room != NULL && IS_OAFFECTED(ch, AFF_RITEOFTHESUN)
    && IS_OUTSIDE(ch) && !IS_SET(ch->in_room->room_flags, ROOM_DARK) 
    && !IS_SET(ch->in_room->room_flags, ROOM_UBERDARK) && !IS_SET(ch->in_room->room_flags, ROOM_NOWEATHER)
    && ch->in_room->area->w_cur.cloud_cover <= 50 && time_info.hour >= season_table[time_info.season].sun_up
    && time_info.hour < season_table[time_info.season].sun_down && (paf = affect_find(ch->affected,gsn_riteofthesun)) != NULL)
    {
	    if (current_time > paf->modifier + PULSE_VIOLENCE)
	    {
		    send_to_char("You feel infused with the sun's power as you complete your rite.\n\r",ch);
		    act("The sunlight flashes around $n, infusing $m with power.",ch,NULL,NULL,TO_ROOM);
		    paf->location = APPLY_HIT;
		    float halp = 12.0 - time_info.hour;
		    halp -= time_info.half ? 0.5 : 0.0;
            halp = fabs(halp) / 3;
		    paf->modifier = round(ch->level * (4.0-halp));
		    ch->max_hit += paf->modifier;
		    ch->hit += paf->modifier;
		    paf->bitvector = 0;	    
		    REMOVE_BIT(ch->oaffected_by,AFF_RITEOFTHESUN);
	    }
	    else
        {
            switch (number_range(1, 4))
	    	{
		        case 1: send_to_char("You draw in more of the sun's energy.\n\r",ch);break;
    		    case 2: send_to_char("The sun's rays warm your body.\n\r",ch); break;
		        case 3: send_to_char("You bask in the sun's radiance.\n\r",ch); break;
		        case 4: send_to_char("As you absorb more sunrays, you feel your rite growing in power.\n\r",ch); break;
		    }
        }
    }

	if (IS_NAFFECTED(ch, AFF_RALLY) && number_bits(2) == 0)
	    REMOVE_BIT(ch->naffected_by, AFF_RALLY);

	if (IS_PAFFECTED(ch, AFF_MAUL) && number_bits(2) == 0)
	    REMOVE_BIT(ch->paffected_by, AFF_MAUL);
	
	if (IS_AFFECTED(ch, AFF_WIZI) && ch->fighting
         && (!IS_NPC(ch) || (ch->pIndexData->vnum != MOB_VNUM_LESSER_ETHEREAL))) // HACK!
	    stop_fighting_all(ch);

	if (ch->fighting == ch)
	    stop_fighting(ch);

	if (ch->mercy_from != NULL)
		stop_fighting(ch);

	if (!ch->fighting && ch->position == POS_FIGHTING)
	    switch_position(ch, POS_STANDING);

	if (ch->position == POS_SLEEPING && ch->fighting != NULL)
	{
	    for (vch = ch->in_room->people; vch != NULL;vch = vch->next_in_room)
		if (vch->fighting == ch)
		    meep = TRUE;

	    if (!meep)
	        ch->fighting = NULL;
	}

	meep = FALSE;
	if (!IS_NPC(ch) && ch->position == POS_FIGHTING && !IS_SET(ch->act, PLR_AUTOATTACK) && ch->fighting != NULL)
	{
		for (vch = ch->in_room->people; vch != NULL;vch = vch->next_in_room)
			if (vch->fighting == ch && (IS_SET(vch->act, PLR_AUTOATTACK) || IS_NPC(vch)))
				meep = TRUE;
			
		if (!meep)
			stop_fighting(ch);
	}
		
	if ((victim = ch->fighting) == NULL)
	{
	    ch->lost_att = 0;
	    continue;
	}

	if (!ch->in_room)
	    stop_fighting(ch);

	if (victim->in_room == NULL || victim->in_room != ch->in_room)
        {
	    stop_fighting(victim);
	    stop_fighting(ch);
	    continue;
        }

	if (is_safe(victim, ch))
	    stop_fighting(ch);

	if (!ch->in_room)
	    continue;
	else
	    room = ch->in_room;

	if (is_affected(ch, gsn_guard))
	   affect_strip(ch, gsn_guard);

	if (IS_OAFFECTED(ch, AFF_GHOST))
	    continue;
	
	if (ch->fighting != NULL && ch->class_num == global_int_class_gladiator)
	    check_decapitate(ch,ch->fighting);

	if (ch->fighting != NULL
	  && (ch->in_room->sector_type == SECT_HILLS
	    || ch->in_room->sector_type == SECT_MOUNTAIN)
	  && get_skill(ch,gsn_terrainlore) > 0)
	    if (number_percent() < get_skill(ch,gsn_terrainlore)/3)
	    {
	    	act("$n outmaneuvers you amidst the rugged terrain.",ch,NULL,victim,TO_VICT);
	    	act("You outmaneuver $N on the uneven terrain, tiring $M.",ch,NULL,victim,TO_CHAR);
	    	ch->fighting->move -= 20;
	    	check_improve(ch,victim,gsn_terrainlore,TRUE,1);
	    }

	if (is_affected(ch, gsn_enslave) && !IS_AFFECTED(ch, AFF_CHARM))
	{
	    if (!IS_NPC(ch))  /* caster */
	    {
		for (vch = char_list; vch; vch = vch->next)
		{
		    if (is_affected(vch, gsn_enslave)
		     && !IS_AFFECTED(vch, AFF_CHARM)
		     && IS_NPC(vch)
		     && (get_modifier(vch->affected, gsn_enslave) == ch->id))
		    {
			act("Your concentration breaks, and $N attacks!", ch, NULL, vch, TO_CHAR);
			act("$N roars in fury and attacks $n!", ch, NULL, vch, TO_NOTVICT);
			act("You roar in fury and attack $n!", ch, NULL, vch, TO_VICT);
			affect_strip(vch, gsn_enslave);
			multi_hit(vch, ch, TYPE_UNDEFINED);
		 	break;
		    }
		}
		affect_strip(ch, gsn_enslave);
	    }
	    else if (!IS_AFFECTED(ch, AFF_CHARM))
	    {
		int vid = get_modifier(ch->affected, gsn_enslave);

		for (vch = char_list; vch; vch = vch->next)
		    if (!IS_NPC(vch)
		     && is_affected(vch, gsn_enslave)
		     && (vch->id == vid))
		    {
			act("$N becomes distracted, and your attempt to enslave $M fails.", vch, NULL, ch, TO_CHAR);
			affect_strip(vch, gsn_enslave);
			break;
		    }
		affect_strip(ch, gsn_enslave);
	    }
	}

	if (IS_OAFFECTED(ch, AFF_INSCRIBE))
	{
	    send_to_char("You cease inscribing.\n\r", ch);
	    affect_strip(ch, gsn_inscribe);
	}

 	if (IS_NAFFECTED(ch, AFF_GARROTE_CH))
	{
	    for (paf = ch->affected; paf; paf = paf->next)
	        if (paf->type == gsn_garrote && paf->bitvector == AFF_GARROTE_CH)
		    break;

	    if (paf && paf->point)
		vch = (CHAR_DATA *) paf->point;
	    else
		vch = NULL;

	    affect_remove(ch, paf);

	    if (vch && IS_VALID(vch))
	    {
		act("In the heat of battle, $n is able to slip the garrote wire.", vch, NULL, NULL, TO_ROOM);
		act("In the heat of battle, you are able to slip the garrote wire.", vch, NULL, NULL, TO_CHAR);

		for (paf = vch->affected; paf; paf = paf->next)
		    if ((paf->type == gsn_garrote) && (paf->bitvector == AFF_GARROTE_VICT))
		    {
			affect_remove(vch, paf);
			break;
		    }
	    }
	}

	if (IS_NAFFECTED(ch, AFF_GARROTE_VICT))
	{
	    act("In the heat of battle, $n is able to slip the garrote wire.", ch, NULL, NULL, TO_ROOM);
	    act("In the heat of battle, you are able to slip the garrote wire.", ch, NULL, NULL, TO_CHAR);

	    for (paf = ch->affected; paf; paf = paf->next)
		if ((paf->type == gsn_garrote) && (paf->bitvector == AFF_GARROTE_VICT))
		    break;
	 
	    if (paf && paf->point)
		vch = (CHAR_DATA *) paf->point;
	    else
		vch = NULL;

	    affect_remove(ch, paf);

	    if (vch && IS_VALID(vch))
		for (paf = vch->affected; paf; paf = paf->next)
		    if ((paf->type == gsn_garrote) && (paf->bitvector == AFF_GARROTE_CH))
		    {
			affect_remove(vch, paf);
			break;
		    }
	}

    // Illusions should not fight their masters	
    if (IS_NPC(ch) && IS_SET(ch->act, ACT_ILLUSION) && ch->master && (ch->master == ch->fighting))
	{
	    act("You realize $n is an illusion, and it disappears.", ch, NULL, ch->fighting, TO_VICT);
	    act("$N realizes $n is an illusion, and it disappears.", ch, NULL, ch->fighting, TO_VICTROOM);
	    extract_char(ch,TRUE);
	    continue;
	}

	if (IS_NPC(victim) && (ch->class_num == global_int_class_ranger))
        {
	    for ( i = 0; i < MAX_CLORE; i++ )
		if (ch->lcreature[i] == victim->pIndexData->vnum)
		{
		    if (ch->lpercent[i] >= 100)
			break;

    		    chance = round(2.5 * learn_table[get_curr_stat(ch,STAT_INT)]);
                    chance += ch->level;
		    chance = ((chance * get_skill(ch, gsn_creaturelore)) / 100);

                    if (number_range(1,1000) <= chance)
		    {
			chance = URANGE(5,100 - ch->lpercent[i], 95);
			if (number_percent() < chance)
			{
			    sprintf(buf, "You feel more knowledgeable in the ways of %s!\n\r", victim->short_descr);
			    send_to_char(buf, ch);
		            ch->lpercent[i]++;
			    gain_exp(ch, 2);
			}
		    }

		    break;
		}
 	    check_improve(ch,victim, gsn_creaturelore, TRUE, 5);
	}

        if (IS_SET(ch->act, PLR_LEARN) && (ch->class_num == global_int_class_ranger) && IS_NPC(victim))
	{
	    for ( i = 0; i < MAX_CLORE; i++ )
                if (ch->lcreature[i] == victim->pIndexData->vnum)
		{
		    send_to_char("You are already learning the ways of this creature.\n\r", ch); 
		    REMOVE_BIT(ch->act, PLR_LEARN);
		    break;
                }

	    if (IS_SET(victim->act, ACT_BADASS))
	    {
	        send_to_char("This creature is too difficult to learn.\n\r", ch);
	        REMOVE_BIT(ch->act, PLR_LEARN);
	    }

            if (IS_SET(ch->act, PLR_LEARN))
	 	for (i = 0; i < MAX_CLORE; i++ )
		    if (ch->lcreature[i] == 0)
		    {
		        ch->lcreature[i] = victim->pIndexData->vnum;
		        ch->lpercent[i] = 0;
		        sprintf(buf, "You begin learning the ways of %s.\n\r", victim->short_descr);
		        send_to_char(buf, ch);
		        break;
		    }

	    REMOVE_BIT(ch->act, PLR_LEARN);
	}
 
        if ((is_affected(ch,gsn_hooded)) && (number_percent() < 5))
        {
            affect_strip(ch, gsn_hooded);
            act("In the heat of battle, $n's hood comes flying off.",
		ch, NULL, NULL, TO_ROOM);
            act("In the heat of battle, your hood comes flying off.",
		ch, NULL, NULL, TO_CHAR);
        }

	if (ch->position <= POS_SLEEPING)
	    continue;

	vers = FALSE;
	obj = get_eq_char(ch, WEAR_WIELD);
	
	if (obj == NULL && (obj = get_eq_char(ch, WEAR_DUAL_WIELD)) != NULL)
	{
	  if (obj->value[0] == WEAPON_SWORD)
	    vers = TRUE;
	  obj = NULL;
	}

	if (obj == NULL)
	    if ((obj = get_eq_char(ch, WEAR_DUAL_WIELD)) != NULL
	     && (get_skill(ch, gsn_versatility) == 0 ||
		get_skill(ch, gsn_versatility) < number_percent()))
	    {
		unequip_char(ch, obj);
		equip_char(ch, obj, WORN_WIELD);
	    }
	if (is_affected(ch, gsn_form_of_the_living_seas)
	  && number_percent() < get_skill(ch,gsn_form_of_the_living_seas))
	{
	    CHAR_DATA *gch;
	    act("You feel better as your form channels healing energy into you.",ch,NULL,NULL,TO_CHAR);
	    ch->hit = UMIN(ch->max_hit,ch->hit + 5 + ch->level/5);
	    if (ch->in_room->sector_type == SECT_UNDERWATER)
	        for (gch = ch->in_room->people; gch; gch = gch->next_in_room)
	            if (is_same_group(ch, gch) && (ch != gch))
	            {
	       	        if (is_an_avatar(gch))
		                continue;

        			act("You feel better as $n's form channels healing energy towards you.", ch, NULL, gch, TO_VICT);
	    	        gch->hit = UMIN(gch->max_hit, gch->hit + get_skill(ch,gsn_form_of_the_living_seas)/10);
	    
	            }
        }

	if ( obj != NULL && obj_is_affected(obj, gsn_runeoflife) && number_bits(3) == 0)
	{
	    if (!is_an_avatar(ch))
	    {
	        act("The Rune of Life on $n's weapon glows brightly!", ch, obj, NULL, TO_ROOM);
	        act("The Rune of Life on $p glows brightly!", ch, obj, NULL, TO_CHAR);
	        ch->hit = UMIN(ch->max_hit, ch->hit + number_fuzzy(ch->level));
	    }
	}	

	if ( obj != NULL 
	  && ((paf = affect_find(obj->affected, gsn_runeofembers)) != NULL)
	  && number_bits(1) == 0)
	{
	    if (is_affected(victim,gsn_ignite))
	    {
		paf = affect_find(victim->affected,gsn_ignite);
		if (number_bits(1) == 0)
		{
		    act("The rune of embers on $n's weapon glows brightly!", ch, obj, NULL, TO_ROOM);
	    	    act("The rune of embers on $p glows brightly!", ch, obj, NULL, TO_CHAR);
		    paf->duration++;
		}
	    }
	    else
	        obj_cast_spell(gsn_ignite,paf->level,ch,victim,obj);
	}
    
    // Handle forged sphere-traits
    if (obj != NULL)
    {
        struct SphereCast
        {
            static void CheckCast(CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj, Forge::Trait trait, int spellSN, int percentFirstLevel, int percentOtherLevels, const char * sentence)
            {
                int traitLevels(static_cast<int>(Forge::TraitCount(*obj, trait)));
                if (traitLevels <= 0)
                    return;

                if (number_percent() <= (percentFirstLevel + ((traitLevels - 1) * percentOtherLevels)))
                {
                    act(sentence, ch, obj, NULL, TO_ALL);
                    obj_cast_spell(spellSN, obj->level, ch, victim, obj);
                }
            }
        };

        SphereCast::CheckCast(ch, victim, obj, Forge::Trait_Water, gsn_iceshard, 2, 5, "$p discharges a shard of pure ice!");
        SphereCast::CheckCast(ch, victim, obj, Forge::Trait_Water, gsn_frostblast, 0, 2, "$p sends out a freezing wave of cold!");
        
        SphereCast::CheckCast(ch, victim, obj, Forge::Trait_Earth, gsn_sandspray, 5, 5, "$p sends out a spray of stinging sand!");
        SphereCast::CheckCast(ch, victim, obj, Forge::Trait_Earth, gsn_density, 0, 2, "$p hums with earthen power!");

        SphereCast::CheckCast(ch, victim, obj, Forge::Trait_Void, gsn_enervatingray, 3, 3, "A dark ray beams out of $p!");
        SphereCast::CheckCast(ch, victim, obj, Forge::Trait_Void, gsn_blindness, 0, 1, "$p crackles malevolently!");
        SphereCast::CheckCast(ch, victim, obj, Forge::Trait_Void, gsn_curse, 1, 1, "$p throbs with negative energy!");

        SphereCast::CheckCast(ch, victim, obj, Forge::Trait_Spirit, gsn_searinglight, 2, 4, "$p flares up in a blast of searing light!");
        SphereCast::CheckCast(ch, victim, obj, Forge::Trait_Spirit, gsn_spiritwrack, 0, 1, "A scorching golden light shines out from $p!");
        SphereCast::CheckCast(ch, victim, obj, Forge::Trait_Spirit, gsn_spiritblock, 0, 1, "Tendrils of energy spiral out of $p!");

        SphereCast::CheckCast(ch, victim, obj, Forge::Trait_Air, gsn_lightning_bolt, 2, 5, "$p crackles with electrical energy!");
        SphereCast::CheckCast(ch, victim, obj, Forge::Trait_Air, gsn_windbind, 0, 2, "As $p cuts through the air, wisps of wind tighten in its wake!");

        SphereCast::CheckCast(ch, victim, obj, Forge::Trait_Fire, gsn_flamestrike, 2, 4, "A blast of fire flares out from $p!");
        SphereCast::CheckCast(ch, victim, obj, Forge::Trait_Fire, gsn_consume, 0, 2, "$p glows with burning power!");
    }

	if (obj && ch->in_room && !IS_SET(obj->extra_flags[0], ITEM_NOREMOVE)
	 && is_affected(ch, gsn_frostbite) && (number_bits(6) == 0))
	    {
		act("$n shivers and $p slips from $s hands!", ch, obj, NULL, TO_ROOM);
		act("You shiver and $p slips from your hands!", ch, obj, NULL, TO_CHAR);
		obj_from_char(obj);
		if (IS_SET(obj->extra_flags[0], ITEM_NODROP))
		    obj_to_char(obj, ch);
		else
		    obj_to_room(obj, ch->in_room);
	    }

	if (is_affected(ch, gsn_density) && number_percent() < 15 * (ch->size - SIZE_SMALL))
	{
	    act("Feeling heavy, you cannot support yourself and crumple to the ground!", ch, NULL, NULL, TO_CHAR);
	    act("$n pauses under $s unnatural density, and crumples to the ground briefly!", ch, NULL, NULL, TO_ROOM);
	    WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
	}

	if (ch->in_room && !IS_IMMORTAL(ch) && area_is_affected(ch->in_room->area, gsn_gravitywell) && !is_affected(ch, gsn_stabilize) && number_percent() < 15 * (ch->size - SIZE_SMALL))
	{
	    act("Under the force of the gravity well, you are pulled to the ground!", ch, NULL, NULL, TO_CHAR);
	    act("Under the force of the gravity well, $n is pulled to the ground!", ch, NULL, NULL, TO_ROOM);
	    WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
	}

	if (is_affected(ch, gsn_grease) && (!is_flying(ch) || is_affected(ch, gsn_earthbind)) && number_percent() < 15)
    {
	    act("Your greased feet slip out from under you, and you go sprawling to the ground!", ch, NULL, NULL, TO_CHAR);
	    act("$n slips suddenly, and goes sprawling to the ground!", ch, NULL, NULL, TO_ROOM);
	    WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE + number_bits(3)));
	}

	if (IS_NPC(ch))
	{
	    if (!IS_SET(ch->act, ACT_NOTRACK))
		ch->tracking = victim;
	}
	    for (paf = ch->affected; paf != NULL; paf = paf->next )
	    {
		if ((paf->location == APPLY_FORM) && (checked == FALSE))
		{
		    check_improve(ch,victim,paf->type,TRUE,4);
		    checked = TRUE;
		    if (ch->mana > skill_table[paf->type].min_mana)
		    {
			if (!vers && obj == NULL)
			{
			    sprintf(buf, "You can't execute your form without a sword.\n\r");
			    send_to_char(buf, ch);
			    unassume_form(ch);
			}	
			else if (!vers && obj->value[0] != WEAPON_SWORD)
			{
	    		    check_improve(ch,victim,gsn_versatility,FALSE,3);
			    sprintf(buf, "Your can't execute your form without a sword.\n\r");
			    send_to_char(buf, ch);
			    unassume_form(ch);
			}	
			else if (vers && get_skill(ch, gsn_versatility) < number_percent())
			{
			    sprintf(buf, "You fail to utilize your sword in your off hand to remain in your form.\n\r");
			    send_to_char(buf, ch);
			    unassume_form(ch);
			}
			else
			{
			    found = FALSE;
			    for (i = 0; form_table[i]; i++)
			    {
				if (*form_table[i] == paf->type)
				{
			            if (is_affected(victim, *form_table[i+1]))
				    {
					send_to_char("Your opponent drives you out of your form with precise countermoves!\n\r", ch);
					send_to_char("You break your opponent's form with precise countermoves!\n\r", victim);
					unassume_form(ch);
					WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE*2));
					found = TRUE;
				    }
				    break;
			        }
			    }

			    if (!found)
			    {
			        expend_mana(ch, skill_table[paf->type].min_mana);
			        if (vers)
	    			    check_improve(ch,victim,gsn_versatility,TRUE,3);
			    }
			}
		    }
		    else
		    {
			sprintf(buf, "Your form has exhausted you.\n\r");
			send_to_char(buf, ch);
			unassume_form(ch);
		    }	
		}
	    }
				
        if (is_affected(ch, gsn_form_of_the_whirlwind))
	    for (vch = ch->in_room->people ; vch != NULL; vch = vch->next_in_room)
		if (vch->fighting == ch && ch->in_room == vch->in_room && IS_AWAKE(ch) && (vch != victim))
		    multi_hit( ch, vch, TYPE_UNDEFINED );

	if (room_is_affected(ch->in_room, gsn_freeze)
	 && (ch->class_num != global_int_class_waterscholar)
	 && (ch->class_num != global_int_class_watertemplar)
	 && !is_flying(ch)
	 && number_percent()<get_modifier(ch->in_room->affected,gsn_freeze))
	{
	    send_to_char("You slip on the frozen ground and go sprawling!\n\r", ch);
	    act("$n slips on the frozen ground and goes sprawling!", ch, NULL, NULL, TO_ROOM);
	    WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE*2));
	}
      
	tcount = 0;
        if (ch->class_num == global_int_class_gladiator)
	{
	    for (vch = ch->in_room->people;vch != NULL; vch = vch->next_in_room)
	    {
	  	if (vch && vch->fighting == ch)
		    tcount++;
	    }

	    if (tcount >= 2)
	    {
    	        if ((chance = get_skill(ch,gsn_corner)) > 0)
	        {
	            if (number_percent() < chance)
		        for (vch = ch->in_room->people ; vch != NULL; vch = vch->next_in_room)
		        {
	        	    check_improve(ch,vch,gsn_corner,TRUE,5);
			    if (vch->fighting == ch && ch->in_room == vch->in_room && IS_AWAKE(ch) && (vch != victim))
		 	    {
		     	        ch->fighting = vch;
	    	     	        multi_hit( ch, vch, TYPE_UNDEFINED );
		 	    }
		        }
	            else
	                check_improve(ch,vch,gsn_corner,FALSE,5);
	            ch->fighting = victim;
	         }
	    }
	}

	if (is_affected(ch, gsn_greatertentaclegrowth) && !is_flying(victim) && number_percent() < 8)
	{
		act("You grab $N with your tentacles, shaking and stunning $M!", ch, NULL, victim, TO_CHAR);
		act("$n grabs you with $s tentacles, shaking and stunning you!", ch, NULL, victim, TO_VICT);
		act("$n grabs $N with a large tentacle, shaking and stunning $m!", ch, NULL, victim, TO_NOTVICT);
		WAIT_STATE(victim, UMAX(victim->wait, 2 * PULSE_VIOLENCE));
	}
	if (is_affected(ch, gsn_lycanthropy) && number_percent() < 5)
	{
		act("You rake $N viciously with your claws!", ch, NULL, victim, TO_CHAR);
		act("$n rakes you viciously with $s claws!", ch, NULL, victim, TO_VICT);
		act("$n rakes $N viciously with $s claws!", ch, NULL, victim, TO_NOTVICT);
		damage_old(ch, victim, number_range((int)(ch->level * .6),(int)(ch->level * 1.4)), 
			gsn_lycanthropy, DAM_SLASH, TRUE);
	}
	
	if (!IS_NPC(ch) && is_affected(ch, gsn_form_of_the_spider)
	 && (number_percent() < (get_skill(ch, gsn_form_of_the_spider)/2 + ch->level/5)))
	{
	    act("You fake and feint, wasting $N's energy.", ch, NULL, victim, TO_CHAR);
	    act("$n fakes and feints, wasting $N's energy.", ch, NULL, victim, TO_NOTVICT);
	    act("$n fakes and feints, tricking you into wasting energy.", ch, NULL, victim, TO_VICT);
	    victim->move = UMAX(0, victim->move - 50);
	}
		
	if (is_affected(ch, gsn_form_of_the_cat)
	 && (number_percent() < ((get_skill(ch, gsn_form_of_the_cat) * .3) + ch->level/5)))
	{
	    act("You dart in and out nimbly, throwing $N off balance.", ch, NULL, victim, TO_CHAR);
	    act("$n darts in and out nimbly, throwing $N off balance.", ch, NULL, victim, TO_NOTVICT);
	    act("$n darts in and out nimbly, throwing you off balance.", ch, NULL, victim, TO_VICT);
	    victim->wait = UMAX(1*PULSE_VIOLENCE+1, victim->wait);
	}

	if (is_affected(ch, gsn_form_of_the_winter_wind)
	 && (number_percent() < ((get_skill(ch, gsn_form_of_the_winter_wind) * .5) + ch->level/5)))
	{
	    act("You discharge a small burst of ice at $N!", ch, NULL, victim, TO_CHAR);
	    act("$n discharges a small burst of ice at $N!", ch, NULL, victim, TO_NOTVICT);
	    act("$n discharges a small burst of ice at you!", ch, NULL, victim, TO_VICT);
	    if (!saves_spell(ch->level, ch, victim, DAM_COLD))
	    {
		OBJ_DATA *pObj;

		af.where	 = TO_AFFECTS;
		af.type	 = gsn_winterwind;
		af.level	 = ch->level;
		af.duration  = ch->level/5;
		af.location  = APPLY_DEX;
		af.bitvector = 0;

		if (is_affected(victim, gsn_winterwind))
		{
		    af.modifier = get_modifier(victim->affected, gsn_winterwind);

		    if (af.modifier < 10 && number_range(1, af.modifier) == 1)
			af.modifier -= 1;

		    affect_strip(victim, gsn_winterwind);
		}
		else
		    af.modifier = -1;

		affect_to_char(victim, &af);

		act("$n shivers slightly as the burst of ice slams into $m.", victim, NULL, NULL, TO_ROOM);
		send_to_char("You feel yourself grow colder as the burst of ice slams into you.\n\r", victim);

		if (!IS_NPC(victim) 
		  && (pObj = get_eq_char(victim, WEAR_WIELD)) 
		  && victim->in_room 
		  && !IS_SET(pObj->extra_flags[0], ITEM_NOREMOVE) 
		  && (number_bits(2) == 0))
		{
		    act("$p slips out of your grasp as your hand becomes numb.", victim, pObj, NULL, TO_CHAR);
		    act("$p slips out of $n's numbed hand.", victim, pObj, NULL, TO_ROOM);
		    obj_from_char(pObj);
		    if (IS_SET(pObj->extra_flags[0], ITEM_NODROP))
			obj_to_char(pObj, victim);
		    else
			obj_to_room(pObj, victim->in_room);
		}
	    }
	    else
	    {
		send_to_char("You resist the chilling effects of the ice.\n\r", victim);
		act("$n appears to be unaffected by the burst of ice.", victim, NULL, NULL, TO_ROOM);
	    }
	}

	if (is_affected(ch, gsn_form_of_the_zephyr)
	 && (number_percent() < (get_skill(ch, gsn_form_of_the_zephyr) * .8)))
	{
	    act("Your illusions momentarily distract $N.", ch, NULL, victim, TO_CHAR);
	    act("You are momentarily distracted by $n's illusions.", ch, NULL, victim, TO_VICT);
	    act("$N is momentarily distracted by $n's illusions.", ch, NULL, victim, TO_NOTVICT);
	    victim->lost_att++;
	}
	
	if (is_affected(victim, gsn_entangle))
	{
	    act("The entangling net slows your attacks.",victim,NULL,NULL,TO_CHAR);
	    victim->lost_att++;
	    if (number_bits(2) == 0)
	    {
	        act("You work your way out of the net, destroying it.",victim,NULL,NULL,TO_CHAR);
		act("$n works $s way out of the net, destroying it.",victim,NULL,NULL,TO_ROOM);
		affect_strip(victim,gsn_entangle);
	    }
	}

        if (ch->in_room->sector_type == SECT_SWAMP
	   && !is_flying(victim)
	   && get_skill(ch,gsn_terrainlore) > 0)
	    if (number_percent() < (get_skill(ch, gsn_terrainlore)) / 3)
	    {
	    	act("You struggle to fight while caught in the mud!",ch,NULL,victim,TO_VICT);
	    	act("$N struggles to fight while caught in the mud.",ch,NULL,victim,TO_NOTVICT);
	    	act("You lure $N into deeper mud, slowing $S attacks.",ch,NULL,victim,TO_CHAR);
	    	victim->lost_att++;
	    	check_improve(ch,victim,gsn_terrainlore,TRUE,1);
	    }
	    
	if (is_affected(ch, gsn_form_of_the_wasp)
	 && (number_percent() < ((get_skill(ch, gsn_form_of_the_wasp) * .5) + ch->level/5)))
	{
	    act("You break through $N's defenses, delivering a carefully controlled thrust.", ch, NULL, victim, TO_CHAR);
	    act("$n breaks through $N's defenses, delivering a carefully controlled thrust.", ch, NULL, victim, TO_NOTVICT);
	    act("$n breaks through your defenses, delivering a carefully controlled thrust.", ch, NULL, victim, TO_VICT);
    	    af.where     = TO_OAFFECTS;
    	    af.type      = gsn_waspstrike;
    	    af.level     = ch->level;
    	    af.duration  = 8;
    	    af.location  = 0; 
	    if (is_affected(victim, gsn_waspstrike))
    		af.modifier  = get_modifier(victim->affected, gsn_waspstrike) - 1;
	    else
    		af.modifier  = -1;
    	    af.bitvector = AFF_BLEEDING;
	    affect_strip(victim, gsn_waspstrike);
    	    affect_to_char( victim, &af );
	}

	if (is_affected(ch, gsn_form_of_the_viper)
	 && (number_percent() < ((get_skill(ch, gsn_form_of_the_viper) * .4) + ch->level/5)))
	{
	    act("You strike with extreme precision at $N, lancing into a critical spot.", ch, NULL, victim, TO_CHAR);
	    act("$n strikes with extreme precision at $N, lancing into a critical spot.", ch, NULL, victim, TO_NOTVICT);
	    act("$n strikes with extreme precision, lancing into a critical spot.", ch, NULL, victim, TO_VICT);
    	    af.where     = TO_AFFECTS;
    	    af.type      = gsn_viperstrike;
    	    af.level     = ch->level;
    	    af.duration  = UMAX(8, (ch->level / 5));
    	    af.location  = APPLY_STR;
    	    af.modifier  = -1;
    	    af.bitvector = 0;
    	    affect_to_char( victim, &af );
	    af.location  = APPLY_DEX;
	    affect_to_char( victim, &af );
	}

	if (is_affected(ch,gsn_form_of_the_asp) && (number_percent() < ((get_skill(ch,gsn_form_of_the_asp)*.4) + ch->level/5)))
    {
        act("You feint, and slip through $N's defenses.", ch,NULL,victim,TO_CHAR);
        act("You send a burst of negative energy down your sword, biting deeply into $N!",ch,NULL,victim,TO_CHAR);

        act("$n feints, and slips through your defenses!", ch,NULL,victim,TO_VICT);
        act("As $s blade strikes, a numbing cold slides down the blade, biting deeply into your flesh!",ch,NULL,victim,TO_VICT);

        act("$n feints, and slips through $N's defenses!",ch,NULL,victim, TO_NOTVICT);
        act("As $n's blade strikes $N, a dim black glow skitters across $s blade. ",ch,NULL,victim,TO_NOTVICT);

        af.where     = TO_AFFECTS;
        af.type      = gsn_aspstrike;
        af.level     = ch->level;
        af.duration  = UMAX(8, (ch->level / 5));
        af.location  = APPLY_STR;
        af.modifier  = -1;
        af.bitvector = 0;
        affect_to_char( victim, &af );
        af.location  = APPLY_DEX;
        affect_to_char( victim, &af );

        af.where     = TO_OAFFECTS;
        af.type      = gsn_waspstrike;
        af.level     = ch->level;
        af.duration  = 8;
        af.location  = APPLY_HIDE;
        if (is_affected(victim, gsn_waspstrike))
            af.modifier  = get_modifier(victim->affected, gsn_waspstrike) -1;
        else
            af.modifier  = -1;
        af.bitvector = AFF_BLEEDING;
        affect_strip(victim, gsn_waspstrike);
        affect_to_char( victim, &af );
    } 

    // Actually perform the round of attacks, or stop fighting
	if ( IS_AWAKE(ch) && ch->in_room == victim->in_room )
	    multi_hit( ch, victim, TYPE_UNDEFINED );
	else
	    stop_fighting(ch);

	if ((victim = ch->fighting) == NULL)
	    continue;

	if (is_affected(ch, gsn_form_of_the_eagle))
	{
	    do_eagle_jab(ch, victim);
	    if ((victim = ch->fighting) == NULL)
	        continue;
	}

	if (is_affected(ch, gsn_form_of_the_crab) && number_percent() < get_skill(ch, gsn_form_of_the_crab) / 2)
	    do_crab_disarm(ch, victim);

    // Check for aurora
    if (!IS_SET(victim->imm_flags, IMM_BLIND) && !IS_AFFECTED(victim, AFF_BLIND) && number_bits(2) == 0)
    {
        AFFECT_DATA * aurora(get_affect(ch, gsn_aurora));
        if (aurora != NULL)
        {
            act("The dazzling aura surrounding you suddenly flares wildly into blinding light!", ch, NULL, NULL, TO_CHAR);
            act("The dazzling aura surrounding $n suddenly flares wildly into blinding light!", ch, NULL, NULL, TO_ROOM);
            if (!saves_spell(aurora->level, ch, victim, DAM_ILLUSION))
            {
                // Apply the blindness
                AFFECT_DATA af = {0};
                af.where     = TO_AFFECTS;
                af.type      = gsn_blindness;
                af.level     = aurora->level;
                af.location  = APPLY_HITROLL;
                af.modifier  = -4;
                af.duration  = 1 + (aurora->level / 17);
                af.bitvector = AFF_BLIND;
                affect_to_char(victim, &af);

                send_to_char("The light plays across your eyes, blinding you!\n", victim);
                act("The light plays across $n's eyes, blinding $m!", victim, NULL, NULL, TO_ROOM);
            }
        }
    }

    // Check for aura of soothing
    AFFECT_DATA * spiritAura(get_affect(victim, gsn_radiateaura));
    if (spiritAura != NULL && spiritAura->modifier == Aura_Soothing)
    {
        if (number_percent() <= get_skill(victim, gsn_radiateaura) / 3)
        {
            check_improve(victim, ch, gsn_radiateaura, true, 2);
            act("A gentle stream of opalescent light issues out from you, flowing towards $N.", victim, NULL, ch, TO_CHAR);
            act("A gentle stream of opalescent light issues out from $n, flowing towards you.", victim, NULL, ch, TO_VICT);
            act("A gentle stream of opalescent light issues out from $n, flowing towards $N.", victim, NULL, ch, TO_NOTVICT);
            tryApplyPacification(victim->level, (victim->level / 17), victim, ch, false);
        }
        else
            check_improve(victim, ch, gsn_radiateaura, false, 2);
    }

    // Check for wardoffrost
    AFFECT_DATA * wardoffrost(get_affect(victim, gsn_wardoffrost));
    if (wardoffrost != NULL && number_bits(3) == 0)
    {
        OBJ_DATA * chWeapon(get_eq_char(ch, WEAR_WIELD));
        if (chWeapon == NULL) chWeapon = get_eq_char(ch, WEAR_DUAL_WIELD);
        if (chWeapon != NULL && !IS_SET(chWeapon->extra_flags[0], ITEM_VIS_DEATH) && !obj_is_affected(chWeapon, gsn_heatsink))
        {
            act("An intense burst of freezing cold flares out from you!", victim, NULL, NULL, TO_CHAR);
            act("An intense burst of freezing cold flares out from $n!", victim, NULL, NULL, TO_ROOM);
            if (!saves_spell(wardoffrost->level, NULL, ch, DAM_COLD))
            {
                AFFECT_DATA haf = {0};
                haf.where   = TO_OBJECT;
                haf.type    = gsn_heatsink;
                haf.level   = wardoffrost->level;
                haf.duration = wardoffrost->level / 12;
                affect_to_obj(chWeapon, &haf);
                act("$p is coated in frost as it is chilled to the core.", ch, chWeapon, NULL, TO_ALL);
            }
        }
    }

	if (ch->in_room && (song = get_room_song(ch->in_room, gsn_echoesoffear))
	 && is_same_group(((CHAR_DATA *) song->point), victim)
	 && can_be_affected(ch,gsn_echoesoffear)
	 && (number_percent() > ((ch->hit * 100) / ch->max_hit)))
	{
	    act("$N's haunting tune breaks your spirit, and you flee from battle!", ch, NULL, ((CHAR_DATA *) song->point), TO_CHAR);
	    act("$N's haunting tune breaks $n's spirit, and $e flees from battle!", ch, NULL, ((CHAR_DATA *) song->point), TO_NOTVICT);
	    act("Your haunting tune breaks $n's spirit, and $e flees from battle!", ch, NULL, ((CHAR_DATA *) song->point), TO_VICT);
	    do_flee(ch, "");
	}

        mprog_hitprcnt_trigger( ch, victim );

	if (!ch->in_room || ch->in_room != room)
	    continue;

        mprog_fight_trigger( ch, victim );

	if (!ch->in_room || ch->in_room != room)
	    continue;

	for ( obj = ch->in_room->contents; obj != NULL; obj = obj->next_content )
	{
	    if (!ch->in_room || ch->in_room != room)
		break;
            oprog_hitprcnt_trigger( obj, victim );
	    if (!ch->in_room || ch->in_room != room)
		break;
            oprog_fight_trigger( obj, victim );
        }

	for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
        {
            if (!ch->in_room || ch->in_room != room)
	        break;
            oprog_hitprcnt_trigger( obj, victim );
	    if (!ch->in_room || ch->in_room != room)
		break;
            oprog_fight_trigger( obj, victim );
        }

	if (!ch->in_room || ch->in_room != room || ((victim = ch->fighting) == NULL))
	    continue;

	/*
	 * Fun for the whole family!
	 */
	check_assist(ch,victim);
    }

    return;
}

/* for auto assisting */
void check_assist(CHAR_DATA *ch,CHAR_DATA *victim)
{
    CHAR_DATA *rch, *rch_next;
    int i;
    bool assisted;

    for (rch = ch->in_room->people; rch != NULL; rch = rch_next)
    {
	rch_next = rch->next_in_room;
	
	if (IS_AWAKE(rch) && rch->fighting == NULL)
	{

	    if (victim->song && (victim->song->type == gsn_invokesympathy)
	     && IS_NPC(rch) && (rch->level <= victim->level)
	     && !IS_AFFECTED(victim, AFF_WIZI)
	     && !IS_AFFECTED(rch, AFF_CHARM)
	     && !IS_AFFECTED(rch, AFF_WIZI)
	     && !saves_spell(victim->level, NULL, rch, DAM_MENTAL))
	    {
		do_emote(rch, "screams and attacks!");
		multi_hit(rch, ch, TYPE_UNDEFINED);
		continue;
	    }

	    /* quick check for ASSIST_PLAYER */
	    if (!IS_NPC(ch) && IS_NPC(rch) 
            && (!IS_AFFECTED(rch, AFF_CHARM) || (rch->master && IS_PK(rch->master, victim)))
	    && IS_SET(rch->off_flags,ASSIST_PLAYERS)
            && !IS_OAFFECTED(rch, AFF_GHOST)
	    &&  rch->level + 6 > victim->level
	    && !IS_AFFECTED(rch, AFF_WIZI)
	    && is_same_group(ch, rch))
	    {
		do_emote(rch,"screams and attacks!");
		multi_hit(rch,victim,TYPE_UNDEFINED);
		continue;
	    }

	    /* PCs next */
	    if (!IS_NPC(ch) || IS_AFFECTED(ch,AFF_CHARM) || find_bilocated_body(ch) != NULL)
        {
            if (((!IS_NPC(rch) && IS_SET(rch->act,PLR_AUTOASSIST))
            ||     IS_AFFECTED(rch,AFF_CHARM)
            ||     find_bilocated_body(rch) != NULL) 
            && !IS_AFFECTED(rch,AFF_WIZI)
            &&   is_same_group(ch,rch) 
            &&   !is_safe(rch, victim))
            {
                if (IS_NPC(rch) && IS_SET(rch->nact, ACT_PACIFIC))
                set_fighting(rch, victim);
                else
                    multi_hit (rch,victim,TYPE_UNDEFINED);
            }
            
            continue;
        }
  	
	    /* now check the NPC cases */

	    if (IS_NPC(rch) && IS_NPC(ch) && !IS_AFFECTED(rch, AFF_CHARM)
	      && !IS_AFFECTED(rch, AFF_WIZI))
	    {
		assisted = FALSE;
	        for (i = 0; i < MAX_ASSIST_VNUM; i++)
		    if (rch->pIndexData->assist_vnum[i] == ch->pIndexData->vnum)
		    {
			do_emote(rch, "screams and attacks!");
			multi_hit(rch, victim, TYPE_UNDEFINED);
			assisted = TRUE;
			break;
		    }
		if (assisted)
		    continue;
	    }	    

 	    if (IS_NPC(ch) && !IS_AFFECTED(ch,AFF_CHARM))
	
        {
            if ((IS_NPC(rch) && IS_SET(rch->off_flags,ASSIST_ALL))
            ||   (IS_NPC(rch) && rch->group && rch->group == ch->group)
            ||   (IS_NPC(rch) && rch->race == ch->race 
               && (IS_SET(rch->off_flags,ASSIST_RACE) 
               || (IS_NPC(ch) && IS_SET(rch->off_flags, ASSIST_NPCRACE))))

            ||   (IS_NPC(rch) && IS_SET(rch->off_flags,ASSIST_ALIGN)
               &&   ((IS_GOOD(rch)    && IS_GOOD(ch))
                 ||  (IS_EVIL(rch)    && IS_EVIL(ch))
                 ||  (IS_NEUTRAL(rch) && IS_NEUTRAL(ch)))) 

            ||   (rch->pIndexData == ch->pIndexData 
               && IS_SET(rch->off_flags,ASSIST_VNUM))
               && !IS_AFFECTED(rch,AFF_WIZI))

            {
                CHAR_DATA *vch;
                CHAR_DATA *target;
                int number;

                if (number_bits(1) == 0)
                continue;
            
                target = NULL;
                number = 0;
                for (vch = ch->in_room->people; vch; vch = vch->next)
                {
                if (can_see(rch,vch)
                &&  is_same_group(vch,victim)
                &&  number_range(0,number) == 0)
                {
                    target = vch;
                    number++;
                }
                }

                if (target != NULL && !is_safe(rch, target))
                {
                do_emote(rch,"screams and attacks!");
                multi_hit(rch,target,TYPE_UNDEFINED);
                }
            }	
        }

	    if (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM) && !IS_AFFECTED(rch, AFF_WIZI))
        {
            if (ch->master->fighting != NULL)
                if (IS_AWAKE(ch))
                    if (!is_safe(ch->master->fighting, ch))
                        multi_hit(ch, ch->master->fighting, TYPE_UNDEFINED);
        }

	}
    }
}

int check_extra_damage( CHAR_DATA *ch, int dam, OBJ_DATA *wield )
{
    int diceroll;
    int exdam;

    exdam = dam;

    if ( get_skill(ch,gsn_enhanced_damage) > 0 )
    {
        diceroll = number_percent();
        if (diceroll <= get_skill(ch,gsn_enhanced_damage))
        {
            check_improve(ch,NULL,gsn_enhanced_damage,TRUE,6);
            exdam += number_fuzzy((exdam * (get_skill(ch,gsn_enhanced_damage)))/300);
        }

        if (is_affected(ch, gsn_prowess))
            exdam = round((exdam * 12) / 10);
    }
    else if ( is_affected(ch, gsn_prowess) )
        exdam = round((exdam * 14) / 10);	

    // Check for aspect of the inferno (up to 50% extra damage)
    AFFECT_DATA * paf(get_affect(ch, gsn_aspectoftheinferno));
    if (paf != NULL && paf->modifier == 0)
        exdam += ((exdam * (50 + (get_skill(ch, gsn_aspectoftheinferno) / 2))) / 200);

    // Check for shadowfiend (30% extra damage)
    if (is_affected(ch, gsn_shadowfiend))
        exdam += (exdam * 13) / 10;

    if ((get_skill(ch,gsn_pugilism) > 0) 
      && ((wield == NULL) || (IS_SET(wield->wear_flags,WEAR_HANDS))))
    {
        diceroll = number_percent();
        if (diceroll <= get_skill(ch,gsn_pugilism))
        {
            check_improve(ch,NULL,gsn_pugilism,TRUE,4);
            exdam += number_fuzzy((exdam * (get_skill(ch,gsn_pugilism)))/450);
        }
    }

    if (is_affected(ch, gsn_lesserhandwarp) || is_affected(ch, gsn_handwarp) || is_affected(ch, gsn_greaterhandwarp)
	|| is_affected(ch, gsn_lycanthropy))
    {
    	exdam += number_fuzzy(round(exdam * 1.3));
    }
	    
    if ( get_skill(ch,gsn_brutal_damage) > 0 )
    {
        diceroll = number_percent();
        if (diceroll <= get_skill(ch,gsn_brutal_damage))
        {
            check_improve(ch,NULL,gsn_brutal_damage,TRUE,6);
            exdam += number_fuzzy((exdam * (get_skill(ch,gsn_brutal_damage)))/450);
        }
    }
    
    exdam -= dam;
    return exdam;
}

bool check_reach(CHAR_DATA *ch, CHAR_DATA *victim, int diff, int faillag)
{
    int engaged = num_engaged(victim, true);
    int chance;
    OBJ_DATA *weapon;
    
    if (engaged == 1)
	return TRUE;

    chance = URANGE(0, ((8 + (2 * diff)) * engaged) - ((3 - diff) * 10), 40 + (10 * diff));
// brazen: We only want to worry about reach     
    if (ch->fighting)
    {
        // Group leader's charisma plays in
        if (ch->leader != NULL)
        {
            if (ch->leader != ch)
                chance -= (get_curr_stat(ch->leader, STAT_CHR) - 18) / 2;

            // Reduced miss chance if the group leader passes a celestial tactician check
            if (number_percent() <= get_skill(ch->leader, gsn_celestialtactician))
            {
                chance -= ch->leader->level / 10;
                check_improve(ch->leader, victim, gsn_celestialtactician, true, 2);
            }

            if (ch->leader->class_num == global_int_class_fighter)
                chance -= ch->leader->level / 10;
        }

        // Now we're going to compare weapon length
        weapon = get_eq_char(ch, ITEM_WIELD);
        if (weapon == NULL)
            chance += 10;
        else
        {
            if (IS_WEAPON_STAT(weapon,WEAPON_TWO_HANDS) )
                chance -= 10;
            switch (weapon->value[0])
            {
                case WEAPON_WHIP: chance -= 10; break;
                case WEAPON_DAGGER:
                case WEAPON_KNIFE: chance += 10; break;
                case WEAPON_POLEARM: chance -= 10; break;
                case WEAPON_SPEAR: chance -= 5; break;
            }
        }
        weapon = get_eq_char(victim, ITEM_WIELD);
        if (weapon == NULL)
            chance -= 10;
        else
        {
            if (IS_WEAPON_STAT(weapon, WEAPON_TWO_HANDS))
                chance += 10;
            switch (weapon->value[0])
            {
                case WEAPON_WHIP: chance += 10; break;
                case WEAPON_DAGGER:
                case WEAPON_KNIFE: chance -= 10; break;
                case WEAPON_POLEARM: chance += 10; break;
                case WEAPON_SPEAR: chance += 5; break;
            }
        }

        // Adjust for oaths
        CHAR_DATA * oathHolder(Oaths::OathHolderFor(*ch));
        if (oathHolder != NULL && oathHolder->in_room == ch->in_room)
            chance -= 10;
        
        if (ch->class_num == global_int_class_fighter && (number_percent()<(get_skill(ch,gsn_reach_mastery)/2)))
        {
            chance = 0;
            check_improve(ch,NULL,gsn_reach_mastery,TRUE,4);
        }

        if (number_percent() <= chance)
        {
            send_to_char("You fail to reach your opponent in the fray.\n\r", ch);
            WAIT_STATE(ch, UMAX(ch->wait, faillag));
            return FALSE;
        }
    }
    return TRUE;
} 

int num_engaged(CHAR_DATA *ch, bool ignoreCharmed)
{
    CHAR_DATA *vch;
    int engaged = 0;

    if (!ch || !ch->in_room)
    	return 0;

    for (vch = ch->in_room->people; vch; vch = vch->next)
    {
        // Ignore those not fighting the target
        if (vch->fighting != ch)
            continue;
            
        // Ignore charmies, if so specified
        if (ignoreCharmed && IS_NPC(vch) && IS_AFFECTED(vch, AFF_CHARM))
            continue;

        ++engaged;
    }

    return engaged;
}

/*
 * Do one group of attacks.
 */
void multi_hit( CHAR_DATA *ch, CHAR_DATA *victim, int dt )
{
    OBJ_DATA *obj;
    OBJ_DATA *dual;
    int     chance;
    AFFECT_DATA *paf = NULL;
    CHAR_DATA *vch = NULL, *vch_next = NULL;

    if (ch->pcdata != NULL)
    {
	ch->pcdata->purchased_item = NULL;
	ch->pcdata->purchased_item_cost = 0;
    }

    /* no attacks for stunnies -- just a check */
    if (ch->position < POS_RESTING)
	return;

    /* lame hack, had to do it */
    if (ch->mercy_from)
    {
	stop_fighting_all(ch);
	switch_position(ch, POS_RESTING);
    }

    if (!IS_NPC(ch) && !IS_SET(ch->act, PLR_AUTOATTACK))
    {
	send_to_char("You refrain from attacking.\n\r", ch);
	return;
    }

    if (victim->song && (victim->song->type == gsn_auraoflight) 
      && can_be_affected(ch,gsn_auraoflight)
      && (number_range(1, 3) == 1))
    {
	act("You are stunned by the cascading lights around $N!", ch, NULL, victim, TO_CHAR);
	act("$n is stunned by the cascading lights around $N!", ch, NULL, victim, TO_NOTVICT);
	act("$n is stunned by your aura of light!", ch, NULL, victim, TO_VICT);
	WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));
	return;
    }

    // Check for diakinesis addition/removal
    if (is_affected(victim, gsn_diakinesis))
        affect_strip(victim, gsn_diakinesis);
    else if (number_percent() <= (get_skill(victim, gsn_diakinesis) * 3) / (is_an_avatar(victim) ? 12 : 4))
    {
        check_improve(victim, ch, gsn_diakinesis, true, 2);

        // Not currently in effect, so add it
        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.type     = gsn_diakinesis;
        af.level    = victim->level;
        af.location = APPLY_HIDE;
        af.modifier = number_range(1, 2);
        affect_to_char(victim, &af);

        act("You open a temporary portal of energy and step through it, reappearing behind $N!", victim, NULL, ch, TO_CHAR);
        act("$n opens a temporary portal of energy and steps through it, reappearing behind you!", victim, NULL, ch, TO_VICT);
        act("$n opens a temporary portal of energy and steps through it, reappearing behind $N!", victim, NULL, ch, TO_NOTVICT);
    }
    else
        check_improve(victim, ch, gsn_diakinesis, false, 2);

    // Handle rune of still
    unsigned int bonusCount;
    int lostAttackChance(Runes::InvokedCountHere(*ch, Rune::Still, bonusCount) * 15);
    while (number_percent() <= (lostAttackChance + static_cast<int>(bonusCount * 10)))
    {
        ++ch->lost_att;
        lostAttackChance /= 2;
    }

    // Check for triumphant shout
    AFFECT_DATA * triumph(get_affect(ch, gsn_triumphantshout));
    if (triumph != NULL && triumph->modifier > 0)
    {
        int attacks(number_range(1, UMIN(triumph->modifier, 2)));
        for (int i(0); i < attacks; ++i)
            one_hit(ch, victim, dt, HIT_PRIMARY);

        triumph->modifier -= attacks;
    }

    // Check for wreath of fear
    if (check_wreathoffear(*ch, *victim))
        return;

    // Check for deathly visage
    check_deathlyvisage(*ch, *victim);

    // Check for focus fury
    AFFECT_DATA * focusfury(get_affect(ch, gsn_focusfury));
    if (focusfury != NULL)
        focusfury->modifier = 0;

    if (IS_NPC(ch))
    {
	    mob_hit(ch,victim,dt);
    	return;
    }

    obj = get_eq_char(ch, WEAR_WIELD);
    dual = get_eq_char(ch, WEAR_DUAL_WIELD);

    // Check for vershak
    bool hasVershak = FALSE;
    if (!is_affected(ch, gsn_roaroftheexalted))
    {
        if (obj && obj_is_affected(obj, gsn_blade_of_vershak))
        {
            one_hit(ch, victim, dt, HIT_PRIMARY);
            hasVershak = TRUE;
        }
        if (!hasVershak && dual && obj_is_affected(dual, gsn_blade_of_vershak))
        {
            one_hit(ch, victim, dt, HIT_PRIMARY);
            hasVershak = TRUE;
        }
    }

    one_hit( ch, victim, dt, HIT_PRIMARY, false);
    check_shieldslam(ch,victim);
    check_snakebite(ch,victim);
    if (ch->in_room->sector_type == SECT_DESERT
      && is_affected(victim,gsn_dirt)
      && get_skill(ch,gsn_terrainlore) > 0)
	if (number_percent() < get_skill(ch,gsn_terrainlore)/2)
	{
	    act("You attack $N again with your offhand while $E is unable to defend $Mself!",ch,NULL,victim,TO_CHAR);
	    one_hit(ch,victim, dt, HIT_DUAL, false);
	    check_improve(ch,victim,gsn_terrainlore,TRUE,1);
	}
    if (IS_NAFFECTED(ch,AFF_FURY))
	if ((paf = affect_find(ch->affected,gsn_fury)) != NULL)
	    if (paf->modifier > 0)
	    {
		vch = ch->in_room->people;
		do
		{
		    vch_next=vch->next;
		    if (vch != victim && vch->fighting == ch)
			one_hit(ch,vch,TYPE_UNDEFINED,HIT_PRIMARY);
		    vch = vch_next;
		} while (vch);
		if (--paf->modifier == 0)
		{
		    send_to_char("Your focused fury ends.\n\r",ch);
		    REMOVE_BIT(ch->naffected_by,AFF_FURY);
		}
	    }
    if (IS_PAFFECTED(ch,AFF_TWOHAND))
	if (number_percent() < get_skill(ch,gsn_rend))
	{
	    one_hit(ch,victim,TYPE_UNDEFINED,HIT_PRIMARY);
	    check_improve(ch,victim,gsn_rend,TRUE,4);
	}
	else
	    check_improve(ch,victim,gsn_rend,FALSE,4);

    if ( dt == gsn_backstab || dt == gsn_dualbackstab )
	return;

    if ((ch->fighting != victim) && !is_affected(ch, gsn_form_of_the_whirlwind)
	&& !IS_NAFFECTED(ch, AFF_FURY))
	return;

    bool hasClockwork(ch->in_room != NULL && is_clockworkgolem_present(*ch->in_room));
    if (IS_AFFECTED(ch,AFF_HASTE) && !hasClockwork)
    {
        one_hit(ch, victim, dt, HIT_PRIMARY);
        if (!IS_NPC(ch) && number_bits(3) == 0) 
            one_hit(ch, victim, dt, HIT_PRIMARY);
    }

    // Check for luck
    if (Luck::CheckOpposed(*ch, *victim) == Luck::Lucky)
        one_hit(ch, victim, dt, HIT_PRIMARY);

    // Check for conduit of the skies
    if (is_affected(ch, gsn_conduitoftheskies))
    {
        one_hit(ch, victim, dt, HIT_PRIMARY);
        for (unsigned int i(0); i < 8 && number_percent() <= 40; ++i)
            one_hit(ch, victim, dt, HIT_PRIMARY);
    }
    
    // Check for ethereal brethren
    AFFECT_DATA * brethren(get_affect(ch, gsn_etherealbrethren));
    if (brethren != NULL && number_percent() <= 15 && brethren->duration >= 1)
    {
        brethren->duration -= 1;
        send_to_char("With a quickness borne of experiences not your own, you strike an extra time!\n", ch);
        one_hit(ch, victim, dt, HIT_PRIMARY, false);
    }

    // Check for living flame specially; yes this is a hack to support WreathOfFlames (greater)
    // Note that living flame's normal effect is handled by the HASTE check above
    AFFECT_DATA * livingFlameAffect = get_affect(ch, gsn_livingflame);
    if (livingFlameAffect != NULL && livingFlameAffect->point != NULL && number_percent() <= 50)
        one_hit(ch, victim, dt, HIT_PRIMARY);

    // Check for oriflamme
    AFFECT_DATA * oriflammeAffect(get_room_affect(ch->in_room, gsn_oriflamme));
    if (oriflammeAffect != NULL && verify_char_room(static_cast<CHAR_DATA*>(oriflammeAffect->point), ch->in_room))
    {
        // Oriflamme is present, as is its creator, so check for group
        if (is_same_group(ch, static_cast<CHAR_DATA*>(oriflammeAffect->point)))
            one_hit(ch, victim, dt, HIT_PRIMARY);
    }

    if (IS_PAFFECTED(ch,AFF_MAUL))
    {
	one_hit(ch, victim, dt, HIT_PRIMARY);
	check_improve(ch,victim,gsn_maul,TRUE,1);
    }

    chance = get_skill(ch,gsn_second_attack)/2;
    if (hasVershak) chance += 20;
    if (is_affected(ch,gsn_tendrilgrowth)) chance += 25;
    if (number_percent() < get_skill(ch,gsn_pugilism)
      && !get_eq_char(ch,WEAR_WIELD)) chance += 15; 
    chance += mod_by_enthusiasm(ch) + mod_by_absorb(ch);

    if (!hasClockwork && (IS_AFFECTED(ch,AFF_SLOW) || is_affected(ch, gsn_windbind) || is_affected(ch, gsn_plantentangle)))
    	chance /= 2;

    if (is_affected(ch, gsn_form_of_the_reed))
	    chance = 0;

    if (is_affected(ch, gsn_levitation))
    	chance = 0;

    if (is_affected(ch, gsn_physikersinstinct))
        chance = 0;

    int avatarType(type_of_avatar(ch));
    if (avatarType == gsn_avatar) chance = UMAX(chance, 80);
    else if (avatarType == gsn_avatarofthelodestar || avatarType == gsn_avataroftheannointed) chance = UMAX(chance, 75);
    else if (avatarType == gsn_avataroftheprotector) chance = UMAX(chance, 70);

    if ( number_percent( ) <= chance )
    {
	one_hit( ch, victim, dt, HIT_PRIMARY );
	check_improve(ch,victim,gsn_second_attack,TRUE,5);
	if ( ch->fighting != victim )
	    return;
    }

    chance = get_skill(ch,gsn_third_attack)/4;

    // Check for aspect of the inferno
    AFFECT_DATA * aspectAff(get_affect(ch, gsn_aspectoftheinferno));
    if (aspectAff != NULL && aspectAff->modifier == 0)
        chance = UMAX(chance, UMAX(75, get_skill(ch, gsn_aspectoftheinferno)) / 2);

    if (hasVershak) chance += 15;
    if (is_affected(ch,gsn_tendrilgrowth)) chance += 20;

    if (number_percent() < get_skill(ch,gsn_pugilism)
      && !get_eq_char(ch,WEAR_WIELD)) chance += 15; 

    if (ch->class_num == CLASS_FIGHTER)
	chance = round(chance * 1.5);
    
    chance += mod_by_enthusiasm(ch) + mod_by_absorb(ch);

    if (is_affected(ch, gsn_adrenaline_rush))
	chance = 50;

    if (!hasClockwork && (IS_AFFECTED(ch,AFF_SLOW) || is_affected(ch, gsn_windbind) || is_affected(ch, gsn_plantentangle)))
	chance = 0;

    if (is_affected(ch, gsn_form_of_the_reed))
	chance = 0;

    if (is_affected(ch, gsn_physikersinstinct))
        chance = 0;

    if (avatarType == gsn_avatar) chance = UMAX(chance, 65);
    else if (avatarType == gsn_avatarofthelodestar || avatarType == gsn_avataroftheannointed) chance = UMAX(chance, 55);
    else if (avatarType == gsn_avataroftheprotector) chance = UMAX(chance, 50);

    if ( number_percent( ) <= chance )
    {
	one_hit( ch, victim, dt, HIT_PRIMARY );
	check_improve(ch,victim,gsn_third_attack,TRUE,6);
	if ( ch->fighting != victim )
	    return;
    }

    chance = get_skill(ch,gsn_fourth_attack)/4;
	if (hasVershak) chance += 10;
		
    if (ch->class_num == CLASS_FIGHTER)
	chance = round(chance * 1.2);

    chance += mod_by_enthusiasm(ch) + mod_by_absorb(ch);

    if (!hasClockwork && (IS_AFFECTED(ch,AFF_SLOW)||is_affected(ch, gsn_windbind) || is_affected(ch, gsn_plantentangle)))
	chance = 0;

    if (is_affected(ch, gsn_form_of_the_reed) || is_affected(ch, gsn_form_of_the_monkey))
	chance = 0;

    if (is_affected(ch, gsn_physikersinstinct))
        chance = 0;

    if (avatarType == gsn_avatar) chance = UMAX(chance, 50);
    else if (avatarType == gsn_avatarofthelodestar || avatarType == gsn_avataroftheannointed) chance = UMAX(chance, 45);
    else if (avatarType == gsn_avataroftheprotector) chance = UMAX(chance, 40);

    if ( number_percent( ) <= chance )
    {
	one_hit( ch, victim, dt, HIT_PRIMARY );
	check_improve(ch,victim,gsn_fourth_attack,TRUE,6);
	if ( ch->fighting != victim )
	    return;
    }

    if (!IS_NAFFECTED(ch, AFF_BLINK) && is_affected(ch, gsn_form_of_the_phantasm) && (number_percent() <= (get_skill(ch, gsn_form_of_the_phantasm))))
    {
        act("You momentarily blink out of existence, reappearing behind $N!", ch, NULL, victim, TO_CHAR);
        act("$n blinks out of existence, reappearing behind you!", ch, NULL, victim, TO_VICT);
        act("$n blinks out of existence, reappearing behind $N!", ch, NULL, victim, TO_NOTVICT);
        SET_BIT(ch->naffected_by, AFF_BLINK);
        one_hit(ch, victim, dt, HIT_PRIMARY, false);
        REMOVE_BIT(ch->naffected_by, AFF_BLINK);
    }

    ch->lost_att = 0;
}

/* procedure for all mobile attacks */
void mob_hit(CHAR_DATA *ch, CHAR_DATA *victim, int dt)
{
    int chance,number;
    CHAR_DATA *vch, *vch_next;

    if (IS_SET(ch->nact, ACT_PACIFIC) && (victim->fighting != ch))
	return;

    // Check for disbelieve
    if (IS_NPC(victim) && IS_SET(victim->act, ACT_ILLUSION) && !IS_SET(ch->act, ACT_ANIMAL)
    && !IS_SET(ch->act, ACT_PET) && !IS_SET(ch->act, ACT_UNDEAD) && !IS_SET(ch->act, ACT_ILLUSION))
    {
        if (number_percent() <= (race_table[ch->race].pc_race ? 3 : 1))
            tryDisbelieve(ch, victim);
    }

    // Check for reducing phantasm cooldown
    if (number_bits(3) == 0)
        PhantasmInfo::checkReduceCooldown(*ch);

    // Check phantasm traits for per-round actions
    // Check the venomous trait for poison
    if (PhantasmInfo::traitCount(*ch, PhantasmTrait::Venomous) > 0 && number_percent() <= 5)
    {
        act("$n bites viciously at you, slavering messily!", ch, NULL, victim, TO_VICT);
        act("$n bites viciously at $N, slavering messily!", ch, NULL, victim, TO_NOTVICT);

        if (saves_spell(ch->level, ch, victim, DAM_POISON))
            send_to_char("You feel queasy, but shake it off.\n", victim);
        else
        {
            send_to_char("A sickening feeling spreads through you, leaving you nauseous.\n", victim);
            act("$n looks very ill.", victim, NULL, NULL, TO_ROOM);

            AFFECT_DATA af = {0};
            af.where    = TO_AFFECTS;
            af.type     = gsn_poison;
            af.level    = ch->level;
            af.duration = 4;
            af.location = APPLY_STR;
            af.modifier = -2;
            af.bitvector = AFF_POISON;
            affect_join(victim, &af);
        }
    }

    // Check the brute trait for bashing
    if (PhantasmInfo::traitCount(*ch, PhantasmTrait::Brute) > 0 && number_percent() <= 5)
        do_bash(ch, "");

    // Check the drake trait for bashing
    if (number_percent() <= Drakes::SpecialCount(*ch, Drakes::Bash))
        do_bash(ch, "");

    // Check the drake trait for diamondskin
    if (number_percent() <= Drakes::SpecialCount(*ch, Drakes::Diamondskin) && !is_affected(ch, gsn_diamondskin))
    {
        // Apply immunity
        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.type     = gsn_diamondskin;
        af.level    = ch->level;
        af.duration = 10;
        affect_to_char(ch, &af);
       
        // Apply cooldown
        af.where     = TO_IMMUNE;
        af.duration  = 1;
        af.bitvector = IMM_WEAPON;
        affect_to_char(ch, &af);
 
        act("You snort angrily, and a diamond carapace fuses over your stony flesh!", ch, NULL, NULL, TO_CHAR);
        act("$n snorts angrily, and a diamond carapace fuses over $s stony flesh!", ch, NULL, NULL, TO_ROOM);
    }

    // Check the drake trait for sandspray
    if (number_percent() <= Drakes::SpecialCount(*ch, Drakes::Sandman))
    {
        act("Your stony maw gapes wipe, and you breathe out a blast of stinging sand!", ch, NULL, victim, TO_CHAR);
        act("$n's stony maw gapes wipe, and $e breathes out a blast of stinging sand!", ch, NULL, victim, TO_ROOM);

        int sandDamage(dice(ch->level, 3));
        if (saves_spell(ch->level, ch, victim, DAM_SLASH))
            sandDamage /= 2;

        damage_old(ch, victim, sandDamage, gsn_sandspray, DAM_SLASH, true);

        // Check for blinding
        if (IS_VALID(victim) && !IS_OAFFECTED(victim, AFF_GHOST) && !IS_SET(victim->imm_flags, IMM_BLIND) 
        && !IS_AFFECTED(victim, AFF_BLIND) && !saves_spell(ch->level, ch, victim, DAM_OTHER) && number_bits(1) == 0)
        {
            act ("$n is blinded by the sand in $s eyes!", victim, NULL, NULL, TO_ROOM);
            act ("You are blinded by the sand in your eyes!", victim, NULL, NULL, TO_CHAR);

            AFFECT_DATA af = {0};
            af.where     = TO_AFFECTS;
            af.type      = gsn_sandspray;
            af.level     = ch->level;
            af.location  = APPLY_HITROLL;
            af.modifier  = -4;
            af.duration  = 1;
            af.bitvector = AFF_BLIND;
            affect_to_char(victim, &af);
        }
    }

    // Check the drake trait for biting
    if (number_percent() <= Drakes::SpecialCount(*ch, Drakes::Biting))
    {
        act("You lunges forward, snapping its stony jaws and tearing a gash in $N's skin!", ch, NULL, victim, TO_CHAR);
        act("$n lunges forward, snapping its stony jaws and tearing a gash in $N's skin!", ch, NULL, victim, TO_NOTVICT);
        act("$n lunges forward, snapping its stony jaws and tearing a gash in your skin!", ch, NULL, victim, TO_VICT);

        AFFECT_DATA af = {0};
        af.where    = TO_OAFFECTS;
        af.type     = gsn_gash;
        af.modifier = -1;
        af.duration = 4;
        af.bitvector = AFF_BLEEDING;
        affect_to_char(victim, &af);
    }

    // Check for zombie/wight specials
    if (IS_NPC(ch))
    {
        if (ch->pIndexData->vnum == MOB_VNUM_BARROWMISTZOMBIE && number_bits(4) == 0)
        {
            // Zombies bites can cause poison
            act("$N lunges forward, biting at you with yellowed teeth!", victim, NULL, ch, TO_CHAR);
            act("$N lunges forward, biting at $n with yellowed teeth!", victim, NULL, ch, TO_ROOM);
            damage_old(ch, victim, number_range(1, 5), gsn_barrowmist, DAM_PIERCE, true);
            if (!IS_AFFECTED(victim, AFF_POISON) && !saves_spell(ch->level, ch, victim, DAM_POISON))
            {
                AFFECT_DATA af = {0};
                af.where    = TO_AFFECTS;
                af.type     = gsn_poison;
                af.level    = ch->level;
                af.duration = af.level / 8;
                af.location = APPLY_STR;
                af.modifier = -2;
                af.bitvector = AFF_POISON;
                affect_to_char(victim, &af);

                send_to_char("You feel very sick.\n", victim);
                act("$n looks very ill.", victim, NULL, NULL, TO_ROOM);
            }
        }
        
        if (ch->pIndexData->vnum == MOB_VNUM_BARROWMISTWIGHT && number_bits(5) == 0)
        {
            // Wights can zap with the drain spell
            act("$N hisses spitefully, reaching for you!", victim, NULL, ch, TO_CHAR);
            act("$N hisses spitefully, reaching for $n!", victim, NULL, ch, TO_ROOM);
            spell_drain(gsn_drain, ch->level, ch, victim, TARGET_CHAR);
        }
    }

    // Check the selfless trait for auto-rescue
    if (ch->master != NULL && static_cast<unsigned int>(number_percent()) <= (PhantasmInfo::traitCount(*ch, PhantasmTrait::Selfless) * 15))
        do_rescue_helper(ch, ch->master);

    // Check the firebreather trait for fireballing
    if (ch->in_room != NULL && PhantasmInfo::traitCount(*ch, PhantasmTrait::Firebreather) > 0 && number_percent() <= 5)
    {
        act("$n draws in air, then breathes forth a blast of fire!", ch, NULL, NULL, TO_ROOM);
        int baseDam(30 + (ch->level - 1) * 2);
        CHAR_DATA * target_next;
        for (CHAR_DATA * target(ch->in_room->people); target != NULL; target = target_next)
        {
            target_next = target->next_in_room;

            if (is_same_group(target, ch) || IS_AFFECTED(target, AFF_WIZI) || IS_OAFFECTED(target, AFF_GHOST)
            || (ch->master != NULL && is_safe_spell(ch->master, target, true)))
                continue;

            int dam(number_range(baseDam / 2, baseDam * 2));
            if (saves_spell(ch->level, ch, target, DAM_FIRE))
                dam /= 2;

            damage_old(ch, target, dam, gsn_fire_breath, DAM_FIRE, true);
        }
    }

    // Check the nightmarish trait for forcing flee
    if (PhantasmInfo::traitCount(*ch, PhantasmTrait::Nightmarish) > 0 && number_percent() <= 5)
    {
        act("$n rises up, displaying $s nightmarish form in all its terrifying glory!", ch, NULL, NULL, TO_ROOM);
        if (!saves_spell(ch->level, ch, victim, DAM_FEAR)
        && (!IS_NPC(victim) || (!IS_SET(victim->act, ACT_NOSUBDUE) && !IS_SET(victim->act, ACT_SENTINEL) && !IS_SET(victim->nact, ACT_SHADE))))
        {
            send_to_char("You panic from fear!\n", victim);
            do_flee(victim, "");
        }
    }
    
    // Check for clockwork golem
    if (ch->pIndexData->vnum == MOB_VNUM_CLOCKWORKGOLEM)
    {
        // Number of attacks is high but drops as it gets hurt
        unsigned int attackCount(1 + ((ch->level * ch->hit) / (5 * ch->max_hit)));
        for (unsigned int i(0); i < attackCount; ++i)
        {
            one_hit(ch, victim, dt, HIT_PRIMARY, false);
            if (ch->fighting != victim)
                return;
        }
        return;
    }

    one_hit(ch,victim,dt,HIT_PRIMARY, false);
    if (ch->fighting != victim)
        return;

    /* Area attack -- BALLS nasty! */
 
    if (IS_SET(ch->off_flags,OFF_AREA_ATTACK))
    {
        for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
        {
            vch_next = vch->next;
            if ((vch != victim && vch->fighting == ch))
            one_hit(ch,vch,dt,HIT_PRIMARY);
        }
    }

    if (ch->fighting != victim || dt == gsn_backstab || dt == gsn_dualbackstab)
        return;

    bool hasClockwork(ch->in_room != NULL && is_clockworkgolem_present(*ch->in_room));
    if (IS_AFFECTED(ch,AFF_HASTE) && !hasClockwork)
        one_hit(ch,victim,dt,HIT_PRIMARY);

    // Check for luck
    if (Luck::CheckOpposed(*ch, *victim) == Luck::Lucky)
        one_hit(ch, victim, dt, HIT_PRIMARY);

    // Check for drake frantic to do extra hits as the drake is hurt more
    int franticCount(Drakes::SpecialCount(*ch, Drakes::Frantic));
    if (franticCount > 0)
    {
        int percInjured(100 - ((ch->hit * 100) / ch->max_hit));
        percInjured = UMAX(0, percInjured);
        while (number_percent() <= (percInjured * franticCount))
        {
            one_hit(ch, victim, dt, HIT_PRIMARY);
            percInjured /= 2;
        }
    }

    chance = get_skill(ch,gsn_second_attack)/2;

    if (!hasClockwork && (IS_AFFECTED(ch,AFF_SLOW) || is_affected(ch, gsn_windbind) || is_affected(ch, gsn_plantentangle)) && !IS_SET(ch->off_flags,OFF_FAST))
	chance /= 2;

    if (is_an_avatar(ch))
	    chance = 80;

    if (number_percent() < chance)
    {
	one_hit(ch,victim,dt,HIT_PRIMARY);
	if (ch->fighting != victim)
	    return;
    }

    chance = get_skill(ch,gsn_third_attack)/4;

    if (!hasClockwork && (IS_AFFECTED(ch,AFF_SLOW) || is_affected(ch, gsn_windbind) || is_affected(ch, gsn_plantentangle)) && !IS_SET(ch->off_flags,OFF_FAST))
	chance = 0;

    if (ch->class_num == CLASS_FIGHTER)
	chance = round(chance * 1.5);

    if (is_an_avatar(ch))
	    chance = 60;

    if (number_percent() < chance)
    {
	one_hit(ch,victim,dt,HIT_PRIMARY);
	if (ch->fighting != victim)
	    return;
    } 

    /* oh boy!  Fun stuff! */

    if (ch->wait > 0)
	return;

    number = number_range(0,2);

    /* now for the skills */

    number = number_range(0,10);

    switch(number) 
    {
    case (0) :
	if (IS_SET(ch->off_flags,OFF_BASH))
	    do_bash(ch,"");
	break;

    case (1) :
	break;


    case (2) :
	if (IS_SET(ch->off_flags,OFF_DISARM) 
	|| (get_weapon_sn(ch) != gsn_hand_to_hand 
	&& (IS_SET(ch->act,ACT_WARRIOR)
   	||  IS_SET(ch->act,ACT_THIEF))))
	    do_disarm(ch,"");
	break;

    case (3) :
	if (IS_SET(ch->off_flags,OFF_KICK))
	    do_kick(ch,"");
	break;

    case (4) :
	if (IS_SET(ch->off_flags,OFF_KICK_DIRT))
	    do_dirt(ch,"");
	break;

    case (6) :
	if (IS_SET(ch->off_flags,OFF_TRIP))
	    do_trip(ch,"");
	break;

    case (7) :
	if (IS_SET(ch->off_flags,OFF_CRUSH))
	{
	    /* do_crush(ch,"") */ ;
	}
	break;
    case (8) :
	if (IS_SET(ch->off_flags,OFF_BACKSTAB))
	{
	    do_backstab(ch,"");
	}
	break;
    case (9) :
	break;

    case (10):
	if (IS_SET(ch->off_flags,OFF_THROW))
	{
	    do_throw(ch,"1.");
	}
	break;

    case (11):
	break;

    }
}
	
void uberskillgetsuberer(CHAR_DATA *ch, CHAR_DATA *victim,int htype,OBJ_DATA *weapon)
{
    int x=0;

    bool found = FALSE;

    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA *paf;

    if (htype == HIT_PRIMARY && victim->level >= ch->level)
    {
        if (!IS_NPC(ch) && (ch->class_num == global_int_class_swordmaster
	 && weapon && weapon->value[0] == WEAPON_SWORD
	 && ch->pcdata->learned[gsn_sword] == 100)) 
        {
            if (ch->pcdata->learned[gsn_favoredblade] > 20)
	    {
	        if (number_percent() < get_skill(ch, gsn_favoredblade))
	        {
	            check_improve(ch,victim,gsn_favoredblade,TRUE,10);
	            if (number_bits(11) == 0)
		    {
		        for (x = 0;x < MAX_SWORDS;x++)
		        {
		            if (ch->pcdata->favored_vnum[x] == weapon->pIndexData->vnum)
		            {
			        if (number_bits(ch->pcdata->favored_bonus[x] / 33) == 0)
			        {
				    ch->pcdata->favored_bonus[x]++;
		                    sprintf(buf,"You feel more attuned to %s.\n\r",weapon->short_descr);
                 		    send_to_char(buf,ch);

   				}
		                found = TRUE;
	                    }
		        }

		        if (!found)
		        {
		            for (x = 0;x < MAX_SWORDS;x++)
		            {
		                if (ch->pcdata->favored_vnum[x] == 0)
		                {
			            ch->pcdata->favored_vnum[x] = weapon->pIndexData->vnum;
			            ch->pcdata->favored_bonus[x] = 1;
		                    sprintf(buf,"You feel more attuned to %s.\n\r",weapon->short_descr);
		                    send_to_char(buf,ch);
			            break;
		                }
		            }
		        }
		    }
	        }
	        else
	            check_improve(ch,victim,gsn_favoredblade,FALSE,6);
            }
        }

        if (!IS_NPC(ch) && (ch->class_num == global_int_class_earthtemplar
	 && weapon && obj_is_affected(weapon, gsn_devotion)))
        {
	    if (get_high_modifier(weapon->affected, gsn_devotion) != ch->id)
	        object_affect_strip(weapon, gsn_devotion);
	    else
	    {
	        AFFECT_DATA *hmod = NULL, *dmod = NULL;

	        for (paf = weapon->affected; paf; paf = paf->next)
		    if (paf->type == gsn_devotion)
		        if (paf->location == APPLY_HITROLL)
			    hmod = paf;
		        else if (paf->location == APPLY_HIT)
			    dmod = paf;

	        if (number_bits(10) == 0)
	        {
		    AFFECT_DATA af;
		    af.valid = TRUE;
 		    af.point = NULL;

	            for (x = 0;x < MAX_SWORDS;x++)
		    {
		        if (ch->pcdata->favored_vnum[x] == weapon->pIndexData->vnum)
		        {
			    if (number_bits(ch->pcdata->favored_bonus[x] / 25) == 0)
			        ch->pcdata->favored_bonus[x]++;

		            found = TRUE;
	                }
		    }

		    if (!found)
		    {
		        for (x = 0;x < MAX_SWORDS;x++)
		        {
		            if (ch->pcdata->favored_vnum[x] == 0)
		            {
			        ch->pcdata->favored_vnum[x] = weapon->pIndexData->vnum;
			        ch->pcdata->favored_bonus[x] = 1;
				found = TRUE;
		  	        break;
		            }
		        }
		    }

		    if (found && number_bits(1) == 0)
		    {
		        if (hmod)
			{
			    hmod->modifier++;
			    ch->hitroll += 1;
			}
		        else
		        {
			    af.where     = TO_OBJECT;
			    af.type	     = gsn_devotion;
			    af.level     = ch->level;
			    af.duration  = -1;
			    af.location  = APPLY_HITROLL;
			    af.modifier  = 1;
			    af.bitvector = 0;
			    obj_affect_join(weapon, &af);
			    ch->hitroll += 1;
		        }
		    }
		    else
		    {
		        if (dmod)
			{
			    dmod->modifier += 1;
			    ch->hit += 1;
			}
		        else
		        {
			    af.where     = TO_OBJECT;
			    af.type	 = gsn_devotion;
			    af.level     = ch->level;
			    af.duration  = -1;
			    af.location  = APPLY_HIT;
			    af.modifier  = 1;
			    af.bitvector = 0;
			    obj_affect_join(weapon, &af);
			    ch->hit += 1;
		        }
		    }
		    sprintf(buf,"You feel more attuned to %s.\n\r",weapon->short_descr);
		    send_to_char(buf,ch);
	        }
	    }
        }
    }
}
/*
 * Hit one guy once.
 */
void one_hit(CHAR_DATA *ch, CHAR_DATA *victim, int dt, int htype, bool canLoseAttack)
{
    OBJ_DATA *wield = NULL, *floatweapon= NULL, *dualwield = NULL, *weapon = NULL;
    int sn = -1;
    int dam, x;
    int victim_ac;
    int diceroll;
    int thac0, thac0_00, thac0_32;
    int skill;
    int chance;
    int dam_type;
    int hf;
    int type;
    bool result;
    AFFECT_DATA *paf;
    bool hit_opp = FALSE;
    /* Step 1: the required various sanity checks */

    if (victim == ch || ch == NULL || victim == NULL || victim->position == POS_DEAD || ch->in_room != victim->in_room)
	return;

    if (!IS_NPC(ch) && !IS_SET(ch->act, PLR_AUTOATTACK))
    {
	send_to_char("You refrain from attacking.\n\r", ch);
	return;
    }

    if (ch->mercy_from || (ch->mercy_to == victim))
    {
	stop_fighting_all(ch);
	switch_position(ch, POS_RESTING);
    }

    // Check for lost attacks
    if (canLoseAttack && ch->lost_att > 0)
    {
        --ch->lost_att;
    	return;
    }

    /* Step 2: extra hit checks */
    floatweapon = get_eq_char(ch, WEAR_FLOAT);

    if (htype == HIT_PRIMARY)
    {
        if (floatweapon)
        {
            if (floatweapon->item_type == ITEM_WEAPON)
            {
                one_hit(ch, victim, TYPE_FLOAT, HIT_FLOAT);

                // Check for call upon wind
                std::pair<Direction::Value, int> windInfo(checkWind(ch));
                AFFECT_DATA * call(get_affect(ch, gsn_calluponwind));
                if (windInfo.second > 0 && call != NULL && call->modifier == Direction::South)
                {
                    // Chance of bonus floating weapon attack
                    if (number_percent() <= 20 + (windInfo.second / (windInfo.first == Direction::South ? 4 : 10)))
                        one_hit(ch, victim, TYPE_FLOAT, HIT_FLOAT);
                }
            }
        }

        if (is_affected(ch, gsn_form_of_the_mongoose) && number_percent() < get_skill(ch,gsn_form_of_the_mongoose) / 3)
            one_hit(ch, victim, dt, HIT_PRIMARY);
    }

    /* Step 3: post-fun sanity checks again */

    if (victim == ch || ch == NULL || victim == NULL || victim->position == POS_DEAD || ch->in_room != victim->in_room)
	return;

    if (ch->mercy_from || (ch->mercy_to == victim))
    {
	stop_fighting_all(ch);
	switch_position(ch, POS_RESTING);
    }

    /* Step 4: pre-attack checks */

    if (is_affected(ch, gsn_shadowmastery) && victim->fighting && (victim->fighting == ch))
    {
	send_to_char("You step out of the concealing shadows.\n\r", ch);
	affect_strip(ch, gsn_shadowmastery);
	REMOVE_BIT(ch->oaffected_by, AFF_SHADOWMASTERY);
    }

    if (!check_reach(ch, victim, REACH_NORMAL, 0))
	return;

    wield = get_eq_char(ch, WEAR_WIELD);
    dualwield = get_eq_char(ch, WEAR_DUAL_WIELD);

    // If you're not holding a weapon in your main hand, there's no way you
    // can have a primary or dual attack.
    // Unless, of course, you are awesome enough to be smacking people with
    // tendrils of Gamaloth
    if (!wield && (htype != HIT_FLOAT) && (hands_free(ch) == 0 && !is_affected(ch,gsn_tendrilgrowth)))
	return;

    // No dual hits for two-handed weapons.   
    if ((htype == HIT_DUAL) 
      && (((wield != NULL) 
	&& IS_SET(wield->value[4], WEAPON_TWO_HANDS) 
	&& (ch->size < SIZE_LARGE)) 
      || IS_PAFFECTED(ch,AFF_TWOHAND)))
	return;

    switch (htype)
    {
	case HIT_PRIMARY:	weapon = wield;		break;
	case HIT_DUAL:		weapon = dualwield;	break;
	case HIT_FLOAT:		weapon = floatweapon;	break;
	case HIT_OPPORTUNITY:  weapon = wield; htype = HIT_PRIMARY; hit_opp = TRUE; 	break;
    }

    if (!IS_NPC(ch) && weapon != NULL
      && ((htype == HIT_PRIMARY 
	&& (get_skill(ch,gsn_augmented_strength) ?
	  get_obj_weight(weapon) > (str_app[URANGE(0,get_curr_stat(ch,STAT_STR)+3,25)].wield*10) :
	  get_obj_weight(weapon) > (str_app[get_curr_stat(ch,STAT_STR)].wield*10)))
        || (htype == HIT_DUAL && dualwield != NULL && !can_use_dual(ch, wield, dualwield))))
    {
        act("You lack the strength to swing $p.", ch, weapon, NULL, TO_CHAR);
        return;
    }

    if (weapon && (number_bits(6) == 0) && (paf = affect_find(ch->affected, gsn_grease))
     && paf->modifier >= 3)
    {
	int drop_chance = 100;
	OBJ_DATA *rearm;

	drop_chance -= (get_weapon_skill(ch,get_weapon_sn(ch),TRUE) / 4);
	drop_chance -= get_curr_stat(ch, STAT_DEX);

	if (is_affected(ch,gsn_clumsiness))
	    drop_chance += 10;

	if (is_affected(ch, gsn_grip))
	    drop_chance /= 2;

	if (obj_is_affected(weapon, gsn_bindweapon))
	    drop_chance /= 2;

	if (number_percent() < drop_chance)
	{
	    act("$p slips from your greased palm!", ch, weapon, NULL, TO_CHAR);
	    act("$p slips from $n's greased palm!", ch, weapon, NULL, TO_ROOM);

	    oprog_remove_trigger(weapon);

	    obj_from_char( weapon );
	    if ( IS_OBJ_STAT(weapon,ITEM_NODROP) || IS_OBJ_STAT(weapon,ITEM_INVENTORY) )
		obj_to_char( weapon, ch );
	    else
	    {
		obj_to_room( weapon, ch->in_room );
		if (IS_NPC(ch) && ch->wait == 0 && can_see_obj(ch,weapon))
		    get_obj(ch,weapon,NULL);
	    }

	    if (ch->class_num == global_int_class_thief)
	    {
		if ((rearm = get_eq_char(victim, WEAR_CONCEAL1)) == NULL)
		    if ((rearm = get_eq_char(victim, WEAR_CONCEAL2)) == NULL)
			return;

		act("$n slips $p from $s sleeve, rearming $mself!", ch, rearm, NULL, TO_ROOM);
		act("You slip $p from your sleeve, rearming yourself!", ch, rearm, NULL, TO_CHAR);
		unequip_char(ch, rearm);
		equip_char(ch, rearm, WORN_WIELD);
	    }

	    return;
	}
    }

    if (is_affected(victim, gsn_form_of_the_cyclone) && htype != HIT_FLOAT && (number_percent() <= (get_skill(victim, gsn_form_of_the_cyclone))))
    {
        // Build the list of candidate targets
        std::vector<CHAR_DATA*> targets;
        for (CHAR_DATA * vch = ch->in_room->people; vch; vch = vch->next_in_room)
        {
            if (vch != ch && vch != victim && (is_same_group(vch, ch) || vch->fighting == victim))
               targets.push_back(vch);
        }

        int chance(20 + (targets.size() * 10));
        if (!targets.empty() && number_percent() <= UMIN(chance, 50))
        {
            // Choose one at random to hit
            CHAR_DATA * target(targets[number_range(0, targets.size() - 1)]);
            act("$N's deceptive spinning and whirling causes you to strike another!", ch, NULL, victim, TO_CHAR);
            act("You dart about in the form of the cyclone, causing $n to strike another!", ch, NULL, victim, TO_VICT);
            act("$N darts abouts, $S deceptive form causing $n's strike to go awry!", ch, NULL, victim, TO_NOTVICT);
            one_hit(ch, target, TYPE_UNDEFINED, htype);
            return;
        }
    }

    if (IS_OAFFECTED(ch, AFF_CONSUMPTION))
    {
        CHAR_DATA *nch, *nch_next;

        act("The ritual is interrupted as $n begins to fight.", ch, NULL, NULL, TO_ROOM);
        send_to_char("The ritual ends as you begin to fight.\n\r", ch);
        for (nch = ch->in_room->people; nch; nch = nch_next)
        {
            nch_next = nch->next_in_room;

            affect_strip(nch, gsn_consumption);
            if (IS_NPC(nch)
             && (nch->pIndexData->vnum == MOB_VNUM_SILVER_VEIL))
                extract_char(nch, TRUE);
        }
        affect_strip(ch, gsn_consumption);
        silver_state = 0;
    }

    if (is_affected(ch, gsn_bladebarrier))
    {
	CHAR_DATA *vch;

	for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
	    if (is_same_group(vch, ch) && is_affected(vch, gsn_bladebarrier))
	    {
		if (vch == ch)
		    send_to_char("Your blade barrier dissipates as you attack.\n\r", vch);
		else
		    act("Your blade barrier dissipates as $n attacks.", ch, NULL, vch, TO_VICT);
		affect_strip(vch, gsn_bladebarrier);
	    }
    }

    if (is_affected(victim, gsn_bladebarrier))
    {
	CHAR_DATA *vch;

	act("The whirring blades surrounding $N tear into you!", ch, NULL, victim, TO_CHAR);
	act("The whirring blades surrounding you tear into $n!", ch, NULL, victim, TO_VICT);
	act("The whirring blades surrounding $N tear into $n!", ch, NULL, victim, TO_NOTVICT);
	damage(victim, ch, number_fuzzy(victim->level * 3), gsn_bladebarrier, DAM_SLASH, TRUE);

	for (vch = victim->in_room->people; vch; vch = vch->next_in_room)
	    if (is_same_group(vch, victim) && is_affected(vch, gsn_bladebarrier))
	    {
		act("Your blade barrier dissipates as $n breaks through.", ch, NULL, vch, TO_VICT);
		affect_strip(vch, gsn_bladebarrier);
	    }
    }	

    /* Step 5: I really need to consolidate these... */

    if (victim == ch || ch == NULL || victim == NULL || victim->position == POS_DEAD || ch->in_room != victim->in_room)
	return;

    if (ch->mercy_from || (ch->mercy_to == victim))
    {
	stop_fighting_all(ch);
	switch_position(ch, POS_RESTING);
    }

    /* Step 5.5: Check for weapon wear */
    if (weapon && !IS_SET(weapon->extra_flags[0], ITEM_NODESTROY)
     && ((number_range(1, 300 * ch->level / UMAX(1, UMIN(weapon->level, 51))) == 1)
     || (obj_is_affected(weapon, gsn_corrosion) && number_bits(1) == 0)))
    {
        if (IS_NPC(ch)
          || !BIT_GET(ch->pcdata->traits, TRAIT_BLACKSMITH)
          || !material_table[weapon->material].metal
          || number_bits(2) == 0)
        {
            if (get_skill(ch,gsn_grace) * 3 / 4 < number_percent())
            {    
                weapon->condition--;
                check_improve(ch,victim,gsn_grace,FALSE,1);
            }
            else
                check_improve(ch,victim,gsn_grace,TRUE,1);
        }

        if (weapon->condition <= 0)
        {
            send_to_char("Your weapon breaks from wear as you strike at your opponent!\n\r", ch);
            obj_from_char(weapon);
            extract_obj(weapon);
            return;
        }
    }

    /* Step 6: Figure out the type of damage message */

    if (!weapon)
    {
	weapon = get_eq_char(ch, WEAR_HANDS);
	if (weapon && (weapon->item_type != ITEM_WEAPON))
	    weapon = NULL;
    }

    if ( dt == TYPE_UNDEFINED )
    {
	dt = TYPE_HIT;
	if ( !weapon || weapon->item_type != ITEM_WEAPON )
	    if (is_affected(ch,gsn_shadowfist))
		dt += DAM_MENTAL;
	    else
		dt += ch->dam_type;
    }

    if (dt < TYPE_HIT)
    {
    	if (weapon)
    	    dam_type = weapon->value[3];
    	else
    	    dam_type = ch->dam_type;
    }
    else if (dt == TYPE_HIT && weapon && (weapon->item_type == ITEM_WEAPON))
        dam_type = weapon->value[3];
    else
    	dam_type = dt - TYPE_HIT;

    if (dam_type == -1)
	dam_type = DAM_BASH;

    if (weapon && obj_is_affected(weapon, gsn_heatsink))
	damage_old(ch,ch,number_range(4,8),gsn_heatsink,DAM_COLD,TRUE);
    
    wrathkyana_combat_effect(ch, victim);

    if (is_an_avatar(ch))
    {
	    dt = TYPE_HIT + 20;
    	dam_type = DAM_HOLY;
    }

    AFFECT_DATA * aspectAff(get_affect(ch, gsn_aspectoftheinferno));
    if (aspectAff != NULL && aspectAff->modifier == 0)
        dam_type = DAM_FIRE;

    if (is_affected(ch, gsn_shadowfiend))
        dam_type = DAM_DEFILEMENT;
   
    /* get the weapon skill */
    sn = get_weapon_sn_from_obj(weapon);

    if (htype == HIT_FLOAT)
	    skill = get_skill(ch, gsn_dancingsword);
    else
	    skill = 20 + get_weapon_skill(ch, sn, (bool) ((htype == HIT_PRIMARY) ? TRUE : FALSE));

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

    if (dt == gsn_backstab)
	thac0 -= 10 * (100 - get_skill(ch,gsn_backstab));

    if (dt == gsn_dualbackstab)
	thac0 -= 10 * (100 - get_skill(ch,gsn_dualbackstab));

    switch(dam_type)
    {
	case(DAM_PIERCE):victim_ac = GET_AC(victim,AC_PIERCE)/10;	break;
	case(DAM_BASH):	 victim_ac = GET_AC(victim,AC_BASH)/10;		break;
	case(DAM_SLASH): victim_ac = GET_AC(victim,AC_SLASH)/10;	break;
	default:	 victim_ac = GET_AC(victim,AC_EXOTIC)/10;	break;
    }; 
	
    if (victim_ac < -15)
	victim_ac = (victim_ac + 15) / 5 - 15;
     
    if ( !can_see( ch, victim ) && !is_blindfighter(ch,TRUE) )
	victim_ac -= 4;

    if (!IS_NPC(ch) && ch->pcdata->condition[COND_HUNGER] > get_max_hunger(ch))
	victim_ac -= 4;

    if ( victim->position < POS_FIGHTING)
	victim_ac += 4;
 
    if (victim->position < POS_RESTING)
	victim_ac += 6;

    uberskillgetsuberer(ch,victim,htype,weapon);

    while ( ( diceroll = number_bits( 5 ) ) >= 20 )
	;

    // Check for diakinesis
    AFFECT_DATA * diakinesis(get_affect(victim, gsn_diakinesis));
    if (diakinesis != NULL && diakinesis->modifier > 0)
    {
        // Diakinesis not used up yet; make the diceroll a miss and decrement the effect
        --diakinesis->modifier;
        diceroll = 0;
    }

    // Check for sonic boom
    for (AFFECT_DATA * sonic(get_affect(ch, gsn_sonicboom)); sonic != NULL; sonic = get_affect(ch, gsn_sonicboom, sonic))
    {
        if (sonic->modifier > 0)
        {
            diceroll = 0;
            --sonic->modifier;
            break;
        }
    }

    // Check for unreal incursion
    if (is_affected(ch, gsn_unrealincursion) && number_bits(5) == 0)
    {
        std::ostringstream mess;
        mess << "You are briefly distracted by " << generate_unreal_entity() << " which " << generate_unreal_predicate() << " across your vision.\n";
        send_to_char(mess.str().c_str(), ch);
        diceroll = 0;
    }

    // Check for lightning bolt
    AFFECT_DATA * bolt(get_affect(ch, gsn_lightning_bolt));
    if (bolt != NULL)
    {
        diceroll = 0;
        affect_remove(ch, bolt);
    }

    // Check for luck
    bool serpent(is_affected(ch, gsn_form_of_the_serpent));
    if (diceroll > 0 && diceroll < 19 && !serpent)
    {
        switch (Luck::CheckOpposed(*ch, *victim))
        {
            case Luck::Unlucky: diceroll = 0; break;
            case Luck::Lucky: diceroll = 19; break;
            default: break;
        }
    }
    
    if ((diceroll == 0 || (diceroll != 19 && diceroll < thac0 - victim_ac)) && !serpent)
    {
        /* Miss. */
        damage_from_obj( ch, victim, weapon, 0, dt, dam_type, TRUE );

        if ((dt == gsn_backstab) || (dt == gsn_dualbackstab) || (dt == gsn_flank))
            return;

        if ((htype == HIT_PRIMARY && !hit_opp) && (((chance = get_skill(ch,gsn_dual_wield)) > 0)
        || (IS_NPC(ch) && IS_SET(ch->off_flags,OFF_DUAL_WIELD))) && (get_eq_char(ch, WEAR_DUAL_WIELD)
        || ((hf = hands_free(ch)) > 1) || ((hf == 1) && wield)))
        {
            chance /= 2;
            chance += get_curr_stat(ch, STAT_DEX);
            chance -= get_curr_stat(victim, STAT_DEX);
                
            if (ch->class_num == CLASS_FIGHTER)
                chance = round(chance * 1.2);

            if (number_percent() < get_skill(ch,gsn_pugilism) && !get_eq_char(ch, WEAR_DUAL_WIELD)) chance += 15; 
                chance += mod_by_enthusiasm(ch) + mod_by_absorb(ch);
    
            if (IS_PAFFECTED(ch,AFF_TWOHAND))
                chance = 0;
            if (number_percent() <= chance)
            {
                one_hit(ch, victim, TYPE_UNDEFINED, HIT_DUAL);
                if (get_eq_char(ch, WEAR_DUAL_WIELD))
                    check_improve(ch,victim,gsn_dual_wield,TRUE,5);
            }
            else if (get_eq_char(ch, WEAR_DUAL_WIELD))
            {
                if (chance > 0)
                   check_improve(ch,victim,gsn_dual_wield,FALSE,5);	
            }
        }
        return;
    }

    if (obj_is_affected(weapon, gsn_cursebarkja))
    {
        if (number_percent() < 6)
	{
	    CHAR_DATA * newvict;

            for (newvict = ch->in_room->people; newvict != NULL; newvict = newvict->next_in_room)
            {
                if (is_same_group(ch, newvict) && !IS_OAFFECTED(newvict, AFF_GHOST) && IS_PK(ch, newvict) && can_see(ch, newvict))
		    break;
            }
	    if (newvict != NULL)
	    {
	        act("Your weapon jerks suddenly, swinging towards $N!", ch, NULL, newvict, TO_CHAR);
		one_hit(ch, newvict, dt, htype);
		return;
	    }
	}
        if (number_percent() < 21)
        {
            act("Your weapon jerks suddenly, failing to hit $n!", ch, NULL, victim, TO_CHAR);
    	    return;
        }
    }

    /* Step 7: Hit. Calc damage. */

    CHAR_DATA *phch(ch);
    if (IS_NPC(ch) && ch->master && IS_SET(ch->act,ACT_ILLUSION)) 
        phch = ch->master;

    // Check whether clockwork golem is present
    bool hasClockwork(false);
    if (ch->in_room != NULL)
        hasClockwork = is_clockworkgolem_present(*ch->in_room);

    if (IS_NPC(ch) && (!ch->pIndexData->new_format || !weapon))
    {
        if (!ch->pIndexData->new_format)
        {
            if (hasClockwork) dam = ch->level;
            else dam = number_range( ch->level / 2, ch->level * 3 / 2 );
            if ( weapon )
                dam += dam / 2;
        }
        else if (hasClockwork) dam = (ch->damage[DICE_NUMBER] * (ch->damage[DICE_TYPE] + 1)) / 2;
        else dam = dice(ch->damage[DICE_NUMBER], ch->damage[DICE_TYPE]);
    }
    else
    {
        if (sn != -1)
            check_improve(ch,victim,sn,TRUE,5);

        if (weapon)
        {
            if (weapon->pIndexData->new_format)
            {
                if (hasClockwork) dam = (weapon->value[1] * (weapon->value[2] + 1)) / 2;
                else dam = dice(weapon->value[1], weapon->value[2]);
            }
            else
            {
                if (hasClockwork) dam = (weapon->value[1] + weapon->value[2]) / 2;
                else dam = number_range(weapon->value[1], weapon->value[2]);
            }
            dam = (dam * skill * 11) / 1000;
        }
        else
        {
            if (hasClockwork) dam = ((1 + (4 * skill) / 100) + (2 * ch->level * skill) / 300) / 2;
            else dam = number_range( 1 + 4 * skill/100, 2 * ch->level/3 * skill/100);
        }
    }

    /*
     * Bonuses.
     */
    dam += check_extra_damage(ch,dam,weapon);

    if ( !IS_AWAKE(victim) )
	dam *= 2;
     else if (victim->position < POS_FIGHTING)
	dam = dam * 5 / 4;

    if (is_affected(victim, gsn_lunarinfluence) && (time_info.phase_rhos == MOON_NEW) && IS_TRUE_EVIL(ch))
	dam = (dam * (100 - (victim->level / 5)) / 100);

    if (is_affected(victim, gsn_lunarinfluence) && (time_info.phase_rhos == MOON_FULL) && IS_TRUE_GOOD(ch))
	dam = (dam * (100 - (victim->level / 5)) / 100);

// We don't want multiplicative damroll and momentum for ETs
    dam += GET_DAMROLL(ch) * (ch->class_num == global_int_class_swordmaster ? UMIN(100,skill) /100 : 1);

    if (htype == HIT_DUAL)
	dam = dam * 4 / 5;

    if ((dt == gsn_backstab) && weapon) 
	dam = round(dam * UMAX(1.5, ((ch->level-1)/12)));

    if ((dt == gsn_dualbackstab) && weapon) 
	dam = round(dam * UMAX(1.5, ((ch->level-1)/20)));

    if (dt == gsn_flank)
	dam = round(dam * 1.5);
    
    if (IS_NPC(victim))
        for (x = 0; x < MAX_CLORE; x++)
	    if (ch->lcreature[x] == victim->pIndexData->vnum)
	    {
		dam *= (1 + (ch->lpercent[x] / 200));
		break;
	    }

    if (weapon)
    {
        // Handle baneblade
        check_baneblade_strike(*ch, *victim, *weapon);

        // Handle holy avenger
        if (((paf = affect_find(weapon->affected, gsn_holyavenger)) != NULL) && IS_TRUE_EVIL(victim))
             dam = dam * (paf->modifier + 100) / 100;

        int ask = get_skill(ch, gsn_augmented_strength);
        if ((htype == HIT_PRIMARY)
        && IS_WEAPON_STAT(weapon, WEAPON_TWO_HANDS)
        && !get_eq_char(ch, WEAR_HOLD)
        && !get_eq_char(ch, WEAR_DUAL_WIELD)
        && !get_eq_char(ch, WEAR_LIGHT)
        && !get_eq_char(ch, WEAR_SHIELD))
        {
            if (ask > 0)
            {
                if (get_obj_weight(weapon) > (str_app[URANGE(0,get_curr_stat(ch,STAT_STR),25)].wield * 10))
                {
                    if (number_percent() <= ask)
                    {   
                    dam = (dam * (ask+10))/100;
                        check_improve(ch,victim,gsn_augmented_strength,TRUE,1);
                    }
                    else
                    {
                    act("You lack the strength to properly swing $p.",ch,weapon,NULL,TO_CHAR);
                    dam = (dam * (ask-25)/100);
                    }
                }	
                else
                dam = (dam * 11/10);
            }
            else
            dam = (dam * 11 / 10);
        }
        dam = dam * UMIN(100, (weapon->condition * 5 / 4)) / 100;
    }

    if (weapon && obj_is_affected(weapon, gsn_applyoil))
    {
        AFFECT_DATA * af = get_obj_affect(weapon, gsn_applyoil);
        act("The magic stored in $p is unleashed!", ch, weapon, NULL, TO_ROOM);
        act("The magic stored in $p is unleashed!", ch, weapon, NULL, TO_CHAR);
        (*skill_table[af->modifier].spell_fun) (af->modifier, af->level, ch, (void *) victim, TARGET_CHAR);
        object_affect_strip(weapon, gsn_applyoil);
    }

    // Check for unfetter mana
    AFFECT_DATA * unfetter(get_affect(ch, gsn_unfettermana));
    if (unfetter != NULL)
    {
        // Make sure to clear arguments before casting
        target_name = "";

        send_to_char("You release your hold on the stored energy, letting it pour forth unfettered!\n", ch);
        const skill_type & storedSkill(skill_table[unfetter->modifier]);
        switch (storedSkill.target)
        {
            // Offensive with target
            case TAR_CHAR_OFFENSIVE: 
            case TAR_OBJ_CHAR_OFF:
                (*storedSkill.spell_fun)(unfetter->modifier, unfetter->level, ch, victim, TARGET_CHAR); 
                break;

            // Defensive with target
            case TAR_CHAR_DEFENSIVE: 
            case TAR_CHAR_DEF_GLOBAL:
            case TAR_OBJ_CHAR_DEF:
            case TAR_CHAR_SELF:      
                (*storedSkill.spell_fun)(unfetter->modifier, unfetter->level, ch, ch, TARGET_CHAR); 
                break;

            // Ignore
            case TAR_IGNORE:
                (*storedSkill.spell_fun)(unfetter->modifier, unfetter->level, ch, NULL, TARGET_NONE);
                break;

            // Other, such as object targets
            default:
                send_to_char("With no clear target for its power, your magic fizzles and dissipates.\n", ch);
                break;
        }

        affect_remove(ch, unfetter);
    }

    if (get_group_song(ch, gsn_eleventhhour) 
      && can_be_affected(ch,gsn_eleventhhour))
	dam = dam * 6 / 5;

    if (hit_opp)
	if (weapon && weapon->value[0] != WEAPON_DAGGER)
	    dam /= 2;

    if ( dam <= 0 )
	dam = 1;

    // Here's a hack for you...
    if (htype == HIT_DUAL)
    {
	    dual_damage = TRUE;
        result = damage_from_obj( ch, victim, weapon, dam, dt, dam_type, TRUE );
        dual_damage = FALSE;	
    }
    else
        result = damage_from_obj( ch, victim, weapon, dam, dt, dam_type, TRUE );

    if (result)
        check_mirrorofsouls(*ch, *victim);
    
    if (result && !is_affected(ch, gsn_roaroftheexalted) && weapon 
    &&  obj_is_affected(weapon, gsn_caressofpricina) && (number_percent() < 16) && victim && !IS_OAFFECTED(victim, AFF_GHOST)
    &&  !IS_SET(victim->act,ACT_UNDEAD))
    {
        int drain = number_range(ch->level * 2 / 3, ch->level);
        act("$p glows darkly, and you feel your life force draining away.", ch, weapon, victim, TO_VICT);
        act("$p glows darkly, and $n grins wickedly.", ch, weapon, victim, TO_NOTVICT);
        act("$p glows darkly, and you feel $N's life force strengthening you.", ch, weapon, victim, TO_CHAR);
        drain = UMIN(victim->hit, drain);
            ch->hit += UMAX(0,drain);
        damage_old(ch, victim, drain, gsn_caressofpricina, DAM_NEGATIVE, TRUE);
    }    

    if ((dt == gsn_backstab) || (dt == gsn_dualbackstab) || (dt == gsn_flank))
	return;

    if ((htype == HIT_PRIMARY && !hit_opp)
     && (((chance = get_skill(ch,gsn_dual_wield)) > 0)
      || (IS_NPC(ch) && IS_SET(ch->off_flags,OFF_DUAL_WIELD)))
     && (get_eq_char(ch, WEAR_DUAL_WIELD)
      || ((hf = hands_free(ch)) > 1) || ((hf == 1) && wield)))
    {
	chance /= 2;
	chance += get_curr_stat(ch, STAT_DEX);
	chance -= get_curr_stat(victim, STAT_DEX);
	if (!IS_PAFFECTED(ch, AFF_TWOHAND))
	    if (number_percent() <= chance)
            {
		one_hit(ch, victim, TYPE_UNDEFINED, HIT_DUAL);
		check_improve(ch,victim,gsn_dual_wield,TRUE,5);
            }
	    else
		check_improve(ch,victim,gsn_dual_wield,FALSE,5);	

	if ((floatweapon = get_eq_char(ch,WEAR_FLOAT)) != NULL)
 	{
	    if (floatweapon->item_type == ITEM_WEAPON)
		one_hit(ch, victim, TYPE_FLOAT, HIT_FLOAT);
	}

    }

    // This really should be a prog or something.
    if (result && IS_NPC(ch) && (ch->pIndexData->vnum == MOB_VNUM_SNAKE))
    {
	AFFECT_DATA af;
        af.valid = TRUE;
        af.point = NULL;

	if (!saves_spell(ch->level * 3/4, ch, victim, DAM_POISON))
	{
	    send_to_char("You feel poison coursing through your veins.\n\r", victim);
	    act("$n looks slightly ill.", victim, NULL, NULL, TO_ROOM);
	
	    af.where	 = TO_AFFECTS;
	    af.type	 = gsn_poison;
	    af.level	 = ch->level / 2;
	    af.duration  = ch->level / 4;
	    af.location  = APPLY_STR;
	    af.modifier  = -1;
	    af.bitvector = AFF_POISON;
	    affect_join( victim, &af );
	}
    }

    if (result && weapon != NULL)
    { 
	int dam, achance;
	
	for (type=0;poison_table[type].spell_fun;type++)
	    if (obj_is_affected(weapon,*(poison_table[type].sn)))
		break;
	if (poison_table[type].spell_fun)
	{
	    if ((paf=affect_find(weapon->affected,*(poison_table[type].sn)))!= NULL)
		poison_table[type].spell_fun(ch,victim,paf->level,gsn_envenom);
	    else
		poison_table[type].spell_fun(ch,victim,ch->level,gsn_envenom);
	    object_affect_strip(weapon,*(poison_table[type].sn));
	}

	if (ch->fighting == victim 
	  && ((weapon->value[3] == DAM_FIRE)
	    || obj_is_affected(weapon, gsn_firebrand))
	  && (number_bits(3) == 0) 
	  && ((achance = get_skill(ch, gsn_aggravatewounds)) > 0))
	{
	    if (number_percent() < achance)
	    {
	        AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

	        act("The wound left by $p begins to burn painfully.", victim, weapon, NULL, TO_CHAR);
	        send_to_char("You aggravate the wound, causing it to burn painfully.\n\r", ch);
	        af.where	 = TO_AFFECTS;
	        af.type	 = gsn_aggravatewounds;
	        af.level     = ch->level;
	        af.duration  = 4;
	        af.modifier  = 0;
	        af.location  = 0;
	        af.bitvector = 0;
	        affect_to_char(victim, &af);
		check_improve(ch,victim, gsn_aggravatewounds, TRUE, 4);
	    }
	    else
		check_improve(ch,victim, gsn_aggravatewounds, FALSE, 4);
	}

	if (ch->fighting == victim && !IS_SET(victim->imm_flags, IMM_POISON) && !IS_AFFECTED(victim, AFF_POISON))
	{
	    int level(-1);
        AFFECT_DATA * poison(get_obj_affect(weapon, gsn_poison));
        if (poison != NULL) level = poison->level;
        if (IS_WEAPON_STAT(weapon, WEAPON_POISON)) level = UMAX(level, weapon->level / 2);

	    if (level >= 0 && !saves_spell(level, NULL, victim, DAM_POISON)) 
	    {
            send_to_char("You feel poison coursing through your veins.\n", victim);
		    act("$n is poisoned by the venom on $p.", victim,weapon,NULL,TO_ROOM);

            AFFECT_DATA af = {0};
    		af.where     = TO_AFFECTS;
    		af.type      = gsn_poison;
    		af.level     = level;
    		af.duration  = level / 8;
    		af.location  = APPLY_STR;
    		af.modifier  = -2;
    		af.bitvector = AFF_POISON;
    		affect_join( victim, &af );
	    }
 	}


    	if (ch->fighting == victim && IS_WEAPON_STAT(weapon,WEAPON_VAMPIRIC)
	  && !IS_SET(victim->act,ACT_UNDEAD) && !IS_OAFFECTED(victim,AFF_GHOST))
	{
	    dam = number_range(1, weapon->level / 5 + 1);
	    act("$p draws life from $n.",victim,weapon,NULL,TO_ROOM);
	    act("You feel $p drawing your life away.",
		victim,weapon,NULL,TO_CHAR);
	    damage_old(ch,victim,dam,0,DAM_NEGATIVE,FALSE);
	    ch->hit += dam/2;
	}

	if (ch->fighting == victim && IS_WEAPON_STAT(weapon,WEAPON_FLAMING))
	{
	    dam = number_range(1,weapon->level / 4 + 1);
	    act("$n is burned by $p.",victim,weapon,NULL,TO_ROOM);
	    act("$p sears your flesh.",victim,weapon,NULL,TO_CHAR);
	    fire_effect( (void *) victim,weapon->level/2,dam,TARGET_CHAR);
	    damage(ch,victim,dam,0,DAM_FIRE,FALSE);
	}

	if (ch->fighting == victim && IS_WEAPON_STAT(weapon,WEAPON_FROST))
	{
	    dam = number_range(1,weapon->level / 6 + 2);
	    act("$p freezes $n.",victim,weapon,NULL,TO_ROOM);
	    act("The cold touch of $p surrounds you with ice.",
		victim,weapon,NULL,TO_CHAR);
	    cold_effect(victim,weapon->level/2,dam,TARGET_CHAR);
	    damage(ch,victim,dam,0,DAM_COLD,FALSE);
	}

	if (ch->fighting == victim && IS_WEAPON_STAT(weapon,WEAPON_SHOCKING))
	{
	    dam = number_range(1,weapon->level/5 + 2);
	    act("$n is struck by lightning from $p.",victim,weapon,NULL,TO_ROOM);
	    act("You are shocked by $p.",victim,weapon,NULL,TO_CHAR);
	    shock_effect(victim,weapon->level/2,dam,TARGET_CHAR);
	    damage(ch,victim,dam,0,DAM_LIGHTNING,FALSE);
	}
    }

    if (weapon && is_affected(victim, gsn_stoneshell))
    {
	if ((weapon->value[3] == DAM_SLASH) && number_bits(8) == 0 && !IS_SET(weapon->extra_flags[0], ITEM_NODESTROY))
	{
	    act("$p shatters against $N!", ch, weapon, victim, TO_CHAR);
	    act("$n's $p shatters against you!", ch, weapon, victim, TO_VICT);
	    act("$n's $p shatters against $N!", ch, weapon, victim, TO_NOTVICT);
	    extract_obj(weapon);
	    weapon = NULL;
	}
	else if (number_bits(9) == 0 && !IS_SET(weapon->extra_flags[0], ITEM_NODESTROY))
	{
	    act("$p shatters against $N!", ch, weapon, victim, TO_CHAR);
	    act("$n's $p shatters against you!", ch, weapon, victim, TO_VICT);
	    act("$n's $p shatters against $N!", ch, weapon, victim, TO_NOTVICT);
	    extract_obj(weapon);
	    weapon = NULL;
	}
    }

    if (weapon && is_affected(ch, gsn_clumsiness) && (number_percent () < 5))
    {
	/* Clumsiness routines */
	if (!IS_OBJ_STAT(weapon, ITEM_NOREMOVE))
	{
	    send_to_char("You dropped your weapon, you clumsy oaf!\n\r",ch);
	    act("$n fumbles $s weapon, and drops it!", ch, NULL,NULL,TO_ROOM);
            obj_from_char( weapon );
	    obj_to_room( weapon, ch->in_room );

	    if (IS_NPC(ch) && ch->wait == 0 && can_see_obj(ch,weapon))
	        get_obj(ch,weapon,NULL);
	}
    }
    if (is_affected(ch,gsn_drawblood))
    {
	if ((weapon 
	  && (weapon->value[0] == WEAPON_AXE
	    || weapon->value[0] == WEAPON_SWORD
	    || weapon->value[3] == DAM_SLASH))
	  || (ch->race == global_int_race_srryn
	    || ch->race == global_int_race_kankoran
	    || is_affected(ch,gsn_beastform)))
	    if (number_bits(1) == 0)
	    {
		AFFECT_DATA af;
		af.valid = TRUE;
		af.point = NULL;
		af.type = gsn_drawblood;
		af.where = TO_OAFFECTS;
		af.location = APPLY_NONE;
		af.modifier = 0;
		af.duration = 4;
		af.bitvector = AFF_BLEEDING;
		affect_to_char(victim,&af);
		act("You gouge a deep gash into $N's skin.",ch,NULL,victim,TO_CHAR);
		act("$n gouges a deep gash into your skin.",ch,NULL,victim,TO_VICT);
		act("$n gouges a deep gash into $N's skin.",ch,NULL,victim,TO_NOTVICT);
	    }
	    
    }
    if (is_affected(victim,gsn_shatter) && weapon && !IS_SET(weapon->extra_flags[0], ITEM_NODESTROY))
    {
	chance = 5;

	if (IS_SET(weapon->extra_flags[0], ITEM_MAGIC))
	    chance -= 2;

	if (IS_SET(weapon->extra_flags[0], ITEM_HUM))
	    chance -= 1;

	if (IS_SET(weapon->extra_flags[0], ITEM_BLESS))
	    chance -= 1;

	if (IS_SET(weapon->extra_flags[0], ITEM_EVIL))
	    chance -= 1;

	if (number_percent() <= chance)
	{
	    send_to_char("Your weapon shatters!\n\r", ch);
            act("$n's $p shatters!",ch,weapon,NULL,TO_ROOM);
	    act("Shards fly from $n as $s skin is hit with a weapon.", victim, NULL, NULL, TO_ROOM);

	    /* Destroy Weapon */
	    obj_from_char(weapon);
	    extract_obj(weapon);
	    weapon = NULL;
	}
    }

    // Check for runes of sunder
    if (result && dam > 0 && victim->mana > 0 && number_percent() <= static_cast<int>(Runes::InvokedCount(*ch, Rune::Sunder) * 15))
    {
        // Each bonus increases the drain by 25%
        victim->mana -= ((dam * (100 + (Runes::BonusCount(*ch) * 25))) / 100);
        victim->mana = UMAX(0, victim->mana);
        act("The rune of Sunder fills your blow with power, breaking $N's concentration!", ch, NULL, victim, TO_CHAR);
        act("Your concentration breaks as $n's blow connects!", ch, NULL, victim, TO_VICT);
   }

    // Check for runes of gravitas
    if (weapon != NULL && number_percent() <= (static_cast<int>(Runes::InvokedCount(*victim, Rune::Gravitas)) * 2))
    {
        // Echoes
        act("Your rune of Gravitas flares up, infusing $p with increased gravity!", victim, weapon, ch, TO_CHAR);
        act("$n's rune of Gravitas flares up, and $p suddenly feels a little heavier in your grasp!", victim, weapon, ch, TO_VICT);

        // Add to the weight
        ++weapon->weight;
        ++ch->carry_weight;

        // Check for bonuses
        if (!is_affected(ch, gsn_density) && ch->size >= SIZE_MEDIUM
        && number_percent() <= static_cast<int>(Runes::BonusCount(*victim) * 25))
        {
            send_to_char("You feel heavy as the gravity seizes you!\n", ch);
            act("$n appears to be having trouble keeping upright.", ch, NULL, NULL, TO_ROOM);

            // Apply effect
            AFFECT_DATA raf = {0};
            raf.where    = TO_AFFECTS;
            raf.type     = gsn_density;
            raf.level    = victim->level;
            raf.duration = (victim->level / 8);
            affect_to_char(ch, &raf);
        }
    }

    // Check for runes of erosion
    unsigned int erosionCount(Runes::InvokedCount(*victim, Rune::Erosion));
    if (erosionCount > 0)
    {
        static const int MinMod = -500;

        // Initialize some common values
        int duration((victim->level / 10) + (4 * Runes::BonusCount(*victim)));
        int modifierMod(static_cast<int>(erosionCount) * 4);
        modifierMod = UMIN(modifierMod, ch->max_hit - 1);

        // Check for effect
        AFFECT_DATA * erosion(get_affect(ch, gsn_erosion));
        if (erosion == NULL)
        {
            // Apply a new effect
            act("Your rune of Erosion flares up, and $N pales slightly!", victim, NULL, ch, TO_CHAR);
            act("$n's rune of Erosion flares up, and you begin to feel a bit fainter.", victim, NULL, ch, TO_VICT);
            act("$n's rune of Erosion flares up, and $N pales slightly!", victim, NULL, ch, TO_NOTVICT);

            AFFECT_DATA raf = {0};
            raf.where    = TO_AFFECTS;
            raf.type     = gsn_erosion;
            raf.level    = victim->level;
            raf.location = APPLY_HIT;
            raf.modifier = UMAX(MinMod, -modifierMod);
            raf.duration = duration;
            affect_to_char(ch, &raf);
        }
        else
        {
            // Manually update effect
            modifierMod = UMAX(MinMod, erosion->modifier - modifierMod) - erosion->modifier; // modifierMod is negative after this
            erosion->duration = UMAX(erosion->duration, duration);
            erosion->modifier += modifierMod;
            ch->max_hit += modifierMod;
        }
    }

    // Check for runes of tremor
    if (number_percent() <= static_cast<int>(Runes::InvokedCount(*victim, Rune::Tremor) * 8))
    {
        int effectiveLevel(victim->level + (Runes::BonusCount(*victim) * 2));
        if (saves_spell(effectiveLevel, victim, ch, DAM_OTHER))
        {
            act("Your rune of Tremor flares up, but $N seems unfazed.", victim, NULL, ch, TO_CHAR);
            act("$n's rune of Tremor flares up, but you ignore the painful vibration.", victim, NULL, ch, TO_VICT);
        }
        else
        {
            // Show echoes
            act("Your rune of Tremor flares up, sending a painful vibration into $N!", victim, NULL, ch, TO_CHAR);
            act("$n's rune of Tremor flares up, sending a painful vibration into you!", victim, NULL, ch, TO_VICT);
            act("$n's rune of Tremor flares up, sending a painful vibration into $N!", victim, NULL, ch, TO_NOTVICT);

            // Check for effect already in existence
            AFFECT_DATA * paf(get_affect(ch, gsn_tremor));
            if (paf == NULL)
            {
                // Apply new effect
                AFFECT_DATA af = {0};
                af.where    = TO_AFFECTS;
                af.type     = gsn_tremor;
                af.level    = effectiveLevel;
                af.modifier = 1;
                af.duration = 5;
                affect_to_char(ch, &af);
            }
            else
            {
                // Renew and intensify effect
                paf->duration = UMAX(paf->duration, 5);
                paf->modifier = UMIN(6, paf->modifier + 1);
            }
        }
    }

    // Check for runes of bind
    if (victim->in_room != NULL && ON_GROUND(victim)
    && !is_affected(ch, gsn_earthbind) && !check_spirit_of_freedom(ch)
    && number_percent() <= static_cast<int>(Runes::InvokedCount(*victim, Rune::Bind) * 8))
    {
        int effectiveLevel(victim->level + (Runes::BonusCount(*victim) * 2));
        if (saves_spell(effectiveLevel, victim, ch, DAM_OTHER))
        {
            act("Your rune of Bind flares up, but $N seems unaffected.", victim, NULL, ch, TO_CHAR);
            act("$n's rune of Bind flares up, but you resist the shackling magics.", victim, NULL, ch, TO_VICT);
        }
        else
        {
            // Show echoes
            if (is_flying(ch))
            {
                act("Your rune of Bind flares up, and $N falls to the ground with a thud!", victim, NULL, ch, TO_CHAR);
                act("$n's rune of Bind flares up, and you fall to the ground with a thud!", victim, NULL, ch, TO_VICT);
                act("$n's rune of Bind flares up, and $N falls to the ground with a thud!", victim, NULL, ch, TO_NOTVICT);
                stop_flying(*ch);
                damage_old(victim, ch, dice(effectiveLevel, 2), gsn_earthbind, DAM_BASH, true);
            }
            else
            {
                act("Your rune of Bind flares up, shackling $N to the earth!", victim, NULL, ch, TO_CHAR);
                act("$n's rune of Bind flares up, shackling you to the earth!", victim, NULL, ch, TO_VICT);
                act("$n's rune of Bind flares up, shackling $N to the earth!", victim, NULL, ch, TO_NOTVICT);
            }

            // Apply effect
            AFFECT_DATA baf = {0};
            baf.where    = TO_AFFECTS;
            baf.type     = gsn_earthbind;
            baf.level    = effectiveLevel;
            baf.duration = (effectiveLevel / 2);
            affect_to_char(ch, &baf);
        }
    }

    // Check for runes of fossil
    if (!IS_OAFFECTED(ch, AFF_PETRIFY) 
    && number_percent() <= static_cast<int>(Runes::InvokedCount(*victim, Rune::Fossil) * 4))
    {
        int effectiveLevel(victim->level + (Runes::BonusCount(*victim) * 2));
        if (saves_spell(effectiveLevel, victim, ch, DAM_OTHER))
        {
            act("Your rune of Fossil flares up, but $N seems to shake it off.", victim, NULL, ch, TO_CHAR);
            act("$n's rune of Fossil flares up, but you shake off the petrifying magics.", victim, NULL, ch, TO_VICT);
        }
        else
        {
            act("Your rune of Fossil flares up, and $N begins to move unsteadily as $S muscles stiffen and freeze!", victim, NULL, ch, TO_CHAR);
            act("$n's rune of Fossil flares up, and you begin to move unsteadily as your muscles stiffen and freeze!", victim, NULL, ch, TO_VICT);
            act("$n's rune of Fossil flares up, and $N begins to move unsteadily as $S muscles stiffen and freeze!", victim, NULL, ch, TO_NOTVICT);

            // Apply petrify effect
            AFFECT_DATA raf = {0};
            raf.where     = TO_OAFFECTS;
            raf.type      = gsn_petrify;
            raf.duration  = (effectiveLevel / 5);
            raf.bitvector = AFF_PETRIFY;
            affect_to_char(ch, &raf);

            // Apply slow effect
            if (!IS_AFFECTED(ch, AFF_SLOW))
            {
                raf.where     = TO_AFFECTS;
                raf.location  = APPLY_DEX;
                raf.modifier  = -1 - (effectiveLevel / 17);
                raf.bitvector = AFF_SLOW;
                affect_to_char(ch, &raf);

                send_to_char("You feel yourself slow down.\n", ch);
                act("$n appears to be moving more slowly.", ch, NULL, NULL, TO_ROOM);
            }
        }
    }

    // Check for runes of truth
    if (IS_NPC(victim) && IS_SET(victim->act, ACT_ILLUSION) && number_percent() <= static_cast<int>(Runes::InvokedCount(*ch, Rune::Truth) * 7))
    {
        act("Your rune of Truth flares up, and $N suddenly vanishes!", ch, NULL, victim, TO_CHAR);
        act("$n's rune of Truth flares up, and $N suddenly vanishes!", ch, NULL, victim, TO_ROOM);
        extract_char(victim, true);
    }
}

// ch may be NULL, but victim must be filled in
int modify_damage(CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * ch_weapon, int dam, int dt, int dam_type, bool & immune)
{
    immune = false;

    // Check for physical modifiers
    bool isPhysical(false);
    switch (dam_type)
    {
        case DAM_BASH: case DAM_PIERCE: case DAM_SLASH:
        case (TYPE_HIT + DAM_BASH): case (TYPE_HIT + DAM_PIERCE): case (TYPE_HIT + DAM_SLASH):
            isPhysical = true;
            break;
    } 
        
    if (dam < 0) 
        dam = 0;	  

    if ((dam > 1) && !IS_NPC(victim) && (dt >= TYPE_HIT) && (victim->pcdata->condition[COND_DRUNK] > 10))
        dam = (9 * dam) / 10;

    // Check for solace of the seas
    struct SolaceCheck
    {
        static bool HasSolace(CHAR_DATA * checkee)
        {
            for (AFFECT_DATA * solace(get_affect(checkee, gsn_solaceoftheseas)); solace != NULL; solace = get_affect(checkee, gsn_solaceoftheseas, solace))
            {
                if (solace->modifier == 1)
                    return true;
            }

            return false;
        }
    };

    if (SolaceCheck::HasSolace(victim) || (ch != NULL && SolaceCheck::HasSolace(ch)))
    {
        immune = true;
        return 0;
    }

    // Check for runes of calcify
    AFFECT_DATA * calcify(get_affect(victim, gsn_calcify));
    if (isPhysical && dam >= 5)
    {
        unsigned int runeCount(Runes::InvokedCount(*victim, Rune::Calcify));
        if (runeCount > 0)
        {
            if (calcify == NULL)
            {
                // Apply a new effect
                AFFECT_DATA af = {0};
                af.where    = TO_AFFECTS;
                af.type     = gsn_calcify;
                af.level    = victim->level;
                af.modifier = 4;
                affect_to_char(victim, &af);
                send_to_char("A hard shell of stone begins to calcify about you!\n", victim);

                calcify = get_affect(victim, gsn_calcify);
                if (calcify == NULL)
                {
                    bug("Calcify is NULL immediately after assigning it", 0);
                    send_to_char("An error has occurred, please contact the gods.\n", victim);
                    return dam;
                }
            }

            // Renew and possibly intensify the effect
            int maxMod(20 + (runeCount * 10));
            calcify->duration = UMAX(calcify->duration, victim->level / 5);
            calcify->modifier += 1 + Runes::BonusCount(*victim) + (dam / 25);
            calcify->modifier = UMIN(calcify->modifier, maxMod);
            calcify->modifier = UMIN(calcify->modifier, 80);
        }
    }

    // Handle calcify itself
    if (calcify != NULL && calcify->modifier > 0)
    {
        // Physical damage might be ignored
        if (isPhysical && number_percent() <= calcify->modifier)
        {
            send_to_char("The blow lands against your calcified shell!\n", victim);
            immune = true;
            return 0;
        }

        // Fire damage can burn away calcification
        if (dam_type == DAM_FIRE)
        {
            send_to_char("The heat burns away some of your calcification!\n", victim);
            calcify->modifier -= (1 + (dam / 25));
            calcify->modifier = UMAX(0, calcify->modifier);
        }
    }

    // Check for runes of strength
    if (ch != NULL && dt >= TYPE_HIT)
    {
        unsigned int runeCount(Runes::InvokedCount(*ch, Rune::Strength));
        if (runeCount > 0)
        {
            unsigned int bonusCount(Runes::BonusCount(*ch));
            if (bonusCount == 0) dam += (runeCount / 2);
            else dam += runeCount + bonusCount;
        }
    }

    // Check for runes of pulverize
    if (dam_type == DAM_BASH && ch != NULL)
    {
        unsigned int runeCount(Runes::InvokedCount(*ch, Rune::Pulverize));
        dam = (dam * (100 + (runeCount * 10))) / 100;
        
        // Check for bonuses
        if (runeCount > 0 && number_percent() <= static_cast<int>(Runes::BonusCount(*ch) * 3))
        {
            act("You reel, dazed from the blow!", victim, NULL, NULL, TO_CHAR);
            act("$n reels, dazed from the blow!", victim, NULL, NULL, TO_ROOM);
            WAIT_STATE(victim, PULSE_VIOLENCE);
        }
    }

    // Check for gaseous form
    if (is_affected(victim, gsn_gaseousform))
    {
        // Check for call upon wind
        int intensifier(0);
        std::pair<Direction::Value, int> windInfo(checkWind(victim));
        AFFECT_DATA * call(get_affect(victim, gsn_calluponwind));
        if (windInfo.second > 0 && call != NULL && call->modifier == Direction::East)
            intensifier = 5 + (windInfo.second / (windInfo.first == Direction::East ? 5 : 10));

        switch (dam_type)
        {
            case DAM_PIERCE:    dam = (dam * (60 - intensifier))  / 100; break;
            case DAM_SLASH:     dam = (dam * (70 - intensifier))  / 100; break;
            case DAM_DROWNING:  dam = (dam * (80 - intensifier))  / 100; break;
            case DAM_LIGHT:     dam = (dam * (90 - intensifier))  / 100; break;
            case DAM_SOUND:     dam = (dam * (110 + intensifier)) / 100; break;
            case DAM_ACID:      dam = (dam * (120 + intensifier)) / 100; break;
            case DAM_BASH:      dam = (dam * (140 + intensifier)) / 100; break;
        }
    }

    // Check for shockcraft charging
    if (dam_type == DAM_LIGHTNING)
    {
        AFFECT_DATA * charging(get_charging_effect(victim));
        if (charging != NULL && ch != victim && victim->in_room != NULL)
        {
            if (number_percent() <= get_skill(victim, gsn_shockcraft))
            {
                check_improve(victim, ch, gsn_shockcraft, true, 6);

                int reduction(dam / 5);
                dam -= reduction;

                AFFECT_DATA * charged(get_charged_effect(victim));
                int prevCharged(0);
                int totalCharged(reduction);
                victim->mana += totalCharged;
                if (charged == NULL)
                {
                    // Apply the new one
                    AFFECT_DATA af = {0};
                    af.where    = TO_AFFECTS;
                    af.type     = gsn_shockcraft;
                    af.level    = victim->level;
                    af.duration = -1;
                    af.location = APPLY_MANA;
                    af.modifier = totalCharged;
                    affect_to_char(victim, &af);
                }
                else
                {
                    // Update the existing one
                    prevCharged = charged->modifier;
                    charged->modifier += totalCharged;
                    victim->max_mana += totalCharged;
                    totalCharged = charged->modifier;
                }
                victim->mana = UMIN(victim->mana, victim->max_mana);

                // Check for exploding from too much charge
                int threshold(victim->level * 4);
                if (number_percent() <= ((totalCharged - threshold) / 5))
                {
                    // Absorbed too much
                    affect_strip(victim, gsn_shockcraft);
                    act("The charge within you grows too strong to contain! A ball of crackling energy erupts from you, discharging everywhere!", victim, NULL, NULL, TO_CHAR);
                    act("A ball of crackling energy erupts from $n, discharging into brilliant-white forks of lightning!", victim, NULL, NULL, TO_ROOM);

                    // Hit everyone in the room
                    for (CHAR_DATA * vch(victim->in_room->people); vch != NULL; vch = vch->next_in_room)
                    {
                        if (!is_safe_spell(victim, vch, true) && victim != vch)
                        {
                            int shockDam(number_range(totalCharged / 2, totalCharged));
                            if (saves_spell(victim->level, NULL, vch, DAM_LIGHTNING)) shockDam /= 2;
                            if (number_bits(1) == 0) shockDam = 0;
                            damage_old(victim, vch, shockDam, gsn_shockcraft, DAM_LIGHTNING, true);
                        }
                    }

                    // Hit the victim with enough damage to be fatal except in cases of very high hp and/or resistances (in practice, arc shield or imm magic)
                    damage_old(victim, victim, totalCharged * 15, gsn_shockcraft, DAM_LIGHTNING, true);
                }
                else
                {
                    // Did not explode, possibly send echoes
                    if (prevCharged < ((threshold * 9) / 10) && totalCharged >= ((threshold * 9) / 10)) send_to_char("The power rages within you, threatening to burst out! You can barely contain it!\n", victim);
                    else if (prevCharged < ((threshold * 7) / 10) && totalCharged >= ((threshold * 7) / 10)) send_to_char("You feel the power crackling within you, its chaotic energy growing difficult to contain!\n", victim);
                    else if ((prevCharged < (threshold / 2) && totalCharged >= (threshold / 2))
                    || (prevCharged < (threshold / 4) && totalCharged >= (threshold / 4)))
                        send_to_char("You feel the power surge within you as you absorb the energy.\n", victim);
                }
            }
            else
                check_improve(victim, ch, gsn_shockcraft, false, 6);
        }

        // Check for shockcraft discharging
        if (get_charging_effect(ch) == NULL)
        {
            AFFECT_DATA * charged(get_charged_effect(ch));
            if (charged != NULL && charged->modifier > 0)
            {
                // Add damage bonus and decrease the charged amount
                int bonus = number_range(1, 15);
                dam = ((dam * (109 + bonus)) / 100);
                bonus /= 3;
                charged->modifier -= bonus;
                ch->max_mana -= bonus;
                if (charged->modifier <= 0)
                {
                    affect_remove(ch, charged);
                    send_to_char("You exhaust the charge within you, draining its last vestiges into your attack.\n", ch);
                }
            }
        }

        // Check for arc shield
        if (number_percent() <= get_skill(victim, gsn_arcshield))
        {
            check_improve(victim, ch, gsn_arcshield, true, 6);

            // Check whether effect already present
            int oldModifier;
            int newModifier;
            AFFECT_DATA * paf(get_affect(victim, gsn_arcshield));
            if (paf == NULL)
            {
                // Not already present, so add it
                AFFECT_DATA af = {0};
                af.where    = TO_AFFECTS;
                af.type     = gsn_arcshield;
                af.level    = victim->level;
                af.duration = 4;
                af.modifier = 1;
                af.location = APPLY_HIDE;
                affect_to_char(victim, &af);

                oldModifier = 0;
                newModifier = 1;
            }
            else
            {
                // Effect is present, so check it
                oldModifier = paf->modifier;
                ++paf->modifier;
                paf->duration = 4;
                newModifier = paf->modifier;
            }

            int maxHits(4 + (victim->level / 5));
            if (oldModifier <= maxHits)
            {
                // Have not yet used up the shield
                if (newModifier > maxHits) 
                    send_to_char("Your arc shield sputters and sparks, then fails completely!\n", victim);
                else if (oldModifier <= ((maxHits * 8) / 10) && newModifier > ((maxHits * 8) / 10))
                    send_to_char("Your arc shield flickers and stalls briefly, then flares back up with a humming sound!\n", victim);
                else if (oldModifier <= (maxHits / 2) && newModifier > (maxHits / 2))
                    send_to_char("Sparks fly off your arc shield as it absorbs the blow!\n", victim);
                else if (oldModifier == 0 && newModifier > 0)
                    send_to_char("Your arc shield crackles as it absorbs the energy!\n", victim);

                immune = true;
                return 0;
            }
        }
        else
            check_improve(victim, ch, gsn_arcshield, false, 6);
    }

    // Check for soulbrand
    if (ch_weapon != NULL)
    {
        AFFECT_DATA * soulbrand(get_obj_affect(ch_weapon, gsn_soulbrand));
        if (soulbrand != NULL)
        {
            // Look for the caster
            CHAR_DATA * soulbrandCaster(get_char_by_id_any(soulbrand->modifier));
            int manaCost(number_range(2, 8));
            if (soulbrandCaster != NULL && soulbrandCaster->mana >= manaCost)
            {
                dam  = (dam * 23) / 20;
                expend_mana(soulbrandCaster, manaCost);
            }
        }
    }

    // Check for sanctuary / Wrath of An'Akarta
    if (dam_type != DAM_DEFILEMENT && (victim->in_room == NULL || !room_is_affected(victim->in_room, gsn_desecration)))
    {
        AFFECT_DATA * wrathAffect(get_affect(ch, gsn_wrathofanakarta));
        if (wrathAffect == NULL || wrathAffect->modifier != 0)
        {
            // Damager does not have an active wrath of An'Akarta affect, so check sanctuary
            if ((dam > 1) && IS_AFFECTED(victim, AFF_SANCTUARY))
            {
                int reduction(40);
                if (victim->in_room != NULL && room_is_affected(victim->in_room, gsn_ordainsanctum)) reduction += 20;
                if (IS_NPC(victim)) reduction += 10;
                dam = (dam * (100 - reduction)) / 100;
            }
        }
    }

    // Check for mantle of rain
    if (is_affected(victim, gsn_mantleofrain))
    {
        switch (dam_type)
        {
            // 15% reduction to fire/slashing/light/sound damage
            case DAM_FIRE:
            case DAM_SLASH:
            case DAM_LIGHT:
            case DAM_SOUND:
                dam = (dam * 17) / 20;
                break;

            // 35% reduction to bashing damage
            case DAM_BASH:
                dam = (dam * 13) / 20;
                break;

            // 25% increase to lightning and drowning damage
            case DAM_LIGHTNING:
            case DAM_DROWNING:
                dam = (dam * 5) / 4;
                break;
        }
    }
    
    if ((dam > 1) && is_affected(victim,gsn_reflectiveaura))
	if (dam_type == DAM_LIGHT)
	    if (dt == gsn_soulflare)
		dam /= 2;
	    else
	    	dam = (dam * 8) / 10;
    
    // Every point of the group leader's charisma above 20 yields an extra 1% of damage
    // Above 18 if the race is the same
    if (ch != NULL && ch->leader != NULL && ch->leader != ch)
    {
 	    int threshold_chr = 20;
    	if (ch->leader->race == ch->race)
    	    threshold_chr -= 2;
	    dam = (dam * UMAX(100, 100 + get_curr_stat(ch->leader, STAT_CHR) - threshold_chr)) / 100;

        // Check for celestial tactician on the group's leader
        if (number_percent() <= get_skill(ch->leader, gsn_celestialtactician))
        {
            dam = (dam * 23) / 20;
            check_improve(ch->leader, victim, gsn_celestialtactician, true, 2);
        }

        // Check for avatar of the lodestar on the group's leader
        int avatarType(type_of_avatar(ch->leader));
        if (avatarType == gsn_avatarofthelodestar)
            dam = (dam * 21) / 20;
    }

    // Firekin means a (3 + (level / 3))% discount on fire damage
    if (dam > 1 && dam_type == DAM_FIRE && number_percent() <= get_skill(victim, gsn_firekin))
    {
        dam = (dam * (100 - (3 + (victim->level / 3)))) / 100;
        check_improve(victim, NULL, gsn_firekin, TRUE, 8);
    }

    // Frostkin means a (3 + (level / 3))% discount on cold damage
    if (dam > 1 && dam_type == DAM_COLD && number_percent() <= get_skill(victim, gsn_frostkin))
    {
        dam = (dam * (100 - (3 + (victim->level / 3)))) / 100;
        check_improve(victim, NULL, gsn_frostkin, TRUE, 8);
    }

    CHAR_DATA *phch(ch);
    if (ch != NULL && IS_NPC(ch) && ch->master && IS_SET(ch->act,ACT_ILLUSION))
    	phch = ch->master;

    if (phch != NULL && dam > 1 && is_affected(victim, gsn_aegisoflaw))
    {
	if (IS_AFFECTED(victim, AFF_SANCTUARY))
	    if (IS_CRIMINAL(phch))
	        dam = dam * 74 / 100;
	    else if (IS_CHAOTIC(phch))
	        dam = dam * 9 / 10;
	    else {}
	else
	    if (IS_CRIMINAL(phch))
	        dam /= 2;
	    else if (IS_CHAOTIC(phch))
	        dam = (4 * dam) / 5;
    }

    // Check for PC only modifiers
    if (ch != NULL && !IS_NPC(ch))
    {
        if (BIT_GET(ch->pcdata->traits, TRAIT_MARKED))
        {
            if (IS_TRUE_EVIL(victim) && ch->religion == god_lookup("Rystaia"))
                dam = dam * 21 / 20;

            if (IS_TRUE_GOOD(victim) && ch->religion == god_lookup("Arkhural"))
                dam = dam * 21 / 20;

            if (IS_CHAOTIC(victim) && ch->religion == god_lookup("Rveyelhi"))
                dam = dam * 21 / 20;

            if (IS_LAWFUL(victim) && ch->religion == god_lookup("Girikha"))
                dam = dam * 21 / 20;
        }

        // Check for somatic arts
        if (number_percent() <= get_skill(ch, gsn_somaticarts))
        {
            int somaticSkill(0);
            if (ch->pcdata->somatic_arts_info != NULL)
            {
                somaticSkill = ch->pcdata->somatic_arts_info->SkillFor(victim->race);
                if (somaticSkill == SomaticArtsInfo::Unknown)
                    somaticSkill = 0;

                if (ch->pcdata->somatic_arts_info->CheckImproveRace(victim->race, ch->race == victim->race, true, get_curr_stat(ch, STAT_INT)))
                {
                    std::ostringstream mess;
                    mess << "Your understanding of " << race_table[victim->race].name << " physiology deepens!\n";
                    send_to_char(mess.str().c_str(), ch);
                }
            }
 
            dam = (dam * (103 + (somaticSkill / 80))) / 100;
            check_improve(ch, victim, gsn_somaticarts, true, 8);
        }
        else
            check_improve(ch, victim, gsn_somaticarts, false, 8);
    }

    if (phch != NULL && dam > 1 && ((IS_AFFECTED(victim, AFF_PROTECT_EVIL) && IS_TRUE_EVIL(phch)) || (IS_AFFECTED(victim, AFF_PROTECT_GOOD) && IS_TRUE_GOOD(phch))))
        dam -= dam / 4;

    if (phch != NULL && dam > 1)
    {
        // Handle avatar/positive channel reduction
        bool isEvil(aura_grade(phch) > 0);
        int avatarType(type_of_avatar(victim));
        int reduction(0);
    	
        if (avatarType == gsn_avataroftheprotector) reduction = (isEvil ? 60 : 10);
        else if (avatarType != -1)                  reduction = (isEvil ? 50 :  0);
        else if (IS_OAFFECTED(victim, AFF_POSCHAN)) reduction = (isEvil ?  5 :  0);
        
        dam = (dam * (100 - reduction)) / 100;
    }

    if (phch != NULL && !IS_NPC(victim) && BIT_GET(victim->pcdata->traits, TRAIT_MARKED))
    {
        if (IS_TRUE_EVIL(phch) && (victim->religion == god_lookup("Jolinn") || victim->religion == god_lookup("Serachel")))
        	dam = dam * 19 / 20;

        if (!IS_TRUE_NEUTRAL(phch) && victim->religion == god_lookup("Enaerai"))
        	dam = dam * 97 / 100;
    }

    /* CHECKS FOR MATERIAL IMMUNITIES SHOULD GO HERE */
    immune = (get_resist(victim, TYPE_HIT + dam_type) == 100);
    if (immune)
        return 0;

    dam -= ((dam * get_resist(victim, TYPE_HIT + dam_type)) / 100);

    // Check for spirit shield/aegis of grace
    if (dam_type == DAM_NEGATIVE || dam_type == DAM_DEFILEMENT 
    || (ch != NULL && (is_demon(ch) || (IS_NPC(ch) && IS_SET(ch->act, ACT_UNDEAD)))))
    {
        if (is_affected(victim, gsn_spiritshield) || is_affected(victim, gsn_lesserspiritshield))
  	        dam = (dam * 7) / 10;
        
        AFFECT_DATA * aegis(get_affect(victim, gsn_aegisofgrace));
        if (aegis != NULL)
            dam = (dam * (100 - aegis->modifier)) / 100;
    }
    
    if (is_affected(victim, gsn_astralform))
    {
        // Ch'taren get extra resistance, since they were already shifted
    	int mod = 98;
	    if (victim->race == global_int_race_chtaren)
  	        mod = 96;

    	switch (dam_type)
	    {
	        case DAM_BASH:
    	    case DAM_PIERCE:
	        case DAM_SLASH: dam = (dam * mod) / 100; break;				
        }
    }

    // +20% damage each from delusions, deathly visage, barbs of althajji
    if (is_affected(victim, gsn_delusions)) dam = (dam * 6) / 5;
    if (is_affected(victim, gsn_deathlyvisage)) dam = (dam * 6) / 5;
    if (is_affected(victim, gsn_barbsofalthajji)) dam = (dam * 6) / 5;

    if (victim->in_room != NULL)
    {
        int mod(aura_grade(victim));

        // Sanctify means goodies are hurt less and evils are hurt more
        if (room_is_affected(victim->in_room, gsn_sanctify))
        {
            int extraDam(0);
            switch (URANGE(-3, mod, 3))
            {
                case -3: extraDam = -25; break;
                case -2: extraDam = -20; break;
                case -1: extraDam = -10; break;
                case  0: extraDam =   0; break;
                case  1: extraDam =  10; break;
                case  2: extraDam =  20; break;
                case  3: extraDam =  25; break;
                default: bug("Unexpected modifier in sanctify damage check", 0); break;
            }

            dam = (dam * (100 + extraDam)) / 100;
        }

        // Canticle of the lightbringer means evils are hurt more
        if (mod > 0 && room_is_affected(victim->in_room, gsn_canticleofthelightbringer))
            dam = (dam * (100 + mod * 5)) / 100;

        // Holy ground means goodies are hurt less
        if (mod < 0 && area_is_affected(victim->in_room->area, gsn_holyground))
            dam = (dam * (100 + mod * 5)) / 100;

        // Check for abode of the spirit
        for (AFFECT_DATA * abode(get_affect(victim, gsn_abodeofthespirit)); abode != NULL; abode = get_affect(victim, gsn_abodeofthespirit, abode))
        {
            if (abode->modifier == victim->in_room->vnum)
            {
                dam = (dam * 3) / 4;
                break;
            }
        }

        // Check for lattice of stone
        if (!isPhysical && room_is_affected(victim->in_room, gsn_latticeofstone))
        {
            int reduction(15 + (determine_saltoftheearth_level(*victim, SPH_SPIRIT) / 5));
            dam = (dam * (100 - reduction)) / 100;
        }

        // Check for masquerade
        if (dam_type == DAM_HOLY)
        {
            AFFECT_DATA * masquerade(get_area_affect(victim->in_room->area, gsn_masquerade));
            if (masquerade != NULL && (ch == NULL || (ch->id != masquerade->modifier && number_percent() > get_skill(ch, gsn_disillusionment))))
                dam = (dam * 3) / 5;
        }
    }

    // Check form of the wraith / cloak of the void
    if (is_affected(victim,gsn_form_of_the_wraith))
    {
	    if (dam_type == DAM_HOLY) dam = (dam * 5) / 4;
    	else if (dam > 1) dam = (dam * 65) / 100;
    }
    else if (is_affected(victim, gsn_cloakofthevoid))
    {
        int mod(0);
        if (victim->in_room != NULL && room_is_affected(victim->in_room, gsn_desecration))
            mod = 1;

        switch (dam_type)
        {
            case DAM_HOLY:          dam = (dam * (11 - mod)) / 10;  break;
            case DAM_COLD:          dam = (dam * (6 - mod)) / 10;   break;
            case DAM_NEGATIVE:
            case DAM_DEFILEMENT:
            case DAM_SLASH:
            case DAM_BASH:
            case DAM_PIERCE:        dam = (dam * (8 - mod)) / 10;   break;
            default:                dam = (dam * (10 - mod)) / 10;  break;
        }
    }

    // Check for a python familiar
    if (isPhysical && is_familiar_present(*victim, gsn_callserpent, OBJ_VNUM_FAMILIAR_SERPENT))
        dam = (dam * 19) / 20;

    // Check for Molten Shield
    bool validRetribution(ch != NULL && ch != victim && (!IS_NPC(ch) || !IS_SET(ch->act, ACT_ILLUSION)));
    if (validRetribution && number_percent() <= 50 && is_affected(victim, gsn_moltenshield))
    {
        act("A burning spray of liquid rock catches $n as $s attack strikes $N's molten shield!", ch, NULL, victim, TO_NOTVICT);
        act("A burning spray of liquid rock catches $n as $s attack strikes your molten shield!", ch, NULL, victim, TO_VICT);
        act("A burning spray of liquid rock catches you as your attack strikes $N's molten shield!", ch, NULL, victim, TO_CHAR);
        damage_new(victim, ch, number_range(4, 12), gsn_moltenshield, DAM_FIRE, true, "molten stone");
    }

    // Check luck; validRetribution helps avoid some weird echoes
    if (validRetribution && dam > 1)
    {
        switch (Luck::CheckOpposed(*victim, *ch))
        {
            case Luck::Lucky: dam /= 2; break;
            case Luck::Unlucky: dam = (dam * 3) / 2; break;
            default: break;
        }
    }

    // Check for hemoplague
    if (validRetribution && number_bits(2) == 0 && !is_affected(ch, gsn_plague))
    {
        AFFECT_DATA * hemoplague(get_affect(victim, gsn_hemoplague));
        if (hemoplague != NULL)
        {
            // Echo about the spray
            act("$N's infected blood sprays all over you!", ch, NULL, victim, TO_CHAR);
            act("Your infected blood sprays all over $n!", ch, NULL, victim, TO_VICT);
            act("$N's infected blood sprays all over $n!", ch, NULL, victim, TO_NOTVICT);

            // Check for save
            if (!saves_spell(hemoplague->level, victim, ch, DAM_DISEASE))
            {
                // Failed the save
                send_to_char("You scream in agony as plague sores erupt from your skin.\n", ch);
                act("$n screams in agony as plague sores erupt from $s skin.", ch, NULL, NULL, TO_ROOM);

                AFFECT_DATA haf = {0};
                haf.where   = TO_AFFECTS;
                haf.type    = gsn_plague;
                haf.level   = (hemoplague->level * 3) / 4;
                haf.duration = hemoplague->level;
                haf.location = APPLY_STR;
                haf.modifier = -5;
                haf.bitvector = AFF_PLAGUE;
                affect_to_char(ch, &haf);
            }
        }
    }

    // Check for Jolt Ward
    if (validRetribution && number_percent() <= 50)
    {
        AFFECT_DATA * jolt(get_affect(victim, gsn_joltward));
        if (jolt != NULL)
        {
            int manaCost(number_range(5, 25));
            CHAR_DATA * caster(get_char_by_id(reinterpret_cast<int>(jolt->point)));
            if (caster != NULL && caster->mana >= manaCost)
            {
                caster->mana -= manaCost;
                act("An arc of crackling electricity leaps out from you!", victim, NULL, NULL, TO_CHAR);
                act("An arc of crackling electricity leaps out from $n!", victim, NULL, NULL, TO_ROOM);
                int effLevel(jolt->level);
                if (ch->level < 25) effLevel = UMIN(ch->level, effLevel);
                damage_old(victim, ch, 3 + number_range(effLevel / 10, effLevel / 2), gsn_joltward, DAM_LIGHTNING, true);
            }
        }
    }

    // Check for Chillfire Shield
    AFFECT_DATA * chillfireShield(get_affect(victim, gsn_chillfireshield));
    if (validRetribution && chillfireShield != NULL && chillfireShield->modifier >= 0)
    {
        // Handle reduction
        int reduction(dam / 4);
        dam -= reduction;

        // Update the shield
        int prevMod(chillfireShield->modifier);
        chillfireShield->modifier += reduction;
        
        // Check for shield destruction
        if (number_percent() < (chillfireShield->modifier - 500) / 100)
        {
            // Reset to just a cooldown
            chillfireShield->duration = UMAX(12, 17 - ((get_skill(victim, gsn_chillfireshield) - 70) / 6));
            chillfireShield->modifier = -1;
            
            act("The force field around $n shatters from the flames, sending flares of heat at $N!", victim, NULL, ch, TO_NOTVICT);
            act("The force field around you shatters from the flames, sending flares of heat at $N!", victim, NULL, ch, TO_CHAR);
            act("The force field around $n shatters from the flames, sending flares of heat at you!", victim, NULL, ch, TO_VICT);
            damage_new(victim, ch, dice(chillfireShield->level, 3), gsn_chillfireshield, DAM_FIRE, true, "backlash");
        }
        else if (prevMod == 0) send_to_char("You feel the dark flames around your force shield begin to warm up.\n", victim);
        else if (prevMod < 200 && chillfireShield->modifier >= 200) send_to_char("The flames around your force shield grow noticeably warmer.\n", victim);
        else if (prevMod < 350 && chillfireShield->modifier >= 350) send_to_char("Your force shield starts to hum as the dark flames around it heat up even more.\n", victim);
        else if (prevMod < 500 && chillfireShield->modifier >= 500)
        {
            act("The dark flames covering the force shield around $n flicker with unusual intensity, and begin to radiate substantial heat.", victim, NULL, NULL, TO_ROOM);
            act("The dark flames covering the force shield around you flicker with unusual intensity, and begin to radiate substantial heat.", victim, NULL, NULL, TO_CHAR);
        }
        else if (prevMod < 700 && chillfireShield->modifier >= 700)
        {
            act("The flames surrounding the force shield around $n grow bright, burning with intense heat!", victim, NULL, NULL, TO_ROOM);
            act("The flames surrounding the force shield around you grow bright, burning with intense heat!", victim, NULL, NULL, TO_CHAR);
        }
        else if (prevMod < 1000 && chillfireShield->modifier >= 1000)
        {
            act("The force shield around $n blazes with bright orange flame!", victim, NULL, NULL, TO_ROOM);
            act("The force shield around you blazes with bright orange flame!", victim, NULL, NULL, TO_CHAR);
        }
    }

    // Check for Essence of the Inferno
    PyreInfo * pyreInfo(getPyreInfoEffect(victim, PyreInfo::Effect_EssenceOfTheInferno));
    if (pyreInfo != NULL)
    {
        // Victim has Essence of the Inferno; calculate the power of the damage reduction
        int reduction = UMAX(0, (get_skill(victim, gsn_bloodpyre) - 70) / (pyreInfo->isEffectGreater() ? 2 : 3));
        if (dam_type != DAM_COLD) reduction += 25;
            
        // Calculate the amount of damage directed to the Inferno
        int damageRedirected = UMIN(dam, (dam * reduction) / 100);
        dam -= damageRedirected;
            
        // Update the modifier
        if (pyreInfo->isEffectGreater())
        {
            unsigned int oldModifier = pyreInfo->effectModifier();
            unsigned int newModifier = UMIN(350, oldModifier + (damageRedirected / 10));
            pyreInfo->setEffectModifier(newModifier);
            if (oldModifier <= 50 && newModifier > 50) send_to_char("You feel the Essence of the Inferno grow angry within you.\n", victim);
            else if (oldModifier <= 100 && newModifier > 100) send_to_char("The heat of the Inferno within you grows more intense as its fury begins to swell!\n", victim);
            else if (oldModifier <= 150 && newModifier > 150) send_to_char("Your vision blurs; for a moment, you can see only blood and feel only hate.\n", victim);
            else if (oldModifier <= 200 && newModifier > 200) send_to_char("You sense the Inferno's rage reach new heights; it threatens to break free!\n", victim);
            else if (oldModifier <= 250 && newModifier > 250) send_to_char("The Inferno rampages within you! You can scarcely contain it!\n", victim);

            if (validRetribution && number_percent() < ((static_cast<int>(pyreInfo->effectModifier()) - 150) / 10))
            {
                // Time to explode all that charged-up damage
                act("A sudden, furious explosion of fire surges from $n, striking $N with white-hot flames!", victim, NULL, ch, TO_NOTVICT);
                act("A sudden, furious explosion of fire surges from $n, striking you with white-hot flames!", victim, NULL, ch, TO_VICT);
                act("You feel the Inferno within you overflow with rage, bursting from you in a furious flare of heat!", victim, NULL, ch, TO_CHAR);
                damage_new(victim, ch, pyreInfo->effectModifier(), gsn_bloodpyre, DAM_FIRE, true, "fiery retribution");
                pyreInfo->setEffectModifier(0);
                send_to_char("Its energy discharged, the Inferno quiets within you, if only for a time.\n", victim);
            }
        }
    }

    // Check for soulfire shield
    AFFECT_DATA * soulfire(get_affect(victim, gsn_soulfireshield));
    if (dam_type != DAM_BASH && dam_type != DAM_SLASH && dam_type != DAM_PIERCE && soulfire != NULL)
    {
        // Convert some of the magical damage to mana damage
        int manaDamage = UMIN(dam, UMAX(0, (dam * soulfire->modifier) / 100));
        manaDamage = UMIN(manaDamage, victim->mana);
        dam -= manaDamage;
        expend_mana(victim, manaDamage);
    }

    // Check for thermal mastery
    if (ch != NULL && (dam_type == DAM_FIRE || dam_type == DAM_COLD))
    {
        int thermalMasterySkill(get_skill(ch, gsn_thermalmastery));
        if (thermalMasterySkill > 0)
        {
            if (number_percent() < thermalMasterySkill)
            {
                dam += 1 + ((dam * ch->level) / (5 * 100));
                check_improve(ch, victim, gsn_thermalmastery, TRUE, 8);
            }
            else
                check_improve(ch, victim, gsn_thermalmastery, FALSE, 8);
        }
    }

    // Fire-specific boosts
    if (dam_type == DAM_FIRE)
    {
        // Check for geothermics
        if (ch != NULL && ch->in_room != NULL && ch->in_room->sector_type == SECT_UNDERGROUND && number_percent() < get_skill(ch, gsn_geothermics))
        {
            dam += ((dam * 3) / 20);
            check_improve(ch, victim, gsn_geothermics, TRUE, 8);
        }

        // Check for Bayyal's Realm
        std::vector<std::pair<CHAR_DATA*, PyreInfo*> > pyreData(getPyreInfoEffectsArea(victim, PyreInfo::Effect_BayyalsRealm));
        if (pyreData.size() > 0)
        {
            // Calculate the damage increase
            bool anyGreater = false;
            int damBonus = 16 + ((pyreData.size() - 1) * 5);
            for (size_t i(0); i < pyreData.size(); ++i)
            {
                if (pyreData[i].second->isEffectGreater())
                {
                    anyGreater = true;
                    damBonus += 2;
                }
            }

            if (anyGreater)
                damBonus += 6;

            dam += (dam * damBonus) / 100;
        }

        // Check for heartfire
        AFFECT_DATA * heartFireAff(get_affect(ch, gsn_heartfire));
        if (heartFireAff != NULL && heartFireAff->duration < 0)
            dam += (dam * heartFireAff->modifier) / 100;

        // Check for burning mind
        if (victim->mana < victim->max_mana)
        {
            int burningmindSkill = get_skill(victim, gsn_burningmind);
            if (burningmindSkill > 0)
            {
                if (number_percent() < burningmindSkill)
                {
                    victim->mana = UMIN(victim->max_mana, (victim->mana + 1 + (dam / 3)));
                    send_to_char("You feel some of the heat energy burn into your mind, restoring it.\n", victim);
                    check_improve(victim, ch, gsn_burningmind, TRUE, 2);
                }
                else
                    check_improve(victim, ch, gsn_burningmind, FALSE, 2);
            }
        }
    }

    // Check for revenant mana restoration
    if (dam_type == DAM_NEGATIVE && dam > 0 && victim->mana < victim->max_mana 
    && number_percent() <= get_skill(victim, gsn_revenant))
    {
        victim->mana = UMIN(victim->max_mana, victim->mana + 1 + (dam / 3));
        send_to_char("The negative energy fills your mind, restoring it.\n", victim);
    }

    // Cold-specific boosts
    if (dam_type == DAM_COLD)
    {
        if (ch != NULL)
        {
            // Check for arctic chill
            AFFECT_DATA * arctic(get_affect(ch, gsn_arcticchill));
            if (arctic != NULL && arctic->modifier == 0)
                dam = (dam * 21) / 20;

            if (ch->in_room != NULL)
            {
                // Check for winter's stronghold
                AFFECT_DATA * stronghold(get_room_affect(ch->in_room, gsn_wintersstronghold));
                if (stronghold != NULL && stronghold->modifier == 1)
                    dam = (dam * 23) / 20;
            }
        }
    }

    // Check for weavecraft damage reduction
    if (victim->in_room != NULL && number_percent() <= get_skill(victim, gsn_weavecraft))
    {
        // Get the ley modifiers
        int positivePower(Weave::AbsolutePositivePower(*victim->in_room));
        int orderPower(Weave::AbsoluteOrderPower(*victim->in_room));
        int reduction(0);
            
        // Base the reduction on damage type
        switch (dam_type)
        {
            // Protect from negative types with positive power
            case DAM_NEGATIVE: case DAM_DEFILEMENT: case DAM_FEAR:
                if (positivePower > 0) reduction = UMIN(48, positivePower * 12);
                break;

            // Protect from positive types with negative power
            case DAM_HOLY: case DAM_LIGHT:
                if (positivePower < 0) reduction = UMIN(48, -positivePower * 12);
                break;

            // Protect from chaotic types with ordered power
            case DAM_FIRE: case DAM_LIGHTNING: case DAM_ILLUSION:
                if (orderPower > 0) reduction = UMIN(48, orderPower * 12);
                break;

            // Protect from ordered types with chaotic power
            case DAM_COLD: case DAM_SOUND: case DAM_MENTAL:
                if (orderPower < 0) reduction = UMIN(48, -orderPower * 12);
                break;

            // Protect from physical types with overall power
            case DAM_SLASH: case DAM_PIERCE: case DAM_BASH:
                reduction = UMIN(20, (abs(positivePower) + abs(orderPower)) * 5);
                break;
        }

        // Reduce the damage
        if (reduction > 0)
        {
            check_improve(victim, NULL, gsn_weavecraft, true, 4);
            dam = (dam * (100 - reduction)) / 100;
        }
    }

    // Check for celestial bulwark damage reduction
    if (dam_type == DAM_NEGATIVE || dam_type == DAM_FEAR || dam_type == DAM_DEFILEMENT)
    {
        OBJ_DATA * shieldObj(get_eq_char(victim, WEAR_SHIELD));
        if (shieldObj != NULL && shieldObj->pIndexData->vnum == OBJ_VNUM_CELESTIALBULWARK)
        {
            static const int chance_table[] = {10, 18, 26, 34, 40, 46, 52, 56, 60, 62, 64};
            static const int chance_table_count = sizeof(chance_table) / sizeof(chance_table[0]);
            
            // Calculate chance of damage negation from the table (values are tenths of a percent); each extra boost beyond the table gives +1 more
            int chance;
            int boostCount(count_obj_affects(shieldObj, gsn_bindessence));
            if (boostCount < chance_table_count) chance = chance_table[boostCount];
            else chance = chance_table[chance_table_count - 1] + boostCount - chance_table_count;
           
            // Check whether damage is negated; hard cap at 200 (20%) 
            if (number_range(0, 1000) < UMIN(200, chance))
            {
                act("A burst of golden light flares from $p, absorbing the negative energy!", victim, shieldObj, NULL, TO_ALL);
                return 0;
            }
        }
    }

    // Check for parasitic bond damage transference
    AFFECT_DATA * parasite(get_affect(victim, gsn_parasiticbond));
    if (parasite != NULL && parasite->modifier != 0)
    {
        // Look up the char by id
        CHAR_DATA * host(get_char_by_id_any(parasite->modifier));
        if (host != NULL && is_affected(host, gsn_parasiticbond))
        {
            // Found a host, give it 25% of the damage
            int transferDam(dam / 4);
            dam -= transferDam;
            host->hit -= transferDam;

            // Check for death from the transfer
            update_pos(host);
            if (host->position == POS_DEAD)
            {
                act("You draw too much life from $N, and sense the link between you vanish as $E dies.", victim, NULL, host, TO_CHAR);
                act("$n draws away the last of your life force, and you collapse, utterly spent.", victim, NULL, host, TO_VICT);
                act("$n suddenly collapses, $s body withering before your sight!", host, NULL, NULL, TO_ROOM);

                std::ostringstream mess;
                mess << host->name << " died from a parasitic bond with " << victim->name << " in " << host->in_room->name << " [" << host->in_room->vnum << "]";
                wiznet(const_cast<char*>(mess.str().c_str()), NULL, NULL, WIZ_DEATHS, 0, 0);
                log_string(mess.str().c_str());
                raw_kill(host);

                // Remove the cooldown
                affect_remove(victim, parasite);
            }
        }
    }

    // Check for aura of empathy damage transference
    if (victim->in_room != NULL)
    {
        CHAR_DATA * gch_next;
        for (CHAR_DATA * gch(victim->in_room->people); gch != NULL; gch = gch_next)
        {
            // Filter out the target and any non-groupmates
            gch_next = gch->next_in_room;
            if (gch == victim || !is_same_group(gch, victim))
                continue;

            // Check for aura of empathy
            AFFECT_DATA * spiritAura(get_affect(gch, gsn_radiateaura));
            if (spiritAura == NULL || spiritAura->modifier != Aura_Empathy)
                continue;

            // Aura present, perform the skill check
            if (number_percent() > get_skill(gch, gsn_radiateaura))
            {
                check_improve(gch, ch, gsn_radiateaura, false, 2);
                continue;
            }
            
            // Skill check passed, transfer some of the damage
            check_improve(gch, ch, gsn_radiateaura, true, 2);
            int transferDam(dam / 4);
            dam -= transferDam;
            gch->hit -= transferDam;

            // Check for death from the transfer
            update_pos(gch);
            if (gch->position == POS_DEAD)
            {
                act("You tremble, having taken too much damage for $N.", gch, NULL, victim, TO_CHAR);
                act("$n trembles, having taken too much damage for you.", gch, NULL, victim, TO_VICT);
                act("$n trembles, having taken too much damage for $N.", gch, NULL, victim, TO_NOTVICT);
                act("You collapse, your body withering to a pale, translucent grey in seconds.", gch, NULL, victim, TO_CHAR);
                act("$n collapses, $s body withering to a pale, translucent grey in seconds.", gch, NULL, victim, TO_ROOM);

                std::ostringstream mess;
                mess << gch->name << " died from an excess of empathy for " << victim->name << " in " << gch->in_room->name << " [" << gch->in_room->vnum << "]";
                wiznet(const_cast<char*>(mess.str().c_str()), NULL, NULL, WIZ_DEATHS, 0, 0);
                log_string(mess.str().c_str());
                raw_kill(gch);
            }
        }
    }

    if (ch != NULL)
    {
        // Check for boost from bindessence handgear
        if (dam_type == DAM_HOLY)
        {
            OBJ_DATA * bound(get_eq_char(ch, WEAR_HANDS));
            if (bound != NULL)
            {
                AFFECT_DATA * boundAff(get_obj_affect(bound, gsn_bindessence));
                if (boundAff != NULL)
                    dam = (dam * (100 + (boundAff->level / 10))) / 100;
            }
        }

        if ((IS_NAFFECTED(ch, AFF_RALLY) || IS_PAFFECTED(ch, AFF_MAUL)) && !(IS_NPC(ch) && IS_SET(ch->act, ACT_ILLUSION)))
    	    dam = (dam * 5) / 4;

        if (IS_NAFFECTED(ch, AFF_RAGE))
	        dam += dam/3;

        if (is_affected(ch,gsn_tendrilgrowth))
	        dam += dam/4;
    }

    if (dt == gsn_predatoryattack && victim->hit < victim->max_hit / 2)
	dam = dam * 12 / 10;

	int acvalue = 0; 

	switch (dam_type)
	{
	    case DAM_SLASH:	acvalue = GET_AC(victim, AC_SLASH);	break;
	    case DAM_PIERCE:	acvalue = GET_AC(victim, AC_PIERCE);	break;
	    case DAM_BASH:	acvalue = GET_AC(victim, AC_BASH);	break;
	    default:		acvalue = GET_AC(victim, AC_EXOTIC);	break;
	}

	if (acvalue >= -150)
	    dam = ((dam * (100 + (acvalue / 50))) / 100);
	else
	    dam = ((dam * (97 + ((acvalue + 150) / 30))) / 100);

    if (victim->in_room)
    {
        if (victim->in_room->sector_type == SECT_UNDERWATER)
        {
            if (dam_type == DAM_LIGHTNING)
                dam = (dam * 13) / 10;
            else if ((dam > 3) && (dam_type == DAM_FIRE) && dt != gsn_boilseas)
                dam /= 4;
        }

        if (dam_type == DAM_LIGHT && area_is_affected(victim->in_room->area, gsn_brume))
            dam = (dam * 3) / 4;
    }

    if (AFFECT_DATA * song = get_group_song(victim, gsn_aegisofmusic))
    {
	    if (can_be_affected(victim,gsn_aegisofmusic) && !is_an_avatar(victim))
        {
	        if (IS_AFFECTED(victim, AFF_SANCTUARY))
	    	    dam = dam * (100 - ((CHAR_DATA *) song->point)->level) / 83;
    	    else
	        	dam = dam * (100 - ((CHAR_DATA *) song->point)->level) / 100;	
        }
    }

    ////////// This section is for skills which reduce via subtraction rather than division; for maximum effect they are placed at the end //////////
    if (dam > 1)
    {
        // Check for durability 
        if (number_percent() <= get_skill(victim, gsn_durability))
        {
            dam -= UMAX(0, (get_curr_stat(victim, STAT_STR) + get_curr_stat(victim, STAT_CON) - 34) / 2);
            if (isPhysical) dam -= (victim->level / 3);
            else dam -= (victim->level / 5);
            check_improve(victim, ch, gsn_durability, true, 8);
        }
        else
            check_improve(victim, ch, gsn_durability, false, 8);

        // Cannot reduce past 1
        dam = UMAX(dam, 1);

        // Check for fugue
        if (isPhysical && dam > 1)
        {
            AFFECT_DATA * fugue(get_affect(victim, gsn_fugue));
            if (fugue != NULL && fugue->location != APPLY_NONE)
                dam = UMAX(1, dam - ((fugue->level * 2) / 3));
        }

        // Check for embrace of the deeps
        AFFECT_DATA * song(get_group_song(victim, gsn_theembraceofthedeeps));
        if (song != NULL && can_be_affected(victim, gsn_theembraceofthedeeps) && !is_an_avatar(victim))
        {
            // Calculate reduction and check for effect
            int reduction(UMIN(dam, song->level / 3));
            int hpMod(0);
            AFFECT_DATA * embrace(get_affect(victim, gsn_theembraceofthedeeps));
            if (embrace != NULL)
            {
                hpMod = embrace->modifier;
                affect_remove(victim, embrace);
            }

            // Make sure the victim can handle the life reduction
            if (reduction > 0 && (ch->max_hit + hpMod - 1) >= 250)
            {
                // Echo if this is the first time
                if (embrace == NULL)
                {
                    act("Thick sludge wells up over your wounds, reducing the pain of the blow!", victim, NULL, NULL, TO_CHAR);
                    act("Thick sludge wells up over $n's wounds, coating them protectively!", victim, NULL, NULL, TO_ROOM);
                }

                // Perform the reduction
                dam = UMAX(1, dam - reduction);
                --hpMod;
            }

            // (Re)apply the effect
            AFFECT_DATA eaf = {0};
            eaf.where   = TO_AFFECTS;
            eaf.type    = gsn_theembraceofthedeeps;
            eaf.level   = song->level;
            eaf.duration = 4;
            eaf.location = APPLY_HIT;
            eaf.modifier = hpMod;
            affect_to_char(victim, &eaf);
            victim->hit = UMIN(victim->hit, victim->max_hit);
        }
    }

    // Check for momentum 
    if (ch != NULL && (dt >= TYPE_HIT || dt == gsn_whirl))
    {
        if (number_percent() <= get_skill(ch, gsn_momentum))
        {
            if (ch_weapon != NULL && IS_WEAPON_STAT(ch_weapon, WEAPON_TWO_HANDS))
                dam += get_skill(ch, gsn_momentum) / 4;
            else if (IS_PAFFECTED(ch, AFF_TWOHAND))
                dam += get_skill(ch, gsn_momentum) / 6;
		  
    	    check_improve(ch, victim, gsn_momentum, true, 3);
        }
        else
    	    check_improve(ch, victim, gsn_momentum, false, 3);
	}

    // Check for earthen vessel
    for (AFFECT_DATA * vessel(get_affect(victim, gsn_earthenvessel)); vessel != NULL; vessel = get_affect(victim, gsn_earthenvessel, vessel))
    {
        if (vessel->location != APPLY_NONE || vessel->modifier <= 0)
            continue;

        // Reduce the damage appropriately
        int reduction(UMAX(0, UMIN(dam - 1, vessel->modifier)));
        dam -= reduction;
        vessel->modifier -= reduction;

        // Echo if spent
        if (vessel->modifier <= 0)
            send_to_char("You feel a slight tremor as your channel to the earth's spirit closes.\n", victim);
    }

    // Check for drake damage modification
    if (ch != NULL)
    {
        // Check for vicious to deal more damage as the target is hurt
        int viciousCount(Drakes::SpecialCount(*ch, Drakes::Vicious));
        if (viciousCount > 0)
        {
            int percInjured(100 - ((victim->hit * 100) / victim->max_hit));
            percInjured = UMAX(0, percInjured);
            dam += ((percInjured * viciousCount) / 10);
        }

        // Check for stonesong
        if (Drakes::HasStonesong(*ch))
        {
            int bonus(15);
            bonus += Drakes::SpecialCount(*ch, Drakes::Attentive);
            dam = ((dam * (100 + bonus)) / 100);
        }

        // Check for underground elementals and drakes to deal more damage
        if (ch->in_room != NULL && ch->in_room->sector_type == SECT_UNDERGROUND 
        && IS_NPC(ch) && (ch->pIndexData->vnum == MOB_VNUM_EARTH_ELEMENTAL || ch->pIndexData->vnum == MOB_VNUM_DRAKE))
            dam = (dam * 11) / 10;
    }

    // Check for drake damage modification on the receiving side
    if (Drakes::HasStonesong(*victim))
    {
        int bonus(15);
        bonus += Drakes::SpecialCount(*victim, Drakes::Attentive);
        dam = ((dam * (100 - bonus)) / 100);
    }

    // Check for underground elementals and drakes to take less damage
    if (victim->in_room != NULL && victim->in_room->sector_type == SECT_UNDERGROUND
    && IS_NPC(victim) && (victim->pIndexData->vnum == MOB_VNUM_EARTH_ELEMENTAL || victim->pIndexData->vnum == MOB_VNUM_DRAKE))
        dam = (dam * 9) / 10;

    // Check for phantasm damage modification
    if (ch != NULL && IS_NPC(ch) && ch->pIndexData->vnum == MOB_VNUM_PHANTASMALMIRROR)
    {
        // Check aura-based modifiers
        int auraMod(aura_grade(victim));
        if (auraMod >= 2)
        {
            // Evil victim
            dam += (PhantasmInfo::traitCount(*ch, PhantasmTrait::Righteous) * 20);
            dam -= (PhantasmInfo::traitCount(*ch, PhantasmTrait::Defiling) * 20);
            dam -= (PhantasmInfo::traitCount(*ch, PhantasmTrait::Imbalanced) * 20);
        }
        else if (auraMod <= -2)
        {
            // Good victim
            dam -= (PhantasmInfo::traitCount(*ch, PhantasmTrait::Righteous) * 20);
            dam += (PhantasmInfo::traitCount(*ch, PhantasmTrait::Defiling) * 20);
            dam -= (PhantasmInfo::traitCount(*ch, PhantasmTrait::Imbalanced) * 20);
        }
        else
        {
            // Neutral victim
            dam -= (PhantasmInfo::traitCount(*ch, PhantasmTrait::Righteous) * 20);
            dam -= (PhantasmInfo::traitCount(*ch, PhantasmTrait::Defiling) * 20);
            dam += (PhantasmInfo::traitCount(*ch, PhantasmTrait::Imbalanced) * 20);
        }

        // Check for merciless
        if (PhantasmInfo::traitCount(*ch, PhantasmTrait::Merciless) > 0)
        {
            int percInjured(100 - ((victim->hit * 100) / victim->max_hit));
            percInjured = UMAX(0, percInjured);
            dam += (percInjured / 5);
        }

        // Check sector-based modifiers
        if (ch->in_room != NULL)
        {
            switch (ch->in_room->sector_type)
            {
                // Civilization
                case SECT_INSIDE:
                case SECT_CITY:
                    dam -= (PhantasmInfo::traitCount(*ch, PhantasmTrait::Feral) * 20);
                    dam += (PhantasmInfo::traitCount(*ch, PhantasmTrait::Civilized) * 20);
                    break;

                // Neither nature nor civilization
                case SECT_ROAD:
                case SECT_UNUSED:
                    break;

                // Nature
                default:
                    dam += (PhantasmInfo::traitCount(*ch, PhantasmTrait::Feral) * 20);
                    dam -= (PhantasmInfo::traitCount(*ch, PhantasmTrait::Civilized) * 20);
                    break;
            }
        }

        // Check for critical hits
        if (static_cast<unsigned int>(number_percent()) <= (PhantasmInfo::traitCount(*ch, PhantasmTrait::Critical) * 5))
            dam *= 2;

        // Check for vampiric
        if (ch->master != NULL && ch->master->in_room == ch->in_room && ch->master->mana < ch->master->max_mana
        && PhantasmInfo::traitCount(*ch, PhantasmTrait::Vampiric) > 0 && number_percent() <= 5)
        {
            act("$N draws life energy with $S attack, restoring your mind!", ch->master, NULL, ch, TO_CHAR);
            ch->master->mana += dam;
            ch->master->mana = UMIN(ch->master->mana, ch->master->max_mana);
        }
    }

    ////// This section is for things which depend on specific damtypes, but don't necessarily modify damage //////
    // Check for corrosion
    int corrosionOdds(dam * (isPhysical ? (dam_type == DAM_BASH ? 2 : 1) : 0));
    if (number_percent() <= corrosionOdds / 30)
    {
        std::vector<OBJ_DATA*> corrodedObjects;
        for (OBJ_DATA * obj(victim->carrying); obj != NULL; obj = obj->next_content)
        {
            // Check whether this object can be shattered from corrosion
            if (obj->worn_on && obj->worn_on != WORN_WIELD && obj->worn_on != WORN_DUAL_WIELD && obj_is_affected(obj, gsn_corrosion))
                corrodedObjects.push_back(obj);
        }

        // Choose a random corroded object, if any
        if (!corrodedObjects.empty())
        {
            OBJ_DATA * obj(corrodedObjects[number_range(0, corrodedObjects.size() - 1)]);
            act("As $p get caught by the blow, it crumples into a useless mess of rusted metal!", victim, obj, NULL, TO_ALL);
            extract_obj(obj);
        }
    }

    // Check for gloomward thwarting
    if (dam > 0 && dam_type == DAM_LIGHT && victim->in_room != NULL
    && get_skill(victim, gsn_gloomward) > 0 && room_is_dark(victim->in_room))
    {
        AFFECT_DATA * gloomward(get_affect(victim, gsn_gloomward));
        if (gloomward == NULL)
        {
            send_to_char("Dazzled by the light, you lose focus on your gloomward.\n", victim);

            AFFECT_DATA af = {0};
            af.where    = TO_AFFECTS;
            af.type     = gsn_gloomward;
            af.level    = victim->level;
            af.duration = 1;
            affect_to_char(victim, &af);
        }
        else
            gloomward->duration = 1;
    }

    return dam;
}

bool damage_new(CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt, int dam_type, bool show, char *attack, const char * attackerName)
{
    static std::vector<DamageInfo> damage(1);
    damage[0].amount = dam;
    damage[0].type = dam_type;
    return damage_new(ch, victim, damage, dt, show, attack, attackerName);
}

bool damage_new(CHAR_DATA *ch, CHAR_DATA *victim, std::vector<DamageInfo> damage, int dt, bool show, char *attack, const char * attackerName)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *vch(NULL), *vch_next(NULL), *xch(NULL);
    AFFECT_DATA *paf(NULL);
    OBJ_DATA *obj(NULL);
    CHAR_DATA *sub(NULL);
    int mod;
    int minhit = 0;
    int fdam, skill;
    bool countered = FALSE;
    /* Step 1: Determine all cases where damage function is invalid. */

    if (!victim)
	return FALSE;
    if (victim->position == POS_DEAD)
	return FALSE;

    if (IS_AFFECTED(victim, AFF_WIZI))
	return FALSE;

    if (IS_OAFFECTED(victim, AFF_GHOST))
	return FALSE;

    /* Step 2: Check for the effects of damage to cancel some affects. */

    if (is_affected(victim, gsn_astralprojection))
    {
        for (CHAR_DATA * echoChar(victim->in_room == NULL ? NULL : victim->in_room->people); echoChar != NULL; echoChar = echoChar->next_in_room)
        {
            if (victim != echoChar && can_see(echoChar, victim))
    	        act("$n is struck by a blow, and $s connection to the Astral Plane is shattered.", victim, NULL, echoChar, TO_VICT);
        }

	    act("You are struck by a blow, and your connection to the Astral Plane is shattered.", victim, NULL, NULL, TO_CHAR);
    	extract_char(victim, TRUE);
	    return FALSE;
    }

    if (IS_OAFFECTED(victim, AFF_EYEFOCUS))
    {
	DESCRIPTOR_DATA *d;

	for (d = descriptor_list; d; d = d->next)
	    if (d->original == victim)
	    {
		vch			= d->character;
		d->character->desc	= NULL;
		d->character		= d->original;
		d->original		= NULL;
		d->character->desc	= d;
		send_to_char("Your senses return as your physical body incurs injury.\n\r", victim);
		act("$n's eyes become more alert.", victim, NULL, NULL, TO_ROOM);
		REMOVE_BIT(victim->oaffected_by, AFF_EYEFOCUS);
		extract_char(vch, TRUE);
	        break;
	    }
    }

    if (IS_NPC(victim) && (victim->pIndexData->vnum == MOB_VNUM_MIRROR_IMAGE || victim->pIndexData->vnum == MOB_VNUM_WEAVEILLUSION))
    {
        act("$n turns translucent and disappears.", victim, NULL, NULL, TO_ROOM);
        extract_char(victim, TRUE);
        return false;
    }

    if (ch != NULL)
    {
        if (checkPyrokineticMirrorAttacked(ch, victim)) return false;
        if (ShadeControl::CheckShadeAttacked(*ch, *victim)) return false;
    }

    if (IS_OAFFECTED(victim, AFF_NOVA_CHARGE))
        if (dt == gsn_wingsofflame || dt == gsn_infernofury)
	    ;
        else
        {
	    int ndam;
	    AFFECT_DATA *vaf;

	    send_to_char("You lose control of the charging nova, and its destructive power consumes you!\n\r", victim);
	    act("$n loses control of the charging nova, and its destructive power consumes $m!", victim, NULL, NULL, TO_ROOM);

	    for (vaf = victim->affected; vaf; vaf = vaf->next)
	        if (vaf->type == gsn_nova)
		    break;

	    ndam = dice(victim->level, 5) * UMAX(1, vaf->modifier - vaf->duration);
	    affect_strip(victim, gsn_nova);

	    damage_old(victim, victim, ndam, gsn_nova, DAM_FIRE, TRUE);
	
	    if (!victim || IS_OAFFECTED(victim, AFF_GHOST))
	        return FALSE;
        }

    if ((paf = get_affect(victim, gsn_delayreaction)) != NULL)
    {
	act("With $n's attention focused away from it, $s reaction boils out of control!", victim, NULL, NULL, TO_ROOM);
	act("With your attention focused away from it, your reaction boils out of control!", victim, 
	    NULL, NULL, TO_CHAR);
    	do_alchemy(victim, (char *)paf->point, paf->modifier, 0, (double)(-1)); //-1 signals explode
    }

// brazen: Ticket #140: this is here to ensure the light sleep affect is 
// removed and dynamic long descriptions are changed properly when someone is 
// sleeping on furniture then is awakened by damage.
// If someone else is attacking the victim, they should come off the furniture
// so they're not standing in a bedroll
    if ((victim->on) && (victim != ch))
        victim->on=NULL;
// If damage is due to an affect on the character (victim == ch), and the
// victim is sleeping, they should come off the furniture; otherwise, they
// should remain on it since they are likely resting
    if ((victim->on) && (victim == ch) && (victim->position == POS_SLEEPING))
        victim->on=NULL;

    if(is_affected(victim, gsn_lightsleep) && victim->position == POS_SLEEPING) 
        affect_strip(victim, gsn_lightsleep);

    /* Step 3: Begin initiating various attack states. */
    if (ch != NULL)
    {
        if (victim != ch)
        {
            if (is_safe(ch, victim))
                return FALSE;

            check_killer(ch, victim);

            if ((victim->position > POS_STUNNED)
             && !IS_OAFFECTED(victim, AFF_GHOST)
             && !global_bool_ranged_attack
             && dt != gsn_delayreaction)
            {
                set_fighting(victim, ch);
                if (victim->fighting && victim->position > POS_STUNNED && victim->position != POS_FIGHTING)
                switch_position(victim, POS_FIGHTING);
            }

            if (!IS_NPC(ch))
                ch->pcdata->adrenaline = IS_NPC(victim) ? UMAX(ch->pcdata->adrenaline, 2) : UMAX(ch->pcdata->adrenaline, 20);

            if ((victim->position > POS_STUNNED) && !IS_OAFFECTED(victim, AFF_GHOST) && dt != gsn_delayreaction)
            {
                set_fighting(ch, victim);
                
                // If victim is charmed, ch might attack victim's master
                if (IS_NPC(ch) && IS_NPC(victim))
                {
                    // Determine master; for charmies this is straightforward
                    CHAR_DATA * redirectMaster(NULL);
                    if (IS_AFFECTED(victim, AFF_CHARM)) 
                        redirectMaster = victim->master;
                    else
                    {
                        // Obtain the deathswarm "master"
                        AFFECT_DATA * deathswarm(get_affect(victim, gsn_deathswarm));
                        if (deathswarm != NULL && ch->in_room != NULL)
                        {
                            redirectMaster = get_char_by_id_any_room(deathswarm->modifier, *ch->in_room);
                            if (redirectMaster == NULL)
                            {
                                if (saves_spell(deathswarm->level, NULL, victim, DAM_OTHER))
                                {
                                    // Deathswarm master is out of the room, so stop fighting
                                    affect_remove(victim, deathswarm);
                                    stop_fighting_all(victim);
                                    act("No longer bound, $n ceases fighting.", victim, NULL, NULL, TO_ROOM);
                                    return false;
                                }
                                else
                                    deathswarm->level = UMAX(1, deathswarm->level - 2);
                            }
                        }
                    }
                    
                    if (redirectMaster != NULL && redirectMaster->in_room == ch->in_room)
                    {
                        // Start at a 12% chance
                        int chance = 12;
                        
                        // Adjust for scapegoat trait
                        if (PhantasmInfo::traitCount(*ch, PhantasmTrait::Scapegoat) > 0)
                            chance = 6;

                        if (number_percent() < chance)
                        {
                            stop_fighting(ch);
                            act("$n redirects $s attacks to you!", ch, NULL, redirectMaster, TO_VICT);
                            multi_hit(ch, redirectMaster, TYPE_UNDEFINED);
                            return false;
                        }
                    }
                }

                // If ch is a soulmirror and victim an NPC, it might redirect
                if (IS_NPC(ch) && IS_NPC(victim) && ch->pIndexData->vnum == MOB_VNUM_MIRROROFSOULS 
                && victim->fighting != ch && number_bits(2) == 0)
                {
                    act("$n turns $s attacks towards $N!", victim, NULL, ch, TO_ROOM);
                    stop_fighting(victim);
                    set_fighting(victim, ch);
                }
            }

            if (counter_works && global_counter_works 
             && !global_bool_ranged_attack && !IS_NPC(victim)
             && get_eq_char(victim, WEAR_WIELD)
             && (victim->position > POS_RESTING)
             && ((dt >= TYPE_HIT) || ((skill_table[dt].spell_fun == spell_null)
                                   || (skill_table[dt].spell_fun == spell_form)))
             && ((skill = get_skill(victim, gsn_counter)) > 5)
             && can_see(victim, ch)
             && (ch->in_room == victim->in_room))
            {
                if (number_percent() <= skill/5)
                {
                act("$N counters $n's attack!", ch, NULL, victim, TO_NOTVICT);
                act("You counter $n's attack!", ch, NULL, victim, TO_VICT);
                act("$N counters your attack!", ch, NULL, victim, TO_CHAR);
                if (ch->fighting == NULL)
                    ch->fighting = victim;
                if (victim->fighting == NULL)
                        victim->fighting = ch;
                victim = ch;
                check_improve(victim,ch, gsn_counter, TRUE, 2);
                countered = TRUE;
                }
                else
                {
                check_improve(victim,ch, gsn_counter, FALSE, 4);
                if (ch->fighting == NULL)
                        ch->fighting = victim;
                if (victim->fighting == NULL)
                        victim->fighting = ch;
                }
            }
        }

        if (ch->fighting == ch->master && ch->master != NULL && (!IS_NPC(ch) || !IS_SET(ch->act, ACT_ILLUSION)))
            stop_follower(ch);

        do_visible(ch, "FALSE");

        if (IS_PAFFECTED(ch, AFF_VOIDWALK))
        {  
            AFFECT_DATA af;
            af.valid = TRUE;
            af.point = NULL;

            act("You shift back into the material world.", ch, NULL, NULL, TO_CHAR);
            act("$n shifts $s essence back into the material world.", ch, NULL, NULL, TO_ROOM);
            ch->hit = UMAX(1, ch->hit / 4);
            affect_strip(ch, gsn_voidwalk);

            af.where     = TO_PAFFECTS;
            af.type      = gsn_voidwalk;
            af.level     = ch->level;
            af.duration  = ch->level / 4;
            af.location  = APPLY_HIT;
            af.modifier  = ch->level * -1;
            af.bitvector = 0;
            affect_to_char(ch, &af);

            WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE * 2));
            send_to_char("You pause for a moment, feeling drained from the experience.\n\r", ch);
        }

        if (is_affected(ch, gsn_rearrange) || is_affected(ch, gsn_alterself))
        {
            affect_strip(ch, gsn_rearrange);
            affect_strip(ch, gsn_alterself);
            act("Your covering illusion fades away.", ch, NULL, NULL, TO_CHAR);
            act("Strips of color peel away from $n, revealing $s true form.", ch, NULL, NULL, TO_ROOM);
        }

        if (is_affected(ch, gsn_englamour))
        {
            affect_strip(ch, gsn_englamour);
            act("The glamour surrounding you suddenly vanishes!", ch, NULL, NULL, TO_CHAR);
            act("Strips of color peel away from $n, revealing $s true form.", ch, NULL, NULL, TO_ROOM);
        }

        if (is_affected(ch, gsn_chameleon))
        {
            affect_strip(ch, gsn_chameleon);
            act("You return to your natural form.", ch, NULL, NULL, TO_CHAR);
            act("$n returns to $s natural form.", ch, NULL, NULL, TO_ROOM);
        }

        if (is_affected(ch, gsn_disguise))
        {
            affect_strip(ch, gsn_disguise);
            act("You discard your disguise to fight.", ch, NULL, NULL, TO_CHAR);
            act("$n discards $s disguise to fight.", ch, NULL, NULL, TO_ROOM);
        }
    }

    // Handle absorb electricity
    if (is_affected(victim, gsn_absorbelectricity) && number_bits(2) != 0)
    {
        size_t i(0);
        while (i < damage.size())
        {
            // If not electric damage, move on
            if (damage[i].type != DAM_LIGHTNING)
            {
                ++i;
                continue;
            }

            // Absorb the damage
            paf = affect_find(victim->affected,gsn_absorbelectricity);
            paf->modifier += damage[i].amount * 2.5;
            if (ch == victim)
            {
                sprintf(buf,"You absorb your %s, and a nimbus of light forms around you.", dt >= TYPE_HIT ? (attack ? attack : ((dt > TYPE_HIT) ? ch->dam_verb : "attack")) : skill_table[dt].noun_damage);
                act(buf, ch, NULL, NULL, TO_CHAR);
                sprintf(buf,"$n absorbs $s %s, and a nimbus of light forms around $m.", dt >= TYPE_HIT ? (attack ? attack : ((dt > TYPE_HIT) ? ch->dam_verb : "attack")) : skill_table[dt].noun_damage);
                act(buf, ch, NULL, NULL, TO_ROOM);
            }
            else if (ch != NULL)
            {
                sprintf(buf, "$N absorbs your %s, and a nimbus of light forms around $M.", dt >= TYPE_HIT ? (attack ? attack : ((dt > TYPE_HIT) ? ch->dam_verb : "attack")) : skill_table[dt].noun_damage);
                act(buf, ch, NULL, victim, TO_CHAR);
                sprintf(buf, "You absorb $n's %s, and a nimbus of light forms around you.", dt >= TYPE_HIT ? (attack ? attack : ((dt > TYPE_HIT) ? ch->dam_verb : "attack")) : skill_table[dt].noun_damage);
                act(buf, ch, NULL, victim, TO_VICT);
                sprintf(buf, "$N absorbs $n's %s, and a nimbus of light forms around $M.", dt >= TYPE_HIT ? (attack ? attack : ((dt > TYPE_HIT) ? ch->dam_verb : "attack")) : skill_table[dt].noun_damage);
                act(buf, ch, NULL, victim, TO_NOTVICT);
            }
            else
            {
                send_to_char("You absorb the electricity, and a nimbus of light forms around you.\n", victim);
                act("$n absorbs the electricity, and a nimbus of light forms around $m.", victim, NULL, NULL, TO_ROOM);
            }

            // Replace this slot
            damage[i] = damage[damage.size() - 1];
            damage.pop_back();
        }

        // Bail out if all damage absorbed
        if (damage.empty())
            return false;
    }

    bool canDefendAuto(false);
    if (ch != NULL)
    {
        if (dual_damage) obj = get_eq_char(ch, WEAR_DUAL_WIELD);
        else obj = get_eq_char(ch, WEAR_WIELD);

        /* Well, it's about time to start the first batch of defenses... */
        canDefendAuto = (dt >= TYPE_HIT || dt == TYPE_FLOAT);
        if (ch == victim) canDefendAuto = false;

        bool canDefendManual(true);
        if (dt == gsn_hew) canDefendManual = false;
        if (!IS_NPC(victim) && !IS_SET(victim->act, PLR_AUTODEFEND)) canDefendManual = false;
        if (static_cast<unsigned int>(number_percent()) <= PhantasmInfo::traitCount(*ch, PhantasmTrait::Marksman) * 10) canDefendManual = false;

        if (canDefendAuto && canDefendManual && global_bool_check_avoid)
        {
            if (check_fend(ch, victim,obj)) return false;
            if (check_stonephalanx(ch, victim, obj)) return false;
            if (check_deflect(ch,victim, obj)) return false;
            if (check_parry(ch, victim, obj)) return false;
            if (check_offhandparry(ch, victim, obj)) return false;
            if (check_dodge(ch, victim, obj)) return false;
            if (check_shamanicward(ch, victim)) return false;
            if (check_showmanship(ch, victim, obj)) return false;
            if (check_savagery(ch, victim, obj, dt))return false;
            if (obj == NULL && check_brawlingblock(ch, victim)) return false;
            if (check_evade(ch, victim, obj)) return false;
            if (check_creaturelore(ch, victim )) return false;
            if (check_divineshield(ch, victim)) return false;
        }

        // Check for call upon wind
        std::pair<Direction::Value, int> windInfo(checkWind(ch));
        if (windInfo.second > 0)
        {
            AFFECT_DATA * call(get_affect(ch, gsn_calluponwind));
            if (call != NULL && call->modifier == Direction::North)
            {
                // North wind adds cold damage if there is any physical damage present, so check for physical
                bool foundPhysical(false);
                DamageInfo * coldDamage(NULL);
                for (size_t i(0); i < damage.size(); ++i)
                {
                    switch (damage[i].type)
                    {
                        case DAM_SLASH: case DAM_BASH: case DAM_PIERCE: foundPhysical = true; break;
                        case DAM_COLD: coldDamage = &damage[i]; break;
                    }
                }

                // Add the damage if appropriate
                if (foundPhysical)
                {
                    // If no cold damage yet, add some
                    if (coldDamage == NULL)
                    {
                        damage.push_back(DamageInfo(0, DAM_COLD));
                        coldDamage = &damage[damage.size() - 1];
                    }

                    // Add to the cold damage according to wind strength
                    coldDamage->amount += ((windInfo.second / (windInfo.first == Direction::North ? 3 : 10)) + 5);
                }
            }
        }
    } 

    // Calculate total damage prior to reduction
    int totalDamage(0);
    bool hasLight(false);
    for (size_t i(0); i < damage.size(); ++i)
    {
        totalDamage += damage[i].amount;
        if (damage[i].type == DAM_LIGHT)
            hasLight = true;
    }
   
    /* Check the rest of the defenses */
    // Note that these defenses are more involuntary in nature. If you're
    // wearing a shield, there's a chance your opponent will hit it instead
    // of you. If you're affected by blur, the magic does its own thing, etc.
    if (canDefendAuto && global_bool_check_avoid)
    {
        if (check_shield_block(ch,victim,obj,totalDamage))
            return FALSE;

        if (is_affected(victim, gsn_deflection)
        &&  (number_percent() < (15 + (victim->level / 5)))
        &&  (victim->mana >= (totalDamage / 40)))
        {
            act("Your attack is pushed aside by an invisible force.",
            ch, NULL, victim, TO_CHAR);
            act("You deflect $n's attack away with a short burst of telekinesis.", ch, NULL, victim, TO_VICT);
            expend_mana(victim, (totalDamage / 40));
            return FALSE;
        }

        if (room_is_affected(ch->in_room, gsn_smokescreen) 
          && (!(ch->race == global_int_race_shuddeni 
            || is_affected(ch,gsn_sensory_vision)
            || number_percent() < get_skill(ch,gsn_blindfighting)))
          && number_percent() < 10)
        {
            act("You momentarily lose sight of $N in the smoky haze, and miss.", ch, NULL, victim, TO_CHAR);
            act("$n seems briefly lost in the smoky haze, and misses you.", ch, NULL, victim, TO_VICT);
            return FALSE;
        }
        
        if (victim->in_room->sector_type == SECT_FIELD
          && get_skill(victim,gsn_terrainlore)>0)
            if (number_percent() < get_skill(victim,gsn_terrainlore)/3)
            {
                act("You lose $N for a moment in the high grasses.",ch,NULL,victim,TO_CHAR);
                act("You use the high grasses to distract $n.",ch,NULL,victim,TO_VICT);
                check_improve(victim,ch,gsn_terrainlore,TRUE,1);
                return FALSE;
            }
        
        if (victim->in_room->sector_type == SECT_FOREST
          && get_skill(victim,gsn_terrainlore) > 0)
            if (number_percent() < get_skill(victim,gsn_terrainlore)/4)
            {
                act("$N darts behind a tree, and your weapon strikes its trunk.",ch,NULL,victim,TO_CHAR);
                act("You slip behind a tree, and its trunk stops $n's attack.",ch,NULL,victim,TO_VICT);
            if (obj && !IS_SET(obj->extra_flags[0], ITEM_NODESTROY)
              && (!IS_NPC(ch) && !BIT_GET(ch->pcdata->traits, TRAIT_BLACKSMITH))
              && !material_table[obj->material].metal)
                if(number_percent() > get_skill(ch,gsn_grace) * 3 / 4)
                {
                obj->condition--;
                check_improve(ch,victim,gsn_grace,FALSE,1);
                }
                else
                check_improve(ch,victim,gsn_grace,TRUE,1);
                check_improve(victim,ch,gsn_terrainlore,TRUE,1);
                return FALSE;
            }
        
        if (victim->in_room->sector_type == SECT_HILLS
          || victim->in_room->sector_type == SECT_MOUNTAIN
          || victim->in_room->sector_type == SECT_SWAMP
          && get_skill(victim,gsn_terrainlore) > 0)
            if (number_percent() < get_skill(victim,gsn_terrainlore)/4)
            {
                act("$N uses the nearby terrain to avoid your attack.",ch,NULL,victim,TO_CHAR);
                act("You use the nearby terrain to avoid $n's attack.",ch,NULL,victim,TO_VICT);
                check_improve(victim,ch,gsn_terrainlore,TRUE,1);
                return FALSE;
            }

        if (is_affected(victim, gsn_blur) && (number_percent() < (10 + (victim->level / 10))))
        {
            act("Your attack passes harmlessly through $N's blurred image.", ch, NULL, victim, TO_CHAR);
            act("$n's attack passes harmlessly through your blurred image.", ch, NULL, victim, TO_VICT);
            return false;
        }

        // Check gloomward and fadeshroud
        int gloomChance(get_skill(victim, gsn_gloomward) / 10);
        if (gloomChance > 0)
        {
            if (is_affected(victim, gsn_fadeshroud))
                gloomChance += 40;

            if (number_percent() < gloomChance && !is_affected(victim, gsn_gloomward) && room_is_dark(victim->in_room))
            {
                act("$N fades into the darkness, letting your blow pass $M by.", ch, NULL, victim, TO_CHAR);
                act("You fade into the darkness, letting the blow pass you by.", ch, NULL, victim, TO_VICT);
                check_improve(victim, ch, gsn_gloomward, true, 2);
                return false;
            }
        }

        // Check puppetmaster (aka reshackle)
        if (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL && number_percent() <= get_skill(victim, gsn_reshackle) / 3)
        {
            act("$N balks at attacking you, seeming unsure of $Mself.", victim, NULL, ch, TO_CHAR);
            if (ch->master->in_room == ch->in_room)
                act("$N balks at attacking, seeming unsure of $Mself.", ch->master, NULL, ch, TO_CHAR);

            check_improve(victim, ch, gsn_reshackle, true, 4);
            return false;
        }

        // Check Breezestep
        AFFECT_DATA * breezestep(get_affect(victim, gsn_breezestep));
        if (breezestep != NULL && breezestep->modifier == 1 && is_flying(victim))
        {
            std::pair<Direction::Value, int> windInfo(checkWind(victim));
            if (windInfo.second > 0)
            {
                // Determine chance of dodging, including call upon wind modifier
                int chance(45 + (windInfo.second / 5));
                AFFECT_DATA * call(get_affect(victim, gsn_calluponwind));
                if (call != NULL && call->modifier == Direction::South)
                    chance = (chance * (windInfo.first == Direction::South ? 14 : 11)) / 10;

                // Check for dodge
                if (number_percent() <= chance)
                {
                    act("$E flits easily away from your attack.", ch, NULL, victim, TO_CHAR);
                    act("You flit easily away from $n's attack.", ch, NULL, victim, TO_VICT);
                    return false;
                }
            }
        }

        // Check Burning Wisp
        OBJ_DATA * wisp(get_eq_char(victim, WEAR_FLOAT));
        if (wisp != NULL && wisp->pIndexData->vnum == OBJ_VNUM_BURNINGWISP && number_percent() < ((get_skill(victim, gsn_burningwisp) / 20) + (wisp->level / 2)))
        {
            act("$p darts wildly about, distracting you!", ch, wisp, NULL, TO_CHAR);
            act("$p darts wildly about, distracting $n!", ch, wisp, NULL, TO_ROOM);
            return false;
        }

        // Check Nightflame Lash
        OBJ_DATA * lash(get_eq_char(victim, WEAR_WIELD));
        if (lash != NULL)
        {
            AFFECT_DATA * lashAff(get_obj_affect(lash, gsn_nightflamelash));
            if (lashAff != NULL && number_percent() <= (20 + (get_skill(victim, gsn_nightflamelash) / 4)))
            {
                act("$p coils around your attack, flinging it harmlessly away from $N.", ch, lash, victim, TO_CHAR);
                act("$p coils around $n's attack, flinging it harmlessly away from you.", ch, lash, victim, TO_VICT);
                return false;
            }
        }

        // Check aspect of the inferno
        AFFECT_DATA * aspectAff(get_affect(victim, gsn_aspectoftheinferno));
        if (aspectAff != NULL && aspectAff->modifier == 0)
        {
            int skillVal(get_skill(victim, gsn_aspectoftheinferno));
            int avoidChance = 30 + (aspectAff->level / 5) + UMAX(0, (skillVal - 70) / 3);
            if (saves_spell(aspectAff->level, victim, ch, DAM_FIRE))
                avoidChance /= 2;

            if (victim->mana >= 3 && number_percent() <= avoidChance)
            {
                expend_mana(victim, 3);
                act("A flare of intense heat bursts out from $N, driving you back!", ch, NULL, victim, TO_CHAR);
                act("A flare of intense heat bursts out from you, driving $n back!", ch, NULL, victim, TO_VICT);
                return false;
            }
        }

        // Check shadowfiend
        if (!hasLight && victim->in_room != NULL && is_affected(victim, gsn_shadowfiend))
        {
            if (room_is_dark(victim->in_room))
            {
                if (number_percent() <= 60)
                {
                    act("$N's wraithform melds with the shadows, easily avoiding your blow.", ch, NULL, victim, TO_CHAR);
                    act("Your wraithform melds with shadows, easily avoiding $n's blow.", ch, NULL, victim, TO_VICT);
                    return false;
                }
            }
            else if (number_percent() <= 30)
            {
                act("Your blow passes harmlessly through $N's shadowy form!", ch, NULL, victim, TO_CHAR);
                act("$n's blow passes harmlessly through your shadowy form!", ch, NULL, victim, TO_VICT);
                return false;
            }
        }

        // Check fetid divination
        AFFECT_DATA * fetid(get_affect(victim, gsn_fetiddivination));
        if (fetid != NULL && fetid->modifier == ch->id && victim->mana >= 3)
        {
            int chance(get_skill(victim, gsn_fetiddivination) / 5);
            chance += get_curr_stat(victim, STAT_WIS);
            if (number_percent() <= chance)
            {
                expend_mana(victim, 3);
                act("Remembering $N's future, you step away from $S attack almost before it begins!", victim, NULL, ch, TO_CHAR);
                act("$n steps away from your attack almost before it begins!", victim, NULL, ch, TO_VICT);
                return false;
            }
        }

        // Check conduit of the skies
        if (is_affected(victim, gsn_conduitoftheskies))
        {
            int skillVal(get_skill(victim, gsn_conduitoftheskies));
            int avoidChance = 40 + UMAX(0, (skillVal - 70) / 3);
            if (saves_spell(victim->level, victim, ch, DAM_LIGHTNING))
                avoidChance /= 2;

            if (number_percent() <= avoidChance)
            {
                act("A crackle of lightning flashes out from $N, driving you back!", ch, NULL, victim, TO_CHAR);
                act("A crackle of lightning flashes out from you, driving $n back!", ch, NULL, victim, TO_VICT);
                return false;
            }
        }

        // Check hovering shield
        OBJ_DATA * hovershield(get_eq_char(victim, WEAR_FLOAT));
        if (hovershield != NULL && obj_is_affected(hovershield, gsn_hoveringshield))
        {
            int skillVal(get_skill(victim, gsn_hoveringshield));
            if (number_percent() <= 30 + UMAX(0, (skillVal - 70) / 3))
            {
                act("$p flies in, intercepting your blow!", ch, hovershield, victim, TO_CHAR);
                act("$p flies in, intercepting $n's blow!", ch, hovershield, victim, TO_VICT);
                return false;
            }
        }

        // Check groupmate protection abilities
        for (xch = victim->in_room->people ; xch != NULL; xch = xch->next_in_room)
        {
            if (!is_same_group(xch, victim))
                continue;

            // The following abilities only apply to groupmates, not the original victim
            if (xch == victim)
                continue;

            // Check shield cover
            if ((xch->class_num == global_int_class_fighter || xch->class_num == global_int_class_watertemplar) && check_shield_block(ch, xch, obj, totalDamage))
                return FALSE;

            // Check ward of the shield bearer
            if (is_affected(xch, gsn_wardoftheshieldbearer) && xch->mana >= 5)
            {
                int chance(15 + UMAX(0, (get_skill(xch, gsn_wardoftheshieldbearer) - 60) / 4));
                if (is_an_avatar(victim))
                    chance /= 2;

                if (number_percent() <= chance)
                {
                    expend_mana(xch, 5);
                    act("A flare of silvery light lashes out from you, turning aside $N's attack!", xch, NULL, ch, TO_CHAR);
                    act("A flare of silvery light lashes out from $n, turning aside your attack!", xch, NULL, ch, TO_VICT);
                    act("A flare of silvery light lashes out from $n, turning aside $N's attack!", xch, NULL, ch, TO_NOTVICT);
                    return false;
                }
            }
        }
    }

    // Check for clay shield
    AFFECT_DATA * clayShield(get_affect(victim, gsn_clayshield));
    if (clayShield != NULL && clayShield->modifier == 0)
    {
        // Block the attack with the clay shield, unless it contains a fire element
        bool hasFire(false);
        for (size_t i(0); i < damage.size(); ++i)
        {
            if (damage[i].type == DAM_FIRE)
            {
                hasFire = true;
                break;
            }
        }

        clayShield->modifier = 1;
        clayShield->duration = 6;
        if (ch == NULL)
        {
            if (hasFire)
            {
                act("The fiery attack smashes through your clay shield unimpeded!", victim, NULL, NULL, TO_CHAR);
                act("The fiery attack smashes through $n's clay shield unimpeded!", victim, NULL, NULL, TO_ROOM);
            }
            else
            {
                act("The blow is halted by your clay shield, which crumbles around you!", victim, NULL, NULL, TO_CHAR);
                act("The blow is halted by $n's clay shield, which crumbles around $m!", victim, NULL, NULL, TO_ROOM);
            }
        }
        else
        {
            if (hasFire)
            {
                act("Your fiery attack smashes through $N's clay shield unimpeded!", ch, NULL, victim, TO_CHAR);
                act("$n's fiery attack smashes through your clay shield unimpeded!", ch, NULL, victim, TO_VICT);
                act("$n's fiery attack smashes through $N's clay shield unimpeded!", ch, NULL, victim, TO_NOTVICT);
            }
            else
            {
                act("Your blow is halted by $N's clay shield, which crumbles around $M!", ch, NULL, victim, TO_CHAR);
                act("$n's blow is halted by your clay shield, which crumbles around you!", ch, NULL, victim, TO_VICT);
                act("$n's blow is halted by $N's clay shield, which crumbles around $M!", ch, NULL, victim, TO_NOTVICT);
            }

        }

        // Check for clayshard
        if (ch != NULL && number_percent() <= get_skill(victim, gsn_clayshard))
        {
            send_to_char("A shard of pure ice erupts from the clay shield as it breaks apart!\n", victim);
            spell_iceshard(gsn_iceshard, victim->level, victim, ch, TARGET_CHAR);
            check_improve(victim, ch, gsn_clayshard, true, 4);

            if (!IS_VALID(ch) || ch->in_room != victim->in_room)
                return false;
        }

        if (!hasFire)
            return false;
    }

    // Adjust the damage by resistance/reduction etc.
    totalDamage = 0;
    bool immune(true);
    for (size_t i(0); i < damage.size(); ++i)
    {
        bool immuneLocal(false);
        damage[i].amount = modify_damage(ch, victim, obj, damage[i].amount, dt, damage[i].type, immuneLocal);
        totalDamage += damage[i].amount;
        if (!immuneLocal)
            immune = false;
    }
    int originalTotalDamage(totalDamage);

    if (ch != NULL)
    {
        if (ch != victim && ch->in_room == victim->in_room
        && can_see(victim,ch)
        && (dt == gsn_icebolt
        || dt == gsn_firebolt
        || dt == gsn_ray_of_light
        || dt == gsn_call_lightning
        || dt == gsn_nova))
        {
            if( number_percent() < get_skill(victim,gsn_opportunism))
            {
                act("You swiftly strike in response to $n's spell!",ch,NULL,victim,TO_VICT);
                act("$N swiftly strikes in response to your spell!",ch,NULL,victim,TO_CHAR);
                act("$N swiftly strikes in response to $n's spell!",ch,NULL,victim,TO_NOTVICT);
                one_hit(victim,ch,TYPE_UNDEFINED,HIT_OPPORTUNITY);
                check_improve(victim,ch,gsn_opportunism,TRUE,1);
            }
            else
                check_improve(victim,ch,gsn_opportunism,FALSE,1);
        }

        if ((dt >= TYPE_HIT) || (dt == TYPE_FLOAT))
        {
            if (IS_NPC(ch))
                totalDamage = mprog_hit_trigger( ch, victim, totalDamage );

            if (obj)
                totalDamage = oprog_hit_trigger( obj, victim, totalDamage );
        }
    } 

    if (IS_OAFFECTED(victim,AFF_GHOST) || !victim->in_room)
    	return FALSE;
 
    if (is_affected(victim, gsn_zeal) || IS_AFFECTED(victim, AFF_BERSERK))
	    minhit = -100;

    if (paf = affect_find(victim->affected, gsn_ignore_pain))
    {
        if (minhit > 0 - (paf->level * 3))
	       minhit = 0 - (paf->level * 3);
    }

    // Check for martyr's requiem
    AFFECT_DATA * requiem(get_group_song(victim, gsn_requiemofthemartyr));
    if (requiem != NULL && can_be_affected(victim, gsn_requiemofthemartyr))
        minhit -= (requiem->level * 3);

    if ((paf = get_affect(victim, gsn_aspectoftheinferno)) != NULL && paf->modifier == 0)
        minhit = UMIN(minhit, paf->level * -2);

    CHAR_DATA * mercy_ch(0);
    if (ch != NULL)
    {
        if (victim->hit - totalDamage <= minhit && check_lastword(victim,ch))
	        return true;
    
        if (victim->hit < victim->max_hit/4 && victim->position >= POS_SLEEPING)
            check_improve(victim,ch,gsn_lastword,TRUE,1);

        // If the ch is a charmie, use the master for consideration of mercy
        if (IS_NPC(ch))
        {
            if (ch->master != 0 && !IS_NPC(ch->master))
                mercy_ch = ch->master;
        }
        else
            mercy_ch = ch;

        if (((victim->hit - totalDamage) <= minhit) && (victim->position >= POS_SLEEPING)
         && mercy_ch != 0 && !IS_NPC(victim)
         && IS_SET(mercy_ch->nact, PLR_MERCY_DEATH)
         && !global_bool_full_damage && (victim != mercy_ch)
         && !IS_AFFECTED(victim, AFF_BERSERK) && !IS_NAFFECTED(victim,AFF_ASHURMADNESS) 
         && !is_affected(victim, gsn_warpath))
        {
            CHAR_DATA *vch;
            bool can_mercy = TRUE;

            for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
            {
                if (vch->fighting == mercy_ch && vch != victim)
                {
                    can_mercy = FALSE;
                    break;
                }
            }

            if (can_mercy && (mercy_ch->in_room == victim->in_room))
            {
                    act("You hold back on your attack, granting $N mercy.", 
                mercy_ch, NULL, victim, TO_CHAR);
                act("$n holds back on $s attack, granting $N mercy.",
                mercy_ch, NULL, victim, TO_NOTVICT);
                act("$n holds back on $s attack, granting you mercy.",
                mercy_ch, NULL, victim, TO_VICT);

                // Already at the mercy of another...
                if (victim->mercy_from && victim->mercy_from != mercy_ch)
                {
                sprintf(buf, "%s is already at the mercy of %s.\n\r", PERS(victim, mercy_ch), PERS(victim->mercy_from, mercy_ch));
                send_to_char(buf, mercy_ch);
                }
                else
                {
                mercy_ch->mercy_to = victim;
                victim->mercy_from = mercy_ch;
                }

                totalDamage = (victim->hit - 1);
            }
        }
    }

    if (global_bool_full_damage)
	    totalDamage = originalTotalDamage;
    
    fdam = totalDamage;
    totalDamage = adjust_for_clockworksoul(ch, *victim, totalDamage);

    if (is_affected(victim, gsn_enhance_pain))
    	fdam += totalDamage / 6;

    if (is_affected(victim, gsn_reduce_pain))
	    fdam -= totalDamage / 6;

    if (ch != NULL && victim->hit - totalDamage < 1 && ch != victim && is_affected(victim,gsn_blink))
    {
        if (is_affected(victim, gsn_blink) && (victim->mana > 0)
        && !IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
        && (number_percent() < get_skill(victim,gsn_blink)/2))
        {
            ROOM_INDEX_DATA *pRoom = NULL;

            if (victim->mana > 1)
            victim->mana /= 2;
            else
            victim->mana = 0;

            pRoom = get_random_room_area(victim); 
            if (pRoom != NULL)	    
            {
                act("$n {rmisses{x $N as $E blinks out of existence.",ch,NULL,victim,TO_NOTVICT);
                act("$n {rmisses{x you as you blink out of existence, appearing somewhere nearby.",ch,NULL,victim,TO_VICT);
                act("You {rmiss{x $N as $E blinks out of existence.",ch,NULL,victim,TO_CHAR);
                char_from_room(victim);
                char_to_room(victim, pRoom);
                do_look(victim, "auto");
                return TRUE;   
            }//Damn, we couldn't find a valid room in 100 rooms. Enough! You die.
        }
    }

    if (show)
    {
    	global_showdam = damage;
        bool restore_global_damage_from(false);
        if (attackerName != NULL && global_damage_from == NULL)
        {
            global_damage_from = const_cast<char*>(attackerName);
            restore_global_damage_from = true;
        }

	    if (dt == TYPE_FLOAT)
	        dam_message( ch, victim, fdam, dt, "floating weapon", immune);
    	else
    	    dam_message( ch, victim, fdam, dt, attack, immune);

        if (restore_global_damage_from)
            global_damage_from = NULL;

	    global_showdam.clear();
    }

    /* I'd abort the function at this point for dam <= 0, but I need to get to the */
    /* mercy stuff below.   - Aeolis                                               */
    if (totalDamage > 0)
    {
        if (IS_NPC(victim) && victim->pIndexData->vnum == MOB_VNUM_PHANTASMALMIRROR && victim->master != NULL)
        {
            // Handle phantasms
            int manaDrainMultiplier(900);
            manaDrainMultiplier -= (125 * PhantasmInfo::traitCount(*victim, PhantasmTrait::Resilient));
            expend_mana(victim->master, (totalDamage * manaDrainMultiplier) / 1000);
        }
        else
        {
            // Actually hurt the victim
            victim->hit -= totalDamage;
            victim->fake_hit += (fdam - totalDamage);

            // Track the damage
            if (ch != NULL && ch != victim)
                victim->experience_list->OnDamage(*ch, totalDamage);
        }
 
        // Check for flare at 20% health or lower
        if (victim->hit <= victim->max_hit / 5)
        {
            AFFECT_DATA * flare(get_affect(victim, gsn_flare));
            if (flare != NULL && flare->location == APPLY_HIT)
            {
                send_to_char("The fire within you bursts out into a flare of heat!\n", victim);
                
                // Remove the flare effect
                int flareLevel(flare->level);
                int hpBonus(flare->modifier);
                affect_strip(victim, gsn_flare);
                victim->hit = UMIN(victim->max_hit, victim->hit + hpBonus);

                // Apply a cooldown
                AFFECT_DATA faf = {0};
                faf.where       = TO_AFFECTS;
                faf.type        = gsn_flare;
                faf.location    = APPLY_NONE;
                faf.duration    = 10 - UMAX(0, (get_skill(victim, gsn_flare) - 70) / 6);
                affect_to_char(victim, &faf);
    	        
                spell_fireball(gsn_fireball, flareLevel, victim, (void *)victim, TARGET_ROOM);
            }
        }
 
        // Check for martyr's shield
        if (victim->in_room != NULL && is_affected(victim, gsn_martyrsshield) && number_percent() <= 25)
        {
            // Find the neediest groupmate
            CHAR_DATA * groupmate(NULL);
            for (CHAR_DATA * gch(victim->in_room->people); gch != NULL; gch = gch->next_in_room)
            {
                if (victim == gch || !is_same_group(victim, gch))
                    continue;

                // Set the groupmate to this guy if the first groupmate or has fewer hp as a % than the groupmate
                if (groupmate == NULL || (((gch->hit * 100) / gch->max_hit) < ((groupmate->hit * 100) / groupmate->max_hit)))
                    groupmate = gch;
            }

            // Heal the groupmate, potentially
            if (groupmate != NULL && groupmate->hit < groupmate->max_hit)
            {
                act("A flare of golden-white light flashes from you to $N, bathing $M in a brief, warm radiance.", victim, NULL, groupmate, TO_CHAR);
                act("A flare of golden-white light flashes from $n to you, and your wounds begin to knit shut!", victim, NULL, groupmate, TO_VICT);
                act("A flare of golden-white light flashes from $n to $N, bathing the latter in a brief, warm radiance.", victim, NULL, groupmate, TO_NOTVICT);
                groupmate->hit = UMIN(groupmate->max_hit, groupmate->hit + totalDamage);
            }
        }

        // Handle direfeast
        check_direfeast(ch, *victim, totalDamage);

        // Check homonculus
    	if ((paf = get_affect(victim, gsn_createhomonculus)) != NULL && paf->location == 0)
        {
            AFFECT_DATA af;
            af.valid = TRUE;
            af.point = NULL;
            
            if (IS_NPC(victim))
            {
                send_to_char("You sense your homonculus being assaulted!\n\r", (CHAR_DATA*)paf->point);
                if (victim->hit <= 0)
                {
                    send_to_char("You sense your homonculus die! The severed link tears into you!\n\r", (CHAR_DATA*)paf->point);
                    affect_strip((CHAR_DATA*)paf->point, gsn_createhomonculus);
                    damage_old(victim, (CHAR_DATA*)paf->point, 500, gsn_createhomonculus, DAM_NONE, TRUE);
                    af.where     = TO_AFFECTS;
                    af.type      = gsn_createhomonculus;
                    af.level     = ch->level;
                    af.duration  = 100;
                    af.location  = APPLY_HIT;
                    af.modifier  = -500;
                    af.bitvector = 0;
                    affect_to_char((CHAR_DATA*)paf->point, &af);
                                                    
                    extract_char(victim, TRUE);
                }
            }
            else
            {
                paf->modifier -= totalDamage;
                victim->hit += totalDamage;
                if (paf->modifier <= 0)
                {
                    victim->hit += paf->modifier;
                    send_to_char("Its energy spent, you feel your homonculus vanish.\n\r", victim);
                    act("The homonculus disappears with a strange popping sound.", 
                        (CHAR_DATA*)paf->point, NULL, NULL, TO_ROOM);
                    extract_char((CHAR_DATA*)paf->point, TRUE);
                    affect_strip(victim, gsn_createhomonculus);
                }
            }
        }

        if (ch != NULL)
        {
            if (dt == gsn_pounce)
            {
                if (is_flying(victim) && !countered)
                {
                    AFFECT_DATA af;
                    af.valid = TRUE;
                    af.point = NULL;

                    act("You bring $N to the ground!", ch, NULL, victim, TO_CHAR);
                    act("$n brings $N to the ground!",ch, NULL,victim, TO_NOTVICT);
                    act("$n brings you to the ground!",ch, NULL,victim,TO_VICT);
                    af.where	 = TO_AFFECTS;
                    af.type	 = gsn_pounce;
                    af.level     = ch->level;
                    af.duration  = 1;
                    af.modifier  = 0;
                    af.location  = APPLY_NONE;
                    af.bitvector = 0;
                    affect_to_char(victim, &af);
                }
            }
            if (dt == gsn_predatoryattack)
            {
                if (!countered && victim->hit < victim->max_hit / 2)
                {
                    AFFECT_DATA af;
                    af.valid = FALSE;
                    af.point = (void *) ch;

                    act("You continue circling $N.", ch, NULL, victim, TO_CHAR);
                    act("$n continues circling $N.",ch, NULL,victim, TO_NOTVICT);
                    act("$n continues circling you.",ch, NULL,victim,TO_VICT);
                    af.where	 = TO_AFFECTS;
                    af.type	 = gsn_predatoryattack;
                    af.level     = ch->level;
                    af.duration  = 0;
                    af.modifier  = 0;
                    af.location  = APPLY_NONE;
                    af.bitvector = 0;
                    affect_to_char(victim, &af);
                }
            }
        
            if (IS_PAFFECTED(ch,AFF_AURA_OF_CORRUPTION) && ((paf = affect_find(ch->affected,gsn_aura_of_corruption)) != NULL))
            {
                if (!IS_NPC(victim) && (IS_NEUTRAL(victim) || IS_GOOD(victim)) && (!saves_spell(paf->level, ch, victim, DAM_NEGATIVE)))
                {
                    AFFECT_DATA *caf;
                    int corrupt = dice(paf->modifier,5);
                    if ((caf = affect_find(victim->affected,gsn_aura_of_corruption)) != NULL)
                        caf->modifier += corrupt;
                    else
                    {
                        AFFECT_DATA aocaf;
                        aocaf.type = gsn_aura_of_corruption;
                        aocaf.duration = 2;
                        aocaf.location = APPLY_HIDE;
                        aocaf.modifier = corrupt;
                        aocaf.bitvector = 0;
                        aocaf.level = paf->level;
                        affect_to_char(victim,&aocaf);
                    }
                    sprintf(buf,"Your influence corrupts %s.\n\r", victim->name);
                    send_to_char(buf,ch);
                    if (!IS_NPC(ch))
                    { 
                        if ((ch->pcdata->karma - corrupt) >= FAINTREDAURA)
                            ch->pcdata->karma -= corrupt;
                        else 
                            ch->pcdata->karma = FAINTREDAURA;

                        corrupt = aura_grade(ch);
                        if (corrupt < 4) 
                            corrupt -= 1;
                        if (corrupt < 2)
                            affect_strip(ch,gsn_aura_of_corruption);
                        else if (paf->modifier != corrupt)
                            paf->modifier = corrupt;
                    }
                }
            }

            if (is_affected(ch,gsn_tendrilgrowth))
            {
                if ((paf = affect_find(victim->affected,gsn_tendrilgrowth)) == NULL)
                {
                    if (!saves_spell(ch->level,ch, victim,DAM_POISON))
                    {
                        AFFECT_DATA pvaf;
                        pvaf.valid = TRUE;
                        pvaf.point = NULL;
                        pvaf.where = TO_AFFECTS;
                        pvaf.level = ch->level;
                        pvaf.duration = ch->level/5;
                        pvaf.type = gsn_tendrilgrowth;
                        pvaf.modifier = -1;
                        pvaf.location = APPLY_RESIST_POISON;
                        pvaf.bitvector = 0;
                        affect_to_char(victim,&pvaf);
                        act("Your susceptibility to poison increases!",victim,NULL,NULL,TO_CHAR);
                        act("$n's skin takes on an ashen appearance.",victim,NULL,NULL,TO_ROOM);
                    }
                }
                else if (paf->modifier > -20 && paf->modifier < 0)
                {
                    if (!saves_spell(paf->level,ch, victim,DAM_POISON))
                    {
                        act("Your susceptibility to poison increases!",victim,NULL,NULL,TO_CHAR);
                        act("$n's skin takes on an ashen appearance.",victim,NULL,NULL,TO_ROOM);
                        paf->modifier--;
                    }
                }
            }	
        }	

        // Lookup fire damage in the set of damage done
        bool hasFireDamage(false);
        for (size_t i(0); i < damage.size(); ++i)
        {
            if (damage[i].amount > 0 && damage[i].type == DAM_FIRE)
            {
                hasFireDamage = true;
                break;
            }
        }
    
        if (hasFireDamage && ((paf = affect_find(victim->affected, gsn_grease)) != NULL)
        && paf->modifier >= 2 && !is_affected(victim, gsn_enflamed))
        {
            AFFECT_DATA af;
            af.valid = TRUE;
            af.point = NULL;

            send_to_char("Your greased clothing bursts into flames!\n\r", victim);
            act("$n's greased clothing bursts into flames!", victim, NULL, NULL, TO_ROOM);

            af.where	 = TO_CHAR;
            af.type	 = gsn_enflamed;
            af.level	 = victim->level;
            af.duration  = 2;
            af.location	 = APPLY_NONE;
            af.modifier	 = 0;
            af.bitvector = 0;
            affect_to_char(victim, &af);
        }

        /* OMG this brotherhood code sux0rs.  Must fix. */
        mod = 0;
        sub = NULL;
        if (victim->hit < 1 && is_affected(victim, gsn_brotherhood))
        {
            for (paf = victim->affected; paf != NULL; paf = paf->next)
            {
                if (paf->type == gsn_brotherhood)
                    mod = paf->modifier;
            }
        }

        if (mod != 0)
        {
            for (vch = victim->in_room->people; vch != NULL; vch = vch->next_in_room)
            {
                if (is_affected(vch, gsn_brotherhood))
                {
                    for (paf = vch->affected; paf != NULL; paf = paf->next)
                    {
                        if (paf->type == gsn_brotherhood && paf->modifier == mod)
                        {
                            sub = vch;
                            break;
                        }
                    }
                }
            }
        }
    
        if (sub != NULL)
        {
            victim->hit += totalDamage;
            sub->hit -= totalDamage;
            victim = sub;
        }
    }

    if (!IS_NPC(victim) && victim->level >= LEVEL_IMMORTAL && victim->hit < 1)
    	victim->hit = 1;

    if ((victim->hit == 1) && victim->mercy_from)
    {
        if (ch != NULL) stop_fighting_all(ch);
        if (mercy_ch != 0) stop_fighting_all(mercy_ch);
        stop_fighting_all(victim);

        if (victim->mercy_from == mercy_ch)
        {
            act("$N falls to $S knees, your weapon poised above $M.", mercy_ch, NULL, victim, TO_CHAR);
            act("$N falls to $S knees, $n's weapon poised above $M.", mercy_ch, NULL, victim, TO_NOTVICT);
            act("You fall to your knees, $n's weapon poised above you.", mercy_ch, NULL, victim, TO_VICT);
        }

        switch_position(victim, POS_RESTING);

        static char * const him_her [] = { "it", "him", "her" };
        static char * const his_her [] = { "its", "his", "her" };

        sprintf(buf,"%s's weapon poised above %s!",victim->mercy_from->name,him_her[URANGE(0,victim->sex,2)]);
        copy_string(victim->pose,buf);
        sprintf(buf,"%s weapon poised above %s!",his_her[URANGE(0,victim->mercy_from->sex,2)],victim->name);
        copy_string(victim->mercy_from->pose,buf);
        /* I'm sure this is a dangerous return, but I don't want any more */
        /* damage coming out.  -- Aeolis				  */
        return TRUE;
    }

    if (totalDamage <= 0)
        return FALSE;

    update_pos(victim);
    switch( victim->position )
    {
	case POS_MORTAL:
	    act( "$n is mortally wounded, and will die soon, if not aided.", victim, NULL, NULL, TO_ROOM );
	     send_to_char("You are mortally wounded, and will die soon, if not aided.\n\r", victim );
	    break;

        case POS_INCAP:
	    act( "$n is incapacitated and will slowly die, if not aided.", victim, NULL, NULL, TO_ROOM );
	    send_to_char("You are incapacitated and will slowly die, if not aided.\n\r", victim );
	    break;

        case POS_STUNNED:
	    act( "$n is stunned, but will probably recover.", victim, NULL, NULL, TO_ROOM );
	    send_to_char("You are stunned, but will probably recover.\n\r", victim );
	    break;

        case POS_DEAD:
	    if (silver_state != 14)
		if ((IS_NPC(victim) && IS_SET(victim->act, ACT_UNDEAD)) || IS_SET(victim->form, FORM_UNDEAD))
		    act("$n is DESTROYED!", victim, 0, 0, TO_ROOM);
		else
            act( "$n is DEAD!!", victim, 0, 0, TO_ROOM );
	    break;

        default:
	    if ( totalDamage > victim->max_hit / 4 )
	    {
	        if (dt > 0 && dt < TYPE_HIT && skill_table[dt].spell_fun == spell_null)
            {
                if (check_defensiveroll(victim))
                {
                    totalDamage /= 2;
                    send_to_char("You roll with the massive blow, avoiding some of the damage.\n\r",victim);
                }
                else
                    send_to_char("That really did HURT!\n\r",victim);
            }
            else
                send_to_char( "That really did HURT!\n\r", victim );
	    }
	    if ( victim->hit < victim->max_hit / 4 )
	        send_to_char( "You sure are BLEEDING!\n\r", victim );
	    break;
    }

    if ( !IS_AWAKE(victim) )
	stop_fighting(victim);

    if ((victim->position == POS_FIGHTING) && (totalDamage >= 25))
	for (paf = victim->affected; paf; paf = paf->next)
	    if (paf->type == gsn_consuming_rage)
        {
            if ((paf->modifier == 0) && (number_bits(2) == 0))
            {
                send_to_char("Your blood surges as your rage intensifies!\n\r", victim);
                paf->modifier = 1;
                victim->damroll++;
            }
            else if (paf->modifier > 0)
            { 
                paf->modifier += (totalDamage / 25);
                victim->damroll += (totalDamage / 25);
            }
        }

    if (victim->position == POS_DEAD)
    {
        if (ch != NULL)
        {
            if (silver_state == 14)
            {
                OBJ_DATA *sweap;
                CHAR_DATA *veilmob;

                sweap = get_eq_char(ch, WEAR_WIELD);
                if (!sweap || !obj_is_affected(sweap, gsn_consumption))
                sweap = get_eq_char(ch, WEAR_DUAL_WIELD);

                if (!IS_NPC(victim) && sweap && obj_is_affected(sweap, gsn_consumption) && (get_modifier(sweap->affected, gsn_consumption) > 0))
                {
                act("You impale $N through the heart with $p.", ch, sweap, victim, TO_CHAR);
                act("$n impales $N through the heart with $p.", ch, sweap, victim, TO_NOTVICT);
                if ((IS_NPC(victim) && IS_SET(victim->act, ACT_UNDEAD)) || IS_SET(victim->form, FORM_UNDEAD))
                    act("$n is DESTROYED!", victim, 0, 0, TO_ROOM);
                else
                    act( "$n is DEAD!!", victim, 0, 0, TO_ROOM );
                act("Darkness streams out from $p, enveloping the corpse of $N.", ch, sweap, victim, TO_CHAR);
                act("Darkness streams out from $p, enveloping the corpse of $N.", ch, sweap, victim, TO_ROOM);
                veilmob = create_mobile(get_mob_index(MOB_VNUM_SILVER_VEIL));
                veilmob->memfocus[0] = victim;
                veilmob->mobvalue[0] = 10 + get_modifier(sweap->affected, gsn_consumption) * 2;
                char_to_room(veilmob, ch->in_room);
                silver_state = 15;
                }
                else
                {
                if ((IS_NPC(victim) && IS_SET(victim->act, ACT_UNDEAD)) || IS_SET(victim->form, FORM_UNDEAD))
                    act("$n is DESTROYED!", victim, 0, 0, TO_ROOM);
                else
                        act( "$n is DEAD!!", victim, 0, 0, TO_ROOM );
                }
            }

            // Adjust for phantasm feeding trait
            if (ch->master != NULL && PhantasmInfo::traitCount(*ch, PhantasmTrait::Feeding) > 0 
            && number_percent() <= (100 - (20 * (ch->master->level - victim->level))))
            {
                act("Flush with the kill, $n seems to swell up and grow slightly larger!", ch, NULL, NULL, TO_ROOM);
                ch->damage[2] += number_range(3, 9);
                ch->damroll = ch->damage[2];
            }

            if (!IS_NPC(ch) && ch != victim 
              && (ch->class_num == global_int_class_barbarian
                || ch->class_num == global_int_class_gladiator)
              && get_skill(ch, gsn_bloodlust) > number_percent())
            {
                int bhit = number_range(victim->level,
                  ch->level <= victim->level ? victim->level * 2 : victim->level);
                if (bhit > ch->max_hit/20)
                {
                act("$n screams in victory, getting a bloodthirsty look in $s eyes!", ch, NULL, NULL, TO_ROOM);
                    act("You scream in victory as bloodlust fills your veins!", ch, NULL, NULL, TO_CHAR);
                }
                else
                act("You feel invigorated as bloodlust fills your veins.",ch,NULL,NULL,TO_CHAR);
                check_improve(ch,victim,gsn_bloodlust,TRUE,1);
                ch->hit += bhit;
            }

            if (ch->class_num == global_int_class_druid)
            {
                if (number_percent() < get_skill(ch,gsn_deathempowerment))
                {
                    if ((paf = affect_find(ch->affected,gsn_deathempowerment)) != NULL)
                    {
                            int gain = victim->level;
                        if (paf->modifier + gain > ch->level * 4)
                        gain = ch->level * 4 - paf->modifier;
                        paf->duration = 36;
                        if (gain > 0)
                        {
                        paf->modifier += gain;
                        ch->hit += gain;
                        ch->max_hit += gain;
                        }		    
                    }
                    else
                    {
                        AFFECT_DATA naf;
                        naf.valid = TRUE;
                        naf.point = NULL;
                        naf.type = gsn_deathempowerment;
                        naf.where = TO_AFFECTS;
                        naf.duration = 48;
                        naf.location = APPLY_HIT;
                        naf.modifier = victim->level;
                        naf.bitvector = 0;
                        affect_to_char(ch,&naf);
                    }
                    check_improve(ch,victim,1,gsn_deathempowerment,TRUE);
                    send_to_char("You feel your affinity to death grow.\n\r",ch);
                }
                else
                    check_improve(ch,victim,1,gsn_deathempowerment,FALSE);
            }
        }

        kill_char(victim, ch);
        if (!IS_NPC(victim) && !IS_OAFFECTED(victim, AFF_GHOST))
            return TRUE;

        for (vch = char_list; vch !=NULL; vch = vch_next)
        {
            vch_next = vch->next;
            if (vch->demontrack == victim)
            {
                act("$n, satisfied at the death of $N, returns to the netherworld, tearing through the fabric of the universe to enter the void.", vch, NULL, victim, TO_ROOM);
                extract_char(vch, TRUE);
            }
        }
            /* RT new auto commands */

        OBJ_DATA * corpse(NULL);
        if (ch != NULL && !IS_NPC(ch)
        &&  (corpse = get_obj_list(ch,"corpse",ch->in_room->contents)) != NULL
        &&  ((corpse->item_type == ITEM_CORPSE_NPC || 
        (corpse->item_type == ITEM_CORPSE_PC))) // && ch->class_num == global_int_class_bandit)))
        && can_see_obj(ch,corpse))
        {
            OBJ_DATA *coins;

            corpse = get_obj_list( ch, "corpse", ch->in_room->contents ); 

            if (corpse->item_type == ITEM_CORPSE_NPC)
            {
                if ( IS_SET(ch->act, PLR_AUTOLOOT) && corpse && corpse->contains) /* exists and not empty */
                do_get( ch, "all corpse" );

                if (IS_SET(ch->act,PLR_AUTOGOLD) && corpse && corpse->contains 
             && !IS_SET(ch->act,PLR_AUTOLOOT))
                if ((coins = get_obj_list(ch,"gcash",corpse->contains)) != NULL)
                        do_get(ch, "all.gcash corpse");
                
            }
            else
            {
                int pskill;

                if (corpse && corpse->contains
                && (IS_SET(ch->act, PLR_AUTOLOOT) || IS_SET(ch->act, PLR_AUTOGOLD))
                && ((pskill = get_skill(ch, gsn_plunder)) > 0))
                {
                    bool psucc=FALSE;
                    send_to_char("You quickly attempt to plunder your fallen foe...\n\r", ch);
                    if (number_percent() < pskill)
                    {
                        check_improve(ch, victim, gsn_plunder, TRUE, 0);
                        pskill /= 2;

                        if (IS_SET(ch->act, PLR_AUTOLOOT))
                        {
                        int icount = 0;
                        OBJ_DATA *plunder;

                            for (plunder = corpse->contains; plunder; plunder = plunder->next_content)
                            icount++;

                            while (icount > 0)
                            {
                            if (number_percent() < pskill)
                        {
                            sprintf(buf, "%d. corpse", icount);
                            do_get(ch, buf);
                            psucc = TRUE;
                        }

                        icount--;
                        }
                    }

                        if (IS_SET(ch->act, PLR_AUTOGOLD) && !IS_SET(ch->act, PLR_AUTOLOOT))
                            if ((coins = get_obj_list(ch, "gcash", corpse->contains)) != NULL)
                                do_get(ch, "all.gcash corpse");

                        if (!psucc)
                        {
                            send_to_char("You didn't get anything.\n\r",ch);
                            check_improve(ch,victim,gsn_plunder,FALSE,1);
                        }
                    }
                    else
                    {
                        check_improve(ch,victim,gsn_plunder,FALSE,1);
                        send_to_char("You didn't get anything.\n\r",ch);
                    }
                }
            }     
        }

        if (ch != NULL && ch->in_room && !IS_NPC(ch) && IS_SET(ch->act, PLR_AUTODES)
        && corpse && corpse->pIndexData->vnum == OBJ_VNUM_CORPSE_NPC)
            do_destroy( ch, "npccorpse" );

        return TRUE;
    }

    if (ch != NULL)
    {
        if (ch->fighting == ch)
            ch->fighting = NULL;

        if ( victim == ch )
            return TRUE;

        // Take care of link dead people.
        if (!IS_NPC(victim) && victim->desc == NULL && !is_affected(victim, gsn_bilocation))
        {
            if ((number_range( 0, victim->wait ) == 0) && (ch->in_room == victim->in_room) && !global_bool_final_valid)
            {
                do_flee( victim, "" );
                return TRUE;
            }
        }
    }

    // Wimp out?
    if (!IS_NPC(victim) && victim->hit > 0 && victim->hit <= victim->wimpy)
    {
        if (is_affected(victim, gsn_blink) && (victim->mana > 0)
        && !IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
        && (number_bits(2) == 0))
        {
            int x = 0;
            ROOM_INDEX_DATA *pRoom = NULL;

            if (victim->mana > 1)
            victim->mana /= 2;
            else
            victim->mana = 0;

            while (x < 100)
            {
                pRoom = get_random_room(victim); 
                x++;
                if (victim->in_room->area == pRoom->area)
                    break;
            }

            if (x < 99 && pRoom)
            {
                send_to_char("You momentarily blink out of existence, appearing somewhere nearby.\n\r", victim);
                act("$n blinks out of existence.", victim, NULL, NULL, TO_ROOM);
                char_from_room(victim);
                char_to_room(victim, pRoom);
                do_look(victim, "auto");
                return TRUE;
            }
        }

        if ( IS_NPC(victim) && totalDamage > 0 && victim->wait < PULSE_VIOLENCE / 2)
        {
            if ( ( IS_SET(victim->act, ACT_WIMPY) && number_bits( 2 ) == 0
            &&   victim->hit < victim->max_hit / 5
            &&   !IS_OAFFECTED(victim,AFF_DEMONPOS))
            ||   ( IS_AFFECTED(victim, AFF_CHARM) && victim->master != NULL
            &&     victim->master->in_room != victim->in_room ) )
                do_flee( victim, "" );
        }

        if (!global_bool_final_valid && victim->wait < PULSE_VIOLENCE / 2 )
    	    do_flee( victim, "" );
    }

    return TRUE;
}

/*
 * Inflict damage from a hit.
 */
bool damage(CHAR_DATA *ch,CHAR_DATA *victim,int dam,int dt,int dam_type, bool show) 
{
    return damage_new(ch, victim, dam, dt, dam_type, show, NULL);
}

static bool damage_from_obj_helper(CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA & weapon, int dam, int dt, int dam_type, bool show, const char * noun)
{
    // Check for bind essence
    AFFECT_DATA * bind(get_obj_affect(&weapon, gsn_bindessence));
    if (bind != NULL && dam > 0)
    {
        static std::vector<DamageInfo> damage(2);
        damage[0].type = dam_type;
        damage[0].amount = dam;
        damage[1].type = DAM_HOLY;
        damage[1].amount = 1 + number_range(0, bind->level / 8);

        std::string holynoun(!str_prefix("holy", noun) ? "" : "holy ");
        holynoun += noun;
        return damage_new(ch, victim, damage, dt, show, const_cast<char*>(holynoun.c_str()));
    }

    return damage_new(ch, victim, dam, dt, dam_type, show, const_cast<char*>(noun));
}

bool damage_from_obj(CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj, int dam, int dt, int dam_type, bool show)
{
    bool isPhysical;
    switch (dam_type)
    {
        case DAM_SLASH: case DAM_BASH: case DAM_PIERCE: isPhysical = true; break;
        default: isPhysical = false; break;
    }

    bool isWeapon(obj && ((obj->item_type == ITEM_WEAPON) || (obj->item_type == ITEM_ARROW)));
    if (isPhysical && victim->in_room != NULL && room_is_affected(victim->in_room, gsn_markofthekaceajka))
    {
        if (isWeapon) return damage_from_obj_helper(ch, victim, *obj, dam, dt, DAM_COLD, show, "chill");
        return damage_new(ch, victim, dam, dt, DAM_COLD, show, "chill");
    }

    if (isWeapon)
    {
        AFFECT_DATA * baneBlade(get_obj_affect(obj, gsn_baneblade));
        if (baneBlade != NULL)
        {
            if (baneBlade->modifier >= 5) return damage_from_obj_helper(ch, victim, *obj, dam, dt, DAM_DEFILEMENT, show, "defilement");
            else return damage_from_obj_helper(ch, victim, *obj, dam, dt, DAM_NEGATIVE, show, "deathly chill");
        }
        if (obj_is_affected(obj, gsn_screamingarrow))  return damage_from_obj_helper(ch, victim, *obj, dam, dt, DAM_SOUND, show, "");
    	if (obj_is_affected(obj, gsn_frostbrand))      return damage_from_obj_helper(ch, victim, *obj, dam, dt, DAM_COLD, show, "frost");
    	if (obj_is_affected(obj, gsn_lightningbrand))  return damage_from_obj_helper(ch, victim, *obj, dam, dt, DAM_LIGHTNING, show, "shock");
    	if (obj_is_affected(obj, gsn_consecrate))      return damage_from_obj_helper(ch, victim, *obj, dam, dt, DAM_HOLY, show, "divine power");
    	if (obj_is_affected(obj, gsn_desecrateweapon)) return damage_from_obj_helper(ch, victim, *obj, dam, dt, DAM_NEGATIVE, show, "deathly chill");
    	if (obj_is_affected(obj, gsn_firebrand) || obj_is_affected(obj, gsn_soulbrand))        return damage_from_obj_helper(ch, victim, *obj, dam, dt, DAM_FIRE, show, "flame");
    	if (obj_is_affected(obj, gsn_defilement) && !is_affected(ch, gsn_roaroftheexalted))    return damage_from_obj_helper(ch, victim, *obj, dam, dt, DAM_DEFILEMENT, show, "defilement");
        return damage_from_obj_helper(ch, victim, *obj, dam, dt, dam_type, show, obj->obj_str);
    }
    
    if (get_modifier(ch->affected, gsn_tendrilgrowth) >= 0) 
        return damage_new(ch, victim, dam, dt, DAM_PIERCE, show, "thorn");

    return damage_new(ch, victim, dam, dt, dam_type, show, NULL);
}
	
/*
 * Inflict damage from a hit.
 */
bool damage_old(CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt, int dam_type, bool show)
{
    return damage_new(ch, victim, dam, dt, dam_type, show, NULL);
}

bool is_safe(CHAR_DATA *ch, CHAR_DATA *victim)
{
    CHAR_DATA *vch;
    AFFECT_DATA *song, *kiss;

    if (victim->in_room == NULL || ch->in_room == NULL)
        return TRUE;

    bool beingFought(false);
    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
        if (vch->fighting == ch)
        {
            beingFought = true;
            break;
        }
    }


    if ((ch->fighting == NULL && !beingFought) && (is_affected(ch,gsn_subdue) || is_affected(ch, gsn_masspeace) || is_affected(ch, gsn_pacify)))
    {
        send_to_char("You're too calm to do that now.\n\r", ch);
        return TRUE;
    }

    if (!beingFought)
    {
        for (AFFECT_DATA * sonic(get_affect(ch, gsn_sonicboom)); sonic != NULL; sonic = get_affect(ch, gsn_sonicboom, sonic))
        {
            if (sonic->modifier >= 0)
            {
                send_to_char("You are still too stunned from the sonic boom to do that just yet.\n", ch);
                return true;
            }
        }
    }

    if (is_affected(ch,gsn_powerwordfear) && (ch->fighting == NULL))
    {
        send_to_char("You are too scared to do that!\n\r", ch);
        return TRUE;
    }

    if (is_affected(ch, gsn_shadowmastery) && !IS_AFFECTED(victim, AFF_HIDE))
    {
        send_to_char("You are too deep into the shadows to strike at them.\n\r", ch);
        SET_BIT(ch->oaffected_by, AFF_SHADOWMASTERY);
        return TRUE;
    }

    if (!victim->fighting && victim != ch)
        counter_works = TRUE;
    else
        counter_works = FALSE;

    if (IS_AFFECTED(victim, AFF_WIZI))
	return TRUE;

    if (victim->fighting == ch || victim == ch)
	return FALSE;

    if (IS_IMMORTAL(ch) && ch->level > LEVEL_IMMORTAL)
	return FALSE;

    if (IS_PAFFECTED(victim, AFF_VOIDWALK))
	return TRUE;

    if ((song = get_room_song(ch->in_room, gsn_songofsoothing)) != NULL)
    {
	if ((((CHAR_DATA *) song->point) != ch) 
	  && can_be_affected(ch,gsn_songofsoothing)
	  && (number_bits(2) != 0))
	{
	    act("Soothed by the sounds of the music, you abandon your aggressive thoughts.", ch, NULL, NULL, TO_CHAR);
	    act("$n is soothed by the gentle sounds of the music.", ch, NULL, NULL, TO_ROOM);
	    WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE/2));
	    return TRUE;
	}
    }

    if (room_is_affected(ch->in_room, gsn_stonetomud)
     && !is_flying(ch)
     && !ch->fighting
     && !is_affected(ch, gsn_stabilize))
    {
	send_to_char("You're too mired in mud to quite manage.\n\r", ch);
	return TRUE;
    }

    if (IS_NAFFECTED(ch, AFF_FLESHTOSTONE) || IS_OAFFECTED(ch, AFF_ENCASE))
    {
	stop_fighting_all(ch);
	return TRUE;
    }

    if (IS_OAFFECTED(ch, AFF_UNCLEANSPIRIT) && ch->in_room)
    {
        if (is_symbol_present(*ch->in_room, OBJ_VNUM_SYMBOL_PROTECT))
        {
            act("$N is protected within the inscripted Circle.", ch, NULL, victim, TO_CHAR);
            return true;
	    }
    }

    if (IS_NAFFECTED(victim, AFF_FLESHTOSTONE))
    {
	act("$n is protected by $s shell of stone.", victim, NULL, NULL, TO_ROOM);
	act("You are protected by your shell of stone.", victim, NULL, NULL, TO_CHAR);
	stop_fighting_all(victim);
	return TRUE;
    }

    if (IS_OAFFECTED(victim, AFF_ENCASE))
    {
	act("$n is protected by a thick layer of ice.", victim, NULL, NULL, TO_ROOM);
	act("You are protected by the encasing ice.", victim, NULL, NULL, TO_CHAR);
	stop_fighting_all(victim);
	return TRUE;
    }

    if (is_affected(victim, gsn_division) && victim->fighting != ch && (victim->fighting != NULL && !IS_NPC(victim->fighting)) && (!IS_NPC(ch) || ch->master != NULL))
    {
	act ("$n is able to keep $N divided from any assistance.", victim, NULL, victim->fighting, TO_NOTVICT);
	act ("You are able to keep $N divided from any assistance.", victim, NULL, victim->fighting, TO_CHAR);
	act ("$n is able to keep you divided from any assistance.", victim, NULL, victim->fighting, TO_VICT);
	return TRUE;
    }

    /* more ghosthood stuff */
    if (IS_OAFFECTED(victim, AFF_GHOST))
    {
	switch_position(victim, POS_STANDING);
	switch_position(ch, POS_STANDING);
	act ("$n is protected by the gods.", victim, NULL,NULL,TO_ROOM);

	if (victim->fighting != NULL)
	    stop_fighting_all(victim);

	return TRUE;
    }
    /* safe room? Adding up here for hero day, heroday  */
    if (IS_SET(victim->in_room->room_flags,ROOM_SAFE))
    {
        send_to_char("Not in this room.\n\r",ch);
		stop_fighting_all(victim);
        return TRUE;
    }
    
    if (IS_OAFFECTED(ch, AFF_GHOST)) // && (!IS_NPC(victim) || ch->perm_stat[STAT_CON] < 4))
    {
	act ("$N is protected by the gods.", ch, NULL, victim, TO_ROOM);
	act ("$N is protected by the gods.", ch, NULL, victim, TO_CHAR);
	return TRUE;
    }

    /* killing mobiles */
    if (IS_NPC(victim) && (victim->desc == NULL || !is_affected(ch, gsn_possession)))
    {

	/* safe room?*/
	if (IS_SET(victim->in_room->room_flags,ROOM_SAFE))
	{
	    send_to_char("Not in this room.\n\r",ch);
	    return TRUE;
	} 

	if (!IS_NPC(ch))
	{
	    /* no pets out of range if owner is there*/
	    if (victim->master != NULL && victim->in_room && victim->master->in_room && victim->in_room == victim->master->in_room && !IS_PK(victim->master, ch) && !IS_NPC(victim->master))
	    {
		act ("The gods protect $N.", ch, NULL, victim, TO_CHAR);
		act ("The gods protect $N.", ch, NULL, victim, TO_NOTVICT);
		act ("The gods protect you.", ch, NULL, victim, TO_VICT);
		return TRUE;
	    }

	}

	if (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL && victim->master != NULL && victim->in_room && victim->master->in_room && ch->master->in_room && (victim->in_room == victim->master->in_room || IS_SET(victim->act, ACT_FAMILIAR)) && !IS_PK(victim->master, ch->master) && !IS_NPC(victim->master))
	    {
		act ("The gods protect $N.", ch, NULL, victim, TO_CHAR);
		act ("The gods protect $N.", ch, NULL, victim, TO_NOTVICT);
		act ("The gods protect you.", ch, NULL, victim, TO_VICT);
		return TRUE;
	    }

    }
    /* killing players */
    else
    {
	for (kiss = ch->affected; kiss; kiss = kiss->next)
	    if ((kiss->type == gsn_succubuskiss) && (kiss->modifier != 0) && (kiss->modifier == victim->id))
	    {
		act("Sensations of pleasure overwhelm you as you attempt to attack $N.", ch, NULL, victim, TO_CHAR);
		act("You fall to your knees in bliss, unable to continue.", ch, NULL, NULL, TO_CHAR);
		act("$n lets out a small gasp, and falls to $s knees.", ch, NULL, NULL, TO_ROOM);
		WAIT_STATE(ch, PULSE_VIOLENCE);
		return TRUE;
	    }

	/* NPC doing the killing */
	if (IS_NPC(ch) && (ch->desc == NULL || ch->trust > 51))
	{
	    for (vch = ch->in_room->people;vch != NULL;vch = vch->next_in_room)
     	    {
	      if (vch->guarding && vch->guarding == victim && (IS_PK(vch, ch) || IS_NPC(ch))
		&& vch->fighting == NULL && victim->fighting != ch && protect_works
		&& ch != vch && vch->position == POS_STANDING)
	      {
		act("$n jumps in front of $N!", vch, NULL, victim, TO_NOTVICT);
		act("You jump in front of $N!", vch, NULL, victim, TO_CHAR);
		act("$n jumps in front of you!", vch, NULL, victim, TO_VICT);
		global_counter_works = FALSE;
		one_hit(ch, vch, TYPE_UNDEFINED, HIT_PRIMARY);
		global_counter_works = TRUE;
		return TRUE;
	      }
	    }

	    /* safe room check */
	    if (IS_SET(victim->in_room->room_flags,ROOM_SAFE))
	    {
		send_to_char("Not in this room.\n\r",ch);
		return TRUE;
	    }

	    if (ch->master && !IS_NPC(ch->master) 
	      && (!IS_PK(ch->master, victim) || (victim->level < 10)))
		return TRUE;

	}
	/* player doing the killing */
	else
	{
            if (is_affected(ch, gsn_possession)
	     && ch->desc && ch->desc->original
	     && !IS_PK(ch->desc->original, victim))
            {
		act ("The gods protect $N from $n.", ch, NULL, victim, TO_CHAR);
		act ("The gods protect $N from $n.", ch, NULL, victim, TO_NOTVICT);
		act ("The gods protect you from $n.", ch, NULL, victim, TO_VICT);
                return TRUE;
            }
	  

	if	(!IS_PK(ch, victim) || victim->level < 10)
	  if ((IS_NPC(ch) && (ch->pIndexData->vnum == MOB_VNUM_SHUNNED_DEMON) && ch->desc && ch->desc->original && !IS_PK(ch->desc->original, victim)) || (is_affected(ch, gsn_commandword) && !IS_PK(ch->commander, victim)) || !IS_NPC(ch) || ((ch->level - victim->level) > 8 && ch->desc->original->trust < 52))
            {
		act ("The gods protect $N from $n.", ch, NULL, victim, TO_CHAR);
		act ("The gods protect $N from $n.", ch, NULL, victim, TO_NOTVICT);
		act ("The gods protect you from $n.", ch, NULL, victim, TO_VICT);
                return TRUE;
            }

            if (ch->level < 8)
            {
                send_to_char("You are too young to be murdering players.\n\r",ch);
                return TRUE;
        
	    }


	    for (vch = ch->in_room->people;vch != NULL;vch = vch->next_in_room)
     	    {
	      if (vch->guarding && vch->guarding == victim && (IS_PK(vch, ch) || IS_NPC(ch))
		&& vch->fighting == NULL && victim->fighting != ch && protect_works
		&& ch != vch && vch->position == POS_STANDING)
	      {
		act("$n jumps in front of $N!", vch, NULL, victim, TO_NOTVICT);
		act("You jump in front of $N!", vch, NULL, victim, TO_CHAR);
		act("$n jumps in front of you!", vch, NULL, victim, TO_VICT);
		global_counter_works = FALSE;
		one_hit(ch, vch, TYPE_UNDEFINED, HIT_PRIMARY);
		global_counter_works = TRUE;
		return TRUE;
	      }
	    }

        }
    }
    return FALSE;
}

bool is_safe_druid(CHAR_DATA *ch)
{

CHAR_DATA *vch;

if (ch->class_num == global_int_class_druid)
	return TRUE;

for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
	{
	if ((!IS_NPC(vch)) && vch->class_num == global_int_class_druid)
		if (is_same_group(ch, vch))
			return TRUE;
	}


return FALSE;
}

 
bool is_safe_spell(CHAR_DATA *ch, CHAR_DATA *victim, bool area )
{
    if (IS_AFFECTED(victim, AFF_WIZI))
	return TRUE;

    if (victim->in_room == NULL || ch->in_room == NULL)
        return TRUE;

    if (area)
    {
        if (is_affected(victim, gsn_meldwithstone) || is_affected(victim, gsn_flameunity) || (IS_NPC(victim) && IS_SET(victim->nact, ACT_SHADE)))
        	return TRUE;
    }

    if (IS_NAFFECTED(victim, AFF_FLESHTOSTONE))
    {
	act("$n is protected by $s shell of stone.", victim, NULL, NULL, TO_ROOM);
	act("You are protected by your shell of stone.", victim, NULL, NULL, TO_CHAR);
	stop_fighting_all(victim);
	return TRUE;
    }

    if (IS_OAFFECTED(victim, AFF_ENCASE))
    {
	act("$n is protected by a thick layer of ice.", victim, NULL, NULL, TO_ROOM);
	act("You are protected by the encasing ice.", victim, NULL, NULL, TO_CHAR);
	stop_fighting_all(victim);
	return TRUE;
    }

    if (victim->fighting == ch)
	return FALSE;

    if (is_affected(ch,gsn_subdue) || is_affected(ch, gsn_pacify))
    {
        send_to_char ("You feel too mellow to do that.\n\r", ch);
        return TRUE;
    }

    for (AFFECT_DATA * sonic(get_affect(ch, gsn_sonicboom)); sonic != NULL; sonic = get_affect(ch, gsn_sonicboom, sonic))
    {
        if (sonic->modifier >= 0)
        {
            send_to_char("You are still too stunned from the sonic boom to do that just yet.\n", ch);
            return true;
        }
    }

    if (is_affected(ch, gsn_powerwordfear) && (ch->fighting == NULL))
    {
	send_to_char ("You are too scared to do that!\n\r", ch);
	return TRUE;
    }

    if (victim == ch && area)
	return TRUE;


    if (IS_PAFFECTED(victim, AFF_VOIDWALK))
	return TRUE;

    /* ghosthood again */
    if (IS_OAFFECTED(victim, AFF_GHOST))
    {
	act ("$n is protected by the gods.",victim, NULL,NULL, TO_ROOM);
	switch_position(victim, POS_STANDING);
	switch_position(ch, POS_STANDING);
	return TRUE;
    }
    if (IS_OAFFECTED(ch, AFF_GHOST)) // && (!IS_NPC(victim) || ch->perm_stat[STAT_CON] < 4))
    {
	act ("$N is protected by the gods.", ch, NULL, victim, TO_ROOM);
	act ("$N is protected by the gods.", ch, NULL, victim, TO_CHAR);
	return TRUE;
    }

    if (IS_IMMORTAL(ch) && ch->level > LEVEL_IMMORTAL && !area)
	return FALSE;

    if (!IS_NPC(victim) && !IS_NPC(ch) && !IS_PK(ch, victim) && !IS_IMMORTAL(ch))
	return TRUE;

    /* killing mobiles */
    if (IS_NPC(victim))
    {
	/* safe room? */
	if (IS_SET(victim->in_room->room_flags,ROOM_SAFE))
	    return TRUE;

	if (!IS_NPC(ch))
	{
	    /* no pets */
	    if (IS_SET(victim->act,ACT_PET))
	   	return TRUE;

	    /* no charmed creatures unless owner */
	    if (IS_AFFECTED(victim,AFF_CHARM) && (area || ch != victim->master))
		return TRUE;
	}
    }
    /* killing players */
    else
    {
	if (area && IS_IMMORTAL(victim) && victim->level > LEVEL_IMMORTAL)
	    return TRUE;

	/* NPC doing the killing */
	if (IS_NPC(ch) && ch->desc == NULL)
	{
	    /* charmed mobs and pets cannot attack players while owned */
	    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master != NULL
	    &&  ch->master->fighting != victim)
		return TRUE;

	    if (ch->commander && !IS_PK(ch->commander, victim))
		return TRUE;
	
	    /* safe room? */
	    if (IS_SET(victim->in_room->room_flags,ROOM_SAFE))
		return TRUE;

	    /* legal kill? -- mobs only hit players grouped with opponent*/
	    if (ch->fighting != NULL && !is_same_group(ch->fighting,victim))
		return TRUE;
	}

	/* player doing the killing */

    }
    return FALSE;
}

/*
 * See if an attack justifies a KILLER flag.
 */
void check_killer( CHAR_DATA *ch, CHAR_DATA *victim )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *vch, *vch_next;
	
    if (victim->position <= POS_SLEEPING || victim->in_room == NULL) return;
	
    /*
     * Follow charm thread to responsible character.
     * Attacking someone's charmed char is hostile!
     */
    if (is_affected(victim, gsn_wardoffire) && ch->in_room == victim->in_room)
    {
	act("A gout of flame explodes from your ward of fire, burning $n!",
	    ch, NULL, victim, TO_VICT);
	act("A gout of flame explodes from $N's ward of fire, burning you!",
	    ch, NULL, victim, TO_CHAR);
	act("A gout of flame explodes from $N's ward of fire, burning $n!",
	    ch, NULL, victim, TO_NOTVICT);
	affect_strip(victim, gsn_wardoffire);

	check_killer(ch, victim);
	damage(victim, ch, dice(victim->level, 4), gsn_wardoffire, DAM_FIRE, TRUE);
    }

    while ( IS_AFFECTED(victim, AFF_CHARM) && victim->master != NULL )
	victim = victim->master;

    if (IS_NPC(victim) && IS_SET(victim->act, ACT_GUILDGUARD) && (ch->fighting == NULL || victim->fighting == NULL))
    {
	sprintf(buf, "Help! %s is attacking the guild!", PERS(ch, victim));
	do_yell(victim, buf);
	sprintf(buf, "%s attacks a guild guard [%s] at %s [room %d]",(IS_NPC(ch) ? ch->short_descr : ch->name),(IS_NPC(victim) ? victim->short_descr : victim->name),victim->in_room->name, victim->in_room->vnum);
	log_string(buf);
    }

    if (!victim->fighting && ch->in_room)
    {
	for (vch = ch->in_room->people; vch; vch = vch_next)
        {
	    vch_next = vch->next_in_room;

	    if (IS_NPC(vch))
	        mprog_attack_trigger(vch, ch, victim);
	}

	rprog_attack_trigger(ch->in_room, ch, victim);
    }
    
    if (!IS_NPC(ch) && !IS_SET(ch->act,PLR_AUTOATTACK))
        SET_BIT(ch->act,PLR_AUTOATTACK);

    /*
     * NPC's are fair game.
     * So are killers and thieves.
     */
    if ( IS_NPC(victim))
	return;

    if (ch->mercy_from)
	return;

    if (victim->mercy_to)
    {
	if (IS_SET(victim->nact, PLR_MERCY_FORCE))
	{
	    REMOVE_BIT(victim->nact, PLR_MERCY_FORCE);
	    do_murder(victim, victim->mercy_to->name);
	    SET_BIT(victim->nact, PLR_MERCY_FORCE);
	}

	if (victim->mercy_to)
	{
	    victim->mercy_to->mercy_from = NULL;
	    victim->mercy_to = NULL;
	}
    }

    if ((vch = victim->mercy_from))
    {
	if (IS_SET(vch->nact, PLR_MERCY_FORCE))
	{
	    REMOVE_BIT(vch->nact, PLR_MERCY_FORCE);
	    do_murder(vch, victim->name);
	    SET_BIT(vch->nact, PLR_MERCY_FORCE);
	}

    }

    /*
     * Charm-o-rama.
     */
    if ( IS_SET(ch->affected_by, AFF_CHARM) )
    {
	if ( ch->master == NULL )
	{
	    char buf[MAX_STRING_LENGTH];

	    sprintf( buf, "Check_killer: %s bad AFF_CHARM",
		IS_NPC(ch) ? ch->short_descr : ch->name );
	    bug( buf, 0 );
	    affect_strip( ch, gsn_charm_person );
	    REMOVE_BIT( ch->affected_by, AFF_CHARM );
	    return;
	}
	return;
    }

    /*
     * NPC's are cool of course (as long as not charmed).
     * Hitting yourself is cool too (bleeding).
     * So is being immortal (Alander's idea).
     * And current killers stay as they are.
     */
    if ( IS_NPC(ch)
    ||   ch == victim
    ||   ch->level >= LEVEL_IMMORTAL
    ||	 ch->fighting  == victim)
	return;

    sprintf(log_buf, "%s is attempting to murder %s.",ch->name, victim->name);
    check_comm(ch, log_buf);
    check_comm (victim, log_buf);

    sprintf(buf,"%s is attempting to murder %s in room %d",ch->name, victim->name, victim->in_room->vnum);
    log_string(buf);

    sprintf(buf,"$N is attempting to murder %s in room %d",victim->name, victim->in_room->vnum);
    wiznet(buf,ch,NULL,WIZ_FLAGS,0,0);

    if (!IS_CRIMINAL(victim) && !IS_SET(victim->in_room->room_flags,ROOM_SAFE))
    {
	CHAR_DATA *guard, *guard_next;

        for (guard = ch->in_room->people; guard; guard = guard_next)
        {
	    guard_next = guard->next_in_room;

	    if (!guard->fighting && (IS_SET(guard->off_flags, ASSIST_GUARD) || IS_SET(guard->off_flags, ASSIST_BOUNCER)) && can_see(guard,victim))
	    {
		if (IS_SET(guard->off_flags, ASSIST_GUARD))
		{
	            int ynum = number_range(1,4);
		    switch (ynum)
		    {
		    	case 1:
			    sprintf(buf, "The laws shall be upheld!  You will be punished, %s!", PERS(ch, guard));
			    break;
		    	case 2:
			    sprintf(buf, "You will not disrupt the peace, %s!", PERS(ch, guard));
			    break;
		        case 3:
			    sprintf(buf, "There shall be no murder committed here, %s!", PERS(ch, guard));
			    break;
		    	case 4:
			    sprintf(buf, "Cease these unlawful actions, %s!", PERS(ch, guard));
			    break;
		    }

		    do_yell(guard, buf);
		}
		multi_hit(guard, ch, TYPE_UNDEFINED);
		if (number_percent() < get_skill(ch,gsn_ferocity)*3/4)
		{
		    act("Undaunted by $N, you continue your assault.",ch,NULL,guard,TO_CHAR);
		    stop_fighting(ch);
		    check_improve(ch,victim,gsn_ferocity,TRUE,1);
		}
		else
		    check_improve(ch,victim,gsn_ferocity,FALSE,1);
	    }
	}
    }

    if (is_affected(victim, gsn_commune_nature))
    {
	sprintf(buf, "Help!  I am being attacked by %s!", ch->name);
	do_druidtalk(victim, buf);
    } 
}


/*
 * Check for parry.
 */
bool check_offhandparry( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj)
{
    OBJ_DATA *vObj,*cObj;
    int chance;
    int skill;
    int hitmod;

    if ( !IS_AWAKE(victim) )
	return FALSE;

    if (is_affected(victim, gsn_form_of_the_bear))
	return FALSE;

    if (is_affected(victim, gsn_form_of_the_dragon) && number_bits(2) != 0)
	return FALSE;

    if (is_affected(victim, gsn_levitation))
	return FALSE;

    chance = get_skill(victim,gsn_offhandparry) * 2 / 5;
    if (chance == 0)
	return FALSE;

    if ((vObj = get_eq_char(victim, WEAR_DUAL_WIELD)) == NULL)
	return FALSE;
   
    hitmod = GET_HITROLL(ch);

    if (hitmod >= 75)
        hitmod = ((hitmod-75)/20) + 75;

    hitmod = hitmod / 8;
    chance -= hitmod;
    // Factor in dex
	chance += (get_curr_stat(victim, STAT_DEX) - 18);

    switch (vObj->value[0])
    {
	case WEAPON_SWORD: 	chance = round(chance * 1.1); 	break;
	case WEAPON_MACE: 	chance = round(chance / 1.1); 	break;
	case WEAPON_AXE: 	chance = round(chance / 1.1); 	break;
	case WEAPON_FLAIL: 	chance = round(chance / 1.2); 	break;
	case WEAPON_WHIP: 	chance = round(chance / 1.2); 	break;
	case WEAPON_EXOTIC:
	    if (victim->class_num == global_int_class_gladiator)
		chance = round(chance * 1.1);
	    break;
	default:				break;
    }

    if (vObj->pIndexData->vnum == OBJ_VNUM_HEIRLOOM)
	chance = round(chance * 1.1);
    
    // 10% bonus for having a weapon forged for parrying
    chance = (chance * (100 + static_cast<int>(Forge::TraitCount(*vObj, Forge::Trait_Parry) * 10))) / 100;

    if ((cObj = get_eq_char(victim,WEAR_WIELD)) != NULL)
    {
	skill = get_weapon_skill_weapon(victim,cObj);
	chance = ((chance *(skill+100))/200);
    }

    if (obj != NULL)
    switch (obj->value[0])
    {	
	case -1: break;
	case WEAPON_WHIP:
	case WEAPON_KNIFE:
	case WEAPON_DAGGER:	chance = round(chance / 1.2);	break;
	case WEAPON_SPEAR: 	chance = round(chance * 1.1);	break;
	case WEAPON_AXE:
	case WEAPON_FLAIL:
	case WEAPON_MACE: 	chance = round(chance / 1.1);	break;
	case WEAPON_POLEARM:	chance = round(chance * 1.25);	break; 
	case WEAPON_EXOTIC:
	    if (victim->class_num == global_int_class_gladiator)
		chance = round(chance / 1.1);
	break;
	default: 				break;
    }
    if (obj != NULL && obj->pIndexData->vnum == OBJ_VNUM_HEIRLOOM)
	chance = round(chance / 1.1);

    if (!can_see(victim, ch) && !is_blindfighter(victim,TRUE))
	chance /= 2;

    if (is_affected(ch, gsn_form_of_the_dragon))
	chance /= 2;

    if (is_affected(victim, gsn_form_of_the_reed))
	chance *= 2;

    if (is_affected(victim, gsn_form_of_the_monkey))
	chance = round(chance * 1.3);
    chance += victim->level - ch->level;

    whinyassbitch(ch,victim,"Offhand parry",chance);

    if ( number_percent( ) >= chance )
	return FALSE;

    if (vObj && obj_is_affected(vObj, gsn_cursebarkja) && (number_percent() < 26))
    {
        act("You move to parry $n's blow with your off hand, but your weapon jerks out of its path!", ch, NULL, victim, TO_VICT);
	return FALSE;
    }
    
    act("You parry $n's attack with your off hand.", ch, NULL, victim, TO_VICT);
    act("$N parries your attack with $S off hand.", ch, NULL, victim, TO_CHAR);
    check_improve(victim,ch,gsn_offhandparry,TRUE,5);

    check_enthusiasm(ch,victim);

    if (is_affected(victim, gsn_form_of_the_rose) 
     && (number_percent() < (skill = get_skill(victim, gsn_form_of_the_rose)) / 6))
    {
	int dam, diceroll;
	
	if (obj == NULL)
	    return TRUE;

        if (obj->pIndexData->new_format)
            dam = dice(obj->value[1],obj->value[2]);
        else
            dam = number_range( obj->value[1],  obj->value[2]);

	dam = (dam * skill) / 60;

        dam = dam * 11/10;

        if ( get_skill(victim,gsn_enhanced_damage) > 0 )
        {
            diceroll = number_percent();
            if (diceroll <= get_skill(victim,gsn_enhanced_damage))
            {
                check_improve(victim,ch,gsn_enhanced_damage,TRUE,6);
                dam += number_fuzzy((dam * (get_skill(victim,gsn_enhanced_damage)))/300);
            }
        }

	dam += victim->damroll;

	damage(victim, ch,  dam, gsn_form_of_the_rose,obj->value[3],TRUE);
    }

    return TRUE;
}

bool check_shamanicward( CHAR_DATA *ch, CHAR_DATA *victim)
{
    int skill = get_skill(victim,gsn_shamanicward);
    if (skill == 0)
        return false;

    skill = skill / 3;

    skill = victim->level - ch->level;

    if (number_percent() > skill)
	return FALSE;

    switch(victim->in_room->sector_type)
    {
	case SECT_INSIDE: return FALSE;
	case SECT_CITY: 
	    act("You call upon nature to block $N's attack.",victim,NULL,ch,TO_CHAR);
	    act("$n calls upon nature to block $N's attack.",victim,NULL,ch,TO_NOTVICT);
	    act("$n calls upon nature to block your attack.",victim,NULL,ch,TO_VICT);
	    break;
	case SECT_FIELD:
	    act("You call upon nature to block $N's attack.",victim,NULL,ch,TO_CHAR);
	    act("$n calls upon nature to block $N's attack.",victim,NULL,ch,TO_NOTVICT);
	    act("$n calls upon nature to block your attack.",victim,NULL,ch,TO_VICT);
	    break;
	case SECT_FOREST: 
	    act("You call upon nature to block $N's attack.",victim,NULL,ch,TO_CHAR);
	    act("$n calls upon nature to block $N's attack.",victim,NULL,ch,TO_NOTVICT);
	    act("$n calls upon nature to block your attack.",victim,NULL,ch,TO_VICT);
	    break;
	case SECT_HILLS: 
	    act("You call upon nature to block $N's attack.",victim,NULL,ch,TO_CHAR);
	    act("$n calls upon nature to block $N's attack.",victim,NULL,ch,TO_NOTVICT);
	    act("$n calls upon nature to block your attack.",victim,NULL,ch,TO_VICT);
	    break;
	case SECT_MOUNTAIN: 
	    act("You call upon nature to block $N's attack.",victim,NULL,ch,TO_CHAR);
	    act("$n calls upon nature to block $N's attack.",victim,NULL,ch,TO_NOTVICT);
	    act("$n calls upon nature to block your attack.",victim,NULL,ch,TO_VICT);
	    break;
	case SECT_WATER_SWIM:
	case SECT_WATER_NOSWIM: 
	    act("You call upon nature to block $N's attack.",victim,NULL,ch,TO_CHAR);
	    act("$n calls upon nature to block $N's attack.",victim,NULL,ch,TO_NOTVICT);
	    act("$n calls upon nature to block your attack.",victim,NULL,ch,TO_VICT);
	    break;
	case SECT_AIR: 
	    act("You call upon nature to block $N's attack.",victim,NULL,ch,TO_CHAR);
	    act("$n calls upon nature to block $N's attack.",victim,NULL,ch,TO_NOTVICT);
	    act("$n calls upon nature to block your attack.",victim,NULL,ch,TO_VICT);
	    break;
	case SECT_DESERT: 
	    act("You call upon nature to block $N's attack.",victim,NULL,ch,TO_CHAR);
	    act("$n calls upon nature to block $N's attack.",victim,NULL,ch,TO_NOTVICT);
	    act("$n calls upon nature to block your attack.",victim,NULL,ch,TO_VICT);
	    break;
	case SECT_UNDERWATER: 
	    act("You call upon nature to block $N's attack.",victim,NULL,ch,TO_CHAR);
	    act("$n calls upon nature to block $N's attack.",victim,NULL,ch,TO_NOTVICT);
	    act("$n calls upon nature to block your attack.",victim,NULL,ch,TO_VICT);
	    break;
	case SECT_UNDERGROUND: 
	    act("You call upon nature to block $N's attack.",victim,NULL,ch,TO_CHAR);
	    act("$n calls upon nature to block $N's attack.",victim,NULL,ch,TO_NOTVICT);
	    act("$n calls upon nature to block your attack.",victim,NULL,ch,TO_VICT);
	    break;
	case SECT_ROAD: 
	    act("You call upon nature to block $N's attack.",victim,NULL,ch,TO_CHAR);
	    act("$n calls upon nature to block $N's attack.",victim,NULL,ch,TO_NOTVICT);
	    act("$n calls upon nature to block your attack.",victim,NULL,ch,TO_VICT);
	    break;
	case SECT_SWAMP: 
	    act("You call upon nature to block $N's attack.",victim,NULL,ch,TO_CHAR);
	    act("$n calls upon nature to block $N's attack.",victim,NULL,ch,TO_NOTVICT);
	    act("$n calls upon nature to block your attack.",victim,NULL,ch,TO_VICT);
	    break;
	default: //should never happen
	    act("You call upon nature to block $N's attack.",victim,NULL,ch,TO_CHAR);
	    act("$n calls upon nature to block $N's attack.",victim,NULL,ch,TO_NOTVICT);
	    act("$n calls upon nature to block your attack.",victim,NULL,ch,TO_VICT);
	    break;
	
    }

    return TRUE;
}

bool check_divineshield( CHAR_DATA *ch, CHAR_DATA *victim )
{
    AFFECT_DATA *shield;

    if ((shield = affect_find(victim->affected, gsn_divineshield)) == NULL)
	return FALSE;

    if (number_percent() > shield->modifier)
	return FALSE;

    if (IS_GOOD(victim))
    {
	act("A shield of light shimmers momentarily, deflecting $n's attack!", ch, NULL, victim, TO_VICT);
	act("A shield of light shimmers around $N, deflecting your attack!", ch, NULL, victim, TO_CHAR);
    }
    else if (IS_EVIL(victim))
    {
	act("Dark energy crackles around you, deflecting $n's attack!", ch, NULL, victim, TO_VICT);
	act("Dark energy crackles around $N, deflecting your attack!", ch, NULL, victim, TO_CHAR);
    }
    else
    {
	act("An unseen force knocks $n's blow aside!", ch, NULL, victim, TO_VICT);
	act("An unseen force knocks your blow aside!", ch, NULL, victim, TO_CHAR);
    }

    return TRUE;
}

bool check_parry( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj )
{
    OBJ_DATA *cObj, *vObj;
    int chance, roll;
    int skill;
    int hitmod;

    if (!IS_AWAKE(victim))
	return FALSE;
    
    if (is_affected(victim, gsn_form_of_the_bear))
	return FALSE;

    if (is_affected(victim, gsn_form_of_the_dragon) && number_bits(2) != 0)
	return FALSE;
   
    // Cannot parry while under the effects of aspect of the inferno
    AFFECT_DATA * paf(get_affect(victim, gsn_aspectoftheinferno));
    if (paf != NULL && paf->modifier == 0)
        return false;
 
    // Check for focus fury
    paf = get_affect(ch, gsn_focusfury);
    if (paf != NULL && paf->modifier == 0)
    {
        paf->modifier = 1;
        return false;
    }
     
    chance = get_skill(victim,gsn_parry) * 2 / 5;
    
    if ( get_eq_char( victim, WEAR_WIELD ) == NULL )
	if (IS_NPC(victim))
	    chance /= 2;
	else
	    return FALSE; 

    hitmod = GET_HITROLL(ch);
    if (hitmod >= 75)
        hitmod = ((hitmod-75)/20) + 75;
    hitmod = hitmod / 7;
    chance -= hitmod;
    
    if (is_affected(victim, gsn_winterwind))
        chance -= (4 * get_modifier(victim->affected, gsn_winterwind));

    vObj = get_eq_char( victim, WEAR_WIELD );

    if (vObj != NULL)
    {
        skill = get_weapon_skill_weapon(victim,vObj);
        chance = ((chance *(skill+100))/200);
        switch (vObj->value[0])
        {	
            case WEAPON_STAFF: 		chance = round(chance * 1.7); 	break; 
            case WEAPON_POLEARM: 	chance = round(chance * 1.6); 	break;
            case WEAPON_SPEAR: 		chance = round(chance * 1.4);	break;
            case WEAPON_AXE: 		chance = round(chance * 0.8); 	break;
            case WEAPON_MACE: 		chance = round(chance * 0.8);	break;
            case WEAPON_DAGGER: 	chance = round(chance * 0.4);	break;
            case WEAPON_KNIFE: 		chance = round(chance * 0.4);	break;
            case WEAPON_WHIP: 		chance = round(chance * 0.3); 	break;
            case WEAPON_FLAIL: 		chance = round(chance * 0.4);	break;
            case WEAPON_EXOTIC:
            {
            if (victim->class_num == global_int_class_gladiator)
                chance = round(chance * 1.3);
            break;
            }
            default: 							break;
        }
        if (vObj->pIndexData->vnum == OBJ_VNUM_HEIRLOOM)
            chance = round(chance * 1.1);

        // 10% bonus for having a weapon forged for parrying
        chance = (chance * (100 + static_cast<int>(Forge::TraitCount(*vObj, Forge::Trait_Parry) * 10))) / 100;
    }

    cObj = get_eq_char(victim, WEAR_DUAL_WIELD);

    if (cObj != NULL)
    {
        switch (cObj->value[0])
	{	
	    case WEAPON_SWORD: 	chance = round(chance * 1.1); 	break;
	    case WEAPON_MACE:
	    case WEAPON_AXE: 	chance = round(chance / 1.1); 	break;
	    case WEAPON_FLAIL:  chance = round(chance / 1.2);	break;
	    case WEAPON_WHIP:
	    {
		int wmsk = get_skill(victim,gsn_whipmastery);
		if (wmsk > 0)
		    if (number_percent() < wmsk)
		    {
			check_improve(victim,ch,gsn_whipmastery,TRUE,1);
		    }
		    else
		    {
			chance = round(chance / 1.2);
		    }
		break;
	    }
	    case WEAPON_EXOTIC:
		if (victim->class_num == global_int_class_gladiator)
                    chance = round(chance * 1.1);
		break;
	    default:				break;
	}
	if (cObj->pIndexData->vnum == OBJ_VNUM_HEIRLOOM)
	    chance = round(chance * 1.1);
    }

    if (obj != NULL)
    {
        switch (obj->value[0])
        {	
            case -1: break;
            case WEAPON_KNIFE:
            case WEAPON_WHIP:
            case WEAPON_DAGGER:	chance = round(chance / 1.2);	break;
            case WEAPON_SPEAR: 	chance = round(chance * 1.1);	break;
            case WEAPON_MACE: 	
            case WEAPON_AXE:	
            case WEAPON_FLAIL: 	chance = round(chance / 1.1);	break;
            case WEAPON_POLEARM:	chance = round(chance * 1.25);	break; 
            case WEAPON_EXOTIC:
                if (victim->class_num == global_int_class_gladiator)
                chance = round(chance / 1.1);	
                break;
            default: 				break;
        }

        // Attacker's blessed weapons make parrying harder for demons
        if (IS_OBJ_STAT(obj, ITEM_BLESS) && is_demon(victim))
            chance = (chance * 19) / 20;

        // Attacker's hierlooms reduce the odds of parrying
        if (obj->pIndexData->vnum == OBJ_VNUM_HEIRLOOM)
            chance = (chance * 9) / 10;

        // Check for baneblade
        if (check_baneblade_modify_parry(*obj))
            chance = (chance * 9) / 10;
    }

    if (!can_see(victim, ch) && !is_blindfighter(victim,TRUE))
	chance /= 2;

    if (is_affected(ch, gsn_form_of_the_dragon))
	chance /= 2;

    if (is_affected(victim, gsn_form_of_the_reed))
	chance *= 2;

    if (is_affected(victim, gsn_form_of_the_monkey))
	chance = round(chance * 1.3);

    if (is_affected(victim, gsn_levitation))
	chance /= 2;

    if (victim->class_num == CLASS_FIGHTER)
	chance = round(chance * 1.2);

    if (victim->in_room && victim->in_room->sector_type == SECT_UNDERWATER && !is_affected(victim, gsn_aquamove))
    {
        if (number_percent() <= get_skill(victim, gsn_waveborne))
            check_improve(victim, ch, gsn_waveborne, true, 4);
        else
        {
            check_improve(victim, ch, gsn_waveborne, false, 4);
	        chance = round(chance * 0.85);
        }
    }

    if (IS_NAFFECTED(ch,AFF_BLINK))
	chance = round(chance * .85);

    chance += victim->level - ch->level;

    whinyassbitch(ch,victim,"Parry",chance);
    
    chance = UMIN(95,chance);
    if ((roll = number_percent()) >= chance)
	return FALSE;

    if (vObj && obj_is_affected(vObj, gsn_cursebarkja) && (number_percent() < 26))
    {
        act("You move to parry $n's blow, but your weapon jerks out of its path!", ch, NULL, victim, TO_VICT);
        return FALSE;
    }
    
    if (roll <= 5)
    {
	if (number_bits(1) == 0)
	{
	    act("You frantically parry $n's attack.", ch, NULL, victim, TO_VICT);
	    act("$N frantically parries your attack.", ch, NULL, victim, TO_CHAR);
	}
	else
	{
	    act("You barely manage to parry $n's attack.", ch, NULL, victim, TO_VICT);
	    act("$N barely manages to parry your attack.", ch, NULL, victim, TO_CHAR);
	}
    }
    else if (roll > (chance - 5))
    {
	int echo = number_range(2, 6);

	switch (echo)
	{
	    case 2:
		act("You cleanly parry $n's attack.", ch, NULL, victim, TO_VICT);
		act("$N cleanly parries your attack.", ch, NULL, victim, TO_CHAR);
		break;

	    case 3:
		act("You deftly parry $n's attack.", ch, NULL, victim, TO_VICT);
		act("$N deftly parries your attack.", ch, NULL, victim, TO_CHAR);
		break;

	    case 4:
		act("You skillfully parry $n's attack.", ch, NULL, victim, TO_VICT);
		act("$N skillfully parries your attack.", ch, NULL, victim, TO_CHAR);
		break;

	    case 5:
		act("You parry $n's attack with casual grace.", ch, NULL, victim, TO_VICT);
		act("$N parries your attack with casual grace.", ch, NULL, victim, TO_CHAR);
		break;

	    case 6:
		act("You easily parry $n's attack.", ch, NULL, victim, TO_VICT);
		act("$N easily parries your attack.", ch, NULL, victim, TO_CHAR);
		break;
	}
    }
    else
    {
        act( "You parry $n's attack.", ch, NULL, victim, TO_VICT    );
        act( "$N parries your attack.", ch, NULL, victim, TO_CHAR    );
    }

    check_improve(victim,ch,gsn_parry,TRUE,4);

    check_enthusiasm(ch,victim); 


    int dam;
    if (get_eq_char(victim,WEAR_DUAL_WIELD) != NULL)
        return TRUE;

    if (vObj == NULL)
        return TRUE;

    if (!IS_SET(vObj->value[4], WEAPON_TWO_HANDS) && get_eq_char(victim,WEAR_SHIELD) == NULL)
        return TRUE;        

    // Check for counterattack
    bool shouldCounter(number_percent() < get_skill(victim, gsn_counterattack) / 5 && victim->class_num == global_int_class_fighter);

    // Check for ethereal brethren
    AFFECT_DATA * brethren(get_affect(victim, gsn_etherealbrethren));
    if (brethren != NULL && number_percent() <= 15 && brethren->duration >= 1)
    {
        brethren->duration -= 1;
        send_to_char("You sense a memory not your own guide your hands into a counterattack!\n", victim);
        shouldCounter = true;
    }

    if (shouldCounter)
    {
        if (vObj->pIndexData->new_format)
            dam = dice(vObj->value[1],vObj->value[2]);
        else
            dam = number_range( vObj->value[1],  vObj->value[2]);

        dam = dam * 11/10;
        dam += victim->damroll/2;
        if (IS_SET(vObj->value[4], WEAPON_TWO_HANDS))
            dam = dam * 5/4;

        damage(victim, ch,  dam, gsn_counterattack,vObj->value[3],TRUE);
        check_improve(victim,ch,gsn_counterattack,TRUE,5);
    }

    if (is_affected(victim, gsn_form_of_the_rose)
     && number_percent() < (skill = get_skill(victim, gsn_form_of_the_rose))/3)
    {
	OBJ_DATA *wield;
	int dam, diceroll;
	wield = get_eq_char(ch, WEAR_WIELD);
	
	if (wield == NULL)
	    return TRUE;

        if (wield->pIndexData->new_format)
            dam = dice(wield->value[1],wield->value[2]);
        else
            dam = number_range( wield->value[1],  wield->value[2]);

	dam = (dam * skill)/100;

            dam = dam * 11/10;

        if (get_skill(victim,gsn_enhanced_damage) > 0 )
        {
            diceroll = number_percent();
            if (diceroll <= get_skill(victim,gsn_enhanced_damage))
            {
                check_improve(victim,ch,gsn_enhanced_damage,TRUE,6);
                dam += number_fuzzy((dam * (get_skill(victim,gsn_enhanced_damage)))/300);
            }
        }

	dam += victim->damroll;

	damage(victim, ch,  dam, gsn_form_of_the_rose,wield->value[3],TRUE);
    }
    return TRUE;
}

bool check_showmanship( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj )
{
    OBJ_DATA *pObj;
    int chance;

    if ( !IS_AWAKE(victim) )
        return FALSE;

    pObj = get_eq_char(victim, WEAR_ARMS);

    if (!pObj)
	return FALSE;

    if (pObj->item_type != ITEM_ARMOR)
	return FALSE;

    chance = (get_skill(victim,gsn_showmanship) / 3) + 3;

    if (chance == 3)
	return FALSE;

    whinyassbitch(ch,victim,"Showmanship",chance);

    if (obj != NULL && obj->pIndexData->value[0] == WEAPON_EXOTIC)
	if (ch->class_num == global_int_class_gladiator)
	    chance = round(chance / 1.1);

    if (obj != NULL && obj->pIndexData->vnum == OBJ_VNUM_HEIRLOOM)
	chance = round(chance / 1.1);

    if ( number_percent( ) >= chance + victim->level - ch->level )
        return FALSE;

    act( "You turn slightly, taking $n's attack on $p.",
	ch, pObj, victim, TO_VICT);
    act( "$N turns slightly, taking your attack on $p.",
	ch, pObj, victim, TO_CHAR);
    check_improve(victim,ch,gsn_showmanship,TRUE,3);

    check_enthusiasm(ch,victim);

    return TRUE;
}

bool check_savagery( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj, 
		     int dam_type )
{
    int chance;

    if ( !IS_AWAKE(victim) )
        return FALSE;
    if (!is_affected(victim, gsn_savagery))
	return FALSE;

    chance = (get_skill(victim,gsn_savagery) / 3) + 3;

    if (chance == 3)
	return FALSE;

    whinyassbitch(ch,victim,"Savagery",chance);

    if (obj != NULL && obj->pIndexData->value[0] == WEAPON_EXOTIC)
	if (ch->class_num == global_int_class_gladiator)
	    chance = round(chance / 1.1);

    if (obj != NULL && obj->pIndexData->vnum == OBJ_VNUM_HEIRLOOM)
	chance = round(chance / 1.1);

    if (!(dam_type == TYPE_HIT+DAM_BASH 
      || dam_type == TYPE_HIT+DAM_PIERCE 
      || dam_type == TYPE_HIT+DAM_SLASH
      || dam_type == DAM_BASH
      || dam_type == DAM_PIERCE
      || dam_type == DAM_SLASH))
	chance = round(chance * 3/4);

    if ( number_percent( ) >= chance + victim->level - ch->level )
        return FALSE;

    act( "You shrug off $n's attack, ignoring the blow.",
      ch, NULL, victim, TO_VICT);
    act( "$N shrugs off your attack, ignoring the blow.",
      ch, NULL, victim, TO_CHAR);

    check_improve(victim,ch,gsn_savagery,TRUE,3);

    check_enthusiasm(ch,victim);

    return TRUE;
}

bool check_deflect( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj )
{
    OBJ_DATA *pObj;
    int chance;

    if ( !IS_AWAKE(victim) )
        return FALSE;

    pObj = get_eq_char(victim, WEAR_WIELD);

    if (!pObj)
	return FALSE;

    if (pObj->value[0] != WEAPON_STAFF)
	return FALSE;

    // Check for ethereal brethren
    AFFECT_DATA * brethren(get_affect(victim, gsn_etherealbrethren));
    if (brethren != NULL && number_percent() <= 15 && brethren->duration >= 1)
    {
        brethren->duration -= 1;
        act("Your staff moves from muscle memory you never made, deflecting $n's blow!", ch, NULL, victim, TO_VICT);
        act("$N deflects your attack with a twirl of $p.",	ch, pObj, victim, TO_CHAR);
        return true;
    }

    chance = (get_skill(victim,gsn_deflect) * 4 / 10) + 3;

    if (chance == 3)
	return FALSE;

    whinyassbitch(ch,victim,"Deflect",chance);

    if (obj != NULL)
    switch (obj->value[0])
    {	
	case -1:				break;
	case WEAPON_SPEAR: 	chance = round(chance * 5/4); 	break;
	case WEAPON_AXE:
	case WEAPON_MACE:
	case WEAPON_FLAIL:
	case WEAPON_WHIP: 	chance = round(chance / 1.2); 	break;
	case WEAPON_POLEARM: 	
	case WEAPON_STAFF: 	chance = round(chance * 5/3); 	break;
	case WEAPON_KNIFE:
	case WEAPON_DAGGER: 	chance = round(chance / 1.1); 	break;
	case WEAPON_EXOTIC:
	    if (ch->class_num == global_int_class_gladiator)
		chance = round(chance / 1.1);
	default: 				break;
    }
    if (obj != NULL && obj->pIndexData->vnum == OBJ_VNUM_HEIRLOOM)
	chance = round(chance / 1.1);

    if ( number_percent( ) >= chance + victim->level - ch->level )
        return FALSE;

    act( "You deflect $n's attack with a twirl of $p.", ch, pObj, victim, TO_VICT);
    act( "$N deflects your attack with a twirl of $p.",	ch, pObj, victim, TO_CHAR);
    check_improve(victim,ch,gsn_deflect,TRUE,5);

    check_enthusiasm(ch,victim);

    return TRUE;
}

bool check_fend( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj )
{
    OBJ_DATA *pObj;
    int chance;
    int hitmod;

    if (!IS_AWAKE(victim))
        return FALSE;

    pObj = get_eq_char(victim, WEAR_WIELD);

    if (!pObj)
	return FALSE;

    if (pObj->value[0] != WEAPON_SPEAR && pObj->value[0] != WEAPON_POLEARM)
	return FALSE;

    chance = get_skill(victim,gsn_fend) / 3 + 3;

    if (chance == 3)
	return FALSE;
    hitmod = GET_HITROLL(ch);

    if (hitmod >= 75)
        hitmod = ((hitmod-75)/20) + 75;

    hitmod = hitmod / 7;
    chance -= hitmod;

    if (obj != NULL)
    switch (obj->value[0])
    {	
	case -1: 					break;
	case WEAPON_SPEAR:	chance = round(chance / 1.2); 		break;
	case WEAPON_POLEARM:	
	case WEAPON_WHIP:	chance = round(chance / 1.4);		break;
	case WEAPON_AXE:	
	case WEAPON_STAFF: 	chance = round(chance * 1.25); 		break;
	case WEAPON_DAGGER: 	
	case WEAPON_KNIFE: 	chance = round(chance * 5/3); 		break;
	case WEAPON_EXOTIC:	
	    if (ch->class_num == global_int_class_gladiator)
		chance = round(chance / 1.1);
	default: 					break;
    }
    if (obj != NULL && obj->pIndexData->vnum == OBJ_VNUM_HEIRLOOM)
	chance = round(chance / 1.1);

    chance += victim->level - ch->level;

    whinyassbitch(ch,victim,"Fend",chance);
    if ( number_percent( ) >= chance )
        return FALSE;

    act("You fend off $n's attack with your weapon.",
	ch, NULL, victim, TO_VICT);
    act("$N fends off your attack with $S weapon.",
	ch, NULL, victim, TO_CHAR);
    check_improve(victim,ch,gsn_fend,TRUE,5);

    check_enthusiasm(ch, victim);

    if (number_percent() < get_skill(victim, gsn_counterattack) / 5)
    {
	OBJ_DATA *wield;
	int dam;
	wield = get_eq_char(victim, WEAR_WIELD);
	
	if (get_eq_char(victim,WEAR_DUAL_WIELD) != NULL)
	    return TRUE;
	
	if (wield == NULL)
	    return TRUE;

        if (wield->pIndexData->new_format)
            dam = dice(wield->value[1],wield->value[2]);
        else
            dam = number_range( wield->value[1],  wield->value[2]);

            dam = dam * 11/10;

	dam += victim->damroll/2;
	if (IS_WEAPON_STAT(wield,WEAPON_TWO_HANDS))
	    dam *= 5/4;

	damage(victim, ch,  dam, gsn_counterattack,wield->value[3],TRUE);
        check_improve(victim,ch,gsn_counterattack,TRUE,5);
    }

    return TRUE;
}


/*
 * Check for shield block.
 */
bool check_shield_block( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj, int dam )
{
    OBJ_DATA *pObj, *shield;
    int chance;

    if ( !IS_AWAKE(victim) )
        return FALSE;

    chance = get_skill(victim,gsn_shield_block) * 9 / 20;

    if (ch->fighting != victim)
    {
	if (!IS_NPC(ch) && !IS_NPC(victim) && !IS_PK(ch, victim))
	    return FALSE;

	if ((victim->class_num == global_int_class_fighter) 
	  || (IS_NPC(victim) 
	  && !IS_OAFFECTED(victim,AFF_GHOST)
	  && victim->pIndexData->vnum == MOB_VNUM_GREATER_SYNDIC))
	    chance = (chance * get_skill(victim, gsn_phalanx))/100;
	else if (victim->class_num == global_int_class_watertemplar)
	    if (is_affected(victim, gsn_shieldcover) 
	      && !IS_OAFFECTED(victim, AFF_GHOST))
		chance = (chance * get_skill(victim, gsn_shieldcover)) / 115;
	    else
		return FALSE;
	else
	    return FALSE;
    }

    if ((shield = get_eq_char(victim, WEAR_SHIELD)) == NULL)
        return FALSE;

    pObj = (get_eq_char(ch, WEAR_WIELD ) );
    if (obj != NULL)
    switch (obj->value[0])
    {	
        case -1: 				break;
        case WEAPON_KNIFE:
        case WEAPON_DAGGER: 	chance = round(chance * 1.25); 	break;

        case WEAPON_FLAIL:
        case WEAPON_MACE: 	
            if (is_affected(victim, gsn_roaroftheexalted) || !obj_is_affected(shield,gsn_mireofoame)) 
                chance = round(chance / 1.4); 	
            break;

        case WEAPON_WHIP:
        case WEAPON_AXE:	
            if (is_affected(victim, gsn_roaroftheexalted) || !obj_is_affected(shield,gsn_mireofoame)) 
                chance = round(chance / 1.2);   
            break;

        case WEAPON_EXOTIC:
            if (ch->class_num == global_int_class_gladiator)
            chance = round(chance / 1.1);
            break;
        default: 				break;
    }

    if (obj != NULL && obj->pIndexData->vnum == OBJ_VNUM_HEIRLOOM)
	chance = round(chance / 1.1);

    if (IS_NAFFECTED(victim,AFF_ARMSHATTER))
	chance /= 2;
    
    chance += victim->level - ch->level;
    
    whinyassbitch(ch,victim,"Shield block", chance);

    if ( number_percent() >= chance )
        return FALSE;

    if (ch->fighting == victim)
    {
        act( "You block $n's attack with your shield.", 
		ch, NULL, victim, TO_VICT);
        act( "$N blocks your attack with $S shield.", 
		ch, NULL, victim, TO_CHAR);
        check_improve(victim,ch,gsn_shield_block,TRUE,3);
    }
    else
    {
	if (victim->class_num == global_int_class_fighter)
	{
	    check_improve(victim,ch,gsn_phalanx,TRUE,4);
	    act("You use your formation to block $n's attack with your shield.",
		    ch, NULL, victim, TO_VICT);
	    act("$N uses $S formation to block your attack with $S shield.",
		    ch, NULL, victim, TO_CHAR);
            act( "$N uses $S formation to block $n's attack with $S shield.", 
	   	    ch, NULL, victim, TO_NOTVICT);
	}
	else if (victim->class_num == global_int_class_watertemplar)
	{
	    check_improve(victim, ch,gsn_shieldcover, TRUE, 3);
	    if (number_bits(1) == 0)
	    {
		act("You throw up a quick wall of ice to block $n's attack.",
			ch, NULL, victim, TO_VICT);
		act("$N throws up a quick wall of ice to block your attack.",
			ch, NULL, victim, TO_CHAR);
		act("$N throws up a quick wall of ice to block $n's attack.",
			ch, NULL, victim, TO_NOTVICT);
		if (victim->mana == 0)
		{
		    send_to_char("You are too tired to continue covering your group with your shield.\n\r", victim);
		    act("$n stops covering $s group with $s shield.", victim, NULL, NULL, TO_ROOM);
		    affect_strip(victim, gsn_shieldcover);
		}
	    }
	    else
	    {
		act("You cover your groupmate from $n's attack with your shield.", ch, NULL, victim, TO_VICT);
		act("$N covers $S groupmate from your attack with $S shield.",
			ch, NULL, victim, TO_CHAR);
	        act("$N covers $S groupmate from $n's attack with $S shield.",
		        ch, NULL, victim, TO_NOTVICT);
	    }
	}	
    }

    if (!pObj && obj_is_affected(shield, gsn_jawsofidcizon) && !is_affected(victim, gsn_roaroftheexalted))
    {
	act("The jaws on $N's shield bite at you!", ch, NULL, victim, TO_CHAR);
	act("The jaws on your shield bite at $n!", ch, NULL, victim, TO_VICT);
	act("The jaws on $N's shield bite at $n.", ch, NULL, victim, TO_NOTVICT);
	damage(victim, ch, dam / 5, gsn_jawsofidcizon, DAM_PIERCE, TRUE);
    }

    if (pObj && obj_is_affected(shield, gsn_jawsofidcizon) && (number_bits(5) == 0) && !is_affected(victim, gsn_roaroftheexalted))
    {
	act("Jaws reach out from $N's shield, grabbing your weapon!", ch, NULL, victim, TO_CHAR);
	act("Jaws reach out from $N's shield, grabbing $n's weapon!", ch, NULL, victim, TO_NOTVICT);
	act("Jaws reach out from your shield, grabbing $n's weapon!", ch, NULL, victim, TO_VICT);
	disarm(victim, ch);
    }

    if (shield->pIndexData->vnum == OBJ_VNUM_LIFE_SHIELD)
    {
	int heal = (dam * 2 / 5);
	CHAR_DATA *vch;

	if (heal > 0)
    {
	    for (vch = victim->in_room->people; vch; vch = vch->next_in_room)
        {
		    if (is_same_group(vch, victim))
            {
                if (is_an_avatar(vch))
                    continue;
                
                if (vch == victim)
                    send_to_char("The healing power of your life shield flows through you.\n\r", vch);
                else
                    act("The healing power of $n's life shield flows through you.", victim, NULL, vch, TO_VICT);
        
                vch->hit = UMIN(vch->hit + heal, vch->max_hit);
            }
        }
    }

	shield->timer -= dam;
	if (shield->timer <= 0)
	{
	    act("Your $p flashes bright and disappears!", victim, shield, NULL, TO_CHAR);
	    act("$n's $p flashes bright and disappears!", victim, shield, NULL, TO_ROOM);
	    extract_obj(shield);
	}
    }
    check_enthusiasm(ch,victim);

    return TRUE;
}

bool check_stonephalanx(CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj)
{
    OBJ_DATA *wield;
    int chance;
    int hitmod;
    bool fake = FALSE;

    if (!IS_AWAKE(victim))
	return FALSE;

    if ((chance = get_skill(victim,gsn_stonephalanx)) == 0)
	return FALSE;

    if ((wield = get_eq_char(victim, WEAR_WIELD)) == NULL)
	return FALSE;

    if (!IS_WEAPON_STAT(wield, WEAPON_TWO_HANDS))
	if (IS_PAFFECTED(victim, AFF_TWOHAND))
	    fake = TRUE;
	else
	    return FALSE;

    if (!victim->in_room
     || (victim->in_room->sector_type == SECT_WATER_SWIM)
     || (victim->in_room->sector_type == SECT_WATER_NOSWIM)
     || (victim->in_room->sector_type == SECT_AIR)
     || (victim->in_room->sector_type == SECT_UNDERWATER))
	return FALSE;

    chance = chance / 2;
    
    hitmod = GET_HITROLL(ch);

    if (hitmod >= 75)
        hitmod = ((hitmod-75)/20) + 75;

    hitmod = hitmod / 9;
    chance -= hitmod;
    
    chance = round(chance * (get_weapon_skill_weapon(victim,wield)+200)/300);

    if (obj != NULL)
    switch (obj->value[0])
    {	
	case -1: 				break;
	case WEAPON_KNIFE:
	case WEAPON_DAGGER: 	chance = round(chance / 5/3); 	break;
	case WEAPON_SPEAR: 	chance = round(chance / 1.2); 	break;
	case WEAPON_FLAIL:
	case WEAPON_MACE: 	chance = round(chance * 1.1); 	break;
	case WEAPON_STAFF:
	case WEAPON_AXE: 	chance = round(chance * 1.25); 	break;
	case WEAPON_POLEARM:
	case WEAPON_WHIP: 	chance = round(chance / 1.4); 	break;
	case WEAPON_EXOTIC:
	    if (ch->class_num == global_int_class_gladiator)
		chance = round(chance / 1.1);
	    break;
	default: 				break;
    }
    if (obj != NULL && obj->pIndexData->vnum == OBJ_VNUM_HEIRLOOM)
	chance = round(chance / 1.1);

    if (fake)
	chance = round(chance * 3/4);
    chance += victim->level - ch->level;
    
    if (fake)
	whinyassbitch(ch,victim,"Stone phalanx (one-handed weapon)",chance);
    else
	whinyassbitch(ch,victim,"Stone phalanx (two-handed weapon)",chance);
	
    if ( number_percent() >= chance)
        return FALSE;

    act("You raise a block of stone from the earth to stop $n's attack.",
	ch, NULL, victim, TO_VICT);
    act("A block of stone rises from the earth to stop your attack.",
	ch, NULL, victim, TO_CHAR);
    check_improve(victim, ch,gsn_stonephalanx, TRUE, 3);

    check_enthusiasm(ch,victim);

    if (number_percent() < get_skill(victim, gsn_counterattack) / 3)
    {
	OBJ_DATA *wield;
	int dam;
	wield = get_eq_char(victim, WEAR_WIELD);
	
	if (wield == NULL)
	    return TRUE;

        if (wield->pIndexData->new_format)
            dam = dice(wield->value[1],wield->value[2]);
        else
            dam = number_range( wield->value[1],  wield->value[2]);

            dam = dam * 11/10;

	dam += victim->damroll/2;

	damage(victim, ch,  dam*5/4, gsn_counterattack,wield->value[3],TRUE);
        check_improve(victim,ch,gsn_counterattack,TRUE,5);
	uberskillgetsuberer(victim,ch,HIT_PRIMARY,wield);
    }

    return TRUE;
}

    

bool is_blindfighter(CHAR_DATA *ch, bool sword)
{
    int chance = get_skill(ch,gsn_blindfighting);
    if (chance == 0)
	if (ch->class_num == CLASS_SWORDMASTER && sword // I would hope so
	  && (chance = get_weapon_skill(ch,gsn_sword,TRUE) - 100) < 1)
	    chance = 0;

    if (chance == 0)
	return FALSE;

    if (number_percent() > chance)
    {
        check_improve(ch,NULL,gsn_blindfighting,FALSE,2);
        return FALSE;
    }
	
    check_improve(ch,NULL,gsn_blindfighting,TRUE,2);
    return TRUE;
}

bool check_brawlingblock( CHAR_DATA *ch, CHAR_DATA *victim )
{
    int chance;
 
    if ( !IS_AWAKE(victim) )
        return FALSE;

    if ((get_skill(victim,gsn_brawlingblock) == 0))
	return FALSE;

    int hands = 0;

    if (get_eq_char(victim, WEAR_WIELD) != NULL)
	hands++;

    if (get_eq_char(victim, WEAR_DUAL_WIELD) != NULL)
	hands++;

    if (get_eq_char(victim, WEAR_LIGHT) != NULL)
	hands++;

    if (get_eq_char(victim, WEAR_SHIELD) != NULL)
	hands++;

    if (get_eq_char(victim, WEAR_HOLD) != NULL)
	hands++;
    
    if (hands > 1)
	return FALSE;

    chance = ((30 * get_skill(victim,gsn_brawlingblock)) / 100);
    chance -= (18 - get_curr_stat(victim, STAT_DEX));

    whinyassbitch(ch,victim,"Brawling block (arms)",chance);
 
    if (number_percent() > chance)
    {
        return FALSE;
    }

    act( "You block $n's attack with your forearm.", ch, NULL, victim, TO_VICT    );
    act( "$N blocks your attack with $S forearm.", ch, NULL, victim, TO_CHAR    );
    check_improve(victim,ch,gsn_brawlingblock,TRUE,4);

    check_enthusiasm(ch,victim);

    return TRUE;
}

bool check_evade(CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj)
{
    int chance;
    int hitmod;
    int roll;

    if ( !IS_AWAKE(victim) )
	return FALSE;

    if ((chance = get_skill(victim,gsn_evade)) == 0)
	return FALSE;

    chance /= 2;

    chance += (2 * (get_curr_stat(victim, STAT_DEX) - 18));

    hitmod = GET_HITROLL(ch);

    if (hitmod >= 75)
        hitmod = ((hitmod-75)/20) + 75;

    hitmod = hitmod / 7;
    chance -= hitmod;

    if (is_affected(victim, gsn_plantentangle))
	chance = ((chance * 4) / 5);

    if (IS_NAFFECTED(victim, AFF_LEGSHATTER))
	chance /= 2;
    
    if (is_affected(victim, gsn_freeze))
	chance -= 4 * get_modifier(victim->affected, gsn_freeze);

    if (!can_see(victim,ch) && !is_blindfighter(victim, FALSE))
	chance /= 2;
    
    if (obj && obj->value[0] == WEAPON_WHIP 
      && get_skill(ch,gsn_whipmastery) > number_percent())
	chance = round(chance * 3 / 4);

    if (IS_NAFFECTED(ch,AFF_BOLO))
	chance /= 2;
    
    if (obj != NULL)
    switch (obj->value[0])
    {	
	case -1: 				break;
	case WEAPON_WHIP: 	chance = round(chance * 1.25); 	break;
	case WEAPON_EXOTIC:
	    if (ch->class_num == global_int_class_gladiator)
		chance = round(chance / 1.1);
	    break;
	default: 				break;
    }
    if (obj != NULL && obj->pIndexData->vnum == OBJ_VNUM_HEIRLOOM)
	chance = round(chance / 1.1);

    if ((victim->in_room && (victim->in_room->sector_type == SECT_UNDERWATER)) && !is_affected(victim, gsn_aquamove))
    {
        if (number_percent() <= get_skill(victim, gsn_waveborne))
            check_improve(victim, ch, gsn_waveborne, true, 4);
        else
        {
            check_improve(victim, ch, gsn_waveborne, false, 4);
	        chance = round(chance * 0.8);
        }
    }

    chance += victim->level - ch->level;

    whinyassbitch(ch,victim,"Evade",chance);

    if ((roll = number_percent()) >= chance)
        return FALSE;

    if (roll <= 5)
    {
	if (number_bits(1) == 0)
	{
	    act("You narrowly evade $n's attack.", ch, NULL, victim, TO_VICT);
	    act("$N narrowly evades your attack.", ch, NULL, victim, TO_CHAR);
	}
	else
	{
	    act("You barely evade $n's attack.", ch, NULL, victim, TO_VICT);
	    act("$N barely evades your attack.", ch, NULL, victim, TO_CHAR);
	}
    }
    else if (roll > (chance - 5))
    {
	int echo = number_range(1, 6);

	switch (echo)
	{
	    case 1:
		act("You clearly evade $n's attack.", ch, NULL, victim, TO_VICT);
		act("$N clearly evades your attack.", ch, NULL, victim, TO_CHAR);
		break;

	    case 2:
		act("You cleanly evade $n's attack.", ch, NULL, victim, TO_VICT);
		act("$N cleanly evades your attack.", ch, NULL, victim, TO_CHAR);
		break;

	    case 3:
		act("You deftly evade $n's attack.", ch, NULL, victim, TO_VICT);
		act("$N deftly evades your attack.", ch, NULL, victim, TO_CHAR);
		break;

	    case 4:
		act("You skillfully evade $n's attack.", ch, NULL, victim, TO_VICT);
		act("$N skillfully evades your attack.", ch, NULL, victim, TO_CHAR);
		break;

	    case 5:
		act("You evade $n's attack with casual grace.", ch, NULL, victim, TO_VICT);
		act("$N evades your attack with casual grace.", ch, NULL, victim, TO_CHAR);
		break;

	    case 6:
		act("You easily evade $n's attack.", ch, NULL, victim, TO_VICT);
		act("$N easily evades your attack.", ch, NULL, victim, TO_CHAR);
		break;
	}
    }
    else
    {
        act( "You evade $n's attack.", ch, NULL, victim, TO_VICT    );
        act( "$N evades your attack.", ch, NULL, victim, TO_CHAR    );
    }

    check_improve(victim,ch,gsn_evade,TRUE,5);
    check_enthusiasm(ch,victim);
    return TRUE;
}

bool check_creaturelore( CHAR_DATA *ch, CHAR_DATA *victim )
{
    int i;
    int chance = 0;

    if (IS_NPC(victim) || !IS_NPC(ch))
	return FALSE;
    if ((chance = get_skill(victim,gsn_creaturelore)) == 0)
	return FALSE;

    for (i = 0; i < MAX_CLORE; i++)
	if (victim->lcreature[i] == ch->pIndexData->vnum)
	{
	    chance = (victim->lpercent[i] * 2 / 3);
	    break;
	}

    if (chance == 0)
	return FALSE;

    chance -= (18 - get_curr_stat(victim, STAT_INT));
    chance -= (18 - get_curr_stat(victim, STAT_WIS));
    chance -= (18 - get_curr_stat(victim, STAT_DEX));
    chance += (victim->level - ch->level);

    whinyassbitch(ch,victim,"Creature Lore",chance);
    if (number_percent() < chance)
    {
	act("You skillfully avoid the blow of the creature.",
	    ch, NULL, victim, TO_VICT);
	act("$N skillfully avoids your attack.", ch, NULL, victim, TO_CHAR);
	check_enthusiasm(ch,victim);
	return TRUE;
    }
    
    return FALSE;
}

/*
 * Check for dodge.
 */
bool check_dodge( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj )
{
    int chance;
    int hitmod;
    int tskill;
    int roll = -1;

    hitmod = GET_HITROLL(ch);

    if ( !IS_AWAKE(victim) )
	return FALSE;

    if (is_affected(victim, gsn_levitation))
	return FALSE;

    chance = get_skill(victim,gsn_dodge) / 2;

    // Check for aspect of the inferno
    AFFECT_DATA * paf(get_affect(victim, gsn_aspectoftheinferno));
    if (paf != NULL && paf->modifier == 0)
        chance = UMAX(chance, UMAX(75, get_skill(victim, gsn_aspectoftheinferno)) / 2);

    // Check for ethereal brethren
    paf = get_affect(victim, gsn_etherealbrethren);
    if (paf != NULL && number_percent() <= 15 && paf->duration >= 1)
    {
        paf->duration -= 1;
        act("You sense an experience not your own guide your feet as you leap clear of $n's blow!", ch, NULL, victim, TO_VICT);
        act("$N dodges your attack.", ch, NULL, victim, TO_CHAR);
        return true;
    }

    chance += (get_curr_stat(victim, STAT_DEX) - 18);

/*  Hitroll makes it harder to dodge. */
    if (hitmod >= 75)
        hitmod = ((hitmod-75)/20) + 75;

    hitmod = hitmod / 5;
    chance -= hitmod;

    if (is_affected(victim, gsn_plantentangle))
	chance = ((chance * 4) / 5);

    if (!IS_NPC(victim) && IS_AFFECTED(victim, AFF_HASTE))
	chance = ((chance * 5) / 4);
	
    if (IS_AFFECTED(victim, AFF_SLOW))
	chance = ((chance * 4) / 5);
	
    if (IS_NAFFECTED(victim, AFF_LEGSHATTER))
	chance /= 2;

    if (is_affected(victim, gsn_freeze))
	chance -= 4 * get_modifier(victim->affected, gsn_freeze);

    if (is_affected(victim, gsn_form_of_the_serpent))
	chance = round(chance*1.3);

    if (is_affected(victim, gsn_form_of_the_zephyr))
	chance = round(chance*1.2);

    if (!can_see(victim,ch) && !is_blindfighter(victim,FALSE))
	chance /= 2;

    if (obj && obj->value[0] == WEAPON_WHIP 
      && get_skill(ch,gsn_whipmastery) > number_percent())
	chance = round(chance * 3 / 4);

    if (IS_NAFFECTED(victim,AFF_BOLO))
	chance /= 2;
	
    if (obj != NULL)
    {
        switch (obj->value[0])
        {	
            case -1: 				break;
            case WEAPON_SPEAR:
            case WEAPON_AXE:
            case WEAPON_STAFF:	chance = round(chance / 1.1);	break;
            case WEAPON_FLAIL:
            case WEAPON_POLEARM: 	chance = round(chance / 1.2); 	break;
            case WEAPON_WHIP: 	
                if (get_skill(victim,gsn_whipmastery) > number_percent())
                chance = round(chance * 1.35);
                chance = round(chance * 1.25); 	
                break;
            case WEAPON_EXOTIC:
                if (ch->class_num == global_int_class_gladiator)
                chance = round(chance / 1.1);
                break;
            default: 				break;
        }

        // Attacker's blessed weapons make dodging harder for demons
        if (IS_OBJ_STAT(obj, ITEM_BLESS) && is_demon(victim))
            chance = (chance * 19) / 20;

        // Attacker's hierlooms make dodging harder
        if (obj->pIndexData->vnum == OBJ_VNUM_HEIRLOOM)
            chance = (chance * 9) / 10;
    }

    if ((victim->in_room && (victim->in_room->sector_type == SECT_UNDERWATER)) && !is_affected(victim, gsn_aquamove))
    {
        if (number_percent() <= get_skill(victim, gsn_waveborne))
            check_improve(victim, ch, gsn_waveborne, true, 4);
        else
        {
            check_improve(victim, ch, gsn_waveborne, false, 4);
        	chance = round(chance * 0.8);
        }
    }

    if (IS_NAFFECTED(ch,AFF_BLINK))
	chance = round(chance *.85);

    chance += (victim->level - ch->level);

    if (((tskill = get_skill(victim, gsn_tumbling)) > 0) && !IS_NPC(victim))
    {
	int tchance = tskill / 5;
	int numobjs = 0, selobj;
	OBJ_DATA *tObj;

	chance += (tskill / 10);
	if (victim->in_room)
	{
	    for (tObj = victim->in_room->contents; tObj; tObj = tObj->next_content)
	    {
		if (!IS_SET(tObj->wear_flags, ITEM_TAKE) && !IS_SET(tObj->extra_flags[0], ITEM_WIZI))
		{
		    tchance += 5;
		    numobjs++;
		}
	    }
	}
	if (IS_NAFFECTED(victim,AFF_LEGSHATTER))
	    chance /= 2;
        if (IS_NAFFECTED(victim,AFF_BOLO))
	    chance /= 2;

	tchance = UMIN(tchance, 90);

// brazen: no other defense improves on failure. Tumbling should be no
// different.
	whinyassbitch(ch,victim,"Tumbling",chance);
	if (number_percent() <= tchance)
	{
	    check_improve(victim, ch, gsn_tumbling, TRUE, 3);
	    if (numobjs < 1 || number_percent() < 70)
	    {
		act("You tumble away from $n's attack.", ch, NULL, victim, TO_VICT);
		act("$N tumbles away from your attack.", ch, NULL, victim, TO_CHAR);
	        check_enthusiasm(ch,victim);
		return TRUE;
	    }

	    selobj = number_range(1, numobjs);

	    for (tObj = victim->in_room->contents; selobj > 1; tObj = tObj->next_content)
	    {
		if (!IS_SET(tObj->wear_flags, ITEM_TAKE) && !IS_SET(tObj->extra_flags[0], ITEM_WIZI))
	            --selobj;
	    }

	    act("You tumble behind $p, avoiding $n's attack!", ch, tObj, victim, TO_VICT);
	    act("$N tumbles behind $p, avoiding your attack!", ch, tObj, victim, TO_CHAR);
	    check_enthusiasm(ch,victim);
    	    return TRUE;
	}
    }
    
    whinyassbitch(ch,victim,"Dodge (post tumble)",chance);

    if ((roll = number_percent()) > chance )
	return FALSE;    

    if (roll <= 5)
    {
	if (number_bits(1) == 0)
	{
	    act("You narrowly dodge $n's attack.", ch, NULL, victim, TO_VICT);
	    act("$N narrowly dodges your attack.", ch, NULL, victim, TO_CHAR);
	}
	else
	{
	    act("You barely dodge $n's attack.", ch, NULL, victim, TO_VICT);
	    act("$N barely dodges your attack.", ch, NULL, victim, TO_CHAR);
	}
    }
    else if ((roll > (chance - 5)) && (roll >= 0))
    {
	int echo = number_range(1, 6);

	switch (echo)
	{
	    case 1:
		act("You clearly dodge $n's attack.", ch, NULL, victim, TO_VICT);
		act("$N clearly dodges your attack.", ch, NULL, victim, TO_CHAR);
		break;

	    case 2:
		act("You cleanly dodge $n's attack.", ch, NULL, victim, TO_VICT);
		act("$N cleanly dodges your attack.", ch, NULL, victim, TO_CHAR);
		break;

	    case 3:
		act("You deftly dodge $n's attack.", ch, NULL, victim, TO_VICT);
		act("$N deftly dodges your attack.", ch, NULL, victim, TO_CHAR);
		break;

	    case 4:
		act("You skillfully dodge $n's attack.", ch, NULL, victim, TO_VICT);
		act("$N skillfully dodges your attack.", ch, NULL, victim, TO_CHAR);
		break;

	    case 5:
		act("You dodge $n's attack with casual grace.", ch, NULL, victim, TO_VICT);
		act("$N dodges your attack with casual grace.", ch, NULL, victim, TO_CHAR);
		break;

	    case 6:
		act("You easily dodge $n's attack.", ch, NULL, victim, TO_VICT);
		act("$N easily dodges your attack.", ch, NULL, victim, TO_CHAR);
		break;
	}
    }
    else if (roll != -1)
    {
        act( "You dodge $n's attack.", ch, NULL, victim, TO_VICT    );
        act( "$N dodges your attack.", ch, NULL, victim, TO_CHAR    );
    }

    check_improve(victim,ch,gsn_dodge,TRUE,5);
    check_enthusiasm(ch,victim);
    return TRUE;
}



/*
 * Set position of a victim.
 */
void update_pos( CHAR_DATA *victim )
{
    int hit = victim->hit;
    AFFECT_DATA *paf;
    if (is_affected(victim, gsn_zeal) || IS_AFFECTED(victim, AFF_BERSERK))
    	hit += 100;

    if (paf = affect_find(victim->affected,gsn_ignore_pain))
	    hit += (paf->level * 3);

    // Check for martyr's requiem
    AFFECT_DATA * requiem(get_group_song(victim, gsn_requiemofthemartyr));
    if (requiem != NULL && can_be_affected(victim, gsn_requiemofthemartyr))
        hit += (requiem->level * 3);

    if ((paf = get_affect(victim, gsn_aspectoftheinferno)) && paf->modifier == 0)
        hit += paf->level * 2;

    // Check for pain channel
    bool isFine(hit > 0);
    if (!isFine && !is_affected(victim, gsn_painchannel) && number_percent() <= get_skill(victim, gsn_painchannel))
    {
        if (hit >= -50) isFine = true;
        else if (hit >= -125) isFine = (number_bits(2) != 0);
        else if (hit >= -200) isFine = (number_bits(1) != 0);
        else isFine = (number_bits(2) == 0);

        if (isFine)
            check_improve(victim, NULL, gsn_painchannel, true, 2);
        else
        {
            // Apply an effect which is the signal that painchannel can no longer save the victim
            // This exists because update_pos can be called many times in a single determination of death, and unlike
            // the other prevent-death-from-negative-hp skills, this one has a random element
            AFFECT_DATA af = {0};
            af.where    = TO_AFFECTS;
            af.location = APPLY_HIDE;
            af.type     = gsn_painchannel;
            affect_to_char(victim, &af);
        }
    }

    if (isFine)
    {
    	if ( victim->position <= POS_STUNNED )
            switch_position(victim, POS_STANDING);
        return;
    }

    if ( IS_NPC(victim) && hit < 1 )
    {
	switch_position(victim, POS_DEAD);
	return;
    }

    if ( hit <= -11 )
    {
	switch_position(victim, POS_DEAD);
	return;
    }

         if ( hit <= -6 ) switch_position(victim, POS_MORTAL);
    else if ( hit <= -3 ) switch_position(victim, POS_INCAP);
    else                  switch_position(victim, POS_STUNNED);
}



/*
 * Start fights.
 */
void set_fighting( CHAR_DATA *ch, CHAR_DATA *victim )
{
    // Cannot fight yourself or a shade
    if (ch == victim || (IS_NPC(ch) && IS_SET(ch->nact, ACT_SHADE)) || (IS_NPC(victim) && IS_SET(victim->nact, ACT_SHADE)))
    	return;

    if (((!IS_NPC(ch) || (IS_NPC(ch) && !IS_SET(ch->nact, ACT_PACIFIC))) && ch->fighting) ||
     (IS_NPC(ch) && IS_SET(ch->nact, ACT_PACIFIC) && ch->fighting && (ch->fighting->fighting == ch)))
        return;

    if (ch->in_room == NULL || ch->in_room != victim->in_room)
        return;

    if ( IS_AFFECTED(ch, AFF_SLEEP) )
    {
	AFFECT_DATA *paf;
	int x;

	affect_strip( ch, gsn_sleep );

	for (paf = ch->affected; paf; paf = paf->next)
	    if (paf->type == gsn_overwhelm)
	    {
		CHAR_DATA *fch = (CHAR_DATA *) paf->point;
		for (x = 0; x < MAX_FOCUS; x++)
		{
		    if (fch->pcdata->focus_on[x]
		    && (fch->pcdata->focus_sn[x] == gsn_overwhelm)
		    && (fch->pcdata->focus_ch[x] == ch))
		    {
			unfocus(fch, x, FALSE);
			break;
		    }
		}
	    }
    }

    if (IS_NPC(victim) && IS_AFFECTED(victim, AFF_WIZI))
	return;

    ch->fighting = victim;
    switch_position(ch, POS_FIGHTING);
    if ((!IS_NPC(ch)) && (!IS_IMMORTAL(ch)))
      ch->pcdata->adrenaline = IS_NPC(victim) ? UMAX(ch->pcdata->adrenaline, 2) : UMAX(ch->pcdata->adrenaline, 20);
}

/*
 * Stop fights.
 */
void stop_fighting(CHAR_DATA *ch)
{
    ch->tracking = NULL;
    ch->fighting	= NULL;
    ch->position	= IS_NPC(ch) ? ch->default_pos : POS_STANDING;
    update_pos( ch );
}

void stop_fighting_all(CHAR_DATA *ch)
{
    CHAR_DATA *fch;

    if (!ch->in_room)
    {
	stop_fighting(ch);
	return;
    }

    for (fch = ch->in_room->people; fch; fch = fch->next_in_room)
    {
	if ((fch == ch) || (fch->fighting == ch))
	{
	    stop_fighting(fch);

	    /* Fix for familiars and pets.  -Erinos */
	    if (fch != ch)
	    {
		if (fch->familiar && (fch->familiar->fighting == ch))
		    stop_fighting(fch->familiar);

		if (fch->pet && (fch->pet->fighting == ch))
		    stop_fighting(fch->pet);
	    }
	}
    }
}


extern	OBJ_DATA *  make_trophy(int vnum, OBJ_DATA *corpse);

/*
 * Make a corpse out of a character.
 *
 * Corpse values:
 * 0: vnum/class_num of killed
 * 1: number of trophies taken
 * 2: race number of victim.
 * 3:
 * 4: something.
 */
void make_corpse(CHAR_DATA *ch, CHAR_DATA *killer, bool decorporealize, bool illusory)
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *corpse;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    char *name;
    int affmod;
    int i;

    if (is_affected(ch, gsn_affinity))
	affmod = get_modifier(ch->affected, gsn_affinity);
    else affmod = -1;

    if ( IS_NPC(ch) )
    {
        if (ch->pIndexData->vnum == MOB_VNUM_ZOMBIE || ch->pIndexData->vnum == MOB_VNUM_BARROWMISTZOMBIE || ch->pIndexData->vnum == MOB_VNUM_BARROWMISTWIGHT)
        {
            act("The force of the black magic dissipates, and $n crumbles to dust!", ch, NULL, NULL, TO_ROOM);
        
            for ( obj = ch->carrying; obj != NULL; obj = obj_next )
            {
                bool floating = FALSE;

                obj_next = obj->next_content;
                if (IS_SET(obj->worn_on, WORN_FLOAT))
                    floating = TRUE;
                obj_from_char(obj);
                if (obj->item_type == ITEM_POTION)
                        obj->timer = number_range(500,1000);
                if (obj->item_type == ITEM_SCROLL)
                        obj->timer = number_range(1000,2500);
                if (IS_SET(obj->extra_flags[0], ITEM_ROT_DEATH) && !floating)
                {
                        obj->timer = number_range(5,10);
                        REMOVE_BIT(obj->extra_flags[0], ITEM_ROT_DEATH);
                }
                REMOVE_BIT(obj->extra_flags[0], ITEM_VIS_DEATH);

                if ( IS_SET( obj->extra_flags[0], ITEM_INVENTORY ) )
                         extract_obj( obj );
                else if (floating)
                {
                    if (IS_OBJ_STAT(obj,ITEM_ROT_DEATH)) /* get rid of it! */
                    { 
                        if (obj->contains != NULL)
                        {
                            OBJ_DATA *in, *in_next;

                            act("$p evaporates, scattering its contents.", ch,obj,NULL,TO_ROOM);
                            for (in = obj->contains; in != NULL; in = in_next)
                            {
                                in_next = in->next_content;
                                obj_from_obj(in);
                                obj_to_room(in,ch->in_room);
                            }
                        }
                        else
                            act("$p evaporates.", ch,obj,NULL,TO_ROOM);
                        extract_obj(obj);
                    }
                    else
                    {
                        act("$p falls to the floor.",ch,obj,NULL,TO_ROOM);
                        obj_to_room(obj,ch->in_room);
                    }
                }
                else if (IS_SET(obj->wear_flags, ITEM_WEAR_SIGIL) || IS_SET(obj->wear_flags, ITEM_PROG))
                    obj_to_char(obj, ch);
                else
                    obj_to_room( obj, ch->in_room );
            }
            return;
        }
        name		= ch->short_descr;
        corpse		= create_object(get_obj_index(OBJ_VNUM_CORPSE_NPC), 0);
        corpse->timer	= number_range( 10, 13);
	
        corpse->owner = str_dup(ch->short_descr);
        corpse->killed_by = killer 
          ? (IS_NPC(killer) 
            ? str_dup(killer->short_descr)
            : str_dup(killer->name))
          : NULL;

        int multiplier(2);
        if (killer != NULL)
        {
            switch (Luck::Check(*killer))
            {
                case Luck::Lucky: multiplier = 3; break;
                case Luck::Unlucky: multiplier = 1; break;
                default: break;
            }
        }

        for (i = 0; i < MAX_COIN; i++)
        {
            if (ch->coins[i] > 0)
            {
                obj_to_obj(create_money(UMAX(1, (ch->coins[i] * multiplier) / 2), i), corpse);
                ch->coins[i] = 0;
            }
        }
        corpse->cost = 0;
        corpse->value[0] = ch->pIndexData->vnum;
        corpse->value[4] = 0;
    }
    else
    {
	name		= ch->name;
	corpse		= create_object(get_obj_index(OBJ_VNUM_CORPSE_PC), 0);
	corpse->timer	= number_range( 25, 40 );
	REMOVE_BIT(ch->act,PLR_CANLOOT);
	corpse->owner = str_dup(ch->name);

	// Unless this is illusory, move the coins over
    if (!illusory)
    {
        for (i = 0; i < MAX_COIN; i++)
        {
            if (ch->coins[i] > 0)
            {
                obj = create_money(ch->coins[i], i);
                obj->lastowner[0] = str_dup(ch->name);
                obj_to_obj(obj, corpse);
                ch->coins[i] = 0;
            }
        }
    }
		
	corpse->cost = 0;
	corpse->value[0] = (ch->class_num * -1);
	if (ch->recall_to)
	    corpse->value[4] = ch->recall_to->vnum;
	else
	    corpse->value[4] = 0;
// brazen: ticket #263: Store the original status of the ROOM_SAVE flag when the corpse is created
/*	corpse->value[3] = IS_SET(ch->in_room->room_flags, ROOM_SAVE) ? 1 : 0;*/
    }
 
    corpse->level = ch->level;
    corpse->objfocus[0] = ch;
    corpse->value[2] = ch->race;
    corpse->size = ch->size;
    if ((ch->in_room && ch->in_room->area->vnum == AREA_VNUM_SHARGOB)
      || (ch->in_room && ch->in_room->area->vnum == AREA_VNUM_MINES)
      || (IS_NPC(ch) && IS_SET(ch->form,FORM_UNDEAD))
      || (IS_NPC(ch) && IS_SET(ch->act,ACT_UNDEAD)))
	corpse->value[1] = CORPSE_POISONED;

    // Set the shorts and longs
    free_string( corpse->short_descr );
    free_string( corpse->description );
    if (decorporealize)
    {
        // Also set the name for decorpealization
        setName(*corpse, "heap clothing");
        corpse->short_descr = str_dup("a heap of clothing");
        corpse->description = str_dup("A heap of clothing has been abandoned here.");

        // Add an effect so other places can check this
        AFFECT_DATA af = {0};
        af.where    = TO_OBJECT;
        af.level    = ch->level;
        af.duration = -1;
        af.type     = gsn_decorporealize;
        affect_to_obj(corpse, &af);
    }
    else
    {
        sprintf( buf, corpse->short_descr, name );
        corpse->short_descr = str_dup( buf );
        sprintf( buf, corpse->description, name );
        corpse->description = str_dup( buf );

        // Handle illusory corpses
        if (illusory)
            corpse->timer = number_range(2, 4);
    }

    if (killer)	corpse->killed_by = str_dup(IS_NPC(killer) ? killer->short_descr : killer->name);
    else corpse->killed_by = str_dup("an unidentified party");

    for ( obj = ch->carrying; obj != NULL; obj = obj_next )
    {
        bool floating = FALSE;
        obj_next = obj->next_content;

        // If this is an illusory corpse, only make illusory objects within
        if (illusory) 
        {
            obj_to_obj(make_illusionary_object(*obj, number_range(2, 4)), corpse);
            continue;
        }

        if (IS_NPC(ch) && ch->pIndexData->pShop && !obj->worn_on)
        {
            extract_obj(obj);
            continue;
        }

        if ((obj->pIndexData->limit_factor > 0) && (obj->pIndexData->current > obj->pIndexData->limit))
        {
            sprintf(buf, "Overlimit on death purge: '%s' (vnum: %d) from %s's corpse -- %d were in existence, limit is %d.",
            obj->short_descr, obj->pIndexData->vnum, ch->name, obj->pIndexData->current, obj->pIndexData->limit);
            log_string(buf);
            extract_obj(obj);
            continue;
        }

        if (IS_SET(obj->worn_on, WORN_FLOAT))
            floating = TRUE;

        obj_from_char(obj);
        if (!IS_VALID(obj))
            continue;

        if (obj->item_type == ITEM_POTION)
            obj->timer = number_range(500,1000);

        if (obj->item_type == ITEM_SCROLL)
            obj->timer = number_range(1000,2500);

        if (IS_SET(obj->extra_flags[0], ITEM_ROT_DEATH) && !floating)
        {
            obj->timer = number_range(5,10);
            REMOVE_BIT(obj->extra_flags[0], ITEM_ROT_DEATH);
        }
        REMOVE_BIT(obj->extra_flags[0], ITEM_VIS_DEATH);

        if (floating)
        {
            if (IS_OBJ_STAT(obj,ITEM_ROT_DEATH)) /* get rid of it! */
            { 
                if (obj->contains != NULL)
                {
                    OBJ_DATA *in, *in_next;

                    act("$p evaporates,scattering its contents.",
                    ch,obj,NULL,TO_ROOM);
                    for (in = obj->contains; in != NULL; in = in_next)
                    {
                        in_next = in->next_content;
                        obj_from_obj(in);
                        obj_to_room(in,ch->in_room);
                    }
                 }
                 else
                    act("$p evaporates.",
                    ch,obj,NULL,TO_ROOM);
                 extract_obj(obj);
            }
            else
            {
                    act("$p falls to the floor.",ch,obj,NULL,TO_ROOM);
                    obj_to_room(obj,ch->in_room);
            }
        }
        else
        {
            if (IS_SET(obj->wear_flags, ITEM_WEAR_SIGIL)
             || IS_SET(obj->wear_flags, ITEM_PROG)
             || ((obj->extra_flags[0] & ITEM_AFFINITY)
              && (get_modifier(obj->affected, gsn_affinity) == affmod)))
                obj_to_char(obj, ch);
            else
                obj_to_obj( obj, corpse );                 
        }
    }

    obj_to_room( corpse, ch->in_room );

// brazen: Ticket #217: Heads will ROLL. Really, they already were, but not for NPCs
    if (is_affected(ch, gsn_decapitate))
    {
        SET_BIT(corpse->value[1], CORPSE_MISSING_HEAD);
        obj = make_trophy(OBJ_VNUM_TROPHY_HEAD, corpse);
        obj->timer = 48;
        obj_to_room(obj, ch->in_room);
    }

    if ((obj_is_affected(get_eq_char(killer, WEAR_WIELD), gsn_cursebarkja) ||
         obj_is_affected(get_eq_char(killer, WEAR_DUAL_WIELD), gsn_cursebarkja)) &&
        (number_percent() < 16))
    {
        act("Thin ribbons of darkness form around $p, disfiguring the remains.", killer, corpse, NULL, TO_CHAR);
        act("Thin ribbons of darkness form around $p, disfiguring the remains.", killer, corpse, NULL, TO_ROOM);
        desiccate_corpse(corpse);
    }
}

/*
 * Improved Death_cry contributed by Diavolo.
 */
void death_cry( CHAR_DATA *ch, bool decorporealize)
{
    ROOM_INDEX_DATA *was_in_room;
    CHAR_DATA *victim;
    int door;
    char *msg;
    char ttmsg[MAX_STRING_LENGTH];
    int vnum;

    vnum = 0;
    msg = "You hear $n's death cry.";

    if (IS_SET(ch->act,ACT_ILLUSION))
        return;

    switch ( number_range(0,10))
    {
    case  0: msg  = "$n hits the ground ... DEAD.";			break;
    case  1: 
	if (ch->material == MATERIAL_FLESH)
	    msg  = "$n splatters blood on your armor.";		
	break;
    case  2: 							
	if (IS_SET(ch->parts,PART_GUTS))
	{
	    msg = "$n spills $s guts all over the floor.";
	    vnum = OBJ_VNUM_GUTS;
	}
	break;
    case  8: 
	if (IS_SET(ch->parts,PART_HEAD))
	{
	    msg  = "$n's severed head plops on the ground.";
	    vnum = OBJ_VNUM_SEVERED_HEAD;				
	}
	break;
    case  4: 
	if (IS_SET(ch->parts,PART_HEART))
	{
	    msg  = "$n's heart is torn from $s chest.";
	    vnum = OBJ_VNUM_TORN_HEART;				
	}
	break;
    case  5: 
	if (IS_SET(ch->parts,PART_ARMS))
	{
	    msg  = "$n's arm is sliced from $s dead body.";
	    vnum = OBJ_VNUM_SLICED_ARM;				
	}
	break;
    case  6: 
	if (IS_SET(ch->parts,PART_LEGS))
	{
	    msg  = "$n's leg is sliced from $s dead body.";
	    vnum = OBJ_VNUM_SLICED_LEG;				
	}
	break;
    case 7:
	if (IS_SET(ch->parts,PART_BRAINS))
	{
	    msg = "$n's head is shattered, and $s brains splash all over you.";
	    vnum = OBJ_VNUM_BRAINS;
	}
    case 3:
	if (IS_SET(ch->parts,PART_TAIL))
	{
	    msg = "$n's tail is cut off, and it lies on the floor twitching.";
	    vnum = OBJ_VNUM_TAIL;
	}
    }

    if (!decorporealize)
        act( msg, ch, NULL, NULL, TO_ROOM );

    if ( vnum != 0 )
    {
	char buf[MAX_STRING_LENGTH];
	OBJ_DATA *obj;
	char *name;

	name		= IS_NPC(ch) ? ch->short_descr : ch->name;
	obj		= create_object( get_obj_index( vnum ), 0 );
	obj->timer	= number_range( 4, 7 );

	sprintf( buf, obj->short_descr, name );
	free_string( obj->short_descr );
	obj->short_descr = str_dup( buf );

	sprintf( buf, obj->description, name );
	free_string( obj->description );
	obj->description = str_dup( buf );

	if (obj->item_type == ITEM_FOOD)
	{
	    if (IS_SET(ch->form,FORM_POISON))
		obj->value[3] = 1;
	    else if (!IS_SET(ch->form,FORM_EDIBLE))
		obj->item_type = ITEM_TRASH;
	}

	if (!obj || !ch || !ch->in_room)	
	  return;

	obj_to_room( obj, ch->in_room );
    }
    if (!is_affected(ch,gsn_muffle))
    {
    	if ( IS_NPC(ch) )
	    msg = "You hear something's death cry.";
    	else
	    msg = "You hear someone's death cry.";

	if ((was_in_room = ch->in_room) != NULL)
    	{
            for ( door = 0; door <= 5; door++ )
            {
	    	EXIT_DATA *pexit;

	    	if ( ( pexit = was_in_room->exit[door] ) != NULL
	    	  && pexit->u1.to_room != NULL
	    	  && pexit->u1.to_room != was_in_room )
	    	{
	            ch->in_room = pexit->u1.to_room;
	            for (victim = ch->in_room->people ; victim != NULL ; victim = victim->next_in_room)
		    {
		    	if (victim != ch)
		    	    if (victim->class_num == global_int_class_watcher)
		     	    {
				sprintf(ttmsg, "You hear %s's death cry.\n\r", PERS(ch, victim));
				send_to_char(ttmsg, victim);
		    	    }
		    	    else
	    			act( msg, victim, ch, NULL, TO_CHAR );
		    }
	    	}
            }

            ch->in_room = was_in_room;
    	}
    }
    return;
}

void whinyassbitch(CHAR_DATA *ch, CHAR_DATA *victim, char *defname,int chance)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *vch;
    int i;

    sprintf(buf,"%s chance, %s vs %s: %d%%\n\r",defname,
      IS_NPC(ch) ? ch->short_descr : ch->name,
      IS_NPC(victim) ? victim->short_descr : victim->name ,chance);

    if (ch->in_room == NULL)
	return;
    for (vch = ch->in_room->people;vch;vch=vch->next_in_room)
    {
	if (IS_IMMORTAL(vch) && IS_SET(ch->nact,PLR_EXTRASPAM))
	    send_to_char(buf,vch);
        for (i=0;i<MAX_SNOOP;i++)
	    if (victim != NULL && victim->desc != NULL
	      && !IS_NPC(victim) && victim->desc->snooped)
		if ((victim->desc->snoop_type[i] == SNOOP_NORMAL
	          || victim->desc->snoop_type[i] == SNOOP_BRIEF)
		  && victim->desc->snoop_by[i]
		  && victim->desc->snoop_by[i]->character
		  && victim->desc->snoop_by[i]->character->in_room != ch->in_room
		  && IS_SET(victim->desc->snoop_by[i]->character->nact,PLR_EXTRASPAM))
		    send_to_char(buf,victim->desc->snoop_by[i]->character);
    }
		
		
}

void kill_char(CHAR_DATA *victim, CHAR_DATA *ch)
{
    CHAR_DATA *vch, *vch_next, *obscurer = NULL;
    char buf[MAX_STRING_LENGTH];
    int xpgain;
    bool obscured = FALSE;
    int chance = 0, vchance;
    unsigned int num_obscure = 0;
    float multiplier = 1.0;

    // Apply experience; do this first to maintain room sanity
    victim->experience_list->OnDeath(*victim);

    // Check for annointed one
    if (!is_affected(victim, gsn_annointedone))
    {
        if (number_percent() <= (get_skill(victim, gsn_annointedone) * 4) / 5)
        {
            // Success; save them
            send_to_char("You fall to your knees as your body fails you, but suddenly a bright shaft of pure white light shines down from above, renewing your will!\n", victim);
            act("$n falls to $s knees as if to die, but suddenly a bright shaft of pure white light shines down on $m from above, and $e rises once more!", victim, NULL, NULL, TO_ROOM);
            victim->hit = UMIN(victim->max_hit, 200);
            victim->wait = 0;
            stop_fighting_all(victim);
            update_pos(victim);
            check_improve(victim, ch, gsn_annointedone, true, 1);

            // Apply the effect so it can't happen again really fast
            AFFECT_DATA aaf = {0};
            aaf.where    = TO_AFFECTS;
            aaf.type     = gsn_annointedone;
            aaf.level    = victim->level;
            aaf.duration = 20;
            affect_to_char(victim, &aaf);
            return;
        }
        
        check_improve(victim, ch, gsn_annointedone, false, 1);
    }

    const Faction * faction(FactionTable::LookupFor(*victim));
    if (ch != NULL && faction != NULL && (!IS_NPC(ch) || ch->master != NULL) && !IS_IMMORTAL(ch) && !IS_SET(victim->act,ACT_ILLUSION))
    {
        if (!faction->HasFlag(Faction::NoObscure) && victim->in_room != NULL)
        {
            for (vch = victim->in_room->people; vch; vch = vch->next_in_room)
            {
                if (vch == victim)
                    continue;
                    
                // If someone from the same faction sees you, no chance of obscuring
                const Faction * vchFaction(FactionTable::LookupFor(*vch));
                if (vchFaction == NULL)
                    continue;
                
                if (vchFaction == faction)
                {
                    chance = -1;
                    break;
                }

                // If someone from an allied faction sees you, no chance of obscuring
                for (size_t i(0); i < faction->AllyCount(); ++i)
                {
                    if (faction->Ally(i) < FactionTable::Instance().Count() && &FactionTable::Instance()[faction->Ally(i)] == vchFaction)
                    {
                        chance = -1;
                        break;
                    }
                }

                // If someone from an enemy faction sees you, no chance of obscuring
                for (size_t i(0); i < faction->EnemyCount(); ++i)
                {
                    if (faction->Enemy(i) < FactionTable::Instance().Count() && &FactionTable::Instance()[faction->Enemy(i)] == vchFaction)
                    {
                        chance = -1;
                        break;
                    }
                }
            }
               
            // Sum up all those trying to obscure the evidence 
            if (chance != -1)
            {
                for (vch = victim->in_room->people; vch; vch = vch->next_in_room)
                {
                    if (IS_SET(vch->paffected_by, AFF_OBSCURE_EVIDENCE) && is_same_group(ch, vch) && ((vchance = get_skill(vch, gsn_obscure_evidence)) > 0))
                    {
                        chance += (vchance / 2);
                        num_obscure++;
                        act("$N attempts to cover up the death of $n.", victim, NULL, vch, TO_NOTVICT);
                        act("You attempt to cover up the death of $n.", victim, NULL, vch, TO_VICT);
                        obscurer = vch;
                    }
                }
            }

            // Check for actual obscuring
            if (num_obscure > 0)
            {
                if (number_percent() <= chance)
                    obscured = TRUE;

                for (vch = victim->in_room->people; vch; vch = vch->next_in_room)
                {
                    if (IS_SET(vch->paffected_by, AFF_OBSCURE_EVIDENCE) && is_same_group(ch, vch) && get_skill(vch, gsn_obscure_evidence) > 0)
                    {
                        if (obscured)
                        {
                            if (num_obscure > 1) send_to_char("Your combined efforts successfully obscure the evidence.\n\r", vch);
                            else                 send_to_char("You successfully obscure the evidence.\n\r", vch);
                            check_improve(vch, NULL, gsn_obscure_evidence, TRUE, num_obscure);
                        }
                        else
                        {
                            if (num_obscure > 1) send_to_char("Your combined efforts fail to obscure the evidence.\n\r", vch);
                            else                 send_to_char("You fail to successfully obscure the evidence.\n\r", vch);
                            check_improve(vch, NULL, gsn_obscure_evidence, FALSE, num_obscure);
                        }	
                    }
                    else
                    {
                        if (obscured)
                        {
                            if (num_obscure > 1) send_to_char("Their combined efforts successfully obscure the evidence.\n\r", vch);
                            else                 act("$N successfully obscures the evidence.", vch, NULL, obscurer, TO_CHAR);
                        }
                        else
                        {
                            if (num_obscure > 1) send_to_char("Their combined efforts fail to obscure the evidence.\n\r", vch);
                            else                 act("$N fails to successfully obscures the evidence.", vch, NULL, obscurer, TO_CHAR);
                        }
                    }
                }
            }
        }

        if (!obscured)
        {
            // Failed to obscure, so adjust for faction hit
            if (IS_SET(victim->nact, ACT_FACT_LEADER)) multiplier *= 15;
            if (IS_SET(victim->nact, ACT_FACT_ELITE))  multiplier *= 5;
            if (IS_SET(victim->act, ACT_BADASS))       multiplier *= 2;

            multiplier *= victim->level;
            multiplier /= ch->level;

            for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
            {
                if (!is_same_group(ch, vch) || vch->pcdata == NULL)
                    continue;
            
                int extraMult(2);
                if (vch == ch) 
                    extraMult = 3;
                        
                int normalChange((FACTION_HIT_NORMAL * multiplier * extraMult) / 2);
                int allyChange((FACTION_HIT_ALLY * multiplier * extraMult) / 2);
                int enemyChange((FACTION_GAIN_ENEMY * multiplier * extraMult) / 2);
                if (Luck::Check(*vch) == Luck::Lucky)
                {
                    act("Fortunately, none of $N's friends saw you!", vch, NULL, victim, TO_CHAR);
                    normalChange = 0;
                    allyChange = 0;
                }

                vch->pcdata->faction_standing->Change(*vch, victim->pIndexData->factionNumber, normalChange, allyChange, enemyChange, true);
            }
        }
    }

    if (ch != NULL && !is_affected(ch, gsn_roaroftheexalted))
    {
        OBJ_DATA *grmlarloth = NULL;
        AFFECT_DATA *gaf, gnaf;
        for (grmlarloth = ch->carrying; grmlarloth != NULL; grmlarloth = grmlarloth->next_content)
        {
            if (grmlarloth->worn_on != WORN_HANDS 
              || grmlarloth->item_type != ITEM_WEAPON)
                continue;
            if (!obj_is_affected(grmlarloth,gsn_hungerofgrmlarloth))
                continue;
            for (gaf = grmlarloth->affected;gaf;gaf = gaf->next)
            {
                if (gaf->type != gsn_hungerofgrmlarloth)
                continue;
                if (gaf->location != APPLY_DAMROLL)
                continue;
                break;
            }
            if (gaf)
            {
                unequip_char(ch,grmlarloth);
                gaf->duration = 10;
                gaf->modifier = UMIN(10,gaf->modifier + number_bits(1) + 1); 
                equip_char(ch, grmlarloth, WORN_HANDS);
                break;
            }
            else
            {
                gnaf.valid = TRUE;
                gnaf.point = NULL;
                gnaf.where = TO_OBJECT;
                gnaf.type = gsn_hungerofgrmlarloth;
                gnaf.level = ch->level;
                gnaf.duration = 10;
                gnaf.location = APPLY_DAMROLL;
                gnaf.modifier = number_bits(1) + 1;
                gnaf.bitvector = 0;
                unequip_char(ch,grmlarloth);
                affect_to_obj(grmlarloth,&gnaf);
                equip_char(ch,grmlarloth,WORN_HANDS);
                break;
            }
        }
    }

    /*First Death Message */
    if (!IS_NPC(victim) && victim->pcdata->death_count == 0)
    {	    
        if (victim->desc->acct == NULL || (victim->desc->acct && victim->desc->acct->award_points <= 0))
        {	
            send_to_char("\n{WNOTE:{x After this and any death, you will experience some time as a ghost.\n\r", victim);	
            send_to_char("As a ghost you will interact only minimally with the physical world.\n\r", victim);
            send_to_char("After a few moments you should seek out a healer and attempt to resurrect.\n\r", victim);
            send_to_char("by typing 'heal resurrect'. If adequate time has transpired since your death,\n\r", victim);
            send_to_char("you will resume your normal form. Make sure you attempt to resurrect sooner rather\n\r", victim);
            send_to_char("than later, for if you linger in your ghostly form too long, you may find your spirit\n\r", victim);
            send_to_char("more apt to journey away from this plane. See 'help DEATH' for more.\n\n\r", victim);
        }
    }
    
    if (ch != NULL)
    {
        if (!IS_NPC(ch) && (ch->clan != 5) && (victim != ch) && !IS_SET(ch->act, PLR_REWARD))
        { 
            if (!IS_NPC(victim) && !IS_NPC(ch) && IS_SET(victim->act, PLR_REWARD))
            {
                act("$n is granted a reward for the slaying of a criminal by Iandir!",ch, NULL, NULL, TO_ROOM);
                act("You are granted a reward for the slaying of a criminal by Iandir!", ch, NULL, NULL, TO_CHAR);
                sprintf(log_buf, "%s was rewarded.", ch->name);
                log_string(log_buf);
                if (ch->level < 51)
                {
                if (IS_SET(ch->nact, PLR_HARDCORE))
                    xpgain = victim->level * 150 * HC_MULT;
                else
                    xpgain = victim->level * 150;
                gain_exp(ch, xpgain);
                sprintf(log_buf, "You have been granted %d experience!\n\r", xpgain);
                send_to_char(log_buf, ch);
                }
                else
                {
                send_to_char("You feel ready to learn a bit more.\n\r", ch);
                ch->practice += 1;
                }
            }	
        }

        if (ch != victim && !IS_NPC(ch) && !IS_NPC(victim) && victim->pcdata->bounty > 0)
        {
            char *strtime;
            sprintf(log_buf,"%s was paid %lu platinum for killing %s.",ch->name,victim->pcdata->bounty,victim->name);
            log_string(log_buf);
            NOTE_DATA *note;
            note = new_note();
            note->sender = str_dup("The Office of Records");
            note->to_list = str_dup(ch->name);
            note->subject = str_dup("Notice of Payment");
            sprintf(log_buf,"%s %s",ch->name,"Immortal");
            note->fake_to = str_dup(log_buf);
            sprintf(log_buf,"%s,\n\r\n\rIn accordance with the system of bounties maintained by our\n\restablishment, %lu platinum coin%s have been deposited into \n\ryour account. As always, the privacy of the contributor(s)\n\ris of our utmost concern.\n\r\n\rIf you have an interest in placing a bounty upon a particular\n\rindividual, please contact a member of the guild to discuss\n\rthese arrangements.\n\r\n\rThe Watchers over Avendar\n\r",ch->name,victim->pcdata->bounty,victim->pcdata->bounty == 1 ? "" : "s");
            note->text = str_dup(log_buf);
            strtime = ctime (&current_time);
            strtime[strlen(strtime)-1] = '\0';
            note->date = str_dup(strtime);
            note->date_stamp = current_time;
            note->next = NULL;
            append_note(note);
            send_to_char("You have a new note waiting.\n\r",ch);
            ch->bank[C_PLATINUM] += victim->pcdata->bounty;
            victim->pcdata->bounty = 0;
        }

        if (IS_NPC(ch) && ch->master && !IS_NPC(ch->master) && ch->master->in_room
        && ch->in_room && ch->in_room == ch->master->in_room
        && !IS_NPC(victim) && IS_SET(victim->act,PLR_REWARD))
        {
            CHAR_DATA *pch;
            pch = ch->master;
            act("$n is granted a reward for the slaying of a criminal by Iandir!",pch , NULL, NULL, TO_ROOM);
            act("You are granted a reward for the slaying of a criminal by Iandir!", pch, NULL, NULL, TO_CHAR);
            sprintf(log_buf, "%s was rewarded.", pch->name);
            log_string(log_buf);
            if (pch->level < 51)
            {
                if (IS_SET(pch->nact, PLR_HARDCORE))
                    xpgain = victim->level * 150 * HC_MULT;
                else
                    xpgain = victim->level * 150;
                gain_exp(pch, xpgain);
                sprintf(log_buf, "You have been granted %d experience!\n\r", xpgain);
                send_to_char(log_buf, pch);
            }
            else
            {
                send_to_char("You feel ready to learn a bit more.\n\r", pch);
                pch->practice += 1;
            }
        }

        if (IS_SET(victim->act,PLR_REWARD) && (ch->clan != 5) && (!IS_NPC(ch) || ((ch->spec_fun == spec_executioner) || IS_AFFECTED(ch, AFF_CHARM))))
            REMOVE_BIT(victim->act,PLR_REWARD);

        if (!IS_SET(victim->act,ACT_ILLUSION) && ch->in_room)
        {
            for (vch = ch->in_room->people; vch; vch = vch_next)
            {
                vch_next = vch->next_in_room;

                // They have to pay the piper if they are in the same group
                // as the killer or are also fighting the victim
                if ((!is_same_group(ch, vch) && (vch->fighting != victim)) || vch == victim)
                    continue;

                //brazen: reinforcing shuddeni stereotypes. Ticket #225
                int mod = aura_grade(victim), badassmod = 1;
                if (IS_NPC(victim) && IS_SET(victim->act,ACT_BADASS)) 
                    badassmod = 3;

                if (!IS_NPC(vch))
                {
                    if (!IS_NPC(victim))
                    {
                        modify_karma(vch, mod * -25);
                        if (mod < 0 && IS_GOOD(vch))
                        {
                            sprintf(buf, "{RROLEPLAY: %s is good. %s is good. %s killed %s.{x", vch->name, victim->short_descr, vch->name, victim->short_descr);
                            log_string(buf);
                            wiznet(buf,vch, NULL, WIZ_ROLEPLAY, WIZ_SECURE, get_trust(vch));
                        }
                    }
                    else if (victim->level <= (vch->level - 8))
                    {
                        if (mod > 0)
                            modify_karma(vch, -1);
                        else if (mod < 0)
                        {
                            if (IS_GOOD(vch))
                            {
                                modify_karma(vch, 25);
                                sprintf(buf, "{RROLEPLAY: %s is good. %s is good. %s killed %s.{x", vch->name, victim->short_descr, vch->name, victim->short_descr);
                                log_string(buf);
                                wiznet(buf,vch, NULL, WIZ_ROLEPLAY, WIZ_SECURE, get_trust(vch));
                            }
                            else
                                modify_karma(vch, 1);
                        }
                    }
                    else
                    {
                        if (mod > 0)
                            modify_karma(vch, -(badassmod * victim->level/7));
                        else if (mod < 0)
                        {
                            if (IS_GOOD(vch))
                            {
                                modify_karma(vch, 50);
                                sprintf(buf,"{rROLEPLAY: %s is good. %s is good. %s killed %s.{x",vch->name,victim->short_descr, vch->name, victim->short_descr);
                                log_string(buf);
                                wiznet(buf,vch,NULL,WIZ_ROLEPLAY,WIZ_SECURE,get_trust(vch));
                            }
                            else
                                modify_karma(vch, (badassmod * victim->level/9));
                        }
                    }
                        
                    if (vch->race == global_int_race_shuddeni)
                        vch->pcdata->karma = URANGE(FAINTREDAURA, vch->pcdata->karma, BLACKAURA);
                    else
                    {
                        if (IS_GOOD(vch))
                           vch->pcdata->karma = URANGE(SILVERAURA, vch->pcdata->karma, REDAURA - 1);
                        else if (IS_NEUTRAL(vch))
                            vch->pcdata->karma = URANGE(BRIGHTGOLDENAURA + 1, vch->pcdata->karma, DARKREDAURA - 1);
                        else
                            vch->pcdata->karma = URANGE(GOLDENAURA + 1, vch->pcdata->karma, BLACKAURA);
                    }
                }

                if (!IS_NPC(victim) && !IS_NPC(vch))
                {
                    vch->pcdata->pkills++;
                    if (IS_GOOD(victim))
                    vch->pcdata->align_kills[ALIGN_GOOD]++;
                    else if (IS_EVIL(victim))
                    vch->pcdata->align_kills[ALIGN_EVIL]++;
                    else if (IS_NEUTRAL(victim))
                    vch->pcdata->align_kills[ALIGN_NEUTRAL]++;
                }

                if (!IS_NPC(victim) && IS_NPC(vch) && vch->master && !IS_NPC(vch->master)
                && (vch->master->in_room != vch->in_room || vch->master->fighting != victim))
                {
                    vch->master->pcdata->pkills++;
                    if (IS_GOOD(victim))
                        vch->master->pcdata->align_kills[ALIGN_GOOD]++;
                    else if (IS_EVIL(victim))
                        vch->master->pcdata->align_kills[ALIGN_EVIL]++;
                    else if (IS_NEUTRAL(victim))
                        vch->master->pcdata->align_kills[ALIGN_NEUTRAL]++;
                }

                if (((!IS_NPC(vch) && (vch->class_num == global_int_class_ranger) && vch->pet
                && vch->in_room && vch->pet->in_room && (vch->pet->in_room == vch->in_room) 
                && is_name("rangercall", vch->pet->name))
                || (IS_NPC(vch) && vch->master && (!IS_NPC(vch->master)
                && (vch->master->class_num == global_int_class_ranger)) && vch->in_room
                && vch->master->in_room && (vch->in_room == vch->master->in_room)
                && is_name("rangercall", vch->name))) && !IS_NPC(victim))
                {
                    CHAR_DATA *pch;

                    if (IS_NPC(vch))
                    {
                        pch = vch;
                        send_to_char("With this new experience, your animal companion improves!\n\r", vch->master);
                    }
                    else
                    {
                        pch = vch->pet;
                        send_to_char("With this new experience, your animal companion improves!\n\r", vch);
                    }

                    pch->hitroll += 1 + (victim->level / 25);
                    pch->damroll += 1 + (victim->level / 25);
                    pch->max_hit += victim->level;
                }

                if (!IS_NPC(victim))
                {
                    sprintf(log_buf, "%s killed by %s at %d", victim->name, (IS_NPC(vch) ? vch->short_descr : vch->name), vch->in_room->vnum);
                    log_string( log_buf );
                }

                if (IS_NPC(victim) && IS_NPC(vch))
                {
                    sprintf( log_buf, "%s (L%d %s %s) got player-toasted by %s (L%d %s %s) at %s [room %d].",
                          victim->name,
                          victim->level,race_table[victim->race].name,
                          class_table[victim->class_num].name,
                          vch->name,
                          vch->level,race_table[vch->race].name,
                          class_table[vch->class_num].name,
                          vch->in_room->name, vch->in_room->vnum);
                }
                else
                {
                    sprintf( log_buf, "%s got toasted by %s at %s [room %d].",
                          (IS_NPC(victim) ? victim->short_descr : victim->name),
                          (IS_NPC(vch) ? vch->short_descr : vch->name),
                          vch->in_room->name, vch->in_room->vnum);
                }

                sprintf( buf, "%s was killed by %s in %s.",
                      (IS_NPC(victim) ? victim->short_descr : victim->name),
                      (IS_NPC(vch) ? vch->short_descr : vch->name), vch->in_room->name);

                check_comm(vch, buf);
                check_comm(victim, buf);

                if (IS_NPC(victim))
                {
                    if (vch == ch)
                        wiznet(log_buf,NULL,NULL,WIZ_MOBDEATHS,0,0);
                }
                else
                {
                    log_string(log_buf);
                    wiznet(log_buf,NULL,NULL,WIZ_DEATHS,0,0);
                }
            }
        }
    }

    victim->fighting = ch;
    raw_kill(victim);
    save_char_obj(victim);
}


void raw_kill( CHAR_DATA *victim )
{
    DESCRIPTOR_DATA *d;
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int i, level, orig_pos;
    AFFECT_DATA paf;
    paf.valid = TRUE;
    paf.point = NULL;
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;
    AFFECT_DATA * check_af;
    CHAR_DATA *killed_by;
    OBJ_DATA *obj, *obj_next;
    char buf[MAX_STRING_LENGTH];

    if (victim->fighting != NULL)
	killed_by = victim->fighting;
    else
	killed_by = NULL;

	// Store off the death location
    if (victim->pcdata != NULL && victim->in_room != NULL && victim->in_room->name != NULL 
    && victim->in_room->area != NULL && victim->in_room->area->name != NULL)
    {
		// Build the string
        std::string location(victim->in_room->area->name);
        location += ": ";
        location += victim->in_room->name;

		// Replace the existing string
        free_string(victim->pcdata->last_death_location);
        victim->pcdata->last_death_location = str_dup(location.c_str());
    }

    if (IS_NAFFECTED(victim, AFF_FLESHTOSTONE) || IS_OAFFECTED(victim, AFF_ENCASE))
	REMOVE_BIT(victim->act, PLR_FREEZE);

   level = victim->level;

    if (is_affected(victim, gsn_cursekijjasku) && killed_by != NULL) 
    {
        if (IS_NPC(killed_by))
        {
        act("{WAs $n dies, $e cries forth, {c'Kijjasku eyilekk dulikra askah!{x",victim,NULL, killed_by,TO_ROOM);
        sprintf(buf,"{WWith your final breath, you cry forth, {c'Kijjasku eyilekk dulikra askah!'{x\n\r");
	send_to_char(buf,victim);
        }
        else
        {
        act("{WAs $n dies, $e cries forth, {c'Kijjasku eyilekk dulikra askah $N!'{x",victim, NULL, killed_by, TO_ROOM);
	    sprintf(buf,"{WWith your final breath, you cry forth, {c'Kijjasku eyilekk dulikra askah %s!'{x\n\r", killed_by->name);
	    send_to_char(buf,victim);
        }
     
        if ((!saves_spell(level,victim, killed_by,DAM_NEGATIVE))
        && (!is_affected(killed_by,AFF_CURSE))
        && (victim != killed_by))
        {
            af.where     = TO_AFFECTS;
            af.type      = gsn_curse;
            af.level     = level;
            af.duration  = level/2 + 10;
            af.location  = APPLY_HITROLL;
            af.modifier  = -1 * (level / 8);
            af.bitvector = AFF_CURSE;
            affect_to_char( killed_by, &af );
            af.location  = APPLY_SAVING_SPELL;
            af.modifier  = level / 8;
            affect_to_char( killed_by, &af );
            send_to_char( "You feel unclean.\n\r", killed_by);
            act("$n looks very uncomfortable.",killed_by,NULL,NULL,TO_CHAR);
        }
        
        if (!saves_spell(level,victim, killed_by,DAM_NEGATIVE)
        && !is_affected(killed_by, AFF_BLIND)
        && !IS_SET(killed_by->imm_flags, IMM_BLIND)
        && (victim != killed_by) )
        { 
            af.where     = TO_AFFECTS;
            af.type      = gsn_blindness;
            af.level     = level;
            af.location  = APPLY_HITROLL;
            af.modifier  = -4;
            af.duration  = level/6;
            af.bitvector = AFF_BLIND;
            affect_to_char( killed_by, &af );
            send_to_char( "You are blinded!\n\r", killed_by );
            act("$n appears to be blinded.",killed_by,NULL,NULL,TO_ROOM);
        }

        if (!is_affected(killed_by, gsn_pox)
        && !is_affected(victim, gsn_plague)
        && !saves_spell(level,victim, killed_by,DAM_DISEASE) 
        && (victim != killed_by) && !(IS_NPC(killed_by) 
        && IS_SET(killed_by->act,ACT_UNDEAD)) 
        && !IS_SET(killed_by->imm_flags, IMM_DISEASE)   )
        {
            af.where     = TO_AFFECTS;
            af.type      = gsn_pox;
            af.level     = level * 3/4;
            af.duration  = level/2;
            af.location  = APPLY_STR;
            af.modifier  = (-1) * level/10;
            af.bitvector = AFF_PLAGUE;
            affect_join(killed_by,&af);
            send_to_char ("You grimace as illness strikes you.\n\r",killed_by);
            act("$n grimaces and looks ill.", killed_by,NULL,NULL,TO_ROOM);
        }
     }

    if (is_an_avatar(victim))
    {
        act("As $n's life slips away, $e returns to $s natural form", victim, NULL, NULL, TO_ROOM);
        act("As your life slips away, you return to your natural form", victim, NULL, NULL, TO_CHAR);
        if (victim->long_descr)
            free_string(victim->long_descr);
        victim->long_descr = &str_empty[0];
        strip_avatar(victim);
    }

    struct BondStripper
    {
        static void Strip(CHAR_DATA * ch, int sn)
        {
            if (!is_affected(ch, sn))
                return;

            int mod(get_high_modifier(ch->affected, sn));
            CHAR_DATA * vch_next;
            for (CHAR_DATA * vch = char_list; vch != NULL; vch = vch_next)
	        {
        	    vch_next = vch->next;
        	    if (vch->id == mod)
        	    {
	                send_to_char("You feel your soul bond tear away!\n", vch);
            		affect_strip(vch, sn);
        	    	damage_old(vch,vch,number_range(2*vch->level,3*vch->level), sn, DAM_OTHER,TRUE);
        	        if (IS_OAFFECTED(vch,AFF_GHOST))
		            { 
            		    char hahaha[MAX_STRING_LENGTH];
            		    sprintf(hahaha,"%s died from spiritbond or bond of souls.",vch->name);
            		    log_string(hahaha);
		                wiznet(hahaha,NULL,NULL,WIZ_DEATHS,0,0);
            		}
        	    }
        	}
        }
    };

    BondStripper::Strip(victim, gsn_spiritbond);
    BondStripper::Strip(victim, gsn_bondofsouls);

    if (!IS_NPC(victim))
	victim->pcdata->adrenaline = 0;

    stop_fighting_all(victim);

    // vicious hack
    orig_pos = victim->position;
    switch_position(victim, POS_STANDING);

    mprog_death_trigger( victim, killed_by );
    
    // In case anything funky happened in the death trigger.
    if (!IS_VALID(victim))
        return;

    for (obj = victim->carrying; obj; obj = obj_next)
    {
        obj_next = obj->next_content;
        oprog_death_trigger(obj, killed_by);
    }

    if (killed_by && killed_by->in_room == victim->in_room && number_percent() < get_skill(killed_by,gsn_muffle))
    {
        af.where = TO_AFFECTS;
        af.type = gsn_muffle;
        af.duration = 0;
        af.modifier = 0;
        af.location = APPLY_NONE;
        af.bitvector = 0;
        af.level     = level * 3/4;
        affect_to_char(victim,&af);
        act("You silence $N's death cry with a swift blow to the neck.",killed_by,NULL,victim,TO_CHAR);
        act("$n silences $N's death cry with a swift blow to the neck.",killed_by,NULL,victim,TO_NOTVICT);
        act("$n silences your death cry with a swift blow to the neck.",killed_by,NULL,victim,TO_VICT);
    }

    // Check for decorporealize
    bool decorporealize(false);
    if (number_percent() <= (get_skill(victim, gsn_decorporealize) / 2))
    {
        check_improve(victim, killed_by, gsn_decorporealize, true, 0);
        decorporealize = true;
    }
    else
        check_improve(victim, killed_by, gsn_decorporealize, false, 0);

    // Check for feign demise
    ROOM_INDEX_DATA * illusoryDeathRoom(NULL);
    if (!is_affected(victim, gsn_feigndemise) && victim->in_room != NULL)
    {
        if (number_percent() <= (get_skill(victim, gsn_feigndemise) / 4))
        {
            // Build the list of valid directions
            std::vector<Direction::Value> directions(Direction::ValidDirectionsFrom(*victim->in_room));
            if (!directions.empty())
            {
                illusoryDeathRoom = Direction::Adjacent(*victim->in_room, directions[number_range(0, directions.size() - 1)]);
                if (illusoryDeathRoom != NULL)
                {
                    if (!can_see_room(victim, illusoryDeathRoom) 
                    || (!IS_SET(victim->in_room->room_flags, ROOM_NOGATE) && IS_SET(illusoryDeathRoom->room_flags, ROOM_NOGATE))
                    || (!IS_SET(victim->in_room->room_flags, ROOM_NOSUM_TO) && IS_SET(illusoryDeathRoom->room_flags, ROOM_NOSUM_TO))
                    || (!IS_SET(victim->in_room->room_flags, ROOM_NOSUM_FROM) && IS_SET(illusoryDeathRoom->room_flags, ROOM_NOSUM_FROM)))
                        illusoryDeathRoom = NULL;
                    else
                    {
                        check_improve(victim, killed_by, gsn_feigndemise, true, 0);

                        // Apply cooldown
                        AFFECT_DATA feign = {0};
                        feign.where     = TO_AFFECTS;
                        feign.type      = gsn_feigndemise;
                        feign.level     = victim->level;
                        feign.duration  = 180;
                        affect_to_char(victim, &feign);
                    }
                }
            }
        }

        // Handle improvement for the various fail cases
        if (illusoryDeathRoom == NULL)
            check_improve(victim, killed_by, gsn_feigndemise, false, 0);
    }

    // Check for phylactery
    OBJ_DATA * phylactery(NULL);
    ROOM_INDEX_DATA * phylacteryRoom(NULL);
    if (!IS_NPC(victim))
    {
        for (phylactery = object_list; phylactery != NULL; phylactery = phylactery->next)
        {
            AFFECT_DATA * phaf(get_obj_affect(phylactery, gsn_imbuephylactery));
            if (phaf != NULL && phaf->modifier == victim->id)
            {
                phylacteryRoom = get_room_for_obj(*phylactery);
                if (phylacteryRoom != NULL)
                    break;
            }
        }
    }

    if (IS_NPC(victim) || (victim->pcdata->age_group != AGE_DEAD))
    	death_cry(victim, decorporealize);

    mprog_all_death_trigger( victim, killed_by, victim );

    if (killed_by && killed_by->in_room)
    {
	rprog_all_death_trigger( killed_by->in_room, killed_by, victim );

	for (obj = killed_by->in_room->contents; obj; obj = obj_next)
	{
	    obj_next = obj->next_content;
	    oprog_all_death_trigger(obj, killed_by, victim);
	}

	for (vch = killed_by->in_room->people; vch; vch = vch_next)
	{
	    vch_next = vch->next_in_room;

	    for (obj = vch->carrying; obj; obj = obj_next)
	    {
		obj_next = obj->next_content;
		oprog_all_death_trigger(obj, killed_by, victim);
	    }
	}
    }

    // end of vicious hack
    switch_position(victim, orig_pos);

    for (d = descriptor_list; d != NULL; d = d->next)
    {
	if (d && d->connected == CON_PLAYING && d->character)
	  if (is_affected(d->character, gsn_thanatopsis) && d->character != victim && !IS_NPC(victim) && d->character->level >= victim->level)
	     act("You hear the death cry of $N echo through your soul.", d->character, NULL, victim, TO_CHAR);
    }

    if (killed_by && killed_by != victim)
    {
        // Check for cohortsvengeance
        if (number_percent() <= get_skill(victim, gsn_cohortsvengeance))
        {
            act("You let out a keening wail as you die, and the heavens themselves seem to answer back!", victim, NULL, NULL, TO_CHAR);
            act("$n lets out a keening wail as $e dies, and the heavens themselves seem to answer back!", victim, NULL, NULL, TO_ROOM);

            check_improve(victim, killed_by, gsn_cohortsvengeance, true, 0);
            AFFECT_DATA af = {0};
            af.where    = TO_AFFECTS;
            af.type     = gsn_cohortsvengeance;
            af.duration = number_range(10, 20);
            af.level    = victim->level;
            affect_to_char(killed_by, &af);
        }
        else
            check_improve(victim, killed_by, gsn_cohortsvengeance, false, 0);
     
        // Check for avenging seraph
        if (is_affected(victim, gsn_avengingseraph))
        {
            act("As $n crumples to the ground, a light explodes from $s body!", victim, NULL, NULL, TO_ROOM);
            act("The light fades, forming into a glowing winged angel, come for vengeance.", victim, NULL, NULL, TO_ROOM);
            summon_avenging_seraph(victim->level, killed_by);
        }
    }

    // Check for phoenix dirge
    if ((IS_NPC(victim) || victim->pcdata->age_group < AGE_DEAD) && !is_affected(victim, gsn_phoenixdirge))
    {
        int dirgeSkill(get_skill(victim, gsn_phoenixdirge));
        if (number_percent() < ((dirgeSkill * 3) / 4))
        {
            // Success; log this
            std::ostringstream mess;
            mess << (IS_NPC(victim) ? victim->short_descr : victim->name) << " returned to life from phoenix dirge.";
            log_string(mess.str().c_str());
            wiznet(const_cast<char*>(mess.str().c_str()), NULL, NULL, WIZ_DEATHS, 0, 0);

            // Burn them back to life
            victim->hit = (victim->max_hit * 4) / 5;
            victim->mana = (victim->max_mana * 4) / 5;
            victim->move = (victim->max_move * 4) / 5;
            victim->wait = 0;
            update_pos(victim);
            ++victim->pcdata->death_count;
            stop_fighting_all(victim);

            act("You have been KILLED!!", victim, NULL, NULL, TO_CHAR);
            act("You feel the power of the phoenix song explode through your body, burning life back into you!", victim, NULL, NULL, TO_CHAR);
            act("The power of the phoenix explodes from $n, burning life back into $m!", victim, NULL, NULL, TO_ROOM);

            // Add a cooldown
            AFFECT_DATA phaf = {0};
            phaf.where      = TO_AFFECTS;
            phaf.type       = gsn_phoenixdirge;
            phaf.level      = victim->level;
            phaf.location   = APPLY_HIT;
            phaf.modifier   = (victim->hit - victim->max_hit);
            phaf.duration   = 8;
            affect_to_char(victim, &phaf);

            check_improve(victim, NULL, gsn_phoenixdirge, true, 0);
            return;
        }
        
        check_improve(victim, NULL, gsn_phoenixdirge, false, 0);
    }
    
    // Check for phoenix fire
    if ((check_af = get_affect(victim, gsn_phoenixfire)) != NULL && 
		(IS_NPC(victim) || victim->pcdata->age_group < AGE_DEAD) && check_af->modifier != 0)
    {
        sprintf( log_buf, "%s returned to life from phoenix fire.",
            (IS_NPC(victim) ? victim->short_descr : victim->name));
	log_string(log_buf);
        wiznet(log_buf,NULL,NULL,WIZ_DEATHS,0,0);

	victim->hit = round(victim->max_hit * .8);
	victim->mana = round(victim->max_mana * .8);
	victim->move = round(victim->max_move * .8);
	victim->wait = 0;
	update_pos(victim);
	act("You have been KILLED!!", victim, NULL, NULL, TO_CHAR);
	act("You feel the power of the phoenix explode through your body, burning life back into you!", victim, NULL, NULL, TO_CHAR);
	victim->pcdata->death_count++;
	if (number_bits(2) == 1)
	{
	    act("The pain of returning to life weakens your body.", victim, NULL, NULL, TO_CHAR);
	    victim->pcdata->death_count++;
	}
	act("The power of the phoenix explodes from $n, burning life back into $m!", victim, NULL, NULL, TO_ROOM);
	stop_fighting_all(victim);
	check_af->modifier = 0;

	return;
    }

    // Check for bilocation; most of this will be handled by lower-level functions, 
    // but here we need to handle the case of killing the illusory body with the consciousness present
    if (IS_NPC(victim) && victim->desc != NULL)
    {
        CHAR_DATA * original(find_bilocated_body(victim));
        if (original != NULL)
        {
            // In the illusory body, so kill the real body, too
            send_to_char("The pain of death tears through your link, claiming your real form as well!\n", victim);
            act("$n's vacant expression seizes, and $e suddenly collapses!", original, NULL, NULL, TO_ROOM);
            raw_kill(original);
        }
    }

    // Check for reclaim essence
    if (!decorporealize && illusoryDeathRoom == NULL && victim->in_room != NULL)
    {
        for (CHAR_DATA * claimer(victim->in_room->people); claimer != NULL; claimer = claimer->next_in_room)
        {
            if (claimer == victim || claimer->mana >= claimer->max_mana || number_percent() > get_skill(claimer, gsn_reclaimessence))
                continue;

            // Time to reclaim some essence
            check_improve(claimer, victim, gsn_reclaimessence, true, 4);
            int claimAmount(dice(4, (victim->level / 2)));
            claimer->mana = UMIN(claimer->max_mana, claimer->mana + claimAmount);
            act("As $N's life force flows back into the Weave, a small portion of it flows into you.", claimer, NULL, victim, TO_CHAR);
        }
    }

    stop_playing_song(victim, NULL);
    if (victim->in_room != NULL && !(IS_NPC(victim) && IS_SET(victim->act, ACT_ILLUSION)))
	    make_corpse(victim, killed_by, decorporealize, illusoryDeathRoom != NULL);

    if (IS_NPC(victim) && victim->pIndexData->vnum == MOB_VNUM_SHUNNED_DEMON)
    	unbind_shunned_demon(victim);

    for ( vch = char_list; vch != NULL; vch = vch_next )
    {
    	vch_next=vch->next;

	    if (vch->tracking == victim)
	        vch->tracking = NULL;

    	if (vch->demontrack == victim && !IS_SET(vch->affected_by, AFF_CHARM) && vch->pIndexData->vnum > 62
	    && vch->in_room && vch->in_room->contents && vch->in_room->contents->item_type == ITEM_CORPSE_PC)
    	{
	        sprintf(buf,"scoops up %s, and returns to the void.",vch->in_room->contents->short_descr);
    	    do_emote(vch, buf);
	        extract_obj(vch->in_room->contents);
    	}
    }

    // Check for bone reaper
    if (!decorporealize && illusoryDeathRoom == NULL && phylactery == NULL)
        check_bonereaper(*victim);

    if ( IS_NPC(victim) )
    {
    	victim->pIndexData->killed++;
    	kill_table[URANGE(0, victim->level, MAX_LEVEL-1)].killed++;
	    extract_char( victim, TRUE );
    	return;
    }

    if (IS_SET(victim->nact, PLR_HARDCORE) && !decorporealize && illusoryDeathRoom == NULL && phylactery == NULL)
    {
    	victim->pcdata->age_group = AGE_DEAD;
	    set_perm_stat(victim, STAT_CON, 1);
    }

    if (decorporealize)
    {
        act("$n's body suddenly vanishes, leaving $s clothes to fall in a heap.", victim, NULL, NULL, TO_ROOM);
        send_to_char("Your physical form is OVERCOME!!!\n", victim);
        send_to_char("You slide out of it before any harm can come to your spirit, decorporealizing into a ghost at your safe haven.\n", victim);
        sprintf(log_buf, "%s just decorporealized.", victim->name);
        wiznet(log_buf, NULL, NULL, WIZ_DEATHS, 0, 0);
        log_string(log_buf);
        extract_char(victim, FALSE);
    }
    else if (illusoryDeathRoom != NULL)
    {
        send_to_char("You whisper a quick glamour just before the killing blow would overcome you.\n", victim);
        send_to_char("Tendrils of illusion bind rapidly into a facsimile of your corpse even as you are whisked away from harm!\n", victim);
        sprintf(log_buf, "%s just feigned death.", victim->name);
        wiznet(log_buf, NULL, NULL, WIZ_DEATHS, 0, 0);
        log_string(log_buf);

        victim->hit = UMAX(1, victim->hit);
        update_pos(victim);
        char_from_room(victim);
        char_to_room(victim, illusoryDeathRoom);
        do_look(victim, "auto");
        act("$n suddenly materializes here.", victim, NULL, NULL, TO_ROOM);
        WAIT_STATE(victim, UMAX(victim->wait, PULSE_VIOLENCE));
        return;
    }
    else if (victim->pcdata->age_group == AGE_DEAD)
    {
    	send_to_char("You have died...\n\r", victim);
	    send_to_char("Your spirit rises up from your body.  You feel the anxious grasp of the spiritworld pulling at you, and realize your time left here is short.\n\r", victim);
    	for (obj = victim->carrying; obj; obj = obj_next)
	    {
    	    obj_next = obj->next_content;
	        if (IS_SET(obj->wear_flags, ITEM_WEAR_SIGIL))
        		equip_char(victim, obj, WORN_SIGIL);
	        else if (IS_SET(obj->wear_flags, ITEM_PROG))
        		equip_char(victim, obj, WORN_PROGSLOT);
    	    else
	        	extract_obj(obj);
    	}
	    sprintf(log_buf,"%s age-died.",victim->name);
        wiznet(log_buf,NULL,NULL,WIZ_DEATHS,0,0);
    	log_string(log_buf);
    }
    else
    {
        // Check for on-imminent-death triggers
        check_bierofunmaking(*victim);

        // Handle echoes
        send_to_char("You have been KILLED!!\n\r", victim);
        if (phylactery == NULL)
            send_to_char("You have returned as a ghost, for the moment.\n\r", victim);
        else
        {
            act("The piece of your soul in $p pulls at you, bringing you back as a ghost!", victim, phylactery, NULL, TO_CHAR);
            object_affect_strip(phylactery, gsn_imbuephylactery);
        }
   
        // Perform normal death
	    sprintf(log_buf,"%s just died.",victim->name);
    	wiznet(log_buf,NULL,NULL,WIZ_DEATHS,0,0);
	    log_string(log_buf);
        extract_char(victim, FALSE, phylacteryRoom);
    }

    // Save off affects which should be reapplied after death
    int resurr = 0;
    std::vector<AFFECT_DATA> affectsToRestore;
    while ( victim->affected )
    {
    	if ((victim->affected->type == gsn_toggleoffaff)
	    && (victim->affected->modifier < 65535))
	        BIT_CLEAR(victim->pcdata->bitptr, victim->affected->modifier);

    	if ((victim->affected->type == gsn_toggleonaff)
	    && (victim->affected->modifier < 65535))
	        BIT_SET(victim->pcdata->bitptr, victim->affected->modifier);

	    if (victim->affected->type == gsn_resurrection)
	        resurr = get_modifier(victim->affected, gsn_resurrection);

        if (victim->affected->type == gsn_heartoftheinferno || victim->affected->type == gsn_attunefount
        ||  victim->affected->type == gsn_empowerphantasm || victim->affected->type == gsn_lavaforge
        ||  (victim->affected->type == gsn_abodeofthespirit && victim->affected->modifier != 0)
        ||  victim->affected->type == gsn_markofloam || victim->affected->type == gsn_bloodofthevizier
        || (victim->affected->type == gsn_bierofunmaking && victim->affected->level != 0)
        || victim->affected->type == gsn_baneblade)
            affectsToRestore.push_back(*victim->affected);

    	affect_remove( victim, victim->affected );
    }

    // Reapply affects
    for (size_t i(0); i < affectsToRestore.size(); ++i)
        affect_to_char(victim, &affectsToRestore[i]);

    victim->affected_by	= race_table[victim->race].aff;
    REMOVE_BIT(victim->affected_by, AFF_FLY_NATURAL);

    for (i = 0; i < 4; i++)
    	victim->armor[i]= 100;

    victim->position	= (phylactery == NULL ? POS_RESTING : POS_STANDING);
    victim->hit		= 1;
    victim->mana	= 1;
    victim->move	= UMAX( victim->max_move/2, victim->move );
    victim->mercy_to	= NULL;
    victim->mercy_from	= NULL;
    victim->pcdata->condition[COND_HUNGER]= get_max_hunger(victim);
    victim->pcdata->food_quality = 100;
    victim->pcdata->condition[COND_THIRST]=48;
    victim->pcdata->condition[COND_DRUNK]=0;

    if (!decorporealize && phylactery == NULL)
    {
        victim->pcdata->death_count++;

        if (!IS_IMMORTAL(victim))
        {
        	if (victim->pcdata->max_deaths == (victim->pcdata->death_count + 15))
	            send_to_char("You feel a moment of terror as the grave calls to your spirit!\n\r", victim);
        	else if (victim->pcdata->max_deaths == (victim->pcdata->death_count + 10))
	            send_to_char("Your spirit cries out for the grave, eager for its final rest.\n\r", victim);
        	else if (victim->pcdata->max_deaths == (victim->pcdata->death_count + 5))
	            send_to_char("Only by the mightiest effort of will are you able to resist the final and eternal call of the grave.\n\r", victim);
        	else if (victim->pcdata->max_deaths == (victim->pcdata->death_count + 1))
	            send_to_char("Almost nothing remains now to bind your spirit to the physical world. Your next death will surely be final.\n\r", victim);
        	else if (victim->pcdata->max_deaths <= victim->pcdata->death_count)
        	{
	            send_to_char("{WYou die, unable to deny the final call of the grave.{w\n\r", victim);
        	    set_perm_stat(victim, STAT_CON, 1);
	        }
        }
    }

    paf.where = TO_OAFFECTS;
    paf.type  = gsn_ghost;
    paf.level = 60;
    if (victim->perm_stat[STAT_CON] < 4 || victim->pcdata->max_deaths <= victim->pcdata->death_count)
    {
    	if (IS_SET(victim->nact, PLR_HARDCORE))
	        paf.duration = victim->level;
    	else
            paf.duration = 120;
    }
    else
        paf.duration = (decorporealize ? 29 : 40);
    
    paf.location = 0;
    paf.modifier = (phylactery == NULL ? 0 : 1);
    paf.bitvector = AFF_GHOST;
    affect_to_char (victim, &paf);

    paf.where = TO_AFFECTS;
    paf.bitvector = AFF_FLYING|AFF_INFRARED|AFF_PASS_DOOR;
    affect_to_char(victim, &paf);

    paf.type = gsn_resurrection;
    paf.duration = 48;
    paf.modifier = ((decorporealize || phylactery != NULL) ? resurr : (resurr >= -2 ? resurr - 1 : resurr - 2));
    paf.bitvector = 0;
   
    if (paf.modifier != 0)
    {
        paf.location = APPLY_STR;
        affect_to_char(victim, &paf);
        paf.location = APPLY_INT;
        affect_to_char(victim, &paf);
        paf.location = APPLY_WIS;
        affect_to_char(victim, &paf);
        paf.location = APPLY_DEX;
        affect_to_char(victim, &paf);
        paf.location = APPLY_CON;
        affect_to_char(victim, &paf);
        paf.location = APPLY_CHR;
        affect_to_char(victim, &paf);
    }

    if (decorporealize)
        act("$n's spirit suddenly appears here.", victim, NULL, NULL, TO_ROOM);
    else if (phylactery != NULL)
        act("A dark fog seeps out of $p, coalescing into the spectral form of $n!", victim, phylactery, NULL, TO_ROOM); 
    else if (victim->pcdata->age_group != AGE_DEAD)
        act ("$n has risen again as a ghost.", victim, NULL, NULL, TO_ROOM);
    else
    	act("$n's ghost rises up from $s fallen form.", victim, NULL, NULL, TO_ROOM);

    if (strcmp(victim->long_descr, ""))
    {
    	free_string(victim->long_descr);
	    victim->long_descr = str_dup("");
    }

    if (victim->class_num == global_int_class_druid)
        lunar_update_char(victim);

    if (!IS_IMMORTAL(victim) && phylactery == NULL)
        WAIT_STATE(victim, 60);

    REMOVE_BIT(victim->pcdata->pcdata_flags, PCD_CALL_RETAINER);
    REMOVE_BIT(victim->oaffected_by, AFF_ONEHANDED);
}

void dam_message(CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt, char *amess, bool immune)
{
    char buf1[256], buf2[256], buf3[256];
    const char *vs;
    const char *vp;
    const char *attack;
    const char *st;
    int perc;
    char punct;

    if ((!ch && !global_damage_from) || !victim)
	return;

	 if ( dam ==   0 ) { st = "";            }
    else if ( dam <=   4 ) { st = "pathetic ";   }
    else if ( dam <=   8 ) { st = "pitiful ";    }
    else if ( dam <=  12 ) { st = "feeble ";     }
    else if ( dam <=  15 ) { st = "weak ";       }
    else if ( dam <=  18 ) { st = "paltry ";     }
    else if ( dam <=  22 ) { st = "inadequate "; }
    else if ( dam <=  25 ) { st = "mediocre ";   }
    else if ( dam <=  31 ) { st = "";            }
    else if ( dam <=  37 ) { st = "average ";    }
    else if ( dam <=  45 ) { st = "telling ";    }
    else if ( dam <=  52 ) { st = "strong ";     }
    else if ( dam <=  61 ) { st = "POTENT ";     }
    else if ( dam <=  75 ) { st = "FORCEFUL ";   }
    else if ( dam <=  85 ) { st = "POWERFUL ";   }
    else if ( dam <=  95 ) { st = "FIERCE ";     }
    else if ( dam <= 110 ) { st = "** VICIOUS ** ";    }
    else if ( dam <= 125 ) { st = "*** BRUTAL *** ";     }
    else if ( dam <= 140 ) { st = "*** MIGHTY *** ";     }
    else if ( dam <= 160 ) { st = "=== FEARSOME === ";   }
    else if ( dam <= 180 ) { st = "=== FEROCIOUS === ";  }
    else if ( dam <= 220 ) { st = ">>> FORMIDABLE <<< "; }
    else if ( dam <= 260 ) { st = "<<< TREMENDOUS >>> "; }
    else if ( dam <= 320 ) { st = "))) HEROIC ((( ";     }
    else if ( dam <= 400 ) { st = "]]] TITANIC [[[ ";    }
    else                   { st = "### GODLIKE ### ";    }

    /* Going for accuracy to 0.25% here.  Multipying everything by 4 because */
    /* I'm lazy, and it'll calculate faster.  Gooo CPU speed.                */

//    if (dam < victim->hit)
//    {
    perc = ((dam * 400) + (victim->hit / 2)) / ((victim->hit > 0) ? victim->hit : 1);

         if (dam  <=    0) { vs = "miss";            vp = "misses";           }
    else if (perc <=    1) { vs = "barely scratch";  vp = "barely scratches"; }
    else if (perc <=    2) { vs = "scratch";         vp = "scratches";        }
    else if (perc <=    4) { vs = "graze";           vp = "grazes";           }
    else if (perc <=    8) { vs = "hurt";            vp = "hurts";            }
    else if (perc <=   12) { vs = "hit";             vp = "hits";             }
    else if (perc <=   16) { vs = "injure";          vp = "injures";          }
    else if (perc <=   22) { vs = "wound";           vp = "wounds";           }
    else if (perc <=   30) { vs = "damage";          vp = "damages";          }
    else if (perc <=   40) { vs = "harm";            vp = "harms";            }
    else if (perc <=   52) { vs = "tear into";       vp = "tears into";       }
//    else if (perc <=   56)66 { vs = "lacerate";        vp = "lacerates";        }
    else if (perc <=   66) { vs = "rend";            vp = "rends";            }
    else if (perc <=   81) { vs = "maul";            vp = "mauls";            }
    else if (perc <=   97) { vs = "savage";          vp = "savages";          }
    else if (perc <=  114) { vs = "mutilate";        vp = "mutilates";        }
    else if (perc <=  140) { vs = "maim";            vp = "maims";            }
    else if (perc <=  180) { vs = "mangle";          vp = "mangles";          }
    else if (perc <=  220) { vs = "devastate";       vp = "devastates";       }
    else if (perc <=  280) { vs = "dismember";       vp = "dismembers";       }
    else if (perc <=  240) { vs = "ravage";          vp = "ravages";          }
    else if (perc <   400) { vs = "sunder";          vp = "sunders";          }
    else if (perc <=  600) { vs = "kill";            vp = "kills";            }
    else if (perc <=  800) { vs = "slay";            vp = "slays";            }
    else if (perc <= 1600) { vs = "butcher";         vp = "butchers";         }
    else if (perc <= 2400) { vs = "slaughter";       vp = "slaughters";       }
    else if (perc <= 3200) { vs = "exterminate";     vp = "exterminates";     }
    else if (perc <= 4000) { vs = "destroy";         vp = "destroys";         }
    else if (perc <= 8000) { vs = "utterly destroy"; vp = "utterly destroys"; }
    else if (perc <=12000) { vs = "annihilate";      vp = "annihilates";      }
    else if (perc <=20000) { vs = "eradicate";       vp = "eradicates";       }
    else                   { vs = "obliterate";      vp = "obliterates";      }

    punct   = (dam <= 37) ? '.' : '!';

    if ( dt == TYPE_HIT && (!amess || (amess[0] == '\0')))
    {
	if (ch  == victim)
	{
	    sprintf( buf1, "$n {r%s{x $mself%c",vp,punct);
	    sprintf( buf2, "You {r%s{x yourself%c",vs,punct);
	}
	else
	{
	    sprintf( buf1, "$n {r%s{x $N%c",  vp, punct );
	    sprintf( buf2, "You {r%s{x $N%c", vs, punct );
	    sprintf( buf3, "$n {r%s{x you%c", vp, punct );
	}
    }
    else
    {
        AFFECT_DATA * aspectAff;
	if (global_damage_noun)
	   attack = global_damage_noun;
    else if (ch != NULL && is_an_avatar(ch))
	   attack = attack_table[20].noun;
    else if (ch != NULL && is_affected(ch, gsn_shadowfiend))
        attack = "raking";
    else if (ch != NULL && dt == TYPE_HIT && ((aspectAff = get_affect(ch, gsn_aspectoftheinferno)) != NULL && aspectAff->modifier == 0))
        attack = "flaming bite";
    else if (amess != NULL)
        attack = amess;
	else if ( dt >= 0 && dt < MAX_SKILL)
	    attack	= skill_table[dt].noun_damage;
	else if ( dt > TYPE_HIT )
	    attack	= ch->dam_verb;
	else
	{
	    bug( "Dam_message: bad dt %d.", dt );
	    dt  = TYPE_HIT;
	    attack  = attack_table[0].name;
	}

	if (immune)
	{
	    if (ch == victim)
	    {
		sprintf(buf1,"$n is unaffected by $s own %s.",attack);
		sprintf(buf2,"Luckily, you are immune to that.");
	    } 
	    else
	    {
	    	sprintf(buf1,"$N is unaffected by $n's %s!",attack);
	    	sprintf(buf2,"$N is unaffected by your %s!",attack);
	    	sprintf(buf3,"$n's %s is powerless against you.",attack);
	    }
	}
	else
	{
	    if (ch == victim)
	    {
		if (global_damage_from == NULL)
		    sprintf( buf1, "$n's }W%s}x%s {r%s{x $m%c", st, attack, vp, punct);
		else
		    sprintf( buf1, "%s's }W%s}x%s {r%s{x $m%c", global_damage_from, st, attack, vp, punct);
		sprintf( buf2, "Your }W%s}x%s {r%s{x you%c", st, attack, vp, punct);
	    }
	    else
	    {
                sprintf( buf2, "Your }W%s}x%s {r%s{x $N%c", st, attack, vp, punct);
		if (global_damage_from == NULL)
		{ 
	    		sprintf( buf1, "$n's }W%s}x%s {r%s{x $N%c", st, attack, vp, punct);
			sprintf( buf3, "$n's }W%s}x%s {r%s{x you%c", st, attack, vp, punct);
		}
		else
		{
			sprintf( buf1, "%s's }W%s}x%s {r%s{x $N%c", global_damage_from, st, attack,vp,punct);
			sprintf( buf3, "%s's }W%s}x%s {r%s{x you%c", global_damage_from, st, attack, vp, punct);
		}
	    }
	}
    }

    if (ch == victim)
    {
        act(buf1,ch,NULL,NULL,TO_ROOM);
        act(buf2,ch,NULL,NULL,TO_CHAR);
    }
    else
    {
        if (ch != NULL)
        {
                act( buf1, ch, NULL, victim, TO_VICTROOM );
                act( buf2, ch, NULL, victim, TO_CHAR );
                act( buf3, ch, NULL, victim, TO_VICT );
        }
        else
        {
            act( buf1, victim, NULL, victim, TO_ROOM);
            act( buf3, victim, NULL, victim, TO_CHAR);
        }
    }
}

/* for offhanddisarm skill. caller checks for success */
void offhanddisarm( CHAR_DATA *ch, CHAR_DATA *victim )
{
    OBJ_DATA *rearm;
    OBJ_DATA *obj;

    if ( ( obj = get_eq_char( victim, WEAR_DUAL_WIELD ) ) == NULL )
      if ( ( obj = get_eq_char( victim, WEAR_SHIELD ) ) == NULL )
        if ( ( obj = get_eq_char( victim, WEAR_HOLD ) ) == NULL )
          if ( ( obj = get_eq_char( victim, WEAR_LIGHT ) ) == NULL )
	    return;

// brazen: offhand disarm should check for nodisarm weapons
    if ( IS_OBJ_STAT(obj,ITEM_NOREMOVE)||IS_OBJ_STAT(obj,ITEM_NODISARM))    
    {
	act("It won't budge!",ch,NULL,victim,TO_CHAR);
	act("$n tries to strip your off hand, but $p won't budge!",
	    ch,obj,victim,TO_VICT);
	act("$n tries to strip $N's off hand, but fails.",ch,NULL,victim,TO_NOTVICT);
	return;
    }

    if ( IS_NPC(victim) && IS_SET(victim->nact, ACT_NODISARM))
    {
	act("You fail to disarm $N.", ch, NULL, victim, TO_CHAR);
	act("$n tries to disarm $N, but fails.", ch, NULL, victim, TO_NOTVICT);
	return;
    }

    act( "$n {rdisarms{x you and sends $p weapon flying!", 
	 ch, obj, victim, TO_VICT    );
    act( "You strip $N's off hand!",  ch, NULL, victim, TO_CHAR    );
    act( "$n strips $N's off hand!",  ch, NULL, victim, TO_NOTVICT );

    oprog_remove_trigger(obj);

    obj_from_char( obj );
    if ( IS_OBJ_STAT(obj,ITEM_NODROP) || IS_OBJ_STAT(obj,ITEM_INVENTORY) )
	obj_to_char( obj, victim );
    else
    {
	obj_to_room( obj, victim->in_room );
	if (IS_VALID(obj) && IS_NPC(victim) && victim->wait == 0 && can_see_obj(victim,obj))
	    get_obj(victim,obj,NULL);
    }

    if (victim->class_num == global_int_class_thief)
    {
      if ((rearm = get_eq_char(victim, WEAR_CONCEAL1)) == NULL)
        if ((rearm = get_eq_char(victim, WEAR_CONCEAL2)) == NULL)
	  return;

      act("$n slips $p from $s sleeve, rearming $mself!", victim, rearm, NULL, TO_ROOM);
      act("You slip $p from your sleeve, rearming yourself!", victim, rearm, NULL, TO_CHAR);
      unequip_char(victim, rearm);
      equip_char(victim, rearm, WORN_DUAL_WIELD);
    }

    return;
}

/*
 * Disarm a creature.
 * Caller must check for successful attack.
 */
bool disarm( CHAR_DATA *ch, CHAR_DATA *victim )
{
    OBJ_DATA *rearm;
    OBJ_DATA *obj;

    if ( ( obj = get_eq_char( victim, WEAR_WIELD ) ) == NULL )
	return FALSE;

    if ( IS_OBJ_STAT(obj,ITEM_NOREMOVE) || IS_OBJ_STAT(obj, ITEM_NODISARM))
    {
	act("$S weapon won't budge!",ch,NULL,victim,TO_CHAR);
	act("$n tries to disarm you, but your weapon won't budge!",
	    ch,NULL,victim,TO_VICT);
	act("$n tries to disarm $N, but fails.",ch,NULL,victim,TO_NOTVICT);
	return FALSE;
    }

    if ( IS_NPC(victim) && IS_SET(victim->nact, ACT_NODISARM))
    {
	act("You fail to disarm $N.", ch, NULL, victim, TO_CHAR);
	act("$n tries to disarm $N, but fails.", ch, NULL, victim, TO_NOTVICT);
	return FALSE;
    }

    act( "$n {rdisarms{x you and sends your weapon flying!", 
	 ch, NULL, victim, TO_VICT    );
    act( "You disarm $N!",  ch, NULL, victim, TO_CHAR    );
    act( "$n disarms $N!",  ch, NULL, victim, TO_NOTVICT );

    oprog_remove_trigger(obj);

    obj_from_char( obj );
    if ( IS_OBJ_STAT(obj,ITEM_NODROP) || IS_OBJ_STAT(obj,ITEM_INVENTORY) )
	obj_to_char( obj, victim );
    else
    {
	obj_to_room( obj, victim->in_room );
	if (IS_NPC(victim) && victim->wait == 0 && can_see_obj(victim,obj))
	    get_obj(victim,obj,NULL);
    }

    if (victim->class_num == global_int_class_thief)
    {
      if ((rearm = get_eq_char(victim, WEAR_CONCEAL1)) == NULL)
        if ((rearm = get_eq_char(victim, WEAR_CONCEAL2)) == NULL)
	  return TRUE;

      act("$n slips $p from $s sleeve, rearming $mself!", victim, rearm, NULL, TO_ROOM);
      act("You slip $p from your sleeve, rearming yourself!", victim, rearm, NULL, TO_CHAR);
      unequip_char(victim, rearm);
      equip_char(victim, rearm, WORN_WIELD);
    }


    return TRUE;
}

void do_berserk( CHAR_DATA *ch, char *argument)
{
    int chance, hp_percent;

    if ((chance = get_skill(ch,gsn_berserk)) == 0)
    {
	send_to_char("You turn red in the face, but nothing happens.\n\r",ch);
	return;
    }
 
    if (IS_AFFECTED(ch,AFF_BERSERK) || is_affected(ch,gsn_berserk))
    {
	send_to_char("You get a little madder.\n\r",ch);
	return;
    }

    if (IS_AFFECTED(ch,AFF_CALM))
    {
	send_to_char("You're feeling to mellow to berserk.\n\r",ch);
	return;
    }

    if (ch->mana < 50)
    {
	send_to_char("You can't get up enough energy.\n\r",ch);
	return;
    }

    /* fighting */
    if (ch->position == POS_FIGHTING)
	chance += 10;

    /* damage -- below 50% of hp helps, above hurts */
    hp_percent = 100 * ch->hit/ch->max_hit;
    chance += 25 - hp_percent/2;

    if (number_percent() < chance)
    {
	AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

	WAIT_STATE(ch,PULSE_VIOLENCE);
	expend_mana(ch, 50);

	/* heal a little damage */
	ch->hit += ch->level * 2;
	ch->hit = UMIN(ch->hit,ch->max_hit);

	send_to_char("The fury of the ancient wargods overtakes you! BLOOD!!\n\r",ch);
	act("$n screams with the fury of the ancients!",ch,NULL,NULL,TO_ROOM);
	check_improve(ch,NULL,gsn_berserk,TRUE,2);

	af.where	= TO_AFFECTS;
	af.type		= gsn_berserk;
	af.level	= ch->level;
	af.duration	= number_fuzzy(ch->level / 8);
	af.modifier	= UMAX(1,ch->level/2);
	af.bitvector 	= AFF_BERSERK;

	af.location	= APPLY_HITROLL;
	affect_to_char(ch,&af);

	af.location	= APPLY_DAMROLL;
	affect_to_char(ch,&af);

	af.modifier	= UMAX(10,10 * (ch->level/5));
	af.location	= APPLY_AC;
	affect_to_char(ch,&af);

	if (!IS_NPC(ch))
	    SET_BIT(ch->act, PLR_AUTOATTACK);

	add_event((void *) ch, 1, &event_berserk);
    }

    else
    {
	WAIT_STATE(ch,2 * PULSE_VIOLENCE);
	expend_mana(ch, 25);

	send_to_char("You get angry, but not angry enough.\n\r",ch);
	check_improve(ch,NULL,gsn_berserk,FALSE,2);
    }
}

void do_savagery( CHAR_DATA *ch, char *argument)
{
    int chance;

    if ((chance = get_skill(ch,gsn_savagery)) == 0)
    {
	send_to_char("Huh?\n\r",ch);
	return;
    }
 
    if (is_affected(ch,gsn_savagery))
    {
	send_to_char("You're already ignoring blows.\n\r",ch);
	return;
    }

    if (IS_AFFECTED(ch,AFF_CALM))
    {
	send_to_char("You feel too mellow to act tough.\n\r",ch);
	return;
    }

    if (ch->move < 25)
    {
	send_to_char("You're too tired to be tough.\n\r",ch);
	return;
    }

    /* fighting */
    if (ch->position == POS_FIGHTING)
	chance += 10;

    if (number_percent() < chance)
    {
	AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

	WAIT_STATE(ch,PULSE_VIOLENCE);
	ch->move -= 25;

	send_to_char("You forget bodily pain.\n\r",ch);
	check_improve(ch,NULL,gsn_savagery,TRUE,2);

	af.where	= TO_AFFECTS;
	af.type		= gsn_savagery;
	af.level	= ch->level;
	af.duration	= UMAX(8,round(ch->level / 4));
	af.modifier	= ch->level/7;
	af.bitvector 	= 0;
	af.location	= APPLY_DAMROLL;
	affect_to_char(ch,&af);

	af.location	= APPLY_HITROLL;
	affect_to_char(ch,&af);
    }
    else
    {
	WAIT_STATE(ch,PULSE_VIOLENCE);
	send_to_char("You try to toughen yourself against blows, but fail.\n\r",ch);
	check_improve(ch,NULL,gsn_savagery,FALSE,2);
    }
}

OBJ_DATA *check_sidestep(CHAR_DATA *ch, CHAR_DATA *victim)
{
    int chance = 0;
    OBJ_DATA *net;
    if ((chance = get_skill(victim,gsn_sidestep)) == 0)
	return NULL;
    
    if ((net = get_eq_char(victim, WEAR_HOLD)) == NULL)
	return NULL;

    if (net->item_type != ITEM_NET)
	return NULL;

    if (number_percent() < chance)
    {
	check_improve(victim,ch,gsn_sidestep,TRUE,1);
	return net;
    }
    else
    {
	check_improve(victim,ch,gsn_sidestep,FALSE,1);
	return NULL;
    }
}


void do_shieldbash( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance, kchance;
    int kdam, troll;
    OBJ_DATA *shield;
    OBJ_DATA *net = NULL;

    one_argument(argument,arg);
 
    if ( (chance = get_skill(ch,gsn_shieldbash)) == 0)
    {	
	send_to_char("Huh?\n\r",ch);
	return;
    }

    if ((shield = get_eq_char(ch, WEAR_SHIELD)) == NULL)
    {
	send_to_char("How do you propose to shield bash without a shield?\n\r", ch);
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

    if (ch->in_room == NULL)
	return;

    if (victim->position < POS_FIGHTING)
    {
	act("You'll have to let $M get back up first.",ch,NULL,victim,TO_CHAR);
	return;
    } 

    if (victim == ch)
    {
	send_to_char("You try to bash your brains out, but fail.\n\r",ch);
	return;
    }

    if (is_safe(ch,victim))
	return;

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
	act("But $N is your friend!",ch,NULL,victim,TO_CHAR);
	return;
    }

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;

    check_killer(ch,victim);

    if ((ch->in_room->sector_type == SECT_UNDERWATER) && !is_affected(ch, gsn_aquamove))
    {
        if (number_percent() <= get_skill(ch, gsn_waveborne))
            check_improve(ch, victim, gsn_waveborne, true, 4);
        else
        {
            check_improve(ch, victim, gsn_waveborne, false, 4);
        	act("$N easily evades your bash as you charge slowly through the water.", ch, NULL, victim, TO_CHAR);
        	act("$N easily evades $n's shield bash as $e charges slowly through the water.", ch, NULL, victim, TO_NOTVICT);
        	act("You easily evade $n's shield bash as $e charges slowly through the water.", ch, NULL, victim, TO_VICT);
            WAIT_STATE(ch,skill_table[gsn_shieldbash].beats/2);
        	return;
        }
    }

    /* modifiers */

    /* size  and weight */
    chance += ch->carry_weight / 250;
    chance -= victim->carry_weight / 200;

    if (ch->size < victim->size)
	chance += (ch->size - victim->size) * 15;
    else
	chance += (ch->size - victim->size) * 10; 


    /* stats */
    chance += get_curr_stat(ch,STAT_STR);
    chance -= (get_curr_stat(victim,STAT_DEX) * 4)/3;
    /* speed */
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
        chance -= 30;

    /* level */
    chance += (ch->level - victim->level);

    /* now the attack */
    chance = (UMIN(chance, 92));
    if (number_percent() < chance )
    {
        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
          sprintf( buf, "Help! %s is bashing me!", PERS(ch, victim));
          do_autoyell( victim, buf );
        }	

	if ((net = check_sidestep(ch,victim)) != NULL)
	{
	    act("$N sweeps $p into your path, slowing you down!",ch,net,victim,TO_CHAR);
	    act("$N sweeps $p into $n's path, slowing $m down!", ch, net, victim, TO_NOTVICT);
	    act("You sweep $p into $n's path, slowing $m down!",ch, net, victim, TO_VICT);
	    WAIT_STATE(ch, UMAX(ch->wait, 3*PULSE_VIOLENCE));
	    extract_obj(net);
	    set_fighting(victim, ch);
	}
	else if (is_affected(victim,gsn_protective_shield) && (number_range(0,2) == 0))
	{
	    act("Your shield bash seems to slide around $N!", 
		ch, NULL, victim, TO_CHAR);	
	    act("$n's shield bash seems to slide around $N!",
		ch, NULL, victim, TO_NOTVICT);
	    act("$n's shield bash slides around your protective shield.",
		ch, NULL, victim, TO_VICT);
            WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
	}
	else if (is_affected(victim, gsn_deflection) && number_range(0,5) == 0)
        {
            act("Your shield bash is deflected by an invisible force around $N.",
		ch, NULL, victim, TO_CHAR);
            act("$n's shield bash is deflected by an invisible force around $N.",
		ch, NULL, victim, TO_NOTVICT);
            act("$N's shield bash is deflected by an invisible force around you.",
		victim, NULL, ch, TO_CHAR);
            WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
        }
	else if (is_affected(victim, gsn_astralform) && number_bits(3) == 0)
        {
            act("Your shield bash passes through $N.",
		ch, NULL, victim, TO_CHAR);
            act("$n's shield bash passes through $N.",
		ch, NULL, victim, TO_NOTVICT);
            act("$N's shield bash passes through you.",
		victim, NULL, ch, TO_CHAR);
            WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
        }
    	else if (IS_NAFFECTED(victim, AFF_AGILITY) 
	  && (number_percent() < get_skill(victim,gsn_agility)/4+((SIZE_MEDIUM - victim->size)*10)))
        {
            act("$N spins nimbly out of the way of your shield bash.",
		ch, NULL, victim, TO_CHAR);
            act("$N spins nimbly out of the way of $n's shield bash.",
		ch, NULL, victim, TO_NOTVICT);
            act("You spin nimbly out of the way of $N's shield bash.",
		victim, NULL, ch, TO_CHAR);
            WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
        }
    	else if ((number_percent() < get_skill(victim, gsn_evade) && number_bits(2) == 0))
	{
	    act("$N evades your shield bash with an acrobatic maneuver.",
		ch,NULL,victim,TO_CHAR);	
	    act("$N evades $n's shield bash with an acrobatic maneuver.",
		ch,NULL,victim,TO_NOTVICT);
	    act("You evade $N's shield bash with an acrobatic maneuver.",
		victim,NULL,ch,TO_CHAR);
	    WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
	}
    	else if (is_affected(victim,gsn_form_of_the_panther)
	  && get_skill(victim,gsn_form_of_the_panther) > number_percent())
	{
	    act("$N sidesteps your shield bash with practiced grace.",
		ch,NULL,victim,TO_CHAR);	
	    act("$N sidesteps $n's shield bash with practiced grace.",
		ch,NULL,victim,TO_NOTVICT);
	    act("You sidestep $N's shield bash with the grace of the panther.",
		victim,NULL,ch,TO_CHAR);
	    WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
	}
	else if (is_affected(victim, gsn_anchor)
	 && ch->in_room->sector_type != SECT_WATER_NOSWIM
	 && ch->in_room->sector_type != SECT_WATER_SWIM
	 && ch->in_room->sector_type != SECT_AIR
	 && ch->in_room->sector_type != SECT_UNDERWATER)
	{
	    act("$N is firmly rooted in the ground, and $n is knocked away!", ch, NULL, victim, TO_NOTVICT);
	    act("$N is firmly rooted in the ground, and you are knocked backwards stunned!", ch, NULL, victim, TO_CHAR);
	    act("$n slams into you, but firmly rooted in the earth, you knock $m away!", ch, NULL, victim, TO_VICT);
	    WAIT_STATE(ch, UMAX(ch->wait, (2*PULSE_VIOLENCE)+2));
	}
	else if ((is_affected(victim, gsn_lessercarapace) && number_bits(1) == 0) 
		|| is_affected(victim, gsn_greatercarapace))
	{
		act("$N's hardened carapace easily withstands $n's shield bash, and $E is unbudged!", 
			ch, NULL, victim, TO_NOTVICT);
		act("Your hardened carapace easily withstands $n's shield bash, and you remain unbudged!",
			ch, NULL, victim, TO_VICT);
		act("$N's hardened carapace easily withstands your shield bash, and $E is unbudged!",
			ch, NULL, victim, TO_CHAR);
	}
	else if (is_affected(victim, gsn_barkskin) && (number_bits(2) != 0))
	{
	    act("$N's bark skin absorbs the brunt of $n's shield bash, and $N stands firm!", ch, NULL, victim, TO_NOTVICT);
	    act("$N's back skin absorbs the brunt of your shield bash, and $E stands firm!", ch, NULL, victim, TO_CHAR);
	    act("Your bark skin absorbs the brunt of $n's shield bash, and you stand firm!", ch, NULL, victim, TO_VICT);
	    WAIT_STATE(ch, UMAX(ch->wait, (2*PULSE_VIOLENCE)+2));
	}
	else if (is_affected(victim, gsn_flameshield) || is_affected(victim, gsn_lambentaura))
	{
	    if (number_bits(2) == 0)
	    {
		act("$n is scorched by $N's shield of flame!", ch, NULL, victim, TO_NOTVICT);
		act("You are scorched by $N's shield of flame!", ch, NULL, victim, TO_CHAR);
		act("$n is scorched by your shield of flame!", ch, NULL, victim, TO_VICT);
       	 	damage(victim,ch,number_range(victim->level, 2*victim->level), gsn_flameshield,DAM_FIRE,TRUE);
		if (ch->in_room != victim->in_room || IS_OAFFECTED(ch, AFF_GHOST))
		    return;

        	act("$n sends you sprawling with a powerful shield bash!", ch,NULL,victim,TO_VICT);
		act("You slam into $N with your shield, and send $M flying!",ch,NULL,victim,TO_CHAR);
		act("$n sends $N sprawling with a powerful shield bash.", ch,NULL,victim,TO_NOTVICT);
		check_improve(ch,victim,gsn_shieldbash,TRUE,1);
		DAZE_STATE(victim, (2*PULSE_VIOLENCE)+number_bits(2));
		WAIT_STATE(ch,skill_table[gsn_shieldbash].beats);
		WAIT_STATE(victim, ((2*PULSE_VIOLENCE) + number_bits(2)));
		damage(ch,victim,number_range(2,2 + 2 * ch->size + chance/20),gsn_shieldbash, DAM_BASH,FALSE);
		check_killer(ch, victim);
		return;
	    }
	    else
	    {
		act("$n is scorched by $N's shield of flame and knocked away!", ch, NULL, victim, TO_NOTVICT);
		act("You are scorched by $N's shield of flame and knocked away!", ch, NULL, victim, TO_CHAR);
		act("$n is scorched by your shield of flame and knocked away!", ch, NULL, victim, TO_VICT);
		WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE+1));
       	 	damage(victim,ch,number_range(victim->level, 2*victim->level),gsn_flameshield,DAM_FIRE,TRUE);
	    }
	}
	else if (is_affected(victim, gsn_windwall) && victim->in_room->sector_type != SECT_UNDERWATER)
	{
	    act("$N's wall of wind slides into the shield bash, knocking $n back down!", ch, NULL, victim, TO_NOTVICT);
	    act("$N's wall of wind slides into the shield bash, knocking you back down!", ch, NULL, victim, TO_CHAR);
	    act("Your wall of wind slides into the shield bash, knocking $n back down!", ch, NULL, victim, TO_VICT);
	    WAIT_STATE(ch, UMAX(ch->wait, (2*PULSE_VIOLENCE)+2));
    }
	else if (is_affected(victim, gsn_mistralward) && victim->in_room->sector_type != SECT_UNDERWATER && number_bits(1) == 0)
	{
	    act("$N's mistral ward throws $n aside, disrupting the shield bash!", ch, NULL, victim, TO_NOTVICT);
	    act("$N's mistral ward throws you aside, disrupting the shield bash!", ch, NULL, victim, TO_CHAR);
	    act("Your mistral ward throws $n aside, disrupting the shield bash!", ch, NULL, victim, TO_VICT);
	    WAIT_STATE(ch, UMAX(ch->wait, (2*PULSE_VIOLENCE)));
    }
	else if (get_eq_char(victim,WEAR_WIELD)
	  && get_skill(victim,gsn_retaliation) / 3 > number_percent())
	{
	    int damage = 0;
	    OBJ_DATA *weapon = get_eq_char(victim,WEAR_WIELD);
	    if (weapon != NULL)
	    {
		act("$N sidesteps your bash, striking you with $s weapon!",ch,NULL, victim, TO_CHAR);
		act("$N sidesteps $n's bash, striking $m with $s weapon!",ch,NULL,victim,TO_NOTVICT);
	        act("You sidestep $n's bash, striking $m with your weapon!",ch,NULL,victim,TO_VICT);
	        WAIT_STATE(ch, UMAX(ch->wait, (2*PULSE_VIOLENCE)));
	        check_improve(victim,ch,gsn_retaliation,TRUE,1);
		damage = dice(weapon->value[1], weapon->value[2]) + GET_DAMROLL(victim);
		damage += check_extra_damage(victim,damage,weapon);
		damage_from_obj(victim,ch,weapon,damage,gsn_retaliation,weapon->value[3],TRUE);
		wrathkyana_combat_effect(victim,ch);
	    }
	}
	else
	{
            act("$n sends you sprawling with a powerful shield bash!", ch,NULL,victim,TO_VICT);
	    act("You slam into $N with your shield, and send $M flying!",ch,NULL,victim,TO_CHAR);
	    act("$n sends $N sprawling with a powerful shield bash.", ch,NULL,victim,TO_NOTVICT);
	    check_improve(ch,victim,gsn_shieldbash,TRUE,1);

	    WAIT_STATE(ch,skill_table[gsn_bash].beats);

	    if ((troll = number_percent()) <= (get_skill(victim, gsn_tumbling) * 4 / 5))
	    {
		act("You tumble away from the brunt of the bash!", ch, NULL, victim, TO_VICT);
		act("$N tumbles away from the brunt of your bash!", ch, NULL, victim, TO_CHAR);
		act("$N tumbles away from the brunt of the bash!", ch, NULL, victim, TO_NOTVICT);

		DAZE_STATE(victim, ((2 * PULSE_VIOLENCE) * (100 - troll)) / 100);
		WAIT_STATE(victim, ((2 * PULSE_VIOLENCE) * (100 - troll)) / 100);
	    }
	    else
	    {
		DAZE_STATE(victim, (2*PULSE_VIOLENCE)+number_bits(2));
		WAIT_STATE(victim, ((2*PULSE_VIOLENCE) + number_bits(2) - 1));
	    }

            /* Check for kidney punch */
            if ((kchance = get_skill(victim, gsn_kidneyshot)) > 0)
            {
	        kchance = ((kchance * 2)/3);
                if (number_percent() < kchance)
	        { 
		    if (is_affected(ch,gsn_diamondskin) && IS_SET(ch->imm_flags, IMM_WEAPON))
		    {
		        act("$n's diamond skin prevents your kidney shot from being effective.", ch, NULL, victim, TO_VICT);
		        act("Your diamond skin stops $N's kidney shot.", ch, NULL, victim, TO_CHAR);
			return;
		    }	
		    act("You double over in pain as $N delivers a quick shot to your kidney.", ch, NULL, victim, TO_CHAR);
		    act("$n doubles over in pain as you quickly jab $m in the kidney.", ch, NULL, victim, TO_VICT);
		    act("$n doubles over in pain as $N delivers a quick shot to $s kidney.", ch, NULL, victim, TO_NOTVICT);
                    kdam = number_range( 1 + get_skill(victim,gsn_kidneyshot)/10, 2 * victim->level/3 * get_skill(victim,gsn_kidneyshot)/200);  
                    kdam += check_extra_damage(victim,kdam,get_eq_char(victim,WEAR_WIELD));
                    kdam += GET_DAMROLL(victim) * UMIN(100,get_skill(victim,gsn_kidneyshot)/2) /100;
		    kdam = round(kdam * 1.2);
                    damage(victim,ch,kdam,gsn_kidneyshot,DAM_OTHER,TRUE);
		    if (number_percent() < (get_skill(victim, gsn_kidneyshot) / 3))
		        ch->wait += (PULSE_VIOLENCE * 2);
		    else
		        ch->wait += PULSE_VIOLENCE;
		    check_improve(victim,ch,gsn_kidneyshot,TRUE,1);
	        }
	        else
		    check_improve(victim,ch,gsn_kidneyshot,FALSE,1);
	    }
	    damage(ch,victim,number_range(2,2 + 2 * ch->size + chance/20),gsn_shieldbash, DAM_BASH,FALSE);
	    if (obj_is_affected(shield, gsn_jawsofidcizon) && !is_affected(ch, gsn_roaroftheexalted))
	    {
		act("You cringe in pain as the jaws of Idcizon bite you.", ch, NULL, victim, TO_VICT);
		act("$n cringes in pain as the jaws of Idcizon bite into $m.", victim, NULL, NULL, TO_ROOM);
		damage(ch, victim, number_range(1, shield->level), gsn_jawsofidcizon, DAM_PIERCE, TRUE);
	    }
	    check_killer(ch, victim);
	    return;
	}

	set_fighting(victim, ch);
    }
    else
    {
        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
          sprintf( buf, "Help!  %s is trying to shield bash me!", PERS(ch, victim) );
          do_autoyell( victim, buf );
        }	
        damage(ch,victim,0,gsn_shieldbash,DAM_BASH,FALSE);
	act("You fall flat on your face!", ch,NULL,victim,TO_CHAR);
	act("$n falls flat on $s face.", ch,NULL,victim,TO_NOTVICT);
	act("You evade $n's bash, causing $m to fall flat on $s face.", ch,NULL,victim,TO_VICT);
	check_improve(ch,victim,gsn_shieldbash,FALSE,1);
//	switch_position(ch, POS_RESTING);
	WAIT_STATE(ch,skill_table[gsn_shieldbash].beats * 3/2); 
    }
}

void do_bash( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance, kchance;
    int kdam, troll;
    OBJ_DATA *net = NULL;
    one_argument(argument,arg);
 
    if ((chance = get_skill(ch,gsn_bash)) == 0)
    {	
	send_to_char("Huh?\n\r",ch);
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

    if (ch->in_room == NULL)
	return;

    if (victim->position < POS_FIGHTING)
    {
	act("You'll have to let $M get back up first.",ch,NULL,victim,TO_CHAR);
	return;
    } 

    if (victim == ch)
    {
	send_to_char("You try to bash your brains out, but fail.\n\r",ch);
	return;
    }

    if (is_safe(ch,victim))
	return;

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
	act("But $N is your friend!",ch,NULL,victim,TO_CHAR);
	return;
    }

    if ((ch->in_room->sector_type == SECT_UNDERWATER) && !is_affected(ch, gsn_aquamove))
    {
        if (number_percent() <= get_skill(ch, gsn_waveborne))
            check_improve(ch, victim, gsn_waveborne, true, 4);
        else
        {
            check_improve(ch, victim, gsn_waveborne, false, 4);
            act("$n flounders comically in the water, trying to bash you.",ch,NULL,victim,TO_VICT);
            act("You flail wildly in the water, trying in vain to bash $N.",ch,NULL,victim,TO_CHAR);
            act("$n flail wildly in the water, trying in vain to bash $N.",ch,NULL,victim,TO_NOTVICT);
            WAIT_STATE(ch,skill_table[gsn_bash].beats/2);
	        return;
        }
    }

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;

    /* modifiers */

    /* size  and weight */
    chance += ch->carry_weight / 250;
    chance -= victim->carry_weight / 200;

    if (ch->size < victim->size)
	chance += (ch->size - victim->size) * 15;
    else
	chance += (ch->size - victim->size) * 10; 


    /* stats */
    chance += get_curr_stat(ch,STAT_STR);
    chance -= (get_curr_stat(victim,STAT_DEX) * 4)/3;
    /* speed */
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
        chance -= 30;

    /* level */
    chance += (ch->level - victim->level);

    /* now the attack */
    chance = (UMIN(chance, 92));
    if (number_percent() < chance )
    {
        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
	    sprintf( buf, "Help! %s is bashing me!", PERS(ch, victim));
            do_autoyell( victim, buf );
        }	

	if ((net = check_sidestep(ch,victim)) != NULL)
	{
	    act("$N sweeps $p into your path, slowing you down!",ch,net,victim,TO_CHAR);
	    act("$N sweeps $p into $n's path, slowing $m down!", ch, net, victim, TO_NOTVICT);
	    act("You sweep $p into $n's path, slowing $m down!",ch, net, victim, TO_VICT);
	    WAIT_STATE(ch, UMAX(ch->wait, 3*PULSE_VIOLENCE));
	    set_fighting(victim, ch);
	    extract_obj(net);
	}
	else if (is_affected(victim,gsn_protective_shield) && (number_bits(3) != 0))
	{
	    act("Your bash seems to slide around $N!", ch,NULL,victim,TO_CHAR);	
	    act("$n's bash seems to slide around $N!", ch,NULL,victim,TO_NOTVICT);
	    act("$N's bash slides around your protective shield.", victim,NULL,ch,TO_CHAR);
            WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
	    set_fighting(victim, ch);
	}
        else if (IS_NAFFECTED(victim, AFF_AGILITY) 
	  && (number_percent() < get_skill(victim,gsn_agility)/2+((SIZE_MEDIUM - victim->size)*10)))
        {
            act("$N spins nimbly out of the way of your bash.",
		ch, NULL, victim, TO_CHAR);
            act("$N spins nimbly out of the way of $n's bash.",
		ch, NULL, victim, TO_NOTVICT);
            act("You spin nimbly out of the way of $N's bash.",
		victim, NULL, ch, TO_CHAR);
            WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
	    set_fighting(victim, ch);
        }
        else if (is_affected(victim,gsn_astralform) && (number_bits(1) != 0))
        {
            act("Your bash passes right through $N!", ch,NULL,victim,TO_CHAR);
            act("$n tries to bash $N, but passes right through $M!", ch,NULL,victim,TO_NOTVICT);
            act("$N's bash passes right through you!", victim,NULL,ch,TO_CHAR);
            WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
	    set_fighting(victim, ch);
        }
    	else if ((number_percent() < get_skill(victim, gsn_evade) && number_bits(2) == 0))
	{
	    act("$N evades your bash with an acrobatic maneuver.", ch,NULL,victim,TO_CHAR);	
	    act("$N evades $n's bash with an acrobatic maneuver.", ch,NULL,victim,TO_NOTVICT);
	    act("You evade $N's bash with an acrobatic maneuver.", victim,NULL,ch,TO_CHAR);
	    WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
	    set_fighting(victim, ch);
	}
    	else if (is_affected(victim,gsn_form_of_the_panther)
	  && get_skill(victim,gsn_form_of_the_panther) > number_percent())
	{
	    act("$N sidesteps your bash with practiced grace.", ch,NULL,victim,TO_CHAR);	
	    act("$N sidesteps $n's bash with practiced grace.", ch,NULL,victim,TO_NOTVICT);
	    act("You sidestep $N's bash with the grace of the panther.", victim,NULL,ch,TO_CHAR);
	    WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
	    set_fighting(victim, ch);
	}
	else if (is_affected(victim, gsn_anchor)
	&& ch->in_room->sector_type != SECT_WATER_NOSWIM
	&& ch->in_room->sector_type != SECT_WATER_SWIM
	&& ch->in_room->sector_type != SECT_AIR
	&& ch->in_room->sector_type != SECT_UNDERWATER)
	{
	    act("$N is firmly rooted in the ground, and $n is knocked away!", ch, NULL, victim, TO_NOTVICT);
	    act("$N is firmly rooted in the ground, and you are knocked backwards stunned!", ch, NULL, victim, TO_CHAR);
	    act("$n slams into you, but firmly rooted in the earth, you knock $m away!", ch, NULL, victim, TO_VICT);
	    WAIT_STATE(ch, UMAX(ch->wait, (2*PULSE_VIOLENCE)+2));
	    set_fighting(victim, ch);
	}
	else if ((is_affected(victim, gsn_lessercarapace) && number_bits(1) == 0)
	|| is_affected(victim, gsn_greatercarapace))
	{
		act("$N's hardened carapace easily withstands $n's bash, and $E is unbudged!",
			ch, NULL, victim, TO_NOTVICT);
		act("Your hardened carapace easily withstands $n's bash, and you remain unbudged!",
			ch, NULL, victim, TO_VICT);
		act("$N's hardened carapace easily withstands your bash, and $E is unbudged!",
			ch, NULL, victim, TO_CHAR);
		set_fighting(victim, ch);
	}
	else if (is_affected(victim, gsn_deflection) && (number_bits(1) == 0))
	{
	    send_to_char("Your bash is deflected away by an invisible force!\n\r", ch);
	    act("You mentally deflect $n's powerful bash!", ch, NULL, victim, TO_VICT);
	    act("$n's attempt to bash $N is deflected away!", ch, NULL, victim, TO_NOTVICT);
	WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE));
	    set_fighting(victim, ch);
	}
	else if (is_affected(victim, gsn_barkskin) && (number_bits(2) != 0))
	{
	    act("$N's bark skin absorbs the brunt of $n's bash, and $N stands firm!", ch, NULL, victim, TO_NOTVICT);
	    act("$N's back skin absorbs the brunt of your bash, and $E stands firm!", ch, NULL, victim, TO_CHAR);
	    act("Your bark skin absorbs the brunt of $n's bash, and you stand firm!", ch, NULL, victim, TO_VICT);
	    WAIT_STATE(ch, UMAX(ch->wait, (2*PULSE_VIOLENCE)+2));
	    set_fighting(victim, ch);
	}
	else if (is_affected(victim, gsn_flameshield) || is_affected(victim, gsn_lambentaura))
	{
	    if (number_bits(2) == 0)
	    {
		act("$n is scorched by $N's shield of flame!", ch, NULL, victim, TO_NOTVICT);
		act("You are scorched by $N's shield of flame!", ch, NULL, victim, TO_CHAR);
		act("$n is scorched by your shield of flame!", ch, NULL, victim, TO_VICT);
       	 	damage(victim,ch,number_range(victim->level, 2*victim->level),gsn_flameshield,DAM_FIRE,TRUE);
		if (ch->in_room != victim->in_room || IS_OAFFECTED(ch, AFF_GHOST))
		    return;
        	act("$n sends you sprawling with a powerful bash!", ch,NULL,victim,TO_VICT);
		act("You slam into $N, and send $M flying!",ch,NULL,victim,TO_CHAR);
		act("$n sends $N sprawling with a powerful bash.", ch,NULL,victim,TO_NOTVICT);
		check_improve(ch,victim,gsn_bash,TRUE,1);
		DAZE_STATE(victim, (2*PULSE_VIOLENCE)+number_bits(2));
		WAIT_STATE(ch,skill_table[gsn_bash].beats);
		WAIT_STATE(victim, ((2*PULSE_VIOLENCE) + number_bits(2)));
		damage(ch,victim,number_range(2,2 + 2 * ch->size + chance/20),gsn_bash,	DAM_BASH,FALSE);
	    }
	    else
	    {
		act("$n is scorched by $N's shield of flame and knocked away!", ch, NULL, victim, TO_NOTVICT);
		act("You are scorched by $N's shield of flame and knocked away!", ch, NULL, victim, TO_CHAR);
		act("$n is scorched by your shield of flame and knocked away!", ch, NULL, victim, TO_VICT);
		WAIT_STATE(ch, UMAX(ch->wait, 2*PULSE_VIOLENCE+1));
       		damage(victim,ch,number_range(victim->level, 2*victim->level),gsn_flameshield,DAM_FIRE,TRUE);
	    }
	    set_fighting(victim, ch);
	}
	else if ((is_affected(victim, gsn_windwall)) && (victim->in_room->sector_type != SECT_UNDERWATER))
	{
	    act("$N's wall of wind slides into the bash, knocking $n back down!", ch, NULL, victim, TO_NOTVICT);
	    act("$N's wall of wind slides into the bash, knocking you back down!", ch, NULL, victim, TO_CHAR);
	    act("Your wall of wind slides into the bash, knocking $n back down!", ch, NULL, victim, TO_VICT);
	    WAIT_STATE(ch, UMAX(ch->wait, (2*PULSE_VIOLENCE)+2));
	    set_fighting(victim, ch);
	}
	else if (is_affected(victim, gsn_mistralward) && victim->in_room->sector_type != SECT_UNDERWATER && number_bits(1) == 0)
	{
	    act("$N's mistral ward throws $n aside, disrupting the bash!", ch, NULL, victim, TO_NOTVICT);
	    act("$N's mistral ward throws you aside, disrupting the bash!", ch, NULL, victim, TO_CHAR);
	    act("Your mistral ward throws $n aside, disrupting the bash!", ch, NULL, victim, TO_VICT);
	    WAIT_STATE(ch, UMAX(ch->wait, (2*PULSE_VIOLENCE)));
        set_fighting(victim, ch);
    }
	else if (get_eq_char(victim,WEAR_WIELD) 
	  && number_percent() < (get_skill(victim,gsn_retaliation)/3))
	{
	    int damage = 0;
	    OBJ_DATA *weapon = get_eq_char(victim,WEAR_WIELD);
	    if (weapon != NULL)
	    {
		act("$N sidesteps your bash, striking you with $s weapon!",ch,NULL, victim, TO_CHAR);
		act("$N sidesteps $n's bash, striking $m with $s weapon!",ch,NULL,victim,TO_NOTVICT);
	        act("You sidestep $n's bash, striking $m with your weapon!",ch,NULL,victim,TO_VICT);
	        WAIT_STATE(ch, UMAX(ch->wait, (2*PULSE_VIOLENCE)));
	        check_improve(victim,ch,gsn_retaliation,TRUE,1);
		damage = dice(weapon->value[1], weapon->value[2]) + GET_DAMROLL(victim);
		damage += check_extra_damage(victim,damage,weapon);
		damage_from_obj(victim,ch,weapon,damage,gsn_retaliation,weapon->value[3],TRUE);
		wrathkyana_combat_effect(victim,ch);
	    }
	}
	else
	{
	    act("$n sends you sprawling with a powerful bash!",	ch,NULL,victim,TO_VICT);
	    act("You slam into $N, and send $M flying!",ch,NULL,victim,TO_CHAR);
	    act("$n sends $N sprawling with a powerful bash.", ch,NULL,victim,TO_NOTVICT);
	    check_improve(ch,victim,gsn_bash,TRUE,1);

	    WAIT_STATE(ch,skill_table[gsn_bash].beats);

	    if ((troll = number_percent()) <= (get_skill(victim, gsn_tumbling) * 4 / 5))
	    {
		act("You tumble away from the brunt of the bash!", ch, NULL, victim, TO_VICT);
		act("$N tumbles away from the brunt of your bash!", ch, NULL, victim, TO_CHAR);
		act("$N tumbles away from the brunt of the bash!", ch, NULL, victim, TO_NOTVICT);

		DAZE_STATE(victim, ((2 * PULSE_VIOLENCE) * (100 - troll)) / 100);
		WAIT_STATE(victim, ((2 * PULSE_VIOLENCE) * (100 - troll)) / 100);
	    }
	    else
	    {
		DAZE_STATE(victim, (2*PULSE_VIOLENCE)+number_bits(2));
		WAIT_STATE(victim, ((2*PULSE_VIOLENCE) + number_bits(2) - 1));
	    }

	    /* Check for kidney punch */
            if ((kchance = get_skill(victim, gsn_kidneyshot)) > 0)
            {
	        kchance = ((kchance * 2)/3);
                if (number_percent() < kchance)
	        {
		    if (is_affected(ch,gsn_diamondskin) && IS_SET(ch->imm_flags, IMM_WEAPON))
		    {
		        act("$n's diamond skin prevents your kidney shot from being effective.", ch, NULL, victim, TO_VICT);
		        act("Your diamond skin stops $N's kidney shot.", ch, NULL, victim, TO_CHAR);
		        return;
		    }	
		    act("You double over in pain as $N delivers a quick shot to your kidney.", ch, NULL, victim, TO_CHAR);
		    act("$n doubles over in pain as you quickly jab $m in the kidney.", ch, NULL, victim, TO_VICT);
		    act("$n doubles over in pain as $N delivers a quick shot to $s kidney.", ch, NULL, victim, TO_NOTVICT);
                    kdam = number_range( 1 + get_skill(victim,gsn_kidneyshot)/10, 2 * victim->level/3 * get_skill(victim,gsn_kidneyshot)/200);  
                    kdam += check_extra_damage(victim,kdam,get_eq_char(victim,WEAR_WIELD));
                    kdam += GET_DAMROLL(victim) * UMIN(100,get_skill(victim,gsn_kidneyshot)/2) /100;
		    kdam = round(kdam * 1.2);
                    damage(victim,ch,kdam,gsn_kidneyshot,DAM_OTHER,TRUE);
		    if (number_percent() < (get_skill(victim, gsn_kidneyshot) / 3))
		        ch->wait += (PULSE_VIOLENCE * 2);
		    else
		        ch->wait += PULSE_VIOLENCE;
		    check_improve(victim,ch,gsn_kidneyshot,TRUE,1);
	        }
	        else
		    check_improve(victim,ch,gsn_kidneyshot,FALSE,1);
	    }
	    damage(ch,victim,number_range(2,2 + 2 * ch->size + chance/20),gsn_bash, DAM_BASH,FALSE);
	}
    }
    else
    {
        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
          sprintf( buf, "Help!  %s is trying to bash me!", PERS(ch, victim) );
          do_autoyell( victim, buf );
        }	
        damage(ch,victim,0,gsn_bash,DAM_BASH,FALSE);
	act("You fall flat on your face!",
	    ch,NULL,victim,TO_CHAR);
	act("$n falls flat on $s face.",
	    ch,NULL,victim,TO_NOTVICT);
	act("You evade $n's bash, causing $m to fall flat on $s face.",
	    ch,NULL,victim,TO_VICT);
	check_improve(ch,victim,gsn_bash,FALSE,1);
//	switch_position(ch, POS_RESTING);
	WAIT_STATE(ch,skill_table[gsn_bash].beats * 3/2); 
    }
    check_killer(ch,victim);
    return;
}



void do_flashpowder( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    /*char buf[MAX_STRING_LENGTH]; * no yell, no buf */
    CHAR_DATA *victim;
    int chance;

    if ( (chance = get_skill(ch,gsn_flashpowder)) == 0)
    {
	send_to_char("Huh?\n\r",ch);
	return;
    }

    if (IS_NAFFECTED(ch, AFF_FLASHPOWDER))
    {
	send_to_char("You are not ready to mix flash powder again yet.\n\r", ch);
	return;
    }

    if (ch->mana < skill_table[gsn_flashpowder].min_mana)
    {
	send_to_char("You are too tired to throw flash powder.\n\r", ch);
	return;
    }

    if (number_percent() > chance)
    {
	act("$n throws something down, but nothing happens.", ch, NULL, NULL, TO_ROOM);
	act("You throw down flash powder, but it fails to explode.", ch, NULL, NULL, TO_CHAR);
	check_improve(ch,NULL,gsn_flashpowder,TRUE,2);
	WAIT_STATE(ch,skill_table[gsn_flashpowder].beats);
	expend_mana(ch, skill_table[gsn_flashpowder].min_mana);
	return;
    }
    else
    {
	check_improve(ch,NULL,gsn_flashpowder,TRUE,2);
	WAIT_STATE(ch,skill_table[gsn_flashpowder].beats);
	send_to_char("Fiery light fills the area as your powder hits the ground!\n\r", ch);
	act("$n throws something down, and fiery light fills the area!", ch, NULL, NULL, TO_ROOM);
    }

    /******* ok, loop and see who gets blinded ****/
    for (victim = ch->in_room->people; victim != NULL; victim = victim->next_in_room)
    {
	if (ch == victim || is_safe_spell(ch, victim, TRUE) || IS_SET(victim->imm_flags, IMM_BLIND) || victim->position <= POS_SLEEPING)
	    continue;

	if (!saves_spell(ch->level+4, ch, victim, DAM_OTHER))
        {
	    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

            act("$n is blinded by the flash!",victim,NULL,NULL,TO_ROOM);
	    send_to_char("You can't see a thing!\n\r",victim);

	    if (!IS_AFFECTED(victim, AFF_BLIND))
	    {
		af.where	= TO_AFFECTS;
		af.type 	= gsn_flashpowder;
		af.level 	= ch->level;
		af.duration	= number_bits(1) == 0 ? 4 : 3;
		af.location	= APPLY_HITROLL;
		af.modifier	= -4;
		af.bitvector 	= AFF_BLIND;

		affect_to_char(victim,&af);
	    }
        }
    } 

    af.where	= TO_NAFFECTS;
    af.type 	= gsn_flashpowder;
    af.level 	= ch->level;
    af.duration	= 1;
    af.location	= 0;
    af.modifier	= 0;
    af.bitvector = AFF_FLASHPOWDER;
    affect_to_char(ch, &af);

}

void do_dirt( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;

    one_argument(argument,arg);

    if ((chance = get_skill(ch,gsn_dirt)) == 0)
//    ||   (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_KICK_DIRT)))
    {
	send_to_char("You get your feet dirty.\n\r",ch);
	return;
    }

    if (ch->in_room->sector_type == SECT_UNDERWATER || ch->in_room->sector_type == SECT_WATER_SWIM || ch->in_room->sector_type == SECT_WATER_NOSWIM)
    {
      send_to_char("You can't kick dirt in water.\n\r",ch);
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

    if (IS_AFFECTED(victim,AFF_BLIND))
    {
	act("$E's already been blinded.",ch,NULL,victim,TO_CHAR);
	return;
    }

    if (IS_SET(victim->imm_flags, IMM_BLIND))
    {
	send_to_char("You failed.\n\r", ch);
	return;
    }


    if (victim == ch)
    {
	send_to_char("Very funny.\n\r",ch);
	return;
    }

    if (is_safe(ch,victim))
	return;

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
	act("But $N is such a good friend!",ch,NULL,victim,TO_CHAR);
	return;
    }

    /* dexterity */
    chance += get_curr_stat(ch,STAT_DEX);
    chance -= 2 * get_curr_stat(victim,STAT_DEX);

    /* speed  */
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
	chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
	chance -= 25;

    /* level */
    chance += (ch->level - victim->level) * 2;

    /* sloppy hack to prevent false zeroes */
    if (chance % 5 == 0)
	chance += 1;

    /* terrain */

    switch(ch->in_room->sector_type)
    {
	case(SECT_INSIDE):		chance -= 20;	break;
	case(SECT_CITY):		chance -= 10;	break;
	case(SECT_FIELD):		chance +=  5;	break;
	case(SECT_SWAMP):		chance -= 10;   break;
	case(SECT_FOREST):				break;
	case(SECT_HILLS):				break;
	case(SECT_MOUNTAIN):		chance -= 10;	break;
	case(SECT_WATER_SWIM):		chance  =  0;	break;
	case(SECT_WATER_NOSWIM):	chance  =  0;	break;
	case(SECT_AIR):			chance  =  0;  	break;
	case(SECT_DESERT):		chance += 10;   break;
    }

    if (chance == 0)
    {
	send_to_char("There isn't any dirt to kick.\n\r",ch);
	return;
    }

    if (IS_SET(victim->imm_flags, IMM_BLIND))
	chance = 0;

    /* now the attack */
    if (number_percent() < chance)
    {
	AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
          sprintf( buf, "I can't see! Someone kicked dirt in my eyes!");
          do_autoyell( victim, buf );
        }	
        act("$n is blinded by the dirt in $s eyes!",victim,NULL,NULL,TO_ROOM);
	act("$n kicks dirt in your eyes!",ch,NULL,victim,TO_VICT);
        damage(ch,victim,number_range(2,5),gsn_dirt,DAM_NONE,FALSE);
	send_to_char("You can't see a thing!\n\r",victim);
	check_improve(ch,victim,gsn_dirt,TRUE,2);
	WAIT_STATE(ch,skill_table[gsn_dirt].beats);

	af.where	= TO_AFFECTS;
	af.type 	= gsn_dirt;
	af.level 	= ch->level;
	af.duration	= ch->in_room->sector_type == SECT_DESERT ? 1 : 0;
	af.location	= APPLY_HITROLL;
	af.modifier	= -4;
	af.bitvector 	= AFF_BLIND;

	affect_to_char(victim,&af);
    }
    else
    {
        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
          sprintf( buf, "Help!  %s tried to kick dirt in my eyes!", PERS(ch, victim) );
          do_autoyell( victim, buf );
        }	
        damage(ch,victim,0,gsn_dirt,DAM_NONE,TRUE);
	check_improve(ch,victim,gsn_dirt,FALSE,2);
	WAIT_STATE(ch,skill_table[gsn_dirt].beats);
    }
	check_killer(ch,victim);
}



void do_tail( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;

    one_argument(argument,arg);
 
    if ( ((chance = get_skill(ch,gsn_tail)) == 0)
    ||  (!(IS_SET(ch->parts, PART_TAIL)))
    ||	 (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_TAIL)))
    {	
	send_to_char("You haven't got a tail to swipe with.\n\r",ch);
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

    if (victim == ch)
    {
	send_to_char("You play with your tail.\n\r",ch);
	return;
    }

    if (is_safe(ch,victim))
	return;

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
	act("But $N is your friend!",ch,NULL,victim,TO_CHAR);
	return;
    }

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;

    /* modifiers */

    /* size  and weight */
    chance += ch->carry_weight / 300;
    chance -= victim->carry_weight / 200;

    if (ch->size < victim->size)
	chance += (ch->size - victim->size) * 15;
    else
	chance += (ch->size - victim->size) * 10; 


    /* stats */
    chance += get_curr_stat(ch,STAT_STR);
    chance -= (get_curr_stat(victim,STAT_DEX) * 4)/3;
    chance -= GET_AC(victim,AC_BASH) /25;
    /* speed */
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
        chance -= 30;

    /* level */
    chance += (ch->level - victim->level);

    if (!IS_NPC(victim) 
	&& chance < get_skill(victim,gsn_dodge) )
    {
	chance -= 3 * (get_skill(victim,gsn_dodge) - chance);
    }

    /* now the attack */
    if (number_percent() < chance )
    {
        act("$n lashes $s tail at you!",
		ch,NULL,victim,TO_VICT);
	act("You swipe at $N with your tail!",ch,NULL,victim,TO_CHAR);
	act("$n swipes at $N with $s tail!",
		ch,NULL,victim,TO_NOTVICT);
	check_improve(ch,victim,gsn_tail,TRUE,1);

	DAZE_STATE(victim, 1 * PULSE_VIOLENCE);
	WAIT_STATE(ch,skill_table[gsn_tail].beats);
	damage(ch,victim,number_range(ch->level/10,
		ch->level/10 * ch->size + chance/20),gsn_tail,
	    DAM_BASH,TRUE);
    }
    else
    {
	act("Your tail swings wildly, missing $N by a mile!",
	    ch,NULL,victim,TO_CHAR);
	act("$n misses $N with $s tail swipe.",
	    ch,NULL,victim,TO_NOTVICT);
	act("You evade $n's tail swipe.",
	    ch,NULL,victim,TO_VICT);
        damage(ch,victim,0,gsn_tail,DAM_BASH,TRUE);
	check_improve(ch,victim,gsn_tail,FALSE,1);
	WAIT_STATE(ch,skill_table[gsn_tail].beats * 3/2); 
    }
	check_killer(ch,victim);
}


void do_bite( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;

    one_argument(argument,arg);
 
    if (((chance = get_skill(ch,gsn_bite)) == 0)
    ||  (!(IS_SET(ch->parts, PART_FANGS)))
    ||	 (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_BITE)))
    {	
	send_to_char("You bite at the air.\n\r",ch);
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

    if (victim == ch)
    {
	send_to_char("You bite your tongue.\n\r",ch);
	return;
    }

    if (is_safe(ch,victim))
	return;

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
	act("But $N is your friend!",ch,NULL,victim,TO_CHAR);
	return;
    }

    if (!check_reach(ch, victim, REACH_EASY, PULSE_VIOLENCE))
	return;

    /* modifiers */

    /* size  and weight */
    chance -= ch->carry_weight / 300;
    chance += victim->carry_weight / 200;

    /* stats */
    chance += get_curr_stat(ch,STAT_DEX);
    chance -= (get_curr_stat(victim,STAT_DEX) * 4)/3;
    chance -= GET_AC(victim,AC_SLASH) /25;
    /* speed */
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
        chance -= 30;

    /* level */
    chance += (ch->level - victim->level);

    if (!IS_NPC(victim) 
	&& chance < get_skill(victim,gsn_dodge) )
    {	
	chance -= 3 * (get_skill(victim,gsn_dodge) - chance);
    }

    /* now the attack */
    if (number_percent() < chance )
    {
        act("$n bites at you with $s teeth!",
		ch,NULL,victim,TO_VICT);
	act("You bite $N!",ch,NULL,victim,TO_CHAR);
	act("$n bites $N!",
		ch,NULL,victim,TO_NOTVICT);
	check_improve(ch,victim,gsn_tail,TRUE,1);

	DAZE_STATE(victim, 1 * PULSE_VIOLENCE);
	WAIT_STATE(ch,skill_table[gsn_bite].beats);
	damage(ch,victim,number_range(ch->level/2,
		ch->level + chance/20),gsn_bite,
	    DAM_PIERCE,TRUE);
    }
    else
    {
	act("Your bite wildly in the air, missing $N by a mile!",
	    ch,NULL,victim,TO_CHAR);
	act("$n's teeth gnashes $s teeth at $N.",
	    ch,NULL,victim,TO_NOTVICT);
	act("You evade $n's bite.",
	    ch,NULL,victim,TO_VICT);
        damage(ch,victim,0,gsn_bite,DAM_PIERCE,TRUE);
	check_improve(ch,victim,gsn_bite,FALSE,1);
	WAIT_STATE(ch,skill_table[gsn_bite].beats * 3/2); 
    }
	check_killer(ch,victim);
}


void do_legsweep( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *weapon;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;

    one_argument(argument,arg);

    if ( (chance = get_skill(ch,gsn_legsweep)) == 0)
    {
	send_to_char("Huh?\n\r",ch);
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

    if (is_flying(victim))
    {
        send_to_char("You cannot legsweep someone who is flying.\n\r",ch);
        return;
    }

    if (is_stabilized(*victim))
    {
        act("$N looks too stable to trip.",ch,NULL,victim,TO_CHAR);
        return;
    }

    if (is_safe(ch,victim))
	return;

    if ((weapon = get_eq_char(ch, WEAR_WIELD))==NULL)
    {
	send_to_char("You must use a polearm or spear in your main hand to leg sweep someone.\n\r", ch);
	return;
    }

    if ((weapon->value[0] != WEAPON_SPEAR) && (weapon->value[0] != WEAPON_POLEARM))
    {
	send_to_char("You must use a polearm or spear in your main hand to leg sweep someone.\n\r", ch);
	return;
    }

    if (is_flying(victim) && !is_affected(victim, gsn_earthbind))
	  chance /= 2;

    if (victim->position < POS_FIGHTING)
    {
	act("$N is already down.",ch,NULL,victim,TO_CHAR);
	return;
    }

    if (victim == ch)
    {
	send_to_char("You knock your feet out from under you!\n\r", ch);
	WAIT_STATE(ch, UMAX(2*PULSE_VIOLENCE, ch->wait));
	return;
    }

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
	act("$N is your beloved master.",ch,NULL,victim,TO_CHAR);
	return;
    }

    if (!check_reach(ch, victim, REACH_EASY, PULSE_VIOLENCE))
	return;

    /* modifiers */

    /* dex */
    chance += get_curr_stat(ch,STAT_DEX);
    chance -= get_curr_stat(victim,STAT_DEX) * 3 / 2;

    /* speed */
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
	chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
	chance -= 20;

    /* level */
    chance += (ch->level - victim->level) * 2;

    if (IS_NAFFECTED(victim, AFF_AGILITY) 
	  && (number_percent() < get_skill(victim,gsn_agility)/2+((SIZE_MEDIUM - victim->size)*10)))
	chance /= 2;

    /* now the attack */
    if (number_percent() < chance)
    {
        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
          sprintf( buf, "Help! %s is knocking me down!", PERS(ch, victim) );
          do_autoyell( victim, buf );
        }	
        act("$n sweeps your feet out from under you!",ch,NULL,victim,TO_VICT);
	act("You sweep $N's feet out from under $M!",ch,NULL,victim,TO_CHAR);
	act("$n sweeps $N's feet out from under $M!",ch,NULL,victim,TO_NOTVICT);
	check_improve(ch,victim,gsn_legsweep,TRUE,1);
	damage(ch,victim,number_range(2, 2 +  2 * victim->size),gsn_legsweep,
	    DAM_BASH,TRUE);
	
	if (is_affected(victim, gsn_form_of_the_hawk)
	  && get_skill(victim,gsn_form_of_the_hawk) > number_percent())
        {
	    act("As you expose yourself to legsweep $N, $S swordwork allows $M to strike at you!", ch, NULL, victim, TO_CHAR);
	    act("As $n exposes $mself to legsweep $N, $N's swordwork allows $M to strike at $m!", ch, NULL, victim, TO_NOTVICT);
	    act("As $n exposes $mself to legsweep you, your swordwork allows you to strike at $m!", ch, NULL, victim, TO_VICT);
	    multi_hit(victim, ch, TYPE_UNDEFINED);
        }

	DAZE_STATE(victim,2 * PULSE_VIOLENCE);
        WAIT_STATE(ch,skill_table[gsn_legsweep].beats);
	WAIT_STATE(victim, UMAX(victim->wait, 2*PULSE_VIOLENCE));

	if (!is_flying(victim))
	    switch_position(victim, POS_RESTING);
    }
    else
    {
        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
          sprintf( buf, "Help! %s is knocking me down!", PERS(ch, victim) );
          do_autoyell( victim, buf );
        }
        act("$n tries sweeps your feet out from under you.",ch,NULL,victim,TO_VICT);
	act("You try to sweep $N's feet out from under $M, but fail.",ch,NULL,victim,TO_CHAR);
	act("$n tries to sweep $N's feet out from under $M, but fails.",ch,NULL,victim,TO_NOTVICT);
	WAIT_STATE(ch,skill_table[gsn_legsweep].beats*2/3);
	damage(ch,victim,0,gsn_legsweep,
	    DAM_BASH,TRUE);
	check_improve(ch,victim,gsn_legsweep,FALSE,1);
    } 
	check_killer(ch,victim);
}

void do_tame( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    int chance;

    if ( (chance = get_skill(ch,gsn_tame)) == 0)
    {
	send_to_char("Huh?\n\r",ch);
	return;
    }

	if (argument[0] == '\0')
	{
	send_to_char("Tame whom?\n\r", ch);
	return;
	}

	if ((victim = get_char_room(ch, argument)) == NULL)
	{
	send_to_char("You don't see them here.\n\r", ch);
	return;
	}

   if (!IS_NPC(victim))
	{
	send_to_char("You can't tame them, they're too in control of themselves.\n\r", ch);
	return;
	}

   if (!IS_SET(victim->act, ACT_AGGRESSIVE) && !IS_SET(victim->nact, ACT_PSYCHO))
	{
	send_to_char("They're not normally aggressive.\n\r", ch);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_tame].beats));
	return;
	} 

   chance += ((ch->level - victim->level) * 2);
   chance = URANGE(5, chance, 95);
   if (IS_SET(victim->imm_flags, IMM_TAME) || IS_SET(victim->act, ACT_UNDEAD)
	|| IS_SET(victim->imm_flags, IMM_CHARM) || IS_SET(victim->act, ACT_NOSUBDUE))
	chance = 0;

   if (number_percent() < chance)
	{
	act("$n calms $N down.", ch, NULL, victim, TO_ROOM);
	act("You calm $N down.", ch, NULL, victim, TO_CHAR);
	REMOVE_BIT(victim->act, ACT_AGGRESSIVE);
	REMOVE_BIT(victim->nact, ACT_PSYCHO);
	stop_fighting_all(victim);
	victim->tracking = NULL;
	check_improve(ch,victim,gsn_tame,TRUE,1);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_tame].beats));
	}
   else
	{
	act("$n tries to calm $N down, but fails.", ch, NULL, victim, TO_ROOM);
	act("You try to calm $N down, but fail.", ch, NULL, victim, TO_CHAR);
	check_improve(ch,victim,gsn_tame,FALSE,1);
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_tame].beats));
	}

}

void do_trip( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;

    one_argument(argument,arg);

    if ((chance = get_skill(ch,gsn_trip)) == 0)
    {
	send_to_char("Tripping?  What's that?\n\r",ch);
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

    if (victim->position < POS_FIGHTING)
    {
	act("$N is already down.",ch,NULL,victim,TO_CHAR);
	return;
    }

    if (victim == ch)
    {
	send_to_char("You fall flat on your face!\n\r",ch);
	WAIT_STATE(ch,(2 * skill_table[gsn_trip].beats)+number_bits(1));
	act("$n trips over $s own feet!",ch,NULL,NULL,TO_ROOM);
	return;
    }

    if ((ch->in_room->sector_type == SECT_UNDERWATER) && !is_affected(ch, gsn_aquamove))
    {
         if (number_percent() <= get_skill(ch, gsn_waveborne))
            check_improve(ch, victim, gsn_waveborne, true, 4);
        else
        {
            check_improve(ch, victim, gsn_waveborne, false, 4);
            act("$n twists awkwardly, trying to trip you.",ch,NULL,victim,TO_VICT);
            act("You flail wildly in the water, trying in vain to trip $N.",ch,NULL,victim,TO_CHAR);
            act("$n flail wildly in the water, trying in vain to trip $N.",ch,NULL,victim,TO_NOTVICT);
            WAIT_STATE(ch,skill_table[gsn_trip].beats/2);
	        return;
        }
    }

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
	act("$N is your beloved master.",ch,NULL,victim,TO_CHAR);
	return;
    }

    if (!check_reach(ch, victim, REACH_EASY, PULSE_VIOLENCE))
	return;

    /* size */
    if (ch->size < victim->size)
        chance += (ch->size - victim->size) * 10;  /* bigger = harder to trip */

    /* dex */
    chance += get_curr_stat(ch,STAT_DEX);
    chance -= get_curr_stat(victim,STAT_DEX) * 3 / 2;

    /* speed */
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
	chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
	chance -= 20;

    /* level */
    chance += (ch->level - victim->level) * 2;

    /* Form of the hawk -- AIAIAI! */

    if (is_affected(victim, gsn_form_of_the_hawk))
	chance /= 6;

    if (IS_NAFFECTED(victim, AFF_AGILITY) 
	  && (number_percent() < get_skill(victim,gsn_agility)/2+((SIZE_MEDIUM - victim->size)*10)))
	chance /= 2;

    if (is_affected(victim, gsn_celerity))
	chance /= 2;

    if ((is_flying(victim)
	&& !(is_affected(victim, gsn_earthbind))))
    {
	act("$S feet aren't on the ground.",ch,NULL,victim,TO_CHAR);
	chance = 0;
    }

    if (is_stabilized(*victim))
    {
        act("$N looks too stable to trip.",ch,NULL,victim,TO_CHAR);
        return;
    }

    /* now the attack */
    chance = UMIN(chance, 92);
    if (number_percent() < chance)
    {
        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
          sprintf( buf, "Help! %s is tripping me!", PERS(ch, victim) );
          do_autoyell( victim, buf );
        }	

	if (is_affected(victim, gsn_form_of_the_hawk)
	  && get_skill(victim, gsn_form_of_the_hawk) > number_percent())
        {
	    act("As you expose yourself to trip $N, $S swordwork allows $M to strike at you!", ch, NULL, victim, TO_CHAR);
	    act("As $n exposes $mself to trip $N, $N's swordwork allows $M to strike at $m!", ch, NULL, victim, TO_NOTVICT);
	    act("As $n exposes $mself to trip you, your swordwork allows you to strike at $m!", ch, NULL, victim, TO_VICT);
	    multi_hit(victim, ch, TYPE_UNDEFINED);
        }

        act("$n trips you and you go down!",ch,NULL,victim,TO_VICT);
	act("You trip $N and $E goes down!",ch,NULL,victim,TO_CHAR);
	act("$n trips $N, sending $M to the ground.",ch,NULL,victim,TO_NOTVICT);
	check_improve(ch,victim,gsn_trip,TRUE,1);

        WAIT_STATE(ch,skill_table[gsn_trip].beats);
	WAIT_STATE(victim,UMAX(victim->wait, 2*PULSE_VIOLENCE + number_bits(2) -1));
//        switch_position(victim, POS_RESTING);
	damage(ch,victim,number_range(2, 2 +  2 * victim->size),gsn_trip,
	    DAM_BASH,TRUE);
    }
    else
    {
        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
          sprintf( buf, "Help! %s tried to trip me!", PERS(ch, victim) );
          do_autoyell( victim, buf );
        }

	if (is_affected(victim, gsn_form_of_the_hawk))
        {
	    act("As you expose yourself to trip $N, $S swordwork allows $M to strike at you!", ch, NULL, victim, TO_CHAR);
	    act("As $n exposes $mself to trip $N, $N's swordwork allows $M to strike at $m!", ch, NULL, victim, TO_NOTVICT);
	    act("As $n exposes $mself to trip you, your swordwork allows you to strike at $m!", ch, NULL, victim, TO_VICT);
	    multi_hit(victim, ch, TYPE_UNDEFINED);
        }
	damage(ch,victim,0,gsn_trip,DAM_BASH,TRUE);
	WAIT_STATE(ch,skill_table[gsn_trip].beats*2/3);
	check_improve(ch,victim,gsn_trip,FALSE,1);
    } 

    check_killer(ch,victim);
    return;
}



void do_kill( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Kill whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    AFFECT_DATA * pyroKinetic(get_affect(victim, gsn_pyrokineticmirror));
    if ((!IS_NPC(victim)
      || (IS_OAFFECTED(victim, AFF_DOPPEL) && victim->memfocus[DOPPEL_SLOT] && !IS_NPC(victim->memfocus[DOPPEL_SLOT]))
      || (pyroKinetic != NULL && pyroKinetic->modifier == 1)
      || is_affected(victim, gsn_bilocation))
     && ch->fighting == NULL)
    {
        {
            send_to_char( "You must MURDER a player.\n\r", ch );
            return;
        }
    }

    if (!IS_NPC(victim) && IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM))
	return;

    if ( victim == ch )
    {
	send_to_char( "You hit yourself.  Ouch!\n\r", ch );
	multi_hit( ch, ch, TYPE_UNDEFINED );
	return;
    }

    if ( is_safe( ch, victim ) )
	return;

    if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim )
    {
	act( "$N is your beloved master.", ch, NULL, victim, TO_CHAR );
	return;
    }

    if ( ch->position == POS_FIGHTING && ch->fighting == victim )
    {
	send_to_char( "You do the best you can!\n\r", ch );
	return;
    }

    if (ch->position == POS_FIGHTING && victim->fighting != ch 
      && ch->class_num != global_int_class_gladiator)
	{
	act("One fight at a time!", ch, NULL, NULL, TO_CHAR);
	return;
	}

    if ( ch->position == POS_FIGHTING && ch->fighting != victim)
    {
	if (ch->class_num != global_int_class_gladiator 
	  && victim->fighting != ch)
	{
	    send_to_char("One fight at a time!\n\r",ch);
	    return;
	}
	
	act("You direct your attacks at $N.", ch, NULL, victim, TO_CHAR);
	act("$n directs $s attacks at $N.", ch, NULL, victim, TO_NOTVICT);
	act("$n directs $s attacks at you.", ch, NULL, victim, TO_VICT);
	ch->fighting = victim;
	if (victim->fighting == NULL)
	{
	    victim->fighting = ch;
    	    if (!IS_NPC(victim))
	    {
		char buf[MAX_STRING_LENGTH];
    		sprintf( buf, "Help!  I am being attacked by %s!", PERS(ch, victim) );
	    	do_autoyell( victim, buf );
	    }
	}

	return;
    }

    if (!IS_NPC(ch))
	SET_BIT(ch->act, PLR_AUTOASSIST);

    WAIT_STATE( ch, 1 * PULSE_VIOLENCE );
    check_killer( ch, victim );
    multi_hit( ch, victim, TYPE_UNDEFINED );
    return;
}



void do_murde( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to MURDER, spell it out.\n\r", ch );
    return;
}



void do_murder( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    bool already_fighting = FALSE;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Murder whom?\n\r", ch );
	return;
    }

    if (IS_AFFECTED(ch,AFF_CHARM) || (IS_NPC(ch) && IS_SET(ch->act,ACT_PET)))
	return;

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if (!IS_NPC(victim) && IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM))
	return;

    if (is_affected(ch, gsn_commandword) && !IS_PK(victim, ch->commander))
    {
	send_to_char("The gods prevent your command.\n\r", ch->commander);
	return;
    }

    if ( victim == ch )
    {
	send_to_char( "Suicide is a mortal sin.\n\r", ch );
	return;
    }

    if ( is_safe( ch, victim ) )
	return;

    if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim )
    {
	act( "$N is your beloved master.", ch, NULL, victim, TO_CHAR );
	return;
    }

    if ( ch->position == POS_FIGHTING 
      && ch->class_num != global_int_class_gladiator
      && victim != ch->fighting)
    {
	send_to_char( "You do the best you can!\n\r", ch );
	return;
    }

    if (is_affected(ch, gsn_commandword) && (ch->level - victim->level > 8))
	return;

    if (victim == ch->mercy_to)
    {
	act("You bring down your weapon upon $N, ending $S life in a single blow.", ch, NULL, victim, TO_CHAR);
	act("$n brings $s weapon down upon $N, ending $S life in a single blow.", ch, NULL, victim, TO_NOTVICT);
	act("$n brings $s weapon down upon you, ending your life.", ch, NULL, victim, TO_VICT);

	global_counter_works = FALSE;
	global_bool_check_avoid = FALSE;
	global_bool_full_damage = TRUE;
	damage(ch, victim, victim->max_hit, TYPE_HIT, DAM_NONE, FALSE);
	global_bool_full_damage = FALSE;
	global_bool_check_avoid = TRUE;
	global_counter_works = TRUE;

    free_string(ch->pose);
    free_string(victim->pose);

	ch->mercy_to = NULL;
	victim->mercy_from = NULL;

	return;
    }
    if (ch->fighting)
	already_fighting = TRUE;

    if (ch->fighting && ch->fighting == victim)
	send_to_char("You do the best you can!\n\r",ch);

    if (ch->fighting && ch->fighting != victim)
    {
	act("You direct your attacks at $N.", ch, NULL, victim, TO_CHAR);
	act("$n directs $s attacks at $N.", ch, NULL, victim, TO_NOTVICT);
	act("$n directs $s attacks at you.", ch, NULL, victim, TO_VICT);
	ch->fighting = victim;
    }

    if (!IS_NPC(ch))
	SET_BIT(ch->act, PLR_AUTOASSIST);

    if (!is_affected(ch, gsn_berserk))
    	WAIT_STATE( ch, 1 * PULSE_VIOLENCE );

    if (!IS_NPC(victim))
    {
        sprintf( buf, "Help!  I am being attacked by %s!", PERS(ch, victim) );
    	do_autoyell( victim, buf );
    }
    check_killer( ch, victim );
    if (!already_fighting)
	multi_hit( ch, victim, TYPE_UNDEFINED );
    if (is_affected(victim, gsn_thirdeye))
	multi_hit(victim, ch, TYPE_UNDEFINED);
    return;
}


void do_flank( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    int chance;

    if ( (chance = get_skill(ch,gsn_flank)) == 0)
    {
	send_to_char("Huh?\n\r",ch);
	return;
    }


    one_argument( argument, arg );

    if (ch->fighting == NULL)
    {
	send_to_char("But you're not fighting anyone!\n\r",ch);
	return;
    }

    victim = ch->fighting;

    if (ch->fighting->fighting == ch)
    {
	send_to_char("You can't flank someone who's attacking you.\n\r", ch);
	return;
    }
 
    if ( is_safe( ch, victim ) )
      return;

    if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL)
    {
	send_to_char( "You need to wield a weapon to flank someone.\n\r", ch );
	return;
    }

    if (!((obj->value[0] == WEAPON_POLEARM) 
    || ((obj->value[0] == WEAPON_SPEAR) 
      && (IS_WEAPON_STAT(obj,WEAPON_TWO_HANDS)))))
    {
	send_to_char("You require a polearm or two-handed spear to flank.\n\r",ch);
	return;
    }

    if (!check_reach(ch, victim, REACH_EASY, PULSE_VIOLENCE))
	return;

    check_killer( ch, victim );
 
    WAIT_STATE( ch, skill_table[gsn_flank].beats );
    
    if (is_affected(victim,gsn_scouting) && number_bits(1) == 0)
    {
        act( "You try to flank $N, but aren't able to escape $S notice.",  ch, NULL, victim, TO_CHAR);
        act( "$n tries to flank you, but isn't able to escape your notice.", ch, NULL, victim, TO_VICT);
        act( "$n tries to flank $N, but isn't able to escape $S notice.",  ch, NULL, victim, TO_NOTVICT);
	return;
    }
    else
    {
        act( "You flank $N for a blow from the side!",  ch, NULL, victim, TO_CHAR    );
        act( "$n comes at you on your flank for a blow from the side!", ch, NULL, victim, TO_VICT    );
        act( "$n flanks $N for a blow from the side!",  ch, NULL, victim, TO_NOTVICT );
    }

    chance = get_skill(ch,gsn_flank);
    chance = round(chance * 0.8);
    chance += (-18 + get_curr_stat(ch, STAT_DEX));


    if (number_percent() < chance
     || (chance >= 2 && !IS_AWAKE(victim)))
    {
        check_improve(ch,victim,gsn_flank,TRUE,1);
	one_hit( ch, victim, gsn_flank, HIT_PRIMARY );
    }
    else
    {
        check_improve(ch,victim,gsn_flank,FALSE,1);
	damage( ch, victim, 0, gsn_flank,DAM_NONE,TRUE);
    }

    return;
}

void do_withdraw( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA *was_in;
    ROOM_INDEX_DATA *now_in;
    CHAR_DATA *victim, *vch, *vch_next;
    int attempt;
    int direction;

    if ( ( victim = ch->fighting ) == NULL )
    {
        if ( ch->position == POS_FIGHTING )
            switch_position(ch, POS_STANDING);
	send_to_char( "You aren't fighting anyone.\n\r", ch );
	return;
    }

	if (number_percent() > (get_skill(ch, gsn_withdraw) * 8 / 10))
	{
	WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));
	DAZE_STATE(ch, UMAX(ch->daze, PULSE_VIOLENCE));
	send_to_char("You try to withdraw, but can't manage to set up your escape.\n\r", ch);
	check_improve(ch,victim,gsn_withdraw,FALSE,1);
	return;
	}

	if(argument[0] == '\0')
	{
	send_to_char("Which way did you want to withdraw?\n\r", ch);
	return;
	}

    if (!str_prefix(argument, "north"))
	direction = 0;
    else if (!str_prefix(argument, "east"))
	direction = 1;
    else if (!str_prefix(argument, "south"))
	direction = 2;
    else if (!str_prefix(argument, "west"))
	direction = 3;
    else if (!str_prefix(argument, "up"))
	direction = 4;
    else if (!str_prefix(argument, "down"))
	direction = 5;
    else
	{
	send_to_char("You can only withdraw in a normal direction.\n\r", ch);
	return;
	}


    was_in = ch->in_room;
    for ( attempt = 0; attempt < 6; attempt++ )
    {
	EXIT_DATA *pexit;
	int door;


	door = direction;
	if ( ( pexit = was_in->exit[door] ) == 0
	||   pexit->u1.to_room == NULL
	||   (IS_SET(pexit->exit_info, EX_CLOSED)
        &&   !IS_AFFECTED(ch, AFF_PASS_DOOR))
	||   number_range(0,ch->daze) != 0
	||   IS_SET(pexit->exit_info, EX_NOFLEE)
	||   is_affected(ch, gsn_whirlpool)
	|| ( IS_NPC(ch)
	&&   IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB) ) )
	    continue;

	if (is_affected(ch, gsn_form_of_the_panther) && !IS_AFFECTED(ch, AFF_SNEAK))
	{
	    SET_BIT(ch->affected_by, AFF_SNEAK);
	    move_char( ch, door, FALSE );
	    REMOVE_BIT(ch->affected_by, AFF_SNEAK);
	}
	else
	{
	    move_char( ch, door, FALSE );
	}

	if ( ( now_in = ch->in_room ) == was_in )
	    continue;

	ch->in_room = was_in;
	act( "$n has fled!", ch, NULL, NULL, TO_ROOM );
	ch->in_room = now_in;
	check_improve(ch,victim,gsn_withdraw,TRUE,1);
	send_to_char("You flee from combat!\n\r", ch);

	for (vch = was_in->people; vch != NULL; vch = vch_next)
	{
	    vch_next = vch->next_in_room;
	    if (!is_same_group(vch,ch) || vch->position < POS_FIGHTING
	      || IS_NAFFECTED(vch, AFF_FLESHTOSTONE)
	      || IS_OAFFECTED(vch, AFF_ENCASE))
	    	continue;
	    stop_fighting_all(vch);
	    act("$n withdraws, following $s leader out.", vch, NULL, NULL, TO_ROOM);
	    move_char(vch, door, FALSE);
	    if (vch->pet != NULL && vch->pet->in_room == was_in)
	    {
		act("$n withdraws, following $s leader out.", vch->pet, NULL, NULL, TO_ROOM);
		move_char(vch->pet, door, FALSE);
	    }
	}

	return;
    }

    send_to_char( "PANIC! You couldn't escape!\n\r", ch );
}

void do_flee( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA *was_in;
    ROOM_INDEX_DATA *now_in;
    CHAR_DATA *victim, *vch, *vch_next;
    int attempt;
    int x, chance;
    OBJ_DATA *wield;
    AFFECT_DATA *song;

    if ( ( victim = ch->fighting ) == NULL )
    {
        if ( ch->position == POS_FIGHTING )
            switch_position(ch, POS_STANDING);
	send_to_char( "You aren't fighting anyone.\n\r", ch );
	return;
    }

    if (!ch->in_room)
	return;

    if (((chance = get_skill(victim, gsn_cutoff)) > 0) 
     && (wield = get_eq_char(victim, WEAR_WIELD))
     && (wield->value[0] == WEAPON_STAFF))
    {
        if (is_affected(ch, gsn_astralform))
    	    chance /= 2;

	if (is_flying(ch))
	{
	    chance = chance / 2;
	    if (number_percent() < chance)
	    {
		act("$N cuts you off with $S staff, preventing your escape!", ch, NULL, victim, TO_CHAR);
		act("$N cuts $n off with $S staff, preventing $s escape!", ch, NULL, victim, TO_NOTVICT);
		act("You cut $n off with your staff, preventing $s escape!", ch, NULL, victim, TO_VICT);
		check_improve(victim,ch, gsn_cutoff, TRUE, 1);
		WAIT_STATE(ch, UMAX(ch->wait, 6));
		return;
	    }
	    else
       		check_improve(victim,ch, gsn_cutoff, FALSE, 1);
	}
	else
	{
	    chance = (chance * 2) / 3;
	    chance += (20 - get_curr_stat(ch, STAT_DEX));
	    if (number_percent() < chance)
	    {
		act("$N trips you with $S staff as you try to flee, and you fall!", ch, NULL, victim, TO_CHAR);
		act("$N trips $n with $S staff as $e tries to flee!", ch, NULL, victim, TO_NOTVICT);
		act("You trip $n with your staff as $e tries to flee away!", ch, NULL, victim, TO_VICT);
		WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE+1));
		check_improve(victim,ch, gsn_cutoff, TRUE, 1);
		return;
	    }
	    else
		check_improve(victim,ch, gsn_cutoff, FALSE, 1);
	}
    }
    if (is_affected(ch, gsn_entangle) && number_range(0,2) == 0)
    {
	send_to_char("The entangling net prevents your escape!",ch);
	return;
    }

    for (vch = ch->in_room->people; vch; vch=vch->next)
	if (IS_NPC(vch) && vch->pIndexData->vnum == MOB_VNUM_SEEDLING_GAMALOTH)
	{
	    act("$N blocks your escape!",ch,NULL,vch,TO_CHAR);
	    act("$N blocks $n's escape!",ch,NULL,vch,TO_NOTVICT);
	    act("You block $n's escape!",ch,NULL,vch,TO_VICT);
	    WAIT_STATE(ch, UMAX(ch->wait,PULSE_VIOLENCE));
	    return;
	}

    if (is_affected(ch, gsn_quicksand) && !is_flying(ch) && (number_bits(2) != 0))
    {
	send_to_char("Trapped in the quicksand, you are unable to escape!\n\r", ch);
	return;
    }

    song = NULL;
    if (((song = get_room_song(ch->in_room, gsn_marchofwar)) != NULL)
     && ((CHAR_DATA *) song->point != ch)
     && can_be_affected(ch,gsn_marchofwar)
     && !is_safe_spell((CHAR_DATA *) song->point, ch, TRUE)
     && (number_percent() <= song->modifier))
    {
	act("The notes of $N's war march ring throughout your mind, driving you to aggression, and you are unable to flee.", ch, NULL, (CHAR_DATA *) song->point, TO_CHAR);
	WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE * 2));
	return;
    }

    if (IS_NAFFECTED(ch, AFF_RAGE))
	{
	send_to_char("As your rage keeps you concentrated on fighting, you are unable to consider escape.\n\r", ch);
	return;
	}

    AFFECT_DATA *paf = NULL;
    if (ch && ch->in_room 
      && (paf = affect_find(ch->in_room->affected, gsn_ringoffire)) != NULL)
    {
	ROOM_INDEX_DATA *oRoom;

	oRoom = ch->in_room;

	send_to_char("You carelessly flee into the ring of fire, scorching yourself!\n\r", ch);
	act("The ring of fire dissipates as $n passes through it.",ch,NULL,NULL,TO_ROOM);
	act("The ring of fire dissipates as you pass through it.",ch,NULL,NULL,TO_CHAR);
	damage_old(ch, ch, number_range(paf->level, paf->level*3), gsn_ringoffire, DAM_FIRE, TRUE);
	room_affect_strip(oRoom,gsn_ringoffire);
	if (!ch || (ch->in_room != oRoom) || IS_OAFFECTED(ch, AFF_GHOST))
	    return;
    }

    x = room_is_affected(ch->in_room, gsn_stonetomud) ? 1 : 6;

    if (!IS_NPC(ch) && BIT_GET(ch->pcdata->traits, TRAIT_COWARD)) x *= 2;
    if (is_affected(ch, gsn_heartofstone)) x *= 2;

    was_in = ch->in_room;
    for ( attempt = 0; attempt < x; attempt++ )
    {
        EXIT_DATA *pexit;
        int door;

        if (IS_NAFFECTED(ch, AFF_BOLO) && number_bits(1) == 0)
            continue;

        door = number_door( );
        if ( ( pexit = was_in->exit[door] ) == 0
        ||   pexit->u1.to_room == NULL
        ||   (IS_SET(pexit->exit_info, EX_CLOSED)
            &&   (IS_SET(pexit->exit_info, EX_NOPASS)
               || !IS_AFFECTED(ch, AFF_PASS_DOOR)))
        ||   IS_SET(pexit->exit_info, EX_NOFLEE)
        ||   number_range(0,ch->daze) != 0
        ||   is_affected(ch, gsn_whirlpool)
        || ( IS_NPC(ch)
        &&   IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB) ) )
            continue;

        if (!ch->valid || ch->position < POS_SLEEPING)
            return;

            if (IS_OAFFECTED(ch,AFF_DEMONPOS) && number_bits(1) == 0)
            
            {
                send_to_char("The demon seizes control, and is lost to the bloodlust of battle!\n\r",ch);
                return;
            }

        if ((song = affect_find(ch->affected, gsn_predatoryattack))!= NULL)
        {
            if (song->point)
                if (ch->in_room == ((CHAR_DATA *)song->point)->in_room 
              && !(is_affected(ch,gsn_forestwalk) && ch->in_room->sector_type == SECT_FOREST)
              && !(ch->race == global_int_race_ethron && (ch->in_room->sector_type != SECT_CITY && ch->in_room->sector_type != SECT_INSIDE)))
                {
                    act("$N slows your escape!",ch,NULL,(CHAR_DATA *)song->point,TO_CHAR);
                    act("You slow $n's escape!",ch,NULL,(CHAR_DATA *)song->point,TO_VICT);
                    act("$N slows $n's escape!",ch,NULL,(CHAR_DATA *)song->point,TO_NOTVICT);
                    WAIT_STATE(ch,UMAX(ch->wait,PULSE_VIOLENCE*1.5));
                }
        }

        global_bool_final_valid = TRUE;
        if (is_affected(ch, gsn_form_of_the_panther) && !IS_AFFECTED(ch, AFF_SNEAK))
        {
            SET_BIT(ch->affected_by, AFF_SNEAK);
            move_char( ch, door, FALSE );
            REMOVE_BIT(ch->affected_by, AFF_SNEAK);
        }
        else
        {
            move_char( ch, door, FALSE );
        }
        global_bool_final_valid = FALSE;

        if (!ch || !ch->valid)
            return; /* they're dead, probably from final strike */

        if ( ( now_in = ch->in_room ) == was_in )
            continue;

        for (vch = was_in->people; vch; vch = vch_next)
        {
            vch_next = vch->next_in_room;
            if (IS_NPC(vch) && (vch->master == ch))
            {
            if (vch->fighting)
            {
                act("$n flees with $s master!", ch, NULL, NULL, TO_ROOM);
                stop_fighting_all(vch);
            }

            move_char(vch, door, FALSE);
            }
        }

        if (IS_OAFFECTED(ch, AFF_GHOST))
            return;

        /* combat should now have stopped in move_char */
    //	stop_fighting( ch, TRUE );
        ch->in_room = was_in;
        act( "$n has fled!", ch, NULL, NULL, TO_ROOM );
        ch->in_room = now_in;

        if (!IS_NPC(ch))
            send_to_char("You flee from combat!\n\r", ch);

        return;
    }

    send_to_char( "PANIC! You couldn't escape!\n\r", ch );
}

static void do_rescue_helper(CHAR_DATA * ch, CHAR_DATA * victim)
{
    if (IS_OAFFECTED(ch, AFF_GHOST))
    {
        send_to_char("You cannot rescue in your ghostly form.\n\r", ch);
        return;
    }

    if ( victim == ch )
    {
        send_to_char( "What about fleeing instead?\n\r", ch );
        return;
    }

    if ( !IS_NPC(ch) && IS_NPC(victim) )
    {
        send_to_char( "They don't need your help!\n\r", ch );
        return;
    }

    if ( ch->fighting == victim )
    {
        send_to_char( "Too late.\n\r", ch );
        return;
    }

    CHAR_DATA * fch;
    if ( ( fch = victim->fighting ) == NULL )
    {
        send_to_char( "That person is not fighting right now.\n\r", ch );
        return;
    }

    if (is_safe(fch, ch))
        return;

    if (!IS_NPC(ch))
	if (!IS_NPC(fch) && !IS_PK(ch, fch))
	{
	    act("The gods protect $N from you.", ch, NULL, fch, TO_CHAR);
	    return;
	}

    if (!IS_NPC(fch) && IS_AFFECTED(ch, AFF_CHARM) 
      && ch->master && !IS_PK(ch->master, fch))
    {
	act("The gods protect $N from you.", ch, NULL, fch, TO_CHAR);
	return;
    }

    WAIT_STATE( ch, skill_table[gsn_rescue].beats );

    int chance = get_skill(ch, gsn_rescue);

    if (!IS_NPC(ch) && BIT_GET(ch->pcdata->traits, TRAIT_BRAVE))
        chance += 15;

    if ( number_percent( ) > chance)
    {
	send_to_char( "You fail the rescue.\n\r", ch );
	check_improve(ch,victim,gsn_rescue,FALSE,1);
	return;
    }

    act( "You rescue $N!",  ch, NULL, victim, TO_CHAR    );
    act( "$n rescues you!", ch, NULL, victim, TO_VICT    );
    act( "$n rescues $N!",  ch, NULL, victim, TO_NOTVICT );
    check_improve(ch,victim,gsn_rescue,TRUE,1);

    if (number_percent() < round(get_skill(fch, gsn_ferocity) * 17/20))
    {
	act("You remain focused on $N.", fch, NULL, victim, TO_CHAR);
	act("$n remains focused on $N.", fch, NULL, victim, TO_NOTVICT);
	act("$n remains focused on you.", fch, NULL, victim, TO_VICT);
	check_improve(fch,ch,gsn_ferocity,TRUE,1);
    }
    else
    {
	check_improve(fch,ch,gsn_ferocity,FALSE,1);
	stop_fighting( fch );
	stop_fighting( victim );

	check_killer( ch, fch );
	set_fighting( ch, fch );
	set_fighting( fch, ch );
    }
}

void do_rescue( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    if (IS_NPC(ch) && IS_SET(ch->nact, ACT_NORESCUE))
	return;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Rescue whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    do_rescue_helper(ch, victim);
}

void do_eagle_jab( CHAR_DATA *ch, CHAR_DATA *victim )
{
    int dam, diceroll;
    OBJ_DATA *wield;
    int skill;
    int dam_type;

    skill = get_skill(ch, gsn_form_of_the_eagle);
    wield = get_eq_char(ch, WEAR_WIELD);
    if (wield == NULL && get_skill(ch, gsn_versatility) > number_percent())
	wield = get_eq_char(ch, WEAR_DUAL_WIELD);

    if (number_percent() < skill * 3 / 4)
    {
	if ( wield != NULL )
        {
            if (wield->pIndexData->new_format)
                dam = (dice(wield->value[1],wield->value[2]) * skill)/100;
            else
                dam = (number_range( wield->value[1], wield->value[2]) * skill)/100;

                dam = dam * 11/10;

	    dam += ch->damroll;
	    dam = round(dam * 1.2);

    	    if ( get_skill(ch,gsn_enhanced_damage) > 0 )
    	    {
       		diceroll = number_percent();
        	if (diceroll <= get_skill(ch,gsn_enhanced_damage))
        	{
            	    check_improve(ch,victim,gsn_enhanced_damage,TRUE,6);
            	    dam += number_fuzzy((dam * (get_skill(ch,gsn_enhanced_damage)))/300);
        	}
    	    }

	    dam_type = wield->value[3];
	    damage(ch,victim, dam, gsn_form_of_the_eagle,dam_type,TRUE);
        }
    }
    else
	damage(ch,victim, 0, gsn_form_of_the_eagle,DAM_PIERCE,TRUE);

    return;
}

void do_pugil( CHAR_DATA *ch, char *argument )
{
     OBJ_DATA *weapon;
    int chance;
    CHAR_DATA *victim;
    int dam_type;

    if ((chance = get_skill(ch,gsn_pugil)) == 0)
    {
	send_to_char(
		"Huh?\n\r", ch);
	return;
    }

    if ( ( victim = ch->fighting ) == NULL )
    {
	send_to_char( "You aren't fighting anyone.\n\r", ch );
	return;
    }


    if ((weapon = get_eq_char(ch, WEAR_WIELD)) == NULL)
	{
	send_to_char("You aren't wielding a weapon.\n\r", ch);
	return;
	}

    if (weapon->value[0] != WEAPON_STAFF)
	{
	send_to_char("You must wield a staff to pugil.\n\r", ch);
	return;
	}
 
    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;

    chance = (chance * 9) /10;
    chance += get_curr_stat(ch, STAT_DEX) - get_curr_stat(victim, STAT_DEX);

    dam_type = weapon->value[3];
    WAIT_STATE( ch, skill_table[gsn_pugil].beats );
    if (chance > number_percent())
    {
	damage_from_obj(ch,victim,weapon,number_range( ch->level, (ch->level + dice(weapon->value[1], weapon->value[2]) + weapon->value[3] + ch->damroll) ), gsn_pugil, dam_type,TRUE);
	check_improve(ch,victim,gsn_pugil,TRUE,1);
    }
    else
    {
	damage( ch, victim, 0, gsn_pugil,dam_type,TRUE);
	check_improve(ch,victim,gsn_pugil,FALSE,1);
    }
	check_killer(ch,victim);
    return;
}

void do_kick( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *victim;
    int chance, dchance;
    bool smashed = FALSE;
    int dam=0;
    int dam_type = DAM_BASH;
 
    if (!get_skill(ch, gsn_kick))
    {
	send_to_char(
	    "Huh?\n\r", ch );
	return;
    }

	if (ch->in_room == NULL)
		return;

    if ( ( victim = ch->fighting ) == NULL )
    {
	send_to_char( "You aren't fighting anyone.\n\r", ch );
	return;
    }

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;

    WAIT_STATE( ch, skill_table[gsn_kick].beats );

    if (is_stabilized(*victim) && number_bits(1) == 0)
    {
        act("The earth bursts up suddenly, deflecting your kick!", ch, NULL, victim, TO_CHAR);
        act("The earth bursts up suddenly, deflecting $n's kick!", ch, NULL, victim, TO_VICT);
        act("The earth bursts up suddenly, deflecting $n's kick!", ch, NULL, victim, TO_NOTVICT);
        return;
    }
  
    if (is_affected(victim,gsn_form_of_the_serpent)
      && number_percent() < (get_skill(victim,gsn_form_of_the_serpent) / 2))
    {
	act("$N quickly sidesteps your kick!", ch, NULL, victim, TO_CHAR);
	act("You quickly sidestep $n's kick with the swiftness of the serpent!", ch, NULL, victim, TO_VICT);
	act("$N quickly sidesteps $n's kick!", ch, NULL, victim, TO_NOTVICT);
	return;
    }

    /* check for chance to dodge kicks       */
    /* also known as the We Love Aelins code */

    dchance = get_skill(victim,gsn_dodge) / 5;

    dchance += (get_curr_stat(victim, STAT_DEX) - 18);

    if (is_affected(victim, gsn_plantentangle))
	dchance = ((dchance * 4) / 5);

    if (is_affected(victim, gsn_freeze))
	dchance -= 4 * get_modifier(victim->affected, gsn_freeze);

    if (!can_see(victim,ch) && !is_blindfighter(victim,FALSE))
	dchance /= 2;

    dchance += (victim->level - ch->level);

    if ((victim->in_room && (victim->in_room->sector_type == SECT_UNDERWATER)) && !is_affected(victim, gsn_aquamove))
    {
        if (number_percent() <= get_skill(victim, gsn_waveborne))
            check_improve(victim, ch, gsn_waveborne, true, 4);
        else
        {
            check_improve(victim, ch, gsn_waveborne, false, 4);
        	dchance = round(dchance * 0.8);
        }
    }

    if (is_affected(victim, gsn_levitation))
	dchance = 0;

    if ( number_percent() < dchance )
    {
        act( "You dodge $n's kick.", ch, NULL, victim, TO_VICT    );
        act( "$N dodges your kick.", ch, NULL, victim, TO_CHAR    );
	act( "$N dodges $n's kick.", ch, NULL, victim, TO_NOTVICT );
        check_improve(victim,ch,gsn_dodge,TRUE,6);
        return;
    }
    chance = get_skill(ch,gsn_kick);
    if (IS_NAFFECTED(ch,AFF_BOLO))
	chance /= 2;

    if ( chance > number_percent() )
    {
        /* check for broken knees */
        if (((chance = get_skill(ch, gsn_kneeshatter)) != 0)
	&& (ch->class_num == global_int_class_bandit
	  || ch->class_num == global_int_class_gladiator)
	&& (!IS_NPC(ch) || IS_SET(ch->act,ACT_CLASSED))
        && !is_affected(victim,gsn_kneeshatter)
	&& !IS_NAFFECTED(ch,AFF_BOLO)
        && ((ch->in_room->sector_type != SECT_UNDERWATER) 
	  || is_affected(ch, gsn_aquamove)))
        {
            chance += (get_curr_stat(ch,STAT_STR) - get_curr_stat(victim,STAT_DEX));
	    if (is_affected(ch, gsn_stoneskin) || is_affected(ch, gsn_stoneshell))
		chance = (chance / 2);
	
	    if (number_percent() < (chance/3))
	    {
		if (is_affected(victim, gsn_diamondskin) && IS_SET(victim->imm_flags, IMM_WEAPON))
		{
		    act("Your attempt to shatter $N's knee is prevented by $S diamond skin!", ch, NULL, victim, TO_CHAR);
		    act("$n's attempt to shatter your knee is prevented by your diamond skin!", ch, NULL, victim, TO_CHAR);
		}
		else
		{
		    act("Noticing an opening, you smash $N's knee with a powerful kick!", ch, NULL, victim, TO_CHAR);
		    act("$N's knee shatters as $n executes a powerful kick.", ch, NULL, victim, TO_NOTVICT);
	 	    act("You scream in agony as $n shatters your knee with a powerful kick.", ch, NULL, victim, TO_VICT);
              	    WAIT_STATE(victim,UMAX(victim->wait, 2*PULSE_VIOLENCE + number_bits(2) -1));

                    smashed = TRUE;
		    af.where	= TO_AFFECTS;
		    af.type		= gsn_kneeshatter;
		    af.level	= ch->level;
		    af.duration	= ch->level/15;
		    af.modifier	= -3;
		    af.location	= APPLY_DEX;
		    af.bitvector	= 0;
		    affect_to_char(victim, &af);
		}
		check_improve(ch,victim,gsn_kneeshatter,TRUE,1);
	    }
	    else
		check_improve(ch,victim,gsn_kneeshatter,FALSE,1);
	} 

	if (smashed)
	    dam = round(number_range(ch->level, (ch->level*2 + UMIN(ch->level,ch->damroll)) / 2) * 1.3);
	else
	    dam = number_range(ch->level, (ch->level + UMIN(ch->level, ch->damroll)) / 2);

	if (is_affected(ch,gsn_shadowfist))
	    dam_type = DAM_MENTAL;

	if (ch->class_num == global_int_class_assassin
	  && (!IS_NPC(ch) || IS_SET(ch->act,ACT_CLASSED))
	  && skill_table[gsn_snapkick].skill_level[ch->class_num] <= ch->level
	  && !IS_NAFFECTED(ch,AFF_BOLO)
	  && number_percent() < get_skill(ch,gsn_snapkick))
	{
	    dam += check_extra_damage(ch,dam,NULL);
	    check_improve(ch,victim,gsn_snapkick,TRUE,1);
	}
	else
	    check_improve(ch,victim,gsn_snapkick,FALSE,1);

	damage(ch,victim,dam, gsn_kick, dam_type, TRUE);
	if (victim && !IS_OAFFECTED(victim,AFF_GHOST)
	  && !IS_NAFFECTED(ch,AFF_BOLO)
	  && number_percent() < get_skill(ch,gsn_snapkick)/2)
	{
	    act("You snap your heel back, and land another kick!",ch,NULL,victim,TO_CHAR);
	    act("$n snaps $s heel back, and lands another kick!",ch,NULL,victim,TO_ROOM);
	    damage(ch,victim,dam/2,gsn_kick,dam_type,TRUE);
	}
	check_improve(ch,victim,gsn_kick,TRUE,1);
    }
    else
    {
	damage( ch, victim, 0, gsn_kick,dam_type,TRUE);
	check_improve(ch,victim,gsn_kick,FALSE,1);
    }
    check_killer(ch,victim);
    return;
}

void do_uppercut( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *victim;
    OBJ_DATA *obj;

    int hands, chance;

    if ((chance = get_skill(ch,gsn_uppercut)) == 0)
    {
	send_to_char(
	    "Huh?\n\r", ch );
	return;
    }

    if ( ( victim = ch->fighting ) == NULL )
    {
	send_to_char( "You aren't fighting anyone.\n\r", ch );
	return;
    }

    hands = 0;

    if ((obj = get_eq_char(ch, WEAR_WIELD)) != NULL
	&& obj->pIndexData->value[0] != WEAPON_MACE)
	hands++;

    if ((obj = get_eq_char(ch, WEAR_DUAL_WIELD)) != NULL
	&& obj->pIndexData->value[0] != WEAPON_MACE)
	hands++;

    if (get_eq_char(ch, WEAR_LIGHT) != NULL)
	hands++;

    if (get_eq_char(ch, WEAR_SHIELD) != NULL)
	hands++;

    if (get_eq_char(ch, WEAR_HOLD) != NULL)
	hands++;

    if (hands > 1)
    {
	send_to_char("You need a free hand or a mace to uppercut someone.\n\r", ch);
	return;
    }

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;

    WAIT_STATE( ch, skill_table[gsn_uppercut].beats );

    if (is_affected(victim, gsn_diamondskin) && IS_SET(victim->imm_flags, IMM_WEAPON))
    {
	act("You uppercut, but your hand crunches painfully on $N's diamond skin.", ch, NULL, victim, TO_CHAR);
	act("$n uppercuts, but $s hand crunches painfully on $N's diamond skin.", ch, NULL, victim, TO_NOTVICT);
	act("$n uppercuts, but $s hand crunches painfully on your diamond skin.", ch, NULL, victim, TO_VICT);
	return;
    }


    if (IS_NAFFECTED(victim, AFF_WARINESS))
	chance /= 2;

    if (is_affected(victim, gsn_sense_danger))
    {
	send_to_char("You sense incoming danger.\n\r", victim);
	chance /= 2;
    }
 
    chance /= 10;
    chance -= get_curr_stat(victim, STAT_DEX);
    chance += get_curr_stat(ch, STAT_STR);
    if (IS_AFFECTED(ch, AFF_HASTE))
   	chance += 15;
    if (IS_AFFECTED(victim, AFF_HASTE))
	chance -= 20;
    if (is_affected(victim,gsn_jab))
	chance += 10;
    if (is_affected(victim,gsn_headbutt))
	chance += 20;
    if (is_affected(victim,gsn_pummel))
	chance += 5 *(get_modifier(victim->affected,gsn_pummel));

    chance += (ch->level - victim->level);

    act( "You take a wild swing!",  ch, NULL, victim, TO_CHAR    );
    act( "$n takes a wild swing!", ch, NULL, victim, TO_VICT    );
    act( "$n takes a wild swing!",  ch, NULL, victim, TO_NOTVICT );

    if (IS_NPC(victim) && (IS_SET(victim->act, ACT_NOSUBDUE) || IS_SET(victim->act, ACT_UNDEAD)))
	chance = 0;

    if (number_percent() < chance)
    {
        act( "You deliver a stunning uppercut to $N's jaw, knocking $M out cold!",  ch, NULL, victim, TO_CHAR    );
        act( "$n delivers a stunning uppercut to your jaw, knocking you out cold!", ch, NULL, victim, TO_VICT    );
        act( "$n delivers a stunning uppercut to $N's jaw, knocking $M out cold!",  ch, NULL, victim, TO_NOTVICT );
	
	stop_fighting_all(victim);
	switch_position(victim, POS_SLEEPING);

	af.where	= TO_AFFECTS;
	af.type		= gsn_uppercut;
	af.level	= ch->level;
	af.duration	= 1;
	af.modifier	= 0;
	af.location	= APPLY_NONE;
	af.bitvector	= AFF_SLEEP;
	affect_to_char(victim, &af);
	
	check_improve(ch,victim,gsn_uppercut,TRUE,1);
    }
    else
    {
	damage( ch, victim, number_range(1,10), gsn_uppercut,DAM_BASH,TRUE);
	check_improve(ch,victim,gsn_uppercut,FALSE,1);
    }
    check_killer(ch,victim);
    return;
}

void do_pummel( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *wield, *shield;
    CHAR_DATA *victim;
    int thac0;
    int thac0_00;
    int thac0_32;
    int victim_ac;
    int dam, mod, dexmod;
    int sn, skill;
    int diceroll;
    OBJ_DATA *net = NULL;

    if (!get_skill(ch,gsn_pummel))
    {
	send_to_char(
	    "Huh?\n\r", ch );
	return;
    }

    if (argument[0] != '\0')
    {
        if ((victim = get_char_room(ch,argument)) == NULL)
        {
	    send_to_char("You don't see them here.\n\r",ch);
	    return;
	}
    }
    else if ((victim = ch->fighting) == NULL)
    {
	send_to_char("Pummel whom?\n\r",ch);
	return;
    }

    if (ch == victim)
    {
	send_to_char("You cannot effectively pummel yourself.\n\r", ch);
        return;
    }

    if (is_safe(ch,victim))
	return;

    if (victim->position == POS_DEAD || ch->in_room != victim->in_room)
	return;

    wield = get_eq_char(ch, WEAR_WIELD);

    if (wield != NULL)
	if (wield->value[0] != WEAPON_MACE)
	{
	    wield = get_eq_char(ch, WEAR_DUAL_WIELD);
	    if (wield == NULL)
	    {
		wield = get_eq_char(ch, WEAR_SHIELD);
		if (wield != NULL)
		{
		    send_to_char("You can't pummel someone with that weapon.\n\r",ch);
		    return;
		}
	    }
	    wield = get_eq_char(ch, WEAR_DUAL_WIELD);
	    if ((wield != NULL) && (wield->value[0] != WEAPON_MACE))
	    {
	        send_to_char("You can't pummel someone with that weapon.\n\r", ch);
	        return;
	    }
	}

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;

    check_killer(ch, victim);

    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_pummel].beats));

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

    if (is_affected(victim, gsn_diamondskin) && IS_SET(victim->imm_flags, IMM_WEAPON))
    {
	act("You attempt to viciously pummel $N, but are blocked by $S diamond skin!", ch, NULL, victim, TO_CHAR);
	act("$n attempts to viciously pummel you, but is blocked by your diamond skin!", ch, NULL, victim, TO_VICT);
	set_fighting(victim, ch);
        return;
    }
    
    if (IS_NAFFECTED(victim, AFF_AGILITY) 
      && (number_percent() < get_skill(victim,gsn_agility)/2+((SIZE_MEDIUM - victim->size)*10)))
    {
	act("You nimbly dodge $n's pummel.",ch,NULL,victim,TO_VICT);
	act("$N nimbly dodges $n's pummel.",ch,NULL,victim,TO_VICTROOM);
	act("$N nimbly dodges your pummel.",ch,NULL,victim,TO_CHAR);
	return;
    }

    if (is_affected(victim,gsn_astralform) && number_bits(1) != 0)
    {
        act("You try to pummel $N, but your blows pass right through $M!",
            ch,NULL,victim,TO_CHAR);
        act("$n tries to pummel you, but $s blows pass right through you!",
            ch, NULL, victim, TO_VICT);
        act("$n tries to pummel $N, but $s blows pass right through $N!",
            ch, NULL, victim, TO_NOTVICT);
	set_fighting(victim, ch);
        return; 
    }


    if (!IS_NPC(victim) && ch->fighting != victim)
    {
	sprintf(buf, "Help!  %s is trying to pummel me!", PERS(ch,victim));
	do_autoyell(victim,buf);
    }

    if (wield && obj_is_affected(wield, gsn_heatsink))
	damage_old(ch,ch,number_range(4,8),gsn_heatsink,DAM_COLD,TRUE);
    
    wrathkyana_combat_effect(ch, victim);

    if ((shield = get_eq_char(victim, WEAR_ABOUT))
     && (shield->pIndexData->vnum == OBJ_VNUM_MANTLE_EARTH)
     && (number_bits(2) != 0))
    {
        act("$N's mantle of earth absorbs your pummeling!", ch, NULL, victim, TO_CHAR);
        act("Your mantle of earth absorbs $n's pummeling!", ch, NULL, victim, TO_VICT);
        act("$N's mantle of earth absorbs $n's pummeling!", ch, NULL, victim, TO_NOTVICT);
	set_fighting(victim, ch);
        return;
    }

    if (get_eq_char(ch, WEAR_WIELD) == wield)
        sn = get_weapon_sn(ch);
    else
	sn = get_dual_sn(ch); 
    skill = get_skill(ch, gsn_pummel);

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

    victim_ac = (GET_AC(victim,AC_BASH)/10);

    if (victim_ac < -15)
        victim_ac = (victim_ac + 15) / 5 - 15;

    if (!can_see(ch, victim) && !is_blindfighter(ch,FALSE))
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

    if ((dexmod = (get_curr_stat(victim, STAT_DEX) - 18)) > 0)
    diceroll -= dexmod;

    if (( diceroll == 0
    || ( diceroll != 19 && diceroll < thac0 - victim_ac ) )
    && !is_affected(ch, gsn_form_of_the_serpent))
    {
        /* Miss. */
        damage( ch, victim, 0, gsn_pummel, DAM_BASH, TRUE );
        check_improve(ch,victim,gsn_pummel,FALSE,1);
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
                dam = (dice(wield->value[1],wield->value[2]) * skill)/100;
            else
                dam = ((number_range( wield->value[1],
                                wield->value[2] )* skill)/100);

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

    dam = round(dam * 1.1);

    if ( dam <= 0 )
        dam = 1;

    act( "You viciously pummel $N!",  ch, NULL, victim, TO_CHAR    );
    act( "$n pummels you viciously!", ch, NULL, victim, TO_VICT    );
    act( "$n pummels $N viciously!",  ch, NULL, victim, TO_NOTVICT );
    if (number_percent() < 33)
        WAIT_STATE(victim, UMAX(victim->wait, (PULSE_VIOLENCE*2)+1));
    else
        WAIT_STATE(victim, UMAX(victim->wait, PULSE_VIOLENCE+1));
    damage(ch,victim, dam, gsn_pummel,DAM_BASH, TRUE);
    
    if (is_affected(victim, gsn_pummel))
      mod = get_modifier(victim->affected, gsn_pummel) + 1;
    else
      mod = 0;

    check_improve(ch, victim,gsn_pummel, TRUE, 1);
    if (victim->in_room && !IS_OAFFECTED(victim,AFF_GHOST))
    {
	af.where       = TO_AFFECTS;
	af.type        = gsn_pummel;
	af.level       = ch->level;
	af.duration    = 5;
	af.modifier    = mod;
	af.location    = 0;
	af.bitvector   = 0;
	affect_strip(victim, gsn_pummel);
	affect_to_char(victim, &af);
    }
}

int rout_chance_modifier_both(CHAR_DATA * victim, CHAR_DATA * leader)
{
	int mod = 0;
	if (IS_AFFECTED(victim, AFF_BLIND)) mod += 50;
	if (IS_AFFECTED(victim, AFF_BERSERK)) mod += 50;
	if (is_affected(victim, gsn_confusion)) mod += 50;
    if (is_affected(victim, gsn_unrealincursion)) mod += 50;
    if (is_affected(victim, gsn_heartofstone)) mod -= 50;
	if (IS_AFFECTED(victim, AFF_HASTE)) mod -= 15;
	if (IS_AFFECTED(victim, AFF_SLOW)) mod += 15;
	mod -= (leader ? get_curr_stat(leader, STAT_CHR) : 0);

    AFFECT_DATA * fugue(get_affect(victim, gsn_fugue));
    if (fugue != NULL && fugue->location != APPLY_NONE)
        mod += 25;
	return mod;
}


int rout_chance_modifier_resist(CHAR_DATA * victim, CHAR_DATA * leader)
{
	int mod = 0;
    if (number_percent() <= get_skill(leader, gsn_celestialtactician))
    {
        check_improve(leader, NULL, gsn_celestialtactician, true, 1);
        mod -= 50;
    }

	if (is_affected(victim, gsn_spiritbond)) mod -= 20;
	if (is_affected(victim, gsn_bondofsouls)) mod -= 20;
	if (IS_OAFFECTED(victim, AFF_DEAFEN)) mod += 10;
	if (is_affected(victim, gsn_zeal)) mod += 5;
	if (is_affected(victim, gsn_frenzy)) mod += 5;
	if (is_affected(victim, gsn_readiness)) mod -= 20;
	if (is_affected(victim, gsn_wariness)) mod -= 20;
	if (victim->position <= POS_SLEEPING) mod += 1000;
	mod += rout_chance_modifier_both(victim, leader);
	return mod;
}

void rout_victim(CHAR_DATA * victim, CHAR_DATA * leader)
{
	act("You split off your group in the confusion!", victim, NULL, leader, TO_CHAR);
	if (victim != leader) act("$n splits off your group in the confusion!", victim, NULL, leader, TO_VICT);	
	act("$n splits off $s group in the confusion!", victim, NULL, leader, TO_NOTVICT);
	stop_follower(victim);

	// 10% chance of fleeing
	if (victim->fighting && number_percent() < 10)
		do_flee(victim, "");
	
	WAIT_STATE(victim, PULSE_VIOLENCE);
}

void do_rout(CHAR_DATA * ch, char * argument)
{
	OBJ_DATA * obj = NULL;
	CHAR_DATA * victim = NULL;
	CHAR_DATA * leader = NULL;
	CHAR_DATA * groupmate = NULL;
	int skill = get_skill(ch, gsn_rout);
	int chance = 0, baseChance = 0;
	bool hasGroup = FALSE, success = FALSE;

	if (skill <= 0)
	{
		send_to_char("Huh?\n\r", ch );
		return;
	}

	// Determine victim
	if (argument[0] == '\0')
	{
		if ((victim = ch->fighting) == NULL)
		{
			send_to_char("Who do you want to rout?\n\r", ch);
			return;
		}
	}
	else
		if ((victim = get_char_room(ch, argument)) == NULL)
		{
			send_to_char("They aren't here.\n\r", ch);
			return;
		}
	// Make sure they've got a polearm or spear, and either should be two-handed
	if ((obj = get_eq_char(ch, WEAR_WIELD)) == NULL 
	|| !(IS_WEAPON_STAT(obj,WEAPON_TWO_HANDS))
	|| (!((obj->value[0] == WEAPON_POLEARM) || (obj->value[0] == WEAPON_SPEAR))))
	{
		send_to_char("You require a polearm or a two-handed spear to properly rout an opponent.\n\r", ch);
		return;
	}

	// Determine leader of victim's group (and make sure there -is- a group)
	if ((leader = victim->leader) == NULL)
		leader = victim;

	for (groupmate = victim->in_room->people; groupmate != NULL; groupmate = groupmate->next_in_room)
	{
		if (groupmate != leader && (is_same_group(groupmate, leader) || groupmate->master == leader))
		{
			hasGroup = TRUE;
			break;
		}
	}
	
	if (!hasGroup)
	{
		send_to_char("Your opponent has no group to rout.\n\r", ch);
		return;
	}

	if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
		return;

	WAIT_STATE(ch, skill_table[gsn_rout].beats);
	if (victim->fighting == NULL)
		set_fighting(victim, ch);

	// First, try to rout the leader of the group
	baseChance = skill - rout_chance_modifier_both(ch, ch);
	chance = baseChance - 30 + rout_chance_modifier_resist(leader, leader);
	act("You thrust and hedge with $p, trying to sow confusion amongst $N's group!", ch, obj, leader, TO_CHAR);
	act("$n thrusts and hedges with $p, trying to sow confusion amongst your group!", ch, obj, leader, TO_VICT);
	act("$n thrusts and hedges with $p, trying to sow confusion amongst $N's group!", ch, obj, leader, TO_NOTVICT);

	
	if (number_percent() < URANGE(5, chance, 95))
	{
		// Success against the leader -- disband the group after checking for fleeing
		act("Your deft movements rout $N's group, which breaks apart, disoriented!", ch, obj, leader, TO_CHAR);
		act("$n's deft movements rout your group, which breaks apart, disoriented!", ch, obj, leader, TO_VICT);
		act("$n's deft movements rout $N's group, which breaks apart, disoriented!", ch, obj, leader, TO_NOTVICT);

		for (groupmate = victim->in_room->people; groupmate != NULL; groupmate = groupmate->next_in_room)
		{
			if (is_same_group(groupmate, leader) && groupmate->master != leader)
				rout_victim(groupmate, leader);
		}
		success = TRUE;
	}
	else
	{
		// Failure against the leader -- check the rest of the group.
		// Each groupmate gets slightly more resistance than the leader did.
		for (groupmate = victim->in_room->people; groupmate != NULL; groupmate = groupmate->next_in_room)
		{
			if (groupmate == leader || !is_same_group(groupmate, leader) || groupmate->master == leader)
				continue;

			chance = baseChance - 35 + rout_chance_modifier_resist(groupmate, leader);
			if (number_percent() < URANGE(5, chance, 95))
			{
				success = TRUE;
				rout_victim(groupmate, leader);
			}
		}
	}
	
	if (success) 
	{
	    for (groupmate = ch->in_room->people; groupmate != NULL; groupmate = groupmate->next)
	    {
		if (is_same_group(groupmate,ch))
		    if (groupmate->fighting != NULL)
		    {
                send_to_char("You take advantage of the confusion, striking at your opponent.\n\r",groupmate);
		        one_hit(groupmate,groupmate->fighting,TYPE_UNDEFINED,HIT_PRIMARY);
		    }
	    }	
	    check_improve(ch, leader, gsn_rout, TRUE, 0);
	}
	else 
	    check_improve(ch, leader, gsn_rout, FALSE, 1);

	check_killer(ch,victim);
}

void do_pommel( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj;
    CHAR_DATA *victim;
    int chance;
    if ((chance = get_skill(ch, gsn_pommel)) == 0)
    {
	send_to_char("Huh?\n\r", ch );
	return;
    }

    if ( ( victim = ch->fighting ) == NULL )
    {
	send_to_char( "You aren't fighting anyone.\n\r", ch );
	return;
    }

    if ( ( obj = get_eq_char(ch, WEAR_WIELD) ) == NULL)
    {
	send_to_char( "You must wield a sword to bash someone with the pommel.\n\r", ch);
	return;
    }

    if (obj->value[0] != WEAPON_SWORD)
    {
	send_to_char( "You must wield a sword to bash someone with the pommel.\n\r", ch);
	return;
    }

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;

    chance -= get_curr_stat(victim,STAT_DEX);
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
        chance -= 10;
    chance += (ch->level - victim->level);

    WAIT_STATE( ch, skill_table[gsn_pommel].beats );
    if ( (chance*3/5) > number_percent())
    {
	damage_from_obj(ch,victim,obj,number_range( ch->level, ch->level*2 ), gsn_pommel,DAM_BASH,TRUE);
	if (victim != NULL && victim->in_room != NULL)
            if (!is_affected(victim,gsn_pommel))
	    {
	        AFFECT_DATA paf;
	        paf.point = NULL;
	        paf.valid = TRUE;
	        paf.duration = 1;
	        paf.type = gsn_pommel;
	        paf.level = ch->level;
	        paf.modifier = 0;
	        paf.bitvector = 0;
	        paf.where = TO_AFFECTS;
	        paf.location = APPLY_NONE;
	        affect_to_char(victim,&paf);
	        act("$N reels from the blow!",ch,NULL,victim,TO_CHAR);
	        act("$N reels from the blow!",ch,NULL,victim,TO_NOTVICT);
	        act("You reel from the blow!",victim,NULL,NULL,TO_CHAR);
	    }	
	check_improve(ch,victim,gsn_pommel,TRUE,1);
    }
    else
    {
	damage( ch, victim, 0, gsn_pommel,DAM_BASH,TRUE);
	check_improve(ch,victim,gsn_pommel,FALSE,1);
    }
	check_killer(ch,victim);
    return;
}

void do_hew( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *wield, *shield;
    int dam;
    int victim_ac;
    int thac0;
    int thac0_00;
    int thac0_32;
    int diceroll;
    int sn,skill;
    int dam_type;
    int hewskill;
    int was_hurt;
    bool result;
    int dt = TYPE_UNDEFINED;

    sn = -1;

    if ((hewskill = get_skill(ch, gsn_hew)) == 0)
    {
	send_to_char("Huh?\n\r", ch );
	return;
    }

    if (argument[0]=='\0')
    {
	if ((victim = ch->fighting) == NULL)
	{
	    send_to_char("Who did you want to hew?\n\r", ch);
	    return;
	}
    }
    else if ((victim = get_char_room(ch, argument))==NULL)
    {
	send_to_char("You don't see them here.\n\r", ch);
	return;
    }

    if (victim == ch)
    {
	send_to_char("You attempt to hew yourself in half, but fail.\n\r", ch);
	return;
    } 

/*
 * Check if they're affected by diamond skin
 */
    if (is_affected(victim, gsn_diamondskin) && IS_SET(victim->imm_flags, IMM_WEAPON))
    {
        act("$N is unaffected by your hew!", ch, NULL, victim, TO_CHAR);
        act("Your diamond skin deflects $n's hew!", ch, NULL, victim, TO_VICT);
        act("$N is unaffected by $n's hew!", ch, NULL, victim, TO_NOTVICT);
        WAIT_STATE(ch,skill_table[gsn_hew].beats);
        return;
    }

    if (!check_reach(ch, victim, REACH_HARD, PULSE_VIOLENCE))
	return;

    /*
     * Figure out the type of damage message.
     */
    wield = get_eq_char( ch, WEAR_WIELD );

    if (wield == NULL || (wield->value[0] != WEAPON_AXE 
      && wield->value[0] != WEAPON_SWORD))
    {
	send_to_char("You can't hew someone in half without an axe or sword!\n\r", ch);
	return;
    }

    /* Check for chopping them in half */

    if (is_safe(ch, victim))
	return;

    //This is a somewhat hacky piece I'm putting in to get rid of the below -- you should
    //be able to initiate combat with hew.  I'm just going to invoke the variable twice further
    //down.  If it's still at 100, everything proceeds normally.  If not, there's no chance of 
    //instakill, and damage is multiplied by 45/100
    
    was_hurt = 100;
    if (victim->hit < victim->max_hit * 75/100 && ch->fighting != victim)
    {   was_hurt = 45;
//	send_to_char("They're too hurt and suspicious of such a blow.\n\r", ch);
//	return;
    }

    if ((shield = get_eq_char(victim, WEAR_ABOUT))
     && (shield->pIndexData->vnum == OBJ_VNUM_MANTLE_EARTH)
     && (number_bits(2) != 0))
    {
        act("$N's mantle of earth stops your cleave!", ch, NULL, victim, TO_CHAR);
        act("Your mantle of earth stops $n's cleave!", ch, NULL, victim, TO_VICT);
        act("$N's mantle of earth stops $n's cleave!", ch, NULL, victim, TO_NOTVICT);

    	if (ch->fighting == NULL && !IS_NPC(victim))
    	{
   	    sprintf(buf, "Help! %s is trying to cut me in half!", PERS(ch, victim));
    	    do_autoyell(victim, buf);
        }

	if (!victim->fighting)
	    victim->fighting = ch;

        return;
    }

    if (ch->fighting == NULL && was_hurt == 100 
      && !IS_SET(victim->act, ACT_NOSUBDUE) 
      && (number_percent() < (hewskill * (wield->value[0] == WEAPON_AXE ? get_skill(ch, gsn_axe) : get_skill(ch, gsn_sword)) * 5) /10000))
    {
        act( "You hew $N's torso right off $S body!",  ch, NULL, victim, TO_CHAR    );
	    act( "$n hews your torso right off your body!", ch, NULL, victim, TO_VICT    );
        act( "$n hews $N's torso right off $S body!",  ch, NULL, victim, TO_NOTVICT );
    	if ((IS_NPC(victim) && IS_SET(victim->act, ACT_UNDEAD)) || IS_SET(victim->form, FORM_UNDEAD))	
	        act("$n is DESTROYED!", victim, 0, 0, TO_ROOM);
    	else
	        act( "$n is DEAD!!", victim, 0, 0, TO_ROOM );
        
        kill_char(victim, ch);
    }
	
    WAIT_STATE( ch, skill_table[gsn_hew].beats );

    if ( victim->position == POS_DEAD || ch->in_room != victim->in_room )
	return;

    if (ch->fighting == NULL && !IS_NPC(victim))
    {
        sprintf(buf, "Help! %s is trying to cut me in half!", PERS(ch, victim));
        do_autoyell(victim, buf);
    }

    dt = TYPE_HIT + wield->value[3];
    dam_type = wield->value[3];

    if (dam_type == -1)
	dam_type = DAM_BASH;

    if (wield && obj_is_affected(wield, gsn_heatsink))
	damage_old(ch,ch,number_range(4,8),gsn_heatsink,DAM_COLD,TRUE);
     
    wrathkyana_combat_effect(ch, victim);

    if (is_an_avatar(ch))
    {
    	dt = TYPE_HIT + 20;
	    dam_type = DAM_HOLY;
    }
    
    AFFECT_DATA * aspectAff(get_affect(ch, gsn_aspectoftheinferno));
    if (aspectAff != NULL && aspectAff->modifier == 0)
        dam_type = DAM_FIRE;

    /* get the weapon skill */
    sn = get_weapon_sn(ch);
    skill = (25 + get_weapon_skill(ch,sn,FALSE)) * hewskill / 100;

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
	case(DAM_PIERCE):victim_ac = GET_AC(victim,AC_PIERCE)/10;	break;
	case(DAM_BASH):	 victim_ac = GET_AC(victim,AC_BASH)/10;		break;
	case(DAM_SLASH): victim_ac = GET_AC(victim,AC_SLASH)/10;	break;
	default:	 victim_ac = GET_AC(victim,AC_EXOTIC)/10;	break;
    }; 
	
    if (victim_ac < -15)
	victim_ac = (victim_ac + 15) / 5 - 15;
     
    if (!can_see(ch, victim) && !is_blindfighter(ch,FALSE))
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
	damage( ch, victim, 0, gsn_hew, dam_type, TRUE );
	check_improve(ch,victim,gsn_hew,FALSE,1);	
	return;
    }

    /*
     * Hit.
     * Calc damage.
     */
    check_improve(ch,victim,gsn_hew,TRUE,1);
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
		dam = dice(wield->value[1],wield->value[2]) * skill/100;
	    else
	    	dam = number_range( wield->value[1] * skill/100, 
				wield->value[2] * skill/100);

	    dam = dam * 11/10;
	}
	else
	    dam = number_range( 1 + 4 * skill/100, 2 * ch->level/3 * skill/100);
    }

    /*Bonuses.*/
    
    dam += check_extra_damage(ch,dam,wield);
    //If they're severely hurt, the below invokes a 1/10th reduction
    dam = (dam * was_hurt) / 100;
    if ( !IS_AWAKE(victim) )
	dam *= 2;
     else if (victim->position < POS_FIGHTING)
	dam = dam * 5 / 4;

    dam += GET_DAMROLL(ch) * UMIN(100,skill) /100;

    if ( dam <= 0 )
	dam = 1;

    /* It's hew, hugh */
    dam *= 2;
    if (!IS_PAFFECTED(ch,AFF_TWOHAND))
	dam = round((dam * 4)/5);
    result = damage_from_obj( ch, victim, wield, dam, gsn_hew, dam_type, TRUE );
}


void do_ambush( CHAR_DATA *ch, char *argument )
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

    if (!get_skill(ch, gsn_ambush))
    {
	send_to_char(
	    "Huh?\n\r", ch );
	return;
    }

    if (ch->fighting != NULL)
	{
	send_to_char("You can't ambush someone while you're fighting!\n\r", ch);
	return;
	}

   if (argument[0]=='\0')
	{
	send_to_char("Who did you want to ambush?\n\r", ch);
	return;
	}

	if ((victim = get_char_room(ch, argument))==NULL)
		{
		send_to_char("You don't see them here.\n\r", ch);
		return;
		}

    if (is_safe(ch, victim))
	return;

    /*
     * Figure out the type of damage message.
     */
    wield = get_eq_char( ch, WEAR_WIELD );

    if (can_see(victim, ch) && IS_AWAKE(victim))
	{
	send_to_char("They already saw you, no way to ambush them.\n\r", ch);
	return;
	}

    if (victim->hit < victim->max_hit * 75/100 && IS_AWAKE(victim))
	{
	send_to_char("They're too hurt and suspicious to not be ready for an ambush.\n\r", ch);
	return;
	}

    if ( victim->position == POS_DEAD || ch->in_room != victim->in_room )
	return;

    WAIT_STATE( ch, skill_table[gsn_ambush].beats );

    if (!IS_NPC(victim))
    {
	sprintf(buf, "Help! %s is ambushing me!", PERS(ch, victim));
	do_autoyell(victim, buf);
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
	    dt = TYPE_HIT + 20;
    	dam_type = DAM_HOLY;
    }

    /* get the weapon skill */
    sn = get_weapon_sn(ch);
    skill = 20 + get_weapon_skill(ch,sn,TRUE);

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
	case(DAM_PIERCE):victim_ac = GET_AC(victim,AC_PIERCE)/10;	break;
	case(DAM_BASH):	 victim_ac = GET_AC(victim,AC_BASH)/10;		break;
	case(DAM_SLASH): victim_ac = GET_AC(victim,AC_SLASH)/10;	break;
	default:	 victim_ac = GET_AC(victim,AC_EXOTIC)/10;	break;
    }; 
	
    if (victim_ac < -15)
	victim_ac = (victim_ac + 15) / 5 - 15;
     
    if (!can_see(ch, victim) && !is_blindfighter(ch,FALSE))
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

    if (get_eq_char(victim,WEAR_WIELD)
      && get_skill(victim,gsn_retaliation) / 2 > number_percent())
    {
	int damage = 0;
	OBJ_DATA *weapon = get_eq_char(victim,WEAR_WIELD);
	if (weapon != NULL)
	{
	    act("$n stops $N's ambush!",victim,NULL,ch,TO_NOTVICT);
	    act("You stop $N's ambush!",victim,NULL,ch,TO_CHAR);
	    act("$n stops your ambush!",victim,NULL,ch,TO_VICT);
	    damage = dice(weapon->value[1],weapon->value[2]) + GET_DAMROLL(victim);
	    damage += check_extra_damage(victim,damage,weapon);
	    damage_from_obj(victim,ch,weapon,damage,weapon->value[0],weapon->value[3],TRUE);
	    wrathkyana_combat_effect(victim,ch);
	    check_improve(victim,ch,gsn_retaliation,TRUE,1);
	    return;
	}
    }

    if (( diceroll == 0
    || ( diceroll != 19 && diceroll < thac0 - victim_ac ) )
    && !is_affected(ch, gsn_form_of_the_serpent))
    {
	/* Miss. */
	damage( ch, victim, 0, gsn_ambush, dam_type, TRUE );

	/* Added by Pug -- they get a chance to dual hit even if the
		primary weapon misses, damnit */

	    check_improve(ch,victim,gsn_ambush,FALSE,1);	

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

    if (number_percent() < get_skill(ch, gsn_ambush) *8/10)
    {
        act( "You jump $N viciously, catching $M completely unprepared!",  ch, NULL, victim, TO_CHAR    );
        act( "$n jumps you suddenly, and you're caught unprepared!", ch, NULL, victim, TO_VICT    );
        act( "$n jumps $N with vicious efficiency, catching $M completely unprepared!",  ch, NULL, victim, TO_NOTVICT );
	WAIT_STATE(victim, (UMAX(victim->wait, 2*PULSE_VIOLENCE)));
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

    if ( dam <= 0 )
	dam = 1;

    /* It's ambush, baby */
    dam *= 2;    
       
    /* Erin: 2/3 damage for normal bandit ambushes */
    if (ch->class_num == global_int_class_bandit)
    {
        if (is_affected(ch,gsn_setambush) && get_modifier(ch->affected,gsn_setambush) == victim->id)
            WAIT_STATE(victim, UMAX(victim->wait, (2*PULSE_VIOLENCE) + number_bits(2) - 1));
        else
          dam = (dam * 2) / 3;
    }    
       
    result = damage( ch, victim, dam, gsn_ambush, dam_type, TRUE );

    check_improve(ch, victim,gsn_ambush, TRUE, 1);
    
}

int get_disarm_chance( CHAR_DATA *ch, CHAR_DATA *victim, int chance, bool offhand )
{
    AFFECT_DATA *paf;
    OBJ_DATA *obj = NULL;
    OBJ_DATA *vObj = NULL;
    int withmods;
    
    /* find weapon skills */
    obj = get_eq_char(ch,WEAR_WIELD);
    if (!offhand)
	vObj = get_eq_char(victim, WEAR_WIELD);
    else
	vObj = get_eq_char(victim, WEAR_DUAL_WIELD);

    if ( obj == NULL)
	withmods = chance * get_skill(ch, gsn_hand_to_hand) / 500;
    else
	withmods = chance/3;

    if (vObj)
	withmods += get_weapon_skill_weapon(ch,obj) - get_weapon_skill_weapon(victim,vObj);

    /* dex vs. strength */
    withmods += get_curr_stat(ch,STAT_DEX);
    withmods -= get_curr_stat(victim,STAT_STR);

    if (is_affected(victim,gsn_clumsiness))
	withmods += 10;

    /* level */
    withmods += (ch->level - victim->level);

    if (is_affected(victim, gsn_grip))
	withmods /= 6;

    if (obj_is_affected(vObj, gsn_bindweapon))
	withmods /= 6;

    if (vObj && vObj->value[0] == WEAPON_STAFF)
	withmods /= 6;

    if ((paf = affect_find(victim->affected, gsn_grease))
     && paf->modifier >= 3)
	withmods = withmods * 3 / 2;

    withmods = UMAX(1,withmods);

    if (offhand)
	whinyassbitch(ch,victim,"Offhand disarm",withmods);
    else
	whinyassbitch(ch,victim,"Disarm",withmods);

    return withmods;
}


void do_crab_disarm( CHAR_DATA *ch, CHAR_DATA *victim )
{
    OBJ_DATA *obj, *vObj;
    int chance;
    AFFECT_DATA *paf;
    chance = 100;

    if (!victim)
	return;
    
    if ((obj = get_eq_char(ch, WEAR_DUAL_WIELD)) != NULL)
    {
        if (obj->value[0] != WEAPON_SWORD)
	{
	    send_to_char("You must wield two swords to assail with the arms of a crab.\n\r",ch);
	    return;
	}
    }
    else
    {
	send_to_char("You must wield two swords to assail with the arms of a crab.\n\r",ch);
	return;
    }
    if ((obj = get_eq_char(ch, WEAR_WIELD)) != NULL)
    {
	if (obj->value[0] != WEAPON_SWORD)
	{
	    send_to_char("You must wield two swords to assail with the arms of a crab.\n\r",ch);
	    return;
	}
    }
    else
    {
	send_to_char("You must wield two swords to assail with the arms of a crab.\n\r",ch);
	return;
    }

    if (!(vObj = get_eq_char(victim, WEAR_WIELD)) || !can_see_obj(ch, vObj))
    {
	send_to_char( "You assail with the arms of a crab, but your opponent has no weapon.\n\r", ch );
	return;
    }
    
    chance += (ch->level - victim->level);

    chance += get_curr_stat(ch,STAT_DEX);
    chance -= 2 * get_curr_stat(victim,STAT_STR);

    chance += get_weapon_skill_weapon(ch, obj) - get_weapon_skill_weapon(victim,vObj) / 2;
    
    if (is_affected(victim, gsn_grip))
	chance /= 5;

    if (vObj && obj_is_affected(vObj, gsn_bindweapon))
	chance /= 6;
    
    if (vObj && vObj->value[0] == WEAPON_STAFF)
	chance /= 6;

    if ((paf = affect_find(victim->affected, gsn_grease))
     && paf->modifier >= 3)
	chance = chance * 3 / 2;
    
    if (is_affected(ch,gsn_form_of_the_crab))
	chance += get_skill(ch,gsn_form_of_the_crab) / 10;
 
    /* and now the attack */
    if (number_percent() < chance)
    {
	act("You assail $N with the arms of a crab!",ch,NULL,victim,TO_CHAR);
	act("$n assails you in a flurry of sword maneuvers!",ch,NULL,victim,TO_VICT);
	act("$n presses $N with a flurry of sword maneuvers!",ch,NULL,victim,TO_NOTVICT);
	disarm( ch, victim );
    }
    else
    {
	act("You assail with the arms of a crab, but fail to disarm $N.",ch,NULL,victim,TO_CHAR);
	act("$n assails you in a flurry of sword maneuvers, but fails to disarm you.",ch,NULL,victim,TO_VICT);
	act("$n tries to disarm $N with a flurry of sword maneuvers, but fails $s attempts.",ch,NULL,victim,TO_NOTVICT);
    }
    return;
}

void do_disarm( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    int chance;

    if ((chance = get_skill(ch,gsn_disarm)) == 0)
    {
	send_to_char( "You don't know how to disarm opponents.\n\r", ch );
	return;
    }

    if ((get_eq_char( ch, WEAR_WIELD ) == NULL)
    && !IS_NPC(ch) && (get_skill(ch,gsn_hand_to_hand) == 0))
    {
	send_to_char( "You must wield a weapon to disarm.\n\r", ch );
	return;
    }

    if ( ( victim = ch->fighting ) == NULL )
    {
	send_to_char( "You aren't fighting anyone.\n\r", ch );
	return;
    }

    if (!(obj = get_eq_char(victim, WEAR_WIELD)) || !can_see_obj(ch, obj))
    {
	send_to_char( "Your opponent is not wielding a weapon.\n\r", ch );
	return;
    }

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;

    chance = get_disarm_chance(ch, victim, chance, FALSE);
    int rskill = get_skill(victim, gsn_reversal);
    if (!can_see_obj(victim,obj))
	rskill = 0;

    /* and now the attack */
    if (number_percent() < chance)
    {
	    WAIT_STATE( ch, skill_table[gsn_disarm].beats );
    	check_improve(ch,victim,gsn_disarm,TRUE,1);
    if (number_percent() < (rskill * 3 / 4))
	{
	    if (get_eq_char(ch,WEAR_WIELD) != NULL)
	    {
		// In Soviet Russia, swordmaster disarms YOU!!
		act("$N swiftly responds to your disarm attempt, reversing it!", ch, NULL, victim, TO_CHAR);
		act("You swiftly respond to $n's disarm attempt, reversing it!", ch, NULL, victim, TO_VICT);
		act("$N swiftly responds to $n's disarm attempt, reversing it!", ch, NULL, victim, TO_NOTVICT);
	        disarm(victim,ch);
		check_improve(victim,ch,gsn_reversal,TRUE,1);
	    }
	    else
	    {
		act("You foil $N's disarm attempt with expert skill.",victim, NULL, ch, TO_CHAR);
		act("$n foils $N's disarm attempt with expert skill.",victim,NULL,ch,TO_NOTVICT);
		act("$n foils your disarm attempt with expert skill.",victim,NULL,ch,TO_VICT);
		check_improve(victim,ch,gsn_reversal,TRUE,2);
	    }
	}
	else
	{
	    disarm( ch, victim );
	    if (get_eq_char(ch,WEAR_WIELD) != NULL)
		check_improve(victim,ch,gsn_reversal,FALSE,1);
	}
    }
    else
    {
	WAIT_STATE(ch,skill_table[gsn_disarm].beats);
	act("You fail to disarm $N.",ch,NULL,victim,TO_CHAR);
	act("$n tries to disarm you, but fails.",ch,NULL,victim,TO_VICT);
	act("$n tries to disarm $N, but fails.",ch,NULL,victim,TO_NOTVICT);
	check_improve(ch,victim,gsn_disarm,FALSE,1);
    }
    check_killer(ch,victim);
    return;
}

void do_offhanddisarm( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    int chance;

    if ((chance = get_skill(ch, gsn_offhanddisarm)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if ( get_eq_char( ch, WEAR_WIELD ) == NULL
    &&   (get_skill(ch,gsn_hand_to_hand) == 0
    ||    (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_DISARM))))
    {
        send_to_char( "You must wield a weapon to disarm.\n\r", ch );
        return;
    }

    if ((victim = ch->fighting) == NULL)
    {
        send_to_char( "You aren't fighting anyone.\n\r", ch );
        return;
    }

    if ((((obj = get_eq_char(victim, WEAR_DUAL_WIELD)) == NULL)
      && ((obj = get_eq_char(victim, WEAR_SHIELD)    ) == NULL)
      && ((obj = get_eq_char(victim, WEAR_HOLD)      ) == NULL)
      && ((obj = get_eq_char(victim, WEAR_LIGHT)     ) == NULL))
     || !can_see_obj(ch, obj))
    {
        send_to_char( "Your opponent has nothing you can disarm in their off hand.\n\r", ch );
        return;
    }

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;

    chance = get_disarm_chance(ch, victim, chance, TRUE);
    int rskill = get_skill(victim, gsn_reversal);
    if (!can_see_obj(victim,obj))
	rskill = 0;

    /* and now the attack */
    if (number_percent() < chance)
    {
	WAIT_STATE( ch, skill_table[gsn_offhanddisarm].beats );
        check_improve(ch,victim,gsn_offhanddisarm,TRUE,1);
        if (number_percent() < rskill*3/4)
	{
	    if (get_eq_char(ch,WEAR_WIELD) != NULL)
	    {
		// In Soviet Russia, swordmaster disarms YOU!!
	        act("$N swiftly responds to your disarm attempt, reversing it!", ch, NULL, victim, TO_CHAR);
	        act("You swiftly respond to $n's disarm attempt, reversing it!", ch, NULL, victim, TO_VICT);
  	        act("$N swiftly responds to $n's disarm attempt, reversing it!", ch, NULL, victim, TO_NOTVICT);
		disarm(victim,ch);
		check_improve(victim,ch,gsn_reversal,TRUE,1);
	    }
	}
	else
	{
            check_improve(victim,ch,gsn_reversal,FALSE,1);
	    offhanddisarm( ch, victim );
	}
    }
    else
    {
        WAIT_STATE(ch,skill_table[gsn_offhanddisarm].beats);
        act("You fail to disarm $N's off hand.",ch,NULL,victim,TO_CHAR);
        act("$n tries to disarm your off hand, but fails.",ch,NULL,victim,TO_VICT);
        act("$n tries to disarm $N's off hand, but fails.",ch,NULL,victim,TO_NOTVICT);
        check_improve(ch,victim,gsn_offhanddisarm,FALSE,1);
    }
    check_killer(ch,victim);
    return;
}


void do_sla( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to SLAY, spell it out.\n\r", ch );
    return;
}



void do_slay( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        send_to_char( "Slay whom?\n\r", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }

    if ( !IS_NPC(victim) && ch != victim && get_trust(victim) >= get_trust(ch) )
    {
        send_to_char( "You failed.\n\r", ch );
        return;
    }

    act( "You slay $M in cold blood!",  ch, NULL, victim, TO_CHAR    );
    act( "$n slays you in cold blood!", ch, NULL, victim, TO_VICT    );
    act( "$n slays $N in cold blood!",  ch, NULL, victim, TO_NOTVICT );
    raw_kill( victim );
}

void do_smi( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to SMITE, spell it out.\n\r", ch );
    return;
}

void do_smite( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    int dam;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        send_to_char( "Smite whom?\n\r", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
        send_to_char( "They aren't here.\n\r", ch );
        return;
    }

    if ( ch == victim )
    {
        send_to_char( "Well... Okay.\n\r", ch );
    }

    if ( !IS_NPC(victim) && victim->level > get_trust(ch) )
    {
        send_to_char( "You failed.\n\r", ch );
        return;
    }

    act( "You send a bolt of lightning at $M!",  ch, NULL, victim, TO_CHAR );
    act( "A bolt of lightning from above {rstrikes{x you!", ch, NULL, victim, TO_VICT    );
    act( "A bolt of lightning from above {rstrikes{x $N!",  ch, NULL, victim, TO_NOTVICT );

    dam = victim->hit / 2;
    victim->hit /= 2;
    victim->mana /= 2;
    victim->move /= 2;
    if ( !IS_NPC(victim)
    &&   victim->level >= LEVEL_IMMORTAL
    &&   victim->hit < 1 )
        victim->hit = 1;
    update_pos( victim );

    switch( victim->position )
    {
    case POS_MORTAL:
        act( "$n is mortally wounded, and will die soon, if not aided.",
            victim, NULL, NULL, TO_ROOM );
        send_to_char(
            "You are mortally wounded, and will die soon, if not aided.\n\r",
            victim );
        break;

    case POS_INCAP:
        act( "$n is incapacitated and will slowly die, if not aided.",
            victim, NULL, NULL, TO_ROOM );
        send_to_char(
            "You are incapacitated and will slowly die, if not aided.\n\r",
            victim );
        break;

    case POS_STUNNED:
        act( "$n is stunned, but will probably recover.",
            victim, NULL, NULL, TO_ROOM );
        send_to_char("You are stunned, but will probably recover.\n\r",
            victim );
        break;

    case POS_DEAD:
	if ((IS_NPC(victim) && IS_SET(victim->act, ACT_UNDEAD)) || IS_SET(victim->form, FORM_UNDEAD))
	    act("$n is DESTROYED!", victim, 0, 0, TO_ROOM);
	else
            act( "$n is DEAD!!", victim, 0, 0, TO_ROOM );
        break;

    default:
        if ( dam > victim->max_hit / 4 )
            send_to_char( "That really did {RHURT{x!\n\r", victim );
        if ( victim->hit < victim->max_hit / 4 )
            send_to_char( "You sure are {RBLEEDING{x!\n\r", victim );
        break;
    }

    /*
     * Sleep spells and extremely wounded folks.
     */
    if ( !IS_AWAKE(victim) )
        stop_fighting( victim );

    /*
     * Payoff for killing things.
     */
    if ( victim->position == POS_DEAD )
        kill_char(victim, ch);
}

void do_sui( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to commit SUICIDE, spell it out.\n\r", ch );
    return;
}

void do_suicide( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
        send_to_char( "Suicide is a serious matter. If you really wish to commit suicide,\n\r", ch );
        send_to_char( "type 'suicide' with the argument 'yes'.\n\r",ch);
        return;
    }

    if ( IS_IMMORTAL(ch) )
    {
        send_to_char( "You failed. That is the problem with being immortal.\n\r", ch );
        return;
    }

    if (is_affected(ch, gsn_commandword))
    {
	send_to_char("You resist the urge to commit suicide.\n\r", ch);
	sprintf(log_buf, "Someone tried to force %s to commit suicide.", ch->name);
	wiznet(log_buf,ch,NULL,WIZ_FLAGS,0,0);
	return;
    }

    if (!strcmp(arg,"yes"))
    {
        act( "You take your own life.",  ch, NULL, ch, TO_CHAR );
        act( "You are DEAD!",  ch, NULL, ch, TO_CHAR );
        act( "$n takes $s own life.", ch, NULL, ch, TO_NOTVICT );
	sprintf(log_buf, "%s committed suicide in %s [%i]\n\r",ch->name, ch->in_room->name,ch->in_room->vnum); 
	log_string(log_buf);
	wiznet(log_buf,ch,NULL,WIZ_FLAGS,0,0);
	raw_kill( ch );
        return;
    }
}


void check_decapitate( CHAR_DATA *ch, CHAR_DATA *victim)
{
    OBJ_DATA *weapon;
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;
    int chance = 0;
    
    if (IS_SET(ch->nact, PLR_MERCY_DEATH) || IS_SET(ch->nact, PLR_AUTOATTACK))
	return;

    if ((weapon = get_eq_char(ch,WEAR_WIELD)) == NULL
      || !(weapon->value[0] == WEAPON_AXE
      || weapon->value[0] == WEAPON_SWORD
      || (weapon->value[0] == WEAPON_EXOTIC
        && weapon->value[3] == DAM_SLASH)))
	return;

    if (IS_SET(victim->act, ACT_NOSUBDUE) 
      || IS_SET(victim->act, ACT_BADASS))
	return;

    if (ch->level < skill_table[gsn_decapitate].skill_level[global_int_class_gladiator])
	return;

    if (is_affected(victim, gsn_diamondskin) && IS_SET(victim->imm_flags, IMM_WEAPON))
    {
	act("$N is unaffected by your blow!", ch, NULL, victim, TO_CHAR);
	act("Your diamond skin deflects $n's decapitating blow!", ch, NULL, victim, TO_VICT);
	act("$N is unaffected by $n's blow!", ch, NULL, victim, TO_NOTVICT);
	return;
    }
    if (victim->hit > victim->max_hit * .15)
	return;
    if (is_safe(ch,victim))
	return;
    if (!check_reach(ch, victim, REACH_HARD, PULSE_VIOLENCE))
	return;
	
    if ((chance = get_skill(ch,gsn_decapitate)/4) == 0)
	return;

    if (victim->in_room == NULL || ch->in_room == NULL)
	return;

    /* stats */
    chance += get_curr_stat(ch,STAT_STR);
    chance -= (get_curr_stat(victim,STAT_DEX) * 4)/3;
    chance += GET_AC(victim,AC_SLASH)/50;
    /* speed */
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
	chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
	chance -= 20;
    /* level */
    chance += (ch->level - victim->level);
    chance = URANGE(0, chance, 20);

    /* now the attack */
    if (number_percent() < chance )
    {
        act("$n takes advantage of your weakened state, and delivers a killing blow!",
		ch,NULL,victim,TO_VICT);
	act("You swing mightily, and lop off $N's head!",ch,NULL,victim,TO_CHAR);
	act("$n swings mightily, and lops $N's head off!",
		ch,NULL,victim,TO_NOTVICT);
	check_improve(ch,victim,gsn_decapitate,TRUE,2);
        act("$n's killing blow {r### DECAPITATES ###{x you!",
		ch,NULL,victim,TO_VICT);
	act("Your killing blow {r### DECAPITATES ###{x $N!",ch,NULL,victim,TO_CHAR);
	act("$n's killing blow {r### DECAPITATES ###{x $N!",
		ch,NULL,victim,TO_NOTVICT);

	if ((IS_NPC(victim) && IS_SET(victim->act, ACT_UNDEAD)) || IS_SET(victim->form, FORM_UNDEAD))	
	    act("$n is DESTROYED!", victim, 0, 0, TO_ROOM);
	else
	    act( "$n is DEAD!!", victim, 0, 0, TO_ROOM );
	
    if ( !IS_NPC(victim) )
	{
	    sprintf( log_buf, "%s decapitated by %s at %d",
		victim->name,
		(IS_NPC(ch) ? ch->short_descr : ch->name),
		ch->in_room->vnum );
	    log_string( log_buf );
	}
        sprintf( log_buf, "%s got toasted by %s at %s [room %d]",
            (IS_NPC(victim) ? victim->short_descr : victim->name),
            (IS_NPC(ch) ? ch->short_descr : ch->name),
            ch->in_room->name, ch->in_room->vnum);
 
        if (IS_NPC(victim))
            wiznet(log_buf,NULL,NULL,WIZ_MOBDEATHS,0,0);
        else
            wiznet(log_buf,NULL,NULL,WIZ_DEATHS,0,0);

	af.where	= TO_AFFECTS;
	af.type		= gsn_decapitate;
	af.level	= ch->level;
	af.duration	= 0;
	af.modifier	= 0;
	af.location	= 0;
	af.bitvector	= 0;
	affect_to_char(victim, &af);

	kill_char(victim, ch);

	WAIT_STATE(ch,skill_table[gsn_decapitate].beats);
    }
    else
    {
        damage(ch,victim,0,gsn_decapitate,DAM_BASH,FALSE);
	act("$N escapes your killing blow!",
	    ch,NULL,victim,TO_CHAR);
	act("$n misses $N with $s killing blow.",
	    ch,NULL,victim,TO_NOTVICT);
	act("You escape $n's killing blow by a hair.",
	    ch,NULL,victim,TO_VICT);
	check_improve(ch,victim,gsn_decapitate,FALSE,3);
    }
    check_killer(ch,victim);
}


void do_bludgeon( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *wield;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;
    int dam;
    int dam_type = 0;
    int sn, skill;
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;

    one_argument(argument,arg);
    wield = get_eq_char( ch, WEAR_WIELD );

    /* get the weapon skill */
    sn = get_weapon_sn(ch);
    skill = 20 + get_weapon_skill(ch,sn,TRUE);
 
    if ( (chance = get_skill(ch,gsn_bludgeon)) == 0)
    {	
	send_to_char("Huh?\n\r",ch);
	return;
    }
     
    if (wield == NULL)
    {
	send_to_char("You must wield a blunt weapon to bludgeon.\n\r", ch);
	return;
    }

    dam_type = wield->value[3];

    if (wield->value[0] != WEAPON_MACE && wield->value[0] != WEAPON_FLAIL)
    {
	send_to_char("You must use a blunt weapon to bludgeon.\n\r", ch);
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

    if (victim->position < POS_FIGHTING)
    {
	act("You'll have to let $M get back up first.",ch,NULL,victim,TO_CHAR);
	return;
    } 

    if (victim == ch)
    {
	send_to_char("You try to smash your brains out, but fail.\n\r",ch);
	return;
    }

    if (is_safe(ch,victim))
	return;

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
	act("But $N is your friend!",ch,NULL,victim,TO_CHAR);
	return;
    }

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;

    /* modifiers */
    if (ch->size < victim->size)
	chance += (ch->size - victim->size) * 15;
    else
	chance += (ch->size - victim->size) * 10; 

    /* stats */
    chance += get_curr_stat(ch,STAT_STR);
    chance -= (get_curr_stat(victim,STAT_DEX) * 4)/3;
    chance += GET_AC(victim,AC_BASH)/100;

    /* speed */
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
        chance -= 30;

    /* level */
    chance += (ch->level - victim->level);

    /* now the attack */
    if (number_percent() < chance )
    {
        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
            sprintf( buf, "Help! %s is smashing me!", PERS(ch, victim) );
            do_autoyell( victim, buf );
        }	
        act("$n swings with a huge blow, and connects! You feel lightheaded!",
	  ch,NULL,victim,TO_VICT);
	act("You swing mightily, and pound $N dizzy!",ch,NULL,victim,TO_CHAR);
	act("$n crushes $N with an incredible smash.",
	  ch,NULL,victim,TO_NOTVICT);
	check_improve(ch,victim,gsn_bludgeon,TRUE,1);

	WAIT_STATE(ch,skill_table[gsn_bludgeon].beats);
	if (number_percent() < get_skill(ch,gsn_bludgeon) / 4 )
	{
            act("You feel stunned from the blow!",
	      ch,NULL,victim,TO_VICT);
	    act("$N looks stunned from your blow!",ch,NULL,victim,TO_CHAR);
	    act("$N looks stunned from the blow!",
	      ch,NULL,victim,TO_NOTVICT);
	    WAIT_STATE(victim, 2 * PULSE_VIOLENCE);
	}
	else if (number_percent() < get_skill(ch,gsn_bludgeon) / 4)
	{
	    act("You reel from the blow!", ch, NULL, victim, TO_VICT);
	    act("$N reels from the blow!", ch, NULL, victim, TO_NOTVICT);
	    act("$N reels from your blow!", ch, NULL, victim, TO_CHAR);
	    af.where	= TO_AFFECTS;
	    af.type	= gsn_bludgeon;
	    af.level	= ch->level;
	    af.duration	= 1;
	    af.modifier	= 0;
	    af.location	= 0;
	    af.bitvector = 0;
	    affect_to_char(victim, &af);
	}
        switch_position(victim, POS_RESTING);

        if (wield->pIndexData->new_format)
	    dam = dice(wield->value[1],wield->value[2]) * skill/100;
        else
	    dam = number_range( wield->value[1] * skill/100, 
				wield->value[2] * skill/100);

	dam += ch->damroll;
	dam += check_extra_damage(ch,dam,wield);

	if ( !IS_AWAKE(victim) )
	    dam *= 2;
	else if (victim->position < POS_FIGHTING)
	    dam = dam * 5 / 4;
	if (!IS_PAFFECTED(ch,AFF_TWOHAND))
	    dam = round((dam * 4) / 5);

	if ( dam <= 0 )
	    dam = 1;

	damage_from_obj(ch,victim,wield,dam,gsn_bludgeon,DAM_BASH,TRUE);
    }
    else
    {
        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
	    sprintf( buf, "Help! %s is smashing me!", PERS(ch, victim) );
	    do_autoyell( victim, buf );
        }	
        damage(ch,victim,0,gsn_bludgeon,DAM_BASH,FALSE);
	act("Your swing goes wide!",
	    ch,NULL,victim,TO_CHAR);
	act("$n misses $N with $s mighty swing.",
	    ch,NULL,victim,TO_NOTVICT);
	act("You evade $n's mighty swing.",
	    ch,NULL,victim,TO_VICT);
	check_improve(ch,victim,gsn_bludgeon,FALSE,1);
	WAIT_STATE(ch,skill_table[gsn_bludgeon].beats); 
    }
    check_killer(ch,victim);
}

void do_ensnare( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *wield;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;
    int dam;

    one_argument(argument,arg);

    if ( (chance = get_skill(ch,gsn_ensnare)) == 0)
    {	
	send_to_char("Huh?\n\r",ch);
	return;
    }
     
    wield = get_eq_char(ch, WEAR_WIELD);

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

    if (victim == ch)
    {
	send_to_char("You tangle no more than your tongue.\n\r",ch);
	return;
    }

    if ((wield == NULL) || (wield->value[0] != WEAPON_WHIP))
    {
	wield = get_eq_char(ch, WEAR_DUAL_WIELD);
	if (wield == NULL)
	{
	    send_to_char("You can't ensnare someone without a whip!\n\r", ch);
	    return;
	}
	if (wield->value[0] != WEAPON_WHIP)
	{
	    send_to_char("You can't ensnare someone without a whip!\n\r", ch);
	    return;
	}
    }

    if (is_safe(ch,victim))
	return;

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
	act("But $N is your friend!",ch,NULL,victim,TO_CHAR);
	return;
    }

    if (!check_reach(ch, victim, REACH_EASY, PULSE_VIOLENCE))
	return;

    /* modifiers */


    /* stats */
    chance -= GET_AC(victim,AC_BASH) / 25;
    /* speed */
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
        chance -= 30;

    /* level */
    chance += (ch->level - victim->level);
    chance /= 2;
    if (is_flying(victim))
	chance /= 2;

    if (IS_NAFFECTED(victim, AFF_AGILITY) 
	  && (number_percent() < get_skill(victim,gsn_agility)/2+((SIZE_MEDIUM - victim->size)*10)))
	chance /= 2;

    /* now the attack */
    if (number_percent() < chance )
    {
        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
          sprintf( buf, "Help!  %s is entangling me!", PERS(ch, victim));
          do_autoyell( victim, buf );
        }	
        act("$n ensnares your legs and you go down!",
		ch,NULL,victim,TO_VICT);
	act("You ensnare $N's legs, sending $M to the ground!",ch,NULL,victim,TO_CHAR);
	act("$n ensnares $N's legs, sending $M to the ground!",
		ch,NULL,victim,TO_NOTVICT);
	check_improve(ch,victim,gsn_ensnare,TRUE,1);

	WAIT_STATE(victim, UMAX(victim->wait, 2*PULSE_VIOLENCE+number_bits(1)));
	WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_ensnare].beats));

    dam = number_bits(2);

    if ( dam <= 0 )
	dam = 1;

	damage(ch,victim,dam,gsn_ensnare,
	    DAM_BASH,TRUE);
    }
    else
    {
        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
          sprintf( buf, "Help!  %s is trying to ensnare me!", PERS(ch, victim) );
          do_autoyell( victim, buf );
        }	
	act("You try to ensnare $N's legs, but fail.",
	    ch,NULL,victim,TO_CHAR);
	act("$n tries to ensnare $N's legs, but $N avoids it.",
	    ch,NULL,victim,TO_NOTVICT);
	act("You evade $n's attempt to ensnare you.",
	    ch,NULL,victim,TO_VICT);
        damage(ch,victim,0,gsn_ensnare,DAM_PIERCE,FALSE);
	check_improve(ch,victim,gsn_ensnare,FALSE,1);
	WAIT_STATE(ch,skill_table[gsn_ensnare].beats); 
    }
	check_killer(ch,victim);
}


void do_grapple( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;
    int dam;
    OBJ_DATA *net = NULL;

    one_argument(argument,arg);

    if ( (chance = get_skill(ch,gsn_grapple)) == 0)
    {	
	send_to_char("Huh?\n\r",ch);
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

    if (victim == ch)
    {
	send_to_char("You wrestle yourself around. How odd.\n\r",ch);
	return;
    }
    
    if (ch->fighting != victim)
    {
         send_to_char("You have to be fighting someone before you can grapple them.\n\r",ch);
         return;
    }

    if (is_safe(ch,victim))
	return;

    if (ch->in_room->sector_type == SECT_AIR)
    {   
	send_to_char("You can't grapple effectively in midair!\n\r", ch);
        return;
    }

    if (ch->in_room->sector_type == SECT_WATER_NOSWIM 
     || ch->in_room->sector_type == SECT_WATER_SWIM
     || ch->in_room->sector_type == SECT_UNDERWATER)
    {
	send_to_char("You can't grapple effectively in the water!\n\r", ch);    
        return;
    }	    

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
	act("But $N is your friend!",ch,NULL,victim,TO_CHAR);
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

    if (is_affected(victim,gsn_astralform) && number_bits(1) != 0)
    {
        act("You try to grapple at $N, but your arms pass right through $M!",
            ch,NULL,victim,TO_CHAR);
        act("$n tries to grapple with $N, but $s arms pass right through $M!",
            ch,NULL,victim,TO_NOTVICT);
        act("$n tries to grapple with you, but $s arms pass right through you!"
            ,ch,NULL,victim,TO_VICT);

	WAIT_STATE(ch,skill_table[gsn_grapple].beats);
        return;
     }

    if (IS_NAFFECTED(victim, AFF_AGILITY) 
	  && (number_percent() < get_skill(victim,gsn_agility)/2+((SIZE_MEDIUM - victim->size)*10)))
    {
	act("You try to grapple with $N, but $E spins nimbly out of the way!",
	    ch, NULL, victim, TO_CHAR);
        act("$n tries to grapple with $N, but $E spins nimbly out of the way!",
	    ch, NULL, victim, TO_NOTVICT);
        act("$n tries to grapple with you, but you spin nimbly out of the way!",
	    ch, NULL, victim, TO_VICT);

	WAIT_STATE(ch,skill_table[gsn_grapple].beats);
	return;
    }

    if (!check_reach(ch, victim, REACH_HARD, PULSE_VIOLENCE))
	return;

    /* stats */
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
    
    chance -= get_curr_stat(victim,STAT_DEX);
    chance -= GET_AC(victim,AC_PIERCE) /25;
    /* speed */
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
        chance -= 30;

    chance += (ch->size * 5 - victim->size * 5);

    /* level */
    chance += (ch->level - victim->level);
    check_killer(ch,victim);

    /* now the attack */
    if (number_percent() < chance )
    {
        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
          sprintf( buf, "Help!  %s is attacking me!", PERS(ch, victim));
          do_autoyell( victim, buf );
        }	
        act("$n grabs you, and twists you around, crushing you into the ground!",
		ch,NULL,victim,TO_VICT);
	act("You grab $N and crush $M into the ground!",ch,NULL,victim,TO_CHAR);
	act("$n grabs $N, and takes $M to the ground in a crushing grab.",
		ch,NULL,victim,TO_NOTVICT);
	check_improve(ch,victim,gsn_grapple,TRUE,1);

	WAIT_STATE(ch,skill_table[gsn_grapple].beats);

        dam = dice(ch->level, 2);

        if ( !IS_AWAKE(victim) )
	    dam *= 2;
        else if (victim->position < POS_FIGHTING)
	    dam = dam * 5 / 4;

        if ( dam <= 0 )
	    dam = 1;

	damage(ch,victim,dam,gsn_grapple, DAM_BASH,TRUE);
	
	if (IS_OAFFECTED(victim,AFF_GHOST) || ch->in_room != victim->in_room)
	    return;

	if (!is_affected(victim, gsn_grapple))
	{
	    act("Your limbs feel harder to move around.",victim, NULL, NULL, TO_CHAR);
	    act("You grind $N's bones together.",ch,NULL,victim,TO_CHAR);
	    af.where	= TO_NAFFECTS;
	    af.type	= gsn_grapple;
	    af.level	= ch->level;
	    af.duration	= 1;
	    af.modifier	= -1 * ch->level/12;
	    af.location	= APPLY_DEX;
	    af.bitvector= 0;
	    affect_to_char(victim, &af);

	    af.location	= APPLY_STR;
	    affect_to_char(victim, &af);
	}
	else if (number_bits(1) == 0 && !IS_NAFFECTED(ch,AFF_GRAPPLE))
	{
	  act("$n looks dazed from the smash into the ground.", victim, NULL, NULL, TO_ROOM);
	  act("You feel dazed from the brutal grappling.", victim, NULL, NULL, TO_CHAR);
	  af.where	= TO_NAFFECTS;
	  af.type	= gsn_grapple;
	  af.level	= ch->level;
	  af.duration	= 0;
	  af.modifier	= 0;
	  af.location	= 0;
	  af.bitvector = AFF_GRAPPLE;
	  affect_to_char(victim, &af);
	}
    }
    else
    {
        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
          sprintf( buf, "Help!  %s is attacking me!", PERS(ch, victim) );
          do_autoyell( victim, buf );
        }	
        damage(ch,victim,0,gsn_grapple,DAM_PIERCE,FALSE);
	act("You try to grapple $N, but can't hold on!",
	    ch,NULL,victim,TO_CHAR);
	act("$n tries to grab $N in a hold, but $N slips out!",
	    ch,NULL,victim,TO_NOTVICT);
	act("You evade $n's attempt to grapple you.",
	    ch,NULL,victim,TO_VICT);
	check_improve(ch,victim,gsn_grapple,FALSE,1);
	WAIT_STATE(ch,skill_table[gsn_grapple].beats); 
    }
    check_killer(ch,victim);
}

void do_slice( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    OBJ_DATA *wield;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;
    int dam;
    int sn, skill;

    one_argument(argument,arg);
    wield = get_eq_char( ch, WEAR_WIELD );

    /* get the weapon skill */
    sn = get_weapon_sn(ch);
    skill = 20 + get_weapon_skill(ch,sn,TRUE);

    if ( (chance = get_skill(ch,gsn_slice)) == 0)
    {	
	send_to_char("Huh?\n\r",ch);
	return;
    }
     
    if ((wield == NULL) || (wield->value[0] != WEAPON_DAGGER))
	if ((wield = get_eq_char(ch,WEAR_DUAL_WIELD)) == NULL
	  || (wield->value[0] != WEAPON_DAGGER))
	{
	    send_to_char("You must wield a dagger to slice.\n\r", ch);
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

    if (victim == ch)
    {
	send_to_char("You think that's a really bad idea.\n\r",ch);
	return;
    }

    if (is_safe(ch,victim))
	return;

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
	act("But $N is your friend!",ch,NULL,victim,TO_CHAR);
	return;
    }

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;

    /* stats */
    chance += get_curr_stat(ch,STAT_STR)/2;
    chance += get_curr_stat(ch,STAT_DEX);
    chance -= get_curr_stat(victim,STAT_DEX);
    chance -= GET_AC(victim,AC_SLASH) /25;
    /* speed */
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
        chance -= 30;

    /* level */
    chance += (ch->level - victim->level);

    if (!IS_NPC(victim) 
	&& chance < get_skill(victim,gsn_dodge) )
    {	
	chance -= 3 * (get_skill(victim,gsn_dodge) - chance);
    }


    /* now the attack */
    if (number_percent() < chance )
    {
        if (IS_AFFECTED(victim, AFF_FLY_NATURAL))
    	{
	    act("$n feints and rolls, coming around behind you to slash at your wings!", ch, NULL, victim, TO_VICT);
	    act("You feint and roll, coming around behind $N to slash at $S wings!", ch, NULL, victim, TO_CHAR);
	    act("$n feints and rolls, coming around behind $N to slash at $S wings!", ch, NULL, victim, TO_NOTVICT);
    	}
        else
    	{
            act("$n feints and rolls, coming around behind you to slash and hamstring you!", ch, NULL, victim, TO_VICT);
            act("You feint and roll, coming around behind $N to slash and hamstring $M!", ch, NULL, victim, TO_CHAR);
            act("$n feints and rolls, coming around behind $N to slash and hamstring $M!", ch, NULL, victim, TO_NOTVICT);
    	}
        
	if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
            sprintf( buf, "Help!  %s tried to slice me!", PERS(ch, victim));
            do_autoyell( victim, buf );
        }	
	check_improve(ch,victim,gsn_slice,TRUE,1);

	WAIT_STATE(ch,skill_table[gsn_slice].beats);

        if (!IS_AFFECTED(victim, AFF_FLYING))
        {
	    af.where	 = TO_AFFECTS;
	    af.type	 = gsn_slice;
	    af.level	 = ch->level;
	    af.duration	 = ch->level/8;
	    af.location	 = APPLY_DEX;
	    af.bitvector = 0;

	    if (is_affected(victim, gsn_slice))
	    {
		af.modifier = (get_modifier(victim->affected, gsn_slice) - 1);
		affect_strip(victim, gsn_slice);
	    }
	    else
	        af.modifier = -1 * ch->level/15;
	    affect_to_char(victim, &af);
        } 

        if (wield->pIndexData->new_format)
            dam = dice(wield->value[1],wield->value[2]) * skill/100;
        else
   	    dam = number_range( wield->value[1] * skill/100, 
				wield->value[2] * skill/100);

        dam += check_extra_damage(ch,dam,wield);

        if ( !IS_AWAKE(victim) )
	    dam *= 2;
        else if (victim->position < POS_FIGHTING)
	    dam = dam * 5 / 4;

        dam += GET_DAMROLL(ch) * UMIN(100,skill) /100;

        if ( dam <= 0 )
	    dam = 1;

	damage_from_obj(ch,victim,wield,dam,gsn_slice,
	    wield->value[3],TRUE);
    }
    else
    {
        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
          sprintf( buf, "Help!  %s tried to slice me!", PERS(ch, victim) );
          do_autoyell( victim, buf );
        }	
        damage(ch,victim,0,gsn_slice,DAM_PIERCE,FALSE);
	act("You slice and miss!",
	    ch,NULL,victim,TO_CHAR);
	act("$n misses $N with $s slice attempt.",
	    ch,NULL,victim,TO_NOTVICT);
	act("You evade $n's slice.",
	    ch,NULL,victim,TO_VICT);
	check_improve(ch,victim,gsn_slice,FALSE,1);
	WAIT_STATE(ch,skill_table[gsn_slice].beats); 
    }
    check_killer(ch,victim);
}



void do_hamstring( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    OBJ_DATA *wield;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;
    int dam;
    int sn, skill;
    int dam_type;
    one_argument(argument,arg);

    /* get the weapon skill */
    sn = get_weapon_sn(ch);
    skill = 20 + get_weapon_skill(ch,sn,TRUE);

    if ( (chance = get_skill(ch,gsn_hamstring)) == 0)
    {	
	send_to_char("You don't know how to hamstring anything.\n\r",ch);
	return;
    }
    
    if ((wield = get_eq_char(ch, WEAR_WIELD)) == NULL)
    {
	send_to_char("You must use a spear in your main hand to hamstring someone.\n\r",ch);
	return;
    }
 
    if (wield->value[0] != WEAPON_SPEAR 
      && (!(wield->value[0] == WEAPON_EXOTIC && wield->value[3] == DAM_SLASH)))
    {
	send_to_char("You must wield a spear in your main hand to hamstring.\n\r", ch);
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

    if (victim == ch)
    {
	send_to_char("You can't reach around to hamstring yourself anyhow.\n\r",ch);
	return;
    }

    if (is_safe(ch,victim))
	return;

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
	act("But $N is your friend!",ch,NULL,victim,TO_CHAR);
	return;
    }

    if (!check_reach(ch, victim, REACH_EASY, PULSE_VIOLENCE))
	return;

    /* modifiers */


    /* stats */
    chance += get_curr_stat(ch,STAT_STR)/2;
    chance += get_curr_stat(ch,STAT_DEX);
    chance -= get_curr_stat(victim,STAT_DEX);
    chance -= GET_AC(victim,AC_PIERCE) /25;
    /* speed */
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
        chance -= 30;

    /* level */
    chance += (ch->level - victim->level);

    if (!IS_NPC(victim) 
	&& chance < get_skill(victim,gsn_dodge) )
    {	
	chance -= 3 * (get_skill(victim,gsn_dodge) - chance);
    }

    /* now the attack */
    if (number_percent() < chance )
    {
        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
          sprintf( buf, "Help!  %s tried to slice me!", PERS(ch, victim));
          do_autoyell( victim, buf );
        }	
        act("$n hamstrings you with $s weapon!",
		ch,NULL,victim,TO_VICT);
	act("You hamstring $N with your weapon!",ch,NULL,victim,TO_CHAR);
	act("$n hamstrings $N with $s weapon.",
		ch,NULL,victim,TO_NOTVICT);
	check_improve(ch,victim,gsn_hamstring,TRUE,1);

	WAIT_STATE(ch,skill_table[gsn_hamstring].beats);

	if (!(is_affected(victim, gsn_hamstring)))
	{
	    af.where	 = TO_OAFFECTS;
	    af.type	 = gsn_hamstring;
	    af.level	 = ch->level;
	    af.duration	 = ch->level/8;
	    af.modifier	 = -1 * ch->level/12;
	    af.location	 = APPLY_DEX;
	    af.bitvector = AFF_BLEEDING;
	    affect_to_char(victim, &af);
	}

        if (wield->pIndexData->new_format)
	    dam = dice(wield->value[1],wield->value[2]) * skill/100*3/2;
        else
	    dam = number_range( wield->value[1] * skill/100, 
				wield->value[2] * skill/100)*3/2;

	dam += check_extra_damage(ch,dam,wield);
        dam_type = wield->value[3];
	
	if(!IS_AWAKE(victim))
            dam *= 2;
	else if (victim->position < POS_FIGHTING)
	    dam = dam * 5 / 4;

	dam += GET_DAMROLL(ch) * UMIN(100,skill) /100;

	if (dam <= 0 )
	    dam = 1;

	damage(ch,victim,dam,gsn_hamstring,dam_type,TRUE);
    }
    else
    {
        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
	    sprintf( buf, "Help!  %s tried to hamstring me!", PERS(ch, victim) );
	    do_autoyell( victim, buf );
        }	
        damage(ch,victim,0,gsn_hamstring,DAM_PIERCE,FALSE);
	act("You slice and miss!",ch,NULL,victim,TO_CHAR);
	act("$n misses $N with $s hamstring attempt.",ch,NULL,victim,TO_NOTVICT);
	act("You evade $n's slice.",ch,NULL,victim,TO_VICT);
	check_improve(ch,victim,gsn_hamstring,FALSE,1);
	WAIT_STATE(ch,skill_table[gsn_hamstring].beats); 
    }
    check_killer(ch,victim);
}


void do_gouge( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;
    int dam;

    one_argument(argument,arg);

    if (((chance = get_skill(ch,gsn_gouge)) == 0)
    &&    !is_affected(ch, gsn_hawkform))
    {	
	send_to_char("Huh?\n\r",ch);
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

    if (victim == ch)
    {
	send_to_char("You think about clawing your own eyes out, but think the wiser of it.\n\r",ch);
	return;
    }

    if (is_safe(ch,victim))
	return;

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
	act("But $N is your friend!",ch,NULL,victim,TO_CHAR);
	return;
    }

    if (IS_AFFECTED(victim, AFF_BLIND))
    {
	act("$N is already blinded.", ch, NULL, victim, TO_CHAR);
	return;
    }

    if (victim->race == global_int_race_shuddeni)
    {
	send_to_char("Your victim lacks eyes to gouge.\n\r", ch);
	return;
    }

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;

    /* stats */
    chance += get_curr_stat(ch,STAT_DEX);
    chance -= get_curr_stat(victim,STAT_DEX);

    /* speed */
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
        chance -= 30;

    /* level */
    chance += (ch->level - victim->level);

    chance /= 2;

    if (IS_SET(victim->imm_flags, IMM_BLIND))
	chance = 0;

    /* now the attack */
    if (number_percent() < chance )
    {
	if (!(is_affected(victim, gsn_gouge)))
	{
		af.where	= TO_AFFECTS;
		af.type		= gsn_gouge;
		af.level	= ch->level;
		af.duration	= ch->level/15;
		af.modifier	= 0;
		af.location	= 0;
		af.bitvector	= AFF_BLIND;
		affect_to_char(victim, &af);
	}

        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
          sprintf( buf, "Help!  %s is gouging my eyes!", PERS(ch, victim));
          do_autoyell( victim, buf );
        }	
        act("$n tears at your face, gouging at your eyes!",
		ch,NULL,victim,TO_VICT);
	act("You claw at $N's face, gouging at $S eyes!",ch,NULL,victim,TO_CHAR);
	act("$n claws $N's face, gouging at $S eyes!",
		ch,NULL,victim,TO_NOTVICT);
	check_improve(ch,victim,gsn_gouge,TRUE,1);

	WAIT_STATE(ch,skill_table[gsn_gouge].beats);

	dam = number_bits(3);

    if ( dam <= 0 )
	dam = 1;

	damage(ch,victim,dam,gsn_gouge,
	    DAM_PIERCE,FALSE);
    }
    else
    {
        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
          sprintf( buf, "Help!  %s tried to gouge out my eyes!", PERS(ch, victim) );
          do_autoyell( victim, buf );
        }	
        damage(ch,victim,0,gsn_gouge,DAM_PIERCE,FALSE);
	act("You claw at $S face, but miss!",
	    ch,NULL,victim,TO_CHAR);
	act("$n claws $N's face ineffectively.",
	    ch,NULL,victim,TO_NOTVICT);
	act("You swat $n's attempts to gouge out your eyes away.",
	    ch,NULL,victim,TO_VICT);
	check_improve(ch,victim,gsn_gouge,FALSE,1);
	WAIT_STATE(ch,skill_table[gsn_gouge].beats); 
    }
	check_killer(ch,victim);
}

void do_impale( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    OBJ_DATA *wield, *shield;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;
    int dam;
    int sn, skill;
    int dam_type;
    one_argument(argument,arg);

    /* get the weapon skill */
    sn = get_weapon_sn(ch);
    skill = 20 + get_weapon_skill(ch,sn,TRUE);

    if ( (chance = get_skill(ch,gsn_impale)) == 0)
    {	
	send_to_char("You don't know how to impale anything.\n\r",ch);
	return;
    }
     
    if (((wield = get_eq_char(ch, WEAR_WIELD)) == NULL)
     || ((wield->value[0] != WEAPON_SPEAR) 
       && !(wield->value[0] == WEAPON_EXOTIC && wield->value[3] == DAM_PIERCE)))
    {
        if (((wield = get_eq_char(ch, WEAR_DUAL_WIELD)) == NULL)
         || ((wield->value[0] != WEAPON_SPEAR)
	   && !(wield->value[0] == WEAPON_EXOTIC && wield->value[3] == DAM_PIERCE)))
	{
	    send_to_char("You must wield a spear to impale.\n\r", ch);
	    return;
	}
    }

    dam_type = wield->value[3];

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

    if (victim == ch)
    {
	send_to_char("You think of impaling yourself, but decide against it.\n\r",ch);
	return;
    }

    if (is_safe(ch,victim))
	return;

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
	act("But $N is your friend!",ch,NULL,victim,TO_CHAR);
	return;
    }

    if (!check_reach(ch, victim, REACH_EASY, PULSE_VIOLENCE))
	return;

    /* modifiers */


    /* stats */
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
    chance -= get_curr_stat(victim,STAT_DEX);
    chance -= GET_AC(victim,AC_PIERCE) /25;
    /* speed */
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
        chance -= 30;

    /* level */
    chance += (ch->level - victim->level);

    if (!IS_NPC(victim) 
	&& chance < get_skill(victim,gsn_dodge) )
    {	
	chance -= 3 * (get_skill(victim,gsn_dodge) - chance);
    }

    /* now the attack */
    if (number_percent() < chance )
    {
        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
          sprintf( buf, "Help!  %s tried to stab me!", PERS(ch, victim));
          do_autoyell( victim, buf );
        }	
        act("$n impales you with $p!",ch,wield,victim,TO_VICT);
	act("You impale $N with $p!",ch,wield,victim,TO_CHAR);
	act("$n impales $N with $p.",ch,wield,victim,TO_NOTVICT);
	check_improve(ch,victim,gsn_impale,TRUE,1);

	WAIT_STATE(ch,skill_table[gsn_impale].beats);

    if ((shield = get_eq_char(victim, WEAR_ABOUT))
     && (shield->pIndexData->vnum == OBJ_VNUM_MANTLE_EARTH)
     && (number_bits(2) != 0))
    {
        act("$N's mantle of earth stops your attempt to impale!", ch, NULL, victim, TO_CHAR);
        act("Your mantle of earth stops $n's attempt to impale!", ch, NULL, victim, TO_VICT);
        act("$N's mantle of earth stops $n's attemt to impale!", ch, NULL, victim, TO_NOTVICT);
        return;
    }


    if (!(is_affected(victim, gsn_impale)))
    {
	af.where	= TO_OAFFECTS;
	af.type		= gsn_impale;
	af.level	= ch->level;
	af.duration	= ch->level/8;
	af.modifier	= -1 * ch->level/12;
	af.location	= APPLY_STR;
	af.bitvector	= AFF_BLEEDING;
	affect_to_char(victim, &af);
    }

        if (wield->pIndexData->new_format)
		dam = dice(wield->value[1],wield->value[2]) * skill/100*3/2;
        else
	    	dam = number_range( wield->value[1] * skill/100, 
				wield->value[2] * skill/100)*3/2;

    dam += check_extra_damage(ch,dam,wield);

    if ( !IS_AWAKE(victim) )
	dam *= 2;
     else if (victim->position < POS_FIGHTING)
	dam = dam * 5 / 4;

    dam += GET_DAMROLL(ch) * UMIN(100,skill) /100;

    if (wield->value[0] == WEAPON_DAGGER)
	dam /= 2;

    if ( dam <= 0 )
	dam = 1;

	damage_from_obj(ch,victim,wield,dam,gsn_impale,
	    dam_type,TRUE);
    }
    else
    {
        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
          sprintf( buf, "Help!  %s tried to impale me!", PERS(ch, victim) );
          do_autoyell( victim, buf );
        }	
        damage(ch,victim,0,gsn_impale,wield->value[3],FALSE);
	act("You lunge and miss!",
	    ch,NULL,victim,TO_CHAR);
	act("$n misses $N with $s impaling attack.",
	    ch,NULL,victim,TO_NOTVICT);
	act("You evade $n's impaling lunge.",
	    ch,NULL,victim,TO_VICT);
	check_improve(ch,victim,gsn_impale,FALSE,1);
	WAIT_STATE(ch,skill_table[gsn_impale].beats); 
    }
	check_killer(ch,victim);
}



void do_throw( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj;
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;
    int weight;
    int dam;
    int skill;
    int sn = gsn_knife;
    int catch_skill = 0;
    int dir = -1;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    if ( arg1[0] == '\0')
    {
	send_to_char( "Throw what?\n\r", ch );
	return;
    }

    if ((chance = get_skill(ch,gsn_throw)) == 0)
    {	
	send_to_char("You fumble and almost hit yourself.\n\r",ch);
	return;
    }

    if ((obj = get_obj_carry(ch, arg1, ch)) == NULL)
    {
	send_to_char("You can't find that weapon to throw.\n\r", ch);
	return;
    }

    if (obj->item_type != ITEM_WEAPON)
    {
	send_to_char("Only weapons may be thrown.\n\r", ch);
	return;
    }

    if (((ch->class_num == global_int_class_fighter) 
      || ch->class_num == global_int_class_firetemplar) 
      && obj->value[0] != WEAPON_SPEAR)
    {
	send_to_char("You cannot throw that.\n\r", ch);
	return;
    }

    if ((ch->class_num == global_int_class_thief) && obj->value[0] != WEAPON_KNIFE && obj->value[0] != WEAPON_DAGGER)
    {
	send_to_char("You cannot throw that.\n\r", ch);
	return;
    }

    if (((weight = get_obj_weight(obj)) > 190) || ((obj->value[0] != WEAPON_SPEAR) && (weight > 50)))
    {
	send_to_char("That weapon is too heavy to throw.\n\r", ch);
	return;
    }

    if (arg2[0] == '\0')
    {
	victim = ch->fighting;
	if (!victim)
	{
	    send_to_char("But you're not fighting anyone!\n\r", ch);
	    return;
	}
    }
    else
    {
	if (arg3[0] == '\0')
	{
	    if ((victim = get_char_room(ch, arg2)) == NULL)
	    {
		send_to_char("You don't see them here.\n\r", ch);
		return;
	    }
	}
	else
	{
	    int i;

	    for (i = 0; i < 6; i++)
		if (!str_prefix(arg3, dir_name[i]))
		    dir = i;

	    if ((dir == -1) || !ch->in_room || !ch->in_room->exit[dir]
	     || (ch->in_room->exit[dir]->exit_info & EX_SECRET))
	    {
		send_to_char("Invalid direction.\n\r", ch);
		return;
	    }

	    if (ch->position == POS_FIGHTING)
	    {
		send_to_char("You can't do that while fighting.\n\r", ch);
		return;
	    }
	    
	    if (ch->in_room->exit[dir]->exit_info & EX_CLOSED)
	    {
		send_to_char("A closed door blocks your way.\n\r", ch);
		return;
	    }

	    if (ch->in_room->exit[dir]->exit_info & EX_WALLED)
	    {
		send_to_char("A wall of earth blocks your way.\n\r", ch);
		return;
	    }

	    if (ch->in_room->exit[dir]->exit_info & EX_ICEWALL)
	    {
    		send_to_char("A wall of ice blocks your way.\n\r", ch);
	    	return;
	    }

	    if (ch->in_room->exit[dir]->exit_info & EX_WALLOFFIRE)
	    {
		send_to_char("A wall of fire blocks your way.\n\r", ch);
		return;
	    }

	    if (ch->in_room->exit[dir]->exit_info & EX_WALLOFVINES)
	    {
            send_to_char("A wall of vines blocks your way.\n\r", ch);
            return;
	    }

	    if (room_is_affected(ch->in_room, gsn_smoke) || room_is_affected(ch->in_room->exit[dir]->u1.to_room, gsn_smoke))
	    {
            send_to_char("You cannot see properly through the rising smoke.\n\r", ch);
            return;
	    }

	    if (!can_see_in_room(ch, ch->in_room->exit[dir]->u1.to_room))
	    {
            send_to_char("You can't see your target there.\n\r",ch);
            return;
	    }
	    
        victim = get_char_room(ch, ch->in_room->exit[dir]->u1.to_room, arg2);
	    if (!victim)
        {
            sprintf(buf, "You don't see your target %s.\n\r",
                (dir == 0 ? "to the north" :
                 dir == 1 ? "to the east" :
                 dir == 2 ? "to the south" :
                 dir == 3 ? "to the west" :
                 dir == 4 ? "above you" : "below you" ));
            send_to_char(buf, ch);
            return;
        }
	}
    }

    /* get the weapon skill */
    switch (obj->value[0])
    {
	case (WEAPON_KNIFE):
	    sn = gsn_knife;
	    break;
	case (WEAPON_DAGGER):
	    sn = gsn_dagger;
	    chance /= 2;
	    break;
	case (WEAPON_SPEAR):
	    sn = gsn_spear;
	    chance *= 2/3;
	    break;
    }

    skill = 20 + get_weapon_skill(ch,sn,TRUE);

    if ((sn == gsn_spear) 
     && (ch->fighting != NULL))
    {
	send_to_char("You are in too close quarters to throw a spear.\n\r", ch);
	return;
    }

    if (victim == ch)
    {
	send_to_char("You try to hit yourself, but fail.\n\r",ch);
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
    chance += skill;   
    chance /= 2;

    /* stats */
    chance += get_curr_stat(ch,STAT_DEX);
    chance -= get_curr_stat(victim,STAT_DEX);
    chance -= GET_AC(victim,AC_PIERCE) /25;

    /* speed */
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
        chance -= 30;

    /* level */
    chance += (ch->level - victim->level);

    if (!IS_NPC(victim) 
     && (chance < get_skill(victim,gsn_dodge)))
	chance -= (get_skill(victim,gsn_dodge) / 4);

    /* now the attack */
    if (number_percent() < chance )
    {
        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
            sprintf( buf, "Help! %s is attacking me!", PERS(ch, victim));
            do_autoyell( victim, buf );
        }	

        obj_from_char( obj );
        obj_to_char( obj, victim );

        if (sn != gsn_spear)
            WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_throw].beats));
        else
            WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_throw].beats*2));

        if (victim->in_room == ch->in_room)
        {
            act("You throw $p at $N!", ch, obj, victim, TO_CHAR);
            act("$n throws $p at $N!", ch, obj, victim, TO_NOTVICT);
            act("$n throws $p at you!", ch, obj, victim, TO_VICT);
        }
        else
        {
            if (IS_NPC(victim)
             && ((IS_SET(victim->act, ACT_SENTINEL)) || (IS_SET(victim->act, ACT_NOTRACK))
              || (IS_SET(victim->act, ACT_STAY_AREA) && (victim->in_room->area != ch->in_room->area)))
                 && (victim->hit < victim->max_hit))
            {
            act("$N alertly dodges your thrown weapon.", ch, NULL, victim, TO_CHAR);
            act("$n alertly dodges a thrown weapon.", victim, NULL, NULL, TO_ROOM );
            return;
            }

            sprintf(buf, "You throw $p %swards, hitting $N!", dir_name[dir]);
            act(buf, ch, obj, victim, TO_CHAR);
            sprintf(buf, "$n throws $p %swards.", dir_name[dir]);
            act(buf, ch, obj, victim, TO_ROOM);
            sprintf(buf, "$p flies in from %s, hitting $N!",
            ((dir == 0) ? "the south" :
             (dir == 1) ? "the west" :
             (dir == 2) ? "the north" :
             (dir == 3) ? "the east" :
             (dir == 4) ? "below" : "above"));
            act(buf, ch, obj, victim, TO_VICTROOM);
            sprintf(buf, "$p flies in from %s, hitting you!",
            ((dir == 0) ? "the south" :
             (dir == 1) ? "the west" :
             (dir == 2) ? "the north" :
             (dir == 3) ? "the east" :
             (dir == 4) ? "below" : "above"));
            act(buf, ch, obj, victim, TO_VICT);
        }

        if (((catch_skill = get_skill(victim,gsn_catch_throw)) != 0)
        && (obj->value[0] != WEAPON_SPEAR)
        && (!victim->fighting || (victim->fighting == ch)))
        {
            if (number_percent() < (catch_skill/2))
            {
            check_improve(victim,ch,gsn_catch_throw,TRUE,1);
            act("$N catches your weapon, and throws it back at you!",
                ch, NULL, victim, TO_CHAR);
            act("$N catches the thrown weapon, and throws it back at $n!",
                ch, NULL, victim, TO_VICTROOM);
            act("You catch the thrown weapon, and throw it back at $N!",
                victim, NULL, ch, TO_CHAR);
            damage(ch,victim,0,gsn_throw,
                    DAM_PIERCE,FALSE);

            if (ch->in_room == victim->in_room)
            {
                char arg[MAX_INPUT_LENGTH];

                one_argument(ch->name, arg);
                sprintf(buf, "1. %s", arg);
                do_throw( victim, buf);
            }

            return;
            }
            check_improve(victim,ch,gsn_catch_throw,FALSE,1);
        }

        check_improve(ch,victim,gsn_throw,TRUE,1);

        if (number_percent() < (get_skill(victim,gsn_agility)/2+(SIZE_MEDIUM - victim->size)*10))
        {
            act("$N alertly dodges $p.",ch,obj,victim,TO_CHAR);
            act("You alertly dodge $p.",ch,obj,victim,TO_VICT);
            act("$N alertly dodges $p.",ch,obj,victim,TO_VICTROOM);
            return;
        }
        
        if (is_affected(victim, gsn_reflectiveaura) && (number_bits(1) == 0))
        {
            act("$p wraps around $N, and flies back towards you!", ch, obj, victim, TO_CHAR);
            act("Flows of air redirect $p, sending it back at $n!", ch, obj, victim, TO_VICT);
            act("$p wraps around $N, flying back towards $n!", ch, obj, victim, TO_VICTROOM);
            victim = ch;
        }

        if (obj->pIndexData->vnum == OBJ_VNUM_FIRE_SPEAR 
        && (victim->in_room->sector_type == SECT_UNDERWATER
        || room_is_affected(victim->in_room,gsn_wallofwater)))
        {
            act("Water engulfs $p, and it hisses out.",ch,obj,NULL,TO_CHAR);
            act("Water engulfs $p, and it hisses out.",ch,obj,NULL,TO_ROOM);
            act("Water engulfs $p, and it hisses out.",victim,obj,NULL,TO_CHAR);
            act("Water engulfs $p, and it hisses out.",victim,obj,NULL,TO_ROOM);
            extract_obj(obj);
            return;
        }
        if (obj->pIndexData->new_format)
            dam = dice(obj->value[1],obj->value[2]) * (skill/100) *3/2;
        else
            dam = number_range( obj->value[1] * (skill/100), obj->value[2] * (skill/100))*(3/2);

        if (sn == gsn_spear)
            dam += dam/3;
    
        dam += weight/3;
        dam += check_extra_damage(ch,dam,obj);

    	if ( !IS_AWAKE(victim) )
            dam *= 2;
     	else if (victim->position < POS_FIGHTING)
            dam = dam * 5 / 4;

        dam += GET_DAMROLL(ch) * UMIN(100,skill) /100;

        // Scale by level
        dam = (dam * UMIN((ch->level + 9), 60)) / 60;

        if ( dam <= 0 )
            dam = 1;

        if ((victim->in_room != ch->in_room) && IS_NPC(victim) && ((IS_SET(victim->act, ACT_SENTINEL) || IS_SET(victim->act, ACT_NOTRACK))) && (victim->hit < victim->max_hit))
            dam = 0;

    	if (ch->in_room != victim->in_room && check_defensiveroll(victim))
    	{
            act("You roll as $p hits you, lessening its affect.",victim,obj,NULL,TO_CHAR);
            dam /= 2;
    	}

        ROOM_INDEX_DATA *troom = victim->in_room;
        CHAR_DATA *vch, *vch_next;
        damage_from_obj(ch,victim,obj,dam,gsn_throw,obj->value[3],TRUE);

        // What's better than throwing spears of fire?	
        if (obj->pIndexData->vnum == OBJ_VNUM_FIRE_SPEAR)
        {
            act("$p explodes in a ball of fire!",troom->people,obj,NULL,TO_ROOM);
            act("$p explodes in a ball of fire!",ch,obj,NULL,TO_CHAR);
            for (vch = troom->people;vch;vch = vch_next)
            {
            vch_next = vch->next_in_room;
            if (is_safe(vch,ch))
                continue;
            dam = number_range(ch->level,ch->level*4);
            if (saves_spell(ch->level/2,ch, vch,DAM_FIRE))
                dam /= 2;
            damage_old(ch,vch,dam,gsn_fireball,DAM_FIRE,TRUE);
            }
            extract_obj(obj);
        }
    }
    else
    {

        if ((ch->fighting != victim) && (!IS_NPC(victim)))
        {
          sprintf( buf, "Help! %s is attacking me!", PERS(ch, victim));
          do_autoyell( victim, buf );
        }	

        damage(ch,victim,0,gsn_throw,DAM_PIERCE,FALSE);
	obj_from_char(obj);
	obj_to_room(obj,victim->in_room);

	if (victim->in_room == ch->in_room)
	{
	    act("You throw $p at $N, and miss!", ch, obj, victim, TO_CHAR);
	    act("$n throws $p at $N, and misses!", ch, obj, victim, TO_NOTVICT);
	    act("$n throws $p at you, and misses!", ch, obj, victim, TO_VICT);
	}
	else
	{
	    sprintf(buf, "You throw $p %swards, narrowly missing $N!", dir_name[dir]);
	    act(buf, ch, obj, victim, TO_CHAR);
	    sprintf(buf, "$n throws $p %swards.", dir_name[dir]);
	    act(buf, ch, obj, victim, TO_ROOM);
	    sprintf(buf, "$p flies in from %s, narrowly missing $N!",
		((dir == 0) ? "the south" :
		 (dir == 1) ? "the west" :
		 (dir == 2) ? "the north" :
		 (dir == 3) ? "the east" :
		 (dir == 4) ? "below" : "above"));
	    act(buf, ch, obj, victim, TO_VICTROOM);
	    sprintf(buf, "$p flies in from %s, narrowly missing you!",
		((dir == 0) ? "the south" :
		 (dir == 1) ? "the west" :
		 (dir == 2) ? "the north" :
		 (dir == 3) ? "the east" :
		 (dir == 4) ? "below" : "above"));
	    act(buf, ch, obj, victim, TO_VICT);
	}

	check_improve(ch,victim,gsn_throw,FALSE,1);
	if (sn != gsn_spear)
	    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_throw].beats)); 
	else
	    WAIT_STATE(ch, UMAX(ch->wait, skill_table[gsn_throw].beats*2));
    }
    check_killer(ch,victim);
}

void do_medstrike( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj;
    CHAR_DATA *victim;
    int chance;
    bool in_form = FALSE;
    if ((chance = get_skill(ch, gsn_medstrike)) == 0)
    {
	send_to_char("Huh?\n\r", ch );
	return;
    }

    if ( ( victim = ch->fighting ) == NULL )
    {
	send_to_char( "You aren't fighting anyone.\n\r", ch );
	return;
    }

    if ( ( obj = get_eq_char(ch, WEAR_WIELD) ) == NULL)
    {
	send_to_char( "You must wield a sword to perform a meditative strike.\n\r", ch);
	return;
    }

    if (obj->value[0] != WEAPON_SWORD)
    {
	send_to_char( "You must wield a sword to perform a meditative strike.\n\r", ch);
	return;
    }

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;

    AFFECT_DATA *paf;
    chance *= .75;

    for (paf = ch->affected; paf; paf=paf->next)
        if (paf->location == APPLY_FORM)
	{
	    in_form = TRUE;
	    break;
	}
    if (paf)
    {
        chance *= 1.2;
        WAIT_STATE( ch, skill_table[gsn_medstrike].beats);
    }
    else
        WAIT_STATE( ch, skill_table[gsn_medstrike].beats*2);
    
    int thac0, thac0_00, thac0_32;
    int victim_ac;
    int dam, diceroll;
    int sn, skill;
    int dam_type = obj->value[3];
    
    if (is_an_avatar(ch))
	    dam_type = TYPE_HIT+DAM_HOLY;
 
    if (obj && obj_is_affected(obj, gsn_heatsink))
	damage_old(ch, ch, number_range(4,8),gsn_heatsink,DAM_COLD,TRUE);

    wrathkyana_combat_effect(ch, victim);

    sn = get_weapon_sn(ch);
    skill = ((get_weapon_skill_weapon(ch,obj)/2) + get_skill(ch,gsn_medstrike)/2);
    if (IS_NPC(ch))
    {
	thac0_00 = 20;
	thac0_32 = -4;
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
    thac0 = interpolate(ch->level, thac0_00, thac0_32);

    if (thac0 < 0)
	thac0 = thac0 / 2;
    if (thac0 < -5)
	thac0 = -5 + (thac0 + 5)/2;
    
    thac0 -= GET_HITROLL(ch) * skill/100;
    thac0 += 5 * (100 - skill) / 100;

    switch(dam_type)
    {
	case(DAM_PIERCE): victim_ac = GET_AC(victim,AC_PIERCE)/10; break;
	case(DAM_BASH): victim_ac = GET_AC(victim,AC_BASH)/10; break;
	case(DAM_SLASH): victim_ac = GET_AC(victim,AC_SLASH)/10; break;
	default: victim_ac = GET_AC(victim,AC_EXOTIC)/10; break;
    };

    if (victim_ac < -15)
	victim_ac = (victim_ac + 15) / 5 - 15;
    if (!can_see(ch,victim) && !is_blindfighter(ch,TRUE))
	victim_ac -= 4;
    if (victim->position < POS_FIGHTING)
	victim_ac += 4;
    if (victim->position < POS_RESTING)
	victim_ac += 6;
    if (victim->position > POS_RESTING)
	if (number_percent() < get_skill(victim,gsn_dodge))
	    victim_ac -= 4;
        if (number_percent() < get_skill(victim,gsn_evade))
	    victim_ac -= 4;

    while ( ( diceroll = number_bits( 5 ) ) >= 20 )
	;
    if (( diceroll == 0 || (diceroll != 19 && diceroll < thac0 - victim_ac ))
      && !is_affected(ch, gsn_form_of_the_serpent))
    {
	/* Miss. */
	damage(ch,victim,0,gsn_medstrike,dam_type,TRUE);
	check_improve(ch,victim,gsn_medstrike,FALSE,1);
	return;
    }
    
    /* Hit */ 
    if (obj->pIndexData->new_format)
	dam = dice(obj->value[1], obj->value[2]);
    else
	dam = number_range(obj->value[1],obj->value[2]);

    dam *= 2;
    dam = (dam * get_skill(ch,gsn_medstrike))/100;
    dam += check_extra_damage(ch,dam,obj);
    if (!IS_AWAKE(victim) )
	dam *= 2;
    dam += (GET_DAMROLL(ch) / 3) * UMIN(100,skill) / 100;
    if (dam <= 0)
	dam = 1;
    
    check_improve(ch,victim,gsn_medstrike,TRUE,1);
    if (in_form)
    {
	act("You work a calculated strike into your form.",ch,NULL,NULL,TO_CHAR);
	act("$n works a calculated strike into $s form.",ch,NULL,NULL,TO_ROOM);
    }
    else
    {
	act("You step forward, striking fluidly with $p.",ch,obj,NULL,TO_CHAR);
	act("$n steps forward, striking fluidly with $p.",ch,obj,NULL,TO_ROOM);
    }
    damage_from_obj(ch,victim,obj,dam,gsn_medstrike,dam_type,TRUE);
    check_killer(ch,victim);
    return;
}

void check_enthusiasm(CHAR_DATA *ch, CHAR_DATA *victim)
{
    AFFECT_DATA *paf = NULL;
    int chance = get_skill(ch,gsn_enthusiasm);
    
    if (chance  > number_percent())
    { 
	paf = affect_find(ch->affected,gsn_enthusiasm);
	if (paf != NULL)
	{
            paf->modifier += 1;
	    check_improve(ch,victim,gsn_enthusiasm,TRUE,3);
	}
    }
    else
        if (is_affected(ch,gsn_enthusiasm))
	    check_improve(ch,victim,gsn_enthusiasm,FALSE,3);
}

int mod_by_enthusiasm(CHAR_DATA *ch)
{
    AFFECT_DATA *paf = NULL;
    int mod = 0;
    paf = affect_find(ch->affected,gsn_enthusiasm);
    if (paf != NULL)
	mod = paf->modifier / 2;
    return mod;
}

int mod_by_absorb(CHAR_DATA *ch)
{
    AFFECT_DATA *paf = NULL;
    int mod = 0;
    paf = affect_find(ch->affected,gsn_absorbelectricity);
    if (paf != NULL)
	if (paf->modifier > 25)
	{
	    mod = 25;
	    paf->modifier -= 25;
	}
	else
	{
	    if (paf->modifier > 0)
	        send_to_char("The last of your stored energy dissipates.\n\r",ch);
	    mod = paf->modifier;
	    paf->modifier = 0;
	}
    return mod;
}

void shielddisarm( CHAR_DATA *ch, CHAR_DATA *victim )
{
    OBJ_DATA *obj;

    if ((obj = get_eq_char( victim, WEAR_SHIELD ) ) == NULL )
	    return;

    if ( IS_OBJ_STAT(obj,ITEM_NOREMOVE))    
    {
	act("It won't budge!",ch,NULL,victim,TO_CHAR);
	act("$n tries to disarm your shield, but $p won't budge!",
	    ch,obj,victim,TO_VICT);
	act("$n tries to disarm $N's shield, but fails.",ch,NULL,victim,TO_NOTVICT);
	return;
    }

    if ( IS_NPC(victim) && IS_SET(victim->nact, ACT_NODISARM))
    {
	act("You fail to disarm $N's shield.", ch, NULL, victim, TO_CHAR);
	act("$n tries to disarm $N's shield, but fails.", ch, NULL, victim, TO_NOTVICT);
	return;
    }

    act( "$n {rstrikes{x your shield and sends $p flying!", 
	 ch, obj, victim, TO_VICT    );
    act( "You strike $N's shield, sending it flying!",  ch, NULL, victim, TO_CHAR    );
    act( "$n strikes $N's shield, sending it flying!",  ch, NULL, victim, TO_NOTVICT );

    oprog_remove_trigger(obj);

    obj_from_char( obj );
    if ( IS_OBJ_STAT(obj,ITEM_NODROP) || IS_OBJ_STAT(obj,ITEM_INVENTORY) )
	obj_to_char( obj, victim );
    else
    {
	obj_to_room( obj, victim->in_room );
	// added an obj check here for melt_drop items/shields of ice/etc
	if (obj && IS_NPC(victim) && victim->wait == 0 && can_see_obj(victim,obj))
	    get_obj(victim,obj,NULL);
    }

    return;
}

void do_shielddisarm( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    int chance;

    if ((chance = get_skill(ch, gsn_shielddisarm)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if ( (obj = get_eq_char(ch, WEAR_WIELD)) == NULL
      || obj->value[0] != WEAPON_FLAIL)
	if ( (obj = get_eq_char(ch, WEAR_DUAL_WIELD)) == NULL
	  || obj->value[0] != WEAPON_FLAIL)
	{
            send_to_char( "You must wield a flail to disarm a shield.\n\r", ch );
            return;
	}

    if ((victim = ch->fighting) == NULL)
    {
        send_to_char( "You aren't fighting anyone.\n\r", ch );
        return;
    }

    if (((obj = get_eq_char(victim, WEAR_SHIELD)    ) == NULL)
     || !can_see_obj(ch, obj))
    {
        send_to_char( "Your opponent has no shield you can disarm.\n\r", ch );
        return;
    }

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;

    chance = get_disarm_chance(ch, victim, chance, TRUE);

    /* and now the attack */
    if (number_percent() < chance)
    {
	WAIT_STATE( ch, skill_table[gsn_shielddisarm].beats );
        check_improve(ch,victim,gsn_shielddisarm,TRUE,1);
	shielddisarm( ch, victim );
    }
    else
    {
        WAIT_STATE(ch,skill_table[gsn_shielddisarm].beats);
        act("You swing at $N's shield, but miss.",ch,NULL,victim,TO_CHAR);
        act("$n tries to disarm your shield, but misses.",ch,NULL,victim,TO_VICT);
        act("$n tries to disarm $N's shield, but misses.",ch,NULL,victim,TO_NOTVICT);
        check_improve(ch,victim,gsn_shielddisarm,FALSE,1);
    }
    check_killer(ch,victim);
    return;
}

int mod_by_haste(CHAR_DATA *ch, CHAR_DATA *victim)
{
    int chance = 0;
    if (IS_AFFECTED(victim,AFF_HASTE) || IS_SET(victim->off_flags,OFF_FAST))
	chance -= 10;
    if (IS_AFFECTED(ch,AFF_HASTE) || IS_SET(ch->off_flags,OFF_FAST))
	chance += 10;
    return chance;	
}

void do_shadowstrike( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj;
    CHAR_DATA *victim;
    int chance;
    bool in_form = FALSE;
    
    if ((chance = get_skill(ch, gsn_shadowstrike)) == 0)
    {
	send_to_char("Huh?\n\r", ch );
	return;
    }

    if ( ( victim = ch->fighting ) == NULL )
    {
	send_to_char( "You aren't fighting anyone.\n\r", ch );
	return;
    }

    if ( ( obj = get_eq_char(ch, WEAR_WIELD) ) == NULL)
    {
	send_to_char( "You must wield a weapon to perform a shadow strike.\n\r", ch);
	return;
    }

    if (obj->value[0] != WEAPON_SWORD)
    {
	send_to_char( "You must wield a sword to perform a shadow strike.\n\r", ch);
	return;
    } 

    if (ch->mana - skill_table[gsn_shadowstrike].min_mana < 0)
    {
	send_to_char("You are too tired to perform a shadow strike.\n\r",ch);
	return;
    }

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;

    AFFECT_DATA *paf;
    chance *= .75;

    for (paf = ch->affected; paf; paf=paf->next)
        if (paf->location == APPLY_FORM)
	{
	    in_form = TRUE;
	    break;
	}
    if (paf)
    {
        chance *= 1.2;
	WAIT_STATE( ch, skill_table[gsn_shadowstrike].beats);
    }
    else
	WAIT_STATE( ch, skill_table[gsn_shadowstrike].beats * 2);
    
    expend_mana(ch, skill_table[gsn_shadowstrike].min_mana);
    
    int thac0, thac0_00, thac0_32;
    int victim_ac;
    int dam, diceroll;
    int skill;
    int dam_type = DAM_NEGATIVE;
    
    if (is_an_avatar(ch))
	    dam_type = TYPE_HIT+DAM_HOLY;
 
    if (obj && obj_is_affected(obj, gsn_heatsink))
	damage_old(ch, ch, number_range(4,8),gsn_heatsink,DAM_COLD,TRUE);

    wrathkyana_combat_effect(ch, victim);

    skill = get_skill(ch,gsn_shadowstrike);

    if (IS_NPC(ch))
    {
	thac0_00 = 20;
	thac0_32 = -4;
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
    thac0 = interpolate(ch->level, thac0_00, thac0_32);

    if (thac0 < 0)
	thac0 = thac0 / 2;
    if (thac0 < -5)
	thac0 = -5 + (thac0 + 5)/2;
    
    thac0 -= GET_HITROLL(ch) * skill/100;
    thac0 += 5 * (100 - skill) / 100;

    switch(dam_type)
    {
	case(DAM_PIERCE): victim_ac = GET_AC(victim,AC_PIERCE)/10; break;
	case(DAM_BASH): victim_ac = GET_AC(victim,AC_BASH)/10; break;
	case(DAM_SLASH): victim_ac = GET_AC(victim,AC_SLASH)/10; break;
	default: victim_ac = GET_AC(victim,AC_EXOTIC)/10; break;
    };

    if (victim_ac < -15)
	victim_ac = (victim_ac + 15) / 5 - 15;
    if (!can_see(ch,victim) && !is_blindfighter(ch,TRUE))
	victim_ac -= 4;
    if (victim->position < POS_FIGHTING)
	victim_ac += 4;
    if (victim->position < POS_RESTING)
	victim_ac += 6;
    if (victim->position > POS_RESTING)
	if (number_percent() < get_skill(victim,gsn_dodge))
	    victim_ac -= 4;
        if (number_percent() < get_skill(victim,gsn_evade))
	    victim_ac -= 4;

    while ( ( diceroll = number_bits( 5 ) ) >= 20 )
	;
    if (( diceroll == 0 || (diceroll != 19 && diceroll < thac0 - victim_ac )))
    {
	/* Miss. */
	damage(ch,victim,0,gsn_shadowstrike,dam_type,TRUE);
	check_improve(ch,victim,gsn_shadowstrike,FALSE,1);
	return;
    }
    
    /* Hit */ 
    if (obj->pIndexData->new_format)
	dam = dice(obj->value[1], obj->value[2]);
    else
	dam = number_range(obj->value[1],obj->value[2]);
    dam *= 2;
    dam = (dam * get_skill(ch,gsn_shadowstrike))/100;
    dam += check_extra_damage(ch,dam,obj);
    if (!IS_AWAKE(victim) )
	dam *= 2;
    dam += (GET_DAMROLL(ch) / 3) * UMIN(100,skill) / 100;
    if (dam <= 0)
	dam = 1;
    
    check_improve(ch,victim,gsn_shadowstrike,TRUE,1);
    
    if (victim->race != global_int_race_shuddeni
      && !IS_SET(victim->imm_flags, IMM_BLIND)
      && number_percent() < skill/3 && !is_affected(victim,gsn_shadowstrike))
    {
	if (in_form)
	{
	    act("You work a calculated strike into your form, shrouding $N in blackness.",ch,NULL,victim,TO_CHAR);
	    act("$n works a calculated strike into $s form, shrouding $N in blackness.",ch,NULL,victim,TO_NOTVICT);
	    act("$n works a calculated strike into $s form, shrouding you in blackness.",ch,NULL,victim,TO_VICT);
	}
	else
	{
	    act("You strike with diabolic precision, shrouding $N in blackness.",ch,NULL,victim,TO_CHAR);
	    act("$n strikes with diabolic precision, shrouding $N in blackness.",ch,NULL,victim,TO_NOTVICT);
	    act("$n strikes with diabolic precision, shrouding you in blackness.",ch,NULL,victim,TO_VICT);
	}
	AFFECT_DATA af;
	af.valid = TRUE;
	af.point = NULL;
	af.where = TO_AFFECTS;
	af.type = gsn_shadowstrike;
	af.level = ch->level;
	af.location = 0;
	af.modifier = 0;
	af.duration = 1;
	af.bitvector = AFF_BLIND;
	affect_to_char(victim,&af);
    }
    else
	if (in_form)
	{
	    act("You work a calculated strike into your form, and shadows flicker around $N.",ch,NULL,victim,TO_CHAR);
	    act("$n works a calculated strike into $s form, and shadows flicker around $N.",ch,NULL,victim,TO_ROOM);
	}
	else
	{
	    act("You strike with diabolic precision, and shadows flicker around $N.",ch,NULL,victim,TO_CHAR);
	    act("$n strikes with diabolic precision, and shadows flicker around $N.",ch,NULL,victim,TO_ROOM);
	}
    damage_from_obj(ch,victim,obj,dam,gsn_shadowstrike,dam_type,TRUE);
    check_killer(ch,victim);
    return;
}


void check_snakebite args( ( CHAR_DATA *ch,CHAR_DATA *victim) )
{
    int chance = get_skill(ch,gsn_snakebite),dam=0;
    AFFECT_DATA poison, *paf;
    poison.valid = TRUE;
    poison.point = NULL;

    if (ch->in_room != victim->in_room || IS_OAFFECTED(victim, AFF_GHOST))
	return;
    if ((paf = affect_find(ch->affected,gsn_snakebite)) == NULL)
	return;

    if (chance < 1)
	return;

    chance = round(chance * 3.0 / 4.0);
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
        chance -= 10;

    chance += ch->level - victim->level;
    if (number_percent() > chance)
	return;
    dam = number_range(paf->level,paf->level*2);
    if (saves_spell(paf->level,ch,victim,DAM_POISON))
        dam /= 2;
    
    damage(ch,victim,dam, gsn_snakebite, DAM_POISON, TRUE);
    check_improve(ch,victim,gsn_snakebite,3,TRUE);
}

void check_shieldslam args( ( CHAR_DATA *ch,CHAR_DATA *victim) )
{
    int chance = get_skill(ch,gsn_shieldslam),dam;
    OBJ_DATA *obj = NULL, *vwield = NULL, *vdual = NULL, *vshield = NULL;
    CHAR_DATA *temp;

    if (ch->in_room != victim->in_room || IS_OAFFECTED(victim,AFF_GHOST))
	return;
    if ((obj = get_eq_char(ch,WEAR_SHIELD)) == NULL)
	return;
    if (skill_table[gsn_shieldslam].skill_level[ch->class_num] > ch->level)
	return;
    if (get_skill(ch,gsn_shieldslam) == 0)
	return;

    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
        chance -= 10;

    chance += ch->level - victim->level;
    dam = number_range(ch->level*2/3,ch->level+GET_DAMROLL(ch)/3);
    vwield = get_eq_char(victim,WEAR_WIELD);
    vdual = get_eq_char(victim,WEAR_DUAL_WIELD);
    vshield = get_eq_char(victim,WEAR_SHIELD);

    if (vwield)
    {
        if ( number_percent() < get_skill(victim,gsn_parry)/3)
	    dam = dam * 4/5;
    	if ((vwield->value[0] == WEAPON_SPEAR 
	    || vwield->value[0] == WEAPON_POLEARM)
          && number_percent() < get_skill(victim,gsn_fend)/3)
	    dam = dam * 9/10;
    	if (IS_SET(vwield->value[4],WEAPON_TWO_HANDS)
	  && number_percent() < get_skill(victim,gsn_stonephalanx)/3)
	    dam = dam * 9/10;
        if (vwield->value[0] == WEAPON_STAFF
	  && number_percent() < get_skill(victim,gsn_deflect)/3)
	    dam = dam * 9/10;
    }

    if (vdual)
	if (number_percent() < get_skill(victim,gsn_offhandparry)/3)
	    dam = dam * 9/10;
    
    if (vshield)
        if (number_percent() < get_skill(victim,gsn_shield_block)/2)
	    dam = dam * 4/5;

    if (number_percent() < get_skill(victim,gsn_dodge)/3)
	dam = dam * 9/10;
    if (number_percent() < get_skill(victim,gsn_evade)/3)
	dam = dam * 9/10;
    if (number_percent() < get_skill(victim,gsn_brawlingblock)/3)
	dam = dam * 9/10;

    if (number_percent() < chance)
    {
	check_improve(ch,victim,gsn_shieldslam,TRUE,1);
	if (victim->level >= skill_table[gsn_counter].skill_level[victim->class_num]
	  &&number_percent() < get_skill(victim,gsn_counter)/2)
	{
	    act("$N counters your shield slam!",ch,NULL,victim,TO_CHAR);
	    act("You counter $n's shield slam!",ch,NULL,victim,TO_VICT);
	    act("$N counters $n's shield slam!",ch,NULL,victim,TO_NOTVICT);
	    temp = victim;
	    victim = ch;
	    ch = temp;
	    check_improve(ch,victim,gsn_counter,TRUE,1);
	}
	damage(ch,victim,dam, gsn_shieldslam, DAM_BASH, TRUE);
    }
    else
	damage(ch,victim,0,gsn_shieldslam, DAM_BASH,TRUE);
}

void do_distract( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    int chance;
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;

    if (IS_NPC(ch) && IS_SET(ch->nact, ACT_NORESCUE))
	return;
    
    if (IS_OAFFECTED(ch, AFF_GHOST))
    {
	send_to_char("You cannot distract others in your ghostly form.\n\r", ch);
	return;
    }

    if ( argument[0] == '\0' )
    {
	if ((victim = ch->fighting) == NULL)
	{
	    send_to_char( "Distract whom?\n\r", ch );
	    return;
	}
    }
    else if ((victim = get_char_room(ch, argument)) == NULL)
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }

    if ( victim == ch )
    {
	send_to_char( "You look the other way.\n\r", ch );
	return;
    }

    if ( victim->fighting == ch )
    {
	act("$N is already paying close attention to you!",ch,NULL,victim,TO_CHAR);
	return;
    }

    if ( victim->fighting == NULL )
    {
	send_to_char( "That person is not fighting right now.\n\r", ch );
	return;
    }

    if (is_safe(victim, ch))
	return;

    if (!IS_NPC(ch))
      if (!IS_NPC(victim) && !IS_PK(ch, victim))
      {
	act("The gods protect $N from you.", ch, NULL, victim, TO_CHAR);
	return;
      }

      if (!IS_NPC(victim) && IS_AFFECTED(ch, AFF_CHARM) &&
	ch->master && !IS_PK(ch->master, victim))
      {
	act("The gods protect $N from you.", ch, NULL, victim, TO_CHAR);
	return;
      }

    WAIT_STATE( ch, skill_table[gsn_distract].beats );

    chance = get_skill(ch, gsn_distract)*3/4;

    if (!IS_NPC(ch) && BIT_GET(ch->pcdata->traits, TRAIT_BRAVE))
        chance += 15;

    if ( number_percent( ) > chance)
    {
	act( "You fail to distract $N.", ch, NULL, victim, TO_CHAR );
	check_improve(ch,victim,gsn_distract,FALSE,1);
	return;
    }

    act( "You distract $N into attacking you!",  ch, NULL, victim, TO_CHAR    );
    act( "$n distracts you into attacking $m!", ch, NULL, victim, TO_VICT    );
    act( "$n distracts $N into attacking $m!",  ch, NULL, victim, TO_NOTVICT );
    check_improve(ch,victim,gsn_distract,TRUE,1);
    
    af.type = gsn_distract;
    af.modifier = 0;
    af.bitvector = 0;
    af.duration = 0;
    af.point = victim->fighting;
    af.where = TO_CHAR;
    af.location = APPLY_NONE;
    affect_to_char(victim, &af);

    stop_fighting( victim->fighting );
    stop_fighting( victim );

    check_killer( ch, victim );
    set_fighting( ch, victim );
    set_fighting( victim, ch );
    return;
}

bool check_defensiveroll(CHAR_DATA *victim)
{
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;

    if (is_affected(victim,gsn_defensiveroll))
	return FALSE;

    if (skill_table[gsn_defensiveroll].skill_level[victim->class_num] > victim->level)
	return FALSE;

    if (number_percent() > get_skill(victim,gsn_defensiveroll))
    {
	check_improve(victim,NULL,gsn_defensiveroll,FALSE,1);
	return FALSE;
    }
		    
    af.type = gsn_defensiveroll;
    af.where = TO_AFFECTS;
    af.duration = 5;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = 0;
    af.level = victim->level;
    affect_to_char(victim,&af);
    check_improve(victim,NULL,gsn_defensiveroll,TRUE,1);
    return TRUE;
}	

bool check_lastword(CHAR_DATA *victim, CHAR_DATA *ch)
{
    if (victim == ch)
	return FALSE;
    if (ch == NULL 
      || ((IS_NPC(ch) && ch->in_room == NULL)
	|| (!IS_NPC(ch) && IS_OAFFECTED(ch,AFF_GHOST))))
	return FALSE;
    if (victim == NULL 
      || ((IS_NPC(victim) && victim->in_room == NULL)
	|| (!IS_NPC(victim) && IS_OAFFECTED(victim,AFF_GHOST))))
	return FALSE;
    if (ch->in_room != victim->in_room)
	return FALSE;
    if (victim->lastword == ch)
	return FALSE;
    if (victim->position > POS_SLEEPING)
    {
	if (number_percent() < get_skill(victim,gsn_lastword))
	{
	    act("As you're struck, you lash out at $n a final time.",ch,NULL,victim,TO_VICT);
	    act("As $N is struck, $E lashes out at $n a final time.",ch,NULL,victim,TO_NOTVICT);
	    act("As you strike $N, $E lashes out at you a final time.",ch,NULL,victim,TO_CHAR);
	    victim->lastword = ch;
	    one_hit(victim, ch, TYPE_UNDEFINED, HIT_PRIMARY);
	    if (victim)
	    {
	        check_improve(victim,ch,gsn_lastword,TRUE,1);
		victim->lastword = NULL;
		if (victim->in_room == NULL || IS_OAFFECTED(victim,AFF_GHOST))
		    return TRUE;
	    }
	}
	else
	    check_improve(victim,ch,gsn_lastword,FALSE,1);
    }
    return FALSE;
}
