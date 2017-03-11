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
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <cmath>
#include <sstream>
#include <vector>
#include <cassert>
#include "songs.h"
#include "interp.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "skills_chirurgeon.h"
#include "demons.h"
#include "psionics.h"
#include "spells_fire.h"
#include "spells_air.h"
#include "spells_fire_water.h"
#include "spells_spirit.h"
#include "spells_spirit_air.h"
#include "spells_water.h"
#include "spells_water_air.h"
#include "spells_air_void.h"
#include "spells_earth.h"
#include "spells_void.h"
#include "EchoAffect.h"
#include "PhantasmInfo.h"
#include "Luck.h"
#include "Faction.h"
#include "NameMaps.h"
#include "RuneInfo.h"
#include "Runes.h"
#include "Drakes.h"
#include "Encumbrance.h"
#include "Oaths.h"

/* command procedures needed */
DECLARE_DO_FUN(do_return	);

extern	bool	hands_free	args( ( CHAR_DATA *ch ) );
extern	void	append_note	args( ( NOTE_DATA *pnote ) );

char meep[MAX_INPUT_LENGTH];
/*
 * Local functions.
 */
int	get_moon_state  args( ( int moon_num, bool get_size ) );
int	get_mval	args( ( CHAR_DATA *ch, char *argument ) );
void	affect_modify	args( ( CHAR_DATA *ch, AFFECT_DATA *paf, bool fAdd, bool fromObj) );
bool	mob_remembers	args( ( CHAR_DATA *mob, char *argument ) );
CHAR_DATA *get_conduit  args( ( CHAR_DATA *ch, ROOM_INDEX_DATA *room, int distance) );
CHAR_DATA *get_affinity_owner  args( ( OBJ_DATA *obj) );
void	add_hostdata	args( ( DESCRIPTOR_DATA *d ) );

void dump_vlinks args ( ( CHAR_DATA *ch ) );

extern int get_moon_state args( ( int moon_num, bool get_size ) );
extern void unbind_shunned_demon (CHAR_DATA *victim);

extern bool global_bool_ranged_attack;

/* MW -- code to see if a person can cast a certain spell, based on their
	possession of a number of stones. Looks up the skills position
	in the clanskill_table (tables.c), and checks if they have 1, 2, 4, 7,
	or 10 in their vault -- which replaced the altar vnum in the house
	table code. You still have to call this function in your
	skill code, it does NOT happen at the interp level */


const std::vector<const char *> & BuildBannedOrders()
{
	static std::vector<const char *> orders;
	orders.push_back("delete");
	orders.push_back("quit");
	orders.push_back("desc");
	orders.push_back("suicide");
	orders.push_back("pray");
	orders.push_back("bg");
	orders.push_back("background");
	orders.push_back("bash");
	orders.push_back("trip");
	orders.push_back("beg");
	orders.push_back("unlearn");
	return orders;
}

bool IsOrderBanned(const char * order)
{
	assert(order != NULL);

	static const std::vector<const char *> & orders(BuildBannedOrders());

	// Check whether the orders in the banned list (as prefixes)
	for (size_t i(0); i < orders.size(); ++i)
	{
		if (!str_prefix(order,orders[i]))
			return true;
	}

	// Check for prog commands (any 'o', 'm', or 'r' followed by a 'p')
	switch (LOWER(order[0]))
	{
		case 'o':
		case 'm':
		case 'r':
			if (order[1] == 'p')
				return true;
	}

	// Make sure they aren't being forced to say Ashur
	if (strstr(order, "Ashur") != 0)
		return true;
        
	// Autotoggles are more system commands than in-character commands,
	// so we're going to ban those as well.
	if (!str_prefix("auto",order))
	    return true;

	// Allow the command
	return false;
}

bool is_pk(CHAR_DATA * ch, CHAR_DATA * victim)
{
    if (IS_NPC(ch) || IS_NPC(victim)) return false;                                     // NPCs cannot be in PK range
    if (ch->level <= 10 || victim->level <= 10) return false;                           // Minimum level disqualifier
    if (is_affected(ch, gsn_ghost) || is_affected(victim, gsn_ghost)) return false;     // Ghost disqualifier
    if (IS_SET(victim->act, PLR_NOPK) && !IS_IMMORTAL(ch)) return false;                // NO_PK bit disqualifier

    // Experience and level difference disqualifiers do not apply to the oathbound
    if (Oaths::OathHolderFor(*ch) != victim && Oaths::OathHolderFor(*victim) != ch)
    {
        // No oath between them, so check experience differential
        int chLevelExp(exp_on_level(ch, ch->level));
        int victimLevelExp(exp_on_level(ch, ch->level));
        if (chLevelExp >= victimLevelExp)
        {
            if (((chLevelExp * 55) / 100) > victimLevelExp)
                return false;
        }
        else if (((victimLevelExp * 55) / 100) > chLevelExp)
            return false;
        
        // Check level differential disqualifier
        if (abs(ch->level - victim->level) >= 13) 
            return false;
    }

    return true;
}

bool is_spellcaster(CHAR_DATA * ch)
{
    switch (ch->class_num)
    {
        case CLASS_WATER_SCHOLAR:
        case CLASS_EARTH_SCHOLAR:
        case CLASS_VOID_SCHOLAR:
        case CLASS_SPIRIT_SCHOLAR:
        case CLASS_AIR_SCHOLAR:
        case CLASS_FIRE_SCHOLAR:
        case CLASS_WATER_TEMPLAR:
        case CLASS_EARTH_TEMPLAR:
        case CLASS_VOID_TEMPLAR:
        case CLASS_SPIRIT_TEMPLAR:
        case CLASS_AIR_TEMPLAR:
        case CLASS_FIRE_TEMPLAR:
        case CLASS_DRUID:
            return true;
    }

    return false;
}

int get_carry_weight(const CHAR_DATA & ch)
{
    int result(ch.carry_weight);
    if (ch.in_room != NULL && area_is_affected(ch.in_room->area, gsn_gravitywell))
        result = (result * 11) / 10;

    return result;
}

bool has_ground(const ROOM_INDEX_DATA & room)
{
    switch (room.sector_type)
    {
        case SECT_AIR:
        case SECT_WATER_SWIM:
        case SECT_WATER_NOSWIM:
        case SECT_UNDERWATER:
            return false;
    }

    return true;
}

void updateWaitState(CHAR_DATA * ch, int beats)
{
    // Adjust beats for skill modifiers
    if (IS_PAFFECTED(ch, AFF_TRUESHOT)) beats += 3;
    if (IS_NAFFECTED(ch, AFF_BOLO)) beats += 3;
    
    int vitalStrikeMod(get_modifier(ch->affected, gsn_vitalstrike) * 2);
    beats += UMAX(0, vitalStrikeMod);

    int tremorMod(get_modifier(ch->affected, gsn_tremor) * 2);
    beats += UMAX(0, tremorMod);

    // Apply modifiers
    ch->daze = UMAX(ch->daze, ch->wait);
    ch->daze = UMAX(ch->daze, beats);
    ch->wait = ((IS_NPC(ch) && find_bilocated_body(ch) == NULL) ? 0 : ch->daze);
}

int get_mval(CHAR_DATA *ch, char *argument)
{
CHAR_DATA *victim;
MEMORY_DATA *md, *md_next;

        if ((victim = get_char_room(ch, argument)) == NULL)
                if ((victim = get_char_world(ch, argument)) == NULL)
                        return FALSE;

for (md = ch->memgroup; md != NULL; md = md_next)
        {
        md_next = md->next;
        if (md->ch == victim)
		return md->value;;
        }

return -1;
}

void modify_karma(CHAR_DATA * ch, int mod)
{
    ch->pcdata->karma = URANGE(SILVERAURA, ch->pcdata->karma + mod, BLACKAURA);
    ch->pcdata->request_points = URANGE(SILVERAURA, ch->pcdata->request_points + mod, BLACKAURA);
}

void expend_mana(CHAR_DATA * ch, int mana)
{
    // Check for aethereal communion
    unsigned int modifier;
    std::vector<CHAR_DATA*> members(MembersOfAetherealCommunion(ch, modifier));
    if (!members.empty())
    {
        if (number_percent() > static_cast<int>(50 + (modifier / 2)))
        {
            // Failed the dice roll
            ch->mana -= mana;
            return;
        }

        // Split the mana over each member of the communion 
        while (mana >= static_cast<int>(members.size()) && !members.empty())
        {
            unsigned int newCost(mana / members.size());
            std::vector<CHAR_DATA*> newMembers;
            for (unsigned int i(0); i < members.size(); ++i)
            {
                // Members with 0 or less mana do not participate
                if (members[i]->mana <= 0)
                    continue;

                // Members with less than the desired cost are drained and not counted in the next round
                if (static_cast<unsigned int>(members[i]->mana) <= newCost)
                {
                    mana -= members[i]->mana;
                    members[i]->mana = 0;
                    continue;
                }

                // Members with sufficient mana for the desired cost eat it and are counted in the next round
                mana -= newCost;
                members[i]->mana -= newCost;
                newMembers.push_back(members[i]);
            }

            // Switch to the new members; std::swap has overloads for std::vector so this should just be a few pointer swaps, not a lot of copying
            std::swap(members, newMembers);
        }
    }

    // The original character eats any remaining mana cost
    ch->mana -= mana;

    // Check for effects which require positive mana to upkeep
    if (ch->mana <= 0)
    {
        // Check for songs
        if (ch->song != NULL)
        {
            ch->mana = 0;
            std::ostringstream mess;
            mess << "Out of breath, you stop performing '" << skill_table[ch->song->type].name << "'.\n";
            stop_playing_song(ch, NULL);
        }

        // Check for quintessence rush
        AFFECT_DATA * rush(get_quintessence_rushing_affect(ch));
        if (rush != NULL)
        {
            send_to_char("With your mind run ragged, the raw quintessence enfolding your spirit rushes to your head!\n", ch);
            
            AFFECT_DATA af = {0};
            af.where    = TO_AFFECTS;
            af.type     = (number_bits(1) == 0 ? gsn_vertigo : gsn_confusion);
            af.level    = rush->level;
            af.duration = number_range(1, 3);
            affect_to_char(ch, &af);
            
            affect_remove(ch, rush);
        }

        // Check for radiate aura
        AFFECT_DATA * spiritAura(get_affect(ch, gsn_radiateaura));
        if (spiritAura != NULL)
        {
            send_to_char("You are too tired to maintain the aura about you, and it slowly fades into nothingness.\n", ch);
            affect_remove(ch, spiritAura);
        }

        // Check for fadeshroud
        AFFECT_DATA * fadeshroud(get_affect(ch, gsn_fadeshroud));
        if (fadeshroud != NULL)
        {
            send_to_char("Unable to focus on the darkness any longer, you release your fadeshroud.\n", ch);
            affect_remove(ch, fadeshroud);
        }

        // Check for corpse sense
        AFFECT_DATA * corpsesense(get_affect(ch, gsn_corpsesense));
        if (corpsesense != NULL)
        {
            send_to_char("Exhausted, you close your mind to the call of the dead.\n", ch);
            affect_remove(ch, corpsesense);
        }

        // Check for phantasms
        if (ch->in_room != NULL)
        {
            CHAR_DATA * phantasm_next;
            for (CHAR_DATA * phantasm(ch->in_room->people); phantasm != NULL; phantasm = phantasm_next)
            {
                phantasm_next = phantasm->next_in_room;
                if (IS_NPC(phantasm) && IS_SET(phantasm->act, ACT_ILLUSION) && phantasm->master == ch)
                {
                    act("You are no longer able to maintain the illusion, and $N fades away.", ch, NULL, phantasm, TO_CHAR);
                    act("$N fades away.", ch, NULL, phantasm, TO_ROOM);
                    stop_follower(phantasm);
                }
            }
        }
    }
}

bool mob_remembers( CHAR_DATA *mob, char *argument)
{
CHAR_DATA *victim;
MEMORY_DATA *md, *md_next;

        if ((victim = get_char_room(mob, argument)) == NULL)
                if ((victim = get_char_world(mob, argument)) == NULL)
                        return FALSE;

for (md = mob->memgroup; md != NULL; md = md_next)
        {
        md_next = md->next;
        if (md->ch == victim)
		return TRUE;
        }

return FALSE;
}

bool stone_check(CHAR_DATA *ch, int sn)
{
    int stonecount, clan, needed;
    ROOM_INDEX_DATA *vault;
    OBJ_DATA *obj;

	if (IS_NPC(ch))
		return TRUE;

	if (ch->trust > 51)
		return TRUE;

	if (ch->clan == 0)
		return FALSE;

	/* I've been victimized by having to do a stupid hack.  Help me. */
	if (sn == gsn_brotherhood)
	    return FALSE;

	if (sn == gsn_coven) // make demonic focus
	    needed = 2;
	else if (sn == gsn_dark_insight)
	    needed = 4;
	else if (sn == gsn_compact_of_Logor)
	    needed = 7;
	else if (sn == gsn_coverofdarkness) // shroud of nyogthua
	    needed = 10;
	else if (sn == skill_lookup(clanskill_table[ch->clan].skill1))
		needed = 0;
	else if (sn == skill_lookup(clanskill_table[ch->clan].skill2))
		needed = 2;
	else if (sn == skill_lookup(clanskill_table[ch->clan].skill3))
		needed = 4;
	else if (sn == skill_lookup(clanskill_table[ch->clan].skill4))
		needed = 7;
	else if (sn == skill_lookup(clanskill_table[ch->clan].skill5))
		needed = 10;
	else
		return TRUE;
	
	stonecount = 0;
	clan = ch->clan;
	if ((vault = get_room_index(clan_table[ch->clan].hall)) == NULL)
		return FALSE;
	for (obj = vault->contents; obj != NULL; obj = obj->next_content)
	{
	if (obj->pIndexData->vnum == OBJ_VNUM_STONE_POWER && obj->carried_by == NULL)
		stonecount++;
	}

	if (stonecount < needed)
		return FALSE;
	else
		return TRUE;
}





/* friend stuff -- for NPC's mostly */
bool is_friend(CHAR_DATA *ch,CHAR_DATA *victim)
{
    int i;

    if (is_same_group(ch,victim))
	return TRUE;

    
    if (!IS_NPC(ch))
	return FALSE;

    if (!IS_NPC(victim))
    {
	if (IS_SET(ch->off_flags,ASSIST_PLAYERS))
	    return TRUE;
	else
	    return FALSE;
    }

    if (IS_AFFECTED(ch,AFF_CHARM))
	return FALSE;

    if (IS_SET(ch->off_flags,ASSIST_ALL))
	return TRUE;

    if (ch->group && ch->group == victim->group)
	return TRUE;

    if (IS_SET(ch->off_flags,ASSIST_VNUM) 
    &&  ch->pIndexData == victim->pIndexData)
	return TRUE;

    if (IS_SET(ch->off_flags,ASSIST_RACE) && ch->race == victim->race)
	return TRUE;

    if (IS_NPC(victim) && IS_SET(ch->off_flags, ASSIST_NPCRACE)
     && (ch->race == victim->race))
	return TRUE;
     
    if (IS_SET(ch->off_flags,ASSIST_ALIGN)
    &&  !IS_SET(ch->act,ACT_NOALIGN) && !IS_SET(victim->act,ACT_NOALIGN)
    &&  ((IS_GOOD(ch) && IS_GOOD(victim))
    ||	 (IS_EVIL(ch) && IS_EVIL(victim))
    ||   (IS_NEUTRAL(ch) && IS_NEUTRAL(victim))))
	return TRUE;

    for (i = 0; i < MAX_ASSIST_VNUM; i++)
	if (ch->pIndexData->assist_vnum[i] == victim->pIndexData->vnum)
	    return TRUE;

    return FALSE;
}

void switch_position(CHAR_DATA *ch, int pos)
{
    free_string(ch->pose);
    if (pos == POS_FIGHTING && global_bool_ranged_attack == TRUE)
        return;
    
    if (pos <= POS_SLEEPING)
    {
        // No singing in your sleep
        if (ch->song)
        {
        	if (ch->harmony) send_to_char("Your songs end.\n\r", ch);
        	else send_to_char("Your song ends.\n\r", ch);
        	stop_playing_song(ch, NULL);
        }

        // No firedancing if asleep
        AFFECT_DATA * paf = get_affect(ch, gsn_firedancer);
        if (paf != NULL && paf->duration < 0)
            stopFiredancing(ch, paf, get_skill(ch, gsn_firedancer));

        // No phantasms if asleep
        if (ch->in_room != NULL)
        {
            CHAR_DATA * vch_next;
            for (CHAR_DATA * vch(ch->in_room->people); vch != NULL; vch = vch_next)
            {
                vch_next = vch->next_in_room;
                if (!IS_NPC(vch) || !IS_SET(vch->act, ACT_ILLUSION) || vch->master != ch)
                    continue;

                // Found a phantasm
                act("As you fall asleep, you release your hold on $N.", ch, NULL, vch, TO_CHAR);
                act("$N fades away.", ch, NULL, vch, TO_ROOM);
                stop_follower(vch);
            }
        }

        // Check for dream mastery
        if (get_skill(ch, gsn_dreammastery) > 0)
        {
            send_to_char("You fall immediately into a deep, restful slumber.\n", ch);
            ch->ticks_slept = 1;
        }
    }

    if (ch->position >= POS_FIGHTING && pos < POS_FIGHTING && IS_AFFECTED(ch, AFF_FLY_NATURAL) && !IS_AFFECTED(ch, AFF_FLYING))
    {
        REMOVE_BIT(ch->affected_by, AFF_FLY_NATURAL);
        send_to_char("You cease naturally flying.\n\r", ch);
    }

    if (ch->position != pos)
    {
        // Pull the ch off any furniture
        if (ch->on != NULL)
        {
            // Make sure any floating disc furniture reverts to container
            if (ch->on->pIndexData->vnum == OBJ_VNUM_DISC)
                revert_floating_disc_from_furniture(*ch->on);

	        ch->on = NULL;
        }

        // Check for echo affect
        AFFECT_DATA * echoAff(get_affect(ch, EchoAffect::GSN));
        if (echoAff != NULL && echoAff->point != NULL)
        {
            // Handle echo affect, including removal if appropriate
            if (static_cast<EchoAffect*>(echoAff->point)->HandlePositionChange(ch, pos))
                affect_remove(ch, echoAff);
        }
    }
    
    if (pos < POS_RESTING)
	if (is_affected(ch,gsn_ready))
	{
	    act("You stop readying your shot.",ch,NULL,NULL,TO_CHAR);
	    act("$n stops readying $s shot.",ch,NULL,NULL,TO_ROOM);
	    affect_strip(ch,gsn_ready);
	}

    // If the position changes from sleeping (or more incapacitated), reset the tick timer
    if (pos > POS_SLEEPING)
        ch->ticks_slept = 0;
    
    ch->position = pos;

    if (IS_NAFFECTED(ch, AFF_LIGHTSLEEP) && (ch->position > POS_SLEEPING))
        affect_strip(ch, gsn_lightsleep);
}

/* returns number of people on an object */
int count_users(OBJ_DATA *obj)
{
    CHAR_DATA *fch;
    int count = 0;

    if (obj->in_room == NULL)
	return 0;

    for (fch = obj->in_room->people; fch != NULL; fch = fch->next_in_room)
	if (fch->on == obj)
	    count++;

    return count;
}
     
/* returns race number */
int race_lookup (const char *name)
{
   int race;

   for ( race = 0; race_table[race].name != NULL; race++)
   {
	if (LOWER(name[0]) == LOWER(race_table[race].name[0])
	&&  !str_prefix( name,race_table[race].name))
	    return race;
   }

   return 0;
} 

int liq_lookup (const char *name)
{
    int liq;

    for ( liq = 0; liq_table[liq].liq_name != NULL; liq++)
    {
	if (LOWER(name[0]) == LOWER(liq_table[liq].liq_name[0])
	&& !str_prefix(name,liq_table[liq].liq_name))
	    return liq;
    }

    return -1;
}

int weapon_lookup (const char *name)
{
    int type;

    for (type = 0; weapon_table[type].name != NULL; type++)
    {
	if (LOWER(name[0]) == LOWER(weapon_table[type].name[0])
	&&  !str_prefix(name,weapon_table[type].name))
	    return type;
    }
 
    return -1;
}

int weapon_type (const char *name)
{
    int type;
 
    for (type = 0; weapon_table[type].name != NULL; type++)
    {
        if (LOWER(name[0]) == LOWER(weapon_table[type].name[0])
        &&  !str_prefix(name,weapon_table[type].name))
            return weapon_table[type].type;
    }
 
    return WEAPON_EXOTIC;
}

int item_ref(int item_type)
{
	int type = 0;
	while (item_table[type].name != NULL && item_table[type].type != item_type) type++;
	return type;
}

int item_lookup(const char *name)
{
    int type;

    for (type = 0; item_table[type].name != NULL; type++)
    {
        if (LOWER(name[0]) == LOWER(item_table[type].name[0])
        &&  !str_prefix(name,item_table[type].name))
            return item_table[type].type;
    }
 
    return -1;
}

char *item_name(int item_type)
{
    int type;

    for (type = 0; item_table[type].name != NULL; type++)
	if (item_type == item_table[type].type)
	    return item_table[type].name;
    return "none";
}

char *weapon_name( int weapon_type)
{
    int type;
 
    for (type = 0; weapon_table[type].name != NULL; type++)
        if (weapon_type == weapon_table[type].type)
            return weapon_table[type].name;
    return "exotic";
}

int season_lookup (const char *name)
{
    int season;

    for (season = 0; season_table[season].name; season++)
        if (!str_cmp(name, season_table[season].name))
	    return season;

    return 0;
}

int god_lookup	(const char *name)
{
    int god;

    for (god = 0; god < MAX_GODS; god++)
	if (!str_cmp(god_table[god].name, name))
	    return god;

    return -1;
}

int attack_lookup  (const char *name)
{
    int att;

    for ( att = 0; attack_table[att].name != NULL; att++)
    {
	if (LOWER(name[0]) == LOWER(attack_table[att].name[0])
	&&  !str_prefix(name,attack_table[att].name))
	    return att;
    }

    return 0;
}

/* returns a flag for wiznet */
long wiznet_lookup (const char *name)
{
    int flag;

    for (flag = 0; wiznet_table[flag].name != NULL; flag++)
    {
	if (LOWER(name[0]) == LOWER(wiznet_table[flag].name[0])
	&& !str_prefix(name,wiznet_table[flag].name))
	    return flag;
    }

    return -1;
}

/* returns class_num number */
int class_lookup (const char *name)
{
   int class_num;
 
   for ( class_num = 0; class_num < MAX_CLASS; class_num++)
   {
        if (LOWER(name[0]) == LOWER(class_table[class_num].name[0])
        &&  !str_prefix( name,class_table[class_num].name))
            return class_num;
   }
 
   return -1;
}


int sphere_lookup (const char *name)
{
   int sphere;
 
   for ( sphere = 0; sphere < MAX_SPHERES; sphere++)
   {
        if (LOWER(name[0]) == LOWER(sphere_table[sphere].name[0])
        &&  !str_prefix( name,sphere_table[sphere].name))
            return sphere;
   }
 
   return -1;
}

int path_lookup(const char * name)
{
    for (int i(0); i < MAX_PATH_COUNT; ++i)
    {
        if (!str_prefix(name, path_table[i].name))
            return i;
    }

    return -1;
}

/* returns ethos number */
int ethos_lookup (const char *name)
{
   int ethos;
 
   for ( ethos = 0; ethos < MAX_ETHOS; ethos++)
   {
        if (LOWER(name[0]) == LOWER(ethos_table[ethos].name[0])
        &&  !str_prefix( name,ethos_table[ethos].name))
            return ethos;
   }
 
   return -1;

}

/* returns arrow-type number */
int arrow_lookup (const char *name)
{
   int arrow;

   for ( arrow = 0; arrow < ARROW_TYPES; arrow++)
   {
        if (LOWER(name[0]) == LOWER(arrow_table[arrow].name[0])
	&&  !str_prefix( name, arrow_table[arrow].name))
            return arrow;
   }

   return -1;

}

ROOM_INDEX_DATA *get_default_hometown(CHAR_DATA *ch)
{
    ROOM_INDEX_DATA *hRoom = NULL;

    if (IS_GOOD(ch))
	hRoom = get_room_index(home_table[1].vnum);
    else if (IS_NEUTRAL(ch))
	hRoom = get_room_index(home_table[2].vnum);
    else if (IS_EVIL(ch))
	hRoom = get_room_index(home_table[3].vnum);

    return hRoom;
};

static int calculateRuneResist(CHAR_DATA * ch, Rune::Type rune, int perRuneBonus)
{
    // Get the rune count
    unsigned int runeCount(Runes::InvokedCount(*ch, rune));
    if (runeCount == 0)
        return 0;

    // This math uses a summation formula underneath; the upshot is that the first rune grants 10 res,
    // the next one 8, the next 6, and so on, always granting at least 1 res
    int result(0);
    if (runeCount >= 5)
    {
        result += (runeCount - 5);
        runeCount = 5;
    }
    result += ((11 - runeCount) * runeCount);

    // Bonuses grant 5% flat, and the per rune bonus adds in as well
    result += (5 * Runes::BonusCount(*ch));
    result += (perRuneBonus * runeCount);
    return result;
}

int get_resist(CHAR_DATA *ch, int res_type)
{
    int res, result;

    if (res_type < TYPE_HIT)
        res = res_type;
    else
    {
        res_type -= TYPE_HIT;

        switch (res_type)
        {
            case DAM_NONE:	res = MAX_RESIST;       break;
            case DAM_BASH:      res = RESIST_BASH;	break;
            case DAM_PIERCE:    res = RESIST_PIERCE;    break;
            case DAM_SLASH:     res = RESIST_SLASH;	break;
            case DAM_FIRE:      res = RESIST_FIRE;      break;
            case DAM_COLD:      res = RESIST_COLD;	break;
            case DAM_LIGHTNING: res = RESIST_LIGHTNING; break;
            case DAM_ACID:      res = RESIST_ACID;      break;
            case DAM_POISON:    res = RESIST_POISON;    break;
            case DAM_NEGATIVE:  res = RESIST_NEGATIVE;  break;
            case DAM_HOLY:	res = RESIST_HOLY;	break;
            case DAM_ENERGY:    res = RESIST_ENERGY;    break;
            case DAM_MENTAL:    res = RESIST_MENTAL;    break;
            case DAM_DISEASE:   res = RESIST_DISEASE;   break;
            case DAM_DROWNING:  res = RESIST_DROWNING;  break;
            case DAM_LIGHT:     res = RESIST_LIGHT;     break;
            case DAM_OTHER:     res = MAX_RESIST;	break;
            case DAM_DEFILEMENT: res = RESIST_DEFILEMENT;  break;
            case DAM_FEAR:      res = RESIST_FEAR;  break;
            case DAM_CHARM:     res = RESIST_CHARM;     break;
            case DAM_SOUND:     res = RESIST_SOUND;	break;
            case DAM_ILLUSION:  res = RESIST_ILLUSION;  break;
            default: 	        res = MAX_RESIST;       break;
        }
    }

    /* check for immunities */
    if (IS_SET(ch->imm_flags, (1 << res)))
	return 100;

    if ((res >= RESIST_BASH) && (res <= RESIST_SLASH))
    {
        if (IS_SET(ch->imm_flags, IMM_WEAPON))
            return 100;

        result = ch->resist[RESIST_WEAPON] + ((res >= MAX_RESIST) ? 0 : ch->resist[res]);
        result += calculateRuneResist(ch, Rune::Stone, 0);
   }
    else
    {
        if (IS_SET(ch->imm_flags, IMM_MAGIC))
            return 100;

        result = ch->resist[RESIST_MAGIC] + ((res >= MAX_RESIST) ? 0 : ch->resist[res]);

        // Check soul of the wind
        std::pair<Direction::Value, int> windInfo(checkWind(ch));
        if (windInfo.second > 0)
        {
            if (number_percent() <= get_skill(ch, gsn_soulofthewind))
            {
                check_improve(ch, NULL, gsn_soulofthewind, true, 12);
                result += 20 + (windInfo.second / 5);
            }
            else
                check_improve(ch, NULL, gsn_soulofthewind, false, 12);
        }
    }

    // Check for brave trait
    if (res == RESIST_FEAR && !IS_NPC(ch) && BIT_GET(ch->pcdata->traits, TRAIT_BRAVE))
        result += 20;

    // Check for runes of ground
    if (res == RESIST_LIGHTNING)
        result += calculateRuneResist(ch, Rune::Ground, 2);

    if (res == RESIST_ILLUSION)
    {
        result += wis_app[get_curr_stat(ch, STAT_WIS)].res_illusion_bonus;
        if (!IS_NPC(ch) && BIT_GET(ch->pcdata->traits, TRAIT_CYNIC))
            result += 10;

        // Check disillusionment
        if (number_percent() <= get_skill(ch, gsn_disillusionment))
        {
            check_improve(ch, NULL, gsn_disillusionment, true, 6);
            result += 15;
        }
        else
            check_improve(ch, NULL, gsn_disillusionment, false, 6);

        // Check for runes of truth
        result += calculateRuneResist(ch, Rune::Truth, 4);
    }

    // Heatlash check
    if (ch->in_room != NULL)
    {
        bool firstAffect = true;
        bool firstGreater = true;
        for (CHAR_DATA * gch(ch->in_room->people); gch != NULL; gch = gch->next_in_room)
        {
            // Applies if any groupmate has the effect up
            if (gch == ch || is_same_group(gch, ch))
            {
                PyreInfo * pyreInfo(getPyreInfoEffect(gch, PyreInfo::Effect_Heatlash));
                if (pyreInfo == NULL)
                    continue;
            
                // Determine whether to apply the affect, and how much
                int multiplier;
                if (res >= RESIST_BASH && res <= RESIST_SLASH && pyreInfo->effectModifier() == PyreInfo::Heatlash_Modifier_Earth) multiplier = 2;
                else if (res == RESIST_FIRE && pyreInfo->effectModifier() == PyreInfo::Heatlash_Modifier_Fire) multiplier = 3;
                else continue;

                result += ((firstAffect ? 20 : 4) * multiplier) / 2;
                firstAffect = false;

                if (pyreInfo->isEffectGreater())
                {
                    result += ((firstGreater ? 8 : 2) * multiplier) / 2;
                    firstGreater = false;
                }
            }
        }
    }

    // Thermal mastery check
    if (res == RESIST_FIRE || res == RESIST_COLD)
    {
        int thermalMasterySkill = get_skill(ch, gsn_thermalmastery);
        if (thermalMasterySkill > 0)
            result += (thermalMasterySkill + (ch->level * 2)) / 20;
    }

	if (res == RESIST_FIRE)
    {
        if (!IS_NPC(ch) && BIT_GET(ch->pcdata->traits, TRAIT_MARKED) && ch->religion == god_lookup("Bayyal"))
    		result += 3;

        // Flameheart check
        int flameheartSkill = get_skill(ch, gsn_flameheart);
        if (flameheartSkill > 0)
        {
            // Flameheart gives 70 resistance to fire at 100%
            result += 20 + (flameheartSkill / 2);
            check_improve(ch, NULL, gsn_flameheart, TRUE, 8);
        }

        // Aspect of the Inferno check
        AFFECT_DATA * aspect(get_affect(ch, gsn_aspectoftheinferno));
        if (aspect != NULL && aspect->modifier == 0)
            result += 20;
    }

    if (res == RESIST_COLD)
    {
        // Flameheart gives a straight 20 vuln to cold
        if (get_skill(ch, gsn_flameheart) > 0)
        {
            result -= 20;
            check_improve(ch, NULL, gsn_flameheart, TRUE, 8);
        }

        // Aspect of the Inferno check
        AFFECT_DATA * aspect(get_affect(ch, gsn_aspectoftheinferno));
        if (aspect != NULL && aspect->modifier == 0)
            result -= 20;

        // Wintertide check
        result += get_skill(ch, gsn_wintertide) / 10;
        check_improve(ch, NULL, gsn_wintertide, true, 8);
    }

    if (res == RESIST_POISON)
    {
        // Detoxify check
        int detox(get_skill(ch, gsn_detoxify));
        if (detox > 0)
        {
            // Detoxify gives 25 resistance to poison at 100%
            result += 10 + UMAX(0, ((detox - 70) / 2));
            check_improve(ch, NULL, gsn_detoxify, true, 8);
        }
    }

    // Check for touch of the desecrator
    if (res == RESIST_DISEASE)
        result += get_skill(ch, gsn_touchofthedesecrator) / 2;

    // Check for revenant 
    switch (res)
    {
        case RESIST_DISEASE:
        case RESIST_POISON:
        case RESIST_DROWNING:   result += get_skill(ch, gsn_revenant) / 3; break;

        case RESIST_FEAR:
        case RESIST_NEGATIVE:
        case RESIST_DEFILEMENT: 
        case RESIST_MENTAL: 
        case RESIST_COLD:       result += get_skill(ch, gsn_revenant) / 4; break;

        case RESIST_HOLY:       if (get_skill(ch, gsn_revenant) > 0) result -= 20; break;
    }
	
    result = URANGE(-100, result, 95);
    return result;
}

