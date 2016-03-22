#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <time.h>
#include "merc.h"
#include "olc.h"
#include "magic.h"
#include "fight.h"
#include "tables.h"
#include "demons.h"
#include "skills_chirurgeon.h"
#include "spells_void.h"
#include "spells_spirit.h"
#include "spells_water_void.h"
#include "Direction.h"
#include "NameMaps.h"
#include <sstream>
#include <set>

/* External declarations */
DECLARE_DO_FUN(do_yell);
DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_say );
DECLARE_DO_FUN(do_autoyell);

extern	void	set_fighting	args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
extern	void	do_emote	args( ( CHAR_DATA *ch, char *argument) );
static bool handle_curse_char(int sn, int level, CHAR_DATA * ch, CHAR_DATA * victim);

bool is_symbol_present(const ROOM_INDEX_DATA & room, int symbol_vnum)
{
    for (const OBJ_DATA * obj(room.contents); obj != NULL; obj = obj->next_content)
    {
        if (obj->pIndexData->vnum == symbol_vnum && obj->value[0] == 1)
            return true;
    }

    return false;
}

bool is_familiar_present(CHAR_DATA & ch, int sn, int obj_vnum)
{
    // Check the familiar slot on the ch
    OBJ_DATA * obj(get_eq_char(&ch, WEAR_FAMILIAR));
    if (obj != NULL && obj->pIndexData->vnum == obj_vnum)
        return true;

    if (ch.in_room == NULL)
        return false;

    // Check the NPCs in the room
    for (CHAR_DATA * pet(ch.in_room->people); pet != NULL; pet = pet->next_in_room)
    {
        if (IS_NPC(pet) && pet->master == &ch)
        {
            AFFECT_DATA * paf(get_affect(pet, sn));
            if (paf != NULL && paf->modifier == ch.id)
                return true;
        }
    }

    return false;
}

CHAR_DATA * call_familiar(CHAR_DATA & ch, int sn, int level, int vnum)
{
    // Sanity-check
    if (ch.in_room == NULL)
        return NULL;

    // Check for NPC
    if (IS_NPC(&ch))
    {
        send_to_char("You lack the presence to command a familiar.\n", &ch);
        return NULL;
    }

    // Check for cooldown
    if (is_affected(&ch, sn))
    {
        send_to_char("You cannot summon another familiar so soon.\n", &ch);
        return NULL;
    }

    // Scan the chars in the world, looking for any with familiar point-backs to this ch
    for (CHAR_DATA * vch(char_list); vch != NULL; vch = vch->next)
    {
        if (!IS_NPC(vch))
            continue;

        AFFECT_DATA * paf(get_affect(vch, sn));
        if (paf != NULL && paf->modifier == ch.id)
        {
            send_to_char("You already have a familiar.\n", &ch);
            return NULL;
        }
    }

    // Check the familiar slot on the ch
    if (get_eq_char(&ch, WEAR_FAMILIAR) != NULL)
    {
        send_to_char("You already have a familiar.\n", &ch);
        return NULL;
    }

    // Verify that the name is set
    if (ch.pcdata->familiarname == NULL)
    {
        // Check whether a name and gender were supplied
        char arg[MAX_STRING_LENGTH];
        target_name = one_argument(target_name, arg);
        if (target_name[0] == '\0')
        {
            send_to_char("You must supply a gender and name for your familiar.\n", &ch);
            return NULL;
        }

        // Get the gender
        if (!str_prefix(arg, "male")) ch.pcdata->familiargender = SEX_MALE;
        else if (!str_prefix(arg, "female")) ch.pcdata->familiargender = SEX_FEMALE;
        else
        {
            send_to_char("Please specify either 'male' or 'female' for the gender.\n", &ch);
            return NULL;
        }

        // Set the name
        copy_string(ch.pcdata->familiarname, target_name);
    }

    // The name and gender have been set, so summon the familiar
    CHAR_DATA * familiar(create_mobile(get_mob_index(vnum)));

    std::ostringstream nameString;
    nameString << familiar->name << ' ' << ch.pcdata->familiarname;
    setName(*familiar, nameString.str().c_str());
    
    familiar->level = level;
    familiar->max_hit = 100 + (level * 5);
    familiar->hit = familiar->max_hit;
    familiar->sex = ch.pcdata->familiargender;
    copy_string(familiar->short_descr, ch.pcdata->familiarname);

    char_to_room(familiar, ch.in_room);
    familiar->master = &ch;
    familiar->leader = &ch;
    familiar->memfocus[0] = &ch;

    // Apply cooldown and familiar effect
    int skill(get_skill(&ch, sn));
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 180 - UMAX(0, (skill - 40));
    affect_to_char(&ch, &af);

    af.duration = -1;
    af.location = APPLY_HIDE;
    af.modifier = ch.id;
    affect_to_char(familiar, &af);
    return familiar;
}

void check_scionofnight(CHAR_DATA & ch)
{
    // Check for transitioning from night
    if (time_info.hour == season_table[time_info.season].sun_up && !time_info.half)
    {
        // Check for effects in existence
        if (!is_affected(&ch, gsn_scionofnight))
            return;
        
        // Remove the effects
        send_to_char("The power of night fades from you as dawn breaks over the world.\n", &ch);
        affect_strip(&ch, gsn_scionofnight);
        return;
    }

    // Check for daytime
    if (time_info.hour >= season_table[time_info.season].sun_up && time_info.hour < season_table[time_info.season].sun_down)
        return;

    // Check for the skill
    int skill(get_skill(&ch, gsn_scionofnight));
    if (skill <= 0)
        return;

    // Check for transitioning to night
    if (time_info.hour == season_table[time_info.season].sun_down && !time_info.half)
        send_to_char("You are wreathed in power as the darkness of night steals over the land.\n", &ch);

    // Determine distance from midnight, measured in half-hours
    int gapHalves;
    if (time_info.hour < season_table[time_info.season].sun_up) 
        gapHalves = time_info.hour * 2 + (time_info.half ? 1 : 0);
    else
        gapHalves = NUM_HOURS_DAY - time_info.hour - (time_info.half ? 1 : 0);

    // Reduce the gap for higher skill, then bound it
    gapHalves -= UMAX(0, (skill - 60) / 10);
    gapHalves = URANGE(0, gapHalves, 12);
    int gapBonus = 12 - gapHalves;

    int aura_mod(aura_grade(&ch));
    aura_mod = URANGE(0, aura_mod, 4);

    // Replace the effects
    affect_strip(&ch, gsn_scionofnight);
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_scionofnight;
    af.level    = ch.level;
    af.duration = -1;
    af.location = APPLY_DAMROLL;
    af.modifier = (gapBonus / 3) + (aura_mod / 2);
    af.modifier = UMAX(3, af.modifier);
    affect_to_char(&ch, &af);
    
    af.location = APPLY_HITROLL;
    affect_to_char(&ch, &af);
    
    af.location = APPLY_RESIST_WEAPON;
    af.modifier = gapBonus + aura_mod - 6;
    af.modifier = UMAX(5, af.modifier);
    affect_to_char(&ch, &af);

    af.location = APPLY_RESIST_MAGIC;
    affect_to_char(&ch, &af);

    // Give chance of improvement
    check_improve(&ch, NULL, gsn_scionofnight, true, 8);
}

static void check_hatefire_damage(int sn, DamageInfo baseDam, CHAR_DATA * ch, CHAR_DATA * victim)
{
    // Prepare the damage
    std::vector<DamageInfo> damage;
    damage.push_back(baseDam);

    // Check hatefire
    if (number_percent() > get_skill(ch, gsn_hatefire))
    {
        // No hatefire, just deal the damage
        check_improve(ch, victim, gsn_hatefire, false, 4);
        damage_new(ch, victim, damage, sn, true);
        return;
    }

    // Handle hatefire
    check_improve(ch, victim, gsn_hatefire, true, 4);
    damage.push_back(DamageInfo(10 + number_range(0, ch->level / 5), DAM_FIRE));

    // Prepare hatefire effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_hatefire;
    af.duration = 1;
    af.level    = ch->level;
    af.location = APPLY_DAMROLL;
    af.modifier = 1;

    // Adjust effect for existing effect 
    AFFECT_DATA * hatefire(get_affect(ch, gsn_hatefire));
    if (hatefire == NULL)
        send_to_char("You begin to seethe with unnatural hatred!\n", ch);
    else
    {
        af.modifier += hatefire->modifier;
        af.modifier = UMIN(af.modifier, 30);
        affect_remove(ch, hatefire);
    }

    // Apply effect and deal damage
    affect_to_char(ch, &af);
    damage_new(ch, victim, damage, sn, true);
}

void do_fellpurpose(CHAR_DATA * ch, char * argument)
{
    // Check for the effect
    AFFECT_DATA * paf(get_affect(ch, gsn_fellpurpose));
    if (paf != NULL)
    {
        send_to_char("You cease drawing additional power into your spells.\n", ch);
        affect_remove(ch, paf);
        return;
    }

    // Check for the skill
    if (get_skill(ch, gsn_fellpurpose) <= 0)
    {
        send_to_char("Huh?\n", ch);
        return;
    }
    
    // Add the effect
    send_to_char("You prepare to draw additional power into your spells.\n", ch);
    
    expend_mana(ch, skill_table[gsn_fellpurpose].min_mana);
    WAIT_STATE(ch, skill_table[gsn_fellpurpose].beats);
    
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_fellpurpose;
    af.level    = ch->level;
    af.duration = -1;
    affect_to_char(ch, &af);
}

static bool should_trigger_corpsesense_helper(ROOM_INDEX_DATA & room, int trophy_vnum, int missingBit0, int missingBit1)
{
    for (OBJ_DATA * obj(room.contents); obj != NULL; obj = obj->next_content)
    {
        // Trigger if the trophied eye/ear is on the ground
        if (obj->pIndexData->vnum == trophy_vnum || obj->pIndexData->vnum == OBJ_VNUM_TROPHY_HEAD)
            return true;

        // Check for corpses
        if (obj->pIndexData->vnum != OBJ_VNUM_CORPSE_NPC && obj->pIndexData->vnum != OBJ_VNUM_CORPSE_PC)
            continue;

        // Check for things which destroy/invalidate the corpse
        if (IS_SET(obj->value[1], CORPSE_DESTROYED)) continue;
        if (IS_SET(obj->value[1], CORPSE_MISSING_HEAD)) continue;
        if (obj_is_affected(obj, gsn_decorporealize)) continue;
        if (IS_SET(obj->value[1], missingBit0) && IS_SET(obj->value[1], missingBit1)) continue;
        return true;
    }

    return false;
}

bool should_trigger_corpsesense_sight(ROOM_INDEX_DATA & room)
{
    return should_trigger_corpsesense_helper(room, OBJ_VNUM_TROPHY_EYE, CORPSE_MISSING_EYE1, CORPSE_MISSING_EYE2);
}

bool should_trigger_corpsesense_hearing(ROOM_INDEX_DATA & room)
{
    return should_trigger_corpsesense_helper(room, OBJ_VNUM_TROPHY_EAR, CORPSE_MISSING_EAR1, CORPSE_MISSING_EAR2);
}

void check_corpsesense_sight(CHAR_DATA & ch)
{
    // Ignore if ch is an NPC or IMM
    if (IS_NPC(&ch) || IS_IMMORTAL(&ch))
        return;

    // Inform all the appropriate PCs
    for (DESCRIPTOR_DATA * desc(descriptor_list); desc != NULL; desc = desc->next)
    {
        if (desc->connected != CON_PLAYING || desc->character->in_room == ch.in_room || !is_affected(desc->character, gsn_corpsesense))
            continue;

        if (number_percent() > get_skill(desc->character, gsn_corpsesense))
        {
            check_improve(desc->character, NULL, gsn_corpsesense, false, 2);
            continue;
        }

        check_improve(desc->character, NULL, gsn_corpsesense, true, 2);
        std::ostringstream buffer;
        buffer << "{WThe dead in " << ch.in_room->name << " have just seen $N.{x";
        act(buffer.str().c_str(), desc->character, NULL, &ch, TO_CHAR);
    }
}

void do_corpsesense(CHAR_DATA * ch, char * argument)
{
    // Check for the effect
    AFFECT_DATA * paf(get_affect(ch, gsn_corpsesense));
    if (paf != NULL)
    {
        send_to_char("You close your mind to the call of the dead.\n", ch);
        affect_remove(ch, paf);
        return;
    }

    // Check for the skill
    if (get_skill(ch, gsn_corpsesense) <= 0)
    {
        send_to_char("Huh?\n", ch);
        return;
    }
    
    // Check for sufficient mana
    if (ch->mana < skill_table[gsn_corpsesense].min_mana)
    {
        send_to_char("You are too tired to listen to the call of the dead.\n", ch);
        return;
    }

    // Add the effect
    send_to_char("You open your mind to the call of the dead.\n", ch);

    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_corpsesense;
    af.level    = ch->level;
    af.duration = -1;
    affect_to_char(ch, &af);

    // Charge mana and lag
    expend_mana(ch, skill_table[gsn_corpsesense].min_mana);
    WAIT_STATE(ch, skill_table[gsn_corpsesense].beats);
}

bool spell_callbat(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    CHAR_DATA * familiar(call_familiar(*ch, sn, level, MOB_VNUM_FAMILIAR_BAT));
    if (familiar == NULL)
        return false;

    act("$n glides into the room on silent, leathery wings.", familiar, NULL, NULL, TO_ROOM);
    return true;
}

bool spell_fadeshroud(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check whether currently affected
    if (is_affected(ch, sn))
    {
        affect_strip(ch, sn);
        send_to_char("You doff your fadeshroud, releasing your focus on the powers of darkness.\n", ch);
        act("$n seems to shimmer, then grow less faded.", ch, NULL, NULL, TO_ROOM);
        return false;
    }

    // Echoes
    if (ch->in_room == NULL || !room_is_dark(ch->in_room))
    {
        send_to_char("You prepare to cloak yourself in darkness, once you enter the gloom.\n", ch);
        act("A shadow steals over $n, leaving $m looking slightly faded.", ch, NULL, NULL, TO_ROOM);
    }
    else
    {
        send_to_char("You shroud yourself in the darkness, fading into the gloom.\n", ch);
        act("A darkness creeps over $n, and $e seems to fade into the gloom.", ch, NULL, NULL, TO_ROOM);
    }

    // Apply effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.location = APPLY_RESIST_LIGHT;
    af.modifier = -10;
    af.duration = -1;
    affect_to_char(ch, &af);
    return true;
}

static void act_to_room_observers(const char * format, CHAR_DATA & ch, OBJ_DATA * obj, CHAR_DATA & victim)
{
    // Make sure there is a room
    if (ch.in_room == NULL)
        return;

    // Iterate the others in the room
    for (CHAR_DATA * echo(ch.in_room->people); echo != NULL; echo = echo->next_in_room)
    {
        // Ignore the ch and those who cannot see the victim
        if (echo == &ch || !can_see(echo, &victim))
            continue;

        // Send the echo
        act(format, echo, obj, &ch, TO_CHAR);
    }
}

void check_deathlyvisage(CHAR_DATA & ch, CHAR_DATA & victim)
{
    // Only have a chance of firing
    if (number_percent() > 15)
        return;

    // Check for the object
    OBJ_DATA * obj(get_eq_char(&ch, WEAR_HOLD));
    if (obj == NULL)
        return;

    // Check for the effect
    AFFECT_DATA * paf(get_obj_affect(obj, gsn_deathlyvisage));
    if (paf == NULL)
        return;

    // Determine which spell will be cast
    int spellNum(number_range(0, 2));
    if (spellNum == 0 && !is_affected(&victim, gsn_deathlyvisage))
    {
        // Attempt to cast deathly visage
        act("$p shrieks hollowly at you!", &victim, obj, &victim, TO_CHAR);
        act("$p shrieks hollowly at $n!", &victim, obj, &victim, TO_ROOM);
        if (!saves_spell(paf->level, &ch, &victim, DAM_FEAR))
        {
            // Landed the effect
            send_to_char("Preternatural fear grips you, turning the world nightmarish!\n", &victim);
            act("$n appears stricken by fear!", &victim, NULL, NULL, TO_ROOM);

            AFFECT_DATA af = {0};
            af.where    = TO_AFFECTS;
            af.type     = gsn_deathlyvisage;
            af.level    = paf->level;
            af.duration = 2;
            affect_to_char(&victim, &af);
        }
        return;
    }

    if (spellNum == 1 && !is_affected(&victim, gsn_curse))
    {
        // Attempt to cast curse
        act("$p spits out a curse in a rasping, thready voice.", &ch, obj, NULL, TO_ALL);
        handle_curse_char(gsn_curse, paf->level, &ch, &victim);
        return;
    }

    // Cast enervating ray
    act("$p's eye sockets flicker to green, then emit a thin beam of light!", &ch, obj, NULL, TO_ALL);
    spell_enervatingray(gsn_enervatingray, paf->level, &ch, &victim, TARGET_CHAR);
}

bool spell_deathlyvisage(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Obtain the shade
    CHAR_DATA * shade(get_char_room(ch, target_name));
    if (shade == NULL)
    {
        send_to_char("You see no such spirit here.\n", ch);
        return false;
    }

    // Verify the shade
    if (!IS_NPC(shade) || !IS_SET(shade->nact, ACT_SHADE))
    {
        send_to_char("You may only bind shades and their ilk.\n", ch);
        return false;
    }

    // Obtain the skull
    OBJ_DATA * skull(get_eq_char(ch, WEAR_HOLD));
    if (skull == NULL || skull->pIndexData->vnum != OBJ_VNUM_TROPHY_SKULL)
    {
        act("You must be holding a proper skull to bind $N.", ch, NULL, shade, TO_CHAR);
        return false;
    }

    // Verify the skull level
    if (skull->level < shade->level)
    {
        act("$p is not powerful enough to contain $N!", ch, skull, shade, TO_CHAR);
        return false;
    }

    // Make sure the skull is not already inhabited
    if (obj_is_affected(skull, sn))
    {
        act("There is already a spirit bound to $p.", ch, skull, NULL, TO_CHAR);
        return false;
    }

    // Verify that the caster is not haunted
    if (is_affected(ch, gsn_shadeswarm))
    {
        send_to_char("The spirits haunting you rage against your magics, preventing you from binding one of their number!\n", ch);
        return false;
    }

    // Echoes
    act("You hold $p out towards $N, chanting intently.", ch, skull, shade, TO_CHAR);
    act("$n holds $p out towards $N, chanting intently.", ch, skull, shade, TO_ROOM);

    // Adjust level if a trigon is present
    if (is_symbol_present(*ch->in_room, OBJ_VNUM_SYMBOL_BIND))
        level += 10;

    // Check for a save
    if (saves_spell(level, ch, shade, DAM_NEGATIVE))
    {
        // Saved; check for shade destruction
        if (!saves_spell(level, ch, shade, DAM_NEGATIVE))
        {
            act("$N resists your attempt to bind $M to $p, tearing $Mself apart in the process!", ch, skull, shade, TO_CHAR);
            std::ostringstream mess;
            mess << "Wisps of essence drift off " << shade->short_descr << ", unravelling it completely in a matter of moments!";
            act_to_room_observers(mess.str().c_str(), *ch, NULL, *shade);
            extract_char(shade, true);
            return true;
        }

        // Check for haunting the caster
        if (!saves_spell(shade->level, shade, ch, DAM_ENERGY))
        {
            // Add the haunting effect
            AFFECT_DATA af = {0};
            af.where    = TO_AFFECTS;
            af.type     = gsn_shadeswarm;
            af.modifier = UMIN(20, 1 + shade->level / 3);
            af.duration = shade->timer / 5;
            af.level    = shade->level;
            affect_to_char(ch, &af);

            // Echoes
            act("$N is pulled towards $p, but at the last second breaks free of your power, vanishing into you instead!", ch, skull, shade, TO_CHAR);
            std::ostringstream mess;
            mess << shade->short_descr << " is pulled towards $p, but at the last second turns and vanishes into $N!";
            act_to_room_observers(mess.str().c_str(), *ch, skull, *shade);
            extract_char(shade, true);
            return true;
        }

        // Nothing special, just resisted
        act("$N resists your attempt to bind $M to $p.", ch, skull, shade, TO_CHAR);
        return true;
    }

    // Successful binding; echoes
    act("$N is drawn into $p as you bind $M with your power!", ch, skull, shade, TO_CHAR);
    std::ostringstream mess;
    mess << shade->short_descr << " is drawn towards $p, swiftly vanishing into it!";
    act_to_room_observers(mess.str().c_str(), *ch, skull, *shade);
    act("$p's empty eye sockets begin to glow a deep violet.", ch, skull, NULL, TO_ALL);

    // Add the effect
    AFFECT_DATA af = {0};
    af.where    = TO_OBJECT;
    af.type     = sn;
    af.location = APPLY_MANA;
    af.modifier = shade->level * 2;
    af.duration = shade->timer;
    af.level    = shade->level;
    af.bitvector = ITEM_EVIL|ITEM_HUM;
    affect_to_obj(skull, &af);

    extract_char(shade, true);
    return true;
}

int count_blackamulet(CHAR_DATA & ch)
{
    // Check for the effect
    if (!is_affected(&ch, gsn_blackamulet))
        return 0;

    // Count the worn items with the evil flag, subtracting those with the bless flag
    int count(0);
    for (OBJ_DATA * obj(ch.carrying); obj != NULL; obj = obj->next_content)
    {
        if (!obj->worn_on)
            continue;

        if (IS_OBJ_STAT(obj, ITEM_EVIL)) ++count;
        if (IS_OBJ_STAT(obj, ITEM_BLESS)) --count;
    }

    return count;
}

bool spell_blackamulet(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for groupmate
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));
    if (!is_same_group(ch, victim))
    {
        send_to_char("You may only apply such dark magics to groupmates.\n", ch);
        return false;
    }

    // Check for effect already present
    AFFECT_DATA * paf(get_affect(victim, sn));
    if (paf == NULL)
    {
        // Effect is absent; echo
        if (ch == victim) 
            send_to_char("You surround yourself in dark power, beginning to draw upon your unholy artifacts.\n", ch);
        else
        {
            act("You surround $N in dark power, allowing $M to draw on $S unholy artifacts.", ch, NULL, victim, TO_CHAR);
            act("$N surrounds you in dark power, allowing you to draw on your unholy artifacts.", ch, NULL, victim, TO_VICT);
        }

        // Apply the effect
        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.type     = sn;
        af.level    = level;
        af.duration = level;
        affect_to_char(victim, &af);
        return true;
    }

    // Effect is present; echo
    if (ch == victim) 
        send_to_char("You renew your will, continuing to draw on your unholy artifacts.\n", ch);
    else
    {
        act("You renew the dark power about $N, allowing $M to continue drawing on $S unholy artifacts.\n", ch, NULL, victim, TO_CHAR);
        act("$n renews the dark power about you, allowing you to continue drawing on your unholy artifacts.\n", ch, NULL, victim, TO_VICT);
    }

    // Renew the duration
    paf->duration = UMAX(paf->duration, level);
    return true;
}

bool spell_darktallow(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Make sure the object is a trophied finger
    OBJ_DATA * finger(static_cast<OBJ_DATA*>(vo));
    if (finger->pIndexData->vnum != OBJ_VNUM_TROPHY_FINGER)
    {
        send_to_char("You can only fashion darktallows from fingerbones.\n", ch);
        return false;
    }

    // Make sure there isn't already something floating in the caster's slot
    OBJ_DATA * floating(get_eq_char(ch, WEAR_FLOAT));
    if (floating != NULL)
    {
        act("You already have $p floating alongside you.", ch, floating, NULL, TO_CHAR);
        return false;
    }

    // Make the darktallow
    OBJ_DATA * obj(create_object(get_obj_index(OBJ_VNUM_DARKTALLOW), 0));
    if (obj == NULL)
    {
        bug("Failed to create darktallow candle.", 0);
        send_to_char("A problem has occurred. Please contact the gods.\n", ch);
        return false;
    }

    // Echoes
    act("You score a jagged rune into $p, infusing it with the power of night!", ch, obj, NULL, TO_CHAR);
    act("$n scores a jagged rune into $p, which pulses with fell power!", ch, obj, NULL, TO_ROOM);
    act("$p lofts gently into the air, floating about and flickering darkly.", ch, obj, NULL, TO_ALL);

    // Initialize and equip the darktallow
    obj->level = UMIN(level, finger->level);
    obj->timer = obj->level;
    obj_to_char(obj, ch);
    equip_char(ch, obj, WORN_FLOAT);

    // Destroy the fingerbone
    extract_obj(finger);
    return true;
}

bool check_wreathoffear(CHAR_DATA & ch, CHAR_DATA & victim)
{
    // Certain NPCs are exempt
    if (IS_NPC(&ch) && (IS_SET(ch.act, ACT_NOSUBDUE) || IS_SET(ch.act, ACT_SENTINEL) || IS_SET(ch.nact, ACT_SHADE)))
        return false;

    // Seek a wreath of fear effect
    for (AFFECT_DATA * paf(get_affect(&victim, gsn_wreathoffear)); paf != NULL; paf = paf->next)
    {
        // Ignore inactive effects
        if (paf->location != APPLY_CHR)
            continue;
        
        // Found the effect
        if (saves_spell(paf->level, &victim, &ch, DAM_FEAR))
            return false;

        // Failed the save; echo and force a flee attempt
        act("The aura of fear surrounding $N overwhelms you, leaving you recoiling in panic!", &ch, NULL, &victim, TO_CHAR);
        act("$n recoils away from you, looking briefly panicked!", &ch, NULL, &victim, TO_VICT);
        act("$n recoils away from $N, looking briefly panicked!", &ch, NULL, &victim, TO_NOTVICT);
        do_flee(&ch, "");
        return (ch.in_room != victim.in_room);
    }

    return false;
}

bool spell_wreathoffear(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You cannot wreath yourself in another aura of fear so soon.\n", ch);
        return false;
    }

    // Echoes
    act("You murmur softly, and are wreathed in a twisted aura of fear and panic.", ch, NULL, NULL, TO_CHAR);
    act("$n seems to grow fearsome, taking on a terrifying aspect!", ch, NULL, NULL, TO_ROOM);

    // Apply effect
    AFFECT_DATA af = {0};
    af.type     = sn;
    af.where    = TO_AFFECTS;
    af.level    = level;
    af.location = APPLY_CHR;
    af.modifier = 1;
    af.duration = level / 10;
    affect_to_char(ch, &af);

    // Apply cooldown
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.duration = 20;
    affect_to_char(ch, &af);

    return true;
}

