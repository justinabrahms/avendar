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
#include <cmath>
#include "merc.h"
#include "magic.h"
#include "tables.h"
#include "fight.h"
#include "spells_spirit.h"
#include "spells_air.h"
#include "spells_air_earth.h"
#include "spells_fire.h"
#include "alchemy.h"
#include "PhantasmInfo.h"
#include "RoomPath.h"
#include "interp.h"
#include "Faction.h"
#include "NameMaps.h"

/* External declarations */
DECLARE_DO_FUN(do_autoyell);
DECLARE_DO_FUN(do_look);

extern	EXIT_DATA *	new_exit	args( ( void ) );
extern void weather_update(void);
extern void change_faction(CHAR_DATA *victim, unsigned int fnum, short fval, bool display);

OBJ_DATA * make_illusionary_object(const OBJ_DATA & source, int timer)
{
    OBJ_DATA * obj(create_object(get_obj_index(OBJ_VNUM_ILLUSION), 0));
    copy_string(obj->short_descr, source.short_descr);
    setName(*obj, source.name);
    copy_string(obj->description, source.description);

    obj->timer = timer;
    obj->item_type = source.item_type;
    obj->wear_flags = source.wear_flags;

    if (source.item_type == ITEM_WEAPON && IS_WEAPON_STAT(&source, WEAPON_TWO_HANDS))
        SET_BIT(obj->value[4], WEAPON_TWO_HANDS);

    if IS_SET(source.extra_flags[0], ITEM_GLOW) SET_BIT(obj->extra_flags[0], ITEM_GLOW);
    if IS_SET(source.extra_flags[0], ITEM_HUM)  SET_BIT(obj->extra_flags[0], ITEM_HUM);
    return obj;
}

static void alter_appearance(CHAR_DATA & ch, const CHAR_DATA & source, int sn, int level, int duration)
{
    if (ch.orig_long[0] == '\0') copy_string(ch.orig_long, ch.long_descr);
    if (ch.orig_short[0] == '\0') copy_string(ch.orig_short, ch.short_descr);
    if (ch.orig_description[0] == '\0') copy_string(ch.orig_description, ch.description);

    copy_string(ch.long_descr, source.long_descr);
    setFakeName(ch, source.name);
    copy_string(ch.short_descr, (IS_NPC(&source) ? source.short_descr : source.name));
    copy_string(ch.description, source.description);
    
    char buf[MAX_STRING_LENGTH];
    one_argument(source.name, buf);
	sprintf(buf, "%s%ld", buf, ch.id);
    setUniqueName(ch, buf);

    ch.fake_race = source.race;

    AFFECT_DATA af = {0};
    af.where     = TO_OAFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = duration;
    af.bitvector = AFF_DISGUISE;
    affect_to_char(&ch, &af);
}

std::pair<Direction::Value, int> checkWind(CHAR_DATA * ch)
{
    if (ch == NULL || ch->in_room == NULL || !IS_OUTSIDE(ch))
        return std::make_pair(Direction::North, -1);

    return std::make_pair(Direction::ReverseOf(static_cast<Direction::Value>(ch->in_room->area->w_cur.wind_dir)), ch->in_room->area->w_cur.wind_mag);
}

void check_conduitoftheskies(CHAR_DATA * ch)
{
    // Perform skill check
    if (number_percent() > get_skill(ch, gsn_conduitoftheskies))
    {
        check_improve(ch, NULL, gsn_conduitoftheskies, false, 2);
        return;
    }    
    check_improve(ch, NULL, gsn_conduitoftheskies, true, 2);

    // Check for existing effect
    AFFECT_DATA * conduit(get_affect(ch, gsn_conduitoftheskies));
    if (conduit != NULL)
    {
         // Effect already exists, prolong the duration
         if (conduit->duration >= ch->level / 5) ++conduit->duration;
         else conduit->duration = ch->level / 5;
         send_to_char("The force of the lightning feeds the energy within you, prolonging its power!\n", ch);
         return;
    }

    // Echos
    act("The force of the lightning strike surges into you, filling you with preternatural energy!", ch, NULL, NULL, TO_CHAR);
    act("You are transformed from within as you begin to crackle with barely-contained power!", ch, NULL, NULL, TO_CHAR);
    act("Sparks shoot wildly off $n as $e crackles with barely-contained power!", ch, NULL, NULL, TO_ROOM);

    // Apply effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_conduitoftheskies;
    af.level    = ch->level;
    af.duration = ch->level / 5;
    af.modifier = 3 * ch->level;
    af.location = APPLY_HIT;
    affect_to_char(ch, &af);

    ch->hit = UMIN(ch->max_hit, ch->hit + (3 * ch->level));
}

AFFECT_DATA * get_charging_effect(CHAR_DATA * ch)
{
    for (AFFECT_DATA * paf(get_affect(ch, gsn_shockcraft)); paf != NULL; paf = get_affect(ch, gsn_shockcraft, paf))
    {
        if (paf->location == APPLY_HIDE)
            return paf;
    }

    return NULL;
}

AFFECT_DATA * get_charged_effect(CHAR_DATA * ch)
{
    for (AFFECT_DATA * paf(get_affect(ch, gsn_shockcraft)); paf != NULL; paf = get_affect(ch, gsn_shockcraft, paf))
    {
        if (paf->location != APPLY_HIDE)
            return paf;
    }

    return NULL;
}

void do_shockcraft(CHAR_DATA * ch, char * argument)
{
    // Check for skill
    if (get_skill(ch, gsn_shockcraft) <= 0)
    {
        send_to_char("Huh?\n", ch);
        return;
    }

    // Check for an argument
    if (argument[0] == '\0')
    {
        // Display the shock status
        // Two effects are used, one to indicate current charge and one to indicate whether charging
        if (get_charging_effect(ch) == NULL) send_to_char("You are currently discharging energy.\n", ch);
        else send_to_char("You are currently absorbing energy.\n", ch);
        return;
    }

    // Check for charging
    if (!str_prefix(argument, "charge"))
    {
        if (get_charging_effect(ch) != NULL)
        {
            send_to_char("You are already absorbing energy.\n", ch);
            return;
        }

        send_to_char("You will a shift in your inner core as you prepare to absorb energy from lightning.\n", ch);
        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.type     = gsn_shockcraft;
        af.level    = ch->level;
        af.duration = -1;
        af.location = APPLY_HIDE;
        affect_to_char(ch, &af);
        return;
    }

    // Check for discharging
    if (!str_prefix(argument, "discharge"))
    {
        AFFECT_DATA * paf(get_charging_effect(ch));
        if (paf == NULL)
        {
            send_to_char("You are already discharging energy.\n", ch);
            return;
        }

        affect_remove(ch, paf);
        send_to_char("You shift your inner being, preparing to discharge your stored energies.\n", ch);
        return;
    }

    send_to_char("Would you like to charge or discharge energy?\n", ch);
}

bool spell_overcharge(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Make sure this is a wand
    OBJ_DATA * obj(static_cast<OBJ_DATA*>(vo));
    if (obj->item_type != ITEM_WAND)
    {
        act("$p is not a wand.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Make sure it is not nodestroy
    if (IS_SET(obj->extra_flags[0], ITEM_NODESTROY))
    {
        act("$p is too immutable to be overcharged.", ch, obj, NULL, TO_CHAR);
        return false;
    }

    // Determine odds of success
    int chance(get_skill(ch, sn) - 5);
    chance = UMAX(chance, 75);

    AFFECT_DATA * paf(get_obj_affect(obj, sn));
    if (paf != NULL)
        chance -= (40 + (paf->modifier * 10));

    chance = URANGE(5, chance, 95);

    // Attempt the overcharge
    if (number_percent() <= chance)
    {
        // Success
        act("A spark of crackling lightning leaps from your fingertips, charging up $p!", ch, obj, NULL, TO_CHAR);
        act("A spark of crackling lightning leaps from $n's fingertips into $p!", ch, obj, NULL, TO_ROOM);

        // Add a level and a charge
        ++obj->value[0]; // Level
        ++obj->value[2]; // Charges Left
        obj->value[1] = UMAX(obj->value[1], obj->value[2]); // Charges total

        // Keep track of how many times this wand has been overcharged
        if (paf == NULL)
        {
            AFFECT_DATA af = {0};
            af.where    = TO_AFFECTS;
            af.level    = level;
            af.type     = sn;
            af.duration = -1;
            af.modifier = 1;
            affect_to_obj(obj, &af);
        }
        else
            ++paf->modifier;

        return true;
    }

    // Failure; melt the wand
    act("You lose control of the spell, sending a bolt of white-hot lightning into $p!", ch, obj, NULL, TO_CHAR);
    act("A bolt of white-hot lightning leaps from $n's outstretched hand directly into $p!", ch, obj, NULL, TO_ROOM);
    act("With a burst of sparks and static, $p is fused into worthlessness!", ch, obj, NULL, TO_ALL);
    extract_obj(obj);
    return true;
}

void check_clingingfog(CHAR_DATA * ch)
{
    // Check clinging fog
    AFFECT_DATA * paf;
    if (!IS_IMMORTAL(ch) && !IS_AFFECTED(ch, AFF_WIZI) && (paf = affect_find(ch->in_room->affected, gsn_clingingfog)) != NULL)
    {
        if (!is_affected(ch, gsn_mistralward) && !is_affected(ch, gsn_clingingfog) && !saves_spell(paf->level, NULL, ch, DAM_OTHER))
        {
            AFFECT_DATA af = {0};
            af.where    = TO_AFFECTS;
            af.type     = gsn_clingingfog;
            af.level    = paf->level;
            af.duration = 1 + number_range(0, paf->level / 25);
            af.bitvector = AFF_FAERIE_FIRE;
            affect_to_char(ch, &af);

            bool anyStripped(false);
            if (is_affected(ch, gsn_invis))         {affect_strip(ch, gsn_invis); anyStripped = true;}
            if (is_affected(ch, gsn_mass_invis))    {affect_strip(ch, gsn_mass_invis); anyStripped = true;}
            if (is_affected(ch, gsn_sneak))         {affect_strip(ch, gsn_sneak); anyStripped = true;}
            if (is_affected(ch, gsn_wildmove))      {affect_strip(ch, gsn_wildmove); anyStripped = true;}
            if (is_affected(ch, gsn_shadowmastery)) {affect_strip(ch, gsn_shadowmastery); anyStripped = true;}
            if (is_affected(ch, gsn_camouflage))    {affect_strip(ch, gsn_camouflage); anyStripped = true;}

            if (IS_OAFFECTED(ch, AFF_SHADOWMASTERY))    {REMOVE_BIT(ch->oaffected_by, AFF_SHADOWMASTERY); anyStripped = true;}
            if (IS_AFFECTED(ch, AFF_HIDE))              {REMOVE_BIT(ch->affected_by, AFF_HIDE); anyStripped = true;}

            REMOVE_BIT(ch->affected_by, AFF_INVISIBLE);
            REMOVE_BIT(ch->affected_by, AFF_SNEAK);

            if (anyStripped)
            {
                send_to_char("The thick, sparkling fog clings to you, revealing your presence!\n", ch);
                act("$n is revealed by the sparkling fog!", ch, NULL, NULL, TO_ROOM);
            }
            else
                send_to_char("The thick, sparkling fog clings to you!\n", ch);
        }
    }
}

bool spell_clingingfog(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not ready to summon another clinging fog yet.\n", ch);
        return false;
    }

    // Sanity-check
    if (ch->in_room == NULL || ch->in_room->sector_type == SECT_UNDERWATER)
    {
        send_to_char("You cannot summon a clinging fog here.\n", ch);
        return false;
    }

    // Check for already present
    if (room_is_affected(ch->in_room, sn))
    {
        send_to_char("A clinging fog already permeates this place.\n", ch);
        return false;
    }

    // Echo
    act("You call to the air, which condenses into a sparkling, clinging fog!", ch, NULL, NULL, TO_CHAR);
    act("$n calls to the air, which condenses into a sparkling, clinging fog!", ch, NULL, NULL, TO_ROOM);

    // Apply cooldown
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.level    = level;
    af.type     = sn;
    af.duration = 18 - UMAX(0, (get_skill(ch, sn) - 70) / 5);
    affect_to_char(ch, &af);

    // Apply room effect
    af.where    = TO_ROOM;
    af.duration = number_range(4, 8);
    affect_to_room(ch->in_room, &af);

    // Apply to all in the room
    for (CHAR_DATA * vch(ch->in_room->people); vch != NULL; vch = vch->next_in_room)
        check_clingingfog(vch);

    return true;
}

bool spell_gralcianfunnel(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to establish another gralcian funnel.\n", ch);
        return false;
    }

    // Sanity-check
    if (ch->in_room == NULL)
    {
        send_to_char("You cannot place a gralcian funnel here.\n", ch);
        return false;
    }

    // Echo
    send_to_char("You bend the air here, funnelling the ambient sound to yourself!\n", ch);

    // Apply effect
    AFFECT_DATA af = {0};
    af.where    = TO_AREA;
    af.type     = sn;
    af.level    = level;
    af.duration = 3 + level / 3;
    af.modifier = ch->id;
    affect_to_area(ch->in_room->area, &af);

    // Apply cooldown
    af.duration = 40 - UMAX(0, (get_skill(ch, sn) - 70) / 3);
    af.where    = TO_AFFECTS;
    affect_to_char(ch, &af);
    return true;
}

static void apply_illusion_cooldown(int level, CHAR_DATA & ch, int duration)
{
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_illusion;
    af.level    = level;
    af.duration = duration;
    affect_to_char(&ch, &af);
}

static std::string make_illusion_long(const char * desc)
{
    std::string result(desc);
    result += " is here.";
    result[0] = UPPER(result[0]);
    return result;
}

static bool handle_illusion_mirror(int level, CHAR_DATA & ch, OBJ_DATA & source)
{
    // Check for wearing
    if (source.worn_on)
    {
        send_to_char("You must stop using it first.\n", &ch);
        return false;
    }

    // Check for nomirror
    if (IS_OBJ_STAT_EXTRA(&source, ITEM_NOMIRROR))
    {
        act("Your magics fizzle and fail against $p.", &ch, &source, NULL, TO_CHAR);
        return false;
    }

    // Make the base obj
    OBJ_DATA * obj(make_illusionary_object(source, 0));
    if (obj == NULL)
    {
        bug("Mirror illusion failed due to bad obj creation", 0);
        send_to_char("An error has occurred, please contact the gods.\n", &ch);
        return false;
    }

    // Clone the source object
    OBJ_DATA * realObj(create_object(source.pIndexData, 0));
    if (realObj == NULL)
    {
        extract_obj(obj);
        bug("Mirror illusion failed due to bad real obj creation", 0);
        send_to_char("An error has occurred, please contact the gods.\n", &ch);
        return false;
    }
    clone_object(&source, realObj);

    // Put the cloned object into an effect on the new obj
    AFFECT_DATA af = {0};
    af.where    = TO_OBJECT;
    af.type     = gsn_illusion;
    af.level    = level;
    af.duration = -1;
    af.point    = realObj;
    af.modifier = current_time + (number_range(5, 15) * 60);
    affect_to_obj(obj, &af);
    obj_to_char(obj, &ch);

    // Put a crumble timer on the source obj, and make it not sellable
    af.point    = NULL;
    affect_to_obj(&source, &af);
    source.cost = 0;

    act("You spin strands of illusion together, gradually forming them into a mirror image of $p!", &ch, &source, NULL, TO_CHAR);
    apply_illusion_cooldown(level, ch, 24 - UMAX(0, (get_skill(&ch, gsn_illusion) - 70) / 5));
    return true;
}

static bool handle_illusion_craft(int level, CHAR_DATA & ch, const char * desc)
{
    // Make the base obj
    OBJ_DATA * obj(create_object(get_obj_index(OBJ_VNUM_CRAFTILLUSION), 0));
    if (obj == NULL)
    {
        bug("Craft illusion failed due to bad obj creation", 0);
        send_to_char("An error has occurred, please contact the gods.\n", &ch);
        return false;
    }

    // Set up the obj
    std::string longDesc(make_illusion_long(desc));
    setName(*obj, desc);
    copy_string(obj->short_descr, desc);
    copy_string(obj->description, longDesc.c_str());
    obj->level = level;
    obj->timer = level / 2;
    obj_to_char(obj, &ch);

    act("You bind wisps of color and hue together, spinning them into $p!", &ch, obj, NULL, TO_CHAR); 
    apply_illusion_cooldown(level, ch, 8 - UMAX(0, (get_skill(&ch, gsn_illusion) - 70) / 10));
    return true;
}

static bool handle_illusion_weave(int level, CHAR_DATA & ch, const char * desc)
{
    // Make the base mob
    CHAR_DATA * mob(create_mobile(get_mob_index(MOB_VNUM_WEAVEILLUSION)));
    if (mob == NULL)
    {
        bug("Weave illusion failed due to bad mob creation", 0);
        send_to_char("An error has occurred, please contact the gods.\n", &ch);
        return false;
    }

    // Set up the mob
    std::string longDesc(make_illusion_long(desc) + '\n');
    setName(*mob, desc);
    copy_string(mob->short_descr, desc);
    copy_string(mob->long_descr, longDesc.c_str());
    copy_string(mob->description, longDesc.c_str());
    setFakeName(*mob, mob->name);
    mob->alignment = 0;
    mob->max_hit = 2;
    mob->hit = mob->max_hit;
    char_to_room(mob, ch.in_room);

    // Apply effect to make mob eventually crumble
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.level    = level;
    af.type     = gsn_illusion;
    af.duration = level / 2;
    affect_to_char(mob, &af);

    // Echoes
    act("You weave strips of chaos and illusion together, giving them form and shape!", &ch, NULL, NULL, TO_CHAR);
    act("$n appears in a swirl of light and color!", mob, NULL, NULL, TO_ROOM);
    apply_illusion_cooldown(level, ch, 8 - UMAX(0, (get_skill(&ch, gsn_illusion) - 70) / 10));
    return true;
}

bool spell_illusion(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not ready to bind together another illusion yet.\n", ch);
        return false;
    }

    // Sanity check
    if (ch->in_room == NULL)
    {
        send_to_char("You cannot make an illusion in this place.\n", ch);
        return false;
    }

    // Break down the arguments
    char arg0[MAX_INPUT_LENGTH];
    char * argument(one_argument(target_name, arg0));

    if (arg0[0] == '\0')
    {
        send_to_char("What sort of illusion did you wish to bind?\n", ch);
        return false;
    }

    if (argument[0] == '\0')
    {
        send_to_char("How should the illusion look?\n", ch);
        return false;
    }

    // Check for mirror argument
    if (!str_prefix(arg0, "mirror"))
    {
        OBJ_DATA * source(get_obj_carry(ch, argument, ch));
        if (source == NULL)
        {
            send_to_char("You are carrying nothing by that name.\n", ch);
            return false;
        }

        return handle_illusion_mirror(level, *ch, *source);
    }

    // Check for type
    if (!str_prefix(arg0, "craft")) return handle_illusion_craft(level, *ch, argument);
    if (!str_prefix(arg0, "weave")) return handle_illusion_weave(level, *ch, argument);

    send_to_char("You only know how to {Wweave{x illusions of people and animals, {Wcraft{x illusory objects, or {Wmirror{x existing objects.\n", ch);
    return false;
}

bool spell_borrowluck(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for effect already present
    if (is_affected(ch, sn))
    {
        send_to_char("You cannot shift the flows of fate again just yet.\n", ch);
        return false;
    }

    // Echoes
    send_to_char("You shift the flows of fate, borrowing good fortune from your future self.\n", ch);

    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 20 + (level / 5);
    af.location = APPLY_LUCK;
    af.modifier = dice(3, 23);
    affect_to_char(ch, &af);

    return true;
}

static void generate_efreet(int level, ROOM_INDEX_DATA & room)
{
    CHAR_DATA * efreet(create_mobile(get_mob_index(MOB_VNUM_AIR_EFREET)));
    efreet->level = number_range(level / 2, level);
    efreet->damroll = number_range(level / 5, level / 3);
    efreet->hitroll = number_range(level / 4, level / 2);
    efreet->damage[0] = efreet->damroll;
    efreet->damage[1] = 2;
    efreet->damage[2] = 3;
    efreet->max_hit = dice(level, 27);
    efreet->hit = efreet->max_hit;

    char_to_room(efreet, &room);
    act("An air efreeti suddenly coalesces from nothing!", efreet, NULL, NULL, TO_ROOM);
}

bool spell_conjureairefreet(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You cannot call upon the forces of chaos again just yet.\n", ch);
        return false;
    }

    // Sanity-check
    if (ch->in_room == NULL)
    {
        send_to_char("You cannot do that here.\n", ch);
        return false;
    }

    // Echo and apply cooldown
    send_to_char("You tear open a passage to the plane of air, calling upon the powers of chaos to invade this place!\n", ch);
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = number_range(90, 130) - UMAX(0, (get_skill(ch, sn) - 70) / 3);
    affect_to_char(ch, &af);

    // Add at least one efreeti to the caster's room
    generate_efreet(level, *ch->in_room);

    // Spawn efreet over the area
    for (const VNUM_RANGE * vnums(ch->in_room->area->vnums); vnums != NULL; vnums = vnums->next)
    {
        for (int vnum(vnums->min_vnum); vnum <= vnums->max_vnum; ++vnum)
        {
            // Get the room
            ROOM_INDEX_DATA * room(get_room_index(vnum));
            if (room == NULL)
                continue;

            // Potentially spawn efreet
            int chance(5);
            while (number_percent() <= chance)
                generate_efreet(level, *room);
        }
    }

    return true;
}

bool spell_mistralward(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for already present
    if (is_affected(ch, sn))
    {
        send_to_char("You are already guarded by a mistral ward.\n", ch);
        return false;
    }

    // Check room
    if (ch->in_room == NULL || ch->in_room->sector_type == SECT_UNDERWATER)
    {
        send_to_char("Such a ward would be ineffective here.\n", ch);
        return false;
    }

    // Echoes
    act("You call forth a mistral ward to shield yourself!", ch, NULL, NULL, TO_CHAR);
    act("$n summons a mistral ward to shield $mself!", ch, NULL, NULL, TO_ROOM);

    // Apply effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = level / 9;
    affect_to_char(ch, &af);
    return true;
}