int get_age(CHAR_DATA *ch)
{
    int newage, newagegroup = (MAX_AGEGROUP - 1), i;
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
	return -1;

    /* blatant hack to update old pfiles to new method */
    if (ch->pcdata->age_group == AGE_NONE)
    {
	ch->id = get_pc_id() - ((ch->played / 36000) * NUM_DAYS_YEAR * NUM_HOURS_DAY * (PULSE_TICK * 2) / PULSE_PER_SECOND);
	ch->pcdata->age_group = AGE_YOUTHFUL;
	ch->pcdata->last_age = pc_race_table[ch->race].age_values[2];
    }

    newage = pc_race_table[ch->race].age_values[3] + ((current_time - ch->id) / ((PULSE_TICK * 2) / PULSE_PER_SECOND * NUM_HOURS_DAY * NUM_DAYS_YEAR));

    if (ch->pcdata->last_age > 0 && newage > ch->pcdata->last_age && ch->level >= 5 && BIT_GET(ch->pcdata->traits, TRAIT_ARISTOCRAT))
    {
	NOTE_DATA *pnote;
	char *notetime;
	unsigned char amount;
//	time_t notetime;
	char buf[MAX_STRING_LENGTH];

	for (i = 0; i < (newage - ch->pcdata->last_age); i++)
	{
	    pnote = new_note();

	    pnote->sender   = str_dup("Your faithful steward");
	    pnote->to_list  = str_dup(ch->name);
	    pnote->subject  = str_dup("Estate collection");
//	    pnote->type	    = NOTE_NOTE;
	    pnote->next	    = NULL;

	    amount = number_fuzzy(ch->level/2);

	    sprintf(buf, "%s,\n\r\n\rYour estates have had a bountiful and productive year.  %d platinum coins\n\rhave been deposited in your account.\n\r\n\r- Your faithful steward\n\r", ch->name, amount);
	    pnote->text	= str_dup(buf);

	    notetime			= ctime( &current_time );
	    notetime[strlen(notetime)-1]	= '\0';
	    pnote->date			= str_dup(notetime);
	    pnote->date_stamp		= current_time;

	    append_note(pnote);

	    ch->bank[C_PLATINUM] += amount;

 	    send_to_char("You have a new note waiting.\n\r", ch);
	}
    }

    if (newage > ch->pcdata->last_age)
    {
        while (ch->pcdata->last_age < newage)
        {
            ch->pcdata->faction_standing->Decay(*ch);
            ++ch->pcdata->last_age;
        }
    }
    else
        ch->pcdata->last_age = newage;

    newage += (ch->pcdata->age_mod + ch->pcdata->age_mod_perm);

    newage = UMAX(1, newage);

    if (ch->perm_stat[STAT_CON] >= 4)
    {
	for (i = 0; i < (MAX_AGEGROUP - 1); i++)
	    if ((newage >= pc_race_table[ch->race].age_margins[i])
	     && (newage < pc_race_table[ch->race].age_margins[i+1]))
		newagegroup = i;

	if (IS_IMM_TRUST(ch))
	    newagegroup = AGE_MATURE;

        while (newagegroup != ch->pcdata->age_group)
        {
            // Check revenant skill for whether to adjust stats
            bool skipStatAdjust(get_skill(ch, gsn_revenant) > 0 && newagegroup > AGE_MATURE);
            if (newagegroup < ch->pcdata->age_group)
            {
                if (!skipStatAdjust)
                {
                    for (i = 0; i < MAX_STATS; i++)
                        ch->perm_stat[i] -= age_table[ch->pcdata->age_group].stat_mod[i];
                }
                ch->pcdata->age_group--;
            }	    
            else
            {
                ch->pcdata->age_group++;
                if (!skipStatAdjust)
                {
                    for (i = 0; i < MAX_STATS; i++)
                        ch->perm_stat[i] += age_table[ch->pcdata->age_group].stat_mod[i];
                }
            }

            for (i = 0; i < MAX_STATS; i++)
                ch->perm_stat[i] = URANGE(3, ch->perm_stat[i], get_max_stat(ch, i));
        }

        if (ch->pcdata->max_age == 0)
        {
            if (number_percent() < 40)
                ch->pcdata->max_age = number_range(pc_race_table[ch->race].age_values[1], pc_race_table[ch->race].age_values[0]);
            else
            {  
                if (number_percent() < 80)
                    ch->pcdata->max_age = number_range(pc_race_table[ch->race].age_values[0], (pc_race_table[ch->race].age_values[0] * 2) - pc_race_table[ch->race].age_values[1]);
                else
                    ch->pcdata->max_age = number_range((pc_race_table[ch->race].age_values[0] * 2) - pc_race_table[ch->race].age_values[1], pc_race_table[ch->race].age_values[2]);
            }

            ch->pcdata->max_age += (ch->perm_stat[STAT_CON] - 18);
        }	

        if (((newage - ch->pcdata->age_mod) > ch->pcdata->max_age) && !IS_IMM_TRUST(ch))
        {
            int num;

            num = number_range(1, 1);
   
            set_perm_stat(ch, STAT_CON, 1);
            ch->pcdata->age_group = AGE_DEAD;

	    switch (num)
	    {
	        default:
		    send_to_char("Your tired body, wearied from years of adventuring, exhales one final\n\rbreath, and quietly falls to the ground.\n\r", ch);
		    act("$n stumbles slightly, and collapses to the ground.", ch, NULL, NULL, TO_ROOM);
	    }
	    sprintf(buf,"%s died of extreme old age in %s [%i].\n\r",ch->name,ch->in_room->name, ch->in_room->vnum);
	    log_string(buf);

	    raw_kill( ch );
        }
    }

    return newage;
}

int get_moon_state(int moon_num, bool get_size)
{
    if ((time_info.hour >= season_table[time_info.season].sun_up)
      && (time_info.hour < season_table[time_info.season].sun_down))
         return -1; // no moon for you
    if (get_size)
        if (moon_num == 1)
	    return MSIZE_MEDIUM;
	else
	    return time_info.size_rhos;
    else
        if (moon_num == 1)
	    return time_info.phase_lunus;
	else
	    return time_info.phase_rhos;
}
///*int get_moon_state( int moon_num, bool get_size )
//{
//    int rise = 0, set = 0;
//    int num_days = (current_time / ((PULSE_TICK * 2) / PULSE_PER_SECOND * NUM_HOURS_DAY));
//
//    switch( moon_num )
//    {
//	case 1:
//	    if (get_size)
//		return MSIZE_MEDIUM;
//
//	    rise = ((num_days + 28) % 36);
//	    rise = ((rise * 2) / 3);
//	    set = rise + 12;
//	    if (set > 23)
//	        set -= 24;
//	    break;
//	case 2:
//	{
//	    int x;
//
//	    if (get_size)
//	    {
//		x = ((num_days % 63) + 1);
//
//		if ((x >= 1) && (x <= 8))
//		    return MSIZE_TINY;
//		if ((x >= 9) && (x <= 18))
//		    return MSIZE_SMALL;
//		if ((x >= 19) && (x <= 24))
//		    return MSIZE_MEDIUM;
//		if ((x >= 25) && (x <= 29))
//		    return MSIZE_LARGE;
//		if ((x >= 30) && (x <= 34))
//		    return MSIZE_HUGE;
//		if ((x >= 35) && (x <= 39))
//		    return MSIZE_LARGE;
//		if ((x >= 40) && (x <= 45))
//		    return MSIZE_MEDIUM;
//		if ((x >= 46) && (x <= 55))
//		    return MSIZE_SMALL;
//		if ((x >= 56) && (x <= 63))
//		    return MSIZE_TINY;
//	    }
//
//
//	    x = ((num_days / 63) * 4);
//	    x = (x % 24);
//
//	    /* Is this the best way to do it?  No.	*/
//	    /* Do I know any other way to do it?  No.   */
//
//	    switch((num_days % 63) + 1)
//	    {
//		case 1: case 2: case 3: case 4: 	rise = 0;  break;
//		case 5: case 6: case 7: case 8: case 9: rise = 23; break;
//		case 10: case 11: case 12: case 13:	rise = 22; break;
//		case 14: case 15: case 16:		rise = 21; break;
//		case 17: case 18: case 19:		rise = 20; break;
//		case 20: case 21:			rise = 19; break;
//		case 22: case 23:			rise = 18; break;
//		case 24: case 25:			rise = 17; break;
//		case 26: 				rise = 16; break;
//		case 27:				rise = 15; break;
//		case 28: 				rise = 14; break;
//		case 29:				rise = 13; break;
//		case 30:				rise = 12; break;
//		case 31:				rise = 11; break;
//		case 32:				rise = 10; break;
//		case 33:				rise = 9;  break;
//		case 34:				rise = 8;  break;
//		case 35:				rise = 7;  break;
//		case 36:				rise = 6;  break;
//		case 37:				rise = 5;  break;
//		case 38:				rise = 4;  break;
//		case 39: case 40:			rise = 3;  break;
//		case 41: case 42:			rise = 2;  break;
//		case 43: case 44:			rise = 1;  break;
//		case 45: case 46: case 47:		rise = 0;  break;
//		case 48: case 49: case 50:		rise = 23; break;
//		case 51: case 52: case 53: case 54:	rise = 22; break;
//		case 55: case 56: case 57: case 58:
//		case 59:				rise = 21; break;
//		case 60: case 61: case 62: case 63:	rise = 20; break;
//	    }
//
//	    rise += x;
//	    set = rise - 6;
//
//	    if (rise > 23)
//	        rise -= 24;
//
//	    if (set > 23)
//	        set -= 24;
//
//	    if (set < 0)
//		set += 24;
//
//	    break;
//	}
//    }
//		
//    if (((rise > set) && (time_info.hour >= rise) && (time_info.hour < set)) || ((rise < set) && ((time_info.hour >= rise) || (time_info.hour < set))))
//    {
//	/* The moon is in the sky */
//
//	switch (rise)
//	{
//	    case 0:
//		return MOON_WANING_HALF;
//	    case 1: case 2: case 3: case 4:
//		return MOON_WANING_CRESCENT;
//	    case 5: case 6: case 7:
//		return MOON_NEW;
//	    case 8: case 9: case 10: case 11:
//		return MOON_WAXING_CRESCENT;
//	    case 12:
//		return MOON_WAXING_HALF;
//	    case 13: case 14: case 15: case 16:
//		return MOON_WAXING_GIBBOUS;
//	    case 17: case 18: case 19:
//		return MOON_FULL;
//	    case 20: case 21: case 22: case 23:
//		return MOON_WANING_GIBBOUS;
//	}
//    }
//	
//    return -1;
//}*/	

int get_current_month()
{
    return get_month(TtoDY(current_time));
}

int get_month(int day_year)
{
    int mnum, total = 0;

    for (mnum = 0; month_table[mnum].name; mnum++)
    {
        total += month_table[mnum].num_days;
        if (day_year <= total)
            break;
    }

    return mnum;
}
 
bool is_clan(CHAR_DATA *ch)
{
    return ch->clan ? TRUE : FALSE;
}

bool is_same_clan(CHAR_DATA *ch, CHAR_DATA *victim)
{
    if (clan_table[ch->clan].independent)
	return FALSE;
    else 
	return (ch->clan == victim->clan);
}

/* checks mob format */
bool is_old_mob(CHAR_DATA *ch)
{
    if (ch->pIndexData == NULL)
	return FALSE;
    else if (ch->pIndexData->new_format)
	return FALSE;
    return TRUE;
}



CHAR_DATA *get_random_pc_world(CHAR_DATA *ch)
{
    DESCRIPTOR_DATA *d;
    int numpcs = 0;

    for (d = descriptor_list; d; d = d->next)
	if ((d->connected == CON_PLAYING)
	 && !d->original
	 && can_see(ch, d->character))
	    numpcs++;

    for (d = descriptor_list; d; d = d->next)
	if ((d->connected == CON_PLAYING)
	 && !d->original
	 && can_see(ch, d->character))
	{
	    if (number_range(1, numpcs) == 1)
	        return d->character;
	    else
		numpcs--;
	}

    return NULL;
}

static int get_base_skill(CHAR_DATA * ch, PC_DATA * pcdata, int sn)
{
    // If you don't have the skill, just bail out
    if (pcdata->learned[sn] <= 0)
        return 0;

    // If you have the skill but only at 1% and aren't yet high-enough level, bail out
    if (pcdata->learned[sn] == 1 && ch->level < skill_table[sn].skill_level[ch->class_num])
        return 0;

    // You have the skill, carry on
    return pcdata->learned[sn];
}

static int get_base_skill(CHAR_DATA * ch, int sn)
{
    // May only call this with a PC
    if (IS_NPC(ch))
    {
        std::ostringstream mess;
        mess << "Called get_base_skill with NPC: '" << ch->name << "'";
        bug(mess.str().c_str(), 0);
        return 0;
    }

    return get_base_skill(ch, ch->pcdata, sn);
}

int get_skill(CHAR_DATA *ch, int sn, bool displayOnly)
{
    int skill = 0;
    CHAR_DATA *vch;
    AFFECT_DATA *paf;
    OBJ_DATA *wield;
    if (sn == -1) /* shorthand for level based skills */
    	skill = ch->level * 5 / 2;
    else if (sn < -1 || sn > MAX_SKILL)
    {
	    bug("Bad sn %d in get_skill.",sn);
    	skill = 0;
    }
    else
    {
        bool treatAsPC(false);
        int pcSkill(0);
        PC_DATA * pcdata(NULL);
        if (!IS_NPC(ch))
        {
            // Is a pc, so treat as one
            treatAsPC = true;
            pcSkill = get_base_skill(ch, sn);
            pcdata = ch->pcdata;
        }
        else
        {
            // Check for bilocation
            CHAR_DATA * original(find_bilocated_body(ch));
            if (original != NULL && !IS_NPC(original))
            {
                // Has a matching PC to grab the skill from
                treatAsPC = true;
                pcSkill = get_base_skill(original, sn);
                pcdata = original->pcdata;
            }

            // Check for astral projection
            if (is_affected(ch, gsn_astralprojection) && ch->desc != NULL && ch->desc->original != NULL && !IS_NPC(ch->desc->original))
            {
                treatAsPC = true;
                pcSkill = get_base_skill(ch->desc->original, sn);
                pcdata = ch->desc->original->pcdata;
            }
        }

        if (treatAsPC)
        {
            // Get the base skill
            skill = pcSkill;
            if (sn == gsn_dodge && is_affected(ch, gsn_foresight))
                skill = UMAX(skill, 95);
        
            if (sn == gsn_third_attack && is_affected(ch, gsn_celerity))
                skill = UMAX(skill, 75);	

            if (is_affected(ch, gsn_hawkform))
            {
                if (sn == gsn_dodge || sn == gsn_evade || sn == gsn_gouge || sn == gsn_third_attack) 
                    skill = UMAX(skill, 95);
            }
        
            if (sn == gsn_fourth_attack && ch->class_num == global_int_class_watcher  && ((wield = get_eq_char(ch,WEAR_WIELD)) != NULL) && wield->value[0] == WEAPON_STAFF)
            {
                if (!displayOnly && number_percent() < (skill = UMAX(skill, get_skill(ch,gsn_swift_staff))))
                    check_improve(ch, NULL,gsn_swift_staff,TRUE,6);
            }

            if (sn == gsn_shield_block && ch->class_num == global_int_class_bandit)
                skill = UMAX(skill, get_skill(ch,gsn_brawlingblock));

            if (is_affected(ch, gsn_wolfform) || is_affected(ch, gsn_lycanthropy))
            {
                if (sn == gsn_dodge) skill = UMAX(skill, 95);
                if (sn == gsn_third_attack && get_moon_state(2, FALSE) == MOON_FULL) skill = UMAX(skill, 95);
                if ((sn == gsn_enhanced_damage) && (get_moon_state(1, FALSE) == MOON_FULL))	skill = UMAX(skill, 95);
                if (sn == gsn_track) skill = UMAX(skill, 95);
                if (sn == gsn_pursue) skill = UMAX(skill, 95);
            }

            if (is_affected(ch, gsn_bearform))
            {
                if (sn == gsn_enhanced_damage) skill = UMAX(skill, 95);
                if (sn == gsn_brutal_damage) skill = UMAX(skill, 95);
                if (sn == gsn_third_attack)	skill = UMAX(skill, 75);
            }

            if ((sn == gsn_track) && is_affected(ch, gsn_eyes_hunter))
                skill = UMAX(skill, 95);
        
            if ((sn == gsn_staves || sn == gsn_wands) && BIT_GET(pcdata->traits, TRAIT_ARCANE_TOUCH))
                skill = UMAX(skill, UMIN(100, skill + 25));
        
            if (sn == gsn_thievescant && !IS_NPC(ch) && BIT_GET(pcdata->traits, TRAIT_THIEVESCANT))
                skill = UMAX(skill, 100);

            if (sn == gsn_haggle && BIT_GET(pcdata->traits, TRAIT_FRUGAL))
                skill += 5;

            for (paf = ch->affected; paf; paf = paf->next)
            {
                if (((paf->type == gsn_skill) || (paf->type == gsn_borrow_knowledge)) && (paf->modifier == sn) && paf->point != NULL)
                {
                    int *i = (int *) paf->point;
                    skill = UMAX(skill, *i);
                }
            }

            if (sn == gsn_swim && BIT_GET(pcdata->traits, TRAIT_AQUATIC))
                skill = UMAX(skill, UMIN(100, skill + 50));
        
            if (sn >= gsn_language_common && sn <= gsn_language_arcane)
            {
                if (IS_PAFFECTED(ch, AFF_MUTE))
                    return UMIN(skill, 5);
            
                if (is_affected(ch, gsn_polyglot)) return 100;
            }

            if (!IS_IMMORTAL(ch) && (skill_table[sn].spell_fun != spell_lang)
            && ch->perm_stat[STAT_CON] < 4)
                return 0;

            for (paf = ch->affected; paf; paf = paf->next)
                if ((paf->type == gsn_forget) && (paf->modifier == sn))
                    return 0;

            if (ch->fighting && !displayOnly)
            {
                if (is_affected(ch->fighting, gsn_form_of_the_mockingbird)
                  && get_skill(ch->fighting,gsn_form_of_the_mockingbird) * 3 / 4 > number_percent())
                {
                if (sn == gsn_lash /* 436 lash */
                  || sn == gsn_gouge
                  || sn == gsn_gash
                  || sn == gsn_pommel
                  || sn == gsn_ensnare /* 235 ensnare*/
                  || sn == gsn_choke /* 435 choke*/
                  || sn == gsn_batter /* 433 batter*/
                  || sn == gsn_cleave /* 434 cleave*/
                  || sn == gsn_cross /* 437 cross*/
                  || sn == gsn_legsweep /* 229 legsweep*/
                  || sn == gsn_sweep
                  || sn == gsn_strip /* 441 strip*/
                  || sn == gsn_decapitate /*237 decapitate*/
                  || sn == gsn_hamstring /* 233 hamstring*/
                  || sn == gsn_impale /* 227 impale*/
                  || sn == gsn_lunge /* 452 thrust*/
                  || sn == gsn_throw /* 182 throw*/
                  || sn == gsn_flank /* 180 flank*/
                  || sn == gsn_drive /* 444 drive*/
                  || sn == gsn_boneshatter /* 456 boneshatter*/
                  || sn == gsn_bludgeon /*181 bludgeon*/
                  || sn == gsn_hew /*226 hew*/
                  || sn == gsn_shieldslam
                  || sn == gsn_inertial_strike) /* 589 inertial strike*/
                {
                     act("$N's precise swordplay and footwork confuses you for a moment.", ch, NULL, ch->fighting, TO_CHAR);
                     act("Your precise swordplay and footwork confuses $n for a moment.", ch, NULL, ch->fighting, TO_VICT);
                     act("$N's precise swordplay and footwork confuses $n for a moment.", ch, NULL, ch->fighting, TO_NOTVICT);
                     return UMIN(skill, 1);
                }
                }
            }

            if (sn == gsn_dual_wield && IS_NAFFECTED(ch, AFF_ARMSHATTER))
                skill = 0;

            if (((sn == gsn_staves) || (sn == gsn_wands))
             && BIT_GET(pcdata->traits, TRAIT_ARCANE_TOUCH))
                skill += 15;

            if ((sn == gsn_haggle)
             && BIT_GET(pcdata->traits, TRAIT_FRUGAL))
                skill += 10;

            if (ch->in_room)
            {
                for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
                {
                    if (IS_OAFFECTED(vch, AFF_MANTLEOFFEAR) && !is_same_group(ch, vch) && (ch->clan != clan_lookup("SHUNNED")))
                    {
                        paf = NULL;
                        for(paf = vch->affected;paf;paf=paf->next)
                        {
                            if (paf->bitvector == AFF_MANTLEOFFEAR)
                            {
                                skill -= (skill * paf->modifier) / 100;
                                break;
                            }
                        }
                        break;
                    }
                }
            }

            if (pcdata->condition[COND_DRUNK] > 10)
                skill = 9 * skill / 10;

            // Adjust for age brackets
            if (!IS_IMMORTAL(ch) && (skill_table[sn].spell_fun != spell_lang) 
            && (pcdata->age_group <= AGE_MATURE || get_base_skill(ch, pcdata, gsn_revenant) <= 0))
            {
                if (BIT_GET(pcdata->traits, TRAIT_TIMELESS) && pcdata->age_group > AGE_MIDDLE)
                    skill += age_table[pcdata->age_group-1].skill_mod;
                else
                    skill += age_table[pcdata->age_group].skill_mod;
            }
        }
        else /* mobiles */
        {
            if ((skill_table[sn].spell_fun == spell_lang) && IS_SET(ch->nact, ACT_OMNILINGUAL))
                skill = 100;
            else if (IS_SET(ch->nact, ACT_CLASSED))
            {
                // Check level and whether this class gets a group containing the skill
                if (group_contains(ch, sn) && skill_table[sn].skill_level[ch->class_num] <= ch->level)
                    skill = UMIN(100, 75 + (ch->level / 2));
                else
                    skill = 0;
            }
            else if (skill_table[sn].spell_fun != spell_null && ch->desc == NULL && !ch->skilled)
                skill = 40 + 2 * ch->level;

            else if (IS_SET(ch->act, ACT_THIEF) && (sn == gsn_sneak || sn == gsn_hide || sn == gsn_steal))
                skill = ch->level * 2 + 20;

                else if ((sn == gsn_dodge && IS_SET(ch->off_flags,OFF_DODGE))
            ||       (sn == gsn_parry && IS_SET(ch->off_flags,OFF_PARRY)))
                skill = ch->level * 2;

            else if (sn == gsn_shield_block)
                skill = 10 + 2 * ch->level;

            else if (sn == gsn_second_attack 
            && (IS_SET(ch->off_flags,OFF_FAST) || IS_SET(ch->act,ACT_WARRIOR)))
                skill = 10 + 3 * ch->level;

            else if (sn == gsn_third_attack && (IS_SET(ch->act,ACT_WARRIOR) && IS_SET(ch->off_flags, OFF_FAST)))
                skill = 80;

            else if (sn == gsn_fourth_attack)
                skill = 0;

            else if (sn == gsn_dual_wield)
                skill = 0;

            else if (sn == gsn_hand_to_hand || sn == gsn_lesserhandwarp || sn == gsn_handwarp || sn == gsn_greaterhandwarp)
                skill = 40 + 2 * ch->level;

            else if (sn == gsn_trip && IS_SET(ch->off_flags,OFF_TRIP))
                skill = 10 + 3 * ch->level;

            else if (sn == gsn_bash && IS_SET(ch->off_flags,OFF_BASH))
                skill = 10 + 3 * ch->level;

            else if (sn == gsn_disarm 
                 &&  (IS_SET(ch->off_flags,OFF_DISARM) 
                 ||   IS_SET(ch->act,ACT_WARRIOR)
                 ||	  IS_SET(ch->act,ACT_THIEF)))
                skill = 20 + 3 * ch->level;

            else if (sn == gsn_berserk && IS_SET(ch->off_flags,OFF_BERSERK))
                skill = 3 * ch->level;

            else if (sn == gsn_kick)
                skill = 10 + 3 * ch->level;

            else if (sn == gsn_backstab && IS_SET(ch->act,ACT_THIEF))
                skill = 20 + 2 * ch->level;

            else if (sn == gsn_rescue)
                skill = 40 + ch->level; 

            else if (sn == gsn_recall)
                skill = 40 + ch->level;
            else if (is_weapon_skill(sn))
                skill = 40 + 5 * ch->level / 2;

            else if (ch->skilled && !IS_SET(skill_table[sn].flags, SN_NOSKILLED))
               skill = UMIN(100, 3*ch->level);
            else
               skill = 0;

            for (paf = ch->affected; paf; paf = paf->next)
                if (((paf->type == gsn_skill) || (paf->type == gsn_borrow_knowledge)) && (paf->modifier == sn) && paf->point != NULL)
                {
                AFFECT_DATA *faf;
                int *i = (int *) paf->point;

                for (faf = ch->affected; faf; faf = faf->next)
                        if ((faf->type == gsn_forget) && (faf->modifier == sn))
                    return 0;

                skill = *i;
                }
            if (ch->pIndexData->vnum == MOB_VNUM_RETAINER)
                if (sn == gsn_lunge
                  || sn == gsn_fend
                  || sn == gsn_rescue
                  || sn == gsn_bandage)
                skill = 100;

            // Wipe any skills for illusions, allowing only the ones set after this point
            if (IS_SET(ch->act, ACT_ILLUSION))
            {
                skill = 0;

                // Adjust for phantasm traits
                if (sn == gsn_rescue)
                {
                    skill += (PhantasmInfo::traitCount(*ch, PhantasmTrait::Protector) * 40);
                    skill += (PhantasmInfo::traitCount(*ch, PhantasmTrait::Guardian) * 40);
                }
                else if (sn == gsn_dodge)
                {
                    skill += (PhantasmInfo::traitCount(*ch, PhantasmTrait::Agile) * 40);
                    skill += (PhantasmInfo::traitCount(*ch, PhantasmTrait::Mercurial) * 40);
                }
                else if (sn == gsn_second_attack)
                {
                    skill += (PhantasmInfo::traitCount(*ch, PhantasmTrait::Ferocious) * 40);
                    skill += (PhantasmInfo::traitCount(*ch, PhantasmTrait::Dextrous) * 40);
                }
                else if (sn == gsn_bash)
                    skill += (PhantasmInfo::traitCount(*ch, PhantasmTrait::Brute) * 80);

                skill = UMIN(100, skill);
            }

            // Adjust for drake traits
            if (sn == gsn_bash) skill += (Drakes::SpecialCount(*ch, Drakes::Bash) * 20);
            if (sn == gsn_charge) skill += Drakes::SpecialCount(*ch, Drakes::Charge);
            if (sn == gsn_dodge) skill += Drakes::SpecialCount(*ch, Drakes::Defender);
            if (sn == gsn_second_attack) skill += Drakes::SpecialCount(*ch, Drakes::Ferocious);
            if (sn == gsn_controlledflight && Drakes::IsMinimumAge(*ch, Drakes::Wingborne)) skill = UMAX(skill, 100);
        }
    }

    // PCs and NPCs both
    if (is_affected(ch, gsn_brainwash)) skill = skill * 2 / 3;
    if (is_affected(ch, gsn_spiritblock)) skill = skill * 2 / 3;
    if (is_affected(ch, gsn_perfection)) skill = 100;
    if (sn == gsn_haggle && is_affected(ch, gsn_heartofstone)) skill = UMIN(0, skill);

    if (!displayOnly && is_affected(ch, gsn_agony) && number_bits(4) == 0)
    {
        send_to_char("The pain wracking your body prevents you from functioning briefly.\n\r", ch);
        skill /= 2;
    }

    AFFECT_DATA * boilAff(get_affect(ch, gsn_boilblood));
    if (!displayOnly && boilAff != NULL && number_bits(5 - boilAff->modifier) == 0)
    {
        send_to_char("Your boiling blood pains you too much to move properly.\n", ch);
        skill /= 2;
    }

    // Check for avatar skills
    int avatarType(type_of_avatar(ch));
    if (avatarType == gsn_avataroftheannointed)
    {
        if (sn == gsn_feint || sn == gsn_batter || sn == gsn_pugil)
        {
            // Annointed avatars get feint, batter, pugil
            int avatarSkill(get_skill(ch, gsn_avataroftheannointed));
            skill = UMAX(skill, avatarSkill);
        }
    }
    else if (avatarType == gsn_avataroftheprotector && sn == gsn_rescue)
    {
        // Protector avatars get rescue
        int avatarSkill(get_skill(ch, gsn_avataroftheprotector));
        skill = UMAX(skill, avatarSkill);
    }

    // Check for roar of the exalted
    AFFECT_DATA * roar(get_affect(ch, gsn_roaroftheexalted));
    if (roar != NULL)
        skill = UMAX(0, skill - roar->modifier);

    // Check for bewilderment
    AFFECT_DATA * bewilderment(get_affect(ch, gsn_bewilderment));
    if (bewilderment != NULL)
        skill = UMAX(0, skill - bewilderment->modifier);

    // Check for grip of elanthemir
    if (is_affected(ch, gsn_breathofelanthemir))
        skill = UMAX(0, skill - 3);

    // Check for whirlpool
    if (is_affected(ch, gsn_whirlpool))
        skill = UMAX(0, skill - 10);

    // Check for shadeswarm
    AFFECT_DATA * swarm(get_affect(ch, gsn_shadeswarm));
    if (swarm != NULL)
        skill = UMAX(0, skill - swarm->modifier);

    if (is_affected(ch, gsn_insectswarm)) skill = skill * 3 / 4;
    if (is_affected(ch, gsn_pommel)) skill = skill * 9 / 10;
    if (is_affected(ch, gsn_headbutt)) skill = skill * 8 / 10;
    if (!displayOnly && Luck::Check(*ch) == Luck::Lucky) skill = (skill * 3) / 2;

    skill = URANGE(0,skill,100);
    if (IS_OAFFECTED(ch, AFF_INSPIRE) && (skill > 0))
        skill += 5;

    return skill;
}