bool spell_scritofkaagn(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Verify that the object is a scroll
    OBJ_DATA * obj(static_cast<OBJ_DATA*>(vo));
    if (obj->item_type != ITEM_SCROLL)
    {
        send_to_char("The scrit of Kaagn may only be applied to scrolls.\n", ch);
        return false;
    }

    // Verify that the scroll hasn't already been empowered
    if (obj_is_affected(obj, sn))
    {
        act("$p has already been empowered by Kaagn's dark scrit.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Increase scroll level
    int skill(get_skill(ch, sn));
    int maxBoost(0);
    maxBoost += UMAX(0, (level - 35) / 4);
    maxBoost += UMAX(0, (skill - 70) / 10);
    obj->value[0] += 5 + number_range(0, maxBoost);

    // Apply effect to prevent recasting
    AFFECT_DATA af = {0};
    af.where    = TO_OBJECT;
    af.type     = sn;
    af.level    = level;
    af.duration = -1;
    affect_to_obj(obj, &af);
    
    // Echoes and caster damage
    act("You slice your finger open, tracing over the arcane script on $p in your own blood.", ch, obj, NULL, TO_CHAR);
    act("$n slices $s finger open, tracing over the arcane script on $p in $s own blood.", ch, obj, NULL, TO_ROOM);
    act("A dark energy courses once over $p, then vanishes.", ch, obj, NULL, TO_ALL); 
    damage_old(ch, ch, number_range(1, 5), sn, DAM_SLASH, true);
    
    return true;
}

static int harvest_soul(CHAR_DATA & ch, CHAR_DATA & shade, bool & mainEcho)
{
    // Harvest the shade
    if (can_see(&ch, &shade))
    {
        mainEcho = true;
        act("Wisps of $N spindle off into you, vanishing as you harvest $S very spirit!", &ch, NULL, &shade, TO_CHAR);
    }

    std::ostringstream buf;
    buf << "Wisps of " << shade.short_descr << " spindle off into $n, vanishing as the spirit unravels!";
    act_to_room_observers(buf.str().c_str(), ch, NULL, shade);

    // Add to the bonus and destroy the shade
    int bonus(5 + (shade.level / 3));
    extract_char(&shade, true);
    return bonus;
}

static void finish_harvest(CHAR_DATA & ch, int bonus, bool mainEcho)
{
    // Make sure the harvester got some echo
    if (!mainEcho)
        send_to_char("You suddenly feel stronger, and realize you have harvested an unworldly energy.\n", &ch);

    // Add the bonus
    for (AFFECT_DATA * paf(get_affect(&ch, gsn_harvestofsouls)); paf != NULL; paf = get_affect(&ch, gsn_harvestofsouls, paf))
    {
        switch (paf->location)
        {
            case APPLY_HIT:     paf->modifier += bonus; ch.max_hit += bonus;    ch.hit += bonus;    break;
            case APPLY_MANA:    paf->modifier += bonus; ch.max_mana += bonus;   ch.mana += bonus;   break;
        }
    }
}

static int harvest_level(CHAR_DATA & ch)
{
    for (AFFECT_DATA * paf(get_affect(&ch, gsn_harvestofsouls)); paf != NULL; paf = get_affect(&ch, gsn_harvestofsouls, paf))
    {
        if (paf->location == APPLY_NONE)
            return paf->level;
    }

    return -1;
}

bool check_harvestofsouls(CHAR_DATA & ch)
{
    // Make sure the char is in a room
    if (ch.in_room == NULL)
        return false;

    // Check whether the char is a shade
    bool mainEcho(false);
    if (IS_NPC(&ch) && IS_SET(ch.nact, ACT_SHADE))
    {
        // Look for a harvesting person in the room
        for (CHAR_DATA * vch(ch.in_room->people); vch != NULL; vch = vch->next_in_room)
        {
            if (IS_NPC(vch) && IS_SET(ch.nact, ACT_SHADE))
                continue;

            if (ch.level >= harvest_level(*vch))
                continue;

            int bonus(harvest_soul(*vch, ch, mainEcho));
            finish_harvest(*vch, bonus, mainEcho);
            return true;
        }

        return false;
    }

    // Make sure the char is harvesting
    int harvestLevel(harvest_level(ch));
    if (harvestLevel < 0)
        return false;

    // Find the shades in the room
    int bonus(0);
    CHAR_DATA * vch_next;
    for (CHAR_DATA * vch(ch.in_room->people); vch != NULL; vch = vch_next)
    {
        // Ignore non-shades and invalid levels
        vch_next = vch->next_in_room;
        if (vch != &ch && IS_NPC(vch) && IS_SET(vch->nact, ACT_SHADE) && harvestLevel > vch->level)
            bonus += harvest_soul(ch, *vch, mainEcho);
    }

    // Finish the harvest, assuming there were shades at all
    if (bonus > 0)
        finish_harvest(ch, bonus, mainEcho);

    return false;
}

bool spell_harvestofsouls(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You cannot harvest more souls again so soon.\n", ch);
        return false;
    }

    // Echoes
    send_to_char("An unearthly hollowness fills you, lending you a dread appetite.\n", ch);

    // Apply effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.location = APPLY_NONE;
    af.duration = 2 + (number_percent() <= get_skill(ch, sn) ? 1 : 0);
    affect_to_char(ch, &af);
    
    af.location = APPLY_HIT;
    af.duration = 60;
    affect_to_char(ch, &af);

    af.location = APPLY_MANA;
    affect_to_char(ch, &af);

    // Perform any initial harvesting
    check_harvestofsouls(*ch);
    return true;
}

bool spell_unholymight(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Prepare the base effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 36;

    // Apply the effect to all groupmates
    for (CHAR_DATA * vch(ch->in_room->people); vch != NULL; vch = vch->next_in_room)
    {
        // Ignore non-groupmates
        if (!is_same_group(ch, vch))
            continue;

        // Replacing existing effect
        affect_strip(vch, sn);
        act("A dark aura surrounds $n, crackling with unholy power.", vch, NULL, NULL, TO_ROOM);
       
        int grade(aura_grade(vch));
        if (grade < 0) 
            act("The dark aura surrounds you, surrounds you, filling you with a sense of emptiness.", vch, NULL, NULL, TO_CHAR);
        else
            act("A dark aura surrounds you, filling you with an unholy power!", vch, NULL, NULL, TO_CHAR);

        // Calculate the modifier according to karma
        af.modifier = ((grade < 0) ? 0 : UMAX(3, grade + 1));
        af.location = APPLY_DAMROLL;
        affect_to_char(vch, &af);

        af.location = APPLY_HITROLL;
        affect_to_char(vch, &af);
    }

    return true;
}

static void checkMakeZombie(ROOM_INDEX_DATA & room, int level, OBJ_DATA & obj)
{
    // Chance of nothing happening
    if (number_bits(1) != 0)
        return;

    // Check for things which destroy/invalidate the corpses
    if (IS_SET(obj.value[1], CORPSE_DESTROYED)) return;
    if (IS_SET(obj.value[1], CORPSE_MISSING_HEAD)) return;
    if (obj_is_affected(&obj, gsn_decorporealize)) return;

    // Look for existing effect on the corpse
    if (number_bits(1) == 0 && !obj_is_affected(&obj, gsn_barrowmist))
    {
        // No effect yet, so add one
        AFFECT_DATA af = {0};
        af.where    = TO_OBJECT;
        af.type     = gsn_barrowmist;
        af.level    = level;
        af.duration = -1;
        affect_to_obj(&obj, &af);

        // Echo about it
        switch (number_range(0, 2))
        {
            case 0: act("$p twitches spastically, limbs jerking and flailing!", room.people, &obj, NULL, TO_ALL); break;
            case 1: act("$p spasms suddenly, then falls still once more.", room.people, &obj, NULL, TO_ALL); break;
            default: act("A faint moaning sound issues from $p, followed by silence.", room.people, &obj, NULL, TO_ALL); break;
        }
        return;
    }

    // Corpse is ready to become a zombie; echo about it
    switch (number_range(0, 2))
    {
        case 0: act("$p suddenly lurches upright, staggering on decaying limbs!", room.people, &obj, NULL, TO_ALL); break;
        case 1: act("A soft groan escapes from $p as it comes suddenly to life!", room.people, &obj, NULL, TO_ALL); break;
        default: act("$p suddenly rises, shambling about with a blank expression!", room.people, &obj, NULL, TO_ALL); break;
    }

    // Make the zombie
    CHAR_DATA * zombie(create_mobile(get_mob_index(MOB_VNUM_BARROWMISTZOMBIE)));
    zombie->level = (level + obj.level) / 2;
    zombie->damroll = 10;
    zombie->hitroll = zombie->level;
    zombie->damage[0] = UMAX(4, (zombie->level * 4) / 10);
    zombie->damage[1] = 6;
    zombie->damage[2] = zombie->damroll;
    zombie->max_hit = dice(20, level) + 200;
    zombie->hit = zombie->max_hit;

    // Desc the zombie
    std::ostringstream desc;
    desc << "Staring blankly about itself, this withered, rotting corpse shambles along aimlessly.";
    desc << "Though its appearance is reminescent of ";
    
    if (obj.pIndexData->vnum == OBJ_VNUM_CORPSE_NPC) desc << get_mob_index(obj.value[0])->short_descr;         // Mob's short desc
    else desc << indefiniteArticleFor(*race_table[obj.value[2]].name) << " " << race_table[obj.value[2]].name;  // Race name
    
    desc << ", the extent of the decay has erased nearly all trace of its former existence. A faint, dark";
    desc << " mist roils from the zombie's mouth each time it moans softly.";

    char * preformatDesc(str_dup(desc.str().c_str()));
    char * formattedDesc(format_string(preformatDesc));
    copy_string(zombie->description, formattedDesc);
    free_string(formattedDesc);

    // Add the zombie to the room, destroying the corpse
    extract_obj(&obj);
    char_to_room(zombie, &room);
}

static void checkMakeWight(ROOM_INDEX_DATA & room, int level, CHAR_DATA & ch)
{
    // Chance of nothing happening; eviler shades are more likely to turn
    unsigned int bits(4);
    if (IS_GOOD(&ch)) bits += 2;
    else if (IS_NEUTRAL(&ch)) ++bits;
    if (number_bits(bits) != 0) 
        return;

    // Shade is ready to become a wight; echo about it
    switch (number_range(0, 2))
    {
        case 0: act("The deathly mists coalesce suddenly about $n, bringing $m back as a wight!", &ch, NULL, NULL, TO_ROOM); break;
        case 1: act("$n solidifies from the dark mists, taking on the form of a wight!", &ch, NULL, NULL, TO_ROOM); break;
        default: act("The roiling mists wreath $n in negative power, drawing $m onto the physical plane!", &ch, NULL, NULL, TO_ROOM); break;
    }

    // Make the wight
    CHAR_DATA * wight(create_mobile(get_mob_index(MOB_VNUM_BARROWMISTWIGHT)));
    wight->level = (ch.level + level) / 2;
    wight->damroll = 10;
    wight->hitroll = wight->level;
    wight->damage[0] = UMAX(4, (wight->level * 4) / 10);
    wight->damage[1] = 6;
    wight->damage[2] = wight->damroll;
    wight->max_hit = dice(10, level) + 100;
    wight->hit = wight->max_hit;

    // Add the wight to the room, destroying the shade
    extract_char(&ch, true);
    char_to_room(wight, &room);
}

void handleUpdateBarrowmist(ROOM_INDEX_DATA & room, AFFECT_DATA & paf)
{
    // Iterate the objects in the room, looking for corpses
    OBJ_DATA * obj_next;
    for (OBJ_DATA * obj(room.contents); obj != NULL; obj = obj_next)
    {
        obj_next = obj->next_content;
        if (obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC || obj->pIndexData->vnum == OBJ_VNUM_CORPSE_NPC)
            checkMakeZombie(room, paf.level, *obj);
    }

    // Iterate the people in the room, looking for shades
    CHAR_DATA * ch_next;
    for (CHAR_DATA * ch(room.people); ch != NULL; ch = ch_next)
    {
        ch_next = ch->next_in_room;
        if (IS_NPC(ch) && IS_SET(ch->nact, ACT_SHADE))
            checkMakeWight(room, paf.level, *ch);
    }
}

bool spell_barrowmist(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity-check
    if (ch->in_room == NULL)
        return false;

    // Check for cooldown
    for (AFFECT_DATA * paf(get_affect(ch, sn)); paf != NULL; paf = get_affect(ch, sn, paf))
    {
        if (paf->modifier == 0)
        {
            send_to_char("You are not yet ready to summon another barrowmist.\n", ch);
            return false;
        }
    }

    // Check for area already having barrowmist
    if (area_is_affected(ch->in_room->area, sn))
    {
        send_to_char("The mists of undeath already fill this place.\n", ch);
        return false;
    }

    // Echo
    send_to_area("A thick, dark mist spreads through this place, reeking of death and decay.\n", *ch->in_room->area);

    // Apply cooldown
    int skill(get_skill(ch, sn));
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 150 - UMAX(0, skill - 70);
    affect_to_char(ch, &af);

    // Apply effect
    af.where    = TO_AREA;
    af.duration = 6 + (level / 5);
    affect_to_area(ch->in_room->area, &af);

    return true;
}

void handle_phylactery_destruction(CHAR_DATA & ch, OBJ_DATA & obj)
{
    // Check for phylactery
    AFFECT_DATA * paf(get_obj_affect(&obj, gsn_imbuephylactery));
    if (paf == NULL)
        return;

    // Echo
    act("You breathe in the dark fog which roils suddenly out of $p!", &ch, &obj, NULL, TO_CHAR);
    act("A dark fog suddenly roils out of $p, and $n breaths it in!", &ch, &obj, NULL, TO_ROOM);

    // Check for caster
    if (!IS_NPC(&ch) && paf->modifier == ch.id)
    {
        send_to_char("You feel your soul knit itself back together, once again made whole.\n", &ch);
        affect_strip(&ch, gsn_imbuephylactery);
        ch.pcdata->death_count = UMAX(0, ch.pcdata->death_count - 1);
        return;
    }

    // Not the caster, so apply a curse
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_curse;
    af.level    = paf->level;
    af.duration = -1;
    af.bitvector = AFF_CURSE;
    af.location = APPLY_HITROLL;
    af.modifier = -1 * (paf->level / 8);
    affect_to_char(&ch, &af);

    af.location = APPLY_SAVING_SPELL;
    af.modifier = paf->level / 8;
    affect_to_char(&ch, &af);

    send_to_char("A dark curse grips you!\n", &ch);
    act("$n looks very uncomfortable.", &ch, NULL, NULL, TO_ROOM);
 
    // Check for whether the caster is in the world
    CHAR_DATA * caster(get_char_by_id(paf->modifier));
    if (caster == NULL)
        return;

    // Inform wiznet
    std::ostringstream logmess;
    logmess << ch.name << " just viewed " << caster->name << "'s past by destroying a phylactery";
    wiznet(const_cast<char*>(logmess.str().c_str()), NULL, NULL, WIZ_ACTIVITY, 0, 0);
    log_string(logmess.str().c_str());
    
    // Caster is in the world, so give the destroyer a vision of the caster's bg
    std::vector<std::string> words;
    std::istringstream wordParser(caster->pcdata->background);
    while (!wordParser.eof())
    {
        std::string word;
        wordParser >> word;
        if (!word.empty())
            words.push_back(word);
    }

    // Handle the case of no bg
    if (words.empty())
    {
        send_to_char("A vision appears to you, but it is so bland and unremarkable that you quickly forget it.\n", &ch);
        return;
    }

    // Choose a random phrase
    int phraseStart(number_range(0, words.size() - 1));
    int phraseLength(number_range(12, 36));
    phraseLength = UMIN(phraseLength, static_cast<int>(words.size()) - phraseStart); 

    // Generate the phrase
    std::ostringstream phrase;
    phrase << "{W...";
    for (int i(0); i < phraseLength; ++i)
    {
        if (i != 0) phrase << ' ';
        phrase << words[phraseStart + i];
    }
    phrase << "...{x\n";

    // Send the echoes
    send_to_char("A vision appears before you, a hazy series of images...\n", &ch);
    send_to_char(phrase.str().c_str(), &ch);
    send_to_char("The moment passes, leaving your vision clear once more.\n", &ch);
}

void do_repossess(CHAR_DATA * ch, char * argument)
{
    // Check for ghost
    if (!IS_OAFFECTED(ch, AFF_GHOST))
    {
        send_to_char("You are no spirit.\n", ch);
        return;
    }

    // Check for the proper effect for repossession
    AFFECT_DATA * paf(get_affect(ch, gsn_ghost));
    if (paf == NULL || paf->modifier != 1 || IS_NPC(ch) || ch->pcdata->age_group == AGE_DEAD)
    {
        send_to_char("You lack the power to repossess your body.\n", ch);
        return;
    }

    // Check to make sure enough has passed on the resurrection timer
    if (paf->duration >= 30)
    {
        send_to_char("You are still in shock from your death, and cannot repossess your body yet.\n", ch);
        return;
    }

    // Find the associated corpse
    OBJ_DATA * corpse;
    for (corpse = (ch->in_room == NULL ? NULL : ch->in_room->contents); corpse != NULL; corpse = corpse->next_content)
    {
        if (corpse->item_type == ITEM_CORPSE_PC && !str_cmp(corpse->owner, ch->name))
            break;
    }

    if (corpse == NULL)
    {
        send_to_char("Your corpse is not here.\n", ch);
        return;
    }

    // Check for destroyed corpse
    if (IS_SET(corpse->value[1], CORPSE_DESTROYED))
    {
        send_to_char("Your corpse has been destroyed, and is no longer fit for repossession.\n", ch);
        return;
    }

    // Echoes
    act("You slide back into your body, limbs twitching as you slowly regain control of it.", ch, NULL, NULL, TO_CHAR);
    act("$n slides back into $s body, gasping for breath.", ch, NULL, NULL, TO_ROOM);
   
    // Wiznet notification 
    char buf[MAX_STRING_LENGTH];
    sprintf(buf,"%s repossessed his/her body in room %d.", ch->name, ch->in_room->vnum);
    wiznet(buf, ch, NULL, WIZ_DEATHS, 0, 0);
    log_string(buf);

    // Restore objects/coins
    OBJ_DATA * obj_next;
    for (OBJ_DATA * obj(corpse->contains); obj != NULL; obj = obj_next)
    {
        obj_next = obj->next_content;
        if (obj->item_type == ITEM_MONEY)
        {
            // Money type objects just get converted to coins
            coins_to_char(ch, obj->value[0], obj->value[1]);
            extract_obj(obj);
            continue;
        }

        // Normal objects move to ch
        obj_from_obj(obj);
        obj_to_char(obj, ch);
    }

    // Finish the "resurrection"
    affect_strip(ch, gsn_ghost);
    save_char_obj(ch);
    extract_obj(corpse);
}

bool spell_imbuephylactery(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("Your soul is still recovering from its last severing.\n", ch);
        return false;
    }

    // Check the object material
    static const int crystal(material_lookup("crystal"));
    static const int diamond(material_lookup("diamond"));
    OBJ_DATA * obj(static_cast<OBJ_DATA*>(vo));
    if (obj->material != crystal && obj->material != diamond)
    {
        send_to_char("Only a vessel of crystal or diamond can properly store a piece of your soul.\n", ch);
        return false;
    }

    // Check for existing phylactery effect
    if (obj_is_affected(obj, sn))
    {
        act("$p already contains a piece of soul.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Check for sufficient deaths left
    if (IS_NPC(ch) || (ch->pcdata->death_count + 1) >= ch->pcdata->max_deaths)
    {
        send_to_char("Your soul is too weak to sever a piece of it.\n", ch);
        return false;
    }

    // Echoes
    act("Your soul cries out in anguish as you tear off a piece, willing it into $p!", ch, obj, NULL, TO_CHAR);
    act("$p darkens perceptibly, as though filled with a black fog.", ch, obj, NULL, TO_ROOM);

    // Charge the death and adjust karma
    ++ch->pcdata->death_count;
    modify_karma(ch, 100);

    // Apply effect
    AFFECT_DATA af = {0};
    af.where    = TO_OBJECT;
    af.type     = sn;
    af.level    = level;
    af.duration = -1;
    af.modifier = ch->id;
    af.bitvector = ITEM_HUM | ITEM_DARK | (aura_grade(ch) >= 2 ? ITEM_EVIL : 0);
    affect_to_obj(obj, &af);

    // Apply cooldown and mana loss
    af.where    = TO_AFFECTS;
    af.duration = 48;
    af.location = APPLY_MANA;
    af.modifier = -100;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    return true;
}

void check_bonereaper_mana(CHAR_DATA & ch, int & mana)
{
    // Check for bone reaper
    OBJ_DATA * obj(get_eq_char(&ch, WEAR_WIELD));
    if (obj == NULL || (obj->pIndexData->vnum != OBJ_VNUM_BONESCYTHE && obj->pIndexData->vnum != OBJ_VNUM_BONESICKLE))
        return;
   
    // Check for some effect left              
    AFFECT_DATA * paf(get_obj_affect(obj, gsn_reaping));
    if (paf == NULL)
        return;
    
    // Decrease the mana cost by up to half, drawing on the life in the bone reaper
    int lifeCost(URANGE(0, mana / 2, paf->modifier));
    mana -= lifeCost;
    paf->modifier -= lifeCost;
    
    // Echo and possible remove effect
    if (paf->modifier > 0)
        act("You draw life force from $p.", &ch, obj, NULL, TO_CHAR);
    else
    {
        act("You draw the last of the life force from $p, draining it completely.", &ch, obj, NULL, TO_CHAR);
        affect_remove_obj(obj, paf);
    }
}

void check_bonereaper(CHAR_DATA & victim)
{
    // Check disqualifiers
    if (victim.in_room == NULL) return;
    if (IS_NPC(&victim) && IS_SET(victim.act, ACT_UNDEAD)) return;
    if (is_demon(&victim)) return;

    // Determine the total life force to be reaped
    int life(victim.level * victim.level);
    if (!IS_NPC(&victim) || IS_SET(victim.act, ACT_BADASS)) life *= 8;
    life /= 10;

    // Collect all the wielded bonereapers in the room
    std::vector<OBJ_DATA*> objs;
    for (CHAR_DATA * ch(victim.in_room->people); ch != NULL; ch = ch->next_in_room)
    {
        if (ch == &victim)
            continue;

        OBJ_DATA * obj(get_eq_char(ch, WEAR_WIELD));
        if (obj != NULL && (obj->pIndexData->vnum == OBJ_VNUM_BONESCYTHE || obj->pIndexData->vnum == OBJ_VNUM_BONESICKLE))
            objs.push_back(obj);
    }

    if (objs.empty())
        return;

    // Apply the life force to each reaper
    life /= objs.size();
    for (size_t i(0); i < objs.size(); ++i)
    {
        act("A faint white mist streams from you into $p.", &victim, objs[i], NULL, TO_CHAR);
        act("A faint white mist streams from $n into $p.", &victim, objs[i], NULL, TO_ROOM);

        // Check for existing effect
        AFFECT_DATA * paf(get_obj_affect(objs[i], gsn_reaping));
        if (paf == NULL)
        {
            // Effect does not exist, so add it
            AFFECT_DATA af = {0};
            af.where    = TO_OBJECT;
            af.type     = gsn_reaping;
            af.level    = objs[i]->level;
            af.duration = -1;
            af.modifier = life;
            af.bitvector = ITEM_HUM;
            affect_to_obj(objs[i], &af);
        }
        else
            paf->modifier += life;
    }
}

bool spell_reaping(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready for another reaping.\n", ch);
        return false;
    }

    // Verify the corpse
    OBJ_DATA * corpse(static_cast<OBJ_DATA*>(vo));
    if (obj_is_affected(corpse, gsn_decorporealize) || (corpse->pIndexData->vnum != OBJ_VNUM_CORPSE_PC && corpse->pIndexData->vnum != OBJ_VNUM_CORPSE_NPC))
    {
        act("You can't extract bones from $p.", ch, corpse, NULL, TO_CHAR);
        return false;
    }

    // Verify the corpse has not been destroyed
    if (IS_SET(corpse->value[1], CORPSE_DESTROYED))
    {
        send_to_char("That corpse has already been torn apart.\n", ch);
        return false;
    }

    // Check for sickle/scythe argument
    int vnum;
    if (!str_prefix(target_name, "sickle")) vnum = OBJ_VNUM_BONESICKLE;
    else if (!str_prefix(target_name, "scythe")) vnum = OBJ_VNUM_BONESCYTHE;
    else
    {
        send_to_char("What kind of bone reaper do you want to make?\n", ch);
        return false;
    }

    // Make the object
    OBJ_DATA * obj(create_object(get_obj_index(vnum), level));
    if (obj == NULL)
    {
        bug("Spell: bone reaper; cannot load object", 0);
        send_to_char("A problem has occurred, please contact the gods.\n", ch);
        return false;
    }

    // Fill out the weapon
    int effectiveLevel(UMIN(corpse->level - 5, level));
    int diceCount(12);
    if (effectiveLevel >= 30) ++diceCount;
    if (effectiveLevel >= 40) ++diceCount;
    if (effectiveLevel >= 48) ++diceCount;
    if (effectiveLevel >= 53) ++diceCount;
    
    obj->level = UMIN(effectiveLevel, LEVEL_HERO);
    obj->value[1] = diceCount;
    obj->value[2] = 2;
    obj_to_char(obj, ch);

    // Echoes
    act("You tear $p apart, extracting its bones.", ch, corpse, NULL, TO_CHAR);
    act("$n tears $p apart, extracting its bones.", ch, corpse, NULL, TO_ROOM);
    act("The bones twist and warp, fusing into $p!", ch, obj, NULL, TO_ALL);

    // Destroy the corpse and apply cooldown
    desiccate_corpse(corpse);

    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 120;
    affect_to_char(ch, &af);
    return true;
}

static void apply_deathswarm(CHAR_DATA & ch, CHAR_DATA & undead, CHAR_DATA & victim)
{
    affect_strip(&undead, gsn_deathswarm);

    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_deathswarm;
    af.level    = ch.level;
    af.duration = -1;
    af.modifier = ch.id;
    affect_to_char(&undead, &af);

    stop_fighting(&undead);
    set_fighting(&undead, &victim);
}

bool spell_deathswarm(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity check
    if (ch->in_room == NULL)
        return false;

    // Adjust level for evilness bonus
    level += aura_grade(ch);

    // Check each undead in the room
    bool anyUndead(false);
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));
    for (CHAR_DATA * undead(ch->in_room->people); undead != NULL; undead = undead->next_in_room)
    {
        // Ignore non-undead or already fighting
        if (!IS_NPC(undead) || !IS_SET(undead->act, ACT_UNDEAD))
            continue;

        // Check for a save; nosubdue save automatically
        anyUndead = true;
        if (IS_SET(undead->act, ACT_NOSUBDUE) || saves_spell(level, ch, undead, DAM_OTHER))
        {
            // A secondary save means the caster is attacked
            if (saves_spell(level, ch, undead, DAM_OTHER))
            {
                act("Your will falters, and $N twists about awkwardly, coming at you!", ch, NULL, undead, TO_CHAR);
                act("$N twists about awkwardly, coming at $n!", ch, NULL, undead, TO_ROOM);
                apply_deathswarm(*ch, *undead, *ch);
                continue;
            }

            // Resisted, but will not attack caster
            act("$N snarls and moans, resisting your power.", ch, NULL, undead, TO_CHAR);
            act("$N snarls and moans, shambling about.", ch, NULL, undead, TO_ROOM);
            continue;
        }

        // Failed to save; undead will attack victim
        if (undead == victim)
        {
            act("$N tears off a small piece of $M own flesh, then stares blankly at you.", ch, NULL, undead, TO_CHAR);
            act("$N tears off a small piece of $M own flesh, then stares blankly at $n.", ch, NULL, undead, TO_ROOM);
        }
        else
        {
            act("$N lurches suddenly towards you!", victim, NULL, undead, TO_CHAR);
            act("$N suddenly lurches towards $n!", victim, NULL, undead, TO_ROOM);
            apply_deathswarm(*ch, *undead, *victim);
        }
    }

    // Check for any undead
    if (anyUndead)
        return true;
    
    send_to_char("There are no undead here to command.\n", ch);
    return false;
}

void check_direfeast(CHAR_DATA * ch, CHAR_DATA & victim, int dam)
{
    // Check for undead and basic sanity checks
    if (ch == NULL || ch->in_room == NULL || dam <= 0 || !IS_NPC(ch) || !IS_SET(ch->act, ACT_UNDEAD))
        return;

    // Make sure the victim is not undead
    if ((IS_NPC(&victim) && IS_SET(victim.act, ACT_UNDEAD)) || get_skill(&victim, gsn_revenant) > 0)
        return;

    // Build a list of direfeasters in the room
    std::vector<CHAR_DATA*> feasters;
    for (CHAR_DATA * feaster(ch->in_room->people); feaster != NULL; feaster = feaster->next_in_room)
    {
        if (feaster != &victim && feaster != ch && feaster->hit < feaster->max_hit && is_affected(feaster, gsn_direfeast))
            feasters.push_back(feaster);
    }

    if (feasters.empty())
        return;

    // Divide out the damage amongst the feasters
    dam = (dam * 20) / (feasters.size() * 100);
    dam = UMAX(dam, 1);

    // Grant the healing to the feasters
    for (unsigned int i(0); i < feasters.size(); ++i)
    {
        send_to_char("Energy flows along the swirl of violet magic, restoring you.\n", feasters[i]);
        feasters[i]->hit += dam;
        feasters[i]->hit = UMIN(feasters[i]->hit, feasters[i]->max_hit); 
    }
}

bool spell_direfeast(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for already up
    if (is_affected(ch, sn))
    {
        send_to_char("You are already drawing in life force.\n", ch);
        return false;
    }

    // Echos
    act("A swirl of dark violet magic surrounds you.", ch, NULL, NULL, TO_CHAR);
    act("A swirl of dark violet magic surrounds $n.", ch, NULL, NULL, TO_ROOM);

    // Apply effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = (level / 12);
    affect_to_char(ch, &af);

    return true;
}

static void apply_duskfall(AREA_DATA & area, AFFECT_DATA & af)
{
    // Check whether the area is already duskfalled
    if (area_is_affected(&area, gsn_duskfall))
        return;

    // Apply effect and echo
    affect_to_area(&area, &af);
    send_to_area("An unnatural darkness settles grimly over the area.\n", area);
}

bool spell_duskfall(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity-check
    if (ch->in_room == NULL)
        return false;

    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to bring about another duskfall.\n", ch);
        return false;
    }

    // Echo, then prepare the effect
    send_to_char("You draw upon the powers of night, seeking to plunge this place into shadow!\n", ch);
    AFFECT_DATA af = {0};
    af.where    = TO_AREA;
    af.type     = sn;
    af.level    = level;
    af.duration = 6;
    apply_duskfall(*ch->in_room->area, af);

    // Walk the alinks, looking for connections to this area
    for (ALINK_DATA * alink(alink_first); alink != NULL; alink = alink->next)
    {
        // Apply the duskfall to all connected areas
        if (alink->a1 == ch->in_room->area) apply_duskfall(*alink->a2, af);
        else if (alink->a2 == ch->in_room->area) apply_duskfall(*alink->a1, af);
    }

    // Apply cooldown
    int skill(get_skill(ch, sn));
    af.where    = TO_AFFECTS;
    af.duration = 70 - (UMAX(0, skill - 60) / 4);
    affect_to_char(ch, &af);
    return true;
}

void check_eyeblighttouch(OBJ_DATA & obj, AFFECT_DATA & paf)
{
    // Check whether the object is worn
    if (!obj.worn_on || obj.carried_by == NULL)
        return;

    // Check for whether the obj will try to take effect this tick
    if (number_bits(4) != 0)
        return;

    // Check whether the bearer is already blind
    if (IS_AFFECTED(obj.carried_by, AFF_BLIND))
        return;

    // Echo
    act("An arc of dark power leaps from $p, striking you!", obj.carried_by, &obj, NULL, TO_CHAR);
    act("An arc of dark power leaps from $p, striking $n!", obj.carried_by, &obj, NULL, TO_ROOM);

    // Check for immunity and saves
    if (IS_SET(obj.carried_by->imm_flags, IMM_BLIND) || saves_spell(paf.level, NULL, obj.carried_by, DAM_NEGATIVE))
    {
        send_to_char("You shrug off the negative energy, leaving it no purchase on you.\n", obj.carried_by);
        return;
    }

    // Did not save; hit with blindness
    act("Your vision clouds over with eyeblight, leaving you blind!", obj.carried_by, NULL, NULL, TO_CHAR);
    act("$n's eyes cloud with darkness, and $e appears to be blinded.", obj.carried_by, NULL, NULL, TO_ROOM);

    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_blindness;
    af.level    = paf.level;
    af.location = APPLY_HITROLL;
    af.modifier = -4;
    af.duration = af.level / 6;
    af.bitvector = AFF_BLIND;
    affect_to_char(obj.carried_by, &af);
}

bool spell_eyeblighttouch(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Build a list of candidate objects
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));
    std::vector<OBJ_DATA*> lit_objs;
    std::vector<OBJ_DATA*> unlit_objs;
    for (OBJ_DATA * obj(victim->carrying); obj != NULL; obj = obj->next_content)
    {
        if (obj->worn_on)
        {
            if (IS_OBJ_STAT(obj, ITEM_GLOW)) lit_objs.push_back(obj);
            else unlit_objs.push_back(obj);
        }
    }

    // Check that there are any objects at all
    if (lit_objs.empty() && unlit_objs.empty())
    {
        act("$N is wearing nothing which can serve as a focus for eyeblight.", ch, NULL, victim, TO_CHAR);
        return false;
    }
 
    // Initial echoes
    act("Fell power courses over your hands, which pulse with dark energy!", ch, NULL, NULL, TO_CHAR);
    act("Fell power courses over $n's hands, which pulse with dark energy!", ch, NULL, NULL, TO_ROOM);

    // Check for dodge (based on dex alone)
    if (number_percent() <= (get_curr_stat(victim, STAT_DEX) * 2))
    {
        // Miss echoes
        act("You leap with hands outstretched towards $N, but $E slips nimbly to the side!", ch, NULL, victim, TO_CHAR);
        act("$n leaps with hands outstretched towards you, but you slip nimbly to the side!", ch, NULL, victim, TO_VICT);
        act("$n leaps with hands outstretched towards $N, but $E slips nimbly to the side!", ch, NULL, victim, TO_NOTVICT);
        return true;
    }

    // Choose an object and check whether it is already eyeblighted
    OBJ_DATA * obj;
    if (!lit_objs.empty()) obj = lit_objs[number_range(0, lit_objs.size() - 1)];
    else obj = unlit_objs[number_range(0, unlit_objs.size() - 1)];
    if (obj_is_affected(obj, sn))
    {
        // Already eyeblighted; echo about it and bail out
        act("You leap towards $N and grasps $p, but it is already infected.", ch, obj, victim, TO_CHAR);
        act("$n leaps towards you and grasps $p, but the dark power on $s hands simply fades.", ch, obj, victim, TO_VICT);
        act("$n leaps towards $N and grasps $p, but the dark power on $s hands simply fades.", ch, obj, victim, TO_NOTVICT);
        return true;
    }

    // Hit an untainted object; echo about it
    act("You leap towards $N, infecting $p with eyeblight!", ch, obj, victim, TO_CHAR);
    act("$n leaps towards you, touching $p with a crackle of dusky power!", ch, obj, victim, TO_VICT);
    act("$n leaps towards $N, touching $p with a crackle of dusky power!", ch, obj, victim, TO_NOTVICT);

    // Apply effect
    AFFECT_DATA af = {0};
    af.where    = TO_OBJECT;
    af.type     = sn;
    af.level    = level;
    af.duration = 3 + (level / 3);
    af.location = APPLY_SAVES;
    af.modifier = -1;
    af.bitvector = (IS_SET(obj->extra_flags[0], ITEM_DARK) ? 0 : ITEM_DARK);
    affect_to_obj(obj, &af);

    // Check for dimming
    if (IS_OBJ_STAT(obj, ITEM_GLOW))
    {
        act("The light fades from $p as it grows dark.", ch, obj, NULL, TO_ALL);
        REMOVE_BIT(obj->extra_flags[0], ITEM_GLOW);
    }

    return true;
}

bool spell_dreadwave(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Room check
    if (ch->in_room == NULL)
        return false;

    // Echoes
    act("A dark wave of pure dread bursts from you, engulfing this place!", ch, NULL, NULL, TO_CHAR);
    act("A dark wave of pure dread bursts from $n, engulfing this place!", ch, NULL, NULL, TO_ROOM);

    // Determine dice size based on whether the room is dark
    int diceSize(3);
    if (room_is_dark(ch->in_room))
    {
        ++diceSize;
        level += 5;
    }

    // Hit ungrouped occupants of the room
    CHAR_DATA * victim_next;
    for (CHAR_DATA * victim(ch->in_room->people); victim != NULL; victim = victim_next)
    {
        // Disqualify certain folks
        victim_next = victim->next_in_room;
        if (is_same_group(ch, victim) || is_safe_spell(ch, victim, true))
            continue;

        // Determine damage
        int dam(dice(level, diceSize));
        if (saves_spell(level, ch, victim, DAM_FEAR))
            damage(ch, victim, dam / 2, sn, DAM_FEAR, true);
        else
        {
            // Did not save; apply full damage and a non-stacking penalty to fear
            damage(ch, victim, dam, sn, DAM_FEAR, true);
            if (IS_VALID(victim) && !IS_OAFFECTED(ch, AFF_GHOST) && !is_affected(victim, sn))
            {
                act("Terror fills you, weakening your resolve!", victim, NULL, NULL, TO_CHAR);
                act("$n looks suddenly panicked!", victim, NULL, NULL, TO_ROOM);

                AFFECT_DATA af = {0};
                af.where    = TO_AFFECTS;
                af.level    = level;
                af.type     = sn;
                af.duration = 4;
                af.location = APPLY_RESIST_FEAR;
                af.modifier = -level / 10;
                affect_to_char(victim, &af);
            }
        }
    }

    return true;
}