bool spell_fatesdoor(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    if (ch->in_room == NULL
    ||   IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
    ||   IS_SET(ch->in_room->room_flags, ROOM_NOGATE)
    || (ch->in_room->area->area_flags & AREA_UNCOMPLETE))
    {
        send_to_char( "You cannot call upon fate in this place.\n", ch );
        return false;
    }

    if (ch->move <= 0)
    {
        send_to_char("You are too tired to open fate's door.\n", ch);
        return false;
    }

    if (is_affected(ch, gsn_matrix))
    {
    	send_to_char("The power of the matrix prevents you from leaving this place.\n", ch);
	    return false;
    }

    if (!IS_NPC(ch) && ch->fighting)
    {
    	send_to_char("You cannot call upon fate while fighting.\n", ch);
	    return false;
    }

    ROOM_INDEX_DATA * to_room(get_random_room(ch));
    if (to_room == NULL)
    {
        send_to_char("The fates have nothing in store for you right now.\n", ch);
        return true;
    }

    send_to_char("You call open fate's door, and step through to parts unknown!\n", ch);

    CHAR_DATA * vch_next;
    for (CHAR_DATA * vch(ch->in_room->people); vch != NULL; vch = vch_next)
    {
        vch_next = vch->next_in_room;
        if (!is_same_group(ch, vch))
            continue;

    	if (vch->fighting) 
	    {
	        send_to_char("You cannot step through fate's door while fighting.\n", vch);
    	    continue;
	    }

        vch->move /= 2;
        
    	act("$n vanishes!", vch, NULL, NULL, TO_ROOM);
	    global_linked_move = TRUE;

        char_from_room(vch);
        char_to_room(vch, to_room);
        act( "$n slowly fades into existence.", vch, NULL, NULL, TO_ROOM );
        do_look(vch, "auto");
    }

    return true;
}

bool spell_mirage(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not ready to summon another mirage yet.\n", ch);
        return false;
    }

    // Sanity-check
    if (ch->in_room == NULL)
    {
        send_to_char("You cannot summon a mirage here.\n", ch);
        return false;
    }

    // Check for area cooldown
    if (area_is_affected(ch->in_room->area, sn))
    {
        send_to_char("This place is already beset by a mirage.\n", ch);
        return false;
    }

    bool endlessFacade(false);
    if (number_percent() <= get_skill(ch, gsn_endlessfacade))
    {
        check_improve(ch, NULL, gsn_endlessfacade, true, 4);
        endlessFacade = true;
    }
    else
        check_improve(ch, NULL, gsn_endlessfacade, false, 4);

    // Echo and apply area effect and cooldown
    send_to_char("You hum softly to yourself, calling forth a mirage over the area.\n", ch);

    AFFECT_DATA af = {0};
    af.where    = TO_AREA;
    af.level    = level;
    af.type     = sn;
    af.duration = (level / 8) + (endlessFacade ? 2 : 0);
    affect_to_area(ch->in_room->area, &af);

    af.where    = TO_AFFECTS;
    af.duration = number_range(60, 100) - UMAX(0, (get_skill(ch, sn) - 70) / 3) - (endlessFacade ? 10 : 0);
    affect_to_char(ch, &af);

    // Send echo to descriptors
    for (DESCRIPTOR_DATA * d = descriptor_list; d != NULL; d = d->next)
    {
        // Filter out certain characters
        if (d->connected != CON_PLAYING || d->character->in_room == NULL ||  d->character->in_room->area != ch->in_room->area)
            continue;

        // Send the echo to the rest, but then filter out the safe characters from the effects
        send_to_char("A shimmering haze sweeps over this place, and suddenly the paths out of here warp and twist!\n", d->character);
    }

    return true;
}

bool spell_sparkingcloud(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not prepared to conjure another sparking cloud yet.\n", ch);
        return false;
    }

    // Check for room
    if (ch->in_room == NULL || ch->in_room->sector_type == SECT_UNDERWATER)
    {
        send_to_char("You cannot call a sparking cloud here!\n", ch);
        return false;
    }

    // Check for room cooldown
    if (room_is_affected(ch->in_room, sn))
    {
        send_to_char("There is already a sparking cloud in this place.\n", ch);
        return false;
    }

    // Echoes
    act("You spread your arms wide, calling forth a crackling cloud sparking with energy!", ch, NULL, NULL, TO_CHAR);
    act("$n spreads $s arms wide, calling forth a crackling cloud sparking with energy!", ch, NULL, NULL, TO_ROOM);

    // Apply effect
    AFFECT_DATA af = {0};
    af.where    = TO_ROOM;
    af.level    = level;
    af.type     = sn;
    af.duration = (level / 12);
    affect_to_room(ch->in_room, &af);

    // Apply cooldown
    af.duration = 12 - UMAX(0, (get_skill(ch, sn) - 60) / 10);
    affect_to_char(ch, &af);
    return true;
}

bool spell_englamour(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for already englamoured/disguised
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));
    if (is_affected(victim, sn))
    {
        if (ch == victim) send_to_char("There is already a glamour about you!\n", ch);
        else act("There is already a glamour about $N!", ch, NULL, victim, TO_CHAR);
        return false;
    }

    if (IS_OAFFECTED(victim, AFF_DISGUISE))
    {
        if (ch == victim) send_to_char("You are already disguised.\n", ch);
        else act("$N is already disguised.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Find a source target
    CHAR_DATA * source(get_char_room(ch, target_name));
    if (source == NULL)
    {
        send_to_char("You see no one here by that name to act as the source for your glamour.\n", ch);
        return false;
    }

    // Make sure source and victim are not the same
    if (victim == source)
    {
        if (ch == victim) send_to_char("You already look like yourself.\n", ch);
        else act("$N already looks like $Mself.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Cannot englamour from illusions
    if (IS_NPC(source) && source->pIndexData->vnum == MOB_VNUM_WEAVEILLUSION)
    {
        act("$N lacks sufficient substance to properly mimic $M.", ch, NULL, source, TO_CHAR);
        return false;
    }

    // Immortal check
    CHAR_DATA * immKiller(NULL);
    if (IS_IMMORTAL(victim) && ch->trust < victim->trust) immKiller = victim;
    else if (IS_IMMORTAL(source) && ch->trust < source->trust) immKiller = source;
    
    if (immKiller != NULL)
    {
        act("You strike down $N for $S hubris!", immKiller, NULL, ch, TO_CHAR);
        act("$n strikes you down for your hubris!", immKiller, NULL, ch, TO_VICT);
        act("$n strikes down $N for $S hubris!", immKiller, NULL, ch, TO_NOTVICT);

        std::ostringstream mess;
        mess << ch->name << " tried to englamour " << victim->name << " with an image of " << source->name << ".\n";
        wiznet(const_cast<char*>(mess.str().c_str()), NULL, NULL, WIZ_DEATHS, 0, 0);
        raw_kill(ch);
        return true;
    }

    // Check for aggression; non-groupmate NPCs generally do not like being englamoured (50% odds of attacking, 100% if evil)
    // However, goodies give goodies the benefit of the doubt, and attacks only happen if the victim can see the caster
    if (IS_NPC(victim) && !is_same_group(ch, victim) && can_see(victim, ch) 
    && (number_bits(1) == 0 || IS_EVIL(victim)) && (!IS_GOOD(victim) || aura_grade(ch) >= -1))
        multi_hit(victim, ch, TYPE_UNDEFINED);

    // Check for save; no echo to the target
    if (!is_same_group(ch, victim) && saves_spell(level, ch, victim, DAM_ILLUSION))
    {
        act("Strips of light and illusion form briefly about $n, then fade away.", victim, NULL, NULL, TO_ROOM);
        return true;
    }

    // Success; send echoes; no echo to the target unless also the caster
    if (ch == victim)
        act("Strips of light and illusion wrap about you, covering you with a glamour of $N!", ch, NULL, source, TO_CHAR);

    act("Strips of light and illusion wrap about $n, covering $m with a glamour of you!", victim, NULL, source, TO_VICT);
    act("Strips of light and illusion wrap about $n, covering $m with a glamour of $N!", victim, NULL, source, TO_NOTVICT);

    // Perform copy
    alter_appearance(*victim, *source, sn, level, level / 3);
    return true;
}

bool spell_runeofair(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity check
    if (ch->in_room == NULL)
    {
        send_to_char("You cannot scribe a rune of air here.\n", ch);
    	return false;
    }

    // Prepare effect
    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 2;

    // Apply the effect to groupmates in the room
    for (CHAR_DATA * vch(ch->in_room->people); vch != NULL; vch = vch->next_in_room)
    {
        if (!is_same_group(ch, vch) || is_affected(vch, gsn_runeofair))
            continue;
        
        if (vch == ch)
            act("As you finish chanting, a rune forms, hovering in your vision and hiding you from the world as long as you remain still.", ch, NULL, NULL, TO_CHAR);
        else
            act("As $n finishes chanting, a rune hovering in your vision hazes your sight, keeping you undetectable as long as you remain still.", ch, NULL, vch, TO_VICT);

        for (CHAR_DATA * seer(ch->in_room->people); seer != NULL; seer = seer->next_in_room)
        {
            if (seer != vch && can_see(seer, vch))
                act("$N slowly fades away, becoming completely invisible.", seer, NULL, vch, TO_CHAR);
        }
        
        affect_to_char(vch, &af);
    }

    return true;
}

static const char * unreal_adjective()
{
    switch (rand() % 39)
    {
        case 0: return "inverted";      case 1: return "shiny";         case 2: return "metallic";
        case 3: return "red-speckled";  case 4: return "bespectacled";  case 5: return "morose";
        case 6: return "spotted";       case 7: return "ecstatic";      case 8: return "mellow";
        case 9: return "jubilant";      case 10: return "bedeviled";    case 11: return "enormous";
        case 12: return "furry";        case 13: return "tiny";         case 14: return "clean-shaven";
        case 15: return "moustached";   case 16: return "bouncing";     case 17: return "placid";
        case 18: return "wobbly";       case 19: return "wart-covered"; case 20: return "wooden";
        case 21: return "gilded";       case 22: return "blue-tinted";  case 23: return "flaking";
        case 24: return "shadowy";      case 25: return "scaly";        case 26: return "squawking";
        case 27: return "cackling";     case 28: return "giggling";     case 29: return "male";
        case 30: return "female";       case 31: return "soft-bellied"; case 32: return "charming";
        case 33: return "ferocious";    case 34: return "tight-lipped"; case 35: return "sacred";
        case 36: return "jellied";      case 37: return "rounded";      case 38: return "tense";
    }

    return "shiny";
}

static const char * unreal_noun()
{
    switch (rand() % 39)
    {
        case 0: return "penguin";       case 1: return "balloon";           case 2: return "harpsichord";
        case 3: return "apple core";    case 4: return "human";             case 5: return "ethron";
        case 6: return "kankoran";      case 7: return "aelin";             case 8: return "alatharya";
        case 9: return "caladaran";     case 10: return "chaja";            case 11: return "srryn";
        case 12: return "shuddeni";     case 13: return "ch'taren";         case 14: return "nefortu";
        case 15: return "titan";        case 16: return "kkhilgh";          case 17: return "lich servant";
        case 18: return "seagull";      case 19: return "cow";              case 20: return "horse";
        case 21: return "wagon";        case 22: return "rainbow";          case 23: return "moon";
        case 24: return "star";         case 25: return "arrow";            case 26: return "lizard";
        case 27: return "Captain Bromrin"; case 28: return "tome";          case 29: return "fruit basket";
        case 30: return "halibut";      case 31: return "quasit";           case 32: return "musk drain";
        case 33: return "maglat";       case 34: return "shovel";           case 35: return "scented candle";
        case 36: return "albatross";    case 37: return "mandolin";         case 38: return "serenade";
    }

    return "balloon";
}

static const char * unreal_adverb()
{
    switch (rand() % 21)
    {
        case 0: return "quickly";       case 1: return "slowly";    case 2: return "leisurely";
        case 3: return "frantically";   case 4: return "merrily";   case 5: return "sadly";
        case 6: return "quirkily";      case 7: return "abashedly"; case 8: return "proudly";
        case 9: return "gracefully";    case 10: return "clumsily"; case 11: return "sneeringly";
        case 12: return "shyly";        case 13: return "calmly";   case 14: return "erratically";
        case 15: return "speedily";     case 16: return "softly";   case 17: return "jaggedly";
        case 18: return "goofily";      case 19: return "stately";  case 20: return "loudly";
    }

    return "quickly";
}

static const char * unreal_verb()
{
    switch (rand() % 24)
    {
        case 0: return "floats";    case 1: return "walks";     case 2: return "gallivants";
        case 3: return "meanders";  case 4: return "flies";     case 5: return "hops";
        case 6: return "skips";     case 7: return "gallops";   case 8: return "canters";
        case 9: return "glides";    case 10: return "shimmies"; case 11: return "dances";
        case 12: return "waltzes";  case 13: return "wanders";  case 14: return "trips";
        case 15: return "passes";   case 16: return "cavorts";  case 17: return "capers";
        case 18: return "zooms";    case 19: return "squirts";  case 20: return "bounds";
        case 21: return "slithers"; case 22: return "crawls";   case 23: return "sneaks";
    }

    return "walks";
}

std::string generate_unreal_predicate()
{
    std::ostringstream result;
    if ((rand() % 2) == 0) result << unreal_adverb() << " " << unreal_verb();
    else result << unreal_verb() << " " << unreal_adverb();
    return result.str();
}

std::string generate_unreal_entity()
{
    std::ostringstream result;
    const char * adj(unreal_adjective());

    switch (adj[0])
    {
        case 'a': case 'e': case 'i': case 'o': case 'u': result << "an "; break;
        default: result << "a "; break;
    }

    result << adj;
    if (rand() % 5 == 0)
    {
        const char * adj2(unreal_adjective());
        if (adj2 != adj)
            result << ", " << adj2;
    }

    result << " " << unreal_noun();
    return result.str();
}

bool spell_unrealincursion(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for already-affected
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));
    if (is_affected(victim, sn))
    {
        if (ch == victim) send_to_char("Your mind is already distorted by unreality.\n", ch);
        else act("$N's mind is already distorted by unreality.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Check for save
    if (saves_spell(level, ch, victim, DAM_ILLUSION))
    {
        send_to_char("Your perception warps and twists, then settles back down.\n", victim);
        if (ch != victim)
            act("$N resists your distortion of reality.", ch, NULL, victim, TO_CHAR);
        return true;
    }

    // Echoes
    send_to_char("Your perception warps and twists as reality distorts!\n", victim);
    if (ch != victim)
        act("You distort $N's reality, barraging $S senses with falsified images and sounds!", ch, NULL, victim, TO_CHAR);

    // Apply effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = (level / 6) + number_range(1, 9);
    af.location = APPLY_RESIST_ILLUSION;
    af.modifier = -(level / 17);
    affect_to_char(victim, &af);

    // Check for stripped zeal
    if (is_affected(victim, gsn_zeal))
    {
        send_to_char("You find it impossible to maintain your zeal amidst such confusion!\n", victim);
        affect_strip(victim, gsn_zeal);
    }

    return true;
}

bool spell_figmentscage(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Sanity-check
    if (ch->in_room == NULL)
    {
        send_to_char("You cannot cast that here.\n", ch);
        return false;
    }

    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to form another illusory cage.\n", ch);
        return false;
    }

    // Echoes
    send_to_char("You conjure a shimmering cage of pure illusion!\n", ch);
    act("Thick walls of shimmering energy rise up, sealing off this place!", ch, NULL, NULL, TO_ROOM);

    // Apply cooldown
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 8 - UMAX(0, (get_skill(ch, sn) - 70) / 10);
    affect_to_char(ch, &af);

    // Apply effect to room
    af.where    = TO_ROOM;
    af.duration = 2;
    affect_to_room(ch->in_room, &af);
    return true;
}

static void listPhantasmSyntax(CHAR_DATA * ch)
{
    send_to_char("Syntax:\n", ch);
    send_to_char("{Wphantasm list{x: lists known phantasms and their empowerments\n", ch);
    send_to_char("{Wphantasm forget <phantasm_name_or_number> confirm{x: removes a phantasm from the list of known phantasms\n", ch);
    send_to_char("{Wphantasm empowerments <phantasm_name_or_number>{x: lists the empowerment options for a phantasm\n", ch);

    if (IS_IMMORTAL(ch))
    {
        send_to_char("\nImmortal-only:\n", ch);
        send_to_char("{Wphantasm target <player_name> <any command above>{x: perform the specified command for the target player\n", ch);
        send_to_char("Example: 'phantasm target Jolinn list' would show you the list of Jolinn's known phantasms.\n", ch);
    }
}

static unsigned int determinePhantasmIndex(CHAR_DATA * ch, CHAR_DATA * target, const PhantasmInfo & info, const char * argument)
{
    if (is_number(argument))
    {
        // Argument is a number, read it in and adjust for 0-based vs 1-based
        int index(atoi(argument));
        --index;
        if (index < 0 || static_cast<unsigned int>(index) >= info.count())
        {
            std::ostringstream mess;
            mess << "Invalid phantasm index; must be from 1 to " << info.count() << ".\n";
            send_to_char(mess.str().c_str(), ch);
            return PhantasmInfo::NotFound;
        }

        // Number checks out
        return static_cast<unsigned int>(index);
    }

    // Not a number, so look it up by name
    unsigned int result(info.lookupIndexByName(argument));
    if (result == PhantasmInfo::NotFound)
    {
        if (target == ch) send_to_char("You have not studied any phantasm by that name.\n", ch);
        else act("$N has not studied any phantasm by that name.", ch, NULL, target, TO_CHAR);
    }

    return result;
}

static void listPhantasmList(CHAR_DATA * ch, CHAR_DATA * victim, const PhantasmInfo & info)
{
    // Echo the header
    if (victim == ch) send_to_char("You are studied in weaving the following phantasms:\n\n", ch);
    else act("$N is studied in weaving the following phantasms:\n", ch, NULL, victim, TO_CHAR);

    // Echo the list
    send_to_char(info.listPhantasms().c_str(), ch);
}

static void listPhantasmEmpowerments(CHAR_DATA * ch, CHAR_DATA * victim, const PhantasmInfo & info, const std::vector<std::string> & args)
{
    // Check for target phantasm
    if (args.size() < 2)
    {
        listPhantasmSyntax(ch);
        return;
    }

    unsigned int phantasmIndex(determinePhantasmIndex(ch, victim, info, args[1].c_str()));
    if (phantasmIndex == PhantasmInfo::NotFound)
        return;

    // List the empowerments
    send_to_char(info.listPossibleEmpowerments(victim->id, phantasmIndex).c_str(), ch);
}

static void forgetPhantasm(CHAR_DATA * ch, CHAR_DATA * victim, PhantasmInfo & info, const std::vector<std::string> & args)
{
    // Check for arguments
    if (args.size() < 3)
    {
        listPhantasmSyntax(ch);
        return;
    }

    // Get the target phantasm
    unsigned int phantasmIndex(determinePhantasmIndex(ch, victim, info, args[1].c_str()));
    if (phantasmIndex == PhantasmInfo::NotFound)
        return;

    // Make sure the confirm argument was specified and spelled out
    if (args[2] != "confirm")
    {
        send_to_char("You must specify 'confirm' fully-spelled out at the end of a forget command.\n", ch);
        send_to_char("Keep in mind that this command is irreversible; that phantasm and all its empowerments will be forever lost.\n", ch);
        return;
    }

    // Forget the phantasm
    info.forgetPhantasm(*ch, *victim, phantasmIndex);
}

void do_phantasm(CHAR_DATA * ch, char * argument)
{
    // Verify skill (or imm)
    if (!IS_IMMORTAL(ch) && get_skill(ch, gsn_phantasmalmirror) <= 0)
    {
        send_to_char("Huh?\n", ch);
        return;
    }

    // Break it down into arguments
    std::vector<std::string> args;
    while (argument[0] != '\0')
    {
        char arg[MAX_INPUT_LENGTH];
        argument = one_argument(argument, arg);
        args.push_back(arg);
    }

    // Check first argument
    if (args.empty())
    {
        listPhantasmSyntax(ch);
        return;
    }

    // Determine target
    CHAR_DATA * victim(ch);
    if (IS_IMMORTAL(ch) && !str_prefix(args[0].c_str(), "target"))
    {
        if (args.size() < 3)
        {
            listPhantasmSyntax(ch);
            return;
        }

        victim = get_char_world(ch, const_cast<char*>(args[1].c_str()));
        if (victim == NULL)
        {
            send_to_char("You see no one by that name.\n", ch);
            return;
        }

        args.erase(args.begin(), args.begin() + 2);
    }

    // Verify the target knows some phantasms
    if (IS_NPC(victim) || victim->pcdata->phantasmInfo == NULL || victim->pcdata->phantasmInfo->count() == 0)
    {
        if (victim == ch) send_to_char("You have not studied the weaving of any phantasms yet.\n", ch);
        else act("$N has not studied the weaving of any phantasms.", ch, NULL, victim, TO_CHAR);
        return;
    }
    PhantasmInfo & info(*victim->pcdata->phantasmInfo);

    // Now determine the action
    if (!str_prefix(args[0].c_str(), "list"))
    {
        listPhantasmList(ch, victim, info);
        return;
    }

    if (!str_prefix(args[0].c_str(), "empowerments"))
    {
        listPhantasmEmpowerments(ch, victim, info, args);
        return;
    }

    if (!str_prefix(args[0].c_str(), "forget"))
    {
        forgetPhantasm(ch, victim, info, args);
        return;
    }

    // Unknown action
    listPhantasmSyntax(ch);
    return;
}

void finishLearningPhantasm(CHAR_DATA * ch, AFFECT_DATA * paf)
{
    // Check for success or failure
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(paf->point));
    if (paf->modifier < 3 || victim == NULL || !verify_char_world(victim))
        send_to_char("The magics of your phantasmal mirror dissipate fruitlessly.\n", ch);
    else
    {
        // Learn the phantasm
        act("You lift your web of phantasmal magic from $N, satisfied in your ability to mirror $M!", ch, NULL, victim, TO_CHAR);
        if (ch->pcdata->phantasmInfo == NULL)
            ch->pcdata->phantasmInfo = new PhantasmInfo;

        ch->pcdata->phantasmInfo->learnPhantasm(ch->id, victim->pIndexData->vnum, paf->modifier - 3);
    }

    // Reset the effect
    paf->modifier = 0;
    paf->point = NULL;
}

static bool handleCopyPhantasm(int sn, int level, CHAR_DATA * ch, char * argument)
{
    static const unsigned int MaxPhantasms = 8;

    // Check the argument
    if (argument[0] == '\0')
    {
        send_to_char("Whom did you wish to copy?\n", ch);
        return false;
    }

    // Get the target
    CHAR_DATA * victim(get_char_room(ch, argument));
    if (victim == NULL)
    {
        send_to_char("You see no one by that name here.\n", ch);
        return false;
    }

    // Make sure the target is an NPC and the caster is a PC
    if (!IS_NPC(victim) || IS_NPC(ch))
    {
        act("You will never be able to make a convincing illusion of $N.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Check against current state
    if (ch->pcdata->phantasmInfo != NULL)
    {
        // Make sure the caster doesn't already know this one
        if (ch->pcdata->phantasmInfo->lookupIndexByVnum(victim->pIndexData->vnum) != PhantasmInfo::NotFound)
        {
            act("You are already studied in making phantasms of $N.", ch, NULL, victim, TO_CHAR);
            return false;
        }

        // Make sure the caster isn't at the limit
        if (ch->pcdata->phantasmInfo->count() >= MaxPhantasms)
        {
            send_to_char("You could not possibly remember any more phantasms in sufficient detail.\n", ch);
            return false;
        }
    }

    // Check for a save
    if (saves_spell(level, ch, victim, DAM_ILLUSION))
        act("You try to weave a phantasmal mirror about $N, but $E resists your magic!", ch, NULL, victim, TO_CHAR);
    else
    {
        act("You weave a phantasmal mirror about $N, trying to burn $S image into your mind!", ch, NULL, victim, TO_CHAR);

        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.type     = sn;
        af.level    = level;
        af.duration = 24 - UMAX(0, (get_skill(ch, sn) - 68) / 4);
        af.modifier = 1;
        af.point    = victim;
        affect_to_char(ch, &af);
    }

    // Repurcussions of trying this
    // Evils: always attack
    // Goodies: attack odds based on karma; eviler means more likely
    // Faction enemy: always attack regardless of align
    // Other: 50/50 chance of attacking
    bool shouldAttack(false);
    if (IS_EVIL(victim)) shouldAttack = true;
    else if (FactionTable::CurrentStanding(*ch, *victim) == Rating_Enemy) shouldAttack = true;
    else if (IS_GOOD(victim))
    {
        int auraMod(aura_grade(ch));
        if (auraMod >= 2) shouldAttack = true;
        else if (auraMod == 1 && number_bits(2) == 0) shouldAttack = true;
    }
    else if (number_bits(1) == 0) shouldAttack = true;

    if (shouldAttack)
        multi_hit(victim, ch, TYPE_UNDEFINED);

    // Adjust faction
    int multiplier(1);
    if (IS_SET(victim->nact, ACT_FACT_LEADER)) multiplier *= 15;
    if (IS_SET(victim->nact, ACT_FACT_ELITE))  multiplier *= 5;
    if (IS_SET(victim->act, ACT_BADASS))       multiplier *= 2;
    ch->pcdata->faction_standing->Change(*ch, victim->pIndexData->factionNumber, multiplier * FACTION_HIT_NORMAL, 0, 0, true);

    return true;
}

bool spell_phantasmalmirror(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to call upon the power of another phantasmal mirror.\n", ch);
        return false;
    }

    // Sanity-check
    if (ch->in_room == NULL)
    {
        send_to_char("You cannot cast that spell here.\n", ch);
        return false;
    }

    // Get the first argument
    char arg[MAX_INPUT_LENGTH];
    char * argument(one_argument(target_name, arg));
    if (arg[0] == '\0')
    {
        send_to_char("Did you wish to copy someone, or weave a phantasm from those you already know?\n", ch);
        return false;
    }

    // Handle the copy case
    if (!str_prefix(arg, "copy"))
        return handleCopyPhantasm(sn, level, ch, argument);

    // Not a copy attempt
    if (IS_NPC(ch) || ch->pcdata->phantasmInfo == NULL || ch->pcdata->phantasmInfo->count() == 0)
    {
        send_to_char("You have not studied the weaving of any phantasms yet.\n", ch);
        return false;
    }

    // Now try to find the referenced phantasm in the first argument
    PhantasmInfo * info(ch->pcdata->phantasmInfo);
    unsigned int phantasmIndex(determinePhantasmIndex(ch, ch, *info, arg));
    if (phantasmIndex == PhantasmInfo::NotFound)
        return false;

    // Make sure the caster doesn't already have one of this type
    if (info->isPhantasmHere(*ch->in_room, phantasmIndex))
    {
        send_to_char("You are maintaining such a phantasm already!\n", ch);
        return false;
    }

    // Make the phantasm
    CHAR_DATA * phantasm(info->generatePhantasm(level, phantasmIndex));
    if (phantasm == NULL)
    {
        send_to_char("An error has occurred; please contact the Immortals.\n", ch);
        return false;
    }

    // Give the phantasm to the caster
    char_to_room(phantasm, ch->in_room);
    phantasm->master = ch;
    phantasm->leader = ch;

    // Apply a cooldown unless the phantasm is Endless
    if (PhantasmInfo::traitCount(*phantasm, PhantasmTrait::Endless) == 0)
    {
        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.type     = sn;
        af.level    = level;
        af.duration = 2;
        affect_to_char(ch, &af);
    }

    // Echoes
    act("You bind wisps of shimmering light and illusion together, weaving them into the form of $N!", ch, NULL, phantasm, TO_CHAR);
    act("$n binds wisps of shimmering light and illusion together, weaving them into the form of $N!", ch, NULL, phantasm, TO_ROOM);
    return true;
}

bool spell_empowerphantasm(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Find the phantasm info for the target
    if (IS_NPC(ch) || ch->pcdata->phantasmInfo == NULL || ch->pcdata->phantasmInfo->count() == 0)
    {
        send_to_char("You have not studied the weaving of any phantasms yet.\n", ch);
        return false;
    }

    // Read off the argument
    char arg[MAX_INPUT_LENGTH];
    char * argument(one_argument(target_name, arg));
    if (argument[0] == '\0')
    {
        send_to_char("You must specify a phantasm and empowerment.\n", ch);
        return false;
    }

    // Now try to find the referenced phantasm in the first argument
    PhantasmInfo * info(ch->pcdata->phantasmInfo);
    unsigned int phantasmIndex(determinePhantasmIndex(ch, ch, *info, arg));
    if (phantasmIndex == PhantasmInfo::NotFound)
        return false;

    // Now try to empower the phantasm
    return info->empowerPhantasm(*ch, level, phantasmIndex, argument);
}

bool spell_sonicboom(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    for (AFFECT_DATA * paf(get_affect(ch, sn)); paf != NULL; paf = get_affect(ch, sn, paf))
    {
        if (paf->modifier == -1)
        {
            send_to_char("You are still building up the power to summon another sonic boom.\n", ch);
            return false;
        }
    }

    // Echoes
    act("You clap your hands together, summoning a thunderous boom!", ch, NULL, NULL, TO_CHAR);
    act("$n claps $s hands together, and a thunderous boom resounds through this place!", ch, NULL, NULL, TO_ROOM);

    // Apply cooldown
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.modifier = -1;
    af.duration = number_range(10, 16);
    affect_to_char(ch, &af);

    af.where = TO_NAFFECTS;
    af.bitvector = AFF_SUBDUE;

    // Hit people in the room
    CHAR_DATA * victim_next;
    for (CHAR_DATA * victim(ch->in_room->people); victim != NULL; victim = victim_next)
    {
        // Disqualify certain people 
        victim_next = victim->next_in_room;
        if (victim == ch || is_same_group(ch, victim) || is_safe_spell(ch, victim, true))
            continue;

        // Check for save
        int dam(dice(level, 2));
        if (saves_spell(level, ch, victim, DAM_SOUND))
            dam /= 2;
        else
        {
            // Unsaved, so full damage and apply the effect
            af.modifier = number_range(3, 12);
            af.duration = number_range(0, 1);
            affect_to_char(victim, &af);
            damage_old(ch, victim, dam, sn, DAM_SOUND, true);
            send_to_char("You are stunned by the blast!\n", victim);
            act("$n looks stunned by the blast!", victim, NULL, NULL, TO_ROOM);
        }
    }

    // Stop all fighting in the room
    for (CHAR_DATA * victim(ch->in_room->people); victim != NULL; victim = victim->next_in_room)
        stop_fighting(victim);

    return true;
}

bool spell_electricalstorm(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You cannot call forth the power of the skies again so soon.\n", ch);
        return false; 
    }

    // Outside check
    if (ch->in_room == NULL || !IS_OUTSIDE(ch))
    {
        send_to_char("You must be outside to call forth the power of the skies.\n", ch);
        return false;
    }

    // Echoes
    act("You raise your arms to the skies, and lightning flashes overhead!", ch, NULL, NULL, TO_CHAR);
    act("$n raises $s arms to the skies, and lightning flashes overhead!", ch, NULL, NULL, TO_ROOM);

    // Prepare effect
    AFFECT_DATA af = {0};
    af.where    = TO_AREA;
    af.type     = sn;
    af.level    = level;
    af.duration = (level / 10) + number_range(1, 5);
    affect_to_area(ch->in_room->area, &af);

    // Walk the alinks, looking for connections to this area
    for (ALINK_DATA * alink(alink_first); alink != NULL; alink = alink->next)
    {
        // Apply the storm to all connected areas
        af.duration = (level / 10) + number_range(1, 5);
        if (alink->a1 == ch->in_room->area) affect_to_area(alink->a2, &af);
        else if (alink->a2 == ch->in_room->area) affect_to_area(alink->a1, &af);
    }

    // Apply cooldown
    af.where    = TO_AFFECTS;
    af.duration = number_range(40, 70) - UMAX(0, (get_skill(ch, sn) - 70) / 3);
    affect_to_char(ch, &af);

    // Force a weather update
    weather_update();
    return true;
}

bool spell_ionize(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for already-affected
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));
    if (is_affected(victim, sn))
    {
        if (ch == victim) send_to_char("The air about you is already charged.\n", ch);
        else act("The air about $N is already charged.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Check for save
    if (ch != victim && saves_spell(level, ch, victim, DAM_LIGHTNING))
    {
        send_to_char("The air about you hums with energy, but quickly returns to normal.\n", victim);
        act("The air about $N hums with energy, but quickly returns to normal.", ch, NULL, victim, TO_CHAR);
        return true;
    }

    // Apply effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.level    = level;
    af.type     = sn;
    af.duration = number_range(level / 2, level);
    affect_to_char(victim, &af);

    send_to_char("The air about you hums with crackling energy!\n", victim);
    act("The air about $n hums with crackling energy!", victim, NULL, NULL, TO_ROOM);
    return true;
}

bool spell_skystrike(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to call forth the power of the skies again.\n", ch);
        return false;
    }

    // Check for outdoors
    if (ch->in_room == NULL || !IS_OUTSIDE(ch))
    {
        send_to_char("You cannot reach to the skies from this place.\n", ch);
        return false;
    }

    // Send echoes
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));
    act("You throw your head back and shout to the skies!", ch, NULL, NULL, TO_CHAR);
    act("$n throws $s head back and shouts to the skies!", ch, NULL, NULL, TO_ROOM);
    act("A jagged bolt of lightning strikes you, followed by a peal of thunder!", victim, NULL, NULL, TO_CHAR);
    act("A jagged bolt of lightning strikes $n with a thunderous boom!", victim, NULL, NULL, TO_ROOM);

    // Prepare damage
    std::vector<DamageInfo> damage(2);
    damage[0].type = DAM_LIGHTNING;
    damage[1].type = DAM_SOUND;
    
    // Cloud cover increases power of strike
    if (silver_state == SILVER_FINAL) level += 10;
    else level += (ch->in_room->area->w_cur.cloud_cover / 10);

    // Calculate final damage amounts and saves
    damage[0].amount = dice(level, 7);
    damage[1].amount = dice(level, 3);

    if (saves_spell(level, ch, victim, damage[0].type)) damage[0].amount /= 2;
    if (saves_spell(level, ch, victim, damage[1].type)) damage[1].amount /= 2;
    else WAIT_STATE(victim, UMAX(victim->wait, (3 * PULSE_VIOLENCE) / 2));

    damage_new(ch, victim, damage, sn, true);

    // Apply cooldown
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 8;
    affect_to_char(ch, &af);
    return true;
}