/* for returning weapon information */
int get_weapon_sn_from_obj(OBJ_DATA *item)
{
    if (item == NULL || item->item_type != ITEM_WEAPON)
        return gsn_hand_to_hand;
    
    switch (item->value[0])
    {
        case(WEAPON_SWORD):     return gsn_sword;        
        case(WEAPON_DAGGER):    return gsn_dagger;      
        case(WEAPON_KNIFE):     return gsn_knife;         
        case(WEAPON_STAFF):     return gsn_staff;        
        case(WEAPON_SPEAR):     return gsn_spear;         
        case(WEAPON_MACE):      return gsn_mace;       
        case(WEAPON_AXE):       return gsn_axe;       
        case(WEAPON_FLAIL):     return gsn_flail;     
        case(WEAPON_WHIP):      return gsn_whip;        
        case(WEAPON_POLEARM):   return gsn_polearm;
        default:		return -1;
   }
}

int get_weapon_sn(CHAR_DATA *ch)
{
    return get_weapon_sn_from_obj(get_eq_char( ch, WEAR_WIELD ));
}

int get_dual_sn(CHAR_DATA *ch)
{
    return get_weapon_sn_from_obj(get_eq_char( ch, WEAR_DUAL_WIELD ));
}

bool is_weapon_skill(int sn)
{
    if ((sn == gsn_sword)
     || (sn == gsn_dagger)
     || (sn == gsn_knife)
     || (sn == gsn_staff)
     || (sn == gsn_spear)
     || (sn == gsn_mace)
     || (sn == gsn_axe)
     || (sn == gsn_flail)
     || (sn == gsn_whip)
     || (sn == gsn_polearm))
	return TRUE;

    return FALSE;
}

int get_weapon_skill(CHAR_DATA *ch, int sn, bool primary)
{
     int skill, x;
     OBJ_DATA *obj;
     int avatarType(type_of_avatar(ch));

     /* -1 is exotic */
    if (sn == -1)
    	skill = UMIN(120,3 * ch->level);
    else if ((sn == gsn_hand_to_hand) && avatarType != -1)
	    skill = get_skill(ch, avatarType);
    else if (sn == gsn_hand_to_hand && is_affected(ch, gsn_shadowfiend))
        skill = UMIN(50, ch->level) + (get_skill(ch, gsn_shadowfiend) / 2);
    else
    	skill = get_skill(ch, sn);

    if (is_affected(ch, gsn_winterwind))
	skill += (4 * get_modifier(ch->affected, gsn_winterwind));

    if (!IS_NPC(ch) && ((ch->class_num == global_int_class_swordmaster) || (ch->class_num == global_int_class_earthtemplar))
     && (primary ? get_weapon_sn(ch) : get_dual_sn(ch) == sn))
	if ((obj = get_eq_char(ch, primary ? WEAR_WIELD : WEAR_DUAL_WIELD)) != NULL)
	    for (x = 0;x < MAX_SWORDS;x++)
	        if (ch->pcdata->favored_vnum[x] == obj->pIndexData->vnum)
		{
		    skill += ch->pcdata->favored_bonus[x];
		    break;
		}

    return skill;
} 

int get_weapon_skill_weapon(CHAR_DATA *ch, OBJ_DATA *obj, bool displayOnly)
{
    int skill, sn, x;
    if (obj == NULL)
	sn = gsn_hand_to_hand;
    else
	sn = get_weapon_sn_from_obj(obj);

     /* -1 is exotic */
    int avatarType(type_of_avatar(ch));
    if (sn == -1)
    	skill = UMIN(120,3 * ch->level);
    else if ((sn == gsn_hand_to_hand) && avatarType != -1)
	    skill = get_skill(ch, avatarType, displayOnly);
    else if (sn == gsn_hand_to_hand && is_affected(ch, gsn_shadowfiend))
        skill = UMIN(50, ch->level) + (get_skill(ch, gsn_shadowfiend, displayOnly) / 2);
    else
    	skill = get_skill(ch, sn, displayOnly);

    if (is_affected(ch, gsn_winterwind))
	skill += (4 * get_modifier(ch->affected, gsn_winterwind));

    if (!IS_NPC(ch) && ((ch->class_num == global_int_class_swordmaster) || (ch->class_num == global_int_class_earthtemplar)))
	if (obj != NULL)
	    for (x = 0;x < MAX_SWORDS;x++)
	        if (ch->pcdata->favored_vnum[x] == obj->pIndexData->vnum)
		{
		    skill += ch->pcdata->favored_bonus[x];
		    break;
		}

    return skill;
} 
int get_save(CHAR_DATA *ch)
{
    int save = ch->saving_throw;
    save += wis_app[get_curr_stat(ch, STAT_WIS)].save_mod;
    return save;
}

/// Used to de-screw characters
void reset_char(CHAR_DATA *ch)
{
     int mod,stat;
     OBJ_DATA *obj, *obj_next;
     AFFECT_DATA *af;
     int i;

     if (IS_NPC(ch))
	return;

    if (ch->pcdata->perm_hit == 0 
    ||	ch->pcdata->perm_mana == 0
    ||  ch->pcdata->perm_move == 0
    ||	ch->pcdata->last_level == 0)
    {
    /* do a FULL reset */
	for (obj = ch->carrying; obj; obj = obj_next)
	{
	    obj_next = obj->next_content;

	    if (!obj->worn_on || IS_SET(obj->worn_on, WORN_CONCEAL1) || IS_SET(obj->worn_on, WORN_CONCEAL2))
		continue;
	    if (!obj->enchanted)
	    for ( af = obj->pIndexData->affected; af != NULL; af = af->next )
	    {
		mod = af->modifier;
		switch(af->location)
		{
		    case APPLY_SEX:	ch->sex		-= mod;
					if (ch->sex < 0 || ch->sex >2)
					    ch->sex = IS_NPC(ch) ?
						0 :
						ch->pcdata->true_sex;
									break;
		    case APPLY_MANA:	ch->max_mana	-= mod;		break;
		    case APPLY_HIT:	ch->max_hit	-= mod;		break;
		    case APPLY_MOVE:	ch->max_move	-= mod;		break;
		}
	    }

            for ( af = obj->affected; af != NULL; af = af->next )
            {
                mod = af->modifier;
                switch(af->location)
                {
                    case APPLY_SEX:     ch->sex         -= mod;         break;
                    case APPLY_MANA:    ch->max_mana    -= mod;         break;
                    case APPLY_HIT:     ch->max_hit     -= mod;         break;
                    case APPLY_MOVE:    ch->max_move    -= mod;         break;
                }
            }
	}
	ch->max_hit -= get_hp_bonus(ch);
	ch->max_mana -= get_mana_bonus(ch);

	/* now reset the permanent stats */
	ch->pcdata->perm_hit 	= ch->max_hit;
	ch->pcdata->perm_mana 	= ch->max_mana;
	ch->pcdata->perm_move	= ch->max_move;
	ch->pcdata->last_level	= ch->played/3600;
	if (ch->pcdata->true_sex < 0 || ch->pcdata->true_sex > 2)
		if (ch->sex > 0 && ch->sex < 3)
	    	    ch->pcdata->true_sex	= ch->sex;
		else
		    ch->pcdata->true_sex 	= 0;

    }

    /* now restore the character to his/her true condition */
    for (stat = 0; stat < MAX_STATS; stat++)
    {
	ch->mod_stat[stat] = 0;
	ch->max_stat[stat] = 0;
    }

    if (ch->pcdata->true_sex < 0 || ch->pcdata->true_sex > 2)
	ch->pcdata->true_sex = 0; 
    ch->sex		= ch->pcdata->true_sex;

    ch->max_hit 	= ch->pcdata->perm_hit + get_hp_bonus(ch);
    ch->max_mana	= ch->pcdata->perm_mana + get_mana_bonus(ch);
    ch->max_move	= ch->pcdata->perm_move;

    for (i = 0; i < 4; i++)
    	ch->armor[i]	= 100;

    ch->hitroll		= 0;
    ch->damroll		= 0;
    ch->saving_throw	= 0;
    ch->luck        = 0;

    for (i = 0; i < MAX_RESIST; i++)
	ch->resist[i] = race_table[ch->race].resist[i];

    /* now start adding back the effects */
//    for (loc = 0; loc < MAX_WEAR; loc++)

    for (obj = ch->carrying; obj; obj = obj_next)
    {
	obj_next = obj->next_content;
//        obj = get_eq_char(ch,loc);

        if (!obj->worn_on || IS_SET(obj->worn_on, WORN_CONCEAL1) || IS_SET(obj->worn_on, WORN_CONCEAL2))
            continue;
	for (i = 0; i < 4; i++)
	    ch->armor[i] -= apply_ac( obj, 0, i );

        if (!obj->enchanted)
	for ( af = obj->pIndexData->affected; af != NULL; af = af->next )
        {
            mod = af->modifier;
            switch(af->location)
            {
		case APPLY_STR:		set_mod_stat(ch, STAT_STR, ch->mod_stat[STAT_STR] + mod); break;
		case APPLY_DEX:		set_mod_stat(ch, STAT_DEX, ch->mod_stat[STAT_DEX] + mod); break;
		case APPLY_INT:		set_mod_stat(ch, STAT_INT, ch->mod_stat[STAT_INT] + mod); break;
		case APPLY_WIS:		set_mod_stat(ch, STAT_WIS, ch->mod_stat[STAT_WIS] + mod); break;
		case APPLY_CON:		set_mod_stat(ch, STAT_CON, ch->mod_stat[STAT_CON] + mod); break;
		case APPLY_CHR:		set_mod_stat(ch, STAT_CHR, ch->mod_stat[STAT_CHR] + mod); break;

		case APPLY_MAXSTR:	set_max_stat(ch, STAT_STR, ch->max_stat[STAT_STR] + mod); break;
		case APPLY_MAXDEX:	set_max_stat(ch, STAT_DEX, ch->max_stat[STAT_DEX] + mod); break;
		case APPLY_MAXINT:	set_max_stat(ch, STAT_INT, ch->max_stat[STAT_INT] + mod); break;
		case APPLY_MAXWIS:	set_max_stat(ch, STAT_WIS, ch->max_stat[STAT_WIS] + mod); break;
		case APPLY_MAXCON:	set_max_stat(ch, STAT_CON, ch->max_stat[STAT_CON] + mod); break;
		case APPLY_MAXCHR:	set_max_stat(ch, STAT_CHR, ch->max_stat[STAT_CHR] + mod); break;

		case APPLY_SEX:		ch->sex			+= mod; break;
		case APPLY_MANA:	ch->max_mana		+= mod; break;
		case APPLY_HIT:		ch->max_hit		+= mod; break;
		case APPLY_MOVE:	ch->max_move		+= mod; break;
		case APPLY_AGE:
		    if (!IS_NPC(ch))
		    {
			ch->pcdata->age_mod += mod;
			get_age(ch);
		    }
		    break;

		case APPLY_AC:		
		    for (i = 0; i < 4; i ++)
			ch->armor[i] += mod; 
		    break;
		case APPLY_HITROLL:	ch->hitroll		+= mod; break;
		case APPLY_DAMROLL:	ch->damroll		+= mod; break;
	
		case APPLY_SAVES:		ch->saving_throw += mod; break;
		case APPLY_SAVING_ROD: 		ch->saving_throw += mod; break;
		case APPLY_SAVING_PETRI:	ch->saving_throw += mod; break;
		case APPLY_SAVING_BREATH: 	ch->saving_throw += mod; break;
		case APPLY_SAVING_SPELL:	ch->saving_throw += mod; break;
		case APPLY_SIZE:		ch->size += mod; break;
        case APPLY_LUCK:        ch->luck += mod; break;
		default:
		    if ((af->location >= FIRST_APPLY_RESIST) && (af->location < (FIRST_APPLY_RESIST + MAX_RESIST)))
			ch->resist[af->location - FIRST_APPLY_RESIST] += mod; break;
	    }
        }
 
        for ( af = obj->affected; af != NULL; af = af->next )
        {
            mod = af->modifier;
            switch(af->location)
            {
		case APPLY_STR:		set_mod_stat(ch, STAT_STR, ch->mod_stat[STAT_STR] + mod); break;
		case APPLY_DEX:		set_mod_stat(ch, STAT_DEX, ch->mod_stat[STAT_DEX] + mod); break;
		case APPLY_INT:		set_mod_stat(ch, STAT_INT, ch->mod_stat[STAT_INT] + mod); break;
		case APPLY_WIS:		set_mod_stat(ch, STAT_WIS, ch->mod_stat[STAT_WIS] + mod); break;
		case APPLY_CON:		set_mod_stat(ch, STAT_CON, ch->mod_stat[STAT_CON] + mod); break;
		case APPLY_CHR:		set_mod_stat(ch, STAT_CHR, ch->mod_stat[STAT_CHR] + mod); break;

		case APPLY_MAXSTR:	set_max_stat(ch, STAT_STR, ch->max_stat[STAT_STR] + mod); break;
		case APPLY_MAXDEX:	set_max_stat(ch, STAT_DEX, ch->max_stat[STAT_DEX] + mod); break;
		case APPLY_MAXINT:	set_max_stat(ch, STAT_INT, ch->max_stat[STAT_INT] + mod); break;
		case APPLY_MAXWIS:	set_max_stat(ch, STAT_WIS, ch->max_stat[STAT_WIS] + mod); break;
		case APPLY_MAXCON:	set_max_stat(ch, STAT_CON, ch->max_stat[STAT_CON] + mod); break;
		case APPLY_MAXCHR:	set_max_stat(ch, STAT_CHR, ch->max_stat[STAT_CHR] + mod); break;
 
                case APPLY_SEX:         ch->sex                 += mod; break;
                case APPLY_MANA:        ch->max_mana            += mod; break;
                case APPLY_HIT:         ch->max_hit             += mod; break;
                case APPLY_MOVE:        ch->max_move            += mod; break;
		case APPLY_AGE:
		    if (!IS_NPC(ch))
		    {
			ch->pcdata->age_mod += mod;
			get_age(ch);
		    }
		    break;

 
                case APPLY_AC:
                    for (i = 0; i < 4; i ++)
                        ch->armor[i] += mod;
                    break;
		case APPLY_HITROLL:     ch->hitroll             += mod; break;
                case APPLY_DAMROLL:     ch->damroll             += mod; break;
 
                case APPLY_SAVES:         ch->saving_throw += mod; break;
                case APPLY_SAVING_ROD:          ch->saving_throw += mod; break;
                case APPLY_SAVING_PETRI:        ch->saving_throw += mod; break;
                case APPLY_SAVING_BREATH:       ch->saving_throw += mod; break;
                case APPLY_SAVING_SPELL:        ch->saving_throw += mod; break;
		case APPLY_SIZE:		ch->size += mod; break;
        case APPLY_LUCK:        ch->luck += mod; break;
		default:
		    if ((af->location >= FIRST_APPLY_RESIST) && (af->location < (FIRST_APPLY_RESIST + MAX_RESIST)))
			ch->resist[af->location - FIRST_APPLY_RESIST] += mod; break;
            }
	}
    }
  
    /* now add back spell effects */
    for (af = ch->affected; af != NULL; af = af->next)
    {
        mod = af->modifier;
        switch(af->location)
        {
		case APPLY_STR:		set_mod_stat(ch, STAT_STR, ch->mod_stat[STAT_STR] + mod); break;
		case APPLY_DEX:		set_mod_stat(ch, STAT_DEX, ch->mod_stat[STAT_DEX] + mod); break;
		case APPLY_INT:		set_mod_stat(ch, STAT_INT, ch->mod_stat[STAT_INT] + mod); break;
		case APPLY_WIS:		set_mod_stat(ch, STAT_WIS, ch->mod_stat[STAT_WIS] + mod); break;
		case APPLY_CON:		set_mod_stat(ch, STAT_CON, ch->mod_stat[STAT_CON] + mod); break;
		case APPLY_CHR:		set_mod_stat(ch, STAT_CHR, ch->mod_stat[STAT_CHR] + mod); break;

		case APPLY_MAXSTR:	set_max_stat(ch, STAT_STR, ch->max_stat[STAT_STR] + mod); break;
		case APPLY_MAXDEX:	set_max_stat(ch, STAT_DEX, ch->max_stat[STAT_DEX] + mod); break;
		case APPLY_MAXINT:	set_max_stat(ch, STAT_INT, ch->max_stat[STAT_INT] + mod); break;
		case APPLY_MAXWIS:	set_max_stat(ch, STAT_WIS, ch->max_stat[STAT_WIS] + mod); break;
		case APPLY_MAXCON:	set_max_stat(ch, STAT_CON, ch->max_stat[STAT_CON] + mod); break;
		case APPLY_MAXCHR:	set_max_stat(ch, STAT_CHR, ch->max_stat[STAT_CHR] + mod); break;
 
                case APPLY_SEX:         ch->sex                 += mod; break;
                case APPLY_MANA:        ch->max_mana            += mod; break;
                case APPLY_HIT:         ch->max_hit             += mod; break;
                case APPLY_MOVE:        ch->max_move            += mod; break;
		case APPLY_AGE:
		    if (!IS_NPC(ch))
		    {
			ch->pcdata->age_mod += mod;
			get_age(ch);
		    }
		    break;

                case APPLY_AC:
                    for (i = 0; i < 4; i ++)
                        ch->armor[i] += mod;
                    break;
                case APPLY_HITROLL:     ch->hitroll             += mod; break;
                case APPLY_DAMROLL:     ch->damroll             += mod; break;
 
                case APPLY_SAVES:         ch->saving_throw += mod; break;
                case APPLY_SAVING_ROD:          ch->saving_throw += mod; break;
                case APPLY_SAVING_PETRI:        ch->saving_throw += mod; break;
                case APPLY_SAVING_BREATH:       ch->saving_throw += mod; break;
                case APPLY_SAVING_SPELL:        ch->saving_throw += mod; break;
		case APPLY_SIZE:		ch->size += mod; break;
        case APPLY_LUCK:        ch->luck += mod; break;
 		default:
		    if ((af->location >= FIRST_APPLY_RESIST) && (af->location < (FIRST_APPLY_RESIST + MAX_RESIST)))
			ch->resist[af->location - FIRST_APPLY_RESIST] += mod; break;
        } 
    }

    /* make sure sex is RIGHT!!!! */
    if (ch->sex < 0 || ch->sex > 2)
	ch->sex = ch->pcdata->true_sex;

    if (BIT_GET(ch->pcdata->traits, TRAIT_MARKED))
    {
        if (ch->religion == god_lookup("Dolgrael"))
		    ch->damroll += 2;
		else if (ch->religion == god_lookup("Iandir"))
	    	for (i = 0; i < 4; i++)	ch->armor[i] -= 50;
		else if (ch->religion == god_lookup("Aeolis"))
		    ch->mod_stat[STAT_CHR] += 2;
		else if (ch->religion == god_lookup("Alajial"))
			for (i = 0; i < 4; i++) ch->armor[i] -= 30;
		else if (ch->religion == god_lookup("Sythrak"))
			ch->damroll += 2;
		else if (ch->religion == god_lookup("Tzajai"))
		    ch->saving_throw -= 2;
    }
}


/*
 * Retrieve a character's trusted level for permission checking.
 */
int get_trust( CHAR_DATA *ch )
{
    if ( ch->desc != NULL && ch->desc->original != NULL )
	ch = ch->desc->original;

    if (ch->trust)
	return ch->trust;

    if ( IS_NPC(ch) && ch->level >= LEVEL_HERO )
	return LEVEL_HERO - 1;
    else
	return ch->level;
}


/*
 * Retrieve a character's age.
 */

/*
int get_age( CHAR_DATA *ch )
{
    return 17 + ( ch->played + (int) (current_time - ch->logon) ) / 72000;
}
*/

int get_max_stat(const CHAR_DATA *ch, int stat)
{
    int max;
    
    if (IS_NPC(ch) || ch->level > LEVEL_IMMORTAL)
	max = 25;
    else
    {
	max = pc_race_table[ch->race].max_stats[stat] + ch->max_stat[stat];
	if (ch->race == global_int_race_human)
	    max += ch->attr_prime[stat];
    
	max = UMIN(max, 25);

	if (BIT_GET(ch->pcdata->traits, TRAIT_CHARMING) && stat == STAT_CHR)
	    max += 1;
    }

    return max;
}

/* command for retrieving stats */
int get_curr_stat(const CHAR_DATA *ch, int stat )
{
    int max = get_max_stat(ch, stat);
    int curr(ch->perm_stat[stat] + ch->mod_stat[stat]);

    // Dexterity-specific modifiers
    if (stat == STAT_DEX)
    {
        // Adjust for encumbrance
        switch (Encumbrance::LevelFor(*ch))
        {
            case Encumbrance::None:     break;
            case Encumbrance::Light:    break;
            case Encumbrance::Medium:   curr -= 2; break;
            case Encumbrance::Heavy:    curr -= 4; break;
            case Encumbrance::Max:      curr -= 6; break;
        }
    }

    // Charisma-specific modifiers
    if (stat == STAT_CHR)
    {
        // Adjust for oathbreaking
        if (Oaths::IsOathBreaker(*ch))
            --curr;
    }

    return URANGE(3, curr, max);
}

int get_hp_bonus(CHAR_DATA *ch)
{
    return (con_app[get_curr_stat(ch, STAT_CON)].hitp * ch->level);
}

int get_mana_bonus(CHAR_DATA *ch)
{
    return (round(((int_app[get_curr_stat(ch, STAT_INT)].mana_bonus / (class_table[ch->class_num].attr_prime == STAT_CHR ? 2 : 1)) + wis_app[get_curr_stat(ch, STAT_WIS)].mana_bonus + (class_table[ch->class_num].attr_prime == STAT_CHR ? chr_app[get_curr_stat(ch, STAT_CHR)].mana_bonus : 0)) * class_table[ch->class_num].fMana / 100.0) * ch->level);
}

void update_hp(CHAR_DATA *ch, int prev_con)
{
    int old_hp_bonus, new_hp_bonus;

    old_hp_bonus = con_app[prev_con].hitp * ch->level;
    new_hp_bonus = con_app[get_curr_stat(ch, STAT_CON)].hitp * ch->level;

    ch->max_hit = UMAX(1, ch->max_hit + new_hp_bonus - old_hp_bonus);

    if (ch->desc && ch->desc->connected == CON_PLAYING && ch->hit > ch->max_hit)
	ch->hit = ch->max_hit;

    return;
}

void update_mana(CHAR_DATA *ch, int prev_stat, int cstat)
{
    int old_mana_bonus, new_mana_bonus;

    old_mana_bonus = round(((int_app[cstat == STAT_INT ? prev_stat : get_curr_stat(ch, STAT_INT)].mana_bonus / (class_table[ch->class_num].attr_prime == STAT_CHR ? 2 : 1)) + wis_app[cstat == STAT_WIS ? prev_stat : get_curr_stat(ch, STAT_WIS)].mana_bonus + (class_table[ch->class_num].attr_prime == STAT_CHR ? chr_app[cstat == STAT_CHR ? prev_stat : get_curr_stat(ch, STAT_CHR)].mana_bonus : 0)) * class_table[ch->class_num].fMana / 100.0) * ch->level;
    new_mana_bonus = round(((int_app[get_curr_stat(ch, STAT_INT)].mana_bonus / (class_table[ch->class_num].attr_prime == STAT_CHR ? 2 : 1)) + wis_app[get_curr_stat(ch, STAT_WIS)].mana_bonus + (class_table[ch->class_num].attr_prime == STAT_CHR ? chr_app[get_curr_stat(ch, STAT_CHR)].mana_bonus : 0)) * class_table[ch->class_num].fMana / 100.0) * ch->level;

    ch->max_mana = ch->max_mana + new_mana_bonus - old_mana_bonus;

    if (ch->desc && ch->desc->connected == CON_PLAYING && ch->mana > ch->max_mana)
	ch->mana = ch->max_mana;

    return;
}

void update_move(CHAR_DATA *ch, int prev_stat, int cstat)
{
    int old_move_bonus, new_move_bonus;

    old_move_bonus = round(((cstat == STAT_CON ? prev_stat : get_curr_stat(ch, STAT_CON)) + (cstat == STAT_DEX ? prev_stat : get_curr_stat(ch, STAT_DEX))) / 10.0 * ch->level);
    new_move_bonus = round((get_curr_stat(ch, STAT_CON) + get_curr_stat(ch, STAT_DEX)) / 10.0 * ch->level);

    ch->max_move = ch->max_move + new_move_bonus - old_move_bonus;

    if (ch->desc && ch->desc->connected == CON_PLAYING && ch->move > ch->max_move)
	ch->move = ch->max_move;

    return;
}

void set_perm_stat(CHAR_DATA *ch, int stat, int value)
{
    int prev_value = get_curr_stat(ch, stat);

    ch->perm_stat[stat] = value;

    if (!IS_NPC(ch))
    {
	if (stat == STAT_CON)
	    update_hp(ch, prev_value);

	if (stat == STAT_DEX || stat == STAT_CON)
	    update_move(ch, prev_value, stat);

	if (stat == STAT_WIS || stat == STAT_INT || stat == STAT_CHR)
	    update_mana(ch, prev_value, stat);
    }

    return;
}

void set_mod_stat(CHAR_DATA *ch, int stat, int value)
{
    int prev_value = get_curr_stat(ch, stat);

    ch->mod_stat[stat] = value;

    if (!IS_NPC(ch))
    {
	if (stat == STAT_CON)
	    update_hp(ch, prev_value);

	if (stat == STAT_DEX || stat == STAT_CON)
	    update_move(ch, prev_value, stat);

	if (stat == STAT_WIS || stat == STAT_INT || stat == STAT_CHR)
	    update_mana(ch, prev_value, stat);
    }

    return;
}

void set_max_stat(CHAR_DATA *ch, int stat, int value)
{
    int prev_value = get_curr_stat(ch, stat);

    ch->max_stat[stat] = value;

    if (!IS_NPC(ch))
    {
	if (stat == STAT_CON)
	    update_hp(ch, prev_value);

	if (stat == STAT_DEX || stat == STAT_CON)
	    update_move(ch, prev_value, stat);

	if (stat == STAT_WIS || stat == STAT_INT || stat == STAT_CHR)
	    update_mana(ch, prev_value, stat);
    }

    return;
}


/* command for returning max training score */
int get_max_train( CHAR_DATA *ch, int stat )
{
    int max;
    
    if (IS_NPC(ch) || ch->level > LEVEL_IMMORTAL)
	return 25;

    max = pc_race_table[ch->race].max_stats[stat];
    if (ch->race == global_int_race_human)   
        max += ch->attr_prime[stat];

    if (stat == STAT_CHR && BIT_GET(ch->pcdata->traits, TRAIT_CHARMING))
        max++;

//    return UMIN(max,25);
    return max;
}
   

int get_max_hunger( CHAR_DATA *ch, PC_DATA * pcdata )
{
    int max = pc_race_table[ch->race].hunger_max;
    if (pcdata == NULL)
        pcdata = ch->pcdata;

    if (BIT_GET(pcdata->traits, TRAIT_HOLLOWLEG))
    	max = max * 23 / 20;

    return max;
}
	
char * is_name_prefix ( char *str, char *namelist )
{
    char name[MAX_INPUT_LENGTH], part[MAX_INPUT_LENGTH];
    char *list, *string;

    /* fix crash on NULL namelist */
    if (namelist == NULL || namelist[0] == '\0')
    	return NULL;

    /* fixed to prevent is_name on "" returning TRUE */
    if (str[0] == '\0')
	return NULL;

    string = str;
    /* we need ALL parts of string to match part of namelist */
    for ( ; ; )  /* start parsing string */
    {
	str = one_argument(str,part);

	if (part[0] == '\0' )
	{
	    strcpy(meep, name);
	    return meep;
	}

	/* check to see if this is part of namelist */
	list = namelist;
	for ( ; ; )  /* start parsing namelist */
	{
	    list = one_argument(list,name);
	    if (name[0] == '\0')  /* this name was not found */
		return NULL;

	    if (!str_prefix(string,name)) 
		{
		strcpy(meep, name);
		return meep; /* full pattern match */
		}

	    if (!str_prefix(part,name))
		break;
	}
    }
}


bool is_name_nopunc ( char *str, char *namelist )
{
    char name[MAX_INPUT_LENGTH], part[MAX_INPUT_LENGTH];
    char *list, *string;
/*  char *sstr;
    int x, len;  */

    /* fix crash on NULL namelist */
    if (namelist == NULL || namelist[0] == '\0')
    	return FALSE;

/*
    for (x = 0, len = strlen(namelist), sstr = namelist;x < len;x++)
    {
	if (ispunct(*namelist))
	  *namelist = ' ';
	namelist++;
    }
    namelist = sstr;
    bug(namelist, 0);
    bug("AIAIAI", 0);
*/

    /* fixed to prevent is_name on "" returning TRUE */
    if (str[0] == '\0')
	return FALSE;

    string = str;
    /* we need ALL parts of string to match part of namelist */
    for ( ; ; )  /* start parsing string */
    {
	str = one_argument(str,part);

	if (part[0] == '\0' )
	    return TRUE;

	/* check to see if this is part of namelist */
	list = namelist;
	for ( ; ; )  /* start parsing namelist */
	{
	    list = one_argument(list,name);
	    if (name[0] == '\0')  /* this name was not found */
		return FALSE;

	    if (!str_cmp(string,name)) 
		return TRUE; /* full pattern match */

	    if (!str_cmp(part,name))
		break;
	}
    }
}

/*
 * See if a string is one of the names of an object.
 */
bool is_name (const char *str, const char *namelist )
{
    char name[MAX_INPUT_LENGTH], part[MAX_INPUT_LENGTH];

    /* fix crash on NULL namelist */
    if (namelist == NULL || namelist[0] == '\0')
    	return FALSE;

    /* fixed to prevent is_name on "" returning TRUE */
    if (str[0] == '\0')
	return FALSE;

    const char * string = str;
    /* we need ALL parts of string to match part of namelist */
    for ( ; ; )  /* start parsing string */
    {
	str = one_argument(str,part);

	if (part[0] == '\0' )
	    return TRUE;

	/* check to see if this is part of namelist */
	const char * list = namelist;
	for ( ; ; )  /* start parsing namelist */
	{
	    list = one_argument(list,name);
	    if (name[0] == '\0')  /* this name was not found */
		return FALSE;

	    if (!str_cmp(string,name)) 
		return TRUE; /* full pattern match */

	    if (!str_cmp(part,name))
		break;
	}
    }
}

bool is_exact_name(const char *str, const char *namelist )
{
    char name[MAX_INPUT_LENGTH];

    if (namelist == NULL)
	return FALSE;

    for ( ; ; )
    {
	namelist = one_argument( namelist, name );
	if ( name[0] == '\0' )
	    return FALSE;
	if ( !str_cmp( str, name ) )
	    return TRUE;
    }
}

char *str_linecopy(char *buf_to, char *buf_from)
{
    char *lend = buf_from;

    while ((*lend != '\n') && (*lend != '\r') && (*lend != '\0'))
	lend++;

    while (buf_from != lend)
	*buf_to++ = *buf_from++;

    *buf_to = '\0';

    while ((*lend == '\n') || (*lend == '\r'))
	lend++;

    if (*lend == '\0')
	return NULL;
    else
	return lend;
}