static void check_drop_eq(CHAR_DATA & ch, int wearslot)
{
    // Look for an object in the slot
    OBJ_DATA * obj(get_eq_char(&ch, wearslot));
    if (obj == NULL)
        return;

    // Echo
    act("$p falls from your dusky claws!", &ch, obj, NULL, TO_CHAR);
    act("$p falls from $n's dusky claws!", &ch, obj, NULL, TO_ROOM);

    // Move the obj to the room
    obj_from_char(obj);
    obj_to_room(obj, ch.in_room);
}

bool spell_shadowfiend(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for the effect
    if (is_affected(ch, sn))
    {
        send_to_char("You are already a shadow wraith!\n", ch);
        return false;
    }

    // Echoes
    act("Your form hollows, growing translucent and wraithlike even as your fingers harden into razor-sharp claws!", ch, NULL, NULL, TO_CHAR);
    act("$n's form hollows, growing translucent and wraithlike even as $s fingers harden into razor-sharp claws!", ch, NULL, NULL, TO_ROOM);

    // Apply effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = level / 2;
    af.location = APPLY_RESIST_LIGHT;
    af.modifier = -40;
    affect_to_char(ch, &af);

    af.location = APPLY_RESIST_WEAPON;
    af.modifier = 10;
    affect_to_char(ch, &af);

    af.location = APPLY_DAMROLL;
    af.modifier = UMAX(1, level / 8);
    affect_to_char(ch, &af);
    
    af.location = APPLY_HITROLL;
    affect_to_char(ch, &af);

    // Drop all held eq
    check_drop_eq(*ch, WEAR_HOLD);
    check_drop_eq(*ch, WEAR_WIELD);
    check_drop_eq(*ch, WEAR_SHIELD);
    check_drop_eq(*ch, WEAR_DUAL_WIELD);
    return true;
}

bool is_nightstalking(const CHAR_DATA & ch)
{
    for (const AFFECT_DATA * paf(get_affect(&ch, gsn_nightstalk)); paf != NULL; paf = get_affect(&ch, gsn_nightstalk, paf))
    {
        if (paf->modifier == 1)
            return true;
    }

    return false;
}

bool spell_nightstalk(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet prepared to stalk the shadows again.\n", ch);
        return false;
    }

    // Check for room
    if (ch->in_room == NULL || !room_is_dark(ch->in_room))
    {
        send_to_char("This place is too well-lit to enter the shadows.\n", ch);
        return false;
    }

    // Echoes
    act("You blend your being with the shadows, preparing to stalk the night.", ch, NULL, NULL, TO_CHAR);
    act("$n's form blurs into darkness, melding with the shadows!", ch, NULL, NULL, TO_ROOM);

    // Add cooldown
    int skill(get_skill(ch, sn));
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 30 - UMAX(0, (skill - 70) / 5);
    affect_to_char(ch, &af);

    // Add effect
    af.duration = 3 + (level / 10);
    af.modifier = 1;
    af.bitvector = AFF_INVISIBLE;
    affect_to_char(ch, &af);
    return true;
}

void check_webofoame_catch(CHAR_DATA & ch, ROOM_INDEX_DATA & room)
{
    // Check for the effect
    AFFECT_DATA * paf(get_room_affect(&room, gsn_webofoame));
    if (paf == NULL)
        return;

    // Found a web, check for a dex save
    WAIT_STATE(&ch, PULSE_VIOLENCE);
    if (number_percent() <= ((get_curr_stat(&ch, STAT_DEX) - 15) * 5))
    {
        act("You manage to slowly wend your way through the stick threads of the dark web.", &ch, NULL, NULL, TO_CHAR);
        act("$n slowly wends $s way through the threads of the dark web.", &ch, NULL, NULL, TO_ROOM);
        return;
    }

    // Failed the dex save; echo
    act("A sticky thread of the web catches you, and you rapidly ensnare yourself as you struggle to escape!", &ch, NULL, NULL, TO_CHAR);
    act("A sticky thread of the web catches $n, who rapidly ensnares $mself as $e struggles to escape!", &ch, NULL, NULL, TO_ROOM);

    // Apply effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_webofoame;
    af.level    = paf->level;
    af.duration = UMIN(1, paf->duration);
    af.bitvector = AFF_SLOW;
    af.location = APPLY_DEX;
    af.modifier = -4;
    affect_to_char(&ch, &af);

    // Change the pose
    copy_string(ch.pose, "caught in a web.");
}

bool check_webofoame_caught(CHAR_DATA & ch)
{
    for (AFFECT_DATA * paf(get_affect(&ch, gsn_webofoame)); paf != NULL; paf = get_affect(&ch, gsn_webofoame, paf))
    {
        if (paf->location != APPLY_NONE)
            return true;
    }
    return false;
}

bool spell_webofoame(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    for (AFFECT_DATA * paf(get_affect(ch, sn)); paf != NULL; paf = get_affect(ch, sn, paf))
    {
        if (paf->location == APPLY_NONE)
        {
            send_to_char("You are not yet ready to spin another web of oame.\n", ch);
            return false;
        }
    }

    // Check room
    if (ch->in_room == NULL || ch->in_room->sector_type == SECT_UNDERWATER)
    {
        send_to_char("You cannot spin a web here!\n", ch);
        return false;
    }

    // Check for existing web
    if (room_is_affected(ch->in_room, sn))
    {
        send_to_char("There is already a web here.\n", ch);
        return false;
    }

    // Echoes
    act("You spin out sticky threads of dark power, weaving them into a gelatinous web!", ch, NULL, NULL, TO_CHAR);
    act("$n spins out sticky threads of dark power, weaving them into a gelatinous web!", ch, NULL, NULL, TO_ROOM);

    // Apply effect and cooldown
    AFFECT_DATA af = {0};
    af.where    = TO_ROOM_AFF;
    af.level    = level;
    af.type     = sn;
    af.duration = 3 + (level / 10);
    affect_to_room(ch->in_room, &af);

    int skill(get_skill(ch, sn));
    af.where    = TO_AFFECTS;
    af.duration = 30 - UMAX(0, (skill - 70) / 5);
    affect_to_char(ch, &af);
    return true;
}