bool spell_joltward(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for already present
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));
    if (is_affected(victim, sn))
    {
        if (ch == victim) send_to_char("You are already surrounded by a static field.\n", ch);
        else act("$N is already surrounded by a static field.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Apply effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.level    = level;
    af.type     = sn;
    af.duration = 10;
    af.modifier = 5 + (level / 5);
    af.location = APPLY_RESIST_ENERGY;
    af.point    = reinterpret_cast<void*>(ch->id);
    affect_to_char(victim, &af);

    send_to_char("The air around you crackles with sparking energy.\n", victim);
    act("The air around $n crackles with sparking energy.", victim, NULL, NULL, TO_ROOM);
    return true;
}

void check_displacement(CHAR_DATA * ch)
{
    if (ch->in_room == NULL || ch->in_room->sector_type == SECT_UNDERWATER)
        return;

    // Check each person in the room
    CHAR_DATA * vict_next;
    for (CHAR_DATA * victim(ch->in_room->people); victim != NULL; victim = vict_next)
    {
        vict_next = victim->next_in_room;
        if (victim == ch)
            continue;
        
        // Check for a displacement effect referencing ch
        AFFECT_DATA * displacement(get_affect(victim, gsn_displacement));
        if (displacement == NULL || displacement->modifier != ch->id)
            continue;

        // Found a displacement effect, check for save (which gets easier every time the displacement happens or is saved against)
        if (IS_NPC(victim))
            check_killer(ch, victim);

        int diffMod(reinterpret_cast<int>(displacement->point));
        displacement->point = reinterpret_cast<void*>(diffMod + 1);
        
        if (saves_spell(displacement->level - diffMod, ch, victim, DAM_BASH) || is_affected(victim, gsn_anchor)
        || (IS_NPC(victim) && (IS_SET(victim->act, ACT_NOSUBDUE) || IS_SET(victim->act, ACT_SENTINEL))))
        {
            send_to_char("You feel the displacement field push at you, but stand firm against it.\n", victim);
            act("$N stands firm against the force of your displacement field.", ch, NULL, victim, TO_CHAR);
            continue;
        }

        // Did not save, check for exit
        std::vector<Direction::Value> directions(Direction::ValidDirectionsFrom(*ch->in_room));
        if (!directions.empty())
        {
            Direction::Value direction(directions[number_range(0, directions.size() - 1)]);
            ROOM_INDEX_DATA * nextRoom(Direction::Adjacent(*ch->in_room, direction, EX_CLOSED|EX_WALLED|EX_WEAVEWALL|EX_ICEWALL));
            if (nextRoom != NULL)
            {
                // Launch 'em
                std::ostringstream mess;
                mess << "The displacement field shoves you away from $N, sending you flying " << Direction::NameFor(direction) << "!";
                act(mess.str().c_str(), victim, NULL, ch, TO_CHAR);

                mess.str("");
                mess << "The force of your displacement field launches $N " << Direction::DirectionalNameFor(direction) << "!";
                act(mess.str().c_str(), ch, NULL, victim, TO_CHAR);

                mess.str("");
                mess << "$N is suddenly launched backwards " << Direction::DirectionalNameFor(direction) << "!";
                act(mess.str().c_str(), ch, NULL, victim, TO_NOTVICT);

                char_from_room(victim);
                char_to_room(victim, nextRoom);
                do_look(victim, "auto");
                act("$n comes flying in, tumbling head over heels!", victim, NULL, NULL, TO_ROOM);

                WAIT_STATE(victim, UMAX(victim->wait, PULSE_VIOLENCE));
                if (!is_flying(victim))
                    damage_old(victim, victim, number_range(40, 65), gsn_displacement, DAM_BASH, true);

                continue;
            }
        }

        // Nowhere to launch, so just damage and lag the target here
        std::ostringstream mess;
        mess << "The displacement field shoves you away from $N, sending you flying!";
        act(mess.str().c_str(), victim, NULL, ch, TO_CHAR);

        mess.str("");
        mess << "The force of your displacement field sends $N flying!";
        act(mess.str().c_str(), ch, NULL, victim, TO_CHAR);

        mess.str("");
        mess << "$N is suddenly launched backwards through the air!";
        act(mess.str().c_str(), ch, NULL, victim, TO_NOTVICT);

        WAIT_STATE(victim, UMAX(victim->wait, PULSE_VIOLENCE / 2));
        if (!is_flying(victim))
            damage_old(victim, victim, number_range(25, 50), gsn_displacement, DAM_BASH, true);
    }
}

bool spell_displacement(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for effect already present on target
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));
    if (is_affected(victim, sn))
    {
        act("$n is already charged with a displacement field.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Check for save
    if (saves_spell(level, ch, victim, DAM_LIGHTNING))
    {
        send_to_char("You feel an electrical charge building about you, but shake it off.\n", victim);
        act("$N shakes off your attempt to charge $M with a displacement field.", ch, NULL, victim, TO_CHAR);
        return true;
    }

    act("You gesture at $N, charging $M with a displacement field.", ch, NULL, victim, TO_CHAR);
    act("$n gestures at you, and you suddenly feel a strong force of repulsion pushing at you!", ch, NULL, victim, TO_VICT);
    act("$n gestures at $N, who staggers backwards a little.", ch, NULL, victim, TO_NOTVICT);

    // Apply effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.modifier = ch->id;
    af.point    = 0;
    af.duration = 1 + (level / 10) + number_range(0, 4);
    affect_to_char(victim, &af);

    // Perform initial check
    check_displacement(ch);
    return true;
}

bool spell_typhoon(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to summon another typhoon.\n", ch);
        return false;
    }

    // Check for outside
    if (ch->in_room == NULL || !IS_OUTSIDE(ch))
    {
        send_to_char("You cannot reach the winds here.\n", ch);
        return false;
    }

    // Check for area affected
    if (area_is_affected(ch->in_room->area, sn))
    {
        send_to_char("There is already a typhoon tearing through here!\n", ch);
        return false;
    }

    // Cast the spell
    send_to_char("You reach for the winds, urging them into a frenzied typhoon!\n", ch);

    AFFECT_DATA af = {0};
    af.where    = TO_AREA;
    af.type     = sn;
    af.level    = level;
    af.duration = number_range(level / 10, level / 5);
    affect_to_area(ch->in_room->area, &af);

    // Apply a cooldown
    af.where    = TO_AFFECTS;
    af.duration = number_range(25, 45) - UMAX(0, (get_skill(ch, sn) - 70) / 3);
    affect_to_char(ch, &af);
    
    // Force a weather update
    weather_update();
    return true;
}

bool spell_unleashtwisters(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to unleash the winds once more.\n", ch);
        return false;
    }

    // Check for wind
    std::pair<Direction::Value, int> windInfo(checkWind(ch));
    if (windInfo.second <= 0)
    {
        send_to_char("There is no wind here to whip up into cyclones.\n", ch);
        return false;
    }
    
    // Check for exit points
    std::vector<Direction::Value> directions(Direction::ValidDirectionsFrom(*ch->in_room));
    if (directions.empty())
    {
        send_to_char("This place is too enclosed to start a twister.\n", ch);
        return false;
    }

    // Spell will be cast; send echoes
    act("You throw your arms wide, shouting to the skies!", ch, NULL, NULL, TO_CHAR);
    act("$n throws $s arms wide, shouting to the skies!", ch, NULL, NULL, TO_ROOM);

    // Apply cooldown
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = number_range(70, 110) - UMAX(0, (get_skill(ch, sn) - 70) / 3);
    affect_to_char(ch, &af);

    // Apply room effects, choosing initial directions for the twister
    unsigned int twisterCount(2 + number_range(0, level / 17));
    af.where    = TO_ROOM;
    for (unsigned int i(0); i < twisterCount; ++i)
    {
        af.duration = 1 + number_range(level / 17, level / 5);
        af.modifier = directions[number_range(0, directions.size() - 1)];
        affect_to_room(ch->in_room, &af);
        act("A whirling twister of air spins into existence!", ch, NULL, NULL, TO_ALL);
    }
    
    return true;
}

bool spell_breezestep(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not yet ready to dance on the breeze again.\n", ch);
        return false;
    }

    // Check for outdoors
    if (ch->in_room == NULL || !IS_OUTSIDE(ch))
    {
        send_to_char("You cannot dance upon the breeze here.\n", ch);
        return false;
    }

    // Check for flying
    if (!is_flying(ch))
    {
        send_to_char("You cannot caper on the breeze with your feet planted on the ground!\n", ch);
        return false;
    }

    // Send echoes
    send_to_char("You feel the wind fill you, guiding your movements as you flit about!\n", ch);

    // Build the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = 3 + (level / 17);
    af.modifier = 1;
    affect_to_char(ch, &af);

    return true;
}

bool spell_curseofeverchange(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for already-affected
    CHAR_DATA * victim(static_cast<CHAR_DATA*>(vo));
    if (is_affected(victim, sn))
    {
        if (ch == victim) send_to_char("You are already at fate's mercy.\n", ch);
        else act("$n is already at fate's mercy.", ch, NULL, victim, TO_CHAR);
        return false;
    }

    // Check for a save
    if (saves_spell(level, ch, victim, DAM_CHARM))
    {
        act("You feel the forces of chaos snatch at you, but you elude their grasp!", victim, NULL, NULL, TO_CHAR);
        if (ch != victim) act("You bind the forces of chaos about $N, but $E slips free!", ch, NULL, victim, TO_CHAR);
        return true;
    }

    // Apply the effect
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = number_range(level / 17, level / 8);
    affect_to_char(victim, &af);

    act("You feel the forces chaos snatch at you, ensnaring you in their grasp!", victim, NULL, NULL, TO_CHAR);
    act("You bind the forces of chaos about $N, casting $S fate to the winds!", ch, NULL, victim, TO_CHAR);
    return true;
}

static void handle_windfall_bad(int level, CHAR_DATA * ch)
{
    // Nasty surprise
    switch (number_range(0, 4))
    {
        case 0:
        {
            // Random stat/hit/dam/save debuff
            AFFECT_DATA af = {0};
            af.where    = TO_AFFECTS;
            af.type     = gsn_beckonwindfall;
            af.level    = level;
            af.location = (number_bits(0) == 0 ? number_range(APPLY_STR, APPLY_CON) : number_range(APPLY_HITROLL, APPLY_SAVES));
            af.modifier = number_range(1, 8) * (af.location == APPLY_SAVES ? 1 : -1);
            af.duration = number_range(2, 20);
            affect_to_char(ch, &af);
            send_to_char("The winds howl in response, and you feel enfeebled!\n", ch);
            act("The winds howl, and $n staggers slightly!", ch, NULL, NULL, TO_ROOM);
            break;
        }

        case 1:
        {
            // Random resist debuff
            AFFECT_DATA af = {0};
            af.where    = TO_AFFECTS;
            af.type     = gsn_beckonwindfall;
            af.level    = level;
            af.location = number_range(APPLY_RESIST_MAGIC, APPLY_RESIST_ILLUSION);
            af.modifier = number_range(1, 15) * -1;
            af.duration = number_range(2, 20);
            affect_to_char(ch, &af);
            send_to_char("The winds howl in response, and you feel enfeebled!\n", ch);
            act("The winds howl, and $n staggers slightly!", ch, NULL, NULL, TO_ROOM);
            break;
        }

        case 2:
        {
            // Random hp/mana/move debuff
            AFFECT_DATA af = {0};
            af.where    = TO_AFFECTS;
            af.type     = gsn_beckonwindfall;
            af.level    = level;
            af.location = number_range(APPLY_MANA, APPLY_MOVE);

            int * maxVal(&ch->max_hit);
            if (af.location == APPLY_MANA) maxVal = &ch->max_mana;
            else if (af.location == APPLY_MOVE) maxVal = &ch->max_move;
            af.modifier = -1 * number_range(*maxVal / 10, *maxVal / 4);

            af.duration = number_range(2, 20);
            affect_to_char(ch, &af);
            send_to_char("The winds howl in response, and you feel enfeebled!\n", ch);
            act("The winds howl, and $n staggers slightly!", ch, NULL, NULL, TO_ROOM);
            break;
        }

        case 3:
        {
            // Random attacking efreet
            CHAR_DATA * efreet = create_mobile(get_mob_index(MOB_VNUM_AIR_EFREET));
            efreet->level = level + 5;
            efreet->damroll = level / 2;
            efreet->hitroll = level / 2;
            efreet->damage[0] = 2;
            efreet->damage[1] = level * 2;
            efreet->damage[2] = 10;
            efreet->hit     = dice(level, 80);
            efreet->max_hit = efreet->hit;
            char_to_room(efreet, ch->in_room);
            send_to_char("The winds howl in response, and the menacing form of an air efreet coalesces before you!\n", ch);
            act("With a howl of wind, the menacing form of an air efreet coalesces before $n!", ch, NULL, NULL, TO_ROOM);
            efreet->tracking = ch;
            multi_hit(efreet, ch, TYPE_UNDEFINED);
            break;
        }

        default:
        {
            // No effect
            send_to_char("The winds are silent, taking no heed of your call.\n", ch);
            break;
        }
    } 
}