/* enchanted stuff for eq */
void affect_enchant(OBJ_DATA *obj)
{
    /* okay, move all the old flags into new vectors if we have to */
    if (!obj->enchanted)
    {
        AFFECT_DATA *paf, *af_new;
        obj->enchanted = TRUE;

        for (paf = obj->pIndexData->affected;
             paf != NULL; paf = paf->next)
        {
	    af_new = new_affect();

            af_new->next = obj->affected;
            obj->affected = af_new;
 
	    af_new->where	= paf->where;
            af_new->type        = UMAX(0,paf->type);
            af_new->level       = paf->level;
            af_new->duration    = paf->duration;
            af_new->location    = paf->location;
            af_new->modifier    = paf->modifier;
            af_new->bitvector   = paf->bitvector;
        }
    }
}

bool check_fortitude(CHAR_DATA *ch, AFFECT_DATA *paf, bool fromObj, bool fAdd)
{
    int chance=0;
    if (!ch || !paf)
	return FALSE;
    if (paf->modifier >= 0)
	return FALSE;
    if (paf->type == gsn_resurrection)
	return FALSE;
    if (fromObj)
	return FALSE;
    if (!fAdd)
	return FALSE;
    if (!is_affected(ch,gsn_fortitude))
	return FALSE;
    if ((chance = get_skill(ch,gsn_fortitude)) < 2)
	return FALSE;
    chance /= 4;
    if (IS_AFFECTED(ch,AFF_BERSERK))
	chance += 25;
    if (IS_AFFECTED(ch,AFF_RAGE))
	chance += 25;
    if (number_percent() < chance)
    {
	send_to_char("You ignore the injury.\n\r",ch);
	check_improve(ch,NULL,gsn_fortitude,TRUE,1);
	paf->modifier = 0;
	return TRUE;
    }
    check_improve(ch,NULL,gsn_fortitude,FALSE,1);
    return FALSE;
}           

/*
 * Apply or remove an affect to a character.
 */
void affect_modify( CHAR_DATA *ch, AFFECT_DATA *paf, bool fAdd, bool fromObj)
{
    OBJ_DATA *wield;
    int mod,i;
    mod = paf->modifier;

    if ( fAdd )
    {
        switch (paf->where)
        {
            case TO_AFFECTS: SET_BIT(ch->affected_by, paf->bitvector); break;
            case TO_NAFFECTS: SET_BIT(ch->naffected_by, paf->bitvector); break;
            case TO_OAFFECTS: SET_BIT(ch->oaffected_by, paf->bitvector); break;
            case TO_PAFFECTS: SET_BIT(ch->paffected_by, paf->bitvector); break;
            case TO_IMMUNE: SET_BIT(ch->imm_flags,paf->bitvector); break;
            case TO_RESIST: SET_BIT(ch->res_flags,paf->bitvector); break;
            case TO_VULN: SET_BIT(ch->vuln_flags,paf->bitvector); break;
            case TO_LANG: SET_BIT(ch->lang_flags,paf->bitvector); break;
        }
    }
    else
    {
        switch (paf->where)
        {
        case TO_AFFECTS:
            REMOVE_BIT(ch->affected_by, paf->bitvector);
            break;
	case TO_NAFFECTS:
	    REMOVE_BIT(ch->naffected_by, paf->bitvector);
	    break;
	case TO_OAFFECTS:
	    REMOVE_BIT(ch->oaffected_by, paf->bitvector);
	    break;
	case TO_PAFFECTS:
	    REMOVE_BIT(ch->paffected_by, paf->bitvector);
	    break;
        case TO_IMMUNE:
            REMOVE_BIT(ch->imm_flags,paf->bitvector);
            break;
        case TO_RESIST:
            REMOVE_BIT(ch->res_flags,paf->bitvector);
            break;
        case TO_VULN:
            REMOVE_BIT(ch->vuln_flags,paf->bitvector);
            break;
	case TO_LANG:
	    REMOVE_BIT(ch->lang_flags,paf->bitvector);
	    break;
        }
	mod = 0 - mod;
    }

    switch ( paf->location )
    {
    default:
	if ((paf->location >= FIRST_APPLY_RESIST) && (paf->location < (FIRST_APPLY_RESIST + MAX_RESIST)))
	{
	    ch->resist[paf->location - FIRST_APPLY_RESIST] += mod;
	    break;
	}
	bug( "Affect_modify: unknown location %d.", paf->location );
	return;

    case APPLY_NONE:						break;
    case APPLY_STR:	if (!check_fortitude(ch,paf,fromObj,fAdd)) set_mod_stat(ch, STAT_STR, ch->mod_stat[STAT_STR] + mod); break;
    case APPLY_DEX:	if (!check_fortitude(ch,paf,fromObj,fAdd)) set_mod_stat(ch, STAT_DEX, ch->mod_stat[STAT_DEX] + mod); break;
    case APPLY_INT:	if (!check_fortitude(ch,paf,fromObj,fAdd)) set_mod_stat(ch, STAT_INT, ch->mod_stat[STAT_INT] + mod); break;
    case APPLY_WIS:	if (!check_fortitude(ch,paf,fromObj,fAdd)) set_mod_stat(ch, STAT_WIS, ch->mod_stat[STAT_WIS] + mod); break;
    case APPLY_CON:	if (!check_fortitude(ch,paf,fromObj,fAdd)) set_mod_stat(ch, STAT_CON, ch->mod_stat[STAT_CON] + mod); break;
    case APPLY_CHR:	if (!check_fortitude(ch,paf,fromObj,fAdd)) set_mod_stat(ch, STAT_CHR, ch->mod_stat[STAT_CHR] + mod); break;
    case APPLY_MAXSTR:	if (!check_fortitude(ch,paf,fromObj,fAdd)) set_max_stat(ch, STAT_STR, ch->max_stat[STAT_STR] + mod); break;
    case APPLY_MAXDEX:	if (!check_fortitude(ch,paf,fromObj,fAdd)) set_max_stat(ch, STAT_DEX, ch->max_stat[STAT_DEX] + mod); break;
    case APPLY_MAXINT:	if (!check_fortitude(ch,paf,fromObj,fAdd)) set_max_stat(ch, STAT_INT, ch->max_stat[STAT_INT] + mod); break;
    case APPLY_MAXWIS:	if (!check_fortitude(ch,paf,fromObj,fAdd)) set_max_stat(ch, STAT_WIS, ch->max_stat[STAT_WIS] + mod); break;
    case APPLY_MAXCON:	if (!check_fortitude(ch,paf,fromObj,fAdd)) set_max_stat(ch, STAT_CON, ch->max_stat[STAT_CON] + mod); break;
    case APPLY_MAXCHR:	if (!check_fortitude(ch,paf,fromObj,fAdd)) set_max_stat(ch, STAT_CHR, ch->max_stat[STAT_CHR] + mod); break;
    case APPLY_SEX:           ch->sex			+= mod;	break;
    case APPLY_CLASS:						break;
    case APPLY_LEVEL:						break;
    case APPLY_AGE:	    
	if (!IS_NPC(ch))
        {
	    ch->pcdata->age_mod += mod;
	    get_age(ch);   	
	}
	break;
    case APPLY_HEIGHT:						break;
    case APPLY_WEIGHT:						break;
    case APPLY_MANA:          ch->max_mana		+= mod;	if (!fAdd && !fromObj) ch->mana = UMAX(1, ch->mana + mod); break;
    case APPLY_HIT:           ch->max_hit		+= mod;	if (!fAdd && !fromObj) ch->hit = UMAX(1, ch->hit + mod); break;
    case APPLY_MOVE:          ch->max_move		+= mod;	if (!fAdd && !fromObj) ch->move = UMAX(1, ch->move + mod); break;
    case APPLY_GOLD:						break;
    case APPLY_EXP:						break;
    case APPLY_AC:
        for (i = 0; i < 4; i ++)
            ch->armor[i] += mod;
        break;
    case APPLY_HITROLL:       ch->hitroll		+= mod;	break;
    case APPLY_DAMROLL:       ch->damroll		+= mod;	break;
    case APPLY_SAVES:   ch->saving_throw		+= mod;	break;
    case APPLY_SAVING_ROD:    ch->saving_throw		+= mod;	break;
    case APPLY_SAVING_PETRI:  ch->saving_throw		+= mod;	break;
    case APPLY_SAVING_BREATH: ch->saving_throw		+= mod;	break;
    case APPLY_SAVING_SPELL:  ch->saving_throw		+= mod;	break;
    case APPLY_SPELL_AFFECT:  					break;
    case APPLY_FORM:						break;
    case APPLY_HIDE:						break;
    case APPLY_RANGE:						break;
    case APPLY_SIZE:		ch->size		+= mod; break;
    case APPLY_LUCK:        ch->luck += mod; break;
    }

    /* Let's make sure alignments don't get screwed up */
    if ((paf->type == gsn_obscurealign) && !fAdd)
	ch->alignment = paf->modifier;

    /*
     * Check for weapon wielding.
     * Guard against recursion (for weapons with affects).
     */
    if ( !IS_NPC(ch) 
    && ( wield = get_eq_char( ch, WEAR_WIELD ) ) != NULL
    && ((get_skill(ch,gsn_augmented_strength) > 0) ? 
      (get_obj_weight(wield) > (str_app[URANGE(0,get_curr_stat(ch,STAT_STR)+3,25)].wield*10)) :
      ((get_obj_weight(wield) > (str_app[get_curr_stat(ch,STAT_STR)].wield*10))))
    &&   !obj_is_affected(wield, gsn_cursebarkja)
    &&	 !IS_OBJ_STAT(wield, ITEM_NOREMOVE))
    {
	static int depth;

	if ( depth == 0 )
	{
	    depth++;
	    act( "You no longer possess the strength to wield $p.", ch, wield, NULL, TO_CHAR );
            act( "$p slips from your weakened grasp.", ch, wield, NULL, TO_CHAR);
	    act( "$p slips from $n's weakened grasp.", ch, wield, NULL, TO_ROOM );
	    unequip_char(ch, wield);
	    oprog_remove_trigger(wield);
	    depth--;
	}
    }

    return;
}


/* find an effect in an affect list */
AFFECT_DATA  *affect_find(AFFECT_DATA *paf, int sn)
{
    AFFECT_DATA *paf_find;
    
    for ( paf_find = paf; paf_find != NULL; paf_find = paf_find->next )
    {
        if ( paf_find->type == sn )
	return paf_find;
    }

    return NULL;
}

/* fix object affects when removing one */

void affect_check(CHAR_DATA *ch,int where,int vector)
{
    AFFECT_DATA *paf;
    OBJ_DATA *obj;

    if (where == TO_OBJECT || where == TO_WEAPON || vector == 0)
	return;

    for (paf = ch->affected; paf != NULL; paf = paf->next)
	if (paf->where == where && paf->bitvector == vector)
	{
	    switch (where)
	    {
	        case TO_AFFECTS:
		    SET_BIT(ch->affected_by,vector);
		    break;
		case TO_NAFFECTS:
		    SET_BIT(ch->naffected_by,vector);
		    break;
		case TO_OAFFECTS:
		    SET_BIT(ch->oaffected_by,vector);
		    break;
		case TO_PAFFECTS:
		    SET_BIT(ch->paffected_by, vector);
		    break;
	        case TO_IMMUNE:
		    SET_BIT(ch->imm_flags,vector);   
		    break;
	        case TO_RESIST:
		    SET_BIT(ch->res_flags,vector);
		    break;
	        case TO_VULN:
		    SET_BIT(ch->vuln_flags,vector);
		    break;
		case TO_LANG:
		    SET_BIT(ch->lang_flags,vector);
		    break;
	    }
	    return;
	}

    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
    {
/*	if (obj->wear_loc == -1)
	    continue;
*/

	if (!obj->worn_on)
	    continue;

        for (paf = obj->affected; paf != NULL; paf = paf->next)
        if (paf->where == where && paf->bitvector == vector)
        {
            switch (where)
            {
                case TO_AFFECTS:
                    SET_BIT(ch->affected_by,vector);
                    break;
                case TO_IMMUNE:
                    SET_BIT(ch->imm_flags,vector);
                    break;
                case TO_RESIST:
                    SET_BIT(ch->res_flags,vector);
                    break;
                case TO_VULN:
                    SET_BIT(ch->vuln_flags,vector);
		    break;
		case TO_LANG:
		    SET_BIT(ch->lang_flags,vector);
		    break;
                  
            }
            return;
        }

        if (obj->enchanted)
	    continue;

        for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
            if (paf->where == where && paf->bitvector == vector)
            {
                switch (where)
                {
                    case TO_AFFECTS:
                        SET_BIT(ch->affected_by,vector);
                        break;
                    case TO_IMMUNE:
                        SET_BIT(ch->imm_flags,vector);
                        break;
                    case TO_RESIST:
                        SET_BIT(ch->res_flags,vector);
                        break;
                    case TO_VULN:
                        SET_BIT(ch->vuln_flags,vector);
                        break;
		    case TO_LANG:
			SET_BIT(ch->lang_flags,vector);
			break;
                }
                return;
            }
    }
}

/*
 * Give an affect to a char.
 */
void affect_to_char( CHAR_DATA *ch, AFFECT_DATA *paf )
{
    AFFECT_DATA *paf_new;

    paf_new = new_affect();

    *paf_new		= *paf;
    paf_new->valid = true;

    paf_new->next	= ch->affected;
    ch->affected	= paf_new;

    affect_modify( ch, paf_new, TRUE, FALSE);
}

/* give an affect to a room. (can you say "stone to mud"?) */
/* Yes, I can.  -- Erinos				   */
void affect_to_room(ROOM_INDEX_DATA *room, AFFECT_DATA *paf)
{
    AFFECT_DATA *paf_new;

    paf_new = new_affect();

    *paf_new		= *paf;
    paf_new->valid = true;
    paf_new->next	= room->affected;
    room->affected	= paf_new;
}

/* give an affect to an area (can you say "creeping curse"?) */
void affect_to_area(AREA_DATA *area, AFFECT_DATA *paf)
{
    AFFECT_DATA *paf_new;

    paf_new = new_affect();

    *paf_new		= *paf;
    paf_new->valid = true;
    paf_new->next	= area->affected;
    area->affected	= paf_new;
}

static void applyEffectBits(OBJ_DATA & obj, const AFFECT_DATA & paf)
{
    switch (paf.where)
    {
        case TO_OBJECT: SET_BIT(obj.extra_flags[0], paf.bitvector); break;
        case TO_OBJECT1: SET_BIT(obj.extra_flags[1], paf.bitvector); break;
        case TO_WEAPON:
            if (obj.item_type == ITEM_WEAPON)
                SET_BIT(obj.value[4], paf.bitvector);
            break;
    }
}

/* give an affect to an object */
void affect_to_obj(OBJ_DATA *obj, AFFECT_DATA *paf)
{
    AFFECT_DATA *paf_new;

    paf_new = new_affect();

    *paf_new		= *paf;
    paf_new->valid = true;
    paf_new->next	= obj->affected;
    obj->affected	= paf_new;

    /* apply any affect vectors to the object's extra_flags */
    if (paf->bitvector)
        applyEffectBits(*obj, *paf);
    else if (paf->location == APPLY_WEIGHT) /* hack */
        obj->weight += paf->modifier;

    if (obj->carried_by && obj->worn_on
     && !IS_SET(obj->worn_on, WORN_CONCEAL1)
     && !IS_SET(obj->worn_on, WORN_CONCEAL2)
     &&	(paf->location != APPLY_SPELL_AFFECT))
	affect_modify(obj->carried_by, paf, TRUE, TRUE);    
}


/*
 * Remove an affect from a char.
 */
void affect_remove( CHAR_DATA *ch, AFFECT_DATA *paf )
{
    int where;
    int vector;

    if ( ch->affected == NULL )
    	return;

    // Check for stripping stonecraft
    bool shouldRestore(true);
    if (paf->type == gsn_stonecraft)
    {
        if (paf->location == APPLY_HIDE)
        {
            // Lookup the object
            OBJ_DATA * obj(static_cast<OBJ_DATA*>(paf->point));
            if (obj == NULL)
                bug("Null object in invoked rune effect", 0);
            else
            {
                // Found the object, turn off the invocation
                AFFECT_DATA * objPaf(get_obj_affect(obj, gsn_stonecraft));
                const Rune * rune(NULL);
                if (objPaf == NULL) 
                    bug("Null object effect in invoked rune effect", 0);
                else
                {
                    rune = RuneTable::Lookup(static_cast<Rune::Type>(objPaf->modifier));
                    if (objPaf->point != ch) bug("Invalid object pointer in invoked runed effect", 0);
                    else objPaf->point = NULL;
                }
            
                // Handle the rune
                if (rune == NULL)
                    bug("Missing matching rune in invoked rune effect", 0);
                else
                {
                    // Echo about the revocation
                    std::ostringstream mess;
                    mess << "You cease invoking the rune of " << rune->name << " on $p, and feel its earthern power fade.";
                    act(mess.str().c_str(), ch, obj, NULL, TO_CHAR);

                    if (obj->carried_by != NULL && obj->carried_by != ch)
                        act("The earthern rune on $p quiets, its power subsiding.", obj->carried_by, obj, NULL, TO_CHAR);
                }
            }

            // Update the mana costs
            Runes::UpdateManaCostsFor(*ch);
        }
        else if (paf->location == APPLY_MANA)
            shouldRestore = false;
    }

    // Check for stripping bilocation
    if (paf->type == gsn_bilocation)
    {
        // Lookup the other body
        CHAR_DATA * body(find_bilocated_body(ch, paf));
        if (body != NULL)
        {
            // The other body must lose its pointer and modifier
            AFFECT_DATA * paf(get_affect(body, gsn_bilocation));
            if (paf == NULL)
                bug("Missing bilocation effect on just-found body", 0);
            else
            {
                paf->point = NULL;
                paf->modifier = -1;
            }

            // Determine real/illusory bodies
            CHAR_DATA * realBody(NULL);
            CHAR_DATA * illusoryBody(NULL);
            if (IS_NPC(ch)) {realBody = body;   illusoryBody = ch;}
            else            {realBody = ch;     illusoryBody = body;}

            // Handle any necessary relocation
            if (illusoryBody->desc == NULL)
                send_to_char("The link between your forms frays away to nothingness.\n", realBody);
            else
            {
                // Inhabiting the illusory body; punt him back to the real one
                relocate_consciousness(illusoryBody, realBody);
                send_to_char("The link between your forms frays, and with a dizzying rush your consciousness tumbles back into your real body.\n", realBody);
                WAIT_STATE(realBody, UMAX(realBody->wait, 2 * PULSE_VIOLENCE));
            }
        }
    }

    // Check for stripping miasmaofwaning
    if (paf->type == gsn_miasmaofwaning && paf->location == APPLY_HIT)
    {
        ch->hit += reinterpret_cast<int>(paf->point);
        shouldRestore = false;
    }

    if ((paf->where == TO_OAFFECTS) && (paf->bitvector == AFF_DISGUISE))
    {
        char buf[26];

        free_string(ch->long_descr);
        if (ch->orig_long[0] != '\0')
        {
            free_string(ch->long_descr);
            ch->long_descr = str_dup(ch->orig_long);
            free_string(ch->orig_long);
            ch->orig_long = &str_empty[0];
        }
        else
            ch->long_descr = &str_empty[0];

        free_string(ch->short_descr);
        if (ch->orig_short[0] != '\0')
        {
            free_string(ch->short_descr);
            ch->short_descr = str_dup(ch->orig_short);
            free_string(ch->orig_short);
            ch->orig_short = &str_empty[0];
        }
        else
            ch->short_descr = &str_empty[0];
        
        setFakeName(*ch, str_empty);
        if (ch->orig_description[0] != '\0')
        {
            free_string(ch->description);
            ch->description = str_dup(ch->orig_description);
            free_string(ch->orig_description);
            ch->orig_description = str_dup("");
        }
        sprintf(buf, "%s0", ch->name);
        setUniqueName(*ch, buf);
        ch->fake_race = -1;
    }

    if ((paf->type == gsn_enhance_pain) || (paf->type == gsn_reduce_pain))
        ch->fake_hit = 0;

    affect_modify( ch, paf, FALSE, !shouldRestore);
    where = paf->where;
    vector = paf->bitvector;

    if (paf->type == gsn_obscurealign)
	ch->alignment = paf->modifier;

    if ( paf == ch->affected )
        ch->affected	= paf->next;
    else
    {
        AFFECT_DATA *prev;

        for ( prev = ch->affected; prev != NULL; prev = prev->next )
        {
            if ( prev->next == paf )
            {
            prev->next = paf->next;
            break;
            }
        }

        if ( prev == NULL )
        {
            bug( "Affect_remove: cannot find paf.", 0 );
            return;
        }
    }

    free_affect(paf);
    affect_check(ch,where,vector);

    // Wake up mobs which were sleeping because of the effect
    if (IS_NPC(ch) && where == TO_AFFECTS && IS_SET(vector, AFF_SLEEP))
        do_wake(ch, "");

    // Make sure pass door is still in place, if appropriate
    if (ch->race == global_int_race_chtaren)
    	SET_BIT(ch->affected_by, AFF_PASS_DOOR);
    else
    {
        for (AFFECT_DATA * pass(ch->affected); pass != NULL; pass = pass->next)
        {
            if (pass->where == TO_AFFECTS && IS_SET(pass->bitvector, AFF_PASS_DOOR))
            {
                SET_BIT(ch->affected_by, AFF_PASS_DOOR);
                break;
            }
        }
    }
}

void affect_remove_room( ROOM_INDEX_DATA *room, AFFECT_DATA *paf)
{
    int where, vector;
    if ( room->affected == NULL )
    {
        bug( "Affect_remove_room: no affect.", 0 );
        return;
    }

    if ((paf->type == gsn_plantgrowth) || (paf->type == gsn_wallofwater) || paf->type == gsn_flood)
    {
    	room->sector_type = paf->modifier;
        if (!is_water_room(*room))
        {
            room_affect_strip(room, gsn_maelstrom);
            room_affect_strip(room, gsn_boilseas);
            room_affect_strip(room, gsn_contaminate);
        }
    }

    if (paf->type == gsn_curse)
        REMOVE_BIT(room->room_flags, ROOM_NO_RECALL);

    where = paf->where;
    vector = paf->bitvector;

    if ( paf == room->affected )
    {
        room->affected    = paf->next;
    }
    else
    {
        AFFECT_DATA *prev;

        for ( prev = room->affected; prev != NULL; prev = prev->next )
        {
            if ( prev->next == paf )
            {
                prev->next = paf->next;
                break;
            }
        }

        if ( prev == NULL )
        {
            bug( "Affect_remove_room: cannot find paf.", 0 );
            return;
        }
    }

    free_affect(paf);

    return;
}

void affect_remove_area( AREA_DATA *area, AFFECT_DATA *paf)
{
    int where, vector;
    if ( area->affected == NULL )
    {
        bug( "Affect_remove_area: no affect.", 0 );
        return;
    }

    // Handle echoes and other such things
    if (paf->type == gsn_sunderweave)
    {
        for (DESCRIPTOR_DATA * d(descriptor_list); d != NULL; d = d->next)
        {
            // Filter out PCs not in the same area
            if (d->character == NULL || d->character->in_room == NULL || d->character->in_room->area != area)
                continue;

            // Only inform spellcasters
            if (is_spellcaster(d->character))
                send_to_char("You feel your link to magic return as the Weave heals around you.\n", d->character);
        }
    }

    // Now strip the affect out
    where = paf->where;
    vector = paf->bitvector;

    if ( paf == area->affected )
    {
        area->affected    = paf->next;
    }
    else
    {
        AFFECT_DATA *prev;

        for ( prev = area->affected; prev != NULL; prev = prev->next )
        {
            if ( prev->next == paf )
            {
                prev->next = paf->next;
                break;
            }
        }

        if ( prev == NULL )
        {
            bug( "Affect_remove_area: cannot find paf.", 0 );
            return;
        }
    }

    free_affect(paf);
}

void affect_remove_obj( OBJ_DATA *obj, AFFECT_DATA *paf)
{
    int where, vector;
    if ( obj->affected == NULL )
    {
        bug( "Affect_remove_object: no affect.", 0 );
        return;
    }

    // Handle stonecraft removal
    if (paf->type == gsn_stonecraft && paf->point != NULL)
    {
        // Find the appropriate char and clean up the invocation
        CHAR_DATA * ch(static_cast<CHAR_DATA*>(paf->point));
        for (AFFECT_DATA * chPaf(get_affect(ch, gsn_stonecraft)); chPaf != NULL; chPaf = get_affect(ch, gsn_stonecraft, chPaf))
        {
            if (chPaf->point == obj)
            {
                affect_remove(ch, chPaf);
                break;
            }
        }
    }

    if (obj->carried_by && obj->worn_on)
        affect_modify( obj->carried_by, paf, FALSE, TRUE);

    where = paf->where;
    vector = paf->bitvector;

    /* remove flags from the object if needed */
    if (paf->bitvector)
    {
        switch( paf->where)
        {
	    case TO_OBJECT:
		REMOVE_BIT(obj->extra_flags[0],paf->bitvector);
		obj->extra_flags[0] |= (obj->pIndexData->extra_flags[0] & paf->bitvector);
		if (paf->type == gsn_leechrune)
			REMOVE_BIT(obj->extra_flags[0], ITEM_NOREMOVE);
		break;

	    case TO_OBJECT1:
		REMOVE_BIT(obj->extra_flags[1],paf->bitvector);
		obj->extra_flags[1] |= (obj->pIndexData->extra_flags[1] & paf->bitvector);
		break;

	    case TO_WEAPON:
		if (obj->item_type == ITEM_WEAPON)
		{
		    REMOVE_BIT(obj->value[4],paf->bitvector);
		    obj->value[4] |= (obj->pIndexData->value[4] & paf->bitvector);
		}
		break;
        }
    }
    else if (paf->location == APPLY_WEIGHT)
        obj->weight -= paf->modifier;

    if (paf->type == gsn_dancingsword)
    	REMOVE_BIT(obj->wear_flags, ITEM_WEAR_FLOAT);
        
    if (paf->type == gsn_hoveringshield)
    {
    	REMOVE_BIT(obj->wear_flags, ITEM_WEAR_FLOAT);
        SET_BIT(obj->wear_flags, ITEM_WEAR_SHIELD);
    }

    if ( paf == obj->affected )
    {
        obj->affected    = paf->next;
    }
    else
    {
        AFFECT_DATA *prev;

        for ( prev = obj->affected; prev != NULL; prev = prev->next )
        {
            if ( prev->next == paf )
            {
                prev->next = paf->next;
                break;
            }
        }

        if ( prev == NULL )
        {
            bug( "Affect_remove_object: cannot find paf.", 0 );
            return;
        }
    }

    free_affect(paf);

    // Adjust the character effects
    if (obj->carried_by && obj->worn_on)
        affect_check(obj->carried_by,where,vector);

    // Adjust the object bits
    for (const AFFECT_DATA * objPaf(obj->affected); objPaf != NULL; objPaf = objPaf->next)
        applyEffectBits(*obj, *objPaf);
}

/*
 * Strip all affects of a given sn.
 */
void affect_strip( CHAR_DATA *ch, int sn )
{
    AFFECT_DATA *paf = NULL;
    AFFECT_DATA *paf_next = NULL;

    for ( paf = ch->affected; paf != NULL; paf = paf_next )
    {
	paf_next = paf->next;
	if ( paf->type == sn )
	    affect_remove( ch, paf );
	
    }

    return;
}

void object_affect_strip( OBJ_DATA *obj, int sn )
{
    AFFECT_DATA *paf = NULL;
    AFFECT_DATA *paf_next = NULL;

    for ( paf = obj->affected; paf != NULL; paf = paf_next )
    {
	paf_next = paf->next;
	if ( paf->type == sn )
	    affect_remove_obj( obj, paf );
    }

    return;
}

void room_affect_strip( ROOM_INDEX_DATA *room, int sn )
{
    AFFECT_DATA *paf = NULL;
    AFFECT_DATA *paf_next = NULL;

    for ( paf = room->affected; paf != NULL; paf = paf_next )
    {
	paf_next = paf->next;
	if ( paf->type == sn )
	    affect_remove_room( room, paf );
    }

    return;
}

void area_affect_strip( AREA_DATA *area, int sn )
{
    AFFECT_DATA *paf = NULL;
    AFFECT_DATA *paf_next = NULL;

    for ( paf = area->affected; paf != NULL; paf = paf_next )
    {
	paf_next = paf->next;
	if ( paf->type == sn )
	    affect_remove_area( area, paf );
    }
}

/* return an int, which is the modifier of the affect
    matching sn in a list of affect_datas */
int get_modifier( AFFECT_DATA *paf, int sn )
{

	if (paf == NULL)
		return -1;

    for ( ; paf != NULL; paf = paf->next )
    {
	if ( paf->type == sn )
	    return paf->modifier;
    }

    return -1;
}

int get_aff_level( AFFECT_DATA *paf, int sn )
{

	if (paf == NULL)
		return -1;

    for ( ; paf != NULL; paf = paf->next )
    {
	if ( paf->type == sn )
	    return paf->level;
    }

    return -1;
}


void * get_aff_point ( AFFECT_DATA *paf, int sn)
{
	if (paf == NULL)
		return NULL;

	for ( ; paf != NULL; paf = paf->next )
	{
		if (paf->type == sn && paf->point)
			return paf->point;
	}

	return NULL;
}


void up_modifier( AFFECT_DATA *paf, int sn)
{

	if (paf == NULL)
		return;

    for ( ; paf != NULL; paf = paf->next )
    {
	if ( paf->type == sn )
	    paf->modifier++;
    }

}

int count_obj_affects(const OBJ_DATA * obj, int sn)
{
    int count(0);
    for (AFFECT_DATA * paf(get_obj_affect(const_cast<OBJ_DATA*>(obj), sn)); paf != NULL; paf = get_obj_affect(const_cast<OBJ_DATA*>(obj), sn, paf))
        ++count;

    return count;
}

int get_high_modifier( AFFECT_DATA *paf, int sn )
{

	if (paf == NULL)
		return -1;

    for ( ; paf != NULL; paf = paf->next )
    {
	if ( paf->type == sn && paf->modifier >= 500 )
	    return paf->modifier;
    }

    return -1;
}


//Same as is_affected, but returns actual affect
AFFECT_DATA * get_affect(const CHAR_DATA *ch, int sn, const AFFECT_DATA * lastAffect)
{
	if (ch == NULL) return NULL;

	for (AFFECT_DATA * paf(lastAffect == NULL ? ch->affected : lastAffect->next); paf != NULL; paf = paf->next )
	{
		if ( paf->type == sn )
			return paf;
	}

	return NULL;
}

/*
 * Return true if a char is affected by a spell.
 */
bool is_affected(const CHAR_DATA *ch, int sn )
{
    AFFECT_DATA *paf;

	if (ch == NULL)
		return FALSE;

    for (paf = ch->affected; paf != NULL; paf = paf->next)
    {
    	if (paf->type == sn )
	        return TRUE;
    }

    return FALSE;
}

// Return true if a char is affected by a spell and the loc matches
bool is_loc_affected(CHAR_DATA *ch, int sn, int loc)
{
	AFFECT_DATA *paf;

	if (ch == NULL)	return FALSE;
	for ( paf = ch->affected; paf != NULL; paf = paf->next )
	{
		if (paf->type == sn && paf->location == loc)
			return TRUE;
	}

	return FALSE;
}