bool spell_gaveloflogor(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for a cooldown on the target
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));
    if (is_affected(victim, sn))
    {
        act("$N has already been recently struck with the gavel of logor.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Echoes
    act("You call down the judgment of logor on $N!", ch, NULL, victim, TO_CHAR);
    act("$n shrieks $s judgment, slamming you with a blast of dark power!", ch, NULL, victim, TO_VICT);
    act("$n shrieks $s judgment, slamming $N with a blast of dark power!", ch, NULL, victim, TO_NOTVICT);

    // Calculate damage based on how hurt the target is
    int percentHurt(100 - ((victim->hit * 100) / victim->max_hit));
    int dam(25 + level + (percentHurt * 3));
    if (saves_spell(level + (percentHurt / 10), ch, victim, DAM_NEGATIVE))
        dam /= 2;

    // Add cooldown to the target
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.location = APPLY_HIDE;
    af.duration = 16;
    affect_to_char(victim, &af);
    
    check_hatefire_damage(sn, DamageInfo(dam, DAM_NEGATIVE), ch, victim);
    return true;
}

static void do_divination_past(CHAR_DATA & ch, CHAR_DATA & victim)
{
    // Iterate all rooms
    std::set<const AREA_DATA *> areas;
    for (int i(0); i < MAX_KEY_HASH; ++i)
    {
        for (const ROOM_INDEX_DATA * room(room_index_hash[i]); room != NULL; room = room->next)
        {
            // Iterate the tracks in the room
            for (const TRACK_DATA * track(room->tracks); track != NULL; track = track->next)
            {
                // Look for tracks matching the victim
                if (track->valid && track->ch == &victim)
                    areas.insert(room->area);
            }
        }
    }

    // List all the areas
    act("A memory of $N's past floats through your head, showing you places where $E has recently trod...", &ch, NULL, &victim, TO_CHAR);

    for (std::set<const AREA_DATA *>::const_iterator iter(areas.begin()); iter != areas.end(); ++iter)
    {
        send_to_char("...", &ch);
        send_to_char((*iter)->name, &ch);
        send_to_char("...\n", &ch);
    }
}

static void do_divination_present(CHAR_DATA & ch, CHAR_DATA & victim)
{
    // Show the group display and affects list
    act("An image of $N forms in your mind, granting you insight!", &ch, NULL, &victim, TO_CHAR);
    show_affects(&victim, &ch, false);
    send_to_char("\n", &ch);
    show_group_listing(&victim, &ch);
    send_to_char("\n", &ch);
}

static void do_divination_future(CHAR_DATA & ch, CHAR_DATA & victim)
{
    // Make the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_fetiddivination;
    af.level    = ch.level;
    af.duration = 48;
    af.modifier = victim.id;
    affect_to_char(&ch, &af);

    // Echo
    act("$N's future appears to you in glimpses too brief for your consciousness, imprinting itself on your mind!", &ch, NULL, &victim, TO_CHAR);
}

bool spell_fetiddivination(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check object for entrails
    OBJ_DATA * obj(static_cast<OBJ_DATA*>(vo));
    if (obj->pIndexData->vnum != OBJ_VNUM_TROPHY_ENTRAILS)
    {
        send_to_char("Entrails are required for proper divination.\n", ch);
        return false;
    }

    // Check for a target argument
    if (target_name[0] == '\0')
    {
        send_to_char("Whose presence did you wish to divine?\n", ch);
        return false;
    }

    // Find the target
    char arg[MAX_INPUT_LENGTH];
    target_name = one_argument(target_name, arg);
    CHAR_DATA * victim(get_char_world(ch, arg));
    if (victim == NULL)
    {
        send_to_char("You sense no one by that name in the world.\n", ch);
        return false;
    }

    // Check for victim being the caster
    if (ch == victim)
    {
        send_to_char("You hardly need dark magic to learn of yourself.\n", ch);
        return false;
    }

    // Make sure the entrails are strong enough
    if (obj->level < victim->level)
    {
        act("$p is not strong enough to divine $N's presence.", ch, obj, victim, TO_CHAR);
        return false;
    }

    // Check for a type argument
    if (target_name[0] == '\0')
    {
        act("Did you wish to divine $s past, present, or future?", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Handle according to type
    typedef void (*DivinationFun)(CHAR_DATA &, CHAR_DATA &);
    DivinationFun divinationFun(NULL);
    if (!str_prefix(target_name, "past")) divinationFun = &do_divination_past;
    else if (!str_prefix(target_name, "present")) divinationFun = &do_divination_present;
    else if (!str_prefix(target_name, "future")) 
    {
        // Only one future effect up at a time
        if (is_affected(ch, sn))
        {
            send_to_char("You cannot imprint another future on your mind again so soon!\n", ch);
            return false;
        }

        divinationFun = do_divination_future;
    }
    else
    {
        send_to_char("You must specify past, present, or future.\n", ch);
        return false;
    }

    // Echo and delegate to the appropriate divination function
    act("You study the swirls and striations in $p, seeking out traces of $N's presence.", ch, obj, victim, TO_CHAR);
    act("$n pours over $p, studying them carefully.", ch, obj, victim, TO_ROOM);
    (*divinationFun)(*ch, *victim);

    // Destroy the entrails
    act("A dark pulse runs down the length of the entrails, which shrivel up and wither away!", ch, obj, NULL, TO_ALL);
    extract_obj(obj);
    return true;
}

GrimseepAffiliation check_grimseep_affiliation(const CHAR_DATA & ch)
{
     // No room means no affiliation
    if (ch.in_room == NULL)
        return Grimseep_None;

    return check_grimseep_affiliation(ch, *ch.in_room);
}

GrimseepAffiliation check_grimseep_affiliation(const CHAR_DATA & ch, ROOM_INDEX_DATA & room)
{
    // No grimseep means no affiliation
    AFFECT_DATA * paf(get_room_affect(&room, gsn_grimseep));
    if (paf == NULL)
        return Grimseep_None;

    // Grimseep exists; check the modifier against the id
    return (paf->modifier == ch.id ? Grimseep_Aided : Grimseep_Hindered);
}

static void check_grimseep_bonus(ROOM_INDEX_DATA & room, AFFECT_DATA & paf)
{
    // Add in some variance
    if (number_bits(2) != 0)
        return;

    // Check for the caster
    for (CHAR_DATA * ch(room.people); ch != NULL; ch = ch->next_in_room)
    {
        if (paf.modifier != ch->id)
            continue;
        
        // Found the caster, check whether he had a bonus recently
        for (AFFECT_DATA * paf(get_affect(ch, gsn_grimseep)); paf != NULL; paf = get_affect(ch, gsn_grimseep, paf))
        {
            if (paf->location == APPLY_HIDE)
                return;
        }
        
        // Echoes    
        act("The dark sludge ripples and oozes, seeping over you!", ch, NULL, NULL, TO_CHAR);
        act("The dark sludge ripples and oozes, seeping over $n!", ch, NULL, NULL, TO_ROOM);

        // Apply a bonus cooldown
        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.type     = gsn_grimseep;
        af.location = APPLY_HIDE;
        af.duration = 20;
        af.level    = ch->level;
        affect_to_char(ch, &af);
 
        // Grant a random bonus
        switch (number_range(0, 2))
        {
            // Healing
            case 0: 
                if ((ch->hit + 250) < ch->max_hit)
                {
                    ch->hit = UMIN(ch->max_hit, ch->hit + 500);
                    act("Slime fills your wounds, closing and healing them before dripping away.", ch, NULL, NULL, TO_CHAR);
                    act("Slime fills $s wounds, closing and healing them before dripping away.", ch, NULL, NULL, TO_ROOM);
                    return;
                }
                break;

            // Toughness
            case 1:
                af.location = APPLY_RESIST_WEAPON;
                af.modifier = 10;
                affect_to_char(ch, &af);
                act("A thick layer of the grime covers you, then dries and hardens into natural armor!", ch, NULL, NULL, TO_CHAR);
                act("A thick layer of the grime covers $m, then dries and hardens into natural armor!", ch, NULL, NULL, TO_ROOM);
                return;
        }

        // Did not grant a bonus yet, so just add hp
        af.location = APPLY_HIT;
        af.modifier = 100;
        affect_to_char(ch, &af);

        act("Silt pours into your mouth, leaving you no choice but to swallow it in great mouthfuls!", ch, NULL, NULL, TO_CHAR);
        act("Silt pours into $s mouth, making $m gag and retch reflexively!", ch, NULL, NULL, TO_ROOM);
        act("You gag and retch reflexively, but once the flow subsides you are left feeling somehow hardier.", ch, NULL, NULL, TO_CHAR);
        return;
    }
}

static bool check_grimseep_decay(ROOM_INDEX_DATA & room, AFFECT_DATA & paf)
{
    int chance(2);
    int amount(1);
    if (!room_is_dark(&room)) 
        chance += 2;
    
    // Check for room-based accelerators
    if (room_is_affected(&room, gsn_blaze) 
    || room_is_affected(&room, gsn_blazinginferno) 
    || room_is_affected(&room, gsn_undyingradiance) 
    || area_is_affected(room.area, gsn_rainoffire)) 
    {
        chance += 50;
        amount += 9;
    }

    // Radiance is a heavy accelerator
    for (CHAR_DATA * ch(room.people); ch != NULL; ch = ch->next_in_room)
    {
        if (is_affected(ch, gsn_radiance))
        {
            chance += 100;
            amount += 9;
        }
    }

    // Check for decay
    if (number_percent() > chance)
        return false;

    // Perform decay
    paf.level -= amount;
    if (paf.level <= 0)
    {
        act("The seeping layer of ooze completely dries out, cracking and flaking away.", room.people, NULL, NULL, TO_ALL);
        affect_remove_room(&room, &paf);
        return true;
    }

    // Decayed but not removed, so just echo
    if (amount >= 5)
        act("Large swathes of the slime coating the ground dry up and crumble away!", room.people, NULL, NULL, TO_ALL);
    else
        act("Bits of the slime coating the ground dry up and crumble away.", room.people, NULL, NULL, TO_ALL);

    return true;
}

static void check_grimseep_spread(ROOM_INDEX_DATA & room, const AFFECT_DATA & paf)
{
    // If there is no spread left in the effect, bail out
    if (paf.level <= 0)
        return;
    
    // Find each attached room
    for (unsigned int i(0); i < Direction::Max; ++i)
    {
        // Look up the adjacent room
        Direction::Value direction(static_cast<Direction::Value>(i));
        ROOM_INDEX_DATA * nextRoom(Direction::Adjacent(room, direction, EX_WALLED|EX_WALLOFFIRE|EX_WEAVEWALL|EX_ICEWALL));
        if (nextRoom == NULL || room_is_affected(nextRoom, gsn_grimseep))
            continue;
        
        // Check odds against sector type
        int chance(-1);
        switch (room.sector_type)
        {
            case SECT_DESERT:   chance = 2; break;
            case SECT_MOUNTAIN: 
            case SECT_UNDERGROUND: chance = 10; break;
            case SECT_ROAD:
            case SECT_INSIDE:
            case SECT_CITY:
            case SECT_HILLS:    chance = 10; break;
            case SECT_FOREST:
            case SECT_FIELD:   chance = 20; break;
            case SECT_SWAMP:    chance = 30; break;
        }

        if (!room_is_dark(&room) || number_percent() > chance)
            continue;

        // Slime is spreading; send echoes
        std::ostringstream mess;
        mess << "The dark slime spreads, oozing " << Direction::DirectionalNameFor(direction) << ".";
        act(mess.str().c_str(), room.people, NULL, NULL, TO_ALL);

        mess.str("");
        mess << "A thick, dark slime seeps in " << Direction::SourceNameFor(Direction::ReverseOf(direction));
        mess << ", coating the ground!";
        act(mess.str().c_str(), nextRoom->people, NULL, NULL, TO_ALL);

        // Apply effect
        AFFECT_DATA af(paf);
        --af.level;
        affect_to_room(nextRoom, &af);
    }
}

void check_grimseep_update(ROOM_INDEX_DATA & room, AFFECT_DATA & paf)
{
    if (!check_grimseep_decay(room, paf))
    {
        check_grimseep_bonus(room, paf);
        check_grimseep_spread(room, paf);
    }
}

bool spell_grimseep(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    for (AFFECT_DATA * paf(get_affect(ch, sn)); paf != NULL; paf = get_affect(ch, sn, paf))
    {
        if (paf->location == APPLY_NONE)
        {
            send_to_char("You are not yet ready to call forth the slime of the dead sea again.\n", ch);
            return false;
        }
    }

    // Check for room type
    if (ch->in_room == NULL || !ON_GROUND(ch))
    {
        send_to_char("You cannot call forth the grimseep here!\n", ch);
        return false;
    }

    // Check for room already affected
    if (room_is_affected(ch->in_room, sn))
    {
        send_to_char("This place is already seeping with slime.\n", ch);
        return false;
    }

    // Echoes
    act("You press your palms to the ground, which begins to weep a foul, viscous slime!", ch, NULL, NULL, TO_CHAR);
    act("$n presses $s palms to the ground, which begins to weep a foul, viscous slime!", ch, NULL, NULL, TO_ROOM);

    // Add cooldown
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 240 - UMAX(0, get_skill(ch, sn) - 60);
    affect_to_char(ch, &af);

    // Add effect
    af.where    = TO_ROOM_AFF;
    af.level    = 3 + (level / 3);
    af.modifier = ch->id;
    af.duration = -1;
    affect_to_room(ch->in_room, &af);

    return true;
}

int check_bloodofthevizier_count(CHAR_DATA & ch, CHAR_DATA & victim)
{
    // Iterate the blood of the vizier effects
    for (AFFECT_DATA * paf(get_affect(&victim, gsn_bloodofthevizier)); paf != NULL; paf = get_affect(&victim, gsn_bloodofthevizier, paf))
    {
        // If the count effect for this caster is found, return the count
        if (paf->level == ch.id && paf->location == APPLY_HIDE)
            return paf->modifier;
    }

    return 0;
}

bool check_bloodofthevizier_drink(CHAR_DATA & ch, OBJ_DATA & obj)
{
    // Look for the effect on the object
    AFFECT_DATA * paf(get_obj_affect(&obj, gsn_bloodofthevizier));
    if (paf == NULL)
        return false;

    // Echoes
    act("You drink from $p, draining it completely.", &ch, &obj, NULL, TO_CHAR);
    act("$n drinks from $p, draining it completely.", &ch, &obj, NULL, TO_ROOM);

    // Found the effect, check for id
    if (paf->modifier != ch.id)
    {
        // Find and remove the various existing effects
        int count(0);
        int duration(-1);
        AFFECT_DATA * blood_next;
        for (AFFECT_DATA * blood(get_affect(&ch, gsn_bloodofthevizier)); blood != NULL; blood = blood_next)
        {
            // Filter out effects not associated with this caster
            blood_next = get_affect(&ch, gsn_bloodofthevizier, blood);
            if (blood->level != paf->modifier)
                continue;

            // Save off the count, if present
            if (blood->location == APPLY_HIDE)
                count = blood->modifier;
            else
                duration = blood->duration;

            // Remove the effect
            affect_remove(&ch, blood);
        }

        // Echoes
        act("A dark pulse of energy surges down your body, crackling with fell power.", &ch, NULL, NULL, TO_CHAR);
        act("A dark pulse of energy surges down $n's body, crackling with fell power.", &ch, NULL, NULL, TO_ROOM);

        // Increase the count unless the previous effect has not worn off
        if (duration < 0)
        {
            ++count;
            duration = 120;
        }

        // Apply the new bonus effects
        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.type     = gsn_bloodofthevizier;
        af.level    = paf->modifier;
        af.duration = duration;

        af.location = APPLY_HIT;
        af.modifier = UMIN(150, paf->level + count * 2);
        affect_to_char(&ch, &af);

        af.location = APPLY_MANA;
        affect_to_char(&ch, &af);

        // Apply the new counter effect
        af.location = APPLY_HIDE;
        af.modifier = count;
        af.duration = -1;
        affect_to_char(&ch, &af);
    }

    // Destroy the object before returning
    extract_obj(&obj);
    return true;
}

bool spell_bloodofthevizier(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for a proper blood vial
    OBJ_DATA * obj(static_cast<OBJ_DATA*>(vo));
    if (obj->pIndexData->vnum != OBJ_VNUM_REAGENT_BLOOD || obj->value[0] != -ch->id)
    {
        act("$p is not a vial of your own blood.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Check for the effect already being present
    if (obj_is_affected(obj, sn))
    {
        act("$p has already been infused with your will.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Echoes
    act("Dark swirls filter through $p as you infuse it with your will.", ch, obj, NULL, TO_CHAR); 
    act("$n murmurs over $p, and dark swirls filter through it before slowly vanishing.", ch, obj, NULL, TO_ROOM); 

    // Apply the effect
    AFFECT_DATA af = {0};
    af.where    = TO_OBJECT;
    af.type     = sn;
    af.duration = -1;
    af.level    = level;
    af.modifier = ch->id;
    affect_to_obj(obj, &af);

    // Set some bits on the blood
    SET_BIT(obj->extra_flags[0], ITEM_EVIL);
    SET_BIT(obj->extra_flags[0], ITEM_HUM);

    return true;
}

void intensify_seedofmadness(CHAR_DATA & ch, AFFECT_DATA & saf)
{
    // Increase the chance of a new malady each time they fail a save vs mental
    if (!saves_spell(saf.level, NULL, &ch, DAM_MENTAL))
    {
        int increase(number_range(1, 2));
        saf.modifier = UMIN(33, saf.modifier + increase);
    }

    if (saf.duration <= 1 || number_percent() > saf.modifier)
        return;

    // Last chance for safety; check for a save
    if (saves_spell(saf.level, NULL, &ch, DAM_FEAR))
    {
        send_to_char("A manic anxiety swells up within you, but you force it back down!\n", &ch);
        return;
    }

    // Malady will occur
    send_to_char("The madness within you grows, overrunning your senses!\n", &ch);
    
    AFFECT_DATA af = {0};
    af.type     = gsn_seedofmadness;
    af.where    = TO_AFFECTS;
    af.level    = saf.level;
    af.duration = number_range(1, saf.duration);
    
    // Choose a malediction
    switch (number_range(0, 19))
    {
        case 0:
            // Plague of madness
            if (!is_affected(&ch, gsn_plague_madness))
            {
                spell_plague_madness(gsn_plague_madness, saf.level, &ch, &ch, skill_table[gsn_plague_madness].target);
                return;
            }
            break;

        case 1:
            // Confusion
            if (!is_affected(&ch, gsn_confusion))
            {
                send_to_char("The world seems to twist as your mind becomes suddenly clouded and confused!\n", &ch);
                af.type = gsn_confusion;
                affect_to_char(&ch, &af);
                return;
            }
            break;

        case 2:
            // Vertigo
            if (!is_affected(&ch, gsn_vertigo))
            {
                send_to_char("You reel, suddenly very dizzy!\n", &ch);
                af.type = gsn_vertigo;
                affect_to_char(&ch, &af);
                return;
            }
            break;

        // Max mana loss
        case 3: case 4: case 5: case 6: case 7: case 8:
            send_to_char("You feel less able to focus.\n", &ch);
            af.location = APPLY_MANA;
            af.modifier = -1 * number_range(1, saf.level);
            affect_to_char(&ch, &af);
            return;

        // Int loss
        case 9: case 10:
            send_to_char("You feel suddenly duller, your mind hampered by your rising anxiety.\n", &ch);
            af.location = APPLY_INT;
            af.modifier = -1;
            affect_to_char(&ch, &af);
            return;

        // Wis loss
        case 11: case 12:
            send_to_char("You feel your judgment break down slightly as the madness takes over.\n", &ch);
            af.location = APPLY_WIS;
            af.modifier = -1;
            affect_to_char(&ch, &af);
            return;

        // Chr loss
        case 13: case 14:
            send_to_char("You feel less suddenly less witty, your tongue swollen and slow.\n", &ch);
            af.location = APPLY_CHR;
            af.modifier = -1;
            affect_to_char(&ch, &af);
            return;

        // Resist fear loss
        case 15: case 16:
            send_to_char("Panic swells within you!\n", &ch);
            af.location = APPLY_RESIST_FEAR;
            af.modifier = -1 * number_range(1, saf.level / 5);
            affect_to_char(&ch, &af);
            return;

        // Resist mental loss
        case 17: case 18:
            send_to_char("You feel your mental barriers crumble.\n", &ch);
            af.location = APPLY_RESIST_MENTAL;
            af.modifier = -1 * number_range(1, saf.level / 5);
            affect_to_char(&ch, &af);
            return;
    }

    // Other; just take current mana
    send_to_char("You feel suddenly drained and weary.\n", &ch);
    ch.mana -= number_range(saf.level, saf.level * 6);
}

bool spell_seedofmadness(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for an argument
    CHAR_DATA * victim(NULL);
    OBJ_DATA * obj(NULL);
    if (target_name[0] == '\0')
    {
        // No argument; check whether fighting
        if (ch->fighting == NULL)
        {
            // Not fighting, bail out
            send_to_char("Plant the seed of madness in whom?\n", ch);
            return false;
        }

        // Use the combat opponent as the victim
        victim = ch->fighting;
    }
    else
    {
        // Argument supplied, find the victim
        char arg[MAX_INPUT_LENGTH];
        target_name = one_argument(target_name, arg);
        victim = get_char_world(ch, arg);
        if (victim == NULL)
        {
            send_to_char("You sense no one by that name.\n", ch);
            return false;
        }

        // Found the victim, check whether target is remote
        if (victim->in_room != ch->in_room)
        {
            // Target is remote, so the caster must consume an object previously held by him
            if (target_name[0] == '\0')
            {
                act("From this distance, you need a link to $N to plant the seed of madness in $M.", ch, NULL, victim, TO_CHAR);
                return false;
            }

            // Find the link object
            obj = get_obj_carry(ch, target_name, ch);
            if (obj == NULL || obj->worn_on)
            {
                send_to_char("You are not carrying anything by that name.\n", ch);
                return false;
            }

            // Validate the link object
            if (IS_SET(obj->extra_flags[0], ITEM_NODESTROY))
            {
                act("$p is too tough to be consumed this way.", ch, obj, NULL, TO_CHAR);
                return false;
            }

            bool ownerMatches(false);
            for (unsigned int i(0); i < MAX_LASTOWNER; ++i)
            {
                if (!str_cmp(obj->lastowner[i], IS_NPC(victim) ? victim->short_descr : victim->name))
                {
                    ownerMatches = true;
                    break;
                }
            }

            if (!ownerMatches)
            {
                act("$p was not recently held by $N, and cannot serve as a link to $M.", ch, obj, victim, TO_CHAR);
                return false;
            }

            // Echoes
            act("Grasping $p tightly, you focus on sending your will out to $N.", ch, obj, victim, TO_CHAR);
            act("$n clutches $p tightly, concentrating.", ch, obj, victim, TO_ROOM);
        }
    }

    // Check for already affected
    if (is_affected(victim, sn))
    {
        act("$N has already been implanted with the seed of madness.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Check for a save
    if (saves_spell(level, ch, victim, DAM_MENTAL))
    {
        // Saved; just send echoes
        send_to_char("Your thoughts grow briefly hazy, but clear again swiftly.\n", victim);
        act("$N resists your efforts to plant the seed of madness in $S mind.", ch, NULL, victim, TO_CHAR);
    }
    else
    {
        // Did not save
        send_to_char("Your thoughts grow suddenly hazy, and you find it hard to focus.\n", victim);
        act("Touching lightly on $S unconscious, you plant the seed of madness in $N's mind!", ch, NULL, victim, TO_CHAR);

        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.type     = sn;
        af.level    = level;
        af.duration = level;
        af.modifier = 0;
        af.location = APPLY_NONE;
        affect_to_char(victim, &af);
    }

    // Remove the object, if one was used
    if (obj != NULL)
    {
        act("$p suddenly seems to hollow out, turning into ash and crumbling away!", ch, obj, NULL, TO_ALL);
        extract_obj(obj);
    }
    return true;
}

void check_bierofunmaking(CHAR_DATA & victim)
{
    // Sanity-checks
    if (victim.in_room == NULL || IS_NPC(&victim))
        return;

    // Check for a bier
    AFFECT_DATA * paf(get_room_affect(victim.in_room, gsn_bierofunmaking));
    if (paf == NULL)
        return;

    // Check whether the caster is present in the room
    CHAR_DATA * ch(get_char_by_id_any_room(paf->modifier, *victim.in_room));
    if (ch == NULL)
        return;

    // Echoes
    act("You fall upon the shadowy bier, your lifeblood seeping into it.", &victim, NULL, NULL, TO_CHAR);
    act("$n falls upon the shadowy bier, $s lifeblood seeping into it.", &victim, NULL, NULL, TO_ROOM);

    act("A pulse of energy surges from the darkness, enveloping you in unholy power!", ch, NULL, NULL, TO_CHAR);
    act("A pulse of energy surges from the darkness, enveloping $n in unholy power!", ch, NULL, NULL, TO_ROOM);

    // Check whether the caster has already benefitted from this victim's death
    std::vector<AFFECT_DATA*> effects;
    for (AFFECT_DATA * bier(get_affect(ch, gsn_bierofunmaking)); bier != NULL; bier = get_affect(ch, gsn_bierofunmaking, bier))
    {
        if (bier->level == victim.id)
            effects.push_back(bier);
    }

    // Strip all effects associated with this victim
    for (size_t i(0); i < effects.size(); ++i)
        affect_remove(ch, effects[i]);

    // Now add in the effects
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_bierofunmaking;
    af.level    = victim.id;
    af.duration = (victim.level * victim.level * victim.level * victim.level) / 1000;
    
    af.location = APPLY_HIT;
    af.modifier = victim.level;
    affect_to_char(ch, &af);

    af.location = APPLY_MANA;
    affect_to_char(ch, &af);

    af.location = APPLY_HITROLL;
    af.modifier = 3;
    affect_to_char(ch, &af);

    af.location = APPLY_DAMROLL;
    affect_to_char(ch, &af);
}

bool spell_bierofunmaking(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    for (const AFFECT_DATA * paf(get_affect(ch, sn)); paf != NULL; paf = get_affect(ch, sn, paf))
    {
        if (paf->level == 0)
        {
            send_to_char("You cannot form another bier of unmaking so soon.\n", ch);
            return false;
        }
    }

    // Check for room effect
    if (ch->in_room == NULL || room_is_affected(ch->in_room, sn))
    {
        send_to_char("There is already a bier of unmaking in this place.\n", ch);
        return false;
    }

    // Echoes
    act("A wave of dark power washes over this place, forming into a shadowy bier!", ch, NULL, NULL, TO_ALL);

    // Add room effect
    AFFECT_DATA af = {0};
    af.where    = TO_ROOM_AFF;
    af.type     = sn;
    af.level    = level;
    af.duration = level;
    af.modifier = ch->id;
    affect_to_room(ch->in_room, &af);

    // Add cooldown
    af.where    = TO_AFFECTS;
    af.level    = 0;
    af.duration = 80 - UMAX(0, get_skill(ch, sn) - 70);
    affect_to_char(ch, &af);

    return true;
}

bool is_in_stasis(CHAR_DATA & ch)
{
    if (ch.in_room == NULL)
        return false;

    AFFECT_DATA * paf(get_room_affect(ch.in_room, gsn_stasisrift));
    return (paf != NULL && paf->modifier != 0);
}

void destroy_stasisrift(ROOM_INDEX_DATA & room, int targetVnum)
{
    // Get the target room
    ROOM_INDEX_DATA * targetRoom(get_room_index(targetVnum));
    if (targetRoom == NULL)
    {
        // No target room, so get the generic recall room
        targetRoom = get_room_index(ROOM_VNUM_TEMPLE);
        if (targetRoom == NULL)
        {
            bug("Unable to destroy stasis rift, no target room available", 0);
            return;
        }
    }

    // Move all people out of the room
    CHAR_DATA * ch_next;
    for (CHAR_DATA * ch(room.people); ch != NULL; ch = ch_next)
    {
        ch_next = ch->next_in_room;
        char_from_room(ch);
        char_to_room(ch, targetRoom);
        if (targetRoom->vnum != 0)
            ch->was_in_room = NULL;

        act("The rift collapses, and you go tumbling back to the physical world!", ch, NULL, NULL, TO_CHAR);
        act("An inky portal suddenly appears, disgorging $n before vanishing again!", ch, NULL, NULL, TO_ROOM);
    }

    // Move all items out of the room
    OBJ_DATA * obj_next;
    for (OBJ_DATA * obj(room.contents); obj != NULL; obj = obj_next)
    {
        obj_next = obj->next_content;
        obj_from_room(obj);
        obj_to_room(obj, targetRoom);
    }

    // Now clean up the room
    free_room_area(&room);
}

bool check_stasisrift(CHAR_DATA & ch, ROOM_INDEX_DATA & room)
{
    // Check for the effect
    AFFECT_DATA * paf(get_room_affect(&room, gsn_stasisrift));
    if (paf == NULL || paf->modifier != 0)
        return false;

    // Check for a save
    if ((ON_GROUND(&ch) && is_affected(&ch, gsn_anchor)) || saves_spell(paf->level, NULL, &ch, DAM_NEGATIVE))
    {
        act("A wave of energy tugs you towards the dark portal, but you stand firm against it!", &ch, NULL, NULL, TO_CHAR);
        act("A wave of energy tugs $n towards the dark portal, but $e stands firm against it!", &ch, NULL, NULL, TO_ROOM);
        return false;
    }

    // Echoes
    act("A wave of energy rips at you, pulling you inexorably into the dark, swirling portal!", &ch, NULL, NULL, TO_CHAR);
    act("A wave of energy drags $n into the dark portal, which swallows $m whole!", &ch, NULL, NULL, TO_ROOM);
    act("The blackened portal contracts to a pinpoint, then vanishes.", &ch, NULL, NULL, TO_ROOM);

    // Create the rift
    ROOM_INDEX_DATA * rift(new_room_area(ch.in_room->area));
    copy_string(rift->name, "An Immutable Darkness");
    copy_string(rift->description, "All around you pure darkness, silent and unchanging. Here there are no sounds to delight your ears, no sights to fill your vision. There is only the same interminable blackness, stretching on and on with a vastness which defies thought itself.");
    rift->description = format_string(rift->description);
    rift->room_flags = ROOM_NO_RECALL|ROOM_NOGATE|ROOM_NOSUM_TO|ROOM_NOSUM_FROM|ROOM_NOWEATHER|ROOM_NOWHERE|ROOM_NOMAGIC;
    rift->sector_type = SECT_UNUSED;

    // Set up exits to point back to the same room
    for (unsigned int i(0); i < Direction::Max; ++i)
    {
        rift->exit[i] = new_exit();
        rift->exit[i]->u1.to_room = rift;
    }

    // Set up the rift to decay
    AFFECT_DATA af = {0};
    af.where    = TO_ROOM_AFF;
    af.type     = gsn_stasisrift;
    af.level    = paf->level;
    af.modifier = (ch.was_in_room == NULL ? ch.in_room->vnum : ch.was_in_room->vnum);
    af.duration = 4;
    affect_to_room(rift, &af);

    // Move the character
    if (ch.in_room->vnum != 0) 
        ch.was_in_room = ch.in_room;

    char_from_room(&ch);
    char_to_room(&ch, rift);

    // Clear the effect from the room
    affect_remove_room(&room, paf);
    return true;
}

bool spell_stasisrift(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not ready to tear open another rift just yet.\n", ch);
        return false;
    }

    // Check whether a rift is already present
    if (ch->in_room == NULL || room_is_affected(ch->in_room, sn))
    {
        send_to_char("There is already a rift here.\n", ch);
        return false;
    }

    // Echoes
    act("You tear a rift in space with an effort of will, causing a darkened, swirling portal to materialize suddenly!", ch, NULL, NULL, TO_CHAR);
    act("$n gestures, and a darkened, swirling portal materializes suddenly!", ch, NULL, NULL, TO_ROOM);
 
    // Create the rift
    AFFECT_DATA af = {0};
    af.where    = TO_ROOM_AFF;
    af.type     = sn;
    af.level    = level;
    af.duration = level / 5;
    affect_to_room(ch->in_room, &af);

    // Apply cooldown
    af.where    = TO_AFFECTS;
    af.duration = 40 - UMAX(0, (get_skill(ch, sn) - 60) / 10);
    affect_to_char(ch, &af);
    return true;
}

bool spell_chill_touch( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int baseDam = 10 + (level / 3);
    int dam         = number_range(baseDam / 2, baseDam * 2);
    if ( !saves_spell( level, ch, victim,DAM_COLD ) )
    {
        act("$n turns blue and shivers.",victim,NULL,NULL,TO_ROOM);
        AFFECT_DATA af;
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
        dam /= 2;

    damage_old( ch, victim, dam, sn, DAM_COLD,TRUE );
    return true;
}

bool spell_abominablerune(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA *paf;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (!ch->in_room)
	return FALSE;

    if (room_is_affected(ch->in_room, gsn_abominablerune))
    {
	send_to_char("An abominable rune is already etched here.\n\r", ch);
	return FALSE;
    }

    for (paf = ch->affected; paf; paf = paf->next)
	if ((paf->type == gsn_abominablerune) && (paf->modifier == 0))
	{
	    send_to_char("You are not yet ready to place another rune.\n\r", ch);
	    return FALSE;
	}

    if (ch->in_room->vnum == 0)
    {
	send_to_char("An abominable rune may not be placed here.\n\r", ch);
	return FALSE;
    }

/*    if (room_is_affected(ch->in_room, gsn_sanctify))
    {
	send_to_char("A holy aura protects this place.\n\r", ch);
	return FALSE;
    }*/

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = level / 10;
    af.modifier  = ch->id;
    af.location  = APPLY_NONE;
    af.bitvector = 0;
    affect_to_room(ch->in_room, &af);

    af.duration = 24;
    af.modifier = 0;
    affect_to_char(ch, &af);

    send_to_char("In the air, you trace out black, hideous patterns in the shape of some abominable rune.\n\r", ch);
    send_to_char("Once you complete the figure, its black lines remain in place, maintaining its unspeakable consistency for all to see.\n\r", ch);

    act("$n makes several dark, mystical gestures, tracing out a hideous shape.", ch, NULL, NULL, TO_ROOM);
    act("As $n completes $s tracing, the abominable rune $e has created remains in place, floating in the center of the room.", ch, NULL, NULL, TO_ROOM);

    return TRUE;
}


bool spell_agony( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    
    if (is_affected(victim, sn))
    {
        act("They're already in agony.", ch, NULL, NULL, TO_CHAR);
        return FALSE;
    }

    act("You glare darkly at $N!",ch, NULL, victim, TO_CHAR);
    act("$n glares darkly at $N!",ch, NULL, victim, TO_NOTVICT);
    act("$n glares darkly at you!",ch, NULL, victim, TO_VICT);

	if (is_loc_not_affected(victim, gsn_anesthetize, APPLY_NONE))
	{
		send_to_char("They are immune to pain.\n\r", ch);
		send_to_char("The agony fails to take hold, as you are immune to pain.\n\r", victim);
		return TRUE;
	}

    int levelMod(0);
    if (is_affected(victim, gsn_barbsofalthajji))
        levelMod = 20;

    if (saves_spell(level + levelMod, ch, victim,DAM_OTHER))
    {
        send_to_char("They resist your spell.\n\r",ch);
        return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level / 11;
    af.location  =  APPLY_STR;
    af.modifier  = -1 * (level/12);
    af.bitvector = 0;
    affect_to_char( victim, &af );
    send_to_char( "You scream in agony as pain wracks your body!\n\r", victim );
    act("$n screams in agony as pain wracks $s body!",victim,NULL,NULL,TO_ROOM);
    return TRUE;
}


bool spell_banish( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *demon = (CHAR_DATA *) vo;

    if (!is_affected(demon, gsn_demoniccontrol) || (demon->leader != ch))
    {
	act("$N does not serve you!", ch, NULL, demon, TO_CHAR);
	return FALSE;
    }

    act("With a complex series of gestures, $n splits reality wide and sends $N spinning back into the void!", ch, NULL, demon, TO_ROOM);
    act("With a complex series of gestures, you split reality wide and send $N spinning back into the void!", ch, NULL, demon, TO_CHAR);

    extract_char(demon, TRUE);

    return TRUE;
}

bool spell_bladeofvershak(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *vObj = (OBJ_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    OBJ_DATA *symbol;
    CHAR_DATA *demon;

    if (IS_NPC(ch))
    {
	send_to_char("You lack the power to bind the power of Selb-Kar.\n\r", ch);
	return FALSE;
    }

    if (!CAN_WEAR(vObj, ITEM_WIELD))
    {
	send_to_char("Only weapons may be bound with the power of Selb-Kar.\n\r", ch);
	return FALSE;
    }

    if (IS_OBJ_STAT(vObj, ITEM_ANTI_EVIL) || IS_OBJ_STAT(vObj, ITEM_BLESS) || CAN_WEAR(vObj, ITEM_NO_SAC))
    {
	act("$p cannot be bound with the power of Selb-Kar.", ch, vObj, NULL, TO_CHAR);
	return FALSE;
    }

    if (obj_is_affected(vObj, gsn_demon_bind))
    {
	act("$p has already been bound with a demonic spirit.", ch, vObj, NULL, TO_CHAR);
	return FALSE;
    }

    if ((silver_state != SILVER_FINAL)
     && (time_info.hour >= season_table[time_info.season].sun_up)
     && (time_info.hour < season_table[time_info.season].sun_down))
    {
	send_to_char("This ritual may not be completed during the day.\n\r",ch);
	return FALSE;
    }

    if (ch->in_room)
    {
	for (symbol = ch->in_room->contents; symbol; symbol = symbol->next_content)
	{
	    if (symbol->pIndexData->vnum == OBJ_VNUM_SYMBOL_BIND)
	    {
		if (symbol->value[0] == 0 || symbol->value[1] != INSMAT_BONEDUST)
		{
		    demon = create_mobile(get_mob_index(MOB_VNUM_DEMON_VERSHAK));

		    char_to_room(demon, ch->in_room);

		    send_to_char("As you complete the casting, a powerful whirlwind of dark smoke begins to swirl above the trigon of binding.\n\r", ch);
		    send_to_char("From within the smoke, several impossible angles coalesce into a demonic form.\n\r", ch);
		    if(number_bits(1) == 0)
		    {
			do_yell(demon, "At last I am free!");
		    	act("$n looks down at the flawed symbol and sneers arrogantly.", demon, NULL, ch, TO_ROOM);
		    	do_say(demon, "Thank you, mortal.");
		    	do_say(demon, "Now you must die.");
		    	act("$n laughs darkly.", demon, NULL, NULL, TO_ROOM);
		    	demon->demontrack = ch;
		    	multi_hit(demon, ch, TYPE_UNDEFINED);
		    	return TRUE;
		    }
		    else
		    {
			do_say(demon,"Your soul will be Shaped!");
			spell_plague_madness(gsn_plague_madness,demon->level,demon,(CHAR_DATA *) ch,skill_table[gsn_plague_madness].target);
			act("$n steps back into the smoke, disappearing with it.",demon,NULL,NULL,TO_ROOM);
			extract_char(demon,TRUE);
			return TRUE;
		    }
		}
		else
		    break;
	    }
	}
    }
    else
	return FALSE;

    if (!symbol)
    {
	send_to_char("There must be a trigon of binding present to cast this.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_OBJECT;
    af.type	 = sn;
    af.level	 = level;
    af.location	 = 0;
    af.modifier  = 0;
    af.duration  = -1;
    af.bitvector = ITEM_HUM;
    affect_to_obj(vObj, &af);

    af.type	 = gsn_demon_bind;
    af.location  = 0;
    af.modifier  = ch->id;
    affect_to_obj(vObj, &af);

    act("As you complete the casting, a piercing, unholy scream fills the space around you, slowly dulling as it is bound into $p.", ch, vObj, NULL, TO_CHAR);
    act("As $n completes the casting, a piercing, unholy scream fills the space around you, slowly dulling as it is bound into $p.", ch, vObj, NULL, TO_ROOM);

    return TRUE;
}

bool spell_blight(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (victim == ch)
    {
        send_to_char("You cannot cast this spell on yourself.\n\r", ch);
        return FALSE;
    }

    if (is_affected(victim,sn))
    {
	act("$N is already overcome with helplessness.",ch,NULL,victim,TO_CHAR);
	return FALSE;
    }
    
    if (saves_spell(level + 2, ch, victim,DAM_OTHER))
    {
        send_to_char("They resist your magic.\n\r",ch);
        return TRUE;
    }

    if (get_resist(victim, RESIST_NEGATIVE) > -100)
    { 
        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = level;
        af.duration  = 6;
        af.location  = APPLY_RESIST_NEGATIVE;
        af.modifier  = UMIN(-10,-10 - (aura_grade(ch)-1) * 5);
        af.bitvector = 0;
        affect_to_char( victim, &af );
     } 
 
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 6;
    af.location  = APPLY_SAVES;
    af.modifier  = level/12;
    af.bitvector = 0;
    affect_to_char( victim, &af );
    
    act("You call forth the blight of Adduthala upon $N!",ch, NULL, victim, TO_CHAR);
    act("$n gestures darkly at $N.",ch, NULL, victim, TO_NOTVICT);
    act("$n gestures darkly, and a feeling of helplessness overcomes you.",ch, NULL, victim, TO_VICT);

    return TRUE;
}


bool spell_blindness( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;

    if (IS_AFFECTED(victim, AFF_BLIND))
    {
        send_to_char( "They are already blinded.\n\r", ch);
        return FALSE;
    }

    if (IS_SET(victim->imm_flags, IMM_BLIND))
    {
        send_to_char("They resist your blindness.\n\r", ch);
        return TRUE;
    }

    int effectLevel(level);
    check_maleficinsight_attack(ch, victim, level, effectLevel);

    if (saves_spell(level, ch, victim,DAM_NEGATIVE))
    {
        send_to_char("They resist your blindness.\n\r", ch);
        return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = effectLevel;
    af.location  = APPLY_HITROLL;
    af.modifier  = -4;
    af.duration  = level/6;
    af.bitvector = AFF_BLIND;
    affect_to_char( victim, &af );
    send_to_char("The darkness of the void fills your eyes, and you are blinded!\n\r", victim);
    act("$n's eyes cloud with darkness, and $e appears to be blinded.", victim, NULL, NULL, TO_ROOM);

    check_boonoftheleech(ch, victim, sn, level);
    return TRUE;
}

bool spell_caressofpricina(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *vObj = (OBJ_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    OBJ_DATA *symbol;
    CHAR_DATA *demon;

    if (IS_NPC(ch))
    {
	send_to_char("You lack the power to bind the caress of Pricina.\n\r", ch);
	return FALSE;
    }

    if (!CAN_WEAR(vObj, ITEM_WIELD))
    {
	send_to_char("Only weapons may be bound with the caress of Pricina.\n\r", ch);
	return FALSE;
    }

    if (IS_OBJ_STAT(vObj, ITEM_ANTI_EVIL) || IS_OBJ_STAT(vObj, ITEM_BLESS) || CAN_WEAR(vObj, ITEM_NO_SAC))
    {
	act("$p cannot be bound with the caress of Pricina.", ch, vObj, NULL, TO_CHAR);
	return FALSE;
    }

    if (obj_is_affected(vObj, gsn_demon_bind))
    {
	act("$p has already been bound with a demonic spirit.", ch, vObj, NULL, TO_CHAR);
	return FALSE;
    }

    if ((silver_state != SILVER_FINAL)
     && (time_info.hour >= season_table[time_info.season].sun_up)
     && (time_info.hour < season_table[time_info.season].sun_down))
    {
	send_to_char("This ritual may not be completed during the day.\n\r",ch);
	return FALSE;
    }

    if (ch->in_room)
    {
	for (symbol = ch->in_room->contents; symbol; symbol = symbol->next_content)
	{
	    if (symbol->pIndexData->vnum == OBJ_VNUM_SYMBOL_BIND)
	    {
		if (symbol->value[0] == 0 || symbol->value[1] != INSMAT_BLOOD_CASTER)
		{
		    demon = create_mobile(get_mob_index(MOB_VNUM_DEMON_SUCCUBI));

		    char_to_room(demon, ch->in_room);

		    send_to_char("As you complete the casting, a powerful whirlwind of dark smoke begins to swirl above the trigon of binding.\n\r", ch);
		    send_to_char("The smoke swirls around you, revealing a sensuous, feminine horror as it lifts.\n\r", ch);
		    if (number_bits(1) == 0)
		    {
			do_yell(demon, "At last I am free!");
		    	act("$n looks down at the flawed symbol and sneers arrogantly.", demon, NULL, ch, TO_ROOM);
		    	do_say(demon, "Thank you, mortal.");
		    	do_say(demon, "Now you must die.");
		    	act("$n laughs darkly.", demon, NULL, NULL, TO_ROOM);
		    	multi_hit(demon, ch, TYPE_UNDEFINED);
		    	demon->demontrack = ch;
		    	return TRUE;
		    }
		    else
		    {
		        act("$n takes your face in her hands, and kisses you deeply.",demon, NULL, ch, TO_VICT);
			act("$n takes $N's face in her hands, and kisses $M deeply.", demon, NULL, ch, TO_NOTVICT);
			af.where = TO_AFFECTS;
			af.type = gsn_succubuskiss;
			af.duration = 24;
			af.level = level;
			af.modifier = ch->id;
			af.location = APPLY_NONE;
			af.bitvector = 0;
			affect_to_char(ch, &af);
			act("$n returns to the whirlwind of smoke, and it dissipates.",demon,NULL,ch,TO_ROOM);
			extract_char(demon,TRUE);
			return TRUE;
		    }
		}
		else
		    break;
	    }
	}
    }
    else
	return FALSE;

    if (!symbol)
    {
	send_to_char("There must be a trigon of binding present to cast this.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_OBJECT;
    af.type	 = sn;
    af.level	 = level;
    af.location	 = 0;
    af.modifier  = 0;
    af.duration  = -1;
    af.bitvector = 0;
    affect_to_obj(vObj, &af);

    af.type	 = gsn_demon_bind;
    af.location  = 0;
    af.modifier  = ch->id;
    affect_to_obj(vObj, &af);

    act("The stench of sex and death surrounds you, as a lithesome shadow forms in the air and is drawn, bound and screaming, into $p.", ch, vObj, NULL, TO_CHAR);
    act("The stench of sex and death surrounds you, as a lithesome shadow forms in the air and is drawn, bound and screaming, into $p.", ch, vObj, NULL, TO_ROOM);

    return TRUE;
}

bool spell_enervatingray( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    int effectLevel(level);
    check_maleficinsight_attack(ch, victim, level, effectLevel);

    int dam(number_range(level / 2, level * 2));
    if (!saves_spell(level, ch, victim, DAM_NEGATIVE))
    {
        act("You strike $N with a ray of green light, and $E looks weaker.",ch,NULL,victim,TO_CHAR);
        act("$N is struck by a ray of green light, and looks weaker.",ch,NULL,victim,TO_NOTVICT);
        act("You are struck by a ray of green light, and feel weaker.", ch, NULL, victim, TO_VICT);

        AFFECT_DATA af = {0};
        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = effectLevel;
        af.duration  = 6;
        af.location  = APPLY_STR;
        af.modifier  = -1;
        af.bitvector = 0;
        affect_join(victim, &af);
    }
    else
    {
        act("You strike $N with a ray of green light.",ch,NULL,victim,TO_CHAR);
        act("$N is struck by a ray of green light.",ch,NULL,victim,TO_NOTVICT);
        act("You are struck by a ray of green light.", victim, NULL, NULL, TO_CHAR);
        dam /= 2;
    }

    check_hatefire_damage(sn, DamageInfo(dam, DAM_NEGATIVE), ch, victim);
    return true;
}

bool spell_cloakofthevoid(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
    if (is_affected(ch, gsn_cloakofthevoid))
    {
        send_to_char("You are already wrapped in void.\n\r",ch);
        return false;
    }

    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 7;
    af.location  = APPLY_AC;
    af.modifier  = -1 * level;
    affect_to_char( ch, &af );
    send_to_char("You feel the cold immunity of the void surround you.\n\r", ch);

    if (is_affected(ch, gsn_sanctuary))
    {
        send_to_char("The white aura around your body fades.\n\r", ch);
        affect_strip(ch, gsn_sanctuary);
    }
 
    return true;
}

bool spell_commandword( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_STRING_LENGTH];

    target_name = one_argument( target_name, arg );

    if ((victim = get_char_room(ch, arg)) == NULL)
    {
        send_to_char("Your spell fails to find a target.\n\r", ch);
        return FALSE;
    }

    if (victim->level > 52 && ch->level < victim->level)
    {
        send_to_char("They are too powerful for you to command.\n\r", ch);
        return FALSE;
    }

    if (is_safe_spell(ch, victim, FALSE))
    {
        act("The gods protect $N.", ch, NULL, victim, TO_NOTVICT);
        act("The gods protect $N.", ch, NULL, victim, TO_CHAR);
        act("The gods protect you.", ch, NULL, victim, TO_VICT);
        return TRUE;
    }

    if (saves_spell(level, ch, victim, DAM_CHARM) || (IS_NPC(victim) && IS_SET(victim->act, ACT_NOSUBDUE)) || IS_SET(victim->imm_flags, IMM_CHARM))
    {
        if (can_see(ch, victim) && !IS_NPC(victim))
                {
                sprintf(buf, "Help! %s is attacking me!", PERS(ch, victim));
                do_autoyell(victim, buf);
                }
        else
                {
                sprintf(buf, "Help! Someone is attacking me!");
                do_autoyell(victim, buf);
                }
        if (IS_AWAKE(victim))
                multi_hit(victim, ch, TYPE_UNDEFINED);
        return TRUE;
    }
	if (IsOrderBanned(target_name))
        {
        send_to_char("The gods won't allow such an order.\n\r", ch);
        sprintf(buf, "%s tried to force someone to do a naughty with command word.\n\r", ch->name);
        bug(buf, 0);
        return TRUE;
        }
sprintf(buf, "%s compels you to '%s'.\n\r", PERS(ch, victim), target_name);
send_to_char(buf, victim);
send_to_char("You compel them to obey your will.\n\r", ch);
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.point	 = (void *) ch;
    af.duration  = 0;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char( victim, &af );
    victim->commander = ch;
interpret(victim, target_name);
if (!victim || !victim->valid)
return TRUE;
victim->commander = NULL;
affect_strip(victim, gsn_commandword);

    return TRUE;
}

static bool handle_curse_room(int sn, int level, CHAR_DATA * ch)
{
    // Sanity check
    if (ch->in_room == NULL)
        return false;

    // Check for already cursed
    if (IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL) || room_is_affected(ch->in_room, sn))
    {
        send_to_char("This place is already cursed.\n", ch);
        return false;
    }

    // Check for sanctify and holy ground
    if (room_is_affected(ch->in_room, gsn_sanctify) || area_is_affected(ch->in_room->area, gsn_holyground))
    {
        send_to_char("You cannot befoul the holiness of this place.\n", ch);
        return false;
    }

    // Adjust level for familiars
    if (is_familiar_present(*ch, gsn_callraven, OBJ_VNUM_FAMILIAR_RAVEN))
        level += 3;

    // Echoes
    act("You rattle off a vile curse, binding this place with unholy magic!", ch, NULL, NULL, TO_CHAR); 
    act("$n rattles off a vile curse, binding this place with unholy magic!", ch, NULL, NULL, TO_ROOM);

    // Apply effect
    AFFECT_DATA af = {0};
    af.where    = TO_ROOM_AFF;
    af.type     = sn;
    af.level    = level;
    af.duration = 1 + (level > 51 ? 1 : 0);

    if (number_percent() <= get_skill(ch, gsn_touchofthedesecrator))
    {
        check_improve(ch, NULL, gsn_touchofthedesecrator, true, 4);
        af.duration += 2;
    }

    affect_to_room(ch->in_room, &af);

    SET_BIT(ch->in_room->room_flags, ROOM_NO_RECALL);
    return true;
}

static bool handle_curse_obj(int sn, int level, CHAR_DATA * ch, OBJ_DATA * obj)
{
    if (IS_OBJ_STAT(obj, ITEM_EVIL))
    {
        act("$p is already filled with evil.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Adjust level for familiars
    if (is_familiar_present(*ch, gsn_callraven, OBJ_VNUM_FAMILIAR_RAVEN))
        level += 3;

    if (IS_OBJ_STAT(obj,ITEM_BLESS))
    {
        AFFECT_DATA *paf(get_obj_affect(obj, gsn_bless));
        if (!saves_dispel(level, paf != NULL ? paf->level : obj->level, 0))
        {
            if (paf != NULL) affect_remove_obj(obj, paf);
            act("$p glows with a red aura.", ch, obj, NULL, TO_ALL);
            REMOVE_BIT(obj->extra_flags[0], ITEM_BLESS);
            return true;
        }

        act("The holy aura of $p is too powerful for you to overcome.", ch, obj, NULL, TO_CHAR);
        return true;
    }

    SET_BIT(obj->extra_flags[0], ITEM_EVIL);

    AFFECT_DATA af = {0};
    af.where        = TO_OBJECT;
    af.type         = sn;
    af.level        = level;
    af.duration     = 12;
    af.location     = APPLY_SAVES;
    af.modifier     = 1;
    affect_to_obj(obj, &af);
    act("$p glows with a malevolent aura.", ch, obj, NULL, TO_ALL);
    return true;
}

static bool handle_curse_char(int sn, int level, CHAR_DATA * ch, CHAR_DATA * victim)
{
    if (is_safe_spell(ch, victim, FALSE))
    {
        act("The gods protect $N from $n.", ch, NULL, victim, TO_NOTVICT);
        act("The gods protect you from $n.", ch, NULL, victim, TO_VICT);
        act("The gods protect $N from you.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Initiate combat at this point
    if (victim->fighting == NULL && victim != ch && victim->master != ch && !victim->mercy_from
    && ch->in_room == victim->in_room && victim->position > POS_SLEEPING && !IS_NAFFECTED(victim, AFF_FLESHTOSTONE))
    {
        check_killer(victim, ch);
        victim->fighting = ch;
        multi_hit(victim, ch, TYPE_UNDEFINED);
    }

    if (IS_AFFECTED(victim,AFF_CURSE))
    {
        act("$N is already cursed.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Adjust level for familiars
    if (is_familiar_present(*ch, gsn_callraven, OBJ_VNUM_FAMILIAR_RAVEN))
        level += 3;

    int effectLevel(level);
    check_maleficinsight_attack(ch, victim, level, effectLevel);
    if (saves_spell(level, ch, victim,DAM_NEGATIVE))
    {
        act("$N resists your curse.", ch, NULL, victim, TO_CHAR);
        return true;
    }

    int duration(level + 20);
    if (number_percent() <= get_skill(ch, gsn_touchofthedesecrator))
        check_improve(ch, NULL, gsn_touchofthedesecrator, true, 4);
    else
        duration /= 2;
 
    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = effectLevel;
    af.duration  = duration;
    af.location  = APPLY_HITROLL;
    af.modifier  = -1 * (level / 8);
    af.bitvector = AFF_CURSE;
    affect_to_char( victim, &af );

    af.location  = APPLY_SAVING_SPELL;
    af.modifier  = level / 8;
    affect_to_char( victim, &af );

    send_to_char( "You feel unclean.\n\r", victim );
    if ( ch != victim )
        act("$N looks very uncomfortable.",ch,NULL,victim,TO_CHAR);

    check_boonoftheleech(ch, victim, sn, level);
    return true;
}


bool spell_curse( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    // Resolve the target
    if (target_name[0] == '\0')
    {
        if (ch->fighting != NULL) 
            return handle_curse_char(sn, level, ch, ch->fighting);

        send_to_char("What did you wish to curse?\n", ch);
        return false;
    }

    if (!str_prefix(target_name, "room")) 
        return handle_curse_room(sn, level, ch);

    OBJ_DATA * obj(get_obj_carry(ch, target_name, ch));
    if (obj != NULL)
        return handle_curse_obj(sn, level, ch, obj);

    CHAR_DATA * victim(get_char_room(ch, target_name));
    if (victim != NULL)
        return handle_curse_char(sn, level, ch, victim);

    send_to_char("You see nothing and nobody by that name here.\n", ch);
    return false;
}

bool spell_cursekijjasku( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( is_affected( ch, sn ) )
    {
        send_to_char("You already bear the curse of Kijjasku.\n\r", ch);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/5;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = 0;
    affect_to_char( ch, &af );

    act("$n pulses with a dark glow.", ch, NULL, NULL, TO_ROOM);
    send_to_char("The words of the curse of Kijjasku burn themselves into your mind.\n\r", ch);

    return TRUE;
}



bool spell_deathwalk( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    DESCRIPTOR_DATA *d, *d_next;

    if (!ch->in_room || !ch->in_room->area)
	return FALSE;

    if (is_affected(ch, gsn_deathwalk))
    {
        send_to_char("You cannot yet summon the power of the deathwalk again.\n\r", ch);
        return FALSE;
    }

/*    if (room_is_affected(ch->in_room, gsn_sanctify))
    {
        send_to_char("A holy aura prevents you from casting your spell here.\n\r",ch);
        return FALSE;
    }*/

    if (area_is_affected(ch->in_room->area, gsn_deathwalk))
    {
        send_to_char("You detect the deathwalk is already in effect.\n\r", ch);
        return FALSE;
    }

    af.where     = TO_AREA;
    af.type 	 = sn;
    af.level     = level;
    af.duration  = 4;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_area(ch->in_room->area, &af);

    af.duration  = 120;
    affect_to_char(ch, &af);

    for (d = descriptor_list; d != NULL; d = d_next)
    {
        d_next = d->next;
        if (d->connected == CON_PLAYING && d->character->in_room && ch->in_room)
        if (d->character->in_room->area == ch->in_room->area)
            send_to_char("A grim silence falls over the surrounding area.\n\r", d->character);
    }

    return TRUE;
}

bool spell_defilementoflogor(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *vObj = (OBJ_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    OBJ_DATA *symbol;
    CHAR_DATA *demon;

    if (IS_NPC(ch))
    {
	send_to_char("You lack the power to bind the defilement of Logor.\n\r", ch);
	return FALSE;
    }

    if (!CAN_WEAR(vObj, ITEM_WIELD))
    {
	send_to_char("Only weapons may be bound with the defilement of Logor.\n\r", ch);
	return FALSE;
    }

    if (IS_OBJ_STAT(vObj, ITEM_ANTI_EVIL) || IS_OBJ_STAT(vObj, ITEM_BLESS) || CAN_WEAR(vObj, ITEM_NO_SAC))
    {
	act("$p cannot be bound with the defilement of Logor.", ch, vObj, NULL, TO_CHAR);
	return FALSE;
    }

    if (obj_is_affected(vObj, gsn_demon_bind))
    {
	act("$p has already been bound with a demonic spirit.", ch, vObj, NULL, TO_CHAR);
	return FALSE;
    }

    if ((silver_state != SILVER_FINAL)
     && (time_info.hour >= season_table[time_info.season].sun_up)
     && (time_info.hour < season_table[time_info.season].sun_down))
    {
	send_to_char("This ritual may not be completed during the day.\n\r",ch);
	return FALSE;
    }

    if (ch->in_room)
    {
	for (symbol = ch->in_room->contents; symbol; symbol = symbol->next_content)
	{
	    if (symbol->pIndexData->vnum == OBJ_VNUM_SYMBOL_BIND)
	    {
		if (symbol->value[0] == 0 || symbol->value[1] != INSMAT_SILVER)
		{
		    demon = create_mobile(get_mob_index(MOB_VNUM_DEMON_LOGOR));

		    char_to_room(demon, ch->in_room);

		    send_to_char("As you complete the casting, a powerful whirlwind of dark smoke begins to swirl above the trigon of binding.\n\r", ch);
		    send_to_char("From within the smoke, a hulking demonic form emerges.\n\r", ch);
		    if (number_bits(1) == 0)
		    {
			do_yell(demon, "At last I am free!");
		    	act("$n looks down at the flawed symbol and sneers arrogantly.", demon, NULL, ch, TO_ROOM);
		    	do_say(demon, "Thank you, mortal.");
		    	do_say(demon, "Now you must die.");
		    	act("$n laughs darkly.", demon, NULL, NULL, TO_ROOM);
		    	multi_hit(demon, ch, TYPE_UNDEFINED);
		    	demon->demontrack = ch;
		    	return TRUE;
		    }
		    else
		    {
			do_say(demon,"The Grand Vizier bids me give you a message.");
			do_say(demon,"'In your future I see only doom.'");
			act("$n stretches out a tendril of darkness toward $p.",demon,vObj,ch,TO_ROOM);
			if (IS_SET(vObj->extra_flags[0], ITEM_NODESTROY))
			{
			    REMOVE_BIT(vObj->extra_flags[0], ITEM_NODESTROY);
			    vObj->condition /= 2;
			    act("$p looks weaker.",ch,vObj,NULL,TO_ROOM);
			}
			else
			{
			    act("$p crumbles into dust.",ch,vObj,NULL,TO_ROOM);
			    extract_obj(vObj);
			}
			do_emote(demon,"laughs nightmarishly. Its red eyes dim and fade, and the cloud dissipates.");
			extract_char(demon,TRUE);
			return TRUE;
		    }
		}
		else
		    break;
	    }
	}
    }
    else
	return FALSE;

    if (!symbol)
    {
	send_to_char("There must be a trigon of binding present to cast this.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_OBJECT;
    af.type	 = sn;
    af.level	 = level;
    af.location	 = 0;
    af.modifier  = 0 /*vObj->value[3]*/;
    af.duration  = -1;
    af.bitvector = 0;
    af.point	 = NULL /*(char *) vObj->obj_str*/;
    affect_to_obj(vObj, &af);

    af.type	 = gsn_demon_bind;
    af.location  = 0;
    af.modifier  = ch->id;
    affect_to_obj(vObj, &af);

    act("As you complete the casting, an inky black mist forms, spiralling eerily around $p, and is gradually drawn into it with an unwholesome shudder.", ch, vObj, NULL, TO_CHAR);
    act("As $n completes the casting, an inky black mist forms, spiralling eerily around $p, and is gradually drawn into it with an unwholesome shudder.", ch, vObj, NULL, TO_ROOM);

    return TRUE;
}

bool spell_demoniccontrol(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA *paf;
    int cost;

    if (ch == victim)
    {
        send_to_char("Your magic dissipates, lacking a suitable target.\n\r", ch);
        return FALSE;
    }

    if (victim->race != race_lookup("demon") || victim->leader != ch || ((paf = affect_find(victim->affected, gsn_demoniccontrol)) == NULL))
    {
        act("You channel magical energy toward $N, but $E remains unfazed.", ch, NULL, victim, TO_CHAR);
        return TRUE;
    }

    cost = (1 << paf->modifier) * 50;

    if (ch->mana < cost)
    {
	act("You lack the energy to re-bind $N.", ch, NULL, victim, TO_CHAR);
	return TRUE;
    }

    expend_mana(ch, cost);
    paf->modifier++;
    paf->duration  = (((victim->pIndexData->vnum >= MOB_VNUM_LESSER_BOUND_FIRST)
                    && (victim->pIndexData->vnum <= MOB_VNUM_LESSER_BOUND_LAST)) ? 48 :
		      ((victim->pIndexData->vnum >= MOB_VNUM_GREATER_BOUND_FIRST)
	  	    && (victim->pIndexData->vnum <= MOB_VNUM_GREATER_BOUND_LAST)) ? 12 : 24);


    act("You channel magical energy into the mystical bond, strengthening and reinforcing your control over $N... for a time.", ch, NULL, victim, TO_CHAR);
 
    WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE*2));

    return TRUE;
}

bool spell_desecration(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    if (!ch->in_room)
        return FALSE;

    if (is_affected(ch, sn))
    {
	send_to_char("You are not yet ready to desecrate another place.\n\r", ch);
	return FALSE;
    }

    if (room_is_affected(ch->in_room, sn))
    {
	send_to_char("This place has already been desecrated.\n\r", ch);
	return FALSE;
    }

    if (room_is_affected(ch->in_room, gsn_sanctify) || area_is_affected(ch->in_room->area, gsn_holyground))
    {
        send_to_char("The sanctity of this place cannot be fouled.\n\r", ch);
        return FALSE;
    }

    if (ch->in_room->sector_type == SECT_AIR)
    {
	send_to_char("This place cannot be desecrated.\n\r", ch);
	return FALSE;
    }

    AFFECT_DATA af = {0};
    af.where	 = TO_ROOM;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = 24;
    affect_to_room(ch->in_room, &af);

    af.where	 = TO_AFFECTS;

    // Base duration for touch of the desecrator; if successful, don't modify it
    if (number_percent() <= get_skill(ch, gsn_touchofthedesecrator))
        check_improve(ch, NULL, gsn_touchofthedesecrator, true, 1);
    else
    {
        // No touch of the desecrator, so use a longer cooldown
        check_improve(ch, NULL, gsn_touchofthedesecrator, false, 1);
        af.duration = 50;
    }

    affect_to_char(ch, &af);


    send_to_char("A dark miasma rises from the ground of this place, as your sinister magic takes hold.\n\r", ch);
    act("$n chants a sinister spell, and a dark miasma rises from the ground of this place.", ch, NULL, NULL, TO_ROOM);

    return TRUE;
}

bool spell_demoncall(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *vch, *vch_next;
    bool found = FALSE;
    if (!ch->in_room)
	return FALSE;

    for (vch = char_list; vch; vch = vch_next)
    {
	vch_next = vch->next;

	if ((vch->in_room != ch->in_room) && (vch->leader == ch) && is_affected(vch, gsn_demoniccontrol))
	{
	    found = TRUE;
	    act("$n disappears suddenly.", vch, NULL, NULL, TO_ROOM);
	    char_from_room(vch);
	    char_to_room(vch, ch->in_room);
	    act("$n appears suddenly.", vch, NULL, NULL, TO_ROOM);
	}
    }
    if (!found)
        send_to_char("You have no demons to call.\n\r",ch);
    return TRUE;
}

bool spell_demonichowl (int sn, int level, CHAR_DATA *ch, void *vo, int target)
{ 
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *vch, *vch_next;

    if (ch->fighting == NULL)
    {
        send_to_char("You need the bloodlust of combat to invoke the demonic powers.\n\r", ch);
        return FALSE;
    }

    if (is_affected(ch, gsn_demonichowl))
    {
        send_to_char("You cannot invoke the dark powers again so soon.\n\r", ch);
        return FALSE;
    }

    act("{c$n cries out in an exultant howl, roaring with a demonic fury!{x",ch, NULL, NULL, TO_ROOM);
    act("{cYou howl exultantly, roaring as the demonic forces flow through you!{x",ch,NULL,NULL,TO_CHAR);

    multi_hit(ch, ch->fighting, TYPE_UNDEFINED);

    af.where        = TO_NAFFECTS;
    af.type         = gsn_demonichowl;
    af.level        = ch->level;
    af.duration     = 6;
    af.location     = 0;
    af.bitvector    = AFF_RALLY;
    af.modifier     = 0;
    affect_to_char(ch, &af);
    if (ch->in_room == NULL)
    	return TRUE;
    for (vch = ch->in_room->people; vch; vch = vch_next)
    {
        vch_next = vch->next_in_room;
        if (is_same_group(ch, vch))
        {
            if (is_an_avatar(vch))
                continue;
        
            vch->hit = UMIN(vch->max_hit, vch->hit+vch->level);
            send_to_char( "Your heart pounds as you are caught up in the fury of the demonic howl!\n\r", vch);
        }
        else if (!is_same_group(ch, vch)
         && !saves_spell(level, ch, vch, DAM_FEAR)
         && vch->fighting && is_same_group(vch->fighting, ch)
         && !is_affected(vch, gsn_mindshell)
         && !is_affected(vch, gsn_mindshield)
         && !is_affected(vch, gsn_courage)
         && !IS_SET(vch->act,ACT_NOSUBDUE))
        {
            send_to_char("A pulse of raw terror courses down your spine at the sound of the demonic howl!\n\r", vch);
            do_flee(vch, "");
        }
    }  
       
    return TRUE; 
}

bool spell_demonpos( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, gsn_demonpos))
    {
	if (IS_OAFFECTED(ch, AFF_DEMONPOS))
	    send_to_char("You are already possessed by a demonic spirit.\n\r", ch);
	else
            send_to_char("To channel the power of a demon again would destroy you.\n\r", ch);
        return FALSE;
    }

    send_to_char("As you complete the chant, a cold wind rushes about you.\n\r",ch);
    send_to_char("You open your mouth, and raise your arms exultantly.\n\r",ch);
    send_to_char("A dark mist roars into your body, filling you with an alien power!\n\r",ch);
    send_to_char("You clench your eyes shut in pain, convulsing as the possession completes itself.\n\r",ch);
    send_to_char("When you next look upon the world, it is through the eyes of a demon.\n\r",ch);

    act("As $n completes $s chant, a cold wind swirls about $m.",ch, NULL, NULL, TO_ROOM);
    act("$n raises $s arms in exultant agony, screaming as a dark mist pours into $s throat.", ch, NULL, NULL, TO_ROOM);
    act("A wash of inhuman emotions crosses $n's face, and $e grins, wickedly.", ch, NULL, NULL, TO_ROOM);


/* We use an oaffect here, since we want to take advantage of
   quick bitvector checking in places like update.c, etc. */

    af.where     = TO_OAFFECTS;
    af.type      = gsn_demonpos;
    af.level     = ch->level;
    af.duration  = 8;
    af.location  = APPLY_HIT;
    af.modifier  = level + 80;
    af.bitvector = AFF_DEMONPOS;
    affect_to_char(ch, &af);

    ch->hit += (level + 80);
    
    af.location = APPLY_HITROLL;
    af.modifier = level/2+5;
    af.bitvector = 0;
    affect_to_char(ch,&af);
  
    af.location = APPLY_DAMROLL;
    affect_to_char(ch,&af);   

    af.location = APPLY_RESIST_HOLY;
    af.modifier = -20;
    affect_to_char(ch,&af);

/* This is the counter for determining when you cast re-cast the spell. */

    af.where     = TO_AFFECTS;
    af.duration  = 24;
    af.location  = APPLY_HIDE;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    affect_strip(ch, gsn_aegisofgrace);
    affect_strip(ch, gsn_spiritshield);
    affect_strip(ch, gsn_lesserspiritshield);
    affect_strip(ch, skill_lookup("protection from evil"));
    affect_strip(ch, gsn_focusmind);

    add_event((void *) ch, 1, &event_demonpos);

    return TRUE;
}

bool spell_drain(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    static const int MaxKiss(60);
    static const int MaxKissDuration(4);

    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA * kiss(get_affect(victim, gsn_kissoftheblackenedtears));

    // Adjust level for bat familiar in room
    if (is_familiar_present(*ch, gsn_callbat, OBJ_VNUM_FAMILIAR_BAT))
        level += 3;

    // Calculate damage
    int dam = dice(((level + 2) / 2), 7);
    if (kiss != NULL)
        dam += kiss->modifier;

    if (saves_spell(level, ch, victim, DAM_NEGATIVE))
        dam /= 2;

    // Deal damage and restore caster's health
    act("$n unleashes a bolt of pure darkness upon $N!", ch, NULL, victim, TO_ROOM);
    check_hatefire_damage(sn, DamageInfo(dam, DAM_NEGATIVE), ch, victim);
    ch->hit = UMIN(ch->hit + (dam / 3), ch->max_hit);

    // Check for kiss of the blackened tears
    if (number_percent() <= get_skill(ch, gsn_kissoftheblackenedtears) && !saves_spell(level, ch, victim, DAM_MENTAL))
    {
        check_improve(ch, victim, gsn_kissoftheblackenedtears, true, 1);

        // Apply or intensify effect
        int mod(1 + (level / 10));
        if (kiss == NULL)
        {
            send_to_char("Your breathing quickens as a sensation of ecstatic anticipation fills you!\n", victim);
            act("$n's breathing quickens, $s expression glazing over.", victim, NULL, NULL, TO_ROOM);

            AFFECT_DATA af = {0};
            af.where    = TO_AFFECTS;
            af.type     = gsn_kissoftheblackenedtears;
            af.level    = level;
            af.duration = MaxKissDuration;
            af.modifier = UMIN(MaxKiss, mod);
            affect_to_char(victim, &af);
        }
        else
        {
            if (kiss->modifier < MaxKiss)
            {
                kiss->modifier = UMIN(MaxKiss, kiss->modifier + mod);
                act("$N falls deeper under your spell, enraptured by your power!", ch, NULL, victim, TO_CHAR);
            }
            else if (kiss->duration < MaxKissDuration)
                act("You renew your power, keeping $N fully-ensnared by your will!", ch, NULL, victim, TO_CHAR);
            
            kiss->duration = MaxKissDuration;
        }
    }

    return true;
}

bool spell_embraceisetaton( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (IS_AFFECTED(victim,AFF_SLOW))
    {
        send_to_char("They're already moving at a crawl.\n\r", ch);
        return FALSE;
    }

    if (victim == ch)
    {
        send_to_char("You cannot call the embrace of Isetaton upon yourself.\n\r", ch);
        return FALSE;
    }

    if (IS_NPC(victim))
    {
    act("{WYou cry out, {c'Isetaton Chosanth Khebt-Ut Ameras!'{x", ch,NULL,victim,TO_CHAR);
    act("{W$n cries out, {c'Isetaton Chosanth Khebt-Ut Ameras!'{x", ch,NULL,victim,TO_NOTVICT);
    act("{W$n cries out, {c'Isetaton Chosanth Khebt-Ut Ameras!'{x", ch,NULL,victim,TO_VICT);
    }
    else
    {
    act("{WYou cry out, {c'Isetaton Chosanth Khebt-Ut Ameras $N!'{x", ch,NULL,victim,TO_CHAR);
    act("{W$n cries out, {c'Isetaton Chosanth Khebt-Ut Ameras $N!'{x", ch,NULL,victim,TO_NOTVICT);
    act("{W$n cries out, {c'Isetaton Chosanth Khebt-Ut Ameras $N!'{x", ch,NULL,victim,TO_VICT);
    }

    act("Black tentacles appear from the fabric of space, clutching at $N!",ch, NULL, victim, TO_CHAR);
    act("Black tentacles appear from the fabric of space, clutching at $N!",ch, NULL, victim, TO_NOTVICT);
    act("Black tentacles appear near you, and slither outward to grab you!" ,ch, NULL, victim, TO_VICT);

    if ((victim->position > POS_SLEEPING)
     && saves_spell(level + 22 - get_curr_stat(victim, STAT_DEX) - 10*(IS_AFFECTED(victim,AFF_HASTE)), ch, victim, DAM_NEGATIVE))
    {
        act("$N nimbly dodges the tentacles, which clutch on empty air." ,ch, NULL, victim, TO_CHAR);
        act("$N nimbly dodges the tentacles, which clutch on empty air." ,ch, NULL, victim, TO_NOTVICT);
        act("Jumping side at the last moment, you dodge the tentacles!" ,ch, NULL, victim, TO_VICT);
        return TRUE;
    }
    
    af.where     = TO_AFFECTS;
    af.type      = gsn_embraceisetaton;
    af.level     = level;
    af.duration  = 8;
    af.modifier  = -3;
    af.location  = APPLY_DEX;
    af.bitvector = AFF_SLOW;
    affect_to_char(victim, &af);
    

    act("The tentacles grab $N, entangling $M in their dark flesh.",ch,NULL,victim,TO_CHAR);
    act("The tentacles grab $N, entangling $M in their dark flesh.",ch,NULL,victim,TO_NOTVICT);
    act("The tentacles latch onto you, entangling you in their dark flesh!",ch,NULL,victim,TO_VICT);
    return TRUE;

}


bool spell_enfeeblement( int sn, int level, CHAR_DATA *ch, void *vo, int target){
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

// brazen: ticket#747: enfeeblement should auto-fail, or check for resist.  
    if (is_affected(victim, sn))
    {
        send_to_char("They are already feeble.\n\r",ch);
	return FALSE;
    };
    
    act("You scream a word of power at $N!",ch, NULL, victim, TO_CHAR);
    act("$n screams a word of power at $N!",ch, NULL, victim, TO_NOTVICT);
    act("$n screams a word of power at you!",ch, NULL, victim, TO_VICT);

    if ( saves_spell( level, ch, victim,DAM_NEGATIVE ))
    {
        send_to_char("They resist your spell.\n\r",ch);
    }
    else
    {
        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = level;
        af.duration  = level / 4;

  	switch(number_range(1,4))
        {
            case 1:
                af.location = number_range(1, 5);
                af.modifier = -1 * (level/11);
                break;
            case 2:
                af.location = number_range(12,14);
                af.modifier = -25 * (level/10);
                break;
            case 3:
                af.location = number_range(18,20);
                af.modifier = -1 * (level/11);
                if (af.location == 20)
                    af.modifier = 1 * (level/11);
                break;
            case 4:
                af.location = APPLY_AC;
                af.modifier = 3 * level;
                break;
        }

        af.bitvector = 0;
        affect_to_char( victim, &af );
        send_to_char( "You feel the dark grip of the void take hold.\n\r", victim );
        act("$n looks more feeble.",victim,NULL,NULL,TO_ROOM);
    }

    return TRUE;
}


bool spell_findfamiliar( int sn, int level, CHAR_DATA *ch, void *vo, int target)

{

CHAR_DATA *familiar;
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

if (ch->familiar != NULL)
        {
        send_to_char("You sense that you already have a familiar in the land.\n\r", ch);
        return FALSE;
        }

if (is_affected(ch, gsn_findfamiliar))
        {
        send_to_char("Distraught over the loss of your familiar, you find yourself unable to call another.\n\r", ch);
        return FALSE;
        }

if (ch->familiar_type < 1)
        {
        send_to_char("You can't seem to find any familiar. You may need immortal intervention.\n\r", ch);
        return FALSE;
        }

familiar = create_mobile(get_mob_index(ch->familiar_type));

act("Answering the call of $n, $N arrives to bond with $m.", ch, NULL, familiar, TO_ROOM);
act("Answering your call, $N arrives to bond with you.", ch, NULL, familiar, TO_CHAR);
char_to_room(familiar, ch->in_room);
familiar->level = ch->level;
ch->familiar = familiar;
familiar->master = ch;
familiar->leader = ch;

        af.where        = TO_AFFECTS;
        af.type         = sn;
        af.level        = level;
        af.duration     = -1;
        af.location     = APPLY_HIT;
        af.modifier     = 4 * level;
        af.bitvector   =  0;
        affect_to_char(ch, &af);

        af.location     = APPLY_MANA;
        af.modifier     = 3 * level;
        affect_to_char(ch, &af);

    return TRUE;
}

bool spell_gate( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim;
    OBJ_DATA *heart;
    bool gate_pet;
    bool gate_familiar;

    if (((victim = get_char_world( ch, target_name)) != NULL)
    && (room_is_affected(victim->in_room, gsn_sanctify)))
    {
         send_to_char("A holy power protects your target.\n\r",ch); 
         return FALSE;
    }

    if (!IS_NPC(ch))
    {
	for (heart = ch->carrying; heart; heart = heart->next_content)
	    if (heart->pIndexData->vnum == OBJ_VNUM_TROPHY_HEART)
		break;

	if (!heart)
	{
	    send_to_char("You lack the neccessary component to gate.\n\r", ch);
	    return FALSE;
	}
	else
	{
	    act("$p is consumed in the casting.", ch, heart, NULL, TO_CHAR);
	    extract_obj(heart);
	    level = (level * 2 / 3) + (heart->level * 1 / 3);
	}
    }

    if ( ( victim = get_char_world( ch, target_name ) ) == NULL
    ||   victim == ch
    ||   ( ch->move == 0 )
    ||   victim->in_room == NULL
    ||   is_symbol_present(*victim->in_room, OBJ_VNUM_SYMBOL_PROTECT)
    ||   is_affected(ch, gsn_matrix)
    ||   !can_see_room(ch,victim->in_room)
    ||   IS_SET(victim->in_room->room_flags, ROOM_NOGATE)
    ||   (victim->in_room->area->area_flags & AREA_UNCOMPLETE )
    ||   IS_SET(victim->in_room->room_flags, ROOM_NOMAGIC)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SAFE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
    ||   IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
    ||   IS_SET(ch->in_room->room_flags, ROOM_NOGATE)
    ||   IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
    ||   (!IS_PK(ch, victim) && !IS_NPC(victim) && IS_SET(victim->act, PLR_NOSUMMON))
    ||   (!IS_NPC(victim) && victim->level > LEVEL_HERO)  /* NOT trust */
    ||   (IS_NPC(victim) && IS_SET(victim->imm_flags,IMM_SUMMON))
    ||   saves_spell( level, ch, victim,DAM_OTHER)
    ||   ((!IS_NPC(ch) || (IS_NPC(ch) && ((ch->pIndexData->vnum < MOB_VNUM_DEMON_FIRST)
                                       || (ch->pIndexData->vnum > MOB_VNUM_DEMON_LAST)))) 
      && (IS_SET(victim->in_room->room_flags, ROOM_SAFE) || 
         IS_SET(victim->in_room->room_flags, ROOM_NOMAGIC))))
        /*This next if added so greater demons can gate into any area, except
                a safe room -- ie, limbo, in the case -- or nomagic rooms,
                but allowing the demons access to nogate/norecall areas for
                added mortal pain and suffering *innocent* */
/*
      if (!IS_NPC(ch) || ch->pIndexData->vnum < 70 || ch->pIndexData->vnum > 75
|| IS_SET(victim->in_room->room_flags, ROOM_SAFE)
        || IS_SET(victim->in_room->room_flags, ROOM_NOMAGIC))  */
      {
        send_to_char( "You failed.\n\r", ch );
        return FALSE;
      }
    if (ch->pet != NULL && ch->in_room == ch->pet->in_room)
        gate_pet = TRUE;
    else
        gate_pet = FALSE;

    if (ch->familiar != NULL && ch->in_room == ch->familiar->in_room)
        gate_familiar = TRUE;
    else
        gate_familiar = FALSE;

    act("$n steps through a gate, vanishing in the void.",ch,NULL,NULL,TO_ROOM);    
    send_to_char("You step into the void, crossing space with your magics.\n\r",ch);
    act("The magical winds of the void tear at you!", ch, NULL, NULL, TO_CHAR);
    if (!IS_NPC(ch))
        damage_old( ch, ch, number_range(ch->level/2, (ch->level*3)/2), sn, DAM_NEGATIVE ,TRUE);
    char_from_room(ch);
    char_to_room(ch,victim->in_room);
    ch->move /= 2;

    act("A black line tears space, and turns sideways into a doorway, and $n steps out.",ch,NULL,NULL,TO_ROOM);
    do_look(ch,"auto");

    if (gate_familiar && ch->familiar)
    {
        act("$n steps through a gate, vanishing in the void.",ch->familiar,NULL,NULL,TO_ROOM);
        send_to_char("You step through a gate and vanish.\n\r",ch->familiar);
        char_from_room(ch->familiar);
        char_to_room(ch->familiar,victim->in_room);
        act("$n follows right behind, through the opaque portal.",ch->familiar,NULL,NULL,TO_ROOM);
        do_look(ch->familiar,"auto");
    }

    if (gate_pet)
    {
        act("$n steps through a gate, vanishing in the void.",ch->pet,NULL,NULL,TO_ROOM);
        send_to_char("You step through a gate and vanish.\n\r",ch->pet);
        char_from_room(ch->pet);
        char_to_room(ch->pet,victim->in_room);
        act("$n follows right behind, through the opaque portal.",ch->pet,NULL,NULL,TO_ROOM);
        do_look(ch->pet,"auto");
    }

    return TRUE;
}

bool spell_globedarkness( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
   
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

   
    if (!ch->in_room)
        return FALSE;

    if (is_affected(ch, gsn_globedarkness))
    {
        send_to_char("You aren't ready to call another globe of darkness yet.\n\r", ch);
        return FALSE;
    }

/*
    if (ch->in_room->room_flag == ROOM_UBERDARK)
    {
        send_to_char("There is already darkness here.\n\r", ch);
        return TRUE;
    }
*/
    af.where     = TO_AFFECTS;
    af.type      = gsn_globedarkness;
    af.level     = level;
    af.modifier  = 0;
    af.location  = 0;
    af.duration  = 10;
    af.bitvector = 0;
    affect_to_char(ch,&af);

    af.where        = TO_ROOM_AFF;
    af.type = gsn_globedarkness;
    af.level        = level;
    af.duration     = 6;
    af.location     = 0;
    af.modifier     = ch->id;
    af.bitvector   = 0;
    affect_to_room(ch->in_room, &af);
        

    send_to_char("You call a globe of darkness to extinguish light in the room.\n\r", ch);
    act("As $n's chant ends, the area is filled with darkness!\n\r", ch, NULL, NULL, TO_ROOM);
    return TRUE;
}

bool spell_harm( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    if (is_safe_spell(ch, victim, FALSE))
        return FALSE;

    act("You call upon the power of the void to inflict harm upon $N!", ch, NULL, victim, TO_CHAR);
    act("$n calls upon the power of the void to inflict harm upon $N!", ch, NULL, victim, TO_VICTROOM);
    act("$n calls upon the power of the void to inflict harm upon you!", ch, NULL, victim, TO_VICT);

    dam = (dice(level, 5) + 10);
    if (saves_spell(level, ch, victim, DAM_NEGATIVE))
        dam /= 2;
    damage( ch, victim, dam, sn, DAM_NEGATIVE,TRUE);
    return TRUE;
}

bool spell_hungerofgrmlarloth(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *vObj = (OBJ_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    OBJ_DATA *symbol;
    CHAR_DATA *demon;

    if (IS_NPC(ch))
    {
	send_to_char("You lack the power to bind the hunger of Grmlarloth.\n\r", ch);
	return FALSE;
    }

    if (!CAN_WEAR(vObj, ITEM_WEAR_HANDS))
    {
	send_to_char("Only glove weapons may be bound with the hunger of Grmlarloth.\n\r", ch);
	return FALSE;
    }

    if (IS_OBJ_STAT(vObj, ITEM_ANTI_EVIL) || IS_OBJ_STAT(vObj, ITEM_BLESS) || CAN_WEAR(vObj, ITEM_NO_SAC))
    {
	act("$p cannot be bound with the hunger of Grmlarloth.", ch, vObj, NULL, TO_CHAR);
	return FALSE;
    }

    if (obj_is_affected(vObj, gsn_demon_bind))
    {
	act("$p has already been bound with a demonic spirit.", ch, vObj, NULL, TO_CHAR);
	return FALSE;
    }

    if ((silver_state != SILVER_FINAL)
     && (time_info.hour >= season_table[time_info.season].sun_up)
     && (time_info.hour < season_table[time_info.season].sun_down))
    {
	send_to_char("This ritual may not be completed during the day.\n\r",ch);
	return FALSE;
    }

    if (ch->in_room)
    {
	for (symbol = ch->in_room->contents; symbol; symbol = symbol->next_content)
	{
	    if (symbol->pIndexData->vnum == OBJ_VNUM_SYMBOL_BIND)
	    {
		
		if (symbol->value[0] == 0 || symbol->value[1] != INSMAT_SALT)
		{
		    demon = create_mobile(get_mob_index(MOB_VNUM_DEMON_GRMLARLOTH));

		    char_to_room(demon, ch->in_room);

		    send_to_char("As you complete the casting, a powerful whirlwind of dark smoke begins to swirl above the trigon of binding.\n\r", ch);
		    send_to_char("The chatter of teeth sounds from within the smoke, and a mass of flesh and jaws emerges.\n\r", ch);
		    if (number_bits(1) == 0)
		    {
			do_yell(demon, "At last I am free!");
		    	act("$n looks down at the flawed symbol and sneers arrogantly.", demon, NULL, NULL, TO_ROOM);
		    	do_say(demon, "Thank you, mortal.");
		    	do_say(demon, "Now you must die.");
		    	act("$n laughs darkly.", demon, NULL, NULL, TO_ROOM);
		    	multi_hit(demon, ch, TYPE_UNDEFINED);
		    	demon->demontrack = ch;
		    	return TRUE;
		    }
		    else
		    {
			do_say(demon, "Today, it is I who will be sated.");
    			af.where	 = TO_AFFECTS;
		 	af.type	 	 = gsn_wrathofthevoid;
	    		af.level	 = level;
		        af.location	 = APPLY_NONE;
			af.modifier  	 = 2;
			af.duration  	 = 24;
			af.bitvector     = 0;
			affect_to_char(ch, &af);
			act("$n approaches you, turning into a foul, viscous liquid as it forces its way down your throat.", demon, NULL, ch, TO_VICT);
			act("$n approaches $N, turning into a foul, viscous liquid as it forces its way down $S throat.",demon, NULL, ch, TO_NOTVICT);
			
			extract_char(demon,TRUE);
			return TRUE;
		    }
		}
		else
		    break;
	    }
	}
    }
    else
	return FALSE;

    if (!symbol)
    {
	send_to_char("There must be a trigon of binding present to cast this.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_OBJECT;
    af.type	 = sn;
    af.level	 = level;
    af.location	 = 0;
    af.modifier  = 0;
    af.duration  = -1;
    af.bitvector = 0;
    affect_to_obj(vObj, &af);

    af.type	 = gsn_demon_bind;
    af.location  = 0;
    af.modifier  = ch->id;
    affect_to_obj(vObj, &af);

    act("An unwholesome clattering fills the area, as of a thousand jaws opening and closing with an urgent hunger.  The sound quickly fades as you bind the presence into $p.", ch, vObj, NULL, TO_CHAR);
    act("An unwholesome clattering fills the area, as of a thousand jaws opening and closing with an urgent hunger.  The sound quickly fades as $n binds the presence into $p.", ch, vObj, NULL, TO_ROOM);

    return TRUE;
}

bool check_idcizon_binding(CHAR_DATA * ch, bool & proceed)
{
    // Sanity-check
    proceed = false;
    if (ch->in_room == NULL)
        return false;
	
    // Check for a trigon of binding
    for (OBJ_DATA * symbol(ch->in_room->contents); symbol != NULL; symbol = symbol->next_content)
	{
        // Ignore non-trigons
	    if (symbol->pIndexData->vnum != OBJ_VNUM_SYMBOL_BIND)
            continue;

        // Check for flawed symbols/wrong reagent
		if (symbol->value[0] != 1 || symbol->value[1] != INSMAT_CHARCOAL)
		{
		    CHAR_DATA * demon(create_mobile(get_mob_index(MOB_VNUM_DEMON_IDCIZON)));
		    char_to_room(demon, ch->in_room);
		    send_to_char("As you complete the casting, a powerful whirlwind of dark smoke begins to swirl above the trigon of binding.\n\r", ch);
		    send_to_char("From within the smoke, a hulking demonic form emerges.\n\r", ch);
		    if (number_bits(1) == 0)
		    {
		    	do_yell(demon, "At last I am free!");
		    	act("$n looks down at the flawed symbol and sneers arrogantly.", demon, NULL, ch, TO_ROOM);
		    	do_say(demon, "Thank you, mortal.");
		    	do_say(demon, "Now you must die.");
		    	act("$n laughs darkly.", demon, NULL, NULL, TO_ROOM);
		    	demon->demontrack = ch;
		    	multi_hit(demon, ch, TYPE_UNDEFINED);
		    	return true;
		    }
			
            do_say(demon,"You will regret your ignorance, mortal.");
			act("$n spits a gob of green ichor, and it lands on your face.",demon,NULL,ch,TO_VICT);
			act("$n spits a gob of green ichor, and it lands on $N's face.",demon,NULL,ch,TO_NOTVICT);
			act("The ichor drips over your mouth, sealing it shut!",demon,NULL,ch,TO_VICT);
			act("The ichor drips over $N's mouth, sealing it shut!",demon,NULL,ch,TO_NOTVICT); 
            AFFECT_DATA af = {0};
    		af.where	 = TO_AFFECTS;
		 	af.type	 	 = gsn_wrathofthevoid;
	    	af.level	 = demon->level;
			af.modifier  = 1;
			af.duration  = 24;
			affect_to_char(ch, &af);
			act("$n leaps into the smoke, and disappears with it.",demon,NULL,ch,TO_ROOM);
			extract_char(demon, true);
			return true;
		}

        // Valid trigon
        proceed = true;
        return true;
	}
    
    // No trigon at all
    return false;
}

bool spell_jawsofidcizon(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *vObj = (OBJ_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (IS_NPC(ch))
    {
	send_to_char("You lack the power to bind the jaws of Idcizon.\n\r", ch);
	return FALSE;
    }

    if (!CAN_WEAR(vObj, ITEM_WEAR_SHIELD))
    {
	send_to_char("Only shields may be bound with the jaws of Idcizon.\n\r", ch);
	return FALSE;
    }

    if (IS_OBJ_STAT(vObj, ITEM_ANTI_EVIL) || IS_OBJ_STAT(vObj, ITEM_BLESS) || CAN_WEAR(vObj, ITEM_NO_SAC))
    {
	act("$p cannot be bound with the jaws of Idcizon.", ch, vObj, NULL, TO_CHAR);
	return FALSE;
    }

    if (obj_is_affected(vObj, gsn_demon_bind))
    {
	act("$p has already been bound with a demonic spirit.", ch, vObj, NULL, TO_CHAR);
	return FALSE;
    }

    if ((silver_state != SILVER_FINAL)
     && (time_info.hour >= season_table[time_info.season].sun_up)
     && (time_info.hour < season_table[time_info.season].sun_down))
    {
	send_to_char("This ritual may not be completed during the day.\n\r",ch);
	return FALSE;
    }

    bool proceed;
    bool result(check_idcizon_binding(ch, proceed));
    if (!proceed)
    {
        if (!result)
            send_to_char("You cannot attempt to summon this demon with no Trigon of Binding present.\n", ch);

        return result;
    }
 
    af.where	 = TO_OBJECT;
    af.type	 = sn;
    af.level	 = level;
    af.location	 = APPLY_HITROLL;
    af.modifier  = level / 12;
    af.duration  = -1;
    af.bitvector = 0;
    affect_to_obj(vObj, &af);

    af.location  = APPLY_DAMROLL;
    affect_to_obj(vObj, &af);

    af.type	 = gsn_demon_bind;
    af.location  = 0;
    af.modifier  = ch->id;
    affect_to_obj(vObj, &af);

    act("As you complete the casting, pained images briefly twist and contort across the face of $p, and a wail of anguish emits from within.", ch, vObj, NULL, TO_CHAR);
    act("As $n completes the casting, pained images briefly twist and contort across the face of $p, and a wail of anguish emits from within.", ch, vObj, NULL, TO_ROOM);

    return TRUE;
}


bool spell_leechrune( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *weapon;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    weapon = (OBJ_DATA *) vo;

    if (weapon == NULL)
    {
        send_to_char("You can't find any such weapon.\n\r", ch);
        return FALSE;
    }

    if (obj_is_affected(weapon, sn))
    {
        send_to_char("That weapon is already inscribed with a leech rune.\n\r", ch);
        return FALSE;
    }

	if (IS_OBJ_STAT(weapon, ITEM_NOREMOVE))
	{
		send_to_char("That weapon already bears the properties of such a rune.\n\r", ch);
		return FALSE;
	}

    if (weapon->item_type != ITEM_WEAPON)
    {
        send_to_char("You can only apply a leech rune to a weapon.\n\r", ch);
        return FALSE;
    }

    af.where     = TO_OBJECT;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/6 * (obj_is_affected(weapon,gsn_demon_bind) ? 2 : 1);
    af.location  = APPLY_HIT;
    af.modifier  = 0 - level;
    af.bitvector = ITEM_NOREMOVE + ITEM_NOUNCURSE;
    affect_to_obj(weapon, &af);

    act("$n chants in a gutteral tongue, tracing an angry rune along $p.", ch, weapon, NULL, TO_ROOM);
    act("You chant an ancient shuddeni invocation, tracing an angry rune upon $p.", ch, weapon, NULL, TO_CHAR);

    return TRUE;
}

bool spell_lesserdemonsummon( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *symbol;
    CHAR_DATA *demon;
    bool aggro = FALSE;
    int sNum, dNum = 0, effect;
//    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (!ch->in_room)
	return FALSE;

    for (symbol = ch->in_room->contents; symbol; symbol = symbol->next_content)
	if ((symbol->pIndexData->vnum >= OBJ_VNUM_SYMBOL_FIRST)
	 && (symbol->pIndexData->vnum <= OBJ_VNUM_SYMBOL_LAST))
	    break;

    if (symbol)
    {
	if (is_affected(ch, gsn_demonsummon))
	{
	    send_to_char("You recite the spell of summoning, but no demonic force heeds your call.\n\r", ch);
	    return FALSE;
	}

	if (symbol->objfocus[0] != ch)
	{
	    send_to_char("You hold no atunement to the symbol here.\n\r", ch);
	    return FALSE;
	}

	if (symbol->value[0] == 0)
	{
	    if (number_range(1, 10) <= 7)
	    {
		send_to_char("You recite the spell of summoning, but no demonic force heeds your call.\n\r", ch);
		return TRUE;
	    }
	    else
		aggro = TRUE;
	}

	sNum = (symbol->pIndexData->vnum - OBJ_VNUM_SYMBOL_FIRST);

	if ((sNum == 3)  /* Maze of Isetaton */
	 && (symbol->value[1] == INSMAT_BLOOD_ALATHARYA))
	    dNum = MOB_VNUM_LESSER_DEVOURING;
	else if ((sNum == 5)  /* Spiral of Bahhaoth */
	 && (symbol->value[1] == INSMAT_MUD))
	    dNum = MOB_VNUM_LESSER_KURLAAC;
	else if ((sNum == 6)  /* Logorin Star */
	 && (symbol->value[1] == INSMAT_BONEDUST))
	    dNum = MOB_VNUM_LESSER_QWABLITH;
	else if ((sNum == 12)  /* The Vakalic Sign */
	 && (symbol->value[1] == INSMAT_CHARCOAL))
	    dNum = MOB_VNUM_VAKALIC_IMP;
	else if ((sNum == 12)  /* The Vakalic Sign */
	 && (symbol->value[1] == INSMAT_SILVER))
	    dNum = MOB_VNUM_FILCHLING;
	else if ((sNum == 13)  /* Lost Cipher of Pnakur */
	 && (symbol->value[1] == INSMAT_HERB1))
	    dNum = MOB_VNUM_LESSER_ETHEREAL;
	else if ((sNum == 17)  /* Tear of Pricina */
	 && (symbol->value[1] == INSMAT_BLOOD_CASTER))
	    dNum = MOB_VNUM_LESSER_WORM;
	else if ((sNum == 18)  /* The Mad Etchings of Kyalee */
	 && (symbol->value[1] == INSMAT_MUD))
	    dNum = MOB_VNUM_POISONOUS_NJITH;
	else if ((sNum == 18)  /* The Mad Etchings of Kyalee */
	 && (symbol->value[1] == INSMAT_SILVER))
	    dNum = MOB_VNUM_LESSER_DJAZITH;
	else if ((sNum == 21)  /* The Aklaju Heiroglyph */
	 && (symbol->value[1] == INSMAT_CHARCOAL))
	    dNum = MOB_VNUM_LESSER_IMPSAGE;
	else if ((sNum == 21)  /* The Aklaju Heiroglyph */
	 && (symbol->value[1] == INSMAT_BONEDUST))
	    dNum = MOB_VNUM_LESSER_ORZUB;
	else if ((sNum == 21)  /* The Aklaju Heiroglyph */
	 && (symbol->value[1] == INSMAT_BLOOD_ANIMAL))
	    dNum = MOB_VNUM_LESSER_GHARKU;

	extract_obj(symbol);

	if (dNum != 0)
	{
	    if ((demon = create_mobile(get_mob_index(dNum))) == NULL)
	    {
		bug("Error loading demon vnum %d.", dNum);
		send_to_char("You feel as though something is wrong...\n\r", ch);
		return FALSE;
	    }

	    char_to_room(demon, ch->in_room);
	    demon->level = ch->level;
	    demon->hitroll = ch->level;
	    demon->memfocus[0] = ch;
	    demon->mobvalue[9] = aggro ? 1 : 0;
	    mprog_load_trigger(demon);
	}
	else
	{
	    effect = number_percent();

	    send_to_char("You feel as though something is terribly wrong...\n\r", ch);

	    if (effect <= 35)
		send_to_char("You recite the spell of summoning, but no demonic force heeds your call.\n\r", ch);
	    else if (effect <= 65)
	    {
		dNum = number_range(MOB_VNUM_LESSER_BOUND_FIRST, MOB_VNUM_LESSER_BOUND_LAST);
		if ((demon = create_mobile(get_mob_index(dNum))) == NULL)
		    return FALSE;

	    	demon->level = ch->level;
	    	demon->hitroll = ch->level;

		char_to_room(demon, ch->in_room);
		do_emote(demon, "appears within the symbol, and attacks!");
		set_fighting(demon, ch);
		multi_hit(demon, ch, TYPE_UNDEFINED);
	    }
	    else if (effect <= 75)
	    {
		dNum = number_range(MOB_VNUM_NORMAL_BOUND_FIRST, MOB_VNUM_NORMAL_BOUND_LAST);
		if ((demon = create_mobile(get_mob_index(dNum))) == NULL)
		    return FALSE;

	    	demon->level = ch->level;
	    	demon->hitroll = ch->level;

		char_to_room(demon, ch->in_room);
		do_emote(demon, "appears within the symbol, and attacks!");
		set_fighting(demon, ch);
		multi_hit(demon, ch, TYPE_UNDEFINED);
	    }
	    else if (effect <= 80)
	    {
	        dNum = number_range(MOB_VNUM_GREATER_BOUND_FIRST, MOB_VNUM_GREATER_BOUND_LAST);
		if ((demon = create_mobile(get_mob_index(dNum))) == NULL)
		    return FALSE;

	    	demon->level = ch->level;
	    	demon->hitroll = ch->level;

		char_to_room(demon, ch->in_room);
		do_emote(demon, "appears within the symbol, and attacks!");
		set_fighting(demon, ch);
		multi_hit(demon, ch, TYPE_UNDEFINED);
	    }
	    else if (effect <= 85)
	    {
		af.where        = TO_ROOM_AFF;
		af.type		= gsn_globedarkness;
	    	af.level        = level;
    		af.duration     = 6;
    		af.location     = 0;
    		af.modifier     = 0;
    		af.bitvector    = 0;
    		affect_to_room(ch->in_room, &af);

		act("The area is suddenly plunged into an inky darkness.", ch, NULL, NULL, TO_CHAR);
		act("The area is suddenly plunged into an inky darkness.", ch, NULL, NULL, TO_ROOM);
	    }
	    else if (effect <= 90)
	    {
		af.where     = TO_AFFECTS;
		af.type      = gsn_pox;
		af.level     = level * 3/4;
		af.duration  = level/2;
		af.location  = APPLY_STR;
		af.modifier  = (-1) * level/10;
		af.bitvector = AFF_PLAGUE;
		affect_to_char(ch,&af);
		send_to_char("You grimace as illness strikes you.\n\r", ch);
		act("$n grimaces and looks ill.", ch, NULL, NULL, TO_ROOM);
	    }
	    else if (effect <= 95)
	    {
		af.where     = TO_AFFECTS;
		af.type      = gsn_curse;
		af.level     = level;
		af.duration  = level/2 + 10;
		af.location  = APPLY_HITROLL;
		af.modifier  = -1 * (level / 8);
		af.bitvector = AFF_CURSE;
		affect_to_char( ch, &af );

		af.location  = APPLY_SAVING_SPELL;
		af.modifier  = level / 8;
		affect_to_char( ch, &af );

		send_to_char( "You feel unclean.\n\r", ch);
		act("$n looks very uncomfortable.", ch, NULL, NULL, TO_ROOM);
	    }
	    else if (effect <= 98)
	    {
		af.where     = TO_AFFECTS;
		af.type      = gsn_sleep;
		af.level     = level;
		af.location  = APPLY_NONE;
		af.modifier  = 0;
		af.duration  = 4;
		af.bitvector = AFF_SLEEP;
		affect_to_char(ch, &af);
		switch_position(ch, POS_SLEEPING);
		send_to_char("A pain explodes inside of your head, and you fall to the ground.\n\r", ch);
		act("$n falls to the ground.", ch, NULL, NULL, TO_ROOM);
	    }
	    else
	    {
		send_to_char("You are blasted by the power of the void!\n\r", ch);
		ch->hit /= 2;
		ch->mana /= 2;
	    }
	}
    }
    else
    {
	send_to_char("There is no inscribed symbol here.\n\r", ch);
	return FALSE;
    }

    return TRUE;
}



bool spell_normaldemonsummon( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *symbol;
    CHAR_DATA *demon;
    bool aggro = FALSE;
    int sNum, dNum = 0, effect;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (!ch->in_room)
	return FALSE;

    for (symbol = ch->in_room->contents; symbol; symbol = symbol->next_content)
	if ((symbol->pIndexData->vnum >= OBJ_VNUM_SYMBOL_FIRST)
	 && (symbol->pIndexData->vnum <= OBJ_VNUM_SYMBOL_LAST))
	    break;

    if (symbol)
    {
	if (is_affected(ch, sn))
	{
	    send_to_char("You recite the spell of summoning, but no demonic force heeds your call.\n\r", ch);
	    return FALSE;
	}

	if (symbol->objfocus[0] != ch)
	{
	    send_to_char("You hold no atunement to the symbol here.\n\r", ch);
	    return FALSE;
	}

	if (symbol->value[0] == 0)
	{
	    if (number_range(1, 10) <= 6)
	    {
		send_to_char("You recite the spell of summoning, but no demonic force heeds your call.\n\r", ch);
		return TRUE;
	    }
	    else
		aggro = TRUE;
	}

	sNum = (symbol->pIndexData->vnum - OBJ_VNUM_SYMBOL_FIRST);

	if ((sNum == 3)  /* Maze of Isetaton */
	 && (symbol->value[1] == INSMAT_SALT))
	    dNum = MOB_VNUM_DEMON_SERVITOR;
	else if ((sNum == 5)  /* Lost Cipher of Pnakur */
	 && (symbol->value[1] == INSMAT_BLOOD_CASTER))
	    dNum = MOB_VNUM_DEMON_DOPPELGANGER;
	else if ((sNum == 6)  /* Logorin Star */
	 && (symbol->value[1] == INSMAT_HERB2))
	    dNum = MOB_VNUM_DEMON_SPAWN;
	else if ((sNum == 7)  /* Angles of Selb-Kar */
	 && (symbol->value[1] == INSMAT_BONEDUST))
	    dNum = MOB_VNUM_DEMON_WVERSHAK;
	else if ((sNum == 8)  /* Seal of the Dragon */
	 && (symbol->value[1] == INSMAT_CHARCOAL))
	    dNum = MOB_VNUM_DEMON_GUARDIAN;
	else if ((sNum == 14)  /* Vkoren Configuration */
	 && (symbol->value[1] == INSMAT_BLOOD_CASTER))
	    dNum = MOB_VNUM_DEMON_ACOLYTE;
	else if ((sNum == 15)  /* Sigil of Logor */
	 && (symbol->value[1] == INSMAT_SILVER))
	    dNum = MOB_VNUM_DEMON_NVAEGRA;
	else if ((sNum == 15)  /* Sigil of Logor */
	 && (symbol->value[1] == INSMAT_BONES_UNDEAD))
	    dNum = MOB_VNUM_DEMON_HORROR;
	else if ((sNum == 16)  /* Scar of Gagaroth */
	 && (symbol->value[1] == INSMAT_SILVER))
	    dNum = MOB_VNUM_DEMON_HOUND;
	else if ((sNum == 17)  /* Tear of Pricina */
	 && (symbol->value[1] == INSMAT_BLOOD_CELESTIAL))
	    dNum = MOB_VNUM_DEMON_SUCCUBUS;
	else if ((sNum == 19)  /* Cracks of Xixzyr */
	 && (symbol->value[1] == INSMAT_BLOOD_ANIMAL))
	    dNum = MOB_VNUM_DEMON_FLIES;
	else if ((sNum == 19)  /* Cracks of Xixzyr */
	 && (symbol->value[1] == INSMAT_CHARCOAL))
	    dNum = MOB_VNUM_DEMON_KHILMESTAHN;

	extract_obj(symbol);

	if (dNum != 0)
	{
	    if ((demon = create_mobile(get_mob_index(dNum))) == NULL)
	    {
		bug("Error loading demon vnum %d.", dNum);
		send_to_char("You feel as though something is wrong...\n\r", ch);
		return FALSE;
	    }

	    char_to_room(demon, ch->in_room);
	    demon->level = ch->level;
	    demon->hitroll = ch->level;
	    demon->memfocus[0] = ch;
	    demon->mobvalue[9] = aggro ? 1 : 0;
	    mprog_load_trigger(demon);
	}
	else
	{
	    effect = number_percent();

	    send_to_char("You feel as though something is terribly wrong...\n\r", ch);

	    if (effect <= 20)
		send_to_char("You recite the spell of summoning, but no demonic force heeds your call.\n\r", ch);
	    else if (effect <= 25)
	    {
		dNum = number_range(MOB_VNUM_LESSER_BOUND_FIRST, MOB_VNUM_LESSER_BOUND_LAST);
		if ((demon = create_mobile(get_mob_index(dNum))) == NULL)
		    return TRUE;

	    	demon->level = ch->level;
	    	demon->hitroll = ch->level;

		char_to_room(demon, ch->in_room);
		do_emote(demon, "appears within the symbol, and attacks!");
		set_fighting(demon, ch);
		multi_hit(demon, ch, TYPE_UNDEFINED);
	    }
	    else if (effect <= 55)
	    {
		dNum = number_range(MOB_VNUM_NORMAL_BOUND_FIRST, MOB_VNUM_NORMAL_BOUND_LAST);
		if ((demon = create_mobile(get_mob_index(dNum))) == NULL)
		    return FALSE;

	    	demon->level = ch->level;
	    	demon->hitroll = ch->level;

		char_to_room(demon, ch->in_room);
		do_emote(demon, "appears within the symbol, and attacks!");
		set_fighting(demon, ch);
		multi_hit(demon, ch, TYPE_UNDEFINED);
	    }
	    else if (effect <= 65)
	    {
	        dNum = number_range(MOB_VNUM_GREATER_BOUND_FIRST, MOB_VNUM_GREATER_BOUND_LAST);
		if ((demon = create_mobile(get_mob_index(dNum))) == NULL)
		    return FALSE;

	    	demon->level = ch->level;
	    	demon->hitroll = ch->level;

		char_to_room(demon, ch->in_room);
		do_emote(demon, "appears within the symbol, and attacks!");
		set_fighting(demon, ch);
		multi_hit(demon, ch, TYPE_UNDEFINED);
	    }
	    else if (effect <= 70)
	    {
		af.where        = TO_ROOM_AFF;
		af.type		= gsn_globedarkness;
	    	af.level        = level;
    		af.duration     = 6;
    		af.location     = 0;
    		af.modifier     = 0;
    		af.bitvector    = 0;
    		affect_to_room(ch->in_room, &af);

		act("The area is suddenly plunged into an inky darkness.", ch, NULL, NULL, TO_CHAR);
		act("The area is suddenly plunged into an inky darkness.", ch, NULL, NULL, TO_ROOM);
	    }
	    else if (effect <= 80)
	    {
		af.where     = TO_AFFECTS;
		af.type      = gsn_pox;
		af.level     = level * 3/4;
		af.duration  = level/2;
		af.location  = APPLY_STR;
		af.modifier  = (-1) * level/10;
		af.bitvector = AFF_PLAGUE;
		affect_to_char(ch,&af);
		send_to_char("You grimace as illness strikes you.\n\r", ch);
		act("$n grimaces and looks ill.", ch, NULL, NULL, TO_ROOM);
	    }
	    else if (effect <= 90)
	    {
		af.where     = TO_AFFECTS;
		af.type      = gsn_curse;
		af.level     = level;
		af.duration  = level/2 + 10;
		af.location  = APPLY_HITROLL;
		af.modifier  = -1 * (level / 8);
		af.bitvector = AFF_CURSE;
		affect_to_char( ch, &af );

		af.location  = APPLY_SAVING_SPELL;
		af.modifier  = level / 8;
		affect_to_char( ch, &af );

		send_to_char( "You feel unclean.\n\r", ch);
		act("$n looks very uncomfortable.", ch, NULL, NULL, TO_ROOM);
	    }
	    else if (effect <= 95)
	    {
		af.where     = TO_AFFECTS;
		af.type      = gsn_sleep;
		af.level     = level;
		af.location  = APPLY_NONE;
		af.modifier  = 0;
		af.duration  = 4;
		af.bitvector = AFF_SLEEP;
		affect_to_char(ch, &af);
		switch_position(ch, POS_SLEEPING);
		send_to_char("A pain explodes inside of your head, and you fall to the ground.\n\r", ch);
		act("$n falls to the ground.", ch, NULL, NULL, TO_ROOM);
	    }
	    else
	    {
		send_to_char("You are blasted by the power of the void!\n\r", ch);
		ch->hit /= 2;
		ch->mana /= 2;
	    }
	}

    }
    else
    {
	send_to_char("There is no inscribed symbol here.\n\r", ch);
	return FALSE;
    }

    return TRUE;
}

bool spell_greaterdemonsummon( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *symbol;
    CHAR_DATA *demon, *vch;
    bool aggro = FALSE, unique = FALSE;
    int sNum, dNum = 0, effect;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (!ch->in_room)
	return FALSE;

    for (symbol = ch->in_room->contents; symbol; symbol = symbol->next_content)
	if ((symbol->pIndexData->vnum >= OBJ_VNUM_SYMBOL_FIRST)
	 && (symbol->pIndexData->vnum <= OBJ_VNUM_SYMBOL_LAST))
	    break;

    if (symbol)
    {

	if (is_affected(ch, gsn_demonsummon))
	{
	    send_to_char("You recite the spell of summoning, but no demonic force heeds your call.\n\r", ch);
	    return FALSE;
	}

	if (symbol->objfocus[0] != ch)
	{
	    send_to_char("You hold no atunement to the symbol here.\n\r", ch);
	    return FALSE;
	}

	if (symbol->value[0] == 0)
	{
	    if (number_range(1, 10) <= 5)
	    {
		send_to_char("You recite the spell of summoning, but no demonic force heeds your call.\n\r", ch);
		return TRUE;
	    }
	    else
		aggro = TRUE;
	}

	sNum = (symbol->pIndexData->vnum - OBJ_VNUM_SYMBOL_FIRST);

	if ((sNum == 3)  /* Maze of Isetaton */
	 && (symbol->value[1] == INSMAT_HERB3))
	    dNum = MOB_VNUM_GREATER_AVATAR;
	else if ((sNum == 4)  /* Blasphemous Symbol of Nygothua */
	 && (symbol->value[1] == INSMAT_BLOOD_CHTAREN))
	    dNum = MOB_VNUM_GREATER_UNCLEAN;
	else if ((sNum == 7)  /* Angles of Selb-Kar */
	 && (symbol->value[1] == INSMAT_SALT))
	    dNum = MOB_VNUM_GREATER_ALIEN;
	else if ((sNum == 9)  /* Eye of Xthjich */
	 && (symbol->value[1] == INSMAT_BLOOD_CELESTIAL))
	{
	    unique = TRUE;
	    dNum = MOB_VNUM_GREATER_AGDUK;
	}
	else if ((sNum == 11)  /* Tetragon of the Dead */
	 && (symbol->value[1] == INSMAT_BONEDUST))
	    dNum = MOB_VNUM_GREATER_HANDMAIDEN;
	else if ((sNum == 11)  /* Tetragon of the Dead */
	 && (symbol->value[1] == INSMAT_SALT))
	    dNum = MOB_VNUM_GREATER_SAPLING;
	else if ((sNum == 14)  /* Vkoren Configuration */
	 && (symbol->value[1] == INSMAT_BLOOD_ALATHARYA))
	{
	    unique = TRUE;
	    dNum = MOB_VNUM_GREATER_BARKJA;
	}
	else if ((sNum == 15)  /* Sigil of Logor */
	 && (symbol->value[1] == INSMAT_BLOOD_CELESTIAL))
	    dNum = MOB_VNUM_GREATER_TASKMASTER;
	else if ((sNum == 15)  /* Sigil of Logor */
	 && (symbol->value[1] == INSMAT_HERB3))
	{
	    unique = TRUE;
	    dNum = MOB_VNUM_GREATER_VAESHIR;
	}
	else if ((sNum == 17)  /* Tear of Pricina */
	 && (symbol->value[1] == INSMAT_HERB1))
	{
	    unique = TRUE;
	    dNum = MOB_VNUM_GREATER_PRICINA;
	}
	else if ((sNum == 18)  /* Mad Etchings of Kyalee */
	 && (symbol->value[1] == INSMAT_BONES_UNDEAD))
	    dNum = MOB_VNUM_GREATER_ARCHON;
	else if ((sNum == 19)  /* Cracks of Xixzyr */
	 && (symbol->value[1] == INSMAT_HERB2))
	    dNum = MOB_VNUM_GREATER_WORM;
	else if ((sNum == 20)  /* Crest of Chaigidon */
	 && (symbol->value[1] == INSMAT_BLOOD_ALATHARYA))
	    dNum = MOB_VNUM_GREATER_WARRIOR;
	else if ((sNum == 22)  /* Crest of Khamurn */
	 && (symbol->value[1] == INSMAT_SILVER))
	    dNum = MOB_VNUM_GREATER_SYNDIC;

	extract_obj(symbol);

	if (unique)
	    for (vch = char_list; vch; vch = vch->next)
		if (IS_NPC(vch) && (vch->pIndexData->vnum == dNum))
		{
		    send_to_char("Your summoning fails to produce any results.\n\r", ch);
		    return TRUE;
		}

	if (dNum != 0)
	{
	    if ((demon = create_mobile(get_mob_index(dNum))) == NULL)
	    {
		bug("Error loading demon vnum %d.", dNum);
		send_to_char("You feel as though something is wrong...\n\r", ch);
		return FALSE;
	    }

	    char_to_room(demon, ch->in_room);
	    demon->level = ch->level;
	    demon->hitroll = ch->level * 3 / 2;
	    demon->memfocus[0] = ch;
	    demon->mobvalue[9] = aggro ? 1 : 0;
	    mprog_load_trigger(demon);
	}
	else
	{
	    effect = number_percent();

	    send_to_char("You feel as though something is terribly wrong...\n\r", ch);

	    if (effect <= 5)
	    {
		send_to_char("You recite the spell of summoning, but no demonic force heeds your call.\n\r", ch);
		return TRUE;
	    }
	    else if (effect <= 10)
	    {
		dNum = number_range(MOB_VNUM_LESSER_BOUND_FIRST, MOB_VNUM_LESSER_BOUND_LAST);
		if ((demon = create_mobile(get_mob_index(dNum))) == NULL)
		    return FALSE;

	    	demon->level = ch->level;
	    	demon->hitroll = ch->level * 3 / 2;

		char_to_room(demon, ch->in_room);
		do_emote(demon, "appears within the symbol, and attacks!");
		set_fighting(demon, ch);
		multi_hit(demon, ch, TYPE_UNDEFINED);
	    }
	    else if (effect <= 15)
	    {
		dNum = number_range(MOB_VNUM_NORMAL_BOUND_FIRST, MOB_VNUM_NORMAL_BOUND_LAST);
		if ((demon = create_mobile(get_mob_index(dNum))) == NULL)
		    return FALSE;

	    	demon->level = ch->level;
	    	demon->hitroll = ch->level * 3 / 2;

		char_to_room(demon, ch->in_room);
		do_emote(demon, "appears within the symbol, and attacks!");
		set_fighting(demon, ch);
		multi_hit(demon, ch, TYPE_UNDEFINED);
	    }
	    else if (effect <= 45)
	    {
	        dNum = number_range(MOB_VNUM_GREATER_BOUND_FIRST, MOB_VNUM_GREATER_BOUND_LAST);
		if ((demon = create_mobile(get_mob_index(dNum))) == NULL)
		    return FALSE;

	    	demon->level = ch->level;
	    	demon->hitroll = ch->level * 3 / 2;

		char_to_room(demon, ch->in_room);
		do_emote(demon, "appears within the symbol, and attacks!");
		set_fighting(demon, ch);
		multi_hit(demon, ch, TYPE_UNDEFINED);
	    }
	    else if (effect <= 50)
	    {
		af.where        = TO_ROOM_AFF;
		af.type		= gsn_globedarkness;
	    	af.level        = level;
    		af.duration     = 6;
    		af.location     = 0;
    		af.modifier     = 0;
    		af.bitvector    = 0;
    		affect_to_room(ch->in_room, &af);

		act("The area is suddenly plunged into an inky darkness.", ch, NULL, NULL, TO_CHAR);
		act("The area is suddenly plunged into an inky darkness.", ch, NULL, NULL, TO_ROOM);
	    }
	    else if (effect <= 60)
	    {
		af.where     = TO_AFFECTS;
		af.type      = gsn_agony;
		af.level     = level;
		af.duration  = level / 11;
		af.location  = APPLY_STR;
		af.modifier  = -1 * (level/12);
		af.bitvector = 0;
		affect_to_char( ch, &af );
		send_to_char( "You scream in agony as pain wracks your body!\n\r", ch );
		act("$n screams in agony as pain wracks $s body!",ch,NULL,NULL,TO_ROOM);

		af.type      = skill_lookup("weaken");
		af.duration  = level / 2;
		af.modifier  = -1 * (level / 7);
		af.bitvector = AFF_WEAKEN;
		affect_to_char( ch, &af );
		send_to_char( "You feel your strength slip away.\n\r", ch );
		act("$n looks tired and weak.",ch,NULL,NULL,TO_ROOM);

		af.type      = gsn_pox;
		af.level     = level * 3/4;
		af.duration  = level/2;
		af.modifier  = (-1) * level/10;
		af.bitvector = AFF_PLAGUE;
		affect_to_char(ch,&af);
		send_to_char("You grimace as illness strikes you.\n\r", ch);
		act("$n grimaces and looks ill.", ch, NULL, NULL, TO_ROOM);
	    }
	    else if (effect <= 65)
	    {
		af.where     = TO_AFFECTS;
		af.type      = gsn_curse;
		af.level     = level;
		af.duration  = level/2 + 10;
		af.location  = APPLY_HITROLL;
		af.modifier  = -1 * (level / 8);
		af.bitvector = AFF_CURSE;
		affect_to_char( ch, &af );

		af.location  = APPLY_SAVING_SPELL;
		af.modifier  = level / 8;
		affect_to_char( ch, &af );

		send_to_char( "You feel unclean.\n\r", ch);
		act("$n looks very uncomfortable.", ch, NULL, NULL, TO_ROOM);
	    }
	    else if (effect <= 70)
	    {
		af.where     = TO_AFFECTS;
		af.type      = gsn_sleep;
		af.level     = level;
		af.location  = APPLY_NONE;
		af.modifier  = 0;
		af.duration  = 4;
		af.bitvector = AFF_SLEEP;
		affect_to_char(ch, &af);
		switch_position(ch, POS_SLEEPING);
		send_to_char("A pain explodes inside of your head, and you fall to the ground.\n\r", ch);
		act("$n falls to the ground.", ch, NULL, NULL, TO_ROOM);
	    }
	    else if (effect <= 75)
	    {
		send_to_char("You are blasted by the power of the void!\n\r", ch);
		ch->hit /= 2;
		ch->mana /= 2;
	    }
	    else if (effect <= 80)
	    {
	    }
	    else if (effect <= 85)
	    {
		send_to_char("Darkness rises up around you, and you are sucked into the void!\n\r", ch);
		act("Darkness rises up around $n, sucking $m into the depths of the void!", ch, NULL, NULL, TO_ROOM);
		char_from_room(ch);
		char_to_room(ch, get_room_index(ROOM_VNUM_VOID));
		do_look(ch, "auto");
	    }
	    else if (effect <= 90)
	    {
		af.where     = TO_AFFECTS;
		af.type      = gsn_plague_madness;
		af.level     = level * 3/4;
		af.duration  = level/2;
		af.location  = APPLY_STR;
		af.modifier  = (-1) * level/10;
		af.bitvector = AFF_PLAGUE;
		affect_to_char(ch, &af);
		send_to_char("You moan as blood slowly begins to ooze from your pores, and madness slowly clouds your mind.\n\r", ch);
		act("$n moans as $e begins to sweat, blood oozing from $s pores.", ch, NULL, NULL, TO_ROOM);
	    }		
	    else if (effect <= 95)
	    {
	    }
	    else if (effect <= 98)
	    {
	    }
	    else if (effect <= 99)
	    {
	    }
	    else if (!IS_NPC(ch))
	    {
		send_to_char("Dark powers of the void assault your mind, and you feel your ability to summon demons diminish!\n\r", ch);
		ch->pcdata->learned[sn] -= 5;
	    }
	}
		
    }
    else
    {
	send_to_char("There is no inscribed symbol here.\n\r", ch);
	return FALSE;
    }

    return TRUE;
}

bool spell_nexus( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *portal;
    ROOM_INDEX_DATA *to_room, *from_room;

    if (((victim = get_char_world( ch, target_name)) != NULL)
    && (room_is_affected(victim->in_room, gsn_sanctify)))
    {
         send_to_char("A holy power protects your target.\n\r",ch);
         return FALSE;
    }

    from_room = ch->in_room;

        if ( ( victim = get_char_world( ch, target_name ) ) == NULL
    ||   victim == ch
    ||   (to_room = victim->in_room) == NULL
    ||   !can_see_room(ch,to_room) || !can_see_room(ch,from_room)
    ||   IS_SET(to_room->room_flags, ROOM_SAFE)
    ||   IS_SET(from_room->room_flags,ROOM_SAFE)
    ||   IS_SET(to_room->room_flags, ROOM_PRIVATE)
    ||   IS_SET(to_room->room_flags, ROOM_SOLITARY)
    ||   IS_SET(to_room->room_flags, ROOM_NO_RECALL)
    ||   IS_SET(from_room->room_flags,ROOM_NO_RECALL)
    ||   IS_SET(victim->in_room->room_flags, ROOM_NOGATE)
    ||   (!IS_NPC(victim) && victim->level >= LEVEL_HERO)  /* NOT trust */
    ||   (IS_NPC(victim) && IS_SET(victim->imm_flags,IMM_SUMMON))
    ||   (!IS_NPC(victim) && IS_SET(victim->act,PLR_NOSUMMON) && !IS_PK(victim,ch))
    ||   IS_SET(ch->in_room->room_flags, ROOM_NOGATE)
    ||   saves_spell ( level, ch, victim,DAM_NONE))
    {
        send_to_char( "You failed.\n\r", ch );
        return TRUE;
    }

    /* portal one */
    portal = create_object(get_obj_index(OBJ_VNUM_PORTAL),0);
    portal->timer = 1 + level / 10;
    portal->value[3] = to_room->vnum;

    obj_to_room(portal,from_room);

    act("$p rises up from the ground.",ch,portal,NULL,TO_ROOM);
    act("$p rises up before you.",ch,portal,NULL,TO_CHAR);

    /* no second portal if rooms are the same */
    if (to_room == from_room)
        return TRUE;

    /* portal two */
    portal = create_object(get_obj_index(OBJ_VNUM_PORTAL),0);
    portal->timer = 1 + level/10;
    portal->value[3] = from_room->vnum;

    obj_to_room(portal,to_room);

    if (to_room->people != NULL)
    {
        act("$p rises up from the ground.",to_room->people,portal,NULL,TO_ROOM);
        act("$p rises up from the ground.",to_room->people,portal,NULL,TO_CHAR);
    }

    return TRUE;
}

bool spell_nightfears(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj, *fObj = NULL;
    int i;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (target_name[0] == '\0')
    {
	send_to_char("Whom do you wish to infect with the fears of the night?\n\r", ch);
	return FALSE;
    }

    if ((victim = get_char_world(ch, target_name)) == NULL)
    {
	send_to_char("They're not here.\n\r", ch);
	return FALSE;
    }

    if (is_safe_spell(ch, victim, FALSE) || (victim->in_room != NULL && is_symbol_present(*victim->in_room, OBJ_VNUM_SYMBOL_PROTECT)))
    {
        act("$N is safe from your predations.", ch, NULL, victim, TO_CHAR);
	    return FALSE;
    }

    for (obj = ch->carrying; obj; obj = obj->next_content)
	if (!IS_SET(obj->extra_flags[0], ITEM_NODESTROY) && !obj->worn_on)
        {
	    for (i = 0; i < MAX_LASTOWNER; i++)
	        if (!str_cmp(obj->lastowner[i], IS_NPC(victim) ? victim->short_descr : victim->name))
		{
		    fObj = obj;
		    break;
		}

	    if (fObj)
	        break;
        }

    if (!fObj)
    {
	send_to_char("You carry nothing recently owned by your victim.\n\r", ch);
	return FALSE;
    }

    if (is_affected(victim, sn))
    {
	send_to_char("They are already infected with the fears of the night.\n\r", ch);
	return FALSE;
    }

    act("$p is consumed in the casting.", ch, fObj, NULL, TO_CHAR);
    extract_obj(fObj);
   
    int effectLevel(level);
    check_maleficinsight_attack(ch, victim, level, effectLevel);
    
	if (saves_spell(level + lethebane_sleep_level_mod(ch, victim), NULL, ch, DAM_NEGATIVE))
    {
	send_to_char("Dark nightmares skitter at the edges of your sanity, but you focus them away.\n\r", victim);
	send_to_char("They resist your spell.\n\r", ch);
	return TRUE;
    }

    i = number_range(1, 3);

    af.where	 = TO_OAFFECTS;
    af.type	 = sn;
    af.level	 = effectLevel;
    af.duration	 = level / 2;
    af.location  = APPLY_NONE;
    af.modifier  = i * 100;
    af.bitvector = AFF_NIGHTFEARS;
    affect_to_char(victim, &af);

    af.where	 = TO_AFFECTS;
    af.modifier  = 0;
    af.bitvector = AFF_SLEEP;
    affect_to_char(victim, &af);

    act("You call upon the powers of the Void, drawing the unwholesome attention of its denizens to the dreams of $N.", ch, NULL, victim, TO_CHAR);

    act("A cold chill skitters across your heart, as the visions of your own nightmares seem to take on a horrifying reality all their own.", ch, NULL, victim, TO_VICT);

    // Also apply a curse
    if (!IS_AFFECTED(victim, AFF_CURSE))
    {
        AFFECT_DATA caf = {0};
        caf.where = TO_AFFECTS;
        caf.type = gsn_curse;
        caf.level = effectLevel;
        caf.duration = 10 + (caf.level / 2);
        caf.location = APPLY_HITROLL;
        caf.modifier = -caf.level / 8;
        caf.bitvector = AFF_CURSE;
        affect_to_char(victim, &caf);

        caf.location = APPLY_SAVING_SPELL;
        caf.modifier = caf.level / 8;
        affect_to_char(victim, &caf);

        send_to_char("You feel unclean.\n", victim);
        act("$n looks very uncomfortable.", victim, NULL, NULL, TO_ROOM);
    }

    return TRUE;
}

bool spell_phasedoor( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim;

    if (((victim = get_char_area( ch, target_name)) != NULL)
    && (room_is_affected(victim->in_room, gsn_sanctify)))
    {
         send_to_char("A holy power protects your target.\n\r",ch);
         return FALSE;
    }

    if ( ( victim = get_char_area( ch, target_name ) ) == NULL
    ||   victim == ch
    ||   victim->in_room == NULL
    ||   !can_see_room(ch,victim->in_room)
    ||   IS_SET(victim->in_room->room_flags, ROOM_NOGATE)
    ||   (victim->in_room->area->area_flags & AREA_UNCOMPLETE )
    ||   IS_SET(victim->in_room->room_flags, ROOM_NOMAGIC)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SAFE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
    ||   IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
    ||   IS_SET(ch->in_room->room_flags, ROOM_NOGATE)
    ||   IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
    ||   (!IS_PK(ch, victim) && !IS_NPC(victim) && IS_SET(victim->act, PLR_NOSUMMON))
    ||   (!IS_NPC(victim) && victim->level > LEVEL_HERO)  /* NOT trust */
    ||   (IS_NPC(victim) && IS_SET(victim->imm_flags,IMM_SUMMON))
    ||   saves_spell( level, ch, victim,DAM_OTHER) )
        /*This next if added so greater demons can gate into any area, except
                a safe room -- ie, limbo, in the case -- or nomagic rooms,
                but allowing the demons access to nogate/norecall areas for
                added mortal pain and suffering *innocent* */
      if (!IS_NPC(ch) || ch->pIndexData->vnum < 70 || ch->pIndexData->vnum > 75
|| IS_SET(victim->in_room->room_flags, ROOM_SAFE)
        || IS_SET(victim->in_room->room_flags, ROOM_NOMAGIC))
    {
        send_to_char( "You failed.\n\r", ch );
        return TRUE;
    }

    act("$n gestures, and steps through a black rift in the fabric of space!",ch,NULL,NULL,TO_ROOM);
    send_to_char("You open a rift in the fabric of space, and step through.\n\r",ch
);
    act("The magical winds of the void tear at you!", ch, NULL, NULL, TO_CHAR);
    if (!IS_NPC(ch))
        damage_old( ch, ch, number_range(ch->level/2, (ch->level*3)/2), sn, DAM_NEGATIVE ,TRUE);
    char_from_room(ch);
    char_to_room(ch,victim->in_room);
    act("A black rift opens in space, and $n steps out.",ch,NULL,NULL,TO_ROOM);
    do_look(ch,"auto");

    return TRUE;
}


bool spell_plague( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    // Check for witherpox
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int witherSkill(get_skill(ch, gsn_witherpox));
    if (witherSkill > 0)
        sn = gsn_pox;

    if (is_affected(victim, sn))
    {
        if (victim == ch) send_to_char("You are already diseased.\n", ch);
        else act("$N is already diseased.", ch, NULL, victim, TO_CHAR);
        return true;
    }

    int effectLevel((level * 3) / 4);
    check_maleficinsight_attack(ch, victim, level, effectLevel);

    if (saves_spell(level, ch, victim, DAM_DISEASE) || (IS_NPC(victim) && IS_SET(victim->act,ACT_UNDEAD)))
    {
        if (ch == victim) send_to_char("You feel momentarily ill, but it passes.\n", ch);
        else act("$N seems to be unaffected.", ch, NULL, victim, TO_CHAR);
        return true;
    }

    // Check for touch of the desecrator
    if (number_percent() <= get_skill(ch, gsn_touchofthedesecrator))
    {
        check_improve(ch, victim, gsn_touchofthedesecrator, true, 2);
        effectLevel += 10;
    }
    else
        check_improve(ch, victim, gsn_touchofthedesecrator, false, 2);

    // Perform witherpox skill test
    if (number_percent() <= witherSkill)
    {
        check_improve(ch, victim, gsn_witherpox, true, 2);
        effectLevel += 4;
    }
    else
        check_improve(ch, victim, gsn_witherpox, false, 2);

    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = effectLevel;
    af.duration  = level;
    af.location  = APPLY_STR;
    af.modifier  = -5;
    af.bitvector = AFF_PLAGUE;
    affect_join(victim,&af);

    if (sn == gsn_pox)
    {
        send_to_char("You grimace as illness strikes you.\n\r",victim);
        act("$n grimaces and looks ill.", victim,NULL,NULL,TO_ROOM);
    }
    else
    {
        send_to_char("You scream in agony as plague sores erupt from your skin.\n\r",victim);
        act("$n screams in agony as plague sores erupt from $s skin.", victim,NULL,NULL,TO_ROOM);
    }
    check_boonoftheleech(ch, victim, sn, level);
    
    if (number_percent() <= get_skill(ch,gsn_pestilence))
    {
        int levelmod(0);
        for (CHAR_DATA * pch(victim->in_room->people); pch != NULL; pch = pch->next_in_room)
        {
            if (ch->mana - 3 < 0)
                break;

            if (pch == ch || !is_same_group(victim,pch) || pch == victim || IS_AFFECTED(pch,AFF_PLAGUE))
                continue;
            
            expend_mana(ch, 3);
            if (saves_spell(level+levelmod, ch, victim,DAM_DISEASE) || (IS_NPC(victim) && IS_SET(victim->act,ACT_UNDEAD)))
            {
                send_to_char("You feel momentarily ill, but it passes.\n\r",pch);
                act("$n seems to be unaffected.",pch,NULL,NULL,TO_ROOM);
                continue;
            }

            affect_join(pch, &af);
            ++levelmod;
            send_to_char("You scream in agony as plague sores erupt from your skin.\n\r", pch);
            act("$n screams in agony as plague sores erupt from $s skin.", pch, NULL, NULL, TO_ROOM);
        }	   
        check_improve(ch, victim, gsn_pestilence, true, 1);
    }
    else
        check_improve(ch, victim, gsn_pestilence, false, 1);

    return true;
}

bool spell_plague_madness(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (IS_AFFECTED(victim, AFF_PLAGUE))
    {
	if (victim == ch)
	    send_to_char("You are already diseased.\n\r", ch);
	else
            act("$N is already diseased.", ch, NULL, victim, TO_CHAR);
        return FALSE;
    }

    if (saves_spell(level, ch, victim,DAM_DISEASE) ||
        (IS_NPC(victim) && IS_SET(victim->act,ACT_UNDEAD)))
    {
        if (ch == victim)
            send_to_char("You feel slightly ill, but it passes.\n\r", ch);
        else
            act("$N seems to be unaffected.",ch,NULL,victim,TO_CHAR);
        return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level * 3/4;
    af.duration  = level/2;
    af.location  = APPLY_STR;
    af.modifier  = (-1) * level/10;
    af.bitvector = AFF_PLAGUE;
    affect_to_char(victim, &af);

    send_to_char("You moan as blood slowly begins to ooze from your pores, and madness slowly clouds your mind.\n\r", victim);
    act("$n moans as $e begins to sweat, blood oozes from $s pores.", victim, NULL, NULL, TO_ROOM);

    add_event((void *) victim, 1, &event_plague_madness);

    return TRUE;
}

bool spell_poison( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    AFFECT_DATA af = {0};

    if (target == TARGET_OBJ)
    {
        obj = (OBJ_DATA *) vo;

        if (obj->item_type == ITEM_FOOD || obj->item_type == ITEM_DRINK_CON)
        {
            if (IS_OBJ_STAT(obj,ITEM_BLESS) || IS_OBJ_STAT(obj,ITEM_BURN_PROOF))            {
                act("Your spell fails to corrupt $p.",ch,obj,NULL,TO_CHAR);
                return TRUE;
            }
            obj->value[3] = 1;
            act("$p is infused with poisonous vapors.",ch,obj,NULL,TO_ALL);
            return TRUE;
        }

        if (obj->item_type == ITEM_WEAPON)
        {
            if (IS_WEAPON_STAT(obj,WEAPON_FLAMING)
            ||  IS_WEAPON_STAT(obj,WEAPON_FROST)
            ||  IS_WEAPON_STAT(obj,WEAPON_VAMPIRIC)
            ||  IS_WEAPON_STAT(obj,WEAPON_SHARP)
            ||  IS_WEAPON_STAT(obj,WEAPON_VORPAL)
            ||  IS_WEAPON_STAT(obj,WEAPON_SHOCKING)
            ||  IS_OBJ_STAT(obj,ITEM_BLESS) || IS_OBJ_STAT(obj,ITEM_BURN_PROOF))            {
                act("You can't seem to envenom $p.",ch,obj,NULL,TO_CHAR);
                return TRUE;
            }

            if (obj_is_affected(obj, sn))
            {
                act("$p is already envenomed.",ch,obj,NULL,TO_CHAR);
                return FALSE;
            }

            af.where     = TO_OBJECT;
            af.type      = sn;
            af.level     = level;
            af.duration  = level / 12;
            affect_to_obj(obj,&af);

            act("$p is coated with deadly venom.",ch,obj,NULL,TO_ALL);
            return TRUE;
        }

        act("You can't poison $p.",ch,obj,NULL,TO_CHAR);
        return FALSE;
    }

    victim = (CHAR_DATA *) vo;

    if (IS_AFFECTED(victim, AFF_POISON))
    {
        send_to_char("They are already ill.\n\r", ch);
        return FALSE;
    }

    if ( saves_spell( level, ch, victim,DAM_POISON) )
    {
        act("$n turns slightly green, but it passes.",victim,NULL,NULL,TO_ROOM);
        send_to_char("You feel momentarily ill, but it passes.\n\r",victim);
        return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/8;
    af.location  = APPLY_STR;
    af.modifier  = -2;
    af.point     = ch;
    af.bitvector = AFF_POISON;
    affect_join( victim, &af );
    send_to_char( "You feel very sick.\n\r", victim );
    act("$n looks very ill.",victim,NULL,NULL,TO_ROOM);
    return TRUE;
}

bool spell_portal( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *portal, *stone;

        if ( ( victim = get_char_world( ch, target_name ) ) == NULL
    ||   victim == ch
    ||   victim->in_room == NULL
    ||   !can_see_room(ch,victim->in_room)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SAFE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
    ||   IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
    ||   IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
    ||   victim->level >= level + 3
    ||   (!IS_NPC(victim) && victim->level >= LEVEL_HERO)  /* NOT trust */
    ||   (IS_NPC(victim) && IS_SET(victim->imm_flags,IMM_SUMMON))
    ||   (IS_NPC(victim) && saves_spell( level, ch, victim,DAM_NONE) )
    ||  (is_clan(victim) && !is_same_clan(ch,victim)))
    {
        send_to_char( "You failed.\n\r", ch );
        return TRUE;
    }

    stone = get_eq_char(ch,WEAR_HOLD);
    if (!IS_IMMORTAL(ch)
    &&  (stone == NULL || stone->item_type != ITEM_WARP_STONE))
    {
        send_to_char("You lack the proper component for this spell.\n\r",ch);
        return FALSE;
    }

    if (stone != NULL && stone->item_type == ITEM_WARP_STONE)
    {
        act("You draw upon the power of $p.",ch,stone,NULL,TO_CHAR);
        act("It flares brightly and vanishes!",ch,stone,NULL,TO_CHAR);
        extract_obj(stone);
    }

    portal = create_object(get_obj_index(OBJ_VNUM_PORTAL),0);
    portal->timer = 2 + level / 25;
    portal->value[3] = victim->in_room->vnum;

    obj_to_room(portal,ch->in_room);

    act("$p rises up from the ground.",ch,portal,NULL,TO_ROOM);
    act("$p rises up before you.",ch,portal,NULL,TO_CHAR);

    return TRUE;
}


bool spell_possession( int sn, int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( ch->desc == NULL )
        return FALSE;

    if ( ch->desc->original != NULL )
    {
        send_to_char("You are already in possession of someone.\n\r", ch);
        return FALSE;
    }

    if ( is_affected(ch, gsn_possession))
    {
        send_to_char("You're not ready to possess someone again.\n\r", ch);
        return FALSE;
    }

    if (((victim = get_char_world( ch, target_name)) != NULL)
    && (room_is_affected(victim->in_room, gsn_sanctify)))
    {
         send_to_char("A holy power protects your target.\n\r",ch);
         return FALSE;
    }

    if ( ( victim = get_char_world( ch, target_name ) ) == NULL )
    {
        send_to_char( "You stretch out with your senses, but cannot find them.\n\r", ch );
        return FALSE;
    }

    if (victim == ch)
    {
        send_to_char( "Ok.\n\r", ch );
        return FALSE;
    }

    if (!IS_NPC(victim))
    {
        send_to_char("They have too strong a spirit to take control!\n\r",ch);
        return FALSE;
    }

    if (!is_room_owner(ch,victim->in_room) && ch->in_room != victim->in_room
    &&  room_is_private(victim->in_room) && !IS_TRUSTED(ch,IMPLEMENTOR))
    {
        send_to_char("That character is in a private room.\n\r",ch);
        return FALSE;
    }

    if ((victim->act & ACT_AGGRESSIVE) || IS_SET(victim->nact, ACT_PSYCHO))
    {
        send_to_char("You failed.\n\r", ch);
        return FALSE;
    }

    if ( victim->desc != NULL )
    {
        send_to_char( "Someone is already dominating their body.\n\r", ch );
        return FALSE;
    }

    if (ch->level < victim->level)
    {
        send_to_char( "You try to seize control, but they resist!\n\r", ch );
        WAIT_STATE(ch, 5*PULSE_VIOLENCE);
        return TRUE;
    }

    if (IS_SET(victim->imm_flags, IMM_CHARM) || saves_spell(level - 5, ch, victim, DAM_CHARM))
    {
        send_to_char( "You try to seize control, but they resist!\n\r", ch );
        WAIT_STATE(ch, 5*PULSE_VIOLENCE);
        return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = gsn_possession;
    af.level     = level;
    af.duration  = 8;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    af.duration  = number_fuzzy(level/8);
    affect_to_char(victim, &af);

    act("You push off from your own body, seizing control over $N.", ch, NULL, victim, TO_CHAR);
    if (ch->race != global_int_race_shuddeni)
        act("$n's eyes become vacant as $s spirit moves to another host.", ch, NULL, victim, TO_ROOM);
    else
        act("$n's body slumps as $s spirit moves to another host.", ch, NULL, victim, TO_ROOM);

    ch->desc->character = victim;
    ch->desc->original  = ch;
    victim->desc        = ch->desc;
    victim->exp         = ch->exp;
    ch->desc            = NULL;

    /* change communications to match */
    if (ch->prompt != NULL)
        victim->prompt = str_dup(ch->prompt);

    victim->comm = ch->comm;
    victim->lines = ch->lines;
    WAIT_STATE(victim, UMAX(victim->wait, 5*PULSE_VIOLENCE));
    WAIT_STATE(ch, UMAX(victim->wait, 5*PULSE_VIOLENCE));
    return TRUE;
}

bool spell_powerwordfear (int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( is_affected( victim, sn ) )
    {
        if (victim == ch)
          send_to_char("You are already scared.\n\r",ch);
        else
          act("$N is already affected by terror.",
                ch,NULL,victim,TO_CHAR);
        return FALSE;
    }

    if (is_affected(victim, gsn_mindshell) || is_affected(victim,gsn_mindshield))
    {
        act("$N is shielded against that attack", ch, NULL, victim, TO_CHAR);
        return TRUE;
    }

    if (is_affected(victim, gsn_courage))
        level -= 3;

    if (IS_NPC(victim))
        {
        send_to_char("They don't seem to have the sense enough to be scared.\n\r", ch);
        return TRUE;
        }

    if (saves_spell(level, ch, victim,DAM_FEAR))
    {
        send_to_char("They refuse to be terrified.\n\r",ch);
        return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 3+(level/15);
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = 0;
    affect_to_char( victim, &af );

        if (number_bits(9) == 0)
                {
                act("$n screams in absolute terror, and clenches up!", victim, NULL, NULL, TO_ROOM);
                act("$n shivers and convulses as $s heart bursts from the sheer terror!", victim, NULL, NULL, TO_ROOM);
                act("You scream in absolute terror, and clench up!", victim, NULL, NULL, TO_CHAR);
                act("You shiver and convulses as your heart bursts from the sheer terror!", victim, NULL, NULL, TO_CHAR);
                kill_char(victim, ch);
                return TRUE;
                }

    send_to_char( "You feel frightened.\n\r", victim );
    if ( ch != victim )
        act("$N screams in terror!",ch,NULL,victim,TO_CHAR);
    do_flee (victim, "");
    if (victim->fighting != NULL)
    do_flee (victim, "");
    if (victim->fighting != NULL)
    do_flee (victim, "");
    return TRUE;
}

bool spell_pox( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(victim, sn))
    {
        if (victim == ch)
            send_to_char("You are already poxed.\n\r", ch);
        else
            act("$N is already poxed.", ch, NULL, victim, TO_CHAR);
        return FALSE;
    }

    if (saves_spell(level, ch, victim,DAM_DISEASE) ||
        (IS_NPC(victim) && IS_SET(victim->act,ACT_UNDEAD)))
    {
        if (ch == victim)
          send_to_char("You feel slightly ill, but it passes.\n\r", ch);
        else
          act("$N seems to be unaffected.",ch,NULL,victim,TO_CHAR);
        return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type       = sn;
    af.level      = level * 3/4;
    af.duration  = level/2;
    af.location  = APPLY_STR;
    af.modifier  = (-1) * level/10;
    af.bitvector = AFF_PLAGUE;
    affect_to_char(victim,&af);

    send_to_char("You grimace as illness strikes you.\n\r",victim);
    act("$n grimaces and looks ill.", victim,NULL,NULL,TO_ROOM);
    return TRUE;
}

bool spell_riteofkaagn(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj, *fObj = NULL;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    int i;
    char buf[75];

    if (target_name[0] == '\0')
    {
	send_to_char("Whom do you wish to curse?\n\r", ch);
	return FALSE;
    }

    if ((victim = get_char_world(ch, target_name)) == NULL)
    {
	send_to_char("They're not here.\n\r", ch);
	return FALSE;
    }

    if (is_safe_spell(ch, victim, FALSE))
	return FALSE;

    for (obj = ch->carrying; obj; obj = obj->next_content)
	if (!IS_SET(obj->extra_flags[0], ITEM_NODESTROY) && !obj->worn_on)
        {
	    for (i = 0; i < MAX_LASTOWNER; i++)
	        if (!str_cmp(obj->lastowner[i], IS_NPC(victim) ? victim->short_descr : victim->name))
		{
		    fObj = obj;
		    break;
		}

	    if (fObj)
	        break;
        }

    if (!fObj)
    {
	send_to_char("You carry nothing recently owned by your victim.\n\r", ch);
	return FALSE;
    }

    if (IS_AFFECTED(victim,AFF_CURSE))
    {
        send_to_char("They are already cursed.\n\r", ch);
        return FALSE;
    }

    act("You hold $p aloft, calling forth the attention of darker forces.", ch, fObj, NULL, TO_CHAR);
    act("$n holds $p aloft, calling forth the attention of darker forces.", ch, fObj, NULL, TO_ROOM);

    do_say(ch, "Jakulr uhr Ede!");
    do_say(ch, "Jakulr uhr Urka!");
    do_say(ch, "Jakulr uhr Tho!");
    sprintf(buf, "Kaagn ri uhr rla yuyula, jakulr nujn %s!", IS_NPC(victim) ? victim->short_descr : victim->name);
    do_say(ch, buf);

    if (saves_spell(level, ch, victim,DAM_NEGATIVE))
    {
        send_to_char("They resist the power of the Rite.\n\r", ch);
	act("$p is consumed in the casting.", ch, fObj, NULL, TO_CHAR);
	extract_obj(fObj);
        return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = gsn_curse;
    af.level     = level;
    af.duration  = level/2 + 10;
    af.location  = APPLY_HITROLL;
    af.modifier  = -1 * (level / 8);
    af.bitvector = AFF_CURSE;
    affect_to_char( victim, &af );

    af.location  = APPLY_SAVING_SPELL;
    af.modifier  = level / 8;
    affect_to_char( victim, &af );

    act("Dark energies swirls around $p, binding themselves to the energies of $N.", ch, fObj, victim, TO_CHAR);
    act("Dark energies begin to swirl around $p.", ch, fObj, NULL, TO_ROOM);

    send_to_char("A cold chill steals across you, as sinister energies clench about the fabric of your soul.\n\r", victim);

    act("$p is consumed in the casting.", ch, fObj, NULL, TO_CHAR);
    extract_obj(fObj);

    return TRUE;
}



bool spell_siphonmana( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo; 
    int dam;

    dam = dice( level, 6 );
    if ( saves_spell( level, ch, victim, DAM_NEGATIVE ) )
        dam /= 2;

    if (IS_NPC(victim))
        dam = UMIN(dam, 75);
    else
	dam = UMIN(victim->mana, dam);

    act("You open a dark conduit to $N, drawing on their mental energy!",ch,NULL,victim,TO_CHAR);
    act("$n points at $N, and an inky conduit of pure darkness jumps between them!",ch, NULL, victim, TO_NOTVICT);

    if (victim->position <= POS_SLEEPING)
	send_to_char("You feel a dark tug at your mind, and you grow wearier.\n\r", victim);
    else
    	act("$n points at you, and a dark conduit begins to drain your energy!", ch, NULL,victim,TO_VICT);

    if (victim->mana > 0) 
        expend_mana(victim, UMIN(dam, victim->mana));

    ch->mana = UMIN(ch->mana+dam, ch->max_mana);
    return TRUE;
}

bool spell_sleep( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( is_affected(ch, sn) )
    {
	send_to_char("You cannot induce sleep again so soon.\n\r", ch);
	return FALSE;
    }

    if ( IS_AFFECTED(victim, AFF_SLEEP)
    ||   (IS_NPC(victim) && IS_SET(victim->act,ACT_UNDEAD)))
        return FALSE;

    if (victim->fighting || IS_SET(victim->act, ACT_NOSUBDUE) || is_affected(victim, sn))
    {
        act("They resist your spell.", ch, NULL, victim, TO_CHAR);
        return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.location  = APPLY_NONE;
    af.modifier  = 0;

    //Stop him from casting it again
    af.duration = 4 + number_range(1, 6);
    af.bitvector = 0;
    affect_to_char(ch, &af);


    if (check_spirit_of_freedom(victim))
    {
	send_to_char("You feel the spirit of freedom surge within you.\n\r", victim);
	act("$N glows briefly, and your sleep spell dissipates.", ch, NULL, victim, TO_CHAR);

	af.duration  = 1;
	af.bitvector = 0;
	affect_to_char(victim, &af);

	return TRUE;
    }

    if (saves_spell(level + lethebane_sleep_level_mod(ch, victim), ch, victim, DAM_OTHER))
    {
        send_to_char("You failed.\n\r", ch);

	af.duration  = 1;
	af.bitvector = 0;
	affect_to_char(victim, &af);

        return TRUE;
    }

    af.duration  = UMAX(1, level/12 - 1);
    af.bitvector = AFF_SLEEP;
    affect_join( victim, &af );

    if (IS_AWAKE(victim))
    {
        send_to_char( "You feel very sleepy ..... zzzzzz.\n\r", victim );
        act( "$n goes to sleep.", victim, NULL, NULL, TO_ROOM );
	switch_position(victim, POS_SLEEPING);
    }
    return TRUE;
}

bool spell_summon( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim;
    OBJ_DATA *symbol;
    bool circle = FALSE;

    if (!ch->in_room)
	return FALSE;

    if ((victim = get_char_world( ch, target_name)) == NULL)
    {
	send_to_char("You failed.\n\r", ch);
	return TRUE;
    }
    
    if (room_is_affected(victim->in_room, gsn_sanctify))
    {
         send_to_char("A holy power protects your target.\n\r",ch);
         return FALSE;
    }

    for (symbol = ch->in_room->contents; symbol; symbol = symbol->next_content)
	if (symbol->pIndexData->vnum == OBJ_VNUM_SYMBOL_SUMMON)
	    break;

    if (!symbol)
    {
	send_to_char("You cannot perform the ritual of summoning without a pentagram.\n\r", ch);
	return FALSE;
    }

    if (victim->in_room != NULL && is_symbol_present(*victim->in_room, OBJ_VNUM_SYMBOL_PROTECT))
		circle = TRUE;

    if ( ( victim = get_char_world( ch, target_name ) ) == NULL
    ||   (!IS_NPC(victim) && (victim->in_room->area != ch->in_room->area))
    ||   victim == ch
    ||   victim->in_room == NULL
    ||   IS_SET(ch->in_room->room_flags, ROOM_SAFE)
    ||   IS_SET(ch->in_room->room_flags, ROOM_NOSUM_TO)
    ||   (IS_SET(ch->in_room->room_flags, ROOM_NO_MOB) && (IS_NPC(ch)))
    ||   IS_SET(victim->in_room->room_flags, ROOM_NOSUM_FROM)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SAFE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
    ||   IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
    ||   (IS_NPC(victim) && IS_SET(victim->act,ACT_AGGRESSIVE))
    ||   (!IS_NPC(victim) && victim->level >= LEVEL_IMMORTAL)
    ||   (victim->in_room->room_flags & ROOM_NOMAGIC)
    ||   victim->fighting != NULL
    ||   victim->level > 51
    ||   is_affected(victim, gsn_anchor)
    ||   (is_affected(victim, gsn_camoublind) && !IS_PAFFECTED(ch, AFF_SHARP_VISION))
    ||   (IS_NPC(victim) && IS_SET(victim->imm_flags,IMM_SUMMON))
    ||   (IS_NPC(victim) && victim->pIndexData->pShop != NULL)
    ||   (saves_spell( level, ch, victim,DAM_OTHER))
    ||   (!IS_NPC(victim) && IS_SET(victim->act,PLR_NOSUMMON) && !IS_PK(victim,
ch))
    ||   (number_percent() < get_resist(victim, RESIST_SUMMON))
    ||   circle)
    {
        send_to_char( "You failed.\n\r", ch );
        return TRUE;
    }

    act( "$n disappears suddenly.", victim, NULL, NULL, TO_ROOM );
    char_from_room( victim );
    char_to_room( victim, ch->in_room );
    act( "$n arrives suddenly.", victim, NULL, NULL, TO_ROOM );
    act( "$n has summoned you!", ch, NULL, victim,   TO_VICT );
    do_look( victim, "auto" );

    if (IS_NPC(victim))
	if ((IS_EVIL(ch) && IS_GOOD(victim) && (number_bits(1) == 0))
	 || (IS_NEUTRAL(ch) && !IS_NEUTRAL(victim) && (number_bits(2) == 0))
	 || (IS_GOOD(ch) && IS_EVIL(victim) && (number_bits(1) == 0)))
	{
	    act("$n roars in fury!", victim, NULL, NULL, TO_ROOM);
	    one_hit(victim, ch, TYPE_UNDEFINED, HIT_PRIMARY, false);
	}

    return TRUE;
}

bool spell_suppress( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;
    DESCRIPTOR_DATA *d;

        if (is_affected(ch, gsn_suppress))
        {
        send_to_char("You cannot summon the power to curse the area again.\n\r", ch);
        return FALSE;
        }

        if (area_is_affected(ch->in_room->area, gsn_suppress))
        {
        send_to_char("This place is already suppressing attempts to flee it.\n\r", ch);
        return FALSE;
        }

        af.where        = TO_AREA;
        af.type = sn;
        af.level        = level;
        af.duration     = 4;
        af.location     = 0;
        af.modifier     = 0;
        af.bitvector   = 0;
        affect_to_area(ch->in_room->area, &af);

        af.duration     = 120;
        affect_to_char(ch, &af);

        for (d = descriptor_list; d != NULL; d = d->next)
        {
        if (d && d->connected == CON_PLAYING && d->character && d->character->in_room && d->character->in_room->area == ch->in_room->area)
                send_to_char("An unsettling force settles around you, as you begin to feel trapped and abandoned.\n\r", d->character);
        }

    return TRUE;
}

bool spell_tentacles( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *tentacles;
    CHAR_DATA *vch;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
        send_to_char("You aren't prepared to summon tentacles again yet!\n\r", ch);
        return FALSE;
    }

    for (vch = ch->in_room->people; vch != NULL; vch=vch->next_in_room)
    {
        if (IS_NPC(vch) && vch->pIndexData->vnum == MOB_VNUM_TENTACLES)
        {
            act("There is already a mess of tentacles thrashing about!", ch, NULL, NULL, TO_CHAR);
            return FALSE;
        }
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 4;
    af.location =  0;
    af.modifier =  0;
    af.bitvector = 0;
    affect_to_char( ch, &af );

    act("A mass of tentacles bursts from the ground!", ch, NULL, NULL, TO_ROOM);
    act("A mass of tentacles bursts from the ground!", ch, NULL, NULL, TO_CHAR);

    tentacles = create_mobile(get_mob_index(MOB_VNUM_TENTACLES));
    tentacles->timer = 2;
    char_to_room(tentacles, ch->in_room);

    return TRUE;
}


bool spell_veilsight(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {
	send_to_char("Your eyes already see beyond the veil.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = level;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_DETECT_INVIS;
    affect_to_char(ch, &af);

    send_to_char("The world momentarily turns hazy as your eyes shift to see beyond the veil.\n\r", ch);
    act("$n's eyes momentarily shift to white, before returning to normal.", ch, NULL, NULL, TO_ROOM);

    return TRUE;
}    

bool spell_voidwalk( int sn, int level,CHAR_DATA *ch,void *vo,int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (IS_PAFFECTED(ch, AFF_VOIDWALK))
    {
        act("You shift back into the material world.", ch, NULL, NULL, TO_CHAR);
        act("$n shifts $s essence back into the material world.", ch, NULL, NULL, TO_ROOM);
        ch->hit = UMAX(1, ch->hit / 4);
        affect_strip(ch, gsn_voidwalk);

	af.where     = TO_PAFFECTS;
	af.type      = gsn_voidwalk;
	af.level     = level;
	af.duration  = ch->level / 4;
	af.location  = APPLY_HIT;
	af.modifier  = ch->level * -1;
	af.bitvector = 0;
	affect_to_char(ch, &af);

	WAIT_STATE(ch, UMAX(ch->wait, PULSE_VIOLENCE * 2));
	send_to_char("You pause for a moment, feeling drained from the experience.\n\r", ch);    		
        return TRUE;
    }

    if (is_affected(ch, gsn_voidwalk))
    {
	send_to_char("You are not yet ready to face the void again.\n\r", ch);
	return FALSE;
    }

    act("$n shifts $s essence into the plane of the void, and disappears from sight.", ch, NULL, NULL, TO_ROOM);
    act("You shift your essence into the plane of the void, and disappear from sight.", ch, NULL, NULL, TO_CHAR);

    af.where     = TO_PAFFECTS;
    af.type      = gsn_voidwalk;
    af.level     = level;
    af.duration  = number_fuzzy(level / 7);
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_VOIDWALK;
    affect_to_char(ch, &af);

    return TRUE;
}

// need to add a mount check here

bool spell_vortex (int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    ROOM_INDEX_DATA *location;
    bool freedom;

    if (is_affected(ch, gsn_vortex))
    {
        send_to_char("You are already in the Void.\n\r", ch);
        return FALSE;
    }

    if (is_affected(victim, gsn_vortex))
    {
        send_to_char("They are already in the Void.\n\r", ch);
        return FALSE;
    }

    if (IS_NPC(victim))
    {
        send_to_char( "You may only draw into the Void sentient targets.\n\r",ch);
        return FALSE;
    }

    if (victim == ch)
    {
        send_to_char ("You would not wish to venture into the Void alone.\n\r", ch);
        return FALSE;
    }

    if ( victim->fighting != NULL )
    {
        send_to_char ("They are busy fighting right now.\n\r", ch);
        return FALSE;
    }

    if (saves_spell(level,ch, victim,DAM_NEGATIVE))
    {
        send_to_char("They resist your magic.\n\r", ch);
        return TRUE;
    }

    if (victim != ch)
    {
        act("$n raises both arms, and a vortex of power appears!", ch,NULL,NULL,TO_ROOM);
        act("You raise both arms and summon a vortex of power!", ch,NULL,victim,TO_CHAR);
    }

    stop_fighting_all(ch);
    stop_fighting_all(victim);

    if (victim->position == POS_SLEEPING)
    {
	act("You awaken as the raw power of the void swirls around you!", ch, NULL, NULL, TO_CHAR);
	act("$n awakens as the raw power of the void swirls around $m!", ch, NULL, NULL, TO_ROOM);
    }
	
    if ((freedom = check_spirit_of_freedom(victim)) == TRUE)
    {
	send_to_char("The spirit of freedom surges within you, and you resist the draw of the vortex!\n\r", victim);
	act("$n glows brightly, and resists the pull of the vortex!", victim, NULL, NULL, TO_ROOM);
    }

    send_to_char( "You are forcefully drawn into the Void through the vortex.\n\r", ch);
    if (!freedom)
        send_to_char( "You are forcefully drawn into the Void through the vortex.\n\r", victim);
    act("$n is drawn into the Void!",ch,NULL,NULL,TO_ROOM);
    if (!freedom)
        act("$n is drawn into the Void!",victim,NULL,NULL,TO_ROOM);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 2;
    af.location  = 0;
    af.modifier  = ch->in_room->vnum;
    af.bitvector = 0;
    affect_to_char( ch, &af );
    if (!freedom)
        affect_to_char( victim, &af );

    location = get_room_index(number_range(50,80));

    char_from_room( ch );
    char_to_room( ch, location );
    if (!freedom)
    {
        char_from_room( victim );
        char_to_room( victim, location );
	do_look(victim, "auto");
    }
    do_look( ch, "auto" );
    set_fighting(victim, ch);
    return TRUE;
}

bool spell_weaken( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( is_affected( victim, sn ) )
    {
        send_to_char("They are already weak.\n\r", ch);
        return FALSE;
    }

    if (saves_spell( level, ch, victim,DAM_OTHER) )
    {
        send_to_char("You failed.\n\r", ch);
        return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level / 2;
    af.location  = APPLY_STR;
    af.modifier  = -1 * (level / 7);
    af.bitvector = AFF_WEAKEN;
    affect_to_char( victim, &af );
    send_to_char( "You feel your strength slip away.\n\r", victim );
    act("$n looks tired and weak.",victim,NULL,NULL,TO_ROOM);
    return TRUE;
}

void do_erase(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj, *obj_next;
    bool found = FALSE;

    if (!ch->in_room)
	return;

    if (IS_OAFFECTED(ch, AFF_GHOST))
    {
	send_to_char("Your incorporeal form cannot erase anything.\n\r", ch);
	return;
    }

    for (obj = ch->in_room->contents; obj; obj = obj_next)
    {
	obj_next = obj->next_content;

	if ((obj->pIndexData->vnum >= OBJ_VNUM_SYMBOL_FIRST) && (obj->pIndexData->vnum <= OBJ_VNUM_SYMBOL_LAST))
	{
        // Check for inscribe skill; lacking it results in a curse
        if (get_skill(ch, gsn_inscribe) > 0)
        {
            act("You carefully smudge the lines of $p, gradually breaking down its power.", ch, obj, NULL, TO_CHAR);
            act("$n carefully smudges the lines of $p, breaking it down gradually.", ch, obj, NULL, TO_ROOM);
        }
        else
        {
	        act("You scuff $p, destroying it.", ch, obj, NULL, TO_CHAR);
    	    act("$n scuffs $p, destroying it.", ch, obj, NULL, TO_ROOM);

            if (!IS_AFFECTED(ch, AFF_CURSE))
            {
                AFFECT_DATA af = {0};
                af.where    = TO_AFFECTS;
                af.type     = gsn_curse;
                af.level    = obj->level;
                af.duration = (af.level / 2 )+ 10;
                af.bitvector = AFF_CURSE;
                af.location = APPLY_HITROLL;
                af.modifier = -af.level / 8;
                affect_to_char(ch, &af);

                af.location = APPLY_SAVING_SPELL;
                af.modifier = af.level / 8;
                affect_to_char(ch, &af);

                act("Dark mists rush out of $p, settling over you!", ch, obj, NULL, TO_CHAR);
                act("Dark mists rush out of $p, settling over $n!", ch, obj, NULL, TO_ROOM);
                send_to_char("You feel unclean.\n", ch);
                act("$n looks very uncomfortable.", ch, NULL, NULL, TO_ROOM);
            }
        }
	    extract_obj(obj);
	    room_affect_strip(ch->in_room, gsn_inscribe);
	    found = TRUE;

	}
    }

    if (!found)
    {
	send_to_char("There are no symbols here to erase.\n\r", ch);
	return;
    }
}

void do_inscribe(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    OBJ_DATA *obj, *reagent = NULL;
    MOB_INDEX_DATA *pMobIndex;
    CHAR_DATA *vch;
    int chance, symbol, i, rnum = 0, stype = 0, race;
    bool found;

    if (!ch->in_room)
	return;
    
    if (ch->in_room->sector_type == SECT_AIR ||
        ch->in_room->sector_type == SECT_WATER_SWIM ||
	ch->in_room->sector_type == SECT_WATER_NOSWIM ||
	ch->in_room->sector_type == SECT_UNDERWATER)
    {
        send_to_char("You cannot inscribe a symbol here.\n\r",ch);
	return;
    }

    if ((chance = get_skill(ch, gsn_inscribe)) == 0)
    {
        send_to_char("Huh?\n\r", ch);
        return;
    }

    if (is_affected(ch, gsn_inscribe))
    {
	if (get_modifier(ch->affected, gsn_inscribe) == 0)
	    send_to_char("You are not yet prepared to inscribe another symbol of power.\n\r", ch);
	else
	    send_to_char("You are already creating a symbol of power.\n\r", ch);
	return;
    }

    for (obj = ch->in_room->contents; obj; obj = obj->next_content)
	if ((obj->pIndexData->vnum >= OBJ_VNUM_SYMBOL_FIRST)
	 && (obj->pIndexData->vnum <= OBJ_VNUM_SYMBOL_LAST))
	{
	    send_to_char("A symbol of power already exists here.\n\r", ch);
	    return;
	}

    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
	if (is_affected(vch, gsn_inscribe) && (get_modifier(vch->affected, gsn_inscribe) != 0))
	{
	    act("$N is already inscribing a symbol here.", ch, NULL, vch, TO_CHAR);
	    return;
	}

    if (argument[0] == '\0')
    {
	found = FALSE;
	send_to_char("You have learned the following symbols:\n\r\n\r", ch);
	for (i = 0; inscribe_table[i].name; i++)
	    if (IS_IMMORTAL(ch) || IS_SET(ch->symbols_known, inscribe_table[i].bit))
	    {
		found = TRUE;
		send_to_char(inscribe_table[i].name, ch);
		send_to_char("\n\r", ch);
	    }

	if (!found)
	    send_to_char("You do not have knowledge of any symbols.\n\r", ch);

	return;
    }

    argument = one_argument(argument, arg);

    /*
     * Step 2: Figure out what symbol they're casting.
     */

    for (symbol = 0; inscribe_table[symbol].name; symbol++)
	if ((IS_IMMORTAL(ch) || IS_SET(ch->symbols_known, inscribe_table[symbol].bit))
	 && is_name(arg, inscribe_table[symbol].name))
	    break;

    if (!inscribe_table[symbol].name)
    {
	send_to_char("You don't know any symbols like that.\n\r", ch);
	return;
    }

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
	send_to_char("What material do you intend to use to inscribe?\n\r", ch);
	return;
    }

    if (!str_cmp(arg, "silver"))
    {
	rnum = OBJ_VNUM_REAGENT_SILVER;
	stype = INSMAT_SILVER;
    }
    else if (!str_cmp(arg, "bone"))
    {
	rnum = OBJ_VNUM_REAGENT_BONEDUST;
	stype = INSMAT_BONEDUST;
    }
    else if (!str_cmp(arg, "charcoal"))
    {
	rnum = OBJ_VNUM_REAGENT_CHARCOAL;
	stype = INSMAT_CHARCOAL;
    }
    else if (!str_cmp(arg, "salt"))
    {
	rnum = OBJ_VNUM_REAGENT_SALT;
	stype = INSMAT_SALT;
    }
    else if (!str_cmp(arg, "mud"))
    {
        rnum = OBJ_VNUM_REAGENT_MUD;
	stype = INSMAT_MUD;
    }
    else if (!str_cmp(arg, "yarrow"))
    {
        rnum = OBJ_VNUM_REAGENT_HERB1;
	stype = INSMAT_HERB1;
    }
    else if (!str_cmp(arg, "paste"))
    {
        rnum = OBJ_VNUM_REAGENT_HERB1;
	stype = INSMAT_HERB1;
    }
    else if (!str_cmp(arg, "vine"))
    {
        rnum = OBJ_VNUM_REAGENT_HERB2;
	stype = INSMAT_HERB2;
    }
    else if (!str_cmp(arg, "bitterthorn"))
    {
        rnum = OBJ_VNUM_REAGENT_HERB2;
	stype = INSMAT_HERB2;
    }
    else if (!str_cmp(arg, "uzith-hazhi"))
    {
        rnum = OBJ_VNUM_REAGENT_HERB3;
	stype = INSMAT_HERB3;
    }
    else if (!str_cmp(arg, "uzith"))
    {
        rnum = OBJ_VNUM_REAGENT_HERB3;
	stype = INSMAT_HERB3;
    }
    else if (!str_cmp(arg, "hazhi"))
    {
        rnum = OBJ_VNUM_REAGENT_HERB3;
	stype = INSMAT_HERB3;
    }
    
    if (rnum > 0)
    {
	for (obj = ch->carrying; obj; obj = obj->next_content)
	{
	    if (obj->pIndexData->vnum == rnum) 
	    {
	        reagent = obj;
		break;
	    }
	      
	    if (obj->pIndexData->vnum == OBJ_VNUM_INSCRIBE_POUCH)
		for (reagent = obj->contains; reagent; reagent = reagent->next_content)
		    if (reagent->pIndexData->vnum == rnum)
			break;
	
	    if (reagent)
		break;
	}

	if (!reagent)
	{
	    send_to_char("You lack the neccesary reagent.\n\r", ch);
	    return;
	}
    }
    else
    {
	if ((reagent = get_obj_carry(ch, arg, ch)) == NULL)
	{
	    send_to_char("You are carrying nothing like that.\n\r", ch);
	    return;
	}

	if (reagent->pIndexData->vnum == OBJ_VNUM_REAGENT_BLOOD)
	{
	    if (reagent->value[0] > 0)
	    {
		pMobIndex = get_mob_index(reagent->value[0]);

		if (IS_SET(pMobIndex->act, ACT_ANIMAL)
		 || IS_SET(pMobIndex->form, FORM_ANIMAL))
		    stype = INSMAT_BLOOD_ANIMAL;
		else if (IS_SET(pMobIndex->act, ACT_UNDEAD)
		 || IS_SET(pMobIndex->form, FORM_UNDEAD))
		    stype = INSMAT_BLOOD_UNDEAD;
		else if ((pMobIndex->race == (race = global_int_race_alatharya))
		 || (reagent->value[1] == race))
		    stype = INSMAT_BLOOD_ALATHARYA;
		else if ((pMobIndex->race == (race = race_lookup("celestial")))
		 || (reagent->value[1] == race))
		    stype = INSMAT_BLOOD_CELESTIAL;
		else if ((pMobIndex->race == (race = global_int_race_chtaren))
		 || (reagent->value[1] == race))
		    stype = INSMAT_BLOOD_CHTAREN;
	    }
	    else
		if (reagent->value[0] == (ch->id * -1))
		    stype = INSMAT_BLOOD_CASTER;
	
	    if (stype == 0)
	    {
		send_to_char("That blood cannot be used to inscribe a symbol.\n\r", ch);
		return;
	    }
	}
	else if (reagent->pIndexData->vnum == OBJ_VNUM_REAGENT_SALT)
	    stype = INSMAT_SALT;
	else if (reagent->pIndexData->vnum == OBJ_VNUM_REAGENT_BONES)
	{
	    if (reagent->value[0] > 0)
	    {
		pMobIndex = get_mob_index(reagent->value[0]);

		if (IS_SET(pMobIndex->act, ACT_UNDEAD)
		 || IS_SET(pMobIndex->form, FORM_UNDEAD))
		    stype = INSMAT_BONES_UNDEAD;
		else if (pMobIndex->race == race_lookup("dragon"))
		    stype = INSMAT_BONES_DRAGON;
	    }

	    if (stype == 0)
	    {
		send_to_char("Those bones cannot be used to inscribe a symbol.\n\r", ch);
		return;
	    }
	}

	if (stype == 0)
	{
	    send_to_char("That cannot be used as a reagent for inscribing a symbol.\n\r", ch);
	    return;
	}
    }

    extract_obj(reagent);

    af.where	 = TO_OAFFECTS;
    af.type	 = gsn_inscribe;
    af.level     = stype;
    af.duration  = -1;
    af.location  = APPLY_HIDE;
    af.bitvector = AFF_INSCRIBE;
    af.modifier  = symbol * MAX_INSCRIBE_ECHOES;
    affect_to_char(ch, &af);

    sprintf(buf, "You carefully begin to inscribe the %s.\n\r", inscribe_table[symbol].name);
    send_to_char(buf, ch);

    sprintf(buf, "$n begins to carefully inscribe the %s.", inscribe_table[symbol].name);
    act(buf, ch, NULL, NULL, TO_ROOM);

    return;
}	

bool spell_desecrateweapon(int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    OBJ_DATA *vObj = (OBJ_DATA *) vo;
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;

    if (!(vObj->item_type == ITEM_WEAPON || vObj->item_type == ITEM_ARROW))
    {
	send_to_char("Only weapons may be desecrated.\n\r", ch);
	return FALSE;
    }

    if (IS_SET(vObj->extra_flags[0], ITEM_BLESS)
      || IS_SET(vObj->extra_flags[0], ITEM_ANTI_EVIL)
)
    {
	act("The purity of $p prevents your attempt to desecrate it.", ch, vObj, NULL, TO_CHAR);
	return FALSE;
    }

    if (vObj->value[3] == DAM_LIGHT)
    {
	send_to_char("You cannot desecrate a weapon of light.\n\r", ch);
	return FALSE;
    }

    if (vObj->value[3] == DAM_NEGATIVE
      || vObj->value[3] == DAM_DEFILEMENT
      || obj_is_affected(vObj,sn))
    {
	send_to_char("That weapon is already tinged with evil.\n\r", ch);
	return FALSE;
    }

    af.where	= TO_OBJECT;
    af.type 	= sn;
    af.level 	= level;
    af.modifier = 0;
    af.location = 0;
    af.duration = level/3 * (IS_SET(vObj->extra_flags[0], ITEM_EVIL) || IS_SET(vObj->extra_flags[0], ITEM_DARK) ? 2 : 1);
    af.bitvector = 0;
    af.point = NULL;
    affect_to_obj(vObj, &af);
    
    act("$p takes on a malevolent tinge.", ch, vObj, NULL, TO_CHAR);
    act("$n chants, and $p takes on a malevolent tinge.", ch, vObj, NULL, TO_ROOM);
    return TRUE;
}

bool spell_dim( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj;
    ROOM_INDEX_DATA *room;
    AFFECT_DATA af, *paf;
    af.valid = TRUE;
    af.point = NULL;
    
    if (ch->in_room == NULL)
	return FALSE;
    
    if (target_name[0] == '\0')
    {
	room = ch->in_room;
	if ((paf = affect_find(room->affected,gsn_continual_light)) == NULL)
        paf = affect_find(room->affected, gsn_undyingradiance);

    if (paf != NULL)
	{
	    if (!saves_dispel(level,paf->level,paf->duration))
	    {
		room_affect_strip(room,paf->type);
		act("The amplified ambient light fades to normal.",ch,NULL,NULL,TO_CHAR);
		act("The amplified ambient light fades to normal.",ch,NULL,NULL,TO_ROOM);
	    }
	    else
		act("The ambient light remains amplified.",ch,NULL,NULL,TO_CHAR);
	    return TRUE;
	}	

	if (room_is_affected(room, sn))
	{
	    send_to_char("This room is already dim.\n\r", ch);
	    return FALSE;
	}
	af.where = TO_ROOM_AFF;
	af.type = sn;
	af.level = level;
	af.location = 0;
	af.modifier = 0;
	af.duration = 12;
	af.bitvector = 0;
	affect_to_room(room, &af);

    	act("$n chants, and the ambient light fades to darkness.",ch, NULL, NULL, TO_ROOM);
	act("The ambient light fades to darkness.",ch,NULL,NULL,TO_CHAR);
	return TRUE;
    }

    if ((obj = get_obj_list(ch, target_name, ch->carrying)) == NULL)
	if ((obj = get_obj_list(ch, target_name,ch->in_room->contents)) == NULL)
	{
	    send_to_char("You don't see that here.\n\r",ch);
	    return FALSE;
	}

    if (obj->worn_on != 0)
    {
	send_to_char("You can't concentrate on that while you're using it.\n\r",ch);
	return FALSE;
    }
    if (obj_is_affected(obj,gsn_aura))
    {
	act("$p is glowing too radiantly for you to dim.",ch,NULL,NULL,TO_CHAR);
	return FALSE;
    }
    if (!IS_OBJ_STAT(obj,ITEM_GLOW))
    {
	act("$p is already dim.", ch, obj, NULL, TO_CHAR);
	return FALSE;
    }
    int success = get_skill(ch,sn) / 2;
    if (IS_SET(obj->extra_flags[0],ITEM_BLESS))
	success -= 10;
    if (IS_SET(obj->extra_flags[0],ITEM_EVIL))
	success += 10;
    if (IS_SET(obj->extra_flags[0],ITEM_DARK))
	success += 10;
    if (IS_SET(obj->extra_flags[0],ITEM_MAGIC))
	success -= 10;
    if (IS_SET(obj->extra_flags[0],ITEM_ANTI_EVIL))
	success -= 10;
    if (IS_SET(obj->extra_flags[0],ITEM_ANTI_GOOD))
	success += 10;
    if (obj_is_affected(obj,gsn_aura))
	success -= 10;
    success += (ch->level - obj->level);
    if (number_percent() > success)
    {
	act("The light within $p begins to dim, but regains its strength.",ch,obj,NULL,TO_ALL);
	return TRUE;
    }
    REMOVE_BIT(obj->extra_flags[0],ITEM_GLOW);
    act("The light within $p dims.",ch,obj,NULL,TO_ALL);
    object_affect_strip(obj,gsn_aura);
    return TRUE;
}

bool spell_mireofoame(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *vObj = (OBJ_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    OBJ_DATA *symbol;
    CHAR_DATA *demon;

    if (IS_NPC(ch))
    {
	send_to_char("You lack the power to bind a scion of Oame.\n\r", ch);
	return FALSE;
    }

    if (!CAN_WEAR(vObj, ITEM_WEAR_SHIELD))
    {
	send_to_char("Only shields may be bound with a scion of Oame.\n\r", ch);
	return FALSE;
    }

    if (IS_OBJ_STAT(vObj, ITEM_ANTI_EVIL) || IS_OBJ_STAT(vObj, ITEM_BLESS) || CAN_WEAR(vObj, ITEM_NO_SAC))
    {
	act("$p cannot be bound with a scion of Oame.", ch, vObj, NULL, TO_CHAR);
	return FALSE;
    }

    if (obj_is_affected(vObj, gsn_demon_bind))
    {
	act("$p has already been bound with a demonic spirit.", ch, vObj, NULL, TO_CHAR);
	return FALSE;
    }

    if ((silver_state != SILVER_FINAL)
     && (time_info.hour >= season_table[time_info.season].sun_up)
     && (time_info.hour < season_table[time_info.season].sun_down))
    {
	send_to_char("This ritual may not be completed during the day.\n\r",ch);
	return FALSE;
    }

    if (ch->in_room)
    {
	for (symbol = ch->in_room->contents; symbol; symbol = symbol->next_content)
	{
	    if (symbol->pIndexData->vnum == OBJ_VNUM_SYMBOL_BIND)
	    {
		if (symbol->value[0] == 0 || symbol->value[1] != INSMAT_MUD)
		{
		    demon = create_mobile(get_mob_index(MOB_VNUM_DEMON_SCION));

		    char_to_room(demon, ch->in_room);

		    send_to_char("As you complete the casting, a powerful whirlwind of dark smoke begins to swirl above the trigon of binding.\n\r", ch);
		    send_to_char("A mass of glistening tentacles writhe out of the smoke.\n\r", ch);
		    if (number_bits(1) == 0)
		    {
			do_yell(demon, "At last I am free!");
		    	act("$n looks down at the flawed symbol and sneers arrogantly.", demon, NULL, ch, TO_ROOM);
		    	do_say(demon, "Thank you, mortal.");
		    	do_say(demon, "Now you must die.");
		    	act("$n laughs darkly.", demon, NULL, NULL, TO_ROOM);
		    	demon->demontrack = ch;
		    	multi_hit(demon, ch, TYPE_UNDEFINED);
		    	return TRUE;
		    }
		    else
		    {
		 	act("$n grasps at your limbs with its many tentacles.",demon,NULL,ch,TO_VICT);
			act("$n grasps at $N's limbs with its many tentacles.",demon,NULL,ch,TO_NOTVICT);
    			af.where	 = TO_AFFECTS;
			af.type		 = gsn_slow;
			af.level	 = level;
			af.location	 = APPLY_DEX;
			af.modifier  	 = -4;
			af.duration  	 = 24;
			af.bitvector 	= AFF_SLOW;
			affect_to_char(ch, &af);
			act("$n dissolves into a pool of ichor, which seeps into the ground.",demon,NULL,NULL,TO_ROOM);
			extract_char(demon,TRUE);
			return TRUE;
		    }
		}
		else
		    break;
	    }
	}
    }
    else
	return FALSE;

    if (!symbol)
    {
	send_to_char("There must be a trigon of binding present to cast this.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_OBJECT;
    af.type	 = sn;
    af.level	 = level;
    af.location	 = APPLY_RESIST_COLD;
    af.modifier  = level/2;
    af.duration  = -1;
    af.bitvector = ITEM_HUM;
    affect_to_obj(vObj, &af);

    af.type	 = gsn_demon_bind;
    af.location  = 0;
    af.modifier  = ch->id;
    affect_to_obj(vObj, &af);

    act("As you complete the casting, a sudden tremor passes through $p as it is suffused by a thick, nauseating putrescence.", ch, vObj, NULL, TO_CHAR);
    act("As $n completes the casting, a sudden tremor passes through $p as it is suffused by a thick, nauseating putrescence.", ch, vObj, NULL, TO_ROOM);

    return TRUE;
}