static int choose_windfall_wandstaff_material()
{
    int options[MAX_MATERIALS];
    unsigned int count(0);
    for (int i(0); i < MAX_MATERIALS; ++i)
    {
        if (material_table[i].charges > 0)
        {
            options[count] = i;
            ++count;
        }
    }

    if (count == 0)
        return 0;

    return options[number_range(0, count - 1)];
}

static int choose_windfall_random_effect(int sn_bits)
{
    int options[MAX_SKILL];
    unsigned int count(0);
    for (int i(0); i < MAX_SKILL; ++i)
    {
        if (IS_SET(skill_table[i].flags, sn_bits))
        {
            options[count] = i;
            ++count;
        }
    }

    if (count == 0)
        return gsn_ghost;

    return options[number_range(0, count - 1)];
}

static const char * choose_windfall_wandstaff_adjective()
{
    switch (number_range(0, 21))
    {
        case 0: return "a long";          case 1: return "a short";     case 2: return "a stout";
        case 3: return "a thin";          case 4: return "a pale";      case 5: return "a crimson";
        case 6: return "an orange";       case 7: return "a blue";      case 8: return "a green";
        case 9: return "a yellow";        case 10: return "a brown";    case 11: return "a purple";
        case 12: return "a coal-black";   case 13: return "a brittle";  case 14: return "a pitted";
        case 15: return "a sturdy";       case 16: return "a sleek";    case 17: return "a shiny";
        case 18: return "a simple";       case 19: return "an elegant"; case 20: return "a polished";
    }

    return "a plain";
}

static const char * choose_windfall_scroll_adjective()
{
    switch (number_range(0, 11))
    {
        case 0: return "a cracked and worn";    case 1: return "a crumbling aged";  case 2: return "a lengthy";
        case 3: return "a crisp new";          case 4: return "a calligraphed";    case 5: return "a finely-lettered";
        case 6: return "a tightly-wound";       case 7: return "a leather-tied";    case 8: return "a gilt-edged";
        case 9: return "a crumpled";            case 10: return "a creased";        case 11: return "a thrice-folded";
    }

    return "a";
}

static const char * choose_windfall_potion_adjective()
{
    switch (number_range(0, 23))
    {
        case 0: return "a burbling";        case 1: return "an oily";       case 2: return "a greasy";
        case 3: return "a viscous";         case 4: return "a slender";     case 5: return "a cloudy";
        case 6: return "a clear";           case 7: return "a pure";        case 8: return "a warm";
        case 9: return "a cold";            case 10: return "a hissing";    case 11: return "a sparkling";
        case 12: return "a frothy";         case 13: return "a stale";      case 14: return "a pink";
        case 15: return "a blue";           case 16: return "a green";      case 17: return "a pale";
        case 18: return "a violet";         case 19: return "a brown";      case 20: return "a foul-smelling";
        case 21: return "an aromatic";      case 22: return "a rancid";     case 23: return "an orange";
    }

    return "a";
}

static void handle_windfall_generated(int level, CHAR_DATA * ch)
{
    // Create the base object and calculate level / spell level
    OBJ_DATA * obj(create_object(get_obj_index(VNUM_ALCHEMY_PRODUCT), level));
    obj->level = ch->level;
    obj->value[0] = obj->level - 5 + number_range(0, 10);
    obj->value[0] = UMAX(1, obj->value[0]);
 
    const char * adj("?");
    const char * type("?");
    switch (number_range(0, 3))
    {
        case 0:
        {
            // Make a magic staff
            obj->item_type = ITEM_STAFF;
            obj->material = choose_windfall_wandstaff_material();
            obj->weight = number_range(30, 70);
            obj->size = 2;
            obj->value[1] = number_range(1, material_table[obj->material].charges);
            obj->value[2] = obj->value[1];
            obj->value[3] = choose_windfall_random_effect(SN_WINDFALL_DEF);
            adj = choose_windfall_wandstaff_adjective();

            switch (number_range(0, 1))
            {
                case 0: type = "staff"; break;
                default: type = "rod"; break;
            }
            break;
        }

        case 1:
        {
            // Make a wand
            obj->item_type = ITEM_WAND;
            obj->material = choose_windfall_wandstaff_material();
            obj->weight = number_range(6, 14);
            obj->size = 1;
            obj->value[1] = 1 + number_range(0, (material_table[obj->material].charges * 3) / 2);
            obj->value[2] = obj->value[1];
            obj->value[3] = choose_windfall_random_effect(SN_WINDFALL_OFF);
            adj = choose_windfall_wandstaff_adjective();

            switch (number_range(0, 5))
            {
                case 0: type = "length"; break;
                case 1: type = "stick"; break;
                default: type = "wand"; break;
            }
            break;
        }

        case 2:
        {
            // Make a scroll
            obj->item_type = ITEM_SCROLL;
            obj->material = material_lookup("parchment");
            obj->weight = number_range(1, 5);
            obj->size = 1;
            obj->value[1] = choose_windfall_random_effect(SN_WINDFALL_DEF);
            obj->value[2] = (number_bits(4) == 0 ? choose_windfall_random_effect(SN_WINDFALL_DEF|SN_WINDFALL_OFF) : -1);
            obj->value[3] = -1;
            obj->value[4] = -1;
            adj = choose_windfall_scroll_adjective();

            switch (number_range(0, 5))
            {
                case 0: type = "inscription"; break;
                case 1: type = "parchment"; break;
                default: type = "scroll"; break;
            }
            break;
        }

        default:
        {
            // Make a potion
            obj->item_type = ITEM_POTION;
            obj->material = material_lookup("glass");
            obj->weight = number_range(1, 5);
            obj->size = 1;
            obj->value[1] = choose_windfall_random_effect(SN_WINDFALL_DEF);
            obj->value[2] = (number_bits(4) == 0 ? choose_windfall_random_effect(SN_WINDFALL_DEF|SN_WINDFALL_OFF) : -1);
            obj->value[3] = -1;
            obj->value[4] = -1;
            adj = choose_windfall_potion_adjective();

            switch (number_range(0, 5))
            {
                case 0: type = "flask"; break;
                case 1: type = "jar"; break;
                case 2: type = "vial"; break;
                default: type = "potion"; break;
            }
            break;
        }
    }

    // Make the object's short and name
    std::ostringstream mess;
    mess << adj << " " << type;
    free_string(obj->short_descr);
    obj->short_descr = str_dup(mess.str().c_str());
    setName(*obj, obj->short_descr);

    // Make the object's long
    mess << " lies here.";
    free_string(obj->description);
    obj->description = str_dup(mess.str().c_str());
    obj->description[0] = UPPER(obj->description[0]);

    // Put the object in the same room
    obj_to_room(obj, ch->in_room);
    act("The winds wash over you, depositing $p at your feet.", ch, obj, NULL, TO_CHAR);
    act("The winds wash over $n, depositing $p at $s feet.", ch, obj, NULL, TO_ROOM);
}

static bool handle_windfall_object(int level, CHAR_DATA * ch)
{
    // Search the obj list for objects marked for windfall
    std::vector<OBJ_INDEX_DATA*> options;
    int matchedVnums(0);
    for (int vnum(0); matchedVnums < top_obj_index; ++vnum)
    {
        OBJ_INDEX_DATA * obj(get_obj_index(vnum));
        if (obj != NULL)
        {
            ++matchedVnums;
            if (IS_OBJ_STAT_EXTRA(obj, ITEM_WINDFALL) && obj->level <= level && (obj->limit == 0 || obj->current < obj->limit))
                options.push_back(obj);
        }
    }

    // If empty, bail out
    if (options.empty())
        return false;

    // Choose one at random to create
    OBJ_INDEX_DATA * index(options[number_range(0, options.size() - 1)]);
    OBJ_DATA * obj(create_object(index, index->level));
    if (obj == NULL)
        return false;

    // Put the object in the same room
    obj_to_room(obj, ch->in_room);
    act("The winds wash over you, depositing $p at your feet.", ch, obj, NULL, TO_CHAR);
    act("The winds wash over $n, depositing $p at $s feet.", ch, obj, NULL, TO_ROOM);
    return true;
}

static void handle_windfall_goodeffect(int level, CHAR_DATA * ch)
{
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = gsn_beckonwindfall;
    af.level    = level;
    af.duration = number_range(10, 50);

    switch (number_range(0, 5))
    {
        case 0: af.location = number_range(APPLY_STR, APPLY_CON); af.modifier = number_range(1, 8); break;
        case 1: af.location = number_range(APPLY_MANA, APPLY_MOVE); af.modifier = number_range(level, level * 5); break;
        case 2: af.location = number_range(APPLY_HITROLL, APPLY_DAMROLL); af.modifier = number_range(1, 8); break;
        case 3: af.location = APPLY_SAVES; af.modifier = -1 * number_range(1, 8); break;
        case 4: af.location = APPLY_AC; af.modifier = -1 * number_range(level, level * 5); break;
        default: af.location = number_range(APPLY_RESIST_MAGIC, APPLY_RESIST_ILLUSION); af.modifier = number_range(1, 15); break;
    }

    // Apply the effect and echo
    affect_to_char(ch, &af);
    act("The winds dance about you, leaving you feeling invigorated!", ch, NULL, NULL, TO_CHAR);
    act("The winds dance about $n, and $e appears invigorated.", ch, NULL, NULL, TO_ROOM);
}

bool spell_beckonwindfall(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    for (AFFECT_DATA * paf(get_affect(ch, sn)); paf != NULL; paf = get_affect(ch, sn, paf))
    {
        if (paf->location == APPLY_NONE && paf->modifier == ch->id)
        {
            send_to_char("The winds will not hear your request again for some time yet.\n", ch);
            return false;
        }
    }

    // Check for wind
    if (checkWind(ch).second <= 0)
    {
        send_to_char("There is no wind here to call to.\n", ch);
        return false;
    }
    
    // Apply cooldown
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.location = APPLY_NONE;
    af.modifier = ch->id;
    af.duration = number_range(20, 140) - UMAX(0, (get_skill(ch, sn) - 70) / 3);
    affect_to_char(ch, &af);

    send_to_char("You call out to the winds, seeking a bequest!\n", ch);

    // Roll for type of windfall
    int roll(number_percent());
    if (roll <= 5)
    {
        handle_windfall_bad(level, ch);
        return true;
    }

    // Check for mundane money drop
    if (roll <= 20)
    {
        if (number_bits(0) == 0) obj_to_room(create_money(1, C_PLATINUM), ch->in_room);
        if (number_bits(0) == 0) obj_to_room(create_money(number_range(1, 5), C_GOLD), ch->in_room);
        obj_to_room(create_money(number_range(1, 10), C_SILVER), ch->in_room);
        obj_to_room(create_money(number_range(2, 10), C_COPPER), ch->in_room);
        act("A few coins roll in with the breeze, settling at your feet.", ch, NULL, NULL, TO_CHAR);
        act("A few coins roll in with the breeze, settling at $n's feet.", ch, NULL, NULL, TO_ROOM);
        return true;
    }

    // Check for big money drop
    if (roll <= 25)
    {
        obj_to_room(create_money(number_range(1, 4), C_PLATINUM), ch->in_room);
        obj_to_room(create_money(number_range(1, 15), C_GOLD), ch->in_room);
        obj_to_room(create_money(number_range(10, 100), C_SILVER), ch->in_room);
        obj_to_room(create_money(number_range(50, 500), C_COPPER), ch->in_room);
        act("You are showered with coins, appearing from nowhere and falling at your feet!", ch, NULL, NULL, TO_CHAR);
        act("$n is showered with coins, appearing from nowhere and falling at $s feet!", ch, NULL, NULL, TO_ROOM);
        return true;
    }

    // Check for generated obj
    if (roll <= 50)
    {
        handle_windfall_generated(level, ch);
        return true;
    }

    // Check for random item from the obj list
    if (roll <= 75 && handle_windfall_object(level, ch))
        return true;

    // Check for permanent hp/mana/move boost
    if (roll <= 80 && number_bits(3) == 0)
    {
        int amount(number_range(2, 5));
        switch (number_range(0, 2))
        {
            case 0:
                ch->max_hit += amount;
                ch->hit += amount;
                send_to_char("The air about you warms in response, and you feel healthier!\n", ch);
                break;

            case 1:
                ch->max_mana += amount;
                ch->mana += amount;
                send_to_char("The air about you warms in response, and you feel your mind expand!\n", ch);
                break;

            default:
                ch->max_move += amount;
                ch->move += amount;
                send_to_char("The air about you warms in response, and you feel more indefatigable!\n", ch);
                break;
        }
        return true;
    }

    // Give a random positive effect
    handle_windfall_goodeffect(level, ch);
    return true;
}

bool spell_calluponwind(int sn, int level, CHAR_DATA * ch, void * vo, int target)
{
    // Check for cooldown
    AFFECT_DATA * paf(get_affect(ch, sn));
    if (paf != NULL && paf->location != APPLY_HIDE)
    {
        send_to_char("You are already calling upon the winds!\n", ch);
        return false;
    }

    // Make sure the caster is outdoors
    if (ch->in_room == NULL || !IS_OUTSIDE(ch))
    {
        send_to_char("You cannot reach the winds here.\n", ch);
        return false;
    }

    // Verify that an argument was supplied
    if (target_name[0] == '\0')
    {
        send_to_char("Upon which wind did you wish to call?\n", ch);
        return false;
    }

    // Get the argument to determine wind direction
    AFFECT_DATA af = {0};
    Direction::Value direction;
    const char * echoChar;
    const char * echoRoom;
    if (!str_prefix(target_name, Direction::NameFor(Direction::North))) 
    {
        direction = Direction::North;
        echoChar = "You speak the name of the north wind, and feel a chill wind greet your call!";
        echoRoom = "As $n speaks an arcane word, a chill wind blows over you, sending a shiver down your spine.";
    }
    else if (!str_prefix(target_name, Direction::NameFor(Direction::East))) 
    {
        direction = Direction::East;
        echoChar = "You whisper the name of the east wind, and a pestilent draft creeps in.";
        echoRoom = "You cough as a thick, pestilent breeze creeps in.";
    }
    else if (!str_prefix(target_name, Direction::NameFor(Direction::South))) 
    {
        direction = Direction::South;
        echoChar = "You call out the name of the south wind, and feel the air warm in response!";
        echoRoom = "$n calls out in an arcane tongue, and the air warms in response!";
    }
    else if (!str_prefix(target_name, Direction::NameFor(Direction::West))) 
    {
        direction = Direction::West;
        echoChar = "You sing aloud the name of the west wind, and feel a playful breeze tug at you.";
        echoRoom = "$n sings aloud an arcane word, and a playful breeze dances about $m.";
    }
    else
    {
        send_to_char("Which wind did you wish to call upon?\n", ch);
        return false;
    }

    // Make sure it wasn't the same wind as previously
    if (paf != NULL && paf->modifier == direction)
    {
        send_to_char("That wind will not heed your call again yet.\n", ch);
        return false;
    }

    // Good to go, send the echoes and add the effect after stripping the old one
    act(echoChar, ch, NULL, NULL, TO_CHAR);
    act(echoRoom, ch, NULL, NULL, TO_ROOM);
    affect_strip(ch, sn);

    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = level;
    af.duration = number_range(20, 40);
    af.modifier = direction;
    affect_to_char(ch, &af);
    return true;
}