// Return true if a char is affected by a spell and loc does not match
bool is_loc_not_affected(CHAR_DATA *ch, int sn, int loc)
{
	AFFECT_DATA *paf;

	if (ch == NULL) return FALSE;
	for ( paf = ch->affected; paf != NULL; paf = paf->next )
	{
		if (paf->type == sn && paf->location != loc)
			return TRUE;
	}

	return FALSE;
}

AFFECT_DATA * get_room_affect_with_modifier(ROOM_INDEX_DATA * room, int sn, int modifier)
{
    for (AFFECT_DATA * paf(room->affected); paf != NULL; paf = paf->next)
    {
        if (paf->type == sn && paf->modifier == modifier)
            return paf;
    }

    return NULL;
}

AFFECT_DATA * get_area_affect(AREA_DATA * area, int sn, AFFECT_DATA * lastAffect)
{
	AFFECT_DATA *paf;
	AFFECT_DATA *paf_next;

	if (area == NULL)
		return NULL;

	for (paf = (lastAffect == NULL ? area->affected : lastAffect->next); paf != NULL; paf = paf_next)
	{
		paf_next = paf->next;
		if ( paf->type == sn )
			return paf;
	}		

	return NULL;
}

AFFECT_DATA * get_room_affect(ROOM_INDEX_DATA * room, int sn, AFFECT_DATA * lastAffect)
{
	AFFECT_DATA *paf;
	AFFECT_DATA *paf_next;

	if (room == NULL)
		return NULL;

	for (paf = (lastAffect == NULL ? room->affected : lastAffect->next); paf != NULL; paf = paf_next)
	{
		paf_next = paf->next;
		if ( paf->type == sn )
			return paf;
	}		

	return NULL;
}

bool room_is_affected(ROOM_INDEX_DATA * room, int sn)
{
    return (get_room_affect(room, sn) != NULL);
}

bool area_is_affected(AREA_DATA *area, int sn)
{
    AFFECT_DATA *paf;

    if (area == NULL)
	return FALSE;

    for ( paf = area->affected; paf != NULL; paf = paf->next )
    {
	if ( paf->type == sn )
	    return TRUE;
    }

    return FALSE;
}

OBJ_DATA *get_obj_potioncontainer(CHAR_DATA *ch)
// Find an alchemist's potion container. Returns NULL if not found.
{
    OBJ_DATA *pc = NULL;
    for (pc = ch->carrying;pc;pc = pc->next)
        if (pc->pIndexData->vnum == OBJ_VNUM_POTION_CONTAINER)
	    break;
    return pc;
}


//Same as obj_is_affected, but returns the affect rather than a bool
AFFECT_DATA * get_obj_affect(const OBJ_DATA * obj, int sn, const AFFECT_DATA * lastAffect)
{
	AFFECT_DATA *paf;
	AFFECT_DATA *paf_next;

	if (!obj)
		return NULL;

	for ( paf = (lastAffect == NULL ? obj->affected : lastAffect->next); paf != NULL; paf = paf_next )
	{
		paf_next = paf->next;
		if ( paf->type == sn )
			return paf;
	}		

	return NULL;
}

bool obj_is_affected(OBJ_DATA *obj, int sn)
{
    AFFECT_DATA *paf;
    AFFECT_DATA *paf_next;

    if (!obj)
	return FALSE;

    for ( paf = obj->affected; paf != NULL; paf = paf_next )
    {
        paf_next = paf->next;
	if ( paf->type == sn )
	    return TRUE;
    }

    return FALSE;
}

/*
 * Add or enhance an affect.
 */
void affect_join( CHAR_DATA *ch, AFFECT_DATA *paf )
{
    AFFECT_DATA *paf_old, *paf_next;
    bool found;

    found = FALSE;
    for ( paf_old = ch->affected; paf_old != NULL; paf_old = paf_next )
    {
        paf_next = paf_old->next;
        if ( paf_old->type == paf->type && paf_old->location == paf->location )
        {
            paf->level = (paf->level += paf_old->level) / 2;
            paf->duration += paf_old->duration;
            paf->modifier += paf_old->modifier;
            affect_remove( ch, paf_old );
            break;
        }
    }

    affect_to_char( ch, paf );
}

void obj_affect_join( OBJ_DATA *obj, AFFECT_DATA *paf )
{
    AFFECT_DATA *affected;

    for ( affected = obj->affected; affected; affected = affected->next )
    {
        if ( affected->type == paf->type && affected->location == paf->location )
        {
            paf->level = (paf->level + affected->level) / 2;

            if (paf->duration >= 0 && affected->duration >= 0)
            paf->duration += affected->duration;
                
            paf->modifier += affected->modifier;
            affect_remove_obj( obj, affected );
            break;
        }
    }

    affect_to_obj( obj, paf );
}


/*
 * Move a char out of a room.
 */
void char_from_room( CHAR_DATA *ch )
{
    if ( ch->in_room == NULL )
    {
		char buf[MAX_STRING_LENGTH];
		sprintf(buf, "Char_from_room (%s): NULL.", (ch->name ? ch->name : "(null)"));
		bug(buf, 0 );
		return;
    }

    // Check for echo affect
    AFFECT_DATA * echoAff(get_affect(ch, EchoAffect::GSN));
    if (echoAff != NULL && echoAff->point != NULL)
    {
        // Handle echo affect, including removal if appropriate
        if (static_cast<EchoAffect*>(echoAff->point)->HandleMoveFrom(ch, ch->in_room))
            affect_remove(ch, echoAff);
    }

    // Check for phantasmal mirror
    for (CHAR_DATA * mirrorCaster(ch->in_room->people); mirrorCaster != NULL; mirrorCaster = mirrorCaster->next_in_room)
    {
        AFFECT_DATA * phantasmalMirror(get_affect(mirrorCaster, gsn_phantasmalmirror));
        if (phantasmalMirror != NULL && phantasmalMirror->modifier > 0 && (ch == mirrorCaster || ch == phantasmalMirror->point))
            finishLearningPhantasm(mirrorCaster, phantasmalMirror);
    }

    free_string(ch->pose);

	// Stop verbs in case this screws up the character iteration
	verb_stop_issued = true;

	if (ch->pcdata != NULL)
	{
		ch->pcdata->purchased_item = NULL;
		ch->pcdata->purchased_item_cost = 0;
	}

    if (ch->mercy_to)
    {
	ch->mercy_to->mercy_from = NULL;
	ch->mercy_to = NULL;
    }

    if (ch->mercy_from)
    {
	ch->mercy_from->mercy_to = NULL;
	ch->mercy_from = NULL;
    }

    if (ch->fighting != NULL)
	stop_fighting_all(ch);

    remove_room_songs(ch->in_room, ch);

    if ( !IS_NPC(ch) )
    {
	--ch->in_room->area->nplayer;
    }

    if ( ch == ch->in_room->people )
    {
	ch->in_room->people = ch->next_in_room;
    }
    else
    {
	CHAR_DATA *prev;

	for ( prev = ch->in_room->people; prev; prev = prev->next_in_room )
	{
        if (prev->fighting == ch)
		stop_fighting(prev);
	    if ( prev->next_in_room == ch )
	    {
		prev->next_in_room = ch->next_in_room;
		break;
	    }
	}

	if ( prev == NULL )
	    bug( "Char_from_room: ch not found.", 0 );
    }

    if (IS_NAFFECTED(ch, AFF_GUARDING))
	affect_strip(ch, gsn_guard);

    if (is_affected(ch, gsn_whirlpool))
        affect_strip(ch, gsn_whirlpool);
    
    if (is_affected(ch, gsn_concealremains))
    {
        int x=1000;
        if (!IS_NPC(ch))
            for (x=0;x<MAX_FOCUS;x++)
            if (ch->pcdata->focus_on[x])
                if (*(skill_table[ch->pcdata->focus_sn[x]].pgsn) == gsn_concealremains)
                break;
        if (x < MAX_FOCUS)
            unfocus(ch,x,TRUE);
    }

    // Check for miasma of waning
    update_miasmaofwaning(ch->in_room);

    if (is_affected(ch, gsn_ready))
    {
        act("You stop readying your shot.",ch,NULL,NULL,TO_CHAR);
        act("$n stops readying a shot.",ch,NULL,NULL,TO_ROOM);
        affect_strip(ch,gsn_ready);
    }
  
    if (!global_bool_ranged_attack)
    {
	AFFECT_DATA *paf;

	if (!IS_NPC(ch))
	{
	    if (ch->pcdata->redit_name)
	    {
	        free_string(ch->pcdata->redit_name);
	        ch->pcdata->redit_name = str_dup("");
	    }

	    if (ch->pcdata->redit_desc)
	    {
	        free_string(ch->pcdata->redit_desc);
	        ch->pcdata->redit_desc = str_dup("");
	    }
	}

	if (IS_OAFFECTED(ch, AFF_CONSUMPTION))
	{
	    CHAR_DATA *nch, *nch_next;

	    act("The ritual is interrupted as $n disappears.", ch, NULL, NULL, TO_ROOM);
	    send_to_char("The ritual ends as you disappear.\n\r", ch);
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

	if (IS_OAFFECTED(ch, AFF_INSCRIBE))
	    affect_strip(ch, gsn_inscribe);

	if (is_affected(ch, gsn_wardoffire))
	{
	    send_to_char("Your ward of fire fades away.\n\r", ch);
	    act("$n's ward of fire fades away.", ch, NULL, NULL, TO_ROOM);
	    affect_strip(ch, gsn_wardoffire);
	}

    affect_strip(ch, gsn_bedrockroots);

	if (ch->in_room->vnum == 0)
	    ch->was_in_room = NULL;

	if (!ch->in_room->people)
	    for (paf = ch->in_room->affected; paf; paf = paf->next)
	    {
		if (paf->type == gsn_icyprison)
		{
		    destroy_icyprison(ch->in_room, get_room_index(paf->modifier));
		    break;
		}

		if ((paf->type == gsn_earthmaw)
		 || (paf->type == gsn_spirit_sanctuary))
		{

		    /** move all PC corpses out of room to prevent loss of stuph **/

		    OBJ_DATA *pObj, *obj_next;
		    ROOM_INDEX_DATA *tRoom;
		    
		    if ((tRoom = get_room_index(paf->modifier)) == FALSE)
			tRoom = get_room_index(ROOM_VNUM_TEMPLE);

		    for (pObj = ch->in_room->contents; pObj; pObj = obj_next)
		    {
			obj_next = pObj->next_content;

			if (pObj->item_type == ITEM_CORPSE_PC)
			{
			    obj_from_room(pObj);
			    obj_to_room(pObj, tRoom);
			}
		    }

		    free_room_area(ch->in_room);
		    break;
		}
	    }		    
    }
		
    ch->in_room      = NULL;
    ch->next_in_room = NULL;
    ch->on 	     = NULL;  /* sanity check! */

    if (!global_bool_ranged_attack)
    {
        if (global_linked_move)
        {
	    if (ch->rider && ch->rider->in_room)
	        char_from_room(ch->rider);
	    else if (ch->mount && ch->mount->in_room)
	        char_from_room(ch->mount);
        }
        else
	    unmount(ch);
    }
}

/*
 * Move a char into a room.
 */
void char_to_room( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *charmer;
    CHAR_DATA *vch;

    if ( pRoomIndex == NULL )
    {
        ROOM_INDEX_DATA *room;
        bug( "Char_to_room: NULL.", 0 );
        
        if ((room = get_room_index(ROOM_VNUM_TEMPLE)) != NULL)
            char_to_room(ch,room);
        
        return;
    }

    ch->in_room		= pRoomIndex;
    ch->next_in_room	= pRoomIndex->people;
    pRoomIndex->people	= ch;

    if ( !IS_NPC(ch) )
    {
        if (ch->in_room->area->empty)
        {
            ch->in_room->area->empty = FALSE;
            ch->in_room->area->age = 0;
        }
        ++ch->in_room->area->nplayer;
    }

    // Check stasis rift and bail out if sucked in
    if (check_stasisrift(*ch, *pRoomIndex)) 
        return;

    // Check flight
    if (is_flying(ch))
    {
        if (flight_blocked(*ch))
        {
            send_to_char("Unable to fly here, you carefully land and cease flight.\n", ch);
            act("$n carefully lands and ceases flight.", ch, NULL, NULL, TO_ROOM);
            stop_flying(*ch);
        }
    }
    else if (ON_GROUND(ch))
    {
        // Check shakestride
        AFFECT_DATA * paf(get_affect(ch, gsn_shakestride));
        if (paf != NULL && paf->modifier == 0)
            shake_ground(ch, *pRoomIndex, gsn_shakestride, 0, false);
    }

    // Basic checks
    update_miasmaofwaning(pRoomIndex);
    check_tuningstone(*ch, *pRoomIndex);
    check_displacement(ch);
    check_clingingfog(ch);

    if (should_trigger_corpsesense_sight(*pRoomIndex))
        check_corpsesense_sight(*ch);

    // Check cloudkill
    AFFECT_DATA *paf;
    if ((paf = affect_find(pRoomIndex->affected,gsn_cloudkill)) != NULL)
    {
        if (!IS_IMMORTAL(ch)
          && !IS_AFFECTED(ch,AFF_POISON)
          && !IS_OAFFECTED(ch,AFF_GHOST)
          && number_percent() < 50
          && (!IS_NPC(ch) || !IS_SET(ch->act, ACT_ILLUSION))
          && !saves_spell(paf->level,NULL, ch,DAM_POISON)
          && (!is_affected(ch, gsn_mistralward) || number_bits(1) == 0))
        {
            AFFECT_DATA naf = {0};
            naf.where = TO_AFFECTS;
            naf.type = gsn_cloudkill;
            naf.duration = 8;
            naf.valid = TRUE;
            naf.point = NULL;
            naf.location = 1;
            naf.modifier = -2;
            naf.bitvector = AFF_POISON;
            naf.level = paf->level;
            affect_to_char(ch,&naf);
            act("You choke as you breathe in the noxious fumes of the stinking cloud!",ch,NULL,NULL,TO_CHAR);
            act("$n chokes and gags, breathing in the noxious fumes of the stinking cloud!",ch,NULL,NULL,TO_ROOM);
        }
    }

    AFFECT_DATA * abode(get_affect(ch, gsn_abodeofthespirit));
    if (abode != NULL && abode->modifier == pRoomIndex->vnum)
        send_to_char("You feel a comfortable warmth fill you as you enter the abode of your spirit.\n", ch);

    // Check glacier's edge
    if (!IS_IMMORTAL(ch) && (!IS_NPC(ch) || ch->desc != NULL) && (paf = affect_find(pRoomIndex->affected, gsn_glaciersedge)) != NULL)
    {
        // Lookup the caster this edge is attuned to
        CHAR_DATA * caster(get_char_by_id(paf->modifier));
        if (caster != NULL && caster != ch)
            act_new("You sense $N cross your glacial line.", caster, NULL, ch, TO_CHAR, POS_SLEEPING);
    }

    // Room effects which only hit damageable mortals
    if (!IS_IMMORTAL(ch) && !IS_OAFFECTED(ch, AFF_GHOST) && (!IS_NPC(ch) || !IS_SET(ch->act, ACT_ILLUSION))) 
    {
        // Check mists of arcing
        paf = get_room_affect(pRoomIndex, gsn_mistsofarcing);
        if (paf != NULL)
            check_mistsofarcing(ch, paf->level);

        // Check hoarfrost
        if ((IS_NPC(ch) || ch->level > 10) && (paf = affect_find(pRoomIndex->affected, gsn_hoarfrost)) != NULL)
        {
            act("The ice coating this place glitters, and a wave of frost strikes you!", ch, NULL, NULL, TO_CHAR);
            act("The ice coating this place glitters, and a wave of frost strikes $n!", ch, NULL, NULL, TO_ROOM);
            check_encasecasting(paf->level / (paf->modifier == ch->id ? 2 : 1), NULL, ch);
        }

        // Check web of oame
        check_webofoame_catch(*ch, *pRoomIndex);

        // Check steam
        if ((paf = affect_find(pRoomIndex->affected, gsn_steam)) != NULL)
        {
            // Do damage and possibly lag
            int effectiveLevel(paf->level);
            if (ch->level < 25)
                effectiveLevel = UMIN(effectiveLevel, ch->level);

            int dam(10 + dice(effectiveLevel, 4));
            if (saves_spell(paf->level, NULL, ch, DAM_FIRE))
                dam /= 2;

            sourcelessDamage(ch, "the scalding steam", dam, gsn_steam, DAM_FIRE);

            if (!saves_spell(paf->level, NULL, ch, DAM_MENTAL))
            {
                send_to_char("You are briefly disoriented by the thick steam.\n", ch);
                WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE));
            }
        }

        // Check kaagn's plaguestone
        if (!IS_AFFECTED(ch, AFF_PLAGUE) && (paf = affect_find(pRoomIndex->affected, gsn_kaagnsplaguestone)) != NULL)
        {
            if (number_percent() <= get_skill(ch, gsn_stonecraft) || saves_spell(paf->level, NULL, ch, DAM_DISEASE))
                send_to_char("You feel nauseous as you enter, but shake it off.\n", ch);
            else
            {
                AFFECT_DATA caf = {0};
                caf.where   = TO_AFFECTS;
                caf.level   = paf->level;
                caf.type    = gsn_plague;
                caf.duration = caf.level;
                caf.location = APPLY_STR;
                caf.modifier = -5;
                caf.bitvector = AFF_PLAGUE;
                affect_join(ch, &caf);
                
                send_to_char("You feel sick as you enter, and boils and sores start to break out on your skin!\n", ch);
                act("$n screams in agony as plague sores erupt from $s skin.", ch, NULL, NULL, TO_ROOM);
            }
        }

        // Check contaminate
        if ((paf = affect_find(pRoomIndex->affected, gsn_contaminate)) != NULL && number_bits(1) == 0)
        {
            if (number_percent() <= get_skill(ch, gsn_maleficinsight))
                check_improve(ch, NULL, gsn_maleficinsight, true, 12);
            else
            {
                check_improve(ch, NULL, gsn_maleficinsight, false, 12);

                if (!saves_spell(paf->level, NULL, ch, DAM_NEGATIVE))
                {
                    // Strike the target with a random malediction
                    AFFECT_DATA caf = {0};
                    caf.where   = TO_AFFECTS;
                    caf.level   = paf->level;

                    bool appliedOne(false);
                    for (unsigned int i(0); i < 5 && !appliedOne; ++i)
                    {
                        switch (number_range(0, 2))
                        {
                            case 0: // Blindness
                                if (!IS_AFFECTED(ch, AFF_BLIND) && !IS_SET(ch->imm_flags, IMM_BLIND))
                                {
                                    caf.type = gsn_blindness;
                                    caf.duration = caf.level / 6;
                                    caf.bitvector = AFF_BLIND;
                                    caf.location = APPLY_HITROLL;
                                    caf.modifier = -4;
                                    affect_to_char(ch, &caf);
                                    send_to_char("As you cross the contaminated waters, darkness fills your eyes!\n", ch);
                                    act("$n's eyes cloud with darkness, and $e appears to be blinded.", ch, NULL, NULL, TO_ROOM);
                                    appliedOne = true;
                                }
                                break;

                            case 1: // Curse
                                if (!IS_AFFECTED(ch, AFF_CURSE))
                                {
                                    caf.type = gsn_curse;
                                    caf.duration = (caf.level / 2) + 10;
                                    caf.location = APPLY_HITROLL;
                                    caf.modifier = -1 * (caf.level / 8);
                                    caf.bitvector = AFF_CURSE;
                                    affect_to_char(ch, &caf);
                                    
                                    caf.location = APPLY_SAVING_SPELL;
                                    caf.modifier = caf.level / 8;
                                    affect_to_char(ch, &caf);

                                    send_to_char("As you cross the contaminated waters, a sense of impurity fills you.\n", ch);
                                    act("$n looks very uncomfortable.", ch, NULL, NULL, TO_ROOM);
                                    appliedOne = true;
                                }
                                break;

                            case 2: // Plague
                                if (!IS_AFFECTED(ch, AFF_PLAGUE))
                                {
                                    caf.type = gsn_plague;
                                    caf.duration = caf.level;
                                    caf.location = APPLY_STR;
                                    caf.modifier = -5;
                                    caf.level = (caf.level * 3) + 4;
                                    caf.bitvector = AFF_PLAGUE;
                                    affect_join(ch, &caf);
                                    
                                    send_to_char("As you cross the contaminated waters, boils and sores start to break out on your skin!\n", ch);
                                    act("$n screams in agony as plague sores erupt from $s skin.", ch, NULL, NULL, TO_ROOM);
                                    appliedOne = true;
                                }
                                break;
                        }
                    }
                }
            }
        }

        // Check heatmine (aka brimstone ward)
        if ((!IS_NPC(ch) || ch->pIndexData->vnum != MOB_VNUM_GENERALPURPOSE) && (paf = affect_find(pRoomIndex->affected, gsn_heatmine)) != NULL)
        {
            // Calculate damage
            int damLevel(paf->level);
            if (ch->level < 25) 
                damLevel = ch->level;

            int dam = number_range(damLevel * 2, damLevel * 5);
            if (paf->modifier == 1)
                dam = (dam * 4) / 3;

            if (saves_spell(paf->level, NULL, ch, DAM_FIRE))
                dam /= 2;

            if (number_percent() < get_skill(ch, gsn_firekin))
                dam /= 2;
            
            act("The smoldering ring of brimstone flickers, then suddenly blazes into a towering pillar of fire!", ch, NULL, NULL, TO_ROOM);
            act("An explosion of fire and force slams into you as you enter!", ch, NULL, NULL, TO_CHAR);
            sourcelessDamage(ch, "the brimstone ward", dam, gsn_heatmine, DAM_FIRE);

            // Calculate lag
            if ((number_percent() > get_skill(ch, gsn_firekin)) && (!saves_spell(paf->level, NULL, ch, DAM_BASH) || (is_flying(ch) && number_percent() < 50)))
            {
                act("$n is thrown backward by the concussive blast!", ch, NULL, NULL, TO_ROOM);
                act("You are thrown backward by the concussive blast!", ch, NULL, NULL, TO_CHAR);
                WAIT_STATE(ch, UMAX(ch->wait, (paf->modifier == 1 ? 3 : 2) * PULSE_VIOLENCE));
                switch_position(ch, POS_RESTING);
            }

            // Decrease duration
            if (paf->duration <= 2)
            {
                room_affect_strip(pRoomIndex, gsn_heatmine);
                if (pRoomIndex->people != NULL)
                {
                    act("The fiery pillar vanishes as swiftly as it appeared, and the ring of smoldering brimstone burns away completely, its power spent.", pRoomIndex->people, NULL, NULL, TO_CHAR);
                    act("The fiery pillar vanishes as swiftly as it appeared, and the ring of smoldering brimstone burns away completely, its power spent.", pRoomIndex->people, NULL, NULL, TO_ROOM);
                }
            }
            else
            {
                paf->duration = UMAX(1, paf->duration - number_range(2, 7));
                if (pRoomIndex->people != NULL)
                {
                    act("The fiery pillar vanishes as swiftly as it appeared, and the ring of smoldering brimstone burns lower.", pRoomIndex->people, NULL, NULL, TO_CHAR);
                    act("The fiery pillar vanishes as swiftly as it appeared, and the ring of smoldering brimstone burns lower.", pRoomIndex->people, NULL, NULL, TO_ROOM);
                }
            }
        }
    }

    if ((paf = get_affect(ch, gsn_conflagration)) != NULL && paf->modifier == 0) checkConflagration(ch);
    if ((paf = get_affect(ch, gsn_steam)) != NULL && paf->modifier == 0) checkSteam(ch);

    if ((paf = affect_find(pRoomIndex->affected,gsn_ringoffire)) != NULL)
    {
	if (!IS_IMMORTAL(ch)
	  && !IS_OAFFECTED(ch,AFF_GHOST))
	{
	    int dam = number_range(paf->level,paf->level*3);
	    if (saves_spell(paf->level,NULL,ch,DAM_FIRE))
		dam /= 2;
	    act("The ring of fire dissipates as $n passes through it.",ch,NULL,NULL,TO_ROOM);
	    act("The ring of fire dissipates as you pass through it.",ch,NULL,NULL,TO_CHAR);
	    room_affect_strip(ch->in_room,gsn_ringoffire);
	    damage(ch,NULL,dam,gsn_ringoffire,DAM_FIRE,TRUE);
	}
    }

    if (room_is_affected(pRoomIndex,gsn_freeze)
      && ch->class_num != global_int_class_watertemplar
      && ch->class_num != global_int_class_waterscholar
      && !is_flying(ch)
      && number_percent()<get_modifier(pRoomIndex->affected,gsn_freeze)
      && (!IS_NPC(ch) || !IS_SET(ch->act, ACT_ILLUSION)))
    {
	send_to_char("You slip on the frozen ground and go sprawling!\n\r",ch);
	act("$n slips on the frozen ground and goes sprawling!", ch, NULL,NULL,TO_ROOM);
	WAIT_STATE(ch, UMAX(ch->wait,PULSE_VIOLENCE*2));
    }
    
    if (ch->clan == clan_lookup("SHUNNED"))
    {
        if (ch->in_room->clan == clan_lookup("SHUNNED"))
	{
	    if (!is_affected(ch, gsn_shadow_ward))
	    {
	        AFFECT_DATA naf;
		naf.type = gsn_shadow_ward;
		naf.duration = -1;
		naf.valid = TRUE;
		naf.point = NULL;
		naf.where = TO_AFFECTS;
		naf.location = APPLY_NONE;
		naf.modifier = 0;
		naf.bitvector = 0;
		naf.level = 60;
		affect_to_char(ch, &naf);
		send_to_char("The wards of the Halls attune themselves to your presence.\n\r",ch);
	    }
	}
	else
	    if (is_affected(ch, gsn_shadow_ward))
	    {
	        affect_strip(ch, gsn_shadow_ward);
		send_to_char("The protective wards of the Halls recede.\n\r",ch);
            }
    }
    else
        if (is_affected(ch, gsn_shadow_ward))
	{
	    affect_strip(ch, gsn_shadow_ward);
	    send_to_char("The protective wards of the Halls recede.\n\r",ch);
	}

    add_room_songs(ch->in_room, ch);

    if (!global_bool_ranged_attack)
    {
    	if (is_affected(ch, gsn_setsnare) && (ch->class_num != global_int_class_ranger))
    	{
	    send_to_char("The snare falls away as you disappear.\n\r", ch);
	    affect_strip(ch, gsn_setsnare);
    	}

	if (is_affected(ch, gsn_quicksand))
	    affect_strip(ch, gsn_quicksand);

	if (is_affected(ch, gsn_camoublind))
	    for (vch = ch->in_room->people;vch;vch=vch->next_in_room)
	    {
		if (vch != ch)
		    send_to_char("You step out from the concealing blinds.\n\r", vch);
	    	affect_strip(vch, gsn_camoublind);
	    }
    }

    if (!IS_NPC(ch) && !IS_SET(ch->comm, COMM_BRIEF)
     && !global_bool_ranged_attack && (ch->in_room->vnum != 0)
     && !BIT_GET(ch->pcdata->travelptr, ch->in_room->vnum)
     && !IS_OAFFECTED(ch, AFF_GHOST))
    {
	int new_ep, new_xp;

        BIT_SET(ch->pcdata->travelptr, ch->in_room->vnum);
	new_ep = URANGE(0, UMAX(ch->in_room->danger, ch->in_room->area->danger), 10);
        ch->ep += new_ep;

// brazen: XP bonus for EP. Scales upward as the PC reaches higher plateaus 
	if (!new_ep) new_xp = 0;
	else
	{
	    if (ch->ep < EP_PLATEAU)
	        new_xp = ((ch->ep / 1000) + 1) * new_ep;
	    else    
		new_xp = (ch->ep / 1000)  * 2 * new_ep;
            gain_exp(ch, new_xp);
	}

	if (IS_SET(ch->act, PLR_EPNOTE))
	{
	    global_bool_sending_brief = TRUE;
	    sprintf(buf, "You gained %d exploration %s and %d experience %s!\n\r", new_ep, new_ep == 1 ? "point" : "points", new_xp, new_xp == 1 ? "point" :"points");
            send_to_char(buf, ch);
	    global_bool_sending_brief = FALSE;
	}
    }

    if (global_linked_move && !global_bool_ranged_attack)
    {
	if (ch->rider && !ch->rider->in_room)
	    char_to_room(ch->rider, pRoomIndex);
	else if (ch->mount && !ch->mount->in_room)
	    char_to_room(ch->mount, pRoomIndex);

	global_linked_move = FALSE;
    }

    if (is_affected(ch, gsn_lovepotion))
    {
	for (charmer = ch->in_room->people; charmer != NULL; charmer = charmer->next_in_room)
	{
		if (charmer == NULL) break;
		if (IS_NPC(charmer) || ABS(ch->level - charmer->level) > 8 || !can_see(ch, charmer)) continue;
		if ((IS_MALE(ch) && IS_FEMALE(charmer)) || (IS_FEMALE(ch) && IS_MALE(charmer)))
		{
			spell_charm_person(gsn_charm_person, 70, charmer, ch, 0);
			affect_strip(ch, gsn_lovepotion);
		}
	}
    }
}

// Give an obj to a char.
void obj_to_char(OBJ_DATA *obj, CHAR_DATA *ch)
{
    Encumbrance::ChangeNotifier notifier(*ch);

	// Catch-all to make sure they can have the item; if not, extract it and bug about it
	if (!IS_NPC(ch) && !IS_IMMORTAL(ch) && IS_OBJ_STAT_EXTRA(obj, ITEM_NOMORTAL))
	{
		std::ostringstream mess;
		mess << "NOMORTAL item '" << obj->name << "' showed up on mortal '" << ch->name << "'";
		bug(mess.str().c_str(), 0);
		extract_obj(obj);
		return;
	}
    if (IS_NPC(ch) && (ch->pIndexData->vnum == MOB_VNUM_MIRROR_IMAGE || ch->act & ACT_ILLUSION))
    {
        if (ch->in_room)
        {
            act("$p falls through $n, landing at $s feet.",ch,obj,NULL,TO_ROOM);
            obj_to_room(obj,ch->in_room);
            return;
        }
        else
            extract_obj(obj);
    }
    obj->next_content	 = ch->carrying;
    ch->carrying	 = obj;
    obj->carried_by	 = ch;
    obj->in_room	 = NULL;
    obj->in_obj		 = NULL;
    ch->carry_number	+= 1;
    ch->carry_weight	+= get_obj_weight( obj );
}

/*
 * Take an obj from its character.
 */
void obj_from_char( OBJ_DATA *obj )
{
    CHAR_DATA *ch;
    int i;
    bool next_last = FALSE;
    char *lpoint;

    if ( ( ch = obj->carried_by ) == NULL )
    {
	bug( "Obj_from_char: null ch.", 0 );
	return;
    }

    Encumbrance::ChangeNotifier notifier(*ch);
    
    if ((obj->lastowner[0][0] == '\0') || str_cmp(obj->lastowner[0], (IS_NPC(obj->carried_by) ? obj->carried_by->short_descr : obj->carried_by->name)))
    {
	for (i = 1; i < MAX_LASTOWNER; i++)
        { 
	    if ((obj->lastowner[i][0] != '\0') && !str_cmp(obj->lastowner[i], (IS_NPC(obj->carried_by) ? obj->carried_by->short_descr : obj->carried_by->name)))
	        next_last = TRUE;

	    lpoint = obj->lastowner[0];
	    obj->lastowner[0] = obj->lastowner[i];
	    obj->lastowner[i] = lpoint;

	    if (next_last)
	        break;
        }

        if (!next_last)
        {
            free_string(obj->lastowner[0]);
            obj->lastowner[0] = (IS_NPC(obj->carried_by) ? str_dup(obj->carried_by->short_descr) : str_dup(obj->carried_by->name));
        }
    }

    if (obj->worn_on) 
    {
	unequip_char( ch, obj );
	oprog_remove_trigger(obj);
    }

    object_affect_strip(obj, gsn_plant);

    // Clear out deathly visage
    AFFECT_DATA * visage(get_obj_affect(obj, gsn_deathlyvisage));
    if (visage != NULL)
    {
        act("$p's empty eye sockets lose their deep violet glow.", ch, obj, NULL, TO_ALL);
        affect_remove_obj(obj, visage);
    }

    if (ch->on == obj)
        ch->on = NULL;

    // If this was the last purchased item, clear it out
    if (ch->pcdata != 0 && ch->pcdata->purchased_item == obj)
    {
	ch->pcdata->purchased_item = NULL;
	ch->pcdata->purchased_item_cost = 0;
    }

    if ( ch->carrying == obj )
    {
	ch->carrying = obj->next_content;
    }
    else
    {
	OBJ_DATA *prev;

	for ( prev = ch->carrying; prev != NULL; prev = prev->next_content )
	{
	    if ( prev->next_content == obj )
	    {
		prev->next_content = obj->next_content;
		break;
	    }
	}

	if ( prev == NULL )
	    bug( "Obj_from_char: obj not in list.", 0 );
    }

    obj->carried_by 	= NULL;
    obj->next_content 	= NULL;
    ch->carry_number	-= 1;
    ch->carry_weight	-= get_obj_weight( obj );
}

/*
 * Find the ac value of an obj, including position effect.
 */
int apply_ac( OBJ_DATA *obj, int iWear, int type )
{
    if ( obj->item_type != ITEM_ARMOR )
	return 0;
    else
	return obj->value[type];
}

/*
 * Find a piece of eq on a character.
 *
 * Nov. 03/00: Routine is still using WEAR_ flags to maintain personal
 *             sanity.  -- Aeolis.
 *
 */
OBJ_DATA *get_eq_char(const CHAR_DATA *ch, int iWear )
{
    OBJ_DATA *obj;

    if (ch == NULL)
	return NULL;

    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
	if (IS_SET(obj->worn_on, (1 << iWear)))
	    return obj;

    return NULL;
}



/*
 * Equip a char with an obj.
 */

void equip_char( CHAR_DATA *ch, OBJ_DATA *obj, int iWear )
{
    Encumbrance::ChangeNotifier notifier(*ch);

    AFFECT_DATA *paf;
    int i, x;

    for (x = 0; x < MAX_WEAR; x++)
	if (IS_SET(iWear, (1 << x)) && get_eq_char(ch, x))
	    return;

    if ((obj->pIndexData->vnum == OBJ_VNUM_ICY_SHIELD)
     && (ch->class_num != global_int_class_watertemplar)
     && !IS_IMMORTAL(ch))
    {
	act("A severe chill courses through you as you attempt to wear $p.", ch, obj, NULL, TO_CHAR);
	spell_frostbite(gsn_frostbite, 99, ch, (void *) ch, TARGET_CHAR);
	act("$p drops from your chilled fingers.", ch, obj, NULL, TO_CHAR);
	act("$p drops from $n's chilled fingers.", ch, obj, NULL, TO_ROOM);
	obj_from_char(obj);
	obj_to_room(obj, ch->in_room);
	return;
    }

    // Only one Heart of the Inferno may be worn at a time
    if (obj_is_affected(obj, gsn_heartoftheinferno))
    {
         for (OBJ_DATA * heart = ch->carrying; heart != NULL; heart = heart->next_content)
         {
             if (heart->worn_on && obj_is_affected(heart, gsn_heartoftheinferno))
             {
                 act("You sense the energies stored in both Hearts conflict powerfully, forcing $p from your hands and to the ground!", ch, obj, NULL, TO_CHAR);
                 act("A flare of light flashes from $p, and $n drops it!", ch, obj, NULL, TO_ROOM);
                 obj_from_char(obj);
                 obj_to_room(obj, ch->in_room);
                 return;
             }
         }
    }

    // Only the creater may wield a Nightflame Lash
    AFFECT_DATA * lashAff(get_obj_affect(obj, gsn_nightflamelash));
    if (lashAff != NULL && lashAff->point != ch)
    {
        act("$p flickers and vanishes!", ch, obj, NULL, TO_CHAR);
        act("$p flickers and vanishes!", ch, obj, NULL, TO_ROOM);
        extract_obj(obj);
        return;
    }

    // Only a revenant may wield a bone reaper
    if ((obj->pIndexData->vnum == OBJ_VNUM_BONESCYTHE || obj->pIndexData->vnum == OBJ_VNUM_BONESICKLE) && get_skill(ch, gsn_revenant) < 1)
    {
        act("Thin tendrils of dark magic creep over $p as you grasp it, latching onto you.", ch, obj, NULL, TO_CHAR);
        act("Pain courses over your arms as the energy lances into them!", ch, obj, NULL, TO_CHAR);
        act("The pain grows too intense, forcing you to drop $p!", ch, obj, NULL, TO_CHAR);
        act("$n drops $p, gasping in pain.", ch, obj, NULL, TO_ROOM);
        obj_from_char(obj);
        obj_to_room(obj, ch->in_room);
        sourcelessDamage(ch, "the dark magic", number_range(2, 6), gsn_reaping, DAM_NEGATIVE);
        return;
    }

    if ((obj->pIndexData->vnum == OBJ_VNUM_NATURAL_ARMOR)
      && ch->class_num != global_int_class_druid
      && ch->class_num != global_int_class_ranger
      && !IS_IMMORTAL(ch))
    {
	act("Itchy nettles cover your skin as you attempt to wear $p.",ch, obj, NULL, TO_CHAR);
	spell_nettles(gsn_nettles, 99, ch, (void *) ch, TARGET_CHAR);
	act("$p drops from your hands.", ch, obj, NULL, TO_CHAR);
	act("$p drops from $n's hands.", ch, obj, NULL, TO_ROOM);
	obj_from_char(obj);
	obj_to_room(obj, ch->in_room);
	return;
    }

    if ((obj->pIndexData->vnum == OBJ_VNUM_LIGHT_WEAPON)
     &&  (ch->class_num != global_int_class_spirittemplar)
     && !IS_IMMORTAL(ch))
    {
        act("You are burned by $p.", ch, obj, NULL, TO_CHAR);
        spell_soulburn(130, 99, ch, (void *) ch, TARGET_CHAR);
        act("$n drops $p.", ch, obj, NULL, TO_ROOM);
        obj_from_char(obj);
        obj_to_room(obj, ch->in_room);
        return; 
    }

    if ((obj->pIndexData->vnum == OBJ_VNUM_MANTLE_EARTH)
     && (ch->class_num != global_int_class_earthtemplar)
     && !IS_IMMORTAL(ch))
    {
	act("$p turns to dust as you attempt to wear it.", ch, obj, NULL, TO_CHAR);
	act("$p turns to dust as $n attempts to wear it.", ch, obj, NULL, TO_ROOM);
	extract_obj(obj);
	return;
    }
    if ((obj->pIndexData->vnum == OBJ_VNUM_FIRE_SPEAR)
      && (ch->class_num != global_int_class_firetemplar)
      && !IS_IMMORTAL(ch))
    {
	act("The flame in $p burns out.",ch, obj, NULL, TO_CHAR);
	act("The flame in $p burns out.",ch, obj, NULL,TO_ROOM);
    }

    if ( ( IS_OBJ_STAT(obj, ITEM_ANTI_EVIL)    && IS_EVIL(ch)    )
    ||   ( IS_OBJ_STAT(obj, ITEM_ANTI_GOOD)    && IS_GOOD(ch)    )
    ||   ( IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch) ) 
    ||   (obj_is_affected(obj, gsn_dedication) && ch->clan != clan_lookup("CONQUEST"))
    ||   (obj_is_affected(obj, gsn_holyavenger) && ch->clan != clan_lookup("CHAMPION"))
    ||   (obj->pIndexData->vnum == OBJ_VNUM_RAIDER_BELT && ch->clan != clan_lookup("RAIDER"))
    ||   (obj_is_affected(obj, gsn_reaver_bind) && (get_modifier(obj->affected, gsn_reaver_bind) != ch->id))
    ||   (obj_is_affected(obj, gsn_demon_bind) && (get_modifier(obj->affected, gsn_demon_bind) != ch->id)))
    {
	/*
	 * Thanks to Morgenes for the bug fix here!
	 */
	act( "You are zapped by $p and drop it.", ch, obj, NULL, TO_CHAR );
	act( "$n is zapped by $p and drops it.",  ch, obj, NULL, TO_ROOM );
	obj_from_char( obj );
	obj_to_room( obj, ch->in_room );
	return;
    }

    if (obj->wear_flags & ITEM_WIELD && iWear != WORN_CONCEAL1
     && iWear != WORN_CONCEAL2 && iWear != WORN_FLOAT
     && !hands_free(ch))
	return;

    for (i = 0; i < 4; i++)
    	ch->armor[i]      	-= apply_ac(obj, iWear, i);

    obj->worn_on = iWear;

    if (!IS_SET(iWear, WORN_CONCEAL1) && !IS_SET(iWear, WORN_CONCEAL2))
    {
	if (!obj->enchanted)
	    for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
	        if ( paf->location != APPLY_SPELL_AFFECT )
	            affect_modify( ch, paf, TRUE, TRUE);

        for ( paf = obj->affected; paf != NULL; paf = paf->next )
		    if ( paf->location == APPLY_SPELL_AFFECT )
    	        affect_to_char(ch, paf);
		    else
	    	    affect_modify(ch, paf, TRUE, TRUE);
    }
}

OBJ_DATA *get_foci( CHAR_DATA *ch, bool world )
{
    OBJ_DATA *obj;
    AFFECT_DATA *paf;

    if (!ch || IS_NPC(ch))
	return NULL;

    if (!world)
    {
        for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
        {
            for (paf = get_obj_affect(obj, gsn_createfoci); paf != NULL; paf = get_obj_affect(obj, gsn_createfoci, paf))
            {
		        if (paf->modifier == ch->id && paf->location == APPLY_HIDE)
	                return obj;
            }
        }

        return NULL;
    }

    for (obj = object_list; obj != NULL; obj = obj->next)
    {
        for (paf = get_obj_affect(obj, gsn_createfoci); paf != NULL; paf = get_obj_affect(obj, gsn_createfoci, paf))
        {
            if (paf->modifier == ch->id && paf->location == APPLY_HIDE)
                return obj;
        }
    }

    return NULL;
}

CHAR_DATA *get_affinity_owner(OBJ_DATA *obj)
{
int x;
CHAR_DATA *vch, *vch_next;

	if (!obj_is_affected(obj, gsn_affinity))
		return NULL;

	x = get_modifier(obj->affected, gsn_affinity);

	for (vch = char_list;vch != NULL;vch = vch_next)
	{
	  vch_next = vch->next;
	  if (is_affected(vch, gsn_affinity) && get_modifier(vch->affected, gsn_affinity) == x)
	    return vch;
	}

	/* char not found. Char therefore not in world. Remove affinity */
	object_affect_strip(obj, gsn_affinity);

	return NULL;
}



/* Get scouter -- to trigger scouting at scan distances */
CHAR_DATA *get_scouter(CHAR_DATA *ch, ROOM_INDEX_DATA *room, CHAR_DATA *last_scout)
{
    CHAR_DATA *vch;
    ROOM_INDEX_DATA *cur;
    EXIT_DATA *pexit;
    int x, y, j;
    int ns = 0;
    bool last_found = (last_scout ? FALSE : TRUE);
    int scanned[100];
    int maxscan;
    
    if (room == NULL || ch == NULL)
	return NULL;

    /* This entire function was lame-ass code.  Fixed by Aeolis. */

    if (area_is_affected(ch->in_room->area, gsn_icestorm))
	return NULL;

    if (room_is_affected(ch->in_room, gsn_smoke))
	return NULL;

    for (x = 0; x < 6; x++)
    {
	cur = room;
	for (y = 0; y < 8; y++)
	{
    	    if ((cur->exit[x] != NULL)
	     && ((pexit = cur->exit[x]) != NULL)
	     && (!(pexit->exit_info & EX_CLOSED
	      || pexit->exit_info & EX_WALLED
	      || pexit->exit_info & EX_WALLOFFIRE
	      || pexit->exit_info & EX_SECRET
	      || room_is_affected(room, gsn_smoke)))
	     && ((cur = cur->exit[x]->u1.to_room) != NULL))
    	    {
		/* This is a fairly significant hack to prevent looping    */
		/* rooms from crashing us.  The drawback is that scouting  */
		/* will be seriously screwed up in highly-randomized       */
		/* areas.  -- Aeolis                                       */

		for (j = 0; j < ns; j++)
		    if (scanned[j] == cur->vnum)
			return NULL;

		for (vch = cur->people; vch != NULL; vch = vch->next_in_room)
	  	    if (IS_NAFFECTED(vch, AFF_SCOUTING))
	  	    {
			if (last_found)
			{
    			    maxscan = 2;
			    if (!IS_NPC(vch) && BIT_GET(vch->pcdata->traits, TRAIT_EAGLE_EYED))
				maxscan += 2;
			    if (is_affected(vch,gsn_eagleeyes))
				maxscan += 3;
			    if (vch->race == global_int_race_alatharya)
				maxscan += 1;
			    if (number_percent() < get_skill(vch,gsn_farsight))
				maxscan += 1;
    			    maxscan = UMIN(maxscan,(vch->level+9)/8);
	    		    ch->scoutdir = x;
			    ch->scoutrooms = y <= maxscan ? y + 1 : 0;
			    if (vch != ch)
				return vch;
	  	        }
			else if (vch == last_scout)
			    last_found = TRUE;
		    }
	    }
	    else break;

	    scanned[ns] = cur->vnum;
	    ns++;
	}
    }

    return NULL;
}



CHAR_DATA *get_conduit(CHAR_DATA *ch, ROOM_INDEX_DATA *room, int distance)
{
  CHAR_DATA *vch;
  int x;

  if (room == NULL || ch == NULL)
	return NULL;

  for (vch = room->people; vch != NULL; vch = vch->next_in_room)
  {
     if (is_affected(vch, gsn_manaconduit) &&
	(get_modifier(ch->affected, gsn_manaconduit) == get_modifier(vch->affected, gsn_manaconduit)) && ch != vch)
	  return vch;
  }

  if (distance > 2)
	return NULL;

  for (x = 0;x < 6;x++)
  {
    if (room->exit[x] != NULL && room->exit[x]->u1.to_room != NULL)
    {
      if ((vch = get_conduit(ch, room->exit[x]->u1.to_room, distance+1)) != NULL)
	return vch;
    }
  }

  return NULL;
}

  
CHAR_DATA *get_char_by_id(long id)
{
    if (id == 0)
        return NULL;

    for (DESCRIPTOR_DATA * d = descriptor_list; d; d = d->next)
    {
        CHAR_DATA * ch = d->original ? d->original : d->character;
        if (ch && (ch->id == id))
            return ch;
    }

    return NULL;
}

/*
 * Unequip a char with an obj.
 */
void unequip_char( CHAR_DATA *ch, OBJ_DATA *obj )
{
    Encumbrance::ChangeNotifier notifier(*ch);

    AFFECT_DATA *paf = NULL;
    AFFECT_DATA *lpaf = NULL;
    AFFECT_DATA *lpaf_next = NULL;
    int i;

    if (!obj->worn_on)
    {
	bug( "Unequip_char: already unequipped.", 0 );
	return;
    }

    if ((obj->item_type == ITEM_INSTRUMENT) && ch->song)
    {
	char buf[MAX_STRING_LENGTH];

	if (ch->harmony)
	{
	    sprintf(buf, "You stop playing '%s'.\n\r", skill_table[ch->harmony->type].name);
	    send_to_char(buf, ch);
	    act("$n's song ends.", ch, NULL, NULL, TO_ROOM);
	    stop_playing_song(ch, ch->harmony);
	}
	
	for (i = 0; song_table[i].song_fun; i++)
	    if (*song_table[i].sn == ch->song->type)
	    {
		if (song_table[i].inst_type != INST_TYPE_VOICE)
		{
		    sprintf(buf, "You stop playing '%s'.\n\r", skill_table[ch->song->type].name);
		    send_to_char(buf, ch);
		    act("$n's song ends.", ch, NULL, NULL, TO_ROOM);
		    stop_playing_song(ch, NULL);
		}

		break;
	    }
    }

    if (obj_is_affected(obj, gsn_bindweapon))
    {
	act("Your binding to $p tears away as it leaves your grasp.", ch, obj, NULL, TO_CHAR);
	object_affect_strip(obj, gsn_bindweapon);
    }

    for (i = 0; i < 4; i++)
    	ch->armor[i]	+= apply_ac(obj, obj->wear_loc, i);

    if (!IS_SET(obj->worn_on, WORN_CONCEAL1) && !IS_SET(obj->worn_on, WORN_CONCEAL2))
    {
    	if (!obj->enchanted)
	    for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
	        if ( paf->location == APPLY_SPELL_AFFECT )
	        {
	            for ( lpaf = ch->affected; lpaf != NULL; lpaf = lpaf_next )
	            {
		        lpaf_next = lpaf->next;
		        if ((lpaf->type == paf->type) &&
		            (lpaf->level == paf->level) &&
		            (lpaf->location == APPLY_SPELL_AFFECT))
		        {
		            affect_remove( ch, lpaf );
			    lpaf_next = NULL;
		        } 
	            }
	        }
	        else
	        {
	            affect_modify( ch, paf, FALSE, TRUE);
		    affect_check(ch,paf->where,paf->bitvector);
	        }

        for ( paf = obj->affected; paf != NULL; paf = paf->next )
	    if ( paf->location == APPLY_SPELL_AFFECT )
	    {
	        bug ( "Norm-Apply: %d", 0 );
	        for ( lpaf = ch->affected; lpaf != NULL; lpaf = lpaf_next )
	        {
		    lpaf_next = lpaf->next;
		    if ((lpaf->type == paf->type) &&
		        (lpaf->level == paf->level) &&
		        (lpaf->location == APPLY_SPELL_AFFECT))
		    {
		        bug ( "location = %d", lpaf->location );
		        bug ( "type = %d", lpaf->type );
		        affect_remove( ch, lpaf );
		        lpaf_next = NULL;
		    }
	        }
	    }
	    else
	    {
	        affect_modify( ch, paf, FALSE, TRUE);
	        affect_check(ch,paf->where,paf->bitvector);	
	    }
    }
    if (obj->item_type == ITEM_BOW)
    {
	if (ch->nocked != NULL)
	{
	    act("You relax your draw on $p.",ch,ch->nocked,NULL,TO_CHAR);
	    act("$n relaxes $s draw on $p.",ch,ch->nocked,NULL,TO_ROOM);
	    ch->nocked = NULL;
	}
	if (is_affected(ch,gsn_ready))
	{
	    act("You stop readying a shot.",ch,NULL,NULL,TO_CHAR);
	    act("$n stops readying a shot.",ch,NULL,NULL,TO_ROOM);
	    affect_strip(ch,gsn_ready);
	}
    }
	
    if (obj->item_type == ITEM_WEAPON 
      && IS_PAFFECTED(ch,AFF_TWOHAND)
      && IS_SET(obj->worn_on,WORN_WIELD))
	REMOVE_BIT(ch->paffected_by,AFF_TWOHAND);
    obj->worn_on = 0;
}

/*
 * Count occurrences of an obj in a list.
 */
int count_obj_list( OBJ_INDEX_DATA *pObjIndex, OBJ_DATA *list )
{
    OBJ_DATA *obj;
    int nMatch;

    nMatch = 0;
    for ( obj = list; obj != NULL; obj = obj->next_content )
    {
	if ( obj->pIndexData == pObjIndex )
	    nMatch++;
    }

    return nMatch;
}



/*
 * Move an obj out of a room.
 */
void obj_from_room( OBJ_DATA *obj )
{
    ROOM_INDEX_DATA *in_room;
    CHAR_DATA *ch;

    if ( ( in_room = obj->in_room ) == NULL )
    {
	bug( "obj_from_room: NULL.", 0 );
	return;
    }

	// Stop verbs in case this screws up prog iteration
	verb_stop_issued = true;

    for (ch = in_room->people; ch != NULL; ch = ch->next_in_room)
	if (ch->on == obj)
	    ch->on = NULL;

    if ( obj == in_room->contents )
    {
	in_room->contents = obj->next_content;
    }
    else
    {
	OBJ_DATA *prev;

	for ( prev = in_room->contents; prev; prev = prev->next_content )
	{
	    if ( prev->next_content == obj )
	    {
		prev->next_content = obj->next_content;
		break;
	    }
	}

	if ( prev == NULL )
	{
	    bug( "Obj_from_room: obj not found.", 0 );
	    return;
	}
    }
    obj->in_room      = NULL;
    obj->next_content = NULL;
}

// Move an obj into a room.
void obj_to_room( OBJ_DATA *obj, ROOM_INDEX_DATA *pRoomIndex )
{
    // Check meltdrop
    if (IS_OBJ_STAT(obj, ITEM_MELT_DROP))
    {
        if (obj->pIndexData->vnum == OBJ_VNUM_ICY_SHIELD)
            act("$p melts into a puddle and disappears.", pRoomIndex->people, obj, NULL, TO_ALL);
        else
            act("$p dissolves into smoke.", pRoomIndex->people, obj, NULL, TO_ALL);

        extract_obj(obj);
        return;
    }

    // Check stasisrift
    AFFECT_DATA * paf(get_room_affect(pRoomIndex, gsn_stasisrift));
    if (paf != NULL && paf->modifier != 0)
    {
        act("$p falls away through the void, vanishing swiftly.", pRoomIndex->people, obj, NULL, TO_ALL);
        extract_obj(obj);
        return;
    }

    // Move the object
    obj->next_content		= pRoomIndex->contents;
    pRoomIndex->contents	= obj;
    obj->in_room		= pRoomIndex;
    obj->carried_by		= NULL;
    obj->in_obj			= NULL;
}

/*
 * Move an object into an object.
 */
void obj_to_obj( OBJ_DATA *obj, OBJ_DATA *obj_to )
{
    obj->next_content		= obj_to->contains;
    obj_to->contains		= obj;
    obj->in_obj			= obj_to;
    obj->in_room		= NULL;
    obj->carried_by		= NULL;

    for ( ; obj_to != NULL; obj_to = obj_to->in_obj )
    {
        if ( obj_to->carried_by != NULL )
            obj_to->carried_by->carry_weight += (get_obj_weight(obj) * WEIGHT_MULT(obj_to)) / 100;
    }
}

/*
 * Move an object out of an object.
 */
void obj_from_obj( OBJ_DATA *obj )
{
    OBJ_DATA *obj_from;

    if ( ( obj_from = obj->in_obj ) == NULL )
    {
	bug( "Obj_from_obj: null obj_from.", 0 );
	return;
    }

    if ( obj == obj_from->contains )
    {
	obj_from->contains = obj->next_content;
    }
    else
    {
	OBJ_DATA *prev;

	for ( prev = obj_from->contains; prev; prev = prev->next_content )
	{
	    if ( prev->next_content == obj )
	    {
		prev->next_content = obj->next_content;
		break;
	    }
	}

	if ( prev == NULL )
	{
	    bug( "Obj_from_obj: obj not found.", 0 );
	    return;
	}
    }

    obj->next_content = NULL;
    obj->in_obj       = NULL;

    for ( ; obj_from != NULL; obj_from = obj_from->in_obj )
    {
	if ( obj_from->carried_by != NULL )
	{
	   /* obj_from->carried_by->carry_number -= get_obj_number( obj ); */
	    obj_from->carried_by->carry_weight -= get_obj_weight( obj ) 
		* WEIGHT_MULT(obj_from) / 100;
	}
    }

    return;
}



/*
 * Extract an obj from the world.
 */
void extract_obj( OBJ_DATA *obj )
{
    OBJ_DATA *obj_content;
    OBJ_DATA *obj_next;
    OBJ_DATA *obj_rand;

    ROOM_INDEX_DATA *oldroom(NULL);

    if ( obj->in_room != NULL )
    {
        // Handle posterns
        bool anyPostern(false);
        for (AFFECT_DATA * paf(get_obj_affect(obj, gsn_forgepostern)); paf != NULL; paf = get_obj_affect(obj, gsn_forgepostern, paf))
        {
            anyPostern = true;
            handle_remove_postern(obj, find_posterns_for(paf->modifier));
        }

        if (anyPostern && obj->in_room->people != NULL)
            act("$p shimmers faintly, then dissolves into tiny motes of silverly light which slowly fade away.", obj->in_room->people, obj, NULL, TO_ALL);

        // Cache the old room and move the obj from it
	    oldroom=obj->in_room;
    	obj_from_room( obj );

        // Check for flame unity
        for (CHAR_DATA * ch(oldroom->people); ch != NULL; ch = ch->next_in_room)
        {
            AFFECT_DATA * paf(get_affect(ch, gsn_flameunity));
            if (paf != NULL && paf->point == obj)
            {
                affect_strip(ch, gsn_flameunity);
                act("You stir, recorporealizing from the flames.", ch, NULL, NULL, TO_CHAR);
                act("$n emerges from the flames, unharmed despite the ash which swirls about $m.", ch, NULL, NULL, TO_ROOM);
            }
        }

        // Check for bloodpyre
        CHAR_DATA * ch(findCharForPyre(obj));
        if (ch != NULL)
        {
            AFFECT_DATA * paf(get_affect(ch, gsn_bloodpyre));
            if (paf == NULL)
                bug("Found char for pyre, but pyre affect is NULL", 0);
            else
            {
                paf->valid = false;
                delete static_cast<PyreInfo*>(paf->point);
                paf->point = NULL;
            }
            send_to_char("You feel your link to the Inferno dissolve into nothingness.\n", ch);
        }
    }
    else if ( obj->carried_by != NULL )
	obj_from_char( obj );
    else if ( obj->in_obj != NULL )
	obj_from_obj( obj );

    for ( obj_content = obj->contains; obj_content; obj_content = obj_next )
    {
	obj_next = obj_content->next_content;
	extract_obj( obj_content );
    }

    if ( object_list == obj )
    {
	object_list = obj->next;
    }
    else
    {
	OBJ_DATA *prev;

	for ( prev = object_list; prev != NULL; prev = prev->next )
	{
	    if ( prev->next == obj )
	    {
		prev->next = obj->next;
		break;
	    }
	}

	if ( prev == NULL )
	{
	    bug( "Extract_obj: obj %d not found.", obj->pIndexData->vnum );
	    return;
	}
    }

    if (!loading_char || (obj->pIndexData->item_type == ITEM_KEY) || IS_SET(obj->extra_flags[0], ITEM_QUEST))
	--obj->pIndexData->current;

    --obj->pIndexData->count;

    if (IS_SET(obj->pIndexData->progtypes, RAND_PROG))
    {
        if (obj == obj_rand_first)
	    obj_rand_first = obj->next_rand;
        else
        {
            for (obj_rand = obj_rand_first; obj_rand; obj_rand = obj_rand->next_rand)
	        if (obj_rand->next_rand == obj)
	        { 
	            obj_rand->next_rand = obj->next_rand;
		    break;
	        }
        }
    }

    free_obj(obj);
    return;
}



/*
 * Extract a char from the world.
 */