bool spell_absorbelectricity(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    
    if (is_affected(ch, gsn_absorbelectricity))
    {
	send_to_char("You are already prepared to absorb lightning attacks.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = level/2;
    af.location	 = APPLY_NONE;
    af.modifier	 = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);
    
    send_to_char("You feel more in tune with electricity.\n\r", ch);
    return TRUE;
}

bool spell_airbubble( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *vch, *vch_next;

    if (!ch->in_room)
        return FALSE;

    AFFECT_DATA af = {0};
    af.where     = TO_PAFFECTS;
    af.type      = gsn_waterbreathing;
    af.level     = level;
    af.duration  = level;
    af.bitvector = AFF_AIRLESS;

    bool gfound(false);
    for (vch = ch->in_room->people ; vch != NULL; vch = vch_next)
    {
        vch_next = vch->next_in_room;

        if (is_same_group(ch, vch))
        {
            AFFECT_DATA * paf(get_affect(vch, gsn_waterbreathing));
            if (paf == NULL) affect_to_char(vch, &af);
            else paf->duration = UMAX(paf->duration, af.duration);
            send_to_char("As an air bubble closes around you, you feel able to breathe underwater.\n", vch );
            gfound = true;
        }
    }

    if (gfound)
        send_to_char("You expand the air bubble around your group.\n", ch);

    return true;
}

bool spell_airrune( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *vch, *vch_next;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (!ch->in_room)
	return FALSE;

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 2;
    af.modifier  = 0;
    af.location  = 0;
    af.bitvector = 0;

    for (vch = ch->in_room->people ; vch != NULL; vch = vch_next)
    {
        vch_next = vch->next_in_room;

        if (is_same_group(ch, vch)
         && !is_affected(vch, gsn_airrune))
        {
            affect_to_char( vch, &af );
            if (vch != ch)
                act("As $n finishes chanting, a rune hovering in your vision hazes your sight, keeping you undetectable as long as you remain still.", ch, NULL, vch, TO_VICT);
            else
                act("As you finish chanting, a rune forms, hovering in your vision, hiding you from the world as long as you remain still.", ch, NULL, NULL, TO_CHAR);
        }
    }

    return TRUE;
}

bool spell_alterself( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    if ((victim = get_char_world(ch, target_name)) == NULL)
    {
    	send_to_char("You can't create an illusion of no one.\n\r",ch);
	    return FALSE;
    }

    if (ch == victim)
    {
        send_to_char("Creating an illusion of yourself on top of yourself wouldn't be very interesting.\n\r", ch);
        return FALSE;
    }

    if (is_affected(ch, sn))
    {
        send_to_char("You don't feel prepared to alter yourself again.\n\r", ch);
        return FALSE;
    }

    if (IS_OAFFECTED(ch, AFF_DISGUISE))
    {
    	send_to_char("Your true appearance is already magically concealed.\n\r", ch);
	    return FALSE;
    }

    if (IS_NPC(victim))
    {
        send_to_char("They aren't distinct enough for a good alteration.\n\r", ch);
        return FALSE;
    }

    if (IS_IMMORTAL(victim) && ch->trust < victim->trust)
    {
        char buf[MAX_STRING_LENGTH];
	    act("$n strikes down $N for $S hubris.", victim, NULL, ch, TO_ROOM);
    	act("You annihilate the impudent form of $n for attempting to impersonate you.", victim, NULL, ch, TO_CHAR);
        sprintf(buf,"%s tried to alter self to %s.\n\r",ch->name, victim->name);
        log_string(buf);
	    raw_kill(ch);
        send_to_char("Don't _DO_ that.\n\r", ch);
        return TRUE;
    }

    act("You weave over youself an illusion of $N.", ch, NULL, victim, TO_CHAR);
    act("Wisps of color and hue spin around $n, who takes the aspect of $N.", ch, NULL, victim, TO_ROOM);

    alter_appearance(*ch, *victim, sn, level, level / 3);
    return TRUE;
}

bool spell_arrowgeas(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *vObj;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *victim;
    char arg[MAX_STRING_LENGTH];
    char *victname;

    victname = one_argument(target_name, arg);

    if (arg[0] == '\0')
    {
	send_to_char("What type of arrow do you wish to shoot?\n\r", ch);
	return FALSE;
    }

    if ((vObj = get_obj_carry(ch, arg, ch)) == NULL)
    {
        send_to_char("You are not carrying of that arrow type.\n\r", ch);
	return FALSE;
    }

    if (!vObj)
    {
	send_to_char("You are not carrying that arrow.\n\r", ch);
	return FALSE;
    }

    if (vObj->item_type != ITEM_ARROW)
    {
	send_to_char("Only arrows may be enchanted in this manner.\n\r", ch);
	return FALSE;
    }

    if (obj_is_affected(vObj, gsn_arrowgeas))
    {
	send_to_char("That arrow is alreay enchanted to strike another.\n\r", ch);
	return FALSE;
    }

    if (victname[0] == '\0')
    {
	send_to_char("Whom do you wish to enchant this arrow to strike?\n\r", ch);
	return FALSE;
    }

    if ((victim = get_char_world(ch, victname)) == NULL)
    {
	send_to_char("You cannot sense that target in the world.\n\r", ch);
	return FALSE;
    }

    if (IS_NPC(victim))
    {
	send_to_char("They do not possess the strength of aura to target with a geas.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_OBJECT;
    af.type	 = sn;
    af.level	 = level;
    af.duration	 = level/5;
    af.location  = APPLY_NONE;
    af.modifier  = victim->id;
    af.bitvector = 0;
    affect_to_obj(vObj, &af);

    act("You chant softly, enchanting $p to strike $N true.", ch, vObj, victim, TO_CHAR);
    act("$n chants, an $p glows softly.", ch, vObj, NULL, TO_ROOM);

    return TRUE;
}

bool spell_ball_lightning(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    CHAR_DATA *vch, *vch_next;
    char buf[MAX_STRING_LENGTH];
    int dam;

    if (!ch->in_room)
        return FALSE;

    act("You focus an explosion of ball lightning at $N!", ch, NULL, victim, TO_CHAR);
    act("$n focuses an explosion of ball lightning at $N!", ch, NULL, victim, TO_NOTVICT);
    act("$n focuses an explosion of ball lightning at you!", ch, NULL, victim, TO_VICT);

    dam = dice(level, 5) + 10;

    for (vch = ch->in_room->people; vch; vch = vch_next)
    {
        vch_next = vch->next_in_room;

        if ((vch->fighting == ch || is_same_group(victim, vch)) && !is_safe_spell(ch, vch, TRUE))
        {
            if (!IS_NPC(vch) && (!ch->fighting || !vch->fighting))
            {
                if (can_see(vch, ch))
                    sprintf(buf, "Help!  %s is attacking me!", PERS(ch, vch));
                else
                    sprintf(buf, "Help!  Someone is attacking me!");
                do_autoyell(vch, buf);
            }
            check_killer(ch, vch);

            if (saves_spell(level, ch, vch, DAM_LIGHTNING))
                damage_old(ch, vch, dam / 2, sn, DAM_LIGHTNING, TRUE);
            else
                damage_old(ch, vch, dam, sn, DAM_LIGHTNING, TRUE);

            dam = (dam * 5) / 4;
        }
    }

    return TRUE;
}

bool spell_bladebarrier(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *vch;
    bool gFound = FALSE;

    if (is_affected(ch, gsn_bladebarrier))
    {
	send_to_char("You are already surrounded by a blade barrier.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = level/5;
    af.location	 = APPLY_NONE;
    af.modifier	 = 0;
    af.bitvector = 0;
    
    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
	if (is_same_group(ch, vch))
	    if (is_affected(vch, gsn_bladebarrier))
		send_to_char("You are already surrounded in a field of blades.\n\r", ch);
	    else if (vch->fighting)
	        send_to_char("Your movement disrupts the forming blade barrier.\n\r", ch);
	    else
	    {
	        send_to_char("You are surrounded in a field of tiny spinning blades.\n\r", vch);
	        affect_to_char(vch, &af);
		if (ch != vch)
		    gFound = TRUE;
	    }

    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
	if (!is_same_group(ch, vch))
	{
	    if (gFound)
		act("A field of tiny blades begins to spin around $n and $s groupmates.", ch, NULL, vch, TO_VICT);
	    else
		act("A field of tiny blades begins to spin around $n.", ch, NULL, vch, TO_VICT);
	}

     return TRUE;
}
	
   
bool spell_blink(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, gsn_blink))
    {
	send_to_char("You are already prepared to blink if neccesary.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration	 = level/2;
    af.location  = APPLY_NONE;
    af.modifier	 = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    send_to_char("You prepare yourself to blink out of existence.\n\r", ch);

    return TRUE;
}
 


bool spell_blur(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *vch;

    if (!ch->in_room)
	return FALSE;

    if (is_affected(ch, gsn_blur))
    {
	send_to_char("Your appearance is already blurred.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = level * 2 / 5;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = 0;

    for (vch = ch->in_room->people; vch; vch = vch->next_in_room)
        if (is_same_group(ch, vch))
	{
    	    affect_to_char(vch, &af);
    	    send_to_char("Your appearance shifts and blurs.\n\r", vch);
    	    act("$n's form shifts and takes on a blurry appearance.", vch, NULL, NULL, TO_ROOM);
	}

    return TRUE;
}


bool spell_chain_lightning(int sn,int level,CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    CHAR_DATA *tmp_vict;
    bool found;
    int dam;
    AFFECT_DATA af;
    af.valid = TRUE;
    af.point = NULL;
    bool mercy_on = FALSE;

    // brazen: chain lightning just does -not- work with mercy at all. 
    // Turn it off if it's on, and turn it back on later
    if (!IS_NPC(ch) && IS_SET(ch->nact, PLR_MERCY_DEATH))
    {
        mercy_on = TRUE;
	REMOVE_BIT(ch->nact, PLR_MERCY_DEATH);
    }

    /* first strike */

    act("A lightning bolt leaps from $n's hand and arcs to $N.",
        ch,NULL,victim,TO_NOTVICT);
    act("A lightning bolt leaps from your hand and arcs to $N.",
        ch,NULL,victim,TO_CHAR);
    act("A lightning bolt leaps from $n's hand and hits you!",
        ch,NULL,victim,TO_VICT);

    af.type = gsn_chain_lightning;
    af.where = TO_AFFECTS;
    af.location = APPLY_HIDE;
    af.modifier = 0;
    af.duration = 0;
    af.level = level;
    af.bitvector = 0;
   
    CHAR_DATA * last_vict(NULL); 
    do
    {
	if (level > ch->level - 4) // must be the first hit
	    tmp_vict = victim;
	else
	{
	    found = FALSE;
	    for (tmp_vict = ch->in_room->people;tmp_vict;tmp_vict =tmp_vict->next_in_room)
	    {
	    	if (tmp_vict == last_vict
	          || is_affected(tmp_vict,gsn_chain_lightning)
              	  || is_safe_spell(ch,tmp_vict,FALSE))
		    continue;
	    	found = TRUE;
	    	break;
	    }
	    if (!found)
	    {
	    	for (tmp_vict = ch->in_room->people;tmp_vict;tmp_vict = tmp_vict->next_in_room)
		    if (is_affected(tmp_vict,gsn_chain_lightning))
		    	affect_strip(tmp_vict,gsn_chain_lightning);
	    	for (tmp_vict = ch->in_room->people;tmp_vict;tmp_vict = tmp_vict->next_in_room)
		    if (!is_safe_spell(ch,tmp_vict,FALSE)
		      && tmp_vict != last_vict)
		   	 break;
	    }
	}
	if (tmp_vict == NULL)
	    break;
	dam = dice(level,5);
	if (saves_spell(level,ch, tmp_vict,DAM_LIGHTNING))
	    dam /= 2;
	damage_old(ch,tmp_vict,dam,sn,DAM_LIGHTNING,TRUE);
	if (tmp_vict)
	    affect_to_char(tmp_vict,&af);
	last_vict = tmp_vict;
	if (ch == NULL || ch->in_room == NULL)
	    return FALSE;

	level -= 4;
    } while(level > 0);

    if (!IS_NPC(ch) && mercy_on)
        SET_BIT(ch->nact, PLR_MERCY_DEATH);
    return TRUE;
}

bool spell_screamingarrow( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    OBJ_DATA *vObj;

    if ((vObj = get_obj_carry(ch, target_name, ch)) == NULL)
    {
	send_to_char("You are not carrying that arrow.\n\r", ch);
	return FALSE;
    }

    if (vObj->item_type != ITEM_ARROW)
    {
	send_to_char("You may only charge arrows in this manner.\n\r", ch);
	return FALSE;
    }

    if (vObj->value[3] == DAM_SOUND
     || obj_is_affected(vObj, gsn_screamingarrow))
    {
	send_to_char("That arrow is already charged with energy.\n\r", ch);
	return FALSE;
    }

    act("You whisper into $p, and it begins to hum with energy.",ch,vObj,NULL,TO_CHAR);
    act("$n whispers into $p, and it begins to hum with energy.",ch,vObj,NULL,TO_ROOM);
    af.where	 = TO_OBJECT;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = level/5;
    af.location  = APPLY_NONE;
    af.modifier	 = 0;
    af.bitvector = IS_SET(vObj->extra_flags[0],ITEM_HUM) ? 0 : ITEM_HUM;
    affect_to_obj(vObj, &af);

    return TRUE;
}

bool spell_cloudkill( int sn, int level,CHAR_DATA *ch,void *vo,int target)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    AFFECT_DATA af, *paf = NULL;
af.valid = TRUE;
af.point = NULL;
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

    if (!ch->in_room)
        return FALSE;

    // Make sure the character is not underwater
    if (ch->in_room->sector_type == SECT_UNDERWATER)
    {
	send_to_char("There is no air here to poison.\n\r", ch);
	return FALSE;
    }
    
    if ((paf = affect_find(ch->affected,gsn_cloudkill)) != NULL
      && paf->bitvector == 0)
    {
        send_to_char("You cannot call another cloud of poison yet.\n\r", ch);
        return FALSE;
    }

    if (room_is_affected(ch->in_room,gsn_cloudkill))
    {
        send_to_char("There is already a thick cloud of poison hanging in the room.\n\r", ch);
        return FALSE;
    }

    level       = UMIN(level, static_cast<int>(sizeof(dam_each)/sizeof(dam_each[0])) - 1);
    level       = UMAX(0, level);

    send_to_char( "Your cloudkill poisons the room!\n\r", ch );

    if (!IS_NPC(ch) || !IS_AFFECTED(ch, AFF_WIZI))
        act( "$n raises $s arms, and green clouds mushroom up!",
        ch, NULL, NULL, TO_ROOM );

    for ( vch = ch->in_room->people; vch != NULL; vch = vch_next )
    {
        vch_next = vch->next_in_room;
        if ( vch->in_room == NULL )
            continue;
        if ( is_safe_spell(vch, ch, TRUE) )
            continue;

        if (vch!=ch && can_see(ch,vch) && !is_same_group(ch, vch))
        {
            if ((ch->fighting != vch) && !IS_NPC(vch) && vch->fighting != ch)
            {
                if (can_see(vch, ch))
                {
                    sprintf( buf, "Help!  %s is attacking me!", PERS(ch, vch));
                    do_autoyell( vch, buf );
                }
                else
                {
                    sprintf( buf, "Help! Someone is attacking me!");
                    do_autoyell( vch, buf );
                }
            }

            if (is_affected(vch, gsn_mistralward) && number_bits(1) == 0)
            {
                act("The poisonous gases spread towards you, but your mistral ward keeps them at bay!", vch, NULL, NULL, TO_CHAR);
                act("The poisonous gases spread towards $n, but a faint puff of wind blows them away!", vch, NULL, NULL, TO_ROOM);
                continue;
            }

            dam = number_range( dam_each[level] / 2, dam_each[level] * 2 );
            if ( saves_spell( level, ch, vch, DAM_POISON) )
                dam /= 2;

            dam /= 3;

            damage_old( ch, vch, saves_spell( level,ch,vch,DAM_POISON)
              ? dam / 2 : dam, sn,DAM_POISON,TRUE);

            if (!(IS_AFFECTED(vch,AFF_POISON)) &&
              !(saves_spell(level-5, ch, vch,DAM_POISON)))
            {
                af.where     = TO_AFFECTS;
                af.type      = gsn_cloudkill;
                af.level     = level-3;
                af.duration  = 2;
                af.location  = APPLY_STR;
                af.modifier  = -2;
                af.bitvector = AFF_POISON;
                affect_join( vch, &af );
                send_to_char( "Your lungs burn as the poison seeps into you.\n\r", vch );
                act("$n looks nauseated.",vch,NULL,NULL,TO_ROOM);
            }

            act("$n chokes on the poisonous gases!",vch,NULL,NULL,TO_ROOM);
            send_to_char("You choke on the poisonous gases!\n\r",vch);
            damage_old(ch,vch,saves_spell(level,ch, vch,DAM_DROWNING)
              ?dam/2:dam, sn,DAM_DROWNING,TRUE);
            if (number_bits(8) == 0)
            {
                act("$n coughs incessantly, $s tongue turning black, and collapses in a heap, dead.", vch, NULL, NULL, TO_ROOM);
                act("You cough incessantly, your tongue turning black, and collapse in a heap, dead.", vch, NULL, NULL, TO_CHAR);
  	        kill_char(vch, ch);
            }

            continue;
        }
    }

    af.where     = TO_ROOM;
    af.type      = gsn_cloudkill;
    af.level     = level;
    af.duration  = 10;
    af.location  = APPLY_HIDE;
    af.modifier  = 0;
    af.bitvector = AFF_CLOUDKILL;
    affect_to_room( ch->in_room, &af );
    
    af.where     = TO_AFFECTS;
    af.location  = 0;
    af.duration  = 22;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    return TRUE;
}

bool spell_conjureairefreeti( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
        CHAR_DATA *efreet;
        AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

        if (is_affected(ch, gsn_conjureairefreet))
        {
        send_to_char("You don't feel you can summon another air efreeti yet.\n\r", ch);
        return FALSE;
        }

        if (!ch->in_room)
                return FALSE;

        if (ch->in_room->sector_type == SECT_UNDERWATER)
        {
        send_to_char("You can't call an air efreeti from underwater!\n\r", ch);
        return FALSE;
        }

        efreet = create_mobile(get_mob_index(MOB_VNUM_AIR_EFREET));

        efreet->level = ch->level;
        efreet->damroll = (ch->level/4);
        efreet->hitroll = ((ch->level*2)/6);
        efreet->damage[0] = (ch->level/4);
        efreet->damage[1] = 2;
        efreet->damage[2] = (ch->level/15);
        efreet->hit     = ((ch->hit * 4)/5);
        efreet->max_hit         = ((ch->hit * 4)/5);

        char_to_room(efreet, ch->in_room);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 50;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
        affect_to_char(ch, &af);

        act("$n calls to the forces of the air, and an air efreeti coalesces!", ch, NULL, NULL, TO_ROOM);
        act("You call to the forces of the air, and an air efreeti coalesces!", ch, NULL, NULL, TO_CHAR);

    return TRUE;
}


bool spell_dancingsword( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *weapon;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    weapon = (OBJ_DATA *) vo;

    if (IS_SET(weapon->wear_flags, ITEM_WEAR_FLOAT))
    {
        send_to_char("That can already float on its own.\n\r", ch);
        return FALSE;
    }

    if (weapon->item_type != ITEM_WEAPON)
    {
        send_to_char("You can only make a weapon fly with that incantation.\n\r", ch);
        return FALSE;
    }

    if (get_eq_char(ch, WEAR_FLOAT) != NULL)
    {
        send_to_char("You already have an item floating around you.\n\r", ch);
        return FALSE;
    }

    af.where     = TO_OBJECT;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/5;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_obj(weapon, &af);

    SET_BIT(weapon->wear_flags, ITEM_WEAR_FLOAT);

    act("You finish the spell, and $p pulls upwards, as if to float.", ch, weapon, NULL, TO_CHAR);
    act("As $n finishes $s spell, $p seems to pull upwards, as if to fly on its own.", ch, weapon, NULL, TO_ROOM);

    equip_char(ch, weapon, WORN_FLOAT);
    oprog_wear_trigger(weapon);

    return TRUE;
}

bool spell_delusions( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (is_affected(victim, gsn_mindshell) || is_affected(victim, gsn_mindshield))
    {
        act("$N is shielded against that attack", ch, NULL, victim, TO_CHAR);
        return TRUE;
    }

    if (is_affected(victim, gsn_delusions))
    {
        send_to_char("They are already deluded.\n\r",ch);
        return FALSE;
    }

    act("You weave an illusion around $N!",ch, NULL, victim, TO_CHAR);
    act("$n spins threads of illusion around $N!",ch, NULL, victim, TO_NOTVICT);
    act("$n spins threads of magic around you, and the world seems to grow larger!",ch, NULL, victim, TO_VICT);

    if ( saves_spell( level, ch, victim,DAM_ILLUSION ) )
    {
        act("You recognize $n's illusion for what it is, and see through it.", ch, NULL, victim, TO_VICT);
		act("$N seems unaffected by your illusion.", ch, NULL, victim, TO_CHAR);
        return TRUE;
    }

    act("As the world grows, everything seems stronger and more intimidating.", victim, NULL, NULL, TO_CHAR);
    act("You sense the illusion take hold!", ch, NULL, NULL, TO_CHAR);

    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;

    // Check for endless facade
    if (number_percent() <= get_skill(ch, gsn_endlessfacade))
    {
        af.duration = level / 4;
        check_improve(ch, victim, gsn_endlessfacade, true, 2);
    }
    else
    {
        af.duration = level / 8;
        check_improve(ch, victim, gsn_endlessfacade, false, 2);
    }

    affect_to_char( victim, &af );

    return TRUE;
}

bool spell_disjunction( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *vObj;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn))
    {    
	send_to_char("You are not yet ready to imbue another arrow with the power of disjunction.\n\r", ch);
	return FALSE;
    }

    if ((vObj = get_obj_carry(ch, target_name, ch)) == NULL)
    {
	send_to_char("You are not carrying of that arrow type.\n\r", ch);
	return FALSE;
    }

    if (!vObj)
    {
	send_to_char("You are not carrying that arrow.\n\r", ch);
	return FALSE;
    }

    if (vObj->item_type != ITEM_ARROW)
    {
	send_to_char("You can only imbue an arrow with the powers of disjunction.\n\r", ch);
	return FALSE;
    }

    if (obj_is_affected(vObj, sn))
    {
	send_to_char("That arrow is already enchanted with disjunction.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_OBJECT;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = level/5;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = 0;
    affect_to_obj(vObj, &af);

    af.where	 = TO_AFFECTS;
    af.duration  = 3;
    affect_to_char(ch, &af);

    act("Chanting softly, you imbue $p with the power of disjunction.",
	ch, vObj, NULL, TO_CHAR);
    act("$n chants softly, and $p shimmers with power.", ch, vObj, NULL, TO_ROOM);

    return TRUE;
}


bool spell_diversions( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
int direction, i;
EXIT_DATA *pexit;

        if (!ch->in_room)
                return FALSE;

        if (target_name[0] == '\0')
        {
        	//send_to_char("Create a diversion exit where?\n\r", ch);
	        //return FALSE;
		i = 0;
		do
		{
			i++;
			direction = number_range(1,6);
		}while(ch->in_room->exit[direction] != NULL && i < 30);
        }
	else
	{
    if ( !str_prefix( target_name, "north" ) ) direction = 0;
        else if (!str_prefix (target_name, "east") ) direction = 1;
        else if (!str_prefix (target_name, "south") ) direction = 2;
        else if (!str_prefix (target_name, "west") ) direction = 3;
        else if (!str_prefix (target_name, "up") ) direction = 4;
        else if (!str_prefix (target_name, "down") ) direction = 5;
        else
                {
                send_to_char("That's not a valid direction.\n\r", ch);
                return FALSE;
                }
	}
        if ((pexit = ch->in_room->exit[direction]) != NULL)
                {
                send_to_char("There's already an exit there!\n\r", ch);
                return FALSE;
                }


        ch->in_room->exit[direction] = new_exit();
        ch->in_room->exit[direction]->u1.to_room = get_room_index( ch->in_room->vnum );    /* ROM OLC */
        ch->in_room->exit[direction]->orig_door = direction;
        ch->in_room->exit[direction]->rs_flags = EX_ILLUSION;
        ch->in_room->exit[direction]->exit_info = EX_ILLUSION;




        if (ch->in_room->exit[direction] == NULL || ch->in_room->exit[direction]->u1.to_room == NULL)
                send_to_char("You failed to create a illusionary exit.\n\r", ch);
        else
                send_to_char("An illusionary exit forms.\n\r", ch);

    return TRUE;
}

bool spell_faerie_fog( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *ich;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    act( "$n conjures a cloud of purple smoke.", ch, NULL, NULL, TO_ROOM);
    send_to_char( "You conjure a cloud of purple smoke.\n\r", ch );

    for ( ich = ch->in_room->people; ich != NULL; ich = ich->next_in_room )
    {
        if ((ich->invis_level > 0) 
	 || (IS_NPC(ich) && IS_AFFECTED(ich, AFF_WIZI)))
            continue;

        if ( ich == ch || saves_spell( level, ch, ich,DAM_OTHER) )
            continue;

        if (IS_AFFECTED(ich, AFF_FAERIE_FIRE))
            continue;

        affect_strip ( ich, gsn_mass_invis                 );
        affect_strip ( ich, gsn_invis                      );
        affect_strip ( ich, gsn_sneak                      );
        affect_strip ( ich, gsn_wildmove                   );
        affect_strip ( ich, gsn_shadowmastery              );
        affect_strip ( ich, gsn_camouflage		   );
        REMOVE_BIT   ( ich->affected_by, AFF_SHADOWMASTERY );
        REMOVE_BIT   ( ich->affected_by, AFF_HIDE          );
        REMOVE_BIT   ( ich->affected_by, AFF_INVISIBLE     );
        REMOVE_BIT   ( ich->affected_by, AFF_SNEAK         );
        act( "$n is revealed!", ich, NULL, NULL, TO_ROOM );
        send_to_char( "You are revealed!\n\r", ich );

        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = level;
        af.modifier  = 0;
        af.duration  = (level / 10);
        af.location  = 0;
        af.bitvector = AFF_FAERIE_FIRE;
        affect_to_char(ich, &af);
    }

    return TRUE;
}

struct DiscInfo
{
    DiscInfo(OBJ_DATA & discIn, const RoomPath & roomPathIn, int casterIDIn, int victimIDIn) :
        disc(&discIn), pathTracker(roomPathIn), casterID(casterIDIn), victimID(victimIDIn) {}

    OBJ_DATA * disc;
    RoomPathTracker pathTracker;
    int casterID;
    int victimID;
};

static std::vector<DiscInfo> s_discs;

static void remove_disc(size_t index)
{
    s_discs[index] = s_discs[s_discs.size() - 1];
    s_discs.pop_back();
}

static void return_remove_disc(size_t index)
{
    // Find the caster
    CHAR_DATA * ch(get_char_by_id_any(s_discs[index].casterID));
    if (ch == NULL || ch->in_room == NULL)
        return;

    // Found a caster
    if (s_discs[index].disc->in_room != NULL)
    {
        act("$p suddenly vanishes with an audible pop!", s_discs[index].disc->in_room->people, s_discs[index].disc, NULL, TO_ALL);
        obj_from_room(s_discs[index].disc);
    }

    // Bring the disc home
    obj_to_room(s_discs[index].disc, ch->in_room);
    act("Unable to complete delivery, $p returns to you, spinning slowly by your feet.", ch, s_discs[index].disc, NULL, TO_CHAR);
    act("$p spins in slowly, coming to rest at $n's feet.", ch, s_discs[index].disc, NULL, TO_ROOM);

    // Now remove the disc
    remove_disc(index);
}

static bool advance_disc(size_t index)
{
    // Perform initial room finding; can be invalid at this point
    ROOM_INDEX_DATA * room(NULL);
    Direction::Value direction;
    if (s_discs[index].pathTracker.HasStep())
    {
        direction = static_cast<Direction::Value>(s_discs[index].pathTracker.Step());
        room = Direction::Adjacent(*s_discs[index].disc->in_room, direction, EX_CLOSED|EX_WALLED|EX_ICEWALL);
    }

    if (room == NULL)
    {
        // Pathing is invalid, update it; start by finding the target
        CHAR_DATA * victim(get_char_by_id_any(s_discs[index].victimID));
        if (victim == NULL || victim->in_room == NULL)
            return false;

        // Find a path to the target
        RoomPath path(*s_discs[index].disc->in_room, *victim->in_room, victim, RoomPath::Infinite, EX_CLOSED|EX_WALLED|EX_ICEWALL);
        if (!path.Exists())
            return false;

        // Found a new path, establish it and do a final verification
        s_discs[index].pathTracker = RoomPathTracker(path);
        if (!s_discs[index].pathTracker.HasStep())
            return false;

        // Update the direction and room values
        direction = static_cast<Direction::Value>(s_discs[index].pathTracker.Step());
        room = Direction::Adjacent(*s_discs[index].disc->in_room, direction, EX_CLOSED|EX_WALLED|EX_ICEWALL);
        if (room == NULL)
        {
            bug("NULL adjacent room in advance_disc right after finding valid path", 0);
            return false;
        }
    }

    // Echo to the source room
    std::ostringstream mess;
    mess << "$p speeds off " << Direction::DirectionalNameFor(direction) << "!";
    act(mess.str().c_str(), s_discs[index].disc->in_room->people, s_discs[index].disc, NULL, TO_ALL);

    // Move the disc
    obj_from_room(s_discs[index].disc);
    obj_to_room(s_discs[index].disc, room);

    // Echo to the destination room
    mess.str("");
    mess << "$p spins in " << Direction::SourceNameFor(Direction::ReverseOf(direction)) << ", whirring as it scoots along!";
    act(mess.str().c_str(), s_discs[index].disc->in_room->people, s_discs[index].disc, NULL, TO_ALL);
    return true;
}

void disc_update()
{
    size_t i(0);
    while (i < s_discs.size())
    {
        // Sanity check
        if (s_discs[i].disc->in_room == NULL)
        {
            bug("Floating disc wound up in NULL room", 0);
            return_remove_disc(i);
            continue;
        }

        // Look for the target in the room
        CHAR_DATA * victim(get_char_by_id_any_room(s_discs[i].victimID, *s_discs[i].disc->in_room));
        if (victim == NULL)
        {
            // Advance the disc by one room
            if (!advance_disc(i))
            {
                return_remove_disc(i);
                continue;
            }
             
            // Look for the target in the room
            victim = get_char_by_id_any_room(s_discs[i].victimID, *s_discs[i].disc->in_room);
            if (victim == NULL)
            {
                // Did not find target yet, so just move on to the next disc
                ++i;
                continue;
            }
        }

        // Found the target; perform final echo and remove the disc from the list
        s_discs[i].disc->timer = UMIN(s_discs[i].disc->timer, 4);
        act("$p slows to a stop, coming to rest at your feet.", victim, s_discs[i].disc, NULL, TO_CHAR);
        act("$p slows to a stop, coming to rest at $n's feet.", victim, s_discs[i].disc, NULL, TO_ROOM);
        remove_disc(i);
    }
}

void do_deliver(CHAR_DATA * ch, char * argument)
{
    // Check for floating disc
    OBJ_DATA * obj(get_eq_char(ch, WEAR_FLOAT));
    if (obj == NULL || obj->pIndexData->vnum != OBJ_VNUM_DISC)
    {
        if (get_skill(ch, gsn_floatingdisc) <= 0) send_to_char("Huh?\n", ch);
        else send_to_char("You have no floating disc here.\n", ch);
        return;
    }

     // Sanity-check
    if (ch->in_room == NULL)
    {
        send_to_char("You cannot deliver things from here.\n", ch);
        return;
    }

    // Make sure it has something on it
    if (obj->contains == NULL)
    {
        act("There is nothing on $p to deliver.", ch, obj, NULL, TO_CHAR);
        return;
    }

    // Find a target
    CHAR_DATA * victim(get_char_world(ch, argument));
    if (victim == NULL)
    {
        send_to_char("You see no one by that name.\n", ch);
        return;
    }

    if (victim->in_room == NULL)
    {
        act("$p wobbles in the air, unable to find a path clear to $N.", ch, obj, victim, TO_CHAR);
        return;
    }

    // Verify a path exists
    RoomPath path(*ch->in_room, *victim->in_room, victim, RoomPath::Infinite, EX_CLOSED|EX_WALLED|EX_ICEWALL);
    if (!path.Exists())
    {
        act("$p wobbles in the air, unable to find a path clear to $N.", ch, obj, victim, TO_CHAR);
        return;
    }
    
    // Echoes
    act("With a wave of your hand and minor force of will, you send the disc off to deliver $p to $N.", ch, obj->contains, victim, TO_CHAR);
    act("$n waves $s hand, and with a whirring sound $p begins to spin!", ch, obj, NULL, TO_ROOM);

    // Put the disc in the room
    obj_from_char(obj);
    obj_to_room(obj, ch->in_room);

    // Add the disc to the discs list
    s_discs.push_back(DiscInfo(*obj, path, ch->id, victim->id));
}

void revert_floating_disc_from_furniture(OBJ_DATA & disc)
{
    disc.item_type = ITEM_CONTAINER;
    disc.value[0] = 10000;
    disc.value[1] = CONT_PUT_ON;
    disc.value[2] = 0;
    disc.value[3] = 10000;
    disc.value[4] = 0;
}

bool try_convert_floating_disc_to_furniture(CHAR_DATA & ch, OBJ_DATA & disc)
{
    // Make sure it doesn't contain anything
    if (disc.contains != NULL)
    {
        act("There is no room on $p for you!", &ch, &disc, NULL, TO_CHAR);
        return false;
    }

    // Turn it into furniture
    disc.item_type = ITEM_FURNITURE;
    disc.value[0] = 1;
    disc.value[1] = 10000;
    disc.value[2] = SLEEP_ON | SIT_ON | REST_ON | STAND_ON;
    disc.value[3] = 120;
    disc.value[4] = 120;
    return true;
}

bool spell_floating_disc(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Check for cooldown
    if (is_affected(ch, sn))
    {
        send_to_char("You are not ready to make another floating disc just yet.\n", ch);
        return false;
    }

    // Verify the slot is open
    OBJ_DATA * floating(get_eq_char(ch, WEAR_FLOAT));
    if (floating != NULL)
    {
        act("You've already got $p floating about you.", ch, floating, NULL, TO_CHAR);
        return false;
    }

    // Make the disc with a huge weight limit
    OBJ_DATA * disc(create_object(get_obj_index(OBJ_VNUM_DISC), 0));
    disc->value[0] = 10000;
    disc->value[3] = 10000;
    disc->timer    = 30;

    // Echo and give the caster the disc
    act("You twirl a finger in the air, and a floating black disc appears!", ch, NULL, NULL, TO_CHAR);
    act("$n twirls a finger in the air, and a floating black disc appears!", ch, NULL, NULL, TO_ROOM);
    obj_to_char(disc, ch);
    wear_obj(ch, disc, true, false, false);

    // Apply a cooldown
    AFFECT_DATA af = {0};
    af.where    = TO_AFFECTS;
    af.level    = level;
    af.type     = sn;
    af.duration = 30 - UMAX(0, (get_skill(ch, sn) - 70) / 3);
    affect_to_char(ch, &af);
    return true;
}

bool spell_fly( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af = {0};

    bool alreadyFlying(is_flying(victim));
    if ((alreadyFlying && IS_AFFECTED(victim, AFF_FLYING)) || is_affected(victim, sn))
    {
        if (victim == ch)
            send_to_char("You are already magically imbued with flight.\n\r",ch);
        else
            act("$N doesn't need your help to fly.", ch, NULL, victim, TO_CHAR);

        return false;
    }

    if (!victim->in_room)
        return false;

    if (flight_blocked(*ch))
        send_to_char("The bouyant magics surround you, but you cannot fly.\n", ch);
    else
    {
        af.bitvector = AFF_FLYING;
        send_to_char("Your feet rise off the ground.\n\r", victim);
        act("$n's feet rise off the ground.", victim, NULL, NULL, TO_ROOM);
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level + 3;
    affect_to_char(victim, &af);

    // NPCs who were not already flying don't want to start just because some guy cast it on them
    if (!alreadyFlying && IS_NPC(victim) && !is_same_group(victim, ch))
        do_land(victim, "");

    return true;
}

bool spell_gaseousform(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
    if (is_affected(ch, gsn_gaseousform))
    {
        send_to_char("You are already in gaseous form.\n",ch);
        return false;
    }

    AFFECT_DATA af = {0};
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level / 7;
    af.bitvector = IS_AFFECTED(ch,AFF_PASS_DOOR) ? 0 : AFF_PASS_DOOR;
    affect_to_char(ch, &af);

    send_to_char("You feel your body become more diffuse.\n", ch);
    act("$n's body becomes more diffuse.", ch, NULL, NULL, TO_ROOM);
    return true;
}

bool spell_greaterinvis( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( IS_AFFECTED(ch, AFF_INVISIBLE) )
    {
        send_to_char("You are already invisible.\n\r", ch);
        return FALSE;
    }

    if ( IS_AFFECTED(ch, AFF_FAERIE_FIRE) )
    {
        send_to_char("Invisibility can't overcome the glow.\n\r", ch);
        return FALSE;
    }

    act( "$n fades out of existence.", ch, NULL, NULL, TO_ROOM );

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level + 12;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_INVISIBLE;
    affect_to_char( ch, &af );
    send_to_char( "You fade out of existence.\n\r", ch );
    return TRUE;
}

bool spell_greaterventriloquate( int sn, int level, CHAR_DATA *ch,void *vo,int target)
{
    char arg[MAX_STRING_LENGTH];
    char buf1[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char speaker[MAX_INPUT_LENGTH];
    CHAR_DATA *vch, *victim;

    target_name = one_argument(target_name, arg);

    if ((victim = get_char_world(ch, arg)) == NULL)
        {
        send_to_char("You couldn't find any such anchor for your ventriloquism.\n\r", ch);
        return FALSE;
        }

    target_name = one_argument( target_name, speaker );

    sprintf( buf1, "{Y%s says, '%s'{x\n\r",          capitalize(speaker), target_name );
    sprintf( buf2, "Someone makes %s say, '%s'\n\r", capitalize(speaker), target_name );
    buf1[0] = UPPER(buf1[0]);

    for ( vch = victim->in_room->people; vch != NULL; vch = vch->next_in_room )
    {
        if (!is_exact_name( speaker, vch->name) && IS_AWAKE(vch))
            send_to_char((!IS_NPC(vch) && BIT_GET(vch->pcdata->traits, TRAIT_CYNIC) && number_bits(2) != 0) || saves_spell(level,ch, vch,DAM_ILLUSION) ? buf2 : buf1, vch );
    }

    return TRUE;
}

bool spell_groupteleport( int sn, int level, CHAR_DATA *ch, void *vo,int target
)
{
    CHAR_DATA *vch, *vch_next;
    ROOM_INDEX_DATA *to_room;

    if ( ch->in_room == NULL
    ||   IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
    ||   IS_SET(ch->in_room->room_flags, ROOM_NOGATE)
    || ( ch->in_room->area->area_flags & AREA_UNCOMPLETE ))
    {
        send_to_char( "You failed.\n\r", ch );
        return FALSE;
    }

    if (is_affected(ch, gsn_matrix))
    {
	send_to_char("The power of the matrix prevents you from leaving this place.\n\r", ch);
	return FALSE;
    }

    if (!IS_NPC(ch) && ch->fighting)
    {
	send_to_char("You cannot teleport while fighting.\n\r", ch);
	return FALSE;
    }

    to_room = get_random_room(ch);

    if (to_room == NULL)
    {
        send_to_char( "You failed.\n\r", ch );
        return TRUE;
    }

    for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
    {
        vch_next = vch->next_in_room;

        if (!is_same_group(ch, vch))
                continue;
	if (vch->fighting) 
	{
	    send_to_char("You cannot teleport while fighting.\n\r",vch);
	    continue;
	}
        
	act( "$n vanishes!", vch, NULL, NULL, TO_ROOM );
	global_linked_move = TRUE;
        char_from_room( vch );
        char_to_room( vch, to_room );
        act( "$n slowly fades into existence.", vch, NULL, NULL, TO_ROOM );
        do_look( vch, "auto" );
    }

    return TRUE;
}

static void gust_item_check(CHAR_DATA * victim)
{
    // Check for tearing off cloth-type equipment
    if (number_percent() > 10)
        return;

    // Choose an obj, make sure it is clothlike
    OBJ_DATA * obj;
    switch (number_range(0, 3))
    {
        case 0: obj = get_eq_char(victim, WEAR_HEAD); break;
        case 1: obj = get_eq_char(victim, WEAR_ABOUT); break;
        case 2: obj = get_eq_char(victim, WEAR_NECK_1); break;
        default: obj = get_eq_char(victim, WEAR_NECK_2); break;
    }

    if (obj == NULL || !material_table[obj->material].clothlike || IS_OBJ_STAT(obj, ITEM_NOREMOVE))
        return;

    // Determine the path to send the item along
    std::vector<std::pair<Direction::Value, ROOM_INDEX_DATA*> > rooms;
    ROOM_INDEX_DATA * room(victim->in_room);
    Direction::Value ignoreDirection(Direction::Max);
    for (unsigned int i(0); i < 8; ++i)
    {
        // Get the list of directions out of here
        std::vector<Direction::Value> directions(Direction::ValidDirectionsFrom(*room, ignoreDirection));
        if (directions.empty())
            break;

        // Get and verify the room
        Direction::Value direction(directions[number_range(0, directions.size() - 1)]);
        ROOM_INDEX_DATA * nextRoom(Direction::Adjacent(*room, direction, EX_CLOSED|EX_WALLED|EX_ICEWALL));
        if (nextRoom == NULL || nextRoom->sector_type == SECT_UNDERWATER)
            continue;

        // Advance to the next
        rooms.push_back(std::make_pair(direction, nextRoom));
        room = nextRoom;
        ignoreDirection = Direction::ReverseOf(direction);
    }

    // Actually take off the obhject
    obj_from_char(obj);
    if (rooms.empty())
    {
        // No path, just drop it in the room
        act("$p is torn off by the fierce winds, and goes flying!", victim, obj, NULL, TO_ALL);
        obj_to_room(obj, victim->in_room);
        return;
    }

    // Echo in the initial room and each room along the way
    std::ostringstream mess;
    mess << "$p is torn off by the fierce winds, and goes flying " << Direction::DirectionalNameFor(rooms[0].first) << "!";
    act(mess.str().c_str(), victim, obj, NULL, TO_ALL);

    for (size_t i(0); i + 1 < rooms.size(); ++i)
    {
        mess.str("");
        mess << "$p comes flying through here, heading " << Direction::NameFor(rooms[i + 1].first) << "!";
        act(mess.str().c_str(), rooms[i].second->people, obj, NULL, TO_ALL);
    }

    // Echo in the final room, and actually place the object there
    mess.str("");
    mess << "$p comes flying in " << Direction::SourceNameFor(Direction::ReverseOf(rooms[rooms.size() - 1].first)) << ", landing with a whumpf!";
    act(mess.str().c_str(), rooms[rooms.size() - 1].second->people, obj, NULL, TO_ALL);
    obj_to_room(obj, rooms[rooms.size() - 1].second);
}

bool spell_gust(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Grab an arg, then get the direction
    char arg[MAX_INPUT_LENGTH];
    target_name = one_argument(target_name, arg);
    if (target_name[0] == '\0')
    {
        send_to_char("Gust in which direction?\n", ch);
        return false;
    }

    // Check direction
    Direction::Value direction(Direction::ValueFor(target_name));
    if (direction == Direction::Max)
    {
        send_to_char("That's not a valid direction.\n", ch);
        return false;
    }

    ROOM_INDEX_DATA * nextRoom(Direction::Adjacent(*ch->in_room, direction, EX_CLOSED|EX_WALLED|EX_WEAVEWALL|EX_ICEWALL));
    if (nextRoom == NULL)
    {
        send_to_char("There is no clear passage in that direction!\n", ch);
        return false;
    }

    // Now look for a target
    CHAR_DATA * victim(get_char_room(ch, arg));
    if (victim != NULL)
    {
        // Echoes
        act("You shout, and a sudden flurry of winds swirls into motion!", ch, NULL, NULL, TO_CHAR);
        act("$n shouts, and a sudden flurry of winds swirls into motion!", ch, NULL, NULL, TO_ROOM);
        act("The winds hammer at $n!", victim, NULL, NULL, TO_ROOM);
        act("The winds hammer at you!", victim, NULL, NULL, TO_CHAR);

        // Check for saves and autosaves
        if ((IS_NPC(victim) && (IS_SET(victim->act, ACT_NOSUBDUE) || IS_SET(victim->act, ACT_SENTINEL) || IS_SET(victim->nact, ACT_SHADE))) 
        || is_affected(victim, gsn_anchor) || saves_spell(level, ch, victim, DAM_BASH))
            return true;

        // Did not save; check for item gusted
        gust_item_check(victim);
    
        // Now move target once, possibly twice
        move_char(victim, direction, false);
        if (number_bits(2) == 0 && victim->in_room != NULL && Direction::Adjacent(*victim->in_room, direction, EX_CLOSED|EX_WALLED|EX_WEAVEWALL|EX_ICEWALL) != NULL)
            move_char(victim, direction, false);

        return true;
    }

    // Did not find victim, so consider effects
    AFFECT_DATA * paf(NULL);
    const char * name(NULL);
    if (!str_prefix(arg, "cloud")) {name = "crackling cloud"; paf = get_room_affect(ch->in_room, gsn_sparkingcloud);}
    else if (!str_prefix(arg, "gases")) {name = "poison gas"; paf = get_room_affect(ch->in_room, gsn_cloudkill);}
    
    if (paf == NULL)
    {
        send_to_char("You see no one here by that name.\n", ch);
        return false;
    }

    // Found a room effect to move, so push it one room; start with echoes
    std::ostringstream mess;
    mess << "A gust of wind rises from you, pushing the " << name << " " << Direction::DirectionalNameFor(direction) << ".";
    act(mess.str().c_str(), ch, NULL, NULL, TO_CHAR);

    mess.str("");
    mess << "A gust of wind rises from $n, pushing the " << name << " " << Direction::DirectionalNameFor(direction) << ".";
    act(mess.str().c_str(), ch, NULL, NULL, TO_ROOM);

    mess.str("");
    mess << "A gust of wind pushes a " << name << " in " << Direction::SourceNameFor(Direction::ReverseOf(direction)) << "!";
    act(mess.str().c_str(), nextRoom->people, NULL, NULL, TO_ALL);

    // Now actually move the effect
    affect_to_room(nextRoom, paf);
    affect_remove_room(ch->in_room, paf);
    return true;
}

bool spell_illusionaryobject( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    OBJ_DATA *illusion(make_illusionary_object(*obj, level));
    obj_to_char(illusion, ch);

    act("$n chants softly, and $p forms another copy of itself.", ch, obj, NULL, TO_ROOM);
    act("You chants softly, and $p forms another copy of itself.", ch, obj, NULL, TO_CHAR);

    return TRUE;
}

bool spell_protectionfromlightning(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, sn) || (ch->imm_flags & IMM_LIGHTNING))
    {
        send_to_char("You are already protected from lightning.\n\r", ch);
        return FALSE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/4;
    af.location  = APPLY_RESIST_LIGHTNING;
    af.modifier  = 50 + (level / 2);
    af.bitvector = 0;
    affect_to_char( ch, &af );
    send_to_char( "You feel yourself become protected from the effects of lightning.\n\r", ch );
    return TRUE;
}


bool spell_lightning_bolt(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    // Handle targeting a pillar of sparks
    if (ch->in_room != NULL && target_name[0] != '\0' && !str_prefix(target_name, "pillar") && room_is_affected(ch->in_room, gsn_pillarofsparks))
    {
        handle_pillar_attacked(*ch, level);
        return true;
    }

    // Handle normal targeting
    CHAR_DATA * victim;
    if (target_name[0] == '\0')
    {
        victim = ch->fighting;
        if (victim == NULL)
        {
            send_to_char("Cast the spell on whom?\n", ch);
            return false;
        }
    }
    else
    {
        victim = get_char_room(ch, target_name);
        if (victim == NULL)
        {
            send_to_char("You see no one by that name here.\n", ch);
            return false;
        }
    }

    int dam = number_range(level, level * 3);
    int spasmOdds(50);
    if (saves_spell( level, ch, victim, DAM_LIGHTNING))
    {
        dam /= 2;
        spasmOdds = 10;
    }

    play_sound_room(ch, "spell_lightningbolt");
    damage_old( ch, victim, dam, sn, DAM_LIGHTNING ,TRUE);

    if (!IS_VALID(victim))
        return true;

    // Check for spasm
    if (!is_affected(victim, sn) && number_percent() <= spasmOdds && number_percent() > get_skill(victim, gsn_arcshield))
    {
        send_to_char("Your muscles spasm from the shock!\n", victim);
        act("$N's muscles spasm from the shock!", ch, NULL, victim, TO_CHAR);

        AFFECT_DATA af = {0};
        af.where    = TO_AFFECTS;
        af.type     = sn;
        af.level    = level;
        af.duration = 1;
        af.location = APPLY_HIDE;
        affect_to_char(victim, &af);
    }

    // Check for drainbolt
    if (number_bits(3) == 0 && ch->hit < ch->max_hit)
    {
        if (number_percent() <= get_skill(ch, gsn_drainbolt))
        {
            check_improve(ch, victim, gsn_drainbolt, true, 6);
            act("A crackle of life energy leaps from $N across the jagged bolt, restoring you!", ch, NULL, victim, TO_CHAR);
            ch->hit = UMIN(ch->max_hit, ch->hit + dam);
        }
        else
            check_improve(ch, victim, gsn_drainbolt, false, 6);
    }

    // Check for incendiary spark
    if (!is_affected(victim, gsn_ignite) && !saves_spell(ch->level, ch, victim, DAM_FIRE))
    {
        if (number_percent() <= get_skill(ch, gsn_incendiaryspark))
        {
            check_improve(ch, victim, gsn_incendiaryspark, true, 6);
            act("The white-hot bolt sparks against you, and you suddenly go up in flames!", victim, NULL, NULL, TO_CHAR);
            act("The white-hot bolt sparks against $n, and $e suddenly goes up in flames!", victim, NULL, NULL, TO_ROOM);

            AFFECT_DATA af = {0};
            af.where    = TO_AFFECTS;
            af.type     = gsn_ignite;
            af.level    = ch->level;
            af.duration = 3;
            affect_to_char(victim, &af);
        }
        else
            check_improve(ch, victim, gsn_incendiaryspark, false, 6);
    }

    return TRUE;
}

bool spell_lightningbrand( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    OBJ_DATA *vObj = (OBJ_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (vObj->item_type != ITEM_WEAPON && vObj->item_type != ITEM_ARROW)
    {
        send_to_char("Only weapons and arrows may be given a lightning brand.\n\r", ch);
        return FALSE;
    }

    if (obj_is_affected(vObj, gsn_lightningbrand)
     || (vObj->value[3] == DAM_LIGHTNING))
    {
        send_to_char("That weapon is already charged with energy.\n\r", ch);
        return FALSE;
    }

    af.where     = TO_OBJECT;
    af.type      = sn;
    af.level     = level;
    af.modifier  = 0 /*vObj->value[3]*/;
    af.location  = 0;
    af.duration  = level/3;
    af.bitvector = 0;
    af.point	 = NULL /*(char *) vObj->obj_str*/;
    affect_to_obj(vObj, &af);

//brazen: Ticket #126: affects which are removed in an atypical manner
//that do things like this cause problems
//    vObj->value[3] = DAM_LIGHTNING;
//    vObj->obj_str = str_dup("shock");

    act("$p begins to crackle with energy.", ch, vObj, NULL, TO_CHAR);
    act("$p begins to crackle with energy.", ch, vObj, NULL, TO_ROOM);

    return TRUE;
}


bool spell_lightning_storm( int sn, int level,CHAR_DATA *ch,void *vo,
int target)
{
AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
DESCRIPTOR_DATA *d;


        if (is_affected(ch, sn))
        {
        send_to_char("You are not ready to call a lightning storm again.\n\r", ch);
        return FALSE;
        }

        if (area_is_affected(ch->in_room->area, sn))
        {
        send_to_char("The sky already crackles with lightning.\n\r", ch);
        return FALSE;
        }

        if (!ch->in_room)
                return FALSE;

        if (ch->in_room->room_flags & ROOM_INDOORS || ch->in_room->sector_type == SECT_INSIDE)
        {
        send_to_char("You can't summon a lightning storm from indoors.\n\r", ch);
        return FALSE;
        }

        af.where        = TO_AREA;
        af.type = sn;
        af.level        = level;
        af.duration     = 3;
        af.location     = 0;
        af.modifier     = 0;
        af.bitvector   = 0;
        affect_to_area(ch->in_room->area, &af);

        af.duration     = 120;
        affect_to_char(ch, &af);
        for (d = descriptor_list; d != NULL; d = d->next)
        {
        if (!d->character)
                continue;
        if (!d->character->in_room)
                continue;
        if (d->character->in_room->area == ch->in_room->area)
                send_to_char("You hear the ominous crackling of a rising lightning storm.\n\r", d->character);
        }

    return TRUE;
}

bool spell_mass_flying(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af = {0};;
    CHAR_DATA *gch;

    if (!ch->in_room)
        return FALSE;

    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if ( !is_same_group( gch, ch ) || (IS_AFFECTED(gch, AFF_FLYING)&& !(is_affected(gch, gsn_gaseousform))) || is_affected(gch, sn))
            continue;

        if (flight_blocked(*gch))
        {
            send_to_char("The bouyant magics surround you, but you are not able to fly.\n", gch);
            if (ch != gch)
                act("Your bouyant magics surround $N, but $E is not able to fly.", ch, NULL, gch, TO_CHAR);

            af.bitvector = 0;
        }
        else
        {
            act( "$n's feet float off the ground.", gch, NULL, NULL, TO_ROOM);
            send_to_char( "Your feet float off the ground.\n\r", gch );
            af.bitvector = AFF_FLYING;
        }

        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = level;
        af.duration  = level + 3;
        af.location  = APPLY_NONE;
        affect_to_char( gch, &af );
    }

    return TRUE;
}

bool spell_mass_invis( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;
    CHAR_DATA *gch;

    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
        if (!is_same_group( gch, ch ))
            continue;

	if (IS_AFFECTED(gch, AFF_INVISIBLE))
	{
	    if (gch == ch)
		send_to_char("You are already invisible.\n\r", ch);
	    else
		act("$N is already invisible.", ch, NULL, gch, TO_CHAR);
	    
	    continue;
	}

	if (IS_AFFECTED(gch, AFF_FAERIE_FIRE))
        {
	    if (gch == ch)
		send_to_char("Invisibility can't overcome your glow.\n\r", ch);
	    else
		act("Invisibility can't overcome $N's glow.", ch, NULL, gch, TO_CHAR);
	    
	    continue;
        }

        act( "$n slowly fades out of existence.", gch, NULL, NULL, TO_ROOM );
        send_to_char( "You slowly fade out of existence.\n\r", gch );

        af.where     = TO_AFFECTS;
        af.type      = sn;
        af.level     = level/2;
        af.duration  = 24;
        af.location  = APPLY_NONE;
        af.modifier  = 0;
        af.bitvector = AFF_INVISIBLE;
        affect_to_char( gch, &af );
    }

    return TRUE;
}

bool spell_mirrorimage(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    MOB_INDEX_DATA *mobindex;
    CHAR_DATA *image;
    int x;
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA af,daf;
af.valid = TRUE;
af.point = NULL;
daf.valid = TRUE;
daf.point = NULL;

    if (is_affected(ch, sn))
    {
	send_to_char("You are not ready to produce more mirror images yet.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = 30;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = 0;
    affect_to_char(ch, &af);
    af.modifier = 1;
    daf.where     = TO_OAFFECTS;
    daf.type	     = gsn_doppelganger;
    daf.level = level;
    daf.duration = 30;
    daf.modifier = 0;
    daf.location = APPLY_NONE;
    daf.bitvector = AFF_DOPPEL;

    if ((mobindex = get_mob_index(MOB_VNUM_MIRROR_IMAGE)) == NULL)
    {
	send_to_char("Something seems amiss...\n\r", ch);
	return FALSE;
    }

    for (x = 0; x < ((level / 10) + 1); x++)
    {
	image = create_mobile(mobindex);

	free_string(image->long_descr);
	sprintf(buf, "%s the %s is flying here.\n\r", PERS(ch, ch), race_table[ch->race].name);
	image->long_descr = str_dup(buf);

	image->memfocus[DOPPEL_SLOT] = ch;
	
	free_string(image->short_descr);
	if (ch->short_descr[0] != '\0')
	    image->short_descr = str_dup(ch->short_descr);
	else
	    image->short_descr = str_dup(ch->name);
	free_string(image->description);
	image->description = str_dup(ch->description);
    setName(*image, ch->name);
	image->leader = ch;
	image->master = ch;

	char_to_room(image, ch->in_room);
	affect_to_char(image,&af);
	affect_to_char(image,&daf);
	act("A mirror image of $n appears in a flash of light!", image, NULL, NULL, TO_ROOM);
    }

    return TRUE;
}

bool spell_missileattraction(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(victim, gsn_missileattraction))
    {
	if (victim == ch)
	    send_to_char("You are already atttracting missile attacks.\n\r", ch);
	else
	    send_to_char("They are already attracting missile attacks.\n\r", ch);
	return FALSE;
    }

    if ((victim != ch) && saves_spell(level, ch, victim, DAM_CHARM))
    {
	send_to_char("You resist their spell.\n\r", victim);
	send_to_char("They resist your spell.\n\r", ch);
	return TRUE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration	 = level / 4 + 3;
    af.location	 = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    if (victim != ch)
        act("You bend the flows of air around $N, causing $M to attract missile fire.", ch, NULL, victim, TO_CHAR);

    send_to_char("You feel a shift in the flows of air around you.\n\r", victim);

    return TRUE;
}

bool spell_reflectiveaura( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, gsn_reflectiveaura))
    {
	send_to_char("You are already surrounded by a reflective aura.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = level/4;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    send_to_char("You are surrounded by a reflective aura.\n\r", ch);

    return TRUE;
}

bool spell_obfuscation( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
        char arg[MAX_STRING_LENGTH];
        int rnum;
        CHAR_DATA *victim;
        CHAR_DATA *vtarget;
        AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

        if (target_name[0] == '\0')
        {
        send_to_char("Who did you want to obfuscate from whom?\n\r", ch);
        return FALSE;
        }

        target_name = one_argument(target_name, arg);

        if (target_name[0] == '\0')
        {
        send_to_char("Who did you want to obfuscate from whom?\n\r", ch);
        return FALSE;
        }

        if ((victim = get_char_room(ch, arg)) == NULL)
        {
        send_to_char("You don't see them here.\n\r", ch);
        return FALSE;
        }

        if ((vtarget = get_char_world(ch, target_name)) == NULL)
        {
        send_to_char("You were unable to sense your target.\n\r", ch);
        return FALSE;
        }

    if (is_affected(victim, gsn_mindshell) || is_affected(victim,gsn_mindshield))
    {
        act("$N is shielded against that attack", ch, NULL, victim, TO_CHAR);
        return TRUE;
    }

    if (is_affected(vtarget, gsn_mindshell) || is_affected(vtarget, gsn_mindshield))
    {
        act("$N is shielded against that attack", ch, NULL, vtarget, TO_CHAR);
        return TRUE;
    }

    if ((IS_SET(victim->act, ACT_NOSUBDUE))
    || (IS_SET(victim->imm_flags,IMM_CHARM)))
    {
        act("You failed.", ch, NULL, victim, TO_CHAR);
        return TRUE;
    }

    if ((IS_SET(vtarget->act, ACT_NOSUBDUE))
    || (IS_SET(victim->imm_flags,IMM_CHARM)))
    {
        act("You failed.", ch, NULL, victim, TO_CHAR);
        return TRUE;
    }


        if (vtarget->in_room == NULL)
                return TRUE;

        if (!is_same_group(ch, vtarget) && vtarget->leader != ch)
                if (saves_spell(level + 10, ch, vtarget, DAM_ILLUSION))
                        {
                        send_to_char("One of your targets fails to succumb to the illusion.\n\r", ch);
                        return TRUE;
                        }

        if (!is_same_group(ch, victim) && victim->leader != ch)
                if (saves_spell(level + 10, ch, victim, DAM_ILLUSION))
                        {
                        send_to_char("One of your targets fails to succumb to the illusion.\n\r", ch);
                        return TRUE;
                        }

        rnum =  number_range(1, 50000);

        act("You feel a cloud of obfuscation settle over you.", victim, NULL, NULL, TO_CHAR);
        act("You feel a cloud of obfuscation settle over you.", vtarget, NULL, NULL, TO_CHAR);
        act("You feel the cloud of obfuscation settle over your targets", ch, NULL, NULL, TO_CHAR);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/12;
    af.location  = APPLY_HIDE;
    af.modifier  = rnum;
    af.bitvector = 0;
    affect_to_char( victim, &af );
    affect_to_char( vtarget, &af );

    return TRUE;
}


bool spell_phantasmalcreation( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    send_to_char("This spell has been removed.\n", ch);
    return true;
}

bool spell_rearrange( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *vict1, *vict2;
    char arg[MAX_STRING_LENGTH];
    char *desc, *fname;
    char lbuf[MAX_STRING_LENGTH];
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    target_name = one_argument(target_name, arg);

    if ((vict1 = get_char_room(ch, arg)) == NULL)
    {
	send_to_char("You don't see them here.\n\r", ch);
	return FALSE;
    }

    if ((vict2 = get_char_world(ch, target_name)) == NULL)
    {
	send_to_char("You don't sense them in the world.\n\r", ch);
	return FALSE;
    }

    if (IS_NPC(vict1))
    {
	act("$N lacks a strong enough aura to rearrange.",
	    ch, NULL, vict1, TO_CHAR);
	return FALSE;
    }

    if (IS_NPC(vict2))
    {
	act("$N lacks a strong enough aura to rearrange.",
	    ch, NULL, vict2, TO_CHAR);
	return FALSE;
    }

    if (IS_OAFFECTED(vict1, AFF_DISGUISE))
    {
	act("$N's true appearance is already masked.",
	    ch, NULL, vict1, TO_CHAR);
	return FALSE;
    }

    if (IS_OAFFECTED(vict2, AFF_DISGUISE))
    {
	act("$N's true appearance is alreay masked.",
	    ch, NULL, vict2, TO_CHAR);
	return FALSE;
    }
    
    if (vict1 == vict2)
    {
	    send_to_char("That would be pointless.\n\r", ch);
	    return FALSE;
    }

    if (IS_IMMORTAL(vict1) || IS_IMMORTAL(vict2))
    {
	send_to_char("You cannot rearrange the appearances of the divine.\n\r", ch);
	return FALSE;
    }

    if (!vict1->description || (vict1->description[0] == '\0'))
    {
	act("$N is too nondescript to rearrange.", ch, NULL, vict1, TO_CHAR);
	return FALSE;
    }

    if (!vict2->description || (vict2->description[0] == '\0'))
    {
	act("$N is too nondescript to rearrange.", ch, NULL, vict2, TO_CHAR);
	return FALSE;
    }

    act("Wisps of color swirl around you, as you take the aspect of $N!", vict1, NULL, vict2, TO_CHAR);
    act("Wisps of color swirl around $n, as $e takes the aspect of $N!", vict1, NULL, vict2, TO_ROOM);
    act("Wisps of color swirl around you, as you take the aspect of $N!", vict2, NULL, vict1, TO_CHAR);
    act("Wisps of color swirl around $n, as $e takes the aspect of $N!", vict2, NULL, vict1, TO_ROOM);

    af.where	 = TO_OAFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = level/2;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_DISGUISE;
    affect_to_char(vict1, &af);
    affect_to_char(vict2, &af);

    desc  = str_dup(vict1->description);
    fname = str_dup(vict1->name);
/*
    if (vict1->long_descr[0] != '\0')
	strcpy(lbuf, vict1->long_descr);
    else if (vict1->pcdata->extitle && vict1->pcdata->extitle[0] != '\0')
	sprintf(lbuf, "%s%s, %s is here.\n\r", vict1->name,
	    vict1->pcdata->title, vict1->pcdata->extitle);
    else
	sprintf(lbuf, "%s%s is here.\n\r", vict1->name, vict1->pcdata->title);
*/
    if (vict1->long_descr[0] != '\0')
	strcpy(lbuf, vict1->long_descr);
    else
	strcpy(lbuf, "");

    free_string(vict1->short_descr);
    vict1->short_descr = str_dup(vict2->name);

    if (vict1->orig_long[0] == '\0')
    {
	free_string(vict1->orig_long);
	vict1->orig_long = str_dup(vict1->long_descr);
    }
    free_string(vict1->long_descr);
    vict1->long_descr = str_dup((vict2->long_descr[0] != '\0') ? vict2->long_descr : "");

    if (vict1->orig_description[0] == '\0')
    {
	free_string(vict1->orig_description);
	vict1->orig_description = str_dup(vict1->description);
    }
    free_string(vict1->description);
    vict1->description = str_dup(vict2->description);

    setFakeName(*vict1, vict2->name);

    vict1->fake_race = vict2->race;
    vict2->fake_race = vict1->race;

    free_string(vict2->short_descr);
    vict2->short_descr = str_dup(fname);

    if (vict2->orig_long[0] == '\0')
    {
	free_string(vict2->orig_long);
	vict2->orig_long = str_dup(vict2->long_descr);
    }
    free_string(vict2->long_descr);
    vict2->long_descr = str_dup(lbuf);

    if (vict2->orig_description[0] == '\0')
    {
	free_string(vict2->orig_description);
	vict2->orig_description = str_dup(vict2->description);
    }
    free_string(vict2->description);
    vict2->description = desc;

    setFakeName(*vict2, fname);
    return TRUE;
}

bool spell_scatter( int sn, int level,CHAR_DATA *ch,void *vo,int target)
{
    int x = 0;
    ROOM_INDEX_DATA *to_room;
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (is_affected(ch, gsn_scatter))
    {
        send_to_char("You don't feel strong enough to scatter people again yet.\n\r", ch);
        return FALSE;
   }

    if (!ch->in_room)
        return FALSE;

    if (ch->in_room->room_flags & ROOM_NO_RECALL
    || ch->in_room->room_flags & ROOM_NOGATE
    || ch->in_room->room_flags & ROOM_INDOORS
    || ch->in_room->sector_type == SECT_INSIDE
    || ch->in_room->sector_type == SECT_UNDERGROUND)
    {
        send_to_char("You are unable to call out a whirlwind here.\n\r", ch);
        return FALSE;
    }

    act("$n summons a raging maelstrom!", ch, NULL, NULL, TO_ROOM);
    act("You summon a raging maelstrom!", ch, NULL, NULL, TO_CHAR);

    for (vch = ch->in_room->people ; vch != NULL ; vch = vch_next)
    {
        vch_next = vch->next_in_room;
        to_room = NULL;
        x = 0;

        if (is_safe_spell(ch, vch, TRUE)
        || ch == vch
        || IS_IMMORTAL(vch)
        || is_affected(vch, gsn_matrix)
        || (IS_NPC(vch) && vch->act & ACT_NOSUBDUE)
        || is_affected(vch, gsn_anchor))
            continue;

        while (x < 10)
        {
            if (to_room == NULL || IS_SET(to_room->room_flags, ROOM_INDOORS)
             || to_room->sector_type == SECT_INSIDE)
                to_room = get_random_room(vch);

            x++;
        }

        if (to_room == NULL || IS_SET(to_room->room_flags, ROOM_INDOORS)
         || to_room->sector_type == SECT_INSIDE)
            continue;

        act("$n is pulled into the maelstrom!", vch, NULL, NULL, TO_ROOM);
        act("You are pulled into the maelstrom!", vch, NULL, NULL, TO_CHAR);
        char_from_room(vch);
        char_to_room(vch, to_room);
        act("A fierce maelstrom appears for a moment, hurling $n out to the ground!", vch, NULL, NULL, TO_ROOM);
        do_look(vch, "auto");
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 12;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_join( ch, &af );

    return TRUE;
}

bool spell_shocking_grasp(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    static const int dam_each[] =
    {
         0,
         5,  5,  10,  12,  14,   16, 20, 25, 29, 33,
        36, 39, 39, 39, 40,     40, 41, 41, 42, 42,
        43, 43, 44, 44, 45,     45, 46, 46, 47, 47,
        48, 48, 49, 49, 50,     50, 51, 51, 52, 52,
        53, 53, 54, 54, 55,     55, 56, 56, 57, 57
    };
    int dam;

    level       = UMIN(level, static_cast<int>(sizeof(dam_each)/sizeof(dam_each[0])) - 1);
    level       = UMAX(0, level);
    dam         = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    if ( saves_spell( level, ch, victim,DAM_LIGHTNING) )
        dam /= 2;
    damage_old( ch, victim, dam, sn, DAM_LIGHTNING ,TRUE);
    return TRUE;
}

bool spell_spectralfist( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    if (ch->in_room == NULL | ch->in_room->sector_type == SECT_UNDERWATER)
    {
        send_to_char("You cannot call a spectral fist here.\n", ch);
        return true;
    }

    int diceSize;
    if (ch->in_room->sector_type != SECT_AIR && saves_spell(level, ch, victim, DAM_ILLUSION))
    {
        act("$n clenches $s fist, and a large floating hand appears in the air!", ch, NULL, victim, TO_ROOM);
        diceSize = 5;
    }
    else
    {
        act("$n forms a huge hand out of the air!", ch, NULL, victim, TO_ROOM);
        diceSize = 7;
    }

    int dam(dice(level, diceSize));
    if (saves_spell(level, ch, victim, DAM_BASH))
        dam /= 2;
    else
    {
        act("Your spectral hand smashes into $N!", ch, NULL, victim, TO_CHAR);
        act("You are smashed by $n's spectral hand!", ch, NULL, victim, TO_VICT);
        act("$n's spectral hand smashes $N!", ch, NULL, victim, TO_NOTVICT);
        WAIT_STATE(victim, (3 * PULSE_VIOLENCE) / 2);
    }

    damage_old(ch, victim, dam, sn, DAM_BASH, true);
    return true;
}

static void perform_suction(int level, CHAR_DATA & ch, ROOM_INDEX_DATA & sourceRoom, ROOM_INDEX_DATA & targetRoom, ROOM_INDEX_DATA * middleRoom)
{
    CHAR_DATA * victim_next;
    for (CHAR_DATA * victim(targetRoom.people); victim != NULL; victim = victim_next)
    {
        // Initial echo
        victim_next = victim->next_in_room;
        send_to_char("A huge gust of wind tears at you!\n", victim);

        // Check disqualifiers
        if (IS_NPC(victim) && (IS_SET(victim->act, ACT_NOSUBDUE) || IS_SET(victim->act, ACT_SENTINEL) || IS_SET(victim->nact, ACT_SHADE)))
            continue;
        
        if (IS_AFFECTED(victim, AFF_WIZI) || is_safe_spell(&ch, victim, true) || is_affected(victim, gsn_anchor) || saves_spell(level, &ch, victim, DAM_BASH))
            continue;

        // Pull the victim
        while (victim->in_room != &sourceRoom)
        {
            act("$n is pulled away by the huge gust of wind!", victim, NULL, NULL, TO_ROOM);
            act("The wind tears at you, pulling you away with unstoppable force!", victim, NULL, NULL, TO_CHAR);

            // Pull through the middle room, if present
            ROOM_INDEX_DATA * currTarget((middleRoom == NULL || middleRoom == victim->in_room) ? &sourceRoom : middleRoom);
            char_from_room(victim);
            char_to_room(victim, currTarget);

            do_look(victim, "auto");
            act("$n is pulled into the room by the gale forces!", victim, NULL, NULL, TO_ROOM);
        }
    }
}

bool spell_suction(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    // Sanity check
    ROOM_INDEX_DATA *origin(ch->in_room);
    if (origin == NULL)
    {
        send_to_char("You cannot create a suction here.\n", ch);
        return false;
    }
   
    // Check for direction 
    if (target_name[0] == '\0')
    {
        send_to_char("In which direction did you wish to create a suction?\n", ch);
        return false;
    }

    Direction::Value direction(Direction::ValueFor(target_name));
    if (direction == Direction::Max)
    {
        send_to_char("That's not a valid direction.\n", ch);
        return false;
    }

    // Check for a valid room
    ROOM_INDEX_DATA * nextRoom(Direction::Adjacent(*ch->in_room, direction, EX_CLOSED|EX_WALLED|EX_WEAVEWALL|EX_ICEWALL));
    if (nextRoom == NULL)
    {
        std::ostringstream mess;
        mess << "There is not enough space " << Direction::DirectionalNameFor(direction) << " to create a proper suction.\n";
        send_to_char(mess.str().c_str(), ch);
        return false;
    }

    // Potentially pick up a bonus room as well
    ROOM_INDEX_DATA * bonusRoom(NULL);
    if (number_bits(2) == 0)
        bonusRoom = Direction::Adjacent(*nextRoom, direction, EX_CLOSED|EX_WALLED|EX_WEAVEWALL|EX_ICEWALL);

    // Echoes
    act("$n calls forth incredible gusts of wind!", ch, NULL, NULL, TO_ROOM);
    act("You call forth incredible gusts of wind!", ch, NULL, NULL, TO_CHAR);

    // Perform the suction
    perform_suction(level, *ch, *ch->in_room, *nextRoom, NULL);
    if (bonusRoom != NULL)
        perform_suction(level, *ch, *ch->in_room, *bonusRoom, nextRoom);

    return true;
}

bool spell_tempest( int sn, int level,CHAR_DATA *ch,void *vo,int target)
{

    char buf[MAX_INPUT_LENGTH];
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;

	// brazen: this spell should not be castable underwater.
    if (ch->in_room == NULL || ch->in_room->sector_type == SECT_UNDERWATER)
    {
        send_to_char("You cannot summon howling winds here.\n\r", ch);
		return FALSE;
    }
    
    send_to_char( "Howling winds answer your call!\n\r", ch );
    act( "$n calls tempest winds to strike $s foes!", ch, NULL, NULL, TO_ROOM );

    std::pair<Direction::Value, int> windInfo(checkWind(ch));
    AFFECT_DATA * call(get_affect(ch, gsn_calluponwind));

    std::vector<DamageInfo> damage(2);
    damage[0].type = DAM_BASH;
    damage[1].type = DAM_SLASH;

    for ( vch = char_list; vch != NULL; vch = vch_next )
    {
        vch_next = vch->next;
        if ( vch->in_room == NULL )
            continue;

        if ((vch->in_room == ch->in_room) && (ch!=vch)
         && !(is_same_group(ch, vch)) && !is_safe_spell(ch,vch,TRUE))
        {
            if ((vch!=ch) && (!IS_IMMORTAL(vch) || (IS_IMMORTAL(vch) && can_see(ch,vch))))
            {
                if ((ch->fighting != vch) && !IS_NPC(vch) && vch->fighting!=ch)
                {
                    if (can_see(vch, ch))
                    {
                        sprintf( buf, "Help!  %s is attacking me!", PERS(ch, vch));
                        do_autoyell( vch, buf );
                    }
                    else
                    {
                        sprintf( buf, "Help! Someone is attacking me!");
                        do_autoyell( vch, buf );
                    }
                }
                
                damage[0].amount = dice(level / 8, 25);
                damage[1].amount = dice(level / 8, 25);
                for (size_t i(0); i < damage.size(); ++i)
                {
                    if (saves_spell(level, ch, vch, damage[i].type))
                        damage[i].amount /= 2;
                }

                damage_new(ch, vch, damage, sn, true);

                // Check for call upon wind
                if (windInfo.second > 0 && call != NULL)
                {
                    // Southerly winds parch and exhaust the target
                    if (!IS_NPC(vch) && call->modifier == Direction::South)
                    {
                        int moveCost(windInfo.second / (windInfo.first == Direction::South ? 1 : 4));
                        moveCost = number_range(1, moveCost * 2);
                        vch->move = UMAX(0, vch->move - moveCost);
                        vch->pcdata->condition[COND_THIRST] = 0;

                        send_to_char("The dry, blistering winds seems to draw the very moisture from you!\n", vch);
                        act("$N grows parched and exhausted from the tempest!", ch, NULL, vch, TO_CHAR);
                    }

                    // Easterly winds poison the target
                    if (call->modifier == Direction::East && !IS_AFFECTED(vch, AFF_POISON))
                    {
                        int saveLevel((level / 2) + (windInfo.second / (windInfo.first == Direction::East ? 2 : 4)));
                        if (!saves_spell(saveLevel, ch, vch, DAM_POISON))
                        {
                            AFFECT_DATA af = {0};
                            af.where     = TO_AFFECTS;
                            af.type      = gsn_poison;
                            af.level     = level;
                            af.duration  = level / 8;
                            af.location  = APPLY_STR;
                            af.modifier  = -2;
                            af.point     = ch;
                            af.bitvector = AFF_POISON;
                            affect_join(vch, &af);

                            send_to_char("The howling winds reek of decay, and you suddenly feel very sick.\n", vch);
                            act("$n looks very ill.", vch, NULL, NULL, TO_ROOM);
                        }
                    }
                }
            }
            continue;
        }
    }
    return TRUE;
}

bool spell_thunderclap( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

	if (IS_OAFFECTED(victim, AFF_DEAFEN))
	{
		send_to_char("They are already deaf.\n\r", ch);
		return FALSE;
	}

    if ((IS_NPC(victim) && IS_SET(victim->act, ACT_NOSUBDUE)) || (saves_spell(level, ch, victim, DAM_SOUND)))
    {
        send_to_char("They resist your spell.\n\r",ch);
        return TRUE;
    }
    
    act ("A thunderclap rings in your ears, and you can't hear anything!", victim, NULL, NULL, TO_CHAR);
    act ("$n is deafened by a thunderclap!", victim, NULL, NULL, TO_ROOM);

    AFFECT_DATA af = {0};
    af.where     = TO_OAFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/8;
    af.bitvector = AFF_DEAFEN;
    affect_to_char( victim, &af );
    return TRUE;
}

bool spell_ventriloquate( int sn, int level, CHAR_DATA *ch,void *vo,int target)
{
    char buf1[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char speaker[MAX_INPUT_LENGTH];
    CHAR_DATA *vch;

    target_name = one_argument( target_name, speaker );

    sprintf( buf1, "{Y%s says, '%s'{x\n\r",          capitalize(speaker), target_name );
    sprintf( buf2, "Someone makes %s say, '%s'\n\r", capitalize(speaker), target_name );
    buf1[0] = UPPER(buf1[0]);

    for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room )
    {
        if (!is_exact_name( speaker, vch->name) && IS_AWAKE(vch))
            send_to_char( (!IS_NPC(vch) && BIT_GET(vch->pcdata->traits, TRAIT_CYNIC) && number_bits(2) != 0) || saves_spell(level,ch,vch,DAM_OTHER) ? buf2 : buf1, vch );
    }

    return TRUE;
}

bool spell_wallofair( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if (!ch->in_room)
	return FALSE;

    if (is_affected(ch, sn))
    {
	send_to_char("You are not ready to summon another wall of air yet.\n\r", ch);
	return FALSE;
    }

    if (room_is_affected(ch->in_room, sn))
    {
	send_to_char("This room is already occupied by a wall of air.\n\r", ch);
	return FALSE;
    }

    af.where	 = TO_AFFECTS;
    af.type	 = sn;
    af.level	 = level;
    af.duration  = level/10;
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = 0;
    affect_to_room(ch->in_room, &af);

    af.duration  = level/2;
    affect_to_char(ch, &af);

    act("$n conjures a massive wall of air, blowing fiercely about the area!", ch, NULL, NULL, TO_ROOM);
    send_to_char("You conjure a massive wall of air, blowing fiercely about the area!\n\r", ch);

    return TRUE;
}


bool spell_windbind( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( is_affected( victim, sn ) )
    {
        if (victim == ch)
          send_to_char("You are already bound with the wind.\n\r",ch);
        else
          act("$N is already bound by the wind.", ch,NULL,victim,TO_CHAR);
        return FALSE;
    }

    if (ch->in_room == NULL || ch->in_room->sector_type == SECT_UNDERWATER)
    {
        act("There is no wind here with which to bind $M!", ch, NULL, victim, TO_CHAR);
        return false;
    }

    std::pair<Direction::Value, int> windInfo(checkWind(ch));
    AFFECT_DATA * call(get_affect(ch, gsn_calluponwind));
    if (windInfo.second > 0 && call != NULL && call->modifier == Direction::North)
    {
        ++level;
        level += (windInfo.second / 50);
        if (windInfo.first == Direction::North)
            level += 12;
    }

    if (saves_spell(level,ch,victim,DAM_OTHER) ||  IS_SET(victim->imm_flags,IMM_MAGIC))
    {
        if (victim != ch) act("The wind grabs at $N, but $E slips its bonds.", ch, NULL, victim, TO_CHAR);
        send_to_char("You feel the wind grab at you, but you resist.\n\r",victim);
        return TRUE;
    }

    if (IS_AFFECTED(victim,AFF_HASTE))
    {
        if (!check_dispel(level,victim,skill_lookup("haste")))
        {
            if (victim != ch) act("The wind grabs at $N, but $E is moving too quickly.", ch, NULL, victim, TO_CHAR);
            send_to_char("You feel the wind grab at you, but you are moving too quickly.\n\r",victim);
            return TRUE;
        }
        act("$n is seized by tendrils of air, but slips away with $s unnatural speed.",victim,NULL,NULL,TO_ROOM);
        return TRUE;
    }

    if (check_spirit_of_freedom(victim))
    {
    	send_to_char("You feel the spirit of freedom surge within you.\n\r", victim);
	    act("$n glows brightly for a moment.", victim, NULL, NULL, TO_ROOM);
    	return TRUE;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/2;
    af.location  = APPLY_DEX;
    af.modifier  = -1 - (level >= 18) - (level >= 25) - (level >= 32);
    af.bitvector = 0;
    affect_to_char( victim, &af );
    send_to_char( "You feel tendrils of air seize you!\n\r", victim );
    act("$n is seized by tendrils of air!",victim,NULL,NULL,TO_ROOM);
    return TRUE;
}

bool spell_windblast( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    int direction, i;
    EXIT_DATA *pexit;
    CHAR_DATA *victim, *vch, *vch_next;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_STRING_LENGTH];

    target_name = one_argument(target_name, arg);

    if ((victim = get_char_room(ch, arg)) == NULL)
    {
        send_to_char("You don't see them here.\n\r", ch);
        return FALSE;
    }

    if (is_safe_spell(ch, victim, FALSE))
    {
        act("The gods protect $N.", ch, NULL, victim, TO_NOTVICT);
        act("The gods protect $N.", ch, NULL, victim, TO_CHAR);
        act("The gods protect you.", ch, NULL, victim, TO_VICT);
        return FALSE;
    }

    if (target_name[0] == '\0')
    {
	i = 0;
	do
	{
		i++;
		direction = number_range(1,6);
	}while(ch->in_room->exit[direction] == NULL && i < 30);
    }
    else
    {
    if ( !str_prefix( target_name, "north" ) ) direction = 0;
        else if (!str_prefix (target_name, "east") ) direction = 1;
        else if (!str_prefix (target_name, "south") ) direction = 2;
        else if (!str_prefix (target_name, "west") ) direction = 3;
        else if (!str_prefix (target_name, "up") ) direction = 4;
        else if (!str_prefix (target_name, "down") ) direction = 5;
        else
    {
        send_to_char("That's not a valid direction.\n\r", ch);
        return FALSE;
    }
    }
    if ((pexit = ch->in_room->exit[direction]) == NULL)
    {
        send_to_char("There is nothing in that direction!\n\r", ch);
        return FALSE;
    }

    send_to_char("You conjure a strong blast of wind!\n\r", ch);
    act("$n conjures a strong blast of wind!", ch, NULL, NULL, TO_ROOM);

    for (vch = victim->in_room->people; vch; vch = vch_next)
    {
	vch_next = vch->next_in_room;

	if (is_same_group(victim, vch))
	{
            if (saves_spell(level+3, ch,vch, DAM_BASH))
            {
                if (can_see(ch, vch) && IS_AWAKE(vch))
                {
                    sprintf(buf, "Help! %s is attacking me!", PERS(ch, victim));
            	    do_autoyell(victim, buf);
                }
                else
                {
                    sprintf(buf, "Help! Someone is attacking me!");
                    do_autoyell(victim, buf);
                }

                if (IS_AWAKE(vch))
            	    one_hit(vch, ch, TYPE_UNDEFINED, HIT_PRIMARY, false);

                continue;
            }

            if (IS_NPC(ch)
             && (IS_SET(vch->act, ACT_NOSUBDUE)
              || IS_SET(vch->act, ACT_SENTINEL)))
            {
        	if (IS_AWAKE(vch))
                    one_hit(vch, ch, TYPE_UNDEFINED, HIT_PRIMARY, false);
        	continue;
            }

            act("The winds hammer at $n!", vch, NULL, NULL, TO_ROOM);
            act("The winds hammer at you!", vch, NULL, NULL, TO_CHAR);
	    if (!is_affected(vch, gsn_anchor))
                move_char(vch, direction, FALSE);
	}
    }
    return TRUE;
}

static bool isValidWindwalkRoom(const ROOM_INDEX_DATA & room)
{
    if  (IS_SET(room.room_flags, ROOM_NOGATE)) return false;
    if  (room.area->area_flags & AREA_UNCOMPLETE) return false;
    if  (IS_SET(room.room_flags, ROOM_NOMAGIC)) return false;
    if  (IS_SET(room.room_flags, ROOM_SAFE)) return false;
    if  (IS_SET(room.room_flags, ROOM_PRIVATE)) return false;
    if  (IS_SET(room.room_flags, ROOM_SOLITARY)) return false;
    if  (IS_SET(room.room_flags, ROOM_NO_RECALL)) return false;
    if	(room.sector_type == SECT_INSIDE) return false;
    if	(room.sector_type == SECT_UNDERGROUND) return false;
    if	(room.sector_type == SECT_UNDERWATER) return false;
    if  (IS_SET(room.room_flags, ROOM_INDOORS)) return false;
    return true; 
}

bool spell_windwalk( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    ROOM_INDEX_DATA *to_room;
    EXIT_DATA *pexit;
    CHAR_DATA *victim;
    bool gate_pet;
    bool gate_familiar;

    if (ch->in_room->sector_type == SECT_INSIDE
     || ch->in_room->sector_type == SECT_UNDERGROUND
     || ch->in_room->sector_type == SECT_UNDERWATER
     || IS_SET(ch->in_room->room_flags, ROOM_INDOORS))
    {
	send_to_char("The winds cannot reach you here!\n\r", ch);
	return FALSE;
    }

    if ( ( victim = get_char_world( ch, target_name ) ) == NULL
    ||   victim == ch
    ||   victim->in_room == NULL
    ||   is_affected(ch, gsn_matrix)
    ||   !can_see_room(ch,victim->in_room)
    ||   !isValidWindwalkRoom(*victim->in_room)
    ||   IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
    ||   (!IS_NPC(victim) && victim->level > LEVEL_HERO)  /* NOT trust */
    ||   (IS_NPC(victim) && IS_SET(victim->imm_flags,IMM_SUMMON))
    ||   saves_spell( level, ch, victim,DAM_OTHER)
    ||   (ch->move == 0) )
    {
        send_to_char( "The winds were unable to carry you to your target.\n\r", ch );
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

    act("$n shifts into insubstantial vapor, and vanishes on the wind.",ch,NULL,NULL,TO_ROOM);
    send_to_char("You shift into insubstantial vapor, and vanish on the wind.\n\r",ch);

    ch->move /= 2;

    char_from_room(ch);

    to_room = victim->in_room;

    for (pexit = to_room->exit[number_range(0, 5)]; pexit != NULL; )
    {
        if (pexit == NULL || number_bits(1) != 0 || !isValidWindwalkRoom(*pexit->u1.to_room))
            break;
        
        to_room = pexit->u1.to_room;
        pexit = to_room->exit[number_range(0, 5)];
    }

    char_to_room(ch,to_room);

    act("The world grows fuzzy, and forms again as you find yourself in a new location...", ch, NULL, NULL, TO_CHAR);
    act("$n forms out of insubstantial vapor before your eyes!", ch, NULL, NULL, TO_ROOM);
    do_look(ch,"auto");

    if (gate_familiar)
    {
    act("$n shifts into insubstantial vapor, and vanishes on the wind.",ch->familiar,NULL,NULL,TO_ROOM);
        send_to_char("You shift into insubstantial vapor, and vanish on the wind.\n\r",ch->familiar);
        char_from_room(ch->familiar);
        char_to_room(ch->familiar,to_room);
        act("$n forms out of vapor before your eyes!", ch->familiar, NULL, NULL, TO_ROOM);
        do_look(ch->familiar,"auto");
    }

    if (gate_pet)
    {
    act("$n shifts into insubstantial vapor, and vanishes on the wind.",ch->pet,NULL,NULL,TO_ROOM);
        send_to_char("You shift into insubstantial vapor, and vanish on the wind.\n\r",ch->pet);
        char_from_room(ch->pet);
        char_to_room(ch->pet,to_room);
        act("$n forms out of vapor before your eyes!", ch->pet, NULL, NULL, TO_ROOM);
        do_look(ch->pet,"auto");
    }
    return TRUE;
}

bool spell_windwall(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
    CHAR_DATA *victim = ch;
    AFFECT_DATA af;
af.valid = TRUE;
af.point = NULL;

    if ( is_affected(victim, gsn_windwall))
    {
        send_to_char("You are already guarded by a wind wall.\n\r",ch);
        return FALSE;
    }

    if(ch->in_room->sector_type == SECT_UNDERWATER)
    {
        send_to_char("The wind cannot protect you underwater.\n\r",ch);
	return FALSE;
    }

    act ("You summon a wall of wind to protect you!", ch, NULL, NULL, TO_CHAR);
    act ("$n summons a wall of wind to protect $m!", ch, NULL, NULL, TO_ROOM);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/9;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char( victim, &af );
    return TRUE;
}

bool spell_windsurge(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;
    int num_exits = 0, door;

    if (is_safe_spell(ch, victim, FALSE) || ch->in_room == NULL)
        return FALSE;
    
    if (ch->in_room->sector_type == SECT_UNDERWATER)
    {
        send_to_char("You cannot summon winds underwater.\n\r", ch);
		return FALSE;
    }

    for (door = 0; door < MAX_DIR; door++)
        if ((ch->in_room->exit[door]) != NULL
	  && !IS_SET(ch->in_room->exit[door]->exit_info, EX_CLOSED)
          && !IS_SET(ch->in_room->exit[door]->exit_info, EX_SECRET)
          && !IS_SET(ch->in_room->exit[door]->exit_info, EX_ICEWALL)
	  && !IS_SET(ch->in_room->exit[door]->exit_info, EX_WALLED))
	    num_exits++;

    if (num_exits == 0)
    {
        send_to_char("You cannot channel a surge of wind here.\n\r", ch);
		return FALSE;
    }

// Okay, passed autofails. Now to calculate damage.

    act("You channel a surge of wind at $N!", ch, NULL, victim, TO_CHAR);
    act("$n channels a surge of wind at $N!", ch, NULL, victim, TO_VICTROOM);
    act("$n channels a surge of wind at you!", ch, NULL, victim, TO_VICT);

    if (num_exits >= 4) 
	dam = dice(level, 3) + level * 2;
    else
	dam = dice(level, 5);

// Choose damage type, reusing doors and num_exits
    
    door = DAM_BASH;
    num_exits = number_percent();
    if (num_exits > 50) 
        door = DAM_SLASH;
    else if (num_exits > 15) 
        door = DAM_PIERCE;

    if (saves_spell(level, ch, victim, door))
        dam /= 2;

    damage(ch, victim, dam, sn, door, TRUE);
    return TRUE;
}

void do_thunderstrike(CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj;
    CHAR_DATA *victim;
    int chance;
    bool in_form = FALSE;

    if ((chance = get_skill(ch, gsn_thunderstrike)) ==0)
    {
	send_to_char("Huh?\n\r", ch);
	return;
    }

    if ( ( victim = ch->fighting) == NULL)
    {
	send_to_char("You aren't fighting anyone.\n\r", ch);
	return;
    }
    
    if ( ( obj = get_eq_char(ch,WEAR_WIELD)) == NULL)
    {
	send_to_char("You must wield a weapon to perform a thunder strike.\n\r",ch);
	return;
    }

    if (obj->value[0] != WEAPON_SWORD)
    {
	send_to_char("You must wield a sword to perform a thunder strike.\n\r",ch);
	return;
    }

    if (ch->mana - skill_table[gsn_thunderstrike].min_mana < 0)
    {
	send_to_char("You are too tired to perform a thunder strike.\n\r",ch);
	return;
    }

    if (!check_reach(ch, victim, REACH_NORMAL, PULSE_VIOLENCE))
	return;
    AFFECT_DATA *paf;
    chance *= .75;

    for ( paf = ch->affected; paf; paf=paf->next)
	if (paf->location == APPLY_FORM)
	{
	    in_form = TRUE;
	    break;
	}
    if (paf)
    {
	chance *= 1.2;
	WAIT_STATE(ch, skill_table[gsn_thunderstrike].beats);
    }
    else
	WAIT_STATE(ch, skill_table[gsn_thunderstrike].beats * 2);

    expend_mana(ch, skill_table[gsn_thunderstrike].min_mana);

    int thac0, thac0_00, thac0_32, victim_ac, dam, diceroll, skill;
    int dam_type = DAM_SOUND;
    if (is_an_avatar(ch))
	    dam_type = TYPE_HIT+DAM_HOLY;
    if(obj && obj_is_affected(obj,gsn_heatsink))
	damage_old(ch, ch, number_range(4,8), gsn_heatsink,DAM_COLD,TRUE);
    wrathkyana_combat_effect(ch, victim);
    skill = get_skill(ch,gsn_thunderstrike);

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
    thac0 = interpolate(ch->level, thac0_00,thac0_32);
    if (thac0 < 0)
	thac0 = thac0 / 2;
    if (thac0 < -5)
	thac0 = -5 + (thac0+5)/2;
    thac0 -= GET_HITROLL(ch) * skill/100;
    thac0 += 5 * (100 - skill) / 100;
    victim_ac = GET_AC(victim,AC_EXOTIC)/10;
    
    if (victim_ac < -15)
	victim_ac = (victim_ac + 15) / 5 - 15;
    if (!can_see(ch,victim) && !is_blindfighter(ch, TRUE))
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

    while ((diceroll = number_bits(5) ) >= 20)
	;
    if ((diceroll == 0 || (diceroll != 19 && diceroll < thac0 - victim_ac)))
    {
	/* Miss */
	damage(ch,victim,0,gsn_thunderstrike,dam_type,TRUE);
	check_improve(ch,victim,gsn_thunderstrike,FALSE,1);
	return;
    }
    /* Hit */
    if (obj->pIndexData->new_format)
	dam = dice(obj->value[1], obj->value[2]);
    else
	dam = number_range(obj->value[1], obj->value[2]);
    dam *= 2;
    dam = (dam * skill)/100;
    dam += check_extra_damage(ch,dam,obj);
    if (!IS_AWAKE(victim))
	dam *= 2;
    dam += (GET_DAMROLL(ch) /3) * UMIN(100,skill) / 100;
    if (dam <= 0)
	dam = 1;
    check_improve(ch,victim,gsn_thunderstrike,TRUE,1);

    if (number_percent() < skill/3 && !is_affected(victim,gsn_thunderstrike))
    {
	if (in_form)
	{
	    act("You work a calculated strike into your form, deafening $N with a resounding boom!",ch,NULL,victim,TO_CHAR);
	    act("$n works a calculated strike into $s form, deafening $N with a resounding boom!",ch,NULL,victim,TO_NOTVICT);
	    act("$n works a calculated strike into $s form, deafening you with a resounding boom!",ch,NULL,victim,TO_VICT);
	}
	else
	{
	    act("You strike with precision, deafening $N with a resounding boom!",ch,NULL,victim,TO_CHAR);
	    act("$n strikes with precision, deafening $N with a resounding boom!",ch,NULL,victim,TO_NOTVICT);
	    act("$n strikes with precision, deafening you with a resounding boom.",ch,NULL,victim,TO_VICT);
	}
	AFFECT_DATA af;
	af.valid = TRUE;
	af.point = NULL;
	af.where = TO_OAFFECTS;
	af.type = gsn_thunderstrike;
	af.level = ch->level;
	af.location = 0;
	af.modifier = 0;
	af.duration = 1;
	af.bitvector = AFF_DEAFEN;
	affect_to_char(victim,&af);
    }
    else
	if (in_form)
	{
	    act("You work a calculated strike into your form, and the sound of thunder fills the air.",ch,NULL,victim,TO_CHAR);
	    act("$n works a calculated strike into $s form, and the sound of thunder fills the air.",ch,NULL,victim,TO_ROOM);
	}
	else
	{
	    act("You strike with precision, and the sound of thunder fills the air.",ch,NULL,victim,TO_CHAR);
	    act("$n strikes with precision, and the sound of thunder fills the air.",ch,NULL,victim,TO_ROOM);
	}
    damage_old(ch,victim,dam,gsn_thunderstrike,dam_type,TRUE);
    check_killer(ch,victim);
    return;
}