void extract_char( CHAR_DATA *ch, bool fPull, ROOM_INDEX_DATA * room)
{
    CHAR_DATA *wch, *wch_next;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    DESCRIPTOR_DATA *d;
    char buf[MAX_STRING_LENGTH];
    int i;
    
	ch->guarding = NULL;
	ch->nocked = NULL;

    // Clean out disguises
    affect_strip(ch, gsn_alterself);
    affect_strip(ch, gsn_hooded);
    affect_strip(ch, gsn_disguise);
    affect_strip(ch, gsn_chameleon);
    affect_strip(ch, gsn_rearrange);
    affect_strip(ch, gsn_englamour);

    // Strip out blood pyre
    AFFECT_DATA * bloodPyreAff(get_affect(ch, gsn_bloodpyre));
    if (bloodPyreAff != NULL)
    {
        if (bloodPyreAff->point != NULL)
        {
            OBJ_DATA * pyre(static_cast<PyreInfo*>(bloodPyreAff->point)->pyre());
            if (verify_obj_world(pyre))
            {
                if (pyre->in_room != NULL && pyre->in_room->people != NULL)
                {
                    act("$p slowly burns out, leaving nothing behind, not even ash.", pyre->in_room->people, pyre, NULL, TO_CHAR);
                    act("$p slowly burns out, leaving nothing behind, not even ash.", pyre->in_room->people, pyre, NULL, TO_ROOM);
                }
                extract_obj(pyre);
            }
           
            // Make sure the effect is gone 
            bloodPyreAff->valid = false;
            delete static_cast<PyreInfo*>(bloodPyreAff->point);
            bloodPyreAff->point = NULL;
        }
    }

    // Strip out spirit bond and bond of souls
    struct BondStripper
    {
        static void Strip(CHAR_DATA * ch, int sn)
        {
            if (!is_affected(ch, sn))
                return;
          
        	int mod = get_high_modifier(ch->affected, sn);
            CHAR_DATA * vch_next;
            for (CHAR_DATA * vch = char_list; vch != NULL; vch = vch_next)
            {     
                vch_next = vch->next;
        	    if (vch->id == mod)
                {
                    send_to_char("You feel your spirit bond fade away.\n\r", vch);
                    affect_strip(vch, sn);
                }
            }
            affect_strip(ch, sn);
        }
    };

    BondStripper::Strip(ch, gsn_spiritbond);
    BondStripper::Strip(ch, gsn_bondofsouls);

    /* psionic focus stuff */

    if (!IS_NPC(ch))
    {
        while (ch->pcdata->focus_sn[0] > 0)
	    if (ch->pcdata->focus_ch[0] == ch)
		unfocus(ch, 0, FALSE);
	    else
	        unfocus(ch, 0, TRUE);
    }
	
    for (d = descriptor_list; d; d = d->next)
    {
	if (d->character && d->character->pcdata)
            for (i = 0; i < MAX_FOCUS; i++)
  	        while (d->character->pcdata->focus_ch[i] == ch)
	        {
		    sprintf(buf, "Your %s link to $N breaks.", skill_table[d->character->pcdata->focus_sn[i]].name);
	 	    act(buf, d->character, NULL, ch, TO_CHAR);
	            unfocus(d->character, i, FALSE);
	        }

	if (d->character && d->character->mindlink == ch)
	{
	    act("Your mindlink to $N breaks.", d->character, NULL, ch, TO_CHAR);
	    d->character->mindlink = NULL;
	}
    }

    if (is_affected(ch, gsn_runeofeyes))
        affect_strip(ch, gsn_runeofeyes);
    
    if (fPull)
    {
        // Wipe out references to the ch
        nuke_pets(ch);
        ch->pet = NULL; /* just in case */
        ch->familiar = NULL; /* same thing */
        die_follower(ch, true);
    }
    else
    {
        // Pets and familiars will stop fighting upon master's death
        if (ch->pet != NULL) stop_fighting_all(ch->pet);
        if (ch->familiar != NULL) stop_fighting_all(ch->familiar);
    }
    
    stop_fighting_all(ch);

    if (!IS_NPC(ch))
      loading_char = TRUE;
    for ( obj = ch->carrying; obj != NULL; obj = obj_next )
    {
	obj_next = obj->next_content;
	if (!IS_SET(obj->wear_flags, ITEM_WEAR_SIGIL) && !obj_is_affected(obj, gsn_affinity)
	 && !IS_SET(obj->wear_flags, ITEM_PROG))
	   extract_obj( obj );
	else if (IS_SET(obj->wear_flags, ITEM_WEAR_SIGIL))
	   equip_char(ch, obj, WORN_SIGIL);
	else if (IS_SET(obj->wear_flags, ITEM_PROG))
	   equip_char(ch, obj, WORN_PROGSLOT);
    }
    if (!IS_NPC(ch))
      loading_char = FALSE;
   
    if (ch->in_room != NULL)
        char_from_room( ch );

    stop_playing_song(ch, NULL);

    /* Death room is set in the clan tabe now */
    if ( !fPull )
    {
        if (room != NULL) char_to_room(ch, room);
        else if (ch->recall_to) char_to_room(ch, ch->recall_to);
        else char_to_room(ch,get_room_index(ROOM_VNUM_TEMPLE));
        return;
    }

    if ( IS_NPC(ch))
        --ch->pIndexData->count;

    if (IS_NPC(ch) && ch->desc && (ch->pIndexData->vnum == MOB_VNUM_SHUNNED_DEMON))
    	unbind_shunned_demon(ch);

    if (ch->desc)
    {
        if (!IS_IMMORTAL(ch))
            add_hostdata(ch->desc);

        if (ch->desc->original)
        {
            do_return(ch, "");
            ch->desc = NULL;
        }
    }

    for ( wch = char_list; wch != NULL; wch = wch_next )
    {
        wch_next = wch->next;
        
        if (wch->reply == ch) wch->reply = NULL;
        if (wch->oocreply == ch) wch->oocreply = NULL;
        if (wch->tracking == ch) wch->tracking = NULL;

        if (IS_NPC(wch) && (wch->leader == ch)
        && ((wch->pIndexData->vnum == MOB_VNUM_MIRROR_IMAGE) || is_affected(wch, gsn_demoniccontrol)))
        {
            if (wch->pIndexData->vnum == MOB_VNUM_GREATER_TASKMASTER)
            {
                int numdemons = 0;
                CHAR_DATA *vch, *demon = NULL;
                for (vch = char_list; vch; vch = vch->next)
                {
                    if (IS_NPC(vch)
                      && (vch->leader == ch->master)
                      && (vch != ch)
                      && (is_affected(vch, gsn_demoniccontrol)))
                    {
                        numdemons++;
                        if (!demon || (demon->level < vch->level))
                            demon = vch;
                    }
                }
                if (demon && (numdemons > 3))
                {
                    act("You feel your control over $N fade.", ch->leader, NULL, demon, TO_CHAR);
                    demon->tracking = vch->master;
                    demon->demontrack = vch->master;
                    if (demon->master->in_room == demon->in_room)
                    {
                        set_fighting(demon, demon->master);
                        act("$n screams in fury and attacks!", demon, NULL, NULL, TO_ROOM);
                        multi_hit(demon, demon->master, TYPE_UNDEFINED);
                    }
                    stop_follower(demon);
                }
            }
            extract_char(wch, TRUE);
        }

        if (IS_OAFFECTED(wch, AFF_DOPPEL) && wch->memfocus[DOPPEL_SLOT] == ch)
        {
            setName(*wch, wch->pIndexData->player_name);
            free_string(wch->short_descr);
            wch->short_descr = str_dup(wch->pIndexData->short_descr);
            free_string(wch->description);
            wch->description = str_dup(wch->pIndexData->description);
            if (!IS_NPC(ch))
                REMOVE_BIT(wch->nact, ACT_CLASSED);
            affect_strip(wch, gsn_doppelganger);
        }
    }

    if (ch == ol_char)
        ol_char = NULL;
    else
    {
        if ( ch == char_list )
            char_list = ch->next;
        else
        {
            CHAR_DATA *prev;

            for ( prev = char_list; prev != NULL; prev = prev->next )
            {
                if ( prev->next == ch )
                {
                    prev->next = ch->next;
                    break;
                }
            }

            if ( prev == NULL )
            {
                bug( "Extract_char: char not found.", 0 );
                return;
            }
        }
    }

    if ((d = ch->desc) != NULL )
        ch->desc->character = NULL;
    free_char( ch );
    
    if (d)
        close_socket(d);
}

/*
 * Find a char in the room.
 */
CHAR_DATA *get_char_room( CHAR_DATA *ch, const char *argument ) {return get_char_room(ch, ch->in_room, argument);}
CHAR_DATA *get_char_room(CHAR_DATA *ch, ROOM_INDEX_DATA * room, const char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *rch, *vch, *possrch = NULL;
    bool ambiguous = FALSE;
    int number;
    int count;

    number = number_argument(argument, arg);
    if (!str_cmp( arg, "self" ))
	    return ch;

    if (room == NULL || room->people == NULL)
        return NULL;

    if (strlen(arg) < 3) ambiguous = TRUE; //need at least 3 letters for name-completion
    count = 0;

    // Check for masquerade
    if (!IS_NPC(ch) && room != NULL)
    {
        AFFECT_DATA * masquerade(get_area_affect(room->area, gsn_masquerade));
        if (masquerade != NULL && masquerade->modifier != ch->id && number_percent() > get_skill(ch, gsn_disillusionment))
        {
            // Masquerade applies; only "skull" is a valid name
            if (str_prefix(arg, "skull"))
                return NULL;

            // Iterate up to the number
            for (vch = room->people; vch != NULL; vch = vch->next_in_room)
            {
                if (can_see(ch, vch))
                {
                    if (number == 1)
                        return vch;

                    --number;
                }
            }

            // Did not find the char
            return NULL;
        }
    }

    if ((is_affected(ch, gsn_confusion) && number_bits(2) == 1) || is_affected(ch, gsn_moonray))
    {
        for (vch = room->people; vch; vch = vch->next_in_room)
        {
            if (can_see(ch, vch))
               count++;
        }

        for (vch = room->people; vch; vch = vch->next_in_room)
        {
            if (can_see(ch, vch))
            {
               if (number_range(1, count) == 1)
                    return vch;

                --count;
            }
        }
    }
    else
    {
        if (!IS_NPC(ch) || (ch->master && !IS_NPC(ch->master)))
        {
            for ( rch = room->people; rch != NULL; rch = rch->next_in_room )
            {
                if (!can_see( ch, rch ))
                     continue;

                if (!str_cmp(arg, rch->unique_name))
                    return rch;

                if (!ambiguous && (!str_prefix(arg, rch->unique_name) || (IS_NPC(rch) && !str_infix(arg, rch->name))))
                {
                    if (possrch == NULL) possrch = rch;
                    else ambiguous = TRUE;
                }		
            
                if (IS_OAFFECTED(rch, AFF_DISGUISE) ?
                    (!is_name( arg, rch->fake_name) && arg[0] != '\0') :
                    (!is_name( arg, rch->name ) &&  (arg[0] != '\0')))
                    continue;
        
                if ( ++count == number )
                    return rch;
            }
        }
        else
        {
            for ( rch = room->people; rch != NULL; rch = rch->next_in_room )
            {
                if (!can_see( ch, rch ))
                        continue;
         
                if (!str_cmp(arg, rch->unique_name))
                    return rch;

                if (!ambiguous && (!str_prefix(arg, rch->unique_name) || (IS_NPC(rch) && !str_infix(arg, rch->name))))
                {
                    if (possrch == NULL) possrch = rch;
                    else ambiguous = TRUE;
                }
                
                if ((!is_name( arg, rch->fake_name) && arg[0] != '\0') && (!is_name( arg, rch->name ) &&  (arg[0] != '\0')))
                        continue;

                if ( ++count == number )
                    return rch;
            }
        }
    }

    if (!ambiguous) 
        return possrch;
    
    return NULL;
}

CHAR_DATA *room_get_char_room( ROOM_INDEX_DATA *room, char *argument )
{
    int number, count = 0;
    char arg[MAX_STRING_LENGTH];
    CHAR_DATA *rch;

    if ((argument[0] == '\0') || !room)
	return NULL;

    number = number_argument( argument, arg );

    for (rch = room->people; rch; rch = rch->next_in_room)
    {
	if (!str_cmp(arg, rch->unique_name))
	    return rch;
        if (IS_OAFFECTED(rch, AFF_DISGUISE) ?
            (!is_name( arg, rch->fake_name) && arg[0] != '\0') :
            (!is_name( arg, rch->name ) &&  (arg[0] != '\0')))
                continue;
        if ( ++count == number )
            return rch;
    }

    return NULL;
}

CHAR_DATA *room_get_char_world( ROOM_INDEX_DATA *room, char *argument )
{
    if ((argument[0] == '\0') || !room)
        return NULL;

    CHAR_DATA * result(room_get_char_room(room, argument));
    if (result != NULL)
        return result;

    return NameMaps::LookupChar(NULL, argument, true);
}

bool verify_char_room(CHAR_DATA * ch, ROOM_INDEX_DATA * room)
{
    for (CHAR_DATA * i(room->people); i != NULL; i = i->next_in_room)
    {
        if (i == ch)
            return true;
    }

    return false;
}

bool verify_pc_world(CHAR_DATA * ch)
{
    for (DESCRIPTOR_DATA * d(descriptor_list); d != NULL; d = d->next)
    {
        if (d->connected == CON_PLAYING && (d->character == ch || d->original == ch))
            return true;
    }

    return false;
}

bool verify_char_world(CHAR_DATA * ch)
{
    for (CHAR_DATA * iter(char_list); iter != NULL; iter = iter->next)
    {
        if (iter == ch)
            return true;
    }

    return false;
}

CHAR_DATA *obj_get_char_room( OBJ_DATA *obj, char *argument )
{
    ROOM_INDEX_DATA *pRoom;
    int number, count;
    CHAR_DATA *rch;
    OBJ_DATA *pObj;
    char arg[MAX_STRING_LENGTH];

    if ((argument[0] == '\0') || !obj)
        return NULL;

    number = number_argument( argument, arg );
    count = 0;

    if (obj->in_obj)
        for (pObj = obj->in_obj; pObj->in_obj; pObj = pObj->in_obj)
            ;
    else
        pObj = obj;

    if (!str_cmp(arg, "self"))
        return pObj->carried_by;

    if (pObj->in_room)
        pRoom = pObj->in_room;
    else if (pObj->carried_by && pObj->carried_by->in_room)
        pRoom = pObj->carried_by->in_room;
    else
        return NULL;

    for (rch = pRoom->people; rch; rch = rch->next_in_room)
    {
	if (!str_cmp(arg, rch->unique_name))
	    return rch;
        if (IS_OAFFECTED(rch, AFF_DISGUISE) ?
            (!is_name( arg, rch->fake_name) && arg[0] != '\0') :
            (!is_name( arg, rch->name ) &&  (arg[0] != '\0')))
                continue;
        if ( ++count == number )
            return rch;
    }

    return NULL;
}

CHAR_DATA *obj_get_char_world( OBJ_DATA *obj, char *argument )
{
    if (argument[0] == '\0' || obj == NULL)
        return NULL;

    if (!str_cmp(argument, "self"))
        return obj->carried_by;

    CHAR_DATA * result(obj_get_char_room(obj, argument));
    if (result != NULL)
    	return result;

    return NameMaps::LookupChar(NULL, argument, true);
}

CHAR_DATA * get_char_by_id_any(long id)
{
    // Scan PCs first because most of the time they are the real target
    for (DESCRIPTOR_DATA * d(descriptor_list); d != NULL; d = d->next)
    {
        if (d->connected == CON_PLAYING && d->character->id == id)
            return d->character;
    }

    // Now scan the whole list
    for (CHAR_DATA * ch(char_list); ch != NULL; ch = ch->next)
    {
        if (ch->id == id)
            return ch;
    }

    return NULL;
}

CHAR_DATA * get_char_by_id_any_room(long id, const ROOM_INDEX_DATA & room)
{
    for (CHAR_DATA * ch(room.people); ch != NULL; ch = ch->next_in_room)
    {
        if (ch->id == id)
            return ch;
    }

    return NULL;
}

/*
 * Find a char in the world.
 */
CHAR_DATA *get_char_world(CHAR_DATA *ch, const char *argument)
{
    if (ch == NULL)
        return NULL;

    CHAR_DATA * result(get_char_room(ch, argument));
    if (result != NULL)
        return result;

    // NPCs must use exact names; PCs can prefix
    return NameMaps::LookupChar(ch, argument, IS_NPC(ch));
}

/*
 * Find a char in the area.
 */
CHAR_DATA *get_char_area( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *wch;
    CHAR_DATA *posswch = NULL;
    bool ambiguous = FALSE;
    int number;
    int count;

    if (ch == NULL)
	return NULL;

    if ( ( wch = get_char_room( ch, argument ) ) != NULL )
	return wch;

    number = number_argument( argument, arg );
    if (strlen(arg) < 3) ambiguous = TRUE;
    count  = 0;
    for ( wch = char_list; wch != NULL ; wch = wch->next )
    {
	if ( wch->in_room == NULL || !can_see( ch, wch ))
	    continue;

	if (ch->in_room->area != wch->in_room->area)
	    continue;

	if (!str_cmp(arg, wch->unique_name))
	    return wch;

	if (!str_prefix(arg, wch->unique_name) && !ambiguous)
	{
		if (posswch == NULL) posswch = wch;
		else ambiguous = TRUE;
	}
	
	if (!is_name( arg, wch->name ) )
	    continue;
	if ( ++count == number )
	    return wch;
    }

    if (!ambiguous) return posswch;
    
    return NULL;
}


/*
 * Find some object with a given index data.
 * Used by area-reset 'P' command.
 */
OBJ_DATA *get_obj_type( OBJ_INDEX_DATA *pObjIndex )
{
    OBJ_DATA *obj;

    for ( obj = object_list; obj != NULL; obj = obj->next )
    {
	if ( obj->pIndexData == pObjIndex )
	    return obj;
    }

    return NULL;
}


/*
 * Find an obj in a list.
 */
OBJ_DATA *get_obj_list( CHAR_DATA *ch, char *argument, OBJ_DATA *list )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    number = number_argument( argument, arg );
    count  = 0;
    for ( obj = list; obj != NULL; obj = obj->next_content )
    {
        if ( (!ch || can_see_obj( ch, obj )) && ((is_name( arg, obj->name )) || (arg[0] == '\0') ))
        {
            if ( ++count == number )
            return obj;
        }
    }

    return NULL;
}



/*
 * Find an obj in player's inventory.
 */
OBJ_DATA *get_obj_carry( CHAR_DATA *ch, char *argument, CHAR_DATA *viewer )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;
    AFFECT_DATA *paf;
    number = number_argument( argument, arg );
    count  = 0;
    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
//	if ( obj->wear_loc == WEAR_NONE

 	if (!obj->worn_on
	&&   (can_see_obj( viewer, obj ) ) 
	&&   (((is_name( arg, obj->name )) || (arg[0] == '\0')) 
	  || (((paf = affect_find(obj->affected,gsn_demon_bind)) != NULL)
	    && (viewer->id == paf->modifier)
	    && ((obj_is_affected(obj,gsn_hungerofgrmlarloth) && !strcmp(arg,"grmlarloth"))
	    || (obj_is_affected(obj,gsn_jawsofidcizon) && !strcmp(arg,"idcizon"))
	    || (obj_is_affected(obj,gsn_defilement) && !strcmp(arg,"logor"))
	    || (obj_is_affected(obj,gsn_blade_of_vershak) && !strcmp(arg,"vershak"))
	    || (obj_is_affected(obj,gsn_caressofpricina) && !strcmp(arg,"pricina"))
	    || (obj_is_affected(obj,gsn_mireofoame) && !strcmp(arg,"oame"))))))	
	{
	    if ( ++count == number )
		return obj;
	}
    }

    return NULL;
}



/*
 * Find an obj in player's equipment.
 */
OBJ_DATA *get_obj_wear( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    number = number_argument( argument, arg );
    count  = 0;
    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
//	if ( obj->wear_loc != WEAR_NONE

	if (obj->worn_on
	&&   can_see_obj( ch, obj )
	&&   ((is_name( arg, obj->name )) || (arg[0] == '\0') ))
	{
	    if ( ++count == number )
		return obj;
	}
    }

    return NULL;
}

OBJ_DATA *get_obj_wear_loot( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    number = number_argument( argument, arg );
    count  = 0;
    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
//        if ( obj->wear_loc != WEAR_NONE

	if (obj->worn_on
         && ((is_name( arg, obj->name )) || (arg[0] == '\0') ))
        {
            if ( ++count == number )
                return obj;
        }
    }

    return NULL;
}


/*
 * Find an obj in the room or in inventory.
 */
OBJ_DATA * room_get_obj_here(ROOM_INDEX_DATA * room, char * argument)
{
    char arg[MAX_STRING_LENGTH];
    int number = number_argument( argument, arg );
    int count  = 0;

    for (OBJ_DATA * obj = room->contents; obj; obj = obj->next_content)
    {
	    if (is_name(arg, obj->name))
    	{
	        if (++count == number)
	            return obj;
    	}
    }

    return NULL;
}

OBJ_DATA *obj_get_obj_here( OBJ_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA *pRoom = ch->carried_by ? ch->carried_by->in_room : ch->in_room;;

    if (!pRoom || (argument[0] == '\0'))
	return NULL;

    if (!str_cmp(argument, "self"))
        return ch;

    return room_get_obj_here(pRoom, argument);
}

OBJ_DATA *get_obj_here( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj;

    if (!ch->in_room)
	return NULL;

    obj = get_obj_list( ch, argument, ch->in_room->contents );
    if ( obj != NULL )
	return obj;

    if ( ( obj = get_obj_carry( ch, argument, ch ) ) != NULL )
	return obj;

    if ( ( obj = get_obj_wear( ch, argument ) ) != NULL )
	return obj;

    return NULL;
}

// Find an object in the room only
OBJ_DATA * get_obj_room(CHAR_DATA * ch, char * argument)
{
    if (!ch->in_room) 
        return NULL;

    return get_obj_list(ch, argument, ch->in_room->contents);
}

bool verify_obj_world(OBJ_DATA * obj)
{
    for (OBJ_DATA * testObj = object_list; testObj != NULL; testObj = testObj->next)
    {
        if (obj == testObj)
            return true;
    }

    return false;
}

ROOM_INDEX_DATA * get_room_for_obj(const OBJ_DATA & obj)
{
    // Handle the case of directly in room
    if (obj.in_room != NULL) 
        return obj.in_room;

    // Handle the case of carried by someone in a room
    if (obj.carried_by != NULL && obj.carried_by->in_room != NULL) 
        return obj.carried_by->in_room;

    // Handle the case of in an object
    if (obj.in_obj != NULL)
    {
        ROOM_INDEX_DATA * result(get_room_for_obj(*obj.in_obj));
        if (result != NULL)
            return result;
    }

    // Handle the case of on an object
    if (obj.on != NULL)
        return get_room_for_obj(*obj.on);
    
    // Nothing left to try, just return null
    return NULL;
}

/*
 * Find an obj in the world.
 */
OBJ_DATA *get_obj_world( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA * obj(get_obj_here(ch, argument));
    if (obj != NULL)
        return obj;

    return NameMaps::LookupObject(ch, argument, true);
}

OBJ_DATA *obj_get_obj_world( OBJ_DATA * ch, char *argument )
{
    OBJ_DATA *obj(obj_get_obj_here(ch, argument));
    if (obj != NULL)
    	return obj;

    return NameMaps::LookupObject(NULL, argument, true);
}

OBJ_DATA * room_get_obj_world(ROOM_INDEX_DATA * room, char * argument)
{
    OBJ_DATA * obj(room_get_obj_here(room, argument));
    if (obj != NULL)
        return obj;

    return NameMaps::LookupObject(NULL, argument, true);
}

/* deduct cost from a character */

void deduct_cost(CHAR_DATA *ch, float cost)
{
    dec_player_coins(ch, value_to_coins(cost, FALSE));
}   

/*
 * Create a stashed money object. Ah, the way the head spins.
 */
OBJ_DATA *create_money_stashed( long amount, int ctype )
{
    OBJ_DATA *obj = create_money(amount, ctype);

    if (obj)
        SET_BIT(obj->extra_flags[0], ITEM_STASHED);

    return obj;
}

/*
 * Create a CONCEALED 'money' obj.
 */

OBJ_DATA *create_money_concealed( long amount, int ctype )
{
    OBJ_DATA *obj = create_money(amount, ctype);

    if (obj)
        SET_BIT(obj->extra_flags[0], ITEM_HIDDEN);

    return obj;
}

/*
 * Create a 'money' obj.
 */
OBJ_DATA *create_money( long amount, int ctype )
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;

    if ( amount <= 0 )
    {
	bug( "Create_money: zero or negative money.", amount);
	return NULL;
    }

    obj = create_object( get_obj_index( OBJ_VNUM_COINS ), 0 );

    sprintf(buf, obj->short_descr, amount, coin_table[ctype].name);
    free_string(obj->short_descr);
    obj->short_descr = str_dup(buf);

    sprintf(buf, obj->name, coin_table[ctype].name);
    setName(*obj, buf);

    sprintf(buf, obj->description, coin_table[ctype].name);
    free_string(obj->description);
    obj->description = str_dup(buf);

    obj->value[1] = ctype;
    obj->value[0] = amount;
    obj->cost = round(amount * coin_table[ctype].value);
    obj->weight = amount / 100;
    obj->level = UMIN(obj->cost / 10000,51);

    return obj;
}



/*
 * Return # of objects which an object counts as.
 * Thanks to Tony Chamberlain for the correct recursive code here.
 */
int get_obj_number( OBJ_DATA *obj )
{
    int number;
 
    if (obj->item_type == ITEM_CONTAINER || obj->item_type == ITEM_MONEY
    ||  obj->item_type == ITEM_GEM || obj->item_type == ITEM_JEWELRY)
        number = 0;
    else
        number = 1;
 
    for ( obj = obj->contains; obj != NULL; obj = obj->next_content )
        number += get_obj_number( obj );
 
    return number;
}

// Return weight of the contents of an object, not including its own weight
int get_weight_contents(const OBJ_DATA & obj)
{
    int weight(0);
    int multiplier(WEIGHT_MULT(&obj));
    for (const OBJ_DATA * iter(obj.contains); iter != NULL; iter = iter->next_content)
        weight += (get_obj_weight(iter) * multiplier) / 100;
    
    return weight;
}

// Return full weight of an object, including contents
int get_obj_weight(const OBJ_DATA *obj )
{
    return (obj->weight + get_weight_contents(*obj));
}

// Returns full weight of an object and contents, ignoring this container's multiplier for contents
int get_true_weight(const OBJ_DATA *obj)
{
    int weight = obj->weight;
    for ( obj = obj->contains; obj != NULL; obj = obj->next_content )
        weight += get_obj_weight( obj );
 
    return weight;
}

bool can_see_in_room(CHAR_DATA *ch, ROOM_INDEX_DATA *pRoom)
{
    CHAR_DATA *vch;
    bool radiance = FALSE;
    AFFECT_DATA *gaf = NULL;

    if (!pRoom || (ch->position <= POS_SLEEPING) || check_blind(ch))
	return FALSE;

    /* If you have Holylight or Detect Wizi, you can always see in the room */

    if ((!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT))
     || (IS_NPC(ch) && IS_SET(ch->act, AFF_DETECT_WIZI)))
	return TRUE;

    for (vch = pRoom->people;vch != NULL; vch = vch->next_in_room)
    {
        if (IS_OAFFECTED(vch,AFF_RADIANCE))
            radiance = TRUE;
    }

    /* If the room is dark and you don't have infravision, you can't see, */
    /* or the room is affected by globe of darkness, you can only see if  */
    /* someone in the room has radiance.   -- Aeolis			  */
    if (IS_SET(pRoom->room_flags,ROOM_UBERDARK))
	    return FALSE;

    // At this point, shuddeni, sensory vision, and radiance can always see
    if (ch->race == global_int_race_shuddeni || IS_PAFFECTED(ch, AFF_SENSORY_VISION) || radiance)
	    return TRUE;

    if ((gaf = affect_find(pRoom->affected,gsn_globedarkness)) != NULL && (gaf->modifier != ch->id))
    	return FALSE;
 
    // At this point, we are dealing with standard room darkness
    if (room_is_dark(pRoom) && (is_affected(ch, gsn_shadowstrike) || (!IS_AFFECTED(ch, AFF_INFRARED) && !IS_AFFECTED(ch, AFF_DARK_VISION) && number_percent() > get_skill(ch, gsn_gloomward))))
    	return FALSE;

    return TRUE;
}    

/// This function comes up sometimes as CPU-heavy on the profiler, so pay attention to how you add things
bool room_is_dark( ROOM_INDEX_DATA *pRoomIndex )
{
    if (pRoomIndex == NULL)
    	return false;

    // Check for light-based room effects
    bool dimmed(false);
    if (!IS_SET(pRoomIndex->room_flags, ROOM_UBERDARK))
    {
        for (AFFECT_DATA * paf(pRoomIndex->affected); paf != NULL; paf = paf->next)
        {
            if (paf->type == gsn_continual_light || paf->type == gsn_undyingradiance || paf->type == gsn_canticleofthelightbringer)
                return false;

            if (paf->type == gsn_dim)
                dimmed = true;
        }
    }

    // Check for per-person effects
    bool duskfall(area_is_affected(pRoomIndex->area, gsn_duskfall));
    unsigned int darktallowCount(0);
    unsigned int glowCount(0);
    for (CHAR_DATA * vch(pRoomIndex->people); vch != NULL; vch = vch->next_in_room)
    {
        if (IS_OAFFECTED(vch, AFF_RADIANCE))
            return false;

        // Check for glowing items, unless duskfalled
        if (!duskfall)
        {
            for (OBJ_DATA * pObj(vch->carrying); pObj != NULL; pObj = pObj->next_content)
            {
                if (!pObj->worn_on)
                    continue;

                if (pObj->extra_flags[0] & ITEM_GLOW)
                {
                    // Has a glowing item, so the room is lit unless this person is affected by shadowstrike
                    if (!is_affected(vch,gsn_shadowstrike))
                        ++glowCount;

                    continue;
                }

                // Check for darktallow
                if (pObj->pIndexData->vnum == OBJ_VNUM_DARKTALLOW)
                    ++darktallowCount;
            }
        }
    }

    // At this point duskfall beats all remaining possibilities
    if (duskfall)
        return true;

    // Now check the ground for glowing items
    for (OBJ_DATA * obj(pRoomIndex->contents); obj != NULL; obj = obj->next_content)
    {
        if (obj->extra_flags[0] & ITEM_GLOW)
            ++glowCount;
    }

    // Compare darktallows and glowing items
    if (darktallowCount > 0 && darktallowCount * 2 >= glowCount)
        return true;

    // Any glowing items mean the room is lit
    if (glowCount > 0)
        return false;

    // Check dim, barrowmist, and the dark flag
    if (dimmed || area_is_affected(pRoomIndex->area, gsn_barrowmist) || IS_SET(pRoomIndex->room_flags, ROOM_DARK))
    	return true;

    // Check sector
    if (pRoomIndex->sector_type == SECT_INSIDE || pRoomIndex->sector_type == SECT_CITY)
    	return false;

    // Nighttime check; this only works if sun_down > sun_up, which holds for now
    if (time_info.hour >= season_table[time_info.season].sun_down || time_info.hour < season_table[time_info.season].sun_up)
    	return true;

    // Silver veil makes the default dark, even during the day
    if (silver_state == SILVER_FINAL)
    	return true;

    return false;
}

bool is_room_owner(CHAR_DATA *ch, ROOM_INDEX_DATA *room)
{
    if (room->owner == NULL || room->owner[0] == '\0')
	return FALSE;

    return is_name(ch->name,room->owner);
}

/*
 * True if room is private.
 */
bool room_is_private( ROOM_INDEX_DATA *pRoomIndex )
{
    CHAR_DATA *rch;
    int count;

    count = 0;
    for ( rch = pRoomIndex->people; rch != NULL; rch = rch->next_in_room )
	count++;

    if ( IS_SET(pRoomIndex->room_flags, ROOM_PRIVATE)  && count >= 2 )
	return TRUE;

    if ( IS_SET(pRoomIndex->room_flags, ROOM_SOLITARY) && count >= 1 )
	return TRUE;
    
    if ( IS_SET(pRoomIndex->room_flags, ROOM_IMP_ONLY) )
	return TRUE;

    return FALSE;
}

/* visibility on a room -- for entering and exits */
bool can_see_room( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex )
{
    if (IS_SET(pRoomIndex->room_flags, ROOM_IMP_ONLY) 
    &&  get_trust(ch) < MAX_LEVEL)
	return FALSE;

    if (IS_SET(pRoomIndex->room_flags, ROOM_GODS_ONLY)
    &&  !IS_IMMORTAL(ch))
	return FALSE;

    if (IS_SET(pRoomIndex->room_flags, ROOM_HEROES_ONLY)
    &&  !IS_IMMORTAL(ch))
	return FALSE;

    if (IS_SET(pRoomIndex->room_flags,ROOM_NEWBIES_ONLY)
    &&  ch->level > 10 && !IS_IMMORTAL(ch))
	return FALSE;

    if (!IS_NPC(ch) && ch->level < 11 && pRoomIndex->clan && ch->clan != pRoomIndex->clan)
	return FALSE;

    return TRUE;
}

enum SightResult {Invisible, Visible, Visible_Forced};

static SightResult can_see_common(CHAR_DATA * ch, CHAR_DATA * victim)
{
    // Characters can always see themselves, and those with detect wizi can see everybody
    if (ch == victim || IS_AFFECTED(ch, AFF_DETECT_WIZI))
    	return Visible_Forced;;

    // Wizinvis characters are always invisible to anyone of lower level
    if (!IS_NPC(victim) && get_trust(ch) < victim->invis_level)
    	return Invisible;

    // PCs with holylight or NPCs trusted over level 51 see everybody left
    if ((!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT)) || (IS_NPC(ch) && (ch->trust > 51)))
	    return Visible_Forced;

    // Shades are invisible to most without shroudsight
    if (IS_NPC(victim) && IS_SET(victim->nact, ACT_SHADE))
    {
        if (number_percent() >= get_skill(ch, gsn_shroudsight))
            return Invisible;

        check_improve(ch, NULL, gsn_shroudsight, TRUE, 12);
    }

    // At this point wizi means definitely invisible
    if (IS_NPC(victim) && IS_AFFECTED(victim, AFF_WIZI))
        return Invisible;

    // Incognito makes someone invisible to all not in the room
    if (get_trust(ch) < victim->incog_level && ch->in_room != victim->in_room)
    	return Invisible;

    // Blind characters can't see anything
    if (check_blind(ch))
    	return Invisible;

    // Check for astral projection
    if (is_affected(victim, gsn_astralprojection) && victim->desc != NULL && !is_affected(ch, gsn_veilsight))
    {
        if (number_percent() >= get_skill(ch, gsn_shroudsight))
    	    return Invisible;

        check_improve(ch, NULL, gsn_shroudsight, true, 12);
    }

    // Check for invis vs detect invis
    if (IS_AFFECTED(victim, AFF_INVISIBLE) && !IS_AFFECTED(ch, AFF_DETECT_INVIS))
    	return Invisible;

    // Check for special hide-from-all skills
    if (is_affected(victim, gsn_meldwithstone) || is_affected(victim, gsn_flameunity) 
    || is_affected(victim, gsn_shadowmastery) || is_affected(victim, gsn_airrune)
    || is_affected(victim, gsn_runeofair))
    	return Invisible;

    // Check for voidwalk
    if (!IS_NPC(ch) && IS_PAFFECTED(victim, AFF_VOIDWALK) && !is_affected(ch, gsn_truesight) && !is_affected(ch, gsn_veilsight))
    {
        if (number_percent() <= get_skill(ch, gsn_shroudsight))
            check_improve(ch, victim, gsn_shroudsight, true, 12);
        else 
            return Invisible;
    }

    // Check for obfuscation
    const AFFECT_DATA * victim_obs(get_affect(victim, gsn_obfuscation));
    if (victim_obs != NULL)
    {
        // Look for a matching modifier on the ch
        const AFFECT_DATA * ch_obs(get_affect(ch, gsn_obfuscation));
        if (ch_obs != NULL && ch_obs->modifier == victim_obs->modifier)
            return Invisible;
    }

    // Check for cloak
	for (const AFFECT_DATA * paf(get_affect(victim, gsn_cloak)); paf != NULL; paf = get_affect(victim, gsn_cloak, paf))
    {
		if (paf->modifier == ch->id)
		    return Invisible;
    }

    // Check for camouflage/camoublind vs sharp vision
    if ((is_affected(victim, gsn_camouflage) || is_affected(victim, gsn_camoublind)) &&	!IS_PAFFECTED(ch, AFF_SHARP_VISION))
    	return Invisible;

    // Check for hide vs detect hidden
    if (IS_AFFECTED(victim, AFF_HIDE) && !IS_AFFECTED(ch, AFF_DETECT_HIDDEN) && victim->fighting == NULL)
    	return Invisible;

    // Check for nightstalk
    if (is_nightstalking(*victim) && victim->level >= ch->level && victim->in_room != NULL && room_is_dark(victim->in_room))
        return Invisible;

    // No disqualifiers, so by default ch can see victim
    return Visible;
}

// Used for visibility in comms, which are generally slightly less restrictive
// In particular, you can see people even if sleeping, to avoid "someone" talking to you
bool can_see_comm( CHAR_DATA *ch, CHAR_DATA *victim )
{
    return (can_see_common(ch, victim) != Invisible);
}

// Used for general visibility
bool can_see( CHAR_DATA *ch, CHAR_DATA *victim )
{
    // Check for the common visibility checks
    switch (can_see_common(ch, victim))
    {
        case Invisible: return false;
        case Visible_Forced: return true;
        default: break;
    }

    /* Essentially, we're checking here whether the room is covered in magic or natural darkness */
    /* There has got to be a better way to do this, but right now I just want it done. -- Aeolis */
    if (!can_see_in_room(ch, victim->in_room))
    {
    	if (victim->in_room && !IS_SET(victim->in_room->room_flags, ROOM_UBERDARK)
        && !room_is_affected(victim->in_room, gsn_globedarkness))
        {
	        /* At this point, we've determined it's natural darkness */
    	    if (!IS_AFFECTED(victim, AFF_FAERIE_FIRE))
	        	return false;
	    }
	    else
	        return false;
    }
	
    // Check for sleeping
    if (!IS_AWAKE(ch))
    	return false;

    return true;
}

/*
 * True if char can see obj.
 */
bool can_see_obj( CHAR_DATA *ch, OBJ_DATA *obj )
{
    if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT) )
	return TRUE;

    if ( IS_SET(obj->extra_flags[0],ITEM_VIS_DEATH))
	return FALSE;

    if ( IS_SET(obj->extra_flags[0],ITEM_STASHED))
	return FALSE;

    if ( IS_SET(obj->extra_flags[0], ITEM_WIZI))
	return FALSE;
    
    CHAR_DATA *vch;

    if (obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC
      || obj->pIndexData->vnum == OBJ_VNUM_CORPSE_NPC)
 	for (vch = ch->in_room->people;vch;vch = vch->next_in_room)
    	{
	    if (is_affected(vch,gsn_concealremains))
	        if (!(IS_IMMORTAL(ch)
		  || ch == vch
		  || is_name(ch->name,obj->owner)
                  || is_affected(ch,gsn_mindshell) 
		  || is_affected(ch,gsn_mindshield)))
	            return FALSE;
        }

    if ( IS_SET(obj->extra_flags[0],ITEM_HIDDEN) && (!IS_AFFECTED(ch, AFF_DETECT_HIDDEN) && !is_affected(ch,gsn_perception)))
	return FALSE;

    // Check for unreal incursion
    if (is_affected(ch, gsn_unrealincursion))
    {
        srand(ch->id + reinterpret_cast<long>(obj));
        bool invisible((rand() % 100) <= 3);
        srand(time(0));
        if (invisible)
            return FALSE;
    }

    //Added by Kestrel, allowing blind players to see food and drinkcontainers too
    if (obj->item_type == ITEM_DRINK_CON)
        return TRUE;
    
    if (obj->item_type == ITEM_FOOD)
        return TRUE;

    if (obj->item_type == ITEM_PILL)
        return TRUE;

    if ( check_blind(ch) && obj->item_type != ITEM_POTION)
        return FALSE;

    if ( IS_SET(obj->extra_flags[0], ITEM_INVIS)
    &&   !IS_AFFECTED(ch, AFF_DETECT_INVIS) )
        return FALSE;

    if ( IS_OBJ_STAT(obj,ITEM_GLOW))
	return TRUE;

    if (room_is_dark(ch->in_room)
      && (is_affected(ch,gsn_shadowstrike)
	|| (!IS_AFFECTED(ch, AFF_INFRARED) && !IS_AFFECTED(ch, AFF_DARK_VISION) && number_percent() > get_skill(ch, gsn_gloomward))))
	return FALSE;

    return TRUE;
}



/*
 * True if char can drop obj.
 */
bool can_drop_obj( CHAR_DATA *ch, OBJ_DATA *obj )
{
    if ( !IS_SET(obj->extra_flags[0], ITEM_NODROP) )
	return TRUE;

// brazen: NPCs should not be able to drop nodrop items
    if ( ch->level >= LEVEL_IMMORTAL )
	return TRUE;

    return FALSE;
}


/*
 * Return ascii name of an affect location.
 */
char *affect_loc_name( int location )
{ 
    char buf[MAX_STRING_LENGTH];
 
    switch ( location )
    {
    default:
        if ((location >= FIRST_APPLY_RESIST) && (location < (FIRST_APPLY_RESIST + MAX_RESIST)))
        {
	    sprintf(buf, "resistance to %s", imm_flags[location - FIRST_APPLY_RESIST].name);
	    return str_dup(buf);
	}
    case APPLY_NONE:		return "none";
    case APPLY_STR:		return "strength";
    case APPLY_DEX:		return "dexterity";
    case APPLY_INT:		return "intelligence";
    case APPLY_WIS:		return "wisdom";
    case APPLY_CON:		return "constitution";
    case APPLY_CHR:		return "charisma";
    case APPLY_SEX:		return "sex";
    case APPLY_CLASS:		return "class";
    case APPLY_LEVEL:		return "level";
    case APPLY_AGE:		return "age";
    case APPLY_MANA:		return "mana";
    case APPLY_HIT:		return "hp";
    case APPLY_MOVE:		return "moves";
    case APPLY_GOLD:		return "gold";
    case APPLY_EXP:		return "experience";
    case APPLY_AC:		return "armor class";
    case APPLY_HITROLL:		return "hit roll";
    case APPLY_DAMROLL:		return "damage roll";
    case APPLY_SAVES:		return "saves";
    case APPLY_SAVING_ROD:	return "save vs rod";
    case APPLY_SAVING_PETRI:	return "save vs petrification";
    case APPLY_SAVING_BREATH:	return "save vs breath";
    case APPLY_SAVING_SPELL:	return "save vs spell";
    case APPLY_SPELL_AFFECT:
    case APPLY_FORM:		
    case APPLY_HIDE:		return "none";
    case APPLY_RANGE:		return "range";
    case APPLY_SIZE:		return "size";
    case APPLY_LUCK:        return "luck";
    case APPLY_MAXSTR:		return "max strength";
    case APPLY_MAXDEX:		return "max dexterity";
    case APPLY_MAXINT:		return "max intelligence";
    case APPLY_MAXWIS:		return "max wisdom";
    case APPLY_MAXCON:		return "max constitution";
    case APPLY_MAXCHR:		return "max charisma";
    }

    bug( "Affect_location_name: unknown location %d.", location );
    return "(unknown)";
}



/*
 * Return ascii name of an affect bit vector.
 */
const char *affect_bit_name( AFFECT_DATA *paf )
{
    int i=0;
    static char buf[MAX_STRING_LENGTH];

    buf[0] = '\0';
    if (!paf) return NULL;
    switch (paf->where)
    {
	case TO_AFFECTS: for (i=0;affect_flags[i].name != NULL;i++)
			     if (affect_flags[i].bit & paf->bitvector)
			     { strcat(buf," "); 
			       strcat(buf,affect_flags[i].name); }; break;
	case TO_OAFFECTS: for (i=0;oaffect_flags[i].name != NULL;i++)
			     if (oaffect_flags[i].bit & paf->bitvector)
			     { strcat(buf," "); 
			       strcat(buf,oaffect_flags[i].name); }; break;
	case TO_PAFFECTS: for (i=0;paffect_flags[i].name != NULL;i++)
			     if (paffect_flags[i].bit & paf->bitvector)
			     { strcat(buf," "); 
			       strcat(buf,paffect_flags[i].name); }; break;
	case TO_NAFFECTS: for (i=0;naffect_flags[i].name != NULL;i++)
			     if (naffect_flags[i].bit & paf->bitvector)
			     { strcat(buf," "); 
			       strcat(buf,naffect_flags[i].name); }; break;
    }
    return ( buf[0] != '\0' ) ? buf+1 : "none";
}



/*
 * Return ascii name of extra flags vector.
 */
const char *extra_bit_name( unsigned long extra_flags, int field, bool hideImmOnlyBits)
{
    static char buf[512];

    buf[0] = '\0';

    if (field == 0)
    {
        if ( extra_flags & ITEM_GLOW         ) strcat( buf, " glow"         );
        if ( extra_flags & ITEM_HUM          ) strcat( buf, " hum"          );
        if ( extra_flags & ITEM_DARK         ) strcat( buf, " dark"         );
        if ( extra_flags & ITEM_WARM         ) strcat( buf, " warm"         );
        if ( extra_flags & ITEM_EVIL         ) strcat( buf, " evil"         );
        if ( extra_flags & ITEM_INVIS        ) strcat( buf, " invis"        );
        if ( extra_flags & ITEM_MAGIC        ) strcat( buf, " magic"        );
        if ( extra_flags & ITEM_NODROP       ) strcat( buf, " nodrop"       );
        if ( extra_flags & ITEM_BLESS        ) strcat( buf, " bless"        );
        if ( extra_flags & ITEM_ANTI_GOOD    ) strcat( buf, " anti-good"    );
        if ( extra_flags & ITEM_ANTI_EVIL    ) strcat( buf, " anti-evil"    );
        if ( extra_flags & ITEM_ANTI_NEUTRAL ) strcat( buf, " anti-neutral" );
        if ( extra_flags & ITEM_NOREMOVE     ) strcat( buf, " noremove"     );
        if ( extra_flags & ITEM_INVENTORY    ) strcat( buf, " inventory"    );
        if ( extra_flags & ITEM_NOPURGE	     ) strcat( buf, " nopurge"	    );
        if ( extra_flags & ITEM_VIS_DEATH    ) strcat( buf, " vis_death"    );
        if ( extra_flags & ITEM_ROT_DEATH    ) strcat( buf, " rot_death"    );
        if ( extra_flags & ITEM_NONMETAL     ) strcat( buf, " nonmetal"     );
        if ( extra_flags & ITEM_NOLOCATE     ) strcat( buf, " no_locate"    );
        if ( extra_flags & ITEM_SELL_EXTRACT ) strcat( buf, " sell_extract" );
        if ( extra_flags & ITEM_QUEST	     ) strcat( buf, " quest"        );
        if ( extra_flags & ITEM_BURN_PROOF   ) strcat( buf, " burn_proof"   );
        if ( extra_flags & ITEM_NOUNCURSE    ) strcat( buf, " no_uncurse"   );
        if ( extra_flags & ITEM_NODESTROY    ) strcat( buf, " nodestroy"    );
        if ( extra_flags & ITEM_HIDDEN	     ) strcat( buf, " hidden"	    );
        if ( extra_flags & ITEM_STASHED	     ) strcat( buf, " stashed"	    );
        if ( extra_flags & ITEM_WIZI	     ) strcat( buf, " wizi"	    );
		if ( extra_flags & ITEM_NODISARM     ) strcat( buf, " nodisarm"	    );
    }
    else if (field == 1)
    {
    }
	else if (field == 2)
	{
        if ( extra_flags & ITEM_FIRE         ) strcat( buf, " fire"         );
        if ( extra_flags & ITEM_NOMORTAL     ) strcat( buf, " nomortal"     );
        if ( extra_flags & ITEM_QUINTESSENCE ) strcat( buf, " quintessence" );
        if ( extra_flags & ITEM_INCENSE      ) strcat( buf, " incense"      );
        if ( extra_flags & ITEM_ANNOINTINGOIL) strcat( buf, " annointingoil");

        if (!hideImmOnlyBits)
        {
            if ( extra_flags & ITEM_WINDFALL)      strcat( buf, " windfall"     );
            if ( extra_flags & ITEM_NOMIRROR)      strcat( buf, " nomirror"     );
        }
	}

    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

/* return ascii name of an act vector */
const char *act_bit_name( int act_flags )
{
    static char buf[512];

    buf[0] = '\0';

    if (IS_SET(act_flags,ACT_IS_NPC))
    { 
 	strcat(buf," npc");
    	if (act_flags & ACT_SENTINEL 	) strcat(buf, " sentinel");
    	if (act_flags & ACT_SCAVENGER	) strcat(buf, " scavenger");
	if (act_flags & ACT_AGGRESSIVE	) strcat(buf, " aggressive");
	if (act_flags & ACT_TRACK_GATE  ) strcat(buf, " track_gate");
	if (act_flags & ACT_NOTRACK	) strcat(buf, " notrack");
	if (act_flags & ACT_STAY_AREA	) strcat(buf, " stay_area");
	if (act_flags & ACT_WIMPY	) strcat(buf, " wimpy");
	if (act_flags & ACT_PET		) strcat(buf, " pet");
	if (act_flags & ACT_TRAIN	) strcat(buf, " train");
	if (act_flags & ACT_PRACTICE	) strcat(buf, " practice");
	if (act_flags & ACT_NOWANDER	) strcat(buf, " nowander");
	if (act_flags & ACT_GUILDGUARD  ) strcat(buf, " guildguard");
	if (act_flags & ACT_ANIMAL	) strcat(buf, " animal");
	if (act_flags & ACT_UNDEAD	) strcat(buf, " undead");
	if (act_flags & ACT_BADASS	) strcat(buf, " badass");
	if (act_flags & ACT_MODIFIED	) strcat(buf, " modified");
	if (act_flags & ACT_THIEF	) strcat(buf, " thief");
	if (act_flags & ACT_WARRIOR	) strcat(buf, " warrior");
	if (act_flags & ACT_NOALIGN	) strcat(buf, " no_align");
	if (act_flags & ACT_NOPURGE	) strcat(buf, " no_purge");
	if (act_flags & ACT_NOSUBDUE	) strcat(buf, " nosubdue");
	if (act_flags & ACT_GUARDIAN	) strcat(buf, " guardian");
	if (act_flags & ACT_IS_HEALER	) strcat(buf, " healer");
	if (act_flags & ACT_IS_CHANGER  ) strcat(buf, " changer");
	if (act_flags & ACT_FAMILIAR	) strcat(buf, " familiar");
	if (act_flags & ACT_GAIN	) strcat(buf, " skill_train");
	if (act_flags & ACT_UPDATE_ALWAYS) strcat(buf," update_always");
    }
    else
    {
	strcat(buf," player");
	if (act_flags & PLR_AUTOATTACK	) strcat(buf, "	autoattack");
	if (act_flags & PLR_AUTODEFEND	) strcat(buf, "	autodefend");
	if (act_flags & PLR_AUTOASSIST	) strcat(buf, " autoassist");
	if (act_flags & PLR_SHOWLINES   ) strcat(buf, " showlines");
	if (act_flags & PLR_AUTOEXIT	) strcat(buf, " autoexit");
	if (act_flags & PLR_AUTOLOOT	) strcat(buf, " autoloot");
	if (act_flags & PLR_AUTODES	) strcat(buf, " autodestroy");
	if (act_flags & PLR_AUTOGOLD	) strcat(buf, " autogold");
	if (act_flags & PLR_AUTOSPLIT	) strcat(buf, " autosplit");
	if (act_flags & PLR_HOLYLIGHT	) strcat(buf, " holy_light");
	if (act_flags & PLR_CANLOOT	) strcat(buf, " loot_corpse");
	if (act_flags & PLR_NOSUMMON	) strcat(buf, " no_summon");
	if (act_flags & PLR_NOFOLLOW	) strcat(buf, " no_follow");
	if (act_flags & PLR_FREEZE	) strcat(buf, " frozen");
	if (act_flags & PLR_COLOUR	) strcat(buf, " colour");
	if (act_flags & PLR_REWARD	) strcat(buf, " reward");
	if (act_flags & PLR_SOUND	) strcat(buf, " sound");
	if (act_flags & PLR_DISPLAY	) strcat(buf, " display");
	if (act_flags & PLR_EPNOTE	) strcat(buf, " epnotice");
	if (act_flags & PLR_INDUCT	) strcat(buf, " induct");
    }
    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

const char *comm_bit_name(int comm_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (comm_flags & COMM_QUIET		) strcat(buf, " quiet");
    if (comm_flags & COMM_DEAF		) strcat(buf, " deaf");
    if (comm_flags & COMM_NOWIZ		) strcat(buf, " no_wiz");
    if (comm_flags & COMM_NOAUCTION	) strcat(buf, " no_auction");
    if (comm_flags & COMM_NOGOSSIP	) strcat(buf, " no_gossip");
    if (comm_flags & COMM_NOQUESTION	) strcat(buf, " no_question");
    if (comm_flags & COMM_NOMUSIC	) strcat(buf, " no_music");
    if (comm_flags & COMM_NOQUOTE	) strcat(buf, " no_quote");
    if (comm_flags & COMM_COMPACT	) strcat(buf, " compact");
    if (comm_flags & COMM_BRIEF		) strcat(buf, " brief");
    if (comm_flags & COMM_PROMPT	) strcat(buf, " prompt");
    if (comm_flags & COMM_COMBINE	) strcat(buf, " combine");
    if (comm_flags & COMM_NOEMOTE	) strcat(buf, " no_emote");
    if (comm_flags & COMM_NOSHOUT	) strcat(buf, " no_shout");
    if (comm_flags & COMM_NOTELL	) strcat(buf, " no_tell");
    if (comm_flags & COMM_NOCHANNELS	) strcat(buf, " no_channels");
    

    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

const char *imm_bit_name(int imm_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (imm_flags & IMM_SUMMON		) strcat(buf, " summon");
    if (imm_flags & IMM_CHARM		) strcat(buf, " charm");
    if (imm_flags & IMM_MAGIC		) strcat(buf, " magic");
    if (imm_flags & IMM_WEAPON		) strcat(buf, " weapon");
    if (imm_flags & IMM_BASH		) strcat(buf, " blunt");
    if (imm_flags & IMM_PIERCE		) strcat(buf, " piercing");
    if (imm_flags & IMM_SLASH		) strcat(buf, " slashing");
    if (imm_flags & IMM_FIRE		) strcat(buf, " fire");
    if (imm_flags & IMM_COLD		) strcat(buf, " cold");
    if (imm_flags & IMM_LIGHTNING	) strcat(buf, " lightning");
    if (imm_flags & IMM_ACID		) strcat(buf, " acid");
    if (imm_flags & IMM_POISON		) strcat(buf, " poison");
    if (imm_flags & IMM_NEGATIVE	) strcat(buf, " negative");
    if (imm_flags & IMM_HOLY		) strcat(buf, " holy");
    if (imm_flags & IMM_ENERGY		) strcat(buf, " energy");
    if (imm_flags & IMM_MENTAL		) strcat(buf, " mental");
    if (imm_flags & IMM_DISEASE		) strcat(buf, " disease");
    if (imm_flags & IMM_DROWNING	) strcat(buf, " drowning");
    if (imm_flags & IMM_LIGHT		) strcat(buf, " light");
    if (imm_flags & IMM_IRON		) strcat(buf, " iron");
    if (imm_flags & IMM_DEFILEMENT	) strcat(buf, " defilement");
    if (imm_flags & IMM_FEAR		) strcat(buf, " fear");
    if (imm_flags & IMM_TAME		) strcat(buf, " tame");
    if (imm_flags & IMM_BLIND		) strcat(buf, " blind");

    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

const char *wear_bit_name(int wear_flags)
{
    static char buf[512];

    buf [0] = '\0';
    if (wear_flags & ITEM_TAKE		) strcat(buf, " take");
    if (wear_flags & ITEM_WEAR_FINGER	) strcat(buf, " finger");
    if (wear_flags & ITEM_WEAR_NECK	) strcat(buf, " neck");
    if (wear_flags & ITEM_WEAR_BODY	) strcat(buf, " torso");
    if (wear_flags & ITEM_WEAR_HEAD	) strcat(buf, " head");
    if (wear_flags & ITEM_WEAR_LEGS	) strcat(buf, " legs");
    if (wear_flags & ITEM_WEAR_FEET	) strcat(buf, " feet");
    if (wear_flags & ITEM_WEAR_HANDS	) strcat(buf, " hands");
    if (wear_flags & ITEM_WEAR_ARMS	) strcat(buf, " arms");
    if (wear_flags & ITEM_WEAR_SHIELD	) strcat(buf, " shield");
    if (wear_flags & ITEM_WEAR_ABOUT	) strcat(buf, " body");
    if (wear_flags & ITEM_WEAR_WAIST	) strcat(buf, " waist");
    if (wear_flags & ITEM_WEAR_WRIST	) strcat(buf, " wrist");
    if (wear_flags & ITEM_WIELD		) strcat(buf, " wield");
    if (wear_flags & ITEM_HOLD		) strcat(buf, " hold");
    if (wear_flags & ITEM_NO_SAC	) strcat(buf, " nosac");
    if (wear_flags & ITEM_WEAR_FLOAT	) strcat(buf, " float");
    if (wear_flags & ITEM_WEAR_FAMILIAR	) strcat(buf, " familiar");

    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

const char *form_bit_name(int form_flags)
{
    static char buf[512];

    buf[0] = '\0';
    if (form_flags & FORM_POISON	) strcat(buf, " poison");
    else if (form_flags & FORM_EDIBLE	) strcat(buf, " edible");
    if (form_flags & FORM_MAGICAL	) strcat(buf, " magical");
    if (form_flags & FORM_INSTANT_DECAY	) strcat(buf, " instant_rot");
    if (form_flags & FORM_OTHER		) strcat(buf, " other");
    if (form_flags & FORM_ANIMAL	) strcat(buf, " animal");
    if (form_flags & FORM_SENTIENT	) strcat(buf, " sentient");
    if (form_flags & FORM_UNDEAD	) strcat(buf, " undead");
    if (form_flags & FORM_CONSTRUCT	) strcat(buf, " construct");
    if (form_flags & FORM_MIST		) strcat(buf, " mist");
    if (form_flags & FORM_INTANGIBLE	) strcat(buf, " intangible");
    if (form_flags & FORM_BIPED		) strcat(buf, " biped");
    if (form_flags & FORM_CENTAUR	) strcat(buf, " centaur");
    if (form_flags & FORM_INSECT	) strcat(buf, " insect");
    if (form_flags & FORM_SPIDER	) strcat(buf, " spider");
    if (form_flags & FORM_CRUSTACEAN	) strcat(buf, " crustacean");
    if (form_flags & FORM_WORM		) strcat(buf, " worm");
    if (form_flags & FORM_BLOB		) strcat(buf, " blob");
    if (form_flags & FORM_MAMMAL	) strcat(buf, " mammal");
    if (form_flags & FORM_BIRD		) strcat(buf, " bird");
    if (form_flags & FORM_REPTILE	) strcat(buf, " reptile");
    if (form_flags & FORM_SNAKE		) strcat(buf, " snake");
    if (form_flags & FORM_DRAGON	) strcat(buf, " dragon");
    if (form_flags & FORM_AMPHIBIAN	) strcat(buf, " amphibian");
    if (form_flags & FORM_FISH		) strcat(buf, " fish");
    if (form_flags & FORM_COLD_BLOOD 	) strcat(buf, " cold_blooded");

    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

const char *part_bit_name(int part_flags)
{
    static char buf[512];

    buf[0] = '\0';
    if (part_flags & PART_HEAD		) strcat(buf, " head");
    if (part_flags & PART_ARMS		) strcat(buf, " arms");
    if (part_flags & PART_LEGS		) strcat(buf, " legs");
    if (part_flags & PART_HEART		) strcat(buf, " heart");
    if (part_flags & PART_BRAINS	) strcat(buf, " brains");
    if (part_flags & PART_GUTS		) strcat(buf, " guts");
    if (part_flags & PART_HANDS		) strcat(buf, " hands");
    if (part_flags & PART_FEET		) strcat(buf, " feet");
    if (part_flags & PART_FINGERS	) strcat(buf, " fingers");
    if (part_flags & PART_EAR		) strcat(buf, " ears");
    if (part_flags & PART_EYE		) strcat(buf, " eyes");
    if (part_flags & PART_LONG_TONGUE	) strcat(buf, " long_tongue");
    if (part_flags & PART_EYESTALKS	) strcat(buf, " eyestalks");
    if (part_flags & PART_TENTACLES	) strcat(buf, " tentacles");
    if (part_flags & PART_FINS		) strcat(buf, " fins");
    if (part_flags & PART_WINGS		) strcat(buf, " wings");
    if (part_flags & PART_TAIL		) strcat(buf, " tail");
    if (part_flags & PART_CLAWS		) strcat(buf, " claws");
    if (part_flags & PART_FANGS		) strcat(buf, " fangs");
    if (part_flags & PART_HORNS		) strcat(buf, " horns");
    if (part_flags & PART_SCALES	) strcat(buf, " scales");

    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

const char *weapon_bit_name(int weapon_flags)
{
    static char buf[512];

    buf[0] = '\0';
    if (weapon_flags & WEAPON_FLAMING	) strcat(buf, " flaming");
    if (weapon_flags & WEAPON_FROST	) strcat(buf, " frost");
    if (weapon_flags & WEAPON_VAMPIRIC	) strcat(buf, " vampiric");
    if (weapon_flags & WEAPON_SHARP	) strcat(buf, " sharp");
    if (weapon_flags & WEAPON_VORPAL	) strcat(buf, " vorpal");
    if (weapon_flags & WEAPON_TWO_HANDS ) strcat(buf, " two-handed");
    if (weapon_flags & WEAPON_SHOCKING 	) strcat(buf, " shocking");
    if (weapon_flags & WEAPON_POISON	) strcat(buf, " poison");

    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

const char *cont_bit_name( int cont_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (cont_flags & CONT_CLOSEABLE	) strcat(buf, " closable");
    if (cont_flags & CONT_PICKPROOF	) strcat(buf, " pickproof");
    if (cont_flags & CONT_CLOSED	) strcat(buf, " closed");
    if (cont_flags & CONT_LOCKED	) strcat(buf, " locked");

    return (buf[0] != '\0' ) ? buf+1 : "none";
}


const char *off_bit_name(int off_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (off_flags & OFF_AREA_ATTACK	) strcat(buf, " area_attack");
    if (off_flags & OFF_BACKSTAB	) strcat(buf, " backstab");
    if (off_flags & OFF_BASH		) strcat(buf, " bash");
    if (off_flags & OFF_BERSERK		) strcat(buf, " berserk");
    if (off_flags & OFF_DISARM		) strcat(buf, " disarm");
    if (off_flags & OFF_DODGE		) strcat(buf, " dodge");
    if (off_flags & OFF_FADE		) strcat(buf, " fade");
    if (off_flags & OFF_FAST		) strcat(buf, " fast");
    if (off_flags & OFF_KICK		) strcat(buf, " kick");
    if (off_flags & OFF_KICK_DIRT	) strcat(buf, " kick_dirt");
    if (off_flags & OFF_PARRY		) strcat(buf, " parry");
    if (off_flags & OFF_RESCUE		) strcat(buf, " rescue");
    if (off_flags & OFF_TAIL		) strcat(buf, " tail");
    if (off_flags & OFF_TRIP		) strcat(buf, " trip");
    if (off_flags & OFF_CRUSH		) strcat(buf, " crush");
    if (off_flags & ASSIST_ALL		) strcat(buf, " assist_all");
    if (off_flags & ASSIST_ALIGN	) strcat(buf, " assist_align");
    if (off_flags & ASSIST_RACE		) strcat(buf, " assist_race");
    if (off_flags & ASSIST_PLAYERS	) strcat(buf, " assist_players");
    if (off_flags & ASSIST_GUARD	) strcat(buf, " assist_guard");
    if (off_flags & ASSIST_VNUM		) strcat(buf, " assist_vnum");
    if (off_flags & ASSIST_NPCRACE	) strcat(buf, " assist_npcrace");

    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

std::vector<CHAR_DATA *> find_cloakers(CHAR_DATA * ch)
{
    std::vector<CHAR_DATA *> result;

    for (DESCRIPTOR_DATA * d(descriptor_list); d != NULL; d = d->next)
    {
        if (d->character == NULL)
            continue;

        for (AFFECT_DATA * paf(get_affect(d->character, gsn_cloak)); paf != NULL; paf = get_affect(d->character, gsn_cloak, paf))
        {
            if (paf->modifier == ch->id)
            {
                result.push_back(d->character);
                break;
            }
        }
    }

    return result;
}

CHAR_DATA *cloak_remove(CHAR_DATA *ch)
{
    DESCRIPTOR_DATA *d;
    AFFECT_DATA *paf;
    int i=0;

    for (d=descriptor_list;d;d=d->next)
        if (d->character 
	  && ((paf=affect_find(d->character->affected,gsn_cloak)) != NULL)
	  && paf->modifier == ch->id)
	{
	    for (i=0;i<focus_slots(d->character);i++)
		if (d->character->pcdata->focus_sn[i] == gsn_cloak)
		    if (d->character->pcdata->focus_ch[i] == ch)
		    {
	    		affect_strip(d->character,gsn_cloak);
			unfocus(d->character,i,TRUE);
			unfocus(d->character,i,TRUE);
			return d->character;
		    }
	}
    return NULL;
}

bool can_be_affected(CHAR_DATA *ch, int sn)
{
    if (IS_NPC(ch) && IS_SET(ch->act,ACT_ILLUSION))
    {
        if (skill_table[sn].spell_fun == spell_song
          || skill_table[sn].spell_fun == spell_focus
          || sn == gsn_rally
          || sn == gsn_taunt
          || sn == gsn_frenzy
          || sn == gsn_zeal)
            return FALSE;
        
    }
    return TRUE;
}
